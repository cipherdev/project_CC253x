#ifndef NP_MAIN_H
#define NP_MAIN_H
/**************************************************************************************************
    Filename:       np_main.h
    Revised:        $Date: 2011-05-03 14:19:23 -0700 (Tue, 03 May 2011) $
    Revision:       $Revision: 25853 $

    Description:

    Public interface file for the Network Processor.

    Copyright (c) 2007 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
**************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_board.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define SA_INIT_CMD_ID     0
#define SA_INC_CMD_ID      1
#define SA_DEC_CMD_ID      2
#define SA_GET_CMD_ID      3
#define SA_GET_RSP_CMD_ID  4
#define SA_MAX_CNT_CMD_ID  5
#define SA_MIN_CNT_CMD_ID  6
  
#define SA_MAX_POLL_EVENT  1
#define SA_MIN_POLL_EVENT  2
  
/* ------------------------------------------------------------------------------------------------
 *                                          Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Global Variables
 * ------------------------------------------------------------------------------------------------
 */

// NPI Task Related
extern uint8 NPI_TaskId;

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
*/

#ifdef __cplusplus
};
#endif

#endif /* NP_MAIN_H */
