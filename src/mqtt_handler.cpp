#include "mqtt_handler.h"
#include "sim800_handler.h"
#include "registers_config.h" // Для RegisterType

void MQTTHandler::begin(SIM800Handler *sim800)
{
    this->sim800 = sim800;
    clientId = generateClientId();

    // Устанавливаем callback для обработки входящих сообщений
    sim800->setMessageCallback([this](const String &topic, const String &payload)
                               { this->onMessageReceived(topic, payload); });

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
    if (sim800 && sim800->isMQTTConnected())
    {
        // Обрабатываем входящие данные SIM800
        sim800->mqttLoop();

        // Обрабатываем сообщения из очереди
        processMessages();
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
    // Проверяем наличие сообщений в SIM800Handler
    while (sim800->hasMessages())
    {
        MQTTMessage msg = sim800->getNextMessage();

        // Проверяем, является ли это командой
        if (msg.topic == getCommandTopic() || msg.topic.endsWith("/command"))
        {
            MQTTCommand cmd;
            if (parseCommand(msg.payload, cmd))
            {
                cmd.topic = msg.topic;
                cmd.timestamp = msg.timestamp;
                commandQueue.push(cmd);

                Serial.print("Command parsed from topic: ");
                Serial.println(msg.topic);
            }
        }
        else
        {
            // Другие сообщения (логируем)
            Serial.print("MQTT message (not a command): ");
            Serial.print(msg.topic);
            Serial.print(" -> ");
            Serial.println(msg.payload);
        }
    }
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

void MQTTHandler::onMessageReceived(const String &topic, const String &payload)
{
    Serial.print("MQTT message received via callback: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    // Если это команда - парсим и добавляем в очередь
    if (topic == getCommandTopic() || topic.endsWith("/command"))
    {
        MQTTCommand cmd;
        if (parseCommand(payload, cmd))
        {
            cmd.topic = topic;
            cmd.timestamp = millis();
            commandQueue.push(cmd);

            Serial.print("Command queued via callback: Address=0x");
            Serial.print(cmd.address, HEX);
            Serial.print(", Value=");
            Serial.println(cmd.value);
        }
    }
}

bool MQTTHandler::parseCommand(const String &payload, MQTTCommand &cmd)
{
    // Parse JSON command
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("Failed to parse JSON command: ");
        Serial.println(error.c_str());
        return false;
    }

    // Извлекаем обязательные поля
    if (!doc.containsKey("address") || !doc.containsKey("value"))
    {
        Serial.println("Command missing required fields (address or value)");
        return false;
    }

    cmd.clientId = doc["client_id"] | "";
    cmd.address = doc["address"];
    cmd.value = doc["value"];
    cmd.commandId = doc["command_id"] | String(millis());

    // Определяем тип регистра
    String typeStr = doc["type"] | "holding";
    cmd.registerType = stringToRegisterType(typeStr);

    return true;
}

RegisterType MQTTHandler::stringToRegisterType(const String &typeStr)
{
    if (typeStr == "coil")
    {
        return REG_COIL;
    }
    else if (typeStr == "discrete")
    {
        return REG_DISCRETE;
    }
    else if (typeStr == "input")
    {
        return REG_INPUT;
    }
    else
    {
        return REG_HOLDING;
    }
}
