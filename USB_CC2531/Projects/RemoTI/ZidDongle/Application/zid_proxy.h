/*
  Filename:       zid_proxy.h
*/
#ifndef ZID_PROXY_H
#define ZID_PROXY_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "usb_zid_reports.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          zidPxyEntry
 *
 * @brief       This function saves the Proxy Entry to NV.
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidPxyEntry( uint8 pairIdx );

/**************************************************************************************************
 * @fn          zidPxyInit
 *
 * @brief       Initialization for a ZID Profile Adapter device acting as a USB proxy.
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
void zidPxyInit(void);

/**************************************************************************************************
 * @fn          zidPxyReport
 *
 * @brief       This function acts to proxy the ZID report received.
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of source.
 * @param       pReport - Pointer to the zid_report_record_t.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the report was successfully proxies; FALSE otherwise.
 */
uint8 zidPxyReport( uint8 pairIdx, zid_report_record_t *pReport );

/**************************************************************************************************
 * @fn          zidSPxyerveHIDClassRequests
 *
 * @brief       This function acts to serve HID Class Requests.
 *
 * input parameters
 *
 * @param       requestType - request type.
 * @param       pRequestData - Pointer to the class request data.
 *
 * output parameters
 *
 * pRequestData - in case of a Get request
 *
 * @return      TRUE if the request was successfully served; FALSE otherwise.
 */
uint8 zidPxyServeHIDClassRequests( uint8 requestType, ZID_CLASS_REQUEST_DATA_OUT *pRequestData );

/**************************************************************************************************
 * @fn          zidPxyReset
 *
 * @brief       Reset for a ZID Profile Adapter device acting as a USB proxy.
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
void zidPxyReset(void);

/**************************************************************************************************
 * @fn          zidPxyUnpair
 *
 * @brief       This function removes the pairIdx from the information table, updates the table in
 *              NV, and minimizes the NV used (minimize any DescSpecs).
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of target.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidPxyUnpair(uint8 pairIdx);

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
