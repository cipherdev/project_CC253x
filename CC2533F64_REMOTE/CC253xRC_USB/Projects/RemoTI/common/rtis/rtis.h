/**************************************************************************************************
  Filename:       rtis.h
  Revised:        $Date: 2008-08-27 14:30:47 -0700 (Wed, 27 Aug 2008) $
  Revision:       $Revision: 17210 $

  Description:    This file contains the the RemoTI (RTI) Surrogate API.

  Copyright 2008-2011 Texas Instruments Incorporated. All rights reserved.

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
#ifndef RTIS_H
#define RTIS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/

/* NPI includes */
#include "npi.h"

/* RTI includes */
#include "rti.h"
#include "rti_constants.h"
#include "rti_deprecated.h"
#include "zid.h"

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

// RTIS Command Ids for NPI Callback
#define RTIS_CMD_ID_RTI_READ_ITEM              0x01  // Deprecated for 0x21
#define RTIS_CMD_ID_RTI_WRITE_ITEM             0x02  // Deprecated for 0x22
#define RTIS_CMD_ID_RTI_INIT_REQ               0x03
#define RTIS_CMD_ID_RTI_PAIR_REQ               0x04
#define RTIS_CMD_ID_RTI_SEND_DATA_REQ          0x05
#define RTIS_CMD_ID_RTI_ALLOW_PAIR_REQ         0x06
#define RTIS_CMD_ID_RTI_STANDBY_REQ            0x07
#define RTIS_CMD_ID_RTI_RX_ENABLE_REQ          0x08
#define RTIS_CMD_ID_RTI_ENABLE_SLEEP_REQ       0x09
#define RTIS_CMD_ID_RTI_DISABLE_SLEEP_REQ      0x0A
#define RTIS_CMD_ID_RTI_UNPAIR_REQ             0x0B
#define RTIS_CMD_ID_RTI_PAIR_ABORT_REQ         0x0C
#define RTIS_CMD_ID_RTI_ALLOW_PAIR_ABORT_REQ   0x0D
//
#define RTIS_CMD_ID_TEST_PING_REQ              0x10
#define RTIS_CMD_ID_RTI_TEST_MODE_REQ          0x11
#define RTIS_CMD_ID_RTI_RX_COUNTER_GET_REQ     0x12
#define RTIS_CMD_ID_RTI_SW_RESET_REQ           0x13
//
#define RTIS_CMD_ID_RTI_READ_ITEM_EX           0x21
#define RTIS_CMD_ID_RTI_WRITE_ITEM_EX          0x22

// RTIS Confirm Ids
#define RTIS_CMD_ID_RTI_INIT_CNF               0x01
#define RTIS_CMD_ID_RTI_PAIR_CNF               0x02
#define RTIS_CMD_ID_RTI_SEND_DATA_CNF          0x03
#define RTIS_CMD_ID_RTI_ALLOW_PAIR_CNF         0x04
#define RTIS_CMD_ID_RTI_REC_DATA_IND           0x05
#define RTIS_CMD_ID_RTI_STANDBY_CNF            0x06
#define RTIS_CMD_ID_RTI_RX_ENABLE_CNF          0x07
#define RTIS_CMD_ID_RTI_ENABLE_SLEEP_CNF       0x08
#define RTIS_CMD_ID_RTI_DISABLE_SLEEP_CNF      0x09
#define RTIS_CMD_ID_RTI_UNPAIR_CNF             0x0A
#define RTIS_CMD_ID_RTI_UNPAIR_IND             0x0B
#define RTIS_CMD_ID_RTI_PAIR_ABORT_CNF         0x0C
#define RTIS_CMD_ID_RTI_RESET_IND              0x0D

// RTI States
enum
{
  RTIS_STATE_INIT,
  RTIS_STATE_READY,
  RTIS_STATE_NETWORK_LAYER_BRIDGE
};

#ifdef __cplusplus
}
#endif

#endif

/*********************************************************************
*********************************************************************/
