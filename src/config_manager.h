#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

struct SystemConfig
{
    // Modbus settings
    uint8_t modbusSlaveId = 1;
    uint32_t modbusBaudRate = 9600;
    uint8_t modbusParity = 0; // 0=none, 1=odd, 2=even
    uint8_t modbusStopBits = 1;

    // MQTT settings
    char mqttServer[100] = "mqtt.your-server.com";
    uint16_t mqttPort = 1883;
    char mqttUser[50] = "";
    char mqttPassword[50] = "";

    // Cellular settings
    char apn[50] = "your-apn";
    char gprsUser[50] = "";
    char gprsPass[50] = "";

    // Polling intervals (ms)
    uint32_t modbusPollInterval = 5000;
    uint32_t mqttPublishInterval = 10000;
    uint32_t statusUpdateInterval = 30000;

    // Debug settings
    bool debugEnabled = true;
    uint8_t logLevel = 1; // 0=error, 1=info, 2=debug
};

class ConfigManager
{
public:
    bool begin();
    bool loadConfig();
    bool saveConfig();
    SystemConfig getConfig() { return config; }
    void setConfig(const SystemConfig &newConfig) { config = newConfig; }
    void printConfig();

private:
    SystemConfig config;
    const char *configFile = "/config.json";

    void loadDefaults();
    bool initFilesystem();
};

#endif