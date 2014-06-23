/**************************************************************************************************
  Filename:       hal_vddmon.h
**************************************************************************************************/
#ifndef HAL_VDDMON_H
#define HAL_VDDMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_defs.h"

/**************************************************************************************************
 * @fn          HalVddMonInit
 *
 * @brief       This function is the subsystem initialization function and should be called as
 *              early as possible during initialization - after GPIO's are safe with everything
 *              turned off and before turning anything on.
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
 **************************************************************************************************
 */
void HalVddMonInit(void);

/**************************************************************************************************
 * @fn          HalVddMonPoll
 *
 * @brief       This function is the subsystem polling function and should be called as
 *              frequently as possible during task servicing.
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
 **************************************************************************************************
 */
void HalVddMonPoll(void);

/**************************************************************************************************
*/

#ifdef __cplusplus
};
#endif

#endif
