#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "modbus_registers.h"

// Modbus objects
SoftwareSerial modbusSerial(MODBUS_RX_PIN, MODBUS_TX_PIN);
ModbusMaster node;

// Register storage
uint16_t holdingRegisters[MAX_HOLDING_REGISTERS];
uint16_t inputRegisters[MAX_INPUT_REGISTERS];
uint8_t coils[MAX_COILS];
uint8_t discreteInputs[MAX_DISCRETE_INPUTS];

void initModbus() {
    pinMode(MODBUS_DE_PIN, OUTPUT);
    digitalWrite(MODBUS_DE_PIN, LOW);
    
    modbusSerial.begin(MODBUS_BAUDRATE);
    
    node.begin(MODBUS_SLAVE_ID, modbusSerial);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    
    Serial.println("Modbus RTU initialized");
}

void preTransmission() {
    digitalWrite(MODBUS_DE_PIN, HIGH);
    delay(1);
}

void postTransmission() {
    delay(1);
    digitalWrite(MODBUS_DE_PIN, LOW);
}

bool readHoldingRegisters(uint16_t startAddr, uint16_t count) {
    uint8_t result;
    uint16_t index = 0;
    
    while (count > 0) {
        uint16_t toRead = min(count, (uint16_t)125);  // MODBUS max per read
        
        result = node.readHoldingRegisters(startAddr + index, toRead);
        
        if (result == node.ku8MBSuccess) {
            for (uint16_t i = 0; i < toRead; i++) {
                if (startAddr + index + i < MAX_HOLDING_REGISTERS) {
                    holdingRegisters[startAddr + index + i] = node.getResponseBuffer(i);
                }
            }
            
            #ifdef MODBUS_DEBUG
            Serial.printf("Read holding registers %d-%d: OK\n", 
                         startAddr + index, startAddr + index + toRead - 1);
            #endif
        } else {
            Serial.printf("Modbus error reading holding registers %d: 0x%02X\n", 
                         startAddr + index, result);
            return false;
        }
        
        index += toRead;
        count -= toRead;
    }
    
    return true;
}

bool readInputRegisters(uint16_t startAddr, uint16_t count) {
    uint8_t result;
    uint16_t index = 0;
    
    while (count > 0) {
        uint16_t toRead = min(count, (uint16_t)125);
        
        result = node.readInputRegisters(startAddr + index, toRead);
        
        if (result == node.ku8MBSuccess) {
            for (uint16_t i = 0; i < toRead; i++) {
                if (startAddr + index + i < MAX_INPUT_REGISTERS) {
                    inputRegisters[startAddr + index + i] = node.getResponseBuffer(i);
                }
            }
            
            #ifdef MODBUS_DEBUG
            Serial.printf("Read input registers %d-%d: OK\n", 
                         startAddr + index, startAddr + index + toRead - 1);
            #endif
        } else {
            Serial.printf("Modbus error reading input registers %d: 0x%02X\n", 
                         startAddr + index, result);
            return false;
        }
        
        index += toRead;
        count -= toRead;
    }
    
    return true;
}

bool readCoils(uint16_t startAddr, uint16_t count) {
    uint8_t result = node.readCoils(startAddr, count);
    
    if (result == node.ku8MBSuccess) {
        for (uint16_t i = 0; i < count; i++) {
            if (startAddr + i < MAX_COILS) {
                coils[startAddr + i] = node.getResponseBuffer(i);
            }
        }
        return true;
    }
    
    Serial.printf("Modbus error reading coils %d: 0x%02X\n", startAddr, result);
    return false;
}

bool writeSingleRegister(uint16_t regAddr, uint16_t value) {
    uint8_t result = node.writeSingleRegister(regAddr, value);
    
    if (result == node.ku8MBSuccess) {
        Serial.printf("Write register %d = %d: OK\n", regAddr, value);
        holdingRegisters[regAddr] = value;  // Update cache
        return true;
    }
    
    Serial.printf("Modbus error writing register %d: 0x%02X\n", regAddr, result);
    return false;
}

bool writeSingleCoil(uint16_t coilAddr, uint8_t value) {
    uint8_t result = node.writeSingleCoil(coilAddr, value);
    
    if (result == node.ku8MBSuccess) {
        Serial.printf("Write coil %d = %d: OK\n", coilAddr, value);
        coils[coilAddr] = value;  // Update cache
        return true;
    }
    
    Serial.printf("Modbus error writing coil %d: 0x%02X\n", coilAddr, result);
    return false;
}

int16_t getRegisterValue(uint16_t regAddr, bool isInput) {
    if (isInput) {
        if (regAddr < MAX_INPUT_REGISTERS) {
            return (int16_t)inputRegisters[regAddr];
        }
    } else {
        if (regAddr < MAX_HOLDING_REGISTERS) {
            return (int16_t)holdingRegisters[regAddr];
        }
    }
    return -32768;  // Error value
}

uint8_t getDiscreteInput(uint16_t inputAddr) {
    if (inputAddr < MAX_DISCRETE_INPUTS) {
        return discreteInputs[inputAddr];
    }
    return 0;
}