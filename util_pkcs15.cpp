
#include <string>

#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"

#include "inc/util_pkcs15.h"

namespace util {

int ep_pkcs15_bind_card(struct sc_card *card, struct sc_pkcs15_card **p15card, struct sc_profile *profile) {
  int r;

  if (r = sc_pkcs15_bind(card, NULL, p15card))
    return r;

  /* XXX: should compare card to profile here to make
   * sure we're not messing things up */
  sc_pkcs15init_set_p15card(profile, *p15card);   
  
  return SC_SUCCESS;
}


void ep_refresh_pin_info(struct sc_card *card, struct sc_pkcs15_auth_info *pin) {
  struct sc_pin_cmd_data data;

  /* Try to update PIN info from card */
  memset(&data, 0, sizeof(data));
  data.cmd = SC_PIN_CMD_GET_INFO;
  data.pin_type = SC_AC_CHV;
  data.pin_reference = pin->attrs.pin.reference;

  if (sc_pin_cmd(card, &data, NULL) == SC_SUCCESS) {
    if (data.pin1.max_tries > 0)
      pin->max_tries = data.pin1.max_tries;
    /* tries_left must be supported or sc_pin_cmd should not return SC_SUCCESS */
    pin->tries_left = data.pin1.tries_left;
  }
}

int ep_has_onepin(sc_profile *profile) {
  sc_pkcs15_auth_info_t sopin_info;
  sc_pkcs15init_get_pin_info(profile, SC_PKCS15INIT_SO_PIN, &sopin_info);
  return  sopin_info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN ? 0 : 1;          
}

sc_pkcs15_object *get_pin_info(struct sc_pkcs15_card *p15card) {
  struct sc_pkcs15_object *objs[32];
  int ii, r;
  
  r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_AUTH_PIN, objs, 32);
  if (r < 0) {
    //util_debug("PIN code enumeration failed: %s\n", sc_strerror(r));
    return NULL;
  }

  if (r == 0) {
    //util_debug("No PIN codes found.\n");
    return NULL;
  }  

  for (ii = 0; ii < r; ii++) {
    struct sc_pkcs15_auth_info *pin_info = (struct sc_pkcs15_auth_info *) objs[ii]->data;
    
    //if (pin_info->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN) continue;        
    //if (pin_info->attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN) continue; /* #TODO: Is this right? */
    //if (pin_info->attrs.pin.flags & SC_PKCS15_PIN_FLAG_UNBLOCKING_PIN) continue;
    
    return objs[ii];
  }

  return NULL;
}

}
