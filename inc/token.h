

#ifndef TOKEN_H
#define TOKEN_H	1

#include <string>
#include "stdlib.h"

#include "include/internal/cef_ptr.h"
#include "include/cef_values.h"

#include "usbtoken/client_handler.h"

#include "inc/ep_pkcs15.h"
#include "inc/watcher.h"

#include <openssl\evp.h>
#include <openssl\conf.h>

#include <openssl\engine.h>

namespace epsilon {

	#define E_TOKEN_APPLICATION_NAME "Epsilon USB Token Manager"

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

	#define E_TOKEN_INSERTED	"token_inserted"	
	#define E_TOKEN_REMOVED		"token_removed"	

	#define EP_KEY_USAGE_ALL SC_PKCS15INIT_X509_DIGITAL_SIGNATURE | SC_PKCS15INIT_X509_NON_REPUDIATION | SC_PKCS15INIT_X509_KEY_ENCIPHERMENT | SC_PKCS15INIT_X509_DATA_ENCIPHERMENT | SC_PKCS15INIT_X509_KEY_AGREEMENT | SC_PKCS15INIT_X509_KEY_CERT_SIGN  | SC_PKCS15INIT_X509_CRL_SIGN

	#define EP_KEY_USAGE_PRKEY SC_PKCS15INIT_X509_DIGITAL_SIGNATURE | SC_PKCS15INIT_X509_NON_REPUDIATION

	class Token : public ClientHandler::ProcessMessageDelegate {
		
	public:

		Token() {
		
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
	OPENSSL_config(NULL);
#endif
	/* OpenSSL magic */
	SSLeay_add_all_algorithms();
	CRYPTO_malloc_init();
#ifdef RANDOM_POOL
	if (!RAND_load_file(RANDOM_POOL, 32))
		util_fatal("Unable to seed random number pool for key generation");
#endif		
		};

		bool EraseToken();
		bool InitToken(const char *, const char *, const char *);
		bool InitPIN(const char *, const char *);
		bool ChangePIN(const char *, const char *);
		bool UnblockPIN(const char *, const char *);
		bool VerifyPIN(const char *);

		bool GetPubKeys(CefRefPtr<TokenContext>, ep_key_info **, size_t *);
		bool GetCerts(CefRefPtr<TokenContext>, ep_key_info **, size_t *);
		bool GetPrKeys(CefRefPtr<TokenContext>, ep_key_info **, size_t *);
	
		bool GetPubKey(CefRefPtr<TokenContext>, const char *, ep_key_info *, const char *);
		bool GetPrKey(CefRefPtr<TokenContext>, const char *, ep_key_info *);
		bool GetCert(CefRefPtr<TokenContext>, const char *, ep_key_info *);
	
		bool GenKey(CefRefPtr<TokenContext>, char *, const char *, char **);
		
		bool ImportPrKey(CefRefPtr<TokenContext>, const char *, size_t, const char *, const char *, const char *, const char *);
		bool ImportPubKey(CefRefPtr<TokenContext>, const char *, size_t, const char *, const char *, const char *);
		bool ImportCert(CefRefPtr<TokenContext>, const char *, size_t, const char *, const char *, const char *);
		
		bool DelPrKey(CefRefPtr<TokenContext>, const char *, const char *);
		bool DelX509(CefRefPtr<TokenContext>, const char *, const char *);

		bool GenX509Req(CefRefPtr<TokenContext>, CefRefPtr<ClientHandler> handler, const char *, const unsigned char *, 
			const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *, 
			const unsigned char *, const unsigned char *, const char *);

		bool ExportX509(CefRefPtr<TokenContext>, CefRefPtr<ClientHandler> handler, const char *, const char *, const char *, unsigned int);

	  virtual bool OnProcessMessageReceived(
	    CefRefPtr<ClientHandler> handler,
	    CefRefPtr<CefBrowser> browser,
	    CefProcessId source_process,
	    CefRefPtr<CefProcessMessage> message) OVERRIDE;

	private:
		
	  IMPLEMENT_REFCOUNTING(Token);	
	};

	// Create the render delegate.
	void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet &);

	CefRefPtr<CefProcessMessage> CreateKeysMsg(const std::string &, const std::string &, bool, struct ep_key_info **, size_t);
	CefRefPtr<CefProcessMessage> CreateKeyMsg(const std::string &, const std::string &, bool, ep_key_info);
	CefRefPtr<CefProcessMessage> CreateKeyGenMsg(const std::string &, const std::string &, const char *id, bool ok);

	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, const std::string &, bool);
	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, const std::string &, const char *);	
	CefRefPtr<CefProcessMessage> CreateMsg(const std::string &, const std::string &, const char *, ep_token_info *);
	
	std::wstring CreateWindowText(const CefString);

} //namespace epsilon

#endif  //TOKEN