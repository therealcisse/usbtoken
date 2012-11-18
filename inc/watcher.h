
#ifndef EP_TOKEN_WATCHER_H
#define EP_TOKEN_WATCHER_H

#if defined(WIN32)
	#include <windows.h>
#endif

#include "usbtoken/client_handler.h"

namespace epsilon {

#if defined(WIN32)

	VOID WINAPI WatchToken(LPVOID lpParam);

#elif defined(OS_LINUX)

	void WatchToken();	

#endif

class TokenWatcher {
public:
	static void *Init(const ClientHandler *);
};

}


#endif //EP_TOKEN_WATCHER_H 1

