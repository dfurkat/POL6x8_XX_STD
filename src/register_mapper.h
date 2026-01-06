#ifndef REGISTER_MAPPER_H
#define REGISTER_MAPPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "registers_config.h"

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

class RegisterMapper
{
public:
    void begin();
    RegisterInfo getRegisterInfo(uint16_t address);
    RegisterInfo getRegisterInfo(const char *name);
    std::vector<RegisterInfo> getAllRegisters();
    bool isValidAddress(uint16_t address);
    bool isValidName(const char *name);
    void exportToJson(JsonDocument &doc);

private:
    void buildMaps();
    std::map<uint16_t, RegisterInfo> addressMap;
    std::map<String, RegisterInfo> nameMap;
    bool initialized = false;
};

#endif