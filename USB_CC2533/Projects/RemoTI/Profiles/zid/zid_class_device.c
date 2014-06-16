/**************************************************************************************************
  Filename:       zid_class_device.c
  Revised:        $Date: 2012-11-07 14:24:46 -0800 (Wed, 07 Nov 2012) $
  Revision:       $Revision: 32120 $

  Description:

  This file contains the definitions for a ZID Profile HID Class Device.


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
#include "gdp_profile.h"
#include "OSAL.h"
#include "osal_snv.h"
#include "rcn_nwk.h"
#include <stddef.h>
#include "zid_class_device.h"
#include "zid_common.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#if !defined ZID_CLD_NVID_BEG
#define ZID_CLD_NVID_BEG  (ZID_COMMON_NVID_END + 1)
#endif

#define ZID_CLD_NVID_ATTR_CFG             (ZID_CLD_NVID_BEG + 0)
#define ZID_CLD_NVID_DESC_BEG             (ZID_CLD_NVID_BEG + 1)
#define ZID_CLD_NVID_DESC_END             (ZID_CLD_NVID_DESC_BEG + (aplcMaxNonStdDescCompsPerHID * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC) - 1)
#define ZID_CLD_NVID_DESC_START(descNum)  (ZID_CLD_NVID_DESC_BEG + ((descNum) * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC))
#define ZID_CLD_NVID_NULL_REPORT_BEG      (ZID_CLD_NVID_DESC_END + 1)
#define ZID_CLD_NVID_NULL_REPORT_END      (ZID_CLD_NVID_NULL_REPORT_BEG + aplcMaxNonStdDescCompsPerHID - 1)

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

typedef enum {
  eCldDor,     // ZID Dormant state.
  eCldCfgGet,  // Attempting to get the Adapter capabilities.
  eCldCfgPxy,  // Attempting to push the proxy table entry configuration.
  eCldCfgExt,  // Attempting to push the non-standard descriptors (optional).
  eCldCfgXmitNonStdDescCompFrags, // pushing non-std descriptor fragments (optional).
  eCldCfgNullReports, // sending set report command frames for any NULL reports (optional).
  eCldCfgComplete,  // Sending the config complete.
  eCldCfgRdy,  // Attempting to end the configuration state with GDP_CMD_CFG_COMPLETE.
  eCldUnpair,  // Awaiting response from RTI_UnpairReq().
  eCldRdy,     // Paired and configured, so ZID Idle or Active state.

  eCldErr
} cldState_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

// Called from zid ProcessEvent().
static void rspWaitTimeout(void);

// Called from zidCld_ReceiveDataInd().
static uint8 rxHandshakeRsp(uint8 srcIndex, uint8 rspCode);
static void rxGetAttr(uint8 srcIndex, uint8 len, uint8 *pData);
static uint8 rxGetSetReport(uint8 srcIndex, uint8 len, uint8 *pData);

// Called from any of the local functionality.
static void sendCldDataReq(uint8 dstIndex, uint8 len, uint8 *pData);
static void pushConfig(void);
static void pullProxy(void);
static void pushProxy(void);
static void pushNonStdDescComp(void);
static uint8 procGetCldAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf);
static void unpairReq(uint8 dstIndex);
static void pushNullReport( void );
static uint8 getCldAttrTableIdx( uint8 attrId );

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

static cldState_t cldState;
static zid_cld_cfg_t cldConfigData;
static uint8 numNonStdDescCompFrags;
static uint8 currentNonStdDescCompNum;
static uint8 currentNonStdDescCompHdrSize;
static uint8 currentNonStdDescCompFragNum;
static uint16 currentNonStdDescCompLen;
static uint8 currentNullReportNum;
static uint8 zidCldCurrentNonStdDescNum;
static uint8 zidCldCurrentNonStdDescFragNum;
static const zid_table_t zidCld_attributeTable[] =
{
  {
    aplZIDProfileVersion,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, ZIDProfileVersion)
  },
  {
    aplIntPipeUnsafeTxWindowTime,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, IntPipeUnsafeTxWindowTime)
  },
  {
    aplHIDParserVersion,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, HIDParserVersion)
  },
  {
    aplHIDDeviceReleaseNumber,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, HIDDeviceReleaseNumber)
  },
  {
    aplHIDVendorId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, HIDVendorId)
  },
  {
    aplHIDProductId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_cld_cfg_t, HIDProductId)
  },
  {
    aplReportRepeatInterval,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, ReportRepeatInterval)
  },
  {
    aplHIDDeviceSubclass,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDDeviceSubclass)
  },
  {
    aplHIDProtocolCode,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDProtocolCode)
  },
  {
    aplHIDCountryCode,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDCountryCode)
  },
  {
    aplHIDNumEndpoints,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDNumEndpoints)
  },
  {
    aplHIDPollInterval,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDPollInterval)
  },
  {
    aplHIDNumStdDescComps,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDNumStdDescComps)
  },
  {
    aplHIDStdDescCompsList,
    aplcMaxStdDescCompsPerHID,
    offsetof(zid_cld_cfg_t, HIDStdDescCompsList)
  },
  {
    aplHIDNumNullReports,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDNumNullReports)
  },
  {
    aplHIDNumNonStdDescComps,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_cld_cfg_t, HIDNumNonStdDescComps)
  }
};
#define ZID_CLD_ATTRIBUTE_TABLE_LEN  (sizeof(zidCld_attributeTable) / sizeof(zidCld_attributeTable[0]))

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

uint8 zidCld_TaskId;

#ifdef ZID_IOT
uint8 mouseTxOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
#endif

/**************************************************************************************************
 * @fn          zidCld_Init
 *
 * @brief       This is the RemoTI task initialization called by OSAL.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidCld_Init(uint8 taskId)
{
  zidCld_TaskId = taskId;
}

/**************************************************************************************************
 * @fn          zidCld_InitItems
 *
 * @brief       This API is used to initialize the ZID configuration items to default values.
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
void zidCld_InitItems(void)
{
  if (osal_snv_read(ZID_CLD_NVID_ATTR_CFG, ZID_CLD_CFG_T_SIZE, &cldConfigData) != SUCCESS)
  {
    cldConfigData.IntPipeUnsafeTxWindowTime = aplcMinIntPipeUnsafeTxWindowTime;
    cldConfigData.ReportRepeatInterval = aplcMaxReportRepeatInterval / 2;
    cldConfigData.HIDParserVersion = 0x0111;         // bcdHID V1.11
    cldConfigData.HIDDeviceSubclass = 0x0;
    cldConfigData.HIDProtocolCode = 0x0;
    cldConfigData.HIDCountryCode = 0x0;
    cldConfigData.HIDDeviceReleaseNumber = 0x0100;   // bcdDevice V1.0
    cldConfigData.HIDVendorId = 0x0451;              // Texas Instruments Vendor Id.
    cldConfigData.HIDProductId = 0x16C2;             // CC2531 ZID Dongle.
    cldConfigData.HIDNumEndpoints = 2;
    cldConfigData.HIDPollInterval = 1;               // (2 ^ (HIDPollInterval-1)) X 125 usec.
    cldConfigData.HIDNumNonStdDescComps = 0;
    cldConfigData.HIDNumStdDescComps = 0;
    cldConfigData.HIDNumNullReports = 0;
    (void)osal_memset(cldConfigData.HIDStdDescCompsList, ZID_STD_REPORT_NONE, aplcMaxStdDescCompsPerHID);
  }

  // Since the Profile Version is not writable, always update in case of an OAD.
  cldConfigData.ZIDProfileVersion = ZID_PROFILE_VERSION;
  (void)osal_snv_write(ZID_CLD_NVID_ATTR_CFG, ZID_CLD_CFG_T_SIZE, &cldConfigData);
  zidUnsafeTime = cldConfigData.IntPipeUnsafeTxWindowTime;

  if (osal_snv_read(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo) != SUCCESS)
  {
    (void)osal_memset(zidPairInfo.cfgCompleteDisc, 0, ZID_PAIR_DISC_CNT);
    (void)osal_memset(zidPairInfo.adapterDisc, 0, ZID_PAIR_DISC_CNT);

    (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);
  }

  zidCnfIdx = zidCfgIdx = zidRspIdx = RTI_INVALID_PAIRING_REF;
}

void zidCld_SetStateActive( void )
{
  cldState = eCldRdy;
}

/**************************************************************************************************
 * @fn          zidCld_ProcessEvent
 *
 * @brief       This routine handles ZID task events.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *              events - Event flags to be processed by this task.
 *
 * output parameters
 *
 * None.
 *
 * @return      16bit - Unprocessed event flags.
 */
uint16 zidCld_ProcessEvent(uint8 taskId, uint16 events)
{
  (void)taskId;

  if (events & ZID_CLD_EVT_CFG)
  {
    pullProxy();
  }

  if (events & ZID_CLD_EVT_SAFE_TX)
  {
    ZID_CLR_UNSAFE();
  }

  if (events & ZID_EVT_RSP_WAIT)
  {
    rspWaitTimeout();
  }

  return 0;  // All events processed in one pass; discard unexpected events.
}

/**************************************************************************************************
 * @fn          zidCld_SendDataReq
 *
 * @brief       This function sets timers, Rx on, and Tx Options according to requisite behavior.
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 * @param       txOptions - Bit mask according to the ZID_TX_OPTION_... definitions.
 * @param       *cmd      - A pointer to the first byte of the ZID data packet (the ZID Command).
 *
 * output parameters
 *
 * None.
 *
 * @return      txOptions, modified as necessary for the ZID transaction being requested.
 */
uint8 zidCld_SendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd)
{
  (void)dstIndex;

  switch (*cmd & GDP_HEADER_CMD_CODE_MASK)
  {
  case GDP_CMD_GET_ATTR:
  case GDP_CMD_PUSH_ATTR:
  case ZID_CMD_SET_REPORT:
  case GDP_CMD_CFG_COMPLETE:
    ZID_SET_RSPING();  // Set status bit for action on send data confirm.
    zidRspIdx = dstIndex;
    // Set here to ensure Tx transitions directly to RxOn with no gap in Rx.
    RCN_NlmeRxEnableReq(aplcMaxRxOnWaitTime);
    break;

  case GDP_CMD_HEARTBEAT:
    zidRspWait(zidCld_TaskId, aplcMaxRxOnWaitTime);
    // Set here to ensure Tx transitions directly to RxOn with no gap in Rx.
    RCN_NlmeRxEnableReq(aplcMaxRxOnWaitTime);
    break;

  default:
    break;
  }

#ifdef ZID_IOT
  txOptions = mouseTxOptions;
#endif

  ZID_SET_CNFING();
  if ((txOptions & RTI_TX_OPTION_SINGLE_CHANNEL) && (!(txOptions & RTI_TX_OPTION_BROADCAST))) // If using Interrupt Pipe transmission model.
  {
    if (ZID_IS_UNSAFE)
    {
      ZID_CLR_CNFING();
      txOptions &= (0xFF ^ RTI_TX_OPTION_BROADCAST);  // Broadcast not allowed with interrupt pipe.
    }
    else  // A safe transmission must be made before allowing anymore unsafe by Interrupt Pipe.
    {
      txOptions &= (0xFF ^ RTI_TX_OPTION_SINGLE_CHANNEL);
      txOptions |= RTI_TX_OPTION_ACKNOWLEDGED;
    }
  }

  return txOptions;
}

/**************************************************************************************************
 * @fn      zidCld_ReceiveDataInd
 *
 * @brief   RTI receive data indication callback asynchronously initiated by
 *          another node. The client is expected to complete this function.
 *
 * input parameters
 *
 * @param   srcIndex -  Pairing table index of sender.
 * @param   rxFlags:   Receive flags.
 * @param   len     - Number of data bytes.
 * @param   *pData  - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return  TRUE if handled here. Otherwise FALSE indicating to pass the data to Application layer.
 */
uint8 zidCld_ReceiveDataInd(uint8 srcIndex, uint8 rxFlags, uint8 len, uint8 *pData)
{
  uint8 cmd = pData[ZID_FRAME_CTL_IDX] & GDP_HEADER_CMD_CODE_MASK;
  uint8 rxOn = pData[ZID_FRAME_CTL_IDX] & GDP_HEADER_DATA_PENDING;
  uint8 rtrn = TRUE;
  (void)rxFlags;

  switch (cmd)
  {
  case GDP_CMD_GENERIC_RSP:
    rtrn = rxHandshakeRsp(srcIndex, pData[ZID_DATA_BUF_IDX]);
    break;

  case GDP_CMD_GET_ATTR:
    rxGetAttr(srcIndex, len, pData);
    break;

  case GDP_CMD_GET_ATTR_RSP:
    if ((srcIndex == zidCfgIdx) && (cldState == eCldCfgGet))
    {
      pushConfig();
    }
    else
    {
      rtrn = FALSE;
    }
    break;

  case ZID_CMD_GET_REPORT:  // Error check and Handshake fail here; otherwise pass to Application.
  case ZID_CMD_SET_REPORT:  // Error check and Handshake here; on success, pass to Application.
    if (GET_BIT(zidPairInfo.cfgCompleteDisc, srcIndex))
    {
      rtrn = rxGetSetReport(srcIndex, len, pData);
    }
    break;

  case ZID_CMD_REPORT_DATA: // Allow data indication to pass up to Application layer - no response.
    rtrn = FALSE;
    break;

  default:
    rxOn = FALSE;
    break;
  }

  if (rxOn)
  {
    zidRspWait(zidCld_TaskId, aplcMaxRxOnWaitTime);
  }

  // ZID specifies to not accept messages from unconfigured devices; if so, do not pass to App.
  if ((rtrn == FALSE) && GET_BIT(zidPairInfo.cfgCompleteDisc, srcIndex))
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

/**************************************************************************************************
 * @fn      zidCld_PairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairReq API call.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void zidCld_PairCnf(uint8 dstIndex)
{
  rcnNwkPairingEntry_t* pEntry;

  // An advanced application may allow pairing with non-ZID (i.e. CERC profile), so check for ZID.
  if ((RCN_NlmeGetPairingEntryReq(dstIndex, &pEntry) == RCN_SUCCESS) &&  // Fail not expected here.
       GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
  {
    zidCfgIdx = dstIndex;
    cldState = eCldCfgGet;
    CLR_BIT(zidPairInfo.adapterDisc, dstIndex);
    CLR_BIT(zidPairInfo.cfgCompleteDisc, dstIndex);
    (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);

//    if (osal_start_timerEx(zidCld_TaskId, ZID_CLD_EVT_CFG, aplcConfigBlackoutTime) != SUCCESS)
//    {
    // Blackout period is handled by the previously configured profile
      (void)osal_set_event(zidCld_TaskId, ZID_CLD_EVT_CFG);
//    }
  }
  else
  {
    osal_set_event( RTI_TaskId, GDP_EVT_CONFIGURATION_FAILED);
  }
}

/**************************************************************************************************
 *
 * @fn      zidCld_Unpair
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API call or by
 *          receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  TRUE if ZID initiated the unpair; FALSE otherwise.
 */
uint8 zidCld_Unpair(uint8 dstIndex)
{
  uint8 rtrn = (cldState == eCldUnpair) ? TRUE : FALSE;

  if (zidCfgIdx == dstIndex)
  {
    zidRspDone(zidCld_TaskId, TRUE);
    cldState = eCldDor;
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn          zidCld_ReadItem
 *
 * @brief       This API is used to read the ZID Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table.
 *
 * input parameters
 *
 * @param       itemId - The Configuration Interface item identifier.
 *                       See special note below for 0xF1-0xFF non-standard descriptors.
 * @param       len    - The length in bytes of the item's data.
 *
 * output parameters
 *
 * @param       *pValue - A pointer to the item identifier's data from table.
 *
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidCld_ReadItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  if (itemId == ZID_ITEM_NON_STD_DESC_FRAG)  // If a non-standard HID descriptor.
  {
    // specCheckWrite() insures valid itemId which will result in a good NV Id.
    itemId = ZID_CLD_NVID_DESC_BEG + (zidCldCurrentNonStdDescNum * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC) + zidCldCurrentNonStdDescFragNum;
    if (osal_snv_read(itemId, len, pValue) != SUCCESS)
    {
      status = RTI_ERROR_OSAL_NV_OPER_FAILED;
    }
  }
  else if (itemId == ZID_ITEM_NULL_REPORT)
  {
    itemId = ZID_CLD_NVID_NULL_REPORT_BEG + zidCldCurrentNonStdDescNum;
    if (osal_snv_read(itemId, len, pValue) != SUCCESS)
    {
      status = RTI_ERROR_OSAL_NV_OPER_FAILED;
    }
  }
  else if ((itemId >= ZID_ITEM_INT_PIPE_UNSAFE_TX_TIME) && (itemId <= ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS))
  {
    /* Cache config in RAM and ensure ZID profile version is correct */
    uint8 idx = getCldAttrTableIdx( itemId );
    if (idx == ZID_TABLE_IDX_INVALID)
    {
      status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
    }
    else
    {
      (void)osal_memcpy( pValue, (uint8 *)&cldConfigData + zidCld_attributeTable[idx].offset, len );
    }
  }
  else
  {
    status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
  }

  return status;
}

/**************************************************************************************************
 * @fn          zidCld_WriteItem
 *
 * @brief       This API is used to write ZID Configuration Interface parameters
 *              to the Configuration Parameters table, and permitted attributes
 *              to the State Attributes table.
 *
 * input parameters
 *
 * @param       itemId:  The Configuration Interface item identifier.
 *                       See special note below for 0xF1-0xFF non-standard descriptors.
 * @param       len:     The length in bytes of the item identifier's data.
 * @param       *pValue: A pointer to the item's data.
 *
 * input parameters
 *
 * None.
 *
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidCld_WriteItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  /* Validate the item before writing it */
  if (zidCommon_specCheckWrite(itemId, len, pValue) == FALSE)
  {
    status = RTI_ERROR_INVALID_PARAMETER;
  }
  else if (itemId == ZID_ITEM_NON_STD_DESC_NUM)
  {
    if (*pValue >= aplcMaxNonStdDescCompsPerHID)
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      zidCldCurrentNonStdDescNum = *pValue;
    }
  }
  else if (itemId == ZID_ITEM_NON_STD_DESC_FRAG_NUM)
  {
    if (*pValue >= ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC)
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      zidCldCurrentNonStdDescFragNum = *pValue;
    }
  }
  else if (itemId == ZID_ITEM_NON_STD_DESC_FRAG)  // If a non-standard HID descriptor.
  {
    // specCheckWrite() insures valid itemId which will result in a good NV Id.
    itemId = ZID_CLD_NVID_DESC_BEG + (zidCldCurrentNonStdDescNum * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC) + zidCldCurrentNonStdDescFragNum;
    if (osal_snv_write(itemId, len, pValue) != SUCCESS)
    {
      status = RTI_ERROR_OSAL_NV_OPER_FAILED;
    }
  }
  else if (itemId == ZID_ITEM_NULL_REPORT)
  {
    itemId = ZID_CLD_NVID_NULL_REPORT_BEG + zidCldCurrentNonStdDescNum;
    if (osal_snv_write(itemId, len, pValue) != SUCCESS)
    {
      status = RTI_ERROR_OSAL_NV_OPER_FAILED;
    }
  }
  else if ((itemId >= ZID_ITEM_INT_PIPE_UNSAFE_TX_TIME) && (itemId <= ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS))
  {
    /* Cache config in RAM and ensure ZID profile version is correct */
    uint8 idx = getCldAttrTableIdx( itemId );
    if (idx == ZID_TABLE_IDX_INVALID)
    {
      status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
    }
    else
    {
      (void)osal_memcpy( (uint8 *)&cldConfigData + zidCld_attributeTable[idx].offset, pValue, len );
      if (osal_snv_write(ZID_CLD_NVID_ATTR_CFG, ZID_CLD_CFG_T_SIZE, &cldConfigData) != SUCCESS)
      {
        status = RTI_ERROR_OSAL_NV_OPER_FAILED;
      }
    }
  }
  else
  {
    status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
  }

  return status;
}

/**************************************************************************************************
 * @fn          rspWaitTimeout
 *
 * @brief       Act on the expiration of a wait for response.
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
static void rspWaitTimeout(void)
{
  switch (cldState)
  {
    case eCldCfgGet:  // Attempting to get the Adapter capabilities.
    case eCldCfgPxy:  // Attempting to push the proxy table entry configuration.
    case eCldCfgExt:  // Attempting to push the non-standard descriptors (optional).
    case eCldCfgNullReports: // sending set report command frames for any NULL reports (optional).
    case eCldCfgComplete:  // Attempting to signal end of config phase.
    case eCldCfgRdy:  // Attempting to end the configuration state with GDP_CMD_CFG_COMPLETE.
    {
      unpairReq(zidCfgIdx);
      break;
    }

    default:
    {
      break;
    }
  }
}

/**************************************************************************************************
 * @fn          rxHandshakeRsp
 *
 * @brief       Act on the receipt of a Handshake response command frame.
 *
 * input parameters
 *
 * @param       srcIndex - Pairing table index.
 * @param       rspCode  - The response code field of the handshake command frame.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if handled here; otherwise FALSE to pass up to the Application.
 */
static uint8 rxHandshakeRsp(uint8 srcIndex, uint8 rspCode)
{
  uint8 rtrn = FALSE;

  switch (cldState)
  {
    case eCldCfgPxy:  // Attempting to push the proxy table entry configuration.
    case eCldCfgExt:  // Attempting to push the non-standard descriptors (optional).
    case eCldCfgXmitNonStdDescCompFrags: // pushing non-std descriptor fragments (optional).
    case eCldCfgNullReports: // sending set report command frames for any NULL reports (optional).
    case eCldCfgComplete:  // Attempting to signal the end of the config phase.
    case eCldCfgRdy:  // Attempting to end the configuration state with GDP_CMD_CFG_COMPLETE.
    {
      if (srcIndex == zidCfgIdx)
      {
        // Target does not support all of this node's configured ZID configuration, so unpair.
        if (rspCode != GDP_GENERIC_RSP_SUCCESS)
        {
          unpairReq(srcIndex);
        }
        else
        {
          pushConfig();
        }
      }
      rtrn = TRUE;
      break;
    }

    default:
    {
      break;
    }
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn          rxGetAttr
 *
 * @brief       Process a Get Attributes command frame.
 *
 * input parameters
 *
 * @param   srcIndex -  Pairing table index.
 * @param   len      - Number of data bytes.
 * @param   *pData   - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rxGetAttr(uint8 srcIndex, uint8 len, uint8 *pData)
{
  uint8 *pBuf;

  if (len < 2)
  {
    return;
  }
  len--;
  pData++;

  if ((len == 1) && (*pData >= aplHIDNonStdDescCompSpec1))  // If a valid DescSpec request.
  {
    // Memory for exactly one DescSpec response.
    pBuf = osal_mem_alloc(GDP_ATTR_RSP_SIZE_HDR + ZID_NON_STD_DESC_SPEC_T_MAX + 1);
  }
  else
  {
    // Memory for responding with potentially all other possible descriptors.
    pBuf = osal_mem_alloc(GDP_ATTR_RSP_SIZE_HDR * len + sizeof(zid_cld_cfg_t) + 1);
  }

  if (pBuf != NULL)
  {
    *pBuf = GDP_CMD_GET_ATTR_RSP;
    len = procGetCldAttr(len, pData, pBuf);
    sendCldDataReq(srcIndex, len, pBuf);
    osal_mem_free(pBuf);
  }
}

/**************************************************************************************************
 * @fn          rxGetSetReport
 *
 * @brief       Process a Get/Set Report command frame.
 *
 * input parameters
 *
 * @param   srcIndex -  Pairing table index.
 * @param   len      - Number of data bytes.
 * @param   *pData   - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if a response is made here (i.e. Handshake with eZidHandShakeInvalidRepId or
 *              GDP_GENERIC_RSP_INVALID_PARAM or Handshake eZidHandShakeSuccess on Set Report) and the
 *              Report Command should not be passed up to Application layer.
 *              Otherwise return FALSE to pass a valid Report Command up to the Application layer.
 */
static uint8 rxGetSetReport(uint8 srcIndex, uint8 len, uint8 *pData)
{
  uint8 rsp = ZID_GENERIC_RSP_INVALID_REPORT_ID;
  bool sendGenericRsp = TRUE;
  zid_get_report_cmd_t *pRep = (zid_get_report_cmd_t *)pData;
  pRep->cmd &= GDP_HEADER_CMD_CODE_MASK;  // No use for data pending bit here.

  if ((pRep->type < ZID_REPORT_TYPE_IN) || (pRep->type > ZID_REPORT_TYPE_FEATURE))
  {
    rsp = GDP_GENERIC_RSP_INVALID_PARAM;
  }
  else if (pRep->id > ZID_STD_REPORT_TOTAL_NUM)
  {
    uint8 buf[sizeof(zid_non_std_desc_comp_t)];
    uint8 cnt;
    zid_non_std_desc_comp_t *pDesc = (zid_non_std_desc_comp_t *)buf;

    // initialize to report not found
    for (cnt = 0; cnt < cldConfigData.HIDNumNonStdDescComps; cnt++)
    {
      uint8 id = ZID_CLD_NVID_DESC_START(cnt);
      (void)osal_snv_read(id, sizeof(zid_non_std_desc_comp_t), buf);  // Fail not expected.

      if ((pDesc->type == ZID_DESC_TYPE_REPORT) && (pDesc->reportId == pRep->id))
      {
        uint16 size = (pDesc->size_h << 8) | pDesc->size_l;
        if ((pRep->cmd == ZID_CMD_SET_REPORT) && (size != (len - sizeof(zid_report_data_cmd_t))))
        {
          rsp = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          sendGenericRsp = FALSE;
        }

        break;
      }
    }

    // If report not found, set status accordingly
    if (cnt == cldConfigData.HIDNumNonStdDescComps)
    {
      rsp = ZID_GENERIC_RSP_INVALID_REPORT_ID;
    }
  }
  else
  {
    uint8 idx;

    for (idx = 0; idx < cldConfigData.HIDNumStdDescComps; idx++)
    {
      if (cldConfigData.HIDStdDescCompsList[idx] == pRep->id)
      {
        if ((pRep->cmd == ZID_CMD_SET_REPORT) &&
            (ZID_StdReportLen[pRep->id] != (len - sizeof(zid_set_report_cmd_t))))
        {
          rsp = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          sendGenericRsp = FALSE;
        }

        break;
      }
    }

    // If report not found, set status accordingly
    if (idx == cldConfigData.HIDNumStdDescComps)
    {
      rsp = ZID_GENERIC_RSP_INVALID_REPORT_ID;
    }
  }

  if (sendGenericRsp == FALSE)
  {
    if (!FEATURE_ZID_SET_REPORT && (pRep->cmd == ZID_CMD_SET_REPORT))
    {
      rsp = GDP_GENERIC_RSP_UNSUPPORTED_REQ;
    }
    else
    {
      return FALSE;  // Return FALSE to pass the validity-checked Get Report up to Application.
    }
  }

  uint8 buf[2] = { GDP_CMD_GENERIC_RSP, (uint8)rsp };
  sendCldDataReq(srcIndex, 2, buf);

  return TRUE;  // Return TRUE so Application is not bothered with an invalid/answered command.
}

/**************************************************************************************************
 * @fn          sendCldDataReq
 *
 * @brief       Construct and send an RTI_SendDataReq() using the input parameters.
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 * @param       len       - Number of bytes to send.
 * @param       *pData    - Pointer to data to be sent.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void sendCldDataReq(uint8 dstIndex, uint8 len, uint8 *pData)
{
  uint16 vendorId = RTI_VENDOR_TEXAS_INSTRUMENTS;
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;

  (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_ID, sizeof(uint16), (uint8 *)&vendorId);
  ZID_SET_TXING();
  // ZID_SendDataReq(), called from RTI_SendDataReq(), enables the Rx on interval and
  // sets the ZID_HID_RX_CLD_MASK bit.
  RTI_SendDataReq(dstIndex, RTI_PROFILE_ZID, vendorId, txOptions, len, pData);
}

/**************************************************************************************************
 * @fn          pullProxy
 *
 * @brief       Pull the standard proxy table parameters supported by the Adapter.
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
static void pullProxy(void)
{
  uint8 buf[7] = { GDP_CMD_GET_ATTR,
                   aplZIDProfileVersion, aplHIDParserVersion, aplHIDCountryCode,
                   aplHIDDeviceReleaseNumber, aplHIDVendorId, aplHIDProductId };
  sendCldDataReq(zidCfgIdx, 7, buf);
}

/**************************************************************************************************
 * @fn          pushConfig
 *
 * @brief       Configuration push state manager invoked to begin a new configuration phase or
 *              upon a successful handshake to a configuration phase underway.
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
static void pushConfig(void)
{
  uint8 loop;  // Loop instead of making recursive calls for call-stack depth.

  do
  {
    loop = FALSE;

    switch (cldState)
    {
      case eCldCfgGet:
      {
        cldState = eCldCfgPxy;
        pushProxy();
        break;
      }

      case eCldCfgPxy:
      {
        cldState = eCldCfgExt;
        currentNonStdDescCompNum = 0;
        loop = TRUE;
        break;
      }

      case eCldCfgExt:
      {
        if (currentNonStdDescCompNum == cldConfigData.HIDNumNonStdDescComps)
        {
          cldState = eCldCfgNullReports;
          currentNullReportNum = 0;
          loop = TRUE;
        }
        else
        {
          zid_non_std_desc_comp_t header;

          /* Read the 1st fragment of the non-std descriptor spec to get length
           * and determine number of fragments we need to push.
           */
          (void)osal_snv_read( ZID_CLD_NVID_DESC_START(currentNonStdDescCompNum),
                               sizeof(zid_non_std_desc_comp_t),
                               &header );  // Fail not expected.
          currentNonStdDescCompLen = (header.size_h << 8) | header.size_l;
          numNonStdDescCompFrags = ((currentNonStdDescCompLen + aplcMaxNonStdDescFragmentSize - 1) / aplcMaxNonStdDescFragmentSize);
          if (numNonStdDescCompFrags == 1)
          {
            currentNonStdDescCompHdrSize = sizeof(zid_non_std_desc_comp_t);
          }
          else
          {
            currentNonStdDescCompHdrSize = sizeof(zid_frag_non_std_desc_comp_t);
          }

          /* Now push 1st fragment */
          currentNonStdDescCompFragNum = 0;
          pushNonStdDescComp();
          currentNonStdDescCompFragNum++;
          if (currentNonStdDescCompFragNum == numNonStdDescCompFrags)
          {
            currentNonStdDescCompNum++;
          }
          else
          {
            currentNonStdDescCompLen -= aplcMaxNonStdDescFragmentSize;
            cldState = eCldCfgXmitNonStdDescCompFrags;
          }
        }
        break;
      }

      case eCldCfgXmitNonStdDescCompFrags:
      {
        if (currentNonStdDescCompFragNum == numNonStdDescCompFrags)
        {
          currentNonStdDescCompNum++;
          cldState = eCldCfgExt;
          loop = TRUE;
        }
        else
        {
          pushNonStdDescComp();
          currentNonStdDescCompLen -= aplcMaxNonStdDescFragmentSize;
          currentNonStdDescCompFragNum++;
        }
        break;
      }

      case eCldCfgNullReports:
      {
        if (currentNullReportNum == cldConfigData.HIDNumNullReports)
        {
          cldState = eCldCfgComplete;
          loop = TRUE;
        }
        else
        {
          pushNullReport();
          currentNullReportNum++;
        }
        break;
      }

      case eCldCfgComplete:
      {
        uint8 cfgComplete[2] = { GDP_CMD_CFG_COMPLETE, GDP_GENERIC_RSP_SUCCESS };
        cldState = eCldCfgRdy;
        sendCldDataReq(zidCfgIdx, 2, cfgComplete);
        // invoke next configuration
        osal_start_timerEx( RTI_TaskId, GDP_EVT_CONFIGURE_NEXT, aplcConfigBlackoutTime );
        break;
      }

      case eCldCfgRdy:
      {
        cldState = eCldRdy;
        SET_BIT(zidPairInfo.cfgCompleteDisc, zidCfgIdx);
        (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);
        zidCfgIdx = RTI_INVALID_PAIRING_REF;
        break;
      }

      default:
      {
        break;
      }
    }
  } while (loop);
}

/**************************************************************************************************
 * @fn          pushProxy
 *
 * @brief       Transmit/push the standard proxy table entry information with the standard
 *              descriptor list.
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
static void pushProxy(void)
{
  uint8 cnt = 0, *pBuf;
  uint8 req[] = {
    aplHIDParserVersion,
    aplHIDDeviceSubclass,
    aplHIDProtocolCode,
    aplHIDCountryCode,
    aplHIDDeviceReleaseNumber,
    aplHIDVendorId,
    aplHIDProductId,
    aplHIDNumEndpoints,
    aplHIDPollInterval,
    aplHIDNumNonStdDescComps,
    aplHIDNumStdDescComps,
    aplHIDNumNullReports,
    aplHIDStdDescCompsList
  };

  // Memory for all response headers + response data + msg command byte.
  if ((pBuf = osal_mem_alloc(ZID_PROXY_ENTRY_T_ITEMS * GDP_PUSH_ATTR_REC_SIZE_HDR +
                             ZID_PROXY_ENTRY_T_SIZE + 1)) == NULL)
  {
    return;
  }
  *pBuf = GDP_CMD_PUSH_ATTR;

  cnt = (cldConfigData.HIDNumStdDescComps == 0) ? (sizeof(req) - 1) : sizeof(req);
  cnt = procGetCldAttr(cnt, req, pBuf);

  sendCldDataReq(zidCfgIdx, cnt, pBuf);
  osal_mem_free(pBuf);
}

/**************************************************************************************************
 * @fn          pushNonStdDescComp
 *
 * @brief       Transmit/push the DescSpecs as 'numNonStdDescComp' counts down.
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
static void pushNonStdDescComp( void )
{
  uint8 attrId, len, *pBuf;

  // Memory for exactly one non-std descriptor spec fragment
  if ((pBuf = osal_mem_alloc( GDP_PUSH_ATTR_REC_SIZE_HDR + ZID_NON_STD_DESC_SPEC_FRAG_T_MAX + 1 )) != NULL)
  {
    attrId = currentNonStdDescCompNum + aplHIDNonStdDescCompSpec1;

    *pBuf = GDP_CMD_PUSH_ATTR;
    len = procGetCldAttr(1, &attrId, pBuf);

    sendCldDataReq(zidCfgIdx, len, pBuf);
    osal_mem_free(pBuf);
  }
}

/**************************************************************************************************
 * @fn          pushNullReport
 *
 * @brief       Transmit/push any NULL reports for corresponding non-std desc comp.
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
static void pushNullReport( void )
{
  uint8 len;
  zid_set_report_cmd_t *pBuf;
  uint8 buf[sizeof(zid_null_report_t) + aplcMaxNullReportSize];
  zid_null_report_t *pReportHdr;

  // Memory for one NULL report
  if ((pBuf = (zid_set_report_cmd_t *)osal_mem_alloc( ZID_SET_REPORT_SIZE_HDR + aplcMaxNullReportSize + 1 )) != NULL)
  {
    /* First read header of NULL report to determine length */
    pReportHdr = (zid_null_report_t *)buf;
    (void)osal_snv_read( ZID_CLD_NVID_NULL_REPORT_BEG + currentNullReportNum,
                         sizeof(zid_null_report_t),
                         (uint8 *)pReportHdr );
    len = pReportHdr->len;

    /* Now that we have the length, read the whole report definition */
    (void)osal_snv_read( ZID_CLD_NVID_NULL_REPORT_BEG + currentNullReportNum,
                         sizeof(zid_null_report_t) + len,
                         (uint8 *)pReportHdr );

    /* Now build set report command */
    pBuf->cmd = ZID_CMD_SET_REPORT;
    pBuf->type = ZID_REPORT_TYPE_IN;
    pBuf->id = pReportHdr->reportId;
    osal_memcpy(pBuf->data, pReportHdr->data, len);

    /* send it */
    sendCldDataReq(zidCfgIdx, sizeof(zid_set_report_cmd_t) + len, (uint8 *)pBuf);
    osal_mem_free(pBuf);
  }
}

/**************************************************************************************************
 * @fn          procGetCldAttr
 *
 * @brief       Process an attribute (list) according to the parameters.
 *
 * input parameters
 *
 * @param        cnt  - The number of attributes in the pReq list.
 * @param       *pReq - Pointer to the buffer containing the Get Attribute command payload.
 * @param       *pBuf - Pointer to the buffer in which to pack the Get Attribute/Push payload.
 *                      The maximum required size is
 *                       (ZID_ATTR_RSP_SIZE_HDR + ZID_DESC_SPEC_SIZE_MAX + 1);
 *                        OR
 *                       (ZID_ATTR_RSP_SIZE_HDR * len + sizeof(zid_proxy_entry_t) +
 *                                                      sizeof(zid_cfg_t) + 1);
 *
 * output parameters
 *
 * None.
 *
 * @return      Zero for insufficient heap memory. Otherwise, the non-zero count of data in pBuf.
 */
static uint8 procGetCldAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf)
{
  uint8 *pBeg = pBuf;
  const uint8 rspF = (*pBuf++ == GDP_CMD_GET_ATTR_RSP) ? TRUE : FALSE;

  if (cnt == 0)
  {
    return 0;
  }

  if (*pReq >= aplHIDNonStdDescCompSpec1)  // If requesting a DescSpec.
  {
    if (cnt == 1)  // Exactly one DescSpec shall be requested.
    {
      uint8 id = *pReq;
      *pBuf++ = id;

      if (rspF)
      {
        if (id >= (aplHIDNonStdDescCompSpec1 + aplcMaxNonStdDescCompsPerHID))
        {
          *pBuf = GDP_ATTR_RSP_UNSUPPORTED;
          return 2;
        }
        else if (cldState > eCldCfgRdy)
        {
          *pBuf++ = GDP_ATTR_RSP_ILLEGAL_REQ;
          return 2;
        }
        else
        {
          *pBuf++ = GDP_ATTR_RSP_SUCCESS;
        }
      }

      id = ZID_CLD_NVID_DESC_START(currentNonStdDescCompNum) + currentNonStdDescCompFragNum;
      if (currentNonStdDescCompLen > aplcMaxNonStdDescFragmentSize)
      {
        cnt = aplcMaxNonStdDescFragmentSize + currentNonStdDescCompHdrSize;
      }
      else
      {
        cnt = currentNonStdDescCompLen + currentNonStdDescCompHdrSize;
      }
      *pBuf++ = cnt;
      (void)osal_snv_read(id, cnt, pBuf);  // Fail not expected.
      // +1 for the CMD byte to simplify math on calling function.
      cnt += (rspF) ? (sizeof(gdp_attr_rsp_t) + 1) : (sizeof(gdp_push_attr_rec_t) + 1);

      return cnt;
    }
    // else fall through to the logic below which will set the status to illegal request.
  }

  while (cnt--)
  {
    uint8 id = *pReq++;
    uint8 idx = getCldAttrTableIdx( id );
    uint8 len;

    *pBuf++ = id;

    if (rspF)
    {
      if (idx == ZID_TABLE_IDX_INVALID)
      {
        if (id >= aplHIDNonStdDescCompSpec1)  // If requesting a DescSpec.
        {
          *pBuf++ = GDP_ATTR_RSP_ILLEGAL_REQ;  // A DescSpec requested with others is illegal.
        }
        else
        {
          *pBuf++ = GDP_ATTR_RSP_UNSUPPORTED;
        }

        continue;  // Done with response for this unsupported attribute, goto parse the next one.
      }
      else if ((id >= aplHIDParserVersion) && (cldState > eCldCfgRdy))
      {
        /* Any aplHID attribute can only be requested during config state */
        *pBuf++ = GDP_ATTR_RSP_ILLEGAL_REQ;
        continue;  // Done with response for this unsupported attribute, goto parse the next one.
      }
      else
      {
        *pBuf++ = GDP_ATTR_RSP_SUCCESS;
      }
    }

    *pBuf++ = len = zidCld_attributeTable[idx].attrLen;

    if (id == aplHIDStdDescCompsList)
    {
      *(pBuf-1) = len = cldConfigData.HIDNumStdDescComps;
    }

    pBuf = osal_memcpy(pBuf, (uint8 *)&cldConfigData + zidCld_attributeTable[idx].offset, len);
  }

  return (uint8)(pBuf - pBeg);
}

/**************************************************************************************************
 * @fn          unpairReq
 *
 * @brief       Request an RTI_UnpairReq() on the 'dstIndex'.
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void unpairReq(uint8 dstIndex)
{
  cldState = eCldDor;
  if (dstIndex != RTI_INVALID_PAIRING_REF)
  {
    if (zidCfgIdx == dstIndex)
    {
      zidCfgIdx = RTI_INVALID_PAIRING_REF;
    }
    cldState = eCldUnpair;
    RTI_UnpairReq(dstIndex);
  }
  // Notify the multiple profile configuration
  osal_set_event( RTI_TaskId, GDP_EVT_CONFIGURATION_FAILED );
}

/**************************************************************************************************
 * @fn          zidTableIdx
 *
 * @brief       Resolve the index in the ZID_Table[] which corresonds to the parameter 'attrId'.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      ZID_ID_TABLE_IDX_INVALID for special descriptors and invalid/unsupported attributes.
 *              Otherwise, the index into the ZID_Table[].
 */
static uint8 getCldAttrTableIdx( uint8 attrId )
{
  uint8 idx;

  for (idx = 0; idx < ZID_CLD_ATTRIBUTE_TABLE_LEN; idx++)
  {
    if (zidCld_attributeTable[idx].attrId == attrId)
    {
      return idx;
    }
  }

  return ZID_TABLE_IDX_INVALID;
}

/**************************************************************************************************
*/
