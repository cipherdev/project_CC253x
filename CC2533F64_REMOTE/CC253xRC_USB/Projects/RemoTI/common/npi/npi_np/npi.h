/**************************************************************************************************
  Filename:       npi.h
  Revised:        $Date: 2010-09-27 08:27:30 -0700 (Mon, 27 Sep 2010) $
  Revision:       $Revision: 23918 $

  Description:    This file contains the Network Processor Interface (NPI),
                  which abstracts the physical link between the Application
                  Processor (AP) and the Network Processor (NP). The NPI
                  serves as the HAL's client for the SPI and UART drivers, and
                  provides API and callback services for its client.

  Copyright 2008-2010 Texas Instruments Incorporated. All rights reserved.

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

#ifndef NPI_H
#define NPI_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "hal_board.h"
#include "hal_rpc.h"

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

// Buffer size - it has to be big enough for the largest NPI RPC packet and NPI overhead
#define NP_MAX_BUF_LEN                 128

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

// NPI API and NPI Callback Message structure
// NOTE: Fields are position dependent. Do not rearrange!
typedef struct
{
  uint8 len;
  uint8 subSys;
  uint8 cmdId;
  uint8 pData[NP_MAX_BUF_LEN];
} npiMsgData_t;

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

extern uint8 NPI_TaskId;

/*********************************************************************
 * FUNCTIONS
 */

// NPI OSAL related functions
extern void NPI_Init( uint8 taskId );
extern uint16 NPI_ProcessEvent( uint8 taskId, uint16 events );

//
// Network Processor Interface APIs
//

/***************************************************************************************************
 * @fn      NPI_SleepRx
 *
 * @brief   Ready the UART for sleep by switching the RX port to a GPIO with
 *          interrupts enabled.
 *
 * @param   None.
 *
 * @return  None.
 ***************************************************************************************************/
extern void NPI_SleepRx( void );

/**************************************************************************************************
 * @fn          NPI_SendAsynchData
 *
 * @brief       This function is called by the client when it has data ready to
 *              be sent asynchronously. This routine allocates an AREQ buffer,
 *              copies the client's payload, and sets up the send.
 *
 * input parameters
 *
 * @param *pMsg  - Pointer to data to be sent asynchronously (i.e. AREQ).
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_SendAsynchData( npiMsgData_t *pMsg );


/**************************************************************************************************
 * @fn          NPI_AsynchMsgCback
 *
 * @brief       This function is a NPI callback to the client that inidcates an
 *              asynchronous message has been received. The client software is
 *              expected to complete this call.
 *
 *              Note: The client must copy this message if it requires it
 *                    beyond the context of this call.
 *
 * input parameters
 *
 * @param       *pMsg - A pointer to an asychronously received message.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_AsynchMsgCback( npiMsgData_t *pMsg );


/**************************************************************************************************
 * @fn          NPI_SynchMsgCback
 *
 * @brief       This function is a NPI callback to the client that inidcates an
 *              synchronous message has been received. The client software is
 *              expected to complete this call.
 *
 *              Note: The client must process this message and provide a reply
 *                    using the same buffer in the context of this call.
 *
 * input parameters
 *
 * @param       *pMsg - A pointer to an sychronously received message.
 *
 * output parameters
 *
 * @param       *pMsg - A pointer to the synchronous reply message.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_SynchMsgCback( npiMsgData_t *pMsg );


/**************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif /* NPI_H */
