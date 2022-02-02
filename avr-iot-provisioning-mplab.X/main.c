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

#include "mcc_generated_files/mcc.h"
#include "conversions.h"
#include "mcc_generated_files/drivers/uart.h"
#include "mcc_generated_files/delay.h"
#include "mcc_generated_files/winc/m2m/m2m_types.h"
#include "mcc_generated_files/winc/m2m/m2m_wifi.h"
#include "mcc_generated_files/winc/common/winc_defines.h"
#include "command_handler/parser/mc_parser.h"
#include "command_handler/mc_board.h"
#include "winc_commands.h"
#include "mcc_generated_files/CryptoAuthenticationLibrary/basic/atca_basic.h"

// Helper to convert from char to uint8_t since the driver layer requires uint8_t argument
void cdc_putc(char data)
{
    uart[cdc].Write((uint8_t) data);
}

int main(void)
{
    /* Initializes MCU, drivers and middleware */
    SYSTEM_Initialize();

    mc_board_init();

    // Initialize WINC stack
    winc_init();

    // Run an ECC selftest of the Random Number Generator to reset the Health Test Error flag.
    // Although the RNG might have intermittent failures setting this flag, the flag is
    // sticky and must be reset somehow. If not all commands requiring a random number will fail.
    uint8_t selftest_result = 0;
    // Just ignore any errors, not much we can do
    atcab_selftest(SELFTEST_MODE_RNG, 0, &selftest_result);

    // Initializing the parser includes sending a welcome message to the host
    // This message can be used by the host to know when the boot-up is done, but
    // then it must be done as the last step before entering the parser loop
    mc_parser_init(cdc_putc);
    while(1) {
        mc_parser(uart[cdc].Read());
    }

}
/**
    End of File
*/
