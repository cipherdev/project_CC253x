/**************************************************************************************************
  Filename:       OnBoard.h
  Revised:        $Date: 2011-05-12 14:05:43 -0700 (Thu, 12 May 2011) $
  Revision:       $Revision: 25963 $

  Description:    Defines stuff for EVALuation boards
                  This file targets the Chipcon CC2430DB/CC2430EB


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
#ifndef ONBOARD_H
#define ONBOARD_H

#include "hal_mcu.h"
#include "hal_sleep.h"

// Internal (MCU) heap size
#if !defined( INT_HEAP_LEN )
  #define INT_HEAP_LEN  2048
#endif

// Memory Allocation Heap
#define MAXMEMHEAP INT_HEAP_LEN

/* OSAL timer defines */
// Timer clock and power-saving definitions
#define TICK_COUNT         1  // TIMAC requires this number to be 1

#ifndef _WIN32
extern void _itoa(uint16 num, uint8 *buf, uint8 radix);
#endif

/* Reset reason for reset indication */
#define ResetReason() ((SLEEPSTA >> 3) & 0x03)

// Power conservation used by OSAL_PwrMgr.c
#define OSAL_SET_CPU_INTO_SLEEP(timeout) halSleep(timeout);  /* Called from OSAL_PwrMgr */

/*
 * Board specific random number generator
 */
#ifdef HAL_MCU_CC2533
extern uint8 MAC_RandomByte( void );
#define Onboard_rand() ((((uint16) MAC_RandomByte())<<8)|((uint16)MAC_RandomByte()))
#else /* HAL_MCU_CC2533 */
extern uint16 HalAdcRand( void );
#define Onboard_rand() HalAdcRand()
#endif /* HAL_MCU_CC2533 */

/*
 * Board specific soft reset.
 */
extern __near_func void Onboard_soft_reset( void );

#endif
/*********************************************************************
 */
