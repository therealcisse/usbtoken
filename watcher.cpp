
#include <string>

#include "inc/token.h"
#include "inc/ep_pkcs15.h"

#include "inc/watcher.h"
#include "usbtoken/client_handler.h"

#include "include/cef_v8.h"

#if defined(WIN32)
	#include <windows.h>
#endif

namespace epsilon {

CefRefPtr<CefProcessMessage> Msg(const std::string &kMessageName, const char *reader) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMessageName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("reader", reader);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

// Bring in platform-specific definitions.
#if defined(WIN32)

VOID WINAPI WatchToken(LPVOID lpParam) {
	sc_context_param_t ctx_param;
	
	ClientHandler *handler = (ClientHandler *) lpParam;
	ASSERT(handler != NULL);

	memset(&ctx_param, 0, sizeof(ctx_param));
  ctx_param.ver      = 0;
  ctx_param.app_name = E_TOKEN_APPLICATION_NAME;

	while(1) {
		
		if(!TokenContext::GetGlobalContext()->InContext()) {

			unsigned int event;
			sc_reader *found = NULL;
			sc_context *ctx = NULL;

			sc_context_create(&ctx, &ctx_param);

			if (sc_ctx_get_reader_count(ctx) == 0) {

				if(sc_wait_for_event(ctx, SC_EVENT_READER_ATTACHED, &found, &event, -1, NULL) == 0) {

					sc_ctx_detect_readers(ctx);

					/* Waiting for a card to be inserted */
					if(sc_wait_for_event(ctx, SC_EVENT_CARD_INSERTED, &found, &event, -1, NULL) == 0) {
						
						/* Card was inserted */						
	      		if(event & SC_EVENT_CARD_INSERTED) {
					CefRefPtr<CefBrowser> browser = handler->GetBrowser();        		
		      		ASSERT(browser.get());

	        		browser->SendProcessMessage(PID_RENDERER, 
								Msg(E_TOKEN_INSERTED, found->name));
	      		}
	        }
				}

				goto release;
			
			} 
			
			//if(sc_wait_for_event(ctx, SC_EVENT_READER_DETACHED, &found, &event, -1, NULL) == 0) {
			
				/* Waiting for a card to be removed */
				if(sc_wait_for_event(ctx, SC_EVENT_CARD_REMOVED, &found, &event, -1, NULL) == 0) {
					
					/* Card was removed */
					if(event & SC_EVENT_CARD_REMOVED) {
					CefRefPtr<CefBrowser> browser = handler->GetBrowser();        		
      		ASSERT(browser.get());

        		browser->SendProcessMessage(PID_RENDERER, 
							Msg(E_TOKEN_REMOVED, found->name));
				}
		}
			
			//}

			release : sc_release_context(ctx);
		}

		//Sleep(2000);	
	}
}
	
void *TokenWatcher::Init(const ClientHandler *handler) {
	DWORD ThreadID;
	HANDLE hHnd = CreateThread(NULL, 0,
										(LPTHREAD_START_ROUTINE)WatchToken,
										(LPVOID) handler, 0, &ThreadID);

	if(!SetThreadPriority(hHnd, THREAD_PRIORITY_LOWEST))
		return NULL;

	//WaitForSingleObject(hHnd, INFINITE);
	//CloseHandle(hHnd);
	//TerminateThread(hHnd, 0);
	return hHnd;
}


#endif

}