#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>
#include "sim800_handler.h"
#include "registers_config.h" // Включаем для RegisterType

struct MQTTCommand
{
    String topic;
    String clientId;
    uint16_t address;
    float value;
    RegisterType registerType;
    unsigned long timestamp;
    String commandId;
};

class MQTTHandler
{
public:
    void begin(SIM800Handler *sim800);
    void loop();
    void publishData(JsonDocument &data);
    void publishRaw(const char *topic, const char *payload);
    void publishStatus(const char *payload);
    void subscribeToCommands();
    void processMessages();
    bool hasPendingCommands();
    MQTTCommand getNextCommand();
    void sendCommandResponse(const MQTTCommand &cmd, bool success, const char *message = "");
    bool isConnected();
    uint32_t getMessageCount() { return messageCount; }

private:
    SIM800Handler *sim800;
    String clientId;
    std::queue<MQTTCommand> commandQueue;
    uint32_t messageCount = 0;

    // Обработка входящих MQTT сообщений
    void onMessageReceived(const String &topic, const String &payload);

    // Генерация ID и топиков
    String generateClientId();
    String getBaseTopic();
    String getDataTopic();
    String getCommandTopic();
    String getResponseTopic();
    String getStatusTopic();

    // Парсинг JSON команд
    bool parseCommand(const String &payload, MQTTCommand &cmd);
    RegisterType stringToRegisterType(const String &typeStr);
};

#endif