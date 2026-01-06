#include "modbus_handler.h"

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
    registerValues.reserve(TOTAL_REGISTER_COUNT);
}

void ModbusHandler::setTransmitMode(bool transmit)
{
    digitalWrite(deRePin, transmit ? HIGH : LOW);
}

float ModbusHandler::scaleValue(float rawValue, const ModbusRegister &reg)
{
    float scaledValue = (rawValue * reg.scale) + reg.offset;

    // Apply limits
    if (scaledValue < reg.min_value)
        scaledValue = reg.min_value;
    if (scaledValue > reg.max_value)
        scaledValue = reg.max_value;

    return scaledValue;
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

float ModbusHandler::readRawRegister(uint16_t address, RegisterType type, uint8_t size)
{
    uint8_t result;
    uint32_t startTime = millis();

    switch (type)
    {
    case REG_INPUT:
        result = modbus.readInputRegisters(address, size);
        break;
    case REG_HOLDING:
        result = modbus.readHoldingRegisters(address, size);
        break;
    case REG_COIL:
        result = modbus.readCoils(address, 1);
        break;
    case REG_DISCRETE:
        result = modbus.readDiscreteInputs(address, 1);
        break;
    default:
        Serial.print("Unknown register type for address 0x");
        Serial.println(address, HEX);
        return NAN;
    }

    if (result == modbus.ku8MBSuccess)
    {
        if (type == REG_COIL || type == REG_DISCRETE)
        {
            return modbus.getResponseBuffer(0);
        }
        else
        {
            if (size == 1)
            {
                return modbus.getResponseBuffer(0);
            }
            else if (size == 2)
            {
                // 32-bit integer
                uint32_t value = ((uint32_t)modbus.getResponseBuffer(0) << 16) |
                                 modbus.getResponseBuffer(1);
                return (float)value;
            }
            else if (size == 4)
            {
                // Float (assuming IEEE 754)
                union
                {
                    uint32_t i;
                    float f;
                } converter;

                converter.i = ((uint32_t)modbus.getResponseBuffer(0) << 16) |
                              modbus.getResponseBuffer(1);
                return converter.f;
            }
        }
    }
    else
    {
        Serial.print("Modbus read error for address 0x");
        Serial.print(address, HEX);
        Serial.print(": ");
        Serial.println(result);
        errorCount++;
    }

    return NAN;
}

float ModbusHandler::readRegisterValue(const ModbusRegister &reg)
{
    float rawValue = readRawRegister(reg.address, reg.type, reg.size);

    if (!isnan(rawValue))
    {
        return scaleValue(rawValue, reg);
    }

    return NAN;
}

template <size_t N>
bool ModbusHandler::pollRegisterArray(const ModbusRegister (&regArray)[N], const char *groupName)
{
    bool success = true;
    int readCount = 0;
    int errorCountLocal = 0;

    if (groupName && strlen(groupName) > 0)
    {
        Serial.print("Polling ");
        Serial.print(groupName);
        Serial.print(" (");
        Serial.print(N);
        Serial.println(" registers)...");
    }

    for (const auto &reg : regArray)
    {
        float value = readRegisterValue(reg);

        if (!isnan(value))
        {
            addOrUpdateRegister(reg, value);
            readCount++;
        }
        else
        {
            success = false;
            errorCountLocal++;

            // Add invalid entry
            RegisterValue regVal;
            regVal.address = reg.address;
            regVal.value = NAN;
            regVal.type = reg.type;
            regVal.size = reg.size;
            regVal.timestamp = millis();
            regVal.valid = false;

            // Update if exists, otherwise add
            bool found = false;
            for (auto &rv : registerValues)
            {
                if (rv.address == reg.address)
                {
                    rv = regVal;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                registerValues.push_back(regVal);
            }
        }

        // Small delay between reads
        delay(10);
    }

    if (groupName && strlen(groupName) > 0)
    {
        Serial.print("  -> Success: ");
        Serial.print(readCount);
        Serial.print("/");
        Serial.print(N);
        Serial.print(", Errors: ");
        Serial.println(errorCountLocal);
    }

    return success;
}

bool ModbusHandler::pollAllRegisters()
{
    bool overallSuccess = true;

    Serial.println("=== Polling all registers ===");

    for (const auto &group : allRegisterGroups)
    {
        bool success = pollRegisterGroup(group.name);
        overallSuccess &= success;
    }

    pollCount++;
    lastPollTime = millis();

    Serial.print("Total polls: ");
    Serial.print(pollCount);
    Serial.print(", Total errors: ");
    Serial.println(errorCount);
    Serial.println("=============================");

    return overallSuccess;
}

bool ModbusHandler::pollRegisterGroup(const char *groupName)
{
    for (const auto &group : allRegisterGroups)
    {
        if (strcmp(group.name, groupName) == 0)
        {
            // Create a pointer to the array and call the template function
            // This is a bit hacky but works
            switch (group.count)
            {
            case sizeof(heatingValves1) / sizeof(ModbusRegister):
                if (group.registers == heatingValves1)
                {
                    return pollRegisterArray(heatingValves1, groupName);
                }
                break;
            case sizeof(heatingPumps1) / sizeof(ModbusRegister):
                if (group.registers == heatingPumps1)
                {
                    return pollRegisterArray(heatingPumps1, groupName);
                }
                break;
                // Add other cases as needed
            }

            // Generic fallback
            bool success = true;
            for (size_t i = 0; i < group.count; i++)
            {
                const ModbusRegister &reg = group.registers[i];
                float value = readRegisterValue(reg);
                if (!isnan(value))
                {
                    addOrUpdateRegister(reg, value);
                }
                else
                {
                    success = false;
                }
                delay(10);
            }
            return success;
        }
    }

    Serial.print("Unknown register group: ");
    Serial.println(groupName);
    return false;
}

bool ModbusHandler::pollSingleRegister(uint16_t address, RegisterType type)
{
    const ModbusRegister *regConfig = findRegisterConfig(address);

    if (regConfig)
    {
        float value = readRegisterValue(*regConfig);

        if (!isnan(value))
        {
            addOrUpdateRegister(*regConfig, value);
            return true;
        }
    }
    else
    {
        // Try to read anyway with default size
        float value = readRawRegister(address, type, 1);

        if (!isnan(value))
        {
            RegisterValue regVal;
            regVal.address = address;
            regVal.value = value;
            regVal.type = type;
            regVal.size = 1;
            regVal.timestamp = millis();
            regVal.valid = true;

            // Update if exists, otherwise add
            bool found = false;
            for (auto &rv : registerValues)
            {
                if (rv.address == address)
                {
                    rv = regVal;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                registerValues.push_back(regVal);
            }

            return true;
        }
    }

    return false;
}

void ModbusHandler::addOrUpdateRegister(const ModbusRegister &reg, float value)
{
    // Check if register already exists
    for (auto &regVal : registerValues)
    {
        if (regVal.address == reg.address)
        {
            regVal.value = value;
            regVal.timestamp = millis();
            regVal.valid = true;
            return;
        }
    }

    // Add new register
    RegisterValue regVal;
    regVal.address = reg.address;
    regVal.value = value;
    regVal.type = reg.type;
    regVal.size = reg.size;
    regVal.timestamp = millis();
    regVal.valid = true;

    registerValues.push_back(regVal);
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
                return writeHoldingRegister(address, intValue);
            }
            else if (regConfig->size == 2)
            {
                uint32_t longValue = static_cast<uint32_t>(rawValue);
                uint16_t values[2] = {(uint16_t)(longValue >> 16), (uint16_t)(longValue & 0xFFFF)};
                return writeMultipleRegisters(address, 2, values);
            }
        }
        else if (type == REG_COIL)
        {
            bool boolValue = rawValue > 0.5;
            return writeCoil(address, boolValue);
        }
    }
    else
    {
        // No config found, write raw value
        if (type == REG_HOLDING)
        {
            uint16_t intValue = static_cast<uint16_t>(value);
            return writeHoldingRegister(address, intValue);
        }
        else if (type == REG_COIL)
        {
            bool boolValue = value > 0.5;
            return writeCoil(address, boolValue);
        }
    }

    return false;
}

bool ModbusHandler::writeHoldingRegister(uint16_t address, uint16_t value)
{
    uint8_t result = modbus.writeSingleRegister(address, value);

    if (result == modbus.ku8MBSuccess)
    {
        Serial.print("Successfully wrote to register 0x");
        Serial.print(address, HEX);
        Serial.print(": ");
        Serial.println(value);
        return true;
    }
    else
    {
        Serial.print("Failed to write to register 0x");
        Serial.print(address, HEX);
        Serial.print(": Error ");
        Serial.println(result);
        errorCount++;
        return false;
    }
}

bool ModbusHandler::writeMultipleRegisters(uint16_t address, uint8_t count, uint16_t *values)
{
    uint8_t result = modbus.writeMultipleRegisters(address, count);

    if (result == modbus.ku8MBSuccess)
    {
        Serial.print("Successfully wrote ");
        Serial.print(count);
        Serial.print(" registers starting at 0x");
        Serial.println(address, HEX);
        return true;
    }
    else
    {
        Serial.print("Failed to write multiple registers at 0x");
        Serial.print(address, HEX);
        Serial.print(": Error ");
        Serial.println(result);
        errorCount++;
        return false;
    }
}

bool ModbusHandler::writeCoil(uint16_t address, bool value)
{
    uint8_t result = modbus.writeSingleCoil(address, value ? 0xFF00 : 0x0000);

    if (result == modbus.ku8MBSuccess)
    {
        Serial.print("Successfully wrote coil 0x");
        Serial.print(address, HEX);
        Serial.print(": ");
        Serial.println(value ? "ON" : "OFF");
        return true;
    }
    else
    {
        Serial.print("Failed to write coil 0x");
        Serial.print(address, HEX);
        Serial.print(": Error ");
        Serial.println(result);
        errorCount++;
        return false;
    }
}

bool ModbusHandler::getDataAsJson(JsonDocument &doc)
{
    doc.clear();

    JsonObject root = doc.to<JsonObject>();
    root["timestamp"] = millis();
    root["poll_count"] = pollCount;
    root["error_count"] = errorCount;
    root["register_count"] = registerValues.size();

    // Add registers by group
    for (const auto &group : allRegisterGroups)
    {
        JsonObject groupObj = root.createNestedObject(group.name);

        for (const auto &regVal : registerValues)
        {
            const ModbusRegister *regConfig = findRegisterConfig(regVal.address);

            if (regConfig)
            {
                // Check if this register belongs to this group
                bool inGroup = false;
                for (size_t i = 0; i < group.count; i++)
                {
                    if (group.registers[i].address == regVal.address)
                    {
                        inGroup = true;
                        break;
                    }
                }

                if (inGroup && regVal.valid)
                {
                    JsonObject regObj = groupObj.createNestedObject(regConfig->name);
                    regObj["value"] = regVal.value;
                    regObj["timestamp"] = regVal.timestamp;
                    regObj["address"] = regVal.address;
                    regObj["type"] = typeToString(regVal.type);
                    regObj["unit"] = regConfig->unit;
                    regObj["valid"] = regVal.valid;
                }
            }
        }
    }

    return !registerValues.empty();
}

RegisterValue ModbusHandler::getRegisterValue(uint16_t address)
{
    for (const auto &regVal : registerValues)
    {
        if (regVal.address == address)
        {
            return regVal;
        }
    }

    RegisterValue notFound;
    notFound.address = address;
    notFound.value = NAN;
    notFound.valid = false;
    return notFound;
}

std::vector<RegisterValue> ModbusHandler::getAllRegisterValues()
{
    return registerValues;
}

RegisterInfo ModbusHandler::getRegisterInfo(uint16_t address)
{
    const ModbusRegister *regConfig = findRegisterConfig(address);

    if (regConfig)
    {
        RegisterInfo info;
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
    RegisterInfo notFound;
    notFound.address = address;
    notFound.name = "Unknown";
    return notFound;
}

std::vector<RegisterInfo> ModbusHandler::getAllRegisterInfo()
{
    std::vector<RegisterInfo> allInfo;

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            const ModbusRegister &reg = group.registers[i];
            RegisterInfo info;
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

String ModbusHandler::typeToString(RegisterType type)
{
    switch (type)
    {
    case REG_COIL:
        return "coil";
    case REG_DISCRETE:
        return "discrete";
    case REG_INPUT:
        return "input";
    case REG_HOLDING:
        return "holding";
    default:
        return "unknown";
    }
}