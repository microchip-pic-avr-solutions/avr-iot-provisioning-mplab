#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

#include "../mcc_generated_files/include/ccp.h"
#include "parser/mc_error.h"
#include "parser/mc_parser.h"
#include "mc_commands.h"
#include "mc_board.h"
#include "../winc_commands.h"

#define LED_ON ("ON")
#define LED_OFF ("OFF")

#define VERSION_WINC ("WINC")

static uint8_t parse_leds(const char *ledstr);

static uint16_t get_winc_version_string(char *version_string, uint16_t *version_length);

struct led_name
{
    const char* name;
    const uint8_t mask;
};

struct led_name led_table[] =
{
    {"ERROR",(1 << 0)},
    {"DATA", (1 << 1)},
    {"CONN", (1 << 2)},
    {"WIFI", (1 << 3)},
    {"ALL",  0x0F}
};

struct led_name *active_led;



void mc_board_init( void )
{
    PORTD.OUTSET = 0xF;
    PORTD.DIRSET = 0xF;
}



uint16_t mc_set_led(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    if (argc != 2) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;

    }
    uint8_t status = parse_leds(argv[0]);
    if (status) {
        return status;
    }

    if (mc_match_string(LED_ON,argv[1])) {
        PORTD.OUTCLR = active_led->mask;
        return MC_STATUS_OK;
    }

    if (mc_match_string(LED_OFF,argv[1])) {
        //current_led off
        PORTD.OUTSET = active_led->mask;
        return MC_STATUS_OK;
    }

    return MC_STATUS_BAD_ARGUMENT_VALUE;
}



uint16_t mc_get_led(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    if (argc != 1) {
        return MC_STATUS_BAD_ARGUMENT_COUNT;
    }
    uint8_t status = parse_leds(argv[0]);
    if (status) {
        return status;
    }
    if (!((PORTD.OUT&active_led->mask )== active_led->mask)) {
        *data_length = snprintf((char*)data,MC_DATA_BUFFER_LENGTH,"LED:%s is ON\n\r",active_led->name);
    } else {
        *data_length = snprintf((char*)data,MC_DATA_BUFFER_LENGTH,"LED:%s is OFF\n\r",active_led->name);
    }

    return MC_STATUS_OK;
}



static uint8_t parse_leds(const char *ledstr)
{
    for (uint8_t i = 0; i < sizeof(led_table)/sizeof(struct led_name); i++) {
        if (mc_match_string(led_table[i].name,ledstr)) {
            active_led = &led_table[i];
            return MC_STATUS_OK;
        }
    }
    return MC_STATUS_BAD_ARGUMENT_VALUE;
}

uint16_t mc_reset(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length)
{
    // Software reset

    // TODO use "../mcc_generated_files/include/rstctrl.h" instead
    // For some reason the mcc-generated code writes a 0x0 to the reset register
    // instead of a 0x1 so the RSTCTRL_reset() functiond does not work,
    // see https://jira.microchip.com/browse/MCCD-2308

    /* SWRR is protected with CCP */
    ccp_write_io((void *)&RSTCTRL.SWRR, 0x1);

    // Execution will never reach this statement due to the reset, but it keeps
    // the compiler happy
    return MC_STATUS_OK;
}

// Extension to mc_version in mc_housekeeping for board specific versions
// The version_arg is the argument to the version command, example:
// MC+VERSION=WINC - The "WINC" part is the argument to be sent in as the version_arg argument
// The version argument is the buffer where the ready formatted version string will be returned
// The value version_length points to will be updated according to the length of the version string
uint16_t get_board_version(char* version_arg, char *version, uint16_t *version_length)
{
    if (mc_match_string(VERSION_WINC, version_arg)) {
        return get_winc_version_string(version, version_length);
    }

    // Unknown version argument
    return MC_STATUS_BAD_ARGUMENT_VALUE;
}

// Get all board specific versions as one string (useful for MC+ABOUT command)
uint16_t get_board_versions(char *versions, uint16_t *version_length)
{
    // The only board specific version for this board is the WINC version
    return get_winc_version_string(versions, version_length);
}

/*
 *  Read out WINC version information and convert to a human readable string
 */
static uint16_t get_winc_version_string(char *version_string, uint16_t *version_length)
{
    uint16_t status;
    tstrM2mRev winc_version;

    status = read_winc_version(&winc_version);
    if (status != MC_STATUS_OK) {
        return status;
    }

    *version_length = snprintf(version_string,
                               BOARD_VERSIONS_MAX_LENGTH,
                               "WINC firmware %d.%d.%d\r\nWINC driver %d.%d.%d\r\n",
                               winc_version.u8FirmwareMajor,
                               winc_version.u8FirmwareMinor,
                               winc_version.u8FirmwarePatch,
                               winc_version.u8DriverMajor,
                               winc_version.u8DriverMinor,
                               winc_version.u8DriverPatch);

    return MC_STATUS_OK;
}
