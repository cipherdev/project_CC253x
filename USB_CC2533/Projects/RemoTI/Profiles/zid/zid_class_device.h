/**************************************************************************************************
  Filename:       zid_class_device.h
**************************************************************************************************/
#ifndef ZID_CLASS_DEVICE_H
#define ZID_CLASS_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "comdef.h"
#include "rti.h"
#include "zid_profile.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

#define ZID_CLD_EVT_CFG                    0x4000
#define ZID_CLD_EVT_SAFE_TX                0x0020

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* A packed structure that contains the ZID Class Device attribute settings */
typedef struct
{
  uint16 ZIDProfileVersion;
  uint16 IntPipeUnsafeTxWindowTime;
  uint16 HIDParserVersion;
  uint16 HIDDeviceReleaseNumber;
  uint16 HIDVendorId;
  uint16 HIDProductId;
  uint8  ReportRepeatInterval;
  uint8  HIDDeviceSubclass;
  uint8  HIDProtocolCode;
  uint8  HIDCountryCode;
  uint8  HIDNumEndpoints;
  uint8  HIDPollInterval;
  uint8  HIDNumStdDescComps;
  uint8  HIDStdDescCompsList[aplcMaxStdDescCompsPerHID];
  uint8  HIDNumNullReports;
  uint8  HIDNumNonStdDescComps;
} zid_cld_cfg_t;
#define ZID_CLD_CFG_T_SIZE  (sizeof(zid_cld_cfg_t))

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

extern uint8 zidCld_TaskId;

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          zidCld_Init
 *
 * @brief       This is the RemoTI task initialization called by OSAL.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidCld_Init(uint8 taskId);

/**************************************************************************************************
 * @fn          zidCld_InitItems
 *
 * @brief       This API is used to initialize the ZID configuration items to default values.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidCld_InitItems(void);

/**************************************************************************************************
 * @fn          zidCld_SetStateActive
 *
 * @brief       This API is used to force the ZID Class Device into the Active state.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
void zidCld_SetStateActive( void );

/**************************************************************************************************
 *
 * @fn          zidCld_ProcessEvent
 *
 * @brief       This routine handles ZID task events.
 *
 * input parameters
 *
 * @param       taskId - Task identifier assigned by OSAL.
 *              events - Event flags to be processed by this task.
 *
 * output parameters
 *
 * None.
 *
 * @return      16bit - Unprocessed event flags.
 */
uint16 zidCld_ProcessEvent(uint8 taskId, uint16 events);

/**************************************************************************************************
 * @fn          zidCld_SendDataReq
 *
 * @brief       This function sets timers, Rx on, and Tx Options according to requisite behavior.
 *
 * input parameters
 *
 * @param       dstIndex  - Pairing table index of target.
 * @param       txOptions - Bit mask according to the ZID_TX_OPTION_... definitions.
 * @param       *cmd      - A pointer to the first byte of the ZID data packet (the ZID Command).
 *
 * output parameters
 *
 * None.
 *
 * @return      txOptions, modified as necessary for the ZID transaction being requested.
 */
uint8 zidCld_SendDataReq(uint8 dstIndex, uint8 txOptions, uint8 *cmd);

/**************************************************************************************************
 * @fn      zidCld_ReceiveDataInd
 *
 * @brief   RTI receive data indication callback asynchronously initiated by
 *          another node. The client is expected to complete this function.
 *
 * input parameters
 *
 * @param   srcIdx -  Pairing table index of sender.
 * @param   rxFlags:   Receive flags.
 * @param   len    - Number of data bytes.
 * @param   *pData - Pointer to data.
 *
 * output parameters
 *
 * None.
 *
 * @return  TRUE if handled here. Otherwise FALSE indicating to pass the data to Application layer.
 */
uint8 zidCld_ReceiveDataInd(uint8 srcIndex, uint8 rxFlags, uint8 len, uint8 *pData);

/**************************************************************************************************
 * @fn      zidCld_PairCnf
 *
 * @brief   RTI confirmation callback initiated by client's RTI_PairReq API call.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  void
 */
void zidCld_PairCnf(uint8 dstIndex);

/**************************************************************************************************
 *
 * @fn      zidCld_Unpair
 *
 * @brief   RTI confirmation callback initiated by client's RTI_UnpairReq API call or by
 *          receiving unpair request command.
 *
 * @param   dstIndex - Pairing table index of paired device, or invalid.
 *
 * @return  TRUE if ZID initiated the unpair; FALSE otherwise.
 */
uint8 zidCld_Unpair(uint8 dstIndex);

/**************************************************************************************************
 * @fn          zidCld_ReadItem
 *
 * @brief       This API is used to read the ZID Configuration Interface item
 *              from the Configuration Parameters table, the State Attributes
 *              table, or the Constants table.
 *
 * input parameters
 *
 * @param       itemId - The Configuration Interface item identifier.
 *                       See special note below for 0xF1-0xFF non-standard descriptors.
 * @param       len    - The length in bytes of the item's data.
 *
 * output parameters
 *
 * @param       *pValue - A pointer to the item identifier's data from table.
 *
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidCld_ReadItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 * @fn          zidCld_WriteItem
 *
 * @brief       This API is used to write ZID Configuration Interface parameters
 *              to the Configuration Parameters table, and permitted attributes
 *              to the State Attributes table.
 *
 * input parameters
 *
 * @param       itemId:  The Configuration Interface item identifier.
 *                       See special note below for 0xF1-0xFF non-standard descriptors.
 * @param       len:     The length in bytes of the item identifier's data.
 * @param       *pValue: A pointer to the item's data.
 *
 * input parameters
 *
 * None.
 *
 * @return      RTI_SUCCESS
 *              RTI_ERROR_INVALID_PARAMETER
 *              RTI_ERROR_OSAL_NV_OPER_FAILED
 *              RTI_ERROR_OUT_OF_MEMORY
 *              RTI_ERROR_UNSUPPORTED_ATTRIBUTE
 */
rStatus_t zidCld_WriteItem(uint8 itemId, uint8 len, uint8 *pValue);

/**************************************************************************************************
 * @fn          zidCld_WriteNonStdDescComp
 *
 * @brief       Store (fragmented, if necessary) a non standard descriptor component into NV
 *
 * input parameters
 *
 * @param  descNum       - non-std descriptor number (0 to (aplcMaxNonStdDescCompsPerHID - 1))
 * @param  pBuf          - Pointer to non-std descriptor component as defined in Table 9 of ZID Profile, r15
 *
 * output parameters
 *
 * None.
 *
 * @return
 *
 * None.
 *
 */
void zidCld_WriteNonStdDescComp( uint8 descNum, zid_non_std_desc_comp_t *pBuf );

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
