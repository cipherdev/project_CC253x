/**************************************************************************************************
  Filename:       rsa_main.c
**************************************************************************************************/

/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

#include "comdef.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"
#include "hal_types.h"
#include "mac_rf4ce.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"
#include "osal_snv.h"
#include "rsa.h"

/**************************************************************************************************
 * FUNCTIONS
 **************************************************************************************************/

/**************************************************************************************************
 * @fn          main
 *
 * @brief       Start of application.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
int main(void)
{
  /* Initialize hardware */
  HAL_BOARD_INIT();

  /* Initialze the HAL driver */
  HalDriverInit();

  /* Initialize NV system */
  osal_snv_init();
  
  /* Initialize MAC */
  MAC_InitRf4ce();

  /* Initialize the operating system */
  osal_init_system();

  /* Enable interrupts */
  HAL_ENABLE_INTERRUPTS();

  /* Start OSAL */
  osal_start_system(); // No Return from here

  return 0;
}

/*************************************************************************************************
**************************************************************************************************/
