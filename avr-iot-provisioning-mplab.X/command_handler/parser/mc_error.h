#ifndef __MC_ERROR_H__
#define __MC_ERROR_H__


#define MC_STATUS_OK                 0
#define MC_STATUS_BAD_COMMAND        1
#define MC_STATUS_BUFFER_OVERRUN     2
#define MC_STATUS_BAD_ARGUMENT_COUNT 3
#define MC_STATUS_BAD_BLOB           4
#define MC_STATUS_BAD_ARGUMENT_VALUE 5

// Convert 8-bit status to 16-bit command handler status
// source is the module returning the status
// status is an 8-bit status code defined by the module, but for all modules 0 means OK
#define TO_MC_STATUS(source, status) \
    (status ? (((uint16_t) source) << 8)|((uint8_t) status) : MC_STATUS_OK)

#define STATUS_SOURCE_COMMAND_HANDLER(status) TO_MC_STATUS(0, status)
#define STATUS_SOURCE_CRYPTOAUTHLIB(status) TO_MC_STATUS(1, status)
#define STATUS_SOURCE_WINC(status) TO_MC_STATUS(2, status)

#endif
