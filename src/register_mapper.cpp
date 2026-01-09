#include "register_mapper.h"

void RegisterMapper::begin()
{
    if (!initialized)
    {
        buildMaps();
        initialized = true;

        Serial.print("Register mapper initialized. Total registers: ");
        Serial.println(addressMap.size());
    }
}

void RegisterMapper::buildMaps()
{
    // Build maps from all register groups
    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            const ModbusRegister &reg = group.registers[i];
            addressMap[reg.address] = reg;
            nameMap[String(reg.name)] = reg;
        }
    }
}

ModbusRegister RegisterMapper::getRegisterInfo(uint16_t address)
{
    auto it = addressMap.find(address);
    if (it != addressMap.end())
    {
        return it->second;
    }

    // Return empty info if not found
    ModbusRegister notFound;
    notFound.address = address;
    notFound.name = "Unknown";
    return notFound;
}

ModbusRegister RegisterMapper::getRegisterInfo(const char *name)
{
    auto it = nameMap.find(String(name));
    if (it != nameMap.end())
    {
        return it->second;
    }

    // Return empty info if not found
    ModbusRegister notFound;
    notFound.name = name;
    return notFound;
}

std::vector<ModbusRegister> RegisterMapper::getAllRegisters()
{
    std::vector<ModbusRegister> allRegisters;

    for (const auto &pair : addressMap)
    {
        allRegisters.push_back(pair.second);
    }

    return allRegisters;
}

bool RegisterMapper::isValidAddress(uint16_t address)
{
    return addressMap.find(address) != addressMap.end();
}

bool RegisterMapper::isValidName(const char *name)
{
    return nameMap.find(String(name)) != nameMap.end();
}

void RegisterMapper::exportToJson(JsonDocument &doc)
{
    doc.clear();

    JsonObject root = doc.to<JsonObject>();
    root["total_registers"] = addressMap.size();
    root["timestamp"] = millis();

    JsonArray registers = root.createNestedArray("registers");

    for (const auto &pair : addressMap)
    {
        const ModbusRegister &reg = pair.second;

        JsonObject regObj = registers.createNestedObject();
        regObj["address"] = reg.address;
        regObj["name"] = reg.name;
        regObj["description"] = reg.description;
        regObj["unit"] = reg.unit;
        regObj["type"] = reg.type;
        regObj["size"] = reg.size;
        regObj["scale"] = reg.scale;
        regObj["offset"] = reg.offset;
        regObj["min_value"] = reg.min_value;
        regObj["max_value"] = reg.max_value;
        regObj["precision"] = reg.precision;
    }
}