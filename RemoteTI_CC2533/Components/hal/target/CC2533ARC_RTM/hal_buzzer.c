/**************************************************************************************************
  Filename:       hal_buzzer.c
**************************************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_board.h"
#include "hal_buzzer.h"
#include "hal_drivers.h"
#include "osal.h"

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

/* Defines for Timer 4 */
#define HAL_T4_CC0_VALUE 125 /* provides pulse width of 125 usec */
#define HAL_T4_TIMER_CTL_DIV32    0xA0  /* Clock pre-scaled by 32 */
#define HAL_T4_TIMER_CTL_START    0x10
#define HAL_T4_TIMER_CTL_CLEAR    0x04
#define HAL_T4_TIMER_CTL_OPMODE_MODULO 0x02  /* Modulo Mode, Count from 0 to CompareValue */
#define HAL_T4_TIMER_CCTL_MODE_COMPARE 0x04
#define HAL_T4_TIMER_CCTL_CMP_TOGGLE 0x10

/* The following define which port pins are being used by the buzzer */
#define HAL_BUZZER_P1_GPIO_PINS  ( BV( 0 ) )

/* These defines indicate the direction of each pin */
#define HAL_BUZZER_P1_OUTPUT_PINS  ( BV( 0 ) )

/* Defines for each output pin assignment */
#define HAL_BUZZER_ENABLE_PIN  P1_0

/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/

#define HAL_BUZZER_DISABLE() ( HAL_BUZZER_ENABLE_PIN = 0 )

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
/* Function to call when ringing of buzzer is complete */
static halBuzzerCBack_t pHalBuzzerRingCompleteNotificationFunction;

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/**************************************************************************************************
 * @fn      HalBuzzerInit
 *
 * @brief   Initilize buzzer hardware
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalBuzzerInit( void )
{
  pHalBuzzerRingCompleteNotificationFunction = NULL;

  /* Initialize outputs */
  HAL_BUZZER_DISABLE();

  /* Configure direction of pins related to buzzer */
  P1DIR |= (uint8) HAL_BUZZER_P1_OUTPUT_PINS;
}

/**************************************************************************************************
 * @fn          HalBuzzerRing
 *
 * @brief       This function rings the buzzer once.
 *
 * input parameters
 *
 * msec - Number of msec to ring the buzzer
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void HalBuzzerRing( uint16 msec,
                    halBuzzerCBack_t buzzerCback )
{
  /* Register the callback fucntion */
  pHalBuzzerRingCompleteNotificationFunction = buzzerCback;

  /* Configure output pin as peripheral since we're using T4 to generate */
  P1SEL |= (uint8) HAL_BUZZER_P1_GPIO_PINS;

  /* Buzzer is "rung" by using T4, channel 0 to generate 4kHz square wave */
  T4CTL = HAL_T4_TIMER_CTL_DIV32 |
          HAL_T4_TIMER_CTL_CLEAR |
          HAL_T4_TIMER_CTL_OPMODE_MODULO;
  T4CCTL0 = HAL_T4_TIMER_CCTL_MODE_COMPARE | HAL_T4_TIMER_CCTL_CMP_TOGGLE;
  T4CC0 = HAL_T4_CC0_VALUE;

  /* Start it */
  T4CTL |= HAL_T4_TIMER_CTL_START;

  /* Setup timer that will end the buzzing */
  osal_start_timerEx( Hal_TaskID,
                      HAL_BUZZER_EVENT,
                      msec );

}

/**************************************************************************************************
 * @fn      HalBuzzerStop
 *
 * @brief   Halts buzzer
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalBuzzerStop( void )
{
  /* Setting T4CTL to 0 disables it and masks the overflow interrupt */
  T4CTL = 0;

  /* Return output pin to GPIO */
  P1SEL &= (uint8) ~HAL_BUZZER_P1_GPIO_PINS;

  /* Inform application that buzzer is done */
  if (pHalBuzzerRingCompleteNotificationFunction != NULL)
  {
    pHalBuzzerRingCompleteNotificationFunction();
  }
}

/**************************************************************************************************
**************************************************************************************************/
