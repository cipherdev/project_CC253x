/**************************************************************************************************
  Filename:       hal_key.c
  Revised:        $Date: 2012-01-13 09:56:19 -0800 (Fri, 13 Jan 2012) $
  Revision:       $Revision: 28930 $

  Description:    This file contains the interface to the HAL KEY Service.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

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
/*********************************************************************
 NOTE: If polling is used, the hal_driver task schedules the KeyRead()
       to occur every 100ms.  This should be long enough to naturally
       debounce the keys.  The KeyRead() function remembers the key
       state of the previous poll and will only return a non-zero
       value if the key state changes.

 NOTE: If interrupts are used, the KeyRead() function is scheduled
       25ms after the interrupt occurs by the ISR.  This delay is used
       for key debouncing.  The ISR disables any further Key interrupt
       until KeyRead() is executed.  KeyRead() will re-enable Key
       interrupts after executing.  Unlike polling, when interrupts
       are enabled, the previous key state is not remembered.  This
       means that KeyRead() will return the current state of the keys
       (not a change in state of the keys).

 NOTE: If interrupts are used, the KeyRead() fucntion is scheduled by
       the ISR.  Therefore, the joystick movements will only be detected
       during a pushbutton interrupt caused by S1 or the center joystick
       pushbutton.

 NOTE: When a switch like S1 is pushed, the S1 signal goes from a normally
       high state to a low state.  This transition is typically clean.  The
       duration of the low state is around 200ms.  When the signal returns
       to the high state, there is a high likelihood of signal bounce, which
       causes a unwanted interrupts.  Normally, we would set the interrupt
       edge to falling edge to generate an interrupt when S1 is pushed, but
       because of the signal bounce, it is better to set the edge to rising
       edge to generate an interrupt when S1 is released.  The debounce logic
       can then filter out the signal bounce.  The result is that we typically
       get only 1 interrupt per button push.  This mechanism is not totally
       foolproof because occasionally, signal bound occurs during the falling
       edge as well.  A similar mechanism is used to handle the joystick
       pushbutton on the DB.  For the EB, we do not have independent control
       of the interrupt edge for the S1 and center joystick pushbutton.  As
       a result, only one or the other pushbuttons work reasonably well with
       interrupts.  The default is the make the S1 switch on the EB work more
       reliably.

*********************************************************************/

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

#define HAL_KEY_BIT0   0x01
#define HAL_KEY_BIT1   0x02
#define HAL_KEY_BIT2   0x04
#define HAL_KEY_BIT3   0x08
#define HAL_KEY_BIT4   0x10
#define HAL_KEY_BIT5   0x20
#define HAL_KEY_BIT6   0x40
#define HAL_KEY_BIT7   0x80

#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

#define HAL_KEY_PDUP2           0x80
#define HAL_KEY_PDUP1           0x40
#define HAL_KEY_PDUP0           0x20

#define HAL_KEY_DEBOUNCE_VALUE  25  // TODO: adjust this value
#define HAL_KEY_POLLING_VALUE   100


#if defined (HAL_BOARD_CC2533RC)
  #define HAL_KEY_COL_BITS      0xFD
  #define HAL_KEY_COL_PORT      P1
  #define HAL_KEY_COL_SEL       P1SEL
  #define HAL_KEY_COL_DIR       P1DIR
  #define HAL_KEY_COL_INP       P2INP // input mode selection register
  #define HAL_KEY_COL_PDUPBIT   HAL_KEY_BIT6 // pull up, down bit
  #define HAL_KEY_COL_IEN       IEN2                    /* Interrupt Enable Register for SW */
  #define HAL_KEY_COL_IENBIT    HAL_KEY_BIT4            /* Interrupt Enable bit for SW */
  #define HAL_KEY_COL_EDGEBIT   HAL_KEY_BIT1
  #define HAL_KEY_COL_ICTL      P1IEN
  #define HAL_KEY_COL_PXIFG     P1IFG // port interrupt status flag

  #define HAL_KEY_ROW_BITS      0xFF /* Enabled port pins */
  #define HAL_KEY_ROW_PORT      P0
  #define HAL_KEY_ROW_SEL       P0SEL
  #define HAL_KEY_ROW_DIR       P0DIR
  #define HAL_KEY_ROW_INP       P2INP // input mode selection register
  #define HAL_KEY_ROW_PDUPBIT   HAL_KEY_BIT5 // pull up, down bit
  #define HAL_KEY_ROW_PULLDOWN  FALSE
  #define HAL_KEY_ROW_IEN       IEN1                    /* Interrupt Enable Register for SW */
  #define HAL_KEY_ROW_IENBIT    HAL_KEY_BIT5            /* Interrupt Enable bit for SW */
#if HAL_KEY_ROW_PULLDOWN
  #define HAL_KEY_ROW_EDGE      HAL_KEY_RISING_EDGE
#else
  #define HAL_KEY_ROW_EDGE      HAL_KEY_FALLING_EDGE
#endif
  #define HAL_KEY_ROW_EDGEBIT   HAL_KEY_BIT0
  #define HAL_KEY_ROW_ICTL      P0IEN
  #define HAL_KEY_ROW_ICTLBITS  HAL_KEY_ROW_BITS
  #define HAL_KEY_ROW_PXIFG     P0IFG // port interrupt status flag

  #define HAL_KEY_P0INT_LOW_USED 0x0F
  #define HAL_KEY_P0INT_HIGH_USED 0xF0
#endif

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/


/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/
static uint8 halKeySavedKeys;     /* used to store previous key state in polling mode */
static halKeyCBack_t pHalKeyProcessFunction;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */
uint8 halSaveIntKey;              /* used by ISR to save state of interrupt-driven keys */

static uint8 HalKeyConfigured;
static uint8 halKeyTimerRunning;  // Set to true while polling timer is running in interrupt
                                  // enabled mode

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt (void);


/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/
/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit( void )
{
#if (HAL_KEY == TRUE)
  /* Initialize previous key to 0 */
  halKeySavedKeys = HAL_KEY_CODE_NOKEY;

  HAL_KEY_COL_SEL &= (uint8) ~HAL_KEY_COL_BITS; // set pin function to GPIO
  HAL_KEY_COL_DIR |= HAL_KEY_COL_BITS;    // set pin direction to output
  
  if (HAL_KEY_ROW_PULLDOWN)
  {
    HAL_KEY_COL_PORT |= HAL_KEY_COL_BITS; // output high on all columns
  }
  else
  {
    HAL_KEY_COL_PORT &= (uint8) ~HAL_KEY_COL_BITS; // output low on all columns
  }

  HAL_KEY_ROW_SEL &= ~(HAL_KEY_ROW_BITS); // set pin function to GPIO
  HAL_KEY_ROW_DIR &= ~(HAL_KEY_ROW_BITS); // set pin direction to input

  // pull down setup if necessary
  if (HAL_KEY_ROW_PULLDOWN)
  {
    HAL_KEY_ROW_INP |= HAL_KEY_ROW_PDUPBIT;
  }

  /* Initialize callback function */
  pHalKeyProcessFunction  = NULL;

  /* Start with key is not configured */
  HalKeyConfigured = FALSE;
  
  halKeyTimerRunning = FALSE;
#endif /* HAL_KEY */
}

/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig (bool interruptEnable, halKeyCBack_t cback)
{
#if (HAL_KEY == TRUE)
  /* Enable/Disable Interrupt or */
  Hal_KeyIntEnable = interruptEnable;

  /* Register the callback fucntion */
  pHalKeyProcessFunction = cback;

  /* Determine if interrupt is enable or not */
  if (Hal_KeyIntEnable)
  {
    PICTL &= ~(HAL_KEY_ROW_EDGEBIT); // set rising or falling edge
    if (HAL_KEY_ROW_EDGE == HAL_KEY_FALLING_EDGE)
    {
      PICTL |= HAL_KEY_ROW_EDGEBIT;
    }
    HAL_KEY_ROW_ICTL |= HAL_KEY_ROW_ICTLBITS; // Set interrupt enable
    HAL_KEY_ROW_IEN |= HAL_KEY_ROW_IENBIT; // enable interrupt

    /* Do this only after the hal_key is configured - to work with sleep stuff */
    if (HalKeyConfigured == TRUE)
    {
      osal_stop_timerEx( Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
    }
  }
  else    /* Interrupts NOT enabled */
  {
    // disable interrupt
    HAL_KEY_ROW_ICTL &= ~(HAL_KEY_ROW_BITS);
    HAL_KEY_ROW_IEN &= ~(HAL_KEY_ROW_IENBIT);

    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_POLLING_VALUE);    /* Kick off polling */
  }

  /* Key now is configured */
  HalKeyConfigured = TRUE;
#endif /* HAL_KEY */
}

/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead ( void )
{

  uint8 keys, colcode = 8, rowcode = 8, i;

#if (HAL_KEY == TRUE)

  // Disable interrupt, as interrupt can be triggered without key press during
  // scanning process
  HAL_KEY_ROW_ICTL &= (uint8) ~HAL_KEY_ROW_ICTLBITS;

  // Detect row first while all columns are still asserted.
  keys = HAL_KEY_ROW_PORT;
  for (rowcode = 0; rowcode < 8; rowcode++)
  {
    if (HAL_KEY_ROW_PULLDOWN)
    {
      if ((1<<rowcode) & HAL_KEY_ROW_BITS & keys)
      {
        break;
      }
    }
    else
    {
      if (((1 << rowcode) & HAL_KEY_ROW_BITS) && (((1 << rowcode) & keys) == 0))
      {
        break;
      }
    }
  }
  
  // de-asserted all columns
  if (HAL_KEY_ROW_PULLDOWN)
  {
    HAL_KEY_COL_PORT &= (uint8) ~HAL_KEY_COL_BITS;
  }
  else
  {
    HAL_KEY_COL_PORT |= HAL_KEY_COL_BITS;
  }
  
  // assert a column one by one to detect column
  for (colcode = 0; colcode < 8; colcode++)
  {
    if ((1 << colcode) & HAL_KEY_COL_BITS)
    {
      // assert a column
      if (HAL_KEY_ROW_PULLDOWN)
      {
        HAL_KEY_COL_PORT |= (uint8) 1 << colcode;
      }
      else
      {
        HAL_KEY_COL_PORT &= ~ ((uint8) 1 << colcode);
      }

      // TODO: Adjust the delay, create a macro in board configuration
      for (i=0; i<80; i++) asm("NOP");
      
      // read all rows
      keys = HAL_KEY_ROW_PORT;
      
      // de-assert the column
      if (HAL_KEY_ROW_PULLDOWN)
      {
        HAL_KEY_COL_PORT &= ~ ((uint8) 1 << colcode);
      }
      else
      {
        HAL_KEY_COL_PORT |= (uint8) 1 << colcode;
      }

      // check whether the specific row was on.
      if (HAL_KEY_ROW_PULLDOWN)
      {
        if (keys & ((uint8)1 << rowcode))
        {
          break;
        }
      }
      else
      {
        if ((keys & ((uint8)1 << rowcode)) == 0)
        {
          break;
        }
      }
    }
  }
  
  // assert all columns for interrupt and for lower current consumption
  if (HAL_KEY_ROW_PULLDOWN)
  {
    HAL_KEY_COL_PORT |= HAL_KEY_COL_BITS;
  }
  else
  {
    HAL_KEY_COL_PORT &= (uint8) ~HAL_KEY_COL_BITS;
  }

  if (Hal_KeyIntEnable)
  {
    // clear interrupt flag. It is necessary since key scanning sets
    // interrupt flag bits.
    HAL_KEY_ROW_PXIFG = (uint8) (~HAL_KEY_ROW_BITS);
    P0IF = 0;
    
    // enable interrupt
    HAL_KEY_ROW_ICTL |= HAL_KEY_ROW_ICTLBITS;
  }

#endif /* HAL_KEY */

  if (colcode == 8 || rowcode == 8)
  {
    keys = HAL_KEY_CODE_NOKEY; // no key pressed
  }
  else
  {
    keys = (rowcode << 3) | colcode;
  }

  return keys;
}


/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalKeyPoll (void)
{
#if (HAL_KEY == TRUE)

  uint8 keys = 0;

  /*
  *  If interrupts are enabled, get the status of the interrupt-driven keys from 'halSaveIntKey'
  *  which is updated by the key ISR.  If Polling, read these keys directly.
  */
  keys = HalKeyRead();

  /* Exit if polling and no keys have changed */
  if (!Hal_KeyIntEnable)
  {
    if (keys == halKeySavedKeys)
    {
      return;
    }
    halKeySavedKeys = keys;     /* Store the current keys for comparation next time */
  }

  /* Invoke Callback if new keys were depressed */
  if ((keys != HAL_KEY_CODE_NOKEY || Hal_KeyIntEnable) &&
      (pHalKeyProcessFunction))
  {
    // When interrupt is enabled, send HAL_KEY_CODE_NOKEY as well so that
    // application would know the previous key is no longer depressed.
    (pHalKeyProcessFunction) (keys, HAL_KEY_STATE_NORMAL);
  }
  
  if (Hal_KeyIntEnable)
  {
    if (keys != HAL_KEY_CODE_NOKEY)
    {
      // In order to trigger callback again as far as the key is depressed,
      // timer is called here.
      osal_start_timerEx(Hal_TaskID, HAL_KEY_EVENT, 50);
    }
    else
    {
      halKeyTimerRunning = FALSE;
    }
  }
#endif /* HAL_KEY */

}

/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void halProcessKeyInterrupt (void)
{

#if (HAL_KEY == TRUE)

  if (HAL_KEY_ROW_PXIFG & HAL_KEY_ROW_BITS)
  {
    // Disable interrupt
    HAL_KEY_ROW_ICTL &= (uint8) ~HAL_KEY_ROW_ICTLBITS;
    // interrupt flag has been set
    HAL_KEY_ROW_PXIFG = (uint8) (~HAL_KEY_ROW_BITS); // clear interrupt flag
    if (!halKeyTimerRunning)
    {
      halKeyTimerRunning = TRUE;
      osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
    }
    // Enable interrupt
    HAL_KEY_ROW_ICTL |= HAL_KEY_ROW_ICTLBITS;
  }
#endif /* HAL_KEY */
}

/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep ( void )
{
  /* Sleep!!!
   * Nothing to do.
   */
}

/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
  /* Wakeup!!!
   * Nothing to do. In fact. HalKeyRead() may not be called here.
   * Calling HalKeyRead() will trigger key scanning and interrupt flag clearing in the end,
   * which is no longer compatible with hal_sleep.c module.
   */
  /* Wake up and read keys */
  return TRUE;
}

/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINE
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
  HAL_ENTER_ISR();

  halProcessKeyInterrupt();

#if HAL_KEY
  /* Make sure that we clear all enabled, but unused P0IFG bits.
   * For P0 we can only enable or disable high or low nibble, not bit by
   * bit. For P1 and P2 enabling of single bits are possible, therefore
   * will not any unused pins generate interrupts on P1 or P2.
   * We could have checked for low and high nibble in P0, but this
   * isn't necessary as long as we only clear unused pin interrupts.
   */
  P0IFG = (uint8) ~(HAL_KEY_P0INT_LOW_USED | HAL_KEY_P0INT_HIGH_USED);
  P0IF = 0;
#endif

  HAL_EXIT_ISR();
}

/**************************************************************************************************
**************************************************************************************************/
