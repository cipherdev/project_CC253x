/**************************************************************************************************
  Filename:       rcn_config.c
  Revised:        $Date: 2011-03-09 16:30:51 -0800 (Wed, 09 Mar 2011) $
  Revision:       $Revision: 25353 $

  Description:    This file contains configurable variables of RF4CE network layer.


  Copyright 2009-2011 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

#include "hal_types.h"
#include "mac_radio_defs.h"
#include "rcn_attribs.h"
#include "rcn_nwk.h"

/**************************************************************************************************
 *                                           Constant
 */


/**************************************************************************************************
 *                                            Macros
 */

// Macro to get MAC LQI value from RSSI value
#define RCN_MAC_LQI_FROM_RSSI(_rssi) \
  (0xff * ((_rssi) - (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + 10)) \
   / (MAC_RADIO_RECEIVER_SATURATION_DBM - \
   (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + 10)))


// Macro to get CCA threshold register value from RSSI level
#if (defined HAL_MCU_CC2530 || defined HAL_MCU_CC2531 || defined HAL_MCU_CC2533)
#define RCN_CCA_TH_FROM_RSSI(_rssi) ((_rssi) + 76)
#elif (defined HAL_MCU_MSP430)
#warning Need to implement RCN_CCA_TH_FROM_RSSI
#else
#warning Arbitrary definition of RCN_CCA_TH_FROM_RSSI macro.
// Note that non-CC2530 RSSI level is arbitrary here
#define RCN_CCA_TH_FROM_RSSI(_rssi) ((_rssi) + 38)
#endif

/**************************************************************************************************
 *                                        Type definitions
 */

/**************************************************************************************************
 *                                        Global Variables
 */
// Duration in milliseconds to suspend frequency agility,
// when the device stayed on all channels too short.
uint16 rcnFASuspendDur = 60000;

// Threshold in number of samples collected staying on a channel
// to determine whether the device stayed on the channel too short.
// One sample is collected every two milliseconds.
// Hence, 30,000 samples equal to duration of 2 * 30,000 = 60,000 milliseconds
uint16 rcnFAShortDurationTh = 30000;

// Threshold of number of samples with high noise level which will trigger
// frequency agility
uint8 rcnFANoisySamplesTh = 16;

// Minimum number of samples to collect before triggering frequency agility
// from a channel
uint8 rcnFAMinNumSamples = 32;

// Threshold of MAC LQI (note that it is not RF4CE LQI) level which is
// used to determine whether an energy sample has high noise.
// Energy sample with LQI value greater than or equal to this threshold is considered
// as high noise sample.
uint8 rcnFALQITh = RCN_MAC_LQI_FROM_RSSI(-72);

// CCA threshold register value (2's complement).
// Note that the offset of this value depends on platform
#if (defined HAL_MCU_CC2530 || defined HAL_MCU_CC2531 || defined HAL_MCU_CC2533)
int8 rcnCCATh = RCN_CCA_TH_FROM_RSSI(-84);
#endif

// Security key exchange command frame transmit power level in units of dBm.
// Note that the transmit power has to be smaller than or equal to
// RF4CE constant nwkcMaxSecCmdTxPower.
int8 rcnSecKeyTxPwr = -25;

// Pairing Table related variables
const uint8 gRCN_CAP_MAX_PAIRS = RCN_CAP_PAIR_TABLE_SIZE;
rcnNwkPairingEntry_t rcnPairingTable[RCN_CAP_PAIR_TABLE_SIZE];

/**************************************************************************************************
 *                                        Local Variables
 */

/**************************************************************************************************
 *                                     Local Function Prototypes
 */

