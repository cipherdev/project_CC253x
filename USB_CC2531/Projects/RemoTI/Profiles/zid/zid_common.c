/**************************************************************************************************
  Filename:       zid_common.c
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include <stddef.h>
#include "comdef.h"
#include "hal_assert.h"
#include "osal.h"
#include "osal_memory.h"
#if defined POWER_SAVING
#include "osal_pwrmgr.h"
#endif
#include "osal_snv.h"
#include "rcn_nwk.h"
#include "rti.h"
#include "zid_adaptor.h"
#include "zid_class_device.h"
#include "zid_common.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

/* Non Standard Descriptor Component processing data */
static uint8 zidCommon_expectedNonStdDescFragId;
static uint8 zidCommon_lastReceivedNonStdDescFragId;
static uint16 zidCommon_numNonStdDescBytesReceived;
static uint16 zidCommon_nonStdDescCompSize;
static uint8 zidCommon_nonStdDescType;
static uint8 zidCommon_nonStdDescCompId;
static uint8 zidCommon_nonStdDescNumFragsExpected;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

// Upon successful Tx of any command which expects a response, only return to power-cycling or
// Rx-off upon receiving a response from the intended pair or expiration of the response wait timer.
// Stronger receipt control is exercised on messages related to Configuration state by checking
// against the zidState and zidCfgIndex.
uint8 zidRspIdx;

// RTI Pairing Table index of the destination of a ZID configuration phase.
uint8 zidCfgIdx;

uint8 zidCnfIdx;  // RTI Pairing Table index of the destination of a zidSendDataReq().

zidStatDisc_t zidStatDisc;
zid_pair_t zidPairInfo;
// RAM cached copy of the IntPipeUnsafeTxWindowTime for faster, frequent use.
uint16 zidUnsafeTime;

const uint8 ZID_StdReportLen[ZID_STD_REPORT_TOTAL_NUM+1] =
{
  0,  // There is no report zero - MOUSE starts at one.
  ZID_MOUSE_DATA_LEN,
  ZID_KEYBOARD_DATA_LEN,
  ZID_CONTACT_DATA_DATA_LEN,
  ZID_GESTURE_TAP_DATA_LEN,
  ZID_GESTURE_SCROLL_DATA_LEN,
  ZID_GESTURE_PINCH_DATA_LEN,
  ZID_GESTURE_ROTATE_DATA_LEN,
  ZID_GESTURE_SYNC_DATA_LEN,
  ZID_TOUCH_SENSOR_PROPERTIES_DATA_LEN,
  ZID_TAP_SUPPORT_PROPERTIES_DATA_LEN
};

/**************************************************************************************************
 * @fn          zidInitItems
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
void zidInitItems(void)
{
  if (FEATURE_ZID_ADA == TRUE)
  {
    zidAda_InitItems();
  }
  else if (FEATURE_ZID_CLD == TRUE)
  {
    zidCld_InitItems();
  }
}

/**************************************************************************************************
 * @fn          zidInitCnf
 *
 * @brief       RTI confirmation callback for the ZID Profile.
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
void zidInitCnf(void)
{
  uint8 nodeCap;

  for (uint8 idx = 0; idx < gRCN_CAP_MAX_PAIRS; idx++)
  {
    rcnNwkPairingEntry_t *pEntry;

    if (RCN_NlmeGetPairingEntryReq(idx, &pEntry) == RCN_SUCCESS)
    {
      if (GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID) &&  // If the entry is a ZID device.
         !GET_BIT(zidPairInfo.cfgCompleteDisc, idx))  // If the configuration did not complete.
      {
        RTI_UnpairReq(idx);
      }
      else
      {
        /* Inform ZID ADA or ZID CLD that we warm started */
        if (FEATURE_ZID_ADA && ZID_IS_ADA(idx))
        {
          zidAda_SetStateActive();
        }
        else if (FEATURE_ZID_CLD && !ZID_IS_ADA(idx))
        {
          zidCld_SetStateActive();
        }
      }
    }
    else  // Clean-up for pairing entries cleared by other than unpair indication.
    {
      CLR_BIT(zidPairInfo.adapterDisc, idx);
      CLR_BIT(zidPairInfo.cfgCompleteDisc, idx);
    }
  }

  (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);

  zidStatDisc = (zidStatDisc_t)0;
  (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_NODE_CAPABILITIES, 1, &nodeCap);
  (nodeCap & RTI_NODE_CAP_NODE_TYPE_BM) ? ZID_SET_TGT() : ZID_CLR_TGT();
}

/**************************************************************************************************
 * @fn          zidSendDataReq
 *
 * @brief       This function sets timers, Rx on, and Tx Options according to requisite behavior.
 *              This function will be called indirectly via RTI_SendDataReq().
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
uint8 zidSendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd)
{
  zidCnfIdx = dstIndex;

  if (FEATURE_ZID_ADA && ZID_IS_ADA(dstIndex))  return zidAda_SendDataReq(dstIndex, txOptions, cmd);
  if (FEATURE_ZID_CLD && !ZID_IS_ADA(dstIndex)) return zidCld_SendDataReq(dstIndex, txOptions, cmd);

  return txOptions;
}

/**************************************************************************************************
 * @fn      zidSendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API call.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  TRUE if ZID initiated the send data request; FALSE otherwise.
 */
uint8 zidSendDataCnf(rStatus_t status)
{
  if (zidCnfIdx == RTI_INVALID_PAIRING_REF)  // If the data confirmation is for a zidSendDataReq().
  {
    return FALSE;
  }

  uint8 idx = zidCnfIdx;
  uint8 taskId;

  zidCnfIdx = RTI_INVALID_PAIRING_REF;

  if (FEATURE_ZID_ADA && ZID_IS_ADA(idx))
  {
    taskId = zidAda_TaskId;
  }
  else if (FEATURE_ZID_CLD && !ZID_IS_ADA(idx))
  {
    taskId = zidCld_TaskId;
  }
  else
  {
    return FALSE;
  }

  if (ZID_IS_CNFING)
  {
    ZID_CLR_CNFING();

    if ((status == RTI_SUCCESS) &&
        (osal_start_timerEx(taskId, ZID_CLD_EVT_SAFE_TX, zidUnsafeTime) == SUCCESS))
    {
      ZID_SET_UNSAFE();
    }
    else
    {
      ZID_CLR_UNSAFE();
    }
  }

  if (ZID_IS_RSPING)  // If a ZID response is required for the ZID message sent.
  {
    ZID_CLR_RSPING();

    if (status == RTI_SUCCESS)
    {
      zidRspWait(taskId, aplcMaxResponseWaitTime);
    }
    else
    {
      zidRspDone(taskId, FALSE);
    }
  }

  if (ZID_IS_TXING)  // If the ZID co-layer initiated this SendDataReq.
  {
    ZID_CLR_TXING();
    status = TRUE;
  }
  else
  {
    status = FALSE;
  }

  return status;
}

/**************************************************************************************************
 * @fn      zidReceiveDataInd
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
 * @param   len:       Number of data bytes.
 * @param   *pData:    Pointer to data received.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the data was handled; otherwise FALSE indicating to pass the data to
 *              Application layer.
 */
uint8 zidReceiveDataInd( uint8 srcIndex,
                         uint16 vendorId,
                         uint8 rxLQI,
                         uint8 rxFlags,
                         uint8 len,
                         uint8 *pData )
{
  // If awaiting a response, only allow a return to power-cycling on response from expected Pair.
  if (zidRspIdx == srcIndex)
  {
    uint8 taskId;
    if (FEATURE_ZID_ADA && ZID_IS_ADA(srcIndex))
    {
      taskId = zidAda_TaskId;
    }
    else if (FEATURE_ZID_CLD && !ZID_IS_ADA(srcIndex))
    {
      taskId = zidCld_TaskId;
    }
    zidRspDone(taskId, TRUE);
  }

  if (FEATURE_ZID_ADA && ZID_IS_ADA(srcIndex))
  {
    return zidAda_ReceiveDataInd( srcIndex, vendorId, rxLQI, rxFlags, len, pData );
  }
  else if (FEATURE_ZID_CLD && !ZID_IS_ADA(srcIndex))
  {
    return zidCld_ReceiveDataInd( srcIndex, rxFlags, len, pData );
  }
  else
  {
    return FALSE;
  }
}

/**************************************************************************************************
 *
 * @fn      zidUnpair
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API call or by
 *          receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  TRUE if ZID initiated the unpair; FALSE otherwise.
 */
uint8 zidUnpair(uint8 dstIndex)
{
  uint8 rtrn = FALSE;

  if (FEATURE_ZID_ADA && ZID_IS_ADA(dstIndex))   rtrn = zidAda_Unpair(dstIndex);
  if (FEATURE_ZID_CLD && !ZID_IS_ADA(dstIndex))  rtrn = zidCld_Unpair(dstIndex);

  zidCfgIdx = RTI_INVALID_PAIRING_REF;
  CLR_BIT(zidPairInfo.adapterDisc, dstIndex);
  CLR_BIT(zidPairInfo.cfgCompleteDisc, dstIndex);
  (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);

  return rtrn;
}

/**************************************************************************************************
 * @fn          zidReadItem
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
rStatus_t zidReadItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_ERROR_INVALID_PARAMETER;

  if (FEATURE_ZID_ADA == TRUE)
  {
    status = zidAda_ReadItem( itemId, len, pValue );
  }
  else if (FEATURE_ZID_CLD == TRUE)
  {
    status = zidCld_ReadItem( itemId, len, pValue );
  }

  return status;
}

/**************************************************************************************************
 * @fn          zidWriteItem
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
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidWriteItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_ERROR_INVALID_PARAMETER;

  if (FEATURE_ZID_ADA == TRUE)
  {
    status = zidAda_WriteItem( itemId, len, pValue );
  }
  else if (FEATURE_ZID_CLD == TRUE)
  {
    status = zidCld_WriteItem( itemId, len, pValue );
  }

  return status;
}

/**************************************************************************************************
 * @fn          zidRspDone
 *
 * @brief       Act on the end of a ZID specified response wait.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 * @param       status - TRUE is got the expected response, FALSE otherwise.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidRspDone(uint8 taskId, uint8 status)
{
  zidRspIdx = RTI_INVALID_PAIRING_REF;
  (void)osal_stop_timerEx(taskId, ZID_EVT_RSP_WAIT);

  if (status == FALSE)
  {
    (void)osal_set_event(taskId, ZID_EVT_RSP_WAIT);
  }
  else
  {
    (void)osal_clear_event(taskId, ZID_EVT_RSP_WAIT);
  }

  if (ZID_IS_CTL || ZID_WAS_STANDBY)
  {
    ZID_CLR_STANDBY();
    RCN_NlmeRxEnableReq(0);
  }
}

/**************************************************************************************************
 * @fn          zidRspWait
 *
 * @brief       Setup to wait for a ZID specified response.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 * @param       timeout - The wait time in milliseconds.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidRspWait(uint8 taskId, uint16 timeout)
{
  if (osal_start_timerEx(taskId, ZID_EVT_RSP_WAIT, timeout) == SUCCESS)
  {
    (void)osal_clear_event(taskId, ZID_EVT_RSP_WAIT);
  }
  else
  {
    (void)osal_set_event(taskId, ZID_EVT_RSP_WAIT);
    timeout = 0;
  }

  if (ZID_IS_CTL)
  {
    RCN_NlmeRxEnableReq(timeout);
  }
  else if (timeout != 0)
  {
    uint8 standby;
    (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_IN_POWER_SAVE, 1, &standby);

    if (standby)
    {
      ZID_SET_STANDBY();
      RCN_NlmeRxEnableReq(0xFFFF);
    }
  }
}

/**************************************************************************************************
 * @fn          zidCommon_ResetNonStdDescCompData
 *
 * @brief       Reset data used to validate non standard descriptor components
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return
 *
 * None.
 */
void zidCommon_ResetNonStdDescCompData( void )
{
  zidCommon_expectedNonStdDescFragId = 0;
  zidCommon_numNonStdDescBytesReceived = 0;
}

/**************************************************************************************************
 * @fn          zidCommon_ProcessNonStdDescComp
 *
 * @brief       Validate a non standard descriptor component
 *
 * input parameters
 *
 * @param  len           - length of record to be checked
 * @param  pData         - Pointer to "attribute value" field of attribute record of push attributes cmd
 * @param  descComplete  - Pointer to field indicating if a complete descriptor has been processed
 *
 * output parameters
 *
 * None.
 *
 * @return      GDP_GENERIC_RSP_SUCCESS if contents were OK
 *              GDP_GENERIC_RSP_INVALID_PARAM if data out of bounds
 *              ZID_GENERIC_RSP_MISSING_FRAGMENT if out of sync fragment received
 */
uint8 zidCommon_ProcessNonStdDescComp( uint8 len, uint8 *pData, bool *descComplete )
{
  zid_non_std_desc_comp_t *pDesc = (zid_non_std_desc_comp_t *)pData;
  uint16 size = (pDesc->size_h << 8) | pDesc->size_l;
  uint8 rtnVal = GDP_GENERIC_RSP_SUCCESS;

  *descComplete = FALSE;
  zidCommon_lastReceivedNonStdDescFragId = zidCommon_expectedNonStdDescFragId;

  if (((pDesc->type != ZID_DESC_TYPE_REPORT) && (pDesc->type != ZID_DESC_TYPE_PHYSICAL)) ||
      (size > aplcMaxNonStdDescCompSize))
  {
    rtnVal = GDP_GENERIC_RSP_INVALID_PARAM;
  }

  if (size > aplcMaxNonStdDescFragmentSize)
  {
    /* Descriptor should be fragmented, so check if expected fragment was received */
    zid_frag_non_std_desc_comp_t *pFragDesc = (zid_frag_non_std_desc_comp_t *)pData;
    uint8 fragLen = len - sizeof(zid_frag_non_std_desc_comp_t);

    if (pFragDesc->fragId != zidCommon_expectedNonStdDescFragId)
    {
      rtnVal = ZID_GENERIC_RSP_MISSING_FRAGMENT;
    }
    else if (fragLen > aplcMaxNonStdDescFragmentSize)
    {
      rtnVal = GDP_GENERIC_RSP_INVALID_PARAM;
    }
    else
    {
      /* Update running fragment data */
      zidCommon_numNonStdDescBytesReceived += fragLen;
      zidCommon_expectedNonStdDescFragId++;

      /* Validate per fragment received */
      if (pFragDesc->fragId == 0)
      {
        /* First fragment, so store type, size, and component id */
        zidCommon_nonStdDescType = pFragDesc->type;
        zidCommon_nonStdDescCompSize = size;
        zidCommon_nonStdDescCompId = pFragDesc->reportId;
        zidCommon_nonStdDescNumFragsExpected = (size + aplcMaxNonStdDescFragmentSize - 1) / aplcMaxNonStdDescFragmentSize;
      }
      else
      {
        /* Not first fragment, so verify header is same */
        if ((pFragDesc->type != zidCommon_nonStdDescType) ||
            (size != zidCommon_nonStdDescCompSize) ||
            (pFragDesc->reportId != zidCommon_nonStdDescCompId))
        {
          rtnVal = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else if (pFragDesc->fragId == (zidCommon_nonStdDescNumFragsExpected - 1))
        {
          /* Last fragment, so make sure we received correct amount of data */
          if (zidCommon_numNonStdDescBytesReceived != zidCommon_nonStdDescCompSize)
          {
            rtnVal = GDP_GENERIC_RSP_INVALID_PARAM;
          }
          else
          {
            *descComplete = TRUE;
          }
        }
      }
    }
  }
  else
  {
    /* Complete descriptor is here, so just indicate we've received everything */
    *descComplete = TRUE;
  }

  return rtnVal;
}

/**************************************************************************************************
 * @fn          zidCommon_GetNonStdDescCompFragNum
 *
 * @brief       Obtain fragment number from non standard descriptor fragment
 *
 * input parameters
 *
 * @param  pDesc         - Pointer to "attribute value" field of attribute record of push attributes cmd
 *
 * output parameters
 *
 * None.
 *
 * @return      fragment number if descriptor was fragmented, otherwise 0
 */
uint8 zidCommon_GetNonStdDescCompFragNum( zid_non_std_desc_comp_t *pDesc  )
{
  uint8 rtnVal;
  uint16 size = (pDesc->size_h << 8) | pDesc->size_l;

  if (size > aplcMaxNonStdDescFragmentSize)
  {
    /* Descriptor should be fragmented, so get fragment number from descriptor */
    zid_frag_non_std_desc_comp_t *pFragDesc = (zid_frag_non_std_desc_comp_t *)pDesc;
    rtnVal = pFragDesc->fragId;
  }
  else
  {
    /* Descriptor not fragmented, so use fragment number of 0 */
    rtnVal = 0;
  }

  return rtnVal;
}

/**************************************************************************************************
 * @fn          zidCommon_GetLastNonStdDescCompFragNum
 *
 * @brief       Obtain fragment number from last received non standard descriptor fragment
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      fragment number of last received non-std descriptor
 */
uint8 zidCommon_GetLastNonStdDescCompFragNum( void )
{
  return zidCommon_lastReceivedNonStdDescFragId;
}

/**************************************************************************************************
 * @fn          zidCommon_specCheckWrite
 *
 * @brief       Check value for ZID specified validity and internally constrained limits
 *              for writing.
 *
 * input parameters
 *
 * @param       attrId  - The ZID Attribute identifier.
 * @param       len     - The length in bytes of the item identifier's data.
 * @param       *pValue - A pointer to the item's data.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the value is within specified boundaries or doesn't have any.
 *              FALSE otherwise.
 */
uint8 zidCommon_specCheckWrite(uint8 attrId, uint8 len, uint8 *pValue)
{
  uint8 rtrn = TRUE;

  if (attrId == ZID_ITEM_NON_STD_DESC_FRAG)
  {
    zid_non_std_desc_comp_t *p = (zid_non_std_desc_comp_t *)pValue;

    if (((p->type != ZID_DESC_TYPE_REPORT) && (p->type != ZID_DESC_TYPE_PHYSICAL)) ||
        (len > (aplcMaxNonStdDescFragmentSize + sizeof(zid_frag_non_std_desc_comp_t))))
    {
      rtrn = FALSE;
    }
  }
  else
  {
    switch (attrId)
    {
      case aplZIDProfileVersion:
      {
        /* Disallow writing the ZID Profile Version */
        rtrn = FALSE;
        break;
      }

      case aplIntPipeUnsafeTxWindowTime:
      {
        if (BUILD_UINT16(pValue[0], pValue[1]) < aplcMinIntPipeUnsafeTxWindowTime)
        {
          rtrn = FALSE;
        }
        break;
      }

      case aplReportRepeatInterval:
      {
        if (*pValue > aplcMaxReportRepeatInterval)
        {
          rtrn = FALSE;
        }
        break;
      }

      case aplHIDPollInterval:
      {
        if ((*pValue < 1) || (*pValue > 16))
        {
          rtrn = FALSE;
        }
        break;
      }

      case aplHIDNumStdDescComps:
      {
        if (*pValue > aplcMaxStdDescCompsPerHID)
        {
          rtrn = FALSE;
        }
        break;
      }

      case aplHIDStdDescCompsList:
      {
        if (len > aplcMaxStdDescCompsPerHID)
        {
          rtrn = FALSE;
        }
        break;
      }

      case aplHIDNumNonStdDescComps:
      {
        if (*pValue > aplcMaxNonStdDescCompsPerHID)
        {
          rtrn = FALSE;
        }
        break;
      }

      default:
      {
        break;
      }
    }
  }

  return rtrn;
}

/**************************************************************************************************
*/
