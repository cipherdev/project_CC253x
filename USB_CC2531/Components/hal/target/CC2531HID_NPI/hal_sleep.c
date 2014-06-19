/*
  Filename:       hal_sleep.c
*/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_board_cfg.h"
#include "hal_drivers.h"
#include "hal_sleep.h"
#include "mac_mcu.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

#if !defined NOP
#define NOP()  asm("NOP")
#endif

#if defined POWER_SAVING

// Sleep mode H/W definitions (enabled with POWER_SAVING compile option).
#define CC2530_PM1                         1  // 32 kHz OSC on, voltage regulator on.
#define CC2530_PM2                         2  // 32 kHz OSC on, voltage regulator off.
#define CC2530_PM3                         3  // 32-kHz OSC off, voltage regulator off.

// Interrupt enable bit masks.
#define STIE_BV                            BV(5)
#define P0IE_BV                            BV(5)
#define P1IE_BV                            BV(4)
#define P2IE_BV                            BV(1)

#define HAL_SLEEP_TIMER_ENABLE_INT()       st(IEN0 |= STIE_BV;)
#define HAL_SLEEP_TIMER_DISABLE_INT()      st(IEN0 &= ~STIE_BV;)
#define HAL_SLEEP_TIMER_CLEAR_INT()        st(STIF = 0;)

#define HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2) st( \
  ien0 = IEN0;  ien1 = IEN1;      ien2 = IEN2; \
  IEN0 = 0x80;  IEN1 &= P0IE_BV;  IEN2 &= (P1IE_BV | P2IE_BV); \
)

#define HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2)  st( IEN0 = ien0;  IEN1 = ien1;  IEN2 = ien2; )

// Convert OSAL msec ticks to 32-KHz ST ticks:
#define HAL_SLEEP_MS_TO_ST(MS)             ((((uint32)(MS)) * 4096) / 125)

// Convert MAC 320 usec ticks to 32-KHz ST ticks:
// 32768/3125 = 10.48576 and this is nearly 671/64 = 10.484375.
#define HAL_SLEEP_MAC_TO_ST(MAC)           ((((uint32)(MAC)) * 671) / 64)

// Data sheet recommends PM1 for sleep shorter than 3 msec.
#define HAL_SLEEP_MIN_FOR_PM2              (HAL_SLEEP_MS_TO_ST(3))

// Empirically determined run-time cost of the parts of hal_sleep():
#define HAL_SLEEP_ADJ_TICKS                (7 + 13)

// Data sheet recommends at least 5 ticks as the minimum delta for setting the compare.
// Also, recommendation was to not allow sleep for less than 2 msec.
#define HAL_SLEEP_MIN_TO_SET               (65 + HAL_SLEEP_ADJ_TICKS)

// Bit 7 is 32-KHz RC OSC calibration disable & data sheet specifies to always set bit 2.
#define SLEEPCMD_MASK                      (BV(2) | BV(7))

#define HAL_SLEEP_ST_GET(STCNT) st (    \
  uint32 T32 = 0;                \
                                 \
  do {                           \
    (STCNT) = T32;               \
    /* Get the sleep timer count; ST0 must be read first. */\
    ((uint8 *) &(T32))[0] = ST0; \
    ((uint8 *) &(T32))[1] = ST1; \
    ((uint8 *) &(T32))[2] = ST2; \
  } while (T32 != (STCNT));      \
)

#define HAL_SLEEP_ST_SET(STCNT) st (    \
  /* Set the sleep timer compare; ST0 must be written last. */\
  ST2 = ((uint8 *) &(STCNT))[2]; \
  ST1 = ((uint8 *) &(STCNT))[1]; \
  ST0 = ((uint8 *) &(STCNT))[0]; \
  while (!(STLOAD & LDRDY));  /* Wait for the sleep timer to load. */\
)
#endif

/* ------------------------------------------------------------------------------------------------
 *                                        Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/* PCON register value to program when setting power mode. All ISR's that are expected to wake chip
 * from sleep clear this variable to eliminate a race condition between setting sleep and servicing
 * the ISR that would wake from sleep.
 */
volatile __data uint8 halSleepPconValue;

#if defined POWER_SAVING
/* ------------------------------------------------------------------------------------------------
 *                                        Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static void halSleepTimer(uint32 timeout);

/* The instruction after the PCON instruction must not be 4-byte aligned.
 * The following code may cause excessive power consumption if not properly aligned.
 * See linker file ".xcl" for actual placement.
 */
#pragma location = "ALIGNED_CODE"
void halSleepExec(void);

#endif

/**************************************************************************************************
 * @fn          halSleep
 *
 * @brief       This function is called from the OSAL task loop using and existing OSAL
 *              interface.  It sets the low power mode of the MAC and the CC2530.
 *
 * input parameters
 *
 * @param       osal_timeout - Next OSAL timer timeout.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleep(uint16 osal_timeout)
{
#if defined POWER_SAVING
  uint32 timeout;
  halDriverBegPM();

  if (osal_timeout == 0)
  {
    timeout = HAL_SLEEP_MAC_TO_ST((MAC_PwrNextTimeout()));
  }
  else
  {
    uint32 macTimeout = HAL_SLEEP_MAC_TO_ST((MAC_PwrNextTimeout()));
    timeout = HAL_SLEEP_MS_TO_ST(osal_timeout);

    if ((macTimeout != 0) && (macTimeout < timeout))  // Choose the lesser of the two timeouts.
    {
      timeout = macTimeout;
    }
  }

  if (((timeout == 0) || (timeout > HAL_SLEEP_MIN_TO_SET)) &&
       (MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) == MAC_SUCCESS))
  {
    // 4.4.2 System Clock
    // NOTE: The change from the 16-MHz clock source to the 32-MHz clock source (and vice versa)
    // aligns with the CLKCONCMD.TICKSPD setting. A slow CLKCONCMD.TICKSPD setting
    // when CLKCONCMD.OSC is changed results in a longer time before the actual source
    // change takes effect. The fastest switching is obtained when CLKCONCMD.TICKSPD equals 000.
    uint8 tickSpdShdw = CLKCONSTA & TICKSPD_MASK;
    CLKCONCMD &= (TICKSPD_MASK ^ 0xFF);

    // Save interrupt enable registers and disable all interrupts not waking from sleep.
    halIntState_t ien0, ien1, ien2;
    HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2);

    halSleepTimer(timeout);

    // 4.4.2 System Clock
    // NOTE: After coming up from PM1, PM2, or PM3, the CPU must wait for CLKCONSTA.OSC to be 0
    // before operations requiring the system to run on the 32-MHz XOSC (such as the radio).
    while ((CLKCONSTA & OSC) != CLKCONCMD_32MHZ);
    CLKCONCMD |= tickSpdShdw;

    MAC_PwrOnReq();  // Power on the MAC - blocks on hi-speed OSC sync.
    HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2);  // Restore interrupt enable registers.

    /* For CC2530, T2 interrupt won’t be generated when the current count is greater than
     * the comparator. The interrupt is only generated when the current count is equal to
     * the comparator. When the CC2530 is waking up from sleep, there is a small window
     * that the count may be grater than the comparator, therefore, missing the interrupt.
     * This workaround will call the T2 ISR when the current T2 count is greater than the
     * comparator. The problem only occurs when POWER_SAVING is turned on, i.e. the 32KHz
     * drives the chip in sleep and SYNC start is used.
     */
    macMcuTimer2OverflowWorkaround();
  }

  halDriverEndPM();
#else
  (void)osal_timeout;
#endif
}

#if defined POWER_SAVING
/**************************************************************************************************
 * @fn          halSleepTimer
 *
 * @brief       This function sets the PM mode according to the timeout;
 *              sets up the ST compare & ISR; and invokes halSleepExec().
 *
 * input parameters
 *
 * @param       timeout - Sleep time requested in 32-KHz ST ticks.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void halSleepTimer(uint32 timeout)
{
  SLEEPCMD = SLEEPCMD_MASK;  // Clear the sleep mode bits.

  if (timeout == 0)
  {
#if defined HAL_MCU_CC2531
    return;
#else
    SLEEPCMD |= CC2530_PM3;
#endif
  }
  else
  {
#if defined HAL_MCU_CC2531
    SLEEPCMD |= CC2530_PM1;
#else
    if (timeout < HAL_SLEEP_MIN_FOR_PM2)
    {
      SLEEPCMD |= CC2530_PM1;
    }
    else
    {
      SLEEPCMD |= CC2530_PM2;
    }
#endif
    uint32 stCmp;
    HAL_SLEEP_ST_GET(stCmp);
    stCmp += (timeout - HAL_SLEEP_ADJ_TICKS);

    HAL_SLEEP_ST_SET(stCmp);
    HAL_SLEEP_TIMER_CLEAR_INT();
    HAL_SLEEP_TIMER_ENABLE_INT();
  }

  halSleepExec();  // Effect the PM mode.
}

/**************************************************************************************************
 * @fn          halSleepTimerIsr
 *
 * @brief       Sleep timer ISR.
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
 **************************************************************************************************
 */
HAL_ISR_FUNCTION(halSleepTimerIsr, ST_VECTOR)
{
  HAL_SLEEP_TIMER_DISABLE_INT();
  CLEAR_SLEEP_MODE();
}

/**************************************************************************************************
 * @fn          halSleepExec
 *
 * @brief       This function puts the CC2530 to sleep by writing to the PCON register.
 *              The instruction after writing to PCON must not be 4-byte aligned or excessive
 *              power consumption may result. Since the write to PCON is 3 instructions, this
 *              function is forced to be even-byte aligned. Thus, this function must not have any
 *              automatic variables and the write to PCON must be the first C statement.
 *              See the linker file ".xcl" for actual placement of this function.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
#pragma optimize=none
void halSleepExec(void)
{
  PCON = halSleepPconValue;
  NOP();  // Allow interrupts to run immediately.
}
#endif

/**************************************************************************************************
 * @fn          halSleepWait
 *
 * @brief       Perform a blocking wait for the specified number of microseconds.
 *              Use assumptions about number of clock cycles needed for the various instructions.
 *              This function assumes a 32 MHz clock.
 *              NB! This function is highly dependent on architecture and compiler!
 *
 * input parameters
 *
 * @param       duration - Duration of wait in microseconds.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
#pragma optimize=none
void halSleepWait(uint16 duration)
{
  duration >>= 1;

  while (duration-- > 0)
  {
    NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
    NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
  }
}

/**************************************************************************************************
*/
