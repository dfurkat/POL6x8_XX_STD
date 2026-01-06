#include "mqtt_handler.h"

// Register type constants
enum RegisterType
{
    REG_COIL = 0,
    REG_DISCRETE = 1,
    REG_INPUT = 2,
    REG_HOLDING = 3
};

MQTTHandler *MQTTHandler::instance = nullptr;

void MQTTHandler::begin(SIM800Handler *sim800)
{
    this->sim800 = sim800;
    clientId = generateClientId();

    // Set static instance for callback
    instance = this;

    // Set callback
    // Note: This depends on your PubSubClient version
    // mqttClient.setCallback(mqttCallback);

    Serial.print("MQTT Handler initialized. Client ID: ");
    Serial.println(clientId);
}

String MQTTHandler::generateClientId()
{
    uint32_t chipId = ESP.getChipId();
    char clientIdBuf[50];
    snprintf(clientIdBuf, sizeof(clientIdBuf), "modbus-gateway-%08X", chipId);
    return String(clientIdBuf);
}

String MQTTHandler::getBaseTopic()
{
    return "modbus/" + clientId;
}

String MQTTHandler::getDataTopic()
{
    return getBaseTopic() + "/data";
}

String MQTTHandler::getCommandTopic()
{
    return getBaseTopic() + "/command";
}

String MQTTHandler::getResponseTopic()
{
    return getBaseTopic() + "/response";
}

String MQTTHandler::getStatusTopic()
{
    return getBaseTopic() + "/status";
}

void MQTTHandler::loop()
{
    if (sim800->isMQTTConnected())
    {
        sim800->mqttLoop();
    }
}

void MQTTHandler::publishData(JsonDocument &data)
{
    if (!isConnected())
        return;

    String jsonStr;
    serializeJson(data, jsonStr);

    if (sim800->mqttPublish(getDataTopic().c_str(), jsonStr.c_str()))
    {
        messageCount++;
    }
    else
    {
        Serial.println("Failed to publish data");
    }
}

void MQTTHandler::publishRaw(const char *topic, const char *payload)
{
    if (!isConnected())
        return;

    sim800->mqttPublish(topic, payload);
}

void MQTTHandler::publishStatus(const char *payload)
{
    if (!isConnected())
        return;

    sim800->mqttPublish(getStatusTopic().c_str(), payload);
}

void MQTTHandler::subscribeToCommands()
{
    if (!isConnected())
        return;

    if (sim800->mqttSubscribe(getCommandTopic().c_str()))
    {
        Serial.print("Subscribed to command topic: ");
        Serial.println(getCommandTopic());
    }
}

void MQTTHandler::processMessages()
{
    // This would typically be called from the MQTT callback
    // Implementation depends on your PubSubClient setup
}

bool MQTTHandler::hasPendingCommands()
{
    return !commandQueue.empty();
}

MQTTCommand MQTTHandler::getNextCommand()
{
    if (commandQueue.empty())
    {
        return MQTTCommand{};
    }

    MQTTCommand cmd = commandQueue.front();
    commandQueue.pop();
    return cmd;
}

void MQTTHandler::sendCommandResponse(const MQTTCommand &cmd, bool success, const char *message)
{
    DynamicJsonDocument doc(512);

    doc["command_id"] = cmd.commandId;
    doc["address"] = cmd.address;
    doc["value"] = cmd.value;
    doc["type"] = (int)cmd.registerType;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    doc["received_at"] = cmd.timestamp;

    String jsonStr;
    serializeJson(doc, jsonStr);

    sim800->mqttPublish(getResponseTopic().c_str(), jsonStr.c_str());
}

bool MQTTHandler::isConnected()
{
    return sim800 && sim800->isMQTTConnected();
}

void MQTTHandler::onMessageReceived(String topic, String payload)
{
    Serial.print("MQTT message received: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    // Parse JSON command
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    MQTTCommand cmd;
    cmd.topic = topic;
    cmd.timestamp = millis();
    cmd.clientId = doc["client_id"] | "";
    cmd.address = doc["address"] | 0;
    cmd.value = doc["value"] | 0.0;
    cmd.commandId = doc["command_id"] | String(millis());

    // Determine register type
    String typeStr = doc["type"] | "holding";
    if (typeStr == "coil")
    {
        cmd.registerType = REG_COIL;
    }
    else if (typeStr == "discrete")
    {
        cmd.registerType = REG_DISCRETE;
    }
    else if (typeStr == "input")
    {
        cmd.registerType = REG_INPUT;
    }
    else
    {
        cmd.registerType = REG_HOLDING;
    }

    // Add to command queue
    commandQueue.push(cmd);

    Serial.print("Command queued: Address=0x");
    Serial.print(cmd.address, HEX);
    Serial.print(", Value=");
    Serial.println(cmd.value);
}

void MQTTHandler::mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (instance)
    {
        String topicStr = String(topic);
        String payloadStr;
        payloadStr.reserve(length);

        for (unsigned int i = 0; i < length; i++)
        {
            payloadStr += (char)payload[i];
        }

        instance->onMessageReceived(topicStr, payloadStr);
    }
}