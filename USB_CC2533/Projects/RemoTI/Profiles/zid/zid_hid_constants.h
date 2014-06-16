/**************************************************************************************************
  Filename:       zid_hid_constants.h
  Revised:        $Date: 2011-04-06 14:11:07 -0700 (Wed, 06 Apr 2011) $
  Revision:       $Revision: 25604 $

  Description:    This file contains USB specified HID constants used by the ZID profile.

  Reference:      HID Usage Tables, Version 1.12, 10/28/2004

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
#ifndef ZID_HID_CONSTANTS_H
#define ZID_HID_CONSTANTS_H

#ifdef __cplusplus
extern "C"
{
#endif

// HID keyboard codes (subset of the codes available in the HID specification)
#define HID_KEYBOARD_RESERVED     0
#define HID_KEYBOARD_A            4
#define HID_KEYBOARD_B            5
#define HID_KEYBOARD_C            6
#define HID_KEYBOARD_D            7
#define HID_KEYBOARD_E            8
#define HID_KEYBOARD_F            9
#define HID_KEYBOARD_G            10
#define HID_KEYBOARD_H            11
#define HID_KEYBOARD_I            12
#define HID_KEYBOARD_J            13
#define HID_KEYBOARD_K            14
#define HID_KEYBOARD_L            15
#define HID_KEYBOARD_M            16
#define HID_KEYBOARD_N            17
#define HID_KEYBOARD_O            18
#define HID_KEYBOARD_P            19
#define HID_KEYBOARD_Q            20
#define HID_KEYBOARD_R            21
#define HID_KEYBOARD_S            22
#define HID_KEYBOARD_T            23
#define HID_KEYBOARD_U            24
#define HID_KEYBOARD_V            25
#define HID_KEYBOARD_W            26
#define HID_KEYBOARD_X            27
#define HID_KEYBOARD_Y            28
#define HID_KEYBOARD_Z            29
#define HID_KEYBOARD_1            30
#define HID_KEYBOARD_2            31
#define HID_KEYBOARD_3            32
#define HID_KEYBOARD_4            33
#define HID_KEYBOARD_5            34
#define HID_KEYBOARD_6            35
#define HID_KEYBOARD_7            36
#define HID_KEYBOARD_8            37
#define HID_KEYBOARD_9            38
#define HID_KEYBOARD_0            39
#define HID_KEYBOARD_RETURN       40
#define HID_KEYBOARD_TAB          43
#define HID_KEYBOARD_SPACEBAR     44
#define HID_KEYBOARD_CAPS_LOCK    57
#define HID_KEYBOARD_RIGHT_ARROW  79
#define HID_KEYBOARD_LEFT_ARROW   80
#define HID_KEYBOARD_DOWN_ARROW   81
#define HID_KEYBOARD_UP_ARROW     82
#define HID_KEYBOARD_LEFT_SHIFT   225
#define HID_KEYBOARD_LEFT_ALT     226

#define HID_MOUSE_BUTTON_LEFT     253
#define HID_MOUSE_BUTTON_MIDDLE   254
#define HID_MOUSE_BUTTON_RIGHT    255

#ifdef __cplusplus
}
#endif

#endif

/**************************************************************************************************
*/
