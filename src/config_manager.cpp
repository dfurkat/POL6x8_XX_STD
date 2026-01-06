#include "config_manager.h"

bool ConfigManager::begin()
{
    if (!initFilesystem())
    {
        Serial.println("Failed to initialize filesystem");
        return false;
    }

    if (!loadConfig())
    {
        Serial.println("Failed to load config, creating default...");
        loadDefaults();
        if (!saveConfig())
        {
            Serial.println("Failed to save default config");
            return false;
        }
    }

    return true;
}

bool ConfigManager::initFilesystem()
{
    if (!SPIFFS.begin())
    {
        Serial.println("SPIFFS mount failed, formatting...");
        if (SPIFFS.format())
        {
            Serial.println("SPIFFS formatted successfully");
            if (SPIFFS.begin())
            {
                return true;
            }
        }
        return false;
    }

    // Check if config file exists
    if (!SPIFFS.exists(configFile))
    {
        Serial.println("Config file doesn't exist, will create default");
    }

    return true;
}

bool ConfigManager::loadConfig()
{
    File file = SPIFFS.open(configFile, "r");
    if (!file)
    {
        Serial.println("Failed to open config file for reading");
        return false;
    }

    size_t size = file.size();
    if (size > 2048)
    {
        Serial.println("Config file too large");
        file.close();
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size + 1]);
    file.readBytes(buf.get(), size);
    buf[size] = '\0';
    file.close();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error)
    {
        Serial.print("Failed to parse config JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // Load Modbus settings
    config.modbusSlaveId = doc["modbus"]["slave_id"] | 1;
    config.modbusBaudRate = doc["modbus"]["baud_rate"] | 9600;
    config.modbusParity = doc["modbus"]["parity"] | 0;
    config.modbusStopBits = doc["modbus"]["stop_bits"] | 1;

    // Load MQTT settings
    strlcpy(config.mqttServer, doc["mqtt"]["server"] | "mqtt.your-server.com",
            sizeof(config.mqttServer));
    config.mqttPort = doc["mqtt"]["port"] | 1883;
    strlcpy(config.mqttUser, doc["mqtt"]["user"] | "", sizeof(config.mqttUser));
    strlcpy(config.mqttPassword, doc["mqtt"]["password"] | "",
            sizeof(config.mqttPassword));

    // Load cellular settings
    strlcpy(config.apn, doc["cellular"]["apn"] | "your-apn", sizeof(config.apn));
    strlcpy(config.gprsUser, doc["cellular"]["user"] | "", sizeof(config.gprsUser));
    strlcpy(config.gprsPass, doc["cellular"]["pass"] | "", sizeof(config.gprsPass));

    // Load intervals
    config.modbusPollInterval = doc["intervals"]["modbus_poll"] | 5000;
    config.mqttPublishInterval = doc["intervals"]["mqtt_publish"] | 10000;
    config.statusUpdateInterval = doc["intervals"]["status_update"] | 30000;

    // Load debug settings
    config.debugEnabled = doc["debug"]["enabled"] | true;
    config.logLevel = doc["debug"]["log_level"] | 1;

    Serial.println("Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfig()
{
    File file = SPIFFS.open(configFile, "w");
    if (!file)
    {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    DynamicJsonDocument doc(2048);

    // Save Modbus settings
    doc["modbus"]["slave_id"] = config.modbusSlaveId;
    doc["modbus"]["baud_rate"] = config.modbusBaudRate;
    doc["modbus"]["parity"] = config.modbusParity;
    doc["modbus"]["stop_bits"] = config.modbusStopBits;

    // Save MQTT settings
    doc["mqtt"]["server"] = config.mqttServer;
    doc["mqtt"]["port"] = config.mqttPort;
    doc["mqtt"]["user"] = config.mqttUser;
    doc["mqtt"]["password"] = config.mqttPassword;

    // Save cellular settings
    doc["cellular"]["apn"] = config.apn;
    doc["cellular"]["user"] = config.gprsUser;
    doc["cellular"]["pass"] = config.gprsPass;

    // Save intervals
    doc["intervals"]["modbus_poll"] = config.modbusPollInterval;
    doc["intervals"]["mqtt_publish"] = config.mqttPublishInterval;
    doc["intervals"]["status_update"] = config.statusUpdateInterval;

    // Save debug settings
    doc["debug"]["enabled"] = config.debugEnabled;
    doc["debug"]["log_level"] = config.logLevel;

    if (serializeJsonPretty(doc, file) == 0)
    {
        Serial.println("Failed to write to config file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Configuration saved successfully");
    return true;
}

void ConfigManager::loadDefaults()
{
    // Already initialized with defaults in struct definition
}

void ConfigManager::printConfig()
{
    Serial.println("=== Current Configuration ===");
    Serial.print("Modbus Slave ID: ");
    Serial.println(config.modbusSlaveId);
    Serial.print("Modbus Baud Rate: ");
    Serial.println(config.modbusBaudRate);
    Serial.print("MQTT Server: ");
    Serial.print(config.mqttServer);
    Serial.print(":");
    Serial.println(config.mqttPort);
    Serial.print("APN: ");
    Serial.println(config.apn);
    Serial.print("Modbus Poll Interval: ");
    Serial.println(config.modbusPollInterval);
    Serial.print("MQTT Publish Interval: ");
    Serial.println(config.mqttPublishInterval);
    Serial.println("=============================");
}