/*
  Filename:       zid_proxy.c
*/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include <stddef.h>
#include "comdef.h"
#include "hal_assert.h"
#include "hal_led.h"
#include "OSAL.h"
#include "osal_snv.h"
#include "rti.h"
#include "rcn_nwk.h"
#include "zid_adaptor.h"
#include "zid_ada_app_helper.h"
#include "zid_common.h"
#include "zid_profile.h"
#include "zid_proxy.h"
#include "zid_dongle.h"
#include "zid_usb.h"
#include "usb_zid_reports.h"
#include "usb_zid_class_requests.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#if (ZID_COMMON_MAX_NUM_PROXIED_DEVICES < RCN_CAP_PAIR_TABLE_SIZE)
#if FEATURE_ZID_ZRC
#warning The ZID USB Proxy capability is less than the RTI pairing capability.
#else
// If acting as the ZID USB proxy host and not allowing CERC devices, then the RTI pairing table
// should not be bigger than the ZID proxy table or the Target device will be allowing pairings and
// then the ZID co-layer will be immediately un-pairing during the configuration state.
#error The ZID USB Proxy capability is less than the RTI pairing capability.
#endif
#endif

#if defined ZID_USB_RNP
// To enumerate the additional Keyboard interface we simply create a fake pxy
// entry stored as constant in flash. This is used if no ZID pairing exists.
// This Keyboard interface will then use the same report and endpoint as defined by ZID.
// Case not resolved in case of pairing with a ZID device with no Keyboard support.
const zid_proxy_entry_t pPxyZIDNPIKeyboard =
{
  0x0111, //  uint16 HIDParserVersion;
  0x0100, //  uint16 HIDDeviceReleaseNumber;
  0x0451, //  uint16 HIDVendorId;
  0x16C2, //  uint16 HIDProductId;
  0x00,   //  uint8  HIDDeviceSubclass;
  0x00,   //  uint8  HIDProtocolCode;
  0x00,   //  uint8  HIDCountryCode;
  0x02,   //  uint8  HIDNumEndpoints;
  0x01,   //  uint8  HIDPollInterval;
  0x02,   //  uint8  HIDNumStdDescComps;
  {ZID_STD_REPORT_MOUSE,
   ZID_STD_REPORT_KEYBOARD,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE,
   ZID_STD_REPORT_NONE},  // HIDStdDescCompsList
  0x00,   //  uint8  HIDNumNonStdDescComps;
  0x00,   //  uint8  HIDNumNullReports;
  0x00,   //  uint8  DeviceIdleRate;
  0x01    //  uint8  CurrentProtocol;
};
#endif //ZID_USB_RNP

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

static bool zidPxy_deviceEnumerated = FALSE;
static uint8 zidPxy_enumeratedDevicePairingEntry = RTI_INVALID_PAIRING_REF;
static uint8 pxyInfoTable[RCN_CAP_PAIR_TABLE_SIZE];

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          zidPxyEntry
 *
 * @brief       This function saves the Proxy Entry to NV & enumerates it on the USB.
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of source.
 * @param       pProxy - Pointer to the zid_proxy_entry_t.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidPxyEntry( uint8 pairIdx )
{
  rStatus_t status;
  zid_proxy_entry_t *pProxy;
  uint8 i;
  zid_non_std_desc_comp_t *nonStdDescCompList[aplcMaxNonStdDescCompsPerHID] = { NULL };
  uint8 localPairIndex = pairIdx;

  /* Set current proxy entry in preparation for reading of proxy info */
  status = RTI_WriteItemEx( RTI_PROFILE_ZID,
                            ZID_ITEM_CURRENT_PXY_NUM,
                            sizeof( uint8 ),
                            &localPairIndex );
  if (status == RTI_SUCCESS)
  {
    /* Read proxy info from ZID ADA */
    pProxy = osal_mem_alloc( sizeof(zid_proxy_entry_t) );
    if (pProxy != NULL)
    {
      status = RTI_ReadItemEx( RTI_PROFILE_ZID,
                               ZID_ITEM_CURRENT_PXY_ENTRY,
                               sizeof(zid_proxy_entry_t),
                               (uint8 *)pProxy );

      if (status == RTI_SUCCESS)
      {
        /* If proxy entry indicates non std descriptors, read them here */
        if (pProxy->HIDNumNonStdDescComps > 0)
        {
          for (i = 0; i < pProxy->HIDNumNonStdDescComps; i++)
          {
            nonStdDescCompList[i] = osal_mem_alloc( ZID_NON_STD_DESC_SPEC_T_MAX );
            (void)zidAdaAppHlp_ReadNonStdDescComp( pairIdx, i, nonStdDescCompList[i] );
          }
        }

        /* Now that information has been collected, enumerate the device */
#if !defined ZID_USB_RNP
        if (USB_ALREADY == zidUsbEnumerate( pProxy ))
        {
          // un-enumerate and try again
          zidUsbUnEnum();
          (void)zidUsbEnumerate(pProxy);
        }
#endif
        zidPxy_deviceEnumerated = TRUE;
        zidPxy_enumeratedDevicePairingEntry = pairIdx;

        /* Set LEDs to indicate device is ready to use */
#if (defined HAL_LED && (HAL_LED == TRUE))
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
        osal_stop_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_LED);
#endif
      }

      /* Done with memory, so free it */
      for (i = 0; i < pProxy->HIDNumNonStdDescComps; i++)
      {
        if (nonStdDescCompList[i] != NULL)
        {
          osal_mem_free( nonStdDescCompList[i] );
        }
      }
      osal_mem_free( pProxy );
    }
  }
}

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
void zidPxyInit(void)
{
  rStatus_t status;
  uint8 pxyIdx;
  bool proxyDevice = FALSE;
  zid_proxy_entry_t *pProxy = NULL;

  /* First see if there is a saved proxy info table */
  status = RTI_ReadItemEx( RTI_PROFILE_ZID,
                           ZID_ITEM_PXY_LIST,
                           sizeof( pxyInfoTable ),
                           (uint8 *)pxyInfoTable );
  if (status == RTI_SUCCESS)
  {
    /* There was a saved table. If any proxied items exist, enumerate them */
    uint8 idx;

    pxyIdx = RTI_INVALID_PAIRING_REF;
    for (idx = 0; idx < ZID_COMMON_MAX_NUM_PROXIED_DEVICES; idx++)
    {
      if (pxyInfoTable[idx] != RTI_INVALID_PAIRING_REF)
      {
        /* In this application, we are only allowing 1 proxy entry. So, we are
         * just going to proxy the last device added, which should correspond
         * to the highest pairing index.
         */
        pxyIdx = pxyInfoTable[idx];
      }
    }
  }
  else
  {
    /* No table, so no proxy entry */
    pxyIdx = RTI_INVALID_PAIRING_REF;
  }

  if (pxyIdx != RTI_INVALID_PAIRING_REF)
  {
    /* Found a device, so attempt to extract its proxy info. First set
     * current proxy entry in preparation for reading of proxy info.
     */
    status = RTI_WriteItemEx( RTI_PROFILE_ZID,
                              ZID_ITEM_CURRENT_PXY_NUM,
                              sizeof( uint8 ),
                              &pxyIdx );
    if (status == RTI_SUCCESS)
    {
      pProxy = osal_mem_alloc( sizeof(zid_proxy_entry_t) );
      if (pProxy != NULL)
      {
        status = RTI_ReadItemEx( RTI_PROFILE_ZID,
                                 ZID_ITEM_CURRENT_PXY_ENTRY,
                                 sizeof(zid_proxy_entry_t),
                                 (uint8 *)pProxy );
        if (status == RTI_SUCCESS)
        {
          proxyDevice = TRUE;
        }
      }
    }
  }

  if (proxyDevice == TRUE)
  {
    zidUsbUnEnum();
#if (defined HAL_LED && (HAL_LED == TRUE))
    if (USB_SUCCESS == zidUsbEnumerate( pProxy ))
    {
      HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    }
    else
    {
      HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
    }
#else
    (void)zidUsbEnumerate( pProxy );
#endif
    zidPxy_deviceEnumerated = TRUE;
    zidPxy_enumeratedDevicePairingEntry = pxyIdx;
  }
  else
  {
#if defined ZID_USB_RNP
    zidUsbUnEnum();
    pProxy = osal_mem_alloc(sizeof(zid_proxy_entry_t));
    if (pProxy != NULL)
    {
      osal_memcpy((uint8*)pProxy, (uint8*)&pPxyZIDNPIKeyboard, sizeof(zid_proxy_entry_t));
#if (defined HAL_LED && (HAL_LED == TRUE))
      if (USB_SUCCESS == zidUsbEnumerate( pProxy ))
      {
//      HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
      }
      else
      {
        HalLedSet(HAL_LED_1, HAL_LED_MODE_BLINK);
      }
#else
      (void)zidUsbEnumerate( pProxy );
#endif
    }
#endif
  }

  /* Free up memory if necessary */
  if (pProxy != NULL)
  {
    osal_mem_free( pProxy );
  }
}

/**************************************************************************************************
 * @fn          zidPxyReport
 *
 * @brief       This function acts to proxy the ZID report received.
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of source.
 * @param       pReport - Pointer to the zid_report_data_cmd_t.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if the report was successfully proxied; FALSE otherwise.
 */
uint8 zidPxyReport( uint8 pairIdx, zid_report_record_t *pReport )
{
  uint8 rtrn = FALSE;
  uint8 i;

  /* Since this application only allows 1 proxy device, the pairing index
   * is not used.
   */
  (void) pairIdx;

  if (pReport->type == ZID_REPORT_TYPE_IN)
  {
    // we will allow any report ID so that user can configure own desired implementation
    uint8 endPoint = ZID_PROXY_ZID_EP_IN_ADDR;

    // Assert the dependency on the packed, consecutive placement of 'id' & 'data'
    // in zid_report_data_cmd_t.
    HAL_ASSERT(offsetof(zid_report_record_t, data) == offsetof(zid_report_record_t, id)+1);

    // Attempt up to 3 times to send the report.
    for (i = 0; i < 3; i++)
    {
      if (rtrn = zidSendInReport((uint8 *)&pReport->id, endPoint, pReport->len - 1) == TRUE)
      {
        break;
      }
    }
  }

  return rtrn;
}

/**************************************************************************************************
 * @fn          zidPxyServeHIDClassRequests
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
uint8 zidPxyServeHIDClassRequests(uint8 requestType, ZID_CLASS_REQUEST_DATA_OUT *pRequestData)
{
  uint8 rtrn = FALSE, write = FALSE;
  rStatus_t status = !RTI_SUCCESS;
  zid_proxy_entry_t pProxy;

  if (zidPxy_deviceEnumerated == TRUE)
  {
    /* Read proxy info from ZID ADA */
    status = RTI_ReadItemEx( RTI_PROFILE_ZID,
                             ZID_ITEM_CURRENT_PXY_ENTRY,
                             sizeof(zid_proxy_entry_t),
                             (uint8 *)&pProxy );
    if (status == RTI_SUCCESS)
    {
      switch (requestType)
      {
        case GET_REPORT:
          break;
        case GET_IDLE:
          *pRequestData = pProxy.DeviceIdleRate;
          rtrn = TRUE;
          break;
        case GET_PROTOCOL:
          *pRequestData = pProxy.CurrentProtocol;
          rtrn = TRUE;
          break;
        case SET_REPORT:
          zidDongleOutputReport( 0, pRequestData , 0);
          rtrn = TRUE;
          break;
        case SET_IDLE:
          pProxy.DeviceIdleRate = *pRequestData;
          rtrn = TRUE;
          write = TRUE;
          break;
        case SET_PROTOCOL:
          pProxy.CurrentProtocol = *pRequestData;
          rtrn = TRUE;
          write = TRUE;
          break;
        default:
          break;
      }

      if (write == TRUE)
      {
        status = RTI_WriteItemEx( RTI_PROFILE_ZID,
                                  ZID_ITEM_CURRENT_PXY_ENTRY,
                                  sizeof(zid_proxy_entry_t),
                                  (uint8 *)&pProxy );
      }
    }
  }

  if (status == RTI_SUCCESS)
  {
    return rtrn;
  }
  else
  {
    return FALSE;
  }
}

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
void zidPxyReset( void )
{
  uint8 idx;

  if (zidPxy_deviceEnumerated == TRUE)
  {
#if !defined ZID_USB_RNP
    zidUsbUnEnum();
#endif
    zidPxy_deviceEnumerated = FALSE;
    zidPxy_enumeratedDevicePairingEntry = RTI_INVALID_PAIRING_REF;

    for (idx = 0; idx < RCN_CAP_PAIR_TABLE_SIZE; idx++)
    {
      pxyInfoTable[idx] = RTI_INVALID_PAIRING_REF;
    }

    (void) RTI_WriteItemEx( RTI_PROFILE_ZID,
                            ZID_ITEM_PXY_LIST,
                            sizeof( pxyInfoTable ),
                            (uint8 *)pxyInfoTable );
  }
}

/**************************************************************************************************
 * @fn          zidPxyUnpair
 *
 * @brief       This function removes the pairIdx from the information table, updates the table in
 *              NV, and minimizes the NV used (minimize any DescSpecs).
 *              If there is room to proxy, increase the Profile Id count to two if supporting ZRC.
 *
 * input parameters
 *
 * @param       pairIdx - Pairing table index of source.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidPxyUnpair( uint8 pairIdx )
{
  if (pairIdx == zidPxy_enumeratedDevicePairingEntry)
  {
#if !defined ZID_USB_RNP
    zidUsbUnEnum();
#endif
    zidPxy_deviceEnumerated = FALSE;
    zidPxy_enumeratedDevicePairingEntry = RTI_INVALID_PAIRING_REF;
  }

#if (defined HAL_LED && (HAL_LED == TRUE))
  HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
#endif
}

/**************************************************************************************************
*/
