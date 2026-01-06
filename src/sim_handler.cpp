#include <Arduino.h>
#include <TinyGsmClient.h>
#include "config.h"

#ifdef USE_CELLULAR
TinyGsm modem(Serial1);
TinyGsmClient gsmClient(modem);

void initSIM800()
{
    pinMode(SIM800_PWR_PIN, OUTPUT);

    Serial1.begin(SIM800_BAUDRATE, SWSERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);

    // Power on modem
    digitalWrite(SIM800_PWR_PIN, HIGH);
    delay(1000);
    digitalWrite(SIM800_PWR_PIN, LOW);
    delay(5000);

    Serial.println("Initializing modem...");

    if (!modem.restart())
    {
        Serial.println("Modem restart failed");
        return;
    }

    String modemInfo = modem.getModemInfo();
    Serial.printf("Modem: %s\n", modemInfo.c_str());

    // Connect to network
    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork())
    {
        Serial.println("Network failed");
        return;
    }
    Serial.println("OK");

    // Connect to GPRS
    Serial.print("Connecting to APN...");
    if (!modem.gprsConnect("internet", "", ""))
    {
        Serial.println("GPRS failed");
        return;
    }
    Serial.println("OK");

    Serial.printf("IP: %s\n", modem.getLocalIP().cString());
}

bool sendDataViaCellular(const char *data)
{
    if (!modem.isGprsConnected())
    {
        if (!modem.gprsConnect("internet", "", ""))
        {
            return false;
        }
    }

    // Here you would implement HTTP or MQTT over cellular
    // For example, using HTTP POST:
    // HttpClient http(gsmClient, "your-server.com", 80);
    // http.post("/api/data", "application/json", data);

    return true;
}
#endif