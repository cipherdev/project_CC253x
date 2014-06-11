/**************************************************************************************************
  Filename:       npi.c
  Revised:        $Date: 2011-04-11 10:53:58 -0700 (Mon, 11 Apr 2011) $
  Revision:       $Revision: 25649 $

  Description:    This file contains the Network Processor Interface (NPI),
                  which abstracts the physical link between the Application
                  Processor (AP) and the Network Processor (NP). The NPI
                  serves as the HAL's client for the SPI and UART drivers, and
                  provides API and callback services for its client.

  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

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
 **************************************************************************************************/

#include "comdef.h"

/* Hal Driver includes */
#include "hal_board.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_uart.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* NPI */
#include "npi.h"

/**************************************************************************************************
 *                                        Type definitions
 **************************************************************************************************/

typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             *msg;
} npiSysEvtMsg_t;

/**************************************************************************************************
 *                                        Global Variables
 **************************************************************************************************/

uint8 NPI_TaskId;
osal_msg_q_t npiTxQueue;

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

/**************************************************************************************************
 *                                     Functions
 **************************************************************************************************/

#if ((defined HAL_UART) && (HAL_UART == TRUE))
#include "./npi_uart.c"
#elif ((defined HAL_SPI) && (HAL_SPI == TRUE))
#include "./npi_spi.c"
#elif ((defined HAL_I2C) && (HAL_I2C == TRUE))
#include "./npi_i2c.c"
#endif

/**************************************************************************************************
 **************************************************************************************************/
