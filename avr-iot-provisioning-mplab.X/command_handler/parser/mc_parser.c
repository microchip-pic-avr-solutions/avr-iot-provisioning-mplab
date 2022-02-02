#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mc_parser.h"
#include "mc_error.h"
#include "mc_housekeeping.h"
#include "../mc_commands.h"
#include "../mc_board.h"


static void mc_return_string(const char *string);
static void mc_print_lf( void );
static uint8_t is_newline(char c);

static uint16_t mc_parse_command( void );
static uint16_t mc_parse_arguments(char *argstring, uint8_t length);


void (*put_char)(char) = NULL;


typedef enum {
    STATE_RESET = 0,
    STATE_READY,
    STATE_DATA,
    STATE_ERR
} mc_parser_state_t;

mc_parser_state_t parser_state;


uint8_t  databuffer[MC_DATA_BUFFER_LENGTH];
char  linebuffer[MC_LINE_BUFFER_LENGTH];
char *argbuffer[MC_MAX_ARGUMENTS + 1];
uint16_t expected_datalength, datalength;
uint16_t linecounter;
const mc_command_t *active_command;
uint8_t  argcount;


void mc_parser_init(void (*putc)(char))
{
    uint16_t about_length = 0;

    parser_state = STATE_READY;

    put_char = putc;

    // print hello terminal message with version.
    mc_return_string(MC_HELLOSTRING);
    // Get the about string...
    mc_about(0, NULL, databuffer, &about_length);
    // ...and print it
    mc_return_string((char *) databuffer);
    // Send READY status after welcome string to tell host we are ready to accept commands
    mc_return_string("READY\r\n");
}



void mc_parser(char input)
{
    if (linecounter >= MC_LINE_BUFFER_LENGTH) {
        mc_print_status(MC_STATUS_BUFFER_OVERRUN);
        linecounter = 0;
        if (is_newline(input)) {
            parser_state = STATE_READY;
        } else {
            parser_state = STATE_ERR;
        }
        return;
    }

    switch (parser_state) {
    case STATE_RESET:
        linecounter = 0;
        parser_state = STATE_READY;
        //intentional fall through:
    case STATE_READY:
        if (is_newline(input)) {
            if (linecounter == 0) {
                return; // superfluous newline
            }
        }

        if (linecounter >= MC_LINE_BUFFER_LENGTH) {
            //overrun
            parser_state = STATE_ERR;
            mc_print_status(MC_STATUS_BUFFER_OVERRUN);
            return;
        }

        linebuffer[linecounter++] = input;

        if (is_newline(input)) {
            mc_print_lf();
            uint16_t status = mc_parse_command();
            if (status== MC_STATUS_OK) {
                datalength = 0;
                if (active_command->data) {
                    if (argcount > 0) {
                        // Note using const to please XC8 which takes const 
                        // argument in strtol (GCC takes non-const)
                        const char *endptr;
                        expected_datalength = strtol((char*)argbuffer[argcount-1], &endptr,0);

                        if (expected_datalength > MC_DATA_BUFFER_LENGTH) {
                            expected_datalength = MC_DATA_BUFFER_LENGTH;

                            mc_print_status(MC_STATUS_BAD_BLOB);
                            parser_state = STATE_RESET;
                            return;
                        }

                        if (*endptr == '\0') {
                            mc_return_string(">");
                            parser_state = STATE_DATA;
                            return;

                        } else {
                            mc_print_status(MC_STATUS_BAD_BLOB);
                            parser_state = STATE_RESET;
                            return;
                        }


                    } else {
                        mc_print_status(MC_STATUS_BAD_COMMAND);
                        parser_state = STATE_RESET;
                        return;
                    }
                }
                status = active_command->command_function(argcount,argbuffer,databuffer,&datalength);
                if (datalength && (put_char != NULL)) {
                    for (uint16_t i=0; i < datalength; i++) {
                        put_char(databuffer[i]);
                    }
                    mc_print_lf();
                }
            }
            mc_print_status(status);
            parser_state = STATE_RESET;

            return;
        }

    break;

    case STATE_DATA:
        if ((datalength == 0) && (is_newline(input))) {
            // Blobs are not allowed to start with newlines and the preceding line
            // might have had a full <cr><lf> or other source of spurious line feeds
            return;
        }

        if (datalength < expected_datalength) {
            databuffer[datalength++] = input;

        } else if (is_newline(input)) {
            mc_print_lf();

            uint16_t status = active_command->command_function(argcount,argbuffer,databuffer,&datalength);

            if (datalength && (put_char != NULL)) {
                for (uint16_t i=0; i < datalength; i++) {
                    put_char(databuffer[i]);
                }
            }
            mc_print_lf();
            mc_print_status(status);
            parser_state = STATE_RESET;
            return;

        } else {
            mc_print_status(MC_STATUS_BAD_BLOB);
            parser_state = STATE_ERR;
        }

        return;
    break;

    case STATE_ERR:
        if (is_newline(input)) {
            parser_state = STATE_RESET;
        }
    break;

    default:
        parser_state = STATE_ERR;
    }


}



static uint16_t mc_parse_command( void )
{
    uint16_t status = MC_STATUS_OK;

    char *remainder = linebuffer;

    uint8_t i = 0;
    uint8_t number_of_commands = mc_number_of_commands();
    // loop through command set to look for matches
    for (;i < number_of_commands; i++) {
        if (mc_match_command(mc_command_set[i].command_string, linebuffer, &remainder)) {
            active_command = &mc_command_set[i];
            break;
        }
    }
    if (i < number_of_commands) {
        argcount = 0;

        if ((remainder[0] == '=') && (!is_newline(remainder[1]))) {
            status = mc_parse_arguments(&remainder[1], &linebuffer[MC_LINE_BUFFER_LENGTH - 1] - &remainder[1]);
        }
        return status;
    }

    return MC_STATUS_BAD_COMMAND;
}



static uint16_t mc_parse_arguments(char *argstring, uint8_t length)
{
    uint8_t i = 0;
    uint8_t argument_length = 0;
    argbuffer[argcount] = argstring;

    while (i < length && argcount < MC_MAX_ARGUMENTS) {
        uint8_t c = mc_normalize_char(argstring[i]);

        argstring[i] = c;

        if (is_newline(c)) {
            if (argument_length == 0) {
                return MC_STATUS_BAD_COMMAND;
            }
            argstring[i] = '\0';
            argcount++;
            return MC_STATUS_OK;
        }
        if (c == ',') {
            if (argument_length == 0) {
                return MC_STATUS_BAD_COMMAND;
            }

            argstring[i] = '\0';
            argcount++;
            argbuffer[argcount] = &argstring[i+1];
        }

        argument_length++;
        i++;
    }

    return MC_STATUS_BAD_COMMAND;
}


/*
 * Check if  the (normalized) beginning of "string" matches the content of "partial"
 * return true, and set *remainder to the remainder
 */
uint8_t mc_match_command(const char* partial, char* string, char** remainder)
{

    uint8_t i = 0;
    while (partial[i]) {
        if (partial[i] != mc_normalize_char(string[i])) {
            return 0;
        }
        i++;
    }

    if ((string[i] == '=') || (is_newline(string[i]))) {
        // Complete command token seen
        *remainder = &string[i];
        return 1;
    }

    return 0;
}


uint8_t mc_match_string(const char* match, const char* string)
{
    uint8_t i = 0;
    while (match[i]) {
        if (match[i] != mc_normalize_char(string[i])) {
            return 0;
        }
        i++;
    }
    if (string[i]) {
        return 0;
    }
    return 1;
}

/*
 * sanitize input character and upper-case it
 */
uint8_t mc_normalize_char( char input)
{
    // pass-through the necessary control characters:
    if ((input == '\0')||(input == '\r')||(input == '\n')) {
         return input;
    }
    // Match all printing characters except lowercase:
    if (((input >= ' ') && (input <= '`' )) ||
        ((input >= '{') && (input <= '~')) ) {
        return input;
    }
    // Convert lowercase to uppercase:
    if ((input >= 'a') && (input <= 'z')) {
        return (input - 'a' + 'A');
    }

    //Any other non-printing character becomes a '#'
    return '#';
}



static void mc_return_string(const char *string)
{
    if (put_char != NULL) {
        while (*string) {
            put_char(*string++);
        }
    }
}



static void mc_print_lf( void )
{
    mc_return_string("\r\n");
}



void mc_print_status(uint16_t status)
{
    if (status == 0) {
        mc_return_string("OK\r\n");
    } else {
        char returnstring[32];
        snprintf(returnstring, 32, "ERROR: 0x%X\r\n", status);
        mc_return_string(returnstring);
    }
}



static uint8_t is_newline(char c)
{
    if ((c == '\n') || (c == '\r')) {
        return 1;
    } else {
        return 0;
    }
}
