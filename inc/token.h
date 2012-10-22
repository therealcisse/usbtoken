

#ifndef TOKEN_H
#define TOKEN_H	1

#include <string>

#include "include/internal/cef_ptr.h"
#include "include/cef_values.h"

#include "usbtoken/client_handler.h"

#include "inc/pkcs15.h"

namespace epsilon {

	#define E_TOKEN_BLANK	 "blank"
	#define E_TOKEN_ABSENT   "absent"

	#define E_PIN_NOT_INITIALIZED  "setpin"
	#define E_PIN_BLOCKED 		   "blocked"
	
	#define E_TOKEN_LOCKED 			"locked"
	
	#define E_TOKEN_READONLY 		"readonly"
	#define E_TOKEN_IN_USE 			"in-use"
	#define E_TOKEN_AUTHENTICATED	"logged-in"

	#define E_TOKEN_AUTHENTICATION_REQUIRED	 "auth-required"
	
	#define E_KEY_AUTHENTICATION_DATA	 "authData"

	class Token : public ClientHandler::ProcessMessageDelegate {
		
	public:

		Token(/*CefRefPtr<ClientApp> client_app*/) /*: client_app_(client_app)*/ {};

		bool EraseToken();
		bool InitToken(const char *, const char *, const char *);
		bool InitPIN(const char *, const char *);
		bool ChangePIN(const char *, const char *);
		bool UnblockPIN(const char *, const char *);
		bool VerifyPIN(const char *);

	  virtual bool OnProcessMessageReceived(
	    CefRefPtr<ClientHandler> handler,
	    CefRefPtr<CefBrowser> browser,
	    CefProcessId source_process,
	    CefRefPtr<CefProcessMessage> message) OVERRIDE;

	private:
		/*CefRefPtr<ClientApp> client_app_;*/
		
	  IMPLEMENT_REFCOUNTING(Token);	
	};

	// Create the render delegate.
	void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet &);

	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, bool);
	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, const char *);	
	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, const char *, ep_token_info *);
	std::wstring CreateWindowText(const CefString);

} //namespace epsilon

#endif  //TOKEN