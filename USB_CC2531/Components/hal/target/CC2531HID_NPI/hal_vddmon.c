/*
  Filename:       _hal_vddmon.c
*/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_adc.h"
#include "hal_board_cfg.h"
#include "hal_defs.h"
#include "hal_mcu.h"
#include "hal_sleep.h"
#include "hal_vddmon.h"

/**************************************************************************************************
 * @fn          HalVddMonInit
 *
 * @brief       This function is the subsystem initialization function and should be called as
 *              early as possible during initialization - after GPIO's are safe with everything
 *              turned off and before turning anything on.
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
void HalVddMonInit(void)
{
#if (defined HAL_VDDMON) && (HAL_VDDMON == TRUE)
  if (!HalAdcCheckVdd(VDD_MIN_INIT))
  {
    extern void halSleepExec(void);
    // S/W brown-out detect - just deep sleep forever.
    HAL_DISABLE_INTERRUPTS();
    SLEEPCMD |= PMODE;
    ALLOW_SLEEP_MODE();
    halSleepExec();
  }
#endif
}

/**************************************************************************************************
 * @fn          HalVddMonPoll
 *
 * @brief       This function is the subsystem polling function and should be called as
 *              frequently as possible during task servicing.
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
void HalVddMonPoll(void)
{
#if (defined HAL_VDDMON) && (HAL_VDDMON == TRUE)
  if (!HalAdcCheckVdd(VDD_MIN_POLL))
  {
    /* Just reset and allow HalVddMonInit() to put the chip into deep sleep when everything is in a
     * safe and off state.
     */
    HAL_SYSTEM_RESET();
  }
#endif
}

/**************************************************************************************************
*/
