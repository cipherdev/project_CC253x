/*
  Filename:       hal_sleep.h
*/

#ifndef HAL_SLEEP_H
#define HAL_SLEEP_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Execute power management procedure
 */
extern void halSleep( uint16 osal_timer );

/*
 * Used in mac_mcu
 */
extern void halSleepWait(uint16 duration);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
