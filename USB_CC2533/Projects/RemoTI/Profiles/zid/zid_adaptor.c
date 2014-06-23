/**************************************************************************************************
  Filename:       zid_adapter.c
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "gdp_profile.h"
#include "OSAL.h"
#include "osal_snv.h"
#include "rti.h"
#include "rcn_nwk.h"
#include <stddef.h>
#include "zid_adaptor.h"
#include "zid_common.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

enum {
  eCfgHIDParserVersion,                    // aplHIDParserVersion                0xE0
  eCfgHIDCountryCode,                      // aplHIDCountryCode                  0xE3
  eCfgHIDDeviceReleaseNumber,              // aplHIDDeviceReleaseNumber          0xE4
  eCfgHIDVendorId,                         // aplHIDVendorId                     0xE5
  eCfgHIDProductId,                        // aplHIDProductId                    0xE6
  eCfgHIDNumEndpoints,                     // aplHIDNumEndpoints                 0xE7
  eCfgHIDPollInterval,                     // aplHIDPollInterval                 0xE8
  eCfgHIDNumStdDescComps,                  // aplHIDNumStdDescComps              0xE9
  eCfgHIDNumNullReports,                   // aplHIDNumNullReports               0xEB
  eCfgHIDNumNonStdDescComps,               // aplHIDNumNonStdDescComps           0xF0
  eCfgHIDNonStdDescCompSpec1,              // aplHIDNonStdDescCompSpec1          0xF1
  eCfgHIDNonStdDescCompSpec2,              // aplHIDNonStdDescCompSpec2          0xF2
  eCfgHIDNonStdDescCompSpec3,              // aplHIDNonStdDescCompSpec3          0xF3
  eCfgHIDNonStdDescCompSpec4,              // aplHIDNonStdDescCompSpec4          0xF4

  eCfgDiscCnt                              // Count of the discrete bits used to track Cfg-Complete.
};

#if !defined ZID_ADA_NVID_BEG
#define ZID_ADA_NVID_BEG  (ZID_COMMON_NVID_END + 1)
#endif

#define ZID_ADA_NVID_ATTR_CFG             (ZID_ADA_NVID_BEG + 0)
#define ZID_ADA_NVID_PXY_LIST             (ZID_ADA_NVID_BEG + 1)
#define ZID_ADA_NVID_CFG_PXY              (ZID_ADA_NVID_BEG + 2)
#define ZID_ADA_NVID_DESC_BEG             (ZID_ADA_NVID_CFG_PXY + ZID_COMMON_MAX_NUM_PROXIED_DEVICES)

#define ZID_ADA_NUM_NVID_PER_PXY          (aplcMaxNonStdDescCompsPerHID * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC)
#define ZID_ADA_NVID_DESC_END             (ZID_ADA_NVID_DESC_BEG + (ZID_COMMON_MAX_NUM_PROXIED_DEVICES * ZID_ADA_NUM_NVID_PER_PXY) - 1)
#define ZID_ADA_NVID_DESC_START(pxyNum, descNum) \
  (ZID_ADA_NVID_DESC_BEG + ((pxyNum) * ZID_ADA_NUM_NVID_PER_PXY) + ((descNum) * ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC))
#define ZID_ADA_NVID_NULL_REPORT_BEG      (ZID_ADA_NVID_DESC_END + 1)
#define ZID_ADA_NVID_NULL_REPORT_END      (ZID_ADA_NVID_NULL_REPORT_BEG + (ZID_COMMON_MAX_NUM_PROXIED_DEVICES * aplcMaxNonStdDescCompsPerHID) - 1)
#define ZID_ADA_NVID_NULL_REPORT_START(pxyNum, descNum) \
  (ZID_ADA_NVID_NULL_REPORT_BEG + ((pxyNum) * aplcMaxNonStdDescCompsPerHID) + (descNum))

#define ZID_ADA_CFG_PXY_ENTRY(pxyNum)     (ZID_ADA_NVID_CFG_PXY + (pxyNum))
/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

typedef enum
{
  eAdaDor,      // ZID Dormant state.
  eAdaCfg,      // Configuration state with recently paired HID Class device.
  eAdaUnpair,   // Awaiting response from RTI_UnpairReq().
  eAdaRdy,      // Paired and configured, so ZID Idle or Active state.
  eAdaTest
} adaState_t;

typedef struct
{
  uint8 pairIndex;
  uint16 vendorId;
  uint8 rxLQI;
  uint8 rxFlags;
  uint8 reportId;
} zidAdaReportInfo_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static void endCfgState(void);
static uint8 rxCfgComplete(uint8 srcIndex);
static void rxGetAdaAttr(uint8 srcIndex, uint8 len, uint8 *pData);
static uint8 rxPushAdaAttr(uint8 srcIndex, uint16 len, uint8 *pData);
static void sendDataReq(uint8 dstIndex, uint8 len, uint8 *pData);
static uint8 procGetAdaAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf);
static void unpairReq(uint8 dstIndex);
static uint8 stdDescCheck(uint8 len, uint8 *pData);
static bool findNonStdDescReport( uint8 proxyIdx, uint8 reportId, uint8 *descNum );
static uint8 rxSetReport( uint16 len, uint8 *pData );
static void sendNullReport( void );
static bool isReportFrameValid( uint8 srcIndex, uint16 vendorId, uint8 rxLQI, uint8 rxFlags, uint8 len, uint8 *pData );
static uint8 getFreeProxyTableEntry( uint8 dstIndex );
static void addToProxyTable( uint8 pairIdx );
static uint8 getProxyTableEntry( uint8 pairIdx );
static uint8 getAttrTableIdx( uint8 attrId );
static uint8 getPxyTableIdx( uint8 attrId );
static void rspWaitTimeout( void );

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

// Bit discretes to track the progress of the Configuration state.
static uint8 cfgDiscs[(eCfgDiscCnt + 7) / 8];
// Dynamically allocated proxy entry structure to store incoming push and verify cfg complete.
static zid_proxy_entry_t *pCfgProxy;

static adaState_t adaState;

/* Used to save last report sent info in case we need to send a NULL report later */
static zidAdaReportInfo_t zidAdaNullReportInfo;

static const zid_table_t zidAda_attributeTable[] =
{
  {
    aplZIDProfileVersion,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_ada_cfg_t, ZIDProfileVersion)
  },
  {
    aplHIDParserVersion,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_ada_cfg_t, HIDParserVersion)
  },
  {
    aplHIDCountryCode,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_ada_cfg_t, HIDCountryCode)
  },
  {
    aplHIDDeviceReleaseNumber,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_ada_cfg_t, HIDDeviceReleaseNumber)
  },
  {
    aplHIDVendorId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_ada_cfg_t, HIDVendorId)
  },
  {
    aplHIDProductId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_ada_cfg_t, HIDProductId)
  }
};
#define ZID_ADA_ATTRIBUTE_TABLE_LEN  (sizeof(zidAda_attributeTable) / sizeof(zidAda_attributeTable[0]))

static const zid_table_t zidAda_proxyAttributeTable[] =
{
  {
    aplHIDParserVersion,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_proxy_entry_t, HIDParserVersion)
  },
  {
    aplHIDDeviceReleaseNumber,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_proxy_entry_t, HIDDeviceReleaseNumber)
  },
  {
    aplHIDVendorId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_proxy_entry_t, HIDVendorId)
  },
  {
    aplHIDProductId,
    ZID_DATATYPE_UINT16_LEN,
    offsetof(zid_proxy_entry_t, HIDProductId)
  },
  {
    aplHIDDeviceSubclass,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDDeviceSubclass)
  },
  {
    aplHIDProtocolCode,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDProtocolCode)
  },
  {
    aplHIDCountryCode,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDCountryCode)
  },
  {
    aplHIDNumEndpoints,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDNumEndpoints)
  },
  {
    aplHIDPollInterval,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDPollInterval)
  },
  {
    aplHIDNumStdDescComps,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDNumStdDescComps)
  },
  {
    aplHIDStdDescCompsList,
    aplcMaxStdDescCompsPerHID,
    offsetof(zid_proxy_entry_t, HIDStdDescCompsList)
  },
  {
    aplHIDNumNonStdDescComps,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDNumNonStdDescComps)
  },
  {
    aplHIDNumNullReports,
    ZID_DATATYPE_UINT8_LEN,
    offsetof(zid_proxy_entry_t, HIDNumNullReports)
  }
};
#define ZID_ADA_PROXY_ATTRIBUTE_TABLE_LEN  (sizeof(zidAda_proxyAttributeTable) / sizeof(zidAda_proxyAttributeTable[0]))

/* Map the RTI pairing table index to the Proxy Entry Index. */
static uint8 zidAda_pxyInfoTable[RCN_CAP_PAIR_TABLE_SIZE];

/* Variables used by RTI_ReadItem and RTIWriteItem to manipulate proxy info */
static uint8 zidAdaCurrentProxyIdx;
static uint8 zidAdaCurrentNonStdDescNum;
static uint8 zidAdaCurrentNonStdDescFragNum;
static uint8 zidAda_nextProxyIdx;
static zid_ada_cfg_t adaConfigData;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

uint8 zidAda_TaskId;

/**************************************************************************************************
 * @fn          zidAda_Init
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
void zidAda_Init(uint8 taskId)
{
  zidAda_TaskId = taskId;
}

/**************************************************************************************************
 * @fn          zidAda_InitItems
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
void zidAda_InitItems(void)
{
  if (osal_snv_read(ZID_ADA_NVID_ATTR_CFG, ZID_ADA_CFG_T_SIZE, &adaConfigData) != SUCCESS)
  {
    adaConfigData.HIDParserVersion = 0x0111;         // bcdHID V1.11
    adaConfigData.HIDDeviceReleaseNumber = 0x0100;   // bcdDevice V1.0
    adaConfigData.HIDVendorId = 0x0451;              // Texas Instruments Vendor Id.
    adaConfigData.HIDProductId = 0x16C2;             // CC2531 ZID Dongle.
    adaConfigData.HIDCountryCode = 0x0;
  }

  // Since the Profile Version is not writable, always update in case of an OAD.
  adaConfigData.ZIDProfileVersion = ZID_PROFILE_VERSION;
  (void)osal_snv_write(ZID_ADA_NVID_ATTR_CFG, ZID_ADA_CFG_T_SIZE, &adaConfigData);

  if (osal_snv_read(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo) != SUCCESS)
  {
    (void)osal_memset(zidPairInfo.cfgCompleteDisc, 0, ZID_PAIR_DISC_CNT);
    (void)osal_memset(zidPairInfo.adapterDisc, 0, ZID_PAIR_DISC_CNT);
    (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);
  }

  /* See if there is a saved proxy info table */
  if (osal_snv_read( ZID_ADA_NVID_PXY_LIST, sizeof(zidAda_pxyInfoTable), zidAda_pxyInfoTable ) != SUCCESS)
  {
    (void)osal_memset( zidAda_pxyInfoTable, RTI_INVALID_PAIRING_REF, sizeof(zidAda_pxyInfoTable));
    (void)osal_snv_write( ZID_ADA_NVID_PXY_LIST, sizeof(zidAda_pxyInfoTable), (uint8 *)zidAda_pxyInfoTable );
  }

  zidAdaCurrentProxyIdx = 0;
  zidAdaCurrentNonStdDescNum = 0;
  zidAdaCurrentNonStdDescFragNum = 0;

  zidCnfIdx = zidCfgIdx = zidRspIdx = RTI_INVALID_PAIRING_REF;
}

/**************************************************************************************************
 * @fn          zidAda_ProcessEvent
 *
 * @brief       This routine handles ZID Adaptor task events.
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
uint16 zidAda_ProcessEvent(uint8 taskId, uint16 events)
{
  (void)taskId;

  if (events & ZID_ADA_EVT_IDLE_RATE_GUARD_TIME)
  {
    if (adaState == eAdaRdy)
    {
      /* Send NULL report if it exists */
      sendNullReport();
    }
  }

  if (events & ZID_EVT_RSP_WAIT)
  {
    rspWaitTimeout();
  }

  return 0;  // All events processed in one pass; discard unexpected events.
}

void zidAda_SetStateActive( void )
{
  adaState = eAdaRdy;
}

/**************************************************************************************************
 * @fn          zidAda_SendDataReq
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
uint8 zidAda_SendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd)
{
  switch (*cmd & GDP_HEADER_CMD_CODE_MASK)
  {
  case GDP_CMD_GET_ATTR:
  case ZID_CMD_GET_REPORT:
  case ZID_CMD_SET_REPORT:
    ZID_SET_RSPING();
    zidRspIdx = dstIndex;
    break;

  default:
    break;
  }

  return txOptions;
}

/**************************************************************************************************
 * @fn      isReportFrameValid
 *
 * @brief   Analyzes a received report data command frame and determines if all the contained reports
 *          are valid. If so, info from the last report is saved in case a NULl report needs to be
 *          sent later.
 *
 * input parameters
 *
 * @param   srcIndex:  Pairing table index.
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
 * @return      TRUE if the frame contains valid reports; otherwise FALSE indicating at least 1 report
 *              was invalid and thus should not be forwarded to the Application layer.
 */
static bool isReportFrameValid( uint8 srcIndex,
                                uint16 vendorId,
                                uint8 rxLQI,
                                uint8 rxFlags,
                                uint8 len,
                                uint8 *pData )
{
  bool rxSecurityEnabled = (rxFlags & RTI_RX_FLAGS_SECURITY) ? TRUE : FALSE;
  bool rtrn = TRUE;

  /* ZID specifies to not act on reports if not completely configured
   * and to discard keyboard reports without security (R11 section 6.4.2).
   */
  if (!GET_BIT(zidPairInfo.cfgCompleteDisc, srcIndex))
  {
    rtrn = FALSE;
  }
  else
  {
    len -= 1; // subtract GDP header length
    pData += 1; // point to beginning of 1st record
    while (len)
    {
      uint8 recordLen = *pData + 1; // record length is length of fields not including length field
      uint8 reportId = ((zid_report_record_t *)pData)->id;
      if ((reportId == ZID_STD_REPORT_KEYBOARD) && (rxSecurityEnabled == FALSE))
      {
        rtrn = FALSE;
      }
      else
      {
        /* Save info needed in case we need to send a NULL report later */
        zidAdaNullReportInfo.pairIndex = srcIndex;
        zidAdaNullReportInfo.vendorId = vendorId;
        zidAdaNullReportInfo.rxLQI = rxLQI;
        zidAdaNullReportInfo.rxFlags = rxFlags;
        zidAdaNullReportInfo.reportId = reportId;
      }
      pData += recordLen;
      len -= recordLen;
    }
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn          sendNullReport
 *
 * @brief       This function sends a NULL report based on last report sent.
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
static void sendNullReport( void )
{
  uint8 buf[aplcMaxNullReportSize + sizeof(zid_report_data_cmd_t) + sizeof(zid_report_record_t)];
  zid_report_data_cmd_t *pReport = (zid_report_data_cmd_t *)buf;
  zid_report_record_t *pRecord = (zid_report_record_t *)&pReport->reportRecordList[0];
  uint8 reportLen, i;
  bool sendNullReport = FALSE;

  pReport->cmd = ZID_CMD_REPORT_DATA;
  pRecord->type = ZID_REPORT_TYPE_IN;
  pRecord->id = zidAdaNullReportInfo.reportId;
  if (zidAdaNullReportInfo.reportId <= ZID_STD_REPORT_TOTAL_NUM)
  {
    /* Standard report, so NULL reports are not pushed by CLD. Just build and send. */
    if (zidAdaNullReportInfo.reportId == ZID_STD_REPORT_MOUSE)
    {
      reportLen = ZID_MOUSE_DATA_LEN;
      sendNullReport = TRUE;
    }
    else if (zidAdaNullReportInfo.reportId == ZID_STD_REPORT_KEYBOARD)
    {
      reportLen = ZID_KEYBOARD_DATA_LEN;
      sendNullReport = TRUE;
    }

    if (sendNullReport == TRUE)
    {
      pRecord->len = sizeof(zid_report_record_t) + reportLen - 1;
      for (i = 0; i < reportLen; i++)
      {
        pRecord->data[i] = 0;
      }
    }
  }
  else
  {
    /* non-std report... look up in NV and copy to buf */
    uint8 proxyIdx = getProxyTableEntry( zidAdaNullReportInfo.pairIndex );
    if (proxyIdx != RTI_INVALID_PAIRING_REF)
    {
      uint8 descNum;
      if (findNonStdDescReport( proxyIdx, zidAdaNullReportInfo.reportId, &descNum ) == TRUE)
      {
        uint8 nonStdDescBuf[ZID_NULL_REPORT_T_MAX];
        zid_null_report_t *pBuf = (zid_null_report_t *)nonStdDescBuf;
        uint8 nvId;

        nvId = ZID_ADA_NVID_NULL_REPORT_START( proxyIdx, descNum );
        if (osal_snv_read( nvId, ZID_NULL_REPORT_T_MAX, pBuf ) == SUCCESS)
        {
          pRecord->len = sizeof(zid_report_record_t) + pBuf->len - 1;
          osal_memcpy( pRecord->data, pBuf->data, pBuf->len );
          sendNullReport = TRUE;
        }
      }
    }
  }

  /* Now send the NULL report if desired */
  if (sendNullReport == TRUE)
  {
    RTI_ReceiveDataInd( zidAdaNullReportInfo.pairIndex,
                        RTI_PROFILE_ZID,
                        zidAdaNullReportInfo.vendorId,
                        zidAdaNullReportInfo.rxLQI,
                        zidAdaNullReportInfo.rxFlags,
                        pRecord->len + 2, // 1 for report size field, and 1 for cmd
                        buf );
  }
}

/**************************************************************************************************
 * @fn      zidAda_ReceiveDataInd
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
uint8 zidAda_ReceiveDataInd( uint8 srcIndex,
                             uint16 vendorId,
                             uint8 rxLQI,
                             uint8 rxFlags,
                             uint8 len,
                             uint8 *pData )
{
  uint8 cmd = pData[ZID_FRAME_CTL_IDX] & GDP_HEADER_CMD_CODE_MASK;
  bool sendGenericResponse = FALSE;
  uint8 rsp;
  uint8 rtrn = FALSE;
  uint8 rxOn = 0;

  switch (cmd)
  {
    case GDP_CMD_GET_ATTR:
    {
      rxGetAdaAttr(srcIndex, len, pData);
      rtrn = TRUE;  // Completely handled here.
      if ((adaState == eAdaCfg) && (zidCfgIdx == srcIndex))
      {
        // Allow each step of the configuration phase its maximum wait time.
        zidRspWait(zidAda_TaskId, aplcMaxConfigWaitTime);
      }
      break;
    }

    case GDP_CMD_GET_ATTR_RSP:
    {
#if FEATURE_ZID_ADA_TST
      if (adaState == eAdaTest)
      {
        // rsp = GDP_GENERIC_RSP_SUCCESS;

        uint8 buf[3] =
        {
          ZID_CMD_GET_REPORT,
          ZID_REPORT_TYPE_IN,
          ZID_STD_REPORT_MOUSE
        };
        sendDataReq(srcIndex, 3, buf);

        rtrn = TRUE;  // Completely handled here.
      }
#endif
      break;
    }

    case ZID_CMD_REPORT_DATA:
    {
      // ZID specifies to not act on reports if not completely configured
      // and to discard keyboard reports without security (R11 section 6.4.2).
      if (isReportFrameValid( srcIndex, vendorId, rxLQI, rxFlags, len, pData ) == FALSE)
      {
        /* Invalid report, so don't forward to application */
        rtrn = TRUE;
      }
      else
      {
        /* Kick idle rate guard timer since we're going to send the report */
        (void)osal_start_timerEx(zidAda_TaskId, ZID_ADA_EVT_IDLE_RATE_GUARD_TIME, aplcIdleRateGuardTime);
      }
      break;
    }

    case ZID_CMD_SET_REPORT:
    {
      /* The CLD can push NULL reports during config phase */
      rsp = rxSetReport( len, pData );

      // This requires a generic response
      sendGenericResponse = TRUE;

      /* completely handled here */
      rtrn = TRUE;
      break;
    }

    case GDP_CMD_HEARTBEAT:
    {
#if FEATURE_ZID_ADA_TST
      if (adaState == eAdaTest)
      {
        uint8 buf[6] =
        {
          GDP_CMD_GET_ATTR | GDP_HEADER_DATA_PENDING,
          0x90,
          0xEF,  // Unsupported
          aplHIDParserVersion,
          0xF1,  // Illegal
          aplHIDStdDescCompsList
        };
        sendDataReq(srcIndex, 6, buf);

        rtrn = TRUE;  // Completely handled here.
      }
#endif
      break;
    }

    case GDP_CMD_PUSH_ATTR:
    {
      rsp = rxPushAdaAttr(srcIndex, len, pData);

      // Allow each step of the configuration phase its maximum wait time.
      zidRspWait(zidAda_TaskId, aplcMaxConfigWaitTime);

      // In any case, we are going to send a generic response
      sendGenericResponse = TRUE;
      rtrn = TRUE;  // Completely handled here.
      break;
    }

    case GDP_CMD_CFG_COMPLETE:
    {
      if ((rsp = rxCfgComplete(srcIndex)) == GDP_GENERIC_RSP_SUCCESS)
      {
        // Set RxOn bit to give Application time to make a request to newly configured Class device.
#ifdef ZID_IOT
        rxOn = GDP_HEADER_DATA_PENDING;
#endif

        // invoke next configuration
        osal_start_timerEx( RTI_TaskId, GDP_EVT_CONFIGURE_NEXT, aplcConfigBlackoutTime );
        // ZID adaptor is now ready.
        adaState = eAdaRdy;
//        rtrn = FALSE;  // Multiprofile pairing needs to continue, do not pass up to Application.

        if (FEATURE_ZID_ADA_TST)
        {
          cmd |= GDP_HEADER_DATA_PENDING;
//          adaState = eAdaTest;
          adaState = eAdaRdy;
          zidCfgIdx = srcIndex;
          zidRspWait(zidAda_TaskId, aplcMaxRxOnWaitTime/2);
        }
      }
      else
      {
        if (adaState == eAdaCfg)
        {
          unpairReq(srcIndex);  // Unpair with a ZID device that fails any configuration.
        }
        rtrn = TRUE;  // Cfg not complete, do not pass up to Application.
      }

      // This requires a generic response
      sendGenericResponse = TRUE;
      break;
    }

    case GDP_CMD_GENERIC_RSP:
    {
      // send to application
      break;
    }

    default:
    {
      rtrn = TRUE;  // Invalid ZID command - don't pass to the Application.
      break;
    }
  }

  if (rtrn == FALSE)
  {
    if ((adaState == eAdaCfg) && (zidCfgIdx == srcIndex))
    {
      // Note how passing the data to the Application must preceed any ZID-colayer response, since
      // Application may need to make a NV write and during the configuration phase the next push may
      // be received during such NV operations if the response is made here and RTI passes this data
      // indication up to the Application.
      // NOTE: This makes the call stack very deep - beware of C-call-stack overflow. The RNP is ok,
      //       but the Dongle sample application had to increase call stack from 0x180 to 0x200.
      RTI_ReceiveDataInd(srcIndex, RTI_PROFILE_ZID, vendorId, rxLQI, rxFlags, len, pData);
      rtrn = TRUE;
    }
    // ZID specifies to not accept messages from unconfigured devices; if so, do not pass to App.
    else if (!GET_BIT(zidPairInfo.cfgCompleteDisc, srcIndex))
    {
      rtrn = TRUE;
    }
  }

  if (sendGenericResponse == TRUE)
  {
    uint8 buf[2] = { (GDP_CMD_GENERIC_RSP | rxOn), rsp };
    sendDataReq(srcIndex, 2, buf);
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn      zidAda_AllowPairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_AllowPairReq API call.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void zidAda_AllowPairCnf(uint8 dstIndex)
{
  rcnNwkPairingEntry_t *pEntry;

  // May be allowing pairing with non-ZID (i.e. CERC profile), so check for ZID.
  if ((RCN_NlmeGetPairingEntryReq(dstIndex, &pEntry) == RCN_SUCCESS) &&  // Fail not expected here.
       GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
  {
    zidAda_nextProxyIdx = getFreeProxyTableEntry( dstIndex );

    // Not expected that allow pairing immediately after the last successful pairing such that the
    // ZID Configuration state with the last pairing did not even complete.
    if (((zidCfgIdx != RTI_INVALID_PAIRING_REF) && (zidCfgIdx != dstIndex)) ||
        (zidAda_nextProxyIdx == RTI_INVALID_PAIRING_REF) ||
        ((pCfgProxy == NULL) && ((pCfgProxy = osal_mem_alloc(sizeof(zid_proxy_entry_t)))==NULL)))
    {
      unpairReq(dstIndex);
    }
    else  // Setup for configuration state on this pairing index.
    {
      /* First initialize items that aren't pushed from class device */
      pCfgProxy->CurrentProtocol = 1;
      pCfgProxy->DeviceIdleRate = 0;

      SET_BIT(zidPairInfo.adapterDisc, dstIndex);
      CLR_BIT(zidPairInfo.cfgCompleteDisc, dstIndex);
      (void)osal_snv_write(ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo);

      // Set all bits to begin and clear bits as the corresponding Attribute is pushed.
      // Clear any non-applicable bits upon receipt of GDP_CMD_CFG_COMPLETE & check for zero.
      (void)osal_memset(cfgDiscs, 0xFF, sizeof(cfgDiscs));
      zidCommon_ResetNonStdDescCompData();
      adaState = eAdaCfg;
      zidCfgIdx = dstIndex;

      // In the first step of the configuration phase, Adapter must allow double the max wait time.
      zidRspWait(zidAda_TaskId, aplcMaxConfigWaitTime);
    }
  }
}

/**************************************************************************************************
 *
 * @fn      zidAdaUnpair
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API call or by
 *          receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  TRUE if ZID initiated the unpair; FALSE otherwise.
 */
uint8 zidAda_Unpair(uint8 dstIndex)
{
  uint8 rtrn = (adaState == eAdaUnpair) ? TRUE : FALSE;

  if (zidCfgIdx == dstIndex)
  {
    endCfgState();
  }

  /* Remove entry from proxy table */
  zidAda_RemoveFromProxyTable( dstIndex );

  return rtrn;
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
  switch (adaState)
  {
  case eAdaDor:
    break;

  case eAdaCfg:      // Configuration state with recently paired HID Class device.
    unpairReq(zidCfgIdx);
    break;

#if FEATURE_ZID_ADA_TST
  case eAdaTest:
    {
      uint8 buf[3] = {
        GDP_CMD_GET_ATTR | GDP_HEADER_DATA_PENDING,
        aplHIDParserVersion,
        aplHIDStdDescCompsList
      };

      sendDataReq(zidCfgIdx, 3, buf);
    }
    break;
#endif

  default:
    break;
  }
}

/**************************************************************************************************
 * @fn          zidAda_ReadItem
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
rStatus_t zidAda_ReadItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  if (itemId == ZID_ITEM_CURRENT_PXY_ENTRY)
  {
    status = osal_snv_read( ZID_ADA_CFG_PXY_ENTRY(zidAdaCurrentProxyIdx), sizeof(zid_proxy_entry_t), pValue );
  }
  else if (itemId == ZID_ITEM_NON_STD_DESC_FRAG)
  {
    status = osal_snv_read( ZID_ADA_NVID_DESC_START(zidAdaCurrentProxyIdx, zidAdaCurrentNonStdDescNum) + zidAdaCurrentNonStdDescFragNum,
                            ZID_NON_STD_DESC_SPEC_FRAG_T_MAX,
                            pValue );
  }
  else if (itemId == ZID_ITEM_PXY_LIST)
  {
    (void)osal_memcpy( pValue, zidAda_pxyInfoTable, sizeof(zidAda_pxyInfoTable) );
  }
  else if ((itemId >= ZID_ITEM_INT_PIPE_UNSAFE_TX_TIME) && (itemId <= ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS))
  {
    /* Cache config in RAM and ensure ZID profile version is correct */
    uint8 idx = getAttrTableIdx( itemId );
    if (idx == ZID_TABLE_IDX_INVALID)
    {
      status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
    }
    else
    {
      (void)osal_memcpy( pValue, (uint8 *)&adaConfigData + zidAda_attributeTable[idx].offset, len );
    }
  }
  else
  {
    status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
  }

  return status;
}

/**************************************************************************************************
 * @fn          zidAda_WriteItem
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
rStatus_t zidAda_WriteItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  /* Validate the item before writing it */
  if (zidCommon_specCheckWrite(itemId, len, pValue) == FALSE)
  {
    status = RTI_ERROR_INVALID_PARAMETER;
  }
  else if (itemId == ZID_ITEM_CURRENT_PXY_NUM)
  {
    uint8 temp;

    temp = getProxyTableEntry( *pValue );
    if (temp == RTI_INVALID_PAIRING_REF)
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      zidAdaCurrentProxyIdx = temp;
    }
  }
  else if (itemId == ZID_ITEM_CURRENT_PXY_ENTRY)
  {
    status = osal_snv_write( ZID_ADA_CFG_PXY_ENTRY(zidAdaCurrentProxyIdx), sizeof(zid_proxy_entry_t), pValue );
  }
  else if (itemId == ZID_ITEM_PXY_LIST)
  {
    (void)osal_memcpy( zidAda_pxyInfoTable, pValue, sizeof(zidAda_pxyInfoTable) );
    if (osal_snv_write( ZID_ADA_NVID_PXY_LIST, sizeof(zidAda_pxyInfoTable), (uint8 *)zidAda_pxyInfoTable ) != SUCCESS)
    {
      status = RTI_ERROR_OSAL_NV_OPER_FAILED;
    }
  }
  else if (itemId == ZID_ITEM_NON_STD_DESC_NUM)
  {
    if (*pValue >= aplcMaxNonStdDescCompsPerHID)
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      zidAdaCurrentNonStdDescNum = *pValue;
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
      zidAdaCurrentNonStdDescFragNum = *pValue;
    }
  }
  else if ((itemId >= ZID_ITEM_INT_PIPE_UNSAFE_TX_TIME) && (itemId <= ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS))
  {
    /* Cache config in RAM and ensure ZID profile version is correct */
    uint8 idx = getAttrTableIdx( itemId );
    if (idx == ZID_TABLE_IDX_INVALID)
    {
      status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
    }
    else
    {
      (void)osal_memcpy( (uint8 *)&adaConfigData + zidAda_attributeTable[idx].offset, pValue, len );
      if (osal_snv_write(ZID_ADA_NVID_ATTR_CFG, ZID_ADA_CFG_T_SIZE, &adaConfigData) != SUCCESS)
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
 * @fn          endCfgState
 *
 * @brief       End the Configuration state and cleanup state variables.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return  None.
 */
static void endCfgState(void)
{
  if (pCfgProxy != NULL)
  {
    osal_mem_free(pCfgProxy);
    pCfgProxy = NULL;
  }

  zidCfgIdx = RTI_INVALID_PAIRING_REF;
  zidRspDone(zidAda_TaskId, TRUE);
  adaState = eAdaDor;
}

/**************************************************************************************************
 * @fn          rxCfgComplete
 *
 * @brief       Determine if the Configuration is complete for the specified index &
 *              clear the Configuration state variables.
 *
 * input parameters
 *
 * @param       srcIndex  - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return  GDP_GENERIC_RSP_SUCCESS or GDP_GENERIC_RSP_CONFIG_FAILURE according to the cfg push received.
 */
static uint8 rxCfgComplete(uint8 srcIndex)
{
  uint8 cnt;

  if ((adaState != eAdaCfg) || (zidCfgIdx != srcIndex) || (pCfgProxy == NULL))
  {
    zidRspDone(zidAda_TaskId, FALSE);
    return GDP_GENERIC_RSP_CONFIG_FAILURE;
  }

  if (pCfgProxy->HIDNumStdDescComps != 0)
  {
    for (cnt = 0; cnt < pCfgProxy->HIDNumStdDescComps; cnt++)
    {
      // Check for uniqueness (other than repeated ZID_STD_REPORT_NONE Id's) and validity is done
      // upon receipt. Check here for the number of descriptors packed at the front of the list.
      if (pCfgProxy->HIDStdDescCompsList[cnt] == ZID_STD_REPORT_NONE)
      {
        SET_BIT(cfgDiscs, eCfgHIDNumStdDescComps);
        break;
      }
    }
  }

  /* Since we originally assume we will receive the maximum number of non-std
   * descriptor components, we need to mark off the ones above and beyond what we
   * actually received.
   */
  for (cnt = pCfgProxy->HIDNumNonStdDescComps; cnt < aplcMaxNonStdDescCompsPerHID; cnt++)
  {
    CLR_BIT(cfgDiscs, eCfgHIDNonStdDescCompSpec1+cnt);
  }

  /* Now check if anything is missing */
  for (cnt = 0; cnt < eCfgDiscCnt; cnt++)
  {
    if (GET_BIT(cfgDiscs, cnt))
    {
      return GDP_GENERIC_RSP_CONFIG_FAILURE;
    }
  }

  SET_BIT(zidPairInfo.cfgCompleteDisc, zidCfgIdx);

  /* Note -- we wouldn't have entered the configuration phase if there was no room in
   * the proxy table, so the add operation should be successful.
   */
  addToProxyTable( srcIndex );

  (void)osal_snv_write( ZID_COMMON_NVID_PAIR_INFO, sizeof(zid_pair_t), (uint8 *)&zidPairInfo );
  (void)osal_snv_write( ZID_ADA_CFG_PXY_ENTRY(zidAda_nextProxyIdx), sizeof(zid_proxy_entry_t), (uint8 *)pCfgProxy );
  (void)osal_snv_write( ZID_ADA_NVID_PXY_LIST, sizeof(zidAda_pxyInfoTable), (uint8 *)zidAda_pxyInfoTable );
  endCfgState();

  return GDP_GENERIC_RSP_SUCCESS;
}

/**************************************************************************************************
 * @fn          getFreeProxyTableEntry
 *
 * @brief       Return the number of devices in the proxy table.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return  number of entries in the proxy table.
 */
static uint8 getFreeProxyTableEntry( uint8 dstIndex )
{
  uint8 i;

  for (i = 0; i < ZID_COMMON_MAX_NUM_PROXIED_DEVICES; i++)
  {
    if ((zidAda_pxyInfoTable[i] == RTI_INVALID_PAIRING_REF) ||
        (zidAda_pxyInfoTable[i] == dstIndex))
    {
      return i;
    }
  }

  return RTI_INVALID_PAIRING_REF;
}

/**************************************************************************************************
 * @fn          addToProxyTable
 *
 * @brief       Get the index of the proxied device table corresponding to pairing index.
 *
 * input parameters
 *
 * @param       pairIndex  - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return  proxy index or RTI_INVALID_PAIRING_REF if it doesn't exist.
 */
static void addToProxyTable( uint8 pairIdx )
{

  zidAda_pxyInfoTable[zidAda_nextProxyIdx] = pairIdx;
}

/**************************************************************************************************
 * @fn          getProxyTableEntry
 *
 * @brief       Get the index of the proxied device table corresponding to pairing index.
 *
 * input parameters
 *
 * @param       pairIndex  - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return  proxy index or RTI_INVALID_PAIRING_REF if it doesn't exist.
 */
static uint8 getProxyTableEntry( uint8 pairIdx )
{
  uint8 i;

  for (i = 0; i < ZID_COMMON_MAX_NUM_PROXIED_DEVICES; i++)
  {
    if (zidAda_pxyInfoTable[i] == pairIdx)
    {
      return i;
    }
  }

  return RTI_INVALID_PAIRING_REF;
}

/**************************************************************************************************
 * @fn          zidAda_RemoveFromProxyTable
 *
 * @brief       Remove an item from the table of proxied devices.
 *
 * input parameters
 *
 * @param       pairIndex  - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return
 *
 * None.
 */
void zidAda_RemoveFromProxyTable( uint8 pairIdx )
{
  uint8 i;

  for (i = 0; i < ZID_COMMON_MAX_NUM_PROXIED_DEVICES; i++)
  {
    if (zidAda_pxyInfoTable[i] == pairIdx)
    {
      zidAda_pxyInfoTable[i] = RTI_INVALID_PAIRING_REF;
      (void)osal_snv_write( ZID_ADA_NVID_PXY_LIST, sizeof(zidAda_pxyInfoTable), (uint8 *)zidAda_pxyInfoTable );
    }
  }
}

/**************************************************************************************************
 * @fn          rxGetAdaAttr
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
static void rxGetAdaAttr(uint8 srcIndex, uint8 len, uint8 *pData)
{
  uint8 *pBuf;

  if (len < 2)
  {
    /* No attributes are being requested, so nothing to do */
    return;
  }

  /* Move data pointers and length past the GDP header */
  len--;
  pData++;

  /* Allocate buffer to hold all adaptor attributes */
  pBuf = osal_mem_alloc(GDP_ATTR_RSP_SIZE_HDR * len + sizeof(zid_ada_cfg_t) + 1);

  if (pBuf != NULL)
  {
    /* Build and send get attributes response command */
    *pBuf = GDP_CMD_GET_ATTR_RSP;
    len = procGetAdaAttr(len, pData, pBuf);
    sendDataReq(srcIndex, len, pBuf);
    osal_mem_free(pBuf);
  }
}

/**************************************************************************************************
 *
 * @fn      rxPushAdaAttr
 *
 * @brief   Receive and process a ZID_CMD_PUSH_ATTR command frame.
 *
 * @param   srcIndex  - Pairing table index of target.
 * @param   len:       Number of data bytes.
 * @param   *pData:    Pointer to data received.
 *
 * @return  GDP_GENERIC_RSP_SUCCESS or GDP_GENERIC_RSP_INVALID_PARAM according to the attributes and
 *          their values.
 */
static uint8 rxPushAdaAttr(uint8 srcIndex, uint16 len, uint8 *pData)
{
  uint8 firstAttr = TRUE;
  uint8 rtrn = GDP_GENERIC_RSP_SUCCESS;
  len--;  // Decrement for the ZID command byte expected to be ZID_CMD_PUSH_ATTR.
  pData++;

  if ((adaState == eAdaCfg) && ((zidCfgIdx != srcIndex) || (pCfgProxy == NULL)))
  {
    return GDP_GENERIC_RSP_INVALID_PARAM;
  }

  while ((len != 0) && (rtrn == GDP_GENERIC_RSP_SUCCESS))
  {
    uint8 attrId = *pData++;
    uint8 idx = getPxyTableIdx( attrId );
    uint8 attrLen;

    attrLen = GDP_PUSH_ATTR_REC_SIZE_HDR;

    /* If not in config state, can't push aplHID attributes */
    if (((adaState != eAdaCfg) && (attrId >= aplHIDParserVersion)) ||
        (len < attrLen))
    {
      rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
      break;
    }
    len -= attrLen;

    if (idx == ZID_TABLE_IDX_INVALID)
    {
      /* Either a non standard descriptor or unsupported attribute */
      if ((attrId >= aplHIDNonStdDescCompSpec1) &&
          (attrId < (aplHIDNonStdDescCompSpec1 + aplcMaxNonStdDescCompsPerHID)) && (*pData++ == len))
      {
        // Exactly one non-std DescSpec shall be sent per Attribute Request Response or Attribute Push.
        if (!firstAttr)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          bool descComplete;

          rtrn = zidCommon_ProcessNonStdDescComp( len, pData, &descComplete );
          if (rtrn == GDP_GENERIC_RSP_SUCCESS)
          {
            /* Store fragment in NV */
            uint8 id = ZID_ADA_NVID_DESC_START(zidAda_nextProxyIdx, attrId - aplHIDNonStdDescCompSpec1) + zidCommon_GetLastNonStdDescCompFragNum();
            (void)osal_snv_write( id, len, pData );
          }

          if (descComplete == TRUE)
          {
            /* We've either received a complete descriptor, or all fragments of a fragmented
             * descriptor have been received. Mark as complete.
             */
            CLR_BIT(cfgDiscs, eCfgHIDNonStdDescCompSpec1+(attrId - aplHIDNonStdDescCompSpec1));

            /* Reset descriptor processing data in case more are pushed */
            zidCommon_ResetNonStdDescCompData();
          }
          else if (rtrn != GDP_GENERIC_RSP_SUCCESS)
          {
            /* We found a problem with the non std descriptor component, so reset processing data */
            zidCommon_ResetNonStdDescCompData();
          }
        }
      }
      else if (attrId != aplZIDProfileVersion) // can technically be pushed, although it doesn't make sense. Just accept it.
      {
        rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
      }

      break;
    }
    firstAttr = FALSE;

    if ((attrLen = *pData++) != zidAda_proxyAttributeTable[idx].attrLen)
    {
      // Allow an aplHIDStdDescCompsList to be less than or equal to the specified maximum.
      if ((attrId != aplHIDStdDescCompsList) || (attrLen > zidAda_proxyAttributeTable[idx].attrLen))
      {
        rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        break;
      }
    }

    switch (attrId)
    {
      case aplHIDParserVersion:
      {
        pCfgProxy->HIDParserVersion = BUILD_UINT16(pData[0], pData[1]);
        CLR_BIT(cfgDiscs, eCfgHIDParserVersion);
        break;
      }

      case aplHIDDeviceSubclass:
      {
        if (*pData > ZID_DEVICE_SUBCLASS_BOOT)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDDeviceSubclass = *pData;
        }
        break;
      }

      case aplHIDProtocolCode:
      {
        if (*pData > ZID_PROTOCOL_CODE_MOUSE)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDProtocolCode = *pData;
        }
        break;
      }

      case aplHIDCountryCode:
      {
        pCfgProxy->HIDCountryCode = *pData;
        CLR_BIT(cfgDiscs, eCfgHIDCountryCode);
        break;
      }

      case aplHIDDeviceReleaseNumber:
      {
        pCfgProxy->HIDDeviceReleaseNumber = BUILD_UINT16(pData[0], pData[1]);
        CLR_BIT(cfgDiscs, eCfgHIDDeviceReleaseNumber);
        break;
      }

      case aplHIDVendorId:
      {
        pCfgProxy->HIDVendorId = BUILD_UINT16(pData[0], pData[1]);
        CLR_BIT(cfgDiscs, eCfgHIDVendorId);
        break;
      }

      case aplHIDProductId:
      {
        pCfgProxy->HIDProductId = BUILD_UINT16(pData[0], pData[1]);
        CLR_BIT(cfgDiscs, eCfgHIDProductId);
        break;
      }

      case aplHIDNumEndpoints:
      {
        if ((*pData < ZID_NUM_ENDPOINTS_MIN) || (*pData > ZID_NUM_ENDPOINTS_MAX))
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDNumEndpoints = *pData;
          CLR_BIT(cfgDiscs, eCfgHIDNumEndpoints);
        }
        break;
      }

      case aplHIDPollInterval:
      {
        if ((*pData < ZID_POLL_INTERVAL_MIN) || (*pData > ZID_POLL_INTERVAL_MAX))
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDPollInterval = *pData;
          CLR_BIT(cfgDiscs, eCfgHIDPollInterval);
        }
        break;
      }

      case aplHIDNumStdDescComps:
      {
        if  (*pData > aplcMaxStdDescCompsPerHID)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDNumStdDescComps = *pData;
          CLR_BIT(cfgDiscs, eCfgHIDNumStdDescComps);
        }
        break;
      }

      case aplHIDStdDescCompsList:
      {
        if (stdDescCheck(attrLen, pData) == FALSE)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          (void)osal_memcpy(pCfgProxy->HIDStdDescCompsList, pData, attrLen);
        }
        break;
      }

      case aplHIDNumNonStdDescComps:
      {
        if  (*pData > aplcMaxNonStdDescCompsPerHID)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDNumNonStdDescComps = *pData;
          CLR_BIT(cfgDiscs, eCfgHIDNumNonStdDescComps);
        }
        break;
      }

      case aplHIDNumNullReports:
      {
        if  (*pData > aplcMaxNonStdDescCompsPerHID)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        else
        {
          pCfgProxy->HIDNumNullReports = *pData;
          CLR_BIT(cfgDiscs, eCfgHIDNumNullReports);
        }
        break;
      }

      default:
      {
        rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        break;
      }
    }

    if (len < attrLen)
    {
      rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
      break;
    }

    len -= attrLen;
    pData += attrLen;
  }

  return rtrn;
}

/**************************************************************************************************
 *
 * @fn      rxSetReport
 *
 * @brief   Receive and process a ZID_CMD_SET_REPORT command frame.
 *
 * @param   srcIndex  - Pairing table index of target.
 * @param   len       - Number of data bytes.
 * @param   *pData    - Pointer to data received.
 *
 * @return  GDP_GENERIC_RSP_SUCCESS or GDP_GENERIC_RSP_INVALID_PARAM according to the attributes and
 *          their values.
 */
static uint8 rxSetReport( uint16 len, uint8 *pData )
{
  uint8 rtrn = GDP_GENERIC_RSP_SUCCESS, reportLen;
  zid_set_report_cmd_t *pReport = (zid_set_report_cmd_t *)pData;
  uint8 descNum;

  reportLen = len - sizeof(zid_set_report_cmd_t);

  if ((adaState != eAdaCfg) || (pReport->type != ZID_REPORT_TYPE_IN) || (reportLen > aplcMaxNullReportSize))
  {
    rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
  }
  else if (findNonStdDescReport( zidAda_nextProxyIdx, pReport->id, &descNum ) == FALSE)
  {
    /* reportId doesn't correspond to received non-std descriptor component */
    rtrn = ZID_GENERIC_RSP_INVALID_REPORT_ID;
  }
  else
  {
    uint8 buf[ZID_NULL_REPORT_T_MAX];
    zid_null_report_t *pBuf = (zid_null_report_t *)buf;
    uint8 nvId;

    /* passed error checking, so go ahead and store it in NVM */
    pBuf->reportId = pReport->id;
    pBuf->len = reportLen;
    osal_memcpy( pBuf->data, pReport->data, reportLen );
    nvId = ZID_ADA_NVID_NULL_REPORT_START( zidAda_nextProxyIdx, descNum );
    (void)osal_snv_write( nvId, sizeof(zid_null_report_t) + reportLen, pBuf );
  }

  return rtrn;
}

/**************************************************************************************************
 *
 * @fn      findNonStdDescReport
 *
 * @brief   Searches received non-std descriptor components for a report containing a specific report ID
 *
 * @param   reportId  - report to look for
 *
 * @return  TRUE if match was found, FALSE otherwise.
 */
static bool findNonStdDescReport( uint8 proxyIdx, uint8 reportId, uint8 *descId )
{
  uint8 descNum;

  for (descNum = 0; descNum < pCfgProxy->HIDNumNonStdDescComps; descNum++)
  {
    uint8 nvId = ZID_ADA_NVID_DESC_START(proxyIdx, descNum);
    zid_non_std_desc_comp_t buf;
    if (osal_snv_read( nvId, sizeof(zid_non_std_desc_comp_t), &buf ) == SUCCESS)
    {
      if (buf.reportId == reportId)
      {
        *descId = descNum;
        return( TRUE );
      }
    }
  }

  return( FALSE );
}

/**************************************************************************************************
 * @fn          sendDataReq
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
static void sendDataReq(uint8 dstIndex, uint8 len, uint8 *pData)
{
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
  uint16 vendorId = RTI_VENDOR_TEXAS_INSTRUMENTS;

  (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_ID, sizeof(uint16), (uint8 *)&vendorId);

  ZID_SET_TXING();
  RTI_SendDataReq(dstIndex, RTI_PROFILE_ZID, vendorId, txOptions, len, pData);
}

/**************************************************************************************************
 * @fn          procGetAdaAttr
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
static uint8 procGetAdaAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf)
{
  uint8 *pBeg = pBuf, *pCfg;

  /* Move past the GDP header */
  pBuf++;

  /* Allocate memory to read adaptor attributes from NVM */
  if ((pCfg = osal_mem_alloc( sizeof(zid_ada_cfg_t) )) == NULL)
  {
    return 0;
  }

  /* Read adaptor config into allocated buffer */
  if (osal_snv_read( ZID_ADA_NVID_ATTR_CFG, ZID_ADA_CFG_T_SIZE, pCfg ) != SUCCESS)
  {
    return 0;
  }

  while (cnt--)
  {
    uint8 id, idx, len;

    /* Get attribute ID from request buffer */
    id = *pReq++;

    /* Lookup attribute in table of adaptor IDs */
    idx = getAttrTableIdx( id );

    /* Store attribute ID in get attribute response command buffer */
    *pBuf++ = id;

    if (idx == ZID_TABLE_IDX_INVALID)
    {
      *pBuf++ = GDP_ATTR_RSP_UNSUPPORTED;
      continue;  // Done with response for this unsupported attribute, goto parse the next one.
    }
    else if ((id >= aplHIDParserVersion) && (adaState != eAdaCfg))
    {
      /* Any aplHID attribute can only be requested during config state */
      *pBuf++ = GDP_ATTR_RSP_ILLEGAL_REQ;
      continue;  // Done with response for this unsupported attribute, goto parse the next one.
    }
    else
    {
      *pBuf++ = GDP_ATTR_RSP_SUCCESS;
    }

    *pBuf++ = len = zidAda_attributeTable[idx].attrLen;
    pBuf = osal_memcpy(pBuf, (pCfg + zidAda_attributeTable[idx].offset), len);
  }
  osal_mem_free(pCfg);

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
  adaState = eAdaDor;
  if (dstIndex != RTI_INVALID_PAIRING_REF)
  {
    if (zidCfgIdx == dstIndex)
    {
      endCfgState();
    }
    adaState = eAdaUnpair;
//    RTI_UnpairReq(dstIndex); // Unpair request handled by multiple profile configuration
  }
  // Notify the multiple profile configuration
  osal_set_event( RTI_TaskId, GDP_EVT_CONFIGURATION_FAILED );
}

/**************************************************************************************************
 * @fn          stdDescCheck
 *
 * @brief       Check a list of standard descriptors for uniqueness and correctness against
 *              the list of standard descriptors.
 *
 * input parameters
 *
 * @param       len       - Number of bytes to check.
 * @param       *pData    - Pointer to data to be checked.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if pData is a list of 'len' number of valid and unique standard descriptors;
 *              FALSE otherwise.
 */
static uint8 stdDescCheck(uint8 len, uint8 *pData)
{
  uint8 idx, stdDescList[aplcMaxStdDescCompsPerHID];

  for (idx = 0; idx < aplcMaxStdDescCompsPerHID; idx++)
  {
    stdDescList[idx] = idx+1;
  }

  if (len <= aplcMaxStdDescCompsPerHID)
  {
    while (len)
    {
      for (idx = 0; idx < aplcMaxStdDescCompsPerHID; idx++)
      {
        if (stdDescList[idx] == *pData)
        {
          // Invalidating the compare value checks the input list for duplicates.
          stdDescList[idx] = ZID_STD_REPORT_NONE;
          break;
        }
      }

      if (idx == aplcMaxStdDescCompsPerHID)
      {
        break;
      }

      pData++;
      len--;
    }
  }

  return ((len == 0) ? TRUE : FALSE);
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
static uint8 getAttrTableIdx( uint8 attrId )
{
  uint8 idx;

  for (idx = 0; idx < ZID_ADA_ATTRIBUTE_TABLE_LEN; idx++)
  {
    if (zidAda_attributeTable[idx].attrId == attrId)
    {
      return idx;
    }
  }

  return ZID_TABLE_IDX_INVALID;
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
static uint8 getPxyTableIdx( uint8 attrId )
{
  uint8 idx;

  for (idx = 0; idx < ZID_ADA_PROXY_ATTRIBUTE_TABLE_LEN; idx++)
  {
    if (zidAda_proxyAttributeTable[idx].attrId == attrId)
    {
      return idx;
    }
  }

  return ZID_TABLE_IDX_INVALID;
}

/**************************************************************************************************
*/
