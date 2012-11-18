

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
	char label[32];
	char manufacturerID[32];

	char reader[32];

	int inuse;

	int onepin;

	int readonly;

	int login_required;

	int token_initialized;
	int pin_initialized;

	int token_absent;
		
	int maxlen;
	int minlen;

	int max_tries;
	int tries_left;
};

#define EP_KEY_TYPE_CERT	0x0001
#define EP_KEY_TYPE_PUBKEY	0x0002
#define EP_KEY_TYPE_PRKEY	0x0004

struct ep_cert_info{
	u8 *data;
	size_t len;

	int version;

	u8* serial_number;
	size_t serial_number_len;

	int issuer_set;

	/*char issuer_name[1024];*/
	u8 issuer_common_name[64];
	u8 issuer_locality_name[128];
	u8 issuer_state_name[128];
	u8 issuer_organization_name[64];
	u8 issuer_organization_unit_name[64];
	u8 issuer_title_name[64];	
	u8 issuer_email_address[128];

	u8 subject_common_name[64];
	u8 subject_locality_name[128];
	u8 subject_state_name[128];
	u8 subject_organization_name[64];
	u8 subject_organization_unit_name[64];
	u8 subject_title_name[64];	
	u8 subject_email_address[128];

	u8 *notBefore;
	u8 *notAfter;
};

struct ep_pubkey_info{ 
	u8 *data;
	size_t len;
};

struct ep_key_info{
	int type;

	unsigned char id[255];
	char label[64];
	unsigned char authid[255];

	int native;

	union {
		struct ep_cert_info cert;
		struct ep_pubkey_info pubkey;
	} key;
};

#ifdef __cplusplus
}
#endif

#endif 