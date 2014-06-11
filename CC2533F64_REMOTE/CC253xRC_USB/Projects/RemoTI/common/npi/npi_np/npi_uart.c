/**************************************************************************************************
  Filename:       npi_uart.c
  Revised:        $Date: 2012-10-29 10:15:09 -0700 (Mon, 29 Oct 2012) $
  Revision:       $Revision: 31950 $

  Description:    This file implements the UART physical link between the
                  Application Processor (AP) and the Network Processor (NP).

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
 *                                           Includes
 **************************************************************************************************/

/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/

#define NPI_UART_TX_READY_EVT         0x4000
#define NPI_UART_RX_READY_EVT         0x2000
#define NPI_UART_EXIT_PM_EVT          0x1000  // Exit PM should be higher priority then enter PM.
#define NPI_UART_ENTER_PM_EVT         0x0800

// UART port selection
#if HAL_UART_DMA == 1
# define NPI_UART_PORT                HAL_UART_PORT_0
# define UxCSR                        U0CSR
#elif HAL_UART_DMA == 2
# define NPI_UART_PORT                HAL_UART_PORT_1
# define UxCSR                        U1CSR
#else // HAL_UART_DMA == 0
# define NPI_UART_PORT                HAL_UART_PORT_MAX
#endif

// UART configuration parameters
#define NPI_UART_BAUD_RATE             HAL_UART_BR_115200

#define NPI_UART_FLOW_THRESHOLD        48
#define NPI_UART_IDLE_TIMEOUT          6
#define NPI_UART_TX_MAX                NP_MAX_BUF_LEN
#define NPI_UART_RX_MAX                NP_MAX_BUF_LEN

// Message command IDs
#define CMD_SERIAL_MSG                 0x01

// State values for UART reception - npiUartCbackProcessData
#define SOF_STATE                      0x00
#define CMD_STATE1                     0x01
#define CMD_STATE2                     0x02
#define LEN_STATE                      0x03
#define DATA_STATE                     0x04
#define FCS_STATE                      0x05

// Special NULL message (used for responding to wake up event) first and only byte content
#define NPI_UART_NULL_MSG              0x00

/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/

// HAL UART Callbacks
static void npUartReqCback( uint8 port, uint8 event );

// Internal Functions
static uint8 *npiUartAlloc( uint8 len );
static void   npiUartSend( uint8 *pBuf );
static bool npiUartTxReady(void);
static void   npiProcessData ( uint8 flag );
static uint8  npiUartCalcFCS( uint8 *msg_ptr, uint8 len );

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
  halUARTCfg_t uartConfig;

  NPI_TaskId = taskId;

#if defined( POWER_SAVING )
  // NPI blocks power-saving until receipt of the RTIS_CMD_ID_RTI_ENABLE_SLEEP_REQ.
  osal_pwrmgr_task_state( NPI_TaskId, PWRMGR_HOLD );
#endif

  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = NPI_UART_BAUD_RATE;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = NPI_UART_FLOW_THRESHOLD;
  uartConfig.rx.maxBufSize        = NPI_UART_RX_MAX;
  uartConfig.tx.maxBufSize        = NPI_UART_TX_MAX;
  uartConfig.idleTimeout          = NPI_UART_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc         = npUartReqCback;

  HalUARTOpen(NPI_UART_PORT, &uartConfig);
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
  (void)taskId;

  if ( events & SYS_EVENT_MSG )
  {
    osal_event_hdr_t *pMsg;

    // process all system event messages
    while ((pMsg = (osal_event_hdr_t *) osal_msg_receive(NPI_TaskId)) != NULL)
    {
      switch (pMsg->event)
      {
      // message from serial interface
      case CMD_SERIAL_MSG:
        {
          // view system event message as a NPI RPC message
          uint8 *pBuf = ((npiSysEvtMsg_t *) pMsg)->msg;

          // check the type of message
          if ( (pBuf[RPC_POS_CMD0] & RPC_CMD_TYPE_MASK) == RPC_CMD_AREQ )
          {
            // remove RPC Command Field Type, leaving only Subsystem for client
            pBuf[RPC_POS_CMD0] &= RPC_SUBSYSTEM_MASK;

            // call the NPI callback implemented by client to process data
            NPI_AsynchMsgCback( (npiMsgData_t *)pBuf );
          }
          else if ( (pBuf[RPC_POS_CMD0] & RPC_CMD_TYPE_MASK) == RPC_CMD_SREQ )
          {
            // reply will be required for a synchronous request
            uint8 *pRspMsg;

            // remove RPC Command Field Type, leaving only Subsystem for client
            pBuf[RPC_POS_CMD0] &= RPC_SUBSYSTEM_MASK;

            // always allocate the max size (for now)
            // NOTE: deallocated in npiUartTxReady
            if ((pRspMsg = npiUartAlloc( NP_MAX_BUF_LEN )) != NULL)
            {
              // copy the client's data (max size for now)
              // NOTE: allocated space includes room for SOF and FCS bytes, but
              //       note that the return pointer is one byte past SOF
              osal_memcpy( &pRspMsg[RPC_POS_LEN], &pBuf[RPC_POS_LEN], NP_MAX_BUF_LEN );

              // call the NPI callback implemented by client to process data
              // NOTE: It is assumed that a SRSP will take place in a reasonable
              //       amount of time. If the SREQ processing is expected to take
              //       a long time before the SRSP is returned, then it should be
              //       sent as a AREQ instead.
              NPI_SynchMsgCback( (npiMsgData_t *)pRspMsg );

              // clients data in buffer; add in Command Field Type
              pRspMsg[RPC_POS_CMD0] = (pRspMsg[RPC_POS_CMD0] & RPC_SUBSYSTEM_MASK) | RPC_CMD_SRSP;

              // send it back
              npiUartSend( pRspMsg );
            }
          }
        }
        break;

      default:
        break;
      }

      osal_msg_deallocate((uint8 *) pMsg);
    }

    return (events ^ SYS_EVENT_MSG);
  }

  if (events & NPI_UART_TX_READY_EVT)
  {
    return (npiUartTxReady()) ? events : (events ^ NPI_UART_TX_READY_EVT);
  }

  if (events & NPI_UART_RX_READY_EVT)
  {
    npiProcessData(((events & NPI_UART_ENTER_PM_EVT) != 0));
    return (events ^ NPI_UART_RX_READY_EVT);
  }

  // Exiting PM should have priority over entering PM since it clears the flag to enter and thereby
  // resolves any possible race condition in favor of not entering PM.
  if (events & NPI_UART_EXIT_PM_EVT)
  {
    uint8 *pBuf = osal_msg_allocate(1);

#if defined( POWER_SAVING )
    osal_pwrmgr_task_state(NPI_TaskId, PWRMGR_HOLD);
#endif
    // Send a NPI_UART_NULL_MSG for confirmation of wakeup.
    if (pBuf)
    {
      pBuf[0] = NPI_UART_NULL_MSG;
      osal_msg_enqueue(&npiTxQueue, pBuf);
      osal_set_event(NPI_TaskId, NPI_UART_TX_READY_EVT);
    }

    return (events & ((NPI_UART_EXIT_PM_EVT | NPI_UART_ENTER_PM_EVT) ^ 0xFFFF));
  }

  if (events & NPI_UART_ENTER_PM_EVT)
  {
    if (!HalUARTBusy())
    {
      HalUARTSuspend();
#if defined( POWER_SAVING )
      osal_pwrmgr_task_state(NPI_TaskId, PWRMGR_CONSERVE);
#endif
      return (events ^ NPI_UART_ENTER_PM_EVT);
    }
    else
    {
      // Don't clear event, but effect a task yield to check again.
      return events;
    }
  }

  return 0;  // Discard unknown events.
}


/**************************************************************************************************
 * @fn          NPI_SendAsynchData API
 *
 * @brief       This function is called by the client when it has data ready to
 *              be sent asynchronously. This routine allocates an AREQ buffer,
 *              copies the client's payload, and sets up the send.
 *
 * input parameters
 *
 * @param *pMsg  - Pointer to message data to be sent.
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
  // NOTE: allocated space includes room for SOF and FCS bytes
  // NOTE: deallocated in npiUartTxReady
  if ( (pAReq = npiUartAlloc( pMsg->len )) != NULL )
  {
    // NOTE: allocate returns pointer to first RPC message byte (i.e. to length field)
    pAReq[RPC_POS_LEN]  = pMsg->len;
    pAReq[RPC_POS_CMD0] = (pMsg->subSys & RPC_SUBSYSTEM_MASK) | RPC_CMD_AREQ;
    pAReq[RPC_POS_CMD1] = pMsg->cmdId;

    // copy the client's payload
    osal_memcpy( &pAReq[RPC_POS_DAT0], pMsg->pData, pMsg->len );

    // send it back
    npiUartSend( pAReq );
  }
}


/**************************************************************************************************
 * @fn          npUartReqCback
 *
 * @brief       This function is called by the UART driver when either data has
 *              been received or the transmitter is ready to send.
 *
 * input parameters
 *
 * @param port - The port being used for UART.
 * @param event - The reason for the callback.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void npUartReqCback(uint8 port, uint8 event)
{
  (void)port;

  if (event == HAL_UART_RX_WAKEUP)  // Called from the ISR, so service first.
  {
    osal_set_event(NPI_TaskId, NPI_UART_EXIT_PM_EVT);
  }
  else
  {
    // HAL_UART_TX_EMPTY is the only event ever OR'ed into the event bit mask.
    if (event & HAL_UART_TX_EMPTY)
    {
      osal_set_event(NPI_TaskId, NPI_UART_TX_READY_EVT);
      event &= (HAL_UART_TX_EMPTY ^ 0xFF);
    }

    if (event)  // Anything else: HAL_UART_RX_FULL, HAL_UART_RX_ABOUT_FULL, HAL_UART_RX_TIMEOUT.
    {
      osal_set_event(NPI_TaskId, NPI_UART_RX_READY_EVT);
    }
  }
}

/**************************************************************************************************
 * @fn          npiUartSend
 *
 * @brief       This function transmits or enqueues the buffer for transmitting on UART.
 *
 * input parameters
 *
 * @param pBuf - Pointer to the buffer to transmit on the UART.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void npiUartSend(uint8 *pBuf)
{
  uint8 cksumLen = pBuf[RPC_POS_LEN] + RPC_FRAME_HDR_SZ;

  pBuf[cksumLen] = npiUartCalcFCS(pBuf, cksumLen); // assumes memory byte at the end is allocated
  pBuf--;
  pBuf[0] = RPC_UART_SOF;                          // assumes memory byte at start of pBuf is allocated

  // queue message, and wait for
  osal_msg_enqueue(&npiTxQueue, pBuf);
  osal_set_event(NPI_TaskId, NPI_UART_TX_READY_EVT);
}


/**************************************************************************************************
 * @fn          npiUartTxReady
 *
 * @brief       This function gets and writes the next chunk of data to the UART.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      true if there is still more to Tx; false otherwise.
 **************************************************************************************************
 */
static bool npiUartTxReady(void)
{
  static uint16 npUartTxCnt = 0;
  static uint8 *npUartTxMsg = NULL;
  static uint8 *pMsg = NULL;

  if ( !npUartTxMsg )
  {
    if ( (pMsg = npUartTxMsg = osal_msg_dequeue(&npiTxQueue)) )
    {
      if (pMsg[0] == NPI_UART_NULL_MSG)
      {
        // Special NULL message to respond to wake up
        npUartTxCnt = 1;
      }
      else
      {
        /* | SOP | Data Length | CMD |  DATA   | FSC |
         * |  1  |     1       |  2  | as dLen |  1  |
         */
        npUartTxCnt = pMsg[1] + RPC_UART_FRAME_OVHD + RPC_FRAME_HDR_SZ;
      }
    }
  }

  if ( npUartTxMsg )
  {
    uint16 len = MIN(NPI_UART_TX_MAX, npUartTxCnt);

    len = HalUARTWrite(NPI_UART_PORT, pMsg, len);
    npUartTxCnt -= len;
    //NP_RDYOut = 0;  // Signal to Master that Tx is pending - sleep not ok.

    if ( npUartTxCnt == 0 )
    {
      osal_msg_deallocate(npUartTxMsg);
      npUartTxMsg = NULL;
    }
    else
    {
      pMsg += len;
    }
  }

  return ((npUartTxMsg != NULL) || (npiTxQueue != NULL));
}

/**************************************************************************************************
 * @fn          npiUartAlloc
 *
 * @brief       This function allocates a buffer for Txing on UART.
 *
 * input parameters
 *
 * @param len - Data length required.
 *
 * output parameters
 *
 * None.
 *
 * @return      Pointer to the buffer obtained; possibly NULL if an allocation failed.
 **************************************************************************************************
 */
static uint8* npiUartAlloc( uint8 len )
{
  uint8 *p;

  if ((p = osal_msg_allocate(len + RPC_FRAME_HDR_SZ + RPC_UART_FRAME_OVHD)) != NULL)
  {
    return p + 1;
  }

  return NULL;
}


/***************************************************************************************************
 * @fn      npiProcessData
 *
 * @brief   | SOF | Data Length  |   CMD   |   Data   |  FCS  |
 *          |  1  |     1        |    2    |  0-Len   |   1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to NPI task
 *
 * @param   flag - Flag to indicate that sleep is pending.
 *
 *
 * @return  None
 ***************************************************************************************************/
static void npiProcessData ( uint8 flag )
{
  static uint8 state = SOF_STATE;
  static uint8 LEN_Token;
  static uint8 FSC_Token;
  static uint8 dataLen;
  static npiSysEvtMsg_t *pMsg;
  uint8 ch;

  while (HalUARTRead (NPI_UART_PORT, &ch, 1))
  {
    switch (state)
    {
    case SOF_STATE:
      if (ch == RPC_UART_SOF)
      {
        state = LEN_STATE;
      }
      else if (flag && (ch == NPI_UART_NULL_MSG))
      {
        osal_set_event(NPI_TaskId, NPI_UART_EXIT_PM_EVT);
      }
      break;

    case LEN_STATE:
      if (ch == RPC_UART_SOF)
      {
        // Repeated SOF means the prior SOF was perhaps garbled wakeup character
        break;
      }
      LEN_Token = ch;

      dataLen = 0;

      /* Allocate memory for the data */
      pMsg = (npiSysEvtMsg_t *)osal_msg_allocate(sizeof(npiSysEvtMsg_t)+RPC_FRAME_HDR_SZ+LEN_Token);

      if (pMsg)
      {
        pMsg->hdr.event = CMD_SERIAL_MSG;
        pMsg->msg = (uint8*)(pMsg+1);
        pMsg->msg[RPC_POS_LEN] = LEN_Token;
        state = CMD_STATE1;
      }
      else
      {
        state = SOF_STATE;
        return;
      }
      break;

    case CMD_STATE1:
      pMsg->msg[RPC_POS_CMD0] = ch;
      state = CMD_STATE2;
      break;

    case CMD_STATE2:
      pMsg->msg[RPC_POS_CMD1] = ch;
      /* If there is no data, skip to FCS state */
      if (LEN_Token)
      {
        state = DATA_STATE;
      }
      else
      {
        state = FCS_STATE;
      }
      break;

    case DATA_STATE:
      /* Fill in the buffer the first byte of the data */
      pMsg->msg[RPC_FRAME_HDR_SZ + dataLen++] = ch;

      /* Check number of bytes left in the Rx buffer */
      if (dataLen < LEN_Token)
      {
        dataLen +=  HalUARTRead (NPI_UART_PORT, &pMsg->msg[RPC_FRAME_HDR_SZ + dataLen], LEN_Token-dataLen);
      }

      /* If number of bytes read is equal to data length, time to move on to FCS */
      if ( dataLen == LEN_Token )
      {
        state = FCS_STATE;
      }
      break;

    case FCS_STATE:
      state = SOF_STATE;
      FSC_Token = ch;

      if ((npiUartCalcFCS ((uint8*)&pMsg->msg[0], RPC_FRAME_HDR_SZ + LEN_Token) == FSC_Token))
      {
        osal_msg_send( NPI_TaskId, (uint8 *)pMsg );
        osal_set_event(NPI_TaskId, NPI_UART_RX_READY_EVT);
        return;
      }
      else
      {
        osal_msg_deallocate ( (uint8 *)pMsg );
      }
      break;

    default:
     break;
    }
  }
}

/***************************************************************************************************
 * @fn      npiUartCalcFCS
 *
 * @brief   Calculate the FCS of a message buffer by XOR'ing each byte.
 *          Remember to NOT include SOP and FCS fields, so start at the CMD field.
 *
 * @param   byte *msg_ptr - message pointer
 * @param   byte len - length (in bytes) of message
 *
 * @return  result byte
 ***************************************************************************************************/
uint8 npiUartCalcFCS( uint8 *msg_ptr, uint8 len )
{
  uint8 x;
  uint8 xorResult;

  xorResult = 0;

  for ( x = 0; x < len; x++, msg_ptr++ )
    xorResult = xorResult ^ *msg_ptr;

  return ( xorResult );
}

/***************************************************************************************************
 * @fn      NPI_SleepRx
 *
 * @brief   Ready the UART for sleep by configuring the RX port to receive a GPIO interrupt.
 *
 * @param   None.
 *
 * @return  None.
 ***************************************************************************************************/
void NPI_SleepRx( void )
{
  osal_set_event(NPI_TaskId, NPI_UART_ENTER_PM_EVT);
}

/**************************************************************************************************
 **************************************************************************************************/
