/*! @file : sdk_hal_i2c0.h
 * @author  Luis José Castrillo Fernández
 * @version 1.0.0
 * @date    30 ene. 2021
 * @brief   Driver para 
 * @details
 *
 */
#ifndef SDK_HAL_I2C0_H_
#define SDK_HAL_I2C0_H_
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "fsl_common.h"


/*!
 * @addtogroup HAL
 * @{
 */
/*!
 * @addtogroup I2C
 * @{
 */
/*******************************************************************************
 * Public Definitions
 ******************************************************************************/
enum _i2c0_register_address_map {
	STATUS_F_STATUS = 0x00,	//FMODE = 0, real time status. FMODE > 0, FIFO status
	OUT_X_MSB,				//[7:0] are eight MSBs of 14-bit sample
	OUT_X_LSB,				//[7:2] are six LSBs of 14-bit sample
	OUT_Y_MSB,				//[7:0] are eight MSBs of 14-bit sample
	OUT_Y_LSB,				//[7:2] are six LSBs of 14-bit sample
	OUT_Z_MSB,				//[7:0] are eight MSBs of 14-bit sample
	OUT_Z_LSB,				//[7:2] are six LSBs of 14-bit sample


	F_SETUP = 0x09,			//FIFO setup
	TRIG_CFG,				//Map of FIFO data capture events
	SYSMOD,					//Current system mode
	INT_SOURCE,				//Interrupt status
	WHO_AM_I,				//Device ID (0x1A)
	XYZ_DATA_CFG,			//Dynamic range settings
	HP_FILTER_CUTOFF,		//Cutoff frequency is set to 16 Hz @800 Hz
	PL_STATUS,				//Landscape/portrait orientation status
	PL_CFG,					//Landscape/portrait configuration.
	PL_COUNT,				//Landscape/portrait debounce counter
	PL_BF_ZCOMP,			//Back/front, Z-lock trip threshold
	P_L_THS_REG,			//Portrait to landscape trip angle is 29°
	FF_MT_CFG,				//Freefall/motion functional block configuration
	FF_MT_SRC,				//Freefall/motion event source register
	FF_MT_THS,				//Freefall/motion threshold register
	FF_MT_COUNT, 			//Freefall/motion debounce counter



	TRANSIENT_CFG = 0x1D, 	//Transient functional block configuration
	TRANSIENT_SCR,			//Transient event status register
	TRANSIENT_THS,			//Transient event threshold
	TRANSIENT_COUNT, 		//Transient debounce counter
	PULSE_CFG,		 		//ELE, double_XYZ or single_XYZ
	PULSE_SRC,		 		//EA, double_XYZ or single_XYZ
	PULSE_THSX,		 		//X pulse threshold
	PULSE_THSY,		 		//Y pulse threshold
	PULSE_THSZ,		 		//Z pulse threshold
	PULSE_TMLT,		 		//Time limit for pulse
	PULSE_LTCY,		 		//Latency time for 2nd pulse
	PULSE_WIND,		 		//Window time for 2nd pulse
	ASLP_COUNT,		 		//Counter setting for auto-sleep
	CTRL_REG1,		 		//ODR = 800 Hz, standby mode.
	CTRL_REG2,		 		//Sleep enable, OS modes, RST, ST
	CTRL_REG3,		 		//Wake from sleep, IPOL, PP_OD
	CTRL_REG4,		 		//Interrupt enable register
	CTRL_REG5,		 		//Interrupt pin (INT1/INT2) map
	OFF_X,		 			//X-axis offset adjust
	OFF_Y,		 			//Y-axis offset adjust
	OFF_Z,		 			//Z-axis offset adjust




};

#define MMA851_I2C_DEVICE_ADDRESS	0x1D

/*******************************************************************************
 * External vars
 ******************************************************************************/

/*******************************************************************************
 * Public vars
 ******************************************************************************/

/*******************************************************************************
 * Public Prototypes
 ******************************************************************************/
/*--------------------------------------------*/
/*!
 * @brief Inicializa I2C0 al baudrate especificado
 *
 * @param baud_rate   baudrate (bps) que se quiere configurado en I2C0
 * @return            resultado de la ejecución
 * @code
 * 		kStatus_Success
 * 		kStatus_Fail
 * @endcode
 */
status_t i2c0MasterInit(uint32_t baud_rate);

/*!
 * @brief Lee 1 byte usando puerto I2C0
 *
 * @param data   			apuntador a memoria donde almacenar dato obtenido
 * @param device_address	direccion en bus I2C de dispositivo remoto a leer
 * @param memory_address	posicion de memoria en el dispositivo remoto que se quiere leer
 * @return            		resultado de la ejecución
 * @code
 * 		kStatus_Success
 * 		kStatus_Fail
 * @endcode
 */
status_t i2c0MasterReadByte(uint8_t *data, uint8_t device_address, int8_t memory_address);

status_t i2c0MasterWriteByte(uint8_t device_address, int8_t memory_address, uint8_t data);




/** @} */ // end of I2C0 group
/** @} */ // end of HAL group

#endif /* SDK_HAL_I2C0_H_ */
