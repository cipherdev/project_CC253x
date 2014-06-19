/**************************************************************************************************
  Filename:       zid_profile.h
**************************************************************************************************/
#ifndef ZID_PROFILE_H
#define ZID_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "gdp_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

// Version 0.9, the r15 as of Jone 16, 2011
#define ZID_PROFILE_VERSION                0x0100

#define ZID_FRAME_CTL_IDX                  0
#define ZID_DATA_BUF_IDX                   1

// ZID  Command Code field values (Table 2).
#define ZID_CMD_GET_REPORT                 0x01
#define ZID_CMD_REPORT_DATA                0x02
#define ZID_CMD_SET_REPORT                 0x03

// ZID response code field (table 3)
#define ZID_GENERIC_RSP_INVALID_REPORT_ID  0x40
#define ZID_GENERIC_RSP_MISSING_FRAGMENT   0x41

// ZID report type field (table 4)
#define ZID_REPORT_TYPE_IN                 0x01
#define ZID_REPORT_TYPE_OUT                0x02
#define ZID_REPORT_TYPE_FEATURE            0x03

// aplHIDDeviceSubclass attribute values (table 7)
#define ZID_DEVICE_SUBCLASS_NONE           0x00
#define ZID_DEVICE_SUBCLASS_BOOT           0x01

// aplHIDProtocolCode attribute values (table 8)
#define ZID_PROTOCOL_CODE_NONE             0x00
#define ZID_PROTOCOL_CODE_KEYBOARD         0x01
#define ZID_PROTOCOL_CODE_MOUSE            0x02

// aplHIDNumEndpoints min/max values (section 4.2.12)
#define ZID_NUM_ENDPOINTS_MIN              0x01
#define ZID_NUM_ENDPOINTS_MAX              0x02

// aplHIDPollInterval min/max values (section 4.2.13)
#define ZID_POLL_INTERVAL_MIN              1
#define ZID_POLL_INTERVAL_MAX              16

// aplCurrentProtocol attribute values (table 13)
#define ZID_PROTO_BOOT                     0x00
#define ZID_PROTO_REPORT                   0x01

// HID descriptor type (table 9)
#define ZID_DESC_TYPE_REPORT               0x22
#define ZID_DESC_TYPE_PHYSICAL             0x23

// Standard RF4CE ZID profile report IDs (table 30)
#define ZID_STD_REPORT_NONE                    0x00
#define ZID_STD_REPORT_MOUSE                   0x01
#define ZID_STD_REPORT_KEYBOARD                0x02
#define ZID_STD_REPORT_CONTACT_DATA            0x03
#define ZID_STD_REPORT_GESTURE_TAP             0x04
#define ZID_STD_REPORT_GESTURE_SCROLL          0x05
#define ZID_STD_REPORT_GESTURE_PINCH           0x06
#define ZID_STD_REPORT_GESTURE_ROTATE          0x07
#define ZID_STD_REPORT_GESTURE_SYNC            0x08
#define ZID_STD_REPORT_TOUCH_SENSOR_PROPERTIES 0x09
#define ZID_STD_REPORT_TAP_SUPPORT_PROPERTIES  0x0A
#define ZID_STD_REPORT_TOTAL_NUM               (ZID_STD_REPORT_TAP_SUPPORT_PROPERTIES)

// The poll interval is calculated as (2 ^ (aplHIDPollInterval-1)).
#define ZID_HID_POLL_INTERVAL_MIN          1
#define ZID_HID_POLL_INTERVAL_MAX          16

// Definitions for the 3 Communication Pipes (Figure 21).
#define ZID_COMM_PIPE_CTRL                 0x0
#define ZID_COMM_PIPE_INT_IN               0x1
#define ZID_COMM_PIPE_INT_OUT              0x2

/* ------------------------------------------------------------------------------------------------
 *                                     ZID Profile Types
 * ------------------------------------------------------------------------------------------------
 */

#define ZID_DATATYPE_UINT8_LEN             1
#define ZID_DATATYPE_UINT16_LEN            2
#define ZID_DATATYPE_UINT24_LEN            3

/* ------------------------------------------------------------------------------------------------
 *                                     ZID Profile Constants
 * ------------------------------------------------------------------------------------------------
 */

// Length of time a HID adaptor will repeat a report from a remote HID class device when its idle
// rate is non-zero, before a direct report command frame must be received from the remote HID
// class device.
#define aplcIdleRateGuardTime              1500  // Time in msec

// Max time a HID class device shall take to send a push attributes command during its cfg state.
#define aplcMaxConfigPrepTime              200   // Time in msec.

// Max time the Adapter shall wait to receive a push attributes command during its cfg state.
#define aplcMaxConfigWaitTime              300   // Time in msec.

// Max number of non-standard descriptors that can be supported on a node.
#define aplcMaxNonStdDescCompsPerHID       4

// Max size of a single non-standard descriptor component. The HID Adaptor shall be able to store
// descriptor components of this size for each component stored on a device.
#define aplcMaxNonStdDescCompSize          256

// Max size of a non-standard descriptor fragment that will fit in an over-the-air frame.
#define aplcMaxNonStdDescFragmentSize      80    // bytes

// Max size of a NULL report. The HID adaptor shall be able to store NULL reports
// of this size for each component stored on a device.
#define aplcMaxNullReportSize              16    // bytes

// The maximum time between consecutive report data transmissions.
#define aplcMaxReportRepeatInterval        100   // msec

// Max number of standard descriptors that can be supported on a device.
#define aplcMaxStdDescCompsPerHID          12

// Min window time permitted when using the Interrupt Pipe before a safe transmission is required.
#define aplcMinIntPipeUnsafeTxWindowTime   50    // Time in msec.

/* ------------------------------------------------------------------------------------------------
 *                                     ZID Profile Attributes
 * ------------------------------------------------------------------------------------------------
 */

// Reserved                           0x00-0x81
// GDP attributes                     0x82-0x83
// Reserved for GDP                   0x84-0x9F
#define aplZIDProfileVersion               0xA0
#define aplIntPipeUnsafeTxWindowTime       0xA1
#define aplReportRepeatInterval            0xA2
// Reserved                           0xA3-0xDF
#define aplHIDParserVersion                0xE0
#define aplHIDDeviceSubclass               0xE1
#define aplHIDProtocolCode                 0xE2
#define aplHIDCountryCode                  0xE3
#define aplHIDDeviceReleaseNumber          0xE4
#define aplHIDVendorId                     0xE5
#define aplHIDProductId                    0xE6
#define aplHIDNumEndpoints                 0xE7
#define aplHIDPollInterval                 0xE8
#define aplHIDNumStdDescComps              0xE9
#define aplHIDStdDescCompsList             0xEA
#define aplHIDNumNullReports               0xEB
// Reserved                           0xEC-0xEF
#define aplHIDNumNonStdDescComps           0xF0
#define aplHIDNonStdDescCompSpec1          0xF1
#define aplHIDNonStdDescCompSpec2          0xF2
#define aplHIDNonStdDescCompSpec3          0xF3
#define aplHIDNonStdDescCompSpec4          0xF4

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */
typedef struct {
  uint8  type;
  uint8  size_l;
  uint8  size_h;
  uint8  reportId;
  uint8  data[];
} zid_non_std_desc_comp_t;

typedef struct {
  uint8  type;
  uint8  size_l;
  uint8  size_h;
  uint8  reportId;
  uint8  fragId;
  uint8  data[];
} zid_frag_non_std_desc_comp_t;

#define ZID_NON_STD_DESC_SPEC_T_MAX       (sizeof(zid_non_std_desc_comp_t) + aplcMaxNonStdDescCompSize)
#define ZID_NON_STD_DESC_SPEC_FRAG_T_MAX  (sizeof(zid_frag_non_std_desc_comp_t) + aplcMaxNonStdDescFragmentSize)

typedef struct
{
  uint8  reportId;
  uint8  len;
  uint8  data[];
} zid_null_report_t;
#define ZID_NULL_REPORT_T_MAX       (sizeof(zid_null_report_t) + aplcMaxNullReportSize)

// A packed structure that constitutes a Get Report command frame
typedef struct
{
  uint8 cmd;     // ZID header command.
  uint8 type;    // Report type.
  uint8 id;      // Report Id.
} zid_get_report_cmd_t;

// A packed structure that constitutes a Set Report command frame
typedef struct
{
  uint8 cmd;     // ZID header command.
  uint8 type;    // Report type.
  uint8 id;      // Report Id.
  uint8 data[];  // Report data.
} zid_set_report_cmd_t;
#define ZID_SET_REPORT_SIZE_HDR             (sizeof(zid_set_report_cmd_t))

// A packed structure that constitutes a Report data command frame.
typedef struct
{
  uint8 len;     // Report size.
  uint8 type;    // Report type.
  uint8 id;      // Report Id.
  uint8 data[];  // Report data.
} zid_report_record_t;

typedef struct
{
  uint8 cmd;     // ZID header command.
  zid_report_record_t reportRecordList[];
} zid_report_data_cmd_t;
#define ZID_REPORT_CMD_ID_IDX              2

// A packed structure that constitutes the data of the ZID_STD_REPORT_MOUSE report.
typedef struct {
  uint8 btns;  // Bits 0-2 : Left/Right/Center click; bits 3-7 : pad.
  uint8 x;     // -127 to +127 relative X movement.
  uint8 y;     // -127 to +127 relative Y movement.
} zid_mouse_data_t;
#define ZID_MOUSE_DATA_LEN                (sizeof(zid_mouse_data_t))
#define ZID_MOUSE_DATA_MAX                 127

// A packed structure that constitutes the data of the ZID_STD_REPORT_KEYBOARD report IN.
typedef struct {
  uint8 mods;     // Bits 0-4 : modifier bits; bits 5-7 : LEDs.
  uint8 reserved;
  uint8 keys[6];  // Key arrays.
} zid_keyboard_data_t;
#define ZID_KEYBOARD_DATA_LEN             (sizeof(zid_keyboard_data_t))
#define ZID_KEYBOARD_MOD_MASK              0x1F
#define ZID_KEYBOARD_LED_MASK              0xE0

// A packed structure that constitutes the data of the ZID_STD_REPORT_KEYBOARD report OUT.
typedef struct {
  uint8 numLock:1;      // Num Lock
  uint8 capsLock:1;     // Caps Lock
  uint8 scrollLock:1;   // Scroll Lock
  uint8 compose:1;      // Compose
  uint8 kana:1;         // Kana
  uint8 reserved:3;     // Reserved
} zid_keyboard_data_out_t;
#define ZID_KEYBOARD_DATA_OUT_LEN          (sizeof(zid_keyboard_data_out_t))
#define ZID_KEYBOARD_LED_OUT_MASK           0x1F

// A packed structure that constitutes the data of the ZID_STD_REPORT_CONTACT_DATA report.
typedef struct {
  uint8 type_index;       // Bits 0-3 : Contact index; bits 4-7 : Contact type.
  uint8 state;            // Bits 0-1 : Contact state
  uint8 maj_ax_orientation;           // Major axis orientation
  uint8 pressure;         // Pressure
  uint8 loc_x;            // Location_x[7:0]
  uint8 loc_xy;           // Bits 0-3 : Location_x[8:11]; Bits 4-7 : Location_y[0:3]
  uint8 loc_y;            // Location_y[4:11]
  uint8 maj_ax_len_lsb;   // Major axis length[0:7]
  uint8 maj_ax_len_msb;   // Major axis length[15:8]
  uint8 min_ax_len_lsb;   // Minor axis length[0:7]
  uint8 min_ax_len_msb;   // Minor axis length[15:8]
} zid_contactData_data_t;
#define ZID_CONTACT_DATA_DATA_LEN             (sizeof(zid_contactData_data_t))
#define ZID_CONTACT_DATA_INDEX_MASK            0x0F
#define ZID_CONTACT_DATA_TYPE_MASK             0xF0
#define ZID_CONTACT_DATA_STATE_MASK            0x03
/* The following 4 states can be OR'ed into the state field, according to Table 21 of the ZID spec*/
#define ZID_CONTACT_DATA_STATE_NO_CONTACT      0x00
#define ZID_CONTACT_DATA_STATE_ARRIVING        0x01
#define ZID_CONTACT_DATA_STATE_ACCURATE        0x02
#define ZID_CONTACT_DATA_STATE_INACCURATE      0x03
/* The following 2 types can be OR'ed into the type_index field, according to Table 22 of the ZID spec*/
#define ZID_CONTACT_DATA_TYPE_FINGER           0x00
#define ZID_CONTACT_DATA_TYPE_PEN              0x10

// A packed structure that constitutes the data of the ZID_STD_REPORT_GESTURE_TAP report.
typedef struct {
  uint8 type_count;     // Bits 0-2: Finger count; Bits 3-7: Type of gesture detected
  uint8 reserved;       // Reserved
  uint8 loc_x;          // Location_x[7:0] (Low byte of 12bit gesture x-position value.)
  uint8 loc_xy;         // Bits 0-3: Location_x[8:11]; Bits 4-7: Location_y[0:3]
  uint8 loc_y;          // Location_y[4:11]
} zid_gestureTap_data_t;
#define ZID_GESTURE_TAP_DATA_LEN                (sizeof(zid_gestureTap_data_t))
#define ZID_GESTURE_TAP_TYPE_MASK               0xF8
#define ZID_GESTURE_TAP_FINGER_COUNT_MASK       0x07
// The following four states can be OR'ed into the type_count field,
// according to Table 23 of the ZID spec.
#define ZID_GESTURE_TAP_TYPE_TAP                0x00
#define ZID_GESTURE_TAP_TYPE_TAP_AND_A_HALF     0x08
#define ZID_GESTURE_TAP_TYPE_DOUBLE_TAP         0x10
#define ZID_GESTURE_TAP_TYPE_LONG_TAP           0x18
#define ZID_GESTURE_TAP_GET_LOC_X(g)            ( ((uint16)(g->loc_xy & 0x0F) << 8) | g->loc_x )      // Not tested
#define ZID_GESTURE_TAP_SET_LOC_X(g,val)        st( g->loc_xy = (HI_UINT16(val) & 0x0F) | (g->loc_xy & 0xF0); \
                                                    g->loc_x = LO_UINT16(val); \
                                                  ) // Not tested
#define ZID_GESTURE_TAP_GET_LOC_Y(g)            ( (g->loc_xy >> 4) | ((uint16)(g->loc_y) << 4) )      // Not tested
#define ZID_GESTURE_TAP_SET_LOC_Y(g,val)        st( g->loc_xy = (g->loc_xy & 0x0F) | (LO_UINT16(val) & 0xF0); \
                                                    g->loc_y = HI_UINT16(val); \
                                                  ) // Not tested

// A packed structure that constitutes the data of the ZID_STD_REPORT_GESTURE_SCROLL report.
typedef struct {
  uint8 type_count;         // Bits 0-2: Finger count; Bits 3-7: Type of gesture detected
  uint8 reserved;           // Reserved
  uint8 dist_dir;           // Bits 0-2: Direction; Bits 4-7: Distance[0:3]
  uint8 distance;           // Distance[4:11]
} zid_gestureScroll_data_t;
#define ZID_GESTURE_SCROLL_DATA_LEN                 (sizeof(zid_gestureScroll_data_t))
#define ZID_GESTURE_SCROLL_TYPE_MASK                0xF8
#define ZID_GESTURE_SCROLL_FINGER_COUNT_MASK        0x07
#define ZID_GESTURE_SCROLL_DIRECTION_MASK           0x07
/* The following three states can be OR'ed into the type_count field, according to Table 24 of the ZID spec*/
#define ZID_GESTURE_SCROLL_TYPE_FLICK               0x00
#define ZID_GESTURE_SCROLL_TYPE_LIN_SCROLL          0x08
#define ZID_GESTURE_SCROLL_TYPE_CIRC_SCROLL         0x10
/* The following 8 states are for flick direction scroll and can be OR'ed into the dist_dir field, according to Table 25 of the ZID spec*/
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_N         0x00
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_NE        0x01    // Optional, included for completeness
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_E         0x02
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_SE        0x03    // Optional, included for completeness
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_S         0x04
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_SW        0x05    // Optional, included for completeness
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_W         0x06
#define ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_NW        0x07    // Optional, included for completeness
/* The following 4 states are for linear scroll and can be OR'ed into the dist_dir field, according to Table 26 of the ZID spec*/
#define ZID_GESTURE_SCROLL_TYPE_LIN_SCROLL_DIR_N    ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_N
#define ZID_GESTURE_SCROLL_TYPE_LIN_SCROLL_DIR_E    ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_E
#define ZID_GESTURE_SCROLL_TYPE_LIN_SCROLL_DIR_S    ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_S
#define ZID_GESTURE_SCROLL_TYPE_LIN_SCROLL_DIR_W    ZID_GESTURE_SCROLL_TYPE_FLICK_DIR_W
/* The following 2 states are for circular scroll and can be OR'ed into the dist_dir field, according to Table 27 of the ZID spec*/
#define ZID_GESTURE_SCROLL_TYPE_CIRC_SCROLL_DIR_CW  0x00
#define ZID_GESTURE_SCROLL_TYPE_CIRC_SCROLL_DIR_CCW 0x01
#define ZID_GESTURE_SCROLL_GET_DISTANCE(g)          ( (g->dist_dir >> 4) | ((uint16)(g->distance) << 4) ) // Not tested

// A packed structure that constitutes the data of the ZID_STD_REPORT_GESTURE_PINCH report.
typedef struct {
  uint8 finger_dir;     // Bit 0: Direction; Bit 1: Finger present; Bits 2-7: Reserved
  uint8 dist_low;       // Distance[0:7]
  uint8 dist_high;      // Bits 0-3: Distance[8:11]; Bits: 4-7: Reserved
  uint8 center_x;       // Center_x[0:7]
  uint8 center_xy;      // Bits 0-3: Center_x[8:11]; Bits- 4-7: Center_y[0:3]
  uint8 center_y;       // Center_y[4:11]
} zid_gesturePinch_data_t;
#define ZID_GESTURE_PINCH_DATA_LEN              (sizeof(zid_gesturePinch_data_t))
#define ZID_GESTURE_PINCH_FINGER_PRESENT_MASK   0x02
#define ZID_GESTURE_PINCH_DIR_MASK              0x01
/* The following 2 states are for pinch direction and can be OR'ed into the finger_dir field, according to Table 28 of the ZID spec*/
#define ZID_GESTURE_PINCH_DIR_TOGETHER          0x00
#define ZID_GESTURE_PINCH_DIR_APART             0x01
#define ZID_GESTURE_PINCH_GET_DISTANCE(g)       ( ((uint16)(g->dist_high & 0x0F) << 8) | g->dist_low )      // Not tested
#define ZID_GESTURE_PINCH_SET_DISTANCE(g,val)  st( g->dist_high = (HI_UINT16(val) & 0x0F) | (g->dist_high & 0xF0); \
                                                    g->dist_low = LO_UINT16(val); \
                                                 ) // Not tested
#define ZID_GESTURE_PINCH_GET_CENTER_X(g)       ( ((uint16)(g->center_xy & 0x0F) << 8) | g->center_x )      // Not tested
#define ZID_GESTURE_PINCH_SET_CENTER_X(g,val)  st( g->center_xy = (HI_UINT16(val) & 0x0F) | (g->center_xy & 0xF0); \
                                                    g->center_x = LO_UINT16(val); \
                                                 ) // Not tested
#define ZID_GESTURE_PINCH_GET_CENTER_Y(g)      ( (g->center_xy >> 4) | ((uint16)(g->center_y) << 4) )      // Not tested
#define ZID_GESTURE_PINCH_SET_CENTER_Y(g,val)  st( g->center_xy = (g->center_xy & 0x0F) | (LO_UINT16(val) & 0xF0); \
                                                   g->center_y = HI_UINT16(val); \
                                                 ) // Not tested

// A packed structure that constitutes the data of the ZID_STD_REPORT_GESTURE_ROTATE report.
typedef struct {
  uint8 finger_dir;     // Bit 0: Direction; Bit 1: Finger present; Bit 2-7: Reserved
  uint8 magnitude;
} zid_gestureRotate_data_t;
#define ZID_GESTURE_ROTATE_DATA_LEN             (sizeof(zid_gestureRotate_data_t) )
#define ZID_GESTURE_ROTATE_DIR_MASK             0x01
#define ZID_GESTURE_ROTATE_FINGER_PRESENT_MASK  0x02

// A packed structure that constitutes the data of the ZID_STD_REPORT_GESTURE_SYNC report.
typedef struct {
  uint8 gesture_count;  // Bit 0-3: Contact count; Bit 4: Gesture activity
} zid_gestureSync_data_t;
#define ZID_GESTURE_SYNC_DATA_LEN            sizeof(zid_gestureSync_data_t)
#define ZID_GESTURE_SYNC_GESTURE_ACTIVE_MASK    0x10
#define ZID_GESTURE_SYNC_GESTURE_COUNT_MASK     0x0F

// A packed structure that constitutes the data of the ZID_STD_REPORT_TOUCH_SENSOR_PROPERTIES report.
typedef struct {
  uint8 ges_rel_ori_add;// Bit 0-3: # of additional contacts; Bit 4-5: origin; Bit 6: Reliable index; Bit 7: gestures
  uint8 resolution_x;      // Resolution_x[0:7]
  uint8 resolution_y;      // Resolution_y[0:7]
  uint8 max_coordinate_x_high;   // MaximumCoordinate_x[4:11]
  uint8 max_coordinate_x_y;      // Bits 0-3: MaximumCoordinate_y[8:11]; Bits- 4-7: MaximumCoordinate_x[0:3]
  uint8 max_coordinate_y_low;    // MaximumCoordinate_y[0:7]
  uint8 shape;       // Bits 0-2: shape
} zid_touchSensorProperties_data_t;
#define ZID_TOUCH_SENSOR_PROPERTIES_DATA_LEN              (sizeof(zid_touchSensorProperties_data_t))

/* The following masks are for manipulating individual fields */
#define ZID_TOUCH_SENSOR_PROPERTIES_NUM_ADDITIONAL_CONTACTS_MASK   0x0F
#define ZID_TOUCH_SENSOR_PROPERTIES_ORIGIN_MASK              0x30
#define ZID_TOUCH_SENSOR_PROPERTIES_RELIABLE_INDEX_MASK      0x40
#define ZID_TOUCH_SENSOR_PROPERTIES_GESTURES_MASK            0x80
#define ZID_TOUCH_SENSOR_PROPERTIES_SHAPE_MASK               0x07

/* The following states are for origin, according to Table 19 of the ZID spec*/
#define ZID_TOUCH_SENSOR_PROPERTIES_ORIGIN_LOWER_LEFT     0x00
#define ZID_TOUCH_SENSOR_PROPERTIES_ORIGIN_UPPER_LEFT     0x10
#define ZID_TOUCH_SENSOR_PROPERTIES_ORIGIN_UPPER_RIGHT    0x20
#define ZID_TOUCH_SENSOR_PROPERTIES_ORIGIN_LOWER_RIGHT    0x30

/* The following states are for shape, according to Table 20 of the ZID spec*/
#define ZID_TOUCH_SENSOR_PROPERTIES_SHAPE_RECTILINEAR    0x00
#define ZID_TOUCH_SENSOR_PROPERTIES_SHAPE_CIRCLE         0x01
#define ZID_TOUCH_SENSOR_PROPERTIES_SHAPE_TRAPEZOID      0x02
#define ZID_TOUCH_SENSOR_PROPERTIES_SHAPE_IRREGULAR      0x07

// A packed structure that constitutes the data of the ZID_STD_REPORT_TAP_SUPPORT_PROPERTIES report.
typedef struct {
  uint8 long_double_tapandhalf_single;// Bit 0: single tap; Bit 1: tap and a half; Bit 2: double tap; Bit 3: long tap
  uint8 reserved0;
  uint8 reserved1;
  uint8 reserved2;
} zid_tapSupportProperties_data_t;
#define ZID_TAP_SUPPORT_PROPERTIES_DATA_LEN              (sizeof(zid_tapSupportProperties_data_t))

/* The following masks are for manipulating individual fields */
#define ZID_TAP_SUPPORT_PROPERTIES_SINGLE_TAP_MASK          0x01
#define ZID_TAP_SUPPORT_PROPERTIES_TAP_AND_A_HALF_MASK      0x02
#define ZID_TAP_SUPPORT_PROPERTIES_DOUBLE_TAP_MASK          0x04
#define ZID_TAP_SUPPORT_PROPERTIES_LONG_TAP_MASK            0x08

/**************************************************************************************************
*/

#ifdef __cplusplus
};
#endif

#endif
