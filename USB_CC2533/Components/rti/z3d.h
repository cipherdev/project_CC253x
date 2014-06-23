/**************************************************************************************************
  Filename:       z3d.h
**************************************************************************************************/
#ifndef Z3D_H
#define Z3D_H

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

#if !defined FEATURE_Z3D_CTL
#define FEATURE_Z3D_CTL  FALSE
#endif
#if !defined FEATURE_Z3D_TGT
#define FEATURE_Z3D_TGT  FALSE
#endif
#if !defined FEATURE_Z3D_ZRC
#define FEATURE_Z3D_ZRC  FALSE
#endif
#if (FEATURE_Z3D_CTL || FEATURE_Z3D_TGT)
#if !defined FEATURE_Z3D
#define FEATURE_Z3D      TRUE
#endif
#else
#define FEATURE_Z3D      FALSE
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

#if !FEATURE_Z3D
void z3dInitCnf(void);
uint8 z3dSendDataCnf(rStatus_t status);
uint8 z3dReceiveDataInd(uint8 srcIndex, uint8 len, uint8 *pData);
rStatus_t z3dReadItem(uint8 itemId, uint8 len, uint8 *pValue);
rStatus_t z3dWriteItem(uint8 itemId, uint8 len, uint8 *pValue);
#endif

#if !FEATURE_Z3D_CTL
void z3dCtlPairCnf(uint8 dstIndex);
#endif

#if !FEATURE_Z3D_TGT
void z3dTgtAllowPairInd(void);
void z3dTgtAllowPairCnf(void);
#endif

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
