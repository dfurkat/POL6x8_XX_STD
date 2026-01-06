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

            addressMap[reg.address] = info;
            nameMap[String(reg.name)] = info;
        }
    }
}

RegisterInfo RegisterMapper::getRegisterInfo(uint16_t address)
{
    auto it = addressMap.find(address);
    if (it != addressMap.end())
    {
        return it->second;
    }

    // Return empty info if not found
    RegisterInfo notFound;
    notFound.address = address;
    notFound.name = "Unknown";
    return notFound;
}

RegisterInfo RegisterMapper::getRegisterInfo(const char *name)
{
    auto it = nameMap.find(String(name));
    if (it != nameMap.end())
    {
        return it->second;
    }

    // Return empty info if not found
    RegisterInfo notFound;
    notFound.name = name;
    return notFound;
}

std::vector<RegisterInfo> RegisterMapper::getAllRegisters()
{
    std::vector<RegisterInfo> allRegisters;

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
        const RegisterInfo &info = pair.second;

        JsonObject reg = registers.createNestedObject();
        reg["address"] = info.address;
        reg["name"] = info.name;
        reg["description"] = info.description;
        reg["unit"] = info.unit;
        reg["type"] = info.type;
        reg["size"] = info.size;
        reg["scale"] = info.scale;
        reg["offset"] = info.offset;
        reg["min_value"] = info.min_value;
        reg["max_value"] = info.max_value;
        reg["precision"] = info.precision;
    }
}