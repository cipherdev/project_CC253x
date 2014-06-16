/**************************************************************************************************
  Filename:       npi_spi.c
  Revised:        $Date: 2012-09-28 10:25:45 -0700 (Fri, 28 Sep 2012) $
  Revision:       $Revision: 31651 $

  Description:    This file contains the Network Processor Interface (NPI) API.

  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

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
 *                                           Includes
 **************************************************************************************************/

#include "hal_spi.h"

/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/

#define NPI_SPI_RX_AREQ_EVENT          0x4000
#define NPI_SPI_RX_SREQ_EVENT          0x2000
#define NPI_SPI_TX_COMPLETE_EVENT      0x1000

/**************************************************************************************************
 *                                        Type definitions
 **************************************************************************************************/

/**************************************************************************************************
 *                                        Global Variables
 **************************************************************************************************/

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/

// HAL SPI Callbacks
uint8* npSpiPollCallback( void );
void   npSpiReqCallback( uint8 type );
bool   npSpiReadyCallback (void );

// Internal Functions
static void npiSpiSend( uint8 *pBuf );

/**************************************************************************************************
 *                                     Functions
 **************************************************************************************************/

/**************************************************************************************************
 *
 * @fn          NPI_Init
 *
 * @brief       This is the Network Processor Interface task initialization called by OSAL.
 *
 * @param       taskId - task ID assigned after it was added in the OSAL task queue
 *
 * @return      none
 *
 **************************************************************************************************/
void NPI_Init( uint8 taskId )
{
  // save task ID assigned by OSAL
  NPI_TaskId = taskId;
 
#if defined( POWER_SAVING )
  osal_pwrmgr_task_state( NPI_TaskId, PWRMGR_HOLD );
#endif
}


/**************************************************************************************************
 * @fn          NPI_ProcessEvent
 *
 * @brief       This function processes the OSAL events and messages for the NPI task.
 *
 * input parameters
 *
 * @param taskId - The task ID assigned to this application by OSAL at system initialization.
 * @param events - A bit mask of the pending event(s).
 *
 * output parameters
 *
 * None.
 *
 * @return      The events bit map received via parameter with the bits cleared which correspond to
 *              the event(s) that were processed on this invocation.
 **************************************************************************************************
 */
uint16 NPI_ProcessEvent( uint8 taskId, uint16 events )
{
  // unused argument
  (void) taskId;
  
  // first check if there are any inter-task messages sent via OSAL
  if ( events & SYS_EVENT_MSG )
  {
    osal_event_hdr_t *pMsg;
    
    while ((pMsg = (osal_event_hdr_t *) osal_msg_receive(NPI_TaskId)) != NULL)
    {
      switch (pMsg->event)
      {
        default:
          break;
      }

      osal_msg_deallocate((uint8 *) pMsg);
    }

    return (events ^ SYS_EVENT_MSG);
  }
  
  // check if an AREQ or SREQ SPI message was received
  if ( (events & NPI_SPI_RX_AREQ_EVENT) || (events & NPI_SPI_RX_SREQ_EVENT) )
  {
    uint8 *pBuf;

    // either way, get the buffer and populate the callback structure
    if ((pBuf = npSpiGetReqBuf()) != NULL )
    {
      // remove RPC Command Field Type, leaving only Subsystem for client
      pBuf[RPC_POS_CMD0] &= RPC_SUBSYSTEM_MASK;

      if (events & NPI_SPI_RX_AREQ_EVENT)
      {
        // allow SPI FSM to return to idle
        npSpiAReqComplete();

        // call the NPI callback implemented by client to process data
        NPI_AsynchMsgCback( (npiMsgData_t *)pBuf );

        // see if a new request has been received
        npSpiMonitor();

        return (events ^ NPI_SPI_RX_AREQ_EVENT);
      }
      else // NP_SPI_RX_SREQ_EVENT
      {
        // call the NPI callback implemented by client to process data
        NPI_SynchMsgCback( (npiMsgData_t *)pBuf );
        
        // add in Command Field Type
        pBuf[RPC_POS_CMD0] = (pBuf[RPC_POS_CMD0] & RPC_SUBSYSTEM_MASK) | RPC_CMD_SRSP;
        
        // send it back
        npiSpiSend( pBuf );

        return (events ^ NPI_SPI_RX_SREQ_EVENT);
      }
    }
  }
  
  if (events & NPI_SPI_TX_COMPLETE_EVENT)
  {
    npSpiMonitor();
  }

  return ( 0 );  /* Discard unknown events. */
}


/**************************************************************************************************
 * @fn          NPI_SendAsynchData API
 *
 * @brief       This function is called by the client when it has data ready to be sent
 *              asynchronously. This routine allocates an AREQ buffer, copies the client's
 *              payload, and sets up the send.
 *
 * input parameters
 *
 * @param *pBuf  - Pointer to data to be sent.
 * @param len    - Size of the buffer to be sent.
 * @param subsys - RPC command subsystem.
 * @param cmdID  - RPC command ID.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.  // RETURN ERROR IF ALLOCATION FAILS?
 **************************************************************************************************
 */
void NPI_SendAsynchData( npiMsgData_t *pMsg )
{
  uint8 *pAReq;

  // allocate a buffer
  // NOTE: After the following AREQ is TX'ed to the master, the DMA complete ISR
  //       will deallocate this buffer.
  // NOTE: This call already allocates space for the header.
  // NOTE: For some reason, the DMA sends n-1 bytes to the master (the master
  //       always gets zero on the last payload byte). Until this is figured out,
  //       one extra byte is added to the payload length, and sent.
  if ( (pAReq = npSpiAReqAlloc( pMsg->len+RPC_FRAME_HDR_SZ+1 )) != NULL )
  {
    // setup the message
    pAReq[RPC_POS_LEN]  = pMsg->len+1;                                        // payload length
    pAReq[RPC_POS_CMD0] = (pMsg->subSys & RPC_SUBSYSTEM_MASK) | RPC_CMD_AREQ; // type and subsystem
    pAReq[RPC_POS_CMD1] = pMsg->cmdId;                                        // ID

    // copy the client's payload
    osal_memcpy( &pAReq[RPC_POS_DAT0], (uint8 *)pMsg->pData, pMsg->len+1 );
    
    // add one extra dummy byte; use value so it's obvious in memory
    pAReq[RPC_FRAME_HDR_SZ+pMsg->len] = 0xFF;
    
    // enqueue data on NPI task queue
    npiSpiSend( pAReq );
  }
}


/**************************************************************************************************
 * @fn          npSpiReqCallback
 *
 * @brief       This function is called by the SPI driver when a SPI AREQ or SREQ frame has been
 *              received. This function sets an OSAL event to process, based on the type of message,
 *              to process the data.
 *
 *              NOTE: This routine is in the context of the SPI ISR, so schedule an event for NPI,
 *                    and return ASAP. The received data is maintained by SPI until processed by
 *                    NPI.
 *
 * input parameters
 *
 * @param type - The type of message received, either AREQ or SREQ.
 *
 * output parameters
 *
 * None.
 *
 * @return      
 *
 * None.
 **************************************************************************************************
 */
void npSpiReqCallback( uint8 type )
{
  if ( type == RPC_CMD_AREQ )
  {
    osal_set_event( NPI_TaskId, NPI_SPI_RX_AREQ_EVENT );
  }
  else if ( type == RPC_CMD_SREQ )
  {
    osal_set_event( NPI_TaskId, NPI_SPI_RX_SREQ_EVENT );
  }
}


/**************************************************************************************************
 * @fn          npSpiPollCallback
 *
 * @brief       This function is called by the SPI driver when a POLL frame is received.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      A pointer to an OSAL message buffer containing the next AREQ frame to transmit,
 *              if any; NULL otherwise.
 **************************************************************************************************
 */
uint8* npSpiPollCallback(void)
{
  return osal_msg_dequeue( &npiTxQueue );
}


/**************************************************************************************************
 * @fn          npSpiTxCompleteCallback
 *
 * @brief       This function is called by the SPI driver when frame transmission is complete.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 **************************************************************************************************
 */
void npSpiTxCompleteCallback(void)
{
  osal_set_event(NPI_TaskId, NPI_SPI_TX_COMPLETE_EVENT);
}

/**************************************************************************************************
 * @fn          npSpiReadyCallback
 *
 * @brief       This function is called by the SPI driver to check if any data is ready to send.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if data is ready to send; FALSE otherwise.
 **************************************************************************************************
 */
bool npSpiReadyCallback(void)
{
  return !OSAL_MSG_Q_EMPTY( &npiTxQueue );
}


/**************************************************************************************************
 * @fn          npiSpiSend
 *
 * @brief       This function transmits or enqueues the buffer for transmitting on SPI.
 *
 * input parameters
 *
 * @param pBuf - Pointer to the buffer to transmit on the SPI.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void npiSpiSend(uint8 *pBuf)
{
  if ((pBuf[RPC_POS_CMD0] & RPC_CMD_TYPE_MASK) == RPC_CMD_SRSP)
  {
    npSpiSRspReady(pBuf);
  }
  else if ((pBuf[RPC_POS_CMD0] & RPC_CMD_TYPE_MASK) == RPC_CMD_AREQ)
  {
    // slave wants to initiate a master POLL to which the slave will return pBuf
    osal_msg_enqueue(&npiTxQueue, pBuf);
    npSpiAReqReady();
  }
}

/***************************************************************************************************
 * @fn      NPI_SleepRx
 *
 * @brief   Remove the NPI_TaskId block on power saving.
 *
 * @param   None.
 *
 * @return  None.
 ***************************************************************************************************/
void NPI_SleepRx( void )
{
#if defined( POWER_SAVING )
  osal_pwrmgr_task_state( NPI_TaskId, PWRMGR_CONSERVE );
#endif
}

/**************************************************************************************************
 **************************************************************************************************/
