/**************************************************************************************************
  Filename:       hal_gpiodbg.c
  Revised:        $Date: 2011-09-23 10:34:21 -0700 (Fri, 23 Sep 2011) $
  Revision:       $Revision: 27694 $

  Description:    This file contains the interface to the HAL GPIO Debug module.


  Copyright 2006-2011 Texas Instruments Incorporated. All rights reserved.

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

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "osal.h"

/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

/* The following define which port pins are spare and can be used for debug */
#define HAL_GPIODBG_P1_GPIO_PINS  ( BV( 7 ) | BV( 6 ) | BV( 5 ) )
#define HAL_GPIODBG_P2_GPIO_PINS  ( BV( 4 ) | BV( 3 ) | BV( 0 ) )

/* P0SEL and P1SEL select functionality by directly mapping pin number to
 * the corresponding bit in the register. P2SEL only allows mapping of
 * pins 0, 3, and 4, and the pin functionality of each of those pins is
 * mapped to bits 0-2 of P2SEL.
 */
#define HAL_GPIODBG_P2SEL_BITS    ( BV( 2 ) | BV( 1 ) | BV( 0 ) )

/* These defines indicate the direction of each pin */
#define HAL_GPIODBG_P1_OUTPUT_PINS  HAL_GPIODBG_P1_GPIO_PINS
#define HAL_GPIODBG_P2_OUTPUT_PINS  HAL_GPIODBG_P2_GPIO_PINS

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
/**************************************************************************************************
 * @fn      HalGpioDbgInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalGpioDbgInit( void )
{
  /* The spare GPIO pins will be initialized to be outputs driven low to save power. */
  P1 &= ~HAL_GPIODBG_P1_GPIO_PINS;
  P2 &= ~HAL_GPIODBG_P2_GPIO_PINS;

  /* Configure spare pin function as GPIO */
  P1SEL &= (uint8) ~HAL_GPIODBG_P1_GPIO_PINS;
  P2SEL &= (uint8) ~HAL_GPIODBG_P2SEL_BITS;

  /* Configure spare pin direction to output */
  P1DIR |= (uint8) HAL_GPIODBG_P1_OUTPUT_PINS;
  P2DIR |= (uint8) HAL_GPIODBG_P2_OUTPUT_PINS;
}

/**************************************************************************************************
**************************************************************************************************/
