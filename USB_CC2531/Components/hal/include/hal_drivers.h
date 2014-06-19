/*
  Filename:       hal_drivers.h

*/
#ifndef HAL_DRIVER_H
#define HAL_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

#define HAL_KEY_EVENT                            0x0001
#define HAL_LED_BLINK_EVENT                      0x0002

#define HAL_MOTION_GYRO_POWERUP_DONE_EVENT       0x0004
#define HAL_MOTION_GYRO_CALIBRATION_DONE_EVENT   0x0008
#define HAL_MOTION_MEASUREMENT_START_EVENT       0x0010
#define HAL_MOTION_MEASUREMENT_DONE_EVENT        0x0020
#define HAL_BUZZER_EVENT                         0x0040
#define HAL_GYRO_REGISTER_ACCESS_READY_EVENT     0x0080
#define HAL_MOTION_DETECTED_EVENT                0x0100
#define HAL_GYRO_ACTIVE_EVENT                    0x0200
#define HAL_MOTION_EVENT \
  ( HAL_MOTION_GYRO_POWERUP_DONE_EVENT | \
    HAL_MOTION_GYRO_CALIBRATION_DONE_EVENT | \
    HAL_MOTION_MEASUREMENT_START_EVENT | \
    HAL_MOTION_MEASUREMENT_DONE_EVENT | \
    HAL_MOTION_DETECTED_EVENT | \
    HAL_GYRO_ACTIVE_EVENT )

/**************************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************************/
extern uint8 Hal_TaskID;

extern void Hal_Init ( uint8 task_id );

/*
 * Process Serial Buffer
 */
extern uint16 Hal_ProcessEvent ( uint8 task_id, uint16 events );

/*
 * Process Polls
 */
extern void Hal_ProcessPoll (void);

/*
 * Initialize HW
 */
extern void HalDriverInit (void);

/**************************************************************************************************
 * @fn          halDriverBegPM
 *
 * @brief       This function is called before entering PM so that drivers can be put into their
 *              lowest power states.
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
 **************************************************************************************************
 */
void halDriverBegPM(void);

/**************************************************************************************************
 * @fn          halDriverEndPM
 *
 * @brief       This function is called after exiting PM so that drivers can be restored to their
 *              ready power states.
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
 **************************************************************************************************
 */
void halDriverEndPM(void);

#ifdef __cplusplus
}
#endif

#endif
/**************************************************************************************************
*/
