/**************************************************************************************************
  Filename:       zid_common.h
  Revised:        $Date: 2012-08-21 14:23:25 -0700 (Tue, 21 Aug 2012) $
  Revision:       $Revision: 31317 $

  Description:

  This file contains the common declarations for a ZID Profile Device.


  Copyright 2010-2011 Texas Instruments Incorporated. All rights reserved.

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
#ifndef ZID_COMMON_H
#define ZID_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "hal_defs.h"
#include "rcn_nwk.h"
#include "rti.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

// Disable ZID Adapter for code-size savings on ZID Class Device-only.
#if !defined FEATURE_ZID_ADA
#define FEATURE_ZID_ADA                    FALSE
#endif

// Disable ZID Class Device for code-size savings on ZID Adapter-only.
#if !defined FEATURE_ZID_CLD
#define FEATURE_ZID_CLD                    FALSE
#endif

#if (FEATURE_ZID_ADA || FEATURE_ZID_CLD)
#if !defined FEATURE_ZID
#define FEATURE_ZID                        TRUE
#if !defined FEATURE_GDP
#define FEATURE_GDP                        TRUE  // GDP is inherited with ZID
#endif
#endif
#else
#define FEATURE_ZID                        FALSE
#endif

#if FEATURE_ZID && !(FEATURE_ZID_ADA | FEATURE_ZID_CLD)
#error Must enable Adapter or Class Device functionality or disable FEATURE_ZID.
#endif

#if (FEATURE_ZID_ADA == TRUE) && (FEATURE_ZID_CLD == TRUE)
#error Cannot configure a node to be both an Adaptor and Class Device
#endif

// Default Handshake of success & no pass to Application on Optional Set Report.
#if !defined FEATURE_ZID_SET_REPORT
#define FEATURE_ZID_SET_REPORT             FALSE
#endif

#define ZID_EVT_RSP_WAIT                   0x0010

// A ZID proxy must limit the number of paired ZID devices to the maximum number that it can proxy.
#define ZID_COMMON_MAX_NUM_PROXIED_DEVICES                1

// Maximum number of non standard descriptor fragments per non standard descriptor
#define ZID_MAX_NON_STD_DESC_FRAGS_PER_DESC    ((aplcMaxNonStdDescCompSize + aplcMaxNonStdDescFragmentSize - 1) / aplcMaxNonStdDescFragmentSize)

/* ------------------------------------------------------------------------------------------------
 *                                      ZID Profile Item Ids
 *
 * It may be required to support local Item Id's which differ from ZID-specified Attribute Id's.
 * ------------------------------------------------------------------------------------------------
 */
#define ZID_ITEM_CURRENT_PXY_NUM               0x80
#define ZID_ITEM_CURRENT_PXY_ENTRY             0x81
#define ZID_ITEM_NON_STD_DESC_NUM              0x82
#define ZID_ITEM_NON_STD_DESC_FRAG_NUM         0x83
#define ZID_ITEM_NON_STD_DESC_FRAG             0x84
#define ZID_ITEM_NULL_REPORT                   0x85
#define ZID_ITEM_PXY_LIST                      0x86

#define ZID_ITEM_ZID_PROFILE_VERSION           aplZIDProfileVersion                        // 0xA0
#define ZID_ITEM_INT_PIPE_UNSAFE_TX_TIME       aplIntPipeUnsafeTxWindowTime                // 0xA1
#define ZID_ITEM_REPORT_REPEAT_INTERVAL        aplReportRepeatInterval                     // 0xA2
#define ZID_ITEM_HID_PARSER_VER                aplHIDParserVersion                         // 0xE0
#define ZID_ITEM_HID_DEVICE_SUBCLASS           aplHIDDeviceSubclass                        // 0xE1
#define ZID_ITEM_HID_PROTOCOL_CODE             aplHIDProtocolCode                          // 0xE2
#define ZID_ITEM_HID_COUNTRY_CODE              aplHIDCountryCode                           // 0xE3
#define ZID_ITEM_HID_DEV_REL_NUM               aplHIDDeviceReleaseNumber                   // 0xE4
#define ZID_ITEM_HID_VENDOR_ID                 aplHIDVendorId                              // 0xE5
#define ZID_ITEM_HID_PRODUCT_ID                aplHIDProductId                             // 0xE6
#define ZID_ITEM_HID_NUM_ENDPOINTS             aplHIDNumEndpoints                          // 0xE7
#define ZID_ITEM_HID_POLL_INTERVAL             aplHIDPollInterval                          // 0xE8
#define ZID_ITEM_HID_NUM_STD_DESC_COMPS        aplHIDNumStdDescComps                       // 0xE9
#define ZID_ITEM_HID_STD_DESC_COMPS_LIST       aplHIDStdDescCompsList                      // 0xEA
#define ZID_ITEM_HID_NUM_NULL_REPORTS          aplHIDNumNullReports                        // 0xEB
#define ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS    aplHIDNumNonStdDescComps                    // 0xF0

#if !defined ZID_COMMON_NVID_BEG
#define ZID_COMMON_NVID_BEG 0x80  // Arbitrary, safe NV Id that does not conflict with RTI usages.
#endif

#define ZID_COMMON_NVID_PAIR_INFO         (ZID_COMMON_NVID_BEG + 0)
#define ZID_COMMON_NVID_END               ZID_COMMON_NVID_PAIR_INFO

#define ZID_NVID_INVALID       0x00        // Failure return value for DescSpecN NV Id lookup.
#define ZID_TABLE_IDX_INVALID  0xFF        // Failure return value for the table index lookup.

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

typedef struct {
  uint8 attrId;
  uint8 attrLen;
  uint8 offset;
} zid_table_t;

/* A packed structure of the following items that constitute a ZID proxy table entry (Table 11):
 * Note that the order of items has been changed from Table 11 so as to create a packed structure.
 */
typedef struct {
  uint16 HIDParserVersion;
  uint16 HIDDeviceReleaseNumber;
  uint16 HIDVendorId;
  uint16 HIDProductId;
  uint8  HIDDeviceSubclass;
  uint8  HIDProtocolCode;
  uint8  HIDCountryCode;
  uint8  HIDNumEndpoints;
  uint8  HIDPollInterval;
  uint8  HIDNumStdDescComps;
  uint8  HIDStdDescCompsList[aplcMaxStdDescCompsPerHID];
  uint8  HIDNumNonStdDescComps;
  uint8  HIDNumNullReports;
  uint8  DeviceIdleRate;
  uint8  CurrentProtocol;
} zid_proxy_entry_t;
#define ZID_PROXY_ENTRY_T_ITEMS       15
#define ZID_PROXY_ENTRY_T_SIZE        (sizeof(zid_proxy_entry_t))

#define ZID_PAIR_DISC_CNT                ((RCN_CAP_PAIR_TABLE_SIZE + 7) / 8)
// A packed structure of the following items that constitute the ZID pairing status & configuration.
typedef struct {
  // Discrete bits corrsponding to the Pairing Table index; set when this device is acting as the
  // ZID Profile HID Adapter device within the pair.
  uint8 adapterDisc[ZID_PAIR_DISC_CNT];

  // If the configuration state is interrupted by a power-cycle, then there will be a valid
  // ZID-enabled pairing table entry with an incomplete configuration. So this separate NV
  // structure is required to enable unpairing on the next power-up to remove the entry and allow
  // user to re-initiate push-button pairing when both sides are ready. It is probably not useful
  // to attempt to re-start the configuration state since the other side would have surely already
  // timed-out the configuration state and initiated its own un-pairing.
  uint8 cfgCompleteDisc[ZID_PAIR_DISC_CNT];
} zid_pair_t;

typedef enum {
  eZidStatDiscTxing,     // Transmitting: ZID initiated the send data request.
  eZidStatDiscCnfing,    // Confirming: ZID message sent Control Pipe.
  eZidStatDiscRsping,    // Responding: ZID response expected within aplcMaxResponseWaitTime.
  eZidStatDiscUnsafe,    // Within the Interrupt Pipe Unsafe Tx Window time.
  eZidStatDiscSpare4,
  eZidStatDiscSpare5,
  eZidStatDiscStandby,   // Was in standby mode (when acting as Target device) before wait for Rsp.
  eZidStatDiscTarget     // Acting as a Target vice Controller device.
} zidStatDisc_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/*
 * Build the TX Options Bit Mask for RTI_SendDataReq().
 *
 * TX_OPTIONS = a modifiable uint8 variable.
 * COMM_PIPE  = { ZID_COMM_PIPE_CTRL, ZID_COMM_PIPE_INT_IN, ZID_COMM_PIPE_INT_OUT }
 * BCAST_FLAG = { TRUE, FALSE }
 *
 * Note that a controller should use ZID_COMM_PIPE_INT_IN for rapid mouse movements and
 * ZID_COMM_PIPE_CTRL for high-reliability report (e.g. keyboard).
 * Note that broadcast is not allowed for interrupt pipe transmissions.
 *
 * Note that it is not recommended to use security for rapid mouse movements (e.g. Interrupt pipes).
 */

#if !defined ZID_SECURE_INTERRUPT_PIPES
#define ZID_SECURE_INTERRUPT_PIPES           FALSE
#endif

#if ZID_SECURE_INTERRUPT_PIPES
#define ZID_TX_OPTIONS_INTERRUPT_PIPE       (RTI_TX_OPTION_SINGLE_CHANNEL | RTI_TX_OPTION_SECURITY)
#else
#define ZID_TX_OPTIONS_INTERRUPT_PIPE       (RTI_TX_OPTION_SINGLE_CHANNEL)
#endif

#define ZID_TX_OPTIONS_CONTROL_PIPE \
  (RTI_TX_OPTION_ACKNOWLEDGED | RTI_TX_OPTION_SECURITY)

#define ZID_TX_OPTIONS_CONTROL_PIPE_BROADCAST \
  (/*RTI_TX_OPTION_SINGLE_CHANNEL |*/ RTI_TX_OPTION_BROADCAST)

#define ZID_TX_OPTIONS_CONTROL_PIPE_NO_SECURITY \
  (RTI_TX_OPTION_ACKNOWLEDGED)

#define ZID_TX_OPTIONS_INTERRUPT_PIPE_WITH_SECURITY       (RTI_TX_OPTION_SINGLE_CHANNEL | RTI_TX_OPTION_SECURITY)

// Macros for the 'zidStatDisc' variable.
#define ZID_IS_TXING           GET_BIT(&zidStatDisc, eZidStatDiscTxing)
#define ZID_SET_TXING()        SET_BIT(&zidStatDisc, eZidStatDiscTxing)
#define ZID_CLR_TXING()        CLR_BIT(&zidStatDisc, eZidStatDiscTxing)
#define ZID_IS_CNFING          GET_BIT(&zidStatDisc, eZidStatDiscCnfing)
#define ZID_SET_CNFING()       SET_BIT(&zidStatDisc, eZidStatDiscCnfing)
#define ZID_CLR_CNFING()       CLR_BIT(&zidStatDisc, eZidStatDiscCnfing)
#define ZID_IS_RSPING          GET_BIT(&zidStatDisc, eZidStatDiscRsping)
#define ZID_SET_RSPING()       SET_BIT(&zidStatDisc, eZidStatDiscRsping)
#define ZID_CLR_RSPING()       CLR_BIT(&zidStatDisc, eZidStatDiscRsping)
#define ZID_IS_UNSAFE          GET_BIT(&zidStatDisc, eZidStatDiscUnsafe)
#define ZID_SET_UNSAFE()       SET_BIT(&zidStatDisc, eZidStatDiscUnsafe)
#define ZID_CLR_UNSAFE()       CLR_BIT(&zidStatDisc, eZidStatDiscUnsafe)

#define ZID_WAS_STANDBY        GET_BIT(&zidStatDisc, eZidStatDiscStandby)
#define ZID_SET_STANDBY()      SET_BIT(&zidStatDisc, eZidStatDiscStandby)
#define ZID_CLR_STANDBY()      CLR_BIT(&zidStatDisc, eZidStatDiscStandby)
#define ZID_IS_CTL            !ZID_IS_TGT
#define ZID_IS_TGT             GET_BIT(&zidStatDisc, eZidStatDiscTarget)
#define ZID_SET_TGT()          SET_BIT(&zidStatDisc, eZidStatDiscTarget)
#define ZID_CLR_TGT()          CLR_BIT(&zidStatDisc, eZidStatDiscTarget)

// TRUE if this device is acting as the ZID Profile HID Adapter for the pairing reference.
#define ZID_IS_ADA(PAIR_IDX)   GET_BIT(zidPairInfo.adapterDisc, PAIR_IDX)

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

// Upon successful Tx of any command which expects a response, only return to power-cycling or
// Rx-off upon receiving a response from the intended pair or expiration of the response wait timer.
// Stronger receipt control is exercised on messages related to Configuration state by checking
// against the zidState and zidCfgIndex.
extern uint8 zidRspIdx;

// RTI Pairing Table index of the destination of a ZID configuration phase.
extern uint8 zidCfgIdx;

extern uint8 zidCnfIdx;  // RTI Pairing Table index of the destination of a zidSendDataReq().

extern zidStatDisc_t zidStatDisc;
extern zid_pair_t zidPairInfo;
// RAM cached copy of the IntPipeUnsafeTxWindowTime for faster, frequent use.
extern uint16 zidUnsafeTime;
extern const uint8 ZID_StdReportLen[ZID_STD_REPORT_TOTAL_NUM+1];

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

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
void zidInitItems(void);

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
void zidInitCnf(void);

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
uint8 zidSendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd);

/**************************************************************************************************
 * @fn      zidSendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API call.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  TRUE if ZID initiated the send data request; FALSE otherwise.
 */
uint8 zidSendDataCnf(rStatus_t status);

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
                         uint8 *pData );

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
uint8 zidUnpair(uint8 dstIndex);

/**************************************************************************************************
 *
 * @fn          zidReadItem
 *
 * @brief       This API is used to read the ZID Configuration Interface item
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
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidReadItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 *
 * @fn          zidWriteItem
 *
 * @brief       This API is used to write ZID Configuration Interface parameters
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
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidWriteItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 * @fn          zidDescSpecNvId
 *
 * @brief       Attempt to find the NV Id used to store the 'attrId' for the 'pxyId'.
 *
 * input parameters
 *
 * @param       attrId - A valid DescSpecN attribute Id.
 * @param       pxyId - A valid index into the pxyIdxTable[].
 *
 * output parameters
 *
 * None.
 *
 * @return      The NV Id used to store the requested DescSpecN; ZID_NV_ID_INVALID if not found.
 */
uint8 zidDescSpecNvId(uint8 attrId, uint8 pxyId);

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
void zidRspDone(uint8 taskId, uint8 status);

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
void zidRspWait(uint8 taskId, uint16 timeout);

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
void zidCommon_ResetNonStdDescCompData( void );

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
uint8 zidCommon_ProcessNonStdDescComp( uint8 len, uint8 *pData, bool *descComplete );

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
uint8 zidCommon_GetNonStdDescCompFragNum( zid_non_std_desc_comp_t *pDesc  );

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
uint8 zidCommon_GetLastNonStdDescCompFragNum( void );

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
uint8 zidCommon_specCheckWrite(uint8 attrId, uint8 len, uint8 *pValue);

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
