

#include "libopensc/opensc.h"

#ifndef __EP_LOCKING__HH
#define __EP_LOCKING__HH

#ifdef __cplusplus
extern "C" {
#endif 

extern sc_thread_context_t sc_thread_ctx;

/*
 * Locking functions
 */

int ep_init_lock(void);

/*
 * Free the lock - note the lock must be held when
 * you come here
 */
void ep_free_lock(void);

#ifdef __cplusplus
}
#endif 

#endif