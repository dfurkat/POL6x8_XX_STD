#include "sim800_handler.h"
#include <SoftwareSerial.h>

SIM800Handler::SIM800Handler() 
    : serial(nullptr)
    , network_connected(false)
    , mqtt_connected(false)
    , mqtt_port(0)
    , processing_message(false)
{
}

void SIM800Handler::begin(SoftwareSerial &serial, uint8_t powerPin, uint8_t resetPin)
{
    this->serial = &serial;

    // Configure pins
    pinMode(powerPin, OUTPUT);
    pinMode(resetPin, OUTPUT);

    // Initial state
    digitalWrite(powerPin, HIGH);
    digitalWrite(resetPin, HIGH);

    // Start serial
    serial.begin(9600);
    delay(3000);

    // Power cycle
    digitalWrite(powerPin, LOW);
    delay(1000);
    digitalWrite(powerPin, HIGH);
    delay(2000);
    digitalWrite(powerPin, LOW);
    delay(3000);

    // Clear buffer
    clearSerialBuffer();

    // Test AT command
    if (sendATCommand("AT"))
    {
        Serial.println("SIM800L initialized");
    }
    else
    {
        Serial.println("SIM800L initialization failed");
    }
}

bool SIM800Handler::connectToNetwork(const char *apn, const char *user, const char *pass)
{
    if (!serial)
        return false;

    Serial.println("Connecting to cellular network...");

    // Send basic AT commands
    if (!sendATCommand("ATE0"))
    {
        Serial.println("Failed to disable echo");
        return false;
    }

    if (!sendATCommand("AT+CFUN=1"))
    {
        Serial.println("Failed to set full functionality");
        return false;
    }

    // Wait for network registration
    if (!checkNetworkRegistration())
    {
        Serial.println("Failed to register to network");
        return false;
    }

    // Setup GPRS
    if (!setupGPRS(apn, user, pass))
    {
        Serial.println("Failed to setup GPRS");
        return false;
    }

    // Get IP address
    if (!getIPAddress())
    {
        Serial.println("Failed to get IP address");
        return false;
    }

    network_connected = true;
    Serial.println("Network connected!");

    return true;
}

bool SIM800Handler::configureMQTT(const char *server, uint16_t port)
{
    if (!network_connected)
    {
        Serial.println("Network not connected");
        return false;
    }

    mqtt_server = server;
    mqtt_port = port;

    Serial.print("MQTT configured: ");
    Serial.print(server);
    Serial.print(":");
    Serial.println(port);

    return true;
}

bool SIM800Handler::connectMQTT(const char *clientId, const char *user, const char *pass)
{
    if (!network_connected)
    {
        Serial.println("Network not connected");
        return false;
    }

    mqtt_client_id = clientId;

    Serial.print("Connecting to MQTT as ");
    Serial.println(clientId);

    // SIM800 has built-in MQTT support via AT commands
    // We'll use AT+CMQTTSTART, AT+CMQTTACCQ, AT+CMQTTCONNECT
    
    // Start MQTT service
    if (!sendATCommand("AT+CMQTTSTART"))
    {
        Serial.println("Failed to start MQTT service");
        return false;
    }

    // Acquire client
    String cmd = "AT+CMQTTACCQ=0,\"";
    cmd += clientId;
    cmd += "\"";
    if (!sendATCommand(cmd.c_str()))
    {
        Serial.println("Failed to acquire MQTT client");
        return false;
    }

    // Connect to broker
    cmd = "AT+CMQTTCONNECT=0,\"tcp://";
    cmd += mqtt_server;
    cmd += ":";
    cmd += mqtt_port;
    cmd += "\",60,1";
    
    // Add credentials if provided
    if (strlen(user) > 0)
    {
        cmd += ",\"";
        cmd += user;
        cmd += "\",\"";
        cmd += pass;
        cmd += "\"";
    }
    
    if (!sendATCommand(cmd.c_str()))
    {
        Serial.println("Failed to connect to MQTT broker");
        return false;
    }

    mqtt_connected = true;
    Serial.println("MQTT connected!");

    return true;
}

void SIM800Handler::mqttLoop()
{
    // Process incoming serial data
    processIncomingData();
}

bool SIM800Handler::mqttPublish(const char *topic, const char *payload)
{
    if (!mqtt_connected)
        return false;

    // Publish using SIM800 AT commands
    // AT+CMQTTPUB=0,"topic","payload",0,0
    String cmd = "AT+CMQTTPUB=0,\"";
    cmd += topic;
    cmd += "\",\"";
    cmd += payload;
    cmd += "\",0,0";

    if (sendATCommand(cmd.c_str()))
    {
        Serial.print("Published to ");
        Serial.println(topic);
        return true;
    }
    else
    {
        Serial.print("Failed to publish to ");
        Serial.println(topic);
        return false;
    }
}

bool SIM800Handler::mqttSubscribe(const char *topic)
{
    if (!mqtt_connected)
        return false;

    // Subscribe using SIM800 AT commands
    // AT+CMQTTSUB=0,"topic",0
    String cmd = "AT+CMQTTSUB=0,\"";
    cmd += topic;
    cmd += "\",0";

    if (sendATCommand(cmd.c_str()))
    {
        Serial.print("Subscribed to ");
        Serial.println(topic);
        return true;
    }
    else
    {
        Serial.print("Failed to subscribe to ");
        Serial.println(topic);
        return false;
    }
}

bool SIM800Handler::isNetworkConnected()
{
    return network_connected;
}

bool SIM800Handler::isMQTTConnected()
{
    return mqtt_connected;
}

void SIM800Handler::reset()
{
    Serial.println("Resetting SIM800L...");
    
    // Disconnect MQTT
    if (mqtt_connected)
    {
        sendATCommand("AT+CMQTTDISC=0,120");
        sendATCommand("AT+CMQTTREL=0");
        mqtt_connected = false;
    }
    
    // Reset network connection
    network_connected = false;
    
    Serial.println("Reset complete");
}

int SIM800Handler::getSignalStrength()
{
    if (!serial)
        return -1;

    sendATCommand("AT+CSQ");
    String response = readResponse(2000);

    // Parse CSQ response
    int start = response.indexOf("+CSQ:");
    if (start != -1)
    {
        int comma = response.indexOf(',', start);
        if (comma != -1)
        {
            String rssiStr = response.substring(start + 6, comma);
            int rssi = rssiStr.toInt();
            if (rssi == 99)
                return -1; // Unknown
            return rssi;
        }
    }

    return -1;
}

// ==================== Новые методы для обработки сообщений ====================

void SIM800Handler::setMessageCallback(MQTTMessageCallback callback)
{
    message_callback = callback;
}

bool SIM800Handler::hasMessages()
{
    return !message_queue.empty();
}

MQTTMessage SIM800Handler::getNextMessage()
{
    if (message_queue.empty())
    {
        return MQTTMessage{};
    }

    MQTTMessage msg = message_queue.front();
    message_queue.pop();
    return msg;
}

void SIM800Handler::processIncomingData()
{
    if (!serial)
        return;

    // Читаем все доступные данные
    while (serial->available())
    {
        char c = serial->read();
        response_buffer += c;

        // Проверяем на завершение строки
        if (c == '\n')
        {
            parseIncomingData();
            response_buffer = "";
        }
    }

    // Также проверяем буфер на наличие данных (на случай если нет \n)
    if (response_buffer.length() > 0)
    {
        parseIncomingData();
    }
}

// ==================== Приватные вспомогательные методы ====================

bool SIM800Handler::sendATCommand(const char *cmd, const char *expect, unsigned long timeout)
{
    if (!serial)
        return false;

    serial->println(cmd);
    
    return waitForResponse(expect, timeout);
}

String SIM800Handler::readResponse(unsigned long timeout)
{
    if (!serial)
        return "";

    unsigned long start = millis();
    String response = "";

    while (millis() - start < timeout)
    {
        while (serial->available())
        {
            char c = serial->read();
            response += c;
        }
        delay(10);
    }

    return response;
}

bool SIM800Handler::waitForResponse(const char *expect, unsigned long timeout)
{
    if (!serial)
        return false;

    unsigned long start = millis();
    String response = "";

    while (millis() - start < timeout)
    {
        while (serial->available())
        {
            char c = serial->read();
            response += c;

            if (expect && response.indexOf(expect) != -1)
            {
                return true;
            }
        }
        delay(10);
    }

    return false;
}

bool SIM800Handler::setupGPRS(const char *apn, const char *user, const char *pass)
{
    // Set APN
    String cmd = "AT+CSTT=\"";
    cmd += apn;
    cmd += "\",\"";
    cmd += user;
    cmd += "\",\"";
    cmd += pass;
    cmd += "\"";

    if (!sendATCommand(cmd.c_str()))
    {
        return false;
    }

    // Activate GPRS
    if (!sendATCommand("AT+CIICR", "OK", 15000))
    {
        return false;
    }

    return true;
}

bool SIM800Handler::checkNetworkRegistration()
{
    // Wait for network registration
    for (int i = 0; i < 30; i++)
    {
        if (sendATCommand("AT+CREG?", "+CREG: 0,1") ||
            sendATCommand("AT+CREG?", "+CREG: 0,5"))
        {
            return true;
        }
        delay(1000);
    }
    return false;
}

bool SIM800Handler::getIPAddress()
{
    return sendATCommand("AT+CIFSR", ".", 5000);
}

void SIM800Handler::parseIncomingData()
{
    if (response_buffer.length() == 0)
        return;

    // Удаляем пробелы и переводы строк
    response_buffer.trim();

    // Пропускаем пустые строки
    if (response_buffer.length() == 0)
        return;

    Serial.print("SIM800 Response: ");
    Serial.println(response_buffer);

    // Проверяем на MQTT сообщение
    if (parseMQTTMessage(response_buffer))
    {
        // Сообщение успешно распарсено
        return;
    }

    // Проверяем другие AT ответы
    if (response_buffer.indexOf("+CMQTTRXSTART") != -1)
    {
        // Начало MQTT сообщения
        processing_message = true;
        current_topic = "";
        current_payload = "";
        
        // Извлекаем длину сообщения
        int start = response_buffer.indexOf("+CMQTTRXSTART:");
        if (start != -1)
        {
            String lenStr = response_buffer.substring(start + 14);
            lenStr.trim();
            Serial.print("MQTT message started, length: ");
            Serial.println(lenStr);
        }
    }
    else if (response_buffer.indexOf("+CMQTTRXEND") != -1)
    {
        // Конец MQTT сообщения
        if (processing_message && current_topic.length() > 0)
        {
            addMessageToQueue(current_topic, current_payload);
        }
        processing_message = false;
        current_topic = "";
        current_payload = "";
    }
    else if (processing_message)
    {
        // Данные MQTT сообщения
        if (current_topic.length() == 0)
        {
            // Первая строка - тема
            current_topic = response_buffer;
        }
        else
        {
            // Последующие строки - payload
            if (current_payload.length() > 0)
            {
                current_payload += "\n";
            }
            current_payload += response_buffer;
        }
    }
}

bool SIM800Handler::parseMQTTMessage(const String &response)
{
    // Парсим полный формат MQTT сообщения SIM800
    // Пример: +CMQTTRX: 0,"topic","payload"
    
    int start = response.indexOf("+CMQTTRX:");
    if (start == -1)
        return false;

    // Извлекаем тему
    int topicStart = response.indexOf('"', start + 9);
    if (topicStart == -1)
        return false;
    
    int topicEnd = response.indexOf('"', topicStart + 1);
    if (topicEnd == -1)
        return false;
    
    String topic = response.substring(topicStart + 1, topicEnd);

    // Извлекаем payload
    int payloadStart = response.indexOf('"', topicEnd + 1);
    if (payloadStart == -1)
        return false;
    
    int payloadEnd = response.indexOf('"', payloadStart + 1);
    if (payloadEnd == -1)
        return false;
    
    String payload = response.substring(payloadStart + 1, payloadEnd);

    // Добавляем в очередь
    addMessageToQueue(topic, payload);
    
    return true;
}

void SIM800Handler::extractMQTTMessage()
{
    // Альтернативный метод извлечения сообщения
    // Используется когда сообщение приходит по частям
    
    if (!serial)
        return;

    // Читаем тему (первая строка после +CMQTTRXSTART)
    String topic = readLine(1000);
    topic.trim();
    
    if (topic.length() == 0)
        return;

    // Читаем payload (может быть несколько строк)
    String payload = "";
    unsigned long start = millis();
    
    while (millis() - start < 5000) // Таймаут 5 секунд
    {
        String line = readLine(1000);
        line.trim();
        
        if (line.indexOf("+CMQTTRXEND") != -1)
        {
            // Конец сообщения
            break;
        }
        
        if (payload.length() > 0)
        {
            payload += "\n";
        }
        payload += line;
    }

    if (topic.length() > 0 && payload.length() > 0)
    {
        addMessageToQueue(topic, payload);
    }
}

void SIM800Handler::addMessageToQueue(const String &topic, const String &payload)
{
    MQTTMessage msg;
    msg.topic = topic;
    msg.payload = payload;
    msg.timestamp = millis();
    msg.qos = 0;
    msg.retain = false;

    message_queue.push(msg);

    Serial.print("MQTT Message queued: ");
    Serial.print(topic);
    Serial.print(" -> ");
    Serial.println(payload);

    // Вызываем callback если установлен
    if (message_callback)
    {
        message_callback(topic, payload);
    }
}

void SIM800Handler::clearSerialBuffer()
{
    if (!serial)
        return;

    while (serial->available())
    {
        serial->read();
    }
}

String SIM800Handler::readLine(unsigned long timeout)
{
    if (!serial)
        return "";

    unsigned long start = millis();
    String line = "";

    while (millis() - start < timeout)
    {
        while (serial->available())
        {
            char c = serial->read();
            
            if (c == '\n' || c == '\r')
            {
                if (line.length() > 0)
                {
                    return line;
                }
            }
            else
            {
                line += c;
            }
        }
        delay(10);
    }

    return line;
}