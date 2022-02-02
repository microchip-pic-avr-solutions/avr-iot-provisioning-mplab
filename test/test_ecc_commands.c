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
#include "mock_atca_basic.h"

#include "conversions.h"
#include "mc_error.h"
#include "ecc_commands.h"
#include "atca_command.h"
#include "mc_argparser.h"

// Buffers for command arguments
static uint8_t arg_data_length[32];
static uint8_t arg_slot[32];
static uint8_t arg_blob_length[32];

// MAX ECC slot size to test the worst case when testing ECC read
#define MAX_ECC_SLOT_SIZE 416

#define STATUS_ATCA_GEN_FAIL ATCA_GEN_FAIL
// STATUS_SOURCE_CRYPTOAUTHLIB = 1, ATCA_GEN_FAIL = 0xE1
#define STATUS_MC_ATCA_GEN_FAIL  (0x0100 |  ATCA_GEN_FAIL)
#define STATUS_ATCA_BAD_PARAM ATCA_BAD_PARAM
// STATUS_SOURCE_CRYPTOAUTHLIB = 1, ATCA_BAD_PARAM = 0xE2
#define STATUS_MC_ATCA_BAD_PARAM 0x01E2

void setUp(void)
{
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

// Helper that populates argv for write command with provided slot and length values
static void populate_write_argv(char *argv[], uint16_t slot, uint16_t length)
{
    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", length);
    // Length of data blob will also be included in argv for write commands. Since data is hex encoded it will require
    // twice as many bytes as the raw binary data
    snprintf(arg_blob_length, sizeof(arg_data_length), "%d", length*2);
    argv[ECC_WRITE_ARG_SLOT] = arg_slot;
    argv[ECC_WRITE_ARG_LENGTH] = arg_data_length;
    argv[ECC_WRITE_ARG_BLOB_LENGTH] = arg_blob_length;
}

// Helper that populates argv for signdigest command with provided length value
static void populate_signdigest_argv(char *argv[], uint16_t length)
{
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", length);
    // Length of data blob will also be included in argv for write commands. Since data is hex encoded it will require
    // twice as many bytes as the raw binary data
    snprintf(arg_blob_length, sizeof(arg_data_length), "%d", length*2);
    argv[ECC_SIGNDIGEST_ARG_LENGTH] = arg_data_length;
    argv[ECC_SIGNDIGEST_ARG_BLOB_LENGTH] = arg_blob_length;
}


// Helper that populates argv for read commands with provided slot and length values
static void populate_read_argv(char *argv[], uint16_t slot, uint16_t length)
{
    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", length);
    argv[ECC_READ_ARG_SLOT] = arg_slot;
    argv[ECC_READ_ARG_LENGTH] = arg_data_length;
}

// Helper that populates argv for commands that only require slot argument
static void populate_slot_argv(char *argv[], uint16_t slot)
{
    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    argv[0] = arg_slot;
}

// Helper that populates argv for public key write command
static void populate_pubkey_write_argv(char *argv[], uint16_t slot, uint16_t bloblength)
{
    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    snprintf(arg_data_length, sizeof(arg_data_length), "%d", bloblength);
    argv[ECC_PUBKEY_WRITE_ARG_SLOT] = arg_slot;
    argv[ECC_PUBKEY_WRITE_ARG_BLOB_LENGTH] = arg_data_length;
}

// Helper to configure atcab_is_slot_locked mock used by cmd_ecc_lock
static void configure_atcab_is_slot_locked_mock(bool *is_locked, uint16_t slot, ATCA_STATUS status)
{
    atcab_is_slot_locked_ExpectAndReturn(slot, is_locked, status);
    // Ignore the is_locked argument as the pointer will be to a local variable during the test
    atcab_is_slot_locked_IgnoreArg_is_locked();
    atcab_is_slot_locked_ReturnThruPtr_is_locked(is_locked);
}

void test_cmd_ecc_genpubkey_no_argument_ok(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_PUB_KEY_SIZE];
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint8_t data_received[ATCA_PUB_KEY_SIZE];
    uint16_t data_length_hex = 0;
    uint16_t data_length_received = 0;

    generate_dummy_data(data, ATCA_PUB_KEY_SIZE);

    atcab_get_pubkey_ExpectAndReturn(0, &data_hex[ATCA_PUB_KEY_SIZE], MC_STATUS_OK);
    atcab_get_pubkey_ReturnMemThruPtr_public_key(data, ATCA_PUB_KEY_SIZE);

    uint16_t result = cmd_ecc_genpubkey(argc, NULL, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC genpubkey reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(ATCA_PUB_KEY_SIZE, data_length_received, "Incorrect number of bytes returned");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data_received, data, data_length_received, "Returned Public key does not match");
}

void test_cmd_ecc_genpubkey_slot_argument_ok(void)
{
    uint8_t argc = ECC_GENPUBKEY_NUM_ARGS;
    char *argv[ECC_GENPUBKEY_NUM_ARGS];
    uint16_t slot = 4;
    uint8_t data[ATCA_PUB_KEY_SIZE];
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint8_t data_received[ATCA_PUB_KEY_SIZE];
    uint16_t data_length_hex = 0;
    uint16_t data_length_received = 0;

    populate_slot_argv(argv, slot);

    generate_dummy_data(data, ATCA_PUB_KEY_SIZE);

    atcab_get_pubkey_ExpectAndReturn(slot, &data_hex[ATCA_PUB_KEY_SIZE], MC_STATUS_OK);
    atcab_get_pubkey_ReturnMemThruPtr_public_key(data, ATCA_PUB_KEY_SIZE);

    uint16_t result = cmd_ecc_genpubkey(argc, argv, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC genpubkey reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(ATCA_PUB_KEY_SIZE, data_length_received, "Incorrect number of bytes returned");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data_received, data, data_length_received, "Returned Public key does not match");
}

void test_cmd_ecc_genpubkey_atcab_get_pubkey_returns_error(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_PUB_KEY_SIZE*2];
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length_received = ATCA_PUB_KEY_SIZE;

    generate_dummy_data(data, ATCA_PUB_KEY_SIZE);

    atcab_get_pubkey_ExpectAndReturn(0, &data[ATCA_PUB_KEY_SIZE], STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_genpubkey(argc, NULL, data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC genpubkey did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "genpubkey should return no data when failing");
}

void test_cmd_ecc_genpubkey_null_pointer_data_returns_error(void)
{
    uint8_t argc = 0;
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length = ATCA_PUB_KEY_SIZE;

    uint16_t result = cmd_ecc_genpubkey(argc, NULL, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC genpubkey did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "genpubkey should return no data when failing");
}

void test_cmd_ecc_genpubkey_null_pointer_data_length_returns_error(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_PUB_KEY_SIZE*2];

    uint16_t result = cmd_ecc_genpubkey(argc, NULL, data, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC genpubkey did not report expected error");
}

void test_cmd_ecc_pubkey_read_ok(void)
{
    uint8_t argc = ECC_PUBKEY_READ_NUM_ARGS;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS];
    uint16_t slot = 15;
    uint8_t data[ATCA_PUB_KEY_SIZE];
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint8_t data_received[ATCA_PUB_KEY_SIZE];
    uint16_t data_length_hex = 0;
    uint16_t data_length_received = 0;

    populate_slot_argv(argv, slot);

    generate_dummy_data(data, ATCA_PUB_KEY_SIZE);

    atcab_read_pubkey_ExpectAndReturn(slot, &data_hex[ATCA_PUB_KEY_SIZE], MC_STATUS_OK);
    atcab_read_pubkey_ReturnMemThruPtr_public_key(data, ATCA_PUB_KEY_SIZE);

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC read pubkey reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(ATCA_PUB_KEY_SIZE, data_length_received, "Incorrect number of bytes returned");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data_received, data, data_length_received, "Returned Public key does not match");
}

void test_cmd_ecc_pubkey_read_atcab_read_pubkey_returns_error(void)
{
    uint8_t argc = ECC_PUBKEY_READ_NUM_ARGS;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS];
    uint16_t slot = 15;
    uint8_t data[ATCA_PUB_KEY_SIZE*2];
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length_received = ATCA_PUB_KEY_SIZE;

    populate_slot_argv(argv, slot);

    atcab_read_pubkey_ExpectAndReturn(slot, &data[ATCA_PUB_KEY_SIZE], STATUS_ATCA_BAD_PARAM);

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_BAD_PARAM, result, "ECC read pubkey did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "ECC read pubkey should not return any data when command fails");
}

void test_cmd_ecc_pubkey_read_too_many_arguments(void)
{
    uint8_t argc = ECC_PUBKEY_READ_NUM_ARGS+1;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS+1];
    uint16_t slot = 15;
    uint8_t data[ATCA_PUB_KEY_SIZE*2];
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length_received = ATCA_PUB_KEY_SIZE;

    populate_slot_argv(argv, slot);

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC read pubkey did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "ECC read pubkey should not return any data when command fails");
}

void test_cmd_ecc_pubkey_read_too_few_arguments(void)
{
    uint8_t argc = 0;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS];
    uint16_t slot = 15;
    uint8_t data[ATCA_PUB_KEY_SIZE*2];
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length_received = ATCA_PUB_KEY_SIZE;

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, data, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC read pubkey did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "ECC read pubkey should not return any data when command fails");
}

void test_cmd_ecc_pubkey_read_null_data_pointer(void)
{
    uint8_t argc = ECC_PUBKEY_READ_NUM_ARGS;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS];
    uint16_t slot = 15;
    // Initialize to something else than 0 to check that the command sets it correctly
    uint16_t data_length_received = ATCA_PUB_KEY_SIZE;

    populate_slot_argv(argv, slot);

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, NULL, &data_length_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC read pubkey did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "ECC read pubkey should not return any data when command fails");
}

void test_cmd_ecc_pubkey_read_null_data_length_pointer(void)
{
    uint8_t argc = ECC_PUBKEY_READ_NUM_ARGS;
    char *argv[ECC_PUBKEY_READ_NUM_ARGS];
    uint16_t slot = 15;
    uint8_t data_received[ATCA_PUB_KEY_SIZE*2];
    uint16_t data_length_received = 0;

    populate_slot_argv(argv, slot);

    uint16_t result = cmd_ecc_pubkey_read(argc, argv, data_received, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC read pubkey did not report the expected error");
}

void  test_cmd_ecc_pubkey_write_ok(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS;
    uint16_t data_length = ATCA_PUB_KEY_SIZE;
    uint8_t data[ATCA_PUB_KEY_SIZE];
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint16_t data_length_hex = 0;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    generate_dummy_data(data, data_length);

    data_length_hex = convert_bin2hex(data_length, data, data_hex);
    populate_pubkey_write_argv(argv, slot, data_length_hex);

    atcab_write_pubkey_ExpectAndReturn(slot, data_hex, MC_STATUS_OK);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC pubkey write reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
    // Check that the data was converted from hex encoding to raw binary data
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_hex, data_length, "Data mismatch");
}

void  test_cmd_ecc_pubkey_write_atcab_write_pubkey_returns_error(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    populate_pubkey_write_argv(argv, slot, data_length_hex);

    atcab_write_pubkey_ExpectAndReturn(slot, data_hex, ATCA_BAD_PARAM);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_BAD_PARAM, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_too_few_data_bytes(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2-1];
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2-1;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    populate_pubkey_write_argv(argv, slot, data_length_hex);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_too_many_data_bytes(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2+1];
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2+1;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    populate_pubkey_write_argv(argv, slot, data_length_hex);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_too_many_arguments(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS+1;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS+1];

    populate_pubkey_write_argv(argv, slot, data_length_hex);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_too_few_arguments(void)
{
    uint8_t argc = 0;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_data_nullpointer(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS+1;
    uint16_t data_length_hex = ATCA_PUB_KEY_SIZE*2;
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS+1];

    populate_pubkey_write_argv(argv, slot, data_length_hex);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, NULL, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC pubkey write did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC pubkey write does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_pubkey_write_data_length_nullpointer(void)
{
    uint8_t argc = ECC_PUBKEY_WRITE_NUM_ARGS;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[ATCA_PUB_KEY_SIZE*2];
    // Just pick a random slot
    uint16_t slot = 15;
    char *argv[ECC_PUBKEY_WRITE_NUM_ARGS];

    populate_pubkey_write_argv(argv, slot, ATCA_PUB_KEY_SIZE*2);

    uint16_t result = cmd_ecc_pubkey_write(argc, argv, data_hex, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC pubkey write did not report the expected error");
}

void test_cmd_ecc_signdigest_ok(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint8_t data[ATCA_SIG_SIZE];
    uint8_t data_received_hex[ATCA_SIG_SIZE*2];
    uint8_t data_received[ATCA_SIG_SIZE*2];
    // The input is a digest with size ATCA_BLOCK_SIZE, but it is hex encoded so twice the raw binary size
    uint16_t data_length_hex = ATCA_BLOCK_SIZE*2;
    uint16_t data_length_received = 0;

    populate_signdigest_argv(argv, data_length_hex);

    generate_dummy_data(data, ATCA_SIG_SIZE);

    atcab_sign_ExpectAndReturn(0, data_received_hex, &data_received_hex[ATCA_SIG_SIZE], MC_STATUS_OK);
    atcab_sign_ReturnMemThruPtr_signature(data, ATCA_SIG_SIZE);

    uint16_t result = cmd_ecc_signdigest(argc, argv, data_received_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_received_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC signdigest reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(ATCA_SIG_SIZE, data_length_received, "Incorrect number of bytes returned");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data_received, data, data_length_received, "Returned signature does not match");
}

void test_cmd_ecc_signdigest_missing_length_argument(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS-1;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint8_t data[ATCA_SIG_SIZE];
    uint8_t data_received_hex[ATCA_SIG_SIZE*2];
    uint8_t data_received[ATCA_SIG_SIZE*2];
    // The input is a digest with size ATCA_BLOCK_SIZE, but it is hex encoded so twice the raw binary size
    uint16_t data_length_hex = ATCA_BLOCK_SIZE*2;
    uint16_t data_length_received = 0;

    populate_signdigest_argv(argv, data_length_hex);

    generate_dummy_data(data, ATCA_SIG_SIZE);

    uint16_t result = cmd_ecc_signdigest(argc, argv, data_received_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_received_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC signdigest did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "When ECC signdigest fails no data should be returned");
}

void test_cmd_ecc_signdigest_too_short_digest_returns_error(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint8_t data[ATCA_SIG_SIZE-1];
    uint8_t data_received_hex[(ATCA_SIG_SIZE-1)*2];
    uint8_t data_received[(ATCA_SIG_SIZE-1)*2];
    // The input is a digest with size ATCA_BLOCK_SIZE, but it is hex encoded so twice the raw binary size
    uint16_t data_length_hex = (ATCA_BLOCK_SIZE-1)*2;
    uint16_t data_length_received = 0;

    populate_signdigest_argv(argv, data_length_hex);

    uint16_t result = cmd_ecc_signdigest(argc, argv, data_received_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC signdigest did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_received, "No data should be returned when the command fails");
}

void test_cmd_ecc_signdigest_atcab_sign_returns_error(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint8_t data_received_hex[ATCA_SIG_SIZE*2];
    uint16_t data_length = ATCA_SIG_SIZE;
    uint16_t data_length_hex = data_length*2;

    populate_signdigest_argv(argv, data_length_hex);

    atcab_sign_ExpectAndReturn(0, data_received_hex, &data_received_hex[ATCA_SIG_SIZE], STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_signdigest(argc, argv, data_received_hex, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC signdigest did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "signdigest should return no data when it fails");
}

void test_cmd_ecc_signdigest_null_pointer_data_returns_error(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint16_t data_length = ATCA_SIG_SIZE;
    uint16_t data_length_hex = data_length*2;

    populate_signdigest_argv(argv, data_length_hex);

    uint16_t result = cmd_ecc_signdigest(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC signdigest did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "signdigest should return no data when it fails");
}

void test_cmd_ecc_signdigest_null_pointer_data_length_returns_error(void)
{
    uint8_t argc = ECC_SIGNDIGEST_NUM_ARGS;
    char *argv[ECC_SIGNDIGEST_NUM_ARGS];
    uint8_t data[ATCA_SIG_SIZE*2];
    uint16_t data_length_hex = ATCA_SIG_SIZE*2;

    populate_signdigest_argv(argv, data_length_hex);

    uint16_t result = cmd_ecc_signdigest(argc, argv, data, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC signdigest did not report the expected error");
}

void  test_cmd_ecc_read_ok(void)
{
    uint8_t argc = ECC_READ_NUM_ARGS;
    uint16_t data_length_received = 0;
    // Any length could be used, but setting the size equal to the biggest ECC slot makes sure the command handles the worst case
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    uint8_t data[MAX_ECC_SLOT_SIZE];
    uint8_t data_received[MAX_ECC_SLOT_SIZE];
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[MAX_ECC_SLOT_SIZE*2];
    uint16_t data_length_hex = 0;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_READ_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);
    generate_dummy_data(data, data_length);

    atcab_read_bytes_zone_ExpectAndReturn(2, slot, 0, data_hex+data_length, data_length, MC_STATUS_OK);
    atcab_read_bytes_zone_ReturnMemThruPtr_data(data, data_length);

    uint16_t result = cmd_ecc_read(argc, argv, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC read reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(data_length, data_length_received, "Incorrect number of bytes received");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_received, data_length_received, "Data mismatch");
}

void  test_cmd_ecc_read_no_length(void)
{
    // Omit length argument to read complete slot
    uint8_t argc = ECC_READ_NUM_ARGS-1;
    uint16_t data_length_received = 0;
    // Any length could be used, but setting the size equal to the biggest ECC slot makes sure the command handles the worst case
    size_t slot_size = 32;
    uint16_t data_length = 32;
    uint8_t data[32];
    uint8_t data_received[32];
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[32*2];
    uint16_t data_length_hex = 0;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_READ_NUM_ARGS-1];

    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    argv[ECC_READ_ARG_SLOT] = arg_slot;

    generate_dummy_data(data, data_length);

    atcab_get_zone_size_ExpectAndReturn(2, slot, (size_t *) &slot_size, MC_STATUS_OK);
    // Ignore the size argument as the pointer will be to a local variable during the test
    atcab_get_zone_size_IgnoreArg_size();
    atcab_get_zone_size_ReturnThruPtr_size(&slot_size);
    atcab_read_bytes_zone_ExpectAndReturn(2, slot, 0, data_hex+data_length, slot_size, MC_STATUS_OK);
    atcab_read_bytes_zone_ReturnMemThruPtr_data(data, data_length);

    uint16_t result = cmd_ecc_read(argc, argv, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC read reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(slot_size, data_length_received, "Incorrect number of bytes received");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_received, data_length_received, "Data mismatch");
}

void test_cmd_ecc_read_no_length_atcab_get_zone_size_returns_error(void)
{
    // Omit length argument to read complete slot
    uint8_t argc = ECC_READ_NUM_ARGS-1;
    // Any length could be used, but setting the size equal to the biggest ECC slot makes sure the command handles the worst case
    size_t slot_size = 32;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[32*2];
    uint16_t data_length_hex = 0;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_READ_NUM_ARGS-1];

    snprintf(arg_slot, sizeof(arg_slot), "%d", slot);
    argv[ECC_READ_ARG_SLOT] = arg_slot;

    atcab_get_zone_size_ExpectAndReturn(2, slot, (size_t *) &slot_size, STATUS_ATCA_BAD_PARAM);
    // Ignore the size argument as the pointer will be to a local variable during the test
    atcab_get_zone_size_IgnoreArg_size();
    atcab_get_zone_size_ReturnThruPtr_size(&slot_size);

    uint16_t result = cmd_ecc_read(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_BAD_PARAM, result, "ECC read did not report expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex, "No data should be returned when command fails");
}

void  test_cmd_ecc_read_atcab_read_bytes_zone_returns_error(void)
{
    uint8_t argc = ECC_READ_NUM_ARGS;
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data[MAX_ECC_SLOT_SIZE*2];
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_READ_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    atcab_read_bytes_zone_ExpectAndReturn(2, slot, 0, data+data_length, data_length, STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_read(argc, argv, data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC read did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "ECC read should not return any data when it fails");
}

void  test_cmd_ecc_read_0_bytes_returns_error(void)
{
    uint8_t argc = ECC_READ_NUM_ARGS;
    uint16_t data_length = 0;
    uint8_t data;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_READ_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    uint16_t result = cmd_ecc_read(argc, argv, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC read did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "ECC read should not return any data when it fails");
}

void  test_cmd_ecc_read_too_few_arguments_returns_error(void)
{
    uint8_t argc = 0;
    uint16_t data_length = 0;
    uint8_t data;

    uint16_t result = cmd_ecc_read(argc, NULL, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC read did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "ECC read should not return any data when it fails");
}

void  test_cmd_ecc_lock_unlocked_slot(void)
{
    uint8_t argc = ECC_LOCK_NUM_ARGS;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_LOCK_NUM_ARGS];
    // Simulate previously unlocked slot
    bool is_locked = false;

    populate_slot_argv(argv, slot);

    configure_atcab_is_slot_locked_mock(&is_locked, slot, MC_STATUS_OK);

    atcab_lock_data_slot_ExpectAndReturn(slot, MC_STATUS_OK);

    uint16_t result = cmd_ecc_lock(argc, argv, NULL, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC lock reported error");
}

void  test_cmd_ecc_lock_already_locked_slot(void)
{
    uint8_t argc = ECC_LOCK_NUM_ARGS;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_LOCK_NUM_ARGS];
    // Simulate already locked slot
    bool is_locked = true;

    populate_slot_argv(argv, slot);

    configure_atcab_is_slot_locked_mock(&is_locked, slot, MC_STATUS_OK);

    // The absence of atcab_lock_data_slot_ExpectAndReturn will check that the atcab_lock_data_slot function is not called

    uint16_t result = cmd_ecc_lock(argc, argv, NULL, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC lock reported error");
}

void  test_cmd_ecc_lock_atcab_is_slot_locked_returns_error(void)
{
    uint8_t argc = ECC_LOCK_NUM_ARGS;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_LOCK_NUM_ARGS];
    // Simulate previously unlocked slot
    bool is_locked = false;

    populate_slot_argv(argv, slot);

    configure_atcab_is_slot_locked_mock(&is_locked, slot, STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_lock(argc, argv, NULL, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC lock did not report the expected error");
}

void  test_cmd_ecc_lock_atcab_atcab_lock_data_slot_returns_error(void)
{
    uint8_t argc = ECC_LOCK_NUM_ARGS;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_LOCK_NUM_ARGS];
    // Simulate previously unlocked slot
    bool is_locked = false;

    populate_slot_argv(argv, slot);

    configure_atcab_is_slot_locked_mock(&is_locked, slot, MC_STATUS_OK);

    atcab_lock_data_slot_ExpectAndReturn(slot, STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_lock(argc, argv, NULL, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC lock did not report the expected error");
}

void  test_cmd_ecc_lock_too_few_arguments(void)
{
    uint8_t argc = ECC_LOCK_NUM_ARGS -1;
    char *argv[ECC_LOCK_NUM_ARGS];
    // Just pick a random slot
    uint16_t slot = 8;

    uint16_t result = cmd_ecc_lock(argc, argv, NULL, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC lock did not report the expected error");
}

void  test_cmd_ecc_writeblob_ok(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    // Any length could be used, but setting the size equal to the biggest ECC slot makes sure the command handles the worst case
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    uint8_t data[MAX_ECC_SLOT_SIZE];
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[MAX_ECC_SLOT_SIZE*2];
    uint16_t data_length_hex = 0;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);
    generate_dummy_data(data, data_length);

    data_length_hex = convert_bin2hex(data_length, data, data_hex);

    atcab_write_bytes_zone_ExpectAndReturn(2, slot, 0, data_hex, data_length, MC_STATUS_OK);

    uint16_t result = cmd_ecc_writeblob(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC writeblob reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC writeblob does not return any data so it should set data_length to 0");
    // Check that the data was converted from hex encoding to raw binary data
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data, data_hex, data_length, "Data mismatch");
}

void  test_cmd_ecc_writeblob_atcab_write_bytes_zone_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    // Any length could be used, but setting the size equal to the biggest ECC slot makes sure the command handles the worst case
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    // hex-encoded data requires twice the size of raw binary data
    uint8_t data_hex[MAX_ECC_SLOT_SIZE*2];
    uint16_t data_length_hex = data_length*2;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    atcab_write_bytes_zone_ExpectAndReturn(2, slot, 0, data_hex, data_length, STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_writeblob(argc, argv, data_hex, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_writeblob_0_bytes_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    uint16_t data_length = 0;
    uint8_t data;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    uint16_t result = cmd_ecc_writeblob(argc, argv, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_writeblob_too_few_arguments_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS-1;
    uint16_t data_length = 0;
    uint8_t data;
    char *argv[ECC_WRITE_NUM_ARGS];

    uint16_t result = cmd_ecc_writeblob(argc, argv, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_COUNT, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_writeblob_null_pointer_arguments_returns_error(void)
{
    uint8_t argc = 3;
    // Use any length except 0 to not trigger other error
    uint16_t data_length = 1;
    uint8_t data;

    uint16_t result = cmd_ecc_writeblob(argc, NULL, &data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_writeblob_null_pointer_data_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    // Use any length except 0 to not trigger other error
    uint16_t data_length = 1;
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    uint16_t result = cmd_ecc_writeblob(argc, argv, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length,  "ECC writeblob does not return any data so it should set data_length to 0");
}


void  test_cmd_ecc_writeblob_too_few_databytes_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    uint16_t data_length_hex = data_length*2;
    uint8_t data[MAX_ECC_SLOT_SIZE*2];
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    populate_read_argv(argv, slot, data_length);

    // Reduce data_length to pretend some data bytes are missing
    data_length_hex--;

    uint16_t result = cmd_ecc_writeblob(argc, argv, data, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void  test_cmd_ecc_writeblob_too_many_databytes_returns_error(void)
{
    uint8_t argc = ECC_WRITE_NUM_ARGS;
    uint16_t data_length = MAX_ECC_SLOT_SIZE;
    uint16_t data_length_hex = data_length*2;
    uint8_t data[MAX_ECC_SLOT_SIZE*2];
    // Just pick a random slot
    uint16_t slot = 8;
    char *argv[ECC_WRITE_NUM_ARGS];

    // Set length argument to less than the number of data bytes
    populate_read_argv(argv, slot, data_length-1);

    uint16_t result = cmd_ecc_writeblob(argc, argv, data, &data_length_hex);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC writeblob did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length_hex,  "ECC writeblob does not return any data so it should set data_length to 0");
}

void test_cmd_ecc_serial_ok(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_SERIAL_NUM_SIZE];
    uint8_t data_hex[ATCA_SERIAL_NUM_SIZE*2];
    uint8_t data_received[ATCA_SERIAL_NUM_SIZE];
    uint16_t data_length_hex = 0;
    uint16_t data_length_received = 0;

    generate_dummy_data(data, ATCA_SERIAL_NUM_SIZE);

    atcab_read_serial_number_ExpectAndReturn(&data_hex[ATCA_SERIAL_NUM_SIZE], MC_STATUS_OK);
    atcab_read_serial_number_ReturnMemThruPtr_serial_number(data, ATCA_SERIAL_NUM_SIZE);

    uint16_t result = cmd_ecc_serial(argc, NULL, data_hex, &data_length_hex);

    data_length_received = convert_hex2bin(data_length_hex, data_hex, data_received);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_OK, result, "ECC serial reported error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(ATCA_SERIAL_NUM_SIZE, data_length_received, "Incorrect number of bytes returned");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(data_received, data, data_length_received, "Returned serial does not match");
}

void test_cmd_ecc_serial_atcab_read_serial_number_returns_error(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_SERIAL_NUM_SIZE*2];
    // Initialize to something else than 0 to check that command actually sets the data_length to 0
    uint16_t data_length = ATCA_SERIAL_NUM_SIZE;

    atcab_read_serial_number_ExpectAndReturn(&data[ATCA_SERIAL_NUM_SIZE], STATUS_ATCA_GEN_FAIL);

    uint16_t result = cmd_ecc_serial(argc, NULL, data, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(STATUS_MC_ATCA_GEN_FAIL, result, "ECC serial did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "No data should be returned when command fails");
}

void test_cmd_ecc_serial_null_pointer_data_returns_error(void)
{
    uint8_t argc = 0;
    // Initialize to something else than 0 to check that command actually sets the data_length to 0
    uint16_t data_length = ATCA_SERIAL_NUM_SIZE;

    uint16_t result = cmd_ecc_serial(argc, NULL, NULL, &data_length);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC serial did not report the expected error");
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, data_length, "No data should be returned when command fails");
}

void test_cmd_ecc_serial_null_pointer_data_length_returns_error(void)
{
    uint8_t argc = 0;
    uint8_t data[ATCA_SERIAL_NUM_SIZE*2];

    uint16_t result = cmd_ecc_serial(argc, NULL, data, NULL);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(MC_STATUS_BAD_ARGUMENT_VALUE, result, "ECC serial did not report the expected error");
}


#endif // TEST
