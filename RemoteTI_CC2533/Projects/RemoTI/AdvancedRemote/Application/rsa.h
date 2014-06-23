/**************************************************************************************************
  Filename:       rsa.h
**************************************************************************************************/

#ifndef RSA_H
#define RSA_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"

/**************************************************************************************************
 *                                        User's  Defines
 **************************************************************************************************/

#ifndef RSA_KEY_INT_ENABLED
#define RSA_KEY_INT_ENABLED       TRUE
#endif

#ifndef RCN_SUPPRESS_SCAN_RESULT
#define RCN_SUPPRESS_SCAN_RESULT  FALSE
#endif
  
#ifndef RSA_FEATURE_IR
#define RSA_FEATURE_IR            FALSE         /* IR interface support */
#endif
  
/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/* Event IDs */
#define RSA_PAIR_EVENT   0x0003

// Channel bitmap is effective only for RCN coordinator. RCN end device is not affected by
// this bitmap.
#ifdef RSA_SINGLE_CH
  // Single channel test purpose special build
# define RSA_MAC_CHANNELS MAC_CHAN_MASK(MAC_CHAN_15)
#else
# define RSA_MAC_CHANNELS \
  (MAC_CHAN_MASK(MAC_CHAN_15) | MAC_CHAN_MASK(MAC_CHAN_20) | MAC_CHAN_MASK(MAC_CHAN_25))
#endif
  
/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/
extern uint8 RSA_TaskId;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the RF4CE Sample Application
 */
extern void RSA_Init( uint8 task_id );

/*
 * Task Event Processor for the RF4CE Sample Application
 */
extern uint16 RSA_ProcessEvent( uint8 task_id, uint16 events );

/*
 * Handle keys
 */
extern void RSA_KeyCback( uint8 keys, uint8 state);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RSA_H */
