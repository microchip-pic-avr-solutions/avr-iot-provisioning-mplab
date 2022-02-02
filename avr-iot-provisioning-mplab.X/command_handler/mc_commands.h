#ifndef __MC_COMMANDS_H__
#define __MC_COMMANDS_H__

#include <stdint.h>

typedef struct {
    const char *command_string;
    uint16_t (*command_function)(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
    uint8_t  data; // true/false: should there be a "data" line following the command
} mc_command_t;


extern const mc_command_t mc_command_set[];

uint8_t mc_number_of_commands( void );

#endif /* __MC_COMMANDS_H__ */