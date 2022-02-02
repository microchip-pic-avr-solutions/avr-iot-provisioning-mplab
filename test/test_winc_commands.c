/*
 *  (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 *  Subject to your compliance with these terms, you may use Microchip software
 *  and any derivatives exclusively with Microchip products. You’re responsible
 *  for complying with 3rd party license terms applicable to your use of 3rd
 *  party software (including open source software) that may accompany
 *  Microchip software.
 *
 *  SOFTWARE IS “AS IS.” NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 *  APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF
 *  NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 *  INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 *  WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 *  BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 *  FULLEST EXTENT ALLOWED BY LAW, MICROCHIP’S TOTAL LIABILITY ON ALL CLAIMS
 *  RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID
 *  DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 */

#ifdef TEST

#include "unity.h"

// Mocking out drivers/libs
#include "mock_spi_flash.h"
#include "mock_delay.h"
#include "mock_m2m_wifi.h"
#include "mock_m2m_fwinfo.h"

#include "conversions.h"
#include "winc_commands.h"
#include "mc_error.h"
#include "winc_defines.h"
#include "spi_flash_map.h"
#include "mc_argparser.h"

#define STATUS_M2M_ERR_FAIL M2M_ERR_FAIL
// STATUS_SOURCE_WINC = 2, M2M_ERR_FAIL = -12 = 0xF4
#define STATUS_MC_M2M_ERR_FAIL 0x02F4


// Buffers for command arguments
static uint8_t arg_address[32];
static uint8_t arg_data_length[32];
static uint8_t arg_blob_length[32];

void setUp(void)
{
    // Just ignore all delays in all tests. They are not important for the tested functionality
    DELAY_milliseconds_Ignore();
}

void tearDown(void)
{
}

// Helper that generates counter dummy data
static void generate_dummy_data(uint8_t *buffer, uint16_t length)
{
    for (uint16_t i = 0;i < length; i++){
        buffer[i] = i;
    }
}

// Helper that populates argv for read command with provided address and length values
static void populate_read_argv(char *argv[], uint32_t address, uint16_t length)
{
    snprintf(arg_address, sizeof(arg_address), "%d", address);
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", length);
    argv[WINC_READ_WRITE_ARG_ADDRESS] = arg_address;
    argv[WINC_READ_WRITE_ARG_LENGTH] = arg_data_length;
}

// Helper that populates argv for write command with provided address and length values
static void populate_write_argv(char *argv[], uint32_t address, uint16_t length)
{
    snprintf(arg_address, sizeof(arg_address), "%d", address);
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", length);
    // Length of data blob will also be included in argv for write commands. Since data is hex encoded it will require
    // twice as many bytes as the raw binary data
    snprintf(arg_blob_length, sizeof(arg_data_length), "%d", length*2);
    argv[WINC_READ_WRITE_ARG_ADDRESS] = arg_address;
    argv[WINC_READ_WRITE_ARG_LENGTH] = arg_data_length;
    argv[WINC_WRITE_ARG_BLOB_LENGTH] = arg_blob_length;
}

// Helper that populates argv for erase command with provided address
static void populate_erase_argv(char *argv[], uint32_t address)
{
    snprintf(arg_address, sizeof(arg_address), "%d", address);
    argv[WINC_ERASE_ARG_ADDRESS] = arg_address;
}

// Helper that configures mocks for a spi_flash_write
static void configure_mock_spi_flash_write(uint8_t *data, uint16_t address, uint16_t data_length, int8_t m2m_status)
{
    // WINC state should be checked before setting download mode.
    // Just assume WINC is not in use (0 means WIFI_STATE_DEINIT)
    m2m_wifi_get_state_ExpectAndReturn(0);
    // WINC download mode must be enabled before writing to flash
    m2m_wifi_download_mode_ExpectAndReturn(0);
    // Configure spi_flash mock
    spi_flash_write_ExpectAndReturn(data, address, data_length, m2m_status);
    // WINC download mode should be disabled after writing is finished
    m2m_wifi_init_ExpectAnyArgsAndReturn(0);
}

// Helper that configures mocks for a spi_flash_erase
static void configure_mock_spi_flash_erase(uint16_t address, int8_t m2m_status)
{
    // WINC state should be checked before setting download mode.
    // Just assume WINC is not in use (0 means WIFI_STATE_DEINIT)
    m2m_wifi_get_state_ExpectAndReturn(0);
    // WINC download mode must be enabled before writing to flash
    m2m_wifi_download_mode_ExpectAndReturn(0);
    // Configure spi_flash mock
    spi_flash_erase_ExpectAndReturn(address, FLASH_SECTOR_SZ, m2m_status);
    // WINC download mode should be disabled after writing is finished
    m2m_wifi_init_ExpectAnyArgsAndReturn(0);
}

// Helper that configures mocks for a spi_flash_read
static void configure_mock_spi_flash_read(uint8_t *data, uint16_t address, uint16_t data_length, int8_t m2m_status)
{
    // WINC state should be checked before setting download mode.
    // Just assume WINC is not in use (0 means WIFI_STATE_DEINIT)
    m2m_wifi_get_state_ExpectAndReturn(0);
    // WINC download mode must be enabled before writing to flash
    m2m_wifi_download_mode_ExpectAndReturn(0);
    // Configure spi_flash mock. Second half of data buffer is used for the raw binary data while the hex encoded data will begin at the start of the buffer
    spi_flash_read_ExpectAndReturn(data, address, data_length, m2m_status);
    // WINC download mode should be disabled after writing is finished
    m2m_wifi_init_ExpectAnyArgsAndReturn(0);
}

void test_cmd_winc_writeblob_one_page_returns_ok(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data[FLASH_PAGE_SZ];
    uint8_t data_hex[FLASH_PAGE_SZ*2];
    uint16_t data_length_hex = 0;
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    populate_write_argv(argv, address, data_length);
    generate_dummy_data(data, data_length);

    data_length_hex = convert_bin2hex(data_length, data, data_hex);

    configure_mock_spi_flash_write(data_hex, address, data_length, M2M_SUCCESS);

    uint16_t result = cmd_winc_writeblob(argc, argv, data_hex, &data_length_hex);

    // Check that the data was converted from hex encoding to raw binary data
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_hex, data_length, "Data mismatch");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE((uint16_t) MC_STATUS_OK, result, "WINC writeblob reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_spi_flash_write_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data_hex[FLASH_PAGE_SZ*2];
    uint16_t data_length_hex = data_length*2;
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    populate_write_argv(argv, address, data_length);

    configure_mock_spi_flash_write(data_hex, address, data_length, STATUS_M2M_ERR_FAIL);

    uint16_t result = cmd_winc_writeblob(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_M2M_ERR_FAIL, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex, "WINC writeblob does not return any data so it should set data_length to 0");
}


void test_cmd_winc_writeblob_more_than_one_page_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data[FLASH_PAGE_SZ];
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    populate_write_argv(argv, address, data_length);
    generate_dummy_data(data, data_length);

    uint16_t result = cmd_winc_writeblob(argc, argv, data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_0_bytes_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = 0;
    uint8_t data;
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    populate_write_argv(argv, address, data_length);

    uint16_t result = cmd_winc_writeblob(argc, argv, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_too_few_arguments_returns_error(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS - 1;
    char *argv[WINC_ERASE_NUM_ARGS-1];
    uint8_t data;
    uint16_t data_length = 1;

    uint16_t result = cmd_winc_writeblob(argc, argv, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_null_pointer_arguments_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    // Use any length except 0 to not trigger other error
    uint16_t data_length = 1;
    uint8_t data;

    uint16_t result = cmd_winc_writeblob(argc, NULL, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_null_pointer_data_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    char *argv[WINC_WRITE_NUM_ARGS];
    // Use any length except 0 to not trigger other error
    uint16_t data_length = 1;

    uint16_t result = cmd_winc_writeblob(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_too_few_databytes_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint16_t data_length_hex = FLASH_PAGE_SZ*2;
    uint8_t data[FLASH_PAGE_SZ*2];
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    populate_write_argv(argv, address, data_length);

    // Reduce data_length to pretend some data bytes are missing
    data_length_hex--;

    uint16_t result = cmd_winc_writeblob(argc, argv, data, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex, "WINC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_winc_writeblob_too_many_databytes_returns_error(void)
{
    uint8_t argc = WINC_WRITE_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data[FLASH_PAGE_SZ];
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_WRITE_NUM_ARGS];

    // Set length argument to less than the number of data bytes
    populate_write_argv(argv, address, data_length-1);

    uint16_t result = cmd_winc_writeblob(argc, argv, data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_winc_read_one_page_returns_ok(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    uint16_t data_length_received = 0;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data[FLASH_PAGE_SZ];
    uint8_t data_received[FLASH_PAGE_SZ];
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[FLASH_PAGE_SZ*2];
    uint16_t data_length_hex = 0;
    // Just pick a random address
    uint32_t address = 32;
    char *argv[WINC_READ_NUM_ARGS];

    populate_read_argv(argv, address, data_length);
    generate_dummy_data(data, data_length);

    configure_mock_spi_flash_read(data_hex+data_length, address, data_length, M2M_SUCCESS);
    // In addition to the above mock configuration the spi_flash_read mock must return the expected data
    spi_flash_read_ReturnMemThruPtr_pu8Buf(data, data_length);

    uint16_t result = cmd_winc_read(argc, argv, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "WINC read reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(data_length, data_length_received, "Incorrect number of bytes received");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_received, data_length_received, "Data mismatch");
}

void test_cmd_winc_read_spi_flash_read_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    uint16_t data_length = FLASH_PAGE_SZ;
    uint8_t data[FLASH_PAGE_SZ];
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_READ_NUM_ARGS];

    populate_read_argv(argv, address, data_length);

    configure_mock_spi_flash_read(data+data_length, address, data_length, STATUS_M2M_ERR_FAIL);

    uint16_t result = cmd_winc_read(argc, argv, data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_M2M_ERR_FAIL, result, "WINC read did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC read does not return any data when it fails so it should set data_length to 0");
}

void  test_cmd_winc_read_more_than_one_page_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    // Initialize to non-zero to check that it gets set to 0
    uint16_t data_length_received = 1;
    uint16_t data_length = FLASH_PAGE_SZ+1;
    uint8_t data_received[FLASH_PAGE_SZ];
    // Just pick a random address
    uint32_t address = 32;
    char *argv[WINC_READ_NUM_ARGS];

    populate_read_argv(argv, address, data_length);

    uint16_t result = cmd_winc_read(argc, argv, data_received, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC read reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When cmd_winc_read fails it should return no data");
}

void test_cmd_winc_read_0_bytes_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    // Initialize to non-zero to check that it gets set to 0
    uint16_t data_length_received = 1;
    uint16_t data_length = 0;
    uint8_t data;
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_READ_NUM_ARGS];

    populate_read_argv(argv, address, data_length);

    uint16_t result = cmd_winc_read(argc, argv, &data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC read did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When cmd_winc_read fails it should return no data");
}

void test_cmd_winc_read_too_few_arguments_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS - 1;
    char *argv[WINC_READ_NUM_ARGS-1];
    uint8_t data;
    // Initialize to non-zero to check that it gets set to 0
    uint16_t data_length_received = 1;

    uint16_t result = cmd_winc_read(argc, argv, &data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When cmd_winc_read fails it should return no data");
}

void test_cmd_winc_read_null_pointer_arguments_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    // Use any length except 0 to not trigger other error
    uint16_t data_length_received = 1;
    uint8_t data;

    uint16_t result = cmd_winc_read(argc, NULL, &data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC writeblob did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When cmd_winc_read fails it should return no data");
}

void test_cmd_winc_read_null_pointer_data_returns_error(void)
{
    uint8_t argc = WINC_READ_NUM_ARGS;
    char *argv[WINC_READ_NUM_ARGS];
    // Use any length except 0 to not trigger other error
    uint16_t data_length_received = 1;

    uint16_t result = cmd_winc_read(argc, argv, NULL, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "WINC read did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When cmd_winc_read fails it should return no data");
}

void test_cmd_winc_erasesector_1_returns_ok(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS;
    uint32_t address = 1;
    char *argv[WINC_ERASE_NUM_ARGS];
    // Use any length except 0 to check that the erase function actually sets it to 0
    uint16_t data_length_received = 1;

    populate_erase_argv(argv, address);

    configure_mock_spi_flash_erase(address, M2M_SUCCESS);

    uint16_t result = cmd_winc_erasesector(argc, argv, NULL, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "WINC erase sector reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "WINC erase sector should not return any data so it should set data_length to 0");
}

void test_cmd_winc_erasesector_when_winc_is_initialized_runs_deinit(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS;
    uint32_t address = 1;
    char *argv[WINC_ERASE_NUM_ARGS];
    // Use any length except 0 to check that the erase function actually sets it to 0
    uint16_t data_length_received = 1;

    populate_erase_argv(argv, address);

    // WINC state should be checked before setting download mode.
    // In this test the WINC is simulated as already being initialized (1 means WIFI_STATE_INIT)
    m2m_wifi_get_state_ExpectAndReturn(1);
    // Since WINC already is initialized it must be deinit before setting download mode
    m2m_wifi_deinit_ExpectAndReturn(NULL, M2M_SUCCESS);
    // WINC download mode must be enabled before writing to flash
    m2m_wifi_download_mode_ExpectAndReturn(0);
    // Configure spi_flash mock
    spi_flash_erase_ExpectAndReturn(address, FLASH_SECTOR_SZ, M2M_SUCCESS);
    // WINC download mode should be disabled after writing is finished
    m2m_wifi_init_ExpectAnyArgsAndReturn(0);

    uint16_t result = cmd_winc_erasesector(argc, argv, NULL, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "WINC erase sector reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "WINC erase sector should not return any data so it should set data_length to 0");
}

void test_cmd_winc_erase_sector_spi_flash_erase_returns_error(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS;
    // Just set length to anything but 0 to check that it gets set to 0 by the erase command implementation
    uint16_t data_length = 1;
    // Just pick a random address
    uint32_t address = 16;
    char *argv[WINC_ERASE_NUM_ARGS];

    populate_erase_argv(argv, address);

    configure_mock_spi_flash_erase(address, STATUS_M2M_ERR_FAIL);

    uint16_t result = cmd_winc_erasesector(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_M2M_ERR_FAIL, result, "WINC erase sector did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC erase sector does not return any data so it should set data_length to 0");
}

void test_cmd_winc_erasesector_too_many_arguments_returns_error(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS + 1;
    char *argv[WINC_ERASE_NUM_ARGS+1];
    // Just set length to anything but 0 to check that it gets set to 0 by the erase command implementation
    uint16_t data_length = 1;

    uint16_t result = cmd_winc_erasesector(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "WINC erase sector did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC erase sector should not return any data so it should set data_length to 0");
}

void test_cmd_winc_erasesector_too_few_arguments_returns_error(void)
{
    uint8_t argc = WINC_ERASE_NUM_ARGS-1;
    char *argv[1];
    // Just set length to anything but 0 to check that it gets set to 0 by the erase command implementation
    uint16_t data_length = 1;

    uint16_t result = cmd_winc_erasesector(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "WINC erase sector did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "WINC erase sector does not return any data so it should set data_length to 0");
}

#endif // TEST
