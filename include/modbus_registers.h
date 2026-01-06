#ifndef MODBUS_REGISTERS_H
#define MODBUS_REGISTERS_H

// Address ranges from documentation (page 50)
enum RegisterRanges
{
    // General functions
    ADDR_GENERAL_START = 1,
    ADDR_GENERAL_END = 100,

    // Heating circuit 1
    ADDR_HEATING1_START = 101,
    ADDR_HEATING1_END = 180,

    // Heating circuit 2
    ADDR_HEATING2_START = 181,
    ADDR_HEATING2_END = 260,

    // Heating circuit 3
    ADDR_HEATING3_START = 261,
    ADDR_HEATING3_END = 340,

    // DHW circuit 1
    ADDR_DHW1_START = 341,
    ADDR_DHW1_END = 390,

    // DHW circuit 2
    ADDR_DHW2_START = 391,
    ADDR_DHW2_END = 440,

    // Makeup circuit 1
    ADDR_MAKEUP1_START = 441,
    ADDR_MAKEUP1_END = 470,

    // Makeup circuit 2
    ADDR_MAKEUP2_START = 471,
    ADDR_MAKEUP2_END = 500,

    // Makeup circuit 3
    ADDR_MAKEUP3_START = 501,
    ADDR_MAKEUP3_END = 530,

    // Schedules
    ADDR_SCHEDULE1_START = 601,
    ADDR_SCHEDULE1_END = 760,

    ADDR_SCHEDULE2_START = 761,
    ADDR_SCHEDULE2_END = 920,

    ADDR_SCHEDULE3_START = 921,
    ADDR_SCHEDULE3_END = 1080,

    ADDR_SCHEDULE_DHW1_START = 1081,
    ADDR_SCHEDULE_DHW1_END = 1240,

    ADDR_SCHEDULE_DHW2_START = 1241,
    ADDR_SCHEDULE_DHW2_END = 1400
};

// Important register addresses (from page 50-92)
enum ImportantRegisters
{
    // Input registers (3x)
    REG_OUTSIDE_TEMP = 1,
    REG_ROOM_TEMP = 2,
    REG_HEATING1_SUPPLY_TEMP = 107,
    REG_HEATING1_RETURN_TEMP = 108,
    REG_HEATING2_SUPPLY_TEMP = 187,
    REG_HEATING2_RETURN_TEMP = 188,
    REG_HEATING3_SUPPLY_TEMP = 267,
    REG_HEATING3_RETURN_TEMP = 268,
    REG_DHW1_SUPPLY_TEMP = 347,
    REG_DHW1_RETURN_TEMP = 348,
    REG_DHW2_SUPPLY_TEMP = 397,
    REG_DHW2_RETURN_TEMP = 398,

    // Holding registers (4x)
    REG_SYSTEM_HOURS = 1,
    REG_SYSTEM_MINUTES = 2,
    REG_SYSTEM_SECONDS = 3,
    REG_HEATING1_MODE = 101,
    REG_HEATING1_SETPOINT = 107,
    REG_DHW1_MODE = 341,
    REG_DHW1_SETPOINT = 347,

    // Coils (0x)
    COIL_ALARM_CONFIRM = 1,
    COIL_CONTROLLER_RESET = 2,
    COIL_SAVE_SETTINGS = 3,
    COIL_LOAD_SETTINGS = 4,

    // Discrete inputs (1x)
    INPUT_WINTER_SUMMER1 = 11,
    INPUT_ALARM_STATUS = 31
};

// Register value mappings
enum OperationModes
{
    MODE_AUTO = 0,
    MODE_PROTECTION = 1,
    MODE_ECONOMY = 2,
    MODE_COMFORT = 3
};

enum SystemStates
{
    STATE_AUTO = 0,
    STATE_OFF = 1,
    STATE_ON = 2,
    STATE_FROST = 3,
    STATE_OVERHEAT = 4,
    STATE_LOCKED = 5
};

#endif