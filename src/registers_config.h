#ifndef REGISTERS_CONFIG_H
#define REGISTERS_CONFIG_H

#include <Arduino.h>

// Типы регистров
enum RegisterType
{
    REG_COIL = 0,     // Coil (дискретный)
    REG_DISCRETE = 1, // Discrete Input (дискретный вход)
    REG_INPUT = 3,    // Input Register (только чтение)
    REG_HOLDING = 4   // Holding Register (чтение/запись)
};

// Структура для конфигурации регистра
struct ModbusRegister
{
    uint16_t address;
    const char *name;
    const char *description;
    const char *unit;
    RegisterType type;
    uint8_t size; // 1 = 16-bit, 2 = 32-bit, 4 = float
    float scale;  // Множитель для преобразования
    float offset; // Смещение
    float min_value;
    float max_value;
    uint8_t precision; // Количество знаков после запятой
};

// Смещения для контуров
const uint16_t HEATING_CIRCUIT_OFFSET = 0x0080;
const uint16_t DHW_CIRCUIT_OFFSET = 0x0080;

// ========== КЛАПАНЫ ОТОПЛЕНИЯ ==========

// Регистры для клапанов отопления (Circuit 1)
const ModbusRegister heatingValves1[] = {
    // Клапан А - Сигнал управления (0-100%)
    {0x0111, "heating1_valveA_signal", "Сигнал клапана A КО 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},

    // Клапан B - Сигнал управления (0-100%)
    {0x0112, "heating1_valveB_signal", "Сигнал клапана B КО 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},

    // ПИД параметры клапана А
    {0x0134, "heating1_valveA_P", "Коэффициент P клапана A", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0135, "heating1_valveA_I", "Коэффициент I клапана A", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0136, "heating1_valveA_D", "Коэффициент D клапана A", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},

    // ПИД параметры клапана B
    {0x0137, "heating1_valveB_P", "Коэффициент P клапана B", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0138, "heating1_valveB_I", "Коэффициент I клапана B", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0139, "heating1_valveB_D", "Коэффициент D клапана B", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},

    // Время работы 3-точечных клапанов
    {0x0140, "heating1_valveA_open_time", "Время открытия клапана A", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x0142, "heating1_valveA_close_time", "Время закрытия клапана A", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x0144, "heating1_valveB_open_time", "Время открытия клапана B", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x0146, "heating1_valveB_close_time", "Время закрытия клапана B", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},

    // Дискретные выходы для управления
    {0x0106, "heating1_valveA_open_cmd", "Команда открытия клапана A", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0107, "heating1_valveA_close_cmd", "Команда закрытия клапана A", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0108, "heating1_valveB_open_cmd", "Команда открытия клапана B", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0109, "heating1_valveB_close_cmd", "Команда закрытия клапана B", "", REG_COIL, 1, 1, 0, 0, 1, 0},

    // Статусы клапанов
    {0x0115, "heating1_valveA_status", "Статус клапана A", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x0116, "heating1_valveB_status", "Статус клапана B", "", REG_INPUT, 1, 1, 0, 0, 3, 0}};

// ========== НАСОСЫ ОТОПЛЕНИЯ ==========

const ModbusRegister heatingPumps1[] = {
    // Насос A - Сигнал управления (0-100%)
    {0x0113, "heating1_pumpA_signal", "Сигнал насоса A КО 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},

    // Насос B - Сигнал управления (0-100%)
    {0x0114, "heating1_pumpB_signal", "Сигнал насоса B КО 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},

    // ПИД параметры насосов
    {0x015A, "heating1_pumps_P", "Коэффициент P насосов", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x015B, "heating1_pumps_I", "Коэффициент I насосов", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x015C, "heating1_pumps_D", "Коэффициент D насосов", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},

    // Ограничения производительности насосов
    {0x015D, "heating1_pumps_min", "Минимальная производительность", "%", REG_HOLDING, 1, 0.1, 0, 0, 100, 1},
    {0x015E, "heating1_pumps_max", "Максимальная производительность", "%", REG_HOLDING, 1, 0.1, 0, 0, 100, 1},

    // Уставка давления
    {0x0158, "heating1_pressure_setpoint", "Уставка давления", "бар", REG_HOLDING, 1, 0.01, 0, 0, 100, 2},

    // Уставка скорости насоса
    {0x0159, "heating1_pump_speed_setpoint", "Уставка скорости насоса", "%", REG_HOLDING, 1, 0.1, 0, 0, 100, 1},

    // Переключение насосов по времени
    {0x0153, "heating1_pumps_switch_date", "Дата переключения насосов", "", REG_HOLDING, 2, 1, 0, 0, 0xFFFFFFFF, 0},
    {0x0155, "heating1_pumps_switch_time", "Время переключения насосов", "", REG_HOLDING, 2, 1, 0, 0, 0xFFFFFFFF, 0},

    // Толчок насоса
    {0x0102, "heating1_pump_kick_interval", "Интервал толчка насоса", "час", REG_HOLDING, 1, 1, 0, 0, 1000, 0},
    {0x0103, "heating1_pump_kick_duration", "Длительность толчка насоса", "сек", REG_HOLDING, 1, 1, 0, 0, 1000, 0},

    // Дискретные выходы насосов
    {0x010A, "heating1_pumpA_cmd", "Команда насоса A", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x010B, "heating1_pumpB_cmd", "Команда насоса B", "", REG_COIL, 1, 1, 0, 0, 1, 0},

    // Статусы насосов
    {0x0117, "heating1_pumpA_status", "Статус насоса A", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x0118, "heating1_pumpB_status", "Статус насоса B", "", REG_INPUT, 1, 1, 0, 0, 3, 0}};

// ========== КЛАПАНЫ ГВС ==========

const ModbusRegister dhwValves[] = {
    // ГВС 1
    {0x0351, "dhw1_valve_signal", "Сигнал клапана ГВС 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x0352, "dhw1_valve_P", "Коэффициент P клапана ГВС 1", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0353, "dhw1_valve_I", "Коэффициент I клапана ГВС 1", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0354, "dhw1_valve_D", "Коэффициент D клапана ГВС 1", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0355, "dhw1_valve_open_time", "Время открытия клапана", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x0357, "dhw1_valve_close_time", "Время закрытия клапана", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x0358, "dhw1_valve_cmd_open", "Команда открытия клапана ГВС 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0359, "dhw1_valve_cmd_close", "Команда закрытия клапана ГВС 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x035A, "dhw1_valve_status", "Статус клапана ГВС 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // ГВС 2
    {0x03D1, "dhw2_valve_signal", "Сигнал клапана ГВС 2", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x03A2, "dhw2_valve_P", "Коэффициент P клапана ГВС 2", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03A3, "dhw2_valve_I", "Коэффициент I клапана ГВС 2", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03A4, "dhw2_valve_D", "Коэффициент D клапана ГВС 2", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03A5, "dhw2_valve_open_time", "Время открытия клапана ГВС 2", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x03A7, "dhw2_valve_close_time", "Время закрытия клапана ГВС 2", "сек", REG_HOLDING, 2, 1, 0, 2, 900, 0},
    {0x03A8, "dhw2_valve_cmd_open", "Команда открытия клапана ГВС 2", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x03A9, "dhw2_valve_cmd_close", "Команда закрытия клапана ГВС 2", "", REG_COIL, 1, 1, 0, 0, 1, 0}};

// ========== НАСОСЫ ГВС ==========

const ModbusRegister dhwPumps[] = {
    // ГВС 1
    {0x035B, "dhw1_pumpA_signal", "Сигнал насоса A ГВС 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x035C, "dhw1_pumpB_signal", "Сигнал насоса B ГВС 1", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x03B0, "dhw1_pumps_P", "Коэффициент P насосов ГВС 1", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03B1, "dhw1_pumps_I", "Коэффициент I насосов ГВС 1", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03B2, "dhw1_pumps_D", "Коэффициент D насосов ГВС 1", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03B3, "dhw1_pumpA_cmd", "Команда насоса A ГВС 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x03B4, "dhw1_pumpB_cmd", "Команда насоса B ГВС 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x03B5, "dhw1_pumpA_status", "Статус насоса A ГВС 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x03B6, "dhw1_pumpB_status", "Статус насоса B ГВС 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // ГВС 2
    {0x0402, "dhw2_pumpA_signal", "Сигнал насоса A ГВС 2", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x0403, "dhw2_pumpB_signal", "Сигнал насоса B ГВС 2", "%", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x03C0, "dhw2_pumps_P", "Коэффициент P насосов ГВС 2", "", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03C1, "dhw2_pumps_I", "Коэффициент I насосов ГВС 2", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03C2, "dhw2_pumps_D", "Коэффициент D насосов ГВС 2", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x03C3, "dhw2_pumpA_cmd", "Команда насоса A ГВС 2", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x03C4, "dhw2_pumpB_cmd", "Команда насоса B ГВС 2", "", REG_COIL, 1, 1, 0, 0, 1, 0}};

// ========== КЛАПАНЫ ПОДПИТКИ ==========

const ModbusRegister makeupValves[] = {
    // Контур подпитки 1
    {0x0443, "makeup1_valve_cmd", "Команда клапана подпитки 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0460, "makeup1_valve_delay", "Задержка клапана подпитки 1", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0461, "makeup1_valve_status", "Статус клапана подпитки 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // Контур подпитки 2
    {0x0473, "makeup2_valve_cmd", "Команда клапана подпитки 2", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0490, "makeup2_valve_delay", "Задержка клапана подпитки 2", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0491, "makeup2_valve_status", "Статус клапана подпитки 2", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // Контур подпитки 3
    {0x0503, "makeup3_valve_cmd", "Команда клапана подпитки 3", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0520, "makeup3_valve_delay", "Задержка клапана подпитки 3", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0521, "makeup3_valve_status", "Статус клапана подпитки 3", "", REG_INPUT, 1, 1, 0, 0, 3, 0}};

// ========== НАСОСЫ ПОДПИТКИ ==========

const ModbusRegister makeupPumps[] = {
    // Контур подпитки 1
    {0x0444, "makeup1_pumpA_cmd", "Команда насоса A подпитки 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0445, "makeup1_pumpB_cmd", "Команда насоса B подпитки 1", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0462, "makeup1_pump_delay", "Задержка насоса подпитки 1", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0463, "makeup1_weekly_cycles", "Количество включений в неделю", "", REG_HOLDING, 1, 1, 0, 0, 100, 0},
    {0x0464, "makeup1_pumpA_status", "Статус насоса A подпитки 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x0465, "makeup1_pumpB_status", "Статус насоса B подпитки 1", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // Контур подпитки 2
    {0x0474, "makeup2_pumpA_cmd", "Команда насоса A подпитки 2", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0475, "makeup2_pumpB_cmd", "Команда насоса B подпитки 2", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0492, "makeup2_pump_delay", "Задержка насоса подпитки 2", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0493, "makeup2_pumpA_status", "Статус насоса A подпитки 2", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x0494, "makeup2_pumpB_status", "Статус насоса B подпитки 2", "", REG_INPUT, 1, 1, 0, 0, 3, 0},

    // Контур подпитки 3
    {0x0504, "makeup3_pumpA_cmd", "Команда насоса A подпитки 3", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0505, "makeup3_pumpB_cmd", "Команда насоса B подпитки 3", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0522, "makeup3_pump_delay", "Задержка насоса подпитки 3", "сек", REG_HOLDING, 1, 1, 0, 0, 3600, 0},
    {0x0523, "makeup3_pumpA_status", "Статус насоса A подпитки 3", "", REG_INPUT, 1, 1, 0, 0, 3, 0},
    {0x0524, "makeup3_pumpB_status", "Статус насоса B подпитки 3", "", REG_INPUT, 1, 1, 0, 0, 3, 0}};

// ========== АВАРИИ И ДИАГНОСТИКА ==========

const ModbusRegister alarmRegisters[] = {
    // Контур отопления 1 аварии
    {0x0104, "heating1_alarms", "Аварии КО 1", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Контур ГВС 1 аварии
    {0x0344, "dhw1_alarms", "Аварии ГВС 1", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Контур подпитки аварии
    {0x0441, "makeup1_alarms", "Аварии подпитки 1", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Контур отопления 2 аварии
    {0x0184, "heating2_alarms", "Аварии КО 2", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Контур отопления 3 аварии
    {0x0204, "heating3_alarms", "Аварии КО 3", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Контур ГВС 2 аварии
    {0x03C4, "dhw2_alarms", "Аварии ГВС 2", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},

    // Общие аварии системы
    {0x0001, "system_alarms", "Общие аварии системы", "", REG_INPUT, 1, 1, 0, 0, 65535, 0}};

// ========== ДАТЧИКИ ДАВЛЕНИЯ ==========

const ModbusRegister pressureSensors[] = {
    // Давление подачи
    {0x0200, "heating1_supply_pressure", "Давление подачи КО 1", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},
    {0x0201, "heating1_return_pressure", "Давление обратки КО 1", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},

    // Перепад давления насосов
    {0x01D0, "heating1_pump_diff_pressure", "Перепад давления насосов КО 1", "бар", REG_INPUT, 1, 0.01, 0, 0, 10, 2},

    // Давления других контуров
    {0x0280, "heating2_supply_pressure", "Давление подачи КО 2", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},
    {0x0281, "heating2_return_pressure", "Давление обратки КО 2", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},
    {0x0300, "heating3_supply_pressure", "Давление подачи КО 3", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},
    {0x0301, "heating3_return_pressure", "Давление обратки КО 3", "бар", REG_INPUT, 1, 0.01, 0, 0, 100, 2},

    // Прессостаты подпитки
    {0x0442, "makeup1_pressostat", "Прессостат подпитки 1", "", REG_INPUT, 1, 1, 0, 0, 1, 0},
    {0x0464, "makeup1_pressostat_delay", "Задержка прессостата", "сек", REG_HOLDING, 1, 1, 0, 0, 60, 0},
    {0x0465, "makeup1_pressure_setpoint", "Уставка давления подпитки", "бар", REG_HOLDING, 1, 0.01, 0, 0, 60, 2},
    {0x0466, "makeup1_pressure_hysteresis", "Гистерезис подпитки", "бар", REG_HOLDING, 1, 0.01, 0, 0, 5, 2},

    // Прессостаты других контуров подпитки
    {0x0472, "makeup2_pressostat", "Прессостат подпитки 2", "", REG_INPUT, 1, 1, 0, 0, 1, 0},
    {0x0502, "makeup3_pressostat", "Прессостат подпитки 3", "", REG_INPUT, 1, 1, 0, 0, 1, 0}};

// ========== ТЕМПЕРАТУРНЫЕ ДАТЧИКИ ==========

const ModbusRegister temperatureSensors[] = {
    // Температуры КО 1
    {0x0202, "heating1_supply_temp", "Температура подачи КО 1", "°C", REG_INPUT, 1, 0.1, 0, -50, 150, 1},
    {0x0203, "heating1_return_temp", "Температура обратки КО 1", "°C", REG_INPUT, 1, 0.1, 0, -50, 150, 1},
    {0x0204, "heating1_outdoor_temp", "Температура наружная", "°C", REG_INPUT, 1, 0.1, 0, -50, 50, 1},
    {0x0205, "heating1_room_temp", "Температура помещения", "°C", REG_INPUT, 1, 0.1, 0, 0, 50, 1},

    // Температуры ГВС 1
    {0x0360, "dhw1_supply_temp", "Температура подачи ГВС 1", "°C", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x0361, "dhw1_return_temp", "Температура обратки ГВС 1", "°C", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x0362, "dhw1_storage_temp", "Температура в бойлере", "°C", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x0363, "dhw1_target_temp", "Целевая температура ГВС", "°C", REG_HOLDING, 1, 0.1, 0, 10, 80, 1},

    // Температуры ГВС 2
    {0x03E0, "dhw2_supply_temp", "Температура подачи ГВС 2", "°C", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x03E1, "dhw2_return_temp", "Температура обратки ГВС 2", "°C", REG_INPUT, 1, 0.1, 0, 0, 100, 1},
    {0x03E2, "dhw2_target_temp", "Целевая температура ГВС 2", "°C", REG_HOLDING, 1, 0.1, 0, 10, 80, 1}};

// ========== КОМАНДЫ УПРАВЛЕНИЯ ==========

const ModbusRegister commandRegisters[] = {
    // Режимы работы (запись)
    {0x0101, "heating1_mode_cmd", "Команда режима КО 1", "", REG_HOLDING, 1, 1, 0, 0, 3, 0},
    // 0=Авто, 1=Защита, 2=Экономия, 3=Комфорт

    // Уставки температуры (запись)
    {0x0104, "heating1_comfort_setpoint_cmd", "Уставка комфорт КО 1", "°C", REG_HOLDING, 1, 0.1, 0, 1, 35, 1},
    {0x0105, "heating1_eco_setpoint_cmd", "Уставка эконом КО 1", "°C", REG_HOLDING, 1, 0.1, 0, 5, 35, 1},
    {0x0106, "heating1_frost_setpoint_cmd", "Уставка замерзания КО 1", "°C", REG_HOLDING, 1, 0.1, 0, 1, 35, 1},

    // Макс. температура подачи
    {0x0108, "heating1_max_supply_temp_cmd", "Макс. температура подачи", "°C", REG_HOLDING, 1, 0.1, 0, 0, 150, 1},

    // Дифференциал перегрева
    {0x0109, "heating1_overheat_diff_cmd", "Дифференциал перегрева", "°C", REG_HOLDING, 1, 0.1, 0, 0, 15, 1},

    // Ограничение температуры обратки
    {0x0126, "heating1_return_limit_P", "Коэффициент P огр. обратки", "", REG_HOLDING, 1, 1, 0, -999, 0, 0},
    {0x0127, "heating1_return_limit_I", "Коэффициент I огр. обратки", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},
    {0x0128, "heating1_return_limit_D", "Коэффициент D огр. обратки", "сек", REG_HOLDING, 1, 1, 0, 0, 999, 0},

    // Режимы работы других контуров
    {0x0181, "heating2_mode_cmd", "Команда режима КО 2", "", REG_HOLDING, 1, 1, 0, 0, 3, 0},
    {0x0201, "heating3_mode_cmd", "Команда режима КО 3", "", REG_HOLDING, 1, 1, 0, 0, 3, 0},
    {0x0341, "dhw1_mode_cmd", "Команда режима ГВС 1", "", REG_HOLDING, 1, 1, 0, 0, 3, 0},
    {0x03C1, "dhw2_mode_cmd", "Команда режима ГВС 2", "", REG_HOLDING, 1, 1, 0, 0, 3, 0},

    // Общие команды системы
    {0x0002, "system_reset_cmd", "Команда сброса системы", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0003, "system_emergency_stop", "Аварийная остановка", "", REG_COIL, 1, 1, 0, 0, 1, 0},
    {0x0004, "system_maintenance_mode", "Режим обслуживания", "", REG_COIL, 1, 1, 0, 0, 1, 0}};

// ========== СЧЕТЧИКИ И СТАТИСТИКА ==========

const ModbusRegister counters[] = {
    // Счетчики времени работы
    {0x1000, "heating1_pumpA_hours", "Часы работы насоса A КО 1", "ч", REG_INPUT, 2, 1, 0, 0, 999999, 0},
    {0x1002, "heating1_pumpB_hours", "Часы работы насоса B КО 1", "ч", REG_INPUT, 2, 1, 0, 0, 999999, 0},
    {0x1004, "heating1_valveA_hours", "Часы работы клапана A КО 1", "ч", REG_INPUT, 2, 1, 0, 0, 999999, 0},
    {0x1006, "heating1_valveB_hours", "Часы работы клапана B КО 1", "ч", REG_INPUT, 2, 1, 0, 0, 999999, 0},

    // Счетчики энергии
    {0x1100, "heating1_energy_consumed", "Потребленная энергия КО 1", "кВт·ч", REG_INPUT, 2, 0.1, 0, 0, 999999, 1},
    {0x1102, "dhw1_energy_consumed", "Потребленная энергия ГВС 1", "кВт·ч", REG_INPUT, 2, 0.1, 0, 0, 999999, 1},
    {0x1104, "system_total_energy", "Общая потребленная энергия", "кВт·ч", REG_INPUT, 2, 0.1, 0, 0, 9999999, 1},

    // Счетчики воды
    {0x1200, "makeup1_water_total", "Всего воды подпитки 1", "м³", REG_INPUT, 2, 0.001, 0, 0, 999999, 3},
    {0x1202, "makeup1_water_today", "Вода подпитки 1 за сегодня", "м³", REG_INPUT, 2, 0.001, 0, 0, 999, 3},

    // Статистика ошибок
    {0x1300, "system_error_count", "Количество ошибок системы", "", REG_INPUT, 2, 1, 0, 0, 999999, 0},
    {0x1302, "heating1_error_count", "Количество ошибок КО 1", "", REG_INPUT, 2, 1, 0, 0, 99999, 0},
    {0x1304, "last_error_code", "Код последней ошибки", "", REG_INPUT, 1, 1, 0, 0, 65535, 0},
    {0x1305, "last_error_timestamp", "Время последней ошибки", "", REG_INPUT, 2, 1, 0, 0, 4294967295.0f, 0}};

// ========== СИСТЕМНАЯ ИНФОРМАЦИЯ ==========

const ModbusRegister systemInfo[] = {
    // Информация об устройстве
    {0xF000, "device_model", "Модель устройства", "", REG_INPUT, 2, 1, 0, 0, 0xFFFFFFFF, 0},
    {0xF002, "device_serial", "Серийный номер", "", REG_INPUT, 2, 1, 0, 0, 0xFFFFFFFF, 0},
    {0xF004, "firmware_version", "Версия прошивки", "", REG_INPUT, 2, 0.01, 0, 0, 999.99, 2},
    {0xF006, "hardware_version", "Версия железа", "", REG_INPUT, 1, 0.1, 0, 0, 99.9, 1},

    // Статус системы
    {0xF010, "system_uptime", "Время работы системы", "ч", REG_INPUT, 2, 1, 0, 0, 999999, 0},
    {0xF012, "system_datetime", "Дата и время системы", "", REG_HOLDING, 4, 1, 0, 0, 0xFFFFFFFF, 0},
    {0xF016, "system_temperature", "Температура платы", "°C", REG_INPUT, 1, 0.1, 0, -40, 125, 1},
    {0xF017, "system_voltage", "Напряжение питания", "В", REG_INPUT, 1, 0.01, 0, 0, 30, 2},

    // Настройки связи
    {0xF020, "modbus_address", "Адрес Modbus", "", REG_HOLDING, 1, 1, 0, 1, 247, 0},
    {0xF021, "modbus_baudrate", "Скорость Modbus", "бод", REG_HOLDING, 1, 1, 0, 0, 7, 0},
    {0xF022, "modbus_parity", "Четность Modbus", "", REG_HOLDING, 1, 1, 0, 0, 2, 0},
    {0xF023, "modbus_stopbits", "Стоп-биты Modbus", "", REG_HOLDING, 1, 1, 0, 0, 2, 0}};

// ========== ВСЕ РЕГИСТРЫ (для итерации) ==========

struct RegisterGroup
{
    const char *name;
    const ModbusRegister *registers;
    size_t count;
};

const RegisterGroup allRegisterGroups[] = {
    {"heating_valves_circuit1", heatingValves1, sizeof(heatingValves1) / sizeof(ModbusRegister)},
    {"heating_pumps_circuit1", heatingPumps1, sizeof(heatingPumps1) / sizeof(ModbusRegister)},
    {"dhw_valves", dhwValves, sizeof(dhwValves) / sizeof(ModbusRegister)},
    {"dhw_pumps", dhwPumps, sizeof(dhwPumps) / sizeof(ModbusRegister)},
    {"makeup_valves", makeupValves, sizeof(makeupValves) / sizeof(ModbusRegister)},
    {"makeup_pumps", makeupPumps, sizeof(makeupPumps) / sizeof(ModbusRegister)},
    {"alarms", alarmRegisters, sizeof(alarmRegisters) / sizeof(ModbusRegister)},
    {"pressure_sensors", pressureSensors, sizeof(pressureSensors) / sizeof(ModbusRegister)},
    {"temperature_sensors", temperatureSensors, sizeof(temperatureSensors) / sizeof(ModbusRegister)},
    {"commands", commandRegisters, sizeof(commandRegisters) / sizeof(ModbusRegister)},
    {"counters", counters, sizeof(counters) / sizeof(ModbusRegister)},
    {"system_info", systemInfo, sizeof(systemInfo) / sizeof(ModbusRegister)}};

// Общее количество регистров
const size_t TOTAL_REGISTER_COUNT =
    sizeof(heatingValves1) / sizeof(ModbusRegister) +
    sizeof(heatingPumps1) / sizeof(ModbusRegister) +
    sizeof(dhwValves) / sizeof(ModbusRegister) +
    sizeof(dhwPumps) / sizeof(ModbusRegister) +
    sizeof(makeupValves) / sizeof(ModbusRegister) +
    sizeof(makeupPumps) / sizeof(ModbusRegister) +
    sizeof(alarmRegisters) / sizeof(ModbusRegister) +
    sizeof(pressureSensors) / sizeof(ModbusRegister) +
    sizeof(temperatureSensors) / sizeof(ModbusRegister) +
    sizeof(commandRegisters) / sizeof(ModbusRegister) +
    sizeof(counters) / sizeof(ModbusRegister) +
    sizeof(systemInfo) / sizeof(ModbusRegister);

#endif