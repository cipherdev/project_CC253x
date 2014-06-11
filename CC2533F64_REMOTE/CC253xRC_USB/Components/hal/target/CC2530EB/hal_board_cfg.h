/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2012-10-23 16:13:32 -0700 (Tue, 23 Oct 2012) $
  Revision:       $Revision: 31902 $

  Description:    Declarations for the CC2530EM used on the SmartRF05EB.


  Copyright 2006-2011 Texas Instruments Incorporated. All rights reserved.

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
#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

/* ------------------------------------------------------------------------------------------------
 *                                       CC2590/CC2591 support
 *
 *                        Define HAL_PA_LNA_CC2590 if CC2530+CC2590EM is used
 *                        Define HAL_PA_LNA if CC2530+CC2591EM is used
 *                        Note that only one of them can be defined
 * ------------------------------------------------------------------------------------------------
 */

#define xHAL_PA_LNA
#define xHAL_PA_LNA_CC2590

/* ------------------------------------------------------------------------------------------------
 *                                       Board Indentifier
 *
 *      Define the Board Identifier for the SmartRF05 EB board
 *      HAL_BOARD_CC2530EB_REV13 for SmartRF05 EB 1.3 board
 *      HAL_BOARD_CC2530EB_REV17 for SmartRF05 EB 1.7 board
 *      HAL_BOARD_CC2530EB_REV18 for SmartRF05 EB 1.8 board
 * ------------------------------------------------------------------------------------------------
 */

#if !defined (HAL_BOARD_CC2530EB_REV18) &&\
    !defined (HAL_BOARD_CC2530EB_REV17) &&\
    !defined (HAL_BOARD_CC2530EB_REV13)
#define HAL_BOARD_CC2530EB_REV18
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_CPU_CLOCK_MHZ     32

/* 32 kHz clock source select in CLKCONCMD */
#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
#define OSC_32KHZ  0x00 /* external 32 KHz xosc */
#else
#define OSC_32KHZ  0x80 /* internal 32 KHz rcosc */
#endif

/* ------------------------------------------------------------------------------------------------
 *                                       LED Configuration
 * ------------------------------------------------------------------------------------------------
 */

#if (defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_CC2530EB_REV18)) &&\
    !defined (HAL_PA_LNA) && !defined (HAL_PA_LNA_CC2590)
  #define HAL_NUM_LEDS            3
#elif defined (HAL_BOARD_CC2530EB_REV13) ||\
      defined (HAL_PA_LNA) || defined (HAL_PA_LNA_CC2590)
  #define HAL_NUM_LEDS            1
#else
  #error Unknown Board Indentifier
#endif

#define HAL_LED_BLINK_DELAY()   st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

/* 1 - Green */
#define LED1_BV           BV(0)
#define LED1_SBIT         P1_0
#define LED1_DDR          P1DIR
#define LED1_POLARITY     ACTIVE_HIGH

#if (defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_CC2530EB_REV18))
  /* 2 - Red */
  #define LED2_BV           BV(1)
  #define LED2_SBIT         P1_1
  #define LED2_DDR          P1DIR
  #define LED2_POLARITY     ACTIVE_HIGH

  /* 3 - Yellow */
  #define LED3_BV           BV(4)
  #define LED3_SBIT         P1_4
  #define LED3_DDR          P1DIR
  #define LED3_POLARITY     ACTIVE_HIGH
#endif

/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

#define ACTIVE_LOW_ENUM   0
#define ACTIVE_HIGH_ENUM  1

/* S1 */
#define PUSH1_BV          BV(1)
#define PUSH1_SBIT        P0_1

#if (defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_CC2530EB_REV18))
  #define PUSH1_POLARITY         ACTIVE_HIGH
  #define PUSH1_POLARITY_ENUM    ACTIVE_HIGH_ENUM
#elif defined (HAL_BOARD_CC2530EB_REV13)
  #define PUSH1_POLARITY         ACTIVE_LOW
  #define PUSH1_POLARITY_ENUM    ACTIVE_LOW_ENUM
#else
  #error Unknown Board Indentifier
#endif

/* Joystick Center Press */
#define PUSH2_BV          BV(0)
#define PUSH2_SBIT        P2_0
#define PUSH2_POLARITY    ACTIVE_HIGH

/* ------------------------------------------------------------------------------------------------
 *                         OSAL NV implemented by internal flash pages.
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_FLASH_PAGE_PHYS        2048UL

/* The CC2530 flash page size is physically 2K. But it is advantageous for upper layer S/W
 * (e.g. OSAL SNV module) to use a larger virtual page size (which must be an even
 * multiple of both the physical page size and the 32K bank size). Implementing the virtual page
 * size abstraction in the HAL saves on the code size and complexity cost of implementing in OSAL.
 * Note that changing the HAL page size requires changes to the linker command file; look for
 * the following warning in the linker file corresponding to the build target:
 * Warning: this correspondence cannot be automatically enforced with linker errors.
 */
// OAD must maintain 2-2K pages to be backward compatible with fielded OAD boot loaders.
#if defined FEATURE_OAD
#define HAL_FLASH_PAGE_SIZE        HAL_FLASH_PAGE_PHYS
#elif defined CC2530F64
#define HAL_FLASH_PAGE_SIZE        HAL_FLASH_PAGE_PHYS
#elif defined CC2530F128
#define HAL_FLASH_PAGE_SIZE        HAL_FLASH_PAGE_PHYS
#else
#define HAL_FLASH_PAGE_SIZE       (HAL_FLASH_PAGE_PHYS * 2)
#endif

#if defined CC2530F64
#define HAL_FLASH_PAGE_CNT         32
#elif defined CC2530F128
#define HAL_FLASH_PAGE_CNT         64
#else
#define HAL_FLASH_PAGE_CNT         128
#endif

#define HAL_FLASH_WORD_SIZE        4

// Flash is partitioned into banks of 32K.
#define HAL_FLASH_PAGE_PER_BANK   ((uint8)(32768 / HAL_FLASH_PAGE_SIZE))

// CODE banks get mapped into the upper 32K XDATA range 8000-FFFF.
#define HAL_FLASH_PAGE_MAP         0x8000

// The last 16 bytes of the last available page are reserved for flash lock bits.
#define HAL_FLASH_LOCK_BITS        16

// Re-defining Z_EXTADDR_LEN here so as not to include a Z-Stack .h file.
#define HAL_FLASH_IEEE_SIZE        8
#define HAL_FLASH_IEEE_OSET       (HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)
#define HAL_FLASH_IEEE_PAGE       ((uint8)(HAL_FLASH_PAGE_CNT * HAL_FLASH_PAGE_PHYS\
                                                              / HAL_FLASH_PAGE_SIZE - 1))
#define HAL_NV_PAGE_END            HAL_FLASH_IEEE_PAGE
#define HAL_NV_PAGE_CNT            2
#define HAL_NV_PAGE_BEG           (HAL_NV_PAGE_END-HAL_NV_PAGE_CNT)

// IEEE address offset in information page
// The value is not supposed to change from board to board but it is defined
// here regardless to be consistent with other projects.
#define HAL_INFOP_IEEE_OSET       0xC

#define HAL_NV_DMA_CH              0  // HalFlashWrite trigger.
#define HAL_DMA_CH_RX              3  // USART RX DMA channel.
#define HAL_DMA_CH_TX              4  // USART TX DMA channel.

#define HAL_NV_DMA_GET_DESC()      HAL_DMA_GET_DESC0()
#define HAL_NV_DMA_SET_ADDR(a)     HAL_DMA_SET_ADDR_DESC0((a))

/* ------------------------------------------------------------------------------------------------
 *  Serial Boot Loader: An SBL-enabled Application image must reserve these specific pages for the
 *  SBL in its linker map file:
 *  - The first page of flash (interrupt vectors).
 *  - The last page of flash (lock bits --> not prgramatically writable/erasable).
 * ------------------------------------------------------------------------------------------------
 */

// This also keeps SB from overwriting itself, so set to first address of 2nd page / 4.
#define HAL_SBL_CODE_ADDR_BEG  (uint16)(0x800 / 4)

// Do not include the CRC in the embedded CRC calculation (0x0890 / 4).
#define HAL_SBL_CRC_ADDR        0x0224

// This is the address in the 64K range to begin to dis-allow data to be read/written in order to
// lock-out attempts to read/write the NV or BootLoader with SBL_READ_CMD/SBL_WRITE_CMD.
#define HAL_SBL_SKIP_ADDR_BEG  (uint16)(HAL_NV_PAGE_BEG * HAL_FLASH_PAGE_SIZE / HAL_FLASH_WORD_SIZE)

// This prevents a hostile attempt to change the skip length such that uint16 wrap allows reading
// into the NV area.
#define HAL_SBL_SKIP_ADDR_MIN  (uint16)((HAL_NV_PAGE_BEG - 10) * HAL_FLASH_PAGE_SIZE /\
                                                                 HAL_FLASH_WORD_SIZE)
// This is the address to re-allow data to be read/written.
#define HAL_SBL_SKIP_ADDR_END  (uint16)(HAL_NV_PAGE_END * HAL_FLASH_PAGE_SIZE / HAL_FLASH_WORD_SIZE)

// This is used to end the embedded CRC calculation range.
#define HAL_SBL_CODE_ADDR_END  HAL_SBL_SKIP_ADDR_BEG

/* ------------------------------------------------------------------------------------------------
 *                Critical Vdd Monitoring to prevent flash damage or radio lockup.
 * ------------------------------------------------------------------------------------------------
 */

// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD_2_0  74   // 2.0 V required to safely read/write internal flash.
#define VDD_2_04 75
#define VDD_2_09 77
#define VDD_2_12 78
#define VDD_2_7  100  // 2.7 V required for the Numonyx device.

#define VDD_MIN_INIT  VDD_2_09
#define VDD_MIN_POLL  VDD_2_04
#define VDD_MIN_FLASH VDD_2_12  // 5% margin over minimum to survive a page erase and compaction.
#define VDD_MIN_XNV  (VDD_2_7+5)  // 5% margin over minimum to survive a page erase and compaction.

/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ----------- RF-frontend Connection Initialization ---------- */
#if defined HAL_PA_LNA || defined HAL_PA_LNA_CC2590
extern void MAC_RfFrontendSetup(void);
#define HAL_BOARD_RF_FRONTEND_SETUP() MAC_RfFrontendSetup()
#else
#define HAL_BOARD_RF_FRONTEND_SETUP()
#endif

/* ----------- Cache Prefetch control ---------- */
#define PREFETCH_ENABLE()     st( FCTL = 0x08; )
#define PREFETCH_DISABLE()    st( FCTL = 0x04; )

/* ----------- Board Initialization ---------- */
#if (defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_CC2530EB_REV18)) &&\
    !defined (HAL_PA_LNA) && !defined (HAL_PA_LNA_CC2590)

#define HAL_BOARD_INIT()                                         \
{                                                                \
  uint16 i;                                                      \
                                                                 \
  SLEEPCMD &= ~OSC_PD;                       /* turn on 16MHz RC and 32MHz XOSC */                \
  while (!(SLEEPSTA & XOSC_STB));            /* wait for 32MHz XOSC stable */                     \
  asm("NOP");                                /* chip bug workaround */                            \
  for (i=0; i<504; i++) asm("NOP");          /* Require 63us delay for all revs */                \
  CLKCONCMD = (CLKCONCMD_32MHZ | OSC_32KHZ); /* Select 32MHz XOSC and the source for 32K clock */ \
  while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); /* Wait for the change to be effective */   \
  SLEEPCMD |= OSC_PD;                        /* turn off 16MHz RC */                              \
                                                                 \
  /* Turn on cache prefetch mode */                              \
  PREFETCH_ENABLE();                                             \
                                                                 \
  HAL_TURN_OFF_LED1();                                           \
  LED1_DDR |= LED1_BV;                                           \
  HAL_TURN_OFF_LED2();                                           \
  LED2_DDR |= LED2_BV;                                           \
  HAL_TURN_OFF_LED3();                                           \
  LED3_DDR |= LED3_BV;                                           \
                                                                 \
  /* configure tristates */                                      \
  P0INP |= PUSH2_BV;                                             \
}

#elif defined (HAL_BOARD_CC2530EB_REV13) ||\
      defined (HAL_PA_LNA) || defined (HAL_PA_LNA_CC2590)

#define HAL_BOARD_INIT()                                         \
{                                                                \
  uint16 i;                                                      \
                                                                 \
  SLEEPCMD &= ~OSC_PD;                       /* turn on 16MHz RC and 32MHz XOSC */                \
  while (!(SLEEPSTA & XOSC_STB));            /* wait for 32MHz XOSC stable */                     \
  asm("NOP");                                /* chip bug workaround */                            \
  for (i=0; i<504; i++) asm("NOP");          /* Require 63us delay for all revs */                \
  CLKCONCMD = (CLKCONCMD_32MHZ | OSC_32KHZ); /* Select 32MHz XOSC and the source for 32K clock */ \
  while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ)); /* Wait for the change to be effective */   \
  SLEEPCMD |= OSC_PD;                        /* turn off 16MHz RC */                              \
                                                                 \
  /* Turn on cache prefetch mode */                              \
  PREFETCH_ENABLE();                                             \
                                                                 \
  /* set direction for GPIO outputs  */                          \
  LED1_DDR |= LED1_BV;                                           \
                                                                 \
  /* Set PA/LNA HGM control P0_7 */                              \
  P0DIR |= BV(7);                                                \
                                                                 \
  /* configure tristates */                                      \
  P0INP |= PUSH2_BV;                                             \
                                                                 \
  /* setup RF frontend if necessary */                           \
  HAL_BOARD_RF_FRONTEND_SETUP();                                 \
}

#endif

/* ----------- Debounce ---------- */
#define HAL_DEBOUNCE(expr)    { int i; for (i=0; i<500; i++) { if (!(expr)) i = 0; } }

/* ----------- Push Buttons ---------- */
#define HAL_PUSH_BUTTON1()        (PUSH1_POLARITY (PUSH1_SBIT))
#define HAL_PUSH_BUTTON2()        (PUSH2_POLARITY (PUSH2_SBIT))
#define HAL_PUSH_BUTTON3()        (0)
#define HAL_PUSH_BUTTON4()        (0)
#define HAL_PUSH_BUTTON5()        (0)
#define HAL_PUSH_BUTTON6()        (0)

/* ----------- LED's ---------- */
#if (defined (HAL_BOARD_CC2530EB_REV17) || defined (HAL_BOARD_CC2530EB_REV18)) &&\
    !defined (HAL_PA_LNA) && !defined (HAL_PA_LNA_CC2590)

  #define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
  #define HAL_TURN_OFF_LED2()       st( LED2_SBIT = LED2_POLARITY (0); )
  #define HAL_TURN_OFF_LED3()       st( LED3_SBIT = LED3_POLARITY (0); )
  #define HAL_TURN_OFF_LED4()       HAL_TURN_OFF_LED1()

  #define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
  #define HAL_TURN_ON_LED2()        st( LED2_SBIT = LED2_POLARITY (1); )
  #define HAL_TURN_ON_LED3()        st( LED3_SBIT = LED3_POLARITY (1); )
  #define HAL_TURN_ON_LED4()        HAL_TURN_ON_LED1()

  #define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
  #define HAL_TOGGLE_LED2()         st( if (LED2_SBIT) { LED2_SBIT = 0; } else { LED2_SBIT = 1;} )
  #define HAL_TOGGLE_LED3()         st( if (LED3_SBIT) { LED3_SBIT = 0; } else { LED3_SBIT = 1;} )
  #define HAL_TOGGLE_LED4()         HAL_TOGGLE_LED1()

  #define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
  #define HAL_STATE_LED2()          (LED2_POLARITY (LED2_SBIT))
  #define HAL_STATE_LED3()          (LED3_POLARITY (LED3_SBIT))
  #define HAL_STATE_LED4()          HAL_STATE_LED1()

#elif defined (HAL_BOARD_CC2530EB_REV13) || defined (HAL_PA_LNA) || defined (HAL_PA_LNA_CC2590)

  #define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
  #define HAL_TURN_OFF_LED2()       HAL_TURN_OFF_LED1()
  #define HAL_TURN_OFF_LED3()       HAL_TURN_OFF_LED1()
  #define HAL_TURN_OFF_LED4()       HAL_TURN_OFF_LED1()

  #define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
  #define HAL_TURN_ON_LED2()        HAL_TURN_ON_LED1()
  #define HAL_TURN_ON_LED3()        HAL_TURN_ON_LED1()
  #define HAL_TURN_ON_LED4()        HAL_TURN_ON_LED1()

  #define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
  #define HAL_TOGGLE_LED2()         HAL_TOGGLE_LED1()
  #define HAL_TOGGLE_LED3()         HAL_TOGGLE_LED1()
  #define HAL_TOGGLE_LED4()         HAL_TOGGLE_LED1()

  #define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
  #define HAL_STATE_LED2()          HAL_STATE_LED1()
  #define HAL_STATE_LED3()          HAL_STATE_LED1()
  #define HAL_STATE_LED4()          HAL_STATE_LED1()

#endif

/* ----------- XNV ---------- */
#define XNV_SPI_BEGIN()             st(P1_3 = 0;)
#define XNV_SPI_TX(x)               st(U1CSR &= ~0x02; U1DBUF = (x);)
#define XNV_SPI_RX()                U1DBUF
#define XNV_SPI_WAIT_RXRDY()        st(while (!(U1CSR & 0x02));)
#define XNV_SPI_END()               st(P1_3 = 1;)

// The TI reference design uses UART1 Alt. 2 in SPI mode.
#define XNV_SPI_INIT() \
st( \
  /* Mode select UART1 SPI Mode as master. */\
  U1CSR = 0; \
  \
  /* Setup for 115200 baud. */\
  U1GCR = 11; \
  U1BAUD = 216; \
  \
  /* Set bit order to MSB */\
  U1GCR |= BV(5); \
  \
  /* Set UART1 I/O to alternate 2 location on P1 pins. */\
  PERCFG |= 0x02;  /* U1CFG */\
  \
  /* Select peripheral function on I/O pins but SS is left as GPIO for separate control. */\
  P1SEL |= 0xE0;  /* SELP1_[7:4] */\
  /* P1.1,2,3: reset, LCD CS, XNV CS. */\
  P1SEL &= ~0x0E; \
  P1 |= 0x0E; \
  P1_1 = 0; \
  P1DIR |= 0x0E; \
  \
  /* Give UART1 priority over Timer3. */\
  P2SEL &= ~0x20;  /* PRI2P1 */\
  \
  /* When SPI config is complete, enable it. */\
  U1CSR |= 0x40; \
  /* Release XNV reset. */\
  P1_1 = 1; \
)

/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_FLASH     TRUE
#define HAL_TIMER     FALSE  // HAL Timer implementation has been removed.

#ifndef HAL_ADC
#define HAL_ADC       TRUE
#endif
#ifndef HAL_AES
#define HAL_AES       TRUE
#endif
#ifndef HAL_AES_DMA
#define HAL_AES_DMA   TRUE
#endif
#ifndef HAL_DMA
#define HAL_DMA       TRUE
#endif
#ifndef HAL_VDDMON
#define HAL_VDDMON    TRUE
#endif

#ifndef HAL_KEY
#define HAL_KEY       TRUE
#endif
#ifndef HAL_LCD
#define HAL_LCD       TRUE
#endif
#ifndef HAL_LED
#define HAL_LED       TRUE
#endif
#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif
#ifndef HAL_UART
#define HAL_UART      FALSE
#endif

#if HAL_UART
#if HAL_DMA  // Always prefer to use DMA over ISR.
#ifndef HAL_UART_DMA
#define HAL_UART_DMA  1
#define HAL_UART_ISR  0
#endif
#if defined POWER_SAVING
#if !defined HAL_UART_GPIO_ISR
#define HAL_UART_GPIO_ISR
#endif
#endif
#endif

// Used to set P2 priority - USART0 over USART1 if both are defined.
#if ((HAL_UART_DMA == 1) || (HAL_UART_ISR == 1))
#define HAL_UART_PRIPO             0x00
#else
#define HAL_UART_PRIPO             0x40
#endif
#else
#define HAL_UART_DMA  0
#define HAL_UART_ISR  0
#endif

#define HAL_UART_USB  0  // USB is only available on the CC2531.

#endif
/*******************************************************************************************************
*/
