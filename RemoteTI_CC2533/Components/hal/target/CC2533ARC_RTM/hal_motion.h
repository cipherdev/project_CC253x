/**************************************************************************************************
  Filename:       hal_motion.h
**************************************************************************************************/
#ifndef HAL_MOTION_H
#define HAL_MOTION_H

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

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

typedef void (*halMotionCBack_t) (int16 gyroMickeysX, int16 gyroMickeysY);
typedef void (*halMotionCalCBack_t) (void);

enum
{
  HAL_MOTION_RESOLUTION_DECREASE,
  HAL_MOTION_RESOLUTION_INCREASE
};
typedef uint8 halMotionMouseResolution_t;

enum
{
  HAL_MOTION_DEVICE_GYRO,
  HAL_MOTION_DEVICE_ACCELEROMETER
};
typedef uint8 halMotionDevice_t;

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          HalMotionInit
 *
 * @brief       This function initializes the HAL Motion Sensor abstraction layer.
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
 */
void HalMotionInit(void);

/**************************************************************************************************
 * @fn          HalMotionConfig
 *
 * @brief       This function configures the HAL Motion Sensor abstraction layer.
 *
 * input parameters
 *
 * @param measCback - Pointer to the application layer callback function for processing motion data
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void HalMotionConfig(halMotionCBack_t measCback);

/**************************************************************************************************
 * @fn          HalMotionEnable
 *
 * @brief       This function powers on the motion sensor hardware and starts the associated H/W timer.
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
 */
void HalMotionEnable(void);

/**************************************************************************************************
 * @fn      HalMotionCal
 *
 * @brief   Calibrate Motion Sensor hardware
 *
 * @param calCback - Pointer to the application layer callback function for notification of calibration complete
 * @param calDuration - duration, in milliseconds, of calibration procedure
 *
 * @return  None
 **************************************************************************************************/
void HalMotionCal( halMotionCalCBack_t calCback,
                   uint16 calDuration );

/**************************************************************************************************
 * @fn          HalMotionDisable
 *
 * @brief       This function powers off the motion sensor hardware and stops the associated H/W timer.
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
 */
void HalMotionDisable(void);

/**************************************************************************************************
 * @fn      HalMotionStandby
 *
 * @brief   Places motion detection hardware in "standby" mode. This means the
 *          hardware is in a lower power state and will only be re-enabled if
 *          motion is detected.
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalMotionStandby( void );

/**************************************************************************************************
 * @fn      HalMotionModifyAirMouseResolution
 *
 * @brief   Increase/decrease air mouse movement resolution
 *
 * @param   action - increase/decrease command
 *
 * @return  None
 **************************************************************************************************/
void HalMotionModifyAirMouseResolution( halMotionMouseResolution_t action );

/**************************************************************************************************
 * @fn      HalMotionHandleOSEvent
 *
 * @brief   Handles Motion Sensor OS Events
 *
 * @param   OS Events that triggered call
 *
 * @return  Event that was handled
 **************************************************************************************************/
uint16 HalMotionHandleOSEvent( uint16 events );

/**************************************************************************************************
 * @fn      HalMotionI2cRead
 *
 * @brief   Reads registers from devices connected to I2C bus
 *
 * @param   device - which device is being written
 *          addr - starting register address to read
 *          numBytes - Number of bytes to read
 *          pBuf - pointer to buffer to place read data
 *
 * @return  None
 **************************************************************************************************/
void HalMotionI2cRead( halMotionDevice_t device, uint8 addr, uint8 numBytes, uint8 *pBuf );

/**************************************************************************************************
 * @fn      HalMotionI2cWrite
 *
 * @brief   Writes registers on devices connected to I2C bus
 *
 * @param   device - which device is being written
 *          addr - starting register address to write
 *          pData - pointer to buffer containing data to be written
 *          numBytes - Number of bytes to read
 *
 * @return  None
 **************************************************************************************************/
void HalMotionI2cWrite( halMotionDevice_t device, uint8 addr, uint8 *pData, uint8 numBytes );

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
