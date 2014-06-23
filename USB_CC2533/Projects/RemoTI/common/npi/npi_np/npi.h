/**************************************************************************************************
  Filename:       npi.h
**************************************************************************************************/

#ifndef NPI_H
#define NPI_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "hal_board.h"
#include "hal_rpc.h"

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

// Buffer size - it has to be big enough for the largest NPI RPC packet and NPI overhead
#define NP_MAX_BUF_LEN                 128

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

// NPI API and NPI Callback Message structure
// NOTE: Fields are position dependent. Do not rearrange!
typedef struct
{
  uint8 len;
  uint8 subSys;
  uint8 cmdId;
  uint8 pData[NP_MAX_BUF_LEN];
} npiMsgData_t;

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

extern uint8 NPI_TaskId;

/*********************************************************************
 * FUNCTIONS
 */

// NPI OSAL related functions
extern void NPI_Init( uint8 taskId );
extern uint16 NPI_ProcessEvent( uint8 taskId, uint16 events );

//
// Network Processor Interface APIs
//

/***************************************************************************************************
 * @fn      NPI_SleepRx
 *
 * @brief   Ready the UART for sleep by switching the RX port to a GPIO with
 *          interrupts enabled.
 *
 * @param   None.
 *
 * @return  None.
 ***************************************************************************************************/
extern void NPI_SleepRx( void );

/**************************************************************************************************
 * @fn          NPI_SendAsynchData
 *
 * @brief       This function is called by the client when it has data ready to
 *              be sent asynchronously. This routine allocates an AREQ buffer,
 *              copies the client's payload, and sets up the send.
 *
 * input parameters
 *
 * @param *pMsg  - Pointer to data to be sent asynchronously (i.e. AREQ).
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_SendAsynchData( npiMsgData_t *pMsg );


/**************************************************************************************************
 * @fn          NPI_AsynchMsgCback
 *
 * @brief       This function is a NPI callback to the client that inidcates an
 *              asynchronous message has been received. The client software is
 *              expected to complete this call.
 *
 *              Note: The client must copy this message if it requires it
 *                    beyond the context of this call.
 *
 * input parameters
 *
 * @param       *pMsg - A pointer to an asychronously received message.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_AsynchMsgCback( npiMsgData_t *pMsg );


/**************************************************************************************************
 * @fn          NPI_SynchMsgCback
 *
 * @brief       This function is a NPI callback to the client that inidcates an
 *              synchronous message has been received. The client software is
 *              expected to complete this call.
 *
 *              Note: The client must process this message and provide a reply
 *                    using the same buffer in the context of this call.
 *
 * input parameters
 *
 * @param       *pMsg - A pointer to an sychronously received message.
 *
 * output parameters
 *
 * @param       *pMsg - A pointer to the synchronous reply message.
 *
 * @return      None.
 **************************************************************************************************
 */
extern void NPI_SynchMsgCback( npiMsgData_t *pMsg );


/**************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif /* NPI_H */
