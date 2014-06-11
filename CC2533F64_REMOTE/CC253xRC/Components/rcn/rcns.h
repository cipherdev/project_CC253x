/**************************************************************************************************
  Filename:       rcns.h
  Revised:        $Date: 2009-02-19 15:01:04 -0800 (Thu, 19 Feb 2009) $
  Revision:       $Revision: 19209 $

  Description:    This file contains interface to RCN surrogate module on network processor side


  Copyright 2008 Texas Instruments Incorporated. All rights reserved.

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

#ifndef RCNS_H
#define RCNS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "rcn_nwk.h"
  
/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

/*********************************************************************
 * FUNCTIONS
 */


/**************************************************************************************************
 * @fn          RCNS_SerializeCback
 *
 * @brief       This function serializes RCN callback and send it over to application processor.
 *
 * input parameters
 *
 * @param       pData - Pointer to callback event structure.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern void RCNS_SerializeCback(rcnCbackEvent_t *pData);

/**************************************************************************************************
 * @fn          RCNS_HandleAsyncMsg
 *
 * @brief       This function handles serialized RCN interface message which came as asynchronous
 *              message.
 *
 * input parameters
 *
 * @param       pData - pointer to a received serialized data starting from length, subsystem id
 *                      and command id, etc.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RCNS_HandleAsyncMsg( uint8 *pData );

/**************************************************************************************************
 * @fn          RCNS_HandleSyncMsg
 *
 * @brief       This function handles serialized RCN interface message which came as synchronous
 *              message and builds a synchronous response message.
 *
 * input parameters
 *
 * @param       pData - pointer to a received serialized data starting from length, subsystem id
 *                      and command id, etc.
 *                      The memory space pointed by this pointer shall be overwritten with a
 *                      synchronous response message.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if RCNS wants control of RCN callback functions, FALSE otherwise.
 */
uint8 RCNS_HandleSyncMsg( uint8 *pData );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RCNS_H */
