/**************************************************************************************************
  Filename:       mac_rf4ce.h
**************************************************************************************************/

#ifndef MAC_RF4CE_H
#define MAC_RF4CE_H

/* ------------------------------------------------------------------------------------------------
 *                                           Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */

extern void MAC_InitRf4ce(void);
extern void MAC_InitRf4ceDevice(void);
extern void MAC_InitRf4ceCoord(void);
extern uint8 MAC_CbackQueryRetransmit(void);

#endif /* MAC_RF4CE_H */
