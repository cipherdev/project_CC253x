/**************************************************************************************************
  Filename:       zid.h
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
