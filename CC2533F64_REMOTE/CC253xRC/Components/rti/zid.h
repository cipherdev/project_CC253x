/**************************************************************************************************
  Filename:       zid.h
  Revised:        $Date: 2011-10-27 16:42:33 -0700 (Thu, 27 Oct 2011) $
  Revision:       $Revision: 28118 $

  Description:

  This file contains the declarations for the RF4CE ZID Profile necessary to eliminate
  build errors and warnings in projects with no use of ZID.


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
#ifndef ZID_H
#define ZID_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "rti.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#if !defined FEATURE_ZID_ADA
#define FEATURE_ZID_ADA  FALSE
#endif
#if !defined FEATURE_ZID_CLD
#define FEATURE_ZID_CLD  FALSE
#endif
#if !defined FEATURE_ZID_ZRC
#define FEATURE_ZID_ZRC  FALSE
#endif
#if ((FEATURE_ZID_ADA == TRUE) || (FEATURE_ZID_CLD == TRUE))
#if !defined FEATURE_ZID
#define FEATURE_ZID      TRUE
#if !defined FEATURE_GDP
#define FEATURE_GDP                        TRUE  // GDP is inherited with ZID
#endif
#endif
#else
#define FEATURE_ZID      FALSE
#define FEATURE_GDP      FALSE
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

#if !FEATURE_ZID_ADA
#define FEATURE_ZID_ADA_APP  FALSE
#define FEATURE_ZID_ADA_DON  FALSE
#define FEATURE_ZID_ADA_PXY  FALSE
#define FEATURE_ZID_ADA_TST  FALSE

void zidAda_AllowPairCnf(uint8 dstIndex);
#endif

#if !FEATURE_ZID_CLD
void zidCld_PairCnf(uint8 dstIndex);
#endif

#if !FEATURE_ZID
#define ZID_EVT_RSP_WAIT 0
#define ZID_ITEM_KEY_EX_TRANSFER_COUNT   0

void gdpInitItems(void);
void zidInitItems(void);
void zidInitCnf(void);
uint8 zidSendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd);
uint8 gdpSendDataCnf(rStatus_t status);
uint8 zidSendDataCnf(rStatus_t status);
uint8 gdpReceiveDataInd(uint8 srcIndex, uint8 len, uint8 *pData);
uint8 zidReceiveDataInd(uint8 srcIndex, uint16 vendorId, uint8 rxLQI, uint8 rxFlags, uint8 len, uint8 *pData);
uint8 zidUnpair(uint8 dstIndex);
rStatus_t gdpReadItem(uint8 itemId, uint8 len, uint8 *pValue);
rStatus_t gdpWriteItem(uint8 itemId, uint8 len, uint8 *pValue);
rStatus_t zidReadItem(uint8 itemId, uint8 len, uint8 *pValue);
rStatus_t zidWriteItem(uint8 itemId, uint8 len, uint8 *pValue);
#endif

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
