/*
  Filename:       hal_mac_cfg.h
*/

#ifndef HAL_MAC_CFG_H
#define HAL_MAC_CFG_H

/*
 *   Board Configuration File for low-level MAC
 *  --------------------------------------------
 *   Manufacturer : Texas Instruments
 *   Part Number  : CC2530EB
 *   Processor    : Texas Instruments CC2530
 *
 */


/* ------------------------------------------------------------------------------------------------
 *                                  Board Specific Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_MAC_RSSI_OFFSET                         -73   /* no units */
#if defined (HAL_PA_LNA)
/* CC22591 RSSI offset */
#define HAL_MAC_RSSI_LNA_HGM_OFFSET                 -9
#define HAL_MAC_RSSI_LNA_LGM_OFFSET                  4
#elif defined (HAL_PA_LNA_CC2590)
/* CC22590 RSSI offset */
#define HAL_MAC_RSSI_LNA_HGM_OFFSET                 -10
#define HAL_MAC_RSSI_LNA_LGM_OFFSET                  0
#endif


/**************************************************************************************************
*/
#endif
