#ifndef MODBUS_HANDLER_H
#define MODBUS_HANDLER_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <vector>
#include "registers_config.h"

struct RegisterValue
{
    uint16_t address;
    float value;
    RegisterType type;
    uint8_t size;
    unsigned long timestamp;
    bool valid;
};

struct RegisterInfo
{
    uint16_t address;
    const char *name;
    const char *description;
    const char *unit;
    RegisterType type;
    uint8_t size;
    float scale;
    float offset;
    float min_value;
    float max_value;
    uint8_t precision;
};

class ModbusHandler
{
public:
    void begin(SoftwareSerial &serial, uint8_t deRePin, uint8_t slaveId);
    bool pollAllRegisters();
    bool pollRegisterGroup(const char *groupName);
    bool pollSingleRegister(uint16_t address, RegisterType type);
    bool writeRegister(uint16_t address, float value, RegisterType type);
    bool getDataAsJson(JsonDocument &doc);
    RegisterValue getRegisterValue(uint16_t address);
    std::vector<RegisterValue> getAllRegisterValues();
    uint32_t getPollCount() { return pollCount; }
    uint32_t getErrorCount() { return errorCount; }

    // Get register info
    RegisterInfo getRegisterInfo(uint16_t address);
    std::vector<RegisterInfo> getAllRegisterInfo();

private:
    ModbusMaster modbus;
    uint8_t deRePin;
    uint8_t slaveId;

    // Statistics
    uint32_t pollCount = 0;
    uint32_t errorCount = 0;
    unsigned long lastPollTime = 0;

    // Storage for register values
    std::vector<RegisterValue> registerValues;

    // Helper methods
    void setTransmitMode(bool transmit);
    float readRegisterValue(const ModbusRegister &reg);
    float readRawRegister(uint16_t address, RegisterType type, uint8_t size);
    bool writeHoldingRegister(uint16_t address, uint16_t value);
    bool writeMultipleRegisters(uint16_t address, uint8_t count, uint16_t *values);
    bool writeCoil(uint16_t address, bool value);
    void addOrUpdateRegister(const ModbusRegister &reg, float rawValue);

    // Scale raw value
    float scaleValue(float rawValue, const ModbusRegister &reg);

    // Poll specific arrays
    template <size_t N>
    bool pollRegisterArray(const ModbusRegister (&regArray)[N], const char *groupName = "");

    // Find register config
    const ModbusRegister *findRegisterConfig(uint16_t address);

    // Convert type to string
    String typeToString(RegisterType type);
};

#endif