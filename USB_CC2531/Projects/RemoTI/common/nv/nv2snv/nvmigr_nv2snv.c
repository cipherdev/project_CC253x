/**************************************************************************************************
  Filename:       nvmigr_nv2snv.c
  Revised:        $Date: 2010-02-02 13:09:33 -0800 (Tue, 02 Feb 2010) $
  Revision:       $Revision: 21636 $

  Description:    RemoTI NV to SNV system migration utility

  Copyright 2010 Texas Instruments Incorporated. All rights reserved.

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

/**************************************************************************************************
 *                                           Includes
 */
#include "nvmigr.h"

#include "hal_types.h"
#include "hal_flash.h"
#include "OSAL.h"

/**************************************************************************************************
 *                                         Local Functions
 */
static uint8 nvmigrIsSnvPage(uint8 pg);

/**************************************************************************************************
 *
 * @fn          NVMIGR_Nv2SnvClearObsoleteNvPages
 *
 * @brief       Clear NV pages built from incompatible NV system
 *
 * @param       none
 *
 * @return      none
 */
void NVMIGR_Nv2SnvClearObsoleteNvPages(void)
{
  uint8 pg;
    
  for ( pg = HAL_NV_PAGE_BEG; pg < HAL_NV_PAGE_BEG + HAL_NV_PAGE_CNT; pg++ )
  {
    if (!nvmigrIsSnvPage(pg))
    {
      /* Non-SNV page... clear */
      HalFlashErase(pg);
    }
  }
}


/**************************************************************************************************
 *
 * @fn          nvmigrIsSnvPage
 *
 * @brief       Check if an NV page is an obsolete NV page
 *
 * @param       none
 *
 * @return      TRUE if the page is an SNV page. FALSE otherwise.
 */
static uint8 nvmigrIsSnvPage(uint8 pg)
{
  uint16 offset;
  
  /* find the offset of the first header */
  for (offset = HAL_FLASH_PAGE_SIZE - HAL_FLASH_WORD_SIZE;
       offset >= HAL_FLASH_WORD_SIZE; /* Stop at page header */
       offset -= HAL_FLASH_WORD_SIZE)
  {
    uint32 tmp;
    
    HalFlashRead(pg, offset, (uint8 *)&tmp, HAL_FLASH_WORD_SIZE);
    if (tmp != 0xFFFFFFFF)
    {
      break;
    }
  }

  while (offset >= HAL_FLASH_WORD_SIZE) /* Stop at page header */
  {
    struct
    {
      uint16 id;
      uint16 len;
    } hdr;

    /* Read an item header */
    HalFlashRead(pg, offset, (uint8 *) &hdr, HAL_FLASH_WORD_SIZE);
    
    /* Invalid entry. Either length is correct or not */
    if (hdr.len & 0x8000)
    {
      /* Invalid length. The page is still a clean new OSAL NV */
      offset -= HAL_FLASH_WORD_SIZE;
    }
    else
    {
      if (hdr.len + HAL_FLASH_WORD_SIZE <= offset)
      {
        /* Length is considered valid */
        offset -= hdr.len + HAL_FLASH_WORD_SIZE;
      }
      else
      {
        /* invalid length. This is non SNV page or a corrupt SNV page */
        return FALSE;
      }
    }

#ifndef OSAL_SNV_UINT16_ID    
    if (!(hdr.id & 0x8000))
    {
      /* A valid ID */
      if (hdr.id > 0x00FE)
      {
        /* Outside the known range. The page is non SNV 8 bit ID page */
        return FALSE;
      }
    }
#endif /* OSAL_SNV_UINT16_ID */
  }
  return TRUE;
}

/**************************************************************************************************
 */
