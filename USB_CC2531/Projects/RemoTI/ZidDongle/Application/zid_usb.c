/**************************************************************************************************
  Filename:       zid_usb.c

**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "hal_assert.h"
#include "OSAL.h"
#include "OSAL_Memory.h"
#include "osal_snv.h"
#include "rti.h"
#include "rcn_nwk.h"

#include "usb_descriptor.h"
#include "usb_framework.h"
#include "usb_zid.h"
#include "usb_zid_reports.h"
#include "usb_interrupt.h"
#include "usb_suspend.h"

#include "zid_common.h"
#include "zid_profile.h"
#include "zid_proxy.h"
#include "zid_usb.h"
#include "zid_dongle.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#define DESC_TYPE_HID          0x21  // HID descriptor (included in the interface descriptor)
#define DESC_TYPE_HIDREPORT    0x22  // Report descriptor (referenced in \ref usbDescLut)

// bNumEndpoints field of the Interface Descriptor
#define ZID_SUPPORT_ONLY_IN_EP  0x01 // Set to 0x01 if only IN EP is supported
#define ZID_SUPPORT_OPT_OUT_EP  0x02 // Set to 0x02 if optional OUT EP is supported

/* There are 3 bytes additional length per extra report for the HID Descriptor.
 * E.g. 15 (5 additional reports, a total of 6 reports).
 */
#define DESC_SIZE_HID_ADD_PER_DESC            3
// HID Class Descriptor size with 1 HID Report Descriptor.
#define DESC_SIZE_HID_W1HIDRD                 0x09
#define DESC_SIZE_HID_TOTAL(W)               (DESC_SIZE_HID_W1HIDRD + \
                                             (DESC_SIZE_HID_ADD_PER_DESC) * ((W)-1))

/* Following are indices to fields of the HID Descriptor which populated on runtime */
#define ZID_DESC_HID_INDEX_LENGTH             0     // bLength
#define ZID_DESC_HID_INDEX_DESCRIPTORTYPE     1     // bDescriptorType
#define ZID_DESC_HID_INDEX_BCDHID             2     // bcdHID           - ZID_ITEM_PARSER_VER
#define ZID_DESC_HID_INDEX_COUNTRYCODE        4     // bCountryCode     - ZID_ITEM_COUNTRY_CODE
#define ZID_DESC_HID_INDEX_NUMDESCRIPTORS     5     // bNumDescriptors  - ZID_ITEM_NUM_STD_DESC + ZID_ITEM_NUM_NON_STD_DESC_COMPS
#define ZID_DESC_HID_INDEX_DESCRIPTORTYPE_1   6     // bDescriptorType  - index of first HID Report Descriptor type

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
#define ZID_USB_NUM_INTERFACES                3
#else
#define ZID_USB_NUM_INTERFACES                1
#endif

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
#define DESC_LUT_INFO_SIZE                   (sizeof(DESC_LUT_INFO) * 3 * 2)  // Need both HID and HID Report descriptors
#define DBLBUF_LUT_INFO_SIZE                 (sizeof(DBLBUF_LUT_INFO) * 3)
#else //ZID_USB_RNP
#define DESC_LUT_INFO_SIZE                   (sizeof(DESC_LUT_INFO) * 1 * 2)  // Need both HID and HID Report descriptors
#define DBLBUF_LUT_INFO_SIZE                 (sizeof(DBLBUF_LUT_INFO) * 1)
#endif //ZID_USB_RNP && ZID_USB_CE

const uint8 usb_device_desc[] =
{
  DESC_SIZE_DEVICE,            // bLength
  DESC_TYPE_DEVICE,            // bDescriptorType
  0x00, 0x02,                  // bcdUSB (USB 2.0)
  0x00,                        // bDeviceClass (Given by interface)
  0x00,                        // bDeviceSubClass
  0x00,                        // bDeviceProtocol
  EP0_PACKET_SIZE,             // bMaxPacketSize0
  0x51, 0x04,                  // idVendor (Set after configuration phase)
  0xC2, 0x16,                  // idProduct (Set after configuration phase)
  0x00, 0x01,                  // bcdDevice (Set after configuration phase)
  0x01,                        // iManufacturer
  0x02,                        // iProduct
  0x03,                        // iSerialNumber
  0x01                         // bNumConfigurations
};

const uint8 zid_usb_report_len[ZID_STD_REPORT_TOTAL_NUM+1] =
{
  0,  // There is no report zero - MOUSE starts at one.
  ZID_MOUSE_DATA_LEN + 1,
  ZID_KEYBOARD_DATA_LEN + 1,
  ZID_CONTACT_DATA_DATA_LEN + 1,
  ZID_GESTURE_TAP_DATA_LEN + 1,
  ZID_GESTURE_SCROLL_DATA_LEN + 1,
  ZID_GESTURE_PINCH_DATA_LEN + 1,
  ZID_GESTURE_ROTATE_DATA_LEN + 1,
  ZID_GESTURE_SYNC_DATA_LEN + 1,
  ZID_TOUCH_SENSOR_PROPERTIES_DATA_LEN + 1,
  ZID_TAP_SUPPORT_PROPERTIES_DATA_LEN + 1
};

/**************************************************************************************************
 * Start of the Configuration1 total size.
 */

const uint8 usb_config_desc[] = {
  DESC_SIZE_CONFIG,            // bLength
  DESC_TYPE_CONFIG,            // bDescriptorType
  // wTotalLength - length of this and subordinate descriptors shall be filled in at run-time.
  0xFF, 0xFF,
  ZID_USB_NUM_INTERFACES,      // bNumInterfaces
  0x01,                        // bConfigurationValue
  0x00,                        // iConfiguration
  0xA0,                        // bmAttributes (Bit 5 remote wakeup)
  25                           // bMaxPower (2mA * 25 = 50 mA)
};

// Generic ZID interface descriptor.
const uint8 usb_iface_desc_zid[] = {
  DESC_SIZE_INTERFACE,         // bLength
  DESC_TYPE_INTERFACE,         // bDescriptorType
  INTERFACE_NUMBER_ZID,        // bInterfaceNumber
  0x00,                        // bAlternateSetting (None)
  0x01,                        // bNumEndpoints
  0x03,                        // bInterfaceClass (HID)
  0x00,                        // bInterfaceSubClass (None)
  0x00,                        // bInterfaceProcotol (N/A)
  0x00                         // iInterface
};

// Generic ZID Endpoint Descriptor (EP5 IN (chosen because of CC2531 USB controller FIFO size))
const uint8 int_in_ep_desc_zid[] = {
  DESC_SIZE_ENDPOINT,          // bLength
  DESC_TYPE_ENDPOINT,          // bDescriptorType
  0x85,                        // bEndpointAddress
  EP_ATTR_INT,                 // bmAttributes (INT)
  0x40, 0x00,                  // wMaxPacketSize  (maximum 64 for interrupt)
  0x0A                         // bInterval (10 full-speed frames = 10 ms)
};

// Generic ZID Endpoint Descriptor (EP4 OUT (chosen because of CC2531 USB controller FIFO size))
#define ZID_EP_OUT_ADDR        ZID_PROXY_ZID_EP_OUT_ADDR
const uint8 int_out_ep_desc_zid[] = {
  DESC_SIZE_ENDPOINT,          // bLength
  DESC_TYPE_ENDPOINT,          // bDescriptorType
  0x04,                        // bEndpointAddress
  EP_ATTR_INT,                 // bmAttributes (INT)
  0x40, 0x00,                  // wMaxPacketSize  (maximum 64 for interrupt)
  0x0A                         // bInterval (10 full-speed frames = 10 ms)
};

#if defined ZID_USB_RNP
// Proprietary interface descriptor for RNP command and response I/O.
const uint8 usb_iface_desc_rnp[] = {
  DESC_SIZE_INTERFACE,         // bLength
  DESC_TYPE_INTERFACE,         // bDescriptorType
  INTERFACE_NUMBER_RNP,        // bInterfaceNumber
  0x00,                        // bAlternateSetting (None)
  0x02,                        // bNumEndpoints
  0x03,                        // bInterfaceClass (HID)
  0x00,                        // bInterfaceSubClass (None)
  0x00,                        // bInterfaceProcotol (N/A)
  0x00                         // iInterface
};

// Generic RNP Endpoint Descriptor (EP3 IN (chosen because of CC2531 USB controller FIFO size))
const uint8 int_in_ep_desc_rnp[] = {
  DESC_SIZE_ENDPOINT,          // bLength
  DESC_TYPE_ENDPOINT,          // bDescriptorType
  0x83,                        // bEndpointAddress
  EP_ATTR_INT,                 // bmAttributes (INT)
  0x0C, 0x00,                  // wMaxPacketSize
  0x0A                         // bInterval (10 full-speed frames = 10 ms)
};

// Generic RNP Endpoint Descriptor (EP3 OUT (chosen because of CC2531 USB controller FIFO size))
#define RNP_EP_OUT_ADDR        ZID_PROXY_RNP_EP_OUT_ADDR
#define RNP_EP_OUT_PACKET_LEN  ZID_DONGLE_OUTBUF_SIZE
const uint8 int_out_ep_desc_rnp[] = {
  DESC_SIZE_ENDPOINT,          // bLength
  DESC_TYPE_ENDPOINT,          // bDescriptorType
  RNP_EP_OUT_ADDR,             // bEndpointAddress
  EP_ATTR_INT,                 // bmAttributes (INT)
  RNP_EP_OUT_PACKET_LEN, 0x00, // wMaxPacketSize
  0x0A                         // bInterval (10 full-speed frames = 10 ms)
};
#endif

#if defined ZID_USB_CE
// Proprietary interface descriptor for Consumer Electronics control.
const uint8 usb_iface_desc_ce[] = {
  DESC_SIZE_INTERFACE,         // bLength
  DESC_TYPE_INTERFACE,         // bDescriptorType
  INTERFACE_NUMBER_CE,         // bInterfaceNumber
  0x00,                        // bAlternateSetting (None)
  0x01,                        // bNumEndpoints
  0x03,                        // bInterfaceClass (HID)
  0x00,                        // bInterfaceSubClass (None)
  0x00,                        // bInterfaceProcotol (N/A)
  0x00                         // iInterface
};

// Proprietary Endpoint Descriptor (EP2 IN (chosen because of CC2531 USB controller FIFO size))
const uint8 int_in_ep_desc_ce[] = {
  DESC_SIZE_ENDPOINT,          // bLength
  DESC_TYPE_ENDPOINT,          // bDescriptorType
  0x82,                        // bEndpointAddress
  EP_ATTR_INT,                 // bmAttributes (INT)
  0x0C, 0x00,                  // wMaxPacketSize
  0x0A                         // bInterval (10 full-speed frames = 10 ms)
};

//// Proprietary Endpoint Descriptor (EP2 OUT (chosen because of CC2531 USB controller FIFO size))
//const uint8 usb_out_ep_desc_ce[] = {
//  DESC_SIZE_ENDPOINT,          // bLength
//  DESC_TYPE_ENDPOINT,          // bDescriptorType
//  0x02,                        // bEndpointAddress
//  EP_ATTR_INT,                 // bmAttributes (INT)
//  0x80, 0x00,                  // wMaxPacketSize
//  0x0A                         // bInterval (10 full-speed frames = 10 ms)
//};
#endif

/**************************************************************************************************
 * End of the Configuration1 total size.
 */

/**************************************************************************************************
 * String descriptors.
 */

// Language Id.
const uint8 string0LangId[] = {
  4,                           // bLength
  DESC_TYPE_STRING,            // bDescriptorType
  0x09, 0x04                   // wLangID (English-US)
};
// Size of Language Id string descriptor
#define DESC_SIZE_STRING_LANGID (sizeof(string0LangId))

// Manufacturer.
const uint8 string1Manufacterer[] = {
  36,                          // bLength
  DESC_TYPE_STRING,            // bDescriptorType
  'T', 0,                      // unicode string
  'e', 0,
  'x', 0,
  'a', 0,
  's', 0,
  ' ', 0,
  'I', 0,
  'n', 0,
  's', 0,
  't', 0,
  'r', 0,
  'u', 0,
  'm', 0,
  'e', 0,
  'n', 0,
  't', 0,
  's', 0
};
// Size of Manufacturer string descriptor
#define DESC_SIZE_STRING_MANUFACTURER (sizeof(string1Manufacterer))

// Product.
const uint8 string2Product[] = {
  30,                          // bLength
  DESC_TYPE_STRING,            // bDescriptorType
  'U', 0,                      // unicode string
  'S', 0,
  'B', 0,
  ' ', 0,
  'C', 0,
  'C', 0,
  '2', 0,
  '5', 0,
  '3', 0,
  '1', 0,
  ' ', 0,
  'Z', 0,
  'I', 0,
  'D', 0
};
// Size of Product string descriptor
#define DESC_SIZE_STRING_PRODUCT (sizeof(string2Product))

// Serial Number - left erased for programming of the IEEE.
const uint8 string3SerialNo[8] = {
  8,
  DESC_TYPE_STRING,
  0x30, 0x00,
  0x30, 0x00,
  0x31, 0x00
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF,
//  0xFF, 0xFF
};
// Size of Serial Number string descriptor
#define DESC_SIZE_STRING_SERIALNO (sizeof(string3SerialNo))

// Size of all string descriptors
#define DESC_SIZE_STRING  ( DESC_SIZE_STRING_LANGID + \
                            DESC_SIZE_STRING_MANUFACTURER + \
                            DESC_SIZE_STRING_PRODUCT + \
                            DESC_SIZE_STRING_SERIALNO)

/**************************************************************************************************
 * HID report descriptors
 */

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
// Vendor specific report descriptor
const uint8 zid_hid_rnp_report_desc[] = {
  0x06, 0x00, 0xFF,   // Usage Pg (Vendor Specific)
  0x09, 0x05,   // Usage (Vendor Usage 5 - RemoTI Target)
  0xA1, 0x01,   // Collection (Application)

  0x85, 0x01,   //   Report ID (1) - 2 byte commands
  0x15, 0x80,   //   Logical Min (-128)
  0x25, 0x7F,   //   Logical Max (127)
  0x09, 0x01,   //   Usage (Vendor Usage 1)
  0x09, 0x02,   //   Usage (Vendor Usage 2)
  0x75, 0x08,   //   Report Size (8)
  0x95, 0x02,   //   Report Count (2)
  0x91, 0x02,   //   Output (Data, Var, Abs)

  0x09, 0x01,   //   Usage (Vendor Usage 1)
  0x09, 0x02,   //   Usage (Vendor Usage 2)
  0x81, 0x02,   //   Input (Data, Var, Abs)

  0x85, 0x02,   //   Report ID (2) - paired entry
  0x09, 0x02,   //   Usage (Vendor Usage 2)
  0x95, 0x01,   //   Report Count (1)
  0x81, 0x02,   //   Input (Data, Var, Abs)
  0x09, 0x03,   //   Usage (Vendor Usage 3)
  0x75, 0x10,   //   Report Size (16)
  0x81, 0x02,   //   Input (Data, Var, Abs)
  0x09, 0x04,   //   Usage (Vendor Usage 4)
  0x75, 0x40,   //   Report Size (64)
  0x81, 0x02,   //   Input (Data, Var, Abs)

  0xC0,         // End Collection
};
// Size of Vendor specific RNP HID descriptor.
#define DESC_SIZE_HIDREPORT_RNP (sizeof(zid_hid_rnp_report_desc))

// Proprietary HID descriptor for RNP command and response I/O.
const uint8 usb_hid_desc_rnp[] = {
  DESC_SIZE_HID_W1HIDRD,        // bLength
  DESC_TYPE_HID,                // bDescriptorType
  0x11, 0x01,                   // bcdHID (HID v1.11)
  0x00,                         // bCountryCode
  0x01,                         // bNumDescriptors
  DESC_TYPE_HIDREPORT,          // bDescriptorType
  DESC_SIZE_HIDREPORT_RNP, 0x00 // wDescriptorLength
};

// Consumer Control report descriptor
const uint8 zid_hid_ce_report_desc[] = {
  0x05, 0x0C,   // Usage Pg (Consumer Devices)
  0x09, 0x01,   // Usage (Consumer Control)
  0xA1, 0x01,   // Collection (Application)
  0x09, 0x02,   //   Usage (Numeric Key Pad)
  0xA1, 0x02,   //   Collection (Logical)
  0x05, 0x09,   //     Usage Pg (Button)
  0x19, 0x01,   //     Usage Min (Button 1)
  0x29, 0x0A,   //     Usage Max (Button 10)
  0x15, 0x01,   //     Logical Min (1)
  0x25, 0x0A,   //     Logical Max (10)
  0x75, 0x04,   //     Report Size (4)
  0x95, 0x01,   //     Report Count (1)
  0x81, 0x00,   //     Input (Data, Ary, Abs)
  0xC0,         //   End Collection
  0x05, 0x0C,   //   Usage Pg (Consumer Devices)
  0x09, 0x86,   //   Usage (Channel)
  0x15, 0xFF,   //   Logical Min (-1)
  0x25, 0x01,   //   Logical Max (1)
  0x75, 0x02,   //   Report Size (2)
  0x95, 0x01,   //   Report Count (1)
  0x81, 0x46,   //   Input (Data, Var, Rel, Null)
  0x09, 0xE9,   //   Usage (Volume Up)
  0x09, 0xEA,   //   Usage (Volume Down)
  0x15, 0x00,   //   Logical Min (0)
  0x75, 0x01,   //   Report Size (1)
  0x95, 0x02,   //   Report Count (2)
  0x81, 0x02,   //   Input (Data, Var, Abs)
  0x09, 0xE2,   //   Usage (Mute)
  0x09, 0x30,   //   Usage (Power)
  0x09, 0x83,   //   Usage (Recall Last)
  0x09, 0x81,   //   Usage (Assign Selection)
  0x09, 0xB0,   //   Usage (Play)
  0x09, 0xB1,   //   Usage (Pause)
  0x09, 0xB2,   //   Usage (Record)
  0x09, 0xB3,   //   Usage (Fast Forward)
  0x09, 0xB4,   //   Usage (Rewind)
  0x09, 0xB5,   //   Usage (Scan Next)
  0x09, 0xB6,   //   Usage (Scan Prev)
  0x09, 0xB7,   //   Usage (Stop)
  0x15, 0x01,   //   Logical Min (1)
  0x25, 0x0C,   //   Logical Max (12)
  0x75, 0x04,   //   Report Size (4)
  0x95, 0x01,   //   Report Count (1)
  0x81, 0x00,   //   Input (Data, Ary, Abs)
  0x09, 0x80,   //   Usage (Selection)
  0xA1, 0x02,   //   Collection (Logical)
  0x05, 0x09,   //     Usage Pg (Button)
  0x19, 0x01,   //     Usage Min (Button 1)
  0x29, 0x03,   //     Usage Max (Button 3)
  0x15, 0x01,   //     Logical Min (1)
  0x25, 0x03,   //     Logical Max (3)
  0x75, 0x02,   //     Report Size (2)
  0x81, 0x00,   //     Input (Data, Ary, Abs)
  0xC0,         //   End Collection
  0x81, 0x03,   //   Input (Const, Var, Abs)

  0xC0          // End Collection
};
// Size of Vendor specific RNP HID descriptor.
#define DESC_SIZE_HIDREPORT_CE (sizeof(zid_hid_ce_report_desc))

// Proprietary HID descriptor for RNP command and response I/O.
const uint8 usb_hid_desc_ce[] = {
  DESC_SIZE_HID_W1HIDRD,        // bLength
  DESC_TYPE_HID,                // bDescriptorType
  0x11, 0x01,                   // bcdHID (HID v1.11)
  0x00,                         // bCountryCode
  0x01,                         // bNumDescriptors
  DESC_TYPE_HIDREPORT,          // bDescriptorType
  DESC_SIZE_HIDREPORT_CE, 0x00  // wDescriptorLength
};
#endif //ZID_USB_RNP && ZID_USB_CE

const uint8 zid_std_report_desc_mouse[] = {
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x02,  // Usage (Mouse)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x01,  // Report Id (1)
  0x09, 0x01,  //   Usage (Pointer)
  0xA1, 0x00,  //   Collection (Physical)
  0x05, 0x09,  //     Usage Page (Buttons)
  0x19, 0x01,  //     Usage Minimum (01) - Button 1
  0x29, 0x03,  //     Usage Maximum (03) - Button 3
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x03,  //     Report Count (3)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Button states
  0x75, 0x05,  //     Report Size (5)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x01,  //     Input (Constant) - Padding or Reserved bits
  0x05, 0x01,  //     Usage Page (Generic Desktop)
  0x09, 0x30,  //     Usage (X)
  0x09, 0x31,  //     Usage (Y)
  0x15, 0x81,  //     Logical Minimum (-127)
  0x25, 0x7F,  //     Logical Maximum (127)
  0x75, 0x08,  //     Report Size (8)
  0x95, 0x02,  //     Report Count (2)
  0x81, 0x06,  //     Input (Data, Variable, Relative) - X & Y coordinate
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default Mouse report only.
#define DESC_SIZE_HIDREPORT_MOUSE (sizeof(zid_std_report_desc_mouse))

const uint8 zid_std_report_desc_keyboard[] = {
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x06,  // Usage (Keyboard)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x02,  //   Report Id (2)
  0xA1, 0x00,  //   Collection (Physical)
  0x05, 0x07,  //     Usage (Key Codes)
  0x19, 0xE0,  //     Usage Minimum (224)
  0x29, 0xE7,  //     Usage Maximum (231)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x08,  //     Report Count (8)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Modifier byte
  0x75, 0x08,  //     Report Size (8)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x01,  //     Input (Constant) - Reserved byte
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x05,  //     Report Count (5)
  0x05, 0x08,  //     Usage Page (LEDs)
  0x19, 0x01,  //     Usage Minimum (1)
  0x29, 0x05,  //     Usage Maximum (5)
  0x91, 0x02,  //     Output (Data, Variable, Absolute) - LED status
  0x75, 0x03,  //     Report Size (3)
  0x95, 0x01,  //     Report Count (1)
  0x91, 0x01,  //     Output (Constant) - Padding or Reserved bits
  0x75, 0x08,  //     Report Size (8)
  0x95, 0x06,  //     Report Count (6)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x65,  //     Logical Maximum (101)
  0x05, 0x07,  //     Usage Page (Key Codes)
  0x19, 0x00,  //     Usage Minimum (0)
  0x29, 0x65,  //     Usage Maximum (101)
  0x81, 0x00,  //     Input (Data, Array) - Key arrays (6 bytes)
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default keyboard report only.
#define DESC_SIZE_HIDREPORT_KEYBOARD  (sizeof(zid_std_report_desc_keyboard))

const uint8 zid_std_report_desc_contact_data[] = {
  0x05, 0x0D,  // Usage page (Digitizers)
  0x09, 0x04,  // Usage (Touch screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x03,  //   Report Id (3)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical minimum (0)
  0x25, 0x0F,  //     Logical maximum (15)
  0x75, 0x04,  //     Report size (4)
  0x95, 0x02,  //     Report count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Contact index & type
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x03,  //     Logical Maximum (3)
  0x75, 0x02,  //     Report size (2)
  0x95, 0x01,  //     Report count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Contact state
  0x75, 0x06,  //     Report size (6)
  0x81, 0x01,  //     Input (constant) - Reserved bits
  0x09, 0x00,  //     Usage (Undefined)
  0x26, 0xFF, 0x00,// Logical Maximum (255)
  0x75, 0x08,  //     Report size (8)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Major axis orientation
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x7F,  //     Logical Maximum (127)
  0x81, 0x02,  //     Input (constant) - Pressure
  0x05, 0x01,  //     Usage page (Generic Desktop)
  0x09, 0x30,  //     Usage (X)
  0x09, 0x31,  //     Usage (Y)
  0x26, 0xFF, 0x0F,// Logical maximum (4095)
  0x75, 0x0C,  //     Report size (12)
  0x95, 0x02,  //     Report count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - X & Y
  0x09, 0x00,  //     Usage (Undefined)
  0x26, 0xFF, 0x1F,// Logical maximum (8191)
  0x75, 0x10,  //     Report size (16)
  0x95, 0x02,  //     Report count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Maj/min axis lengths
  0xC0,        //   End collection
  0xC0         // End collection
};
// Default ContactData report only.
#define DESC_SIZE_HIDREPORT_CONTACT_DATA  (sizeof(zid_std_report_desc_contact_data))

const uint8 zid_std_report_desc_gesture_tap[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x04,  // Report Id (4)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical minimum (0)
  0x25, 0x07,  //     Logical maximum (7)
  0x75, 0x03,  //     Report size (3)
  0x95, 0x01,  //     Report count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Finger count
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x1F,  //     Logical maximum (35)
  0x75, 0x05,  //     Report size (5)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Type
  0x75, 0x08,  //     Report size (8)
  0x81, 0x01,  //     Input (Constant) - Reserved byte
  0x05, 0x01,  //     Usage Page (Generic desktop)
  0x09, 0x30,  //     Usage (X)
  0x09, 0x31,  //     Usage (Y)
  0x26, 0xFF, 0x0F,// Logical maximum (4095)
  0x75, 0x0C,  //     Report size (12)
  0x95, 0x02,  //     Report count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - X & Y positions
  0xC0,        //   End collection
  0xC0         // End collection
};
// Default GestureTap report only.
#define DESC_SIZE_HIDREPORT_GESTURE_TAP (sizeof(zid_std_report_desc_gesture_tap))

const uint8 zid_std_report_desc_gesture_scroll[] = {
  0x05, 0x0D,  // Usage page (Digitizers)
  0x09, 0x04,  // Usage (Touch screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x05,  //   Report Id (5)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical minimum (0)
  0x25, 0x07,  //     Logical maximum (7)
  0x75, 0x03,  //     Report size (3)
  0x95, 0x01,  //     Report count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Finger count
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x1F,  //     Logical maximum (31)
  0x75, 0x05,  //     Report size (5)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Gesture type
  0x75, 0x08,  //     Report count (8)
  0x81, 0x01,  //     Input (constant) - Reserved byte
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x07,  //     Logical maximum (7)
  0x75, 0x03,  //     Report size (3)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Direction
  0x75, 0x01,  //     Report size (1)
  0x81, 0x01,  //     Input (constant) - Reserved bit
  0x09, 0x00,  //     Usage (Undefined)
  0x26, 0xFF, 0x0F,// Logical maximum (4095)
  0x75, 0x0C,  //     Report size (12)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Sroll distance
  0xC0,        //   End collection
  0xC0         // End collection
};
// Default GestureScroll report only.
#define DESC_SIZE_HIDREPORT_GESTURE_SCROLL  (sizeof(zid_std_report_desc_gesture_scroll))

//TSO
const uint8 zid_std_report_desc_gesture_pinch[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch Screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x06,  //   Report Id (6)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x09, 0x22,  //     Usage (Finger)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x02,  //     Report Count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Dir. & Finger
  0x75, 0x06,  //     Report Size (6)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x01,  //     Input (Constant)  - Reserved
  0x09, 0x00,  //     Usage (Undefined)
  0x26, 0xFF, 0x0F,// Logical Maximum (4095)
  0x75, 0x0C,  //     Report Size (12)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Distance
  0x75, 0x04,  //     Report Size (4)
  0x81, 0x01,  //     Input (Constant) - Reserved
  0x05, 0x01,  //     Usage Page (Generic Desktop)
  0x09, 0x30,  //     Usage (X)
  0x09, 0x31,  //     Usage (Y)
  0x75, 0x0C,  //     Report Size (12)
  0x95, 0x02,  //     Report Count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Center X & Y
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default GesturePinch report only.
#define DESC_SIZE_HIDREPORT_GESTURE_PINCH   (sizeof(zid_std_report_desc_gesture_pinch))

const uint8 zid_std_report_desc_gesture_rotation[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch Screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x07,  //   Report Id (7)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x09, 0x22,  //     Usage (Finger)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x02,  //     Report Count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Dir. & Finger
  0x75, 0x06,  //     Report Size (6)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x01,  //     Input (Constant)  - Reserved
  0x09, 0x00,  //     Usage (Undefined)
  0x26, 0xFF, 0x00,// Logical Maximum (255)
  0x75, 0x08,  //     Report Size (8)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Magnitude
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default GestureRotate report only.
#define DESC_SIZE_HIDREPORT_GESTURE_ROTATION    (sizeof(zid_std_report_desc_gesture_rotation))

const uint8 zid_std_report_desc_sync[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch Screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x08,  //   Report Id (8)
  0xA1, 0x01,  //   Collection (Application)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x0F,  //     Logical Maximum (15)
  0x75, 0x04,  //     Report Size (4)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Contact Count
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Gesture Present
  0x75, 0x03,  //     Report Size (3)
  0x81, 0x01,  //     Input (Constant)  - Reserved
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default Sync report only.
#define DESC_SIZE_HIDREPORT_SYNC    (sizeof(zid_std_report_desc_sync))

const uint8 zid_std_report_desc_touch_sensor_properties[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch Screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x09,  //   Report Id (9)
  0xA1, 0x02,  //   Collection (Logical)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x0F,  //     Logical Maximum (15)
  0x75, 0x04,  //     Report Size (4)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - # additional contacts
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x03,  //     Logical Maximum (3)
  0x75, 0x02,  //     Report Size (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Origin
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x02,  //     Report count (2)
  0x81, 0x02,  //     Input (Data, Variable, Absolute)  - Rel. index, gestures
  0x09, 0x00,  //     Usage (Undefined)
  0x25, 0x7F,  //     Logical Maximum (127)
  0x75, 0x08,  //     Report Size (8)
  0x81, 0x02,  //     Input (Data, Variable, Absolute)  - Resolution x & y
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x01,  //     Logical Minimum (1)
  0x26, 0xFF, 0x0F,// Logical Maximum (4095)
  0x75, 0x0C,  //     Report Size (12)
  0x81, 0x02,  //     Input (Data, Variable, Absolute)  - Max coordinate x & y
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x07,  //     Logical Maximum (7)
  0x75, 0x04,  //     Report Size (4)
  0x95, 0x01,  //     Report count (1)
  0x81, 0x02,  //     Input (Data, Variable, Absolute)  - Shape
  0x81, 0x01,  //     Input (Constant)  - Reserved bit
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default Touch Sensor Properties report only.
#define DESC_SIZE_HIDREPORT_TOUCH_SENSOR_PROPERTIES    (sizeof(zid_std_report_desc_touch_sensor_properties))

const uint8 zid_std_report_desc_tap_support_properties[] = {
  0x05, 0x0D,  // Usage Page (Digitizers)
  0x09, 0x04,  // Usage (Touch Screen)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x0A,  //   Report Id (10)
  0xA1, 0x00,  //   Collection (Physical)
  0x09, 0x00,  //     Usage (Undefined)
  0x15, 0x00,  //     Logical Minimum (0)
  0x25, 0x01,  //     Logical Maximum (1)
  0x75, 0x01,  //     Report Size (1)
  0x95, 0x04,  //     Report Count (4)
  0x81, 0x02,  //     Input (Data, Variable, Absolute) - Tap support indicators
  0x75, 0x04,  //     Report Size (4)
  0x95, 0x01,  //     Report Count (1)
  0x81, 0x01,  //     Input (Constant) - Reserved bits
  0x75, 0x08,  //     Report Size (8)
  0x95, 0x03,  //     Report Count (3)
  0x81, 0x01,  //     Input (Constant)  - Reserved bytes
  0xC0,        //   End Collection
  0xC0         // End Collection
};
// Default Touch Sensor Properties report only.
#define DESC_SIZE_HIDREPORT_TAP_SUPPORT_PROPERTIES    (sizeof(zid_std_report_desc_tap_support_properties))

const uint8 zidStdHIDReportSizeTable[] = {
  0,  // There is no report zero - MOUSE starts at one.
  DESC_SIZE_HIDREPORT_MOUSE,
  DESC_SIZE_HIDREPORT_KEYBOARD,
  DESC_SIZE_HIDREPORT_CONTACT_DATA,
  DESC_SIZE_HIDREPORT_GESTURE_TAP,
  DESC_SIZE_HIDREPORT_GESTURE_SCROLL,
  DESC_SIZE_HIDREPORT_GESTURE_PINCH,
  DESC_SIZE_HIDREPORT_GESTURE_ROTATION,
  DESC_SIZE_HIDREPORT_SYNC,
  DESC_SIZE_HIDREPORT_TOUCH_SENSOR_PROPERTIES,
  DESC_SIZE_HIDREPORT_TAP_SUPPORT_PROPERTIES
};

const uint8* zidStdHIDReportTable[] = {
  NULL,  // There is no report zero - MOUSE starts at one.
  zid_std_report_desc_mouse,
  zid_std_report_desc_keyboard,
  zid_std_report_desc_contact_data,
  zid_std_report_desc_gesture_tap,
  zid_std_report_desc_gesture_scroll,
  zid_std_report_desc_gesture_pinch,
  zid_std_report_desc_gesture_rotation,
  zid_std_report_desc_sync,
  zid_std_report_desc_touch_sensor_properties,
  zid_std_report_desc_tap_support_properties
};

USB_DESCRIPTOR_MARKER usbDescriptorMarker;

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

#define ZID_DEVICE_DESC               zidDesc
#define ZID_CONFIG_DESC               (ZID_DEVICE_DESC              +  DESC_SIZE_DEVICE)

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
#define ZID_RNP_IFACE_DESC            (ZID_CONFIG_DESC              +  DESC_SIZE_CONFIG)
#define ZID_RNP_HID_DESC              (ZID_RNP_IFACE_DESC           +  DESC_SIZE_INTERFACE)
#define ZID_RNP_IN_EP_DESC            (ZID_RNP_HID_DESC             +  DESC_SIZE_HID_W1HIDRD)
#define ZID_RNP_OUT_EP_DESC           (ZID_RNP_IN_EP_DESC           +  DESC_SIZE_ENDPOINT)

#define ZID_CE_IFACE_DESC             (ZID_RNP_OUT_EP_DESC          +  DESC_SIZE_ENDPOINT)
#define ZID_CE_HID_DESC               (ZID_CE_IFACE_DESC            +  DESC_SIZE_INTERFACE)
#define ZID_CE_IN_EP_DESC             (ZID_CE_HID_DESC              +  DESC_SIZE_HID_W1HIDRD)

#define ZID_IFACE_DESC                (ZID_CE_IN_EP_DESC            +  DESC_SIZE_ENDPOINT)
#else  // !ZID_USB_RNP
#define ZID_IFACE_DESC                (ZID_CONFIG_DESC              +  DESC_SIZE_CONFIG)
#endif // ZID_USB_RNP

#define ZID_HID_DESC                  (ZID_IFACE_DESC               +  DESC_SIZE_INTERFACE)
#define ZID_IN_EP_DESC                (ZID_HID_DESC                 +  DESC_SIZE_HID_W1HIDRD)
#define ZID_OUT_EP_DESC               (ZID_IN_EP_DESC               +  DESC_SIZE_ENDPOINT)

#define ZID_STRING_DESC_LANGID        (ZID_OUT_EP_DESC              +  DESC_SIZE_ENDPOINT)
#define ZID_STRING_DESC_MANUFACTURER  (ZID_STRING_DESC_LANGID       +  DESC_SIZE_STRING_LANGID)
#define ZID_STRING_DESC_PRODUCT       (ZID_STRING_DESC_MANUFACTURER +  DESC_SIZE_STRING_MANUFACTURER)
#define ZID_STRING_DESC_SERIALNO      (ZID_STRING_DESC_PRODUCT      +  DESC_SIZE_STRING_PRODUCT)

#define ZID_DEVICE_DESC_2EP_END       (ZID_STRING_DESC_SERIALNO     +  DESC_SIZE_STRING_SERIALNO)
#define ZID_DEVICE_DESC_1EP_END       (ZID_DEVICE_DESC_2EP_END      -  DESC_SIZE_ENDPOINT)

#define DESC_SIZE_ZID_ADD             (DESC_SIZE_INTERFACE + DESC_SIZE_HID_W1HIDRD + 2*DESC_SIZE_ENDPOINT)
#define ZID_DEVICE_DESC_NO_ZID_END    (ZID_DEVICE_DESC_2EP_END      -  DESC_SIZE_ZID_ADD)

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

static uint8 *zidDesc = NULL;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          usbOutReportPoll
 *
 * @brief       Poll output reports from the USB Host.
 *
 * input parameters
 *
 * uint8 endPoint
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void usbOutReportPoll(uint8 endPoint)
{
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  uint8 controlReg;
  uint16 bytesNow;
  uint8 oldEndpoint;
  uint8 *pBuf;
  uint8 readSize;   // Dependent on EP

  // Variables and buffers that should be controlled by application
  uint8 *pData;
  uint8 len = 0;
  switch (endPoint) {
  case RNP_EP_OUT_ADDR:
    len = RNP_EP_OUT_PACKET_LEN;
    break;
  case ZID_EP_OUT_ADDR:
    len = 2;
    break;
  }
  pData = osal_mem_alloc(len);

  // Save the old index setting, then select endpoint 0 and fetch the control register
  oldEndpoint = USBFW_GET_SELECTED_ENDPOINT();
  USBFW_SELECT_ENDPOINT(endPoint);

  // Read registers for interrupt reason
  controlReg = USBCSOL;

  // Check allocated size for this OUT EP. This is the maximum number of bytes
  // that can be read out of the FIFO.
  readSize = (USBMAXO << 3);

  // Receive OUT packets
  if (controlReg & USBCSOL_OUTPKT_RDY)
  {
    // Read FIFO
    bytesNow = (uint16) USBCNTL;
    bytesNow |= ((uint16) USBCNTH) << 8;

    if (bytesNow > len)
    {
      // Ignore invalid length report.
      while (bytesNow > len)
      {
        usbfwReadFifo(((&USBF0) + (endPoint << 1)), len, pData);
        bytesNow -= len;
      }
      if (bytesNow > 0)
      {
        usbfwReadFifo(((&USBF0) + (endPoint << 1)), bytesNow, pData);
      }
      // clear outpkt_ready flag and overrun, sent_stall, etc. just in case
      USBCSOL = 0;
    }
    else
    {
      pBuf = pData;
      while (bytesNow > readSize)
      {
        // since usbfwReadFifo function supports only upto readSize byte read
        // multiple calls are made in the loop
        usbfwReadFifo(((&USBF0) + (endPoint << 1)), readSize, pBuf);
        bytesNow -= readSize;
        pBuf += readSize;
      }
      if (bytesNow > 0)
      {
        // read the remainder
        usbfwReadFifo(((&USBF0) + (endPoint << 1)), bytesNow, pBuf);
      }
      // clear outpkt_ready flag and overrun, sent_stall, etc. just in case
      USBCSOL = 0;

      // application specific handling
      zidDongleOutputReport(endPoint, pData, len);
    }
  }
  // free memory
  osal_mem_free(pData);

  // Restore the old index setting
  USBFW_SELECT_ENDPOINT(oldEndpoint);
#else
  (void)endPoint;
#endif  //ZID_USB_RNP
}

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
uint8 zidUsbEnumerate(zid_proxy_entry_t *pPxy)
{
  // zidDesc points to a super buffer in order to have 1 alloc & test for failure; rep desc at end.
  #define STD_HID_REPORT_DESC_BUF  (zidDesc + wTotalLength + DESC_SIZE_DEVICE + DESC_SIZE_STRING)
  #define RNP_HID_REPORT_DESC_BUF  (STD_HID_REPORT_DESC_BUF + wStdHIDReportDescLength)
  #define  CE_HID_REPORT_DESC_BUF  (RNP_HID_REPORT_DESC_BUF + DESC_SIZE_HIDREPORT_RNP)

  uint16 wTotalLength = DESC_SIZE_CONFIG + DESC_SIZE_INTERFACE + DESC_SIZE_ENDPOINT;
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID
    wTotalLength += 2*DESC_SIZE_ENDPOINT + DESC_SIZE_INTERFACE;
  } else {
    wTotalLength += 2*DESC_SIZE_INTERFACE + 3*(DESC_SIZE_ENDPOINT);
  }
#endif //ZID_USB_RNP && ZID_USB_CE
  uint16 wStdHIDReportDescLength = 0;
  uint8 usb_hid_desc_zid[DESC_SIZE_HID_W1HIDRD];

  if (zidDesc != NULL)
  {
    return USB_ALREADY;
  }

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  uint8 hasKeyboard = FALSE;

  if (pPxy == NULL) {
    // enumerate as only RNP over HID
  } else {
#endif
  // If the number of endpoints are equal to 0x02 it means the optional OUT EP is active.
  // In that case, add this descriptor, and of course add to the wTotalLength
  if (pPxy->HIDNumEndpoints == ZID_SUPPORT_OPT_OUT_EP)
  {
    wTotalLength += DESC_SIZE_ENDPOINT; // Add size of optional EP
  }
  else
    if (pPxy->HIDNumEndpoints != ZID_SUPPORT_ONLY_IN_EP)  // Invalid number of EPs.
  {
    return USB_INVALID;
  }

  for (uint8 i = 0; i < pPxy->HIDNumStdDescComps; i++)
  {
    wStdHIDReportDescLength += zidStdHIDReportSizeTable[pPxy->HIDStdDescCompsList[i]];
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
    if (pPxy->HIDStdDescCompsList[i] == ZID_STD_REPORT_KEYBOARD)
      hasKeyboard = TRUE;
#endif //defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  }

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (hasKeyboard == FALSE)
    wStdHIDReportDescLength += zidStdHIDReportSizeTable[ZID_STD_REPORT_KEYBOARD];
#endif //defined (ZID_USB_RNP) && defined (ZID_USB_CE)

  // Add size of HID Descriptor, since this is now known.
  /* NOTE: This may seem stupid since the size is fixed.
   *       This is however because two standard reports are merged for one HID Report Descriptor.
   *       This again to support Windows standard HID driver. ZID spec allows for an increasing
   *       size of the HID Report Descriptor depending on the number of descriptors.
   */
  wTotalLength += DESC_SIZE_HID_W1HIDRD;

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  }
    // The "RNP" over USB defines one Vendor Specific HID Descriptor and a HID Consumer Control descriptor
  wTotalLength += 2*DESC_SIZE_HID_W1HIDRD;
#endif //defined ZID_USB_RNP

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID
    zidDesc = osal_mem_alloc(wTotalLength + DESC_SIZE_DEVICE + DESC_SIZE_HIDREPORT_RNP + DESC_SIZE_HIDREPORT_CE + DESC_SIZE_STRING);
  } else {
    zidDesc = osal_mem_alloc(wTotalLength + DESC_SIZE_DEVICE + wStdHIDReportDescLength + DESC_SIZE_HIDREPORT_RNP + DESC_SIZE_HIDREPORT_CE + DESC_SIZE_STRING);
  }
  if (zidDesc == NULL)
#else  // ZID_USB_RNP
  if ((zidDesc = osal_mem_alloc(wTotalLength + DESC_SIZE_DEVICE + wStdHIDReportDescLength + DESC_SIZE_STRING)) == NULL)
#endif // ZID_USB_RNP
  {
    return USB_MEMORY0;
  }

  uint8 *ptr = STD_HID_REPORT_DESC_BUF;
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID
  } else {
#endif // ZID_USB_RNP

    /*** HID Class Descriptor
    * Generic ZID standard HID Class Descriptor, populated on run-time from
    * ZID attributes in NV.
    * Must be generated first since the const version does not exist in flash,
    * as they do for the other descriptors. The reason is that size is a function
    * of the number of supported reports, which is conveyed during the configuration
    * phase.
    * According to ZID spec we should increase the size of this descriptor based
    * on the aplHIDNumNonStdDescComps + aplHIDNumStdDescComps. However, this does not
    * work for standard windows HID driver. Windows standard HID driver can handle
    * a single concatenated HID Report Descriptor. Thus, we implemented this to
    * support standard Windows HID driver.
    */

    // Size could be a function of the number of Report Descriptors supported
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_LENGTH] = DESC_SIZE_HID_W1HIDRD;
    // Type is for ZID always 0x21 - HID descriptor
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_DESCRIPTORTYPE] = DESC_TYPE_HID;

    usb_hid_desc_zid[ZID_DESC_HID_INDEX_BCDHID+0] = LO_UINT16(pPxy->HIDParserVersion);
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_BCDHID+1] = HI_UINT16(pPxy->HIDParserVersion);
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_COUNTRYCODE] = pPxy->HIDCountryCode;

    // Must be only 1 HID Report Descriptor per interface to be recognized by standard
    // Windows HID driver
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_NUMDESCRIPTORS] = 0x01;
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_DESCRIPTORTYPE_1] = DESC_TYPE_HIDREPORT;

    // Low byte first, since all the standard reports are less than 255 -> only low byte.
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_DESCRIPTORTYPE_1+1] = LO_UINT16(wStdHIDReportDescLength);
    usb_hid_desc_zid[ZID_DESC_HID_INDEX_DESCRIPTORTYPE_1+2] = HI_UINT16(wStdHIDReportDescLength);

//    uint8 *ptr = STD_HID_REPORT_DESC_BUF;
    uint8 i = 0;
    // If ZID_STD_REPORT_MOUSE is reported, this must come first in the concatenated report descriptor
    for (i = 0; i < pPxy->HIDNumStdDescComps; i++)
    {
      if (pPxy->HIDStdDescCompsList[i] == ZID_STD_REPORT_MOUSE) {
        ptr = osal_memcpy(ptr, zidStdHIDReportTable[pPxy->HIDStdDescCompsList[i]],
                          zidStdHIDReportSizeTable[pPxy->HIDStdDescCompsList[i]]);
        break;
      }
    }
    // Then all the others
    for (i = 0; i < pPxy->HIDNumStdDescComps; i++)
    {
      if (pPxy->HIDStdDescCompsList[i] != ZID_STD_REPORT_MOUSE) {
        ptr = osal_memcpy(ptr, zidStdHIDReportTable[pPxy->HIDStdDescCompsList[i]],
                          zidStdHIDReportSizeTable[pPxy->HIDStdDescCompsList[i]]);
      }
    }
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (hasKeyboard == FALSE)
    ptr = osal_memcpy(ptr, zidStdHIDReportTable[ZID_STD_REPORT_KEYBOARD],
                      zidStdHIDReportSizeTable[ZID_STD_REPORT_KEYBOARD]);
  }
  ptr = RNP_HID_REPORT_DESC_BUF;
  ptr = osal_memcpy(ptr, zid_hid_rnp_report_desc, sizeof(zid_hid_rnp_report_desc));
  ptr = osal_memcpy(ptr, zid_hid_ce_report_desc, sizeof(zid_hid_ce_report_desc));
#endif //(ZID_USB_RNP)

  ptr = zidDesc;
  ptr = osal_memcpy(ptr, usb_device_desc,       sizeof(usb_device_desc));
  ptr = osal_memcpy(ptr, usb_config_desc,       sizeof(usb_config_desc));
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  // The superstructure is not dependent on order, the FW will pick out the
  // requested descriptors when asked. It searches through the whole superstructure.
  // However, order is important for the subsequent filling in of the attributes
  // retrieved in the configuration phase! See macros defined further up in this file.
  ptr = osal_memcpy(ptr, usb_iface_desc_rnp,    sizeof(usb_iface_desc_rnp));
  ptr = osal_memcpy(ptr, usb_hid_desc_rnp,      sizeof(usb_hid_desc_rnp));
  ptr = osal_memcpy(ptr, int_in_ep_desc_rnp,    sizeof(int_in_ep_desc_rnp));
  ptr = osal_memcpy(ptr, int_out_ep_desc_rnp,   sizeof(int_out_ep_desc_rnp));

  ptr = osal_memcpy(ptr, usb_iface_desc_ce,     sizeof(usb_iface_desc_ce));
  ptr = osal_memcpy(ptr, usb_hid_desc_ce,       sizeof(usb_hid_desc_ce));
  ptr = osal_memcpy(ptr, int_in_ep_desc_ce,     sizeof(int_in_ep_desc_ce));

  if (pPxy == NULL) {
    // enumerate as only RNP over HID
  } else {
#endif // ZID_USB_RNP
    ptr = osal_memcpy(ptr, usb_iface_desc_zid,    sizeof(usb_iface_desc_zid));
    ptr = osal_memcpy(ptr, usb_hid_desc_zid,      DESC_SIZE_HID_W1HIDRD);
    ptr = osal_memcpy(ptr, int_in_ep_desc_zid,    sizeof(int_in_ep_desc_zid));
    if (pPxy->HIDNumEndpoints == ZID_SUPPORT_OPT_OUT_EP)  // Add optional OUT EP Descriptor.
    {
      ptr = osal_memcpy(ptr, int_out_ep_desc_zid, sizeof(int_out_ep_desc_zid));
    }
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  }
#endif // ZID_USB_RNP
  // Strings must come last, because when it gives the full Configuration Descriptor
  // it utilizes the fact that they actually come packed in a superstructer. It
  // only finds the pointer to the Configuration Descriptor and reports back the wTotalLength
  ptr = osal_memcpy(ptr, string0LangId,         sizeof(string0LangId));
  ptr = osal_memcpy(ptr, string1Manufacterer,   sizeof(string1Manufacterer));
  ptr = osal_memcpy(ptr, string2Product,        sizeof(string2Product));
  (void)osal_memcpy(ptr, string3SerialNo,       sizeof(string3SerialNo));

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID
  } else {
#endif // ZID_USB_RNP
    /** For all descriptors, change fields according to attributes set in the
    *  configuration phase.
    */

    /** Device Descriptor
    */
    ptr = ZID_DEVICE_DESC + 8;
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
    // If ZID_USB_RNP is defined we want to keep the same driver hooked, i.e. we don't
    // want to change Vendor and Product ID.
    ptr += 4;
#else //defined (ZID_USB_RNP) && defined (ZID_USB_CE)
    ptr = osal_memcpy(ptr, (uint8 *)&pPxy->HIDVendorId, 2);
    ptr = osal_memcpy(ptr, (uint8 *)&pPxy->HIDProductId, 2);
#endif //defined (ZID_USB_RNP) && defined (ZID_USB_CE)
    ptr = osal_memcpy(ptr, (uint8 *)&pPxy->HIDDeviceReleaseNumber, 2);

    /** Interface Descriptor
    */
    ptr = ZID_IFACE_DESC + 4;
    *ptr = pPxy->HIDNumEndpoints;
    ptr = ZID_IFACE_DESC + 6;
    *ptr = pPxy->HIDDeviceSubclass;
    ptr = ZID_IFACE_DESC + 7;
    *ptr = pPxy->HIDProtocolCode;

    /** Endpoint IN Descriptor
    */
    ptr = ZID_IN_EP_DESC + 6;
    *ptr = pPxy->HIDPollInterval;

    /** Endpoint OUT Descriptor (optional)
    */
    if (pPxy->HIDNumEndpoints == ZID_SUPPORT_OPT_OUT_EP)
    {
      ptr = ZID_OUT_EP_DESC + 6;
      *ptr = pPxy->HIDPollInterval;
    }

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  }
#endif // ZID_USB_RNP

  /*** Configuration Descriptor
   * wTotalLength field of Configuration Descriptor is the total length in
   * bytes of data returned for this configuration. This is equal to the combined
   * lengths of the Configuration, Interface, HID and Endpoint descriptors.
   */
  ptr = ZID_CONFIG_DESC + 2; // Index to the wTotalLength, in usb_config_desc.
  *ptr++ = LO_UINT16(wTotalLength);
  *ptr++ = HI_UINT16(wTotalLength);

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID, i.e. only two interface
    *ptr = 2; // bNumInterfaces
  }
#endif // ZID_USB_RNP
  usbDescInit(ZID_DEVICE_DESC);

  // Serial Number - left erased for programming of the IEEE.
  if (usbDescriptorMarker.pUsbDescLut != NULL)
  {
    (void)osal_mem_free(usbDescriptorMarker.pUsbDescLut);
  }
  usbDescriptorMarker.pUsbDescLut = (DESC_LUT_INFO *)osal_mem_alloc(DESC_LUT_INFO_SIZE);
  HAL_ASSERT(usbDescriptorMarker.pUsbDescLut != NULL);
  usbDescriptorMarker.pUsbDescLutEnd = usbDescriptorMarker.pUsbDescLut;
  usbDescriptorMarker.pUsbDescLutEnd += (uint16)(DESC_LUT_INFO_SIZE / (sizeof(DESC_LUT_INFO)));

  usbDescriptorMarker.pUsbDescStart = ZID_DEVICE_DESC;
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  if (pPxy == NULL) {
    // enumerate as only RNP over HID
    usbDescriptorMarker.pUsbDescEnd = ZID_DEVICE_DESC_NO_ZID_END;
  } else {
#endif // ZID_USB_RNP
    if (pPxy->HIDNumEndpoints == ZID_SUPPORT_ONLY_IN_EP)
    {
      usbDescriptorMarker.pUsbDescEnd = ZID_DEVICE_DESC_1EP_END;
    }
    else
    {
      usbDescriptorMarker.pUsbDescEnd = ZID_DEVICE_DESC_2EP_END;
    }
#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  }
#endif // ZID_USB_RNP

  if (usbDescriptorMarker.pUsbDblbufLut != NULL)
  {
    (void)osal_mem_free(usbDescriptorMarker.pUsbDblbufLut);
  }
  usbDescriptorMarker.pUsbDblbufLut = (DBLBUF_LUT_INFO *)osal_mem_alloc(DBLBUF_LUT_INFO_SIZE);
  HAL_ASSERT(usbDescriptorMarker.pUsbDblbufLut != NULL);
  usbDescriptorMarker.pUsbDblbufLutEnd = usbDescriptorMarker.pUsbDblbufLut;
  usbDescriptorMarker.pUsbDblbufLutEnd += (uint16)(DBLBUF_LUT_INFO_SIZE / sizeof(DBLBUF_LUT_INFO));

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)

  usbDescriptorMarker.pUsbDescLut[0].valueMsb = DESC_TYPE_HID;
  usbDescriptorMarker.pUsbDescLut[0].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[0].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[0].indexLsb = INTERFACE_NUMBER_RNP; // Interface number
  usbDescriptorMarker.pUsbDescLut[0].pDescStart = ZID_RNP_HID_DESC;
  usbDescriptorMarker.pUsbDescLut[0].length = sizeof(usb_hid_desc_rnp);
  
  usbDescriptorMarker.pUsbDescLut[1].valueMsb = DESC_TYPE_HIDREPORT;
  usbDescriptorMarker.pUsbDescLut[1].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[1].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[1].indexLsb = INTERFACE_NUMBER_RNP; // Interface number
  usbDescriptorMarker.pUsbDescLut[1].pDescStart = RNP_HID_REPORT_DESC_BUF;
  usbDescriptorMarker.pUsbDescLut[1].length = DESC_SIZE_HIDREPORT_RNP;

  usbDescriptorMarker.pUsbDescLut[2].valueMsb = DESC_TYPE_HID;
  usbDescriptorMarker.pUsbDescLut[2].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[2].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[2].indexLsb = INTERFACE_NUMBER_CE; // Interface number
  usbDescriptorMarker.pUsbDescLut[2].pDescStart = ZID_CE_HID_DESC;
  usbDescriptorMarker.pUsbDescLut[2].length = sizeof(usb_hid_desc_ce);
  
  usbDescriptorMarker.pUsbDescLut[3].valueMsb = DESC_TYPE_HIDREPORT;
  usbDescriptorMarker.pUsbDescLut[3].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[3].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[3].indexLsb = INTERFACE_NUMBER_CE; // Interface number
  usbDescriptorMarker.pUsbDescLut[3].pDescStart = CE_HID_REPORT_DESC_BUF;
  usbDescriptorMarker.pUsbDescLut[3].length = DESC_SIZE_HIDREPORT_CE;
  
  usbDescriptorMarker.pUsbDescLut[4].valueMsb = DESC_TYPE_HID;
  usbDescriptorMarker.pUsbDescLut[4].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[4].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[4].indexLsb = INTERFACE_NUMBER_ZID; // Interface number
  usbDescriptorMarker.pUsbDescLut[4].pDescStart = ZID_HID_DESC;
  usbDescriptorMarker.pUsbDescLut[4].length = DESC_SIZE_HID_W1HIDRD;
  
  usbDescriptorMarker.pUsbDescLut[5].valueMsb = DESC_TYPE_HIDREPORT;
  usbDescriptorMarker.pUsbDescLut[5].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[5].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[5].indexLsb = INTERFACE_NUMBER_ZID; // Interface number
  usbDescriptorMarker.pUsbDescLut[5].pDescStart = STD_HID_REPORT_DESC_BUF;
  usbDescriptorMarker.pUsbDescLut[5].length = wStdHIDReportDescLength;
  
#else
  
  usbDescriptorMarker.pUsbDescLut[0].valueMsb = DESC_TYPE_HIDREPORT;
  usbDescriptorMarker.pUsbDescLut[0].valueLsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[0].indexMsb = 0x00;
  usbDescriptorMarker.pUsbDescLut[0].indexLsb = INTERFACE_NUMBER_ZID; // Interface number
  usbDescriptorMarker.pUsbDescLut[0].pDescStart = STD_HID_REPORT_DESC_BUF;
  usbDescriptorMarker.pUsbDescLut[0].length = wStdHIDReportDescLength;

#endif //ZID_USB_RNP && ZID_USB_CE

#if defined (ZID_USB_RNP) && defined (ZID_USB_CE)
  usbDescriptorMarker.pUsbDblbufLut[0].pInterface = (USB_INTERFACE_DESCRIPTOR *)ZID_RNP_IFACE_DESC;
  usbDescriptorMarker.pUsbDblbufLut[0].inMask = 0;
  usbDescriptorMarker.pUsbDblbufLut[0].outMask = 0;

  usbDescriptorMarker.pUsbDblbufLut[1].pInterface = (USB_INTERFACE_DESCRIPTOR *)ZID_CE_IFACE_DESC;
  usbDescriptorMarker.pUsbDblbufLut[1].inMask = 0;
  usbDescriptorMarker.pUsbDblbufLut[1].outMask = 0;
  
  usbDescriptorMarker.pUsbDblbufLut[2].pInterface = (USB_INTERFACE_DESCRIPTOR *)ZID_IFACE_DESC;
  usbDescriptorMarker.pUsbDblbufLut[2].inMask = 0x10; // Set EP5 to double buffering on IN
  usbDescriptorMarker.pUsbDblbufLut[2].outMask = 0;
#else
  
  usbDescriptorMarker.pUsbDblbufLut[0].pInterface = (USB_INTERFACE_DESCRIPTOR *)ZID_IFACE_DESC;
  usbDescriptorMarker.pUsbDblbufLut[0].inMask = 0x10; // Set EP5 to double buffering on IN
  usbDescriptorMarker.pUsbDblbufLut[0].outMask = 0;
  
#endif //ZID_USB_RNP && ZID_USB_CE

  usbHidInit();

  return USB_SUCCESS;
}

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
void zidUsbUnEnum(void)
{
  if (zidDesc != NULL)
  {
    HAL_USB_PULLUP_DISABLE();
    HAL_USB_INT_DISABLE();
    HAL_USB_DISABLE;
    osal_mem_free(usbDescriptorMarker.pUsbDescLut);
    usbDescriptorMarker.pUsbDescLut = NULL;
    osal_mem_free(usbDescriptorMarker.pUsbDblbufLut);
    usbDescriptorMarker.pUsbDblbufLut = NULL;

    // This logic should stay here:
    osal_mem_free(zidDesc);
    zidDesc = NULL;
  }
}

/**************************************************************************************************
*/
