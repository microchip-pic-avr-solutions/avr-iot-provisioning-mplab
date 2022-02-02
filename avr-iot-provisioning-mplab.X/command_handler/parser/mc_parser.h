#ifndef __MC_PARSER_H__
#define __MC_PARSER_H__
#include <stdint.h>

#define MC_DATA_BUFFER_LENGTH 1024
#define MC_LINE_BUFFER_LENGTH 128
#define MC_MAX_ARGUMENTS      15

// Command handler version
#define MC_VERSIONSTRING ("1.3.0")

void mc_parser_init(void (*putc)(char));
void mc_parser( char input );

uint8_t mc_normalize_char(char input);
uint8_t mc_match_command(const char* partial, char* string, char** remainder);
uint8_t mc_match_string(const char* match, const char* string);
uint16_t mc_get_version (uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
void mc_print_status(uint16_t status);


#endif /* __MC_PARSER_H__ */
