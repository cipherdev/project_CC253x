/**************************************************************************************************
  Filename:       gdp_profile.h
**************************************************************************************************/
#ifndef GDP_PROFILE_H
#define GDP_PROFILE_H

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

// Event for multiple profile configuration
#define GDP_EVT_CONFIGURE_NEXT            0x1000 // Configure next profile event
// Event for multiple profile configuration. This event is set if any subconfigurations fail
#define GDP_EVT_CONFIGURATION_FAILED      0x0800 // Configuration phase failed

// Identifier for which function invoked Configuration
#define GDP_CFG_INVOKED_BY_INVALID         0x00
#define GDP_CFG_INVOKED_BY_ALLOW_PAIR      0x01
#define GDP_CFG_INVOKED_BY_PAIR            0x02

// GDP header (a.k.a. Frame Control field)
#define GDP_HEADER_DATA_PENDING            0x80
#define GDP_HEADER_GDP_CMD                 0x40
#define GDP_HEADER_CMD_CODE_MASK           0x4F

// GDP Command Code field values (Table 3).
#define GDP_CMD_GENERIC_RSP                0x40
#define GDP_CMD_CFG_COMPLETE               0x41
#define GDP_CMD_HEARTBEAT                  0x42
#define GDP_CMD_GET_ATTR                   0x43
#define GDP_CMD_GET_ATTR_RSP               0x44
#define GDP_CMD_PUSH_ATTR                  0x45

// GDP command field constants
#define GDP_GENERIC_RSP_SUCCESS            0x00
#define GDP_GENERIC_RSP_UNSUPPORTED_REQ    0x01
#define GDP_GENERIC_RSP_INVALID_PARAM      0x02
#define GDP_GENERIC_RSP_CONFIG_FAILURE     0x03

#define GDP_ATTR_RSP_SUCCESS               0x00
#define GDP_ATTR_RSP_UNSUPPORTED           0x01
#define GDP_ATTR_RSP_ILLEGAL_REQ           0x02

// GDP Command field indices
#define GDP_PUSH_ATTR_RECORD_ATTR_ID_IDX   0x00
#define GDP_PUSH_ATTR_RECORD_ATTR_LEN_IDX  0x01
#define GDP_PUSH_ATTR_RECORD_ATTR_VAL_IDX  0x02

/* ------------------------------------------------------------------------------------------------
 *                                     GDP Profile Constants
 * ------------------------------------------------------------------------------------------------
 */

// Length of time between completing the configuration phase of one profile and starting the next
// to allow the node to perform internal housekeeping tasks.
#define aplcConfigBlackoutTime             100   // Time in msec

// Max time a device waits after receiving successful NLME-AUTO-DISCOVERY.confirm for a pair ind.
#define aplcGdpMaxPairIndicationWaitTime   1200  // Time in msec.

// Max time a node shall wait for a response to a Heartbeat or stay awake by Rx ctl bit.
#define aplcMaxRxOnWaitTime                100   // Time in msec.

// Max time a device shall wait for a response command frame following a request command frame.
#define aplcMaxResponseWaitTime            200   // Time in msec.

// Min value of the KeyExTransferCount parameter passed to a pair request primitive during pairing.
#define aplcMinKeyExchangeTransferCount    3

/* ------------------------------------------------------------------------------------------------
 *                                     GDP Profile Attributes
 * ------------------------------------------------------------------------------------------------
 */

// Reserved                           0x00-0x81
#define aplKeyExchangeTransferCount        0x82
#define aplPowerStatus                     0x83

#define GDP_ATTR_KEY_EXCHANGE_TRANSFER_COUNT_SIZE 1
#define GDP_ATTR_POWER_STATUS_SIZE 1
#define GDP_TOTAL_ATTR_SIZE  (GDP_ATTR_KEY_EXCHANGE_TRANSFER_COUNT_SIZE + GDP_ATTR_POWER_STATUS_SIZE)

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

  // A packed structure that constitutes an Attribute record field with any data packed LSB first.
typedef struct {
  uint8 id;
  uint8 len;
  uint8 data[];
} gdp_push_attr_rec_t;
#define GDP_PUSH_ATTR_REC_SIZE_HDR             (sizeof(gdp_push_attr_rec_t))

// A packed structure that constitutes an Attribute Status record field with any data packed LSB.
typedef struct {
  uint8 id;
  uint8 status;
  uint8 len;
  uint8 data[];
} gdp_attr_rsp_t;
#define GDP_ATTR_RSP_SIZE_HDR             (sizeof(gdp_attr_rsp_t))

typedef struct
{
  uint8 pairingRef; // current pairing index for multiple profile configuration
  uint8 prevConfiguredProfile;  // previously configured profile
  uint8 invokedBy;  // configuration invoked by; RTI_AllowPairCnf or RTI_PairCnf
} gdpCfgParam_t;
#define GDP_CONFIG_PHASE_PARAM_SIZE       (sizeof(gdpCfgParam_t))

/**************************************************************************************************
*/

#ifdef __cplusplus
};
#endif

#endif
