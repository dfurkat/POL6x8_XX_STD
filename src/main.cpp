#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include "modbus_handler.h"
#include "mqtt_handler.h"
#include "sim800_handler.h"
#include "config_manager.h"
#include "register_mapper.h"

// Hardware pins
#define MODBUS_RX_PIN D5
#define MODBUS_TX_PIN D6
#define MODBUS_DE_RE_PIN D7
#define SIM800_RX_PIN D1
#define SIM800_TX_PIN D2
#define SIM800_PWR_PIN D3
#define SIM800_RST_PIN D4
#define STATUS_LED D0

// Objects
SoftwareSerial modbusSerial(MODBUS_RX_PIN, MODBUS_TX_PIN);
SoftwareSerial sim800Serial(SIM800_RX_PIN, SIM800_TX_PIN);

ModbusHandler modbusHandler;
SIM800Handler sim800Handler;
MQTTHandler mqttHandler;
ConfigManager configManager;
RegisterMapper registerMapper;

// Timing
unsigned long lastModbusPoll = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastStatusBlink = 0;
unsigned long connectionAttemptTime = 0;
const unsigned long CONNECTION_RETRY_INTERVAL = 30000; // 30 seconds

// System state
enum SystemState
{
    STATE_INIT,
    STATE_CONNECTING_CELLULAR,
    STATE_CONNECTING_MQTT,
    STATE_RUNNING,
    STATE_ERROR
};

SystemState systemState = STATE_INIT;
bool ledState = false;

// Configuration
SystemConfig config;

// Forward declarations
void handleConnectingCellular(unsigned long currentMillis);
void handleConnectingMQTT(unsigned long currentMillis);
void handleRunningState(unsigned long currentMillis);
void handleErrorState(unsigned long currentMillis);
void publishIndividualRegisters();
void publishSystemStatus();

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n====================================");
    Serial.println("   MODBUS to MQTT Gateway v1.0");
    Serial.println("====================================");

    // Initialize pins
    pinMode(MODBUS_DE_RE_PIN, OUTPUT);
    pinMode(SIM800_PWR_PIN, OUTPUT);
    pinMode(SIM800_RST_PIN, OUTPUT);
    pinMode(STATUS_LED, OUTPUT);

    digitalWrite(MODBUS_DE_RE_PIN, LOW);
    digitalWrite(SIM800_PWR_PIN, HIGH);
    digitalWrite(SIM800_RST_PIN, HIGH);
    digitalWrite(STATUS_LED, LOW);

    // Load configuration
    Serial.println("Loading configuration...");
    if (!configManager.loadConfig())
    {
        Serial.println("Failed to load config, creating default...");
        config = configManager.getConfig();
        configManager.saveConfig();
    }
    else
    {
        config = configManager.getConfig();
        Serial.println("Configuration loaded successfully");
    }

    // Initialize serial ports
    modbusSerial.begin(config.modbusBaudRate);
    sim800Serial.begin(9600);

    // Initialize handlers
    modbusHandler.begin(modbusSerial, MODBUS_DE_RE_PIN, config.modbusSlaveId);
    sim800Handler.begin(sim800Serial, SIM800_PWR_PIN, SIM800_RST_PIN);
    registerMapper.begin();

    Serial.println("System initialized");
    Serial.print("Modbus slave ID: ");
    Serial.println(config.modbusSlaveId);
    Serial.print("Modbus baud rate: ");
    Serial.println(config.modbusBaudRate);

    systemState = STATE_CONNECTING_CELLULAR;
    connectionAttemptTime = millis();
}

void loop()
{
    unsigned long currentMillis = millis();

    // Status LED blinking based on state
    if (currentMillis - lastStatusBlink > 500)
    {
        lastStatusBlink = currentMillis;
        ledState = !ledState;

        switch (systemState)
        {
        case STATE_INIT:
            digitalWrite(STATUS_LED, LOW);
            break;
        case STATE_CONNECTING_CELLULAR:
            digitalWrite(STATUS_LED, ledState ? HIGH : LOW); // Slow blink
            break;
        case STATE_CONNECTING_MQTT:
            digitalWrite(STATUS_LED, ledState ? HIGH : LOW); // Slow blink
            break;
        case STATE_RUNNING:
            digitalWrite(STATUS_LED, HIGH); // Solid on
            break;
        case STATE_ERROR:
            digitalWrite(STATUS_LED, (currentMillis % 200) < 100 ? HIGH : LOW); // Fast blink
            break;
        }
    }

    // State machine
    switch (systemState)
    {
    case STATE_CONNECTING_CELLULAR:
        handleConnectingCellular(currentMillis);
        break;

    case STATE_CONNECTING_MQTT:
        handleConnectingMQTT(currentMillis);
        break;

    case STATE_RUNNING:
        handleRunningState(currentMillis);
        break;

    case STATE_ERROR:
        handleErrorState(currentMillis);
        break;

    default:
        break;
    }
}

void handleConnectingCellular(unsigned long currentMillis)
{
    static unsigned long lastAttempt = 0;

    if (currentMillis - lastAttempt > 2000)
    {
        lastAttempt = currentMillis;

        Serial.print("Connecting to cellular network (APN: ");
        Serial.print(config.apn);
        Serial.println(")...");

        if (sim800Handler.connectToNetwork(config.apn, config.gprsUser, config.gprsPass))
        {
            Serial.println("✓ Cellular network connected!");
            systemState = STATE_CONNECTING_MQTT;
            connectionAttemptTime = currentMillis;
        }
        else
        {
            Serial.println("✗ Failed to connect to cellular network");

            // Check if we should retry
            if (currentMillis - connectionAttemptTime > CONNECTION_RETRY_INTERVAL)
            {
                Serial.println("Resetting SIM800...");
                sim800Handler.reset();
                connectionAttemptTime = currentMillis;
            }
        }
    }
}

void handleConnectingMQTT(unsigned long currentMillis)
{
    static unsigned long lastAttempt = 0;

    if (currentMillis - lastAttempt > 3000)
    {
        lastAttempt = currentMillis;

        Serial.println("Configuring MQTT...");
        sim800Handler.configureMQTT(config.mqttServer, config.mqttPort);

        Serial.print("Connecting to MQTT server ");
        Serial.print(config.mqttServer);
        Serial.print(":");
        Serial.print(config.mqttPort);
        Serial.println("...");

        String clientId = "modbus-gateway-" + String(ESP.getChipId(), HEX);
        if (sim800Handler.connectMQTT(clientId.c_str(), config.mqttUser, config.mqttPassword))
        {
            Serial.println("✓ MQTT connected!");

            // Initialize MQTT handler
            mqttHandler.begin(&sim800Handler);
            mqttHandler.subscribeToCommands();

            systemState = STATE_RUNNING;
            Serial.println("✓ System is now RUNNING");
            Serial.println("===============================");
        }
        else
        {
            Serial.println("✗ Failed to connect to MQTT");

            if (currentMillis - connectionAttemptTime > CONNECTION_RETRY_INTERVAL)
            {
                systemState = STATE_CONNECTING_CELLULAR;
                connectionAttemptTime = currentMillis;
            }
        }
    }
}

void handleRunningState(unsigned long currentMillis)
{
    // Maintain MQTT connection
    mqttHandler.loop();

    // Check connection status
    if (!sim800Handler.isNetworkConnected())
    {
        Serial.println("Lost cellular connection!");
        systemState = STATE_CONNECTING_CELLULAR;
        connectionAttemptTime = currentMillis;
        return;
    }

    if (!sim800Handler.isMQTTConnected())
    {
        Serial.println("Lost MQTT connection!");
        systemState = STATE_CONNECTING_MQTT;
        connectionAttemptTime = currentMillis;
        return;
    }

    // Process MQTT messages (commands)
    mqttHandler.processMessages();

    // Handle MQTT commands
    if (mqttHandler.hasPendingCommands())
    {
        MQTTCommand cmd = mqttHandler.getNextCommand();

        Serial.print("Executing command: ");
        Serial.print(cmd.topic);
        Serial.print(" -> Address: 0x");
        Serial.print(cmd.address, HEX);
        Serial.print(", Value: ");
        Serial.println(cmd.value);

        // Get register info
        ModbusRegister regInfo = registerMapper.getRegisterInfo(cmd.address);

        // Validate value range
        if (cmd.value < regInfo.min_value || cmd.value > regInfo.max_value)
        {
            Serial.print("Value out of range! Min: ");
            Serial.print(regInfo.min_value);
            Serial.print(", Max: ");
            Serial.println(regInfo.max_value);
            mqttHandler.sendCommandResponse(cmd, false, "Value out of range");
        }
        else
        {
            // Execute Modbus write
            bool writeSuccess = modbusHandler.writeRegister(
                cmd.address, cmd.value, regInfo.type);

            // Send response
            mqttHandler.sendCommandResponse(cmd, writeSuccess,
                                            writeSuccess ? "Success" : "Write failed");

            if (writeSuccess)
            {
                // Immediately read back to verify
                modbusHandler.pollSingleRegister(cmd.address, regInfo.type);
            }
        }
    }

    // Read Modbus registers periodically
    if (currentMillis - lastModbusPoll >= config.modbusPollInterval)
    {
        lastModbusPoll = currentMillis;

        // Poll all register groups
        bool success = modbusHandler.pollAllRegisters();

        if (success)
        {
            // Get latest data as JSON
            DynamicJsonDocument doc(4096);
            if (modbusHandler.getDataAsJson(doc))
            {
                // Add system info
                doc["_system"]["timestamp"] = currentMillis;
                doc["_system"]["free_heap"] = ESP.getFreeHeap();
                doc["_system"]["uptime"] = millis() / 1000;
                doc["_system"]["rssi"] = "N/A"; // Not applicable for cellular

                // Publish to MQTT
                mqttHandler.publishData(doc);

                // Also publish to individual topics if configured
                publishIndividualRegisters();
            }
        }
        else
        {
            Serial.println("Modbus poll failed!");
        }
    }

    // Periodic MQTT status update
    if (currentMillis - lastMqttPublish >= config.mqttPublishInterval)
    {
        lastMqttPublish = currentMillis;
        publishSystemStatus();
    }
}

void handleErrorState(unsigned long currentMillis)
{
    static unsigned long lastErrorReport = 0;

    if (currentMillis - lastErrorReport > 10000)
    {
        lastErrorReport = currentMillis;
        Serial.println("System in ERROR state. Attempting recovery...");

        // Try to reset and restart
        sim800Handler.reset();
        systemState = STATE_CONNECTING_CELLULAR;
        connectionAttemptTime = currentMillis;
    }
}

void publishIndividualRegisters()
{
    // Publish each register to its own topic for easier subscription
    std::vector<RegisterValue> values = modbusHandler.getAllRegisterValues();

    for (const auto &regValue : values)
    {
        ModbusRegister info = registerMapper.getRegisterInfo(regValue.address);

        if (info.name)
        {
            // Create JSON for individual register
            DynamicJsonDocument doc(256);
            doc["value"] = regValue.value;
            doc["timestamp"] = regValue.timestamp;
            doc["address"] = regValue.address;
            doc["type"] = info.type;
            doc["unit"] = info.unit;

            String jsonStr;
            serializeJson(doc, jsonStr);

            // Publish to topic like: modbus/registers/heating1_valveA_signal
            String topic = "modbus/registers/" + String(info.name);
            mqttHandler.publishRaw(topic.c_str(), jsonStr.c_str());
        }
    }
}

void publishSystemStatus()
{
    DynamicJsonDocument doc(512);

    doc["status"] = "running";
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["modbus_polls"] = modbusHandler.getPollCount();
    doc["mqtt_messages"] = mqttHandler.getMessageCount();
    doc["cellular_strength"] = sim800Handler.getSignalStrength();

    String jsonStr;
    serializeJson(doc, jsonStr);

    mqttHandler.publishStatus(jsonStr.c_str());
}