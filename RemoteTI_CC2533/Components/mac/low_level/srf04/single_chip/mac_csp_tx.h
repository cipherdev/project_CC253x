/**************************************************************************************************
  Filename:       mac_csp_tx.h
**************************************************************************************************/

#ifndef MAC_CSP_TX_H
#define MAC_CSP_TX_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_mcu.h"
#include "mac_mcu.h"
#include "mac_high_level.h"


/* ------------------------------------------------------------------------------------------------
 *                                         Prototypes
 * ------------------------------------------------------------------------------------------------
 */
MAC_INTERNAL_API void macCspTxReset(void);

MAC_INTERNAL_API void macCspTxPrepCsmaUnslotted(void);
MAC_INTERNAL_API void macCspTxPrepCsmaSlotted(void);
MAC_INTERNAL_API void macCspTxPrepSlotted(void);

MAC_INTERNAL_API void macCspTxGoCsma(void);
MAC_INTERNAL_API void macCspTxGoSlotted(void);

MAC_INTERNAL_API void macCspForceTxDoneIfPending(void);

MAC_INTERNAL_API void macCspTxRequestAckTimeoutCallback(void);
MAC_INTERNAL_API void macCspTxCancelAckTimeoutCallback(void);

MAC_INTERNAL_API void macCspTxStopIsr(void);
MAC_INTERNAL_API void macCspTxIntIsr(void);


/**************************************************************************************************
 */
#endif
