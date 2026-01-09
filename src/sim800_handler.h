#ifndef SIM800_HANDLER_H
#define SIM800_HANDLER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <queue>
#include <functional>

// Структура для хранения MQTT сообщения
struct MQTTMessage
{
    String topic;
    String payload;
    unsigned long timestamp;
    uint8_t qos;
    bool retain;
};

// Callback тип для обработки сообщений
typedef std::function<void(const String &topic, const String &payload)> MQTTMessageCallback;

// Simplified SIM800 Handler without TinyGSM dependency
class SIM800Handler
{
private:
    SoftwareSerial *serial;
    bool network_connected;
    bool mqtt_connected;
    String mqtt_server;
    uint16_t mqtt_port;
    String mqtt_client_id;

    // Очередь входящих сообщений
    std::queue<MQTTMessage> message_queue;

    // Callback для новых сообщений
    MQTTMessageCallback message_callback;

    // Buffer для обработки AT ответов
    String response_buffer;
    bool processing_message;
    String current_topic;
    String current_payload;

public:
    SIM800Handler();
    ~SIM800Handler() = default;

    void begin(SoftwareSerial &serial, uint8_t powerPin, uint8_t resetPin);
    bool connectToNetwork(const char *apn, const char *user = "", const char *pass = "");
    bool configureMQTT(const char *server, uint16_t port);
    bool connectMQTT(const char *clientId, const char *user = "", const char *pass = "");
    void mqttLoop();
    bool mqttPublish(const char *topic, const char *payload);
    bool mqttSubscribe(const char *topic);
    bool isNetworkConnected();
    bool isMQTTConnected();
    void reset();
    int getSignalStrength();

    // Новые методы для обработки сообщений
    void setMessageCallback(MQTTMessageCallback callback);
    bool hasMessages();
    MQTTMessage getNextMessage();
    void processIncomingData();

private:
    // AT command helpers
    bool sendATCommand(const char *cmd, const char *expect = "OK", unsigned long timeout = 2000);
    String readResponse(unsigned long timeout = 2000);
    bool waitForResponse(const char *expect, unsigned long timeout = 2000);

    // SIM800 specific commands
    bool setupGPRS(const char *apn, const char *user, const char *pass);
    bool checkNetworkRegistration();
    bool getIPAddress();

    // MQTT message parsing
    void parseIncomingData();
    bool parseMQTTMessage(const String &response);
    void extractMQTTMessage();
    void addMessageToQueue(const String &topic, const String &payload);

    // Вспомогательные методы
    void clearSerialBuffer();
    String readLine(unsigned long timeout = 100);
};

#endif