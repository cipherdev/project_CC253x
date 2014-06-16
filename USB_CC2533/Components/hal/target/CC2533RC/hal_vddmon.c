/**************************************************************************************************
  Filename:       _hal_vddmon.c
  Revised:        $Date: 2011-09-27 17:21:52 -0700 (Tue, 27 Sep 2011) $
  Revision:       $Revision: 27740 $

  Description:    This file is the implementation for the HAL Vdd monitoring subsystem.


  Copyright 2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
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
  if (!HalBatMonRead(HAL_BATMON_MIN_INIT))
  {
    // S/W brown-out detect - just deep sleep forever.
    extern void halSleepExec(void);
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
  if (!HalBatMonRead(HAL_BATMON_MIN_POLL))
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
