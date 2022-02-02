/*
 *  (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 *  Subject to your compliance with these terms, you may use Microchip software
 *  and any derivatives exclusively with Microchip products. You're responsible
 *  for complying with 3rd party license terms applicable to your use of 3rd
 *  party software (including open source software) that may accompany
 *  Microchip software.
 *
 *  SOFTWARE IS "AS IS". NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
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
#include <stdint.h>
#include <stdlib.h>
#include "ecc_commands.h"
#include "conversions.h"
#include "mcc_generated_files/CryptoAuthenticationLibrary/atca_command.h"
#include "mcc_generated_files/CryptoAuthenticationLibrary/basic/atca_basic.h"
#include "command_handler/mc_argparser.h"
#include "command_handler/parser/mc_error.h"

#define DEVICE_PRIVATE_KEY_SLOT 0
#define MAX_ECC_DATA_SLOT 15

// Helpers
static uint16_t parse_and_check_read_args(uint8_t argc, char *argv[], uint16_t *slot_parsed, uint16_t *length_parsed);
static uint16_t parse_and_check_otp_read_args(uint8_t argc, char *argv[], uint16_t *length_parsed);
static uint16_t parse_and_check_write_args(uint8_t argc, char *argv[], uint16_t *slot_parsed, uint16_t *length_parsed);
static uint16_t parse_arg_ecc_slot(const char *arg, uint16_t *slot_parsed);

/*
 * Get ECC serial number
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command expects no arguments in argv
 *  data: pointer to a buffer used for returning the ECC serial number.
 *        The data is hex encoded, i.e. twice as many bytes as the raw binary
 *        value of the ECC serial number (i.e. ATCA_SERIAL_NUM_SIZE*2)
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.
 */
uint16_t cmd_ecc_serial(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint8_t atca_status = ATCA_SUCCESS;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    if (argc > 0) {
        // This command does not take any arguments, so if there are any it could indicate a badly formatted command
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }

    // The data buffer must be at least twice the size of the raw bytes of
    // the Public key so that it can contain a hex encoded variant. Then if
    // the raw bytes are put in the second half then the same buffer can be
    // used as both input and output for the hex conversion
    atca_status = atcab_read_serial_number(&data[ATCA_SERIAL_NUM_SIZE]);

    if (atca_status == ATCA_SUCCESS) {
        *data_length = convert_bin2hex(ATCA_SERIAL_NUM_SIZE, &data[ATCA_SERIAL_NUM_SIZE], data);;
    } else {
        *data_length = 0;
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

/*
 * Generate public key (based on ECC private key)
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following argument:
 *      slot: ECC slot containing private key used to generate the public key. Optional, defaults to 0.
 *  data: pointer to a buffer containing the hex encoded public key to be returned.
 *        The hex encoded key will take up twice as many bytes as the raw binary version of the key
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.
 */
uint16_t cmd_ecc_genpubkey(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t cmd_status = MC_STATUS_OK;
    uint16_t slot = 0;


    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Set data length to 0 up-front in case anything goes wrong and nothing can be returned
    *data_length = 0;

    if (argc > ECC_GENPUBKEY_NUM_ARGS) {
        // This command takes one (optional argument)
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    } else if (ECC_GENPUBKEY_NUM_ARGS == argc) {
        // parse argument
        cmd_status = parse_arg_ecc_slot(argv[ECC_GENPUBKEY_ARG_SLOT], &slot);
    }
    else {
        // The slot argument is optional and defaults to 0
        slot = 0;
    }

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    // The data buffer must be at least twice the size of the raw bytes of
    // the Public key so that it can contain a hex encoded variant. Then if
    // the raw bytes are put in the second half then the same buffer can be
    // used as both input and output for the hex conversion
    atca_status = atcab_get_pubkey(slot, &data[ATCA_PUB_KEY_SIZE]);

    if (atca_status == ATCA_SUCCESS) {
        *data_length = convert_bin2hex(ATCA_PUB_KEY_SIZE, &data[ATCA_PUB_KEY_SIZE], data);
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

/*
 * Read public key from specified slot
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following argument:
 *      slot: ECC slot containing private key used to generate the public key. Optional, defaults to 0.
 *  data: pointer to a buffer containing the hex encoded public key to be returned.
 *        The hex encoded key will take up twice as many bytes as the raw binary version of the key
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.
 */
uint16_t cmd_ecc_pubkey_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t cmd_status = MC_STATUS_OK;
    uint16_t slot = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Set data length to 0 up-front in case anything goes wrong and nothing can be returned
    *data_length = 0;

    if (ECC_PUBKEY_READ_NUM_ARGS == argc) {
        // parse argument
        cmd_status = parse_arg_ecc_slot(argv[ECC_PUBKEY_READ_ARG_SLOT], &slot);
    } else {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    // The data buffer must be at least twice the size of the raw bytes of
    // the Public key so that it can contain a hex encoded variant. Then if
    // the raw bytes are put in the second half then the same buffer can be
    // used as both input and output for the hex conversion
    atca_status = atcab_read_pubkey(slot, &data[ATCA_PUB_KEY_SIZE]);

    if (atca_status == ATCA_SUCCESS) {
        *data_length = convert_bin2hex(ATCA_PUB_KEY_SIZE, &data[ATCA_PUB_KEY_SIZE], data);
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

/*
 * Write Public key to ECC slot
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following arguments:
 *      slot: ECC slot to write the public key to
 *  data: pointer to a buffer containing the hex encoded public key data to be written. The public key must be 64 bytes long (i.e. 128 bytes when hex encoded)
 *  data_length: pointer to variable with number of bytes of data in the data buffer.
 *
 */
uint16_t cmd_ecc_pubkey_write(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t slot = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Data is hex encoded so twice the number of bytes as the real data
    if (*data_length != ATCA_PUB_KEY_SIZE*2) {
        // Since no data should be returned from this command the data_length parameter must be set to 0
        *data_length = 0;
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // The data_length value has now been verified so it can just be set to 0 since this command should not return any data
    *data_length = 0;

    if (ECC_PUBKEY_WRITE_NUM_ARGS == argc) {
        // parse argument
        cmd_status = parse_arg_ecc_slot(argv[ECC_PUBKEY_WRITE_ARG_SLOT], &slot);
    } else {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    // Since the hex encoded data requires twice the amount of storage space as
    // the raw binary data it is safe to use the same buffer as source and
    // target for the hex decoding operation
    // Note do not use data_length parameter as it has already been set to 0
    convert_hex2bin(ATCA_PUB_KEY_SIZE*2, data, data);

    atca_status = atcab_write_pubkey(slot, data);

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}


/*
 * Generate signature for digest using device private key
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command does not expect any arguments in argv
 *  data: pointer to a buffer containing the hex encoded digest to sign. The
 *      same buffer will be used to return the generated signature in hex
 *      encoded format (i.e. ATCA_SIG_SIZE*2)
 *  data_length: pointer to variable with number of bytes of data in the data buffer.
 */
uint16_t cmd_ecc_signdigest(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint8_t atca_status = ATCA_SUCCESS;
    uint8_t digest_size = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    if (argc != ECC_SIGNDIGEST_NUM_ARGS) {
        // When reporting a failed command the data_length must be set to 0 to
        // make sure caller does not interpret the incoming blob as the signature
        *data_length = 0;
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }

    // raw binary data takes up half the number of bytes compared to hex encoded
    // data so the same buffer can be used as input and output for the conversion
    digest_size = convert_hex2bin(*data_length, data, data);
    if (digest_size != ATCA_BLOCK_SIZE) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Since the data buffer must have space for the hex encoded version of both
    // the digest and the signature the buffer must have space for the raw binary
    // version in the second half of the buffer
    // Note: it is assumed here that ATCA_SIG_SIZE > ATCA_BLOCK_SIZE
    atca_status = atcab_sign(DEVICE_PRIVATE_KEY_SLOT, data, &data[ATCA_SIG_SIZE]);
    if (atca_status == ATCA_SUCCESS) {
        *data_length = convert_bin2hex(ATCA_SIG_SIZE, &data[ATCA_SIG_SIZE], data);
    } else {
        // Something went wrong, can't trust content of data buffer so no
        // signature can be sent back
        *data_length = 0;
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

/*
 * Read from ECC slot
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following arguments:
 *      slot: ECC slot to read (always reading from start of slot)
 *      length: number of bytes to read. Optional. Should be non-zero but it can
 *              be omitted to indicate the complete slot should be read
 *  data: pointer to a buffer containing the hex encoded data that was read
 *  data_length: pointer to variable with number of bytes of data in the data buffer.
 *      Since the data is hex encoded the data_length will be twice the length
 *      argument of the command
 */
uint16_t cmd_ecc_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t slot = 0;
    uint16_t length = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    cmd_status = parse_and_check_read_args(argc, argv, &slot, &length);

    // No data received yet so set to 0 in case something is wrong and the function returns before the read is done
    *data_length = 0;

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    // Since the data buffer must have space for the hex encoded data it should
    // be safe to read the raw data to the second half of the buffer so that the
    // same buffer can be used as the target when converting from raw binary to
    // hex encoded data
    uint8_t *data_raw = data+length;

    atca_status = atcab_read_bytes_zone(ATCA_ZONE_DATA, slot, 0, data_raw, length);

    if (atca_status == ATCA_SUCCESS) {
        // The data blob should be hex-encoded
        *data_length = convert_bin2hex(length, data_raw, data);
    } else {
        *data_length = 0;
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}


/*
 * Read from ECC OTP area
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following arguments:
 *      length: number of bytes to read. Optional. Should be non-zero but it can
 *              be omitted to indicate the complete OTP area should be read
 *  data: pointer to a buffer containing the hex encoded data that was read
 *  data_length: pointer to variable with number of bytes of data in the data buffer.
 *      Since the data is hex encoded the data_length will be twice the length
 *      argument of the command
 */
uint16_t cmd_ecc_otp_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t dummy_slot = 0;
    uint16_t length = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    cmd_status = parse_and_check_otp_read_args(argc, argv, &length);

    // No data received yet so set to 0 in case something is wrong and the function returns before the read is done
    *data_length = 0;

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    // Since the data buffer must have space for the hex encoded data it should
    // be safe to read the raw data to the second half of the buffer so that the
    // same buffer can be used as the target when converting from raw binary to
    // hex encoded data
    uint8_t *data_raw = data+length;

    atca_status = atcab_read_bytes_zone(ATCA_ZONE_OTP, dummy_slot, 0, data_raw, length);

    if (atca_status == ATCA_SUCCESS) {
        // The data blob should be hex-encoded
        *data_length = convert_bin2hex(length, data_raw, data);
    } else {
        *data_length = 0;
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

// Helper to check input arguments to ECC read command and returns the parsed address and length arguments through the pointer parameters
static uint16_t parse_and_check_read_args(uint8_t argc, char *argv[], uint16_t *slot_parsed, uint16_t *length_parsed)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;

    // Check that arguments are valid
    // slot argument is mandatory, length is optional
    if (argc < (ECC_READ_NUM_ARGS-1) || argc > ECC_READ_NUM_ARGS) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // parse arguments
    cmd_status = parse_arg_ecc_slot(argv[ECC_READ_ARG_SLOT], slot_parsed);
    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    if (argc > 1) {
        // length argument provided
        if (!parse_arg_uint16(argv[ECC_READ_ARG_LENGTH], length_parsed)) {
            return MC_STATUS_BAD_ARGUMENT_VALUE;
        }
    } else {
        // When the no length argument is provided the complete slot should be read
        size_t slot_size;
        atca_status = atcab_get_zone_size(ATCA_ZONE_DATA, *slot_parsed, &slot_size);
        *length_parsed = (uint16_t) slot_size;
        if (atca_status != ATCA_SUCCESS) {
            return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
        }
    }

    // It is not possible to read past the end of the slot but that is handled by CryptoAuthLib
    if (*length_parsed == 0) {
        cmd_status = MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    return cmd_status;
}

// Helper to check input arguments to ECC OTP read command and returns the parsed length argument through the pointer parameters
static uint16_t parse_and_check_otp_read_args(uint8_t argc, char *argv[], uint16_t *length_parsed)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;

    // Check that arguments are valid
    // length is optional
    if (argc < (ECC_OTP_READ_NUM_ARGS-1) || argc > ECC_OTP_READ_NUM_ARGS) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    if (argc > 0) {
        // length argument provided
        if (!parse_arg_uint16(argv[ECC_OTP_READ_ARG_LENGTH], length_parsed)) {
            return MC_STATUS_BAD_ARGUMENT_VALUE;
        }
    } else {
        // When the no length argument is provided the complete area should be read
        size_t slot_size;
        uint16_t dummy_slot = 0;
        atca_status = atcab_get_zone_size(ATCA_ZONE_OTP, dummy_slot, &slot_size);
        *length_parsed = (uint16_t) slot_size;
        if (atca_status != ATCA_SUCCESS) {
            return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
        }
    }

    // It is not possible to read past the end of the area but that is handled by CryptoAuthLib
    if (*length_parsed == 0) {
        cmd_status = MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    return cmd_status;
}

// Helper to check input arguments to ECC write command and returns the parsed address and length arguments through the pointer parameters
static uint16_t parse_and_check_write_args(uint8_t argc, char *argv[], uint16_t *slot_parsed, uint16_t *length_parsed)
{
    uint16_t cmd_status = MC_STATUS_OK;
    // Check that arguments are valid
    if (argc != ECC_WRITE_NUM_ARGS) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // parse arguments
    cmd_status = parse_arg_ecc_slot(argv[ECC_WRITE_ARG_SLOT], slot_parsed);
    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    if (!parse_arg_uint16(argv[ECC_WRITE_ARG_LENGTH], length_parsed)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // It is not possible to write past the end of the slot but that is handled by CryptoAuthLib
    if (*length_parsed == 0) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    return cmd_status;

}

static uint16_t parse_arg_ecc_slot(const char *arg, uint16_t *slot_parsed)
{
    if (parse_arg_uint16(arg, slot_parsed) && *slot_parsed <= MAX_ECC_DATA_SLOT) {
        return MC_STATUS_OK;
    }

    return MC_STATUS_BAD_ARGUMENT_VALUE;
}

/*
 * Write to ECC slot
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command takes the following arguments:
 *      slot: ECC slot to write to (always writing from start of slot)
 *      length: number of bytes to write.  Should be non-zero
 *  data: pointer to a buffer containing the hex encoded data to be written
 *  data_length: pointer to variable with number of bytes of data in the data buffer.
 *      Since the data is hex encoded the data_length should be twice the length
 *      argument of the command
 */
uint16_t cmd_ecc_writeblob(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t slot = 0;
    uint16_t length = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    cmd_status = parse_and_check_write_args(argc, argv,  &slot, &length);

    if (cmd_status != MC_STATUS_OK) {
        // This function never returns any data so data_length should be set to 0
        *data_length = 0;
        return cmd_status;
    }

    // Some extra checks specific to write command
    if (*data_length == 0 || *data_length != length*2) {
        // This function never returns any data so data_length should be set to 0
        *data_length = 0;
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Since the hex encoded data requires twice the amount of storage space as
    // the raw binary data it is safe to use the same buffer as source and
    // target for the hex decoding operation
    convert_hex2bin(*data_length, data, data);

    // This function never returns any data so data_length should be set to 0
    *data_length = 0;

    atca_status = atcab_write_bytes_zone(ATCA_ZONE_DATA, slot, 0, data, length);

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}

/*
 * Lock ECC slot
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command expects argv to contain one argument:
 *      slot: ECC slot to lock
 *  data: pointer to a buffer containing data input and/or output. For this
 *      command the buffer is not used
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.
 */
uint16_t cmd_ecc_lock(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    uint8_t atca_status = ATCA_SUCCESS;
    uint16_t slot = 0;
    bool is_locked;

    // Check that arguments are valid
    if (argc != ECC_LOCK_NUM_ARGS) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // parse argument
    cmd_status = parse_arg_ecc_slot(argv[0], &slot);
    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

	// Check if slot  is locked
	atca_status = atcab_is_slot_locked(slot, &is_locked);
	if (atca_status != ATCA_SUCCESS) {
		return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
    }

	// Lock slot if not already locked
	if(!is_locked) {
		atca_status = atcab_lock_data_slot(slot);
    }

    return STATUS_SOURCE_CRYPTOAUTHLIB(atca_status);
}
