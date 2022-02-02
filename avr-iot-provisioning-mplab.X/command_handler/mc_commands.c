#include <stdbool.h>
#include "mc_commands.h"

#include "parser/mc_housekeeping.h"  // for mc_get_version()
#include "mc_board.h"  // for mc_set_led, mc_get_led, mc_reset
#include "../winc_commands.h" // for WINC related commands
#include "../ecc_commands.h" // for ECC related commands

#define MC_NUMBER_OF_COMMANDS sizeof(mc_command_set)/sizeof(mc_command_t)

const mc_command_t mc_command_set[] = {
    {"MC+ABOUT",mc_about,false},
    {"MC+BLOBTEST",mc_blobtest, true},
    {"MC+PING", mc_ping, false},
    {"MC+ECC+GENPUBKEY", cmd_ecc_genpubkey, false},
    {"MC+ECC+LOCK", cmd_ecc_lock, false},
    {"MC+ECC+OTP+READ", cmd_ecc_otp_read, false},
    {"MC+ECC+READ", cmd_ecc_read, false},
    {"MC+ECC+PUBKEY+READ", cmd_ecc_pubkey_read, false},
    {"MC+ECC+SERIAL", cmd_ecc_serial, false},
    {"MC+ECC+SIGNDIGEST", cmd_ecc_signdigest, true},
    {"MC+ECC+WRITEBLOB", cmd_ecc_writeblob, true},
    {"MC+ECC+PUBKEY+WRITE", cmd_ecc_pubkey_write, true},
    {"MC+GETLED",mc_get_led, false},
    {"MC+LISTCOMMANDS",mc_list_commands,false},
    {"MC+RESET",mc_reset, false},
    {"MC+SETLED",mc_set_led, false},
    {"MC+VERSION",mc_get_version, false},
    {"MC+WINC+ERASE",cmd_winc_erasesector, false},
    {"MC+WINC+READ",cmd_winc_read, false},
    {"MC+WINC+WRITEBLOB",cmd_winc_writeblob, true}
};

uint8_t mc_number_of_commands( void )
{
    return MC_NUMBER_OF_COMMANDS;
}
