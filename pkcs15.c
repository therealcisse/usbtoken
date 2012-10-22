
#include "libopensc/pkcs15.h"
#include "pkcs15init/pkcs15-init.h"
#include "pkcs15init/profile.h"
#include "util_opensc.h"

#include "inc/pkcs15.h"

/*

$ pkcs15-init -E
$ pkcs15-init -v -C -p pkcs15+onepin --pin PIN --puk PUK

*/

#define NELEMENTS(x)  (sizeof(x)/sizeof((x)[0]))

static const char *app_name = "epsilon-usbtoken";

/* Local functions */
static sc_pkcs15_object_t *get_pin_info(struct sc_pkcs15_card *);

static void ep_print_pin_info(struct sc_card *, const struct sc_pkcs15_object *);
static int  ep_list_pins(struct sc_pkcs15_card *);
static void ep_refresh_pin_info(struct sc_card *, struct sc_pkcs15_auth_info *);



static void print_common_flags(const struct sc_pkcs15_object *);


static const char *opt_profile = "pkcs15+onepin";
static int    verbose          = 2;
static int    opt_wait         = 0;

int ep_open_reader_and_card(struct sc_context **ctx, struct sc_card **card) {
  int r;
  sc_context_param_t ctx_param;
  const char *reader = NULL;

  memset(&ctx_param, 0, sizeof(ctx_param));
  ctx_param.ver      = 0;
  ctx_param.app_name = app_name;

  r = sc_context_create(ctx, &ctx_param);
  if (r) {
    util_error("Failed to establish context: %s\n", sc_strerror(r));
    return 0;
  }

  if (verbose > 1) {
    (*ctx)->debug = verbose;
    sc_ctx_log_to_file(*ctx, "log.txt");
  }

  return util_connect_card(*ctx, card, reader, opt_wait, verbose) ? 0 : 1;
}

/*
 * Erase card
 */
int ep_do_erase(const char *opt_profile) {
  struct sc_context *ctx = NULL;
  struct sc_card *card;
  struct sc_pkcs15_card *p15card;
  struct sc_profile *profile = NULL;  
  int r;

  /* Connect to the card */
  if (!ep_open_reader_and_card(&ctx, &card))
    return -1;  

  /* Bind the card-specific operations and load the profile */
  if ((r = sc_pkcs15init_bind(card, opt_profile,
    NULL, &profile)) < 0) {
    util_debug("Couldn't bind to the card: %s\n", sc_strerror(r));
    return r;
  }

  p15card = sc_pkcs15_card_new();
  p15card->card = card; 

  r = sc_pkcs15init_erase_card(p15card, profile, NULL);

  sc_pkcs15_card_free(p15card);
  
  if (profile)
    sc_pkcs15init_unbind(profile);

  if (card) {
    sc_unlock(card);
    sc_disconnect_card(card);
  } 

  sc_release_context(ctx);

  return r;
}

/*
 * Initialize pkcs15 application
 */
int ep_do_init_app(const char *opt_profile, char *opt_pin, char *opt_puk, char *opt_label, char *opt_serial) {
  struct sc_context *ctx = NULL;
  struct sc_card *card;
  struct sc_profile *profile = NULL;  
  struct sc_pkcs15init_initargs args;
  sc_pkcs15_auth_info_t info;
  int so_puk_disabled = 0;
  int r;

  /* Connect to the card */
  if (!ep_open_reader_and_card(&ctx, &card))
    return -1;  

  /* Bind the card-specific operations and load the profile */
  if ((r = sc_pkcs15init_bind(card, opt_profile,
    NULL, &profile)) < 0) {
    util_debug("Couldn't bind to the card: %s\n", sc_strerror(r));
    return r;
  } 

  memset(&args, 0, sizeof(args));

  sc_pkcs15init_get_pin_info(profile, SC_PKCS15INIT_SO_PIN, &info);

  if ((info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_UNBLOCK_DISABLED) 
      && (info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN))
    so_puk_disabled = 1;

  args.so_pin = (const u8 *) opt_pin;
  if (args.so_pin)
    args.so_pin_len = strlen((const char *) args.so_pin);

  if (!so_puk_disabled)   {
    args.so_puk = (const u8 *) opt_puk;
    if (args.so_puk)
      args.so_puk_len = strlen((const char *) args.so_puk);
  }

  args.serial = (const char *)opt_serial;
  args.label = opt_label;

  r = sc_pkcs15init_add_app(card, profile, &args);

  if (profile)
    sc_pkcs15init_unbind(profile);

  if (card) {
    sc_unlock(card);
    sc_disconnect_card(card);
  }   

  sc_release_context(ctx);

  return r; 
}

/*
 * Store a PIN/PUK pair
 */
int ep_do_store_pin(const char *opt_profile, char *opt_pin, char *opt_puk, char *opt_label) {
  struct sc_context *ctx = NULL;
  struct sc_card *card;
  struct sc_profile *profile = NULL;
  struct sc_pkcs15init_pinargs args;
  struct sc_pkcs15_card *p15card;
  int     r;

  /* Connect to the card */
  if (!ep_open_reader_and_card(&ctx, &card))
    return -1;    

  /* Bind the card-specific operations and load the profile */
  if ((r = sc_pkcs15init_bind(card, opt_profile,
    NULL, &profile)) < 0) {
    util_debug("Couldn't bind to the card: %s\n", sc_strerror(r));
    return r;
  } 

  memset(&args, 0, sizeof(args));
  //sc_pkcs15_format_id(pin_id, &args.auth_id);
  args.pin = (u8 *) opt_pin;
  args.pin_len = strlen(opt_pin);
  args.label = opt_label;

  args.puk = (u8 *) opt_puk;
  args.puk_len = opt_puk ? strlen(opt_puk) : 0;

  if (r = ep_pkcs15_bind_card(card, &p15card, profile)) {
    util_debug("PKCS#15 binding failed: %s\n", sc_strerror(r));
    goto out;
  }

  r = sc_pkcs15init_store_pin(p15card, profile, &args);

out: 

  if (p15card)
    sc_pkcs15_unbind(p15card);

  if (profile)
    sc_pkcs15init_unbind(profile);

  if (card) {
    sc_unlock(card);
    sc_disconnect_card(card);
  }   

  sc_release_context(ctx);

  return r;
}

int ep_unblock_pin(struct sc_pkcs15_card *p15card, u8 *puk, u8 *pin) {
  struct sc_pkcs15_auth_info *pinfo = NULL;
  sc_pkcs15_object_t *pin_obj;
  int r;
  
  if (!(pin_obj = get_pin_info(p15card)))
    return 2;
  pinfo = (sc_pkcs15_auth_info_t *) pin_obj->data;
  ep_refresh_pin_info(p15card->card, pinfo);

  if (pinfo->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN) 
    return 1;

  r = sc_pkcs15_unblock_pin(p15card, pin_obj, 
      puk, puk ? strlen((char *) puk) : 0,
      pin, pin ? strlen((char *) pin) : 0);

  if (r == SC_ERROR_PIN_CODE_INCORRECT) {
    util_debug("PUK code incorrect; tries left: %d\n", pinfo->tries_left);
    return 3;
  } else if (r) {
    util_debug("PIN unblocking failed: %s\n", sc_strerror(r));
    return 2;
  }

  if (verbose)
    util_debug("PIN successfully unblocked.\n");

  return SC_SUCCESS;
}

int ep_change_pin(struct sc_pkcs15_card *p15card, u8 *pincode, u8 *newpin) {
  sc_pkcs15_object_t *pin_obj;
  sc_pkcs15_auth_info_t *pinfo = NULL;
  int r;

  if (!(pin_obj = get_pin_info(p15card)))
    return 2;

  pinfo = (sc_pkcs15_auth_info_t *) pin_obj->data;
  ep_refresh_pin_info(p15card->card, pinfo);

  if (pinfo->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN) 
    return 1;

  if (pinfo->tries_left != -1) {
    if (pinfo->tries_left != pinfo->max_tries) {
      if (pinfo->tries_left == 0) {
        util_debug("PIN code blocked!\n");
        return 2;
      } else {
        util_debug("%d PIN tries left.\n", pinfo->tries_left);
      }
    }
  }

  r = sc_pkcs15_change_pin(p15card, pin_obj, 
      pincode, pincode ? strlen((char *) pincode) : 0,
      newpin, newpin ? strlen((char *) newpin) : 0);

  if (r == SC_ERROR_PIN_CODE_INCORRECT) {
    util_debug("PIN code incorrect; tries left: %d\n", pinfo->tries_left);
    return 3;
  } else if (r) {
    util_debug("PIN code change failed: %s\n", sc_strerror(r));
    return 2;
  }

  if (verbose)
    util_debug("PIN code changed successfully.\n");

  return 0;
}

int ep_verify_pin(struct sc_pkcs15_card *p15card, u8 *pin) {
  struct sc_pkcs15_object *pin_obj;
  int r;
  
  if ((pin_obj = get_pin_info(p15card)) == NULL) 
    return -1;

  if ((r = sc_pkcs15_verify_pin(p15card, pin_obj, pin, pin? strlen((char *) pin) : 0)) < 0)
    util_debug("verify_pin failed: %s\n", sc_strerror(r));

  return r;
}

int ep_auth_obj(struct sc_pkcs15_card *p15card, sc_pkcs15_object_t *obj, u8 *pin) {
  sc_pkcs15_object_t  *pin_obj;
  int     r;

  if (obj->auth_id.len == 0)
    return 0;

  if (r = sc_pkcs15_find_pin_by_auth_id(p15card, &obj->auth_id, &pin_obj))
    return r;

  return sc_pkcs15_verify_pin(p15card, pin_obj, pin, pin? strlen((char *) pin) : 0);
}

static sc_pkcs15_object_t *
get_pin_info(struct sc_pkcs15_card *p15card) {
  struct sc_pkcs15_object *objs[32];
  int ii, r;
  
  r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_AUTH_PIN, objs, 32);
  if (r < 0) {
    util_debug("PIN code enumeration failed: %s\n", sc_strerror(r));
    return NULL;
  }

  if (r == 0) {
    util_debug("No PIN codes found.\n");
    return NULL;
  }  

  for (ii = 0; ii < r; ii++) {
    struct sc_pkcs15_auth_info *pin_info = (struct sc_pkcs15_auth_info *) objs[ii]->data;
    
    if (pin_info->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN) continue;        
    if (pin_info->attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN) continue; /* #TODO: Is this right? */
    if (pin_info->attrs.pin.flags & SC_PKCS15_PIN_FLAG_UNBLOCKING_PIN) continue;
    
    return objs[ii];
  }

  return NULL;
}

int ep_pkcs15_bind_card(struct sc_card *card, struct sc_pkcs15_card **p15card, struct sc_profile *profile) {
  int r;

  if (r = sc_pkcs15_bind(card, NULL, p15card)) {
    util_debug("PKCS#15 binding failed: %s\n", sc_strerror(r));
    return r;
  }

  /* XXX: should compare card to profile here to make
   * sure we're not messing things up */
  sc_pkcs15init_set_p15card(profile, *p15card);   
  
  return SC_SUCCESS;
}

static void 
print_common_flags(const struct sc_pkcs15_object *obj) {
  const char *common_flags[] = {"private", "modifiable"};
  unsigned int i;
  util_debug("\tObject Flags   : [0x%X]", obj->flags);
  for (i = 0; i < NELEMENTS(common_flags); i++) {
    if (obj->flags & (1 << i)) {
      util_debug(", %s", common_flags[i]);
    }
  }
  util_debug("\n");
}

static void 
ep_print_pin_info(struct sc_card *card, const struct sc_pkcs15_object *obj) {  
  int i;
  const char *pin_flags[] = {
    "case-sensitive", "local", "change-disabled",
    "unblock-disabled", "initialized", "needs-padding",
    "unblockingPin", "soPin", "disable_allowed",
    "integrity-protected", "confidentiality-protected",
    "exchangeRefData"
  };
  const char *pin_types[] = {"bcd", "ascii-numeric", "UTF-8",
    "halfnibble bcd", "iso 9664-1"}; 
  struct sc_pkcs15_auth_info *pin = (struct sc_pkcs15_auth_info *) obj->data;

  if (pin->auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN)
    util_debug("pin.auth_type != SC_PKCS15_PIN_AUTH_TYPE_PIN\n");

  ep_refresh_pin_info(card, pin);

  util_debug("PIN [%s]\n", obj->label);
  print_common_flags(obj);  
  
  if (obj->auth_id.len)
    util_debug("\tAuth ID        : %s\n", sc_pkcs15_print_id(&obj->auth_id));
  
  util_debug("\tID             : %s\n", sc_pkcs15_print_id(&pin->auth_id));
  
  if (pin->auth_type == SC_PKCS15_PIN_AUTH_TYPE_PIN) {
    util_debug("\tFlags          : [0x%02X]", pin->attrs.pin.flags);
    
    for (i = 0; i < NELEMENTS(pin_flags); i++)
      if (pin->attrs.pin.flags & (1 << i))
        util_debug(", %s", pin_flags[i]);

    util_debug("\n");
    util_debug("\tLength         : min_len:%lu, max_len:%lu, stored_len:%lu\n",
      (unsigned long)pin->attrs.pin.min_length, (unsigned long)pin->attrs.pin.max_length,
      (unsigned long)pin->attrs.pin.stored_length);    

    util_debug("\tTries          : max_tries:%d, tries_left:%d\n",
      (unsigned long)pin->max_tries, (unsigned long)pin->tries_left);

    util_debug("\tPad char       : 0x%02X\n", pin->attrs.pin.pad_char);
    util_debug("\tReference      : %d\n", pin->attrs.pin.reference);
    
    if (pin->attrs.pin.type < NELEMENTS(pin_types))
      util_debug("\tType           : %s\n", pin_types[pin->attrs.pin.type]);
    else
      util_debug("\tType           : [encoding %d]\n", pin->attrs.pin.type);
  }

  if (pin->path.len || pin->path.aid.len)
    util_debug("\tPath           : %s\n", sc_print_path(&pin->path));
}

static int 
ep_list_pins(struct sc_pkcs15_card *p15card) {
  int r, i;
  struct sc_pkcs15_object *objs[32];
  
  r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_AUTH_PIN, objs, 32);
  if (r < 0) {
    util_debug("PIN enumeration failed: %s\n", sc_strerror(r));
    return 1;
  }

  if (verbose)
    util_debug("Card has %d PIN code(s).\n\n", r);
  
  for (i = 0; i < r; i++) {
    ep_print_pin_info(p15card->card, objs[i]);
    util_debug("\n");
  }

  return 0;
}

static void
ep_refresh_pin_info(struct sc_card *card, struct sc_pkcs15_auth_info *pin) {
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

static int ep_has_onepin(struct sc_profile *profile) {
  sc_pkcs15_auth_info_t sopin_info;
  sc_pkcs15init_get_pin_info(profile, SC_PKCS15INIT_SO_PIN, &sopin_info);
  return sopin_info.attrs.pin.flags & SC_PKCS15_PIN_FLAG_SO_PIN ? 0 : 1;          
}

int ep_get_token_info(struct ep_token_info *info) {
  struct sc_context *ctx = NULL;
  struct sc_card *card;
  struct sc_pkcs15_card *p15card;
  struct sc_profile *profile;  
  sc_pkcs15_object_t *pin_obj;
  sc_pkcs15_auth_info_t *pinfo = NULL;
  int r;

  memset(info, 0, sizeof(*info));

  /* Connect to the card */
  if (!ep_open_reader_and_card(&ctx, &card)){
	  info->token_absent = 1;
    return -1;  
  }

  /* Bind the card-specific operations and load the profile */
  if ((r = sc_pkcs15init_bind(card, opt_profile,
    NULL, &profile)) < 0) {
    util_debug("Couldn't bind to the card: %s\n", sc_strerror(r));
    return r;
  }

  info->onepin = ep_has_onepin(profile);

  if (r = ep_pkcs15_bind_card(card, &p15card, profile)) {
    util_debug("PKCS#15 binding failed: %s\n", sc_strerror(r));
    info->token_initialized = 0;
    goto out2;
  }  

  if ((pin_obj = get_pin_info(p15card)) == NULL) 
    goto out;

  pinfo = (sc_pkcs15_auth_info_t *) pin_obj->data;
  ep_refresh_pin_info(p15card->card, pinfo);

  info->token_initialized = 1;

  info->inuse = profile->card->reader->flags & SC_READER_CARD_INUSE ? 1 : 0; 
  
  info->pin_initialized = pinfo->attrs.pin.flags & SC_PKCS15_PIN_FLAG_INITIALIZED ? 1 : 0;  

  info->minlen = pinfo->attrs.pin.min_length;
  info->maxlen = pinfo->attrs.pin.max_length;
  info->max_tries = pinfo->max_tries;
  info->tries_left = pinfo->tries_left;  

out:    

	if (p15card)
	  sc_pkcs15_unbind(p15card);

out2:

    if (profile)
      sc_pkcs15init_unbind(profile);

    if (card) {
      sc_unlock(card);
      sc_disconnect_card(card);
    }   

	if(ctx)
		sc_release_context(ctx); 

    return r < 0 ? 1 : SC_SUCCESS;  
}
