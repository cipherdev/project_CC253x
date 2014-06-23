/**************************************************************************************************
  Filename:       mac_assert.h
**************************************************************************************************/

#ifndef MAC_ASSERT_H
#define MAC_ASSERT_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_assert.h"


/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/*
 *  The MAC_ASSERT() macro is for use during debugging.  The given expression must
 *  evaluate as "true" or else an assert occurs.  At that point, the call stack
 *  feature of the debugger can pinpoint where the problem occurred.
 *
 *  The MAC_ASSERT_FORCED() macro will immediately call the assert handler routine
 *  but only if asserts are enabled.
 *
 *  The MAC_ASSERT_STATEMENT() macro is used to insert a code statement that is only
 *  needed when asserts are enabled.
 *
 *  The MAC_ASSERT_DECLARATION() macro is used to insert a declaration that is only
 *  needed when asserts are enabled.
 *
 *  To disable asserts and save code size, the project should define MACNODEBUG.
 *  Also note, if HAL assert are disabled, MAC asserts are disabled as well.
 *
 */
#ifdef MACNODEBUG
#define MAC_ASSERT(expr)
#define MAC_ASSERT_FORCED()
#define MAC_ASSERT_STATEMENT(statement)
#define MAC_ASSERT_DECLARATION(declaration)
#else
#define MAC_ASSERT(expr)                        HAL_ASSERT( expr )
#define MAC_ASSERT_FORCED()                     HAL_ASSERT_FORCED()
#define MAC_ASSERT_STATEMENT(statement)         HAL_ASSERT_STATEMENT( statement )
#define MAC_ASSERT_DECLARATION(declaration)     HAL_ASSERT_DECLARATION( declaration )
#endif


/**************************************************************************************************
 */
#endif
