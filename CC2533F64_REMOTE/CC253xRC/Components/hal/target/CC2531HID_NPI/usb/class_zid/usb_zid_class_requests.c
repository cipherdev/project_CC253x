/***********************************************************************************

    Filename:		usb_zid_class_request.c

    Description:	USB class request handler.

***********************************************************************************/


/***********************************************************************************
* INCLUDES
*/

#include "usb_descriptor.h"
#include "usb_framework.h"
#include "usb_zid_class_requests.h"
#include "usb_zid_reports.h"
#include "zid_common.h"
#include "zid_profile.h"
#include "zid_usb.h"
#include "zid_dongle.h"
#include "zid_proxy.h"
#include "OSAL_Memory.h"


/***********************************************************************************
* @fn          usbcrSetReport
*
* @brief       Implements support for the HID class request SET_REPORT.
*
* @return      none
*/
void usbcrSetReport(void)
{
    // Received setup header?
    if (usbfwData.ep0Status == EP_IDLE) {

        // Sanity check the incoming setup header:
        // Only accept output report for HID, let the proxy determine what to do with it
        if ( (HI_UINT16(usbSetupHeader.value) == HID_REP_TYPE_OUTPUT) ) {

            zidClassRequestDataOut = (ZID_CLASS_REQUEST_DATA_OUT *)osal_mem_alloc(usbSetupHeader.length);
            // Prepare to receive the data
            usbfwData.ep0Status = EP_RX;
            usbSetupData.pBuffer = zidClassRequestDataOut;
            usbSetupData.bytesLeft = usbSetupHeader.length;
            return;

        } else {

            // Unsupported: Stall the request
            usbfwData.ep0Status = EP_STALL;
            return;
        }

    // Received data?
    } else if (usbfwData.ep0Status == EP_RX) {

      // Send OUT report up to Application
      zidPxyServeHIDClassRequests(usbSetupHeader.request, zidClassRequestDataOut);

      osal_mem_free(zidClassRequestDataOut);

    }

} // usbcrSetReport


/***********************************************************************************
* @fn          usbcrGetReport
*
* @brief       Implements support for the HID class request GET_REPORT.
*
* @return      none
*/
void usbcrGetReport(void)
{
  uint8 n;
  // Received setup header?
  if (usbfwData.ep0Status == EP_IDLE) {

    // Perform a table search (on index and value)
    usbSetupData.pBuffer = NULL;
    for (n = 0; n < ((uint16)usbDescriptorMarker.pUsbDescLutEnd - (uint16)usbDescriptorMarker.pUsbDescLut) / sizeof(DESC_LUT_INFO); n++) {
      if ((usbDescriptorMarker.pUsbDescLut[n].valueMsb == HI_UINT16(usbSetupHeader.value))
          && (usbDescriptorMarker.pUsbDescLut[n].valueLsb == LO_UINT16(usbSetupHeader.value))
            && (usbDescriptorMarker.pUsbDescLut[n].indexMsb == HI_UINT16(usbSetupHeader.index))
              && (usbDescriptorMarker.pUsbDescLut[n].indexLsb == LO_UINT16(usbSetupHeader.index)) )
      {
        // Call GetReport() from the paired ZID controller (which corresponds to the requested report).
        // This will be responded with a SendReport if successfull, i.e. not necessary to do more.
        return;
      }
    }

    // If here is reached, this means the report was not found

    // Unsupported: Stall the request
    usbfwData.ep0Status = EP_STALL;
    return;

    // Data transmitted?
  } else if (usbfwData.ep0Status == EP_TX) {

    // The USB firmware library will return here after the keyboard/mouse
    // report has been transmitted, but there is nothing for us to do here.
  }

} // usbcrGetReport


/***********************************************************************************
* @fn          usbcrSetProtocol
*
* @brief       Implements support for the HID class request SET_PROTOCOL.
*              This request is only required for HID devices in the "boot" subclass.
*
* @return      none
*/
void usbcrSetProtocol(void)
{
  // Received setup header?
  if (usbfwData.ep0Status == EP_IDLE) {

    // Sanity check setup request parameters
    if ((usbSetupHeader.value & 0xFFFE) ||
        (usbSetupHeader.length != 0) ||
          (usbSetupHeader.index > ZID_IFCE_INDEX)) {

            // Unsupported: Stall the request
            usbfwData.ep0Status = EP_STALL;
            return;

          } else {
            // Optional in the ZID spec
            usbfwData.ep0Status = EP_STALL;
          }

    // This request has only a setup stage (no data stage)
    return;
  }

} // usbcrSetProtocol


/***********************************************************************************
* @fn          usbcrGetProtocol
*
* @brief       Implements support for the HID class request GET_PROTOCOL.
*              This request is only required for HID devices in the "boot" subclass.
*
* @return      none
*/
void usbcrGetProtocol(void)
{
  static uint8 dataBuf;

  // Received setup header?
  if (usbfwData.ep0Status == EP_IDLE) {

    // Sanity check setup request parameters
    if ((usbSetupHeader.value != 0) ||
        (usbSetupHeader.length != 1) ||
          (usbSetupHeader.index > ZID_IFCE_INDEX)) {

            // Unsupported: Stall the request
            usbfwData.ep0Status = EP_STALL;
            return;

          } else {
            uint8 status = FALSE;
            // Optional in the ZID spec
            status = zidPxyServeHIDClassRequests(GET_PROTOCOL, (uint8*) &dataBuf);
            if (status == FALSE)
            {
              usbfwData.ep0Status = EP_STALL;
            }
            else
            {
              usbSetupData.pBuffer = (uint8 *)&dataBuf;
              usbSetupData.bytesLeft = 1;
              usbfwData.ep0Status = EP_TX;
            }
            return;
          }

    // Data transmitted?
  } else if (usbfwData.ep0Status == EP_TX) {

        // The USB firmware library will return here after the protocol data
        // has been transmitted, but there is no need for us to do anything here.
    }

} // usbcrGetProtocol


/***********************************************************************************
* @fn          usbcrSetIdle
*
* @brief       Implements support for the HID class request SET_IDLE.
*              This request is optional for mouse devices, but required by keyboards.
*
* @return      none
*/
void usbcrSetIdle(void)
{
  // Received setup header?
  if (usbfwData.ep0Status == EP_IDLE) {

    // Sanity check setup request parameters
    if ((usbSetupHeader.length != 0) ||
        (usbSetupHeader.index != ZID_IFCE_INDEX)) {

          // Unsupported: Stall the request
          usbfwData.ep0Status = EP_STALL;
          return;

        } else {
          /* You could map the ReportID (which is found in the low byte of wValue),
          * to find a paired device.
          */
          uint8 value;
          //uint8 idleRate = HI_UINT16(usbSetupHeader.value);
          switch (LO_UINT16(usbSetupHeader.index)) {

          case ZID_IFCE_INDEX:
            value = HI_UINT16(usbSetupHeader.value);
            zidPxyServeHIDClassRequests(SET_IDLE, (uint8*) &value);
            break;
          default:
            break;
          }

          // This request has only a setup stage (no data stage)
          return;
        }
  }

} // usbcrSetIdle


/***********************************************************************************
* @fn          usbcrGetIdle
*
* @brief       Implements support for the HID class request GET_IDLE.
*              This request is optional for mouse devices, but required by keyboards.
*
* @return      none
*/
void usbcrGetIdle(void)
{
  static uint8 dataBuf;

    // Received setup header?
    if (usbfwData.ep0Status == EP_IDLE) {

        // Sanity check setup request parameters
        if ((usbSetupHeader.length != 1) ||
            (usbSetupHeader.index > ZID_IFCE_INDEX)) {

            // Unsupported: Stall the request
            usbfwData.ep0Status = EP_STALL;
            return;

        } else {
            uint8 status = FALSE;
            // Optional in the ZID spec
            status = zidPxyServeHIDClassRequests(GET_IDLE, (uint8*) &dataBuf);
            if (status == FALSE)
            {
              usbfwData.ep0Status = EP_STALL;
            }
            else
            {
              usbSetupData.pBuffer = (uint8 *)&dataBuf;
              usbSetupData.bytesLeft = 1;
              usbfwData.ep0Status = EP_TX;
            }
            return;
        }

    // Data transmitted?
    } else if (usbfwData.ep0Status == EP_TX) {

        // The USB firmware library will return here after the idle data
        // has been transmitted, but there is no need for us to do anything here.
    }

} // usbcrGetIdle

