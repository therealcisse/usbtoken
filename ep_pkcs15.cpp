
#include <string>

#include "inc/ep_pkcs15.h"
#include "inc/token.h"

#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"

#include "inc/util_pkcs15.h"

#include <windows.h>

namespace epsilon {

	CefRefPtr<TokenContext> TokenContext::instance_ = NULL;

	sc_pkcs15_card *CreateP15Card(CefRefPtr<TokenContext> ctx) {
	  sc_pkcs15_card *p15card = sc_pkcs15_card_new();
	  p15card->card = ctx->GetCard(); 
	  return p15card;
	}

	void DestroyP15Card(sc_pkcs15_card *p15card) {
		sc_pkcs15_card_free(p15card);
	}

	sc_pkcs15_card *BindP15Card(CefRefPtr<TokenContext> ctx) {
		sc_pkcs15_card *p15card;
		
		if(::util::ep_pkcs15_bind_card(ctx->GetCard(), &p15card, ctx->GetProfile()) == SC_SUCCESS) {
			sc_pkcs15init_set_p15card(ctx->GetProfile(), p15card);
			return p15card;
		}
		
		return  NULL;
	}

	void ReleaseP15Card(sc_pkcs15_card *p15card) {
		sc_pkcs15_unbind(p15card);
	}	

	void GetTokenInfo(CefRefPtr<TokenContext> ctx, ep_token_info *info) {
	  sc_pkcs15_card *p15card;
	  sc_pkcs15_object *pin_obj;
	  sc_pkcs15_auth_info *pinfo = NULL;

	  /*for(unsigned int i = 0, len = sc_ctx_get_reader_count(ctx->GetSCContext()); i < len; i++) {
		  sc_reader *reader = sc_ctx_get_reader_by_id(ctx->GetSCContext(), 0);
	  }*/

	  memset(info, 0, sizeof(*info));
	  memset(info->manufacturerID, 0, sizeof(info->manufacturerID));
	  memset(info->label, 0, sizeof(info->label));
	  memset(info->reader, 0, sizeof(info->reader));

	  strncpy(info->reader, ctx->GetCard()->reader->name, sizeof(info->reader));

	  info->onepin = ::util::ep_has_onepin(ctx->GetProfile());

	  info->minlen = ctx->GetProfile()->pin_minlen;
	  info->maxlen = ctx->GetProfile()->pin_maxlen;

	  if ((p15card = BindP15Card(ctx)) == NULL) {
		info->token_initialized = 0;
		return;
	  }  

	  strncpy(info->label, p15card->tokeninfo->label, sizeof(info->label));
	  info->readonly = (p15card->tokeninfo->flags >> 0) & 1; /* see pkcs15-tool `dump` for details */

	  info->login_required = (p15card->tokeninfo->flags >> 1) & 1 ;
	  strncpy(info->manufacturerID, p15card->tokeninfo->manufacturer_id, sizeof(info->manufacturerID));

	  if ((pin_obj = ::util::get_pin_info(p15card)) == NULL)
			goto out;

	  pinfo = (sc_pkcs15_auth_info_t *) pin_obj->data;
	  ::util::ep_refresh_pin_info(p15card->card, pinfo);

	  info->token_initialized = 1;
	  info->inuse = ctx->GetProfile()->card->reader->flags & SC_READER_CARD_INUSE ? 1 : 0; 
	  info->pin_initialized = pinfo->attrs.pin.flags & SC_PKCS15_PIN_FLAG_INITIALIZED ? 1 : 0;  
	  info->minlen = pinfo->attrs.pin.min_length;
	  info->maxlen = pinfo->attrs.pin.max_length;
	  info->max_tries = pinfo->max_tries;
	  info->tries_left = pinfo->tries_left;

	  out: ReleaseP15Card(p15card);
	}

	bool TokenContext::isTokenPresent(sc_context **ctx) {
		sc_context_param_t ctx_param;
  
		memset(&ctx_param, 0, sizeof(ctx_param));
		ctx_param.ver      = 0;
		ctx_param.app_name = E_TOKEN_APPLICATION_NAME;
		ctx_param.thread_ctx = &sc_thread_ctx;

		if (sc_context_create(ctx, &ctx_param))
			return false;

		if (sc_ctx_get_reader_count(*ctx) > 0)

			/* Automatically try to skip to a reader with a card if reader not specified */
			for (unsigned int i = 0; i < sc_ctx_get_reader_count(*ctx); i++) {
				sc_reader *reader = sc_ctx_get_reader(*ctx, i);
				if (sc_detect_card_presence(reader) & SC_READER_CARD_PRESENT)
					return true;
			}

		sc_release_context(*ctx);
		*ctx = NULL;

		return false;
	}

	bool TokenContext::Bind(const char *reader) {
		if(in_context)
			return true;
				
	  /* Connect to the card */
		if (!::util::ep_open_reader_and_card(&ctx, reader, &card)) 
	    return false;  	
	    
	  /* Bind the card-specific operations and load the profile */
	  if (sc_pkcs15init_bind(card, kProfileName.c_str(),
	    NULL, &profile) < 0) {

		 if (card) {
			sc_unlock(card);
			sc_disconnect_card(card);
		  }   

		  if(ctx)
	  		sc_release_context(ctx);

		  return false;
	  }
	  
	  in_context = true;
	  return true;
	}

	void TokenContext::Destroy() {
		if(!InContext()) return;

	  if (profile) {
		if (profile->dirty != 0 /*&& profile->p15_data != NULL */&& profile->pkcs15.do_last_update) {
			if((/*profile->p15_data = */BindP15Card(this)) != NULL) {
				sc_pkcs15init_unbind(profile);
				//if(profile->p15_data) ReleaseP15Card(profile->p15_data);
			}
		} else {
			sc_pkcs15init_unbind(profile);
		}
	  }

	  if (card) {
	    sc_unlock(card);
	    sc_disconnect_card(card);
	  }   

	  if(ctx)
	  	sc_release_context(ctx);

	  /* NULLify variables */

	  profile = NULL;
	  card = NULL;
	  ctx =  NULL;

	  in_context = false;
	}

	sc_profile *TokenContext::GetProfile() const {
		return profile;
	}

	std::string TokenContext::GetProfileName() {
		return kProfileName;
	}

	sc_context *TokenContext::GetSCContext() const {
		return ctx;		
	}

	sc_card *TokenContext::GetCard() const {
		return card;
	}

	bool TokenContext::InContext() const {
		return in_context;
	}
}