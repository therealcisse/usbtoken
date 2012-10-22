
#include <string>

#include "inc/ep_pkcs15.h"

#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"

#include "inc/util_pkcs15.h"

namespace epsilon {

	sc_pkcs15_card *CreateP15Card(const TokenContext *const ctx) {
	  sc_pkcs15_card *p15card = sc_pkcs15_card_new();
	  p15card->card = ctx->GetCard(); 
	  return p15card;
	}

	void DestroyP15Card(sc_pkcs15_card *p15card) {
		sc_pkcs15_card_free(p15card);
	}

	sc_pkcs15_card *BindP15Card(const TokenContext *const ctx) {
		sc_pkcs15_card *p15card;
		return ::util::ep_pkcs15_bind_card(ctx->GetCard(), &p15card, ctx->GetProfile()) == SC_SUCCESS ? p15card : NULL;
	}

	void ReleaseP15Card(sc_pkcs15_card *p15card) {
		sc_pkcs15_unbind(p15card);
	}	

	void TokenContext::GetInfo(ep_token_info *info) const {
	  sc_pkcs15_card *p15card;
	  sc_pkcs15_object *pin_obj;
	  sc_pkcs15_auth_info *pinfo = NULL;

	  memset(info, 0, sizeof(*info));
	  info->onepin = ::util::ep_has_onepin(profile);

	  info->minlen = GetProfile()->pin_minlen;
	  info->maxlen = GetProfile()->pin_maxlen;

	  if ((p15card = BindP15Card(this)) == NULL) {
		info->token_initialized = 0;
		return;
	  }  

	  if ((pin_obj = ::util::get_pin_info(p15card)) == NULL)
			goto out;

	  pinfo = (sc_pkcs15_auth_info_t *) pin_obj->data;
	  ::util::ep_refresh_pin_info(p15card->card, pinfo);

	  info->token_initialized = 1;
	  info->inuse = profile->card->reader->flags & SC_READER_CARD_INUSE ? 1 : 0; 
	  info->pin_initialized = pinfo->attrs.pin.flags & SC_PKCS15_PIN_FLAG_INITIALIZED ? 1 : 0;  
	  info->minlen = pinfo->attrs.pin.min_length;
	  info->maxlen = pinfo->attrs.pin.max_length;
	  info->max_tries = pinfo->max_tries;
	  info->tries_left = pinfo->tries_left;

	  out: ReleaseP15Card(p15card);
	}

	bool TokenContext::Bind() {

	  /* Connect to the card */
	  if (!ep_open_reader_and_card(&ctx, &card))
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
		if(!in_context) return;

	  if (profile)
	    sc_pkcs15init_unbind(profile);

	  if (card) {
	    sc_unlock(card);
	    sc_disconnect_card(card);
	  }   

	  if(ctx)
	  	sc_release_context(ctx);

	  in_context = false;
	}

	sc_profile *TokenContext::GetProfile() const {
		return profile;
	}

	std::string TokenContext::GetProfileName() const {
		return kProfileName;
	}

	sc_card *TokenContext::GetCard() const {
		return card;
	}

	bool TokenContext::InContext() const {
		return in_context;
	}

}