/*
    (c) 2019 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

#include <string.h>
#include "atca_hal.h"
#include "atca_i2c_hal.h"
#include "../atca_device.h"
#include "../atca_status.h"
#include "../../mcc.h"

#include "../../drivers/i2c_simple_master.h"
// Needed for ECC wake fix (see https://jira.microchip.com/browse/PAI-1099 and https://confluence.microchip.com/x/1YXDEg)
#include "../../include/twi0_master.h"



/** \brief initialize an I2C interface using given config
 * \param[in] hal - opaque ptr to HAL data
 * \param[in] cfg - interface configuration
 */
uint8_t i2c_address = 0;

ATCA_STATUS hal_i2c_init(void *hal, ATCAIfaceCfg *cfg)
{
        i2c_address = (cfg->atcai2c.slave_address) >> 1;

	return ATCA_SUCCESS;
}


/** \brief HAL implementation of I2C post init
 * \param[in] iface  instance
 * \return ATCA_SUCCESS
 */
ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
    return ATCA_SUCCESS;
}


/** \brief HAL implementation of I2C send over ASF
 * \param[in] iface     instance
 * \param[in] txdata    pointer to space to bytes to send
 * \param[in] txlength  number of bytes to send
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t *txdata, int txlength)
{
	txdata[0] = 0x03; // insert the Word Address Value, Command token
	txlength++;       // account for word address value byte.

	i2c_writeNBytes(i2c_address, txdata, txlength);

	return ATCA_SUCCESS;
}

/** \brief HAL implementation of I2C receive function for ASF I2C
 * \param[in] iface     instance
 * \param[in] rxdata    pointer to space to receive the data
 * \param[in] rxlength  ptr to expected number of receive bytes to request
 * \return ATCA_STATUS
 */

ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t *rxdata, uint16_t *rxlength)
{
	uint16_t      rxdata_max_size = *rxlength;

	*rxlength = 0;
	if (rxdata_max_size < 1) {
		return ATCA_SMALL_BUFFER;
	}

	*rxdata = 0;
	i2c_readNBytes(i2c_address, rxdata, 1);

	if ((rxdata[0] < ATCA_RSP_SIZE_MIN) || (rxdata[0] > ATCA_RSP_SIZE_MAX)) {
		return ATCA_INVALID_SIZE;
	}
	if (rxdata[0] > rxdata_max_size) {
		return ATCA_SMALL_BUFFER;
	}

	i2c_readNBytes(i2c_address, &rxdata[1], rxdata[0] - 1);
	*rxlength = rxdata[0];

	return ATCA_SUCCESS;
}

/** \brief wake up CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to wakeup
 */

ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
    ATCAIfaceCfg *cfg  = atgetifacecfg(iface);
	uint8_t       data[4];
    // Not needed after adding manual fix for ECC wake
	//uint8_t       zero_byte = 0;
    uint8_t       second_byte = 0xFF;

    // Manual fix for ECC wake issue,
    // see https://jira.microchip.com/browse/PAI-1099 and https://confluence.microchip.com/x/1YXDEg
	// i2c_writeNBytes(i2c_address, &zero_byte, 1);
    while(!I2C0_Open(0x00));
    I2C0_SetBuffer((uint8_t*)&second_byte, 1);
    // notice that we are passing in a NULL instead of i2c_restartWrite; this terminates transfer when seeing the first NACK
    I2C0_SetAddressNackCallback(NULL, NULL);
    I2C0_MasterWrite();
    while(I2C0_BUSY == I2C0_Close()); // sit here until finished.

	atca_delay_us(cfg->wake_delay);

	// receive the wake up response
	i2c_readNBytes(i2c_address, data, 4);

	return hal_check_wake(data, 4);

}

/** \brief idle CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to idle
 */

ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
	uint8_t data = 0x02;

	i2c_writeNBytes(i2c_address, &data, 1);

	return ATCA_SUCCESS;
}

/** \brief sleep CryptoAuth device using I2C bus
 * \param[in] iface  interface to logical device to sleep
 */

ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
	uint8_t data = 0x01;

	i2c_writeNBytes(i2c_address, &data, 1);
        return ATCA_SUCCESS;
}

/** \brief manages reference count on given bus and releases resource if no more refences exist
 * \param[in] hal_data - opaque pointer to hal data structure - known only to the HAL implementation
 * return ATCA_SUCCESS
 */

ATCA_STATUS hal_i2c_release(void *hal_data)
{
    //TODO: For the moment, don't do anything

    return ATCA_SUCCESS;
}

/** \brief discover i2c buses available for this hardware
 * this maintains a list of logical to physical bus mappings freeing the application
 * of the a-priori knowledge
 * \param[in] i2c_buses - an array of logical bus numbers
 * \param[in] max_buses - maximum number of buses the app wants to attempt to discover
 * \return ATCA_SUCCESS
 */

ATCA_STATUS hal_i2c_discover_buses(int i2c_buses[], int max_buses)
{

    //TODO: For the moment, don't do anything

    return ATCA_SUCCESS;
}

/** \brief discover any CryptoAuth devices on a given logical bus number
 * \param[in]  busNum  logical bus number on which to look for CryptoAuth devices
 * \param[out] cfg     pointer to head of an array of interface config structures which get filled in by this method
 * \param[out] found   number of devices found on this bus
 * \return ATCA_SUCCESS
 */

ATCA_STATUS hal_i2c_discover_devices(int busNum, ATCAIfaceCfg cfg[], int *found)
{
	//TODO: For the moment, don't do anything

    return ATCA_SUCCESS;
}

//*** Section below is not part of MCC generated code ****

// This code was added to be able to get in sync with an ATECC608 device after
// system resets, ESD events, noise on I2C lines etc.
// The algorithm for this synchronization sequence is described in the ATECC608B
// data sheet section 7.8 I2C synchronization

static void i2c2_sda_set_output(void) { PA2_SetDigitalOutput(); }
static void i2c2_sda_set_input(void) { PA2_SetDigitalInput(); }
static void i2c2_sda_set(bool on)
{
    if (on) {
        PA2_SetHigh();
    } else {
        PA2_SetLow();
    }
}
static void i2c2_scl_set_output(void) { PA3_SetDigitalOutput(); }
static void i2c2_scl_set_input(void) { PA3_SetDigitalInput(); }
static void i2c2_scl_set(bool on)
{
    if (on) {
        PA3_SetHigh();
    } else {
        PA3_SetLow();
    }
}

// Attempt read, but terminate upon first NACK
static twi0_error_t attempt_read(void){
    uint8_t *rxdata = 0;
    while(I2C0_BUSY == I2C0_Open(i2c_address)){
        // sit here until we get the bus..
    }
    I2C0_SetBuffer(rxdata, 1);
    // notice that we are passing in a NULL instead of i2c_restartRead; this terminates transfer when seeing the first NACK
    I2C0_SetAddressNackCallback(NULL, NULL);
    I2C0_MasterRead();
    twi0_error_t i2c_err = I2C0_Close();
    while(I2C0_BUSY == i2c_err){
        // sit here until finished.
        i2c_err = I2C0_Close();
    }
    return i2c_err;
}

/** \brief  Reset I2C bus and re-sync ECC communication
 * \param[in] iface  interface to logical device to wakeup
 */
ATCA_STATUS hal_ecc_sync(ATCAIface iface)
{
    // The I2C module does not support I2C bus software reset, so it must be
    // done manually
    // Close the bus
    I2C0_Close();
    // Disable the I2C module (to enable I2C lines as normal GPIO)
    // Non public function in twi0_master driver: I2C0_MasterClose();
    TWI0.MCTRLA = 0 << TWI_ENABLE_bp;

    // Start by SDA driven high by external pull-up and SCL actively driven high
    // since we are the host driving the communication
    i2c2_sda_set_input();
    i2c2_scl_set(true);
    i2c2_scl_set_output();

    // Start condition
    i2c2_sda_set(false);
    i2c2_sda_set_output();
    // Should be at least 250 ns delay
    atca_delay_us(1);

    // Nine clocks while SDA is held high by pull-up resistor
    i2c2_scl_set(false);
    i2c2_sda_set_input();
    // Should be at least 400 ns delay
    atca_delay_us(1);
    i2c2_scl_set(true);
    // Should be at least 400 ns delay
    atca_delay_us(1);
    // Remaining 8 cycles
    for (int i=0;i<8;i++){
        i2c2_scl_set(false);
        // Should be at least 400 ns delay
        atca_delay_us(1);
        i2c2_scl_set(true);
        // Should be at least 400 ns delay
        atca_delay_us(1);
    }

    // Start condition
    i2c2_sda_set(false);
    i2c2_sda_set_output();
    // Should be at least 400 ns delay
    atca_delay_us(1);

    // Stop condition
    i2c2_sda_set_input();
    // Should be at least 500 ns delay
    atca_delay_us(1);

    // Release clock (will stay high due to pull-up)
    i2c2_scl_set_input();


    // Back to using the I2C module again
    // Non public function in twi0_master driver: I2C0_MasterOpen();
    TWI0.MCTRLA = 1 << TWI_ENABLE_bp;

    // Attempt a read
    twi0_error_t i2c_err = attempt_read();
    if (I2C0_FAIL == i2c_err){
        // Read failed, ECC device might be at sleep
        hal_i2c_wake(iface);
        i2c_err = attempt_read();
        if (I2C0_FAIL == i2c_err){
            // Read failed again, device might be busy. Wait max execution time
            atca_delay_ms(1900);
            // Try one last time
            i2c_err = attempt_read();
            if (I2C0_FAIL == i2c_err){
                return ATCA_TIMEOUT;
            }
        }
    }

    // Reset the ECC device by writing to the reset register
	uint8_t data = 0x00;
	i2c_writeNBytes(i2c_address, &data, 1);

    return ATCA_SUCCESS;
}


/** @} */
