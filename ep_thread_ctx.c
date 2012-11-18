
#include "inc/ep_thread_ctx.h"

#include "stdlib.h"

#include "pkcs11/pkcs11.h"

#if defined(HAVE_PTHREAD)

#include <pthread.h>

CK_RV __ep_mutex_create(void **mutex) {
	pthread_mutex_t *m = malloc(sizeof(*mutex));
	if (m == NULL)
		return CKR_GENERAL_ERROR;;
	pthread_mutex_init(m, NULL);
	*mutex = m;
	return CKR_OK;
}

CK_RV __ep_mutex_lock(void *p) {
	if (pthread_mutex_lock((pthread_mutex_t *) p) == 0)
		return CKR_OK;
	else
		return CKR_GENERAL_ERROR;
}

CK_RV __ep_mutex_unlock(void *p) {
	if (pthread_mutex_unlock((pthread_mutex_t *) p) == 0)
		return CKR_OK;
	else
		return CKR_GENERAL_ERROR;
}

CK_RV __ep_mutex_destroy(void *p) {
	pthread_mutex_destroy((pthread_mutex_t *) p);
	free(p);
	return CKR_OK;
}

static CK_C_INITIALIZE_ARGS _def_locks = {
	__ep_mutex_create, __ep_mutex_destroy, __ep_mutex_lock, __ep_mutex_unlock, 0, NULL };

#elif defined(_WIN32)

#include <windows.h>

#undef CreateMutex

CK_RV __ep_mutex_create(void **mutex) {
	CRITICAL_SECTION *m;

	m = (CRITICAL_SECTION *)malloc(sizeof(*m));
	if (m == NULL)
		return CKR_GENERAL_ERROR;
	InitializeCriticalSection(m);
	*mutex = m;
	return CKR_OK;
}

CK_RV __ep_mutex_lock(void *p) {
	EnterCriticalSection((CRITICAL_SECTION *) p);
	return CKR_OK;
}

CK_RV __ep_mutex_unlock(void *p) {
	LeaveCriticalSection((CRITICAL_SECTION *) p);
	return CKR_OK;
}

CK_RV __ep_mutex_destroy(void *p) {
	DeleteCriticalSection((CRITICAL_SECTION *) p);
	free(p);
	return CKR_OK;
}

static CK_C_INITIALIZE_ARGS _def_locks = {
	__ep_mutex_create, __ep_mutex_destroy, __ep_mutex_lock, __ep_mutex_unlock, 0, NULL };

#endif

static CK_C_INITIALIZE_ARGS_PTR	global_locking;
static void *			global_lock = NULL;

#if (defined(HAVE_PTHREAD) || defined(_WIN32))

#define HAVE_OS_LOCKING
	static CK_C_INITIALIZE_ARGS_PTR default_mutex_funcs = &_def_locks;

#else
	static CK_C_INITIALIZE_ARGS_PTR default_mutex_funcs = NULL;

#endif

/* wrapper for the locking functions for libopensc */
static int 
ep_create_mutex(void **m) {
	if (global_locking == NULL)
		return SC_SUCCESS;
	if (global_locking->CreateMutex(m) == CKR_OK)
		return SC_SUCCESS;
	else
		return SC_ERROR_INTERNAL;
}

static int 
ep_lock_mutex(void *m) {
	if (global_locking == NULL)
		return SC_SUCCESS;
	if (global_locking->LockMutex(m) == CKR_OK)
		return SC_SUCCESS;
	else
		return SC_ERROR_INTERNAL;
}

static int 
ep_unlock_mutex(void *m) {
	if (global_locking == NULL)
		return SC_SUCCESS;
	if (global_locking->UnlockMutex(m) == CKR_OK)
		return SC_SUCCESS;
	else
		return SC_ERROR_INTERNAL;
	
}

static int 
ep_destroy_mutex(void *m) {
	if (global_locking == NULL)
		return SC_SUCCESS;
	if (global_locking->DestroyMutex(m) == CKR_OK)
		return SC_SUCCESS;
	else
		return SC_ERROR_INTERNAL;
}

sc_thread_context_t sc_thread_ctx = {
	0, ep_create_mutex, ep_lock_mutex,
	ep_unlock_mutex, ep_destroy_mutex, NULL
};

/*
 * Locking functions
 */

int ep_init_lock(void) {
	int rv = SC_SUCCESS;

	if (global_lock)
		return SC_SUCCESS;

	/* If the app tells us OS locking is okay,
	 * use that. Otherwise use the supplied functions.
	 */
	global_locking = NULL;

	/* Shall not be used in threaded environment, use operating system locking */
	global_locking = default_mutex_funcs;

	if (global_locking != NULL) {
		/* create mutex */
		rv = global_locking->CreateMutex(&global_lock);
	}

	return rv;
}

static void
__ep_unlock(void *lock) {
	if (!lock)
		return;
	if (global_locking) {
		while (global_locking->UnlockMutex(lock) != CKR_OK)
			;
	} 
}

/*
 * Free the lock - note the lock must be held when
 * you come here
 */
void ep_free_lock(void) {
	void	*tempLock;

	if (!(tempLock = global_lock))
		return;

	/* Clear the global lock pointer - once we've
	 * unlocked the mutex it's as good as gone */
	global_lock = NULL;

	/* Now unlock. On SMP machines the synchronization
	 * primitives should take care of flushing out
	 * all changed data to RAM */
	__ep_unlock(tempLock);

	if (global_locking)
		global_locking->DestroyMutex(tempLock);
	global_locking = NULL;
}