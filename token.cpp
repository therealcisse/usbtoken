
#include <sstream>

#include "inc/token.h"
#include "inc/pkcs15.h"
#include "inc/ep_pkcs15.h"
#include "inc/op.h"

#include "inc/util_pkcs15.h"
#include "resource.h"

#if defined(WIN32)
#include "windows.h"
#endif

namespace epsilon {

bool Token::EraseToken() {
	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {
  	sc_pkcs15_card *p15card;
  	int r = -1;

  	if ((p15card = CreateP15Card(ctx)) == NULL)
			goto out;

  	r = sc_pkcs15init_erase_card(p15card, ctx->GetProfile(), NULL);

  	DestroyP15Card(p15card);

		out: ctx->Destroy();

		return r < 0 ? false : true;
	}

	return false;
}

bool Token::InitToken(const char* sopin, const char* sopuk, const char *label) {
	
	if(!EraseToken())
		return false;

	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {
	  sc_pkcs15init_initargs args;
	  sc_pkcs15_auth_info info;
	  const char *serial =  NULL;
	  int so_puk_disabled = 0;
	  int r;	

	  memset(&args, 0, sizeof(args));

	  sc_pkcs15init_get_pin_info(ctx->GetProfile(), SC_PKCS15INIT_SO_PIN, &info);

	  if ((info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_UNBLOCK_DISABLED) 
	      && (info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN))
	    so_puk_disabled = 1;

	  args.so_pin = (const u8 *) sopin;
	  if (args.so_pin)
	    args.so_pin_len = strlen((const char *) args.so_pin);

	  if (!so_puk_disabled)   {
	    args.so_puk = (const u8 *) sopuk;
	    if (args.so_puk)
	      args.so_puk_len = strlen((const char *) args.so_puk);
	  }

	  args.serial = (const char *)serial;
	  args.label = label;

	  r = sc_pkcs15init_add_app(ctx->GetCard(), ctx->GetProfile(), &args);		

		ctx->Destroy();

		return r < 0 ? false : true;
	}

	return false;
}

//bool Token::InitPIN(const char *pin, const char *puk) {
//	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
//	ASSERT(!ctx->InContext());
//
//	if(ctx->Bind()) {
//		  sc_pkcs15_card *p15card;
//		  struct sc_pkcs15init_pinargs args;
//		  int r = -1;
//
//		  if ((p15card = BindP15Card(ctx)) == NULL)
//				goto out;
//
//  memset(&args, 0, sizeof(args));
//  //sc_pkcs15_format_id(pin_id, &args.auth_id);
//  args.pin = (u8 *) pin;
//  args.pin_len = strlen(pin);
//
//  args.puk = (u8 *) puk;
//  args.puk_len = puk ? strlen(puk) : 0;
//
//  r = sc_pkcs15init_store_pin(p15card, ctx->GetProfile(), &args);			
//
//		 ReleaseP15Card(p15card);
//		 
//		 out: ctx->Destroy();		
//		 return r < 0 ? false : true;	
//	}
//
//	return false;
//}

bool Token::ChangePIN(const char *pincode, const char *newpin) {
	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {

	  sc_pkcs15_card *p15card;
		sc_pkcs15_object *pin_obj;	  
	  int r = -1;

	  if ((p15card = BindP15Card(ctx)) == NULL)
			goto out;
	
	  if (!(pin_obj = ::util::get_pin_info(p15card)))
	    goto release_p15card;

	  r = sc_pkcs15_change_pin(p15card, pin_obj, 
	      (const u8 *) pincode, pincode ? strlen(pincode) : 0,
	      (const u8 *) newpin, newpin ? strlen(newpin) : 0);

	  release_p15card: ReleaseP15Card(p15card);
	 
	  out: ctx->Destroy();	
	  return r < 0 ? false : true;		
	}

	return false;
}

bool Token::UnblockPIN(const char* puk, const char* pin) {
	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {	
	  sc_pkcs15_card *p15card;
	  //sc_pkcs15_auth_info *pinfo = NULL;
	  sc_pkcs15_object *pin_obj;
	  int r = -1;

	  if ((p15card = BindP15Card(ctx)) == NULL)
			goto out;

  if (!(pin_obj = ::util::get_pin_info(p15card)))
    goto release_p15card;

  //pinfo = (sc_pkcs15_auth_info *) pin_obj->data;
  //::util::ep_refresh_pin_info(p15card->card, pinfo);

  /*if (pinfo->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN) 
    return 1;*/

  r = sc_pkcs15_unblock_pin(p15card, pin_obj, 
      (const u8 *) puk, puk ? strlen(puk) : 0,
      (const u8 *) pin, pin ? strlen(pin) : 0);

	  /* Test PUK Locking */
	  //{
	  	//sc_pkcs15_auth_info *pinfo = NULL;
	  	
	  	/* Compare these */ 	

	  	//pinfo = (sc_pkcs15_auth_info *) pin_obj->data;
			//sc_pkcs15init_get_pin_info(ctx->GetProfile(), SC_PKCS15INIT_USER_PUK, pinfo_usr);
			//sc_pkcs15init_get_pin_info(ctx->GetProfile(), SC_PKCS15INIT_SO_PUK, pinfo_so);
	  	//::util::ep_refresh_pin_info(p15card->card, pinfo); 
	  	//::util::ep_refresh_pin_info(p15card->card, pinfo_so); 
	  //}


  /*if (r == SC_ERROR_PIN_CODE_INCORRECT) {
    util_debug("PUK code incorrect; tries left: %d\n", pinfo->tries_left);
    return 3;
  } else if (r) {
    util_debug("PIN unblocking failed: %s\n", sc_strerror(r));
    return 2;
  }*/

	
		release_p15card: ReleaseP15Card(p15card);
		 
		out: ctx->Destroy();	
		return r < 0 ? false : true;		
	 }	

	 return false;
}

bool Token::VerifyPIN(const char *pin) {
	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {	
	  sc_pkcs15_card *p15card;
	  int r = -1;

	  if ((p15card = BindP15Card(ctx)) == NULL)
			goto out;
  
		r = ::util::ep_verify_pin(p15card, pin);
	
		 ReleaseP15Card(p15card);
		 
		 out: ctx->Destroy();			
		 return r < 0 ? false : true;
	}
	return false;
}

const char* kMsgName  = "kMsg";

const char* kMessageShow       = "show";
const char* kMessageGetStatus  = "getstatus";
const char* kMessageErase      = "erase";
const char* kMessageInit       = "init";
const char* kMessageInitPIN    = "setpin";
const char* kMessageChangePIN  = "changepin";
const char* kMessageUnblockPIN = "unblock";
const char* kMessageVerifyPIN  = "verifypin";
const char* kMessageSetPIN     = "setpin";
const char* kMessageLogout     = "logout";

//const char* kMessageSetTitle   = "titleset";
const char* kMessageGetReaders   = "getreaders";

const char* kMessageGetCert   = "get.x509-certificate";
const char* kMessageGetPubkey = "get.pubkey";
const char* kMessageGetPrkey  = "get.prkey";

const char* kMessageGetPrKeys  = "get.prkeys";
const char* kMessageGetPubKeys = "get.pubkeys";
const char* kMessageGetCerts   = "get.x509-certificates";

const char* kMessageGenKey   = "keygen";
const char* kMessageGenReq   = "csr.gen";

const char* kMessageImportPubKey = "import.pubkey";
const char* kMessageImportPrKey  = "import.prkey";
const char* kMessageImportCert   = "import.x509";

const char* kMessageExportX509   = "export.x509";

const char* kMessageDelX509  = "del.x509";
const char* kMessageDelPrKey = "del.prkey";

const char* kMessageSetLang = "set.lang";

bool Token::OnProcessMessageReceived(
	CefRefPtr<ClientHandler> handler,
	CefRefPtr<CefBrowser> browser,
	CefProcessId source_process,
	CefRefPtr<CefProcessMessage> message) {

	//::util::ep_gen_x509_req(ctx, handler, 
	//"8e8e701ea54e827ac8bef445c23ec40e4cc1bbc3", (const unsigned char *)"qq", (const unsigned char *)"qq", 
	//(const unsigned char *)"qq", (const unsigned char *)"qq", (const unsigned char *)"qq", (const unsigned char *)"qq", (const unsigned char *)"qq@qq.com", "4444");

	const std::string & message_name = message->GetName();

	if(message_name == kMessageGetStatus) {

		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {

			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));
			bool isAuthenticated = args->HasKey(CefString(E_KEY_AUTHENTICATION_DATA));

			class GetStatusOp : public Op {
				const char *MsgId;
				bool isAuthenticated;
			public:
				GetStatusOp(CefRefPtr<CefBrowser> browser, const char *MsgId, bool isAuthenticated)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  isAuthenticated(isAuthenticated)
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
					ep_token_info info;

					/* Does this really means that token is blank? */
					if(!ctx->Bind())
						return CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_ABSENT);

					/* Get all token information */
					GetTokenInfo(ctx, &info);
					ctx->Destroy();

					if(info.token_absent) 
						return CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_ABSENT);

					if(!info.token_initialized)
						return CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_BLANK, &info);

					if(info.readonly)
						return CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_READONLY);

					if(info.inuse)
						return CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_IN_USE);

					if(!info.pin_initialized)
						return CreateMsg(kMessageGetStatus, MsgId, E_PIN_NOT_INITIALIZED);

					if(info.tries_left == 0)
						return CreateMsg(kMessageGetStatus, MsgId, E_PIN_BLOCKED, &info);
  
					return isAuthenticated ?

						CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_AUTHENTICATED)

						:

						CreateMsg(kMessageGetStatus, MsgId, E_TOKEN_AUTHENTICATION_REQUIRED, &info);
				}

				IMPLEMENT_REFCOUNTING(GetStatusOp);
			};

			GetStatusOp::RunOp(new GetStatusOp(browser, MsgId.ToString().c_str(), isAuthenticated));
			return true;				
		}			
	}

	if(message_name == kMessageSetLang) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {	
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);			
			
			int langId = args->GetInt(CefString("LangID"));
			CefString MsgId = args->GetString(CefString("MsgId"));

			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(kMessageSetLang, MsgId, handler->SetLang(langId)));				

			return true;
		}
	}

	/* Login */

	if(message_name == kMessageVerifyPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);			
			
			CefString MsgId = args->GetString(CefString("MsgId"));
			CefString pin = args->GetString(CefString("pin"));

			class LoginOp: public Op {
				const char *MsgId;
				const char *pin;
			public:

				LoginOp(CefRefPtr<CefBrowser> browser, const char *pin, const char *MsgId): 
					Op(browser), 
					pin(::strdup(pin)), 
					MsgId(::strdup(MsgId)) 
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					bool r = VerifyPIN(pin);

					ep_token_info info;
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
				
						/* Get all token information */
						GetTokenInfo(ctx, &info);
						ctx->Destroy();

						return r ?

							 CreateMsg(kMessageVerifyPIN, MsgId, E_TOKEN_AUTHENTICATED, &info)				
			
						: 

							CreateMsg(kMessageVerifyPIN, MsgId, E_TOKEN_AUTHENTICATION_REQUIRED, &info);				
					}

					/*else 
						 Some weird error happened */

					return NULL;
				}

				IMPLEMENT_REFCOUNTING(LoginOp);
			};			
			
			LoginOp::RunOp(new LoginOp(browser, pin.ToString().c_str(), MsgId.ToString().c_str()));
			return true;
		}
	}

	/* Erase Token */

	/*if(message_name == kMessageErase) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefString MsgId = argList->GetDictionary(0)->GetString(CefString("MsgId"));			
			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(kMessageErase, MsgId, EraseToken()));				
			
		}
			
		return true;
	}*/	

	/* Init Token */

	if(message_name == kMessageInit) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
			
			CefString pin = args->GetString(CefString("pin"));			
			CefString puk = args->GetString(CefString("puk"));
			CefString label = args->GetString(CefString("label"));

			class InitOp : public Op {
				const char *MsgId;
				const char *pin;
				const char *puk;
				const char *label;
			public:
				InitOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *pin, const char *puk, const char *label)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  pin(::strdup(pin)),
					  puk(::strdup(puk)),
					  label(::strdup(label))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					return CreateMsg(kMessageInit, MsgId, InitToken(pin, puk, label));
				}

				IMPLEMENT_REFCOUNTING(InitOp);
			};
			
			InitOp::RunOp(new InitOp(browser, MsgId.ToString().c_str(), pin.ToString().c_str(), puk.ToString().c_str(), label.ToString().c_str()));
			return true;
		}
	}		

	/* Set PIN Token */

	/*if(message_name == kMessageSetPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));			
			
			CefString pin = args->GetString(CefString("pin"));			
			CefString puk = args->GetString(CefString("puk"));			

			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(kMessageSetPIN, MsgId, InitPIN(pin.ToString().c_str(), puk.ToString().c_str())));				
			
		}
			
		return true;
	}*/

	/* Change PIN Token */

	if(message_name == kMessageChangePIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
			
			CefString pin = args->GetString(CefString("pincode"));			
			CefString newpin = args->GetString(CefString("pin"));
			
			class ChangePINOp : public Op {
				const char *MsgId;
				const char *pin;
				const char *newpin;
			public:
				ChangePINOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *pin, const char *newpin)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  pin(::strdup(pin)),
					  newpin(::strdup(newpin))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					return CreateMsg(kMessageChangePIN, MsgId, ChangePIN(pin, newpin));
				}

				IMPLEMENT_REFCOUNTING(ChangePINOp);
			};
			
			ChangePINOp::RunOp(new ChangePINOp(browser, MsgId.ToString().c_str(), pin.ToString().c_str(), newpin.ToString().c_str()));
			return true;			
		}
	}		

	/* Unblock Token */

	if(message_name == kMessageUnblockPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
			
			CefString puk = args->GetString(CefString("puk"));			
			CefString pin = args->GetString(CefString("pin"));	

			class UnblockPINOp : public Op {
				const char *MsgId;
				const char *puk;
				const char *pin;
			public:
				UnblockPINOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *puk, const char *pin)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  puk(::strdup(puk)),
					  pin(::strdup(pin))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					return CreateMsg(kMessageUnblockPIN, MsgId, UnblockPIN(puk, pin));
				}

				IMPLEMENT_REFCOUNTING(UnblockPINOp);
			};
			
			UnblockPINOp::RunOp(new UnblockPINOp(browser, MsgId.ToString().c_str(), puk.ToString().c_str(), pin.ToString().c_str()));
			return true;			
		}
	}		

	/* Get Readers */

	/*if(message_name == kMessageGetReaders) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString replyto = args->GetString(CefString("replyto"));*/
/*

  sc_context_param_t ctx_param;
  memset(&ctx_param, 0, sizeof(ctx_param));
  ctx_param.ver      = 0;
  ctx_param.app_name = E_TOKEN_APPLICATION_NAME;

  sc_context *ctx = NULL;

  sc_context_create(&ctx, &ctx_param);

  unsigned int len = sc_ctx_get_reader_count(ctx);
	
  for(unsigned int i = 0; i < len; i++) {
		  sc_reader *reader = sc_ctx_get_reader_by_id(ctx, 0);
	  }

			
			sc_release_context(ctx);

			*/
			
	/*	}
			
		return true;
	}*/		

	if(message_name == kMessageGetPrKeys) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));

			class GetPrKeysOp : public Op {
				const char *MsgId;
			public:
				GetPrKeysOp(CefRefPtr<CefBrowser> browser, const char *MsgId)
					: Op(browser),
					  MsgId(::strdup(MsgId))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						struct ep_key_info *key_info[32];
						size_t len = 0;

						bool ok = GetPrKeys(ctx, key_info, &len);

						CefRefPtr<CefProcessMessage> response = CreateKeysMsg(kMessageGetPrKeys, MsgId, ok, key_info, len);	

						for(size_t i = 0; i < len; ++i)
							free(key_info[i]);

						ctx->Destroy();
						return response;
					}

					return CreateKeysMsg(kMessageGetPrKeys, MsgId, false, NULL, 0);
				}

				IMPLEMENT_REFCOUNTING(GetPrKeysOp);
			};

			GetPrKeysOp::RunOp(new GetPrKeysOp(browser, MsgId.ToString().c_str()));
			return true;
		}
	}

	/*if(message_name == kMessageGetPubKeys) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {
				ep_key_info *key_info[32];
				size_t len = 0;

				bool ok = GetPubKeys(ctx, key_info, &len);

			browser->SendProcessMessage(PID_RENDERER, 
				CreateKeysMsg(kMessageGetPubKeys, MsgId, ok, key_info, len));		

			for(size_t i = 0; i < len; ++i)
				free(key_info[i]);

				ctx->Destroy();
			}
		}
	}*/

	if(message_name == kMessageGetCerts) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));

			class GetX509sOp : public Op {
				const char *MsgId;
			public:
				GetX509sOp(CefRefPtr<CefBrowser> browser, const char *MsgId)
					: Op(browser),
					  MsgId(::strdup(MsgId))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						struct ep_key_info *key_info[32];
						size_t len = 0;

						bool ok = GetCerts(ctx, key_info, &len);

						CefRefPtr<CefProcessMessage> response = CreateKeysMsg(kMessageGetCerts, MsgId, ok, key_info, len);	

						for(size_t i = 0; i < len; ++i)
							free(key_info[i]);

						ctx->Destroy();
						return response;
					}

					return CreateKeysMsg(kMessageGetCerts, MsgId, false, NULL, 0);
				}

				IMPLEMENT_REFCOUNTING(GetX509sOp);
			};

			GetX509sOp::RunOp(new GetX509sOp(browser, MsgId.ToString().c_str()));
			return true;
		}
	}

	if(message_name == kMessageGetCert) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));
			CefString id = args->GetString(CefString("id"));

			class GetX509Op : public Op {
				const char *MsgId;
				const char *id;
			public:
				GetX509Op(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *id)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  id(::strdup(id))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						ep_key_info key_info;

						bool ok = GetCert(ctx, id, &key_info);						

						CefRefPtr<CefProcessMessage> response = CreateKeyMsg(kMessageGetCert, MsgId, ok, key_info);	

						if(key_info.key.cert.data) free(key_info.key.cert.data);

						ctx->Destroy();
						return response;
					}		

					return CreateMsg(kMessageGetCert, MsgId, false);
				}

				IMPLEMENT_REFCOUNTING(GetX509Op);
			};

			GetX509Op::RunOp(new GetX509Op(browser, MsgId.ToString().c_str(), id.ToString().c_str()));
			return true;
		}
	}

	/*if(message_name == kMessageGetPubkey) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));
			CefString id = args->GetString(CefString("id"));
			CefString pin = args->GetString(CefString("authData"));

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {
				ep_key_info key_info;

				bool ok = GetPubKey(ctx, id.ToString().c_str(), &key_info, pin.ToString().c_str());

			browser->SendProcessMessage(PID_RENDERER, 
				CreateKeyMsg(kMessageGetPubkey, MsgId, ok, key_info));	

				if(key_info.key.pubkey.data) free(key_info.key.pubkey.data);

				ctx->Destroy();
			}
		}
	}*/

	/*if(message_name == kMessageGetPrkey) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
			CefString id = args->GetString(CefString("id"));

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {
				ep_key_info key_info;

				bool ok = GetPrKey(ctx, id.ToString().c_str(), &key_info);

			browser->SendProcessMessage(PID_RENDERER, 
				CreateKeyMsg(kMessageGetPrkey, MsgId, ok, key_info));			

				ctx->Destroy();
			}
		}
	}*/

	if(message_name == kMessageImportPrKey) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));			
			
			CefString format = args->GetString(CefString("format"));			
			CefString label = args->GetString(CefString("label"));
			CefString passphrase = args->GetString(CefString("passphrase"));
			CefString authData = args->GetString(CefString("authData"));

			CefString data = args->GetString(CefString("data"));	
			size_t data_len = args->GetInt(CefString("data_len"));	

			//CefString path = args->GetString(CefString("path"));

			class ImportPrKeyOp : public Op {
				const char *MsgId;
				const char *data;
				size_t data_len;
				const char *format;
				const char *label;
				const char *passphrase;
				const char *authData;
			public:
				ImportPrKeyOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *data, size_t data_len, const char *label, const char *format, const char *passphrase, const char *authData)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  data(::strdup(data)),
					  data_len(data_len),
					  format(::strdup(format)),
					  label(::strdup(label)),
					  passphrase(::strdup(passphrase)),
					  authData(::strdup(authData))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						bool r = ImportPrKey(ctx, data, data_len, label, format, passphrase, authData);
						ctx->Destroy();

						// needs: id, authid
						return CreateMsg(kMessageImportPrKey, MsgId, r);						
					}

					return CreateMsg(kMessageImportPrKey, MsgId, false);
				}

				IMPLEMENT_REFCOUNTING(ImportPrKeyOp);
			};

			ImportPrKeyOp::RunOp(new ImportPrKeyOp(browser, MsgId.ToString().c_str(), data.ToString().c_str(), data_len, label.ToString().c_str(), format.ToString().c_str(), passphrase.ToString().c_str(), authData.ToString().c_str()));
			return true;
		}
	}

	//if(message_name == kMessageImportPubKey) {
	//	CefRefPtr<CefListValue> argList = message->GetArgumentList();			
	//	if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
	//		
	//		CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
	//		CefString MsgId = args->GetString(CefString("MsgId"));			
	//		
	//		CefString format = args->GetString(CefString("format"));			
	//		CefString label = args->GetString(CefString("label"));
	//		CefString authData = args->GetString(CefString("authData"));

	//		CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	//		
	//		if(ctx->Bind()) {

	//			CefString data = args->GetString(CefString("data"));
	//			size_t data_len = args->GetInt(CefString("data_len"));

	//			// needs: id
	//		browser->SendProcessMessage(PID_RENDERER, 
	//			CreateMsg(kMessageImportPubKey, MsgId, ImportPubKey(ctx, data.ToString().c_str(), data_len, label.ToString().c_str(), format.ToString().c_str(), authData.ToString().c_str())));			

	//			ctx->Destroy();
	//		}
	//	}
	//}

	if(message_name == kMessageImportCert) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
						
			CefString format = args->GetString(CefString("format"));			
			CefString label = args->GetString(CefString("label"));
			CefString authData = args->GetString(CefString("authData"));

			CefString data = args->GetString(CefString("data"));	
			size_t data_len = args->GetInt(CefString("data_len"));	

			class ImportX509Op : public Op {
				const char *MsgId;
				const char *data;
				size_t data_len;
				const char *format;
				const char *label;
				const char *authData;
			public:
				ImportX509Op(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *data, size_t data_len, const char *label, const char *format, const char *authData)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  data(::strdup(data)),
					  data_len(data_len),
					  format(::strdup(format)),
					  label(::strdup(label)),
					  authData(::strdup(authData))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						bool r = ImportCert(ctx, data, data_len, label, format, authData);
						ctx->Destroy();

						return CreateMsg(kMessageImportCert, MsgId, r);				
					}

					return CreateMsg(kMessageImportCert, MsgId, false);
				}

				IMPLEMENT_REFCOUNTING(ImportX509Op);
			};

			ImportX509Op::RunOp(new ImportX509Op(browser, MsgId.ToString().c_str(), data.ToString().c_str(), data_len, label.ToString().c_str(), format.ToString().c_str(), authData.ToString().c_str()));
			return true;
		}
	}

	if(message_name == kMessageGenKey) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
			
			CefString label = args->GetString(CefString("label"));
			CefString authData = args->GetString(CefString("authData"));

			class GenKeyOp : public Op {
				const char *MsgId;
				const char *label;
				const char *authData;
			public:
				GenKeyOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *label, const char *authData)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  label(::strdup(label)),
					  authData(::strdup(authData))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {

						char *id = NULL;
						bool ok = GenKey(ctx, (char *) label, authData, &id);

						ctx->Destroy();
						return CreateKeyGenMsg(kMessageGenKey, MsgId, id, ok);							
					}

					return CreateKeyGenMsg(kMessageGenKey, MsgId, NULL, false);
				}

				IMPLEMENT_REFCOUNTING(GenKeyOp);
			};

			GenKeyOp::RunOp(new GenKeyOp(browser, MsgId.ToString().c_str(), label.ToString().c_str(), authData.ToString().c_str()));
			return true;
		}
	}

	if(message_name == kMessageDelX509) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
						
			CefString id = args->GetString(CefString("id"));
			CefString authData = args->GetString(CefString("authData"));

			class DelX509Op : public Op {
				const char *MsgId;
				const char *id;
				const char *authData;
			public:
				DelX509Op(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *id, const char *authData)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  id(::strdup(id)),
					  authData(::strdup(authData))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						bool r = DelX509(ctx, id, authData);
						ctx->Destroy();

						// needs: id
						return CreateMsg(kMessageDelX509, MsgId, r);									
					}

					return CreateMsg(kMessageDelX509, MsgId, false);							
				}

				IMPLEMENT_REFCOUNTING(DelX509Op);
			};

			DelX509Op::RunOp(new DelX509Op(browser, MsgId.ToString().c_str(), id.ToString().c_str(), authData.ToString().c_str()));
			return true;
		}
	}

	if(message_name == kMessageDelPrKey) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));		
						
			CefString id = args->GetString(CefString("id"));
			CefString authData = args->GetString(CefString("authData"));

			class DelPrKeyOp : public Op {
				const char *MsgId;
				const char *id;
				const char *authData;
			public:
				DelPrKeyOp(CefRefPtr<CefBrowser> browser, const char *MsgId, const char *id, const char *authData)
					: Op(browser),
					  MsgId(::strdup(MsgId)),
					  id(::strdup(id)),
					  authData(::strdup(authData))
				{
				}

				CefRefPtr<CefProcessMessage> Do() const {
					CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
					if(ctx->Bind()) {
						bool r = DelPrKey(ctx, id, authData);
						ctx->Destroy();

						// needs: id
						return CreateMsg(kMessageDelPrKey, MsgId, r);									
					}

					return CreateMsg(kMessageDelPrKey, MsgId, false);							
				}

				IMPLEMENT_REFCOUNTING(DelPrKeyOp);
			};

			DelPrKeyOp::RunOp(new DelPrKeyOp(browser, MsgId.ToString().c_str(), id.ToString().c_str(), authData.ToString().c_str()));
			return true;
		}
	}

	if(message_name == kMessageGenReq) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));			
						
			CefString id = args->GetString(CefString("id"));
			CefString cn = args->GetString(CefString("cn"));
			CefString o = args->GetString(CefString("o"));
			CefString ou = args->GetString(CefString("ou"));
			CefString city = args->GetString(CefString("city"));
			CefString region = args->GetString(CefString("region"));
			CefString country = args->GetString(CefString("country"));
			CefString emailAddress = args->GetString(CefString("emailAddress"));
			CefString authData = args->GetString(CefString("authData"));

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {

				// needs: id
			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(kMessageGenReq, MsgId, GenX509Req(ctx, handler, id.ToString().c_str(), (const unsigned char *)cn.ToString().c_str(), (const unsigned char *)o.ToString().c_str(), (const unsigned char *)ou.ToString().c_str(), (const unsigned char *)city.ToString().c_str(), (const unsigned char *)region.ToString().c_str(), (const unsigned char *)country.ToString().c_str(), (const unsigned char *)emailAddress.ToString().c_str(), authData.ToString().c_str())));			

				ctx->Destroy();
			}

			return true;
		}		
	}

	if(message_name == kMessageExportX509) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString MsgId = args->GetString(CefString("MsgId"));			
						
			CefString id = args->GetString(CefString("id"));
			CefString format = args->GetString(CefString("format"));
			CefString path = args->GetString(CefString("fileName"));
			CefString authData = args->GetString(CefString("authData"));

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {

				// needs: id
			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(kMessageExportX509, MsgId, ExportX509(ctx, handler, path.ToString().c_str(), id.ToString().c_str(), authData.ToString().c_str(), _stricmp(format.ToString().c_str(), "der") == 0 ? FORMAT_ASN1 : FORMAT_PEM)));			

				ctx->Destroy();
			}

			return true;
		}
	}

	if(message_name == kMessageShow) {

#if defined(WIN32)

		::SetTimer(handler->GetMainHwnd(), IDM_BROWSER_SHOW, 1500, NULL);
		return true;

#endif		
		
	}

	return false;
}

//bool Token::GetPubKeys(CefRefPtr<TokenContext> ctx, ep_key_info **keys, size_t *len) {	
//	if((*len = ::util::ep_token_read_all_pubkeys(ctx, keys, 32)) < 0) 
//		return false; 
//	else 
//		return true;
//}

bool Token::GetCerts(CefRefPtr<TokenContext> ctx, ep_key_info **keys, size_t *len) {
	if((*len = ::util::ep_token_read_all_certs(ctx, keys, 32)) < 0) 
		return false; 
	else 
		return true;
}

bool Token::GetPrKeys(CefRefPtr<TokenContext> ctx, ep_key_info **keys, size_t *len) {
	if((*len = ::util::ep_token_read_all_prkeys(ctx, keys, 32)) < 0) 
		return false; 
	else 
		return true;
}

//bool Token::GetPubKey(CefRefPtr<TokenContext> ctx, const char *id, ep_key_info *key, const char *pin) {
//	return ::util::ep_token_read_pubkey(ctx, key, id, (unsigned char *) pin) < 0 ? false : true ;
//}

//bool Token::GetPrKey(CefRefPtr<TokenContext> ctx, const char *id, ep_key_info *key) {
//	return ::util::ep_token_read_prkey(ctx, key, id) < 0 ? false : true ;
//}

bool Token::GetCert(CefRefPtr<TokenContext> ctx, const char *id, ep_key_info *key) {
	return ::util::ep_token_read_certificate(ctx, key, id) < 0 ? false : true ;
}

bool Token::GenKey(CefRefPtr<TokenContext> ctx, char *label, const char *authData, char **id) {
	return ::util::ep_generate_key(ctx, "RSA/2048", label, id, NULL, authData, EP_KEY_USAGE_PRKEY) < 0 ? false : true;
}

bool Token::ImportPrKey(CefRefPtr<TokenContext> ctx, const char *data, size_t len, const char *label, const char *format, const char *passphrase, const char *authData) {
	return ::util::ep_store_private_key(ctx, data, len, label, format, (char *)passphrase, authData) < 0 ? false : true;
}

//bool Token::ImportPubKey(CefRefPtr<TokenContext> ctx, const char *data, size_t len, const char *label, const char *format, const char *authData) {
//	return ::util::ep_store_public_key(ctx, data, len, label, format, authData) < 0 ? false : true;
//}

bool Token::ImportCert(CefRefPtr<TokenContext> ctx, const char *data, size_t len, const char *label, const char *format, const char *authData) {
	return ::util::ep_store_certificate(ctx, data, len, label, format, authData) < 0 ? false : true;
}

bool Token::GenX509Req(CefRefPtr<TokenContext> ctx, CefRefPtr<ClientHandler> handler, const char *id, const unsigned char *cn, const unsigned char *o, 
	const unsigned char *ou, const unsigned char *city, const unsigned char *region, const unsigned char *country, const unsigned char *emailAddress, const char *authData) {
	return ::util::ep_gen_x509_req(ctx, handler, id, cn, o, ou, city, region, country, emailAddress, authData) < 0 ? false : true ;
}

bool Token::ExportX509(CefRefPtr<TokenContext> ctx, CefRefPtr<ClientHandler> handler, const char *path, const char *id, const char *authData, unsigned int format) {
	return ::util::ep_export_x509_certificate(ctx, handler, path, id, authData, format) < 0 ? false : true ;	
}

bool Token::DelPrKey(CefRefPtr<TokenContext> ctx, const char *id, const char *authData) {
	return ::util::ep_delete_prkey(ctx, id, authData) < 0 ? false : true ;
}

bool Token::DelX509(CefRefPtr<TokenContext> ctx, const char *id, const char *authData) {
	return ::util::ep_delete_x509_certificate(ctx, id, authData) < 0 ? false : true ;
}

CefRefPtr<CefProcessMessage> CreateKeysMsg(const std::string &key, const std::string &id, bool ok, struct ep_key_info **key_info, size_t len) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("MsgKey", key);
	args->SetString("MsgId", id);
	args->SetBool("ok", ok);

	CefRefPtr<CefListValue> keys = CefListValue::Create();
	if(ok)		
		for(unsigned int i = 0; i < len; ++i) {
			ep_key_info *key = key_info[i];
			CefRefPtr<CefDictionaryValue> keyobj = CefDictionaryValue::Create();
			
			if(key->type == EP_KEY_TYPE_CERT) {
				keyobj->SetString("type", "X509Certificate");
				//keyobj->SetString("data", (char *) key->key.cert.data);
			}

			else if(key->type == EP_KEY_TYPE_PUBKEY) {
				keyobj->SetString("type", "PubKey");
				//keyobj->SetString("data", (char *) key->key.pubkey.data);
			}

			else if(key->type == EP_KEY_TYPE_PRKEY) {
				keyobj->SetString("type", "PrKey");
			}

			
			keyobj->SetString("id", (char *) key->id);
			keyobj->SetString("authid", (char *) key->authid);
			keyobj->SetString("label", key->label);	
			keyobj->SetBool("native", key->native <= 0?false:true);	

			keys->SetDictionary(i, keyobj);
		}			
	args->SetList("keys", keys);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateKeyMsg(const std::string &kName, const std::string &id, bool ok, ep_key_info key) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("MsgKey", kName);
	args->SetString("MsgId", id);
	args->SetBool("ok", ok);

	if(ok) {
		CefRefPtr<CefDictionaryValue> keyobj = CefDictionaryValue::Create();
		switch(key.type) {

			case EP_KEY_TYPE_CERT:{
				keyobj->SetString("type", "X509Certificate");
				//keyobj->SetString("data", (char *) key.key.cert.data);

				keyobj->SetString("serial", (const char *)key.key.cert.serial_number);
				keyobj->SetInt("version", key.key.cert.version);

				keyobj->SetString("notBefore", (const char *)key.key.cert.notBefore);
				keyobj->SetString("notAfter", (const char *)key.key.cert.notAfter);

				CefRefPtr<CefDictionaryValue> subject = CefDictionaryValue::Create();

				subject->SetString("cn", (const char *) key.key.cert.subject_common_name);
				subject->SetString("locality_name", (const char *) key.key.cert.subject_common_name);
				subject->SetString("state_name", (const char *) key.key.cert.subject_common_name);
				subject->SetString("o", (const char *) key.key.cert.subject_organization_name);
				subject->SetString("ou", (const char *) key.key.cert.subject_organization_unit_name);

				keyobj->SetDictionary("subject", subject);

				if(key.key.cert.issuer_set == 1) {
					CefRefPtr<CefDictionaryValue> issuer = CefDictionaryValue::Create();

					issuer->SetString("cn", (const char *) key.key.cert.issuer_common_name);
					issuer->SetString("locality_name", (const char *) key.key.cert.issuer_common_name);
					issuer->SetString("state_name", (const char *) key.key.cert.issuer_common_name);
					issuer->SetString("o", (const char *) key.key.cert.issuer_organization_name);
					issuer->SetString("ou", (const char *) key.key.cert.issuer_organization_unit_name);	

					keyobj->SetDictionary("issuer", issuer);
				}

				}break;

			case EP_KEY_TYPE_PUBKEY:{
				keyobj->SetString("type", "PubKey");
				keyobj->SetString("data", (char *) key.key.pubkey.data);
				}break;

			case EP_KEY_TYPE_PRKEY:
				keyobj->SetString("type", "PrKey");
				break;

			default:
				break;
		};
			
		keyobj->SetString("id", (char *) key.id);
		keyobj->SetString("authid", (char *) key.authid);
		keyobj->SetString("label", key.label);	
		keyobj->SetBool("native", key.native <= 0?false:true);	

		args->SetDictionary("key", keyobj);
	}

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &key, const std::string &id, bool ok) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("MsgKey", key);
	args->SetString("MsgId", id);
	args->SetBool("ok", ok);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateKeyGenMsg(const std::string &key, const std::string &msgid, const char *id, bool ok) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("MsgKey", key);
	args->SetString("MsgId", msgid);
	args->SetBool("ok", ok);

	if(ok)
		args->SetString("keyid", id);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &key, const std::string &id, const char *status) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("MsgKey", key);
	args->SetString("MsgId", id);
	args->SetString("status", status);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &key, const std::string &id, const char *status, ep_token_info *info) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMsgName);

	CefRefPtr<CefDictionaryValue> options = CefDictionaryValue::Create();
	options->SetString("MsgKey", key);
	options->SetString("MsgId", id);

	options->SetString("status", status);

	options->SetInt("maxlen", info->maxlen);
	options->SetInt("minlen", info->minlen);

	options->SetString("reader", info->reader);
	options->SetString("label", info->label);
	
	if(info->max_tries != 0) 
		options->SetInt("max_tries", info->max_tries);

	options->SetInt("tries_left", info->tries_left);

	response->GetArgumentList()->SetDictionary(0, options);
	return response;
}	

void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet& delegates) {
	delegates.insert(new Token);
}
	
}
