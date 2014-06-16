/**************************************************************************************************
  Filename:       zid_usb.h
  Revised:        $Date: 2011-09-07 18:03:16 -0700 (Wed, 07 Sep 2011) $
  Revision:       $Revision: 27485 $

  Description:

  This file contains the declarations for a ZID Profile HID Adapter implementing the CC2531 USB.


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
#ifndef ZID_USB_H
#define ZID_USB_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */
  #define USB_SUCCESS   0
  #define USB_MEMORY0   1
  #define USB_INVALID   2
  #define USB_ALREADY   3
/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
#define INTERFACE_NUMBER_RNP                0x00
#define INTERFACE_NUMBER_CE                 0x01
#define INTERFACE_NUMBER_ZID                0x02
#else //ZID_USB_RNP
#if defined (ZID_USB_CE) || defined (ZID_USB_RNP)
#warning ZID_USB_RNP and ZID_USB_CE must be defined together for now, due to TargetEmulator
#endif
#define INTERFACE_NUMBER_ZID                0x00
#endif //ZID_USB_RNP && ZID_USB_CE
  
#define ZID_PROXY_RNP_EP_OUT_ADDR        0x03
#define ZID_PROXY_RNP_EP_IN_ADDR         ZID_PROXY_RNP_EP_OUT_ADDR
#define ZID_PROXY_ZID_EP_OUT_ADDR        0x04
#define ZID_PROXY_ZID_EP_IN_ADDR         0x05
  
extern const uint8 zid_usb_report_len[ZID_STD_REPORT_TOTAL_NUM+1];

/**************************************************************************************************
 * @fn          zidUsbEnumerate
 *
 * @brief       This function initializes the USB Descriptor globals and enumerates the CC2531 to
 *              the USB host.
 *
 * input parameters
 *
 * @param       pPxy - A pointer to the zid_proxy_entry_t.
 *
 * output parameters
 *
 * None.
 *
 * @return      0 = SUCCESS
 *              1 = Not enough memory.
 *              2 = Invalid config.
 *              3 = Already enumerated.
 */
uint8 zidUsbEnumerate(zid_proxy_entry_t *pPxy);

/**************************************************************************************************
 * @fn          zidUsbUnEnum
 *
 * @brief       This function un-initializes the USB Descriptor globals for a ZID Target.
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
void zidUsbUnEnum(void);

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
