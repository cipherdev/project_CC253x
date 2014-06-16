/**************************************************************************************************
  Filename:       zid_dongle.h
  Revised:        $Date: 2011-10-04 18:32:17 -0700 (Tue, 04 Oct 2011) $
  Revision:       $Revision: 27809 $

  Description:

  This file contains the declarations for a ZID Profile HID Adapter device acting as a
  self-starting RTI Application or a USB dongle.


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
#ifndef ZID_DONGLE_H
#define ZID_DONGLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

//efine SYS_EVENT_MSG                      0x8000
#define ZID_DONGLE_EVT_INIT                0x4000
#define ZID_DONGLE_EVT_LED                 0x2000
// OSAL events the application uses
#define ZID_DONGLE_EVT_START              0x0001 // Start the application
#define ZID_DONGLE_EVT_KEY_RELEASE_TIMER  0x0002 // timer to detect key release
#define ZID_DONGLE_EVT_RTI_INIT_CNF       0x0004 // timer to detect key release

// output report buffer size
#define ZID_DONGLE_OUTBUF_SIZE              3
// max output report buffer size
#define ZID_DONGLE_OUTBUF_SIZE_MAX         64

// Vendor specific report identifiers
#define ZID_DONGLE_CMD_REPORT_ID            1
#define ZID_DONGLE_PAIR_ENTRY_REPORT_ID     2

// output report ID
#define ZID_DONGLE_OUTPUT_REPORT_ID      ZID_DONGLE_CMD_REPORT_ID

// command identifiers of vendor defined usage 1
#define ZID_DONGLE_CMD_ALLOW_PAIR        1
#define ZID_DONGLE_CMD_UNPAIR            2
#define ZID_DONGLE_CMD_CLEAR_PAIRS       3
#define ZID_DONGLE_CMD_GET_MAX_PAIRS     4
#define ZID_DONGLE_CMD_GET_PAIR_ENTRY    5
#define ZID_DONGLE_CMD_GET_CHANNEL       6
#ifdef FEATURE_SBL
#define ZID_DONGLE_CMD_ENTER_BOOTMODE    7
#endif

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

// Remote control 2 byte report
typedef struct
{
  uint8 data[2];
} zidDongleReportRemote_t;

// Vendor specific control 2 byte report
typedef struct
{
  uint8 reportId;
  uint8 data[2];
} zidDongleReportVendor_t;

// Vendor specific control 11 byte report
typedef struct
{
  uint8 reportId;
  uint8 pairRef;
  uint16 vendorId;
  uint8 ieeeAddr[8];
} zidDongleReportPairEntry_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

extern uint8 zidDongleTaskId;

/**************************************************************************************************
 * @fn          Dongle_Init
 *
 * @brief       This is the ZID Dongle task initialization called by OSAL.
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
void Dongle_Init(uint8 taskId);

/**************************************************************************************************
 *
 * @fn          Dongle_ProcessEvent
 *
 * @brief       This routine handles the OSAL task events.
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
uint16 Dongle_ProcessEvent(uint8 taskId, uint16 events);

/*********************************************************************
 * FUNCTIONS
 */

//
// Network Processor Interface APIs
//

/**************************************************************************************************
 * @fn          zidDongleOutputReport
 *
 * @brief       This function is a ZIDNPI callback to the client that inidcates a
 *              message has been received. The client software is
 *              expected to complete this call.
 *
 *              Note: The client must process this message and provide a reply.
 *
 * input parameters
 *
 * uint8 *      zidNPIOutBuf  - pointer to buffer for OUT report
 * uint8        len           - lenght of OUT report
 *
 * output parameters
 *
 * none
 *
 * @return      None.
 **************************************************************************************************
 */
void zidDongleOutputReport(uint8 endPoint, uint8 *zidNPIOutBuf , uint8 len );

/***************************************************************************
 * @fn          zidDongleSendCmdReport
 *
 * @brief   Send the remote input report ID=2 stored in global data structure
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************
 */
void zidDongleSendCmdReport(uint8 cmdId, uint8 status);

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
