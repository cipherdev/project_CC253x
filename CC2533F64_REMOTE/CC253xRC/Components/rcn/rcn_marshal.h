/**************************************************************************************************
  Filename:       rcn_marshal.h
  Revised:        $Date: 2010-08-04 18:40:21 -0700 (Wed, 04 Aug 2010) $
  Revision:       $Revision: 23311 $

  Description:    This file contains constants and type definitions used for data marshalling
                  of RemoTI network layer primitives.


  Copyright 2008-2010 Texas Instruments Incorporated. All rights reserved.

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

#ifndef RCN_MARSHAL_H
#define RCN_MARSHAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "rcn_nwk.h"

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

// Primitive ID enumerations
enum
{
  RCN_NLDE_DATA_REQ = 1,
  RCN_NLME_DISCOVERY_REQ,
  RCN_NLME_DISCOVERY_RSP,
  RCN_NLME_GET_REQ,
  RCN_NLME_PAIR_REQ,
  RCN_NLME_PAIR_RSP,
  RCN_NLME_RESET_REQ,
  RCN_NLME_RX_ENABLE_REQ,
  RCN_NLME_SET_REQ,
  RCN_NLME_START_REQ,
  RCN_NLME_UNPAIR_REQ,
  RCN_NLME_UNPAIR_RSP,
  RCN_NLME_AUTO_DISCOVERY_REQ,
  RCN_NLME_AUTO_DISCOVERY_ABORT_REQ,
  RCN_NLME_DISCOVERY_ABORT_REQ
};


/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

#ifdef _MSC_VER
#pragma pack(1)
#endif

// -- Serialized primitive structures --

// NLDE-DATA.Request
PACK_1 typedef struct
{
  uint8 pairingRef;
  uint8 profileId;
  uint16 vendorId;
  uint8 nsduLength;
  uint8 txOptions;
  // NSDU follows in variable length
} rcnNldeDataReqStream_t;

// NLDE-DATA.Indication
PACK_1 typedef struct
{
  uint8 pairingRef;
  uint8 profileId;
  uint16 vendorId;
  uint8 nsduLength;
  uint8 rxLinkQuality;
  uint8 rxFlags;
  // NSDU follows in variable length
} rcnNldeDataIndStream_t;

// NLME-GET.Reqeust
PACK_1 typedef struct
{
  uint8 attribute;
  uint8 attributeIndex;
} rcnNlmeGetReq_t;

// NLME-GET.Confirm
PACK_1 typedef struct
{
  uint8 status;
  uint8 attribute;
  uint8 attributeIndex;
  uint8 length;
  // attribute value in variable length follows
} rcnNlmeGetCnf_t;

// NLME-RESET.Request
PACK_1 typedef struct
{
  uint8 setDefaultNib;
} rcnNlmeResetReq_t;

// NLME-RESET.Confirm
PACK_1 typedef struct
{
  uint8 status;
} rcnNlmeResetCnf_t;

// NLME-RX-ENABLE.Request
PACK_1 typedef struct
{
  uint16 rxOnDurationInMs;
} rcnNlmeRxEnableReq_t;

// NLME-RX-ENABLE.Confirm
PACK_1 typedef struct
{
  uint8 status;
} rcnNlmeRxEnableCnf_t;

// NLME-SET.Request
PACK_1 typedef struct
{
  uint8 nibAttribute;
  uint8 nibAttributeIndex;
  uint8 length;
  // attribute value in variable length follows
} rcnNlmeSetReq_t;

// NLME-SET.Confirm
PACK_1 typedef struct
{
  uint8 status;
  uint8 nibAttribute;
  uint8 nibAttributeIndex;
} rcnNlmeSetCnf_t;

// NLME-START.Request has empty payload

// NLME-UNPAIR.Request
PACK_1 typedef struct
{
  uint8 pairingRef;
} rcnNlmeUnpairReq_t;

// NLME-UNPAIR.response
PACK_1 typedef struct
{
  uint8 pairingRef;
} rcnNlmeUnpairRsp_t;

// Unified request and response primitive without pointers inside
PACK_1 typedef struct
{
  uint8 primId;
  union
  {
    rcnNldeDataReqStream_t dataReq;
    rcnNlmeDiscoveryReq_t discoveryReq;
    rcnNlmeDiscoveryRsp_t discoveryRsp;
    rcnNlmeGetReq_t getReq;
    rcnNlmePairReq_t pairReq;
    rcnNlmePairRsp_t pairRsp;
    rcnNlmeResetReq_t resetReq;
    rcnNlmeRxEnableReq_t rxEnableReq;
    rcnNlmeSetReq_t setReq;
    rcnNlmeUnpairReq_t unpairReq;
    rcnNlmeUnpairRsp_t unpairRsp;
    rcnNlmeAutoDiscoveryReq_t autoDiscoveryReq;
  } prim;
} rcnReqRspSerialized_t;

// Unified confirm and indication primitive without pointers inside
PACK_1 typedef struct
{
  uint8 primId;
  union
  {
    rcnNldeDataIndStream_t dataInd;
    rcnNldeDataCnf_t dataCnf;
    rcnNlmeCommStatusInd_t commStatusInd;
    rcnNlmeDiscoveryInd_t discoveryInd;
    rcnNlmeDiscoveredEvent_t discoveredEvent;
    rcnNlmeDiscoveryCnf_t discoveryCnf;
    rcnNlmeGetCnf_t getCnf;
    rcnNlmePairInd_t pairInd;
    rcnNlmePairCnf_t pairCnf;
    rcnNlmeResetCnf_t resetCnf;
    rcnNlmeRxEnableCnf_t rxEnableCnf;
    rcnNlmeSetCnf_t setCnf;
    rcnNlmeStartCnf_t startCnf;
    rcnNlmeUnpairCnf_t unpairCnf;
    rcnNlmeUnpairInd_t unpairInd;
    rcnNlmeAutoDiscoveryCnf_t autoDiscoveryCnf;
  } prim;
} rcnCnfIndSerialized_t;


#ifdef _MSC_VER
#pragma pack()
#endif


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RCN_MARSHAL_H */
