/**************************************************************************************************
  Filename:       rsa.h
  Revised:        $Date: 2010-11-19 17:55:33 -0800 (Fri, 19 Nov 2010) $
  Revision:       $Revision: 24473 $

  Description:    This file contains the the HID Remote Control Sample Application
                  protypes and definitions


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

#ifndef RSA_H
#define RSA_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"

/**************************************************************************************************
 *                                        User's  Defines
 **************************************************************************************************/

#ifndef RSA_KEY_INT_ENABLED
#define RSA_KEY_INT_ENABLED       TRUE
#endif

#ifndef RCN_SUPPRESS_SCAN_RESULT
#define RCN_SUPPRESS_SCAN_RESULT  FALSE
#endif
  
#ifndef RSA_FEATURE_IR
#define RSA_FEATURE_IR            FALSE         /* IR interface support */
#endif
  
/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/* Event IDs */
#define RSA_PAIR_EVENT   0x0003

// Channel bitmap is effective only for RCN coordinator. RCN end device is not affected by
// this bitmap.
#ifdef RSA_SINGLE_CH
  // Single channel test purpose special build
# define RSA_MAC_CHANNELS MAC_CHAN_MASK(MAC_CHAN_15)
#else
# define RSA_MAC_CHANNELS \
  (MAC_CHAN_MASK(MAC_CHAN_15) | MAC_CHAN_MASK(MAC_CHAN_20) | MAC_CHAN_MASK(MAC_CHAN_25))
#endif
  
/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/
extern uint8 RSA_TaskId;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the RF4CE Sample Application
 */
extern void RSA_Init( uint8 task_id );

/*
 * Task Event Processor for the RF4CE Sample Application
 */
extern uint16 RSA_ProcessEvent( uint8 task_id, uint16 events );

/*
 * Handle keys
 */
extern void RSA_KeyCback( uint8 keys, uint8 state);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RSA_H */
