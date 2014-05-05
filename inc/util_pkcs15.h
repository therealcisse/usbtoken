

#ifndef UTIL_PKCS_15_H
#define UTIL_PKCS_15_H 1

#include "config.h"

#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
#include <openssl/conf.h>
#endif
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/bn.h>
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#include <openssl/opensslconf.h> /* for OPENSSL_NO_EC */
#ifndef OPENSSL_NO_EC
#include <openssl/ec.h>
#endif /* OPENSSL_NO_EC */
#endif /* OPENSSL_VERSION_NUMBER >= 0x10000000L */

//#include "inc/token.h"

#include "epToken/client_handler.h"

#define	MAX_CERTS	4

#define FORMAT_UNDEF    0
#define FORMAT_ASN1     1
#define FORMAT_TEXT     2
#define FORMAT_PEM      3
#define FORMAT_NETSCAPE 4
#define FORMAT_PKCS12   5
#define FORMAT_SMIME    6
#define FORMAT_ENGINE   7
#define FORMAT_IISSGC	8	/* XXX this stupid macro helps us to avoid
				 * adding yet another param to load_*key() */
#define FORMAT_PEMRSA	9	/* PEM RSAPubicKey format */
#define FORMAT_ASN1RSA	10	/* DER RSAPubicKey format */
#define FORMAT_MSBLOB	11	/* MS Key blob format */
#define FORMAT_PVK	12	/* MS PVK file format */

#define REQUEST_EXTENSIONS

namespace util {

EVP_PKEY *ep_load_pubkey(void *data, size_t len, int format);

#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
static EVP_PKEY *
load_netscape_key(BIO *key, int format);
#endif /* ndef OPENSSL_NO_RC4 */

static int make_REQ(X509_REQ *req, EVP_PKEY *pkey, char *subj, int multirdn = 1,
			unsigned long chtype = MBSTRING_ASC);

static int build_subject(X509_REQ *req, char *subject, unsigned long chtype, int multirdn);

static X509_NAME *parse_name(char *subject, long chtype, int multirdn);

int ep_pkcs15_bind_card(struct sc_card *, struct sc_pkcs15_card **, struct sc_profile *);

void ep_refresh_pin_info(struct sc_card *, struct sc_pkcs15_auth_info *);

int ep_has_onepin(sc_profile *);

sc_pkcs15_object *get_pin_info(struct sc_pkcs15_card *);

int ep_open_reader_and_card(struct sc_context **, const char *, struct sc_card **);

int ep_authenticate_object(struct sc_pkcs15_card *p15card, sc_pkcs15_object_t *obj, const u8 *pin);

int ep_verify_pin(struct sc_pkcs15_card *p15card, const char *pin);

/*  Crypto tools */

int ep_delete_prkey(CefRefPtr<epsilon::TokenContext> ctx, const char *id, const char *authData);

int ep_delete_x509_certificate(CefRefPtr<epsilon::TokenContext> ctx, const char *id, const char *authData);

static int 
pass_cb(char *buf, int len, int flags, void *d);

/* Key generation */

int ep_gen_x509_req(CefRefPtr<epsilon::TokenContext>, CefRefPtr<ClientHandler> handler, const char *, const unsigned char *, 
	const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *, const unsigned char *, const char *);

int ep_export_x509_certificate(CefRefPtr<epsilon::TokenContext> ctx, CefRefPtr<ClientHandler> handler, const char *path, const char *id, const char *authData, unsigned int format);

int ep_generate_key(CefRefPtr<epsilon::TokenContext> ctx, 
  const char *spec, const char *pubkey_label, char **id, char *authid, const char *authData, int x509_usage);

static int 
init_keyargs(struct sc_pkcs15init_prkeyargs *args, 
		char *id = NULL, char *authid = NULL, const char *label = NULL, int x509_usage = 0);

/* Private keys */

static int 
do_read_pem_private_key(const char *data, size_t len, const char *passphrase, 
			EVP_PKEY **key);

static int
do_read_pkcs12_private_key(const char *data, size_t len, const char *passphrase,
			EVP_PKEY **key, X509 **certs, unsigned int max_certs);

int ep_read_private_key(const char *data, size_t len, const char *format,
			EVP_PKEY **pk, X509 **certs, unsigned int max_certs, char *passphrase);

/* Public keys */

static EVP_PKEY *
do_read_pem_public_key(const char *data, size_t len);

static EVP_PKEY *
do_read_der_public_key(const char *data, size_t len);

int ep_read_public_key(const char *data, size_t len, const char *format, EVP_PKEY **out);

/* Certificates */

static X509 *
do_read_pem_certificate(const char *data, size_t len);

static X509 *
do_read_der_certificate(const char *data, size_t len);

int ep_read_certificate(const char *data, size_t len, const char *format, X509 **out);

/* Token read keys */

int ep_token_read_certificate(CefRefPtr<epsilon::TokenContext> ctx, struct ep_key_info *key, const char *cert_id);

int ep_token_read_pubkey(CefRefPtr<epsilon::TokenContext> ctx, struct ep_key_info *key, const char *key_id, u8 *pin);

int ep_token_read_prkey(CefRefPtr<epsilon::TokenContext> ctx, struct ep_key_info *key, const char *key_id);

int ep_token_read_all_pubkeys(CefRefPtr<epsilon::TokenContext> ctx, struct ::ep_key_info **keys, size_t ret_size);

int ep_token_read_all_prkeys(CefRefPtr<epsilon::TokenContext> ctx, struct ::ep_key_info **keys, size_t ret_size);

int ep_token_read_all_certs(CefRefPtr<epsilon::TokenContext> ctx, struct ::ep_key_info **keys, size_t ret_size);

/* FS read keys */

static int
is_cacert_already_present(struct sc_pkcs15_card *p15card, struct sc_pkcs15init_certargs *args);

static int
do_read_check_certificate(sc_pkcs15_cert_t *sc_oldcert,
	void *data, int len, const char *format, sc_pkcs15_der_t *newcert_raw);

/* Delete object(s) by ID. The 'which' param can be any combination of
 * SC_PKCS15INIT_TYPE_PRKEY, SC_PKCS15INIT_TYPE_PUBKEY, SC_PKCS15INIT_TYPE_CERT
 * and SC_PKCS15INIT_TYPE_CHAIN. In the last case, every cert in the chain is
 * deleted, starting with the cert with ID 'id' and untill a CA cert is
 * reached that certified other remaining certs on the card.
 */
int ep_delete_crypto_objects(sc_pkcs15_card *p15card,
				struct sc_profile *profile,
				const sc_pkcs15_id id,
				unsigned int which);

/*
 * Store a private key
 */
int ep_store_private_key(CefRefPtr<epsilon::TokenContext> ctx, const char *data, size_t len, 
	const char *label, const char *format, char *passphrase, const char *authData);

static int
do_store_public_key(struct sc_pkcs15_card *p15card, struct sc_profile *profile, 
	const char *data, size_t len, const char *format, char *id, const char *label, EVP_PKEY *pkey);

/*
 * Store a public key
 */
int ep_store_public_key(CefRefPtr<epsilon::TokenContext> ctx, 
	const char *data, size_t len, const char *format, const char *label, const char *authData);

/*
 * Download certificate to card
 */
int ep_store_certificate(CefRefPtr<epsilon::TokenContext> ctx,
	const char *data, size_t len, const char *label, const char *format, const char *authData, unsigned int authority = 0);

static int 
ep_gen_key_id(struct sc_pkcs15_card *p15card, char *id, unsigned int type);

static int
do_convert_bignum(sc_pkcs15_bignum_t *dst, const BIGNUM *src);

static int 
do_convert_private_key(struct sc_pkcs15_prkey *key, EVP_PKEY *pk);

static int 
do_convert_public_key(struct sc_pkcs15_pubkey *key, EVP_PKEY *pk);

static int 
do_convert_cert(sc_pkcs15_der_t *der, X509 *cert);

static void
init_gost_params(struct sc_pkcs15init_keyarg_gost_params *params, EVP_PKEY *pkey);

}

#endif