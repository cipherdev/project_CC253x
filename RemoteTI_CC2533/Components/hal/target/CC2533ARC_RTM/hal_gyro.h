/**************************************************************************************************
  Filename:       hal_gyro.h
**************************************************************************************************/
#ifndef HAL_GYRO_H
#define HAL_GYRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_GYRO_OUTPUT_SETTLING_TIME 200 // time from power on until data valid, in ms
#define HAL_GYRO_CAL_SETTLING_TIME 10 // time from AZ asserted until outputs settled, in ms
#define HAL_GYRO_I2C_ADDRESS 0x68

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */
typedef void (*halGyroEnableCback_t) ( void );

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          HalGyroInit
 *
 * @brief       This function initializes the HAL Gyroscope abstraction layer.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************/
void HalGyroInit( void );

/**************************************************************************************************
 * @fn          HalGyroEnable
 *
 * @brief       This function configures the gyro for operation.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************/
void HalGyroEnable( halGyroEnableCback_t cback );

/**************************************************************************************************
 * @fn          HalGyroHandleGyroRegisterAccessReadyEvent
 *
 * @brief       Callback to handle expiry of gyro powerup wait timer.
 *
 * @param       None.
 *
 * @return      None.
 **************************************************************************************************/
void HalGyroHandleGyroRegisterAccessReadyEvent( void );

/**************************************************************************************************
 * @fn          HalGyroDisable
 *
 * @brief       This function powers off the gyro.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************/
void HalGyroDisable( void );

/**************************************************************************************************
 * @fn      HalGyroRead
 *
 * @brief   Performs A/D reading on channels connected to X, Y, and Z output on Gyroscope.
 *
 * @param   None
 *
 * @return  X, Y, and Z channel readings
 **************************************************************************************************/
void HalGyroRead( int16 *x, int16 *y, int16 *z );

/**************************************************************************************************
 * @fn          HalGyroReadAccelData
 *
 * @brief       Reads X, Y and Z data out registers from Accelerometer.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * @param       *x - X register value.
 * @param       *y - Y register value.
 * @param       *z - Z register value.
 *
 * @return      None.
 */
void HalGyroReadAccelData( int8 *x, int8 *y, int8 *z );

/**************************************************************************************************
 * @fn      HalGyroEnableI2CPassThru
 *
 * @brief   Enables I2C pass thru mode on the IMU-3000, which allows the MCU to talk to the
 *          accelerometer.
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalGyroEnableI2CPassThru( void );

/**************************************************************************************************
 * @fn      HalGyroDisableI2CPassThru
 *
 * @brief   Disables I2C pass thru mode on the IMU-3000, which means the gyro will master the I2C
 *          accesses to the accelerometer.
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalGyroDisableI2CPassThru( void );

/**************************************************************************************************
 * @fn      HalGyroStartGyroMeasurements
 *
 * @brief   Enables the gyro clock and also configures the gyro to control the aux I2C
 *          interface so it can read accelerometer data.
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalGyroStartGyroMeasurements( void );

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
