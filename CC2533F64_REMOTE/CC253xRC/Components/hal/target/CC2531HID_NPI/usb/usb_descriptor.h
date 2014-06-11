/**************************************************************************************************
  Filename:       usb_descriptor.h
  Revised:        $Date: 2012-11-07 14:24:46 -0800 (Wed, 07 Nov 2012) $
  Revision:       $Revision: 32120 $

  Description:

  This file contains the declarations for USB standard descriptors.


  Copyright 2010-2012 Texas Instruments Incorporated. All rights reserved.

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
#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "usb_framework_structs.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define EP0_PACKET_SIZE          32  // The maximum data packet size for Endpoint 0.

#define DESC_TYPE_DEVICE       0x01
#define DESC_TYPE_CONFIG       0x02
#define DESC_TYPE_STRING       0x03
#define DESC_TYPE_INTERFACE    0x04
#define DESC_TYPE_ENDPOINT     0x05

#define DESC_SIZE_DEVICE       0x12
#define DESC_SIZE_CONFIG       0x09
#define DESC_SIZE_INTERFACE    0x09
#define DESC_SIZE_ENDPOINT     0x07

#define EP_ATTR_CTRL           0x00  // Control (endpoint 0 only)
#define EP_ATTR_ISO            0x01  // Isochronous (not acknowledged)
#define EP_ATTR_BULK           0x02  // Bulk
#define EP_ATTR_INT            0x03  // Interrupt (guaranteed polling interval)
#define EP_ATTR_TYPE_BM        0x03  // Endpoint type bitmask

typedef struct {
    uint8 const * pUsbDescStart;               // USB descriptor start pointer
    uint8 const * pUsbDescEnd;                 // USB descriptor end pointer
    DESC_LUT_INFO * pUsbDescLut;         // Start of USB desc look-up table pointer
    DESC_LUT_INFO * pUsbDescLutEnd;      // End of USB desc look-up table pointer
    DBLBUF_LUT_INFO * pUsbDblbufLut;     // Start of double-buffering look-up table pointer
    DBLBUF_LUT_INFO * pUsbDblbufLutEnd;  // End of double-buffering look-up table pointer
} USB_DESCRIPTOR_MARKER;

extern USB_DESCRIPTOR_MARKER usbDescriptorMarker;

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          usbDescInit
 *
 * @brief       This function initializes the USB Descriptor globals.
 *
 * input parameters
 *
 * @param       desc - Pointer to the device descriptor conglomerate.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void usbDescInit(uint8 *desc);

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
