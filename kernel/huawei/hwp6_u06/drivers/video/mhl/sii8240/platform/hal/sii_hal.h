/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **********************************************************************************/

/**
 * @file sii_hal.h
 *
 * @brief Defines the hardware / OS abstraction layer API used by Silicon
 *        Image MHL drivers.
 *
 * $Author: Dave Canfield
 * $Rev: $
 * $Date: Jan. 24, 2011
 *
 *****************************************************************************/

#if !defined(SII_HAL_H)
#define SII_HAL_H

#include <linux/kernel.h>


#ifdef __cplusplus
extern "C" {
#endif  /* _defined (__cplusplus) */


#if defined(DEBUG)
        /** @brief If true, enable debug messages from the driver to be printed. */
        extern bool gSiiDrvDbgPrintEnable;
#endif


        /***** public type definitions ***********************************************/

        /**
         * @defgroup sii_hal_api Hardware Abstraction Layer (HAL) API
         * @{
         */

        /**
         *  Signature of interrupt handler function used by the driver layer code.
         */
        typedef void (*fwIrqHandler_t)(void);


        /**
         * Signature of timer callback function used with HalRequestTimerCallback().
         */
        typedef void (*timerCallbackHandler_t)(void *);


        /** Status return value type for HAL module functions.*/
        typedef enum
        {
                HAL_RET_SUCCESS,			/**< The operation was successfully completed */
                HAL_RET_FAILURE,			/**< Generic error */
                HAL_RET_PARAMETER_ERROR,	/**< Invalid parameter passed to a HAL function */
                HAL_RET_NO_DEVICE,			/**< Indicates that the specified hardware device was not found */
                HAL_RET_DEVICE_NOT_OPEN,	/**< The specified device is not open for use */
                HAL_RET_NOT_INITIALIZED,	/**< HAL abstraction layer module has not been initialized*/
                HAL_RET_OUT_OF_RESOURCES,	/**< Needed resources (memory/hardware) were not available. */
                HAL_RET_TIMEOUT,			/**< The requested operation timed out */
                HAL_RET_ALREADY_INITIALIZED /**< HalInit called more than once */
        } halReturn_t;


        /** Define IDs of timer counters supported */
#define ELAPSED_TIMER               0xFF
#define ELAPSED_TIMER1              0xFE


        /** @brief GPIO pin state definitions taken from the firmware's GPIO header file */
#define settingMode3X				0
#define settingMode1X				1
#define settingMode9290				0
#define settingMode938x				1
#define GPIO_PIN_SW5_P4_ENABLED		0
#define GPIO_PIN_SW5_P4_DISABLED	1



        /***** public function prototypes ********************************************/


        /*****************************************************************************/
        /**
         * @brief Initialize the HAL layer module.
         *
         *  This function provides the HAL module the opportunity to perform any
         *  needed initialization and must be called BEFORE any other HAL functions.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				HAL initialization successful
         *  @retval HAL_RET_ALREADY_INITIALIZED	HAL already initialized
         *  @retval HAL_RET_FAILURE				An error occurred during initialization
         *
         *****************************************************************************/
        halReturn_t HalInit(void);



        /*****************************************************************************/
        /**
         * @brief Terminate access to the hardware abstraction layer.
         *
         *  This function must be called when access to the hardware abstraction layer
         *  support is no longer required, for example when the driver using the services
         *  of the HAL is being stopped or unloaded. Resources acquired during HalInit
         *  will be released and any subsequent attempts to call any other HAL function
         *  (except for HalInit) will result in an HAL_RET_NOT_INITIALIZED error.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				HAL terminated successfully
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (HalInit not called)
         *
         *****************************************************************************/
        halReturn_t HalTerm(void);


        /*****************************************************************************/
        /**
         * @brief Request access to the specified I2c device.
         *
         *  MHL transmitters are I2c devices which must be successfully opened using
         *  this function before attempting to use any of the HAL I2c or SMBUS
         *  communication functions.
         *
         *  @param[in]		DeviceName		Name of the I2C device to be opened.
         *  @param[in]		DriverName		Name of the driver opening the I2C device.
         *
         *  @return	status (success or error code)
         *  @retval	HAL_RET_SUCCESS				MHL transmitter successfully opened
         *	@retval	HAL_RET_PARAMETER_ERROR		DeviceName too long
         *	@retval	HAL_RET_NO_DEVICE			DeviceName does not exist
         *
         *****************************************************************************/
        halReturn_t HalOpenI2cDevice(char const *DeviceName, char const *DriverName);



        /*****************************************************************************/
        /**
         * @brief Terminate access to an MHL transmitter device.
         *
         *  Called when access to the previously opened I2c device is no longer needed.
         *  Causes any resources used to communicate with the device to be released.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				MHL transmitter successfully closed
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *
         *****************************************************************************/
        halReturn_t HalCloseI2cDevice(void);


#if 0 	// These are all replaced by CRA functions
        /*****************************************************************************/
        /**
         * @brief Read a byte from an I2c device using SMBus protocol.
         *
         *  Reads a byte from the previously opend I2c device using
         *  the SMBus Byte Data protocol.
         *
         *  @param[in]		command				SMBus command value to send.
         *  @param[out]		pRetByteRead		Pointer to where the byte read is returned.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Read successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusReadByteData(uint8_t command, uint8_t *pRetByteRead);



        /*****************************************************************************/
        /**
         * @brief Write a byte to an I2c device using SMBus protocol.
         *
         *  Writes a byte to the previously opened I2c device using
         *  the SMBus Byte Data protocol.
         *
         *  @param[in]		command				SMBus command value to send.
         *  @param[in]		writeByte			Byte value to write to I2c device.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Write successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusWriteByteData(uint8_t command, uint8_t writeByte);



        /*****************************************************************************/
        /**
         * @brief Read a word from an I2c device using SMBus protocol.
         *
         *  Reads a word from the previously opened I2c device using
         *  the SMBus Word Data protocol.
         *
         *  @param[in]		command				SMBus command value to send.
         *  @param[out]		pRetWordRead		Pointer to where the word read is returned.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Read successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusReadWordData(uint8_t command, uint16_t *pRetWordRead);



        /*****************************************************************************/
        /**
         * @brief Write a word to an I2c device using SMBus protocol.
         *
         *  Writes a word to the previously opened I2c device using
         *  the SMBus Word Data protocol.
         *
         *  @param[in]		command				SMBus command value to send.
         *  @param[in]		wordData			Word value to write to I2c device.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Write successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusWriteWordData(uint8_t command, uint16_t wordData);



        /*****************************************************************************/
        /**
         * @brief Read a series of bytes from an I2c device using SMBus protocol.
         *
         *  Uses the SMbus Block Data protocol to read up to 32 bytes from the
         *  previously opened I2c device.
         *
         *  @param[out]		command				SMBus command value to send.
         *  @param[out]		buffer				Buffer where data read will be returned.
         *  @param[in,out]	bufferLen			On input specifies the length of buffer,
         *  									on return contains number of bytes read.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Read successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_PARAMETER_ERROR		Transfer length too large
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusReadBlock(uint8_t command, uint8_t *buffer, uint8_t *bufferLen);



        /*****************************************************************************/
        /**
         * @brief Write a series of bytes to an I2c device using SMBus protocol.
         *
         *  Uses the SMbus Block Data protocol to write up to 32 bytes to the
         *  previously opened I2c device.
         *
         *  @param[in]		command				SMBus command value to send.
         *  @param[in]		buffer				Buffer containing data to be sent.
         *  @param[in]		bufferLen			Specifies the length of buffer,
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Write successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_PARAMETER_ERROR		Transfer length too large
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalSmbusWriteBlock(uint8_t command, uint8_t const *blockData,
                                       uint8_t length);



        /*****************************************************************************/
        /**
         * @brief Write a series of bytes to an I2c device.
         *
         *  This function can write up to 255 bytes to the specified device since it
         *  does not use the SMBus protocol to perform the transfer.  Also the specified
         *  i2cAddr does not have to reference an address within the MHL transmitter,
         *  but the I2c device does need to reside on the same I2c bus as the MHL
         *  transmitter.
         *
         *  @param[in]		i2cAddr				I2c address to write to.
         *  @param[in]		length				Specifies the length of buffer.
         *  @param[in]		buffer				Buffer containing data to be sent.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Write successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalI2cMasterWrite(uint8_t i2cAddr, uint8_t length,
                                      uint8_t *buffer);



        /*****************************************************************************/
        /**
         * @brief Read a series of bytes from an I2c device.
         *
         *  This function can write up to 255 bytes to the specified device since it
         *  does not use the SMBus protocol to perform the transfer.  Also the specified
         *  i2cAddr does not have to reference an address within the MHL transmitter,
         *  but the I2c device does need to reside on the same I2c bus as the MHL
         *  transmitter.
         *
         *  @param[in]		i2cAddr				I2c address to write to.
         *  @param[in]		length				Specifies the length of buffer.
         *  @param[in]		buffer				Buffer where data read will be returned.
         *
         *  @return         status (success or error code)
         *  @retval	HAL_RET_SUCCESS				Read successful
         *  @retval HAL_RET_NOT_INITIALIZED		HAL not initialized (need to call HalInit)
         *  @retval	HAL_RET_DEVICE_NOT_OPEN		MHL transmitter device not currently open
         *  @retval HAL_RET_PARAMETER_ERROR		Transfer length too large
         *  @retval HAL_RET_FAILURE				The SMBus transaction failed.
         *
         *****************************************************************************/
        halReturn_t HalI2cMasterRead(uint8_t i2cAddr, uint8_t length,
                                     uint8_t *buffer);



        /*****************************************************************************/
        /**
         * @brief Read a single byte from a register within an I2c device.
         *
         * This function performs an I2c SMB read transfer of a single byte.  The
         * function assumes that the device address specified is one of the addresses
         * used by the MHL transmitter.  Trying to use this function to read from any
         * other I2c device will not work.  Additionally, HalOpenI2cDevice must have
         * been called before calling this function.
         *
         *  @param[in]		deviceID		I2c device address.
         *  @param[in]		offset			Offset of the I2c register to be read.
         *
         *  @return         Byte value read or 0xFF in case of error
         *
         *****************************************************************************/
        uint8_t I2C_ReadByte(uint8_t deviceID, uint8_t offset);



        /*****************************************************************************/
        /**
         * @brief Write a single byte to a register within an I2c device.
         *
         * This function performs an I2c SMB write transfer of a single byte.  The
         * function assumes that the device address specified is one of the addresses
         * used by the MHL transmitter.  Trying to use this function to write to any
         * other I2c device will not work.  Additionally, HalOpenI2cDevice must have
         * been called before calling this function.
         *
         *  @param[in]		deviceID		I2c device address.
         *  @param[in]		offset			Offset of the I2c register to be written.
         *  @param[in]		value			Value to write to register.
         *
         *  @return         Nothing.
         *
         *****************************************************************************/
        void I2C_WriteByte(uint8_t deviceID, uint8_t offset, uint8_t value);
#endif // #if 0



        /*****************************************************************************/
        /**
         * @brief Install IRQ handler.
         *
         *  This function enables the handling of interrupts from the MHL transmitter.
         *  The transmitter device must have been successfully opened before attempting
         *  to enable interrupts.
         *
         *  @param[in]		irqHandler	Firmware function to call when an interrupt
         *  							is received.
         *
         *  @return         status (success or error code)
         *  @retval			HAL_RET_SUCCESS - Interrupt handler successfully installed.
         *  @retval			HAL_RET_PARAMETER_ERROR - irqHandler parameter is NULL.
         *  @retval			HAL_RET_DEVICE_NOT_OPEN - MHL transmitter device not opened.
         *  @retval			HAL_RET_FAILURE - Platform didn't assign an interrupt number
         *  				or there was an error while attempting to install the handler.
         *
         *****************************************************************************/
        halReturn_t HalInstallIrqHandler(fwIrqHandler_t irqHandler);



        /*****************************************************************************/
        /**
         * @brief Remove IRQ handler.
         *
         *  This function un-installs the MHL transmitter device interrupt handler
         *  installed by HalInstallIrqHandler.
         *
         *  @return         status (success or error code)
         *  @retval			HAL_RET_SUCCESS - Interrupt handler successfully removed.
         *  @retval			HAL_RET_DEVICE_NOT_OPEN - MHL transmitter device not opened.
         *  @retval			HAL_RET_FAILURE - Interrupt handler was never installed.
         *
         *****************************************************************************/
        halReturn_t HalRemoveIrqHandler(void);



        /*****************************************************************************/
        /**
         * @brief Set a timer counter.
         *
         * This function can be called to set either one of the count down timers or
         * one of the elapsed timers.  If index specifies one of the elapsed timers
         * the timer's count is initialized to zero and the m_sec parameter is used
         * to set the granularity at which it counts up.  If index specifies one of
         * the count down timers the m_sec parameter is used to set the initial count
         * of the timer.
         *
         *  @param[in]	index	Indicates which timer counter to set.
         *  @param[in]	m_sec	Timer counter value to set in milliseconds.
         *
         *  @return 	Nothing.
         *
         *****************************************************************************/
        void HalTimerSet ( uint8_t index, uint16_t m_sec );



        /*****************************************************************************/
        /**
         * @brief Wait for the specified number of milliseconds to elapse.
         *
         *  @param[in]	m_sec	The number of milliseconds to delay.
         *
         * @note: This function may put the calling process to sleep and therefore
         * must not be called from an interrupt context.
         *
         *****************************************************************************/
        void HalTimerWait(uint16_t m_sec);

        /*****************************************************************************/
        /**
         * @brief Check if the specified timer has expired.
         *
         * This function checks to see if the specified timer has expired yet.  The
         * timerIndex should specify one of the count down timers and not one of the
         * elapsed timers.
         *
         *  @param[in]	timerIndex	Indicates which timer counter to check.
         *
         *  @return 	Non-zero if the timer has expired.
         *
         *****************************************************************************/
        uint8_t HalTimerExpired (uint8_t timerIndex);



        /*****************************************************************************/
        /**
         * @brief Read the current value of one of the elapsed timer counters.
         *
         * The elapsed timers unlike the other timers count up at a msec. granularity
         * specified when they are started.  This function returns the current value
         * of the specified elapsed timer.  The timer count returned needs to be
         * multiplied by the granularity specified when the timer was set in order
         * to calculate the elapsed time.
         *
         *  @param[in]	elapsedTimerIndex	Indicates which elapsed timer counter
         *  								to check.
         *
         *  @return 	The current value of the timer, or zero if an invalid timer
         *  			index is specified.
         *
         *****************************************************************************/
        uint16_t HalTimerElapsed(uint8_t elapsedTimerIndex);

        /*
        HalGpioGetTxIntPin
        	gets the status of the interrupt pin.
           */
        int HalGpioGetTxIntPin(void);


        /*****************************************************************************/
        /**
         * @brief Platform specific function to control the reset pin of the MHL
         * 		  transmitter device.
         *
         * The driver will call this function to reset the MHL transmitter device.
         * In this sample driver a processor GPIO pin is used to control the state of
         * the MHL tranmitter's Reset pin.
         *
         *  @param[in]	value	If true, the Reset pin should be set HIGH,
         *  					otherwise the pin should be set LOW.
         *
         * @return        status (success or error code)
         * @retval	HAL_RET_SUCCESS			Pin set to requested value
         * @retval	HAL_RET_NOT_INITIALIZED	HAL not initialized (HalInit not called)
         *
         *****************************************************************************/
        halReturn_t HalGpioSetTxResetPin(bool value);



        /*****************************************************************************/
        /**
         * @brief Platform specific function to control power on the USB port.
         *
         * This function will be called when the driver determines that power should
         * either be applied or removed from the USB port.  If the driver determines
         * that the MHL port is connected to an MHL sink device, this function will
         * be called to remove power from the USB port since USB device(s) on the
         * platform can't be used while an MHL sink device is attached.
         * In this sample driver a processor GPIO pin is used to control the state of
         * USB port power.
         *
         *  @param[in]	value	If true the function is being requested to remove
         *  					power from the USB port.  If false, power should
         *  					be applied to the USB port.
         *
         * @return        status (success or error code)
         * @retval	HAL_RET_SUCCESS			Pin set to requested value
         * @retval	HAL_RET_NOT_INITIALIZED	HAL not initialized (HalInit not called)
         *
         *****************************************************************************/
        halReturn_t HalGpioSetUsbVbusPowerPin(bool value);



        /*****************************************************************************/
        /**
         * @brief Platform specific function to control power on the MHL port.
         *
         * This function will be called when the driver determines that power should
         * either be applied or removed from the MHL port.  If the driver determines
         * that the MHL port is connected to an MHL sink device and that the sink
         * device does not require power from the MHL link, this function will
         * be called to remove power from the MHL port.
         * In this sample driver a processor GPIO pin is used to control the state of
         * MHL port power.
         *
         *  @param[in]	powerOn		If true, Vbus power is already available and this
         *  						function should turn off Vbus power from being
         *  						supplied from the platform.  If false, this function
         *  						should cause the platform to begin supplying power
         *  						to the MHL port.
         *
         * @return        status (success or error code)
         * @retval	HAL_RET_SUCCESS			Pin set to requested value
         * @retval	HAL_RET_NOT_INITIALIZED	HAL not initialized (HalInit not called)
         *
         *****************************************************************************/
        halReturn_t HalGpioSetVbusPowerPin(bool powerOn);



        /*****************************************************************************/
        /**
         * @brief Acquire the lock that prevents races with the interrupt handler.
         *
         * This lock is acquired by the interrupt handler before calling into the driver
         * layer.  All calls into the MHL Tx or driver layer code also must first
         * acquire this lock to prevent race conditions with other threads of execution
         * that need to execute within these driver layers.
         *
         * @return        status (success or error code)
         * @retval	HAL_RET_SUCCESS			Pin set to requested value
         * @retval	HAL_RET_NOT_INITIALIZED	HAL not initialized (HalInit not called)
         * @retval	HAL_RET_FAILURE			Lock acquisition failed
         *
         *****************************************************************************/
        halReturn_t HalAcquireIsrLock(void);



        /*****************************************************************************/
        /**
         * @brief Release the lock that prevents races with the interrupt handler.
         *
         * This function releases the lock acquired by HalAcquireIsrLock().
         *
         * @return        status (success or error code)
         * @retval	HAL_RET_SUCCESS			Pin set to requested value
         * @retval	HAL_RET_NOT_INITIALIZED	HAL not initialized (HalInit not called)
         *
         *****************************************************************************/
        halReturn_t HalReleaseIsrLock(void);


#ifdef SHARE_SWITCHER_WITH_USB
        /*****************************************************************************/
        /**
        * @brief Usb and MHL DP DN switcher operation.
        * @return        status (success or error code)
        * @isSwitchToMhl true means switch To MHL
        *
        *****************************************************************************/
        halReturn_t HalSwitcherSwitchToMhl(bool isSwitchToMhl);
#endif

        /**
         * @}
         * end sii_hal_api group
         */

#ifdef __cplusplus
}
#endif  /* _defined (__cplusplus) */

#endif /* _defined (SII_HAL_H) */
