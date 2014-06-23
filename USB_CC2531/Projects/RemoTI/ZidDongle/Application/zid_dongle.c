/*
  Filename:       zid_dongle.c
*/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "hal_assert.h"
#if !defined(ZID_DONGLE_NANO)
#include "hal_key.h"
#endif
#include "hal_led.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"
#include "rti.h"
#include "rcn_nwk.h"
#include "zid_adaptor.h"
#include "zid_common.h"
#include "zid_dongle.h"
#include "zid_profile.h"
#include "zid_proxy.h"

#include "zid_usb.h"
#include "zid_dongle.h"
#include "usb_zid_reports.h"

#include <stddef.h>

#include "usb_suspend.h"
#ifdef FEATURE_SBL
#include "hal_flash.h"
#endif
/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */
#ifdef FEATURE_SBL
const __code __root uint32 crc_field @ "CHECKSUM_LOCATION" = 0x00010001;
#define SERIAL_BOOT_CLEAR_MASK    0xFFFF0000
#endif

// RX ON delay duration in milliseconds
//       maybe ultimately the interrupt latency has to be reduced.
//       or maybe there is something wrong with the set up.
//       To verify whether it has anything to do with interrupt latency,
//       create a log stored into NV to store whether there are commands received
//       immedately after resumption.
#define ZID_DONGLE_TIMING_RXON_DELAY   2000

// Key release detect timer duration in milliseconds
#define ZID_DONGLE_KEY_RELEASE_DETECT_DUR 100

// Key event repeat delay duration as multiples of CERC repeat commands
#define ZID_DONGLE_KEY_REPEAT_DELAY    10

// Subset of HID key codes, used in this application
#define ZID_DONGLE_KEYCODE_INVALID       0x00
#define ZID_DONGLE_KEYCODE_A             0x04
#define ZID_DONGLE_KEYCODE_L             0x0F
#define ZID_DONGLE_KEYCODE_RIGHT_ARROW   0x4F
#define ZID_DONGLE_KEYCODE_LEFT_ARROW    0x50
#define ZID_DONGLE_KEYCODE_DOWN_ARROW    0x51
#define ZID_DONGLE_KEYCODE_UP_ARROW      0x52
#define ZID_DONGLE_KEYCODE_NUMLOCK       0x53
#define ZID_DONGLE_KEYCODE_ENTER         0x58
#define ZID_DONGLE_KEYCODE_CAPS_LOCK     0x39
#define ZID_DONGLE_KEYCODE_CAPS_LOCK_TOGGLE     0x82
#define ZID_DONGLE_KEYCODE_BACKSPACE     0x2A; // Backspace
#define ZID_DONGLE_KEYCODE_ROOT_MENU     0x29; // Escape
#define ZID_DONGLE_KEYCODE_DELETE_FORWARD 0x4C; // Delete
// Modifier, must be OR'd in
#define ZID_DONGLE_MODIFIER_LEFT_CTRL    0x01
#define ZID_DONGLE_MODIFIER_LEFT_SHIFT   0x02
#define ZID_DONGLE_MODIFIER_LEFT_ALT     0x04
#define ZID_DONGLE_MODIFIER_LEFT_GUI     0x08
#define ZID_DONGLE_MODIFIER_RIGHT_CTRL   0x10
#define ZID_DONGLE_MODIFIER_RIGHT_SHIFT  0x20
#define ZID_DONGLE_MODIFIER_RIGHT_ALT    0x40
#define ZID_DONGLE_MODIFIER_RIGHT_GUI    0x80

// index to the first key code in the HID report buffer
#define ZID_DONGLE_BUFIDX_FIRST_KEY      3

// application states
#define ZID_DONGLE_STATE_NULL            1
#define ZID_DONGLE_STATE_READY           2
#define ZID_DONGLE_STATE_AUTO_PAIRING    3
#define ZID_DONGLE_STATE_UNPAIRING       4

// CERC Command Discovery Response command frame
const __code uint8 zidDongleCmdDiscResp[] =
{
  RTI_CERC_COMMAND_DISCOVERY_RESPONSE,
  0x00, // command bank number
  // supported command bitmap follows
  0x1F, // cmds 0x00 - 0x07
  0x00, // cmds 0x08 - 0x0F
  0x00, // cmds 0x10 - 0x17
  0x00, // cmds 0x18 - 0x1F
  0xFF, // cmds 0x20 - 0x27
  0x03, // cmds 0x28 - 0x2F
  0x07, // cmds 0x30 - 0x37
  0x00, // cmds 0x38 - 0x3F
  0xFF, // cmds 0x40 - 0x47
  0x1B, // cmds 0x48 - 0x4F
  0x00, // cmds 0x50 - 0x57
  0x00, // cmds 0x58 - 0x5F
  0x00, // cmds 0x60 - 0x67
  0x01, // cmds 0x68 - 0x6F
  0x00, // cmds 0x70 - 0x77
  0x00, // cmds 0x78 - 0x7F
  0x00, // cmds 0x80 - 0x87
  0x00, // cmds 0x88 - 0x8F
  0x00, // cmds 0x90 - 0x97
  0x00, // cmds 0x98 - 0x9F
  0x00, // cmds 0xA0 - 0xA7
  0x00, // cmds 0xA8 - 0xAF
  0x00, // cmds 0xB0 - 0xB7
  0x00, // cmds 0xB8 - 0xBF
  0x00, // cmds 0xC0 - 0xC7
  0x00, // cmds 0xC8 - 0xCF
  0x00, // cmds 0xD0 - 0xD7
  0x00, // cmds 0xD8 - 0xDF
  0x00, // cmds 0xE0 - 0xE7
  0x00, // cmds 0xE8 - 0xEF
  0x00, // cmds 0xF0 - 0xF7
  0x00 // cmds 0xF8 - 0xFF
};

// Count of repetition polling for INPUT packet ready
#define ZID_DONGLE_INPUT_REPEAT_COUNT 3

#define ZID_DONGLE_REPORT_REMOTE_MAX_NUM_SELECTIONS 3

#define ZID_DONGLE_REPORT_NUMERIC_0         1

#define ZID_DONGLE_REPORT_REMOTE_MUTE       1
#define ZID_DONGLE_REPORT_REMOTE_POWER      2
#define ZID_DONGLE_REPORT_REMOTE_LAST       3
#define ZID_DONGLE_REPORT_REMOTE_ASSIGN     4
#define ZID_DONGLE_REPORT_REMOTE_PLAY       5
#define ZID_DONGLE_REPORT_REMOTE_PAUSE      6
#define ZID_DONGLE_REPORT_REMOTE_RECORD     7
#define ZID_DONGLE_REPORT_REMOTE_FAST_FWD   8
#define ZID_DONGLE_REPORT_REMOTE_REWIND     9
#define ZID_DONGLE_REPORT_REMOTE_SCAN_NEXT 10
#define ZID_DONGLE_REPORT_REMOTE_SCAN_PREV 11
#define ZID_DONGLE_REPORT_REMOTE_STOP      12

#define ZID_DONGLE_REPORT_REMOTE_INCREASE  0x01 // channel up
#define ZID_DONGLE_REPORT_REMOTE_DECREASE  0x03 // channel down

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */
// Macros for the remote control 2 byte report
#define ZID_DONGLE_REPORT_REMOTE_INIT(_s) \
  st((_s).data[0] = 0; (_s).data[1] = 0;)
#define ZID_DONGLE_REPORT_REMOTE_HAS_SOME(_s) \
  ((_s).data[0] != 0 || (_s).data[1] != 0)
#define ZID_DONGLE_REPORT_REMOTE_SET_NUMERIC(_s,_x) \
  st((_s).data[0] &= 0xF0; (_s).data[0] |= (_x);)
#define ZID_DONGLE_REPORT_REMOTE_SET_CHANNEL(_s,_x) \
  st((_s).data[0] &= 0xCF; (_s).data[0] |= ((_x)&0x03)<<4;)
#define ZID_DONGLE_REPORT_REMOTE_SET_VOLUME_UP(_s) \
  st((_s).data[0] &= 0x3F; (_s).data[0] |= 0x40;)
#define ZID_DONGLE_REPORT_REMOTE_SET_VOLUME_DOWN(_s) \
  st((_s).data[0] &= 0x3F; (_s).data[0] |= 0x80;)
#define ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(_s,_x) \
  st((_s).data[1] &= 0xF0; (_s).data[1] |= (_x);)
#define ZID_DONGLE_REPORT_REMOTE_SET_SELECTION(_s,_x) \
  st((_s).data[1] &= 0xCF; (_s).data[1] |= ((_x)&0x03)<<4;)
#define ZID_DONGLE_REPORT_REMOTE_MAX_NUM_SELECTIONS 3

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static void appInit(void);
static void appConfig(void);
static void zidDongleKeyReleaseDetected( void );
static void hidProcessCercCtrl(uint8 len, uint8 *pData);
static uint8 zidParseKeyCode(uint8 *pData);
static void zidDataInd(uint8 srcIndex, uint8 len, uint8 *pData);
static zidDongleReportRemote_t* zidDongleParseRemRep(uint8 len, uint8 *pData);
static void zidDongleProcessKeyboard( void );
static void buildAndSendZidKeyboardReport( uint8 key );

#ifdef ZID_IOT
static void buildAndSendSetReport( uint8 frameNum );
static void buildAndSendGetReport( uint8 frameNum );
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

// List of supported target device types: maximum up to 6 device types.
static const uint8 tgtList[RTI_MAX_NUM_SUPPORTED_TGT_TYPES] =
{
  RTI_DEVICE_REMOTE_CONTROL,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID
};

// List of implemented device types: maximum up to 3 device types.
static const uint8 devList[RTI_MAX_NUM_DEV_TYPES] =
{
  RTI_DEVICE_TELEVISION,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID
};

// List of implemented device types: maximum up to 3 device types.
static const uint8 profileList[RTI_MAX_NUM_PROFILE_IDS] =
{
  RTI_PROFILE_ZRC, RTI_PROFILE_ZID, 0, 0, 0, 0, 0
};

static const uint8 vendorName[RTI_VENDOR_STRING_LENGTH] = "TI-LPRF";

// application state variable
static uint8 zidDongleState;

// keyboard input report buffer
static zid_keyboard_data_t zidDongleKeyRepBuf;

// HID Remote Report buffer.
static zidDongleReportRemote_t zidDongleRemRepBuf;

// Application power conserve state variable.
//usbHidPwrState_t hidPwrState;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */
#ifdef ZID_IOT
uint8 zzz_setGetReportCount = 0;
bool zzz_runningIot = FALSE;
#endif

uint8 zidDongleTaskId;

#if ((defined HAL_KEY) && (HAL_KEY == TRUE))
/**************************************************************************************************
 *
 * @fn      keyCback
 *
 * @brief   Callback service for keys.
 *
 * @param   keys  - key that was pressed (i.e. the scanned row/col index)
 *          state - shifted
 *
 * @return  void
 */
static void keyCback(uint8 keys, uint8 state)
{
  (void)state;

  if (keys & HAL_KEY_SW_1)
  {
    RTI_AllowPairReq();
#if (defined HAL_LED) && (HAL_LED == TRUE)
    if (SUCCESS == osal_start_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_LED,
                                                       DPP_AUTO_RESP_DURATION))
    {
      HalLedBlink(HAL_LED_2, 30, 50 /* 50 % duty */, 1000);
    }
#endif
  }
  else if (keys & HAL_KEY_SW_2)
  {
    uint8 value8 = CLEAR_STATE;
    (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_STARTUP_CTRL, 1, &value8);
    zidPxyReset();
    HAL_SYSTEM_RESET();
  }
}
#endif

/**************************************************************************************************
 * @fn          appInit
 *
 * @brief       This is the ADA APP / Dongle initialization and RemoTI startup.
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
static void appInit(void)
{
#if (defined HAL_LED) && (HAL_LED == TRUE)
  // Blink LED_2 to indicate that the device is starting up.
  HalLedBlink(HAL_LED_2,20,50,1000);
#endif

#if defined( POWER_SAVING )
  uint16 val = RCN_NWKC_MAX_DUTY_CYCLE;
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_STDBY_DEFAULT_DUTY_CYCLE, 2, (uint8 *)&val);
  osal_pwrmgr_device(PWRMGR_BATTERY);
#endif

  appConfig();
  RTI_InitReq();  // Start RemoTI application framework and stack

#if ((defined HAL_KEY) && (HAL_KEY == TRUE))
  HalKeyConfig(HAL_KEY_INTERRUPT_ENABLE, keyCback);
#endif
}

/**************************************************************************************************
 * @fn      appConfig
 *
 * @brief   Configure RTI parameters specific to the Dongle.
 *
 * @param
 *
 * None.
 *
 * @return  None.
 *
 */
static void appConfig(void)
{
  union { // Space for largest number of bytes, not counting strings.
    uint8 TgtTypes[RTI_MAX_NUM_SUPPORTED_TGT_TYPES];
    uint8 DevList[RTI_MAX_NUM_DEV_TYPES];
    uint8 VendorName[RTI_VENDOR_STRING_LENGTH];
    uint8 ProfileList[RTI_MAX_NUM_PROFILE_IDS];
    uint8 Buf[SADDR_EXT_LEN];
  } u;

  (void)osal_memcpy(u.TgtTypes, tgtList, RTI_MAX_NUM_SUPPORTED_TGT_TYPES);
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES,
                                         RTI_MAX_NUM_SUPPORTED_TGT_TYPES, u.TgtTypes);

  // No User String pairing; 1 Device (Television); 2 Profiles (ZRC & ZID)
  u.Buf[0] = RTI_BUILD_APP_CAPABILITIES(0, 1, 2);
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_CAPABILITIES, 1, u.Buf);

  (void)osal_memcpy(u.DevList, devList,  RTI_MAX_NUM_DEV_TYPES);
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_DEV_TYPE_LIST,
                                         RTI_MAX_NUM_DEV_TYPES, u.DevList);

  (void)osal_memcpy(u.ProfileList, profileList, RTI_MAX_NUM_PROFILE_IDS);
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_PROFILE_ID_LIST,
                                                RTI_MAX_NUM_PROFILE_IDS, u.ProfileList);

  // Target Type; A/C Pwr; Security capable; Channel Normalization capable.
  u.Buf[0] = RTI_BUILD_NODE_CAPABILITIES(1, 1, 1, 1);
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_NODE_CAPABILITIES, 1, u.Buf);

  *(uint16 *)u.Buf = RTI_VENDOR_TEXAS_INSTRUMENTS;
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_ID, 2, u.Buf);

  (void)osal_memcpy(u.VendorName, vendorName, sizeof(vendorName));
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_NAME,
                                              sizeof(vendorName), u.VendorName);
}

/**************************************************************************************************
 *
 * @fn          zidDongleSuspendEnter
 *
 * @brief       hook function to be called upon entry into USB suspend mode
 *
 * @param       none
 *
 * @return      none
 */
void zidDongleSuspendEnter(void)
{
  // cycle the receiver
  RTI_StandbyReq(RTI_STANDBY_ON);

  // vote to go to sleep
  (void)osal_pwrmgr_task_state( zidDongleTaskId, PWRMGR_CONSERVE );
}

/**************************************************************************************************
 *
 * @fn          zidDongleSuspendExit
 *
 * @brief       hook function to be called upon exit from USB suspend mode
 *
 * @param       none
 *
 * @return      none
 */
void zidDongleSuspendExit(void)
{
  // vote to not go to sleep
  (void)osal_pwrmgr_task_state( zidDongleTaskId, PWRMGR_HOLD );

  // turn on receiver
  RTI_StandbyReq(RTI_STANDBY_OFF);
}


/**************************************************************************************************
 * @fn          Dongle_Init
 *
 * @brief       This is the RemoTI task initialization called by OSAL.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void Dongle_Init(uint8 taskId)
{
  zidDongleTaskId = taskId;

  // initial application state
  zidDongleState = ZID_DONGLE_STATE_NULL;

  // for USB to operate properly, don't allow sleep
  (void)osal_pwrmgr_task_state( taskId, PWRMGR_HOLD );

  (void)osal_set_event(zidDongleTaskId, ZID_DONGLE_EVT_INIT);

  // USB suspend entry/exit hook function setup
  pFnSuspendEnterHook= zidDongleSuspendEnter;
  pFnSuspendExitHook= zidDongleSuspendExit;
}

/**************************************************************************************************
 * @fn          Dongle_ProcessEvent
 *
 * @brief       This routine handles ZID task events.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *              events - Event flags to be processed by this task.
 *
 * output parameters
 *
 * None.
 *
 * @return      16bit - Unprocessed event flags.
 */
uint16 Dongle_ProcessEvent(uint8 taskId, uint16 events)
{
  (void)taskId;

  if (events & ZID_DONGLE_EVT_INIT)
  {
    appInit();
  }

#if (defined HAL_LED) && (HAL_LED == TRUE)
  if (events & ZID_DONGLE_EVT_LED)
  {
    HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
  }
#endif

  if ( events & ZID_DONGLE_EVT_KEY_RELEASE_TIMER )
  {
    // Key release detection timer event
    zidDongleKeyReleaseDetected();
  }

  return 0;  // All events processed in one pass; discard unexpected events.
}

/**************************************************************************************************
 *
 * @fn      RTI_ReceiveDataInd
 *
 * @brief   This function is used by the RTI stack to indicate to the Target application
 *          that data has been received from another RF4CE device.
 *          This function is to be completed by the application, and as such,
 *          constitutes a callback.
 *
 * @param   srcIndex - Specifies the index to the pairing table entry which contains
 *                     the information about the source node the data was received from.
 * @param   profileId - Specifies the identifier of the profile which indicates the format of
 *                      the received data.
 * @param   vendorId - Specifies the identifier of the vendor transmitting the data.
 * @param   rxLQI - Specifies the Link Quality Indication.
 * @param   rxFlags - Specifies the reception indication flags.
 *                    One or more of the following reception indication flags can be specified:
 *                      RTI_RX_FLAGS_BROADCAST
 *                      RTI_RX_FLAGS_SECURITY
 *                      RTI_RX_FLAGS_VENDOR_SPECIFIC
 * @param   len - Specifies the number of bytes of the received data.
 * @param   pData - Specifies a pointer to the received data.
 *
 * @return  void
 */
void RTI_ReceiveDataInd(uint8 srcIndex, uint8 profileId, uint16 vendorId, uint8 rxLQI,
                        uint8 rxFlags, uint8 len, uint8 *pData)
{
  (void) vendorId;
  (void) rxLQI;

  if (profileId == RTI_PROFILE_ZID)
  {
    zidDataInd(srcIndex, len, pData);
    return;
  }
  // CERC standard command.
  if (len > 0 && profileId == RTI_PROFILE_ZRC && (rxFlags & RTI_RX_FLAGS_VENDOR_SPECIFIC) == 0)
  {
    (void)usbsuspDoRemoteWakeup();

    if ((pData[0] == RTI_CERC_USER_CONTROL_PRESSED)  ||
        (pData[0] == RTI_CERC_USER_CONTROL_REPEATED) ||
          (pData[0] == RTI_CERC_USER_CONTROL_RELEASED))
    {
      hidProcessCercCtrl(len, pData);
    }
    else if (pData[0] == RTI_CERC_COMMAND_DISCOVERY_REQUEST &&
             pData[1] == 0 && len == 2)
    {
      // Valid discovery request command is received.
      // Send command discovery response command.

      // Command discovery response command frame has to be copied to RAM buffer
      // since RTI_SendDataReq takes RAM buffer pointer only.
      uint8 * pBuf = (uint8 *) osal_mem_alloc(sizeof(zidDongleCmdDiscResp));
      if (pBuf)
      {
        osal_memcpy(pBuf, zidDongleCmdDiscResp, sizeof(zidDongleCmdDiscResp));
        RTI_SendDataReq(srcIndex, RTI_PROFILE_ZRC, 0, RTI_TX_OPTION_ACKNOWLEDGED,
                        sizeof(zidDongleCmdDiscResp), pBuf);
        osal_mem_free(pBuf);
      }
    }
  }
}

/**************************************************************************************************
 * @fn      hidProcessCercCtrl
 *
 * @brief   Process the received data buffer of a valid CERC control frame.
 *
 * @param   len - The length of the pData buffer.
 * @param   pData - A valid pointer to the received data buffer.
 *
 * @return  None
 */
static void hidProcessCercCtrl(uint8 len, uint8 *pData)
{
  /* Necessary to cover the following CERC 1.1 scenario: key pressed + held for some time.
   * If key press is received but no repeats, the release timer expires and generates a key
   * release report. Then if the key release command is received, the payload should not be
   * used to generate a new key press nor a new key release.
   */
  static uint8 zidDongleKeyPressed = ZID_DONGLE_KEYCODE_INVALID;
  static uint8 zidDongleRemoteReport = FALSE;

  /* Necessary to cover the following CERC 1.1 scenario: no key press or repeat commands are
   * received but the key release with payload is received. This one command frame is now
   * responsible for generating the key press report and as soon as possible thereafter,
   * the key release report.
   */
  uint8 releaseDelay = FALSE;

  if (len > 1)
  {
    uint8 keyCode;
    zidDongleReportRemote_t *pRemRep;

    if ((keyCode = zidParseKeyCode(pData)) != ZID_DONGLE_KEYCODE_INVALID)
    {
      if ((pData[0] == RTI_CERC_USER_CONTROL_PRESSED)  // If command frame is a key press.
           // Or if the key press frame was missed and payload is present (CERC 1.1 or later).
           || (zidDongleKeyPressed == ZID_DONGLE_KEYCODE_INVALID))
      {
        zidDongleKeyPressed = keyCode;
        zidDongleKeyRepBuf.keys[0] = keyCode;
        if (keyCode == ZID_DONGLE_KEYCODE_L) {
            zidDongleKeyRepBuf.mods |= ZID_DONGLE_MODIFIER_LEFT_CTRL;
            zidDongleKeyRepBuf.mods |= ZID_DONGLE_MODIFIER_LEFT_ALT;
            zidDongleKeyRepBuf.keys[0] = ZID_DONGLE_KEYCODE_DELETE_FORWARD;
        }
        zidDongleProcessKeyboard();

        // Need to give the USB time to process the lost press message before the release.
        if (pData[0] == RTI_CERC_USER_CONTROL_RELEASED)
        {
          releaseDelay = TRUE;
          zidDongleKeyPressed = ZID_DONGLE_KEYCODE_INVALID;
        }
      }
      else if (pData[0] == RTI_CERC_USER_CONTROL_RELEASED)
      {
        // It is ok now to interpret the next key repeat/release with payload as a missed press.
        zidDongleKeyPressed = ZID_DONGLE_KEYCODE_INVALID;
      }
    }
    else if ((pRemRep = zidDongleParseRemRep(len, pData)) != NULL)
    {
      if ((pData[0] == RTI_CERC_USER_CONTROL_PRESSED) ||
           // Or if the key press frame was missed and payload is present (CERC 1.1 or later).
          (zidDongleRemoteReport == FALSE))
      {
        zidDongleRemoteReport = TRUE;
        osal_memcpy((uint8 *)(&zidDongleRemRepBuf), (uint8 *)pRemRep, sizeof(zidDongleReportRemote_t));
        // endpoint 2 for this report
        zidSendInReport((uint8*)pRemRep, 2, sizeof(zidDongleReportRemote_t));

        // Need to give the USB time to process the lost press message before the release.
        if (pData[0] == RTI_CERC_USER_CONTROL_RELEASED)
        {
          releaseDelay = TRUE;
          zidDongleRemoteReport = FALSE;
        }
      }
      else if (pData[0] == RTI_CERC_USER_CONTROL_RELEASED)
      {
        // It is ok now to interpret the next key repeat/release with payload as a missed press.
        zidDongleRemoteReport = FALSE;
      }
    }
  }

  if (pData[0] == RTI_CERC_USER_CONTROL_RELEASED)
  {
    if (releaseDelay)
    {
      // Delay event which invokes zidDongleKeyReleaseDetected() which handles key release reports.
      osal_start_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_KEY_RELEASE_TIMER, ZID_DONGLE_KEY_REPEAT_DELAY);
    }
    else
    {
      osal_stop_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_KEY_RELEASE_TIMER);
      // Force event which invokes zidDongleKeyReleaseDetected() which handles key release reports.
      osal_set_event(zidDongleTaskId, ZID_DONGLE_EVT_KEY_RELEASE_TIMER);
    }
  }
  else
  {
    osal_start_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_KEY_RELEASE_TIMER, ZID_DONGLE_KEY_RELEASE_DETECT_DUR);
  }
}

/**************************************************************************************************
 *
 * @fn      zidDongleKeyReleaseDetected
 *
 * @brief   Handles detection of key release
 *
 * @param   None
 *
 * @return  None
 */
static void zidDongleKeyReleaseDetected( void )
{
  if (zidDongleKeyRepBuf.keys[0] != ZID_DONGLE_KEYCODE_INVALID)
  {
    // Keyboard report key is released
    uint8 i;
    zidDongleKeyRepBuf.mods = ZID_DONGLE_KEYCODE_INVALID;
    for (i=0; i<sizeof(zidDongleKeyRepBuf.keys); i++) {
      zidDongleKeyRepBuf.keys[i] = ZID_DONGLE_KEYCODE_INVALID;
    }
    zidDongleProcessKeyboard();
  }
  if (ZID_DONGLE_REPORT_REMOTE_HAS_SOME(zidDongleRemRepBuf))
  {
    // Consumer control key is released
    ZID_DONGLE_REPORT_REMOTE_INIT(zidDongleRemRepBuf);
    // endpoint 2 for this report
    zidSendInReport((uint8*)&zidDongleRemRepBuf, 2, sizeof(zidDongleReportRemote_t));
  }
}

/**************************************************************************************************
 * @fn      zidParseKeyCode
 *
 * @brief   Parse the received data buffer for a valid key command.
 *
 * @param   pData - A valid pointer to the received data buffer.
 *
 * @return  Valid key code parsed or ZID_DONGLE_KEYCODE_INVALID.
 */
static uint8 zidParseKeyCode(uint8 *pData)
{
  uint8 keyCode = ZID_DONGLE_KEYCODE_INVALID;

  switch (pData[1])
  {
  case RTI_CERC_UP:
    keyCode = ZID_DONGLE_KEYCODE_UP_ARROW;
    break;
  case RTI_CERC_DOWN:
    keyCode = ZID_DONGLE_KEYCODE_DOWN_ARROW;
    break;
  case RTI_CERC_LEFT:
    keyCode = ZID_DONGLE_KEYCODE_LEFT_ARROW;
    break;
  case RTI_CERC_RIGHT:
    keyCode = ZID_DONGLE_KEYCODE_RIGHT_ARROW;
    break;
  case RTI_CERC_F1_BLUE:
    keyCode = ZID_DONGLE_KEYCODE_L;
    break;
  case RTI_CERC_F2_RED:
    keyCode = ZID_DONGLE_KEYCODE_CAPS_LOCK;
    break;
#if (defined XBMC_MAPPING) && (XBMC_MAPPING == TRUE)
  case RTI_CERC_BACKWARD:
    keyCode = ZID_DONGLE_KEYCODE_BACKSPACE;
    break;
  case RTI_CERC_ROOT_MENU:
    keyCode = ZID_DONGLE_KEYCODE_ROOT_MENU;
    break;
#endif //XBMC_MAPPING
  default:
    keyCode = ZID_DONGLE_KEYCODE_INVALID;
    break;
  }

  return keyCode;
}
/**************************************************************************************************
 * @fn      zidDongleParseRemRep
 *
 * @brief   Parse the received data buffer for a valid remote report.
 *
 * @param   len - The length of the pData buffer.
 * @param   pData - A valid pointer to the received data buffer.
 *
 * @return  Valid key code parsed or ZID_DONGLE_KEYCODE_INVALID.
 */
static zidDongleReportRemote_t* zidDongleParseRemRep(uint8 len, uint8 *pData)
{
  static zidDongleReportRemote_t parseBuf;
  ZID_DONGLE_REPORT_REMOTE_INIT(parseBuf);

  switch (pData[1])
  {
  case RTI_CERC_CHANNEL_UP:
    ZID_DONGLE_REPORT_REMOTE_SET_CHANNEL(parseBuf, ZID_DONGLE_REPORT_REMOTE_INCREASE);
    break;
  case RTI_CERC_CHANNEL_DOWN:
    ZID_DONGLE_REPORT_REMOTE_SET_CHANNEL(parseBuf, ZID_DONGLE_REPORT_REMOTE_DECREASE);
    break;
  case RTI_CERC_VOLUME_UP:
    ZID_DONGLE_REPORT_REMOTE_SET_VOLUME_UP(parseBuf);
    break;
  case RTI_CERC_VOLUME_DOWN:
    ZID_DONGLE_REPORT_REMOTE_SET_VOLUME_DOWN(parseBuf);
    break;
  case RTI_CERC_MUTE:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_MUTE);
    break;
  case RTI_CERC_POWER:  // Left in to be backwards compatible with CERC v1.0.
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_POWER);
    break;
  case RTI_CERC_PREVIOUS_CHANNEL:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_LAST);
    break;
  case RTI_CERC_SELECT:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_ASSIGN);
    break;
  case RTI_CERC_PLAY:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_PLAY);
    break;
  case RTI_CERC_PAUSE:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_PAUSE);
    break;
  case RTI_CERC_RECORD:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_RECORD);
    break;
  case RTI_CERC_FAST_FORWARD:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_FAST_FWD);
    break;
  case RTI_CERC_REWIND:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_REWIND);
    break;
  case RTI_CERC_FORWARD:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_SCAN_NEXT);
    break;
  case RTI_CERC_BACKWARD:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_SCAN_PREV);
    break;
  case RTI_CERC_STOP:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_STOP);
    break;
  case RTI_CERC_SELECT_MEDIA_FUNCTION:
    if (len > 2 && pData[2] < ZID_DONGLE_REPORT_REMOTE_MAX_NUM_SELECTIONS)
    {
      ZID_DONGLE_REPORT_REMOTE_SET_SELECTION(parseBuf, pData[2] + 1);
    }
    break;
  case RTI_CERC_POWER_TOGGLE_FUNCTION:
    ZID_DONGLE_REPORT_REMOTE_SET_BUTTON(parseBuf, ZID_DONGLE_REPORT_REMOTE_POWER);
    break;
  default:
    if (pData[1] >= RTI_CERC_NUM_0 &&
        pData[1] <= RTI_CERC_NUM_9)
    {
      ZID_DONGLE_REPORT_REMOTE_SET_NUMERIC(parseBuf, pData[1] - RTI_CERC_NUM_0 + 1);
    }
    else
    {
      return NULL;
    }
    break;
  }

  return &parseBuf;
}

/**************************************************************************************************
@fn      zidDongleProcessKeyboard( )
 *
 * @brief   Process a keyboard report. I.e.
 *
 *
 */
static void zidDongleProcessKeyboard( void )
{
  uint8 i;
  uint8* keyboardReport = osal_mem_alloc(zid_usb_report_len[ZID_STD_REPORT_KEYBOARD]);

  // Problem here since the Basic Remote sends a different report than expected->
  // need to parse.
  keyboardReport[0] = ZID_STD_REPORT_KEYBOARD;
  keyboardReport[1] = zidDongleKeyRepBuf.mods;  // modifiers
  keyboardReport[2] = zidDongleKeyRepBuf.reserved;  // modifiers

  for (i=0; i<sizeof(zidDongleKeyRepBuf.keys); i++) {
    keyboardReport[i+3] = zidDongleKeyRepBuf.keys[i];
  }

  // Send the received HID keyboard report when USB endpoint is ready
  // Keep trying to send the report until success. Might end up in endless loop.
  for (i = 0; i < 3; i++)
  {
    if (zidSendInReport(keyboardReport, ZID_PROXY_ZID_EP_IN_ADDR, zid_usb_report_len[ZID_STD_REPORT_KEYBOARD]) == TRUE)
    {
      break;
    }
  }

  // For each successfully sent keyboard report, also clear the HID
  // keyboard report and send a blank report
  osal_start_timerEx(zidDongleTaskId, ZID_DONGLE_EVT_KEY_RELEASE_TIMER, ZID_DONGLE_KEY_RELEASE_DETECT_DUR);

  // free the report buffer
  osal_mem_free( keyboardReport );
}

/**************************************************************************************************
 *
 * @fn      zidDongleOutputReport
 *
 * @brief   Handles output report from host
 *
 * @param   None
 *
 * @return  None
 */
void zidDongleOutputReport(uint8 endPoint, uint8 *zidDongleOutBuf , uint8 len)
{
  (void)len; // unused parameter

  // ZID_DONGLE Pair Entry report buffer.
  static zidDongleReportPairEntry_t pairEntryReport;

  if ( endPoint == ZID_PROXY_RNP_EP_OUT_ADDR) {
    // Decode the output report
    if (ZID_DONGLE_OUTPUT_REPORT_ID == zidDongleOutBuf[0])
    {
      // correct report ID
      uint8 cmdId = zidDongleOutBuf[1];
      switch (cmdId)
      {
      case ZID_DONGLE_CMD_ALLOW_PAIR:
        if ( zidDongleState == ZID_DONGLE_STATE_READY )
        {
          // send the pair request; no parameters required
          // handle the confirm separately
#if (defined HAL_LED) && (HAL_LED == TRUE)
          // blink red LED to indicate that pairing is in progress
          HalLedBlink(HAL_LED_2, 30, 50 /* 50 % duty */, 1000);
#endif
          RTI_AllowPairReq();
          zidDongleState = ZID_DONGLE_STATE_AUTO_PAIRING;
        }
        else
        {
          // invalid state
          zidDongleSendCmdReport(cmdId, RTI_ERROR_NOT_PERMITTED);
        }
        break;
      case ZID_DONGLE_CMD_UNPAIR:
        if ( zidDongleState == ZID_DONGLE_STATE_READY )
        {
          RTI_UnpairReq(zidDongleOutBuf[2]);
        }
        else
        {
          // invalid state
          zidDongleSendCmdReport(cmdId, RTI_ERROR_NOT_PERMITTED);
        }
        break;
      case ZID_DONGLE_CMD_CLEAR_PAIRS:
        {
          uint8 maxPairs, i, result = RTI_SUCCESS;

          // retrieve pairing table size
          RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES, 1, &maxPairs);

          // iterate all pairing table entries and clear them
          for (i = 0; i < maxPairs; i++)
          {
            rcnNwkPairingEntry_t pairEntry;

            // set the pairing entry index
            RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &i);

            // get a pairing entry
            if (RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY, sizeof(rcnNwkPairingEntry_t),
                               (uint8 *) &pairEntry) == RTI_SUCCESS)
            {
              uint8 singleResult;

              // invalidate a valid pairing entry
              pairEntry.pairingRef = RTI_INVALID_PAIRING_REF;
              singleResult = RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY,
                                             sizeof(rcnNwkPairingEntry_t),
                                             (uint8 *) &pairEntry);
              if (RTI_SUCCESS != singleResult)
              {
                // If it fails to clear even one pairing entry, the resulting
                // status should indicate failure.
                result = singleResult;
              }
            }
          }
          if (RTI_SUCCESS == result)
          {
            // Also clear the proxy table so ZidProxy module doesn't think devices are attached
            zidPxyReset();
#if (defined HAL_LED) && (HAL_LED == TRUE)
            // indicate that there is no valid pairing entry
            HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);

            // indicate that there is no valid pairing entry
            HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
#endif
          }
          // send the reply report
          zidDongleSendCmdReport(cmdId, result);
        }
        break;
      case ZID_DONGLE_CMD_GET_MAX_PAIRS:
        {
          uint8 maxPairs;

          // retrieve pairing table size
          RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES, 1, &maxPairs);
          zidDongleSendCmdReport(cmdId, maxPairs);
        }
        break;
      case ZID_DONGLE_CMD_GET_PAIR_ENTRY:
        {
          rcnNwkPairingEntry_t pairEntry;
          uint8 i;

          uint8 maxPairs;
          // retrieve pairing table size
          RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES, 1, &maxPairs);
          if (zidDongleOutBuf[2] >= maxPairs) {
            // invalid pairing entry requested
            pairEntryReport.pairRef = RTI_INVALID_PAIRING_REF;
          } else {
            // set the pairing entry index
            RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &zidDongleOutBuf[2]);
            if (RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY, sizeof(rcnNwkPairingEntry_t),
                               (uint8 *) &pairEntry) == RTI_SUCCESS)
            {
              // succesfully read the valid pairing entry
              // copy fields into the report format

              pairEntryReport.pairRef = pairEntry.pairingRef;
              pairEntryReport.vendorId = pairEntry.vendorIdentifier;
              sAddrExtCpy(pairEntryReport.ieeeAddr, pairEntry.ieeeAddress);
            }
            else
            {
              // must be invalid pairing entry
              pairEntryReport.pairRef = RTI_INVALID_PAIRING_REF;
            }
          }
          pairEntryReport.reportId = ZID_DONGLE_PAIR_ENTRY_REPORT_ID;
          //        uint8 i;
          //Keep trying to send the report until success. Might end up in endless loop.
          for (i = 0; i < 3; i++)
          {
            if (zidSendInReport((uint8 *)&pairEntryReport, ZID_PROXY_RNP_EP_IN_ADDR, sizeof(zidDongleReportPairEntry_t)) == TRUE)
            {
              break;
            }
          }
        }
        break;
      case ZID_DONGLE_CMD_GET_CHANNEL:
        {
          // retrive current MAC channel
          uint8 ch;

          RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_CURRENT_CHANNEL, 1, &ch);
          zidDongleSendCmdReport(cmdId, ch);
        }
        break;
#ifdef FEATURE_SBL
      case ZID_DONGLE_CMD_ENTER_BOOTMODE:
        {
//          HAL_DISABLE_INTERRUPTS();
          // Erase the preamble enable magic number
          // so that the boot loader shall start as serial boot loading mode
          {
            uint32 buf = SERIAL_BOOT_CLEAR_MASK;
            HalFlashWrite(HAL_USB_CRC_ADDR >> 2, (uint8 *) &buf, 1);
          }
//          HAL_ENABLE_INTERRUPTS();
#define   WDT_TIMEOUT    0x08
#define   WD_REGISTER    WDCTL

          // do reset by setting the WDT and waiting for it to timeout
          WD_REGISTER = WDT_TIMEOUT;
        }
      break;
#endif
      default:
        break;
      }
    }
  }
#if (defined HAL_LED) && (HAL_LED == TRUE)
  // Check if the endPoint is either the endPoint assigned to ZID_STD_REPORTS or
  // if it came from EP0. It would come from EP0 in case of a SetReport class request
  // from USB host.
  else if ( endPoint == ZID_PROXY_ZID_EP_OUT_ADDR || endPoint == 0)
  {
    if ( zidDongleOutBuf[0] == ZID_STD_REPORT_KEYBOARD )
    {
      static uint8 led_on = FALSE;
      zid_keyboard_data_out_t *pOutReport = (zid_keyboard_data_out_t *)(&zidDongleOutBuf[1]);

      // caps lock is being used to send reports to class device, and also to toggle LED on dongle
      if (pOutReport->capsLock)
      {
        // indicate that Keyboard OUT report indicated to light an LED
        HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
        led_on = TRUE;

        // Send keyboard report to controller
        buildAndSendZidKeyboardReport( zidDongleOutBuf[1] );
      }
#ifdef ZID_IOT
      else if (pOutReport->scrollLock)
      {
        zzz_setGetReportCount++;
      }
#endif
      else
      {
        if(TRUE == led_on)
        {
          // indicate that Keyboard OUT report indicated to light no LEDs
          HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
          led_on = FALSE;
        }
      }
    }
  }
#endif
}

/**************************************************************************************************
 * @fn      zidDongleSendCmdReport
 *
 * @brief   Send the remote input report ID=2 stored in global data structure
 *
 * @param   None
 *
 * @return  None
 */
void zidDongleSendCmdReport(uint8 cmdId, uint8 status)
{
  zidDongleReportVendor_t report;
  uint8 i;

  report.reportId = ZID_DONGLE_CMD_REPORT_ID;
  report.data[0] = cmdId;
  report.data[1] = status;

  for (i=0; i<3; i++)
  {
    if (zidSendInReport((uint8 *)&report, ZID_PROXY_RNP_EP_IN_ADDR, sizeof(zidDongleReportVendor_t)) == TRUE)
    {
      break;
    }
  }
}

/**************************************************************************************************
 *
 * @fn          zidDataInd
 *
 * @brief       This function handles ZID specific application functionality.
 *
 * input parameters
 *
 * @param       srcIndex:  Pairing table index.
 * @param       len:       Number of data bytes.
 * @param       *pData:    Pointer to data received.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void zidDataInd(uint8 srcIndex, uint8 len, uint8 *pData)
{
  uint8 tmp = *pData & GDP_HEADER_CMD_CODE_MASK;
  (void)len;

  switch (tmp)
  {
    case ZID_CMD_REPORT_DATA:
    {
#ifdef ZID_IOT
      if (zzz_runningIot == TRUE)
      {
        zzz_setGetReportCount++;
        if (zzz_setGetReportCount == 4)
        {
          buildAndSendGetReport( zzz_setGetReportCount );
          return;
        }
      }
#endif
      /* Report records were validated in ZID, so no error checking needed */
      len -= 1; // subtract GDP header length
      pData += 1; // point to beginning of 1st record
#if (defined HAL_LED && (HAL_LED == TRUE))
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
#endif
      while (len)
      {
        uint8 recordLen = *pData + 1; // record length is length of fields not including length field

        (void)zidPxyReport( srcIndex, (zid_report_record_t *)pData );

        pData += recordLen;
        len -= recordLen;
      }
#if (defined HAL_LED && (HAL_LED == TRUE))
      HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
#endif
      break;
    }

    case GDP_CMD_HEARTBEAT:
    case GDP_CMD_PUSH_ATTR:
      break;

    case GDP_CMD_CFG_COMPLETE:
    {
      // Can't do anything here. The configuration is not necessarily ready, try to enumerate upon AllowPairCnf
      break;
    }

    case GDP_CMD_GENERIC_RSP:
    {
#ifdef ZID_IOT
      if (zzz_runningIot == TRUE)
      {
        zzz_setGetReportCount++;
        if (zzz_setGetReportCount == 2 || zzz_setGetReportCount == 6)
        {
          buildAndSendSetReport( zzz_setGetReportCount );
        }
        else if (zzz_setGetReportCount == 3 || zzz_setGetReportCount == 5)
        {
          buildAndSendGetReport( zzz_setGetReportCount );
        }
      }
#endif
      break;
    }

    default:
      break;
  }
}

/**************************************************************************************************
 *
 * @fn      rsaBuildAndSendZidKeyboardReport
 *
 * @brief   Send a ZID profile keyboard report to target
 *
 * @param   key  - key that was pressed
 *
 * @return  void
 */
static void buildAndSendZidKeyboardReport( uint8 key )
{
  uint8 buf[ZID_KEYBOARD_DATA_LEN + sizeof(zid_report_data_cmd_t) + sizeof(zid_report_record_t)];
  zid_report_data_cmd_t *pReport = (zid_report_data_cmd_t *)buf;
  zid_report_record_t *pRecord = (zid_report_record_t *)&pReport->reportRecordList[0];
  zid_keyboard_data_t *pKeyboard = (zid_keyboard_data_t *)(&pRecord->data[0]);
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
  rcnNwkPairingEntry_t *pEntry;
  uint8 dstIndex;

  /* Keyboard reports should go OTA via reliable Control transmission model, no broadcast */
  pReport->cmd = ZID_CMD_REPORT_DATA;
  pRecord->len = sizeof(zid_report_record_t) + ZID_KEYBOARD_DATA_LEN - 1;
  pRecord->type = ZID_REPORT_TYPE_OUT;
  pRecord->id = ZID_STD_REPORT_KEYBOARD;
  pKeyboard->mods = 0x02; // simulate Caps lock
  pKeyboard->reserved = 0;
  pKeyboard->keys[0] = key;
  pKeyboard->keys[1] = 0;
  pKeyboard->keys[2] = 0;
  pKeyboard->keys[3] = 0;
  pKeyboard->keys[4] = 0;
  pKeyboard->keys[5] = 0;

  /* Find destination to send to */

  for (dstIndex = 0; dstIndex < RCN_CAP_MAX_PAIRS; dstIndex++)
  {
    // May be allowing pairing with non-ZID (i.e. CERC profile), so check for ZID.
    if ((RCN_NlmeGetPairingEntryReq( dstIndex, &pEntry ) == RCN_SUCCESS) &&  // Fail not expected here.
        GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      /* Send report */
      RTI_SendDataReq( dstIndex,
                       RTI_PROFILE_ZID,
                       RTI_VENDOR_TEXAS_INSTRUMENTS,
                       txOptions,
                       sizeof( buf ),
                       buf );
      break;
    }
  }
}

#ifdef ZID_IOT
/**************************************************************************************************
 *
 * @fn      buildAndSendSetReport
 *
 * @brief   Send a ZID profile keyboard report to target
 *
 * @param   key  - key that was pressed
 *
 * @return  void
 */
static void buildAndSendSetReport( uint8 frameNum )
{
  uint8 buf[ZID_MOUSE_DATA_LEN + sizeof(zid_set_report_cmd_t)];
  zid_set_report_cmd_t *pReport = (zid_set_report_cmd_t *)buf;
  zid_mouse_data_t *pMouse = (zid_mouse_data_t *)(&pReport->data[0]);
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
  rcnNwkPairingEntry_t *pEntry;
  uint8 dstIndex;

  /* Keyboard reports should go OTA via reliable Control transmission model, no broadcast */
  pReport->cmd = ZID_CMD_SET_REPORT | GDP_HEADER_DATA_PENDING;
  if (frameNum == 2)
  {
    pReport->type = 0xFF;
    pReport->id = 0xFF;
  }
  else
  {
    pReport->type = ZID_REPORT_TYPE_OUT;
    pReport->id = ZID_STD_REPORT_MOUSE;
  }
  pMouse->btns = 0;
  pMouse->x = 0;
  pMouse->y = 0;

  /* Find destination to send to */
  for (dstIndex = 0; dstIndex < RCN_CAP_MAX_PAIRS; dstIndex++)
  {
    // May be allowing pairing with non-ZID (i.e. CERC profile), so check for ZID.
    if ((RCN_NlmeGetPairingEntryReq( dstIndex, &pEntry ) == RCN_SUCCESS) &&  // Fail not expected here.
        GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      /* Send report */
      RTI_SendDataReq( dstIndex,
                       RTI_PROFILE_ZID,
                       RTI_VENDOR_TEXAS_INSTRUMENTS,
                       txOptions,
                       sizeof( buf ),
                       buf );
      break;
    }
  }
}

/**************************************************************************************************
 *
 * @fn      buildAndSendGetReport
 *
 * @brief   Send a ZID profile keyboard report to target
 *
 * @param   key  - key that was pressed
 *
 * @return  void
 */
static void buildAndSendGetReport( uint8 frameNum )
{
  uint8 buf[sizeof(zid_get_report_cmd_t)];
  zid_get_report_cmd_t *pReport = (zid_get_report_cmd_t *)buf;
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
  rcnNwkPairingEntry_t *pEntry;
  uint8 dstIndex;

  /* Keyboard reports should go OTA via reliable Control transmission model, no broadcast */
  pReport->cmd = ZID_CMD_GET_REPORT | GDP_HEADER_DATA_PENDING;
  if (frameNum == 3)
  {
    pReport->type = ZID_REPORT_TYPE_IN;
    pReport->id = ZID_STD_REPORT_MOUSE;
  }
  else if (frameNum == 4)
  {
    pReport->type = ZID_REPORT_TYPE_IN;
    pReport->id = 0xFF;
  }
  else if (frameNum == 5)
  {
    pReport->type = 0xFF;
    pReport->id = 0xFF;
  }

  /* Find destination to send to */
  for (dstIndex = 0; dstIndex < RCN_CAP_MAX_PAIRS; dstIndex++)
  {
    // May be allowing pairing with non-ZID (i.e. CERC profile), so check for ZID.
    if ((RCN_NlmeGetPairingEntryReq( dstIndex, &pEntry ) == RCN_SUCCESS) &&  // Fail not expected here.
        GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      /* Send report */
      RTI_SendDataReq( dstIndex,
                       RTI_PROFILE_ZID,
                       RTI_VENDOR_TEXAS_INSTRUMENTS,
                       txOptions,
                       sizeof( buf ),
                       buf );
      break;
    }
  }
}
#endif

/**************************************************************************************************
 *
 * @fn      RTI_InitCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_InitReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_InitReq has returned.
 *
 * @param   status - Result of RTI_InitReq API call.
 *
 * @return  void
 */
void RTI_InitCnf( rStatus_t status )
{
  if ( status == RTI_SUCCESS )
  {
    uint8 value;

    // Now that RTI_InitReq() is done, change startup control settings so that
    // it will restore configured parameters after power cycle
    value = RESTORE_STATE;
    RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_STARTUP_CTRL, 1, &value);

    // Check if we have have previous valid entries in pairing table.
    // 'value' will contain the number of valid paring entries
    if (RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_NUMBER_OF_ACTIVE_ENTRIES, 1, &value)
        == RTI_SUCCESS && value)
    {
#if (defined HAL_LED) && (HAL_LED == TRUE)
      // Turn on LED1 (yellow) if we have a valid paring entry
      HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);

      // Turn off LED2 (red) because we have a valid entry
      HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
    }
    else if(0 == value)
    {
      // Turn on LED2 (red) to indicate that we don't have any valid pairing entry
      HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
#endif
    }
    zidDongleState = ZID_DONGLE_STATE_READY;
  }
  else
  {
    //We should not get here, but trap in case we do
    HAL_ASSERT_FORCED();
  }

  zidPxyInit();
}

/**************************************************************************************************
 *
 * @fn      RTI_PairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_PairReq has returned.
 *
 * @param   status - Result of RTI_PairReq API call.
 *
 * @return  void
 */
void RTI_PairCnf( rStatus_t status, uint8 dst, uint8 dev )
{
  // This node is configured as target and will not issue a RTI_PairReq.

  // unused arguments
  (void) status;
  (void) dst;
  (void) dev;
}


/**************************************************************************************************
 *
 * @fn      RTI_PairAbortCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairAbortReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_PairAbortReq has returned.
 *
 * @param   status - Result of RTI_PairAbortReq API call.
 * @return  void
 */
void RTI_PairAbortCnf( rStatus_t status )
{
  (void) status; // unused argument
}

/**************************************************************************************************
 *
 * @fn      RTI_AllowPairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_AllowPairReq API
 *          call. The client is expected to complete this function.
 *
 * @param   status - Result of RTI_AllowPairReq API call.
 * @param   dstIndex - pairing table entry index for newly paired peer device
 * @param   devType - device type of the newly paired peer device
 *
 * @return  void
 */
void RTI_AllowPairCnf( rStatus_t status, uint8 dstIndex, uint8 devType )
{
  (void)devType;
  uint8 activePairCount;

  rcnNwkPairingEntry_t *pEntry;

  if (status == RCN_SUCCESS)
  {
    // May be allowing pairing with non-ZID (i.e. CERC profile), so check for ZID.
    if ((RCN_NlmeGetPairingEntryReq(dstIndex, &pEntry) == RCN_SUCCESS) &&  // Fail not expected here.
        GET_BIT(pEntry->profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      zidPxyEntry(dstIndex);
    }
  }

  if (RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_NUMBER_OF_ACTIVE_ENTRIES, 1, &activePairCount)
      == RTI_SUCCESS && activePairCount)
  {
#if (defined HAL_LED) && (HAL_LED == TRUE)
    // at least one valid pairing entry
    // indicate by leaving the green LED on
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
#endif
  }
  else
  {
#if (defined HAL_LED) && (HAL_LED == TRUE)
    // no valid pairing entry
    // indicate by leaving the green LED off
    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
#endif
  }

  if (zidDongleState == ZID_DONGLE_STATE_AUTO_PAIRING)
  {
    // pairing triggered by host
    // send the reply command
    zidDongleSendCmdReport(ZID_DONGLE_CMD_ALLOW_PAIR, status);
  }
  zidDongleState = ZID_DONGLE_STATE_READY;

#ifdef ZID_IOT
  // if we are running test case 3.6.1, fire off the 1st set report frame
  if (zzz_setGetReportCount == 1)
  {
    zzz_runningIot = TRUE;
    buildAndSendSetReport( zzz_setGetReportCount );
  }
#endif
}


/**************************************************************************************************
 *
 * @fn      RTI_UnpairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_UnpairReq has returned.
 *
 * @param   status   - Result of RTI_PairReq API call.
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void RTI_UnpairCnf( rStatus_t status, uint8 dstIndex )
{
  // unused arguments
  (void) status;

  // Change application state
  if ( zidDongleState == ZID_DONGLE_STATE_UNPAIRING )
  {
    zidDongleState = ZID_DONGLE_STATE_READY;

    zidPxyUnpair(dstIndex);

    // Send HID report. "status" does not matter, the entry must have been removed.
    zidDongleSendCmdReport(ZID_DONGLE_CMD_UNPAIR, dstIndex);
  }
}


/**************************************************************************************************
 *
 * @fn      RTI_UnpairInd
 *
 * @brief   RTI indication callback initiated by receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device.
 *
 * @return  void
 */
void RTI_UnpairInd( uint8 dstIndex )
{
  zidPxyUnpair(dstIndex);

  // Send HID report
  zidDongleSendCmdReport(ZID_DONGLE_CMD_UNPAIR, dstIndex);
}


/**************************************************************************************************
 *
 * @fn      RTI_SendDataCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_SendDataReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_SendDataReq has returned.
 *
 * @param   status - Result of RTI_SendDataReq API call.
 *
 * @return  void
 */
void RTI_SendDataCnf( rStatus_t status )
{
  // This node is configured as target and will not issue a RTI_SendDataReq

  (void) status; // unused argument
}

/**************************************************************************************************
 *
 * @fn      RTI_XXX_Cnf
 *
 * @brief   These functions are used by the RTI stack to respond to the corresponding
 *          RTI_XXX_Req call from the application.
 *          These functions are to be completed by the application, and are thereby callbacks.
 *
 * @param   status - Status of RTI_XXX_Req action.
 *
 * @return  void
 */
void RTI_DisableSleepCnf( rStatus_t status )
{
  (void) status;
}

void RTI_EnableSleepCnf( rStatus_t status )
{
  (void) status;
}

void RTI_RxEnableCnf( rStatus_t status )
{
  (void) status;
}

void RTI_StandbyCnf( rStatus_t status )
{
  (void) status;
}

/**************************************************************************************************
*/
