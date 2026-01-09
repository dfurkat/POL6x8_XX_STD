#include "modbus_handler.h"

// Static instance for callbacks
static ModbusHandler *instance = nullptr;

// Callback functions
static void preTransmissionCallback()
{
    if (instance)
    {
        instance->setTransmitMode(true);
    }
}

static void postTransmissionCallback()
{
    if (instance)
    {
        instance->setTransmitMode(false);
    }
}

void ModbusHandler::begin(SoftwareSerial &serial, uint8_t deRePin, uint8_t slaveId)
{
    this->deRePin = deRePin;
    this->slaveId = slaveId;

    // Store instance pointer for static callbacks
    instance = this;

    modbus.begin(slaveId, serial);

    Serial.print("Modbus handler initialized. Slave ID: ");
    Serial.print(slaveId);
    Serial.print(", DE/RE pin: ");
    Serial.println(deRePin);
    
    modbus.preTransmission(preTransmissionCallback);
    modbus.postTransmission(postTransmissionCallback);
    
    // Reserve space for register values
    registerValues.reserve(100);
}

void ModbusHandler::setTransmitMode(bool transmit)
{
    digitalWrite(deRePin, transmit ? HIGH : LOW);
}

const ModbusRegister *ModbusHandler::findRegisterConfig(uint16_t address)
{
    // Search through all register groups
    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            if (group.registers[i].address == address)
            {
                return &group.registers[i];
            }
        }
    }
    return nullptr;
}

bool ModbusHandler::writeRegister(uint16_t address, float value, RegisterType type)
{
    if (type != REG_HOLDING && type != REG_COIL)
    {
        Serial.println("Error: Can only write to HOLDING registers or COILs");
        return false;
    }

    const ModbusRegister *regConfig = findRegisterConfig(address);

    if (regConfig)
    {
        // Validate value range
        if (value < regConfig->min_value || value > regConfig->max_value)
        {
            Serial.print("Value ");
            Serial.print(value);
            Serial.print(" out of range [");
            Serial.print(regConfig->min_value);
            Serial.print(", ");
            Serial.print(regConfig->max_value);
            Serial.println("]");
            return false;
        }

        // Apply inverse scaling
        float rawValue = (value - regConfig->offset) / regConfig->scale;

        if (type == REG_HOLDING)
        {
            if (regConfig->size == 1)
            {
                uint16_t intValue = static_cast<uint16_t>(rawValue);
                uint8_t result = modbus.writeSingleRegister(address, intValue);
                return result == modbus.ku8MBSuccess;
            }
        }
        else if (type == REG_COIL)
        {
            bool boolValue = rawValue > 0.5;
            uint8_t result = modbus.writeSingleCoil(address, boolValue ? 0xFF00 : 0x0000);
            return result == modbus.ku8MBSuccess;
        }
    }
    else
    {
        // No config found, write raw value
        if (type == REG_HOLDING)
        {
            uint16_t intValue = static_cast<uint16_t>(value);
            uint8_t result = modbus.writeSingleRegister(address, intValue);
            return result == modbus.ku8MBSuccess;
        }
        else if (type == REG_COIL)
        {
            bool boolValue = value > 0.5;
            uint8_t result = modbus.writeSingleCoil(address, boolValue ? 0xFF00 : 0x0000);
            return result == modbus.ku8MBSuccess;
        }
    }

    return false;
}

bool ModbusHandler::pollSingleRegister(uint16_t address, RegisterType type)
{
    const ModbusRegister *regConfig = findRegisterConfig(address);

    if (regConfig)
    {
        // Simplified read - just return true for now
        return true;
    }
    else
    {
        // Try to read anyway
        return true;
    }
}

bool ModbusHandler::pollAllRegisters()
{
    // Simplified implementation - just return true for now
    pollCount++;
    Serial.print("Poll #");
    Serial.println(pollCount);
    return true;
}

bool ModbusHandler::getDataAsJson(JsonDocument &doc)
{
    doc.clear();

    JsonObject root = doc.to<JsonObject>();
    root["timestamp"] = millis();
    root["poll_count"] = pollCount;
    root["error_count"] = errorCount;
    root["status"] = "ok";

    // Add some dummy data for testing
    JsonObject testData = root.createNestedObject("test");
    testData["temperature"] = 25.5;
    testData["pressure"] = 1.2;
    testData["humidity"] = 60.0;

    return true;
}

ModbusRegisterInfo ModbusHandler::getRegisterInfo(uint16_t address)
{
    const ModbusRegister *regConfig = findRegisterConfig(address);

    if (regConfig)
    {
        ModbusRegisterInfo info;
        info.address = regConfig->address;
        info.name = regConfig->name;
        info.description = regConfig->description;
        info.unit = regConfig->unit;
        info.type = regConfig->type;
        info.size = regConfig->size;
        info.scale = regConfig->scale;
        info.offset = regConfig->offset;
        info.min_value = regConfig->min_value;
        info.max_value = regConfig->max_value;
        info.precision = regConfig->precision;
        return info;
    }

    // Return empty info if not found
    ModbusRegisterInfo notFound;
    notFound.address = address;
    notFound.name = "Unknown";
    return notFound;
}

std::vector<ModbusRegisterInfo> ModbusHandler::getAllRegisterInfo()
{
    std::vector<ModbusRegisterInfo> allInfo;

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            const ModbusRegister &reg = group.registers[i];
            ModbusRegisterInfo info;
            info.address = reg.address;
            info.name = reg.name;
            info.description = reg.description;
            info.unit = reg.unit;
            info.type = reg.type;
            info.size = reg.size;
            info.scale = reg.scale;
            info.offset = reg.offset;
            info.min_value = reg.min_value;
            info.max_value = reg.max_value;
            info.precision = reg.precision;
            allInfo.push_back(info);
        }
    }

    return allInfo;
}

RegisterValue ModbusHandler::getRegisterValue(uint16_t address)
{
    RegisterValue notFound;
    notFound.address = address;
    notFound.value = NAN;
    notFound.valid = false;
    return notFound;
}

std::vector<RegisterValue> ModbusHandler::getAllRegisterValues()
{
    return std::vector<RegisterValue>();
}