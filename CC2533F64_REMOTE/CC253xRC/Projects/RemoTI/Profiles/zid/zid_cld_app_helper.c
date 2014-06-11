/**************************************************************************************************
  Filename:       zid_cld_app_helper.c
  Revised:        $Date: 2011-09-07 18:03:16 -0700 (Wed, 07 Sep 2011) $
  Revision:       $Revision: 27485 $

  Description:    ZID Class Device application helper module. Used to provide
                  a ZID Class Device application with helper functions to
                  operate on ZID profile data.

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
#include "OSAL.h"
#include "rti.h"
#include "zid_common.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
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

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          zidCldAppHlp_WriteNonStdDescComp
 *
 * @brief       Store (fragmented, if necessary) a non standard descriptor component into NV
 *
 * input parameters
 *
 * @param  descNum  - non-std descriptor number (0 to (aplcMaxNonStdDescCompsPerHID - 1))
 * @param  pBuf     - Pointer to non-std descriptor component as defined in Table 9 of ZID Profile, r15
 *
 * output parameters
 *
 * None.
 *
 * @return
 *
 * None.
 *
 */
rStatus_t zidCldAppHlp_WriteNonStdDescComp( uint8 descNum, zid_non_std_desc_comp_t *pBuf )
{
  uint16 len;
  uint8 localDescNum = descNum;
  uint8 fragNum = 0;
  uint8 *pSrc = (uint8 *)pBuf;

  /* Unpack descriptor length */
  len = (pBuf->size_h << 8) | pBuf->size_l;

  /* Validate input data */
  if ((pBuf == NULL) || (descNum >= aplcMaxNonStdDescCompsPerHID) || (len > aplcMaxNonStdDescCompSize))
  {
    return RTI_ERROR_INVALID_PARAMETER;
  }

  /* First store current descriptor number in ZID */
  (void)RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_NUM, 1, &localDescNum );

  /* Determine if non-std descriptor component needs to be fragmented or not */
  if (len > aplcMaxNonStdDescFragmentSize)
  {
    uint8 *pData = osal_mem_alloc( ZID_NON_STD_DESC_SPEC_FRAG_T_MAX );
    zid_frag_non_std_desc_comp_t *pFrag = (zid_frag_non_std_desc_comp_t *)pData;
    if (pData == NULL)
    {
      return RTI_ERROR_OUT_OF_MEMORY;
    }

    /* first copy the attribute "header" */
    osal_memcpy( pData, pSrc, sizeof( zid_non_std_desc_comp_t ) );
    pSrc += sizeof( zid_non_std_desc_comp_t );

    /* Store each fragment */
    while (len > 0)
    {
      uint8 fragLen;

      if (len > aplcMaxNonStdDescFragmentSize)
      {
        fragLen = aplcMaxNonStdDescFragmentSize;
      }
      else
      {
        fragLen = len;
      }
      pFrag->fragId = fragNum;
      osal_memcpy( pFrag->data, pSrc, fragLen );

      /* Write fragment number */
      (void)RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_FRAG_NUM, 1, &fragNum );

      /* Write fragment data */
      if (RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_FRAG, fragLen + sizeof( zid_frag_non_std_desc_comp_t ), pData ) != RTI_SUCCESS)
      {
        return RTI_ERROR_INVALID_PARAMETER;
      }

      /* Bump pointers for next fragment */
      pSrc += fragLen;
      len -= fragLen;
      fragNum++;
    }

    /* Free up memory */
    osal_mem_free( pData );
  }
  else
  {
    /* Just store buffer as is */
    (void)RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_FRAG_NUM, 1, &fragNum );

    if (RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_FRAG, len + sizeof( zid_non_std_desc_comp_t ), (uint8 *)pBuf ) != RTI_SUCCESS)
    {
      return RTI_ERROR_INVALID_PARAMETER;
    }
  }

  return RTI_SUCCESS;
}

/**************************************************************************************************
 * @fn          zidCldAppHlp_WriteNullReport
 *
 * @brief       Store a NULL report corresponding to a non standard descriptor component into NV
 *
 * input parameters
 *
 * @param  descNum  - non-std descriptor number for report (0 to (aplcMaxNonStdDescCompsPerHID - 1))
 * @param  pBuf     - Pointer to null report as defined in Table 14 of ZID Profile, r15
 *
 * output parameters
 *
 * None.
 *
 * @return
 *
 * None.
 *
 */
rStatus_t zidCldAppHlp_WriteNullReport( uint8 descNum, zid_null_report_t *pBuf )
{
  uint8 localDescNum = descNum;

  /* Validate input data */
  if ((pBuf == NULL) || (descNum >= aplcMaxNonStdDescCompsPerHID) || (pBuf->len > aplcMaxNullReportSize))
  {
    return RTI_ERROR_INVALID_PARAMETER;
  }

  /* First store current descriptor number in ZID */
  (void)RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NON_STD_DESC_NUM, 1, &localDescNum );

  /* Just store buffer as is */
  if (RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_NULL_REPORT, pBuf->len + sizeof( zid_null_report_t ), (uint8 *)pBuf ) != RTI_SUCCESS)
  {
    return RTI_ERROR_INVALID_PARAMETER;
  }

  return RTI_SUCCESS;
}

/**************************************************************************************************
*/
