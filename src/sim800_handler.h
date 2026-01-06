#ifndef SIM800_HANDLER_H
#define SIM800_HANDLER_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// Include cellular config FIRST
#include "cellular_config.h"

// Forward declarations to avoid including headers in .h file
class TinyGsm;
template <typename T>
class TinyGsmClient;
class PubSubClient;

class SIM800Handler
{
public:
    SIM800Handler();
    ~SIM800Handler();

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

private:
    // Opaque pointer pattern - hide implementation
    struct GsmData;
    GsmData *gsmData;

    // Disable copy
    SIM800Handler(const SIM800Handler &) = delete;
    SIM800Handler &operator=(const SIM800Handler &) = delete;
};

#endif