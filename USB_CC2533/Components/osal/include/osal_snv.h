/**************************************************************************************************
  Filename:       osal_snv.h
**************************************************************************************************/

#ifndef OSAL_SNV_H
#define OSAL_SNV_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

#include "hal_types.h"

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */
#ifdef OSAL_SNV_UINT16_ID
  typedef uint16 osalSnvId_t;
  typedef uint16 osalSnvLen_t;
#else
  typedef uint8 osalSnvId_t;
  typedef uint8 osalSnvLen_t;
#endif

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
 * @fn      osal_snv_init
 *
 * @brief   Initialize NV service.
 *
 * @return  none
 */
extern void osal_snv_init( void );

/*********************************************************************
 * @fn      osal_snv_read
 *
 * @brief   Read data from NV.
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Length of data to read.
 * @param   *pBuf - Data is read into this buffer.
 *
 * @return  SUCCESS if successful.
 *          Otherwise, NV_OPER_FAILED for failure.
 */
extern uint8 osal_snv_read( osalSnvId_t id, osalSnvLen_t len, void *pBuf);

/*********************************************************************
 * @fn      osal_snv_write
 *
 * @brief   Write a data item to NV.
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Length of data to write.
 * @param   *pBuf - Data to write.
 *
 * @return  SUCCESS if successful, NV_OPER_FAILED if failed.
 */
extern uint8 osal_snv_write( osalSnvId_t id, osalSnvLen_t len, void *pBuf);

/*********************************************
 * @fn      osal_snv_makeRoomInActivePage
 *
 * @brief   Check if there is room for the new items, of length given in the table
 *          lenTable, in active page.
 *          If there is no room, perform a compaction and transfer page.
 *
 * @param   lenTable - Table of the items different lengths
 * @param   numOfItems - Number of items in lenTable
 *
 * @return  SUCCESS if successful.
 *          Otherwise, NV_OPER_FAILED for failure.
 */
extern uint8 osal_snv_makeRoomInActivePage(uint8 numOfItems, osalSnvLen_t lenTable[]);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SNV.H */
