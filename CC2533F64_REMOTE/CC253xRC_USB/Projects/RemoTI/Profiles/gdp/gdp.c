/**************************************************************************************************
  Filename:       gdp.c
  Revised:        $Date: 2011-09-02 20:46:01 -0700 (Fri, 02 Sep 2011) $
  Revision:       $Revision: 27460 $

  Description:

  This file contains the definitions for a GDP Profile.


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

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "gdp.h"
#include "gdp_profile.h"
#include "OSAL.h"
#include "osal_snv.h"
#include "rti.h"
#include "rcn_nwk.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static void rxGetAttr(uint8 srcIndex, uint8 len, uint8 *pData);
static uint8 rxPushAttr(uint16 len, uint8 *pData);
static void sendDataReq(uint8 dstIndex, uint8 len, uint8 *pData);
static uint8 procGetAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf);

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8 gdp_powerStatus;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
bool gdpTxStatus;

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
void gdpInitItems(void)
{
  uint8 buf;

  /* init key exchange transfer count */
  if (osal_snv_read(GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT, 1, &buf) != SUCCESS)
  {
    buf = DPP_DEF_KEY_TRANSFER_COUNT;
    (void)osal_snv_write(GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT, 1, &buf);
  }

  /* init power status */
  gdp_powerStatus = 0;
}

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
                         uint8 *pData )
{
  uint8 cmd = *pData & GDP_HEADER_CMD_CODE_MASK;
  bool sendGenericResponse = FALSE;
  uint8 rsp;
  uint8 rtrn = FALSE;

  switch (cmd)
  {
    case GDP_CMD_GET_ATTR:
    {
      rxGetAttr(srcIndex, len, pData);
      rtrn = TRUE;  // Completely handled here.
      break;
    }

    case GDP_CMD_GET_ATTR_RSP:
    {
      /* assuming that get attribute response was something requested by the
       * application, so just pass it up.
       */
      break;
    }

    case GDP_CMD_PUSH_ATTR:
    {
      rsp = rxPushAttr(len, pData);

      /* Pushed GDP attributes are sent to application */
      sendGenericResponse = TRUE;
      break;
    }

    default:
    {
      rtrn = TRUE;  // Invalid GDP command - don't pass to the Application.
      break;
    }
  }

  if (sendGenericResponse == TRUE)
  {
    uint8 buf[2] =
    {
      GDP_CMD_GENERIC_RSP,
      rsp
    };
    sendDataReq(srcIndex, 2, buf);
  }

  return rtrn;
}

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
rStatus_t gdpReadItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status;

  if (itemId == GDP_ITEM_POWER_STATUS) // ZID/GDP attributes not required to be saved in NV
  {
    if (len == GDP_ATTR_POWER_STATUS_SIZE)
    {
      *pValue = gdp_powerStatus;
      status = RTI_SUCCESS;
    }
    else
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
  }
  else if (itemId == GDP_ITEM_KEY_EX_TRANSFER_COUNT)
  {
    if (len != GDP_ATTR_KEY_EXCHANGE_TRANSFER_COUNT_SIZE)
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      status = osal_snv_read( GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT, len, pValue );
    }
  }
  else
  {
    status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
  }

  return status;
}

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
rStatus_t gdpWriteItem(uint8 itemId, uint8 len, uint8 *pValue)
{
  rStatus_t status = RTI_SUCCESS;

  if (itemId == GDP_ITEM_POWER_STATUS)
  {
    if (len == GDP_ATTR_POWER_STATUS_SIZE)
    {
      gdp_powerStatus = *pValue;
      status = RTI_SUCCESS;
    }
    else
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
  }
  else if (itemId == GDP_ITEM_KEY_EX_TRANSFER_COUNT)
  {
    if ((*pValue < aplcMinKeyExchangeTransferCount) ||
        (len != GDP_ATTR_KEY_EXCHANGE_TRANSFER_COUNT_SIZE))
    {
      status = RTI_ERROR_INVALID_PARAMETER;
    }
    else
    {
      status = osal_snv_write( GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT, len, pValue );
    }
  }
  else
  {
    status = RTI_ERROR_UNSUPPORTED_ATTRIBUTE;
  }

  return status;
}

/**************************************************************************************************
 * @fn      gdpSendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API call.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  TRUE if GDP initiated the send data request; FALSE otherwise.
 */
uint8 gdpSendDataCnf(rStatus_t status)
{
  if (GDP_IS_TXING)  // If the ZID co-layer initiated this SendDataReq.
  {
    GDP_CLR_TXING();
    status = TRUE;
  }
  else
  {
    status = FALSE;
  }

  return status;
}

/**************************************************************************************************
 * @fn          rxGetAttr
 *
 * @brief       Process a Get Attributes command frame.
 *
 * input parameters
 *
 * @param   srcIndex -  Pairing table index.
 * @param   len      - Number of data bytes.
 * @param   *pData   - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rxGetAttr(uint8 srcIndex, uint8 len, uint8 *pData)
{
  uint8 *pBuf;

  if (len < 2)
  {
    return;
  }
  len--;
  pData++;

  /* Memory for responding with potentially all GDP items */
  pBuf = osal_mem_alloc((GDP_ATTR_RSP_SIZE_HDR * len) + GDP_TOTAL_ATTR_SIZE);

  if (pBuf != NULL)
  {
    *pBuf = GDP_CMD_GET_ATTR_RSP;
    len = procGetAttr(len, pData, pBuf);
    sendDataReq(srcIndex, len, pBuf);
    osal_mem_free(pBuf);
  }
}

/**************************************************************************************************
 *
 * @fn      rxPushAttr
 *
 * @brief   Receive and process a GDP_CMD_PUSH_ATTR command frame.
 *
 * @param   len:       Number of data bytes.
 * @param   *pData:    Pointer to data received.
 *
 * @return  GDP_GENERIC_RSP_SUCCESS or GDP_GENERIC_RSP_INVALID_PARAM according to the attributes and
 *          their values.
 */
static uint8 rxPushAttr( uint16 len, uint8 *pData )
{
  uint8 rtrn = GDP_GENERIC_RSP_SUCCESS;
  len--;  // Decrement for the GDP command byte expected to be GDP_CMD_PUSH_ATTR.
  pData++;

  while ((len != 0) && (rtrn == GDP_GENERIC_RSP_SUCCESS))
  {
    uint8 attrId = *pData++;
    uint8 attrLen;

    /* First subtract Attribute ID and Attribute length field length from total length */
    len -= GDP_PUSH_ATTR_REC_SIZE_HDR;

    /* Extract Attribute length */
    attrLen = *pData++;

    /* Process it */
    switch (attrId)
    {
      case aplPowerStatus:
      {
        if (attrLen != GDP_ATTR_POWER_STATUS_SIZE)
        {
          rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        }
        /* Power status is just pushed to application so nothing else to do */
        break;
      }

      default:
      {
        rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
        break;
      }
    }

    /* check for misaligned attribute */
    if (len < attrLen)
    {
      rtrn = GDP_GENERIC_RSP_INVALID_PARAM;
    }

    len -= attrLen;
    pData += attrLen;
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn          sendDataReq
 *
 * @brief       Construct and send an RTI_SendDataReq() using the input parameters.
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 * @param       len       - Number of bytes to send.
 * @param       *pData    - Pointer to data to be sent.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void sendDataReq(uint8 dstIndex, uint8 len, uint8 *pData)
{
  uint16 vendorId = RTI_VENDOR_TEXAS_INSTRUMENTS;

  /* In case vendor ID is not default, read it */
  (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_ID, sizeof(uint16), (uint8 *)&vendorId);

  /* Remember that this transaction was started by GDP so confirm
   * doesn't get sent to application.
   */
  GDP_SET_TXING();
  RTI_SendDataReq( dstIndex,
                   RTI_PROFILE_GDP,
                   vendorId,
                   GDP_TX_OPTIONS_CONTROL_PIPE,
                   len,
                   pData );
}

/**************************************************************************************************
 * @fn          procGetAttr
 *
 * @brief       Process an attribute (list) according to the parameters.
 *
 * input parameters
 *
 * @param        cnt  - The number of attributes in the pReq list.
 * @param       *pReq - Pointer to the buffer containing the Get Attribute command payload.
 * @param       *pBuf - Pointer to the buffer in which to pack the Get Attribute/Push payload.
 *
 * output parameters
 *
 * None.
 *
 * @return      Zero for insufficient heap memory. Otherwise, the non-zero count of data in pBuf.
 */
static uint8 procGetAttr(uint8 cnt, uint8 *pReq, uint8 *pBuf)
{
  uint8 *pBeg = pBuf;
  pBuf++;  // Move pointer past Frame Control field of command

  while (cnt--)
  {
    uint8 id = *pReq++;
    uint8 val;
    bool fail = FALSE;

    /* Extract Attribute ID */
    *pBuf++ = id;

    if (id == aplKeyExchangeTransferCount)
    {
      if (osal_snv_read(GDP_NVID_KEY_EXCHANGE_TRANSFER_COUNT, 1, &val) != SUCCESS)
      {
        fail = TRUE;
      }
    }
    else if (id == aplPowerStatus)
    {
      val = gdp_powerStatus;
    }
    else
    {
      fail = TRUE;
    }

    if (fail)
    {
      *pBuf++ = GDP_ATTR_RSP_UNSUPPORTED;
    }
    else
    {
      *pBuf++ = GDP_ATTR_RSP_SUCCESS;
      *pBuf++ = 1;
      *pBuf++ = val;
    }
  }

  return (uint8)(pBuf - pBeg);
}

/**************************************************************************************************
*/
