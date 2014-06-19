/**************************************************************************************************
  Filename:       hal_mcu.h
 
**************************************************************************************************/
#ifndef _HAL_MCU_H
#define _HAL_MCU_H

/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_defs.h"
#include "hal_types.h"

/* ------------------------------------------------------------------------------------------------
 *                                        Target Defines
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_MCU_CC2531

/* ------------------------------------------------------------------------------------------------
 *                                     Compiler Abstraction
 * ------------------------------------------------------------------------------------------------
 */

#ifdef __IAR_SYSTEMS_ICC__
#include <ioCC2531.h>
#define HAL_COMPILER_IAR
#define HAL_MCU_LITTLE_ENDIAN()   __LITTLE_ENDIAN__
#define _PRAGMA(x) _Pragma(#x)
#define HAL_ISR_FUNC_DECLARATION(f,v)   _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNCTION(f,v)           HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)
#else
#error "ERROR: Unknown compiler."
#endif

/* ------------------------------------------------------------------------------------------------
 *                                        Interrupt Macros
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_ENABLE_INTERRUPTS()         st( EA = 1; )
#define HAL_DISABLE_INTERRUPTS()        st( EA = 0; )
#define HAL_INTERRUPTS_ARE_ENABLED()    (EA)

typedef unsigned char halIntState_t;
#define HAL_ENTER_CRITICAL_SECTION(x)   st( x = EA;  HAL_DISABLE_INTERRUPTS(); )
#define HAL_EXIT_CRITICAL_SECTION(x)    st( EA = x; )
#define HAL_CRITICAL_STATEMENT(x)       st( halIntState_t _s; HAL_ENTER_CRITICAL_SECTION(_s); x; HAL_EXIT_CRITICAL_SECTION(_s); )

#ifdef __IAR_SYSTEMS_ICC__
  /* IAR library uses XCH instruction with EA. It may cause the higher priority interrupt to be
   * locked out, therefore, may increase interrupt latency.  It may also create a lockup condition.
   * This workaround should only be used with 8051 using IAR compiler. When IAR fixes this by
   * removing XCH usage in its library, compile the following macros to null to disable them.
   */
  #define HAL_ENTER_ISR()               { halIntState_t _isrIntState = EA; HAL_ENABLE_INTERRUPTS();
  #define HAL_EXIT_ISR()                  CLEAR_SLEEP_MODE();          EA = _isrIntState; }
#else
#error "ERROR: Unknown compiler."
#endif

/* ------------------------------------------------------------------------------------------------
 *                                        Reset Macro
 * ------------------------------------------------------------------------------------------------
 */

// Invoke the soft reset (LJMP to reset vector) so that the USB pull-up is not dropped.
#define HAL_SYSTEM_RESET()  st ( \
  extern __near_func void Onboard_soft_reset(void); \
  Onboard_soft_reset(); \
)

/* ------------------------------------------------------------------------------------------------
 *                                        CC253x rev numbers
 * ------------------------------------------------------------------------------------------------
 */

#define REV_A          0x00    /* workaround turned off */
#define REV_B          0x11    /* PG1.1 */
#define REV_C          0x20    /* PG2.0 */
#define REV_D          0x21    /* PG2.1 */

/* ------------------------------------------------------------------------------------------------
 *                                        CC253x sleep common code
 * ------------------------------------------------------------------------------------------------
 */

/* PCON bit definitions */
#define PCON_IDLE  BV(0)            /* Writing 1 to force CC253x to enter sleep mode */

/* SLEEPCMD bit definitions */
#define OSC_PD     BV(2)            /* Idle Osc: powered down=1 */
#define PMODE     (BV(1) | BV(0))   /* Power mode bits */

/* SLEEPSTA bit definitions */
#define XOSC_STB   BV(6)  /* XOSC: powered, stable=1 */
#define HFRC_STB   BV(5)  /* HFRCOSC: powered, stable=1 */

/* SLEEPCMD and SLEEPSTA bit definitions */
#define OSC_PD     BV(2)  /* 0: Both oscillators powered up and stable
                           * 1: oscillators not stable */

/* CLKCONCMD bit definitions */
#define OSC              BV(6)
#define TICKSPD_MASK     (BV(5) | BV(4) | BV(3))
#define TICKSPD(x)       (x << 3)
#define CLKSPD(x)        (x << 0)
#define CLKCONCMD_32MHZ  (0)
#define CLKCONCMD_16MHZ  (CLKSPD(1) | TICKSPD(1) | OSC)
#define CLKCONCMD_32MHZ_TICKSPD_16MHZ  (CLKSPD(0) | TICKSPD(1))
#define CLKCONCMD_32MHZ_TICKSPD_8MHZ   (CLKSPD(0) | TICKSPD(2))
#define CLKCONCMD_32MHZ_TICKSPD_4MHZ   (CLKSPD(0) | TICKSPD(3))

/* STLOAD */
#define LDRDY            BV(0) /* Load Ready. This bit is 0 while the sleep timer
                                * loads the 24-bit compare value and 1 when the sleep
                                * timer is ready to start loading a newcompare value. */

extern volatile __data uint8 halSleepPconValue;

#if defined POWER_SAVING
/* Any ISR that is used to wake up the chip shall call this macro. This prevents the race condition
 * when the PCON IDLE bit is set after such a critical ISR fires during the prep for sleep.
 */
#define CLEAR_SLEEP_MODE()        st( halSleepPconValue = 0; )
#define ALLOW_SLEEP_MODE()        st( halSleepPconValue = 1; )
#define CHECK_SLEEP_MODE()          ( halSleepPconValue != 0 )
#else
#define CLEAR_SLEEP_MODE()
#define ALLOW_SLEEP_MODE()
#define CHECK_SLEEP_MODE()
#endif

#endif
/**************************************************************************************************
 */
