#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void initMQTT()
{
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    Serial.println("MQTT client initialized");
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    // Forward to main handler
    handleMQTTCommand(topic, payload, length);
}

bool reconnectMQTT()
{
    Serial.print("Connecting to MQTT...");

    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
    {
        Serial.println("connected");

        // Subscribe to command topics
        mqttClient.subscribe(TOPIC_COMMANDS);
        mqttClient.subscribe(TOPIC_CONFIG);

        // Publish connection message
        mqttClient.publish(TOPIC_STATUS, "{\"status\":\"connected\"}");

        return true;
    }
    else
    {
        Serial.printf("failed, rc=%d\n", mqttClient.state());
        return false;
    }
}

void mqttLoop()
{
    if (!mqttClient.connected())
    {
        return;
    }
    mqttClient.loop();
}

bool mqttConnected()
{
    return mqttClient.connected();
}

bool mqttPublish(const char *topic, const char *payload, bool retained)
{
    if (!mqttClient.connected())
    {
        return false;
    }

    bool result = mqttClient.publish(topic, payload, retained);

#ifdef MQTT_DEBUG
    if (result)
    {
        Serial.printf("MQTT publish to %s: %s\n", topic, payload);
    }
    else
    {
        Serial.printf("MQTT publish failed to %s\n", topic);
    }
#endif

    return result;
}