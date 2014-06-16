/**************************************************************************************************
  Filename:       zid_adapter.h
  Revised:        $Date: 2011-10-27 16:42:33 -0700 (Thu, 27 Oct 2011) $
  Revision:       $Revision: 28118 $

  Description:

  This file contains the declarations for a ZID Profile HID Adapter Device.


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
#ifndef ZID_ADAPTER_H
#define ZID_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "rti.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

// Implement a ZID mini-app (that can co-exist with the RNP by UART, for example) that auto-starts
// the RNP as a Target and then acts on CC2531 key presses and controls the LEDs.
#if !defined FEATURE_ZID_ADA_APP
#define FEATURE_ZID_ADA_APP  FALSE
#endif

// Implement a ZID Dongle that enumerates the proprietary IN/OUT EP for RNP control via USB HID.
#if !defined FEATURE_ZID_ADA_DON
#define FEATURE_ZID_ADA_DON  FALSE
#endif

// Setting to TRUE enables the Adapter to proxy the USB descriptors and reports.
// Currently only Mouse and Keyboard would be supported - see zid_proxy.c and zid_usb.c
#if !defined FEATURE_ZID_ADA_PXY
#define FEATURE_ZID_ADA_PXY  FALSE
#endif

// Implement test & debug code.
#if !defined FEATURE_ZID_ADA_TST
#define FEATURE_ZID_ADA_TST  FALSE
#endif

// ZID ADA task events
#define ZID_ADA_EVT_IDLE_RATE_GUARD_TIME 0x0001

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* A packed structure that contains the ZID Adaptor attribute settings */
typedef struct
{
  uint16 ZIDProfileVersion;
  uint16 HIDParserVersion;
  uint16 HIDDeviceReleaseNumber;
  uint16 HIDVendorId;
  uint16 HIDProductId;
  uint8  HIDCountryCode;
} zid_ada_cfg_t;
#define ZID_ADA_CFG_T_SIZE  (sizeof(zid_ada_cfg_t))

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

extern uint8 zidAda_TaskId;

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
void zidAda_Init(uint8 taskId);

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
void zidAda_InitItems(void);

void zidAda_SetStateActive( void );

/**************************************************************************************************
 *
 * @fn          zidAda_ProcessEvent
 *
 * @brief       This routine handles ZID ADA task events.
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
uint16 zidAda_ProcessEvent(uint8 taskId, uint16 events);

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
uint8 zidAda_SendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd);

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
                             uint8 *pData );

/**************************************************************************************************
 * @fn      zidAda_AllowPairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_AllowPairReq API call.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void zidAda_AllowPairCnf(uint8 dstIndex);

/**************************************************************************************************
 *
 * @fn      zidAda_Unpair
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API call or by
 *          receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  TRUE if ZID initiated the unpair; FALSE otherwise.
 */
uint8 zidAda_Unpair(uint8 dstIndex);

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
rStatus_t zidAda_ReadItem(uint8 itemId, uint8 len, uint8 *pValue);

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
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidAda_WriteItem(uint8 itemId, uint8 len, uint8 *pValue);

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
void zidAda_RemoveFromProxyTable( uint8 pairIdx );

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
