/**************************************************************************************************
  Filename:       rcn_nwk.h
  Revised:        $Date: 2011-09-07 18:03:16 -0700 (Wed, 07 Sep 2011) $
  Revision:       $Revision: 27485 $

  Description:    This file contains the the Remote Control Network Layer interfaces


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

#ifndef RCN_NWK_H
#define RCN_NWK_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "rcn_attribs.h"
#include "saddr.h"

#if !defined _WIN32 && !defined RNP_HOST
#include "hal_mcu.h"
#include "mac_api.h"
#endif

// -- macros --

// Define a prefix macro for exported functions and global variables
// which can be used for generating proper DLL interface for windows platform
#ifdef _WIN32
#  ifdef RTILIB_EXPORTS
#    define RCNLIB_API __declspec(dllexport)
#  else
#    define RCNLIB_API __declspec(dllimport)
#  endif
#else
#  define RCNLIB_API
#endif

#ifndef ATTR_PACKED
# ifdef __GNUC__
#  define ATTR_PACKED __attribute__ ((__packed__))
# else
#  define ATTR_PACKED
# endif
#endif


/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

#ifndef MAC_SUCCESS
#  define MAC_SUCCESS                0
#  define MAC_INVALID_INDEX          0xF9
#  define MAC_INVALID_PARAMETER      0xE8
#  define MAC_UNSUPPORTED_ATTRIBUTE  0xF4
#endif

// Network layer primitive status field values
#define RCN_SUCCESS                               MAC_SUCCESS
#define RCN_ERROR_INVALID_INDEX                   MAC_INVALID_INDEX
#define RCN_ERROR_INVALID_PARAMETER               MAC_INVALID_PARAMETER
#define RCN_ERROR_UNSUPPORTED_ATTRIBUTE           MAC_UNSUPPORTED_ATTRIBUTE
#define RCN_ERROR_NO_ORG_CAPACITY                 0xB0
#define RCN_ERROR_NO_REC_CAPACITY                 0xB1
#define RCN_ERROR_NO_PAIRING                      0xB2
#define RCN_ERROR_NO_RESPONSE                     0xB3
#define RCN_ERROR_NOT_PERMITTED                   0xB4
#define RCN_ERROR_DUPLICATE_PAIRING               0xB5
#define RCN_ERROR_FRAME_COUNTER_EXPIRED           0xB6
#define RCN_ERROR_DISCOVERY_ERROR                 0xB7
#define RCN_ERROR_DISCOVERY_TIMEOUT               0xB8
#define RCN_ERROR_SECURITY_TIMEOUT                0xB9
#define RCN_ERROR_SECURITY_FAILURE                0xBA

  // -- non-standard error codes --
#define RCN_ERROR_NO_SECURITY_KEY                 0xBD
#define RCN_ERROR_OUT_OF_MEMORY                   0xBE
#define RCN_ERROR_COMMUNICATION                   0xBF


// RF4CE Network layer constants

////////////////
// Supported channels
#define RCN_MAC_CHANNEL_1        MAC_CHAN_15
#define RCN_MAC_CHANNEL_2        MAC_CHAN_20
#define RCN_MAC_CHANNEL_3        MAC_CHAN_25
#define RCN_NUM_MAC_CHANNELS     3

////////////////
// RF4CE Constants
#define RCN_NWKC_CHANNEL_MASK      (MAC_CHAN_MASK(RCN_MAC_CHANNEL_1)| \
                                    MAC_CHAN_MASK(RCN_MAC_CHANNEL_2)| \
                                    MAC_CHAN_MASK(RCN_MAC_CHANNEL_3))

#define RCN_NWKC_MAX_DUTY_CYCLE    1000 // 1 second
#define RCN_NWKC_FRAME_COUNTER_WINDOW 1024
#define RCN_NWKC_MAX_KEY_SEED_WAIT_TIME 60

#if !defined MACNODEBUG  // The RCN Library build shall not depend on the pairing table size.
#ifndef RCN_CAP_PAIR_TABLE_SIZE
#define RCN_CAP_PAIR_TABLE_SIZE  10 // nwkcMaxPairingTableEntries
#endif
#endif
#define RCN_CAP_MAX_PAIRS        gRCN_CAP_MAX_PAIRS

// The following corresponds to nwkcMaxNodeDescListSize
#ifndef RCN_CAP_NODE_DESC_TABLE_SIZE
#define RCN_CAP_NODE_DESC_TABLE_SIZE  10 // determines maximum number of discovered events
#endif
#if RCN_CAP_NODE_DESC_TABLE_SIZE < 3 // nwkcMinNodeDescTableSize
#  error "RCN_CAP_NODE_DESC_TABLE_SIZE does not meet standard specified minimum"
#endif

// nwkcMinActivePeriod is not defined as it is not checked against
// nwkcMinTargetPairingTableSize is not defined
// nwkcMinNWKHeaderOverhead is not defined
// nwkcMinControllerPairingTableSize is not defined
// nwkcNodeCapabilities is not defined as constant.

#define RCN_NWKC_PROTOCOL_IDENTIFIER                 0xce
#define RCN_NWKC_PROTOCOL_VERSION                    0x01

// nwkcVendorIdentifier is not defined as constant.
// nwkcVendorString is not defined as a constant.

// This will most likely become a standard constant.
#define RCN_SECURITY_MVAL                         4
#define RCN_SECURITY_LEVEL                        5
// The above two values have correlation to each other.

// Node capabilities bits
#define RCN_NODE_CAP_TARGET                       0x01
#define RCN_NODE_CAP_POWER_SOURCE                 0x02
#define RCN_NODE_CAP_SECURITY_CAPABLE             0x04
#define RCN_NODE_CAP_CHANNEL_NORMALIZATION_CAPABLE 0x08
#define RCN_NODE_CAP_NUM_OF_PROFILES              0x70

// TX OPTIONS BIT MASK
#define RCN_TX_OPTION_BROADCAST                   0x01
#define RCN_TX_OPTION_IEEE_ADDRESS                0x02
#define RCN_TX_OPTION_ACKNOWLEDGED                0x04
#define RCN_TX_OPTION_SECURITY                    0x08
#define RCN_TX_OPTION_SINGLE_CHANNEL              0x10
#define RCN_TX_OPTION_CHANNEL_DESIGNATOR          0x20
#define RCN_TX_OPTION_VENDOR_SPECIFIC             0x40

// RX FLAGS BIT MASK
#define RCN_RX_FLAGS_BROADCAST                    0x01
#define RCN_RX_FLAGS_SECURITY                     0x02
#define RCN_RX_FLAGS_VENDOR_SPECIFIC              0x04

// DstAddrType bits
#define RCN_ADDR_TYPE_IEEE_ADDRESS                0x01

// Wild card device type used in discovery request
#define RCN_DEVICE_TYPE_WILDCARD                  0xff

// Callback event ID enumeration
enum
{
  RCN_NLDE_DATA_IND = 1,
  RCN_NLDE_DATA_CNF,
  RCN_NLME_COMM_STATUS_IND,
  RCN_NLME_DISCOVERY_IND,
  RCN_NLME_DISCOVERED_EVENT,
  RCN_NLME_DISCOVERY_CNF,
  RCN_NLME_GET_CNF,
  RCN_NLME_PAIR_IND,
  RCN_NLME_PAIR_CNF,
  RCN_NLME_RESET_CNF,
  RCN_NLME_RX_ENABLE_CNF,
  RCN_NLME_SET_CNF,
  RCN_NLME_START_CNF,
  RCN_NLME_UNPAIR_CNF,
  RCN_NLME_UNPAIR_IND,
  RCN_NLME_AUTO_DISCOVERY_CNF,
  RCN_NLME_DISCOVERY_ABORT_CNF,
  RCN_NLME_EVENTID_RESERVED_0 // ceiling of the callback event id
};

// Invalid paiirng reference value to be used to clear a pairing table entry
// with RCN_NLME_SET_PAIRING_ENTRY_REQ primitive.
#define RCN_INVALID_PAIRING_REF   0xff

// Identifiers of non-volatile memory items used by network layer
#define RCN_NVID_NULL                                    0x00
#define RCN_NVID_IEEE_ADDRESS                            0x01
#define RCN_NVID_NWK_ACTIVE_PERIOD                       0x02
#define RCN_NVID_NWK_DISCOVERY_LQI_THRESHOLD             0x03
#define RCN_NVID_NWK_DISCOVERY_REPETITION_INTERVAL_IN_MS 0x04
#define RCN_NVID_NWK_DUTY_CYCLE_IN_MS                    0x05
#define RCN_NVID_NWK_FRAME_COUNTER                       0x06
#define RCN_NVID_NWK_INDICATE_DISCOVERY_REQUESTS         0x07
#define RCN_NVID_NWK_MAX_DISCOVERY_REPETITIONS           0x08
#define RCN_NVID_NWK_MAX_FIRST_ATTEMPT_CSMA_BACKOFFS     0x09
#define RCN_NVID_NWK_MAX_FIRST_ATTEMPT_FRAME_RETRIES     0x0A
#define RCN_NVID_NWK_MAX_REPORTED_NODE_DESCRIPTORS       0x0B
#define RCN_NVID_NWK_RESPONSE_WAIT_TIME                  0x0C
#define RCN_NVID_NWK_SCAN_DURATION                       0x0D
#define RCN_NVID_NWK_USER_STRING                         0x0E
#define RCN_NVID_NWKC_NODE_CAPABILITIES                  0x0F
#define RCN_NVID_NWKC_VENDOR_IDENTIFIER                  0x10
#define RCN_NVID_NWKC_VENDOR_STRING                      0x11
#define RCN_NVID_STARTED                                 0x21
#define RCN_NVID_PAN_ID                                  0x22
#define RCN_NVID_SHORT_ADDRESS                           0x23
#define RCN_NVID_PAIRING_ENTRY_0                         0x24
// Pairing entries continues and hence RCN_NVID_PAIRING_ENTRY_0 has to be defined
// as the last among the above.

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

#ifdef _MSC_VER
#pragma pack(1)
#endif

/* Primitive Data Structures */

// sub structure for application capabilities field in macro
#define RCN_APP_CAPA_GET_USER_STRING(_capa) \
  ((_capa) & 0x01)
#define RCN_APP_CAPA_SET_USER_STRING(_capa,_val) \
{                                                \
  ((_capa) &= ~0x01);                            \
  ((_capa) |= (_val));                           \
}
#define RCN_APP_CAPA_GET_NUM_DEV_TYPES(_capa) \
  (((_capa) & 0x06) >> 1)
#define RCN_APP_CAPA_SET_NUM_DEV_TYPES(_capa,_val) \
{                                                  \
  ((_capa) &= ~0x06);                              \
  ((_capa) |= ((_val) << 1));                      \
}
#define RCN_APP_CAPA_GET_NUM_PROFILES(_capa) \
  (((_capa) & 0x70) >> 4)
#define RCN_APP_CAPA_SET_NUM_PROFILES(_capa,_val) \
{                                                 \
  ((_capa) &= ~0x70);                             \
  ((_capa) |= ((_val) << 4));                     \
}

// complete application capabilities structure to send its own capabilities
PACK_1 typedef struct ATTR_PACKED
{
  uint8 appCapabilities;
  uint8 devTypeList[RCN_MAX_NUM_DEV_TYPES];
  uint8 profileIdList[RCN_MAX_NUM_PROFILE_IDS];
} rcnAppInfo_t;

// complete device information of a peer device as received (except for node capabilities field)
PACK_1 typedef struct ATTR_PACKED
{
  uint8 nodeCapabilities;
  uint16 vendorId;
  uint8 vendorString[RCN_VENDOR_STRING_LENGTH];
  uint8 appCapabilities;
  uint8 userString[RCN_USER_STRING_LENGTH];
  uint8 devTypeList[RCN_MAX_NUM_DEV_TYPES];
  uint8 profileIdList[RCN_MAX_NUM_PROFILE_IDS];
} rcnDevInfo_t;

// NLDE-DATA.Request
PACK_1 typedef struct ATTR_PACKED
{
  uint8 pairingRef;
  uint8 profileId;
  uint16 vendorId;
  uint8 nsduLength;
  uint8 txOptions;
  uint8 *internal; // internal data structure. Do not use.
  uint8 *nsdu;
} rcnNldeDataReq_t;

// NLDE-DATA.Indication
PACK_1 typedef struct ATTR_PACKED
{
  uint8 pairingRef;
  uint8 profileId;
  uint16 vendorId;
  uint8 nsduLength;
  uint8 rxLinkQuality;
  uint8 rxFlags;
  uint8 *nsdu;
} rcnNldeDataInd_t;

// NLDE-DATA.Confirmation
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 pairingRef;
} rcnNldeDataCnf_t;

// NLME-COMM-STATUS.Indication
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 pairingRef;
  uint16 dstPanId;
  uint8 dstAddrMode;
  union {
    uint8 ieeeAddress[8];
    uint16 shortAddress;
  } dstAddr;
} rcnNlmeCommStatusInd_t;

// NLME-DISCOVERY.Reqeust
PACK_1 typedef struct ATTR_PACKED
{
  uint16 dstPanId;
  uint16 dstNwkAddr;
  rcnAppInfo_t appInfo;
  uint8 searchDevType;
  uint8 discProfileIdListSize;
  // although spec can have up to 255 entries, practically as OrgProfileIdList is
  // limited to 7, the DiscProfileIdList is also limited to the same number.
  uint8 discProfileIdList[RCN_MAX_NUM_PROFILE_IDS];
  uint16 discDurationInMs;  // discovery duration is in milisecond unit and limited to 16 bit value
} rcnNlmeDiscoveryReq_t;

// NLME-DISCOVERY.Response
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  sAddrExt_t dstIeeeAddress;
  rcnAppInfo_t appInfo;
  uint8 discReqLqi;
} rcnNlmeDiscoveryRsp_t;

// NLME-DISCOVERY.Indication
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  sAddrExt_t orgIeeeAddress;
  rcnDevInfo_t orgDevInfo;
  uint8 searchDevType;
  uint8 rxLinkQuality;
} rcnNlmeDiscoveryInd_t;

// Parital NLME-DISCOVERY.Confirm to pass a single record
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 logicalChannel;
  uint16 panId;
  sAddrExt_t ieeeAddress;
  rcnDevInfo_t devInfo;
  uint8 discReqLqi;
} rcnNlmeDiscoveredEvent_t;

// NLME-DISCOVERY.Confirm without target descriptors
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 numNodes;
} rcnNlmeDiscoveryCnf_t;

// NLME-AUTO-DISCOVERY.request
PACK_1 typedef struct ATTR_PACKED
{
  uint16 autoDiscDurationInMs; // NOTE that unit is ms and size is of 16 bit
  rcnAppInfo_t appInfo;
} rcnNlmeAutoDiscoveryReq_t;

// NLME-DISCOVERY-ACCEPT.confirm
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  sAddrExt_t srcIeeeAddr;
} rcnNlmeAutoDiscoveryCnf_t;

// NLME-PAIR.Request
PACK_1 typedef struct ATTR_PACKED
{
  uint8 logicalChannel;
  sAddrExt_t dstIeeeAddress;
  uint16 dstPanId;
  rcnAppInfo_t appInfo;
  uint8 keyExTransferCount;
} rcnNlmePairReq_t;

// NLME-PAIR.Response
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint16 dstPanId;
  sAddrExt_t dstIeeeAddr;
  rcnAppInfo_t appInfo;
  uint8 provPairingRef;
} rcnNlmePairRsp_t;

// NLME-PAIR.Indication
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint16 srcPanId;
  sAddrExt_t orgIeeeAddress;
  rcnDevInfo_t orgDevInfo;
  uint8 provPairingRef;
  uint8 keyExTransferCount;
} rcnNlmePairInd_t;

// NLME-PAIR.Confirm
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 pairingRef;
  rcnDevInfo_t recDevInfo;
} rcnNlmePairCnf_t;

// NLME-START.Confirm
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
} rcnNlmeStartCnf_t;

// NLME-UNPAIR.Confirm
PACK_1 typedef struct ATTR_PACKED
{
  uint8 status;
  uint8 pairingRef;
} rcnNlmeUnpairCnf_t;

// NLME-UNPAIR.Indication
PACK_1 typedef struct ATTR_PACKED
{
  uint8 pairingRef;
} rcnNlmeUnpairInd_t;

// callback event structure
PACK_1 typedef struct ATTR_PACKED
{
  uint8 eventId;
  union
  {
    rcnNldeDataInd_t dataInd;
    rcnNldeDataCnf_t dataCnf;
    rcnNlmeCommStatusInd_t commStatusInd;
    rcnNlmeDiscoveryInd_t discoveryInd;
    rcnNlmeDiscoveredEvent_t discoveredEvent;
    rcnNlmeDiscoveryCnf_t discoveryCnf;
    rcnNlmeAutoDiscoveryCnf_t autoDiscoveryCnf;
    rcnNlmePairInd_t pairInd;
    rcnNlmePairCnf_t pairCnf;
    rcnNlmeStartCnf_t startCnf;
    rcnNlmeUnpairCnf_t unpairCnf;
    rcnNlmeUnpairInd_t unpairInd;
  } prim; // primitive union
} rcnCbackEvent_t;


/* This union of request and response primitives is not used in network layer
   codes, but it is a good recommendation for an application to use this union
   when calling network layer interface functions.
*/
PACK_1 typedef struct ATTR_PACKED
{
  uint8 primId;
  union
  {
    rcnNldeDataReq_t dataReq;
    rcnNlmeDiscoveryReq_t discoveryReq;
    rcnNlmeDiscoveryRsp_t discoveryRsp;
    rcnNlmePairReq_t pairReq;
    rcnNlmePairRsp_t pairRsp;
    rcnNlmeAutoDiscoveryReq_t autoDiscoveryReq;
  } prim; // primitive union
} rcnReqRspPrim_t;


#ifdef _MSC_VER
#pragma pack()
#endif

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

// The following two variables hold the SFD mark of last transmitted or received network layer data.
extern uint32 rcnMcpsTimestamp;
extern uint16 rcnMcpsTimestamp2;
extern const uint8 gRCN_CAP_MAX_PAIRS;

/*********************************************************************
 * FUNCTIONS
 */


/**************************************************************************************************
 * @fn          RCN_CbackEvent
 *
 * @brief       This is a function prototype for a callback function from RF4CE network layer.
 *              Network layer shall de-allocate the memory block allocated for event structure
 *              immediately after return of this function call and hence application should not
 *              dereference event structure after this function returns.
 *
 * input parameters
 *
 * @param       event - a callback event
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern void RCN_CbackEvent( rcnCbackEvent_t *event );


/**************************************************************************************************
 * @fn          RCN_CbackRxCount
 *
 * @brief       This is a function prototype for a callback function from RF4CE network layer.
 *              This function is valid only when test mode feature is turned on at build time.
 *              Network layer shall call this callback function to indicate that a CRC passed
 *              MAC frame was received.
 *              Client should increment its receive counter.
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
extern void RCN_CbackRxCount(void);


/**************************************************************************************************
 * @fn          RCN_NldeDataAlloc
 *
 * @brief       This function is a special function to allocate a memory block
 *              where NSDU should be copied into before passing NLDE-DATA.Request primitive
 *              to network layer.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure describing NLDE-Data.Request.
 *                          nsdu field of the C structure will be filled in before this function
 *                          returns if the function successfully allocated memory block.
 *
 * output parameters
 *
 * None.
 *
 * @return      either RCN_SUCCESS or RCN_ERROR_OUT_OF_MEMORY
 */
extern RCNLIB_API uint8 RCN_NldeDataAlloc( rcnNldeDataReq_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NldeDataReq
 *
 * @brief       This function requests transfer of a data NSDU.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure containing NLDE-Data.Request.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NldeDataReq( rcnNldeDataReq_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NlmeDiscoveryReq
 *
 * @brief       This function requests discovery of other devices of interest operating in the
 *              personal operating space of the device.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure containing NLME-Discovery.Request.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeDiscoveryReq( rcnNlmeDiscoveryReq_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NlmeDiscoveryAbortReq
 *
 * @brief       This function requests aborting on-going discovery.
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
extern RCNLIB_API void RCN_NlmeDiscoveryAbortReq( void );

/**************************************************************************************************
 * @fn          RCN_NlmeDiscoveryRsp
 *
 * @brief       This function requests the local NLME to respond to a discovery request command.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure containing NLME-Discovery.Response.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeDiscoveryRsp( rcnNlmeDiscoveryRsp_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NlmeGetReq
 *
 * @brief       This function retrieves an Network Information Base (NIB) attribute value.
 *
 * input parameters
 *
 * @param       attribute - NIB attribute enumeration
 * @param       attributeIndex - NIB attribute index in case NIB attribute is a table or an array
 *                               type
 *
 * output parameters
 *
 * @param       value     - pointer to a data block where to copy the attribute value into.
 *
 * @return      RCN_SUCCESS or RCN_ERROR_UNSUPPORTED_ATTRIBUTE
 */
extern RCNLIB_API uint8 RCN_NlmeGetReq( uint8 attribute, uint8 attributeIndex, uint8 *value );

/**************************************************************************************************
 * @fn          RCN_NlmeGetSizeReq
 *
 * @brief       This function retrieves an Network Information Base (NIB) attribute value size.
 *              In case the attribute is a table or an array, the returned size is that of an
 *              element.
 *
 * input parameters
 *
 * @param       attribute - NIB attribute enumeration
 *
 * output parameters
 *
 * NOne.
 *
 * @return      Attribute value size or 0 if attribute is not found
 */
extern RCNLIB_API uint8 RCN_NlmeGetSizeReq( uint8 attribute );

/**************************************************************************************************
 * @fn          RCN_NlmeGetPairingEntryReq
 *
 * @brief       This function retrieves a pairing table entry.
 *              This function is an auxiliary function complementing RCN_NlmeGetReq() function
 *              as copying a whole pairing table is unrealistic.
 *              This function is available only to the application residing in the same process(or).
 *
 * input parameters
 *
 * @param       pairingRef - pairing reference number
 *
 * output parameters
 *
 * @param       entry     - address of a pointer where to store location of the pairing table
 *                          entry.
 *
 * @return      RCN_SUCCESS or RCN_ERROR_NO_PAIRING
 */
extern uint8 RCN_NlmeGetPairingEntryReq( uint8 pairingRef, rcnNwkPairingEntry_t **entry );

/**************************************************************************************************
 * @fn          RCN_NlmePairReq
 *
 * @brief       This function requests the local NLME to pair with another device.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure containing NLME-PAIR.Request.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmePairReq( rcnNlmePairReq_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NlmePairRsp
 *
 * @brief       This function requests the local NLME to respond to a pairing request command.
 *
 * input parameters
 *
 * @param       primitive - a pointer to a C structure containing NLME-PAIR.Response.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmePairRsp( rcnNlmePairRsp_t *primitive );

/**************************************************************************************************
 * @fn          RCN_NlmeResetReq
 *
 * @brief       This function resets network layer.
 *
 * input parameters
 *
 * @param       setDefaultNib - If TRUE, the NWK layer is reset and all NIB attributes are set
 *                              to their default values.
 *                              If FALSE, the NWK layer is reset but all NIB attributes retain
 *                              their values prior to the generation of the NLME-RESET.request
 *                              primitive.
 *
 * output parameters
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeResetReq( uint8 setDefaultNib );

/**************************************************************************************************
 * @fn          RCN_NlmeRxEnableReq
 *
 * @brief       This function requests that the receiver is either enabled (for a finite period
 *              or until further notice) or disabled..
 *
 * input parameters
 *
 * @param       rxOnDurationInMs - Duration in ms for which the receiver is to be enabled.
 *                                 If this parameter is equal to 0x0000,
 *                                 the receiver is to be disabled.
 *                                 If this parameter is equal to 0xffff,
 *                                 the receiver is to be enabled until further notice.
 *
 * output parameters
 *
 * @return      RCN_SUCCESS, MAC_PAST_TIME, MAC_ON_TIME_TOO_LONG or MAC_INVALID_PARAMETER
 */
extern RCNLIB_API uint8 RCN_NlmeRxEnableReq( uint16 rxOnDurationInMs );

/**************************************************************************************************
 * @fn          RCN_NlmeSetReq
 *
 * @brief       This function sets an NIB attribute value.
 *
 * input parameters
 *
 * @param       attribute - The identifier of the NIB attribute to write.
 * @param       attributeIndex - index within a table or an array if the NIB attibute is a table
 *                               or an array
 *
 * @param       value - pointer to attribute value
 *
 * output parameters
 *
 * @return      RCN_SUCCESS, RCN_ERROR_UNSUPPORTED_ATTRIBUTE or RCN_ERROR_INVALID_INDEX
 */
extern RCNLIB_API uint8 RCN_NlmeSetReq( uint8 attribute, uint8 attributeIndex, uint8 *value );

/**************************************************************************************************
 * @fn          RCN_NlmeSetPairingEntryReq
 *
 * @brief       This function sets a pairing table entry if an application modified a pairing
 *              entry retrieved by RCN_NlmeGetPairingEntryReq call.
 *              This function is available only to the application residing in the same process(or).
 *
 * input parameters
 *
 * @param       index - index to the pairing table
 *
 * output parameters
 *
 * @return      RCN_SUCCESS or RCN_ERROR_INVALID_INDEX
 */
extern uint8 RCN_NlmeSetPairingEntryReq( uint8 index );

/**************************************************************************************************
 * @fn          RCN_NlmeStartReq
 *
 * @brief       This function requests NLME to start network.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeStartReq( void );

/**************************************************************************************************
 * @fn          RCN_NlmeUnpairReq
 *
 * @brief       This function removes an entry from the pairing table.
 *              This function will trigger a callback of RCN_NLME_UNPAIR_CNF with
 *              the following status codes:
 *              RCN_SUCCESS, RCN_ERROR_NO_PAIRING, MAC_TRANSACTION_OVERFLOW,
 *              MAC_TRANSACTION_EXPIRED, MAC_CHANNEL_ACCESS_FAILURE, MAC_INVALID_ADDRESS,
 *              MAC_NO_ACK, MAC_COUNTER_ERROR, MAC_FRAME_TOO_LONG, MAC_UNAVAILABLE_KEY,
 *              MAC_UNSUPPORTED_SECURITY or MAC_INVALID_PARAMETER
 *
 * input parameters
 *
 * @param       pairingRef - The reference into the local pairing table of the entry
 *                           that is to be removed.
 *
 * output parameters
 *
 * None
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeUnpairReq( uint8 pairingRef );


/**************************************************************************************************
 * @fn          RCN_NlmeUnpairRsp
 *
 * @brief       This function removes an entry from the pairing table in response to
 *              NLME-UNPAIR.indication.
 *
 * input parameters
 *
 * @param       pairingRef - The reference into the local pairing table of the entry
 *                           that is to be removed.
 *
 * output parameters
 *
 * @return      None
 */
extern RCNLIB_API void RCN_NlmeUnpairRsp( uint8 pairingRef );


/**************************************************************************************************
 * @fn          RCN_NlmeAutoDiscoveryReq
 *
 * @brief       This function configures the local NLME to respond to a discovery request command.
 *
 * input parameters
 *
 * @param       pPrimitive - a pointer to a C structure containing NLME-DISCOVERY-ACCEPT.request.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern RCNLIB_API void RCN_NlmeAutoDiscoveryReq( rcnNlmeAutoDiscoveryReq_t *pPrimitive );


/**************************************************************************************************
 * @fn          RCN_NlmeAutoDiscoveryAbortReq
 *
 * @brief       This function requests aborting on-going auto-discovery.
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
extern RCNLIB_API void RCN_NlmeAutoDiscoveryAbortReq( void );


/**************************************************************************************************
 * @fn          RCN_NlmeUpdateKeyReq
 *
 * @brief       This function requests changing a security link key of a pairing table entry.
 *
 * input parameters
 *
 * @param       pairingRef - pairing reference of the pairing entry
 * @param       pNewLinkKey - pointer to new 128-bit key array
 *
 * output parameters
 *
 * None.
 *
 * @return      RCN_SUCCESS, RCN_ERROR_NO_PAIRING or RCN_ERROR_NOT_PERMITTED.
 */
extern RCNLIB_API uint8 RCN_NlmeUpdateKeyReq(uint8 pairingRef, uint8 *pNewLinkKey);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RCN_NWK_H */
