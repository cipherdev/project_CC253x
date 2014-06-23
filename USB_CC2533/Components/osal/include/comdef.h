/**************************************************************************************************
  Filename:       comdef.h
**************************************************************************************************/

#ifndef COMDEF_H
#define COMDEF_H

#ifdef __cplusplus
extern "C"
{
#endif


/*********************************************************************
 * INCLUDES
 */

/* HAL */
#include "hal_types.h"
#include "hal_defs.h"

/*********************************************************************
 * SPlint Keywords
 */
#define VOID (void)

#if defined ( S_SPLINT_S )
  #define NULL_OK /*@null@*/
      // Indicates that a pointer is allowed to be NULL.

  #define INP /*@in@*/
      // Indicates that a parameter must be completely defined.

  #define OUTP /*@out@*/
      // Indicates that a passed in function parameter will be modified
      // by the function.

  #define UNUSED /*@unused@*/
      // Indicates that a parameter is intentionally not used.

  #define ONLY /*@only@*/
      // Indicates a reference is the only pointer to the object it
      // points to.   Example would be:
      //     ONLY NULL_OK void *osal_mem_alloc( uint16 size );
      //     void osal_mem_free( ONLY void *ptr );
      // Where ONLY, in this case, indicates allocated memory.

  #define READONLY /*@observer@*/
      // Indicates that the pointer points to data that is read only.

  #define SHARED /*@shared@*/
      // declares storage that may be shared arbitrarily, but never
      // released.

  #define KEEP /*@keep@*/
      // similar to only, except the caller may use the reference after
      // the call.
#else
  #define NULL_OK
  #define INP
  #define OUTP
  #define UNUSED
  #define ONLY
  #define READONLY
  #define SHARED
  #define KEEP
#endif

/*********************************************************************
 * CONSTANTS
 */

#ifndef false
  #define false 0
#endif

#ifndef true
  #define true 1
#endif

#ifndef CONST
  #define CONST const
#endif

#ifndef GENERIC
  #define GENERIC
#endif

/*** Generic Status Return Values ***/
#define SUCCESS                   0x00
#define FAILURE                   0x01
#define INVALIDPARAMETER          0x02
#define INVALID_TASK              0x03
#define MSG_BUFFER_NOT_AVAIL      0x04
#define INVALID_MSG_POINTER       0x05
#define INVALID_EVENT_ID          0x06
#define INVALID_INTERRUPT_ID      0x07
#define NO_TIMER_AVAIL            0x08
#define NV_ITEM_UNINIT            0x09
#define NV_OPER_FAILED            0x0A
#define INVALID_MEM_SIZE          0x0B
#define NV_BAD_ITEM_LEN           0x0C

/*********************************************************************
 * TYPEDEFS
 */

// Generic Status return
typedef uint8 Status_t;

// Data types
typedef int32   int24;
typedef uint32  uint24;

/*********************************************************************
 * Global System Events
 */

#define SYS_EVENT_MSG               0x8000  // A message is waiting event

/*********************************************************************
 * Global Generic System Messages
 */

#define KEY_CHANGE                0xC0    // Key Events

// OSAL System Message IDs/Events Reserved for applications (user applications)
// 0xE0 – 0xFC

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* COMDEF_H */
