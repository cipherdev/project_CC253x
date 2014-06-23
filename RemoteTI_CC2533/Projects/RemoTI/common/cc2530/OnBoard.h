/**************************************************************************************************
  Filename:       OnBoard.h
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
