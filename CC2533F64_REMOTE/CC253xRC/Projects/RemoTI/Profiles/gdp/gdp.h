/**************************************************************************************************
  Filename:       gdp.h
  Revised:        $Date: 2011-09-01 09:35:23 -0700 (Thu, 01 Sep 2011) $
  Revision:       $Revision: 27412 $

  Description:

  This file contains the common declarations for the Generic Device Profile (GDP).


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
#ifndef GDP_H
#define GDP_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "gdp_profile.h"
#include "rti.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                      GDP Profile Item Ids
 *
 * It may be required to support local Item Id's which differ from GDP-specified Attribute Id's.
 * ------------------------------------------------------------------------------------------------
 */

/* GDP item IDs to be used for RTI_ReadItemEx() and RTI_WriteItemEx() */
#define GDP_ITEM_KEY_EX_TRANSFER_COUNT     aplKeyExchangeTransferCount                 // 0x82
#define GDP_ITEM_POWER_STATUS              aplPowerStatus                              // 0x83

/* GDP NV items */
#if !defined GDP_NVID_BEG
#define GDP_NVID_BEG 0xA0  // Arbitrary, safe NV Id that does not conflict with RTI usages.
#endif

#define GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT      (GDP_NVID_BEG + 0)

#define GDP_NVID_INVALID       0x00        // Failure return value for DescSpecN NV Id lookup.

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* Build the TX Options Bit Mask for RTI_SendDataReq(). */
#define GDP_TX_OPTIONS_CONTROL_PIPE \
  (RTI_TX_OPTION_ACKNOWLEDGED | RTI_TX_OPTION_SECURITY)

#define GDP_IS_TXING           gdpTxStatus
#define GDP_SET_TXING()        gdpTxStatus = TRUE
#define GDP_CLR_TXING()        gdpTxStatus = FALSE

// Macros for interpreting the aplPowerStatus attribute
#define POWER_STATUS_SET_POWER_METER(x,v)       st(x = ((x) & 0xF0) | ((v) & 0x0F);)
#define POWER_STATUS_GET_POWER_METER(x,v)       st(x = ((v) & 0x0F);)
#define POWER_STATUS_SET_CHARGING_BIT(x)        st(x |= 0x10;)
#define POWER_STATUS_GET_CHARGING_BIT(x)        ((x) >> 5)
#define POWER_STATUS_CLR_CHARGING_BIT(x)        st(x &= ~0x10;)
#define POWER_STATUS_SET_IMPENDING_DOOM_BIT(x)  st(x |= 0x80;)
#define POWER_STATUS_GET_IMPENDING_DOOM_BIT(x)  ((x) >> 8)
#define POWER_STATUS_CLR_IMPENDING_DOOM_BIT(x)  st(x &= ~0x80;)

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
extern bool gdpTxStatus;

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          gdpInitItems
 *
 * @brief       This API is used to initialize the GDP configuration items to default values.
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
void gdpInitItems(void);

/**************************************************************************************************
 * @fn      gdpReceiveDataInd
 *
 * @brief   RTI receive data indication callback asynchronously initiated by
 *          another node. The client is expected to complete this function.
 *
 * input parameters
 *
 * @param   srcIndex:  Pairing table index.
 * @param   len:       Number of data bytes.
 * @param   pData:     Pointer to data received.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the data was handled; otherwise FALSE indicating to pass the data to
 *              Application layer.
 */
uint8 gdpReceiveDataInd( uint8 srcIndex,
                         uint8 len,
                         uint8 *pData );

/**************************************************************************************************
 * @fn          gdpReadItem
 *
 * @brief       This API is used to read the GDP Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table.
 *
 * input parameters
 *
 * @param       itemId - The Configuration Interface item identifier.
 *
 * @param       len    - The length in bytes of the item's data.
 *
 * output parameters
 *
 * @param       pValue - A pointer to the item identifier's data from table.
 *
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t gdpReadItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 * @fn          gdpWriteItem
 *
 * @brief       This API is used to write GDP Configuration Interface parameters
 *              to the Configuration Parameters table, and permitted attributes
 *              to the State Attributes table.
 *
 * input parameters
 *
 * @param       itemId:  The Configuration Interface item identifier.
 *                       See special note below for 0xF1-0xFF non-standard descriptors.
 * @param       len:     The length in bytes of the item identifier's data.
 * @param       pValue:  A pointer to the item's data.
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
rStatus_t gdpWriteItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 * @fn      gdpSendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API call.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  TRUE if GDP initiated the send data request; FALSE otherwise.
 */
uint8 gdpSendDataCnf(rStatus_t status);

#endif

/**************************************************************************************************
*/
