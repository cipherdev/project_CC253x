/**************************************************************************************************
  Filename:       rcns.c
  Revised:        $Date: 2011-02-28 12:05:34 -0800 (Mon, 28 Feb 2011) $
  Revision:       $Revision: 25226 $

  Description:    This file contains RF4CE network layer surrogate / streaming-interface-module
                  implementation.


  Copyright 2008-2011 Texas Instruments Incorporated. All rights reserved.

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

#include "hal_types.h"
#include "hal_rpc.h"

#include "OSAL.h"

#include "rcn_nwk.h"
#include "rcn_marshal.h"
#include "rcns.h"

#include "npi.h"

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

static npiMsgData_t rcnNpiBuf;

// Confirm/indication primitive size
static const struct
{
  uint8 eventId;
  uint8 size;
} rcnCnfIndLength[] =
{
  { RCN_NLDE_DATA_CNF, sizeof(rcnNldeDataCnf_t) },
  { RCN_NLME_COMM_STATUS_IND, sizeof(rcnNlmeCommStatusInd_t) },
  { RCN_NLME_DISCOVERY_IND, sizeof(rcnNlmeDiscoveryInd_t) },
  { RCN_NLME_DISCOVERED_EVENT, sizeof(rcnNlmeDiscoveredEvent_t) },
  { RCN_NLME_DISCOVERY_CNF, sizeof(rcnNlmeDiscoveryCnf_t) },
  { RCN_NLME_PAIR_IND, sizeof(rcnNlmePairInd_t) },
  { RCN_NLME_PAIR_CNF, sizeof(rcnNlmePairCnf_t) },
  { RCN_NLME_START_CNF, sizeof(rcnNlmeStartCnf_t) },
  { RCN_NLME_UNPAIR_CNF, sizeof(rcnNlmeUnpairCnf_t) },
  { RCN_NLME_UNPAIR_IND, sizeof(rcnNlmeUnpairInd_t) },
  { RCN_NLME_AUTO_DISCOVERY_CNF, sizeof(rcnNlmeAutoDiscoveryCnf_t) },
  { RCN_NLME_DISCOVERY_ABORT_CNF, 0 },
};

#define RCN_CNF_IND_LENGTH_COUNT (sizeof(rcnCnfIndLength)/sizeof(rcnCnfIndLength[0]))

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
void RCNS_SerializeCback(rcnCbackEvent_t *pData)
{
  rcnNpiBuf.subSys   = RPC_SYS_RCN_CLIENT;

  if (pData->eventId == RCN_NLDE_DATA_IND)
  {
    // Serialization is required to remove pointer and to serialize dereferenced data
    // to the primitive.
    
    if (sizeof(rcnNldeDataIndStream_t) + pData->prim.dataInd.nsduLength <= NP_MAX_BUF_LEN)
    {
      rcnNpiBuf.len = sizeof(rcnNldeDataIndStream_t) + pData->prim.dataInd.nsduLength;
      rcnNpiBuf.cmdId = pData->eventId;
      osal_memcpy(rcnNpiBuf.pData, &pData->prim.dataInd, sizeof(rcnNldeDataIndStream_t));
      osal_memcpy(&rcnNpiBuf.pData[sizeof(rcnNldeDataIndStream_t)],
                  pData->prim.dataInd.nsdu, pData->prim.dataInd.nsduLength);
      
      NPI_SendAsynchData( &rcnNpiBuf );
    }
  }
  else
  {
    uint8 i;
    // generic processing for the rest of the events
    
    for (i = 0; i < RCN_CNF_IND_LENGTH_COUNT; i++)
    {
      if (rcnCnfIndLength[i].eventId == pData->eventId)
      {
        if (rcnCnfIndLength[i].size <= NP_MAX_BUF_LEN)
        {
          rcnNpiBuf.len = rcnCnfIndLength[i].size;
          osal_memcpy(&rcnNpiBuf.cmdId, pData, rcnCnfIndLength[i].size + 1);
          
          NPI_SendAsynchData( &rcnNpiBuf );
        }
      }
    }
  }
}


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
void RCNS_HandleAsyncMsg( uint8 *pData )
{
  static rcnNldeDataReq_t dataReq;
  rcnReqRspSerialized_t *pPrim = (rcnReqRspSerialized_t *) &pData[RPC_POS_CMD1];
  
  rcnNpiBuf.subSys   = RPC_SYS_RCN_CLIENT;
  
  switch (pPrim->primId)
  {
  case RCN_NLDE_DATA_REQ:
    // Allocate network layer buffer for NLDE-DATA.Request primitive
    osal_memcpy(&dataReq, &pPrim->prim.dataReq, sizeof(pPrim->prim.dataReq));
    if (RCN_NldeDataAlloc(&dataReq) == RCN_SUCCESS)
    {
      // memory allocation was successful
      // Copy NSDU content over to the allocated memory buffer
      osal_memcpy(dataReq.nsdu,
                  &pData[RPC_POS_DAT0 + sizeof(rcnNldeDataReqStream_t)],
                  dataReq.nsduLength);
      RCN_NldeDataReq(&dataReq);
    }
    break;
  case RCN_NLME_DISCOVERY_REQ:
    RCN_NlmeDiscoveryReq(&pPrim->prim.discoveryReq);
    break;
#ifndef FEATURE_CONTROLLER_ONLY
  case RCN_NLME_DISCOVERY_RSP:
    RCN_NlmeDiscoveryRsp(&pPrim->prim.discoveryRsp);
    break;
#endif
  case RCN_NLME_DISCOVERY_ABORT_REQ:
    RCN_NlmeDiscoveryAbortReq();
    break;
  case RCN_NLME_GET_REQ:
    {
      rcnNlmeGetCnf_t *pGetCnf;
      uint8 len = RCN_NlmeGetSizeReq(pPrim->prim.getReq.attribute);

      // Prepare serialized NLME-GET.confirm      
      rcnNpiBuf.cmdId = RCN_NLME_GET_CNF;
      rcnNpiBuf.len = sizeof(rcnNlmeGetCnf_t) + len;
      pGetCnf = (rcnNlmeGetCnf_t *) rcnNpiBuf.pData;
      pGetCnf->attribute = pPrim->prim.getReq.attribute;
      pGetCnf->attributeIndex = pPrim->prim.getReq.attributeIndex;
      pGetCnf->length = len;
  
      if (len > 0)
      {
        pGetCnf->status = RCN_NlmeGetReq(pPrim->prim.getReq.attribute,
                                         pPrim->prim.getReq.attributeIndex,
                                         (uint8 *) (pGetCnf + 1));
      }
      else
      {
        pGetCnf->status = RCN_ERROR_UNSUPPORTED_ATTRIBUTE;
      }
        
      NPI_SendAsynchData( &rcnNpiBuf );
    }
    break;
  case RCN_NLME_PAIR_REQ:
    RCN_NlmePairReq(&pPrim->prim.pairReq);
    break;
#ifndef FEATURE_CONTROLLER_ONLY
  case RCN_NLME_PAIR_RSP:
    RCN_NlmePairRsp(&pPrim->prim.pairRsp);
    break;
#endif
  case RCN_NLME_RESET_REQ:
    RCN_NlmeResetReq(pPrim->prim.resetReq.setDefaultNib);
    {
      rcnNlmeResetCnf_t *pResetCnf =
        (rcnNlmeResetCnf_t *) rcnNpiBuf.pData;
        
      rcnNpiBuf.cmdId = RCN_NLME_RESET_CNF;
      rcnNpiBuf.len = sizeof(rcnNlmeResetCnf_t);

      pResetCnf->status = RCN_SUCCESS;
      NPI_SendAsynchData( &rcnNpiBuf );
    }
    break;
  case RCN_NLME_RX_ENABLE_REQ:
    {
      rcnNlmeRxEnableCnf_t *pRxEnableCnf =
        (rcnNlmeRxEnableCnf_t *)rcnNpiBuf.pData;
        
      rcnNpiBuf.cmdId = RCN_NLME_RX_ENABLE_CNF;
      rcnNpiBuf.len = sizeof(rcnNlmeRxEnableCnf_t);
      pRxEnableCnf->status = RCN_NlmeRxEnableReq(pPrim->prim.rxEnableReq.rxOnDurationInMs);
      NPI_SendAsynchData( &rcnNpiBuf );
    }
    break;
  case RCN_NLME_SET_REQ:
    {
      rcnNlmeSetCnf_t *pSetCnf =
        (rcnNlmeSetCnf_t *)rcnNpiBuf.pData;
      
      rcnNpiBuf.len = sizeof(rcnNlmeSetCnf_t);
      rcnNpiBuf.cmdId = RCN_NLME_SET_CNF;
      
      pSetCnf->status = RCN_NlmeSetReq(pPrim->prim.setReq.nibAttribute,
                                       pPrim->prim.setReq.nibAttributeIndex,
                                       ((uint8 *)(&pPrim->prim.setReq)) + sizeof(rcnNlmeSetReq_t));
      pSetCnf->nibAttribute = pPrim->prim.setReq.nibAttribute;
      pSetCnf->nibAttributeIndex = pPrim->prim.setReq.nibAttributeIndex;
      NPI_SendAsynchData( &rcnNpiBuf );
    }
    break;
  case RCN_NLME_START_REQ:
    RCN_NlmeStartReq();
    break;
  case RCN_NLME_UNPAIR_REQ:
    RCN_NlmeUnpairReq(pPrim->prim.unpairReq.pairingRef);
    break;
  case RCN_NLME_UNPAIR_RSP:
    RCN_NlmeUnpairRsp(pPrim->prim.unpairRsp.pairingRef);
    break;
#ifndef FEATURE_CONTROLLER_ONLY
  case RCN_NLME_AUTO_DISCOVERY_REQ:
    RCN_NlmeAutoDiscoveryReq(&pPrim->prim.autoDiscoveryReq);
    break;
  case RCN_NLME_AUTO_DISCOVERY_ABORT_REQ:
    RCN_NlmeAutoDiscoveryAbortReq();
    break;
#endif
  default:
    break;
  }
}

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
uint8 RCNS_HandleSyncMsg( uint8 *pData )
{
  rcnReqRspSerialized_t *pPrim = (rcnReqRspSerialized_t *) &pData[RPC_POS_CMD1];
  uint8 result = FALSE;
  
  switch (pPrim->primId)
  {
  case RCN_NLME_GET_REQ:
    {
      rcnNlmeGetCnf_t *pGetCnf = (rcnNlmeGetCnf_t *) &pData[RPC_POS_DAT0];
      uint8 len = RCN_NlmeGetSizeReq(pPrim->prim.getReq.attribute);
      
      {
        uint8 attribute = pPrim->prim.getReq.attribute;
        uint8 attributeIndex = pPrim->prim.getReq.attributeIndex;
      
        pGetCnf->attribute = attribute;
        pGetCnf->attributeIndex = attributeIndex;
      }
      
      pGetCnf->length = len;

      pData[RPC_POS_LEN] = sizeof(rcnNlmeGetCnf_t) + len;
      pData[RPC_POS_CMD0] = RPC_SYS_RCN_CLIENT;
      pData[RPC_POS_CMD1] = RCN_NLME_GET_CNF;

      if (len > 0)
      {
        // has to use pGetCnf attribute and attribute index since
        // the pData buffer is already overwritten
        pGetCnf->status = RCN_NlmeGetReq(pGetCnf->attribute,
                                         pGetCnf->attributeIndex,
                                         (uint8 *) (pGetCnf + 1));
      }
      else
      {
        pGetCnf->status = RCN_ERROR_UNSUPPORTED_ATTRIBUTE;
      }
    }
    break;
  case RCN_NLME_RESET_REQ:
    RCN_NlmeResetReq(pPrim->prim.resetReq.setDefaultNib);
    {
      rcnNlmeResetCnf_t *pResetCnf =
        (rcnNlmeResetCnf_t *) &pData[RPC_POS_DAT0];
        
      pData[RPC_POS_LEN] = sizeof(rcnNlmeResetCnf_t);
      pData[RPC_POS_CMD0] = RPC_SYS_RCN_CLIENT;
      pData[RPC_POS_CMD1] = RCN_NLME_RESET_CNF;
        
      pResetCnf->status = RCN_SUCCESS;
    }
    result = TRUE;
    break;
  case RCN_NLME_RX_ENABLE_REQ:
    {
      rcnNlmeRxEnableCnf_t *pRxEnableCnf =
        (rcnNlmeRxEnableCnf_t *)&pData[RPC_POS_DAT0];
      uint32 rxOnDuration = pPrim->prim.rxEnableReq.rxOnDurationInMs;
      
      pData[RPC_POS_LEN] = sizeof(rcnNlmeRxEnableCnf_t);
      pData[RPC_POS_CMD0] = RPC_SYS_RCN_CLIENT;
      pData[RPC_POS_CMD1] = RCN_NLME_RX_ENABLE_CNF;
        
      pRxEnableCnf->status = RCN_NlmeRxEnableReq(rxOnDuration);
    }
    break;
  case RCN_NLME_SET_REQ:
    {
      rcnNlmeSetCnf_t *pSetCnf =
        (rcnNlmeSetCnf_t *)&pData[RPC_POS_DAT0];
      uint8 attribute = pPrim->prim.setReq.nibAttribute;
      uint8 attributeIndex = pPrim->prim.setReq.nibAttributeIndex;
      uint8 status = RCN_NlmeSetReq(attribute,
                                     attributeIndex,
                                     ((uint8 *)(&pPrim->prim.setReq)) + sizeof(rcnNlmeSetReq_t));

      pData[RPC_POS_LEN] = sizeof(rcnNlmeSetCnf_t);
      pData[RPC_POS_CMD0] = RPC_SYS_RCN_CLIENT;
      pData[RPC_POS_CMD1] = RCN_NLME_SET_CNF;
      pSetCnf->status = status;
      pSetCnf->nibAttribute = attribute;
      pSetCnf->nibAttributeIndex = attributeIndex;
    }
    break;
  default:
    break;
  }
  
  return result;
}

/**************************************************************************************************
 **************************************************************************************************/
