#include <Arduino.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include "config.h"
#include "modbus_registers.h"

// Forward declarations
void setup();
void loop();
void readAllRegisters();
void publishToMQTT();
void handleMQTTCommand(char *topic, byte *payload, unsigned int length);
void watchdogCheck();

// Global objects
Ticker modbusTicker;
Ticker mqttTicker;
Ticker watchdogTicker;
unsigned long lastWatchdogReset = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== Climatix Controller Monitor ===");

  // Initialize subsystems
  initWiFi();
  initMQTT();
  initModbus();
  initSIM800(); // Optional for cellular backup

  // Setup timers
  modbusTicker.attach_ms(MODBUS_POLL_INTERVAL, readAllRegisters);
  mqttTicker.attach_ms(MQTT_PUBLISH_INTERVAL, publishToMQTT);
  watchdogTicker.attach_ms(WATCHDOG_TIMEOUT, watchdogCheck);

  // Initial read
  readAllRegisters();

  Serial.println("System initialized successfully");
}

void loop()
{
  // Handle MQTT reconnection and messages
  if (!mqttConnected())
  {
    reconnectMQTT();
  }
  mqttLoop();

  // Reset watchdog
  lastWatchdogReset = millis();

  // Handle other tasks
  handleSIM800(); // If using cellular

  delay(100);
}

void readAllRegisters()
{
  static int readPhase = 0;

  switch (readPhase)
  {
  case 0:
    readHoldingRegisters(ADDR_GENERAL_START, ADDR_GENERAL_END);
    break;
  case 1:
    readHoldingRegisters(ADDR_HEATING1_START, ADDR_HEATING1_END);
    break;
  case 2:
    readHoldingRegisters(ADDR_HEATING2_START, ADDR_HEATING2_END);
    break;
  case 3:
    readHoldingRegisters(ADDR_HEATING3_START, ADDR_HEATING3_END);
    break;
  case 4:
    readInputRegisters(REG_OUTSIDE_TEMP, REG_DHW2_RETURN_TEMP);
    break;
  case 5:
    readCoils(COIL_ALARM_CONFIRM, 10); // First 10 coils
    break;
  }

  readPhase = (readPhase + 1) % 6;
}

void publishToMQTT()
{
  if (!mqttConnected())
    return;

  StaticJsonDocument<1024> doc;
  JsonObject telemetry = doc.createNestedObject("telemetry");

  // Add temperature readings
  telemetry["outside_temp"] = getRegisterValue(REG_OUTSIDE_TEMP, true);
  telemetry["heating1_supply"] = getRegisterValue(REG_HEATING1_SUPPLY_TEMP, true);
  telemetry["heating1_return"] = getRegisterValue(REG_HEATING1_RETURN_TEMP, true);
  telemetry["dhw1_supply"] = getRegisterValue(REG_DHW1_SUPPLY_TEMP, true);

  // Add system status
  telemetry["heating1_mode"] = getRegisterValue(REG_HEATING1_MODE, false);
  telemetry["system_hours"] = getRegisterValue(REG_SYSTEM_HOURS, false);

  // Add alarm status
  telemetry["alarm"] = getDiscreteInput(INPUT_ALARM_STATUS);

  // Convert to JSON string
  char buffer[512];
  size_t n = serializeJson(doc, buffer);

  // Publish to MQTT
  mqttPublish(TOPIC_TELEMETRY, buffer, n);

  // Publish system status
  mqttPublish(TOPIC_STATUS, "online", false);
}

void handleMQTTCommand(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("MQTT Command: %s\n", topic);

  // Parse JSON payload
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error)
  {
    Serial.printf("JSON parse error: %s\n", error.c_str());
    return;
  }

  // Check which command
  if (strstr(topic, "set_mode"))
  {
    int circuit = doc["circuit"]; // 1, 2, 3 for heating, 4, 5 for DHW
    int mode = doc["mode"];       // 0-3 as per MODBUS docs

    uint16_t regAddress;
    if (circuit == 1)
      regAddress = REG_HEATING1_MODE;
    else if (circuit == 2)
      regAddress = REG_HEATING1_MODE + 80; // Approx offset
    else if (circuit == 4)
      regAddress = REG_DHW1_MODE;

    writeSingleRegister(regAddress, mode);
  }
  else if (strstr(topic, "set_setpoint"))
  {
    int circuit = doc["circuit"];
    float setpoint = doc["setpoint"];

    // Convert to MODBUS register value (usually temperature * 10)
    uint16_t value = setpoint * 10;
    writeSingleRegister(REG_HEATING1_SETPOINT + (circuit - 1) * 80, value);
  }
  else if (strstr(topic, "reset_alarm"))
  {
    writeSingleCoil(COIL_ALARM_CONFIRM, 1);
  }
  else if (strstr(topic, "save_settings"))
  {
    writeSingleCoil(COIL_SAVE_SETTINGS, 1);
  }
  else if (strstr(topic, "load_settings"))
  {
    writeSingleCoil(COIL_LOAD_SETTINGS, 1);
  }

  // Acknowledge command
  mqttPublish("climatix/01/ack", "{\"status\":\"processed\"}", false);
}

void watchdogCheck()
{
  if (millis() - lastWatchdogReset > WATCHDOG_TIMEOUT)
  {
    Serial.println("Watchdog timeout - resetting system");
    ESP.restart();
  }
}