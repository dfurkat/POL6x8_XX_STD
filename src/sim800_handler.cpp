#include "sim800_handler.h"

// Include TinyGSM headers ONLY in .cpp file
#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Define the actual implementation structure
struct SIM800Handler::GsmData
{
    SoftwareSerial *serial;
    TinyGsm *modem;
    TinyGsmClient *client;
    PubSubClient *mqtt;
    bool networkConnected;
    uint8_t powerPin;
    uint8_t resetPin;

    GsmData()
        : serial(nullptr),
          modem(nullptr),
          client(nullptr),
          mqtt(nullptr),
          networkConnected(false),
          powerPin(0),
          resetPin(0) {}

    ~GsmData()
    {
        cleanup();
    }

    void cleanup()
    {
        if (mqtt)
        {
            delete mqtt;
            mqtt = nullptr;
        }
        if (client)
        {
            delete client;
            client = nullptr;
        }
        if (modem)
        {
            delete modem;
            modem = nullptr;
        }
    }
};

SIM800Handler::SIM800Handler()
    : gsmData(new GsmData()) {}

SIM800Handler::~SIM800Handler()
{
    delete gsmData;
}

void SIM800Handler::begin(SoftwareSerial &serial, uint8_t powerPin, uint8_t resetPin)
{
    gsmData->serial = &serial;
    gsmData->powerPin = powerPin;
    gsmData->resetPin = resetPin;

    // Configure pins
    pinMode(powerPin, OUTPUT);
    pinMode(resetPin, OUTPUT);

    // Initial state
    digitalWrite(powerPin, HIGH);
    digitalWrite(resetPin, HIGH);

    // Initialize serial
    serial.begin(9600);
    delay(3000);

    // Power cycle the modem
    Serial.println("Powering on SIM800L...");
    digitalWrite(powerPin, LOW);
    delay(1000);
    digitalWrite(powerPin, HIGH);
    delay(2000);
    digitalWrite(powerPin, LOW);
    delay(3000);

    // Create modem instance
    gsmData->modem = new TinyGsm(serial);

    // Wait for modem to respond
    Serial.println("Initializing modem...");
    if (!gsmData->modem->init())
    {
        Serial.println("Failed to initialize modem");
        return;
    }

    // Create client
    gsmData->client = new TinyGsmClient(*gsmData->modem);

    // Create MQTT client
    gsmData->mqtt = new PubSubClient(*gsmData->client);
    gsmData->mqtt->setBufferSize(512);
    gsmData->mqtt->setKeepAlive(60);

    Serial.println("SIM800L handler initialized");
}

bool SIM800Handler::connectToNetwork(const char *apn, const char *user, const char *pass)
{
    if (!gsmData->modem)
    {
        Serial.println("Modem not initialized");
        return false;
    }

    Serial.println("=== Connecting to Cellular Network ===");

    // Restart modem
    Serial.print("Restarting modem... ");
    if (!gsmData->modem->restart())
    {
        Serial.println("FAILED");
        return false;
    }
    Serial.println("OK");

    // Get modem info
    String modemInfo = gsmData->modem->getModemInfo();
    Serial.print("Modem: ");
    Serial.println(modemInfo);

    // Unlock SIM if needed
    Serial.print("Checking SIM status... ");
    if (gsmData->modem->getSimStatus() != 1)
    {
        Serial.println("SIM not ready");
        return false;
    }
    Serial.println("OK");

    // Wait for network
    Serial.print("Waiting for network... ");
    if (!gsmData->modem->waitForNetwork())
    {
        Serial.println("FAILED");
        return false;
    }
    Serial.println("OK");

    // Get network info
    String operatorName = gsmData->modem->getOperator();
    Serial.print("Operator: ");
    Serial.println(operatorName);

    int signalQuality = gsmData->modem->getSignalQuality();
    Serial.print("Signal: ");
    Serial.print(signalQuality);
    Serial.println(" dBm");

    // Connect to GPRS
    Serial.print("Connecting to GPRS (APN: ");
    Serial.print(apn);
    Serial.println(")... ");

    if (!gsmData->modem->gprsConnect(apn, user, pass))
    {
        Serial.println("FAILED");
        return false;
    }

    Serial.println("CONNECTED");

    // Get IP address
    String localIP = gsmData->modem->getLocalIP();
    Serial.print("Local IP: ");
    Serial.println(localIP);

    gsmData->networkConnected = true;
    Serial.println("=== Network Connection Established ===");

    return true;
}

bool SIM800Handler::configureMQTT(const char *server, uint16_t port)
{
    if (!gsmData->networkConnected)
    {
        Serial.println("Network not connected");
        return false;
    }

    if (!gsmData->mqtt)
    {
        Serial.println("MQTT client not initialized");
        return false;
    }

    gsmData->mqtt->setServer(server, port);

    Serial.print("MQTT configured: ");
    Serial.print(server);
    Serial.print(":");
    Serial.println(port);

    return true;
}

bool SIM800Handler::connectMQTT(const char *clientId, const char *user, const char *pass)
{
    if (!gsmData->mqtt)
    {
        Serial.println("MQTT client not initialized");
        return false;
    }

    Serial.print("Connecting to MQTT as ");
    Serial.print(clientId);
    Serial.println("...");

    bool connected = gsmData->mqtt->connect(clientId, user, pass);

    if (connected)
    {
        Serial.println("MQTT connected");
    }
    else
    {
        Serial.print("MQTT connection failed. State: ");
        Serial.println(gsmData->mqtt->state());
    }

    return connected;
}

void SIM800Handler::mqttLoop()
{
    if (gsmData->mqtt && gsmData->networkConnected)
    {
        gsmData->mqtt->loop();
    }
}

bool SIM800Handler::mqttPublish(const char *topic, const char *payload)
{
    if (!isMQTTConnected())
    {
        return false;
    }

    bool success = gsmData->mqtt->publish(topic, payload);

    if (!success)
    {
        Serial.print("Failed to publish to topic: ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
    }

    return success;
}

bool SIM800Handler::mqttSubscribe(const char *topic)
{
    if (!isMQTTConnected())
    {
        return false;
    }

    bool success = gsmData->mqtt->subscribe(topic);

    if (success)
    {
        Serial.print("Subscribed to topic: ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("Failed to subscribe to topic: ");
        Serial.println(topic);
    }

    return success;
}

bool SIM800Handler::isNetworkConnected()
{
    if (!gsmData->modem)
        return false;
    return gsmData->networkConnected && gsmData->modem->isNetworkConnected();
}

bool SIM800Handler::isMQTTConnected()
{
    if (!gsmData->mqtt)
        return false;
    return gsmData->mqtt->connected();
}

void SIM800Handler::reset()
{
    Serial.println("Resetting SIM800L...");

    if (gsmData->modem)
    {
        // Disconnect GPRS first
        gsmData->modem->gprsDisconnect();
        delay(1000);

        // Restart modem
        gsmData->modem->restart();
        delay(3000);

        gsmData->networkConnected = false;
    }

    Serial.println("SIM800L reset complete");
}

int SIM800Handler::getSignalStrength()
{
    if (!gsmData->modem)
        return -1;

    int rssi = gsmData->modem->getSignalQuality();

    // Convert to dBm (approximate)
    if (rssi == 0)
        return -113;
    if (rssi == 1)
        return -111;
    if (rssi >= 2 && rssi <= 30)
        return -109 + (2 * (rssi - 2));
    if (rssi == 31)
        return -51;
    if (rssi == 99)
        return -999; // Unknown

    return -113 + (2 * rssi);
}