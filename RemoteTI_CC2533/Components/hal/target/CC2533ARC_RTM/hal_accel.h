/**************************************************************************************************
  Filename:       hal_accel.h
**************************************************************************************************/
#ifndef HAL_ACCEL_H
#define HAL_ACCEL_H

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
#define HAL_ACCEL_I2C_ADDRESS 0x0F
#define HAL_ACCEL_OUTPUT_DATA_ADDRESS 0x06 // corresponds to X_OUT register address

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */
typedef void (*halAccelCBack_t) ( void );

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          HalAccelInit
 *
 * @brief       This function initializes the HAL Accelerometer abstraction layer.
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
void HalAccelInit( void );

/**************************************************************************************************
 * @fn          HalAccelEnable
 *
 * @brief       This function configures the accelerometer for operation.
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
void HalAccelEnable( void );

/**************************************************************************************************
 * @fn          HalAccelDisable
 *
 * @brief       Places the Accelerometer in low power mode.
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
void HalAccelDisable( void );

/**************************************************************************************************
 * @fn          HalAccelMotionDetect
 *
 * @brief       Places the Accelerometer in motion detection mode.
 *
 * input parameters
 *
 * @param       cback - callback function to invoke if motion is detected
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void HalAccelMotionDetect( halAccelCBack_t cback );

/**************************************************************************************************
 * @fn          HalAccelRead
 *
 * @brief       Reads Y and Z data out registers from Accelerometer.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * @param       *y - Y register value.
 * @param       *z - Z register value.
 *
 * @return      None.
 */
void HalAccelRead( int8 *y, int8 *z );

/**************************************************************************************************
 * @fn          HalAccelProcessMotionDetectEvent
 *
 * @brief       Post interrupt processing when motion detect interrupt has been received.
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
void halAccelProcessMotionDetectEvent( void );

/**************************************************************************************************
*/

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
