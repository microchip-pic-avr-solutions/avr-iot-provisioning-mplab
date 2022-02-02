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

#include <stdlib.h>
#include <string.h>
#include "winc_commands.h"
#include "mcc_generated_files/winc/spi_flash/spi_flash.h"
#include "mcc_generated_files/winc/spi_flash/spi_flash_map.h"
#include "mcc_generated_files/winc/m2m/m2m_wifi.h"
#include "mcc_generated_files/winc/common/winc_defines.h"
#include "mcc_generated_files/winc/m2m/m2m_fwinfo.h"
#include "mcc_generated_files/delay.h"
#include "command_handler/parser/mc_parser.h"
#include "command_handler/mc_argparser.h"
#include "command_handler/parser/mc_error.h"
#include "conversions.h"

// Helpers
static uint16_t parse_and_check_write_args(uint8_t argc, char *argv[], uint32_t *address_parsed, uint16_t *length_parsed);
static uint16_t parse_and_check_read_args(uint8_t argc, char *argv[], uint32_t *address_parsed, uint16_t *length_parsed);
static uint16_t parse_and_check_erase_args(uint8_t argc, char *argv[], uint32_t *address_parsed);
static uint16_t parse_and_check_common_args(uint8_t argc, char *argv[], uint8_t num_args, uint32_t *address_parsed, uint16_t *length_parsed);
static uint16_t winc_download_mode(bool set);

uint16_t winc_init(void) {
    // Initialize WINC stack
    return winc_download_mode(false);
}

/*
 * Write data blob (max one page) to WINC flash
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command expects argv to contain two arguments:
 *      destination: address/offset to start writing at
 *      length: number of bytes to write.  The length argument should match the data_length parameter and should be non-zero
 *  data: pointer to a buffer containing the hex encoded data to be written.
 *      This buffer might be used for sending data back from the function, but
 *      for this instance there will not be any data to return
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.  This parameter is a
 *      pointer so that the command implementation can modify it to tell the caller how much data is returned through
 *      the buffer pointed to by the data parameter.  For this instance there will not be any data to return so the
 *      value pointed to by data_length will always be set to 0
 */
uint16_t cmd_winc_writeblob(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    int8_t m2m_status = M2M_SUCCESS;
	uint32_t address = 0;
    uint16_t length = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    cmd_status = parse_and_check_write_args(argc, argv, &address, &length);

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

    winc_download_mode(true);
	m2m_status = spi_flash_write(data, address, length);
    winc_download_mode(false);

    return STATUS_SOURCE_WINC(m2m_status);
}

/*
 * Read data from WINC flash
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command expects argv to contain two arguments:
 *      destination: address/offset to start reading from
 *      length: number of bytes to read.  Should be non-zero
 *  data: pointer to a buffer where the read data will be returned.
 *      The returned data will be hex-encoded so the size must be equal to the
 *      length argument times 2
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.  This parameter is a
 *      pointer so that the command implementation can modify it to tell the caller how much data is returned through
 *      the buffer pointed to by the data parameter.
 */
uint16_t cmd_winc_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
	uint16_t cmd_status = MC_STATUS_OK;
    int8_t m2m_status = M2M_SUCCESS;
    uint32_t address = 0;
    uint16_t length = 0;

    if (!check_pointers(data, data_length)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    cmd_status = parse_and_check_read_args(argc, argv, &address, &length);

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
    winc_download_mode(true);
	m2m_status = spi_flash_read(data_raw, address, length);
    winc_download_mode(false);

    if (m2m_status != M2M_SUCCESS) {
        // Something went wrong, can't trust the data so tell the caller no data returned
        *data_length = 0;
    } else {
        // The data blob should be hex-encoded
        *data_length = convert_bin2hex(length, data_raw, data);
    }

    return STATUS_SOURCE_WINC(m2m_status);
}

// Helper to check input arguments to WINC read command and returns the parsed address and length arguments through the pointer parameters
static uint16_t parse_and_check_read_args(uint8_t argc, char *argv[], uint32_t *address_parsed, uint16_t *length_parsed)
{
    uint16_t cmd_status;

    cmd_status = parse_and_check_common_args(argc, argv, WINC_READ_NUM_ARGS, address_parsed, length_parsed);

    return cmd_status;
}

// Helper to check input arguments to WINC write command and returns the parsed address and length arguments through the pointer parameters
static uint16_t parse_and_check_write_args(uint8_t argc, char *argv[], uint32_t *address_parsed, uint16_t *length_parsed)
{
    uint16_t cmd_status;

    cmd_status = parse_and_check_common_args(argc, argv, WINC_WRITE_NUM_ARGS, address_parsed, length_parsed);

    return cmd_status;
}

// Helper to parse and check address and length parameters common to both read and write commands and returns the parsed arguments through the pointer parameters
static uint16_t parse_and_check_common_args(uint8_t argc, char *argv[], uint8_t num_args, uint32_t *address_parsed, uint16_t *length_parsed)
{
    // Check that arguments are valid
    if (argc != num_args) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // parse arguments
    if (!parse_arg_uint32(argv[WINC_READ_WRITE_ARG_ADDRESS], address_parsed) ||
        !parse_arg_uint16(argv[WINC_READ_WRITE_ARG_LENGTH], length_parsed)) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // Even though the spi_flash_write function support bigger chunks than one page the write CLI command only support
    //  chunks up to one page to limit buffer sizes needed
    // Writing 0 bytes is also regarded as an error
    if (*length_parsed > FLASH_PAGE_SZ || *length_parsed == 0) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    return MC_STATUS_OK;

}

/*
 * Erase WINC sector starting at provided address
 *
 * Parameters:
 *  argc: number of items in the argv parameter
 *  argv: command arguments, this command expects argv to contain one argument:
 *      address: address/offset to start erasing at
 *  data: pointer to a buffer containing the data to be written.  This buffer might be used for sending data back from
 *      the function, but for this instance there will not be any data to return
 *  data_length: pointer to variable with number of bytes of actual data in the data buffer.  This parameter is a
 *      pointer so that the command implementation can modify it to tell the caller how much data is returned through
 *      the buffer pointed to by the data parameter.  For this instance there will not be any data to return so the
 *      value pointed to by data_length will always be set to 0
 */
uint16_t cmd_winc_erasesector(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    uint16_t cmd_status = MC_STATUS_OK;
    int8_t m2m_status = M2M_SUCCESS;
    uint32_t address = 0;

    cmd_status = parse_and_check_erase_args(argc, argv, &address);

    // The erase command should not return any data
    *data_length = 0;

    if (cmd_status != MC_STATUS_OK) {
        return cmd_status;
    }

    winc_download_mode(true);
	m2m_status = spi_flash_erase(address, FLASH_SECTOR_SZ);
    winc_download_mode(false);

    return STATUS_SOURCE_WINC(m2m_status);
}

// Helper to check input arguments to WINC erase command and returns the parsed address argument through the pointer parameters
static uint16_t parse_and_check_erase_args(uint8_t argc, char *argv[], uint32_t *address_parsed)
{
    // Check that arguments are valid
    if (argc != WINC_ERASE_NUM_ARGS) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    if (!argv) {
        return MC_STATUS_BAD_ARGUMENT_VALUE;
    }

    // parse arguments
    *address_parsed = (uint32_t) strtol((const char *) argv[WINC_ERASE_ARG_ADDRESS], NULL, 0);

    return MC_STATUS_OK;
}

// Set/unset WINC download mode
static uint16_t winc_download_mode(bool set)
{
    int8_t m2m_status = M2M_SUCCESS;
    if (set) {
        // First check that the WINC isn't already initialized
        if(WIFI_STATE_DEINIT != m2m_wifi_get_state()) {
            // WINC is already initialized and must be de-initialized first
            m2m_wifi_deinit(NULL);
        }
        m2m_status = m2m_wifi_download_mode();
    } else {
        tstrWifiInitParam wifi_parameters;
        memset((uint8_t*)&wifi_parameters, 0, sizeof(wifi_parameters));
        m2m_status = m2m_wifi_init(&wifi_parameters);
    }
    DELAY_milliseconds(250);
    return STATUS_SOURCE_WINC(m2m_status);
}

/*
 * Read WINC firmware version
 *
 * Parameters:
 *  version_info: pointer to struct where version info can be returned
 */
uint16_t read_winc_version(tstrM2mRev *version_info)
{
    int8_t m2m_status = M2M_SUCCESS;

	m2m_status = m2m_fwinfo_get_firmware_info(true, version_info);
	return STATUS_SOURCE_WINC(m2m_status);
}
