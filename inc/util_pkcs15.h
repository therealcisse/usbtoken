

#ifndef UTIL_PKCS_15_H
#define UTIL_PKCS_15_H 1

#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"

namespace util {

int ep_pkcs15_bind_card(struct sc_card *card, struct sc_pkcs15_card **p15card, struct sc_profile *profile);

void ep_refresh_pin_info(struct sc_card *card, struct sc_pkcs15_auth_info *pin);

int ep_has_onepin(sc_profile *profile);

sc_pkcs15_object *get_pin_info(struct sc_pkcs15_card *p15card);

}

#endif