/**************************************************************************************************
  Filename:       rsa_point.c
  Revised:        $Date: 2012-11-14 15:49:48 -0800 (Wed, 14 Nov 2012) $
  Revision:       $Revision: 32189 $

  Description:    RemoTI Sample Application (RSA) for the CC2530 Advanced Pointing Remote Control.

  Copyright 2010-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/


/**************************************************************************************************
 *                                           Includes
 */
#include "gdp.h"

/* Hal Driver includes */
#include "hal_adc.h"
#if (defined HAL_BUZZER) && (HAL_BUZZER == TRUE)
#include "hal_buzzer.h"
#endif
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_lcd.h"
#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
#include "hal_motion.h"
#endif
#ifdef FEATURE_OAD
#include "oad.h"
#include "oad_client.h"
#endif
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"
#include "OnBoard.h"
#include "rti_constants.h"
#include "rti.h"
#include "rsa.h"
#include "zid_common.h"
#include "zid_class_device.h"
#include "zid_cld_app_helper.h"
#include "zid_hid_constants.h"
#include "zid_profile.h"

/**************************************************************************************************
 *                                           Macros
 */

/**************************************************************************************************
 *                                           Constant
 */

// RSA Events
#define RSA_EVT_INIT                    0x0001
#define RSA_EVT_REBOOT                  0x0002
#define RSA_EVT_OAD_INACTIVITY          0x0004
#define RSA_EVT_RANDOM_BACKOFF_TIMER    0x0008
#define RSA_EVT_DOUBLE_CLICK_TIMER      0x0010
#define RSA_EVT_MOTION_SENSOR_TIMER     0x0020

// OAD polling timeout duration in 200 ms
#define RSA_POLL_TIMEOUT                 200

// Motion Sensor calibration time, in milliseconds
#define RSA_CAL_DURATION 5000

// Time of no movement which causes transition from motion detection to low power, in ms
#define RSA_EXIT_MOTION_DETECT_TIMEOUT 60000 // 1 minute

// RSA States
enum
{
  RSA_STATE_INIT,
  RSA_STATE_READY,
  RSA_STATE_PAIR,
  RSA_STATE_NDATA,
  RSA_STATE_UNPAIR,
  RSA_STATE_TEST,
  RSA_STATE_TEST_COMPLETED,
  RSA_STATE_OAD
};

// key mapping table
typedef struct
{
  uint8 key;
  uint8 profile;
} rsaKeyMap_t;

// -- media selections: these are arbitrary --
// these are secondary codes based on an initial key press that corresponds to the Media Function
#define RSA_MEDIA_RECORDED_TV  1
#define RSA_MEDIA_LIVE_TV      2
#define RSA_MEDIA_END          3

// -- special key map command codes --

// media selection
#define RSA_MEDIA_SEL            0x80

// vendor specific extended TV command set range for keymap
#define RSA_EXT_TV               (RSA_MEDIA_SEL + RSA_MEDIA_END)

// Note that sequence has to be maintained to use function pointer array
#define RSA_ACT_START                     0xB0
#define RSA_ACT_PAIR                      0xB0
#define RSA_ACT_UNPAIR                    0xB1
#define RSA_ACT_TOGGLE_TGT                0xB2
#define RSA_ACT_TEST_MODE                 0xB3
#define RSA_ACT_AIR_MOUSE_CAL             0xB4
#define RSA_ACT_MOUSE_RESOLUTION_DECREASE 0xB5
#define RSA_ACT_MOUSE_RESOLUTION_INCREASE 0xB6
#ifdef FEATURE_OAD
#define RSA_ACT_POLL                      0xB7
#elif defined ZID_IOT
#define RSA_ACT_TX_OPTIONS                0xB7
#endif

// target device type selection key
#define RSA_TGT_TYPE             0xC0

// List of supported target device types: maximum up to 6 device types.
static const uint8 tgtList[RTI_MAX_NUM_SUPPORTED_TGT_TYPES] =
{
  RTI_DEVICE_TELEVISION,
  RTI_DEVICE_VIDEO_PLAYER_RECORDER,
  RTI_DEVICE_SET_TOP_BOX,
  RTI_DEVICE_MEDIA_CENTER_PC,
  RTI_DEVICE_GAME_CONSOLE,
  RTI_DEVICE_MONITOR
};

// List of implemented device types: maximum up to 3 device types.
static const uint8 devList[RTI_MAX_NUM_DEV_TYPES] =
{
  RTI_DEVICE_REMOTE_CONTROL,
  RTI_DEVICE_RESERVED_INVALID,
  RTI_DEVICE_RESERVED_INVALID
};

// List of implemented profiles: maximum up to 7 profiles.
static const uint8 profileList[RTI_MAX_NUM_PROFILE_IDS] =
{
  RTI_PROFILE_ZID, RTI_PROFILE_ZRC, 0, 0, 0, 0, 0
};

static const uint8 vendorName[RTI_VENDOR_STRING_LENGTH] = "TI-LPRF";

// -- key maps --

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
// CC2530 Remote keymap
static const rsaKeyMap_t __code rsaKeyMap[48] =
{
  // 0b00 <KPb> <KPa>
  // row mapped    to P0 and P1
  // column mapped to shift register controlled by P0 and P2
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 00 0000
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 00 0001
  { RTI_CERC_ROOT_MENU, RTI_PROFILE_ZRC },                   // 0b00 00 0010   - MENU
  { HID_KEYBOARD_LEFT_ARROW, RTI_PROFILE_ZID },              // 0b00 00 0011   - NAV LEFT
  { RTI_CERC_REWIND, RTI_PROFILE_ZRC },                      // 0b00 00 0100   - REV
  { HID_MOUSE_BUTTON_LEFT, RTI_PROFILE_ZID },                // 0b00 00 0101   - MOUSE LEFT
  { RTI_CERC_VOLUME_DOWN, RTI_PROFILE_ZRC },                 // 0b00 00 0110   - VOL-
  { RTI_CERC_VOLUME_UP, RTI_PROFILE_ZRC },                   // 0b00 00 0111   - VOL+
  { RTI_CERC_RECORD, RTI_PROFILE_ZRC },                      // 0b00 00 1000   - REC
  { HID_KEYBOARD_1, RTI_PROFILE_ZID },                       // 0b00 00 1001   - 1
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 00 1010   - AV
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 00 1011
  { RSA_ACT_PAIR, RTI_PROFILE_RTI },                         // 0b00 00 1100   - RED (K1)
  { HID_KEYBOARD_7, RTI_PROFILE_ZID },                       // 0b00 00 1101   - 7
  { HID_KEYBOARD_4, RTI_PROFILE_ZID },                       // 0b00 00 1110   - 4
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 00 1111
  { RSA_ACT_MOUSE_RESOLUTION_DECREASE, RTI_PROFILE_RTI },    // 0b00 01 0000   - FAV
  { RTI_CERC_VIDEO_ON_DEMAND, RTI_PROFILE_ZRC },             // 0b00 01 0001   - VOD
  { HID_KEYBOARD_DOWN_ARROW, RTI_PROFILE_ZID },              // 0b00 01 0010   - NAV DOWN
  { HID_KEYBOARD_RETURN, RTI_PROFILE_ZID },                  // 0b00 01 0011   - OK
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 01 0100
  { HID_MOUSE_BUTTON_MIDDLE, RTI_PROFILE_RTI },              // 0b00 01 0101   - MOUSE MIDDLE
  { HID_KEYBOARD_UP_ARROW, RTI_PROFILE_ZID },                // 0b00 01 0110   - NAV UP
  { RTI_CERC_MUTE, RTI_PROFILE_ZRC },                        // 0b00 01 0111   - MUTE
  { RTI_CERC_PLAY, RTI_PROFILE_ZRC },                        // 0b00 01 1000   - PLAY
  { HID_KEYBOARD_2, RTI_PROFILE_ZID },                       // 0b00 01 1001   - 2
  { HID_KEYBOARD_0, RTI_PROFILE_ZID },                       // 0b00 01 1010   - 0
#ifdef FEATURE_OAD
  { RSA_ACT_POLL, RTI_PROFILE_RTI },                         // 0b00 01 1011   - YELLOW (K3)
#else
  { RSA_ACT_UNPAIR, RTI_PROFILE_RTI },                       // 0b00 01 1011   - YELLOW (K3)
#endif
#ifdef ZID_IOT
  { RSA_ACT_TX_OPTIONS, RTI_PROFILE_RTI },                   // 0b00 01 1100   - GREEN (K2)
#else
  { RSA_ACT_TEST_MODE, RTI_PROFILE_RTI },                    // 0b00 01 1100   - GREEN (K2)
#endif
  { HID_KEYBOARD_8, RTI_PROFILE_ZID },                       // 0b00 01 1101   - 8
  { HID_KEYBOARD_5, RTI_PROFILE_ZID },                       // 0b00 01 1110   - 5
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 01 1111

  { RTI_CERC_POWER_TOGGLE_FUNCTION, RTI_PROFILE_ZRC },       // 0b00 10 0000   - POWER
  { RSA_ACT_MOUSE_RESOLUTION_INCREASE, RTI_PROFILE_RTI },    // 0b00 10 0001   - TV
  { RTI_CERC_BACKWARD, RTI_PROFILE_ZRC },                    // 0b00 10 0010   - BACK
  { HID_KEYBOARD_RIGHT_ARROW, RTI_PROFILE_ZID },             // 0b00 10 0011   - NAV RIGHT
  { RTI_CERC_FAST_FORWARD, RTI_PROFILE_ZRC },                // 0b00 10 0100   - FFWD
  { HID_MOUSE_BUTTON_RIGHT, RTI_PROFILE_ZID },               // 0b00 10 0101   - MOUSE RIGHT
  { RTI_CERC_CHANNEL_DOWN, RTI_PROFILE_ZRC },                // 0b00 10 0110   - CH-
  { RTI_CERC_CHANNEL_UP, RTI_PROFILE_ZRC },                  // 0b00 10 0111   - CH+
  { RTI_CERC_STOP, RTI_PROFILE_ZRC },                        // 0b00 10 1000   - STOP
  { HID_KEYBOARD_3, RTI_PROFILE_ZID },                       // 0b00 10 1001   - 3
  { RTI_CERC_PAUSE, RTI_PROFILE_ZRC },                       // 0b00 10 1010   - PAUSE (?)
  { RSA_ACT_AIR_MOUSE_CAL, RTI_PROFILE_RTI },                // 0b00 10 1011   - BLUE (K4)
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID },                // 0b00 10 1100
  { HID_KEYBOARD_9, RTI_PROFILE_ZID },                       // 0b00 10 1101   - 9
  { HID_KEYBOARD_6, RTI_PROFILE_ZID },                       // 0b00 10 1110   - 6
  { HID_KEYBOARD_RESERVED, RTI_PROFILE_ZID }                 // 0b00 10 1111
};
#endif

#define RSA_IR_TOGGLE_BIT           1                        // IR signal toggle bit.

#if !defined HAL_KEY_CODE_NOKEY
#define HAL_KEY_CODE_NOKEY          0xFF
#endif

#ifdef ZID_IOT
extern uint8 mouseTxOptions;
#endif

/**************************************************************************************************
 *                                          Typedefs
 */

// Test request command
// Test parameters structure should be compliant with the TI vendor specific command format.
typedef struct
{
  uint8 startCondition;
  uint8 testType;
  uint8 txOptions;
  uint8 userDataSize; // user data is one byte plus this size as one byte should be used
                      // to carray test command identifier
  uint16 numPackets;
  uint16 maxBackoffDuration;
} rrtTestReq_t;

enum
{
  RSA_TEST_TERMINATE,
  RSA_TEST_LATENCY,
  RSA_TEST_PER
};

enum
{
  RSA_TEST_START_IMMEDIATE,
  RSA_TEST_START_ON_BUTTON
};

enum
{
  RSA_MOTION_DETECTOR_DISABLED,
  RSA_MOTION_DETECTOR_STANDBY,
  RSA_MOTION_DETECTOR_ENABLED
};
typedef uint8 rsaMotionDetectorState_t;

typedef struct
{
  uint8 testType;
  struct
  {
    uint16 numPackets;
    uint16 bin[29];
  } latency;
} rrtTestReportRsp_t;

/**************************************************************************************************
 *                                        Global Variables
 */

uint8 RSA_TaskId;

/**************************************************************************************************
 *                                        Local Variables
 */

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
static uint16 rsaNoMotionCount;
// Keeps track of left/middle/right mouse buttons
static uint8 rsaMouseButtonStates;
static bool rsaBlockMouseMovements;
static bool rsaKeyPressed;
static rsaMotionDetectorState_t rsaMotionDetectorState = RSA_MOTION_DETECTOR_DISABLED;
static bool rsaNoMotionTimerRunning = FALSE;
#endif

// current target device pairing reference
static uint8 rsaDestIndex;

// current state
static uint8 rsaState;

// key state
static uint8 rsaKeyRepeated;

// designated target device type
static uint8 rsaTgtDevType;

static bool rsaDoubleClickTimerRunning;
static bool rsaDisableMouseMovementAfterKeyRelease;

static rrtTestReq_t rsaTestReqCache =
{
  RSA_TEST_START_IMMEDIATE,
  RSA_TEST_LATENCY,
  RTI_TX_OPTION_ACKNOWLEDGED | RTI_TX_OPTION_VENDOR_SPECIFIC,
  8,
  1000,
  64
};

static rrtTestReportRsp_t rsaTestReportCache;

// Max app payload, used for vendor specific data
static uint8 pTestData[128];

// Measurement variables
static uint32 rsaSendReqTimeStamp;
static uint32 rsaSendCnfTimeStamp;

struct {
  // count of successful confirmations within 100ms, 200ms, 300ms, ...
  uint16 latencyCnt[11];
  uint16 failCnt;
  uint16 reqCnt;
} rsaAutoTestStatistics;

static rcnNwkPairingEntry_t rsaPairingEntryBuf;
static uint8 rsaMaxNumPairingEntries;

// Function prototypes for forward reference follow

// OS Related
void   RSA_Init( uint8 taskId );
uint16 RSA_ProcessEvent( uint8 taskId, uint16 events );

// HAL Related
#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
void RSA_KeyCback( uint8 keys, uint8 state );
void RSA_CalCompleteCback( void );
void RSA_MotionSensorCback( int16 gyroMickeysX, int16 gyroMickeysY );
void RSA_BuzzerCompleteCback( void );
static void rsaBuildAndSendZidMouseReport( uint8 mouseStates, int8 mickeysX, int8 mickeysY );
static void rsaBuildAndSendZidKeyboardReport( uint8 key );
#endif

static void rsaConfig(void);
static void rsaSelectInitialTargetDevice(void);
static void rsaPairKeyAction(void);
static void rsaUnpairKeyAction(void);
static void rsaTargetDeviceToggleKeyAction(void);
static void rsaToggleTestModeKeyAction(void);
#ifdef FEATURE_OAD
static void rsaPollKeyAction(void);
#endif
#ifdef ZID_IOT
static void rsaMouseTxOptionSet( void );
#endif
#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
static void rsaAirMouseCalKeyAction( void );
static void rsaAirMouseResolutionDecrease( void );
static void rsaAirMouseResolutionIncrease( void );
static void rsaAirMouseCursorCtlKeyPress( void );
static void rsaAirMouseCursorCtlKeyRelease( void );
static void rsaRRTRunTest(uint8);
static void rsaRRTSendReport(void);
#endif
static void rsaRRTSendData(void);
static void rxGetReport(uint8 srcIndex, uint8 *pData);
static void rsaBuildAndSendZrcReport( uint8 cmd, bool keyPressed, bool keyRepeated );

// function pointer map for special key actions
typedef void (*rsaActionFn_t)(void);

const rsaActionFn_t rsaSpecialKeyActions[] =
{
  rsaPairKeyAction,                // set state to discovery, and turn on LED2
  rsaUnpairKeyAction,
  rsaTargetDeviceToggleKeyAction,
  rsaToggleTestModeKeyAction,
#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
  rsaAirMouseCalKeyAction,
  rsaAirMouseResolutionDecrease,
  rsaAirMouseResolutionIncrease,
#endif
#ifdef FEATURE_OAD
  rsaPollKeyAction
#elif defined ZID_IOT
  rsaMouseTxOptionSet
#endif
};
#define RSA_SPECIAL_KEY_ACTIONS_SIZE (sizeof(rsaSpecialKeyActions) / sizeof(rsaSpecialKeyActions[0]))

void HalLedBlink (uint8 leds, uint8 numBlinks, uint8 percent, uint16 period)
{
  (void) leds;
  (void) numBlinks;
  (void) percent;
  (void) period;
}

uint8 HalLedSet(uint8 led, uint8 mode)
{
  (void)led;
  (void)mode;
  return 0;
}

/**************************************************************************************************
 *
 * @fn          RSA_Init
 *
 * @brief       Initialize the application
 *
 * @param       taskId - taskId of the task after it was added in the OSAL task queue
 *
 * @return      none
 */
void RSA_Init(uint8 taskId)
{
  // initialize the task id
  RSA_TaskId = taskId;

  // set sample application state to not ready until RTI Init is successfully confirmed
  rsaState = RSA_STATE_INIT;

  // schedule Remote Application startup
  (void)osal_set_event( RSA_TaskId, RSA_EVT_INIT );

  rsaTgtDevType = RTI_DEVICE_RESERVED_FOR_WILDCARDS;
}

/**************************************************************************************************
 *
 * @fn          RSA_ProcessEvent
 *
 * @brief       This routine handles events
 *
 * @param       taskId - ID of the application task when it registered with the OSAL
 *              events - Events for this task
 *
 * @return      16bit - Unprocessed events
 */
uint16 RSA_ProcessEvent(uint8 taskId, uint16 events)
{
  uint8* pMsg;
  (void)taskId;

  if (events & SYS_EVENT_MSG)
  {
    while ((pMsg = osal_msg_receive(RSA_TaskId)) != NULL)
    {
      (void)osal_msg_deallocate((uint8 *) pMsg);
    }

    return events ^ SYS_EVENT_MSG;
  }

  // Application Initialization
  if ( events & RSA_EVT_INIT )
  {
    if ( rsaState == RSA_STATE_INIT )
    {
      rsaConfig();  // Configure RTI parameters relevant to the RSA.

      // Initialize the RF4CE software and begin network operation.
      // NOTE: Configuration Parameters need to be set prior to this call.
      RTI_InitReq();

      // Enable power savings.
      // This function can be called irrespective of RTI_InitReq() status.
      RTI_EnableSleepReq();

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
      /* Setup Keyboard callback */
      HalKeyConfig(RSA_KEY_INT_ENABLED, RSA_KeyCback);
      // Config Motion Sensor processing
      HalMotionConfig( RSA_MotionSensorCback );
#endif
    }
  }

  // The following are latency test related codes.
  if (events & RSA_EVT_RANDOM_BACKOFF_TIMER)
  {
    if (rsaState == RSA_STATE_TEST)
    {
      uint8 i, profileId, vendorId;

      rsaAutoTestStatistics.reqCnt++;
      rsaTestReportCache.latency.numPackets++;


      // send the data over the air
      profileId = RTI_PROFILE_ZRC;
      vendorId  = RTI_VENDOR_TEXAS_INSTRUMENTS;
      pTestData[0] = RTI_PROTOCOL_TEST;
      pTestData[1] = RTI_CMD_TEST_DATA;
      for (i = 0; i < rsaTestReqCache.userDataSize; i++)
      {
        pTestData[i+2] = i;
      }

      rsaSendReqTimeStamp = osal_GetSystemClock();
      RTI_SendDataReq( rsaDestIndex, profileId, vendorId, rsaTestReqCache.txOptions, rsaTestReqCache.userDataSize+2, pTestData);

    }
  }

#ifdef FEATURE_OAD
  if ( events & RSA_EVT_REBOOT )
  {
    OAD_Reboot();
  }
  else if (( events & RSA_EVT_OAD_INACTIVITY ) && rsaState == RSA_STATE_OAD )
  {
    // OAD inactivity timer expired
    rsaState = RSA_STATE_READY;
    OAD_State = OAD_CLIENT_IDLE_STATE;
  }
#endif

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
  if (events & RSA_EVT_DOUBLE_CLICK_TIMER)
  {
    /* No special processing needed; just indicate timer no longer running */
    rsaDoubleClickTimerRunning = FALSE;
  }

  if (events & RSA_EVT_MOTION_SENSOR_TIMER)
  {
    /* user hasn't performed mouse activity -- turn off motion sensor system */
    HalMotionDisable();
    rsaMotionDetectorState = RSA_MOTION_DETECTOR_DISABLED;
  }
#endif

  return 0;
}

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
    (void)RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_CONST_ITEM_MAX_PAIRING_TABLE_ENTRIES, 1,
                                                            &rsaMaxNumPairingEntries);
    // RTI has been successfully started, so application is now ready to go
    rsaState = RSA_STATE_READY;

    // select initial target device type
    // currently, this routine finds the first pairing entry index, if one exists
    rsaSelectInitialTargetDevice();

    status = RESTORE_STATE;
    (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_STARTUP_CTRL, 1, &status);
  }
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
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 * @param   devType  - Pairing table index device type, or invalid.
 * @return  void
 */
void RTI_PairCnf( rStatus_t status, uint8 dstIndex, uint8 devType )
{
  (void) devType; // unused argument

  // turn off transmission light indicator
  HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);

  if ( status == RTI_SUCCESS )
  {
    rsaDestIndex = dstIndex;
    (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &rsaDestIndex);
    (void)RTI_ReadItemEx(RTI_PROFILE_RTI,  RTI_SA_ITEM_PT_CURRENT_ENTRY,
                         sizeof(rsaPairingEntryBuf), (uint8 *)&rsaPairingEntryBuf);

    /* Provide feedback that pairing was successful */
#if (defined HAL_BUZZER) && (HAL_BUZZER == TRUE)
    /* Tell OSAL to not go to sleep because buzzer uses T3 */
    osal_pwrmgr_task_state( RSA_TaskId, PWRMGR_HOLD );

    HalBuzzerRing( 500, RSA_BuzzerCompleteCback ); // ring buzzer for 500msec
#endif
  }

  // TEMP so RC can keep working
  rsaState = RSA_STATE_READY;

#ifdef ZID_IOT
  // Turn on RX in case target send test configuration
  RTI_RxEnableReq(RTI_RX_ENABLE_ON);
#endif
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

  if (RSA_STATE_PAIR == rsaState)
  {
    // turn off transmission light indicator
    HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);

    rsaState = RSA_STATE_READY;
  }
}

/**************************************************************************************************
 *
 * @fn      RTI_AllowPairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_AllowPairReq API
 *          call. The client is expected to complete this function.
 *
 *          NOTE: It is possible that this call can be made to the RTI client
 *                before the call to RTI_AllowPairReq has returned.
 * @param   status   - Result of RTI_PairReq API call.
 * @param   dstIndex - Pairing table entry of paired device, or invalid
 * @param   devType  - Pairing table index device type, or invalid
 *
 * @return  void
 */
void RTI_AllowPairCnf( rStatus_t status, uint8 dstIndex, uint8 devType )
{
  // Do nothing.
  // Controller does not trigger AllowPairReq() and hence is not expecting this callback.

  // unused arguments
  (void) status;
  (void) dstIndex;
  (void) devType;
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
  (void) dstIndex;

  // RTI has been successfully started, so application is now ready to go
  rsaState = RSA_STATE_READY;

  // select initial target device type
  // currently, this routine finds the first pairing entry index, if one exists
  rsaSelectInitialTargetDevice();
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
  // unused arguments
  (void) dstIndex;
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
  if ( status == RTI_SUCCESS )
  {
    if (rsaState == RSA_STATE_NDATA)
    {
      HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
      rsaState = RSA_STATE_READY;
    }
    else if (rsaState == RSA_STATE_TEST)
    {
      uint32 time;

      // Record latecy
      rsaSendCnfTimeStamp = osal_GetSystemClock();
      time = rsaSendCnfTimeStamp - rsaSendReqTimeStamp;
      if (time < 1000)
      {
        if (time < 100)
        {
          rsaTestReportCache.latency.bin[time/10]++;
        }
        else
        {
          rsaTestReportCache.latency.bin[(time-100)/50 + 10]++;
        }
      }
      else
      {
        rsaTestReportCache.latency.bin[28]++;
      }

      if (rsaTestReportCache.latency.numPackets ==
          rsaTestReqCache.numPackets)
      {
        rsaState = RSA_STATE_TEST_COMPLETED;
        // TURN ON LED
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
      }
      else
      {
        rsaRRTSendData();
      }
    }
    else if (rsaState == RSA_STATE_TEST_COMPLETED)
    {
      rsaState = RSA_STATE_TEST;
      HalLedBlink(HAL_LED_1, 0, 50, 2000);
    }
  }
  else
  {
    if (rsaState == RSA_STATE_NDATA)
    {
      HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
      rsaState = RSA_STATE_READY;
    }
    else if (rsaState == RSA_STATE_TEST)
    {
      rsaTestReportCache.latency.bin[28]++;

      if (rsaTestReportCache.latency.numPackets ==
          rsaTestReqCache.numPackets)
      {
        rsaState = RSA_STATE_TEST_COMPLETED;
        // TURN ON LED
        HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
      }
      else
      {
        rsaRRTSendData();
      }
    }
  }
}

/**************************************************************************************************
 *
 * @fn      RTI_RxEnableCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_RxEnableReq API
 *          call. The client is expected to complete this function.
 *
 * @param   status - Result of RTI_EnableRxReqReq API call.
 *
 * @return  void
 */
void RTI_RxEnableCnf( rStatus_t status )
{
  // Do nothing

  (void) status; // unused argument
}

/**************************************************************************************************
 *
 * @fn      RTI_EnableSleepCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_EnableSleepReq API
 *          call. The client is expected to complete this function.
 *
 * @param   status - Result of RTI_EnableSleepReq API call.
 *
 * @return  void
 *
 */
void RTI_EnableSleepCnf( rStatus_t status )
{
  // Do nothing

  (void) status; // unused argument
}

/**************************************************************************************************
 *
 * @fn      RTI_DisableSleepCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_DisableSleepReq API
 *          call. The client is expected to complete this function.
 *
 * @param   status - Result of RTI_EnableSleepReq API call.
 *
 * @return  void
 *
 */
void RTI_DisableSleepCnf( rStatus_t status )
{
  // Do nothing

  (void) status; // unused argument
}

/**************************************************************************************************
 *
 * @fn      RTI_ReceiveDataInd
 *
 * @brief   RTI receive data indication callback asynchronous initiated by
 *          another node. The client is expected to complete this function.
 *
 * input parameters
 *
 * @param   srcIndex:  Pairing table index.
 * @param   profileId: Profile identifier.
 * @param   vendorId:  Vendor identifier.
 * @param   rxLQI:     Link Quality Indication.
 * @param   rxFlags:   Receive flags.
 * @param   len:       Number of bytes to send.
 * @param   *pData:    Pointer to data to be sent.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 *
 */
void RTI_ReceiveDataInd(uint8 srcIndex, uint8 profileId, uint16 vendorId, uint8 rxLQI,
                                                           uint8 rxFlags, uint8 len, uint8 *pData)
{
  (void)rxLQI;

  if (profileId == RTI_PROFILE_ZID)
  {
    uint8 cmd = pData[ZID_FRAME_CTL_IDX] & GDP_HEADER_CMD_CODE_MASK;

    if (cmd == ZID_CMD_GET_REPORT)
    {
      rxGetReport(srcIndex, pData);
    }
    else if (cmd == ZID_CMD_REPORT_DATA)
    {
      zid_report_data_cmd_t *pReport = (zid_report_data_cmd_t *)pData;
      zid_report_record_t *pRecord = (zid_report_record_t *)&pReport->reportRecordList[0];
      zid_keyboard_data_t *pKeyboard = (zid_keyboard_data_t *)(&pRecord->data[0]);

      if (pRecord->id == ZID_STD_REPORT_KEYBOARD)
      {
        if (pKeyboard->mods != 0)
        {
          /* Tell OSAL to not go to sleep because buzzer uses T3 */
          osal_pwrmgr_task_state( RSA_TaskId, PWRMGR_HOLD );

          /* Ring buzzer */
          HalBuzzerRing( 40, RSA_BuzzerCompleteCback ); // ring buzzer for 500msec
        }
      }
    }
  }
  else if (len && (vendorId == RTI_VENDOR_TEXAS_INSTRUMENTS) &&
                    (rxFlags & RTI_RX_FLAGS_VENDOR_SPECIFIC))
  {
    if (pData[0] == RTI_PROTOCOL_TEST && rsaState == RSA_STATE_TEST)
    {
      if (pData[1] == RTI_CMD_TEST_PARAMETERS)
      {
        (void)osal_memcpy(&rsaTestReqCache, &pData[2], (sizeof(rrtTestReq_t)));
      }
    }
#ifdef FEATURE_OAD
    else if (pData[0] == RTI_PROTOCOL_OAD &&
             (rsaState == RSA_STATE_READY || rsaState == RSA_STATE_OAD))
    {
      // Profile identifier is not used for TI vendor specific commands
      // as proper use of profile identifier is questionable.
      OAD_ReceiveDataInd(srcIndex, len, pData);
      if (OAD_State == OAD_CLIENT_IDLE_STATE)
      {
        // Set the state back to non-OAD state
        rsaState = RSA_STATE_READY;
        // Stop OAD inactivity watchdog timer
        (void)osal_stop_timerEx(RSA_TaskId, RSA_EVT_OAD_INACTIVITY);
      }
      else
      {
        rsaState = RSA_STATE_OAD;
        if (OAD_State == OAD_CLIENT_REBOOT)
        {
          // Delay a bit till last OAD message is passed to server and reboot
          (void)osal_start_timerEx(RSA_TaskId, RSA_EVT_REBOOT, 1000);
        }
        else
        {
          // (re-)start inactivity watchdog timer
          // This shall be application specific timer but has to be bigger
          // than longest inactivity of OAD protocol, which is usually the time
          // it takes for OAD client to calculate CRC and send back enable confirm.
          (void)osal_start_timerEx(RSA_TaskId, RSA_EVT_OAD_INACTIVITY, 5000);
        }
      }
    }
#endif
  }
}

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
/**************************************************************************************************
 *
 * @fn      RSA_KeyCback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - key that was pressed (i.e. the scanned row/col index)
 *          state - shifted
 *
 * @return  void
 */
void RSA_KeyCback(uint8 keys, uint8 state)
{
  static uint8 rsaLastKey = HAL_KEY_CODE_NOKEY;
  static uint8 rsaLastCmd = HID_KEYBOARD_RESERVED;
  uint8 cmd = rsaKeyMap[keys].key; // gets the key code based on the row/col index
  uint8 profile = rsaKeyMap[keys].profile;
  bool sendMouseReport = FALSE;

  (void) state; // unused argument

  if (keys == HAL_KEY_CODE_NOKEY)
  {
    cmd = HID_KEYBOARD_RESERVED;  // Overwrite the invalid cmd lookup on key release.
    rsaKeyRepeated = 0;
    if (rsaMouseButtonStates != 0)
    {
      /* While mouse buttons are sensed as buttons, they are reported in
       * ZID Mouse Reports, which are sent from the RSA_MotionSensorCback
       * function. The global "rsaMouseButtonStates" keeps track of the state
       * of the mouse buttons, and this callback function simply sets/clears
       * the state of these buttons.
       */
      rsaLastKey = HAL_KEY_CODE_NOKEY;
      rsaMouseButtonStates = 0;
      sendMouseReport = TRUE;
    }
    else if (rsaLastCmd == HID_MOUSE_BUTTON_MIDDLE)
    {
      rsaAirMouseCursorCtlKeyRelease();
      rsaLastCmd = HID_KEYBOARD_RESERVED;
    }
  }
  else if (rsaLastKey == keys)
  {
    // same key is still pressed
    if (rsaKeyRepeated < 255)
    {
      rsaKeyRepeated++;
    }
  }
  else
  {
    rsaLastKey = keys;  // remember last row/col index
    rsaLastCmd = cmd;
    rsaKeyRepeated = 0;
  }

  // Check if we are in test mode since keys will have differnt mapping
  if (rsaState == RSA_STATE_TEST && !rsaKeyRepeated && keys != HAL_KEY_CODE_NOKEY)
  {
    // Check if toggle key is pressed and go back to normal if so
    if ( cmd == RSA_ACT_TEST_MODE )
    {
      // Toggle test mode
      rsaSpecialKeyActions[3]();
      return;
    }

    //perform test
    rsaRRTRunTest(cmd);
    return;
  }

  if (rsaState == RSA_STATE_TEST_COMPLETED && !rsaKeyRepeated)
  {
    if (cmd == HID_KEYBOARD_RETURN)
    {
      rsaRRTSendReport();
      return;
    }
  }

  if (RSA_STATE_PAIR == rsaState && RSA_ACT_PAIR == cmd && 0 == rsaKeyRepeated)
  {
    // Pairing already in progress
    // Pair key is used to toggle pairing action
    RTI_PairAbortReq();
    return;
  }

  /* Handle mouse button clicks */
  if (cmd == HID_MOUSE_BUTTON_LEFT)
  {
    rsaMouseButtonStates |= 0x01;
  }
  else if (cmd == HID_MOUSE_BUTTON_RIGHT)
  {
    rsaMouseButtonStates |= 0x02;
  }
  else if (cmd == HID_MOUSE_BUTTON_MIDDLE)
  {
    rsaAirMouseCursorCtlKeyPress();
    return;
  }

  if ((rsaState != RSA_STATE_READY) && (rsaState != RSA_STATE_INIT))
  {
    if (keys == HAL_KEY_CODE_NOKEY)
    {
      rsaLastKey = HAL_KEY_CODE_NOKEY;
    }

    // Keys are handled only in ready state
    return;
  }

  /* Mouse button clicks are simply recorded in the global "rsaMouseButtonStates"
   * and reported by the motion sensor callback function.
   */
  if (cmd == HID_MOUSE_BUTTON_LEFT)
  {
    sendMouseReport = TRUE;
  }
  else if (cmd == HID_MOUSE_BUTTON_RIGHT)
  {
    sendMouseReport = TRUE;
  }
  else if (cmd >= RSA_TGT_TYPE)
  {
    // Some non-ZRC keys are used to perform RTI tasks, so check that here.
    // Select a target device type
    rsaTgtDevType = cmd - RSA_TGT_TYPE;
    // Toggle key action will switch to the proper target device
    // in addtional it would toggle between targets with the same matching device type.
    rsaTargetDeviceToggleKeyAction();

    // Reset target device type so that generic device toggle key would work
    // with any device types.
    rsaTgtDevType = RTI_DEVICE_RESERVED_FOR_WILDCARDS;
  }
  else if (cmd >= RSA_ACT_START)
  {
    // action table command
    rsaSpecialKeyActions[cmd - RSA_ACT_START]();
  }

  // else we have a valid key command or TI extended TV command,
  // so as long as we are ready, and currently paired with a target, process the key command
  else if ( (sendMouseReport == FALSE) &&
            (rsaState == RSA_STATE_READY) &&
            (rsaDestIndex != RTI_INVALID_PAIRING_REF) )
  {
    uint8 key;

    // first check if the key command is power toggle, and handle as a special case
    if (cmd == RTI_CERC_POWER_TOGGLE_FUNCTION)
    {
      // power toggle key should not repeat and hence as toggle key is
      // repeatedly pressed, it should be ignored.
      if ( rsaKeyRepeated )
      {
        return;
      }
    }
    // If STOP key is pressed for 5 seconds, clear the pairing table
    if ( cmd == RTI_CERC_STOP && rsaKeyRepeated >= 100)
    {
      // 5 seconds upon pressing stop key, clear pairing table
      uint8 value8 = CLEAR_STATE;
      (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_CP_ITEM_STARTUP_CTRL, 1, &value8);

#ifdef FEATURE_OAD
      OAD_Reboot();
#else
      HAL_DISABLE_INTERRUPTS();
      RTI_SwResetReq();
      while (1)
      {
        asm("NOP");
      }
#endif // FEATURE_OAD
    }

    if (keys == HAL_KEY_CODE_NOKEY)
    {
      key = 0; // indicate no key pressed anymore
      rsaKeyPressed = FALSE;
      cmd = rsaKeyMap[rsaLastKey].key;  // Gets the last key code based on the row/col index.
      profile = rsaKeyMap[rsaLastKey].profile;
      rsaLastKey = HAL_KEY_CODE_NOKEY;
      if (cmd == RTI_CERC_POWER_TOGGLE_FUNCTION)
      {
        // no key release sent for power button
        return;
      }
    }
    else  // key pressed
    {
      key = cmd; // insert key that was pressed
      rsaKeyPressed = TRUE;
    }

    /* Only send ZID keys */
    if ((profile == RTI_PROFILE_ZID) &&
         GET_BIT(rsaPairingEntryBuf.profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      rsaBuildAndSendZidKeyboardReport( key );
    }
    else if ((profile == RTI_PROFILE_ZRC) &&
             GET_BIT(rsaPairingEntryBuf.profileDiscs, RCN_PROFILE_DISC_ZRC))
    {
      rsaBuildAndSendZrcReport( cmd, rsaKeyPressed, rsaKeyRepeated );
    }
  }
  else if (keys == HAL_KEY_CODE_NOKEY)
  {
    rsaLastKey = HAL_KEY_CODE_NOKEY;
  }

  if ((sendMouseReport == TRUE) &&
      (rsaMotionDetectorState != RSA_MOTION_DETECTOR_ENABLED) &&
       GET_BIT(rsaPairingEntryBuf.profileDiscs, RCN_PROFILE_DISC_ZID))
  {
    rsaBuildAndSendZidMouseReport( rsaMouseButtonStates, 0, 0 );
  }
}

static void rsaBuildAndSendZrcReport( uint8 cmd, bool keyPressed, bool keyRepeated )
{
  uint8 len = 1;
  uint8 pData[5];  // Note that array size has to match maximum possible data length
  uint8 txOptions = RTI_TX_OPTION_ACKNOWLEDGED | RTI_TX_OPTION_SECURITY;
  uint8 profileId = RTI_PROFILE_ZRC;
  uint16 vendorId = RTI_VENDOR_TEXAS_INSTRUMENTS;

  if (keyPressed == FALSE)
  {
    pData[0] = RTI_CERC_USER_CONTROL_RELEASED;
  }
  else if (keyRepeated)
  {
    pData[0] = RTI_CERC_USER_CONTROL_REPEATED;
  }
  else if (cmd >= RSA_EXT_TV)  // TI vendor specific command for TV extension
  {
    pData[0] = RTI_PROTOCOL_EXT_TV_RC;
  }
  else  // ZRC control command.
  {
    pData[0] = RTI_CERC_USER_CONTROL_PRESSED;
  }

  if (pData[0] == RTI_PROTOCOL_EXT_TV_RC)
  {
    pData[1] = cmd - RSA_EXT_TV;
    len = 2;
    txOptions |= RTI_TX_OPTION_VENDOR_SPECIFIC;
  }
  else if ((pData[0] == RTI_CERC_USER_CONTROL_PRESSED) ||
           (cmd < RSA_EXT_TV))  // CERC command repeat/release.
  {
    len++; // at least one more byte for RC command code

    // check if this key command is mapped to mutiple col/row indexes,
    //   which indicates multiple CERC subfunctions
    if ( cmd >= RSA_MEDIA_SEL )
    {
      pData[1] = RTI_CERC_SELECT_MEDIA_FUNCTION;

      // the select media function requires a media number, so increase
      //   length by one to handle the payload
      len++;

      // now fill in any extra payload
      pData[2] = cmd - RSA_MEDIA_SEL;
    }
    else
    {
      // set up plain RC command code
      pData[1] = cmd;
    }
  }

  // sending data state
  rsaState = RSA_STATE_NDATA;

  // send the data over the air
  RTI_SendDataReq( rsaDestIndex, profileId, vendorId, txOptions, len, pData);
}

/**************************************************************************************************
 *
 * @fn      RSA_CalCompleteCback
 *
 * @brief   Callback service to inform app of completion of motion sensor calibration
 *
 * @param   none
 *
 * @return  void
 */
void RSA_CalCompleteCback( void )
{
  /* Provide feedback that calibration is complete */
#if (defined HAL_BUZZER) && (HAL_BUZZER == TRUE)
  /* Tell OSAL to not go to sleep because buzzer uses T3 */
  osal_pwrmgr_task_state( RSA_TaskId, PWRMGR_HOLD );

  /* Ring buzzer */
  HalBuzzerRing( 500, RSA_BuzzerCompleteCback ); // ring buzzer for 500msec
#endif
}

/**************************************************************************************************
 *
 * @fn      RSA_BuzzerCompleteCback
 *
 * @brief   Callback function called when ringing of buzzer is done
 *
 * @param   none
 *
 * @return  void
 */
void RSA_BuzzerCompleteCback( void )
{
#if (defined HAL_BUZZER) && (HAL_BUZZER == TRUE)
  /* Tell OSAL it's OK to go to sleep */
  osal_pwrmgr_task_state( RSA_TaskId, PWRMGR_CONSERVE );
#endif
}

/**************************************************************************************************
 *
 * @fn      RSA_MotionSensorCback
 *
 * @brief   Callback service for motion sensor samples
 *
 * @param   gyroMickeysX - X-axis movement in units of "Mickeys" (approx 1/200 inch per Mickey)
 *          gyroMickeysY - Y-axis movement
 *
 * @return  void
 */
void RSA_MotionSensorCback( int16 gyroMickeysX, int16 gyroMickeysY )
{
  /* used to see if any changes in mouse buttons */
  static uint8 savedMouseButtonStates = 0;

  /* Check for no motion for 10 seconds. If true, then place motion detection
   * hardware in standby mode.
   */
  if ((gyroMickeysX == 0) && (gyroMickeysY == 0))
  {
    rsaNoMotionCount += 1;
    if (rsaNoMotionCount >= 1000) // samples arrive every 10 msec
    {
      HalMotionStandby();
      rsaMotionDetectorState = RSA_MOTION_DETECTOR_STANDBY;
      rsaNoMotionCount = 0;
      osal_start_timerEx(RSA_TaskId, RSA_EVT_MOTION_SENSOR_TIMER, RSA_EXIT_MOTION_DETECT_TIMEOUT);
      rsaNoMotionTimerRunning = TRUE;
    }
  }
  else
  {
    rsaMotionDetectorState = RSA_MOTION_DETECTOR_ENABLED;
    rsaNoMotionCount = 0;
    if (rsaNoMotionTimerRunning == TRUE)
    {
      /* cancel motion sensor poweroff timer */
      osal_stop_timerEx(RSA_TaskId, RSA_EVT_MOTION_SENSOR_TIMER);
      rsaNoMotionTimerRunning = FALSE;
    }
  }

  /* Only send a mouse report if we are paired with a target and not currently
   * communicating with it.
   */
  if ((rsaDestIndex != RTI_INVALID_PAIRING_REF) && (rsaState == RSA_STATE_READY))
  {
    /* Make sure movement data is within bounds of report fields */
    if (gyroMickeysX > ZID_MOUSE_DATA_MAX)
    {
      gyroMickeysX = ZID_MOUSE_DATA_MAX;
    }
    else if (gyroMickeysX < -ZID_MOUSE_DATA_MAX)
    {
      gyroMickeysX = -ZID_MOUSE_DATA_MAX;
    }
    if (gyroMickeysY > ZID_MOUSE_DATA_MAX)
    {
      gyroMickeysY = ZID_MOUSE_DATA_MAX;
    }
    else if (gyroMickeysY < -ZID_MOUSE_DATA_MAX)
    {
      gyroMickeysY = -ZID_MOUSE_DATA_MAX;
    }

    /* Only process mouse data if connected to a ZID capable target */
    if (GET_BIT(rsaPairingEntryBuf.profileDiscs, RCN_PROFILE_DISC_ZID))
    {
      /* Only send the report if something meaningful to report */
      if ((rsaMouseButtonStates != savedMouseButtonStates) || // mouse button change
          ((rsaBlockMouseMovements == FALSE) && // mouse movements are allowed
          (rsaKeyPressed == FALSE) && // freeze mouse movements if key pressed
          ((gyroMickeysX != 0) || // mouse movement was detected
          (gyroMickeysY != 0))))
      {
        rsaBuildAndSendZidMouseReport( rsaMouseButtonStates,
                                       (int8)gyroMickeysX,
                                       (int8)gyroMickeysY );
        savedMouseButtonStates = rsaMouseButtonStates;
      }
    }
  }
}

/**************************************************************************************************
 *
 * @fn      rsaBuildAndSendZidMouseReport
 *
 * @brief   Send a ZID profile mouse report to target
 *
 * @param   mouseStates - bitmap of left/middle/right mouse button states
 *          mickeysX - amount of mouse movement along X-axis in units of Mickeys (1/200 of an inch)
 *          mickeysY - amount of mouse movement along Y-axis
 *
 * @return  void
 */
static void rsaBuildAndSendZidMouseReport( uint8 mouseStates, int8 mickeysX, int8 mickeysY )
{
  uint8 buf[ZID_MOUSE_DATA_LEN + sizeof(zid_report_data_cmd_t) + sizeof(zid_report_record_t)];
  zid_report_data_cmd_t *pReport = (zid_report_data_cmd_t *)buf;
  zid_report_record_t *pRecord = (zid_report_record_t *)&pReport->reportRecordList[0];
  zid_mouse_data_t *pMouse = (zid_mouse_data_t *)(&pRecord->data[0]);
  // Set reliable control pipe mode for mouse click (i.e. like a keyboard key press).
  uint8 txOptions = (mouseStates==0) ? ZID_TX_OPTIONS_INTERRUPT_PIPE : ZID_TX_OPTIONS_CONTROL_PIPE;

  /* Rapid mouse movements should go OTA via Interrupt transmission model, no broadcast */
  pReport->cmd = ZID_CMD_REPORT_DATA;
  pRecord->len = sizeof(zid_report_record_t) + ZID_MOUSE_DATA_LEN - 1;
  pRecord->type = ZID_REPORT_TYPE_IN;
  pRecord->id = ZID_STD_REPORT_MOUSE;
  pMouse->btns = mouseStates;
  pMouse->x = mickeysX;
  pMouse->y = mickeysY;

  /* Send report */
  rsaState = RSA_STATE_NDATA;
  RTI_SendDataReq( rsaDestIndex,
                   RTI_PROFILE_ZID,
                   RTI_VENDOR_TEXAS_INSTRUMENTS,
                   txOptions,
                   sizeof( buf ),
                   buf );
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
static void rsaBuildAndSendZidKeyboardReport( uint8 key )
{
  uint8 buf[ZID_KEYBOARD_DATA_LEN + sizeof(zid_report_data_cmd_t) + sizeof(zid_report_record_t)];
  zid_report_data_cmd_t *pReport = (zid_report_data_cmd_t *)buf;
  zid_report_record_t *pRecord = (zid_report_record_t *)&pReport->reportRecordList[0];
  zid_keyboard_data_t *pKeyboard = (zid_keyboard_data_t *)(&pRecord->data[0]);
  uint8 txOptions = ZID_TX_OPTIONS_CONTROL_PIPE;

  /* Keyboard reports should go OTA via reliable Control transmission model, no broadcast */
  pReport->cmd = ZID_CMD_REPORT_DATA;
  pRecord->len = sizeof(zid_report_record_t) + ZID_KEYBOARD_DATA_LEN - 1;
  pRecord->type = ZID_REPORT_TYPE_IN;
  pRecord->id = ZID_STD_REPORT_KEYBOARD;
  pKeyboard->mods = 0; // no keyboard modifiers for now
  pKeyboard->reserved = 0;
  pKeyboard->keys[0] = key;
  pKeyboard->keys[1] = 0;
  pKeyboard->keys[2] = 0;
  pKeyboard->keys[3] = 0;
  pKeyboard->keys[4] = 0;
  pKeyboard->keys[5] = 0;

  /* Send report */
  rsaState = RSA_STATE_NDATA;
  RTI_SendDataReq( rsaDestIndex,
                   RTI_PROFILE_ZID,
                   RTI_VENDOR_TEXAS_INSTRUMENTS,
                   txOptions,
                   sizeof( buf ),
                   buf );
}
#endif

/**************************************************************************************************
 * @fn      rsaConfig
 *
 * @brief   This function configures RTI parameters relevant to the RSA.
 *
 * @param
 *
 * None.
 *
 * @return  void
 *
 */
static void rsaConfig(void)
{
  uint8 *pData = osal_mem_alloc( aplcMaxNonStdDescCompSize );

  (void)osal_memcpy( pData, tgtList, RTI_MAX_NUM_SUPPORTED_TGT_TYPES );
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_NODE_SUPPORTED_TGT_TYPES, RTI_MAX_NUM_SUPPORTED_TGT_TYPES, pData );

  // No User String pairing; 1 Device (Remote Control); 2 Profiles (ZID & ZRC)
  pData[0] = RTI_BUILD_APP_CAPABILITIES(0, 1, 2);
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_CAPABILITIES, 1, pData );

  (void)osal_memcpy( pData, devList, RTI_MAX_NUM_DEV_TYPES );
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_DEV_TYPE_LIST, RTI_MAX_NUM_DEV_TYPES, pData );

  (void)osal_memcpy( pData, profileList, RTI_MAX_NUM_PROFILE_IDS );
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_APPL_PROFILE_ID_LIST, RTI_MAX_NUM_PROFILE_IDS, pData);

  // Controller Type; No A/C Pwr; Security capable; Channel Normalization capable.
  pData[0] = RTI_BUILD_NODE_CAPABILITIES(0, 0, 1, 1);
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_NODE_CAPABILITIES, 1, pData );

  *(uint16 *)pData = RTI_VENDOR_TEXAS_INSTRUMENTS;
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_ID, 2, pData );

  (void)osal_memcpy( pData, vendorName, sizeof(vendorName) );
  (void)RTI_WriteItemEx( RTI_PROFILE_RTI, RTI_CP_ITEM_VENDOR_NAME, sizeof(vendorName), pData );

  /* Now fill in class device config info */
  pData[0] = 2;
  RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_HID_NUM_STD_DESC_COMPS, 1, pData );
  pData[0] = ZID_STD_REPORT_MOUSE;
  pData[1] = ZID_STD_REPORT_KEYBOARD;
  RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_HID_STD_DESC_COMPS_LIST, 2, pData );

  *(uint16 *)pData = 0x1C63; // RemoTI ZID Advanced Remote
  RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_HID_PRODUCT_ID, 2, pData );

#ifdef ZID_IOT
  {
    uint8 i;

    /* init key exchange transfer count to something small so key ex doesn't dominate logs */
    pData[0] = 5;
    RTI_WriteItemEx( RTI_PROFILE_GDP, GDP_ITEM_KEY_EX_TRANSFER_COUNT, 1, pData );

    /* Write the number of non-std descriptors defined */
    pData[0] = 2;
    RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_HID_NUM_NON_STD_DESC_COMPS, 1, pData );

    /* Write a non-std descriptor that needs to be fragmented */
    pData[0] = ZID_DESC_TYPE_REPORT;
    pData[1] = 100; // length is 10
    pData[2] = 0;
    pData[3] = 0xBE;
    for (i = 0; i < 100; i++)
    {
      pData[4 + i] = i;
    }
    (void)zidCldAppHlp_WriteNonStdDescComp( 0, (zid_non_std_desc_comp_t *)pData );

    /* Write the 2nd non-std descriptor */
    pData[0] = ZID_DESC_TYPE_PHYSICAL;
    pData[1] = 2;
    pData[2] = 0;
    pData[3] = 0x00;
    pData[4] = 0xA1;
    pData[5] = 0xA2;
    (void)zidCldAppHlp_WriteNonStdDescComp( 1, (zid_non_std_desc_comp_t *)pData );

    /* Set up Null reports for the non std descriptor corresponding to a report */
    pData[0] = 1;
    RTI_WriteItemEx( RTI_PROFILE_ZID, ZID_ITEM_HID_NUM_NULL_REPORTS, 1, pData );

    /* Write a non-std descriptor that needs to be fragmented */
    pData[0] = 0xBE; // corresponds to report Id of non std descriptor
    pData[1] = 16;
    for (i = 0; i < 16; i++)
    {
      pData[2 + i] = i + 100;
    }
    (void)zidCldAppHlp_WriteNullReport( 0, (zid_null_report_t *)pData );
  }
#endif

  // lastly, free the scratch memory
  osal_mem_free( pData );
}

/**************************************************************************************************
 * @fn          rsaSelectInitialTargetDevice
 *
 * @brief       This routine is for the case where the RC boots, and there already are
 *              pairing table entries, so which target does the RC pair with? This routine chooses
 *              the last valid pairing entry.
 * Note: this logic of selection may not be very useful when the pairing table size is huge.
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
static void rsaSelectInitialTargetDevice(void)
{
  uint8 maxIdx = rsaMaxNumPairingEntries;
  rsaDestIndex = RTI_INVALID_PAIRING_REF;

  for (uint8 i = 0; i < maxIdx; i++)
  {
    (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &i);

    // RTI_ReadItemEx succeeds only when pairing entry pointed by current entry index is valid.
    if (RTI_ReadItemEx(RTI_PROFILE_RTI,  RTI_SA_ITEM_PT_CURRENT_ENTRY,
                          sizeof(rsaPairingEntryBuf), (uint8 *)&rsaPairingEntryBuf) == RTI_SUCCESS)
    {
      rsaDestIndex = i;  // Latch the last valid entry.
    }
  }

  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &rsaDestIndex);
}

/**************************************************************************************************
 * @fn          rsaPairKeyAction
 *
 * @brief       This function performs action required when a pair trigger key is depressed.
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
static void rsaPairKeyAction(void)
{
  if (0 == rsaKeyRepeated)
  {
    // pair key only acts upon key press.
    // Allowing key depress to repeat pairing would conflict with pair toggling
    rsaState = RSA_STATE_PAIR;

    // IR generation uses the same pin for LED_2, hence do not use LED_2
    HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);

    // begin pairing
    RTI_PairReq();
  }
}

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
/**************************************************************************************************
 * @fn          rsaAirMouseCalKeyAction
 *
 * @brief       This function performs action required when the Air Mouse key is depressed.
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
static void rsaAirMouseCalKeyAction( void )
{
  if (rsaKeyRepeated == 0) // only act on initial keypress detection
  {
    HalMotionCal( RSA_CalCompleteCback, RSA_CAL_DURATION );
  }
}

/**************************************************************************************************
 * @fn          rsaAirMouseResolutionDecrease
 *
 * @brief       This function decreases the resolution (i.e. speed) of air mouse movement.
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
static void rsaAirMouseResolutionDecrease( void )
{
  /* Simply call HAL function to modify resolution */
  if (rsaKeyRepeated == 0)
  {
    HalMotionModifyAirMouseResolution( HAL_MOTION_RESOLUTION_DECREASE );
  }
}

/**************************************************************************************************
 * @fn          rsaAirMouseResolutionIncrease
 *
 * @brief       This function increases the resolution (i.e. speed) of air mouse movement.
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
static void rsaAirMouseResolutionIncrease( void )
{
  /* Simply call HAL function to modify resolution */
  if (rsaKeyRepeated == 0)
  {
    HalMotionModifyAirMouseResolution( HAL_MOTION_RESOLUTION_INCREASE );
  }
}

#ifdef ZID_IOT
static void rsaMouseTxOptionSet( void )
{
  if (rsaKeyRepeated == 0) // only act on initial keypress detection
  {
    if (mouseTxOptions == ZID_TX_OPTIONS_CONTROL_PIPE)
    {
      mouseTxOptions = ZID_TX_OPTIONS_CONTROL_PIPE_BROADCAST;
    }
    else if (mouseTxOptions == ZID_TX_OPTIONS_CONTROL_PIPE_BROADCAST)
    {
      mouseTxOptions = ZID_TX_OPTIONS_CONTROL_PIPE_NO_SECURITY;
    }
    else if (mouseTxOptions == ZID_TX_OPTIONS_CONTROL_PIPE_NO_SECURITY)
    {
      mouseTxOptions = ZID_TX_OPTIONS_INTERRUPT_PIPE_WITH_SECURITY;
    }
    else if (mouseTxOptions == ZID_TX_OPTIONS_INTERRUPT_PIPE_WITH_SECURITY)
    {
      mouseTxOptions = ZID_TX_OPTIONS_INTERRUPT_PIPE;
    }
    else
    {
      mouseTxOptions = ZID_TX_OPTIONS_CONTROL_PIPE;
    }
  }
}
#endif

/**************************************************************************************************
 * @fn          rsaAirMouseCursorCtlKeyPress
 *
 * @brief       This function performs action required when the Air Mouse control
 *              key is pressed.
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
static void rsaAirMouseCursorCtlKeyPress( void )
{
  if (rsaDestIndex != RTI_INVALID_PAIRING_REF) // only perform action if paired with target
  {
    if (rsaKeyRepeated == 0) // only act on initial keypress detection
    {
      /* cancel motion sensor poweroff timer */
      osal_stop_timerEx(RSA_TaskId, RSA_EVT_MOTION_SENSOR_TIMER);

      rsaNoMotionCount = 0;
      rsaBlockMouseMovements = FALSE;
      rsaDisableMouseMovementAfterKeyRelease = TRUE;
      rsaNoMotionTimerRunning = FALSE;
      if (rsaMotionDetectorState != RSA_MOTION_DETECTOR_ENABLED)
      {
        HalMotionEnable();
        rsaMotionDetectorState = RSA_MOTION_DETECTOR_ENABLED;
      }
    }
  }
}

/**************************************************************************************************
 * @fn          rsaAirMouseCursorCtlKeyRelease
 *
 * @brief       This function performs action required when the Air Mouse cursor control
 *              key is released.
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
static void rsaAirMouseCursorCtlKeyRelease( void )
{
  /* Releasing air mouse cursor control key. Perform double click processing. */
  if (rsaDoubleClickTimerRunning == TRUE)
  {
    /* key released again within double click time, so double click has occurred */
    rsaDisableMouseMovementAfterKeyRelease = FALSE;
  }
  else
  {
    /* Start double click and motion sensor power off timers */
    rsaDoubleClickTimerRunning = TRUE;
    osal_start_timerEx(RSA_TaskId, RSA_EVT_DOUBLE_CLICK_TIMER, 500);
    osal_start_timerEx(RSA_TaskId, RSA_EVT_MOTION_SENSOR_TIMER, 5000);
  }

  /* Now determine what we should do with mouse movements */
  if (rsaDisableMouseMovementAfterKeyRelease == TRUE)
  {
    rsaBlockMouseMovements = TRUE;
  }
}
#endif

/**************************************************************************************************
 * @fn          rsaUnpairKeyAction
 *
 * @brief       This function performs action required when un-pair trigger key is depressed.
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
static void rsaUnpairKeyAction(void)
{
  if (0 == rsaKeyRepeated)
  {
    // pair key only acts upon key press.
    // Allowing key depress to repeat pairing would conflict with pair toggling
    rsaState = RSA_STATE_UNPAIR;

    // begin pairing
    RTI_UnpairReq( rsaDestIndex );
  }
}

/**************************************************************************************************
 * @fn          rsaTargetDeviceToggleKeyAction
 *
 * @brief       This function performs action required when target device toggle key is depressed.
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
static void rsaTargetDeviceToggleKeyAction(void)
{
  uint8 maxCnt = rsaMaxNumPairingEntries - 1;
  uint8 maxIdx = rsaMaxNumPairingEntries;
  uint8 idx = (rsaDestIndex == RTI_INVALID_PAIRING_REF) ? 0 : rsaDestIndex;

  if (rsaKeyRepeated)  // Toggle-Key should not repeat; force user to release key and press again.
  {
    return;
  }

  for (uint8 cnt = 0; cnt < maxCnt; cnt++)
  {
    if (++idx == maxIdx)
  {
      idx = 0;
  }

    // Set current pairing table item to the current search index in order to read it.
    (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &idx);

    // RTI_ReadItemEx succeeds only when pairing entry pointed by current entry index is valid.
    if (RTI_ReadItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY,
                          sizeof(rsaPairingEntryBuf), (uint8 *)&rsaPairingEntryBuf) == RTI_SUCCESS)
    {
      if (rsaTgtDevType == RTI_DEVICE_RESERVED_FOR_WILDCARDS)
      {
        rsaDestIndex = idx;
        return;
    }

      for (uint8 type = 0; type < RCN_MAX_NUM_DEV_TYPES; type++)
    {
        if (rsaPairingEntryBuf.devTypeList[type] == rsaTgtDevType)
      {
          rsaDestIndex = idx;
          return;
        }
      }
    }
  }

  // No valid entry to toggle to, so restore the originally selected entry.
  (void)RTI_WriteItemEx(RTI_PROFILE_RTI, RTI_SA_ITEM_PT_CURRENT_ENTRY_INDEX, 1, &rsaDestIndex);
}

#ifdef FEATURE_OAD
/**************************************************************************************************
 * @fn          rsaPollKeyAction
 *
 * @brief       This function performs action required when poll key is depressed.
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
static void rsaPollKeyAction(void)
{
  uint8 data = RTI_PROTOCOL_POLL;

  if (rsaKeyRepeated == 0 && rsaState == RSA_STATE_READY &&
      rsaDestIndex != RTI_INVALID_PAIRING_REF)
  {
    // Turn on receiver for some duration
    RTI_RxEnableReq(RSA_POLL_TIMEOUT);

    // Send poll message
    // Note that if poll message were to be triggered periodically, it is better
    // to use unacknowledged transmission for power saving purpose.
    RTI_SendDataReq(rsaDestIndex,
                    OAD_PROFILE_ID,
                    OAD_VENDOR_ID,
                    OAD_TX_OPTIONS,
                    1,
                    &data);

    rsaState = RSA_STATE_NDATA;
  }
}
#endif // FEATURE_OAD

/**************************************************************************************************
 * @fn          rsaToggleTestModeKeyAction
 *
 * @brief       This function performs action required when toggle test mode key is depressed.
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
static void rsaToggleTestModeKeyAction(void)
{
  if (!rsaKeyRepeated)
  {
    if (rsaState != RSA_STATE_TEST)
    {
      // Turn on RX in case target send test configuration
      RTI_RxEnableReq(RTI_RX_ENABLE_ON);

      // slow blink LED to indicate ready for test
      HalLedBlink(HAL_LED_1, 0, 50, 2000);
      rsaState = RSA_STATE_TEST;

    }
    else if (rsaState == RSA_STATE_TEST)
    {
      // Turn off RX, going back to normal mode
      RTI_RxEnableReq(RTI_RX_ENABLE_OFF);

      // turn off LED indicating normal mode
      HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
      rsaState = RSA_STATE_READY;
    }
  }
}

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
/**************************************************************************************************
 * @fn          rsaRRTRunTest
 *
 * @brief       This function initiates automated testing
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
static void rsaRRTRunTest(uint8 cmd)
{
  //Reset stats in case test is run again
  (void)osal_memset(&rsaTestReportCache, 0, sizeof(rsaTestReportCache));

  // latency test
  if (cmd == HID_KEYBOARD_1)
  {
    rsaTestReportCache.testType = RSA_TEST_LATENCY;
    HalLedBlink(HAL_LED_1, 0, 50, 500);
    rsaRRTSendData();
  }
}
#endif

/**************************************************************************************************
 * @fn          rsaRRTSendData
 *
 * @brief       This function sends a test data
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
static void rsaRRTSendData(void)
{
   if (rsaTestReqCache.maxBackoffDuration > 0)
   {
    // Not a good random number generation mechanism
    // but this will do as a sample application
    uint16 seed = osal_rand();

    (void)osal_start_timerEx(RSA_TaskId,
                       RSA_EVT_RANDOM_BACKOFF_TIMER,
                       (seed % rsaTestReqCache.maxBackoffDuration) + 1);

   }
   else
   {
     (void)osal_set_event(RSA_TaskId, RSA_EVT_RANDOM_BACKOFF_TIMER);
   }
}

#if (defined HAL_MOTION) && (HAL_MOTION == TRUE)
static void rsaRRTSendReport(void)
{
  uint8 pData[sizeof(rsaTestReportCache)+2];
  uint8 profileId, vendorId;

  profileId = RTI_PROFILE_ZRC;
  vendorId  = RTI_VENDOR_TEXAS_INSTRUMENTS;
  pData[0] = RTI_PROTOCOL_TEST;
  pData[1] = RTI_CMD_TEST_REPORT;

  (void)osal_memcpy(&pData[2], &rsaTestReportCache, sizeof(rsaTestReportCache));

  RTI_SendDataReq( rsaDestIndex, profileId, vendorId, rsaTestReqCache.txOptions, sizeof(pData), pData);
}
#endif


/**************************************************************************************************
 * @fn          rxGetReport
 *
 * @brief       Process a Get Report command frame.
 *
 * input parameters
 *
 * @param   srcIndex -  Pairing table index.
 * @param   *pData   - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void rxGetReport(uint8 srcIndex, uint8 *pData)
{
  uint8 buf[16], len = sizeof(zid_report_data_cmd_t);
  // ZID co-layer would not allow to pass up to here if not a Mouse or Keyboard GetReport.
  uint8 id = ((zid_get_report_cmd_t *)pData)->id;
  zid_report_data_cmd_t *pRsp = (zid_report_data_cmd_t *)buf;
  zid_report_record_t *pRecord = &pRsp->reportRecordList[0];
  pRsp->cmd = ZID_CMD_REPORT_DATA;
  pRecord->type = ZID_REPORT_TYPE_IN;
  pRecord->id = id;
  pRecord->len = sizeof(zid_report_record_t) + ZID_StdReportLen[id] - 1;

  len += ZID_StdReportLen[id] + sizeof(zid_report_record_t);

  if (id == ZID_STD_REPORT_MOUSE)
  {
    zid_mouse_data_t *pMouse = (zid_mouse_data_t *)(&pRecord->data[0]);

    pMouse->btns = 0;  // no button -> click release.
    pMouse->x = 0;
    pMouse->y = 0;
  }
  else // if (id == ZID_STD_REPORT_KEYBOARD)
  {
    zid_keyboard_data_t *pKeyboard = (zid_keyboard_data_t *)(&pRecord->data[0]);

    pKeyboard->mods = 0;     // no keyboard modifiers for now
    pKeyboard->keys[0] = 0;  // no key -> key release.
    pKeyboard->keys[1] = 0;
    pKeyboard->keys[2] = 0;
    pKeyboard->keys[3] = 0;
    pKeyboard->keys[4] = 0;
    pKeyboard->keys[5] = 0;
  }

  rsaState = RSA_STATE_NDATA;
  RTI_SendDataReq(srcIndex, RTI_PROFILE_ZID, RTI_VENDOR_TEXAS_INSTRUMENTS,
                                              ZID_TX_OPTIONS_CONTROL_PIPE, len, buf);
}

/**************************************************************************************************
*/
