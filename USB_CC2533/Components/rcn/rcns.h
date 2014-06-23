/**************************************************************************************************
  Filename:       rcns.h
**************************************************************************************************/

#ifndef RCNS_H
#define RCNS_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "rcn_nwk.h"
  
/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

/*********************************************************************
 * FUNCTIONS
 */


/**************************************************************************************************
 * @fn          RCNS_SerializeCback
 *
 * @brief       This function serializes RCN callback and send it over to application processor.
 *
 * input parameters
 *
 * @param       pData - Pointer to callback event structure.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
extern void RCNS_SerializeCback(rcnCbackEvent_t *pData);

/**************************************************************************************************
 * @fn          RCNS_HandleAsyncMsg
 *
 * @brief       This function handles serialized RCN interface message which came as asynchronous
 *              message.
 *
 * input parameters
 *
 * @param       pData - pointer to a received serialized data starting from length, subsystem id
 *                      and command id, etc.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void RCNS_HandleAsyncMsg( uint8 *pData );

/**************************************************************************************************
 * @fn          RCNS_HandleSyncMsg
 *
 * @brief       This function handles serialized RCN interface message which came as synchronous
 *              message and builds a synchronous response message.
 *
 * input parameters
 *
 * @param       pData - pointer to a received serialized data starting from length, subsystem id
 *                      and command id, etc.
 *                      The memory space pointed by this pointer shall be overwritten with a
 *                      synchronous response message.
 *
 * output parameters
 *
 * None.
 *
 * @return      TRUE if RCNS wants control of RCN callback functions, FALSE otherwise.
 */
uint8 RCNS_HandleSyncMsg( uint8 *pData );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RCNS_H */
