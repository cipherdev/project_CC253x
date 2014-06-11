/**************************************************************************************************
  Filename:       z3d.h
  Revised:        $Date: 2011-03-22 21:23:52 -0700 (Tue, 22 Mar 2011) $
  Revision:       $Revision: 25490 $

  Description:

  This file contains the declarations for the RF4CE Z3D Profile necessary to eliminate
  build errors and warnings in projects with no use of Z3D.


  Copyright 2011 Texas Instruments Incorporated. All rights reserved.

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
