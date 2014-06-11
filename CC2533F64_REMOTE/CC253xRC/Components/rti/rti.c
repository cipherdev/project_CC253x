/**************************************************************************************************
  Filename:       rti.c
  Revised:        $Date: 2012-05-21 12:08:34 -0700 (Mon, 21 May 2012) $
  Revision:       $Revision: 30577 $

  Description:    This file contains the the RemoTI (RTI) API.


  Copyright 2008-2012 Texas Instruments Incorporated. All rights reserved.

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

/* HAL includes */
#include "hal_assert.h"
#include "hal_led.h"
#include "hal_flash.h"

/* OAD includes */
#if defined FEATURE_OAD
#include "oad.h"
#endif

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"

/* RCN includes */
#include "rcn_nwk.h"

/* RTI includes */
#include "rti_constants.h"
#include "rti.h"

/* GDP includes */
#include "gdp_profile.h"

/* ZID includes */
#include "zid.h"
#if FEATURE_ZID_ADA
#include "zid_adaptor.h"
#endif
#if FEATURE_ZID_CLD
#include "zid_class_device.h"
#endif
#if FEATURE_ZID
#include "gdp.h"
#include "zid_common.h"
#include "zid_profile.h"
#endif

/* Z3D includes */
#include "z3d.h"
#if (FEATURE_Z3D_CTL || FEATURE_Z3D_TGT)
#include "z3d_common.h"
#endif

/**************************************************************************************************
 *                                            Macros
 */
// Macro to read commissioned IEEE address
// It is platform dependent as HalFlash API is available only in certain
// platforms
#if (defined HAL_MCU_CC2530 || defined HAL_MCU_CC2531 || defined HAL_MCU_CC2533)
# define RTI_READ_COMMISSIONED_IEEE_ADDR(pBuf) \
  st( HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, pBuf, HAL_FLASH_IEEE_SIZE); )
#elif (defined HAL_MCU_MSP430)
# define RTI_READ_COMMISSIONED_IEEE_ADDR(pBuf) \
  st( (void)osal_memcpy((pBuf), (uint8 *)HAL_NV_IEEE_ADDR, SADDR_EXT_LEN); )
#elif (defined HAL_NV_IEEE_PAGE)
// A platform that support OSAL_NV for commissioned IEEE address reading
extern __near_func void nvReadBuf(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);
# define RTI_READ_COMMISSIONED_IEEE_ADDR(pBuf) \
  nvReadBuf(HAL_NV_IEEE_PAGE, (HAL_NV_PAGE_SIZE-SADDR_EXT_LEN), pBuf, SADDR_EXT_LEN)
#else
# warning New MCU requires specific implementation of RTI_READ_COMMISSIONED_IEEE_ADDR macro.
#endif

/**************************************************************************************************
 *                                         Constants
 */

//efine SYS_EVENT_MSG              0x8000
// Self-scheduled event to start the RCN out of the client's task context.
#define RTI_EVT_RCN_START_REQ      0x4000
// The time out event is used for accepting pair request command once auto discovery succeeds.
#define RTI_EVT_ALLOW_PAIR_TIMEOUT 0x2000
// Implement a timer event for ZID so that Adapter-only does not need to instantiate its own task.
//efine ZID_ADA_EVT_RSP_WAIT       0x1000

// NV Ids
#ifdef OSAL_SNV_UINT16_ID
/* It must be using non-compact regular NV */
# define RTI_NVID_BOOT_FLAG       0x0301
# define RTI_NVID_STARTUP_CONTROL 0x0302
# define RTI_NVID_SUPPORTED_TGTS  0x0303
# define RTI_NVID_APP_CAP         0x0304
# define RTI_NVID_DEVTYPE_LIST    0x0305
# define RTI_NVID_PROFILE_LIST    0x0306
# define RTI_NVID_ACTIVE_PERIOD   0x0307
# define RTI_NVID_DUTY_CYCLE      0x0308
# define RTI_NVID_DISC_DURATION   0x0309
# define RTI_NVID_DISC_LQI_THR    0x030A
#else
# if RCN_CAP_PAIR_TABLE_SIZE > 20
#  error "OSAL SNV interface constraint prohibits so many pairing entries"
# endif
# define RTI_NVID_BOOT_FLAG       0x70
# define RTI_NVID_STARTUP_CONTROL 0x71
# define RTI_NVID_SUPPORTED_TGTS  0x72
# define RTI_NVID_APP_CAP         0x73
# define RTI_NVID_DEVTYPE_LIST    0x74
# define RTI_NVID_PROFILE_LIST    0x75
# define RTI_NVID_ACTIVE_PERIOD   0x76
# define RTI_NVID_DUTY_CYCLE      0x77
# define RTI_NVID_DISC_DURATION   0x78
# define RTI_NVID_DISC_LQI_THR    0x79
#endif

// Cold boot or warm boot enumeration
#define RTI_COLD_BOOT 0
#define RTI_WARM_BOOT 1

#if FEATURE_ZID
#define RTI_ALLOW_PAIR_TIMEOUT_DUR  aplcGdpMaxPairIndicationWaitTime

#if FEATURE_ZID_ADA
#define RTI_CONST_RNP_IMAGE_ID      0x02  // ZID ADA-only in RNP.
#else
#define RTI_CONST_RNP_IMAGE_ID      0x01  // ZID CLD-only in RNP.
#endif
#else
#define RTI_CONST_RNP_IMAGE_ID      0x00  // No ZID in RNP.
#define RTI_ALLOW_PAIR_TIMEOUT_DUR  aplcGdpMaxPairIndicationWaitTime
#endif

/**************************************************************************************************
 *                                        Type definitions
 */

// generic function pointer for osal_snv_read and osal_snv_write
typedef uint8 (*nv_rw_func_t)(osalSnvId_t, osalSnvLen_t, void *);

typedef enum
{
  RTI_STATE_START,
  RTI_STATE_READY,
  RTI_STATE_DISCOVERY,
  RTI_STATE_DISCOVERED,
  RTI_STATE_DISCOVERY_ERROR,
  RTI_STATE_DISCOVERY_ABORT,
  RTI_STATE_PAIR,
  RTI_STATE_NDATA,
  RTI_STATE_UNPAIR,
  RTI_STATE_CONFIGURATION
} rtiState_t;

/**************************************************************************************************
 *                                        Global Variables
 */

uint8 RTI_TaskId;                 // Task ID
rcnReqRspPrim_t rtiReqRspPrim;    // common storage for request and response primitive
configParams_s configParamTable;  // Configuration Parameter Table

#if !defined FEATURE_CONTROLLER_ONLY
#if !defined ZRC_CMD_MATRIX  // If Application does not override the default command matrix.
#define ZRC_CMD_MATRIX       // Define the minimum, mandatory command matrix for a TV here.
static const uint8 ZrcCmdMatrix[32] = {
  0x1F, 0x22, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif
#endif

/**************************************************************************************************
 *                                        Local Variables
 */

static rtiState_t rtiState;

// State Attributes Table
static stateAttribs_s stateAttribTable;

// storage to cache discovered target's node descriptor
static __no_init rcnNlmeDiscoveredEvent_t rtiDiscoveredEventCache;

// whether pairing is allowed or not
// This flag is set to TRUE or FALSE as a result of network layer auto discovery procedure
static uint8 allowPairFlag;

#if (defined FEATURE_ZID_ADA && FEATURE_ZID_ADA)
static osalSnvLen_t itemsToStoreInNV[] = {
  sizeof(rcnNwkPairingEntry_t),
  sizeof(zid_pair_t),
  sizeof(zid_proxy_entry_t),
  RCN_CAP_PAIR_TABLE_SIZE, //sizeof(zidAda_pxyInfoTable)
  sizeof(zid_pair_t)
};
#elif (defined FEATURE_ZID_CLD && FEATURE_ZID_CLD)
static osalSnvLen_t itemsToStoreInNV[] = {
  sizeof(rcnNwkPairingEntry_t),
  sizeof(zid_pair_t)
};
#else // Only ZRC
static osalSnvLen_t itemsToStoreInNV[] = {
  sizeof(rcnNwkPairingEntry_t)
};
#endif //defined FEATURE_ZID_ADA

// RCN callback function handler which should handle all RCN callback events
// instead of RTI itself in case RTI is set to bridge mode
static rtiRcnCbackFn_t pRtiBridgeHandler = NULL;

// Volatile buffer just to store the NIB attribute value temporarily
#if (defined HAL_MCU_MSP430)
static __no_init uint16 pRtiNibBuf[RCN_MAX_ATTRIB_SIZE_EXCEPT_PAIR_TBL/2];
#else
static __no_init uint8 pRtiNibBuf[RCN_MAX_ATTRIB_SIZE_EXCEPT_PAIR_TBL];
#endif

// Volatile buffer to hold Configuration phase parameters:
static gdpCfgParam_t gdpCfgParam = {
// current pairing index for multiple profile configuration
  RTI_INVALID_PAIRING_REF,
// previously configured profile
  RTI_PROFILE_GDP,
// configuration invoked by; RTI_AllowPairCnf or RTI_PairCnf
  GDP_CFG_INVOKED_BY_INVALID
};

// Volatile buffer to keep allow pair confirm parameter while NLME-COMM-STATUS.indication
// is pending for pair response and security link key exchange.
static __no_init struct
{
  uint8 pairingRef;
  uint8 devType;
} rtiAllowPairCnfParams;

// RTI configuration parameter storage information
static const struct
{
  osalSnvId_t nvid;
  uint8 size;
  uint8 *pCache;
} rtiCpStorage[] =
{
  // RTI_CP_ITEM_STARTUP_CTRL
  { RTI_NVID_STARTUP_CONTROL, sizeof(configParamTable.startupCtrl),
  &configParamTable.startupCtrl },
  // RTI_CP_ITEM_NODE_CAPABILITIES
  { RCN_NVID_NWKC_NODE_CAPABILITIES, sizeof(configParamTable.nodeCapabilities),
  &configParamTable.nodeCapabilities },
  // RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES
  { RTI_NVID_SUPPORTED_TGTS, sizeof(configParamTable.nodeCap),
  (uint8 *) &configParamTable.nodeCap },
  // RTI_CP_ITEM_APPL_CAPABILITIES
  { RTI_NVID_APP_CAP, sizeof(configParamTable.appCap.appCapabilities),
  &configParamTable.appCap.appCapabilities },
  // RTI_CP_ITEM_APPL_DEV_TYPE_LIST
  { RTI_NVID_DEVTYPE_LIST, sizeof(configParamTable.appCap.devTypeList),
  configParamTable.appCap.devTypeList },
  // RTI_CP_ITEM_APPL_PROFILE_ID_LIST
  { RTI_NVID_PROFILE_LIST, sizeof(configParamTable.appCap.profileIdList),
  configParamTable.appCap.profileIdList },
  // RTI_CP_ITEM_STDBY_DEFAULT_ACTIVE_PERIOD
  { RTI_NVID_ACTIVE_PERIOD, sizeof(configParamTable.stdByInfo.standbyActivePeriod),
  (uint8 *) &configParamTable.stdByInfo.standbyActivePeriod },
  // RTI_CP_ITEM_STDBY_DEFAULT_DUTY_CYCLE
  { RTI_NVID_DUTY_CYCLE, sizeof(configParamTable.stdByInfo.standbyDutyCycle),
  (uint8 *) &configParamTable.stdByInfo.standbyDutyCycle },
  // RTI_CP_ITEM_VENDOR_ID
  { RCN_NVID_NWKC_VENDOR_IDENTIFIER, sizeof(configParamTable.vendorInfo.vendorId),
  (uint8 *) &configParamTable.vendorInfo.vendorId },
  // RTI_CP_ITEM_VENDOR_NAME
  { RCN_NVID_NWKC_VENDOR_STRING, sizeof(configParamTable.vendorInfo.vendorName),
  configParamTable.vendorInfo.vendorName },
  // RTI_CP_ITEM_DISCOVERY_DURATION
  { RTI_NVID_DISC_DURATION, sizeof(configParamTable.discoveryInfo.discDuration),
  (uint8 *) &configParamTable.discoveryInfo.discDuration },
  // RTI_CP_ITEM_DEFAULT_DISCOVERY_LQI_THRESHOLD
  { RTI_NVID_DISC_LQI_THR, sizeof(configParamTable.discoveryInfo.discLQIThres),
  &configParamTable.discoveryInfo.discLQIThres }
};

static uint8 dppKeyTransferCnt;

/**************************************************************************************************
 *                                     Local Function Prototypes
 */

static rStatus_t rtiReadItem(uint8 itemId, uint8 len, uint8 *pValue);
static rStatus_t rtiWriteItem(uint8 itemId, uint8 len, uint8 *pValue);

// Callback Related
void         RCN_CbackEvent( rcnCbackEvent_t *pData );
static void  rtiOnNlmeCommStatusInd(rcnCbackEvent_t *pData);
static void  rtiOnNlmeDiscoveredEvent(rcnCbackEvent_t *pData);
static void  rtiOnNlmeDiscoveryCnf(rcnCbackEvent_t *pData);
static void  rtiOnNlmeDiscoveryAbortCnf(void);
static void  rtiOnNlmeAutoDiscoveryCnf(rcnCbackEvent_t *pData);
static void  rtiOnNlmePairCnf(rcnCbackEvent_t *pData);
#ifndef FEATURE_CONTROLLER_ONLY
static void  rtiOnNlmePairInd(rcnCbackEvent_t *pData);
#endif
static void  rtiOnNlmeUnpairInd(rcnCbackEvent_t *pData);
static void  rtiOnNldeDataCnf(rcnCbackEvent_t *pData);
static void  rtiOnNldeDataInd(rcnCbackEvent_t *pData);
static void  rtiOnNlmeUnpairCnf(rcnCbackEvent_t *pData);

// Local Functions
static void      rtiWriteNV( void );
static void      rtiReadNV( void );
static void      rtiReadWriteNV( nv_rw_func_t nv_rw_func );
static uint8     rtiReadWriteItem( uint8 itemId, uint8 len, uint8 *pValue, nv_rw_func_t nv_rw_func );
static void      rtiResetCPinRAM( void );
static void      rtiResetSA( void );
static void      rtiProgramIeeeAddr( void );

// Multiple profile configuration
static void rtiConfigureNextProfile();

/**************************************************************************************************
 *
 * @fn          RTI_Init
 *
 * @brief       This is the RemoTI task initialization called by OSAL. The
 *              Configuration Parameters (CP) table and State Attributes tables
 *              are set to their default values in RAM, the NV memory is
 *              initialized (but note that it is not updated if previously
 *              written), and a boot flag in NV is checked. If this is a first
 *              time boot (factory programmed or cold reset), then update
 *              CP table in NV memory with default values. Otherwise, read CP
 *              table from NV memory.
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
void RTI_Init( uint8 taskId )
{
  uint8 rtiBootFlag;

  // save task ID assigned by OSAL
  RTI_TaskId = taskId;

  // reset Configuration Parameter table to its default setting in RAM
  rtiResetCPinRAM();

  // Program commissioned IEEE address if such address is valid
  rtiProgramIeeeAddr();

  // check NV boot flag
  rtiBootFlag = RTI_COLD_BOOT;
  osal_snv_read( RTI_NVID_BOOT_FLAG, 1, &rtiBootFlag );

  // check if this is a first time boot
  if ( rtiBootFlag == RTI_COLD_BOOT )
  {
    // flash was re-imaged at load time, and this is the first time we are
    // booting, so set the Configuration Parameter table in NV to its default
    // settings

    // update the NV with the default Configuration Parameter table
    rtiWriteNV();

    // set the boot flag so we don't hammer the NV on subsequent resets
    rtiBootFlag = RTI_WARM_BOOT;
    osal_snv_write(RTI_NVID_BOOT_FLAG, 1, &rtiBootFlag);
  }
  else // this is a warm reset
  {
    // CP table remains as is in NV, so read it to RAM for RTI use
    rtiReadNV();
  }

  // Reset allow pairing related variables.
  allowPairFlag = FALSE;
  rtiAllowPairCnfParams.pairingRef = RCN_INVALID_PAIRING_REF;

  if (FEATURE_GDP)  gdpInitItems();
  if (FEATURE_ZID)  zidInitItems();

#if defined( POWER_SAVING )
  // startup with power management disabled until a RTI_InitReq is issued;
  // this is required as the MAC will cause an immediate assert if its
  // low level init (MAC_MlmeResetReq) hasn't been called by the RCN
  // (RCN_NlmeResetReq), which won't happen until the client performs a
  // RTI_InitReq
  osal_pwrmgr_task_state( RTI_TaskId, PWRMGR_HOLD );
#endif
}

/**************************************************************************************************
 *
 * @fn          RTI_ProcessEvent
 *
 * @brief       This routine handles RTI task events.
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
uint16 RTI_ProcessEvent( uint8 taskId, uint16 events )
{
  (void)taskId;

  if (events & RTI_EVT_ALLOW_PAIR_TIMEOUT)
  {
    if (allowPairFlag)  // To handle race condition, allowPairFlag has to be checked.
    {
      allowPairFlag = FALSE;
      RTI_AllowPairCnf(RTI_ERROR_ALLOW_PAIRING_TIMEOUT, RTI_INVALID_PAIRING_REF,
                                                        RTI_INVALID_DEVICE_TYPE);
      if (FEATURE_Z3D_TGT)  z3dTgtAllowPairCnf();
    }

    return (events ^ RTI_EVT_ALLOW_PAIR_TIMEOUT);
  }

  if (events & RTI_EVT_RCN_START_REQ)
  {
    RCN_NlmeResetReq(TRUE);  // Reset network layer and clear NIB.

    // Reset state attributes whose default values could be different from
    // network layer spec defined default values
    rtiResetSA();

    // Start the RCN; the RCN_NLME_START_CNF confirm callback is generated to RTI when done,
    // which results in the RTI_InitCnf() callback to the Application.
    RCN_NlmeStartReq();

    return (events ^ RTI_EVT_RCN_START_REQ);
  }

  if (events == GDP_EVT_CONFIGURE_NEXT)
  {
    gdpCfgParam.prevConfiguredProfile++;
    rtiConfigureNextProfile();
    return ( events ^ GDP_EVT_CONFIGURE_NEXT );
  }

  if (events == GDP_EVT_CONFIGURATION_FAILED)
  {
    // Unpair and send an unsuccessful pair response to Application
    RTI_UnpairReq(gdpCfgParam.pairingRef);  // Unpair with a device that fails any configuration.
    rtiState = RTI_STATE_READY;
    if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_ALLOW_PAIR)
    {
      RTI_AllowPairCnf( (RTI_ERROR_FAILED_TO_CONFIGURE_INV_MASK | (gdpCfgParam.prevConfiguredProfile && 0xF)), RTI_INVALID_PAIRING_REF,
                      RTI_INVALID_PAIRING_REF );
    }
    else if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_PAIR)
    {
      RTI_PairCnf( (RTI_ERROR_FAILED_TO_CONFIGURE_INV_MASK | (gdpCfgParam.prevConfiguredProfile && 0xF)), RTI_INVALID_PAIRING_REF,
                      RTI_INVALID_PAIRING_REF );
    }
    return ( events ^ GDP_EVT_CONFIGURATION_FAILED );
  }

  return ( 0 );  // Discard unknown events.
}

/**************************************************************************************************
 *
 * @fn          rtiReadItem
 *
 * @brief       This API is used to read the RTI Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table.
 *
 * input parameters
 *
 * @param       itemId - The Configuration Interface item identifier.
 * @param       len    - The length in bytes of the item identifier's data.
 *
 * output parameters
 *
 * @param       *pValue - A pointer to the item identifier's data from table.
 *
 * @return      RTI_SUCCESS, RTI_ERROR_NOT_PERMITTED, RTI_ERROR_INVALID_INDEX,
 *              RTI_ERROR_INVALID_PARAMETER, RTI_ERROR_UNKNOWN_PARAMETER,
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE, RTI_ERROR_OSAL_NV_OPER_FAILED,
 *              RTI_ERROR_OSAL_NV_ITEM_UNINIT, RTI_ERROR_OSAL_NV_BAD_ITEM_LEN
 */
static rStatus_t rtiReadItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  if (itemId <= RCN_NIB_NWK_RANGE_END)  // If item is a State attribute (i.e. NIB attribute).
  {
    if (itemId == RCN_NIB_NWK_PAIRING_TABLE)
    {
      // The pairing table should be accessed thru RTI_SA_ITEM_PT_CURRENT_ENTRY.
      status = RTI_ERROR_NOT_PERMITTED;
    }
    else
    {
      status = RCN_NlmeGetReq(itemId, 0, (uint8 *)pRtiNibBuf);
      (void)osal_memcpy(pValue, pRtiNibBuf, len);
    }
  }
  else if (itemId < RTI_CP_ITEM_END)
  {
    // Then all operations are identical except for whether it is a read or write.
    status = rtiReadWriteItem( itemId, len, pValue, osal_snv_read );
  }
  else
  {
    switch (itemId )
    {
    case RTI_SA_ITEM_PT_NUMBER_OF_ACTIVE_ENTRIES:
      {
        uint8 cnt = 0;

        for (uint8 i = 0; i < RCN_CAP_MAX_PAIRS; i++)
        {
          rcnNwkPairingEntry_t *pEntry;

          if (RCN_NlmeGetPairingEntryReq(i, &pEntry) == RCN_SUCCESS)
          {
            cnt++;
          }
        }

        *pValue = cnt;
      }
      break;

    case RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX:
      *pValue = stateAttribTable.curPairTableIndex;
      break;

    case RTI_SA_ITEM_PT_CURRENT_ENTRY:
      {
        rcnNwkPairingEntry_t *pEntry;

        // get the pairing table entry given the currently set index
        if ((status = RCN_NlmeGetPairingEntryReq(stateAttribTable.curPairTableIndex, &pEntry))
                                                                               == RCN_SUCCESS)
        {
          // No endianness housekeeping here; change should happen on other side (in app processor).
          (void)osal_memcpy(pValue, pEntry, len);
        }
      }
      break;

    case RTI_CONST_ITEM_SW_VERSION:
      *pValue = RTI_CONST_SW_VERSION;
      break;

    case RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES:
      *pValue = RCN_CAP_MAX_PAIRS;
      break;

    case RTI_CONST_ITEM_NWK_PROTOCOL_IDENTIFIER:
      *pValue = RTI_CONST_NWK_PROTOCOL_IDENTIFIER;
      break;

    case RTI_CONST_ITEM_NWK_PROTOCOL_VERSION:
      *pValue = RTI_CONST_NWK_PROTOCOL_VERSION;
      break;

#if defined FEATURE_OAD
    case RTI_CONST_ITEM_OAD_IMAGE_ID:
      {
        preamble_t info;
        OAD_GetPreamble(OAD_OP_IMAGE, &info);
        (void)osal_memcpy(pValue, (uint8 *)&info.imgID, sizeof(uint32));
      }
      break;
#endif

    case RTI_CONST_ITEM_RNP_IMAGE_ID:
      *pValue = RTI_CONST_RNP_IMAGE_ID;
      break;

    default:
      status = RTI_ERROR_INVALID_PARAMETER;
      break;
    }
  }

  return status;
}

/**************************************************************************************************
 *
 * @fn          RTI_ReadItemEx
 *
 * @brief       This API is used to read the RTI Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table for any supported profile.
 *
 * input parameters
 *
 * @param       profileId - The Profile identifier.
 * @param       itemId - The Configuration Interface item identifier.
 * @param       len    - The length in bytes of the item identifier's data.
 *
 * output parameters
 *
 * @param       *pValue - A pointer to the item identifier's data from table.
 *
 * @return      RTI_SUCCESS, RTI_ERROR_NOT_PERMITTED, RTI_ERROR_INVALID_INDEX,
 *              RTI_ERROR_INVALID_PARAMETER, RTI_ERROR_UNKNOWN_PARAMETER,
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE, RTI_ERROR_OSAL_NV_OPER_FAILED,
 *              RTI_ERROR_OSAL_NV_ITEM_UNINIT, RTI_ERROR_OSAL_NV_BAD_ITEM_LEN
 */
rStatus_t RTI_ReadItemEx(uint8 profileId, uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t rtnVal = RTI_ERROR_INVALID_PARAMETER;

  switch (profileId)
  {
    case RTI_PROFILE_ZRC:
    {
      if ((itemId == aplKeyRepeatInterval) || (itemId == aplKeyRepeatWaitTime)) // App must implement.
      {
        return RTI_ERROR_INVALID_PARAMETER;
      }
      else if (itemId == aplKeyExchangeTransferCount)
      {
        *pValue = dppKeyTransferCnt;
        return RTI_SUCCESS;
      }
    }
    // No break - fall through.

    case RTI_PROFILE_RTI:
    {
      rtnVal = rtiReadItem(itemId, len, pValue);
      break;
    }

    case RTI_PROFILE_ZID:
    {
      if (FEATURE_ZID)
      {
        rtnVal = zidReadItem(itemId, len, pValue);
      }
      break;
    }

    case RTI_PROFILE_GDP:
    {
      if (FEATURE_GDP)
      {
        rtnVal = gdpReadItem(itemId, len, pValue);
      }
      break;
    }

    default:
    {
      /* rtnVal is initialized to indicate error, so no need to do anything here */
      break;
    }
  }

  return rtnVal;
}

/**************************************************************************************************
 *
 * @fn          rtiWriteItem
 *
 * @brief       This API is used to write RTI Configuration Interface parameters
 *              to the Configuration Parameters table, and permitted attributes
 *              to the State Attributes table.
 *
 * input parameters
 *
 * @param       itemId:  The Configuratin Interface item identifier.
 * @param       len:     The length in bytes of the item identifier's data.
 * @param       *pValue: A pointer to the item's data.
 *
 * input parameters
 *
 * None.
 *
 * @return      RTI_SUCCESS, RTI_ERROR_NOT_PERMITTED, RTI_ERROR_INVALID_INDEX,
 *              RTI_ERROR_INVALID_PARAMETER, RTI_ERROR_UNKNOWN_PARAMETER,
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE, RTI_ERROR_OSAL_NV_OPER_FAILED,
 *              RTI_ERROR_OSAL_NV_ITEM_UNINIT, RTI_ERROR_OSAL_NV_BAD_ITEM_LEN
 */
static rStatus_t rtiWriteItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  if (itemId <= RCN_NIB_NWK_RANGE_END)  // If item is a State attribute (i.e. NIB attribute).
  {
    if (itemId == RCN_NIB_NWK_PAIRING_TABLE)
    {
      // The pairing table should be accessed thru RTI_SA_ITEM_PT_CURRENT_ENTRY.
      status = RTI_ERROR_NOT_PERMITTED;
    }
    else
    {
      status = RCN_NlmeSetReq(itemId, 0, pValue);
    }
  }
  else if (itemId < RTI_CP_ITEM_END)  // If item is in the Configuration Parameters table.
  {
    // Then all operations are identical except for whether it is a read or write.
    status = rtiReadWriteItem(itemId, len, pValue, osal_snv_write);
  }
  else
  {
    switch (itemId)
    {
    case RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX:
      stateAttribTable.curPairTableIndex = *pValue;
      break;

    case RTI_SA_ITEM_PT_CURRENT_ENTRY:
      status = RCN_NlmeSetReq(RCN_NIB_NWK_PAIRING_TABLE,stateAttribTable.curPairTableIndex,pValue);
#if defined FEATURE_ZID_ADA && FEATURE_ZID_ADA == TRUE
      if (status == RTI_SUCCESS)
      {
        if (((rcnNwkPairingEntry_t *)pValue)->pairingRef == RTI_INVALID_PAIRING_REF)
        {
          /* Since we're clearing a pairing table entry, make sure corresponding
           * proxy table entry is cleared.
           */
          zidAda_RemoveFromProxyTable( stateAttribTable.curPairTableIndex );
        }
      }
#endif
      break;

    case RTI_SA_ITEM_PT_NUMBER_OF_ACTIVE_ENTRIES:
    case RTI_CONST_ITEM_SW_VERSION:
    case RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES:
    case RTI_CONST_ITEM_NWK_PROTOCOL_IDENTIFIER:
    case RTI_CONST_ITEM_NWK_PROTOCOL_VERSION:
#if defined FEATURE_OAD
    case RTI_CONST_ITEM_OAD_IMAGE_ID:
#endif
    case RTI_CONST_ITEM_RNP_IMAGE_ID:
      status = RTI_ERROR_NOT_PERMITTED;  // These items are read-only.
    break;

    default:  // No other Id's are valid.
      status = RTI_ERROR_INVALID_PARAMETER;
      break;
    }
  }

  return status;
}

/**************************************************************************************************
 *
 * @fn          RTI_WriteItemEx
 *
 * @brief       This API is used to write the RTI Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table for any supported profile.
 *
 * input parameters
 *
 * @param       profileId - The Profile identifier.
 * @param       itemId - The Configuration Interface item identifier.
 * @param       len    - The length in bytes of the item identifier's data.
 * @param       *pValue: A pointer to the item's data.
 *
 * input parameters
 *
 * None.
 *
 * @return      RTI_SUCCESS, RTI_ERROR_NOT_PERMITTED, RTI_ERROR_INVALID_INDEX,
 *              RTI_ERROR_INVALID_PARAMETER, RTI_ERROR_UNKNOWN_PARAMETER,
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE, RTI_ERROR_OSAL_NV_OPER_FAILED,
 *              RTI_ERROR_OSAL_NV_ITEM_UNINIT, RTI_ERROR_OSAL_NV_BAD_ITEM_LEN
 */
rStatus_t RTI_WriteItemEx(uint8 profileId, uint8 itemId, uint8 len, uint8 *pValue)
{
  switch (profileId)
  {
  case RTI_PROFILE_ZRC:
    if ((itemId == aplKeyRepeatInterval) || (itemId == aplKeyRepeatWaitTime)) // App must implement.
    {
      return RTI_ERROR_INVALID_PARAMETER;
    }
    else if (itemId == aplKeyExchangeTransferCount)
    {
      if (*pValue < aplcMinKeyExchangeTransferCount)
      {
        return RTI_ERROR_INVALID_PARAMETER;
      }
      dppKeyTransferCnt = *pValue;
      return RTI_SUCCESS;
    }
    // No break - fall through.

  case RTI_PROFILE_RTI:
    return rtiWriteItem(itemId, len, pValue);

  case RTI_PROFILE_ZID:
    if (FEATURE_ZID)
    {
      return zidWriteItem(itemId, len, pValue);
    }

  case RTI_PROFILE_GDP:
    if (FEATURE_GDP)
    {
      return gdpWriteItem(itemId, len, pValue);
    }

  default:
    return RTI_ERROR_INVALID_PARAMETER;
  }
}

/**************************************************************************************************
 *
 * @fn          rtiReadWriteItem
 *
 * @brief       This API is used to set the RTI Configuration Interface
 *              parameters for the Configuration Parameters table, attributes
 *              for the State Attributes table, and in the case of a read, the
 *              constants from the the Constants table.
 *
 * input parameters
 *
 * @param       itemId:     The Configuratin Interface attribute identifier.
 * @param       len:        The length in bytes of the attribute identifier.
 * @param       *pValue:    A pointer to the attribute data for writes
 * @param       nv_rw_func: Pointer to osal_nv_read or osal_nv_write function.
 *
 * output parameters
 *
 * @param       *pValue:    A pointer to the attribute data on reads.
 *
 * @return      RTI_SUCCESS, RTI_ERROR_NOT_PERMITTED, RTI_ERROR_UNKNOWN_PARAMETER,
 *              RTI_ERROR_OSAL_NV_OPER_FAILED, RTI_ERROR_OSAL_NV_ITEM_UNINIT,
 *              RTI_ERROR_OSAL_NV_BAD_ITEM_LEN
 */
uint8 rtiReadWriteItem( uint8 itemId, uint8 len, uint8 *pValue, nv_rw_func_t nv_rw_func )
{
  osalSnvId_t nvid = rtiCpStorage[itemId - RTI_CP_ITEM_STARTUP_CTRL].nvid;
  uint8 status = (nv_rw_func)(nvid, len, pValue);

  if (nv_rw_func == osal_snv_read && status != SUCCESS)
  {
    // In case of read operation failure, copy the default value
    uint8 *pCache = rtiCpStorage[itemId - RTI_CP_ITEM_STARTUP_CTRL].pCache;
    (void)osal_memcpy(pValue, pCache, len);
    status = SUCCESS;
  }

  return status;
}

/**************************************************************************************************
 *
 * @fn          RTI_InitReq
 *
 * @brief       This API is used to initialize the RemoTI stack and begin
 *              network operation. A RemoTI confirmation callback is generated
 *              and handled by the client.
 *
 *              The first thing this function does is take a snapshot of the
 *              Configuration Parameters (CP) table stored in NV memory, and
 *              only the snapshot will be used by RTI until another call is made
 *              to this function (presumably due to a reset). Therefore, any
 *              changes to the CP table must be made prior to calling this
 *              function. Once the RTI is started, subsequent changes by the
 *              client to the CP table can be made, but they will have no affect
 *              on RTI operation. The CP table is stored in NV memory and will
 *              persist across a device reset. The client can restore the
 *              the CP table to its default settings by setting the Startup
 *              Option parameter accordingly.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_RCN_INVALID_INDEX
 *              RTI_ERROR_RCN_UNSUPPORTED_ATTRIBUTE
 *              RTI_ERROR_UNKNOWN_PARAMETER
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
void RTI_InitReq( void )
{
  uint8 status   = RTI_SUCCESS;
  uint8 startRCN = FALSE;
  uint8 started;

  // take snapshot of Configuration Parameter table stored in NV to capture any
  // table changes made by client after startup; RTI will only used RAM values
  // from this point on
  rtiReadNV();

  if ((rtiState != RTI_STATE_START) && (rtiState != RTI_STATE_READY))
  {
    status = RTI_ERROR_NOT_PERMITTED;
  }
  else
  {
    switch (configParamTable.startupCtrl)  // Startup sequence is based on client's CP startup option.
    {
    // restore NIB attributes from saved values
      case RESTORE_STATE:
        // check if the RCN has ever been started
        // NOTE: Since the client will typically update the Configuration Parameters
        //       table prior to calling this routine, it is possible it changed the
        //       startup option to restore the NIB state. But if the RCN was never
        //       previously started, then it is possible the NIB default values may
        //       not be sufficient for normal operation. Consequently, the RCN
        //       maintains an internal flag that tracks if it has already been
        //       started, which RTI can use to correct this client "error".
        if ( (status = RCN_NlmeGetReq( RCN_NIB_STARTED, 0, &started )) == RCN_SUCCESS )
        {
          // check if the RCN has already been started
          if ( started )
          {
            rtiState = RTI_STATE_READY;
            // reset network layer, but retain NIB values; return success
            RCN_NlmeResetReq( FALSE );
            if (!( configParamTable.nodeCapabilities & RCN_NODE_CAP_TARGET ))
            {
              // By default turn off receiver on controller
              RCN_NlmeRxEnableReq(0);
            }
          }
          else // RCN hasn't been started before, so there really isn't a NIB to restore...
          {
            // ...so clear NIB and start the RCN
            startRCN = TRUE;
          }
        }
        break;

      // reset NIB attributes to default values, and startup
      case CLEAR_STATE:
        // clear NIB and start the RCN
        startRCN = TRUE;
        break;

      // restore configuration parameters to default values, reset NIB to default values, and startup
      case CLEAR_CONFIG_CLEAR_STATE:
        // set configuration parameters in RAM to their default values
        rtiResetCPinRAM();

        // update the NV with the default CP table
        rtiWriteNV();

        // clear NIB and start the RCN
        startRCN = TRUE;
        break;

      default:
        // bad parameter was passed
        status = RTI_ERROR_INVALID_PARAMETER;
        break;
    }
  }

  // check if we have to startup the RCN
  if ( startRCN == TRUE )
  {
    // starting up the RCN/MAC can take some time, so schedule an event to
    // start things up, and return immediately to the client; a callback
    // will be made by the RCN, handled by RTI, and a callback to the
    // RTI's client will in turn be made at that time
    osal_set_event( RTI_TaskId, RTI_EVT_RCN_START_REQ );
  }
  else // we can confirm immediately
  {
    if (FEATURE_ZID && (status == RTI_SUCCESS))  zidInitCnf();
    if (FEATURE_Z3D && (status == RTI_SUCCESS))  z3dInitCnf();
    RTI_InitCnf(status);
  }

#if defined( POWER_SAVING )
  if (rtiState == RTI_STATE_READY)
  {
    // The RTI Task can now allow power saving since RCN_NlmeResetReq() will have been invoked.
    osal_pwrmgr_task_state( RTI_TaskId, PWRMGR_CONSERVE );
  }
#endif
}

/**************************************************************************************************
 *
 * @fn          RTI_PairReq
 *
 * @brief       This API is used to initiate a pairing process. Note that this
 *              call actually consists of a discovery followed by pairing. That
 *              is, a NLME-DISCOVERY.request followed by NLME-PAIR.request.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_NOT_PERMITTED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_MAC_TRANSACTION_EXPIRED
 *              RTI_ERROR_MAC_TRANSACTION_OVERFLOW
 *              RTI_ERROR_MAC_NO_RESOURCES
 *              RTI_ERROR_MAC_UNSUPPORTED
 *              RTI_ERROR_MAC_BAD_STATE
 *              RTI_ERROR_MAC_CHANNEL_ACCESS_FAILURE
 *              RTI_ERROR_MAC_NO_ACK
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
void RTI_PairReq( void )
{
  uint8 i;

  if (rtiState != RTI_STATE_READY)  // Shouldn't have called in this state.
  {
    RTI_PairCnf(RTI_ERROR_NOT_PERMITTED, RTI_INVALID_PAIRING_REF, RTI_INVALID_PAIRING_REF);
    return;
  }

  // Make sure there is room in NV, without compaction, for this requested pairing.
  // We do not want a compaction to start after the pairing because this may cause
  // a conflict in timing for the configuration phase.
  osal_snv_makeRoomInActivePage(sizeof(itemsToStoreInNV), itemsToStoreInNV);

  // prepare the application capabilities field
  rtiReqRspPrim.prim.discoveryReq.appInfo.appCapabilities = 0;

  // set application capabilities as configured
  rtiReqRspPrim.prim.discoveryReq.appInfo.appCapabilities = configParamTable.appCap.appCapabilities;

  // list of devices types supported by this node
  for (i=0; i<RCN_APP_CAPA_GET_NUM_DEV_TYPES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.discoveryReq.appInfo.devTypeList[i] = configParamTable.appCap.devTypeList[i];
  }

  // list of profiles disclosed by this node
  for (i=0; i<RCN_APP_CAPA_GET_NUM_PROFILES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.discoveryReq.appInfo.profileIdList[i] =
    rtiReqRspPrim.prim.discoveryReq.discProfileIdList[i] = configParamTable.appCap.profileIdList[i];
  }
  // Set search profile ID(s) to the supported profile ID(s).
  rtiReqRspPrim.prim.discoveryReq.discProfileIdListSize = i;

  // Note: if a user string is to be included, it should have already been set
  //       in the Configuration Parameters table

  // PAN identifier of the destination device for discovery; use wildcard setting
  rtiReqRspPrim.prim.discoveryReq.dstPanId     = RTI_DEST_PAN_ID_WILDCARD;

  // address of destination device for discovery; use wildcard setting
  rtiReqRspPrim.prim.discoveryReq.dstNwkAddr   = RTI_DEST_NWK_ADDR_WILDCARD;

  // the device type to disover; use wildcard setting
#ifdef RTI_CERC_IOT
  // IOT specific codes
  rtiReqRspPrim.prim.discoveryReq.searchDevType   = RTI_DEVICE_TELEVISION;
#else // RTI_CERC_IOT
  // For normal RTI build, discovery will look for any device.
  // Hence wildcard is used.
  rtiReqRspPrim.prim.discoveryReq.searchDevType   = RTI_REC_DEV_TYPE_WILDCARD;
#endif

  // max number of MAC symbols to wait for discovery responses to be sent back
  // from potential target nodes on each channel; use 100 milliseconds
  rtiReqRspPrim.prim.discoveryReq.discDurationInMs = configParamTable.discoveryInfo.discDuration;

  // wait for the RCN discovery confirmation
  rtiState = RTI_STATE_DISCOVERY;

  // send the request to the RCN
  RCN_NlmeDiscoveryReq( &rtiReqRspPrim.prim.discoveryReq );
}

/**************************************************************************************************
 *
 * @fn          RTI_PairAbortReq
 *
 * @brief       This API is used to abort an on-going pairing process.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_PAIR_COMPLETE
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
void RTI_PairAbortReq( void )
{
  if (RTI_STATE_DISCOVERY == rtiState ||
      RTI_STATE_DISCOVERED == rtiState ||
      RTI_STATE_DISCOVERY_ERROR == rtiState)
  {
    // Running discovery now; abort discovery.
    rtiState = RTI_STATE_DISCOVERY_ABORT;
    RCN_NlmeDiscoveryAbortReq();
  }
  else
  {
    // Shouldn't have called in this state, or already pairing and abort not allowed.
    RTI_PairAbortCnf(RTI_ERROR_NOT_PERMITTED);
  }
}

/**************************************************************************************************
 *
 * @fn          RTI_AllowPairReq
 *
 * @brief       This function is used by the Target application to ready the
 *              node for a pairing request, and thereby allow this node to respond.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_OSAL_NO_TIMER_AVAIL
 *              RTI_ERROR_ALLOW_PAIRING_TIMEOUT
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
void RTI_AllowPairReq( void )
{
#ifndef FEATURE_CONTROLLER_ONLY
  uint8 i;

  // Make sure there is room in NV, without compaction, for this requested pairing.
  // We do not want a compaction to start after the pairing because this may cause
  // a conflict in timing for the configuration phase.
  osal_snv_makeRoomInActivePage(sizeof(itemsToStoreInNV), itemsToStoreInNV);

  // Fill in NLME-AUTO-DISCOVERY.request parameters
  rtiReqRspPrim.prim.autoDiscoveryReq.autoDiscDurationInMs = DPP_AUTO_RESP_DURATION;
  rtiReqRspPrim.prim.autoDiscoveryReq.appInfo.appCapabilities = configParamTable.appCap.appCapabilities;
  for (i = 0; i < RCN_APP_CAPA_GET_NUM_DEV_TYPES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.autoDiscoveryReq.appInfo.devTypeList[i] =
      configParamTable.appCap.devTypeList[i];
  }
  for (i = 0; i < RCN_APP_CAPA_GET_NUM_PROFILES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.autoDiscoveryReq.appInfo.profileIdList[i] =
      configParamTable.appCap.profileIdList[i];
  }

  // Send the primitive
  RCN_NlmeAutoDiscoveryReq(&rtiReqRspPrim.prim.autoDiscoveryReq);

  if (FEATURE_Z3D_TGT)  z3dTgtAllowPairInd();
#endif
}

/**************************************************************************************************
 *
 * @fn          RTI_AllowPairAbortReq
 *
 * @brief       This API is used to attempt to abort an on-going allow-pairing process.
 *
 *              It is possible that allow pair is at a state of no return (no aborting).
 *              There is no callback associated to this function call.
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
void RTI_AllowPairAbortReq( void )
{
#ifndef FEATURE_CONTROLLER_ONLY
  // it is possible that an auto discovery might be on-going
  // Stop auto discovery
  RCN_NlmeAutoDiscoveryAbortReq();
#endif
}


/**************************************************************************************************
 *
 * @fn          RTI_UnpairReq
 *
 * @brief       This API is used to initiate a network layer process to trigger un-pair request
 *              command and deletion of pairing entry.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_MAC_TRANSACTION_EXPIRED
 *              RTI_ERROR_MAC_TRANSACTION_OVERFLOW
 *              RTI_ERROR_MAC_NO_RESOURCES
 *              RTI_ERROR_MAC_UNSUPPORTED
 *              RTI_ERROR_MAC_BAD_STATE
 *              RTI_ERROR_MAC_CHANNEL_ACCESS_FAILURE
 *              RTI_ERROR_MAC_NO_ACK
 *
 * input parameters
 *
 * @param   dstIndex - destination index for the pairing entry to remove
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_UnpairReq(uint8 dstIndex)
{
  if (rtiState == RTI_STATE_READY)
  {
    rtiState = RTI_STATE_UNPAIR;
    RCN_NlmeUnpairReq(dstIndex);
  }
}

/**************************************************************************************************
 *
 * @fn          RTI_SendDataReq
 *
 * @brief       This function sends data to the destination specified by the
 *              pairing table index.
 *
 *              The client's confirm callback will provide a status, which can
 *              be one of the following:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_NO_PAIRING_INDEX
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_NOT_PERMITTED
 *              RTI_ERROR_MAC_BEACON_LOSS
 *              RTI_ERROR_MAC_PAN_ID_CONFLICT
 *              RTI_ERROR_MAC_SCAN_IN_PROGRESS
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 * @param       profileId - Profile identifier.
 * @param       vendorId  - Vendor identifier.
 * @param       txOptions - value corresponding to TxOptions NLDE-DATA.request
 *                          primitive of the RF4CE spec.
 * @param       len       - Number of bytes to send.
 * @param       *pData    - Pointer to data to be sent.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_SendDataReq(uint8 dstIndex, uint8 profileId, uint16 vendorId, uint8 txOptions,
                                                                       uint8 len, uint8 *pData)
{
  rcnNwkPairingEntry_t *pEntry;
  rStatus_t status;

  if ((status = RCN_NlmeGetPairingEntryReq(dstIndex, &pEntry)) != RCN_SUCCESS)
  {
    status = RTI_ERROR_INVALID_PARAMETER;
  }
  else
  {
    // ZID may fill-in more Tx Options according to requisite behavior.
    if (FEATURE_ZID && (profileId == RTI_PROFILE_ZID))
    {
      txOptions = zidSendDataReq(dstIndex, txOptions, pData);
    }

    rtiReqRspPrim.prim.dataReq.pairingRef = dstIndex;
    rtiReqRspPrim.prim.dataReq.profileId  = profileId;
    rtiReqRspPrim.prim.dataReq.vendorId   = vendorId;
    rtiReqRspPrim.prim.dataReq.txOptions  = txOptions;
    rtiReqRspPrim.prim.dataReq.nsduLength = len;

    if ( (status = RCN_NldeDataAlloc(&rtiReqRspPrim.prim.dataReq)) == RCN_SUCCESS )
    {
      osal_memcpy( rtiReqRspPrim.prim.dataReq.nsdu, pData, len );

      rtiState = RTI_STATE_NDATA;
      RCN_NldeDataReq( &rtiReqRspPrim.prim.dataReq );
      return;
    }
  }

  RTI_SendDataCnf(status);
}

/**************************************************************************************************
 *
 * @fn          RTI_StandbyReq
 *
 * @brief       This function places the Target into or takes it out of standby.
 *              Note that the node will automatically come out of standby when
 *              it receives a data packet, and return to standby mode when
 *              nothing else is to be done. Standby will also persist across
 *              resets. The client is responsible for explicitly enabling
 *              and disabling standby using the RTI_StandbyReq API. The client
 *              can monitor the standby state by reading the State Attribute
 *              item RTI_SA_ITEM_IN_POWER_SAVE.
 *
 *              A call to RTI_StandbyCnf will occur as a consequence of this
 *              API function call which can return the following status values:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 *              RTI_ERROR_INVALID_INDEX
 *              RTI_ERROR_UNKNOWN_STATUS_RETURNED
 *
 * input parameters
 *
 * @param       mode - RTI_STANDBY_ON, RTI_STANDBY_OFF
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_StandbyReq( uint8 mode )
{
  uint16 rxOnDuration;

  // check the standby mode
  switch( mode )
  {
  case RTI_STANDBY_OFF:
    // only duty cycle is used by RCN; force to zero to disable
    rxOnDuration = 0xffff;
    break;

  case RTI_STANDBY_ON:
    // set duty cycle to value provided by client to enable standby
    rxOnDuration = 0;
    break;

  default:
    // unknown mode
    RTI_StandbyCnf( RTI_ERROR_INVALID_PARAMETER );
    return;
  }

  // set the duty cycle according to mode and return status directly to client
  RTI_StandbyCnf(RCN_NlmeRxEnableReq(rxOnDuration));
}

/**************************************************************************************************
 *
 * @fn          RTI_RxEnableReq
 *
 * @brief       This function is used by the Controller or the Target
 *              client to enable the radio receiver, enable the radio
 *              receiver for a specified amount of time, or disable the radio
 *              receiver.
 *
 *              A call to RTI_RxEnableCnf will occur as a consequence of this
 *              API function call which can return the following status values:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *
 * input parameters
 * @param       duration -  0:         turn receiver off
 *                          0xFFFF:    turn receiver on
 *                          1..0xFFFE: turn receiver on for duration in milliseconds
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_RxEnableReq( uint16 duration )
{
  // pass client request on to RCN to set the receiver enable
  RTI_RxEnableCnf(RCN_NlmeRxEnableReq(duration));
}


/**************************************************************************************************
 *
 * @fn          RTI_EnableSleepReq
 *
 * @brief       This function is used by the Controller or the Target
 *              client to put the network processor (NP) to sleep.
 *
 *              A call to RTI_EnableSleepCnf will occur as a consequence of this
 *              API function call which can return the following status values:
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_NOT_PERMITTED
 *
 * input parameters
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_EnableSleepReq( void )
{
#if defined( POWER_SAVING )
  RTI_EnableSleepCnf( RTI_SUCCESS );
#else
  RTI_EnableSleepCnf( RTI_ERROR_NOT_PERMITTED );
#endif
}

/**************************************************************************************************
 *
 * @fn          RTI_DisableSleepReq
 *
 * @brief       This function is used by the Controller or the Target
 *              client to prevent the network processor (NP) from going to
 *              sleep.
 *
 *              A call to RTI_DisableSleepCnf will occur as a consequence of this
 *              API function call which can return the following status values:
 *
 *              Note: This can be called by client or by RTIS when it handles
 *              the NPI Wakeup Callback.
 *
 *              RTI_SUCCESS
 *              RTI_ERROR_NOT_PERMITTED
 *
 * input parameters
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_DisableSleepReq( void )
{
#if defined( POWER_SAVING )
  RTI_DisableSleepCnf( RTI_SUCCESS );
#else
  RTI_DisableSleepCnf( RTI_ERROR_NOT_PERMITTED );
#endif
}


/**************************************************************************************************
 * @fn          RCN_CbackEvent
 *
 * @brief       This callback function sends RCN events to the RTI. The RTI must
 *              implement this function.  A typical implementation of this
 *              function would allocate an OSAL message, copy the event
 *              parameters to the message, and send the message to the
 *              application's OSAL event handler.  This function may be
 *              executed from task or interrupt context and therefore must
 *              be reentrant.
 *
 * input parameters
 *
 * @param       pData - Pointer to RCN callback structure.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RCN_CbackEvent( rcnCbackEvent_t *pData )
{
  if (pRtiBridgeHandler)
  {
    // network layer bridge handler function is set up
    // in this case, just route the callback to the bridge
    pRtiBridgeHandler(pData);
    return;
  }

  switch ( pData->eventId )
  {
  case RCN_NLME_START_CNF:
    rtiState = (pData->prim.startCnf.status == RTI_SUCCESS) ? RTI_STATE_READY : RTI_STATE_START;

#if defined( POWER_SAVING )
    if (rtiState == RTI_STATE_READY)
    {
      // The RTI Task can now allow power saving since RCN_NlmeResetReq() will have been invoked.
      osal_pwrmgr_task_state( RTI_TaskId, PWRMGR_CONSERVE );
    }
#endif

    if ((configParamTable.nodeCapabilities & RCN_NODE_CAP_TARGET) == 0)
    {
      RCN_NlmeRxEnableReq(0);  // By default turn off receiver on controller.
    }

    /* Since the RTI_InitCnf() will inform the application of the completion of
     * the initialization process, any profile specific initialization should be
     * done first to ensure any NVM items are properly initialized before the
     * application reads them.
     */
    if (FEATURE_ZID && (pData->prim.startCnf.status == RTI_SUCCESS))  zidInitCnf();
    if (FEATURE_Z3D && (pData->prim.startCnf.status == RTI_SUCCESS))  z3dInitCnf();
    RTI_InitCnf(pData->prim.startCnf.status);
    break;

  case RCN_NLME_COMM_STATUS_IND:
    rtiOnNlmeCommStatusInd( pData );
    break;

  case RCN_NLME_DISCOVERED_EVENT:
    rtiOnNlmeDiscoveredEvent( pData );
    break;

  case RCN_NLME_DISCOVERY_CNF:
    rtiOnNlmeDiscoveryCnf( pData );
    break;

  case RCN_NLME_DISCOVERY_ABORT_CNF:
    rtiOnNlmeDiscoveryAbortCnf();
    break;

  // Discovery indication will not occur ignored in ZRC discovery procedure.

  case RCN_NLME_AUTO_DISCOVERY_CNF:
    rtiOnNlmeAutoDiscoveryCnf( pData );
    break;

  case RCN_NLME_PAIR_CNF:
    rtiOnNlmePairCnf( pData );
    break;

#ifndef FEATURE_CONTROLLER_ONLY
    // To save code for controller-only build, NLME-PAIR.indication handling can be compiled out.
  case RCN_NLME_PAIR_IND:
    rtiOnNlmePairInd( pData );
    break;
#endif

  case RCN_NLME_UNPAIR_IND:
    rtiOnNlmeUnpairInd( pData );
    break;

  case RCN_NLME_UNPAIR_CNF:
    rtiOnNlmeUnpairCnf(pData);
    break;

  case RCN_NLDE_DATA_CNF:
    rtiOnNldeDataCnf(pData);
    break;

  case RCN_NLDE_DATA_IND:
    rtiOnNldeDataInd(pData);
    break;
  }
}

/**************************************************************************************************
 * @fn          RTI_SetBridgeMode
 *
 * @brief       This function sets RTI either into, or out of, network layer bridge mode
 *              where all RCN callback events will be routed to a designated callback function
 *              and where RTI shall not process RCN callbacks other than such routing.
 *
 * input parameters
 *
 * @param       pCback - Pointer to a RCN callback function to where RTI should
 *                       route all RCN callbacks.
 *                       It can be set to NULL to indicate that RTI should take
 *                       over RCN callback handling again.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RTI_SetBridgeMode(rtiRcnCbackFn_t pCback)
{
  pRtiBridgeHandler = pCback;
}

/**************************************************************************************************
 * @fn          rtiOnNldeDataCnf
 *
 * @brief       This function handles RCN callback event for NLDE-DATA.confirm
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNldeDataCnf( rcnCbackEvent_t *pData )
{
  rtiState = RTI_STATE_READY;

  // If the ZID co-layer sent data, don't bother Application layer with an unexpected data confirm.
  if ((!FEATURE_ZID || (zidSendDataCnf(pData->prim.dataCnf.status) == FALSE)) &&
      (!FEATURE_Z3D || (z3dSendDataCnf(pData->prim.dataCnf.status) == FALSE)) &&
      (!FEATURE_GDP || (gdpSendDataCnf(pData->prim.dataCnf.status) == FALSE)))
  {
    RTI_SendDataCnf(pData->prim.dataCnf.status);
  }
}

/**************************************************************************************************
 * @fn          rtiOnNldeDataInd
 *
 * @brief       This function handles RCN callback event for NLDE-DATA.indication
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNldeDataInd( rcnCbackEvent_t *pData )
{
  bool consumed = FALSE;

  /* If data is strictly for one of the profiles (e.g. Configuration state
   * transactions), then don't bother Application layer with unexpected or
   * consumed data indication.
   */
  if (FEATURE_GDP && (pData->prim.dataInd.profileId == RTI_PROFILE_GDP))
  {
    consumed = gdpReceiveDataInd( pData->prim.dataInd.pairingRef,
                                  pData->prim.dataInd.nsduLength,
                                  pData->prim.dataInd.nsdu);
  }
  else if (FEATURE_ZID && (pData->prim.dataInd.profileId == RTI_PROFILE_ZID))
  {
    consumed = zidReceiveDataInd( pData->prim.dataInd.pairingRef,
                                  pData->prim.dataInd.vendorId,
                                  pData->prim.dataInd.rxLinkQuality,
                                  pData->prim.dataInd.rxFlags,
                                  pData->prim.dataInd.nsduLength,
                                  pData->prim.dataInd.nsdu);
  }
  else if (FEATURE_Z3D && (pData->prim.dataInd.profileId == RTI_PROFILE_Z3S))
  {
    consumed = z3dReceiveDataInd( pData->prim.dataInd.pairingRef,
                                  pData->prim.dataInd.nsduLength,
                                  pData->prim.dataInd.nsdu );
  }

  if (consumed == FALSE)
  {
#ifndef FEATURE_CONTROLLER_ONLY
    if ((pData->prim.dataInd.profileId == RTI_PROFILE_ZRC) &&
       ((pData->prim.dataInd.rxFlags & RTI_RX_FLAGS_VENDOR_SPECIFIC) == 0) &&
        (pData->prim.dataInd.nsdu[0] == RTI_CERC_COMMAND_DISCOVERY_REQUEST))
    {
      if ((pData->prim.dataInd.rxFlags & RTI_RX_FLAGS_BROADCAST) == 0)
      {
        const uint8 len = sizeof(ZrcCmdMatrix) + 2;
        uint8 *pBuf = osal_mem_alloc(len);

        if (pBuf != NULL)
        {
          uint8 txOptions = RTI_TX_OPTION_ACKNOWLEDGED |
              ((pData->prim.dataInd.rxFlags & RTI_RX_FLAGS_SECURITY) ? RTI_TX_OPTION_SECURITY : 0);

          pBuf[0] = RTI_CERC_COMMAND_DISCOVERY_RESPONSE;
          pBuf[1] = 0;  // Reserved (was command bank number).
          (void)osal_memcpy(pBuf+2, ZrcCmdMatrix, sizeof(ZrcCmdMatrix));

          RTI_SendDataReq(pData->prim.dataInd.pairingRef, RTI_PROFILE_ZRC,
                                     configParamTable.vendorInfo.vendorId, txOptions, len, pBuf);
          (void)osal_mem_free(pBuf);
        }
      }
    }
    else
#endif
    {
      RTI_ReceiveDataInd( pData->prim.dataInd.pairingRef,
                          pData->prim.dataInd.profileId,
                          pData->prim.dataInd.vendorId,
                          pData->prim.dataInd.rxLinkQuality,
                          pData->prim.dataInd.rxFlags,
                          pData->prim.dataInd.nsduLength,
                          pData->prim.dataInd.nsdu );
    }
  }
}

/**************************************************************************************************
 * @fn          rtiOnNlmeCommStatusInd
 *
 * @brief       This function handles RCN callback event for NLME-COMM-STATUS.indication
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeCommStatusInd( rcnCbackEvent_t *pData )
{
  if (rtiAllowPairCnfParams.pairingRef != RCN_INVALID_PAIRING_REF &&
      pData->prim.commStatusInd.pairingRef == rtiAllowPairCnfParams.pairingRef)
  {
    // NLME-COMM-STATUS.indication was issued against pair response command accepting pairing.
    rtiState = RTI_STATE_CONFIGURATION;
    gdpCfgParam.prevConfiguredProfile = RTI_PROFILE_GDP;
    gdpCfgParam.pairingRef = pData->prim.pairCnf.pairingRef;
    gdpCfgParam.invokedBy = GDP_CFG_INVOKED_BY_ALLOW_PAIR;
    if (pData->prim.commStatusInd.status == RTI_SUCCESS)
    {
      // invoke next configuration after blackout time
      osal_start_timerEx( RTI_TaskId, GDP_EVT_CONFIGURE_NEXT,  aplcConfigBlackoutTime );
    }
    else
    {
      rtiState = RTI_STATE_READY;
      RTI_AllowPairCnf(pData->prim.commStatusInd.status,
                       rtiAllowPairCnfParams.pairingRef,
                       rtiAllowPairCnfParams.devType);
    }
  }
}

/**************************************************************************************************
 * @fn          rtiOnNlmeDiscoveredEvent
 *
 * @brief       This function handles the callback event generated by the RCN
 *              every time a discovery response is received from a target as a
 *              result of the controller's discovery request.
 *
 *              Note that the pairing procedure follows CERC profile specified push button
 *              pairing. As a result, only one node descriptor is expected.
 *              If the found device does not match the device types the RTI is programmed
 *              to support (supported target device type), the pairing procedure will
 *              ultimately fail.
 *
 *              NOTE: This call is not a primitive of the RF4CE specification.
 *                    The call rather conveys node descriptor which is included in
 *                    NLME-DISCOVERY.confirm primitive.
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeDiscoveredEvent( rcnCbackEvent_t *pData )
{
  if (rtiState == RTI_STATE_DISCOVERY)  // Only if waiting to discover a target.
  {
    rtiState = RTI_STATE_DISCOVERY_ERROR;

    if (pData->prim.discoveredEvent.status == RCN_SUCCESS)
    {
      // If compiled and configured to exclusively pair with matching user strings.
      if (FEATURE_USER_STRING_PAIRING &&
          RCN_APP_CAPA_GET_USER_STRING(configParamTable.appCap.appCapabilities))
      {
        uint8 uStr[RTI_USER_STRING_LENGTH];
        RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_USER_STRING, RTI_USER_STRING_LENGTH, uStr);

        // First ensure that a user string is present, then compare it to the expected string.
        if (((RCN_APP_CAPA_GET_USER_STRING(pData->prim.discoveredEvent.devInfo.appCapabilities))==0)
        ||   (osal_memcmp(pData->prim.discoveredEvent.devInfo.userString, uStr,
              osal_strlen((char *)pData->prim.discoveredEvent.devInfo.userString)) == FALSE))
        {
          return;
        }
      }

      // Check the target device's capabilities to see if any of its device types the controller.
      for (uint8 i = 0;
           i < RCN_APP_CAPA_GET_NUM_DEV_TYPES(pData->prim.discoveredEvent.devInfo.appCapabilities);
           i++)
      {
        uint8 devType = pData->prim.discoveredEvent.devInfo.devTypeList[i];

        // Ensure that the device type is valid.
        if ((devType < RTI_DEVICE_TARGET_TYPE_START) || (devType >=  RTI_DEVICE_TARGET_TYPE_END))
        {
          return;
        }

        // Check this device against the list of client supported devices.
        for (uint8 j = 0; j < RTI_MAX_NUM_SUPPORTED_TGT_TYPES; j++)
        {
          if ((devType == configParamTable.nodeCap.supportedTgtTypes.supportedTgtTypes[j]) ||
              (configParamTable.nodeCap.supportedTgtTypes.supportedTgtTypes[j] ==
                                                        RTI_DEVICE_RESERVED_FOR_WILDCARDS))
          {
            // Save the discovered target's node descriptor for handling by the discover confirm.
            (void)osal_memcpy(&rtiDiscoveredEventCache, &pData->prim.discoveredEvent,
                                                                sizeof(rtiDiscoveredEventCache));
            rtiState = RTI_STATE_DISCOVERED;
            return;
          }
        }
      }
    }
  }
  else if (rtiState == RTI_STATE_DISCOVERED)
  {
    // more than one device was discovered, so abort the pairing (see NOTE in @brief)
    rtiState = RTI_STATE_DISCOVERY_ERROR;
  }
}

/**************************************************************************************************
 * @fn          rtiOnNlmeDiscoveryCnf
 *
 * @brief       This function handles RCN callback event for NLME-DISCOVERY.confirm
 *
 * input parameters
 *
 * @param    pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeDiscoveryCnf( rcnCbackEvent_t *pData )
{
  (void) pData; // unused argument

  // Build and send NLME-PAIR.Request
  if (rtiState == RTI_STATE_DISCOVERED)
  {
    uint8 i;

    // we have our target, so prepare for pairing

    rtiReqRspPrim.prim.pairReq.logicalChannel = rtiDiscoveredEventCache.logicalChannel;
    rtiReqRspPrim.prim.pairReq.dstPanId       = rtiDiscoveredEventCache.panId;

    sAddrExtCpy( rtiReqRspPrim.prim.pairReq.dstIeeeAddress, rtiDiscoveredEventCache.ieeeAddress );

    // prepare the application capabilities field
    rtiReqRspPrim.prim.pairReq.appInfo.appCapabilities = configParamTable.appCap.appCapabilities;

    // list of devices types supported by this node
    for (i=0; i<RCN_APP_CAPA_GET_NUM_DEV_TYPES(configParamTable.appCap.appCapabilities); i++)
    {
      rtiReqRspPrim.prim.pairReq.appInfo.devTypeList[i] = configParamTable.appCap.devTypeList[i];
    }

    // list of profiles supported by this node
    for (i=0; i<RCN_APP_CAPA_GET_NUM_PROFILES(configParamTable.appCap.appCapabilities); i++)
    {
     rtiReqRspPrim.prim.pairReq.appInfo.profileIdList[i] = configParamTable.appCap.profileIdList[i];
    }

    // Set key exchange transfer count
    if ( configParamTable.nodeCapabilities & RCN_NODE_CAP_SECURITY_CAPABLE )
    {
#if (FEATURE_GDP == TRUE)
      rtiReqRspPrim.prim.pairReq.keyExTransferCount = aplcMinKeyExchangeTransferCount;
      (void)gdpReadItem( GDP_ITEM_KEY_EX_TRANSFER_COUNT,
                         1,
                         (uint8 *)&rtiReqRspPrim.prim.pairReq.keyExTransferCount );
#else
      rtiReqRspPrim.prim.pairReq.keyExTransferCount = dppKeyTransferCnt;
#endif
    }
    else
    {
      rtiReqRspPrim.prim.pairReq.keyExTransferCount = 0;
    }

    rtiState = RTI_STATE_PAIR;
    RCN_NlmePairReq( &rtiReqRspPrim.prim.pairReq );
  }
  else if ( (rtiState == RTI_STATE_DISCOVERY) || (rtiState == RTI_STATE_DISCOVERY_ERROR) )
  {
    rtiState = RTI_STATE_READY;
    RTI_PairCnf( RTI_ERROR_FAILED_TO_DISCOVER, RTI_INVALID_PAIRING_REF, RTI_INVALID_PAIRING_REF );
  }
}


/**************************************************************************************************
 * @fn          rtiOnNlmeDiscoveryAbortCnf
 *
 * @brief       This function handles RCN callback event for discovery abort
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
static void rtiOnNlmeDiscoveryAbortCnf( void )
{
  if (RTI_STATE_DISCOVERY_ABORT == rtiState)
  {
    // update state and call confirmation callback
    rtiState = RTI_STATE_READY;
    RTI_PairAbortCnf(RTI_SUCCESS);
  }
}


/**************************************************************************************************
 * @fn          rtiOnNlmeAutoDiscoveryCnf
 *
 * @brief       This function handles RCN callback event for
 *              NLME-AUTO-DISCOVERY.confirm
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeAutoDiscoveryCnf( rcnCbackEvent_t *pData )
{
  uint8 status = pData->prim.autoDiscoveryCnf.status;

  if (RCN_SUCCESS == status &&
    (status = osal_start_timerEx(RTI_TaskId, RTI_EVT_ALLOW_PAIR_TIMEOUT,
                                 RTI_ALLOW_PAIR_TIMEOUT_DUR)) == SUCCESS ) // 1 second
  {
    allowPairFlag = TRUE;
  }
  else // osal_start_timerEx failed
  {
    // so return error status to client immediately
    RTI_AllowPairCnf(status, RTI_INVALID_PAIRING_REF, RTI_INVALID_DEVICE_TYPE);
    if (FEATURE_Z3D_TGT)  z3dTgtAllowPairCnf();
  }
}

/**************************************************************************************************
 * @fn          rtiOnNlmePairCnf
 *
 * @brief       This function handles RCN callback event for NLME-PAIR.confirm
 *
 *              If we are expecting a pair confirmation, it was successful,
 *              and the entry is valid, then make it the current paired device.
 *              Then remove any duplicate entries in the pairing table, and
 *              update the pairing table entries in NV.
 *
 *              Then notify the client via the RTI_PairCnf callback.
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmePairCnf( rcnCbackEvent_t *pData )
{
  rtiState = RTI_STATE_READY;

  if (pData->prim.pairCnf.status == RCN_SUCCESS)
  {
    rtiState = RTI_STATE_CONFIGURATION;
    gdpCfgParam.prevConfiguredProfile = RTI_PROFILE_GDP;
    gdpCfgParam.pairingRef = pData->prim.pairCnf.pairingRef;
    gdpCfgParam.invokedBy = GDP_CFG_INVOKED_BY_PAIR;

    // invoke next configuration after blackout time
    osal_start_timerEx( RTI_TaskId, GDP_EVT_CONFIGURE_NEXT,  aplcConfigBlackoutTime );
  }
  else
  {
    // failed to pair, so report error
    RTI_PairCnf( RTI_ERROR_FAILED_TO_PAIR, RTI_INVALID_PAIRING_REF, RTI_INVALID_DEVICE_TYPE );
  }
}


/**************************************************************************************************
 * @fn          rtiConfigureNextProfile
 *
 * @brief       This function finds next profile to configure, and configures it
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiConfigureNextProfile( void )
{
  rcnNwkPairingEntry_t *pEntry;

  // is the pairing table entry valid?
  if (RCN_NlmeGetPairingEntryReq(gdpCfgParam.pairingRef, &pEntry) == RCN_SUCCESS)
  {
    while (!(pEntry->profileDiscs[(gdpCfgParam.prevConfiguredProfile>>3)]
             & BV(gdpCfgParam.prevConfiguredProfile & 0x07)))
    {
      gdpCfgParam.prevConfiguredProfile++;
      if (gdpCfgParam.prevConfiguredProfile >= (RCN_PROFILE_DISCS_SIZE<<3))
      {
        rtiState = RTI_STATE_READY;
        if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_PAIR)
        {
          RTI_PairCnf( RTI_SUCCESS, gdpCfgParam.pairingRef,
                      pEntry->devTypeList[0] );
          return;
        }
        else if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_ALLOW_PAIR)
        {
          RTI_AllowPairCnf( RTI_SUCCESS, gdpCfgParam.pairingRef,
                      pEntry->devTypeList[0] );
          return;
        }
      }
    }

    // If we made it here, we need to initiate the configuration phase for the next profile
    if (gdpCfgParam.prevConfiguredProfile == RTI_PROFILE_ZID)
    {
      if (FEATURE_ZID_CLD && gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_PAIR)
      {
        zidCld_PairCnf(gdpCfgParam.pairingRef);
        // this function should set a new GDP_EVT_CONFIGURE_NEXT event when next profile is added
        return;
      }
      else if (FEATURE_ZID_ADA && gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_ALLOW_PAIR)
      {
        zidAda_AllowPairCnf(rtiAllowPairCnfParams.pairingRef);
        // this function should set a new GDP_EVT_CONFIGURE_NEXT event when next profile is added
        return;
      }
    }
    else if (gdpCfgParam.prevConfiguredProfile == RTI_PROFILE_Z3S)
    {
      if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_PAIR)
      {
//      if (FEATURE_Z3D_CTL) {
//        z3dCtl_PairCnf(gdpCfgParam.pairingRef);
//        // this function should set a new GDP_EVT_CONFIGURE_NEXT event
//    }
        return;
      }
      else if (gdpCfgParam.invokedBy == GDP_CFG_INVOKED_BY_ALLOW_PAIR)
      {
//      if (FEATURE_Z3D_TGT)  z3dTgtAllowPairCnf();
//        // this function should set a new GDP_EVT_CONFIGURE_NEXT event
        return;
      }
    }
    // Fall through here if profile is not supported
    osal_set_event( RTI_TaskId, GDP_EVT_CONFIGURE_NEXT );
  }
  else
  {
    rtiState = RTI_STATE_READY;
    RTI_PairCnf( RTI_ERROR_FAILED_TO_PAIR, gdpCfgParam.pairingRef,
                pEntry->devTypeList[0] );
    return;
  }
}


#ifndef FEATURE_CONTROLLER_ONLY
/**************************************************************************************************
 * @fn          rtiOnNlmePairInd
 *
 * @brief       This function handles RCN callback event for NLME-PAIR.indication
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmePairInd( rcnCbackEvent_t *pData )
{
  uint8 pairingRef = rtiReqRspPrim.prim.pairRsp.provPairingRef = pData->prim.pairInd.provPairingRef;
  uint8 devTypeCnt = RCN_APP_CAPA_GET_NUM_DEV_TYPES(pData->prim.pairInd.orgDevInfo.appCapabilities);
  (void)osal_stop_timerEx(RTI_TaskId, RTI_EVT_ALLOW_PAIR_TIMEOUT);

  // If compiled and configured to exclusively pair with matching user strings.
  if (FEATURE_USER_STRING_PAIRING &&
      RCN_APP_CAPA_GET_USER_STRING(configParamTable.appCap.appCapabilities))
  {
    uint8 uStr[RTI_USER_STRING_LENGTH];
    (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_USER_STRING, RTI_USER_STRING_LENGTH, uStr);

    // First ensure that a user string is present, then compare it to the expected string.
    if (((RCN_APP_CAPA_GET_USER_STRING(pData->prim.pairInd.orgDevInfo.appCapabilities)) == 0) ||
        (osal_memcmp(pData->prim.pairInd.orgDevInfo.userString, uStr,
         osal_strlen((char *)pData->prim.pairInd.orgDevInfo.userString)) == FALSE))
    {
      devTypeCnt = 0;  // Force logic below to set RCN_ERROR_NOT_PERMITTED;
    }
  }

  if (devTypeCnt > 0 && allowPairFlag)  // Note that any peer device type is accepted.
  {
    if (((pData->prim.pairInd.orgDevInfo.nodeCapabilities & RCN_NODE_CAP_SECURITY_CAPABLE) == 0) ||
         (pData->prim.pairInd.keyExTransferCount < aplcMinKeyExchangeTransferCount))
    {
      rtiReqRspPrim.prim.pairRsp.status = RCN_ERROR_NOT_PERMITTED;
    }
    else if (rtiReqRspPrim.prim.pairRsp.provPairingRef == RCN_INVALID_PAIRING_REF)
    {
      rtiReqRspPrim.prim.pairRsp.status = RCN_ERROR_NO_REC_CAPACITY;
    }
    else
    {
      rtiReqRspPrim.prim.pairRsp.status = RCN_SUCCESS;
    }
  }
  else
  {
    rtiReqRspPrim.prim.pairRsp.status = RCN_ERROR_NOT_PERMITTED;
  }

  rtiReqRspPrim.prim.pairRsp.appInfo.appCapabilities = configParamTable.appCap.appCapabilities;
  for (uint8 i=0; i < RCN_APP_CAPA_GET_NUM_DEV_TYPES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.pairRsp.appInfo.devTypeList[i] = configParamTable.appCap.devTypeList[i];
  }
  for (uint8 i=0; i < RCN_APP_CAPA_GET_NUM_PROFILES(configParamTable.appCap.appCapabilities); i++)
  {
    rtiReqRspPrim.prim.pairRsp.appInfo.profileIdList[i] = configParamTable.appCap.profileIdList[i];
  }

  rtiReqRspPrim.prim.pairRsp.dstPanId = pData->prim.pairInd.srcPanId;
  sAddrExtCpy(rtiReqRspPrim.prim.pairRsp.dstIeeeAddr, pData->prim.pairInd.orgIeeeAddress);

  RCN_NlmePairRsp(&rtiReqRspPrim.prim.pairRsp);  // Send the primitive to the network layer

  if (rtiReqRspPrim.prim.pairRsp.status == RCN_SUCCESS)  // Wait for NLME-COMM-STATUS.indication.
  {
    rtiAllowPairCnfParams.pairingRef = pairingRef;
    rtiAllowPairCnfParams.devType = pData->prim.pairInd.orgDevInfo.devTypeList[0];
    allowPairFlag = FALSE;  // Do not allow any further pair request command.
  }
  else if (allowPairFlag)
  {
    // Callback should be issued only while allow pair flag is true.
    // This is to prevent a case of the callback being made twice when
    // pair request command is received after waiting timer expires.

    allowPairFlag = FALSE;  // The flag has to be reset before calling the callback function.
    RTI_AllowPairCnf( RTI_ERROR_ALLOW_PAIRING_TIMEOUT, RTI_INVALID_PAIRING_REF, RTI_INVALID_DEVICE_TYPE );
    if (FEATURE_Z3D_TGT)  z3dTgtAllowPairCnf();
  }
}
#endif // FEATURE_CONTROLLER_ONLY

/**************************************************************************************************
 * @fn          rtiOnNlmeUnairInd
 *
 * @brief       This function handles RCN callback event for NLME-UNPAIR.indication
 *
 * input parameters
 *
 * @param       pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeUnpairInd( rcnCbackEvent_t *pData )
{
  if (FEATURE_ZID)  (void)zidUnpair(pData->prim.unpairInd.pairingRef);
  RTI_UnpairInd(pData->prim.unpairInd.pairingRef);  // Notify application of the un-pairing.

  // Allow the network layer to delete the pairing entry
  RCN_NlmeUnpairRsp(pData->prim.unpairInd.pairingRef);
}

/**************************************************************************************************
 * @fn          rtiOnNlmeUnpairCnf
 *
 * @brief       This function handles RCN callback event for NLME-UNPAIR.confirm
 *
 * input parameters
 *
 * @param    pData - pointer to RCN callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiOnNlmeUnpairCnf(rcnCbackEvent_t *pData)
{
  rtiState = RTI_STATE_READY;

  // If the ZID co-layer initiated the unpair request, then notify the Application layer with
  // an unpair indication instead of an unexpected unpair confirm.
  if ((FEATURE_ZID) && (zidUnpair(pData->prim.unpairCnf.pairingRef) == TRUE))
  {
    RTI_UnpairInd(pData->prim.unpairCnf.pairingRef);
  }
  else
  {
    RTI_UnpairCnf(pData->prim.unpairCnf.status, pData->prim.unpairCnf.pairingRef);
  }
}

/**************************************************************************************************
 * @fn          rtiResetCPinRAM
 *
 * @brief       Set the RTI Configuration Parameters (CP) table to their default settings in RAM.
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
static void rtiResetCPinRAM(void)
{
  // RTI_NVID_STARTUP_CONTROL - RTI_NVID_STARTUP_CONTROL
  configParamTable.startupCtrl = CLEAR_STATE;

  // RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES - RTI_NVID_SUPPORTED_TGTS
  (void)osal_memset(configParamTable.nodeCap.supportedTgtTypes.supportedTgtTypes,
                    RTI_DEVICE_RESERVED_INVALID,
             sizeof(configParamTable.nodeCap.supportedTgtTypes.supportedTgtTypes));

  // RTI_CP_ITEM_APPL_CAPABILITIES - RTI_NVID_APP_CAP
  configParamTable.appCap.appCapabilities = RTI_DEFAULT_APP_CAPABILITIES;
  // RTI_CP_ITEM_APPL_DEV_TYPE_LIST - RTI_NVID_DEVTYPE_LIST
  (void)osal_memset(configParamTable.appCap.devTypeList, RTI_DEVICE_RESERVED_INVALID,
             sizeof(configParamTable.appCap.devTypeList));
  // RTI_CP_ITEM_APPL_PROFILE_ID_LIST - RTI_NVID_PROFILE_LIST
  (void)osal_memset(configParamTable.appCap.profileIdList, 0,
             sizeof(configParamTable.appCap.profileIdList));

  // RTI_CP_ITEM_STDBY_DEFAULT_ACTIVE_PERIOD - RTI_NVID_ACTIVE_PERIOD
  configParamTable.stdByInfo.standbyActivePeriod = RTI_STANDBY_ACTIVE_PERIOD;
  // RTI_CP_ITEM_STDBY_DEFAULT_DUTY_CYCLE - RTI_NVID_DUTY_CYCLE
  configParamTable.stdByInfo.standbyDutyCycle = RTI_STANDBY_DUTY_CYCLE;

  // RTI_CP_ITEM_DISCOVERY_DURATION - RTI_NVID_DISC_DURATION
  configParamTable.discoveryInfo.discDuration = DPP_DISCOVERY_DURATION;
  // RTI_CP_ITEM_DEFAULT_DISCOVERY_LQI_THRESHOLD - RTI_NVID_DISC_LQI_THR
  configParamTable.discoveryInfo.discLQIThres = DPP_LQI_THRESHOLD;

  // RTI_CP_ITEM_NODE_CAPABILITIES - RCN_NVID_NWKC_NODE_CAPABILITIES
  configParamTable.nodeCapabilities = RTI_DEFAULT_NODE_CAPABILITIES;

  // RTI_CP_ITEM_VENDOR_ID - RCN_NVID_NWKC_VENDOR_IDENTIFIER
  configParamTable.vendorInfo.vendorId = 0;
  // RTI_CP_ITEM_VENDOR_NAME - RCN_NVID_NWKC_VENDOR_STRING
  (void)osal_memset(configParamTable.vendorInfo.vendorName, 0,
             sizeof(configParamTable.vendorInfo.vendorName));

  dppKeyTransferCnt = DPP_DEF_KEY_TRANSFER_COUNT;
}

/**************************************************************************************************
 * @fn          rtiResetSA
 *
 * @brief       This function sets the RTI State Attributes (SA) to their default
 *              settings in case its default setting is different from network layer default
 *              setting.
 *              Note that it is assumed that RCN_NlmeResetReq(TRUE) is called prior to this call
 *              and that configuration parameters are all set up prior to this call as well.
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
static void rtiResetSA( void )
{
  // set RCN standby active period
  RCN_NlmeSetReq( RCN_NIB_NWK_ACTIVE_PERIOD, 0, (uint8 *)&configParamTable.stdByInfo.standbyActivePeriod );

  // set RCN duty cycle period
  RCN_NlmeSetReq( RCN_NIB_NWK_DUTY_CYCLE, 0, (uint8 *)&configParamTable.stdByInfo.standbyDutyCycle );

  // set the RCN discovery LQI threshold
  RCN_NlmeSetReq( RCN_NIB_NWK_DISCOVERY_LQI_THRESHOLD, 0, &configParamTable.discoveryInfo.discLQIThres );

#ifdef RTI_CERC_IOT
  // For IOT, nwkResponseWaitTime is set to 1 second
  {
    uint16 value16 = 1000;
    RCN_NlmeSetReq( RCN_NIB_NWK_RESPONSE_WAIT_TIME, 0, (uint8 *) &value16);
  }
#endif

  // Current Pairing Table Index
  stateAttribTable.curPairTableIndex = RTI_INVALID_PAIRING_REF;

  // Discovery/Pairing Procedure settings.
  *(uint16 *) pRtiNibBuf = DPP_DISCOVERY_REP_INTERVAL;
  RCN_NlmeSetReq( RCN_NIB_NWK_DISCOVERY_REPETITION_INTERVAL, 0, (uint8 *)pRtiNibBuf);
  pRtiNibBuf[0] = FALSE;
  RCN_NlmeSetReq( RCN_NIB_NWK_INDICATE_DISCOVERY_REQUESTS, 0, (uint8 *)pRtiNibBuf);
  pRtiNibBuf[0] = DPP_MAX_DISCOVERY_REPS;
  RCN_NlmeSetReq( RCN_NIB_NWK_MAX_DISCOVERY_REPETITIONS, 0, (uint8 *)pRtiNibBuf);
  pRtiNibBuf[0] = DPP_MAX_REPORTED_NODE_DESC;
  RCN_NlmeSetReq( RCN_NIB_NWK_MAX_REPORTED_NODE_DESCRIPTORS, 0, (uint8 *)pRtiNibBuf);
}

/**************************************************************************************************
 * @fn          rtiReadNV
 *
 * @brief       This function reads the Configuration Parameters table, stored
 *              in NV memory, into RAM for RTI use.
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
static void rtiReadNV( void )
{
  rtiReadWriteNV( osal_snv_read );  // Read Configuration Parameters table from NV into RAM.
}

/**************************************************************************************************
 * @fn          rtiWriteNV
 *
 * @brief       This function writes the Configuration Parameters table stored
 *              in RAM to the NV memory.
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
static void rtiWriteNV( void )
{
  rtiReadWriteNV( osal_snv_write );  // Write Configuration Parameters table from RAM into NV.
}

/**************************************************************************************************
 * @fn          rtiReadWriteNV
 *
 * @brief       This function reads/writes the Configuration Parameters table
 *              from/to RAM/NV and writes/reads it to/from NV/RAM.
 *
 * input parameters
 *
 * @param       nv_rw_func - Function pointer to either osal_nv_write or
 *                           osal_nv_read.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rtiReadWriteNV( nv_rw_func_t nv_rw_func )
{
  // Configuration Parameters table

  uint8 i;

  for (i = 0; i < sizeof(rtiCpStorage)/sizeof(rtiCpStorage[0]); i++)
  {
    osalSnvId_t nvid = rtiCpStorage[i].nvid;
    uint8 size = rtiCpStorage[i].size;
    uint8 *pCache = rtiCpStorage[i].pCache;

    (nv_rw_func)( nvid, size, pCache );
  }
}

/**************************************************************************************************
 * @fn          rtiProgramIeeeAddr
 *
 * @brief       This function programs commissioned IEEE address to network layer if commissioned
 *              address is valid.
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
static void rtiProgramIeeeAddr( void )
{
  sAddrExt_t extAddr;
  uint8 i;

  RTI_READ_COMMISSIONED_IEEE_ADDR(extAddr);

  // Check if the commissioned IEEE address is valid (non-0xFFFFFFFFFFFFFFFF).
  for (i = 0; i < SADDR_EXT_LEN; i++)
  {
    if (extAddr[i] != 0xFF)
      break;
  }

  if (i < SADDR_EXT_LEN)
  {
    // Valid IEEE address
    // Write the IEEE address to network layer.
    RCN_NlmeSetReq(RCN_NIB_IEEE_ADDRESS, 0, extAddr);
  }
}

/**************************************************************************************************
 **************************************************************************************************/
