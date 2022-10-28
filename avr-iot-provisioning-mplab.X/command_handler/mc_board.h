#ifndef __MC_BOARD_H__
#define __MC_BOARD_H__

void mc_board_init( void );

uint16_t mc_set_led(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_get_led(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_reset(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);

uint16_t get_board_version(char *version_arg, char *version, uint16_t *version_length);
uint16_t get_board_versions(char *versions, uint16_t *version_length);

#define MC_HELLOSTRING   ("Welcome to the avr-iot command handler!\r\n")
#define MC_BOARD_NAME    ("AVR-IoT WG")
#define MC_FW_NAME       ("AVR-IoT provisioning FW")
// Version of the complete firmware (as opposed to the Command handler version MC_VERSIONSTRING found in mc_parser.h)
#define MC_FW_VERSION    ("0.4.9")

// WINC version contains both a firmware version and a driver version
// "WINC firmware xx.yy.zz\r\nWINC driver xx.yy.zz\r\n"
// There must be space for null termination
#define BOARD_VERSIONS_MAX_LENGTH 47

#endif
