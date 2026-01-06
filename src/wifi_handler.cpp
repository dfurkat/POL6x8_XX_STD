#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "config.h"

void initWiFi()
{
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("\nWiFi connection failed");
        // Fallback to cellular
    }
}

bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}