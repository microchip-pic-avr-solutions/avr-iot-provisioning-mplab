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
#include "mc_argparser.h"
#include "stdlib.h"
#include <stdio.h>

// Helper that parses and checks argument as unsigned long value
static bool parse_arg_ulong(const char *arg, unsigned long *arg_parsed)
{
    // Note using const to please XC8 which takes const
    // argument in strtol (GCC takes non-const)
    const char *endptr;

    // Check that argument is not empty
    if (arg[0] == '\0') {
        return false;
    }

    *arg_parsed = strtoul(arg, &endptr, 0);

    // Check for null termination to catch incorrectly formatted commands
    if (*endptr != '\0') {
        return false;
    }

    return true;
}

// Helper that parses and checks argument that is supposed to be an uint16
// Returns true if parsing was successful, false if the argument is invalid
bool parse_arg_uint16(const char *arg, uint16_t *arg_parsed)
{
    unsigned long argvalue;

    if (parse_arg_ulong(arg, &argvalue)) {
        // Parsed value must fit within a uint16
        if (argvalue >= 0 && argvalue <= UINT16_MAX) {
            *arg_parsed = (uint16_t) argvalue;
            return true;
        }
    }

    return false;
}

// Helper that parses and checks argument that is supposed to be an uint32
// Returns true if parsing was successful, false if the argument is invalid
bool parse_arg_uint32(const char *arg, uint32_t *arg_parsed)
{
    unsigned long argvalue;

    if (parse_arg_ulong(arg, &argvalue)) {
        // Parsed value must fit within a uint32
        if (argvalue >= 0 && argvalue <= UINT32_MAX) {
            *arg_parsed = (uint32_t) argvalue;
            return true;
        }
    }

    return false;
}

// Helper to check data and data_length arguments
// Returns true if pointers are valid, false if not
bool check_pointers(const uint8_t *data, uint16_t *data_length)
{
    if (!data_length) {
        return false;
    } else if (!data) {
        // Data pointer is invalid so data can't be used or returned
        *data_length = 0;
        return false;
    }

    return true;
}
