#ifndef __MC_HOUSEKEEPING_H__
#define __MC_HOUSEKEEPING_H__
#include <stdint.h>

uint16_t mc_get_version (uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_blobtest(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_ping(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_about(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t mc_list_commands(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);

#endif
