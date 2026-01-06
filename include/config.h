#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// MQTT Configuration
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "climatix-controller-01"

// MQTT Topics
#define TOPIC_TELEMETRY "climatix/01/telemetry"
#define TOPIC_COMMANDS "climatix/01/commands"
#define TOPIC_STATUS "climatix/01/status"
#define TOPIC_CONFIG "climatix/01/config"

// Modbus Configuration
#define MODBUS_SLAVE_ID 1
#define MODBUS_BAUDRATE 9600
#define MODBUS_SERIAL_CONFIG SERIAL_8N1
#define MODBUS_DE_PIN D1 // MAX485 DE/RE control pin
#define MODBUS_RX_PIN D2 // SoftwareSerial RX
#define MODBUS_TX_PIN D3 // SoftwareSerial TX

// SIM800L Configuration (if using cellular)
#define SIM800_RX_PIN D5
#define SIM800_TX_PIN D6
#define SIM800_PWR_PIN D7
#define SIM800_BAUDRATE 9600

// Timing Configuration
#define MODBUS_POLL_INTERVAL 5000   // ms
#define MQTT_PUBLISH_INTERVAL 10000 // ms
#define WATCHDOG_TIMEOUT 30000      // ms

// Register Configuration
#define MAX_HOLDING_REGISTERS 1070
#define MAX_INPUT_REGISTERS 403
#define MAX_COILS 100
#define MAX_DISCRETE_INPUTS 100

#endif