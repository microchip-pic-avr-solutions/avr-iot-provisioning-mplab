#ifdef TEST

#include "unity.h"
#include <inttypes.h>

#include "mc_argparser.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_parse_arg_uint16_decimal(void)
{
    bool status;
    uint16_t value = 15;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT16(value, arg_parsed);
}

void test_parse_arg_uint16_0(void)
{
    bool status;
    uint16_t value = 0;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT16(value, arg_parsed);
}

void test_parse_arg_uint16_65535(void)
{
    bool status;
    uint16_t value = 65535;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT16(value, arg_parsed);
}

void test_parse_arg_uint32_decimal(void)
{
    bool status;
    uint32_t value = 70000;
    uint8_t arg[32] = {0};
    uint32_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint32(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT32(value, arg_parsed);
}

void test_parse_arg_uint32_0(void)
{
    bool status;
    uint32_t value = 0;
    uint8_t arg[32] = {0};
    uint32_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint32(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT32(value, arg_parsed);
}

void test_parse_arg_uint32_4294967295(void)
{
    bool status;
    uint64_t value = 4294967295;
    uint8_t arg[32] = {0};
    uint32_t arg_parsed;

    snprintf(arg, sizeof(arg), "%llu", value);

    status = parse_arg_uint32(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT32(value, arg_parsed);
}


void test_parse_arg_uint16_hex(void)
{
    bool status;
    uint16_t value = 20;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "0x%X", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_TRUE(status);
    TEST_ASSERT_EQUAL_UINT16(value, arg_parsed);
}

void test_parse_arg_uint16_no_int(void)
{
    bool status;
    uint8_t *arg = "FIVE";
    uint16_t arg_parsed;

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_FALSE(status);
}

void test_parse_arg_uint16_invalid_termination(void)
{
    bool status;
    uint16_t value = 15;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    // Add a string after the number to test terminatin check
    snprintf(arg, sizeof(arg), "%d FIVE", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_FALSE(status);
}

void test_parse_arg_uint16_empty(void)
{
    bool status;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    // Create empty string by adding 0 termination as first character
    arg[0] = '\0';

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_FALSE(status);
}

void test_parse_arg_uint16_negative(void)
{
    bool status;
    int16_t value = -15;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_FALSE(status);
}

void test_parse_arg_uint16_out_of_range(void)
{
    bool status;
    uint32_t value = 0x10000;
    uint8_t arg[32] = {0};
    uint16_t arg_parsed;

    snprintf(arg, sizeof(arg), "%d", value);

    status = parse_arg_uint16(arg, &arg_parsed);

    TEST_ASSERT_FALSE(status);
}

#endif // TEST
