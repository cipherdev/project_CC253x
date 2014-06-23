/**************************************************************************************************
  Filename:       hal_gpiodbg.c
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
