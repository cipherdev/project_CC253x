/**************************************************************************************************
  Filename:       rti_deprecated.h
**************************************************************************************************/
#ifndef RTI_DEPRECATED_H
#define RTI_DEPRECATED_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "rti.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define RTI_PROFILE_CERC                              0x01

#define RTI_ERROR_PAIR_COMPLETE                       0x24

#define RTI_DISCOVERY_DURATION                        100
#define RTI_DISCOVERY_LQI_THRESHOLD                   0
#define RTI_RESP_WAIT_TIME                            100
#define RTI_CERC_DISCOVERY_REPETITION_INTERVAL        1000 // one second, equals 0x00f424 symbols
#define RTI_CERC_MAX_DISCOVERY_REPETITIONS            30
#define RTI_CERC_MAX_REPORTED_NODE_DESCRIPTORS        1
#define RTI_CERC_AUTO_DISC_DURATION                   30000 // thirty seconds, equals 0x1c9c38 symbols

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

// PC Tools left still using the deprecated Read/Write API.
extern RTILIB_API rStatus_t RTI_ReadItem(uint8 itemId, uint8 len, uint8 *pValue);
extern RTILIB_API rStatus_t RTI_WriteItem(uint8 itemId, uint8 len, uint8 *pValue);

#ifdef __cplusplus
};
#endif

#endif

/**************************************************************************************************
*/
