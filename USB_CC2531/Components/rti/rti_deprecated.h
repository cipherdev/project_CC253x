/**************************************************************************************************
  Filename:       rti_deprecated.h
  Revised:        $Date: 2011-08-30 09:47:47 -0700 (Tue, 30 Aug 2011) $
  Revision:       $Revision: 27358 $

  Description:

  This file contains the deprecated declarations of the RTI API.


  Copyright 2011 Texas Instruments Incorporated. All rights reserved.

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
#ifndef RTI_DEPRECATED_H
#define RTI_DEPRECATED_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "rti.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define RTI_PROFILE_CERC                              0x01

#define RTI_ERROR_PAIR_COMPLETE                       0x24

#define RTI_DISCOVERY_DURATION                        100
#define RTI_DISCOVERY_LQI_THRESHOLD                   0
#define RTI_RESP_WAIT_TIME                            100
#define RTI_CERC_DISCOVERY_REPETITION_INTERVAL        1000 // one second, equals 0x00f424 symbols
#define RTI_CERC_MAX_DISCOVERY_REPETITIONS            30
#define RTI_CERC_MAX_REPORTED_NODE_DESCRIPTORS        1
#define RTI_CERC_AUTO_DISC_DURATION                   30000 // thirty seconds, equals 0x1c9c38 symbols

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

// PC Tools left still using the deprecated Read/Write API.
extern RTILIB_API rStatus_t RTI_ReadItem(uint8 itemId, uint8 len, uint8 *pValue);
extern RTILIB_API rStatus_t RTI_WriteItem(uint8 itemId, uint8 len, uint8 *pValue);

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
