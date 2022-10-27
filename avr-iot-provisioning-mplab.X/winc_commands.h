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

#ifndef WINC_COMMANDS_H
#define WINC_COMMANDS_H

#include <stdint.h>
#include <stdbool.h>
#include "mcc_generated_files/winc/m2m/m2m_types.h"

enum winc_read_write_args{
    WINC_READ_WRITE_ARG_ADDRESS = 0,
    WINC_READ_WRITE_ARG_LENGTH,
    // For write the blob length will also exist in the argument list, but it
    // is not needed as the separate data_length parameter contain the same
    // value
    WINC_WRITE_ARG_BLOB_LENGTH,
    WINC_WRITE_NUM_ARGS
};

#define WINC_READ_NUM_ARGS WINC_WRITE_ARG_BLOB_LENGTH

uint16_t winc_init(void);
uint16_t winc_download_mode(bool set);

uint16_t cmd_winc_writeblob(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_winc_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);

enum winc_erase_args{
    WINC_ERASE_ARG_ADDRESS = 0,
    WINC_ERASE_NUM_ARGS
};

uint16_t cmd_winc_erasesector(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);

uint16_t read_winc_version(tstrM2mRev *version_info);

#endif // WINC_COMMANDS_H
