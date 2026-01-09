#ifndef REGISTER_MAPPER_H
#define REGISTER_MAPPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "registers_config.h"

class RegisterMapper
{
public:
    void begin();
    ModbusRegister getRegisterInfo(uint16_t address);
    ModbusRegister getRegisterInfo(const char *name);
    std::vector<ModbusRegister> getAllRegisters();
    bool isValidAddress(uint16_t address);
    bool isValidName(const char *name);
    void exportToJson(JsonDocument &doc);

private:
    void buildMaps();
    std::map<uint16_t, ModbusRegister> addressMap;
    std::map<String, ModbusRegister> nameMap;
    bool initialized = false;
};

#endif