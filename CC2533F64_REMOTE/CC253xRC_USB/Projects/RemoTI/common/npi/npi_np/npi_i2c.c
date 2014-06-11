/**************************************************************************************************
  Filename:       npi_i2c.c
  Revised:        $Date: 2012-10-26 09:59:10 -0700 (Fri, 26 Oct 2012) $
  Revision:       $Revision: 31921 $

  Description:

  This file contains the Network Processor Interface (NPI) API for I2C.


  Copyright 2010-2012 Texas Instruments Incorporated. All rights reserved.

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

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "hal_assert.h"
#include "hal_i2c.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#ifdef NPI_I2C_SRDY_ACTIVE_HIGH
  #define NPI_I2C_SRDY   1
#else
  #define NPI_I2C_SRDY   0
#endif // NPI_I2C_SRDY_ACTIVE_HIGH

#ifdef I2C_CONFIG_ON_PORT1
// SRDY pin
#define SRDY     P1_2
#define SRDY_BIT BV(2)
#else
// SRDY pin
#define SRDY     P0_6
#define SRDY_BIT BV(6)
#endif // I2C_CONFIG_ON_PORT1

// MRDY pin
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
#ifdef I2C_CONFIG_ON_PORT1
  #define MRDY      P1_3
  #define MRDY_BIT  BV(3)
#else
  #define MRDY     P0_7
  #define MRDY_BIT BV(7)

#endif // I2C_CONFIG_ON_PORT1
  #ifdef NPI_I2C_MRDY_ACTIVE_HIGH
    #define NPI_I2C_MRDY   1
  #else
    #define NPI_I2C_MRDY   0
  #endif // NPI_I2C_MRDY_ACTIVE_HIGH
#endif // NPI_I2C_MRDY_ACTIVE_HIGH) || NPI_I2C_MRDY_ACTIVE_LOW

// OSAL Events
//efine SYS_EVENT_MSG                  0x8000
#define NPI_EVENT_I2C_SREQ             0x4000
#define NPI_EVENT_I2C_POLL             0x2000
#define NPI_EVENT_I2C_AREQ             0x1000
#if defined POWER_SAVING
#define NPI_EVENT_I2C_EXIT_PM          0x0800
#define NPI_EVENT_I2C_ENTER_PM         0x0400
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

// An OSAL queue for receiving AREQ messages.
static osal_msg_q_t npiRxQueue;
// The local buffer for receiving an SREQ and sending the SRSP.
static npiMsgData_t npiMsgSREQ;

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static uint8 npiI2CCB(uint8 cnt);
static void npiGpioInit(void);
static void npiRecvAsynchData(npiMsgData_t *pMsg);
void npiProcessSREQCmd(void);
void npiProcessPOLLCmd(void);
void npiProcessAREQCmd(void);

/**************************************************************************************************
 *
 * @fn          NPI_Init
 *
 * @brief       This is the Network Processor Interface task initialization called by OSAL.
 *              NOTE: NPI task power manager state is always conserve.
 *
 * @param       taskId - task ID assigned after it was added in the OSAL task queue
 *
 * @return      none
 */
void NPI_Init(uint8 taskId)
{
  NPI_TaskId = taskId;
  npiGpioInit();
  HalI2CInit(HAL_I2C_SLAVE_ADDR_DEF, npiI2CCB);  // Open I2C as Slave device.

#if defined POWER_SAVING
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
  // Note that the RTI task still blocks power-saving until the host requests RTI_Init() and
  // (rtiState == RTI_STATE_READY).
  (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_ENTER_PM);
#endif
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
 */
uint16 NPI_ProcessEvent(uint8 taskId, uint16 events)
{
  (void)taskId;

  if (events & NPI_EVENT_I2C_SREQ)
  {
    npiProcessSREQCmd();
  }

  if (events & NPI_EVENT_I2C_POLL)
  {
    npiProcessPOLLCmd();
  }

  if (events & NPI_EVENT_I2C_AREQ)
  {
    npiProcessAREQCmd();
  }

#if defined POWER_SAVING
  if (events & NPI_EVENT_I2C_EXIT_PM)
  {
    // NPI blocks power-saving until receipt of RTIS_CMD_ID_RTI_ENABLE_SLEEP_REQ and/or an
    // MRDY/SRDY de-assertion.
    (void)osal_pwrmgr_task_state(NPI_TaskId, PWRMGR_HOLD);
    if (MRDY == NPI_I2C_MRDY)
    {
      SRDY = NPI_I2C_SRDY;
    }
  }
  else if (events & NPI_EVENT_I2C_ENTER_PM)
  {
    if ((SRDY != NPI_I2C_SRDY) && HalI2CReady2Sleep())  // Conserve power when ready.
    {
      (void)osal_pwrmgr_task_state(NPI_TaskId, PWRMGR_CONSERVE);
    }
    else  // Keep the ENTER_PM latched until PM is ok or another explicit EXIT_PM.
    {
      return events;
    }
  }
#endif

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
 */
void NPI_SendAsynchData( npiMsgData_t *pMsg )
{
  uint8* p;

  if ((p = osal_msg_allocate(pMsg->len + RPC_FRAME_HDR_SZ)) != NULL)
  {
    // Prepare message
    p[RPC_POS_LEN]  = pMsg->len;
    p[RPC_POS_CMD0] = (pMsg->subSys & RPC_SUBSYSTEM_MASK) | RPC_CMD_AREQ;
    p[RPC_POS_CMD1] = pMsg->cmdId;

    osal_memcpy( &p[RPC_POS_DAT0], pMsg->pData, pMsg->len );  // copy the client's payload
    osal_msg_enqueue(&npiTxQueue, p);  // Enqueue the AREQ.
    SRDY = NPI_I2C_SRDY;  // Assert SRDY to notify master we have data.
#if defined POWER_SAVING
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_EXIT_PM);
#endif
  }
}

/**************************************************************************************************
 * @fn      NPI_SleepRx
 *
 * @brief   Sleep command has been received from the host
 *
 * @param   None.
 *
 * @return  None.
 */
void NPI_SleepRx( void )
{
#if defined POWER_SAVING
  (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_ENTER_PM);
#endif
}

/*************************************************************************************************
 *
 * @fn      npiI2CCB
 *
 * @brief   Callback service for I2C
 *
 * @param   cnt - Count of I2C data received if non-zero; otherwise Master Read indication.
 *
 * @return  void
 *
 */
static uint8 npiI2CCB(uint8 cnt)
{
  if (cnt == 0)  // Notification of Master Read request.
  {
    /* In proper RPC message exchange, the Host App will have always sent an SREQ or POLL before
     * attempting the Master Read request, so return TRUE to clock stretch until ready with the
     * expected response.
     */
    return TRUE;
  }
  else if (npiMsgSREQ.subSys == 0)  // If the local global buffer is available for use.
  {
    if (cnt == HalI2CRead(cnt, (uint8 *)&npiMsgSREQ))
    {
      if (npiMsgSREQ.subSys == 0)
      {
        (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_POLL);
      }
      else if ((npiMsgSREQ.subSys & RPC_CMD_TYPE_MASK) == RPC_CMD_AREQ)
      {
        npiRecvAsynchData(&npiMsgSREQ);
        npiMsgSREQ.subSys = 0;  // Free the local global buffer for re-use.
      }
      else
      {
        (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_SREQ);
      }
    }
  }

  return FALSE;  // N/A when cnt is non-zero.
}

/**************************************************************************************************
 * @fn          npiGpioInit
 *
 * @brief       This configures the GPIOs (SRDY and MRDY) needed for the RNP protocol over I2C.
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
 */
static void npiGpioInit(void)
{
  SRDY = !NPI_I2C_SRDY;  // Set initial state
#ifdef I2C_CONFIG_ON_PORT1
  P1SEL &= ~(SRDY_BIT);  // Pin is GPIO
  P1DIR |= SRDY_BIT;     // Pin is output
#else
  P0SEL &= ~(SRDY_BIT);  // Pin is GPIO
  P0DIR |=  SRDY_BIT;    // Pin is output
#endif // I2C_CONFIG_ON_PORT1

#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
#ifdef I2C_CONFIG_ON_PORT1
  P1SEL &= ~(MRDY_BIT);  // Pin is GPIO
  P1DIR &= ~(MRDY_BIT);  // Pin is input
#else
  P0SEL &= ~(MRDY_BIT);  // Pin is GPIO
  P0DIR &= ~(MRDY_BIT);  // Pin is input
#endif // I2C_CONFIG_ON_PORT1
#if NPI_I2C_MRDY         // active  high MRDY
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH)
#ifdef I2C_CONFIG_ON_PORT1
  PICTL &= ~(BV(1));     // Rising Edge on P1_L int
#else
  PICTL &= ~(BV(0));     // Rising Edge on P0 int
#endif // // I2C_CONFIG_ON_PORT1
#else  // if (NPI_I2C_MRDY_ACTIVE_LOW)
#ifdef I2C_CONFIG_ON_PORT1
  PICTL |= (BV(1));     // Falling Edge on P1_L int
#else
  PICTL |= (BV(0));     // Falling Edge on P0 int
#endif // // I2C_CONFIG_ON_PORT1
#endif // (NPI_I2C_MRDY_ACTIVE_HIGH)
#else                    // active log SRDY
#ifdef I2C_CONFIG_ON_PORT1
  PICTL |= BV(1);        // Falling Edge on P1 int
#else
  PICTL |= BV(0);        // Falling Edge on P0 int
#endif // // I2C_CONFIG_ON_PORT1
#endif
  P0IFG &= MRDY_BIT;     // Clear P0 interrupt
#ifdef I2C_CONFIG_ON_PORT1
  P1IFG &= ~MRDY_BIT;   // Clear P1 interrupt
  P1IEN |= MRDY_BIT;    // P1 MRDY bit interrupt enable
  IEN2  |= BV(4);       // enable P1 interrupts
#else
  P0IFG &= ~MRDY_BIT;     // Clear P0 interrupt
  P0IEN |= MRDY_BIT;     // P0 MRDY bit interrupt enable
  IEN1  |= BV(5);       // enable P0 interrupts
#endif // I2C_CONFIG_ON_PORT1
#endif // defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
}

/**************************************************************************************************
 * @fn          npiRecvAsynchData API
 *
 * @brief       This function is called upon receiving an AREQ from the master.
 *
 * input parameters
 *
 * @param *pMsg  - Pointer to the AREQ received.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void npiRecvAsynchData(npiMsgData_t *pMsg)
{
  const uint8 len = pMsg->len + RPC_FRAME_HDR_SZ;
  uint8 *pReq;

  if ((pReq = osal_msg_allocate(len)) != NULL)
  {
    (void)osal_memcpy(pReq, pMsg, len);
    (void)osal_msg_enqueue(&npiRxQueue, pReq);
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_AREQ);
  }
}

/**************************************************************************************************
 * @fn          npiProcessSREQCmd
 *
 * @brief       This fucntion processes the SREQ command received from the master.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void npiProcessSREQCmd(void)
{
  // Remove RPC Command Field Type, leaving only Subsystem for client.
  npiMsgSREQ.subSys &= RPC_SUBSYSTEM_MASK;

  NPI_SynchMsgCback(&npiMsgSREQ);
  uint8 len = npiMsgSREQ.len + RPC_FRAME_HDR_SZ;

  // Clients data in buffer; add in Command Field Type
  npiMsgSREQ.subSys = (npiMsgSREQ.subSys & RPC_SUBSYSTEM_MASK) | RPC_CMD_SRSP;

  (void)HalI2CWrite(len, (uint8 *)&npiMsgSREQ.len);
  npiMsgSREQ.subSys = 0;  // Free the local global buffer for re-use.
}

/**************************************************************************************************
 * @fn          npiProcessPOLLCmd
 *
 * @brief       This fucntion processes the POLL command received from the master.
 *
 * input parameters
 *
 * @param
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void npiProcessPOLLCmd(void)
{
  uint8 *pBuf = osal_msg_dequeue(&npiTxQueue);

  if (pBuf == NULL)
  {
    SRDY = !NPI_I2C_SRDY;  // Always de-assert SRDY before responding to a POLL with no data.
#if defined POWER_SAVING
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
    // Whenever the host has MRDY control to wake the NP from PM, re-enter PM automatically.
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_ENTER_PM);
#endif
#endif
    uint8 pollBuf[RPC_FRAME_HDR_SZ] = { 0, 0, 0 };
    (void)HalI2CWrite(RPC_FRAME_HDR_SZ, pollBuf);
  }
  else
  {
    (void)HalI2CWrite(pBuf[RPC_POS_LEN] + RPC_FRAME_HDR_SZ, (uint8 *)&pBuf[RPC_POS_LEN]);
    (void)osal_msg_deallocate(pBuf);
  }

  // Deassert SRDY only if the Rx & Tx queues are empty.
  if ((OSAL_MSG_Q_EMPTY(&npiRxQueue)) && (OSAL_MSG_Q_EMPTY(&npiTxQueue)))
  {
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
    // Don't de-assert if MRDY is asserted.
    if (MRDY == !NPI_I2C_MRDY)
#endif
    {
      SRDY = !NPI_I2C_SRDY;
#if defined POWER_SAVING
#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
      // Whenever the host has MRDY control to wake the NP from PM, re-enter PM automatically.
      (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_ENTER_PM);
#endif
#endif
    }
  }
}

/**************************************************************************************************
 * @fn          npiProcessAREQCmd
 *
 * @brief       This function processes the received and queued AREQ messages.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void npiProcessAREQCmd(void)
{
  uint8 *pBuf;

  if ((pBuf = osal_msg_dequeue(&npiRxQueue)) != NULL)
  {
    // remove RPC Command Field Type, leaving only Subsystem for client.
    pBuf[RPC_POS_CMD0] &= RPC_SUBSYSTEM_MASK;

    // Call the NPI callback implemented by client to process data.
    NPI_AsynchMsgCback((npiMsgData_t *)pBuf);
    (void)osal_msg_deallocate(pBuf);
  }

  if (!OSAL_MSG_Q_EMPTY(&npiRxQueue))
  {
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_AREQ);
  }
}

#if defined (NPI_I2C_MRDY_ACTIVE_HIGH) || defined (NPI_I2C_MRDY_ACTIVE_LOW)
/**************************************************************************************************
 * @fn      Port Interrupt Handler
 *
 * @brief   This function is the Port 0 interrupt service routine.
 *
 * @param   None.
 *
 * @return  None.
 */
#ifdef I2C_CONFIG_ON_PORT1
HAL_ISR_FUNCTION(port1Isr, P1INT_VECTOR)
{
  HAL_ENTER_ISR();

  if (P1IFG & MRDY_BIT)
  {
#if defined POWER_SAVING
//Do Not assert SRDY here, since we may just exit sleep mode on this interrupt, 
// Assert it when PM event will be process if needed    
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_EXIT_PM);
#else
    SRDY = NPI_I2C_SRDY;
#endif
  }

  // NOTE: This must be the last thing done in this routine.
  P1IFG = 0;  // clear all interrupt bits
  P1IF  = 0;  // clear the next level

  HAL_EXIT_ISR();
}
#else
HAL_ISR_FUNCTION(port0Isr, P0INT_VECTOR)
{
  HAL_ENTER_ISR();

  if (P0IFG & MRDY_BIT)
  {
#if defined POWER_SAVING
//Do Not assert SRDY here, since we may just exit sleep mode on this interrupt, 
// Assert it when PM event will be process if needed    
    (void)osal_set_event(NPI_TaskId, NPI_EVENT_I2C_EXIT_PM);
#else
    SRDY = NPI_I2C_SRDY;
#endif
  }

  // NOTE: This must be the last thing done in this routine.
  P0IFG = 0;  // clear all interrupt bits
  P0IF  = 0;  // clear the next level

  HAL_EXIT_ISR();
}
#endif // I2C_CONFIG_ON_PORT1
#endif

/**************************************************************************************************
*/
