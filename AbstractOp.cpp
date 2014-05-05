
// Bring in platform-specific definitions.
#if defined(WIN32)
#include <windows.h>
#include <process.h>
#endif	

#include "inc\op.h"

void TOnEnd(Op *op, CefRefPtr<CefProcessMessage> response) {
	op->OnEnd(response);
	
	//We can call ~Op now
	op->Release();
}

// Bring in platform-specific definitions.
#if defined(WIN32)

void TRun(void *pArg) {

	Op *op = static_cast<Op*>(pArg);
	
	CefRefPtr<CefProcessMessage> response = op->Do();
	
	response.get() ?

		CefPostTask(TID_UI, NewCefRunnableFunction(TOnEnd, op, response))
		
		:
		
		//We can call ~Op now
		op->Release()

		;

	_endthread();
}

#endif

void Op::RunOp(CefRefPtr<Op> op) {

	// Make sure an additional ref is kept for the thread function so that ~Op is not called
	op->AddRef();

// Bring in platform-specific definitions.
#if defined(WIN32)
	_beginthread(TRun, 0, (void *)op.get());	
#endif	

}
