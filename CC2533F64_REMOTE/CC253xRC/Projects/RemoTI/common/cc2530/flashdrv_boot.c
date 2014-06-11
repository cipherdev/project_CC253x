/**************************************************************************************************
  Filename:       flashdrv_boot.c
  Revised:        $Date: 2011-09-07 18:03:16 -0700 (Wed, 07 Sep 2011) $
  Revision:       $Revision: 27485 $

  Description:    Implementation of flash driver for CC2530 boot loader.
                  The user of this driver (boot loader) has to be near code model and
                  interrupt is always disabled (such assumption is to reduce code size).

  Copyright 2008-2009 Texas Instruments Incorporated. All rights reserved.

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

#include "flashdrv.h"
#include <ioCC2530.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Constants
 */
#define FLASHDRV_DMA_CH 0
#define ERASE           0x01

/*********************************************************************
 * Typedefs
 */
// DMA descriptor
typedef struct {
  unsigned char srcAddrH;
  unsigned char srcAddrL;
  unsigned char dstAddrH;
  unsigned char dstAddrL;
  unsigned char xferLenV;
  unsigned char xferLenL;
  unsigned char ctrlA;
  unsigned char ctrlB;
} flashdrvDmaDesc_t;

/*********************************************************************
 * Local variables
 */
static flashdrvDmaDesc_t flashdrvDmaDesc; // DMA descriptor table

/*********************************************************************
 * Local functions
 */
static __monitor void flashdrvWriteTrigger(void);

/**************************************************************************************************
 * @fn          FLASHDRV_Init
 *
 * @brief       This function initializes flash driver.
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
void FLASHDRV_Init(void)
{
#if FLASHDRV_DMA_CH == 0
  DMA0CFGH = (unsigned char) ((unsigned short) &flashdrvDmaDesc >> 8);
  DMA0CFGL = (unsigned char) (((unsigned short) &flashdrvDmaDesc) & 0xFF);
#else
# error "flashdrv DMA channel other than 0 is not supported"
#endif
  DMAIE = 1;
}

/**************************************************************************************************
 * @fn          FLASHDRV_Read
 *
 * @brief       This function reads 'cnt' bytes from the internal flash.
 *
 * input parameters
 *
 * @param       pg - Valid HAL flash page number (ie < 128).
 * @param       offset - Valid offset into the page (so < HAL_NV_PAGE_SIZE and byte-aligned is ok).
 * @param       buf - Valid buffer space at least as big as the 'cnt' parameter.
 * @param       cnt - Valid number of bytes to read: a read cannot cross into the next 32KB bank.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void FLASHDRV_Read(unsigned char pg,
                   unsigned short offset,
                   unsigned char *buf,
                   unsigned short cnt)
{
  // Calculate the offset into the containing flash bank as it gets mapped into XDATA.
  unsigned char *ptr = (unsigned char *)(offset + 0x8000) +
                       ((pg % 16) * 2048);
  unsigned char memctr = MEMCTR;  // Save to restore.

  pg /= 16;  // Calculate the flash bank from the flash page.

  // Calculate and map the containing flash bank into XDATA.
  MEMCTR = (MEMCTR & 0xF8) | pg;

  while (cnt--)
  {
    *buf++ = *ptr++;
  }

  MEMCTR = memctr;
}

/**************************************************************************************************
 * @fn          FLASHDRV_Write
 *
 * @brief       This function reads 'cnt' bytes from the internal flash.
 *
 * input parameters
 *
 * @param       addr - Valid HAL flash write address: actual addr / 4 and quad-aligned.
 * @param       buf - Valid buffer space at least as big as 'cnt' X 4.
 * @param       cnt - Number of 4-byte blocks to write.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void FLASHDRV_Write(unsigned short addr,
                    unsigned char *buf,
                    unsigned short cnt)
{
  flashdrvDmaDesc.srcAddrH = (unsigned char) ((unsigned short)buf >> 8);
  flashdrvDmaDesc.srcAddrL = (unsigned char) (unsigned short) buf;
  flashdrvDmaDesc.dstAddrH = (unsigned char) ((unsigned short)&FWDATA >> 8);
  flashdrvDmaDesc.dstAddrL = (unsigned char) (unsigned short) &FWDATA;
  flashdrvDmaDesc.xferLenV =
    (0x00 << 5) |               // use length
    (unsigned char)(unsigned short)(cnt >> 6);  // length (12:8). Note that cnt is flash word
  flashdrvDmaDesc.xferLenL = (unsigned char)(unsigned short)(cnt * 4);
  flashdrvDmaDesc.ctrlA =
    (0x00 << 7) | // word size is byte
    (0x00 << 5) | // single byte/word trigger mode
    18;           // trigger source is flash
  flashdrvDmaDesc.ctrlB =
    (0x01 << 6) | // 1 byte/word increment on source address
    (0x00 << 4) | // zero byte/word increment on destination address
    (0x00 << 3) | // The DMA is to be polled and shall not issue an IRQ upon completion.
    (0x00 << 2) | // use all 8 bits for transfer count
    0x02; // DMA priority high

  DMAIRQ &= ~( 1 << FLASHDRV_DMA_CH ); // clear IRQ
  DMAARM = (0x01 << FLASHDRV_DMA_CH ); // arm DMA

  FADDRL = (unsigned char)addr;
  FADDRH = (unsigned char)(addr >> 8);
  flashdrvWriteTrigger();
}

/**************************************************************************************************
 * @fn          FLASHDRV_Erase
 *
 * @brief       This function must be copied to RAM before running because it triggers and then
 *              awaits completion of Flash write, which can only be done from RAM.
 *
 * input parameters
 *
 * @param       pg - page to erase
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void FLASHDRV_Erase(unsigned char pg)
{
  FADDRH = pg << 1;
  FCTL = ERASE;
  asm("NOP");
  while(FCTL == 0x80);
}


/**************************************************************************************************
 * @fn          flashdrvWriteTrigger
 *
 * @brief       This function must be copied to RAM before running because it triggers and then
 *              awaits completion of Flash write, which can only be done from RAM.
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
static __monitor void flashdrvWriteTrigger(void)
{
  FCTL |= 0x02;         // Trigger the DMA writes.
  while (FCTL & 0x80);  // Wait until writing is done.
}
