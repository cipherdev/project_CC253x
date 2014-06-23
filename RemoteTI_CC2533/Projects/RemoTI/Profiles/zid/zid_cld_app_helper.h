/**************************************************************************************************
  Filename:       zid_cld_app_helper.h
**************************************************************************************************/
#ifndef ZID_CLD_APP_HELPER_H
#define ZID_CLD_APP_HELPER_H

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

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Functions
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          zidCldAppHlp_WriteNonStdDescComp
 *
 * @brief       Store (fragmented, if necessary) a non standard descriptor component into NV
 *
 * input parameters
 *
 * @param  descNum  - non-std descriptor number (0 to (aplcMaxNonStdDescCompsPerHID - 1))
 * @param  pBuf     - Pointer to non-std descriptor component as defined in Table 9 of ZID Profile, r15
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
rStatus_t zidCldAppHlp_WriteNonStdDescComp( uint8 descNum, zid_non_std_desc_comp_t *pBuf );

/**************************************************************************************************
 * @fn          zidCldAppHlp_WriteNullReport
 *
 * @brief       Store a NULL report corresponding to a non standard descriptor component into NV
 *
 * input parameters
 *
 * @param  descNum  - non-std descriptor number for report (0 to (aplcMaxNonStdDescCompsPerHID - 1))
 * @param  pBuf     - Pointer to null report as defined in Table 14 of ZID Profile, r15
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
rStatus_t zidCldAppHlp_WriteNullReport( uint8 descNum, zid_null_report_t *pBuf );

#ifdef __cplusplus
};
#endif

#endif
/**************************************************************************************************
*/
