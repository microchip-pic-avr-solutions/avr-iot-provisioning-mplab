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

#ifndef ECC_COMMANDS_H
#define	ECC_COMMANDS_H

#ifdef	__cplusplus
extern "C" {
#endif

enum ecc_read_args{
    ECC_READ_ARG_SLOT = 0,
    // The length argument is optional
    ECC_READ_ARG_LENGTH,
    ECC_READ_NUM_ARGS
};

enum ecc_otp_read_args{
    ECC_OTP_READ_ARG_LENGTH = 0,
    // The length argument is optional
    ECC_OTP_READ_NUM_ARGS
};

enum ecc_write_args{
    ECC_WRITE_ARG_SLOT = 0,
    ECC_WRITE_ARG_LENGTH,
    // The blob length will also exist in the argument list, but it is not needed as the separate data_length parameter
    // contain the same value
    ECC_WRITE_ARG_BLOB_LENGTH,
    ECC_WRITE_NUM_ARGS
};

enum ecc_signdigest_args{
    ECC_SIGNDIGEST_ARG_LENGTH,
    // The blob length will also exist in the argument list, but it is not needed as the separate data_length parameter
    // contain the same value
    ECC_SIGNDIGEST_ARG_BLOB_LENGTH,
    ECC_SIGNDIGEST_NUM_ARGS
};

enum ecc_lock_args{
    ECC_LOCK_ARG_SLOT,
    ECC_LOCK_NUM_ARGS
};

enum ecc_genpubkey_args{
    ECC_GENPUBKEY_ARG_SLOT,
    ECC_GENPUBKEY_NUM_ARGS
};

enum ecc_pubkey_read_args{
    ECC_PUBKEY_READ_ARG_SLOT,
    ECC_PUBKEY_READ_NUM_ARGS
};

enum ecc_pubkey_write_args{
    ECC_PUBKEY_WRITE_ARG_SLOT,
    // Length of blob will also be part of argument list
    ECC_PUBKEY_WRITE_ARG_BLOB_LENGTH,
    ECC_PUBKEY_WRITE_NUM_ARGS
};

uint16_t cmd_ecc_serial(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_genpubkey(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_signdigest(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_pubkey_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_pubkey_write(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_otp_read(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_writeblob(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);
uint16_t cmd_ecc_lock(uint8_t argc, char *argv[], uint8_t *data, uint16_t *data_length);

#ifdef	__cplusplus
}
#endif

#endif	/* ECC_COMMANDS_H */

