/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2012-10-23 16:13:32 -0700 (Tue, 23 Oct 2012) $
  Revision:       $Revision: 31902 $

  Description:    Declarations for the CC2531.


  Copyright 2006-2012 Texas Instruments Incorporated. All rights reserved.

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
#include "usb_board_cfg.h"

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
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_CPU_CLOCK_MHZ     32

#if !defined OSC32K_CRYSTAL_INSTALLED
  #define OSC32K_CRYSTAL_INSTALLED  FALSE
#endif

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

#define HAL_NUM_LEDS            2

#define HAL_LED_BLINK_DELAY()   st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

/* 1 - Green */
#define LED1_BV           BV(0)
#define LED1_SBIT         P0_0
#define LED1_DDR          P0DIR
#define LED1_POLARITY     ACTIVE_LOW

/* 2 - Red */
#define LED2_BV           BV(1)
#define LED2_SBIT         P1_1
#define LED2_DDR          P1DIR
#define LED2_POLARITY     ACTIVE_HIGH

/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* S1 */
#define PUSH1_BV          BV(2)
#define PUSH1_SBIT        P1_2
#define PUSH1_POLARITY    ACTIVE_LOW

/* S2 */
#define PUSH2_BV          BV(3)
#define PUSH2_SBIT        P1_3
#define PUSH2_POLARITY    ACTIVE_LOW

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
#if defined HAL_SB_BOOT_CODE
#define HAL_FLASH_PAGE_SIZE        2048
#else
//efine HAL_FLASH_PAGE_SIZE        2048
#define HAL_FLASH_PAGE_SIZE        4096
#endif
#define HAL_FLASH_PAGE_MULT        256UL

#define HAL_FLASH_WORD_SIZE        4

// Flash is partitioned into banks of 32K.
#define HAL_FLASH_PAGE_PER_BANK   ((uint8)((uint16)32 * 1024 / HAL_FLASH_PAGE_SIZE))

// CODE banks get mapped into the upper 32K XDATA range 8000-FFFF.
#define HAL_FLASH_PAGE_MAP         0x8000

// The last 16 bytes of the last available page are reserved for flash lock bits.
#define HAL_FLASH_LOCK_BITS        16

// Re-defining Z_EXTADDR_LEN here so as not to include a Z-Stack .h file.
#define HAL_FLASH_IEEE_SIZE        8
#define HAL_FLASH_IEEE_OSET       (HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)
#define HAL_FLASH_IEEE_PAGE       ((uint8)(HAL_FLASH_PAGE_MULT * 1024 / HAL_FLASH_PAGE_SIZE - 1))

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
 *                    Reserving 1st 3 pages and last page for USB boot loader.
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_USB_IMG_ADDR       0x1800
#define HAL_USB_CRC_ADDR       0x1890
// Size of internal flash less 4 pages for boot loader and 6 pages for NV.
#define HAL_USB_IMG_SIZE      (0x40000 - 0x2000 - 0x3000)

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
#ifdef ZID_DONGLE_NANO

#define MCU_IO_INPUT_P0_PULLDOWN(port, pin)  st( P0SEL &= ~BV(pin); \
                                              P0DIR &= ~BV(pin); \
                                              P0INP &= ~BV(pin); \
                                              P2INP |= BV(port + 5); \
                                             )
#define MCU_IO_INPUT_P1_PULLDOWN(port, pin)  st( P1SEL &= ~BV(pin); \
                                              P1DIR &= ~BV(pin); \
                                              P1INP &= ~BV(pin); \
                                              P2INP |= BV(port + 5); \
                                             )
#define MCU_IO_INPUT_P2_PULLDOWN(port, pin)  st( P2SEL &= ~BV(pin); \
                                              P2DIR &= ~BV(pin); \
                                              P2INP &= ~BV(pin); \
                                              P2INP |= BV(port + 5); \
                                             )

#if defined HAL_SB_BOOT_CODE

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
  HAL_USB_PULLUP_DISABLE();                  /* Disconnect D+ signal to host. */                  \
                                                                 \
  /* Turn on cache prefetch mode */                              \
  PREFETCH_ENABLE();                                            \
  MCU_IO_INPUT_P0_PULLDOWN(0,0);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,1);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,2);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,3);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,4);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,5);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,6);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,7);                              \
\
  MCU_IO_INPUT_P1_PULLDOWN(1,1);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,2);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,3);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,4);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,5);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,6);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,7);                              \
\
  MCU_IO_INPUT_P2_PULLDOWN(2,3);                              \
  MCU_IO_INPUT_P2_PULLDOWN(2,4);                              \
  P2INP |= 0x6;                                               \
}
#else

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
  PREFETCH_ENABLE();                                            \
                                                                 \
  MCU_IO_INPUT_P0_PULLDOWN(0,0);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,1);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,2);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,3);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,4);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,5);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,6);                              \
  MCU_IO_INPUT_P0_PULLDOWN(0,7);                              \
\
  MCU_IO_INPUT_P1_PULLDOWN(1,1);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,2);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,3);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,4);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,5);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,6);                              \
  MCU_IO_INPUT_P1_PULLDOWN(1,7);                              \
\
  MCU_IO_INPUT_P2_PULLDOWN(2,3);                              \
  MCU_IO_INPUT_P2_PULLDOWN(2,4);                              \
  P2INP |= 0x6;                                               \

}
#endif

#else

#if defined HAL_SB_BOOT_CODE

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
  HAL_USB_PULLUP_DISABLE();                  /* Disconnect D+ signal to host. */                  \
                                                                 \
  /* Turn on cache prefetch mode */                              \
  PREFETCH_ENABLE();                                            \
}

#else

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
  PREFETCH_ENABLE();                                            \
                                                                 \
  HAL_TURN_OFF_LED1();                                           \
  LED1_DDR |= LED1_BV;                                           \
  HAL_TURN_OFF_LED2();                                           \
  LED2_DDR |= LED2_BV;                                           \
}
#endif

#endif
/* Check for lock of 32MHz XOSC and the source for 32K clock. */
#define HAL_BOARD_STABLE()  (CLKCONSTA == (CLKCONCMD_32MHZ | OSC_32KHZ))

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
#define HAL_TURN_OFF_LED1()       st( LED1_SBIT = LED1_POLARITY (0); )
#define HAL_TURN_OFF_LED2()       st( LED2_SBIT = LED2_POLARITY (0); )
#define HAL_TURN_OFF_LED3()       HAL_TURN_OFF_LED1()
#define HAL_TURN_OFF_LED4()       HAL_TURN_OFF_LED2()

#define HAL_TURN_ON_LED1()        st( LED1_SBIT = LED1_POLARITY (1); )
#define HAL_TURN_ON_LED2()        st( LED2_SBIT = LED2_POLARITY (1); )
#define HAL_TURN_ON_LED3()        HAL_TURN_ON_LED1()
#define HAL_TURN_ON_LED4()        HAL_TURN_ON_LED2()

#define HAL_TOGGLE_LED1()         st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
#define HAL_TOGGLE_LED2()         st( if (LED2_SBIT) { LED2_SBIT = 0; } else { LED2_SBIT = 1;} )
#define HAL_TOGGLE_LED3()         HAL_TOGGLE_LED1()
#define HAL_TOGGLE_LED4()         HAL_TOGGLE_LED2()

#define HAL_STATE_LED1()          (LED1_POLARITY (LED1_SBIT))
#define HAL_STATE_LED2()          (LED2_POLARITY (LED2_SBIT))
#define HAL_STATE_LED3()          HAL_STATE_LED1()
#define HAL_STATE_LED4()          HAL_STATE_LED2()

/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

#define HAL_FLASH     TRUE
#define HAL_LCD       FALSE
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
#ifndef HAL_HID
#define HAL_HID FALSE
#endif
#ifndef HAL_KEY
#define HAL_KEY       TRUE
#endif
#ifndef HAL_LED
#define HAL_LED       TRUE
#endif
#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

#ifndef HAL_UART
#define HAL_UART !HAL_HID
#define HAL_UART_DMA  0
#define HAL_UART_ISR  0
#define HAL_UART_USB  1
#endif

#ifndef HAL_UART_DMA
#define HAL_UART_DMA 0
#endif
#ifndef HAL_UART_ISR
#define HAL_UART_ISR 0
#endif

#ifndef HAL_UART_USB
# if HAL_UART
#  define HAL_UART_USB 1
# else
#  define HAL_UART_USB 0
# endif
#endif

#endif
/*******************************************************************************************************
*/
