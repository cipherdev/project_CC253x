/**************************************************************************************************
  Filename:       RTIS.c
**************************************************************************************************/

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

/* OSAL includes */
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

/* RCN surrogate includes */
#include "rcns.h"

/* RTIS includes */
#include "rti.h"
#include "rtis.h"

/* Serial Bootloader command handler */
#if (defined FEATURE_SERIAL_BOOT || defined FEATURE_SBL)
#include "sb_load.h"
#endif

#include "zid.h"
#if FEATURE_ZID
#include "zid_common.h"
#endif

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

static uint8 rtisState;  // current state

/**************************************************************************************************
 *
 * @fn          RTI_Init
 *
 * @brief       This is the RemoTI task initialization called by OSAL.
 *
 * input parameters
 *
 * @param       taskId: Task ID assigned after it was added in the OSAL task
 *                      queue.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTIS_Init( void )
{
  rtisState = RTIS_STATE_READY;
}

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
 */
void NPI_SynchMsgCback( npiMsgData_t *pMsg )
{
  if (pMsg->subSys == RPC_SYS_RCAF)
  {
    switch( pMsg->cmdId )
    {
    case RTIS_CMD_ID_RTI_READ_ITEM:  // Deprecated.
      pMsg->len = 1 + pMsg->pData[1];  // set up msg length before pMsg->pData[1] is overwritten.
      // unpack itemId and len data and send to RTI to read config interface
      // using input buffer as the reply buffer
      // Note: the status is stored in the first word of the payload
      // Note: the subsystem Id and command Id remain the same, so we only
      //       need return to complete the synchronous call
      pMsg->pData[0] = RTI_ReadItemEx(RTI_PROFILE_RTI, pMsg->pData[0],
                                             pMsg->pData[1], &pMsg->pData[1]);
      break;

    case RTIS_CMD_ID_RTI_WRITE_ITEM:  // Deprecated.
      pMsg->len = 1;  // confirm message length has to be set up
      // unpack itemId and len data and send to RTI to write config interface
      // Note: the status is stored in the first word of the payload
      // Note: the subsystem Id and command Id remain the same, so we only
      //       need return to complete the synchronous call
      pMsg->pData[0] = RTI_WriteItemEx(RTI_PROFILE_RTI, pMsg->pData[0],
                                        pMsg->pData[1], &pMsg->pData[2]);
      break;

#ifdef FEATURE_TEST_MODE // global test mode compile flag
    case RTIS_CMD_ID_RTI_RX_COUNTER_GET_REQ:
#if (defined HAL_MCU_MSP430)
      {
        uint16 tmp = RTI_TestRxCounterGetReq( pMsg->pData[0] );
        (void)osal_memcpy(pMsg->pData, (uint8 *)&tmp, sizeof(uint16));
      }
#else
      *(uint16 *) &pMsg->pData[0] = RTI_TestRxCounterGetReq( pMsg->pData[0] );
#endif
      pMsg->len = 2;
      break;
#endif // FEATURE_TEST_MODE

    case RTIS_CMD_ID_RTI_READ_ITEM_EX:
      pMsg->len = pMsg->pData[2] + 1;
      pMsg->pData[0] = RTI_ReadItemEx(pMsg->pData[0], pMsg->pData[1],
                                      pMsg->pData[2], &pMsg->pData[1]);
      break;

    case RTIS_CMD_ID_RTI_WRITE_ITEM_EX:
      pMsg->len = 1;
      pMsg->pData[0] = RTI_WriteItemEx(pMsg->pData[0], pMsg->pData[1],
                                       pMsg->pData[2], &pMsg->pData[3]);
      break;

    default:
      // nothing can be done here!
      break;
    }
  }
#if !defined CC2533F64
  else if (pMsg->subSys == RPC_SYS_RCN)
  {
    // special case handling
    if (RCNS_HandleSyncMsg((uint8 *) pMsg))
    {
      rtisState = RTIS_STATE_NETWORK_LAYER_BRIDGE;
      RTI_SetBridgeMode((rtiRcnCbackFn_t) RCNS_SerializeCback);
    }
  }
#endif
}

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
 */
void NPI_AsynchMsgCback( npiMsgData_t *pMsg )
{
  if (pMsg->subSys == RPC_SYS_RCAF)
  {
    switch( pMsg->cmdId )
    {
    // ping request
    case RTIS_CMD_ID_TEST_PING_REQ:
      // disregard
      break;

    // init request
    case RTIS_CMD_ID_RTI_INIT_REQ:
      if (rtisState == RTIS_STATE_NETWORK_LAYER_BRIDGE)
      {
        // Pull RTI out of bridge mode
        RTI_SetBridgeMode(NULL);
      }

      // set state during init request
      rtisState = RTIS_STATE_READY;

      // send the init request; no parameters required
      // handle the confirm separately
      RTI_InitReq();
      break;

    // pair request
    case RTIS_CMD_ID_RTI_PAIR_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        // send the pair request; no parameters required
        // handle the confirm separately
        RTI_PairReq();
      }
      break;

    case RTIS_CMD_ID_RTI_PAIR_ABORT_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        RTI_PairAbortReq();
      }
      break;

    // send data request
    case RTIS_CMD_ID_RTI_SEND_DATA_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        // unpack parameters, and send data request to RTI
        // handle the confirm separately
        RTI_SendDataReq(  pMsg->pData[0],         // dstIndex
                          pMsg->pData[1],         // profileId
                          (uint16)pMsg->pData[2] | ((uint16)pMsg->pData[3] << 8), // vendorId
                          pMsg->pData[4],         // txOptions
                          pMsg->pData[5],         // len
                         &pMsg->pData[6] );       // *pData
      }
      break;

    // allow pair request
    case RTIS_CMD_ID_RTI_ALLOW_PAIR_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        // send the allow pair request; no parameters required
        // handle the confirm separately
        RTI_AllowPairReq();
      }
      break;

    case RTIS_CMD_ID_RTI_ALLOW_PAIR_ABORT_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        RTI_AllowPairAbortReq();
      }
      break;

    case RTIS_CMD_ID_RTI_STANDBY_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        RTI_StandbyReq(pMsg->pData[0]); // mode
      }
      break;

    case RTIS_CMD_ID_RTI_RX_ENABLE_REQ:
      if ( rtisState == RTIS_STATE_READY )
      {
        RTI_RxEnableReq( ((uint16) pMsg->pData[0] | ((uint16) pMsg->pData[1] << 8)) );
      }
      break;

    // enable sleep request
    case RTIS_CMD_ID_RTI_ENABLE_SLEEP_REQ:
      RTI_EnableSleepReq();
      // Request the network processor interface module to turn receiver off.
      NPI_SleepRx();
      break;

    // enable sleep request
    case RTIS_CMD_ID_RTI_DISABLE_SLEEP_REQ:
      RTI_DisableSleepReq();
      break;

#ifdef FEATURE_TEST_MODE // global test mode compile flag
    case RTIS_CMD_ID_RTI_SW_RESET_REQ:
      RTI_SwResetReq();
      break;

    case RTIS_CMD_ID_RTI_TEST_MODE_REQ:
      RTI_TestModeReq( pMsg->pData[0], // mode
                       pMsg->pData[1], // txPower
                       pMsg->pData[2] ); // channel
      break;

#endif // FEATURE_TEST_MODE

    case RTIS_CMD_ID_RTI_UNPAIR_REQ:
      RTI_UnpairReq( pMsg->pData[0] ); // dstIndex
      break;

    default:
      // nothing can be done here!
      break;
    }
  }
#if !defined CC2533F64
  else if (pMsg->subSys == RPC_SYS_RCN)
  {
    rtisState = RTIS_STATE_NETWORK_LAYER_BRIDGE;
    RTI_SetBridgeMode((rtiRcnCbackFn_t) RCNS_SerializeCback);
    RCNS_HandleAsyncMsg((uint8 *) pMsg);
  }
#endif
#if (defined FEATURE_SERIAL_BOOT || defined FEATURE_SBL)
  else if (pMsg->subSys == RPC_SYS_BOOT)
  {
    SB_HandleMsg((uint8*) pMsg);
  }
#endif
}

/**************************************************************************************************
 *
 * @fn      RTI_InitCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_InitReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_InitReq has returned.
 *
 * @param   status - Result of RTI_InitReq API call.
 *
 * @return  void
 */
void RTI_InitCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

#ifdef FEATURE_USER_STRING_PAIRING
  // Note how unlike all other RemoTI configuration items that must be configured before invoking
  // RTI_InitReq(), the user string must be configured only after receiving the init confirm.
  // The below is commented-out by default so as not to override a configuration set by the host
  // application using RTI_WriteItemEx().
  //uint8 UserString[RTI_USER_STRING_LENGTH] = "RemoTI UserStr";
  //RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_USER_STRING, RTI_USER_STRING_LENGTH, UserString);
#endif

  // RTI Init call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_INIT_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Init confirm
  NPI_SendAsynchData( &pMsg );
}

/**************************************************************************************************
 *
 * @fn      RTI_PairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_PairReq has returned.
 *
 * @param   status   - Result of RTI_PairReq API call.
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 * @param   devType  - Pairing table index device type, or invalid.
 *
 * @return  void
 */
void RTI_PairCnf( rStatus_t status, uint8 dstIndex, uint8 devType )
{
  npiMsgData_t pMsg;

  // RTI Pair call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_PAIR_CNF;
  pMsg.len      = 3;
  pMsg.pData[0] = status;
  pMsg.pData[1] = dstIndex;
  pMsg.pData[2] = devType;

  // ...and send the Pair confirm
  NPI_SendAsynchData( &pMsg );
}

/**************************************************************************************************
 *
 * @fn      RTI_PairAbortCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairAbortReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_PairAbortReq has returned.
 *
 * @param   status   - Result of RTI_PairAbortReq API call.
 *
 * @return  void
 */
void RTI_PairAbortCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI Pair call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_PAIR_ABORT_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Pair confirm
  NPI_SendAsynchData( &pMsg );
}

/**************************************************************************************************
 *
 * @fn      RTI_AllowPairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_AllowPairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_AllowPairReq has returned.
 *
 * @param   status   - Result of RTI_PairReq API call.
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 * @param   devType  - Pairing table index device type, or invalid.
 *
 * @return  void
 */
void RTI_AllowPairCnf( rStatus_t status, uint8 dstIndex, uint8 devType )
{
  npiMsgData_t pMsg;

  // RTI Allow Pair call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_ALLOW_PAIR_CNF;
  pMsg.len      = 3;
  pMsg.pData[0] = status;
  pMsg.pData[1] = dstIndex;
  pMsg.pData[2] = devType;

  // ...and send the Allow Pair confirm
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_UnpairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_UnpairReq has returned.
 *
 * @param   status   - Result of RTI_PairReq API call.
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void RTI_UnpairCnf( rStatus_t status, uint8 dstIndex )
{
  npiMsgData_t pMsg;

  // Build an asynchronous message
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_UNPAIR_CNF;
  pMsg.len      = 2;
  pMsg.pData[0] = status;
  pMsg.pData[1] = dstIndex;

  // Send the message
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_UnpairInd
 *
 * @brief   RTI indication callback initiated by receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device.
 *
 * @return  void
 */
void RTI_UnpairInd( uint8 dstIndex )
{
  npiMsgData_t pMsg;

  // Build an asynchronous message
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_UNPAIR_IND;
  pMsg.len      = 1;
  pMsg.pData[0] = dstIndex;

  // Send the message
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_SendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_SendDataReq has returned.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  void
 */
void RTI_SendDataCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI Send Data call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_SEND_DATA_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Send Data confirm
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_ReceiveDataInd
 *
 * @brief   RTI receive data indication callback asynchronously initiated by
 *          another node. The client is expected to complete this function.
 *
 * input parameters
 *
 * @param   srcIndex:  Pairing table index.
 * @param   profileId: Profile identifier.
 * @param   vendorId:  Vendor identifier.
 * @param   rxLQI:     Link Quality Indication.
 * @param   rxFlags:   Receive flags.
 * @param   len:       Number of bytes to send.
 * @param   *pData:    Pointer to data to be sent.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_ReceiveDataInd( uint8 srcIndex, uint8 profileId, uint16 vendorId, uint8 rxLQI, uint8 rxFlags, uint8 len, uint8 *pData )
{
  npiMsgData_t pMsg;

  // RTI has received data from network, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_REC_DATA_IND;
  pMsg.len      = 7+len;
  pMsg.pData[0] = srcIndex;
  pMsg.pData[1] = profileId;
  pMsg.pData[2] = (vendorId >> 0) & 0xFF;
  pMsg.pData[3] = (vendorId >> 8) & 0xFF;
  pMsg.pData[4] = rxLQI;
  pMsg.pData[5] = rxFlags;
  pMsg.pData[6] = len;

  // ...copy the received data to be sent...
  osal_memcpy( &pMsg.pData[7], pData, len );

  // ...and send the Receive Data Indication
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_StandbyCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_StandbyReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_RxEnableReq has returned.
 *
 * input parameters
 *
 * @param   status - RTI_SUCCESS
 *                   RTI_ERROR_INVALID_PARAMETER
 *                   RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 *                   RTI_ERROR_INVALID_INDEX
 *                   RTI_ERROR_UNKNOWN_STATUS_RETURNED
 *
 * output parameters
 *
 * None.
 *
 * @return  None.
 */
void RTI_StandbyCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI Standby call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_STANDBY_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Standby confirm
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_RxEnableCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_RxEnableReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_RxEnableReq has returned.
 *
 * input parameters
 *
 * @param   status - RTI_SUCCESS
 *                   RTI_ERROR_RCN_INVALID_PARAMETER
 *
 * output parameters
 *
 * None.
 *
 * @return  None.
 */
void RTI_RxEnableCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI RX Enable call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_RX_ENABLE_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the RX Enable confirm
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_EnableSleepCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_EnableSleepReq
 *          API call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_EnableSleepReq has returned.
 *
 * input parameters
 *
 * @param   status - RTI_SUCCESS
 *
 * output parameters
 *
 * None.
 *
 * @return  None.
 */
void RTI_EnableSleepCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI Enable Sleep call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_ENABLE_SLEEP_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Enable Sleep confirm
  NPI_SendAsynchData( &pMsg );
}


/**************************************************************************************************
 *
 * @fn      RTI_DisableSleepCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_DisableSleepReq
 *          API call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_DisableSleepReq has returned.
 *
 * input parameters
 *
 * @param   status - RTI_SUCCESS
 *
 * output parameters
 *
 * None.
 *
 * @return  None.
 */
void RTI_DisableSleepCnf( rStatus_t status )
{
  npiMsgData_t pMsg;

  // RTI Disable Sleep call has completed, so prep an asynchronous message...
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_DISABLE_SLEEP_CNF;
  pMsg.len      = 1;
  pMsg.pData[0] = status;

  // ...and send the Disable Sleep confirm
  NPI_SendAsynchData( &pMsg );
}

/**************************************************************************************************
 *
 * @fn      RTI_ResetInd
 *
 * @brief   RTI indication that is used to notify AP that the NP has been reset.
 *
 * @param   void
 *
 * @return  void
 */
void RTI_ResetInd( void )
{
  npiMsgData_t pMsg;

  // Build an asynchronous message
  pMsg.subSys   = RPC_SYS_RCAF;
  pMsg.cmdId    = RTIS_CMD_ID_RTI_RESET_IND;
  pMsg.len      = 0;

  // Send the message
  NPI_SendAsynchData( &pMsg );
}

/**************************************************************************************************
 **************************************************************************************************/

