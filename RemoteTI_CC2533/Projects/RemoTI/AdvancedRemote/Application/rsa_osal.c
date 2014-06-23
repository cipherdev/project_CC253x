/**************************************************************************************************
  Filename:       rsa_osal.c
**************************************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/

#include "comdef.h"
#include "hal_drivers.h"
#include "hal_types.h"
#include "mac_api.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "rcn_task.h"
#include "rsa.h"
#include "rti.h"
#include "zid_class_device.h"


/*********************************************************************
 * GLOBAL VARIABLES
 */

// The order in this table must be identical to the task initialization calls below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] =
{
  macEventLoop,
  RCN_ProcessEvent,
  RTI_ProcessEvent,
  zidCld_ProcessEvent,
  RSA_ProcessEvent,
  Hal_ProcessEvent
};

const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks( void )
{
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++ );
  RCN_Init( taskID++ );
  RTI_Init( taskID++ );
  zidCld_Init( taskID++ );
  RSA_Init( taskID++ );
  Hal_Init( taskID );
}

/*********************************************************************
*********************************************************************/
