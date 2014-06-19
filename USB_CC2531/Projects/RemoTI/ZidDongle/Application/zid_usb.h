/*
  Filename:       zid_usb.h

*/
#ifndef ZID_USB_H
#define ZID_USB_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */
  #define USB_SUCCESS   0
  #define USB_MEMORY0   1
  #define USB_INVALID   2
  #define USB_ALREADY   3
/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
#define INTERFACE_NUMBER_RNP                0x00
#define INTERFACE_NUMBER_CE                 0x01
#define INTERFACE_NUMBER_ZID                0x02
#else //ZID_USB_RNP
#if defined (ZID_USB_CE) || defined (ZID_USB_RNP)
#warning ZID_USB_RNP and ZID_USB_CE must be defined together for now, due to TargetEmulator
#endif
#define INTERFACE_NUMBER_ZID                0x00
#endif //ZID_USB_RNP && ZID_USB_CE
  
#define ZID_PROXY_RNP_EP_OUT_ADDR        0x03
#define ZID_PROXY_RNP_EP_IN_ADDR         ZID_PROXY_RNP_EP_OUT_ADDR
#define ZID_PROXY_ZID_EP_OUT_ADDR        0x04
#define ZID_PROXY_ZID_EP_IN_ADDR         0x05
  
extern const uint8 zid_usb_report_len[ZID_STD_REPORT_TOTAL_NUM+1];

/**************************************************************************************************
 * @fn          zidUsbEnumerate
 *
 * @brief       This function initializes the USB Descriptor globals and enumerates the CC2531 to
 *              the USB host.
 *
 * input parameters
 *
 * @param       pPxy - A pointer to the zid_proxy_entry_t.
 *
 * output parameters
 *
 * None.
 *
 * @return      0 = SUCCESS
 *              1 = Not enough memory.
 *              2 = Invalid config.
 *              3 = Already enumerated.
 */
uint8 zidUsbEnumerate(zid_proxy_entry_t *pPxy);

/**************************************************************************************************
 * @fn          zidUsbUnEnum
 *
 * @brief       This function un-initializes the USB Descriptor globals for a ZID Target.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidUsbUnEnum(void);

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
