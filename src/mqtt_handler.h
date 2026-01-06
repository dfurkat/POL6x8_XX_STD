#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <queue>
#include "sim800_handler.h"

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

    void onMessageReceived(String topic, String payload);
    String generateClientId();
    String getBaseTopic();
    String getDataTopic();
    String getCommandTopic();
    String getResponseTopic();
    String getStatusTopic();

    // MQTT callback (static wrapper)
    static void mqttCallback(char *topic, byte *payload, unsigned int length);
    static MQTTHandler *instance;
};

#endif