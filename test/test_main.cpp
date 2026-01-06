#include <Arduino.h>
#include <unity.h>
#include "registers_config.h"
#include "modbus_handler.h"
#include "register_mapper.h"

// Test fixtures
ModbusHandler modbusHandler;
RegisterMapper registerMapper;
SoftwareSerial testSerial(D5, D6);

void setUp()
{
    Serial.begin(115200);
    delay(100);
}

void tearDown()
{
    // Clean up after each test
}

void test_register_count()
{
    Serial.println("Testing register counts...");

    TEST_ASSERT_EQUAL(12, sizeof(allRegisterGroups) / sizeof(RegisterGroup));

    size_t total = 0;
    for (const auto &group : allRegisterGroups)
    {
        total += group.count;
    }

    TEST_ASSERT_EQUAL(TOTAL_REGISTER_COUNT, total);
    Serial.print("Total registers: ");
    Serial.println(total);
}

void test_register_address_uniqueness()
{
    Serial.println("Testing register address uniqueness...");

    std::vector<uint16_t> addresses;

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            uint16_t addr = group.registers[i].address;

            // Check for duplicates
            for (uint16_t existing : addresses)
            {
                if (existing == addr)
                {
                    Serial.print("Duplicate address found: 0x");
                    Serial.println(addr, HEX);
                    TEST_FAIL();
                }
            }

            addresses.push_back(addr);
        }
    }

    TEST_PASS();
}

void test_register_name_uniqueness()
{
    Serial.println("Testing register name uniqueness...");

    std::vector<String> names;

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            String name = String(group.registers[i].name);

            // Check for duplicates
            for (const String &existing : names)
            {
                if (existing == name)
                {
                    Serial.print("Duplicate name found: ");
                    Serial.println(name);
                    TEST_FAIL();
                }
            }

            names.push_back(name);
        }
    }

    TEST_PASS();
}

void test_register_types()
{
    Serial.println("Testing register types...");

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            RegisterType type = group.registers[i].type;
            TEST_ASSERT_TRUE(type >= 0 && type <= 4);
        }
    }

    TEST_PASS();
}

void test_register_sizes()
{
    Serial.println("Testing register sizes...");

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            uint8_t size = group.registers[i].size;
            TEST_ASSERT_TRUE(size == 1 || size == 2 || size == 4);
        }
    }

    TEST_PASS();
}

void test_register_ranges()
{
    Serial.println("Testing register ranges...");

    for (const auto &group : allRegisterGroups)
    {
        for (size_t i = 0; i < group.count; i++)
        {
            float min = group.registers[i].min_value;
            float max = group.registers[i].max_value;
            TEST_ASSERT_TRUE(min <= max);
        }
    }

    TEST_PASS();
}

void test_register_mapper()
{
    Serial.println("Testing register mapper...");

    registerMapper.begin();

    // Test known registers
    RegisterInfo info = registerMapper.getRegisterInfo(0x0111);
    TEST_ASSERT_EQUAL_STRING("heating1_valveA_signal", info.name);
    TEST_ASSERT_EQUAL(REG_INPUT, info.type);

    info = registerMapper.getRegisterInfo(0x0101);
    TEST_ASSERT_EQUAL_STRING("heating1_mode_cmd", info.name);
    TEST_ASSERT_EQUAL(REG_HOLDING, info.type);

    // Test invalid register
    info = registerMapper.getRegisterInfo(0xFFFF);
    TEST_ASSERT_EQUAL_STRING("Unknown", info.name);

    TEST_PASS();
}

void test_modbus_handler_init()
{
    Serial.println("Testing Modbus handler initialization...");

    modbusHandler.begin(testSerial, D7, 1);

    // Check initial state
    TEST_ASSERT_EQUAL(0, modbusHandler.getPollCount());
    TEST_ASSERT_EQUAL(0, modbusHandler.getErrorCount());

    TEST_PASS();
}

void test_json_export()
{
    Serial.println("Testing JSON export...");

    DynamicJsonDocument doc(4096);
    bool success = modbusHandler.getDataAsJson(doc);

    // Should return false when no data
    TEST_ASSERT_FALSE(success);

    // But document should have some structure
    TEST_ASSERT_TRUE(doc.containsKey("timestamp"));

    TEST_PASS();
}

void runAllTests()
{
    UNITY_BEGIN();

    RUN_TEST(test_register_count);
    RUN_TEST(test_register_address_uniqueness);
    RUN_TEST(test_register_name_uniqueness);
    RUN_TEST(test_register_types);
    RUN_TEST(test_register_sizes);
    RUN_TEST(test_register_ranges);
    RUN_TEST(test_register_mapper);
    RUN_TEST(test_modbus_handler_init);
    RUN_TEST(test_json_export);

    UNITY_END();
}

void setup()
{
    delay(2000); // Wait for serial
    Serial.println("\n\n=== Starting Unit Tests ===");

    runAllTests();
}

void loop()
{
    // Tests run once
}