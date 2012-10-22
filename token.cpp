
#include <sstream>

#include "inc/token.h"
#include "inc/pkcs15.h"
#include "inc/ep_pkcs15.h"

#include "inc/util_pkcs15.h"

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

	  //ep_token_info tinfo;
	  //ctx->GetInfo(&tinfo);

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

bool Token::InitPIN(const char *pin, const char *puk) {
	CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
	ASSERT(!ctx->InContext());

	if(ctx->Bind()) {
		  sc_pkcs15_card *p15card;
		  struct sc_pkcs15init_pinargs args;
		  int r = -1;

		  if ((p15card = BindP15Card(ctx)) == NULL)
				goto out;

  memset(&args, 0, sizeof(args));
  //sc_pkcs15_format_id(pin_id, &args.auth_id);
  args.pin = (u8 *) pin;
  args.pin_len = strlen(pin);

  args.puk = (u8 *) puk;
  args.puk_len = puk ? strlen(puk) : 0;

  r = sc_pkcs15init_store_pin(p15card, ctx->GetProfile(), &args);			

		 ReleaseP15Card(p15card);
		 
		 out: ctx->Destroy();		
		 return r < 0 ? false : true;	
	}

	return false;
}

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
    sc_pkcs15_object *pin_obj;
	  int r = -1;

	  if ((p15card = BindP15Card(ctx)) == NULL)
			goto out;
  
  if ((pin_obj = ::util::get_pin_info(p15card)) == NULL) 
    goto release_p15card;

  r = sc_pkcs15_verify_pin(p15card, pin_obj, (const u8 *) pin, pin? strlen(pin) : 0);
	
		 release_p15card: ReleaseP15Card(p15card);
		 
		 out: ctx->Destroy();			
		 return r < 0 ? false : true;
	}
	return false;
}

const char* kMessageGetStatus  = "getstatus";
const char* kMessageErase			 = "erase";
const char* kMessageInit       = "init";
const char* kMessageInitPIN    = "setpin";
const char* kMessageChangePIN  = "changepin";
const char* kMessageUnblockPIN = "unblock";
const char* kMessageVerifyPIN  = "verifypin";
const char* kMessageSetPIN     = "setpin";
const char* kMessageLogout     = "logout";

const char* kMessageSetTitle   = "titleset";

bool Token::OnProcessMessageReceived(
	CefRefPtr<ClientHandler> handler,
	CefRefPtr<CefBrowser> browser,
	CefProcessId source_process,
	CefRefPtr<CefProcessMessage> message) {

	const std::string & message_name = message->GetName();

	if(message_name == kMessageGetStatus) {

		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {

			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			std::string replyto = args->GetString(CefString("replyto"));
			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			ep_token_info info;

			/* Does this really means that token is blank? */
			if(!ctx->Bind()) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_ABSENT));
				return true;
			}

			/* Get all token information */
			ctx->GetInfo(&info);

			if(info.token_absent) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_ABSENT));
				return true;
			}

			if(!info.token_initialized) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_BLANK, &info));
				return true;
			}

			if(info.readonly) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_READONLY));
				return true;
			}

			if(info.inuse) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_IN_USE));
				return true;
			}

			if(!info.pin_initialized) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_PIN_NOT_INITIALIZED));
				return true;
			}

			if(info.tries_left == 0) {
				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_PIN_BLOCKED, &info));
				return true;
			}

			if(args->HasKey(CefString(E_KEY_AUTHENTICATION_DATA)))

				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_AUTHENTICATED));

			else

				browser->SendProcessMessage(PID_RENDERER, 
					CreateMsg(replyto, E_TOKEN_AUTHENTICATION_REQUIRED, &info));

			ctx->Destroy();

			//client_app_.RemoveMessageCallback(replyto.ToString(), browser->GetIdentifier());

			return true;				
		}			
	}

	if(message_name == kMessageSetTitle) {
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_STRING) {		
			CefString action = argList->GetString(0);
			handler->SetWindowTitle(CreateWindowText(action));
			return true;
		}
	}

	/* Login */

	if(message_name == kMessageVerifyPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			ep_token_info info;
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);			
			
			CefString replyto = args->GetString(CefString("replyto"));
			CefString pin = args->GetString(CefString("pin"));
			
			bool r = VerifyPIN(pin.ToString().c_str());

			CefRefPtr<TokenContext> ctx = TokenContext::GetGlobalContext();
			
			if(ctx->Bind()) {
				
				/* Get all token information */
				ctx->GetInfo(&info);
				ctx->Destroy();

				if(r)

					browser->SendProcessMessage(PID_RENDERER, 
						CreateMsg(replyto, E_TOKEN_AUTHENTICATED, &info));				
			
				else 

					browser->SendProcessMessage(PID_RENDERER, 
						CreateMsg(replyto, E_TOKEN_AUTHENTICATION_REQUIRED, &info));				
			}

			/*else 
				 Some weird error happened */
			
			return true;
		}
	}

	/* Erase Token */

	if(message_name == kMessageErase) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefString replyto = argList->GetDictionary(0)->GetString(CefString("replyto"));			
			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(replyto, EraseToken()));				
			
		}
			
		return true;
	}	

	/* Init Token */

	if(message_name == kMessageInit) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString replyto = args->GetString(CefString("replyto"));			
			
			CefString pin = args->GetString(CefString("pin"));			
			CefString puk = args->GetString(CefString("puk"));
			CefString label = args->GetString(CefString("label"));

			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(replyto, InitToken(pin.ToString().c_str(), puk.ToString().c_str(), label.ToString().c_str())));				
			
		}
			
		return true;
	}		

	/* Set PIN Token */

	if(message_name == kMessageSetPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString replyto = args->GetString(CefString("replyto"));			
			
			CefString pin = args->GetString(CefString("pin"));			
			CefString puk = args->GetString(CefString("puk"));			

			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(replyto, InitPIN(pin.ToString().c_str(), puk.ToString().c_str())));				
			
		}
			
		return true;
	}

	/* Change PIN Token */

	if(message_name == kMessageChangePIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString replyto = args->GetString(CefString("replyto"));			
			
			CefString pin = args->GetString(CefString("pincode"));			
			CefString newpin = args->GetString(CefString("pin"));

			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(replyto, ChangePIN(pin.ToString().c_str(), newpin.ToString().c_str())));				
			
		}
			
		return true;
	}		

	/* Unblock Token */

	if(message_name == kMessageUnblockPIN) {
		
		CefRefPtr<CefListValue> argList = message->GetArgumentList();			
		if(argList->GetSize() > 0 && argList->GetType(0) == VTYPE_DICTIONARY) {		
			
			CefRefPtr<CefDictionaryValue> args = argList->GetDictionary(0);
			CefString replyto = args->GetString(CefString("replyto"));			
			
			CefString puk = args->GetString(CefString("puk"));			
			CefString pin = args->GetString(CefString("pin"));			
			
			browser->SendProcessMessage(PID_RENDERER, 
				CreateMsg(replyto, UnblockPIN(puk.ToString().c_str(), pin.ToString().c_str())));				
			
		}
			
		return true;
	}		

	return false;
}

std::wstring CreateWindowText(const CefString action) {
	std::wstringstream ss;
	ss << action.ToString().c_str() << " | Epsilon Token Manager";
	return ss.str();
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &kMessageName, bool ok) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMessageName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetBool("ok", ok);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &kMessageName, const char *status) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(kMessageName);

	CefRefPtr<CefDictionaryValue> args = CefDictionaryValue::Create();
	args->SetString("status", status);

	response->GetArgumentList()->SetDictionary(0, args);
	return response;
}

CefRefPtr<CefProcessMessage> CreateMsg(const std::string &key, const char *status, ep_token_info *info) {
	CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create(key);

	CefRefPtr<CefDictionaryValue> options = CefDictionaryValue::Create();

	options->SetString("status", status);

	options->SetInt("maxlen", info->maxlen);
	options->SetInt("minlen", info->minlen);
	
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
