/**************************************************************************************************
  Filename:       _hal_batmon.c
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_board_cfg.h"
#include "hal_batmon.h"
#include "hal_defs.h"
#include "hal_mcu.h"
#include "hal_sleep.h"

/**************************************************************************************************
 * @fn          HalBatMonRead
 *
 * @brief       This function is the subsystem utility read function and should be called before
 *              any Vdd-critical operation (e.g. flash write or erase operations.)
 *
 * input parameters
 *
 * @param       vddMask - A valid BATTMON_VOLTAGE mask.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the measured Vdd exceeds the paramter level; FALSE otherwise.
 **************************************************************************************************
 */
uint8 HalBatMonRead(uint8 vddMask)
{
  uint8 rtrn = TRUE;

#if (defined HAL_BATMON) && (HAL_BATMON == TRUE)
  MONMUX = 0;           // Setup BATTMON mux to measure AVDD5.
  BATMON = vddMask;
  halSleepWait(2);      // Wait at least 2 us before reading BATTMON_OUT.
  rtrn = (BATMON & BATTMON_OUT) ? TRUE : FALSE;
  BATMON = BATTMON_PD;  // Turn off for power saving.
#endif

  return rtrn;
}

/**************************************************************************************************
*/
