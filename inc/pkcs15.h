

#ifndef EP_PKCS_15_H
#define EP_PKCS_15_H 1

#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"
#include "util_opensc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ep_token_info{
	int inuse;

	int onepin;

	int readonly;

	int token_initialized;
	int pin_initialized;

	int token_absent;
		
	int maxlen;
	int minlen;

	int max_tries;
	int tries_left;
};

int ep_do_erase(const char *);
int ep_do_init_app(const char *, char *, char *, char *, char *);
int ep_do_store_pin(const char *, char *, char *, char *);
int ep_change_pin(struct sc_pkcs15_card *p15card, u8 *, u8 *);
int ep_unblock_pin(struct sc_pkcs15_card *p15card, u8 *, u8 *);
int ep_auth_obj(struct sc_pkcs15_card *, sc_pkcs15_object_t *, u8 *);
int ep_verify_pin(struct sc_pkcs15_card *, u8 *);

int ep_open_reader_and_card(struct sc_context **ctx, struct sc_card **);
int ep_pkcs15_bind_card(struct sc_card *, struct sc_pkcs15_card **, struct sc_profile *);
int ep_get_token_info(struct ep_token_info*);

#ifdef __cplusplus
}
#endif

#endif 