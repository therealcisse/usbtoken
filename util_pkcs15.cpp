
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#include "epToken/usbtoken.h"

#include "inc/token.h"
#include "inc/ep_pkcs15.h"
#include "inc/util_pkcs15.h"

#include "libopensc/asn1.h"

#include "engine_pkcs11.h"

#include "inc/ep_thread_ctx.h"

#ifdef REQUEST_EXTENSIONS

/* Add extension using V3 code: we can set the config file as NULL
 * because we wont reference any other sections.
 */

static int 
add_ext(STACK_OF(X509_EXTENSION) *sk, int nid, char *value)
	{
	X509_EXTENSION *ex;
	ex = X509V3_EXT_conf_nid(NULL, NULL, nid, value);
	if (!ex)
		return 0;
	sk_X509_EXTENSION_push(sk, ex);

	return 1;
	}
	
#endif

namespace util {

#ifndef OPENSSL_NO_ENGINE
/* Try to load an engine in a shareable library */
static ENGINE *try_load_engine(const char *sopath)
	{
	ENGINE *e = ENGINE_by_id("dynamic");
	if (e)
		{
		if (!ENGINE_ctrl_cmd_string(e, "SO_PATH", sopath, 0)
			|| !ENGINE_ctrl_cmd_string(e, "LIST_ADD", "1", 0)		
			|| !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0))
			{
			ENGINE_free(e);
			e = NULL;
			}
		}
	return e;
	}

ENGINE *setup_engine(const char *id, const char *sopath, const char *modulepath, const char *pin, const char *args)
        {
        ENGINE *e;
		ENGINE_load_builtin_engines();

        if (id)
                {

		if((e = ENGINE_by_id(id)) == NULL
			&& (e = try_load_engine(sopath)) == NULL)
			{
			fprintf(stderr, "invalid engine \"%s\"\n", id);
			//ERR_print_errors(err);
			return NULL;
			}

	if(!ENGINE_ctrl_cmd_string(e, "MODULE_PATH", modulepath, 0)
		|| !ENGINE_ctrl_cmd_string(e, "PIN", pin, 0)
		|| !ENGINE_ctrl_cmd_string(e, "INIT_ARGS", args, 0)) {
		return NULL;
	}

	if(!ENGINE_init(e)) {
	     /* the engine couldn't initialise, release 'e' */
	     ENGINE_free(e);
	     return NULL;
	 }			

 //   //ENGINE_ctrl_cmd(e, "SET_USER_INTERFACE", 0, ui_method, 0, 1);
	//	if(!ENGINE_set_default(e, ENGINE_METHOD_ALL))
	//		{
	//		printf("can't use that engine\n");
	//		//ERR_print_errors(err);
	//		ENGINE_free(e);
	//		return NULL;
	//		}

		printf("engine \"%s\" set.\n", ENGINE_get_id(e));

		/* Release the functional reference from ENGINE_init() */
		 ENGINE_finish(e);

		/* Free our "structural" reference. */
		ENGINE_free(e);
		}

        return e;
        }
#endif

int ep_open_reader_and_card(struct sc_context **ctx, const char *reader, struct sc_card **card) {
	int verbose = 1;
  sc_context_param_t ctx_param;
  
  memset(&ctx_param, 0, sizeof(ctx_param));
  ctx_param.ver      = 0;
  ctx_param.app_name = E_TOKEN_APPLICATION_NAME;
  ctx_param.thread_ctx = &sc_thread_ctx;

  if (sc_context_create(ctx, &ctx_param))
    return 0;

  if (verbose > 1) {
    (*ctx)->debug = verbose;
    sc_ctx_log_to_file(*ctx, "log.txt");
  }

  return ::util_connect_card(*ctx, card, reader, 0, verbose) ? 0 : 1;
}

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

int ep_authenticate_object(struct sc_pkcs15_card *p15card, sc_pkcs15_object_t *obj, const u8 *pin) {
	sc_pkcs15_object_t	*pin_obj;
	int	r;

	if (obj->auth_id.len == 0)
		return SC_SUCCESS;

	r = sc_pkcs15_find_pin_by_auth_id(p15card, &obj->auth_id, &pin_obj);

	return r != SC_SUCCESS ? r : sc_pkcs15_verify_pin(p15card, pin_obj, pin, pin? strlen((char *) pin) : 0);
}

/* Verify pin */
int ep_verify_pin(struct sc_pkcs15_card *p15card, const char *pin) {
	struct sc_pkcs15_object *pin_obj;
	int r;

  if ((pin_obj = ::util::get_pin_info(p15card)) == NULL) 
    return -1;

  r = sc_pkcs15_verify_pin(p15card, pin_obj, (const u8 *) pin, pin? strlen(pin) : 0);

  return r;
}

EVP_PKEY *ep_load_pubkey(void *data, size_t len, int format)
	{
	BIO *key=NULL;
	EVP_PKEY *pkey=NULL;

	key=BIO_new_mem_buf(data, len);

	if (key == NULL)
		{
		//ERR_print_errors(err);
		goto end;
		}


	if (format == FORMAT_ASN1)
		{
		pkey=d2i_PUBKEY_bio(key, NULL);
		}
#ifndef OPENSSL_NO_RSA
	else if (format == FORMAT_ASN1RSA)
		{
		RSA *rsa;
		rsa = d2i_RSAPublicKey_bio(key, NULL);
		if (rsa)
			{
			pkey = EVP_PKEY_new();
			if (pkey)
				EVP_PKEY_set1_RSA(pkey, rsa);
			RSA_free(rsa);
			}
		else
			pkey = NULL;
		}
	else if (format == FORMAT_PEMRSA)
		{
		RSA *rsa;
		rsa = PEM_read_bio_RSAPublicKey(key, NULL, 
			NULL, NULL);
		if (rsa)
			{
			pkey = EVP_PKEY_new();
			if (pkey)
				EVP_PKEY_set1_RSA(pkey, rsa);
			RSA_free(rsa);
			}
		else
			pkey = NULL;
		}
#endif
	else if (format == FORMAT_PEM)
		{
		pkey=PEM_read_bio_PUBKEY(key,NULL,
			NULL, NULL);
		}
#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
	else if (format == FORMAT_NETSCAPE || format == FORMAT_IISSGC)
		pkey = load_netscape_key(key, format);
#endif
#if !defined(OPENSSL_NO_RSA) && !defined(OPENSSL_NO_DSA)
	else if (format == FORMAT_MSBLOB)
		pkey = b2i_PublicKey_bio(key);
#endif
	else
		{
		//BIO_printf(err,"bad input format specified for key file\n");
		goto end;
		}
 end:
	if (key != NULL) BIO_free(key);
	//if (pkey == NULL)
		//BIO_printf(err,"unable to load %s\n", key_descrip);
	return(pkey);
	}

#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
static EVP_PKEY *
load_netscape_key(BIO *key, int format)
	{
	EVP_PKEY *pkey;
	BUF_MEM *buf;
	RSA	*rsa;
	const unsigned char *p;
	int size, i;

	buf=BUF_MEM_new();
	pkey = EVP_PKEY_new();
	size = 0;
	if (buf == NULL || pkey == NULL)
		goto error;
	for (;;)
		{
		if (!BUF_MEM_grow_clean(buf,size+1024*10))
			goto error;
		i = BIO_read(key, &(buf->data[size]), 1024*10);
		size += i;
		if (i == 0)
			break;
		if (i < 0)
			{
				//BIO_printf(err, "Error reading %s %s",
					//key_descrip, file);
				goto error;
			}
		}
	p=(unsigned char *)buf->data;
	rsa = d2i_RSA_NET(NULL,&p,(long)size,NULL,
		(format == FORMAT_IISSGC ? 1 : 0));
	if (rsa == NULL)
		goto error;
	BUF_MEM_free(buf);
	EVP_PKEY_set1_RSA(pkey, rsa);
	return pkey;
error:
	BUF_MEM_free(buf);
	EVP_PKEY_free(pkey);
	return NULL;
	}
#endif /* ndef OPENSSL_NO_RC4 */

static int make_REQ(X509_REQ *req, EVP_PKEY *pkey, char *subj, int multirdn,
			unsigned long chtype)
	{
	int ret=0,i;

	/* setup version number */
	if (!X509_REQ_set_version(req,0L)) goto err; /* version 1 */

	i = build_subject(req, subj, chtype, multirdn);
	
	if(!i) goto err;

	if (!X509_REQ_set_pubkey(req,pkey)) goto err;

	ret=1;
err:
	return(ret);
	}

static int 
pkey_ctrl_string(EVP_PKEY_CTX *ctx, char *value)
	{
	int rv;
	char *stmp, *vtmp = NULL;
	stmp = BUF_strdup(value);
	if (!stmp)
		return -1;
	vtmp = strchr(stmp, ':');
	if (vtmp)
		{
		*vtmp = 0;
		vtmp++;
		}
	rv = EVP_PKEY_CTX_ctrl_str(ctx, stmp, vtmp);
	OPENSSL_free(stmp);
	return rv;
	}

static int 
do_sign_init(EVP_MD_CTX *ctx, EVP_PKEY *pkey,
		const EVP_MD *md, STACK_OF(OPENSSL_STRING) *sigopts)
	{
	EVP_PKEY_CTX *pkctx = NULL;
	int i;
	EVP_MD_CTX_init(ctx);
	if (!EVP_DigestSignInit(ctx, &pkctx, md, NULL, pkey))
		return 0;
	for (i = 0; i < sk_OPENSSL_STRING_num(sigopts); i++)
		{
		char *sigopt = sk_OPENSSL_STRING_value(sigopts, i);
		if (pkey_ctrl_string(pkctx, sigopt) <= 0)
			{
			//BIO_printf(err, "parameter error \"%s\"\n", sigopt);
			//ERR_print_errors(bio_err);
			return 0;
			}
		}
	return 1;
	}

int do_X509_REQ_sign(X509_REQ *x, EVP_PKEY *pkey, const EVP_MD *md,
			STACK_OF(OPENSSL_STRING) *sigopts)
	{
	int rv;
	EVP_MD_CTX mctx;
	EVP_MD_CTX_init(&mctx);
	rv = do_sign_init(&mctx, pkey, md, sigopts);
	if (rv > 0)
		rv = X509_REQ_sign_ctx(x, &mctx);
	EVP_MD_CTX_cleanup(&mctx);
	return rv > 0 ? 1 : 0;
	}

/*
 * subject is expected to be in the format /type0=value0/type1=value1/type2=...
 * where characters may be escaped by \
 */
static int build_subject(X509_REQ *req, char *subject, unsigned long chtype, int multirdn)
	{
	X509_NAME *n;

	if (!(n = parse_name(subject, chtype, multirdn)))
		return 0;

	if (!X509_REQ_set_subject_name(req, n))
		{
		X509_NAME_free(n);
		return 0;
		}
	X509_NAME_free(n);
	return 1;
}

/*
 * subject is expected to be in the format /type0=value0/type1=value1/type2=...
 * where characters may be escaped by \
 */
static X509_NAME *parse_name(char *subject, long chtype, int multirdn)
	{
	size_t buflen = strlen(subject)+1; /* to copy the types and values into. due to escaping, the copy can only become shorter */
	char *buf = (char *)OPENSSL_malloc(buflen);
	size_t max_ne = buflen / 2 + 1; /* maximum number of name elements */
	char **ne_types = (char **)OPENSSL_malloc(max_ne * sizeof (char *));
	char **ne_values = (char **)OPENSSL_malloc(max_ne * sizeof (char *));
	int *mval = (int *)OPENSSL_malloc (max_ne * sizeof (int));

	char *sp = subject, *bp = buf;
	int i, ne_num = 0;

	X509_NAME *n = NULL;
	int nid;

	if (!buf || !ne_types || !ne_values)
		{
		//BIO_printf(bio_err, "malloc error\n");
		goto error;
		}	

	if (*subject != '/')
		{
		//BIO_printf(bio_err, "Subject does not start with '/'.\n");
		goto error;
		}
	sp++; /* skip leading / */

	/* no multivalued RDN by default */
	mval[ne_num] = 0;

	while (*sp)
		{
		/* collect type */
		ne_types[ne_num] = bp;
		while (*sp)
			{
			if (*sp == '\\') /* is there anything to escape in the type...? */
				{
				if (*++sp)
					*bp++ = *sp++;
				else	
					{
					//BIO_printf(bio_err, "escape character at end of string\n");
					goto error;
					}
				}	
			else if (*sp == '=')
				{
				sp++;
				*bp++ = '\0';
				break;
				}
			else
				*bp++ = *sp++;
			}
		if (!*sp)
			{
			//BIO_printf(bio_err, "end of string encountered while processing type of subject name element #%d\n", ne_num);
			goto error;
			}
		ne_values[ne_num] = bp;
		while (*sp)
			{
			if (*sp == '\\')
				{
				if (*++sp)
					*bp++ = *sp++;
				else
					{
					//BIO_printf(bio_err, "escape character at end of string\n");
					goto error;
					}
				}
			else if (*sp == '/')
				{
				sp++;
				/* no multivalued RDN by default */
				mval[ne_num+1] = 0;
				break;
				}
			else if (*sp == '+' && multirdn)
				{
				/* a not escaped + signals a mutlivalued RDN */
				sp++;
				mval[ne_num+1] = -1;
				break;
				}
			else
				*bp++ = *sp++;
			}
		*bp++ = '\0';
		ne_num++;
		}	

	if (!(n = X509_NAME_new()))
		goto error;

	for (i = 0; i < ne_num; i++)
		{
		if ((nid=OBJ_txt2nid(ne_types[i])) == NID_undef)
			{
			//BIO_printf(bio_err, "Subject Attribute %s has no known NID, skipped\n", ne_types[i]);
			continue;
			}

		if (!*ne_values[i])
			{
			//BIO_printf(bio_err, "No value provided for Subject Attribute %s, skipped\n", ne_types[i]);
			continue;
			}

		if (!X509_NAME_add_entry_by_NID(n, nid, chtype, (unsigned char*)ne_values[i], -1,-1,mval[i]))
			goto error;
		}

	OPENSSL_free(ne_values);
	OPENSSL_free(ne_types);
	OPENSSL_free(buf);
	return n;

error:
	X509_NAME_free(n);
	if (ne_values)
		OPENSSL_free(ne_values);
	if (ne_types)
		OPENSSL_free(ne_types);
	if (buf)
		OPENSSL_free(buf);
	return NULL;
}

std::string Format(char *fmt, ...) {
    int size = 100;
    std::string str;
    va_list ap;
    for(;;) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt, ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            return str;
        }
        if (n > -1)
            size=n+1;
        else
            size*=2;
    }
}

int ep_gen_x509_req(epsilon::TokenContext *ctx, CefRefPtr<ClientHandler> handler, const char *id, const unsigned char *cn, const unsigned char *o, 
	const unsigned char *ou, const unsigned char *city, const unsigned char *region, const unsigned char *country, const unsigned char *emailAddress, const char *authData) {
	
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15_object *obj;
	struct sc_pkcs15_pubkey *pubkey = NULL;
	struct sc_pkcs15_cert *cert = NULL;
	struct sc_pkcs15_id kid;	
	int r = -1, i;

	ENGINE *e = NULL;

	X509_REQ *x = NULL;
	EVP_PKEY *pk = NULL;
	STACK_OF(X509_EXTENSION) *exts = NULL;

	FILE *fp = NULL;

	std::string path, defaultFileName;

	u8 *out;

	if((p15card = epsilon::BindP15Card(ctx)) == NULL)
		return r;

	if(ep_verify_pin(p15card, authData) < 0)
		goto out;

	memset(&kid, 0, sizeof(kid));
	memset(kid.value, 0, sizeof(kid.value));
	sc_pkcs15_format_id(id, &kid);

	i = sc_pkcs15_find_pubkey_by_id(p15card, &kid, &obj);
	if (i >= 0) {
		//if (verbose)
			//printf("Reading public key with ID '%s'\n", opt_pubkey);
		i = ep_authenticate_object(p15card, obj, (const u8 *)authData);
		if (i >= 0)
			i = sc_pkcs15_read_pubkey(p15card, obj, &pubkey);
	} else if (i == SC_ERROR_OBJECT_NOT_FOUND) {
	
		/* No pubkey - try if there's a certificate */
		i = sc_pkcs15_find_cert_by_id(p15card, &kid, &obj);
		if (i >= 0) {
			//if (verbose)
			//printf("Reading certificate with ID '%s'\n", opt_pubkey);			
			i = sc_pkcs15_read_certificate(p15card, (struct sc_pkcs15_cert_info *)obj->data, &cert);
		}

		if (i >= 0)
			pubkey = cert->key;
	}

	if(i < 0)
		goto out;

	void *p = pubkey->data.value;
	if((pk=ep_load_pubkey(p, pubkey->data.len, FORMAT_ASN1RSA)) == NULL)
		goto end;
	
	if ((x=X509_REQ_new()) == NULL)
		{
		goto end;
		}

	if (!make_REQ(x, pk, (char *)Format("/CN=%s/O=%s/OU=%s/L=%s/ST=%s/C=%s/emailAddress=%s/", cn, o, ou, city, region, country, emailAddress).c_str()))
		{
		//BIO_printf(bio_err,"problems making Certificate Request\n");
		goto end;
		}

#ifdef REQUEST_EXTENSIONS
	/* Certificate requests can contain extensions, which can be used
	 * to indicate the extensions the requestor would like added to 
	 * their certificate. CAs might ignore them however or even choke
	 * if they are present.
	 */

	/* For request extensions they are all packed in a single attribute.
	 * We save them in a STACK and add them all at once later...
	 */

	exts = sk_X509_EXTENSION_new_null();
	/* Standard extenions */

	add_ext(exts, NID_key_usage, "critical,digitalSignature,keyEncipherment");

	/* This is a typical use for request extensions: requesting a value for
	 * subject alternative name.
	 */

	add_ext(exts, NID_subject_alt_name, (char *)Format("email:%s", emailAddress).c_str());

	/* Some Netscape specific extensions */
	add_ext(exts, NID_netscape_cert_type, "client,email");



//#ifdef CUSTOM_EXT
//	/* Maybe even add our own extension based on existing */
//	{
//		int nid;
//		nid = OBJ_create("1.2.3.4", "MyAlias", "My Test Alias Extension");
//		X509V3_EXT_add_alias(nid, NID_netscape_comment);
//		add_ext(x, nid, "example comment alias");
//	}
//#endif

	/* Now we've created the extensions we add them to the request */

	X509_REQ_add_extensions(x, exts);

	sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);

#endif

	e = setup_engine(
					"pkcs11", 
					".\\engine_pkcs11.dll",
					".\\opensc-pkcs11.dll", authData, NULL
				);

	const char *iid = ENGINE_get_id(e);
	printf("%d", iid);

	if(sc_pkcs15_find_prkey_by_id(p15card, &kid, &obj) == 0) {
	
		u8 *x509_req = NULL;
		int size;
		if((size = i2d_X509_REQ(x, &x509_req)) < 0)
			goto end;

		
		x509_req[size] = 0;

		/* This has to be done on the key */
		out = (u8 *)malloc(size);
		// if(ep_sign(p15card, obj, x509_req, size, SC_ALGORITHM_RSA_HASH_SHA1|SC_ALGORITHM_RSA_PAD_PKCS1, out) == 0) {
			defaultFileName = Format("%s.csr", obj->label);
			path = handler->GetDownloadPath(defaultFileName);

			if (path.empty()) {
				//Save to My Documents
				  path = defaultFileName;
			}

			fp = fopen(path.c_str(), "w");

			if(!fp)
				goto end;

			if(!PEM_write_X509_REQ_NEW(fp, x)) 
				goto end;

			r = 0;		
		// }	
	}
	
end: 

	if(e)
		e = NULL;

	if(out)
		free(out);

	if(fp)
		fclose(fp);

	if(x)
		X509_REQ_free(x);
	
	if(pk)
		EVP_PKEY_free(pk);

out : 
	
	if(pubkey)
		sc_pkcs15_free_pubkey(pubkey);

	epsilon::ReleaseP15Card(p15card);

	return e == NULL && r;
}

int ep_export_x509_certificate(epsilon::TokenContext *ctx, CefRefPtr<ClientHandler> handler, const char *path, const char *id, const char *authData, unsigned int format) {
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15_object *obj;
	struct sc_pkcs15_cert *cert = NULL;
	struct sc_pkcs15_id kid;	
	int r = -1;

	std::string defaultFileName = Format("%s.%s", path, format == FORMAT_ASN1 ? "der" : "crt"), mypath;

	FILE *fp;

	X509 *x509;

	if((p15card = epsilon::BindP15Card(ctx)) == NULL)
		return r;

	if((r = ep_verify_pin(p15card, authData)) < 0)
		goto out;

	memset(&kid, 0, sizeof(kid));
	memset(kid.value, 0, sizeof(kid.value));
	sc_pkcs15_format_id(id, &kid);

	r = sc_pkcs15_find_cert_by_id(p15card, &kid, &obj);
	if (r >= 0) {
		//if (verbose)
		//printf("Reading certificate with ID '%s'\n", opt_pubkey);			
		r = sc_pkcs15_read_certificate(p15card, (struct sc_pkcs15_cert_info *)obj->data, &cert);
	}

	if(r >= 0) {

		r = -1;

		if((x509 = d2i_X509(NULL, (const u8 **)&cert->data, cert->data_len)) == NULL)
			goto out;
		
		/* Select a sensible label based on subject's name */

		//defaultFileName = Format("%s.%s", obj->label, format == FORMAT_ASN1 ? "der" : "crt");
		mypath = handler->GetDownloadPath(defaultFileName);

		if (mypath.empty())
				mypath = defaultFileName;

		fp = fopen(mypath.c_str(), "w");

		if(!fp)
			goto out;

		switch(format) {
		case FORMAT_ASN1:

			if(!i2d_X509_fp(fp,x509)) 
				goto out;	

			break;
		default:

			if(!PEM_write_X509(fp, x509)) 
				goto out;	

			break;
		}

		r = 0;
	}

out : 
	
	if(fp)
		fclose(fp);

	if(x509)
		X509_free(x509);
	
	epsilon::ReleaseP15Card(p15card);

	return r;
}

/*  Key management functions */

static int
print_pem_object(unsigned char **buf, const u8 *data, size_t data_len) {
	size_t		buf_len = 1024;
	int		r;

	/* With base64, every 3 bytes yield 4 characters, and with
	 * 64 chars per line we know almost exactly how large a buffer we
	 * will need. */
	buf_len = (data_len + 2) / 3 * 4;
	buf_len += 2 * (buf_len / 64 + 2); /* certain platforms use CRLF */
	buf_len += 64;			   /* slack for checksum etc */

	if (!(*buf = (u8 *)malloc(buf_len))) {
		perror("print_pem_object");
		return 1;
	}

	r = sc_base64_encode(data, data_len, *buf, buf_len, 64);
	if (r < 0) {
		fprintf(stderr, "Base64 encoding failed: %s\n", sc_strerror(r));
		free(buf);
		return r;
	}

	/*if (opt_outfile != NULL) {
		outf = fopen(opt_outfile, "w");
		if (outf == NULL) {
			fprintf(stderr, "Error opening file '%s': %s\n",
				opt_outfile, strerror(errno));
			free(buf);
			return 2;
		}
	} else
	fprintf(stdout,
		"-----BEGIN %s-----\n"
		"%s"
		"-----END %s-----\n",
		kind, buf, kind);
	if (outf != stdout)
		fclose(outf);
	free(buf);*/
	return buf_len;
}

int ep_delete_prkey(epsilon::TokenContext *ctx, const char *id, const char *authData) {
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15_object *obj;
	struct sc_pkcs15_id kid;
	int r;

	if((p15card = epsilon::BindP15Card(ctx)) == NULL)
		return 0;

	if((r = ep_verify_pin(p15card, authData)) < 0)
		goto out;

	memset(&kid, 0, sizeof(kid));
	memset(kid.value, 0, sizeof(kid.value));
	sc_pkcs15_format_id(id, &kid);

	if ((r = sc_pkcs15_find_prkey_by_id(p15card, &kid, &obj)) != 0)
		goto out;

	r = sc_pkcs15init_delete_object(p15card, ctx->GetProfile(), obj);

	if(r >= 0 && sc_pkcs15_find_pubkey_by_id(p15card, &kid, &obj) == 0)
		r = sc_pkcs15init_delete_object(p15card, ctx->GetProfile(), obj);		

	out : epsilon::ReleaseP15Card(p15card);

	return r;
}

int ep_delete_x509_certificate(epsilon::TokenContext *ctx, const char *id, const char *authData) {
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15_object *obj;
	struct sc_pkcs15_id kid;
	int r;

	if((p15card = epsilon::BindP15Card(ctx)) == NULL)
		return 0;

	if((r = ep_verify_pin(p15card, authData)) < 0)
		goto out;

	memset(&kid, 0, sizeof(kid));
	memset(kid.value, 0, sizeof(kid.value));
	sc_pkcs15_format_id(id, &kid);

	if ((r = sc_pkcs15_find_cert_by_id(p15card, &kid, &obj)) != 0)
		goto out;

	r = sc_pkcs15init_delete_object(p15card, ctx->GetProfile(), obj);

	out : epsilon::ReleaseP15Card(p15card);

	return r;
}

/*
 * Generate a new private key
 */
int ep_generate_key(epsilon::TokenContext *ctx, 
  const char *spec, const char *pubkey_label, char **id, char *authid, const char *authData, int x509_usage) {
	struct sc_pkcs15init_keygen_args keygen_args;
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15_object *obj;
	unsigned int keybits = 1024;
	int		r;

	if((p15card = epsilon::BindP15Card(ctx)) == NULL)
		return 0;

	if((r = ep_verify_pin(p15card, authData)) < 0)
		goto out;

	memset(&keygen_args, 0, sizeof(keygen_args));
	keygen_args.pubkey_label = pubkey_label;

	if ((r = init_keyargs(&keygen_args.prkey_args, NULL, authid, pubkey_label, x509_usage)) < 0)
		goto out;

  keygen_args.prkey_args.access_flags |= 
	  SC_PKCS15_PRKEY_ACCESS_SENSITIVE 
	| SC_PKCS15_PRKEY_ACCESS_ALWAYSSENSITIVE 
	| SC_PKCS15_PRKEY_ACCESS_NEVEREXTRACTABLE
	| SC_PKCS15_PRKEY_ACCESS_LOCAL;

	/* Parse the key spec given on the command line */
	if (!_strnicmp(spec, "rsa", 3)) {
		keygen_args.prkey_args.key.algorithm = SC_ALGORITHM_RSA;
		spec += 3;
	} 

	else if (!_strnicmp(spec, "dsa", 3)) {
		keygen_args.prkey_args.key.algorithm = SC_ALGORITHM_DSA;
		spec += 3;
	} 

	else if (!_strnicmp(spec, "gost2001", strlen("gost2001"))) {
		keygen_args.prkey_args.key.algorithm = SC_ALGORITHM_GOSTR3410;
		keybits = SC_PKCS15_GOSTR3410_KEYSIZE;
		/* FIXME: now only SC_PKCS15_PARAMSET_GOSTR3410_A */
		keygen_args.prkey_args.params.gost.gostr3410 = SC_PKCS15_PARAMSET_GOSTR3410_A;
		spec += strlen("gost2001");
	} 

	else if (!_strnicmp(spec, "ec", 2)) {
		keygen_args.prkey_args.key.algorithm = SC_ALGORITHM_EC;
		spec += 2;
	} 

	else {
		//util_error("Unknown algorithm \"%s\"", spec);
		return SC_ERROR_INVALID_ARGUMENTS;
	}

	if (*spec == '/' || *spec == '-' || *spec == ':')
		spec++;

	if (*spec)   {
		if (isalpha(*spec) && keygen_args.prkey_args.key.algorithm == SC_ALGORITHM_EC)   {
			keygen_args.prkey_args.params.ec.named_curve = _strdup(spec);
			keybits = 0;
		}
		else {
			char	*end;

			keybits = strtoul(spec, &end, 10);
			if (*end) {
				printf("Invalid number of key bits \"%s\"", spec);
				r = SC_ERROR_INVALID_ARGUMENTS;
				goto out;
			}
		}
	}
	r = sc_pkcs15init_generate_key(p15card, ctx->GetProfile(), &keygen_args, keybits, &obj);

	if(r >= 0) {

		struct sc_pkcs15_id kid = ((struct sc_pkcs15_prkey_info *) obj->data)->id;
		r = sc_pkcs15_find_pubkey_by_id(p15card, &kid, &obj);
		if (r != 0) {
			printf("sc_pkcs15_find_pubkey_by_id returned %d\n", r);
			goto out;
		}

		//*id = (char *)sc_pkcs15_print_id(&keygen_args.prkey_args.id);
		*id = (char *)sc_pkcs15_print_id(&kid);
	}

	out : epsilon::ReleaseP15Card(p15card);

	return r;
}

static int 
init_keyargs(struct sc_pkcs15init_prkeyargs *args, 
	char *id, char *authid, const char *label, int x509_usage) {
	memset(args, 0, sizeof(*args));
	memset(args->id.value, 0, sizeof(args->id.value));
	
	if (id) 
		sc_pkcs15_format_id(id, &args->id);
	
	if (authid) 
		sc_pkcs15_format_id(authid, &args->auth_id);

	// else if (!opt_insecure) {
	// 	util_error("no PIN given for key - either use --insecure or \n"
	// 	      "specify a PIN using --auth-id");
	// 	return SC_ERROR_INVALID_ARGUMENTS;
	// }

	//if (opt_extractable) {
		//args->access_flags |= SC_PKCS15_PRKEY_ACCESS_EXTRACTABLE;
	//}

	if(label) 
		args->label = label;

	if(x509_usage != 0) 
		args->x509_usage = x509_usage;
	
	return 0;
}

/*
 * Read a private key
 */
static int 
pass_cb(char *buf, int len, int flags, void *d) {
	size_t pass_len = 0;
	int  plen;
	char *pass = (char *)d;

	plen = strlen(pass);
	if (plen <= 0)
		return 0;
	if (plen > len)
		plen = len;
	memcpy(buf, pass, plen);
	return plen;
}

static u8 *
ep_base64_decode(const char *input, int length) {
	u8 *buffer = (u8 *)malloc(length);
	memset(buffer, 0, length);

	if(sc_base64_decode(input, buffer, length) < 0) {
		free(buffer);
		return NULL;
	}

	return buffer;
}

static int
do_read_pem_private_key(const char *data, size_t len, const char *passphrase,
			EVP_PKEY **key) {
	BIO	*bio;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		return -1;

	void *p = buff;
	bio = BIO_new_mem_buf(p, len);
	*key = PEM_read_bio_PrivateKey(bio, NULL, pass_cb, (char *) passphrase);
	BIO_free(bio);
	free(buff);
	
	if (*key == NULL) {
		//ossl_print_errors();
		return SC_ERROR_CANNOT_LOAD_KEY;
	}
	
	return 0;
}

static int
do_read_pkcs12_private_key(const char *data, size_t len, const char *passphrase,
			EVP_PKEY **key, X509 **certs, unsigned int max_certs) {
	PKCS12		*p12;
	EVP_PKEY	*user_key = NULL;
	X509		*user_cert = NULL;
	STACK_OF(X509)	*cacerts = NULL;
	int		i, ncerts = 0;

	*key = NULL;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		goto error;

	const u8 *p = buff;
	p12 = d2i_PKCS12(NULL, &p, len);

	if (p12 == NULL
	 || !PKCS12_parse(p12, passphrase, &user_key, &user_cert, &cacerts)) {
		
		free(buff);
		goto error;
	}

	if (!user_key) {
		printf("No key in pkcs12 file?!\n");
		free(buff);
		return SC_ERROR_CANNOT_LOAD_KEY;
	}

	CRYPTO_add(&user_key->references, 1, CRYPTO_LOCK_EVP_PKEY);
	if (user_cert && max_certs)
		certs[ncerts++] = user_cert;

	/* Extract CA certificates, if any */
	for(i = 0; cacerts && ncerts < (int)max_certs && i < sk_X509_num(cacerts); i++)
		certs[ncerts++] = sk_X509_value(cacerts, i);

	/* bump reference counts for certificates */
	for (i = 0; i < ncerts; i++) {
		CRYPTO_add(&certs[i]->references, 1, CRYPTO_LOCK_X509);
	}

	if (cacerts)
		sk_X509_free(cacerts);

	free(buff);
	*key = user_key;
	return ncerts;

error:	//ossl_print_errors();
	return SC_ERROR_CANNOT_LOAD_KEY;
}

int ep_read_private_key(const char *data, size_t len, const char *format,
			EVP_PKEY **pk, X509 **certs, unsigned int max_certs, char *passphrase) {
	int	r;

	if (!format || !_stricmp(format, "pem")) 
		r = do_read_pem_private_key(data, len, passphrase, pk);

	else if (!_stricmp(format, "pkcs12")) {
		
		r = do_read_pkcs12_private_key(data, len,
				passphrase, pk, certs, max_certs);		

	} else {

		printf("Error when reading private key. "
		      "Key file format \"%s\" not supported.\n", format);
		return SC_ERROR_NOT_SUPPORTED;

	}

	if (r < 0)
		printf("Unable to read private key\n");

	return r;
}

/*
 * Read a public key
 */
static EVP_PKEY *
do_read_pem_public_key(const char *data, size_t len) {
	BIO		*bio;
	EVP_PKEY	*pk;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		return NULL;

	void *p = buff;
	bio = BIO_new_mem_buf(p, len);
	pk = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
	BIO_free(bio);
	free(buff);
	//if (pk == NULL) 
		; //ossl_print_errors();
	return pk;
}

static EVP_PKEY *
do_read_der_public_key(const char *data, size_t len) {
	BIO	*bio;
	EVP_PKEY *pk;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		return NULL;

	void *p = buff;
	bio = BIO_new_mem_buf(p, len);	
	pk = d2i_PUBKEY_bio(bio, NULL);
	BIO_free(bio);
	free(buff);
	//if (pk == NULL) 
		; //ossl_print_errors();
	return pk;
}

int ep_read_public_key(const char *data, size_t len, const char *format, EVP_PKEY **out) {
	
	if (!format || !_stricmp(format, "pem")) {
		*out = do_read_pem_public_key(data, len);
	} 

	else if (!_stricmp(format, "der")) {
		*out = do_read_der_public_key(data, len);
	} 

	else {
		printf("Error when reading public key. "
		      "File format \"%s\" not supported.\n",
		      format);
	}

	if (!*out) {
		printf("Unable to read public key\n");
		return -1;
	}

	return 0;
}

/*
 * Read a certificate
 */
static X509 *
do_read_pem_certificate(const char *data, size_t len) {
	BIO	*bio;
	X509	*xp;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		return NULL;

	void *p = buff;
	bio = BIO_new_mem_buf(p, len);
	xp = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
	free(buff);
	//if (xp == NULL) 
		 //ossl_print_errors();
	return xp;
}

static X509 *
do_read_der_certificate(const char *data, size_t len) {
	//BIO	*bio;
	X509	*xp;

	u8 *buff = ep_base64_decode(data, len);
	if(buff == NULL)
		return NULL;

	//bio = BIO_new_mem_buf(buff, len);
	const u8 *p = buff;
	xp = d2i_X509(NULL, &p, len);
	free(buff);
	//if (xp == NULL) 
		 //ossl_print_errors();
	return xp;
}

int ep_read_certificate(const char *data, size_t len, const char *format, X509 **out) {
	
	if (!format || !_stricmp(format, "pem")) {
		*out = do_read_pem_certificate(data, len);
	} 

	else if (!_stricmp(format, "der")) {
		*out = do_read_der_certificate(data, len);
	} 

	else {
		printf("Error when reading certificate. "
		      "File format \"%s\" not supported.\n",
		      format);
	}

	if (!*out) {
		printf("Unable to read certificate\n");
		return -1;
	}

	return 0;
}

/* success if r >= 0 */
int ep_token_read_certificate(epsilon::TokenContext *ctx, struct ep_key_info *key, const char *cert_id) {
  int r = -1, i, count;
  struct sc_pkcs15_id id;
  struct sc_pkcs15_object *objs[32];
  struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {

	  id.len = SC_PKCS15_MAX_ID_SIZE;
	  sc_pkcs15_hex_string_to_id(cert_id, &id);
  
	  r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_CERT_X509, objs, 32);
	  if (r < 0) {
		fprintf(stderr, "Certificate enumeration failed: %s\n", sc_strerror(r));
		goto out;
	  }

	  count = r;
	  for (i = 0; i < count; i++) {
		struct sc_pkcs15_cert_info *cinfo = (struct sc_pkcs15_cert_info *) objs[i]->data;
		struct sc_pkcs15_cert *cert = NULL;

		if (sc_pkcs15_compare_id(&id, &cinfo->id) != 1)
		  continue;
      
		/*if (verbose)
		  printf("Reading certificate with ID '%s'\n", opt_cert); */
		r = sc_pkcs15_read_certificate(p15card, cinfo, &cert);
		if (r) {
		  fprintf(stderr, "Certificate read failed: %s\n", sc_strerror(r));
		  goto out;
		}

		  memset(key, 0, sizeof(*key));
		  memset(key->id, 0, sizeof(unsigned char));
		  memset(key->label, 0, sizeof(char));
		  memset(key->authid, 0, sizeof(unsigned char));		  

		key->type = EP_KEY_TYPE_CERT;
	
		memcpy(key->id, sc_pkcs15_print_id(&cinfo->id), sizeof(key->id));
	
		memcpy(key->label, objs[i]->label, sizeof(key->label));
		memcpy(key->authid, sc_pkcs15_print_id(&(objs[i]->auth_id)), sizeof(key->authid));

		//key->key.cert.data = (u8 *) malloc(cert->data_len * sizeof(u8));		

		u8	*data = NULL;
		if(cert->data && cert->data_len)   {

			memset(&key->key.cert, 0, sizeof(key->key.cert));

			const unsigned char *p = (const unsigned char*)cert->data;
			X509 *xp = d2i_X509(NULL, &p, cert->data_len);

			key->key.cert.version = cert->version;

			//memset(key->key.cert.serial_number, 0, sizeof(u8));			
			key->key.cert.serial_number = cert->serial;
			
			memset(key->key.cert.subject_common_name, 0, sizeof(key->key.cert.subject_common_name));
			memset(key->key.cert.subject_locality_name, 0, sizeof(key->key.cert.subject_locality_name));
			memset(key->key.cert.subject_state_name, 0, sizeof(key->key.cert.subject_state_name));
			memset(key->key.cert.subject_organization_name, 0, sizeof(key->key.cert.subject_organization_name));
			memset(key->key.cert.subject_organization_unit_name, 0, sizeof(key->key.cert.subject_organization_unit_name));
			memset(key->key.cert.subject_title_name, 0, sizeof(key->key.cert.subject_title_name));
			memset(key->key.cert.subject_email_address, 0, sizeof(key->key.cert.subject_email_address));

			int loc = -1;

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_commonName, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_common_name, e->value->data, sizeof(key->key.cert.subject_common_name));
				}
			}

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_localityName, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_locality_name, e->value->data, sizeof(key->key.cert.subject_locality_name));
				}
			}

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_stateOrProvinceName, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_state_name, e->value->data, sizeof(key->key.cert.subject_state_name));
				}
			}

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_organizationName, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_organization_name, e->value->data, sizeof(key->key.cert.subject_organization_name));
				}
			}
			
			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_organizationalUnitName, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_organization_unit_name, e->value->data, sizeof(key->key.cert.subject_organization_unit_name));
				}
			}

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_title, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_title_name, e->value->data, sizeof(key->key.cert.subject_title_name));
				}
			}

			if((loc = X509_NAME_get_index_by_NID(xp->cert_info->subject, NID_Mail, -1)) != -1) {
				X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->subject, loc);
				if(e->set) {
					memcpy(key->key.cert.subject_email_address, e->value->data, sizeof(key->key.cert.subject_email_address));
				}
			}

			{
				const ASN1_TIME *validity = NULL;
				//memset(key->key.cert.notBefore, 0, sizeof(u8));	
				//(key->key.cert.notAfter, 0, sizeof(u8));				
			
				if((validity = X509_get_notBefore(xp)) != NULL && validity->length > 0)
					key->key.cert.notBefore = validity->data;

				if((validity = X509_get_notAfter(xp)) != NULL && validity->length > 0)
					key->key.cert.notAfter = validity->data;

			}

			if(xp->cert_info->issuer != NULL) {
				key->key.cert.issuer_set = 1;

				memset(key->key.cert.issuer_common_name, 0, sizeof(key->key.cert.issuer_common_name));
				memset(key->key.cert.issuer_locality_name, 0, sizeof(key->key.cert.issuer_locality_name));
				memset(key->key.cert.issuer_state_name, 0, sizeof(key->key.cert.issuer_state_name));
				memset(key->key.cert.issuer_organization_name, 0, sizeof(key->key.cert.issuer_organization_name));
				memset(key->key.cert.issuer_organization_unit_name, 0, sizeof(key->key.cert.issuer_organization_unit_name));
				memset(key->key.cert.issuer_title_name, 0, sizeof(key->key.cert.issuer_title_name));
				memset(key->key.cert.issuer_email_address, 0, sizeof(key->key.cert.issuer_email_address));

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_commonName, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_common_name, e->value->data, sizeof(key->key.cert.issuer_common_name));
					}
				}

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_localityName, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_locality_name, e->value->data, sizeof(key->key.cert.issuer_locality_name));
					}
				}

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_stateOrProvinceName, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_state_name, e->value->data, sizeof(key->key.cert.issuer_state_name));
					}
				}

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_organizationName, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_organization_name, e->value->data, sizeof(key->key.cert.issuer_organization_name));
					}
				}
			
				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_organizationalUnitName, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_organization_unit_name, e->value->data, sizeof(key->key.cert.issuer_organization_unit_name));
					}
				}

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_title, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_title_name, e->value->data, sizeof(key->key.cert.issuer_title_name));
					}
				}

				if((loc = X509_NAME_get_index_by_NID(xp->cert_info->issuer, NID_Mail, -1)) != -1) {
					X509_NAME_ENTRY *e = X509_NAME_get_entry(xp->cert_info->issuer, loc);
					if(e->set) {
						memcpy(key->key.cert.issuer_email_address, e->value->data, sizeof(key->key.cert.issuer_email_address));
					}
				}
			}

			/*r = print_pem_object(&data, cert->data, cert->data_len);

		  //r = pubkey_pem_encode(pubkey, &pubkey->data, &pem_key);
		  if (r < 0) {
			fprintf(stderr, "Error encoding PEM key: %s\n", sc_strerror(r));
			free(data);
		  } else {
			  key->key.cert.data = data;
			  key->key.cert.len = r;
		  }*/
		}

		//print_pem_object("CERTIFICATE", cert->data, cert->data_len);
		/*free:*/ sc_pkcs15_free_certificate(cert);

		goto out;
	  }

	  fprintf(stderr, "Certificate with ID '%s' not found.\n", cert_id);

	  out: epsilon::ReleaseP15Card(p15card);
	}
	return r;
}

/* success if r >= 0 */
int ep_token_read_pubkey(epsilon::TokenContext *ctx, struct ep_key_info *key, const char *key_id, u8 *pin) {
  int r = -1;
  struct sc_pkcs15_id id;
  struct sc_pkcs15_object *obj;
  sc_pkcs15_pubkey_t *pubkey = NULL;
  sc_pkcs15_cert_t *cert = NULL;
  //sc_pkcs15_der_t pem_key;
  struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {

	  id.len = SC_PKCS15_MAX_ID_SIZE;
	  sc_pkcs15_hex_string_to_id(key_id, &id);

	  r = sc_pkcs15_find_pubkey_by_id(p15card, &id, &obj);
	  if (r >= 0) {
		//if (verbose)
		  //printf("Reading public key with ID '%s'\n", opt_pubkey);
		r = ep_authenticate_object(p15card, obj, pin);
		if (r >= 0)
		  r = sc_pkcs15_read_pubkey(p15card, obj, &pubkey);
	  } else if (r == SC_ERROR_OBJECT_NOT_FOUND) {
		/* No pubkey - try if there's a certificate */
		r = sc_pkcs15_find_cert_by_id(p15card, &id, &obj);
		if (r >= 0) {
		  //if (verbose)
			//printf("Reading certificate with ID '%s'\n", opt_pubkey);
		  r = sc_pkcs15_read_certificate(p15card,
			(sc_pkcs15_cert_info_t *) obj->data,
			&cert);
		}
		if (r >= 0)
		  pubkey = cert->key;
	  }

	  if (r == SC_ERROR_OBJECT_NOT_FOUND) {
		fprintf(stderr, "Public key with ID '%s' not found.\n", key_id);
		goto out;
	  }

	  if (r < 0) {
		fprintf(stderr, "Public key enumeration failed: %s\n", sc_strerror(r));
		goto out;
	  }

	  if (!pubkey) {
		fprintf(stderr, "Public key not available\n");
		goto out;
	  }

		  memset(key, 0, sizeof(*key));
		  memset(key->id, 0, sizeof(unsigned char));
		  memset(key->label, 0, sizeof(char));
		  memset(key->authid, 0, sizeof(unsigned char));

	  key->type = EP_KEY_TYPE_PUBKEY;
	
		memcpy(key->id, key_id, sizeof(key->id));
	
		memcpy(key->label, obj->label, sizeof(key->label));
		memcpy(key->authid, sc_pkcs15_print_id(&obj->auth_id), sizeof(key->authid));

		//key->key.pubkey.data = malloc(pubkey->data.len * sizeof(u8));		

	u8	*data;
	if (pubkey->data.value && pubkey->data.len)   {
		/* public key data is present as 'direct' value of pkcs#15 object */
		/*data = (u8 *) calloc(1, pubkey->data.len);
		if (!data) {
			//SC_FUNC_RETURN(ctx, SC_LOG_DEBUG_NORMAL, SC_ERROR_OUT_OF_MEMORY);
			printf("ep_token_read_pubkey:Out of memory");
			goto free;
		}*/
		
		//memcpy(data, pubkey->data.value, pubkey->data.len);
		//len = obj->content.len;
		
		r = print_pem_object(&data, pubkey->data.value, pubkey->data.len);

	  //r = pubkey_pem_encode(pubkey, &pubkey->data, &pem_key);
	  if (r < 0) {
		fprintf(stderr, "Error encoding PEM key: %s\n", sc_strerror(r));
		free(data);
	  } else {
		  key->key.pubkey.data = data;
		  key->key.pubkey.len = r;
	  }

	}		
	  
	  if (cert)
		sc_pkcs15_free_certificate(cert);
	  else if (pubkey)
		sc_pkcs15_free_pubkey(pubkey);


		out: epsilon::ReleaseP15Card(p15card);
	}
  return r;
}

/* success if r >= 0 */
int ep_token_read_prkey(epsilon::TokenContext *ctx, struct ep_key_info *key, const char *key_id) {
  int r = -1;
  struct sc_pkcs15_id id;
  struct sc_pkcs15_object *obj;
  sc_pkcs15_prkey_info *prkey = NULL;
  struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {

	  id.len = SC_PKCS15_MAX_ID_SIZE;
	  sc_pkcs15_hex_string_to_id(key_id, &id);

	  r = sc_pkcs15_find_prkey_by_id(p15card, &id, &obj);

	  if (r >= 0) {

		  prkey = (struct sc_pkcs15_prkey_info *) obj->data;

		  memset(key, 0, sizeof(*key));
		  memset(key->id, 0, sizeof(unsigned char));
		  memset(key->label, 0, sizeof(char));
		  memset(key->authid, 0, sizeof(unsigned char));

   		key->type = EP_KEY_TYPE_PRKEY;
	
		memcpy(key->id, sc_pkcs15_print_id(&prkey->id), sizeof(key->id));
	
		memcpy(key->label, obj->label, sizeof(key->label));
		memcpy(key->authid, sc_pkcs15_print_id(&obj->auth_id), sizeof(key->authid));

	  } 

	  /* Key will be cleared by `epsilon::ReleaseP15Card(p15card);`*/
	  //if (prkey) 
		  //sc_pkcs15_free_prkey_info(prkey);


		epsilon::ReleaseP15Card(p15card);
	}
  return r;
}

int ep_token_read_all_prkeys(epsilon::TokenContext *ctx, struct ::ep_key_info **keys, size_t ret_size) {
	int r = -1;
	struct sc_pkcs15_object *objs[32];
	struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {
	
		r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_PRKEY, objs, 32);
		if (r <= 0) {
			fprintf(stderr, "Private key enumeration failed: %s\n", sc_strerror(r));
			goto out;
		}

		for(int i = 0; i < r; ++i) {
			struct sc_pkcs15_prkey_info *prkey = (struct sc_pkcs15_prkey_info *) objs[i]->data;
			struct ::ep_key_info *key = (struct ::ep_key_info *) malloc(sizeof(struct ::ep_key_info));

			  memset(key, 0, sizeof(*key));
			  memset(key->id, 0, sizeof(unsigned char));
			  memset(key->label, 0, sizeof(char));
			  memset(key->authid, 0, sizeof(unsigned char));

   			key->type = EP_KEY_TYPE_PRKEY;
	
			memcpy(key->id, sc_pkcs15_print_id(&prkey->id), sizeof(key->id));
	
			memcpy(key->label, objs[i]->label, sizeof(key->label));
			memcpy(key->authid, sc_pkcs15_print_id(&objs[i]->auth_id), sizeof(key->authid));

			key->native = ((struct sc_pkcs15_prkey_info *)objs[i]->data)->native;

			keys[i] = key;

			if(ret_size <= (size_t) i) break;
		}

		out: epsilon::ReleaseP15Card(p15card);	
	}

	return r;
}

int ep_token_read_all_pubkeys(epsilon::TokenContext *ctx, struct ::ep_key_info **keys, size_t ret_size) {
	int r = -1;
	struct sc_pkcs15_object *objs[32];
	struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {
	
		r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_PUBKEY, objs, 32);
		if (r <= 0) {
			fprintf(stderr, "Public key enumeration failed: %s\n", sc_strerror(r));
			goto out;
		}

		for(int i = 0; i < r; ++i) {
			struct sc_pkcs15_pubkey_info *pubkey = (struct sc_pkcs15_pubkey_info *) objs[i]->data;
			struct ::ep_key_info *key = (struct ::ep_key_info *) malloc(sizeof(struct ::ep_key_info));

			  memset(key, 0, sizeof(*key));
			  memset(key->id, 0, sizeof(unsigned char));
			  memset(key->label, 0, sizeof(char));
			  memset(key->authid, 0, sizeof(unsigned char));

   			key->type = EP_KEY_TYPE_PUBKEY;
	
			memcpy(key->id, sc_pkcs15_print_id(&pubkey->id), sizeof(key->id));
	
			memcpy(key->label, objs[i]->label, sizeof(key->label));
			memcpy(key->authid, sc_pkcs15_print_id(&objs[i]->auth_id), sizeof(key->authid));

			key->native = ((struct sc_pkcs15_pubkey_info *)objs[i]->data)->native;

			keys[i] = key;

			if(ret_size <= (size_t) i) break;
		}

		out: epsilon::ReleaseP15Card(p15card);	
	}

	return r;
}

int ep_token_read_all_certs(epsilon::TokenContext *ctx, struct ::ep_key_info **keys, size_t ret_size) {
	int r = -1;
	struct sc_pkcs15_object *objs[32];
	struct sc_pkcs15_card *p15card;

	if ((p15card = epsilon::BindP15Card(ctx)) != NULL) {
	
		r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_CERT_X509, objs, 32);
		if (r < 0) {
			fprintf(stderr, "X509 Certificates enumeration failed: %s\n", sc_strerror(r));
			goto out;
		}

		for(int i = 0; i < r; ++i) {
			struct sc_pkcs15_cert_info *cert = (struct sc_pkcs15_cert_info *) objs[i]->data;
			struct ::ep_key_info *key = (struct ::ep_key_info *) malloc(sizeof(struct ::ep_key_info));

			  memset(key, 0, sizeof(*key));
			  memset(key->id, 0, sizeof(unsigned char));
			  memset(key->label, 0, sizeof(char));
			  memset(key->authid, 0, sizeof(unsigned char));

   			key->type = EP_KEY_TYPE_CERT;
	
			memcpy(key->id, sc_pkcs15_print_id(&cert->id), sizeof(key->id));
	
			memcpy(key->label, objs[i]->label, sizeof(key->label));
			memcpy(key->authid, sc_pkcs15_print_id(&objs[i]->auth_id), sizeof(key->authid));

			key->native = 0;

			keys[i] = key;

			if(ret_size <= (size_t) i) break;
		}

		out: epsilon::ReleaseP15Card(p15card);	
	}

	return r;
}

/* Storing crypto keys */

/*
 * Store a private key
 */
int ep_store_private_key(epsilon::TokenContext *ctx, const char *data, size_t len, 
	const char *label, const char *format, char *passphrase, const char *authData) {
	
	struct sc_pkcs15init_prkeyargs args;
	struct sc_pkcs15_card *p15card;
	char id[4] = {NULL};
	EVP_PKEY	*pkey = NULL;
	X509		*cert[MAX_CERTS];
	int		r, i, ncerts;

	if((p15card = epsilon::BindP15Card(ctx)) != NULL) {

		if((r = ep_verify_pin(p15card, authData)) < 0)
			goto out;

		// Generate 4-digit id here
		while(ep_gen_key_id(p15card, id, SC_PKCS15_TYPE_PRKEY) >= 0) {}

		if ((r = init_keyargs(&args, id, NULL, label)) < 0)
			return r;

		r = ep_read_private_key(data, len, format, &pkey, cert, MAX_CERTS, passphrase);
		if (r < 0)
			return r;
		ncerts = r;

		if (ncerts) {
			char	namebuf[256];

			printf("Importing %d certificates:\n", ncerts);
			for (i = 0; i < ncerts; i++) {
				printf("  %d: %s\n",
					i, X509_NAME_oneline(cert[i]->cert_info->subject,
						namebuf, sizeof(namebuf)));
			}
		}

		if ((r = do_convert_private_key(&args.key, pkey)) < 0)
			return r;
		init_gost_params(&args.params.gost, pkey);

		if (ncerts) {
			unsigned int	usage;

			/* tell openssl to cache the extensions */
			X509_check_purpose(cert[0], -1, -1);
			usage = cert[0]->ex_kusage;

			/* No certificate usage? Assume ordinary
			 * user cert */
			if (usage == 0)
				usage = 0x1F;

			/* If the user requested a specific key usage on the
			 * command line check if it includes _more_
			 * usage bits than the one specified by the cert,
			 * and complain if it does.
			 * If the usage specified on the command line
			 * is more restrictive, use that.
			 */
			/*if (~usage & x509_usage) {
				fprintf(stderr,
					"Warning: requested key usage incompatible with "
					"key usage specified by X.509 certificate\n");
			}*/

			args.x509_usage = /*x509_usage? x509_usage : */usage;
		}

		r = sc_pkcs15init_store_private_key(p15card, ctx->GetProfile(), &args, NULL);

		if (r < 0)
			goto out;

		/* If there are certificate as well (e.g. when reading the
		 * private key from a PKCS #12 file) store them, too.
		 */
		for (i = 0; i < ncerts && r >= 0; i++) {
			struct sc_pkcs15init_certargs cargs;
			char	namebuf[SC_PKCS15_MAX_LABEL_SIZE-1];

			memset(&cargs, 0, sizeof(cargs));

			/* Encode the cert */
			if ((r = do_convert_cert(&cargs.der_encoded, cert[i])) < 0)
				return r;

			X509_check_purpose(cert[i], -1, -1);
			cargs.x509_usage = cert[i]->ex_kusage;
			cargs.label = X509_NAME_oneline(cert[i]->cert_info->subject,
						namebuf, sizeof(namebuf));

			/* Just the first certificate gets the same ID
			 * as the private key. All others get
			 * an ID of their own */
			if (i == 0) {
				cargs.id = args.id;
				/*if (opt_cert_label != 0)
					cargs.label = opt_cert_label;*/
			} else {
				if (is_cacert_already_present(p15card, &cargs)) {
					printf("Certificate #%d already present, "
						"not stored.\n", i);
					goto next_cert;
				}
				cargs.authority = 1;
			}

			r = sc_pkcs15init_store_certificate(p15card, ctx->GetProfile(),
			       		&cargs, NULL);
	next_cert:
			free(cargs.der_encoded.value);
		}
	
		/* No certificates - store the public key */
		if (ncerts == 0) {
			r = do_store_public_key(p15card, ctx->GetProfile(), data, len, format, (char *)sc_pkcs15_print_id(&args.id), label, pkey);
		}

		out: epsilon::ReleaseP15Card(p15card);
	}

	return r;
}

/*
 * Store a public key
 */
static int
do_store_public_key(struct sc_pkcs15_card *p15card, struct sc_profile *profile, 
	const char *data, size_t len, const char *format, char *id, const char *label, EVP_PKEY *pkey) {
	struct sc_pkcs15init_pubkeyargs args;
	sc_pkcs15_object_t *dummy;
	int		r = 0;

	memset(&args, 0, sizeof(args));
	sc_pkcs15_format_id(id, &args.id);
	args.label = label;

	if (pkey == NULL)
		r = ep_read_public_key(data, len, format, &pkey);
	if (r >= 0) {
		r = do_convert_public_key(&args.key, pkey);
		if (r >= 0)
			init_gost_params(&args.params.gost, pkey);
	}
	if (r >= 0)
		r = sc_pkcs15init_store_public_key(p15card, profile, &args, &dummy);

	return r;
}

/*
 * Store a public key
 */
int ep_store_public_key(epsilon::TokenContext *ctx, 
	const char *data, size_t len, const char *format, const char *label, const char *authData) {
	
	struct sc_pkcs15_card *p15card;
	char id[4];
	int		r = 0;

	if((p15card = epsilon::BindP15Card(ctx)) != NULL) {

		if((r = ep_verify_pin(p15card, authData)) < 0)
			goto out;

		// Generate 4-digit id here
		while(ep_gen_key_id(p15card, id, SC_PKCS15_TYPE_PUBKEY) >= 0) {}

		r = do_store_public_key(p15card, ctx->GetProfile(), data, len, format, id, label, NULL);

		out : epsilon::ReleaseP15Card(p15card);
	}

	return r;
}

/*
 * Download certificate to card
 */
int ep_store_certificate(epsilon::TokenContext *ctx,
	const char *data, size_t len, const char *label, const char *format, const char *authData, unsigned int authority) {
	
	struct sc_pkcs15_card *p15card;
	struct sc_pkcs15init_certargs args;
	char id[4];
	X509	*cert = NULL;
	int	r;

	if((p15card = epsilon::BindP15Card(ctx)) != NULL) {

		if((r = ep_verify_pin(p15card, authData)) < 0)
			goto out;

		// Generate 4-digit id here
		while(ep_gen_key_id(p15card, id, SC_PKCS15_TYPE_CERT_X509) >= 0) {}

		memset(&args, 0, sizeof(args));
		sc_pkcs15_format_id(id, &args.id);
		args.label = label;
		args.authority = authority;

		r = ep_read_certificate(data, len, format, &cert);
		if (r >= 0)
			r = do_convert_cert(&args.der_encoded, cert);
		if (r >= 0)
			r = sc_pkcs15init_store_certificate(p15card, ctx->GetProfile(),
						&args, NULL);

		if (args.der_encoded.value)
			free(args.der_encoded.value);

		out : epsilon::ReleaseP15Card(p15card);
	}

	return r;
}

/* Storing utilities */

/* generate an object id */

static int 
ep_gen_key_id(struct sc_pkcs15_card *p15card, char *id, unsigned int type) {
	int r;
	struct sc_pkcs15_object *dummy = NULL;
	struct sc_pkcs15_id objectid;

	memset(&objectid, 0, sizeof(objectid));
	memset(objectid.value, 0, sizeof(objectid.value));

	int random_number =  rand() % 900 + 100;
	_itoa(random_number, id, 10);
	sc_pkcs15_format_id(id, &objectid);

	switch(type) {
		case SC_PKCS15_TYPE_CERT_X509:
			r = sc_pkcs15_find_cert_by_id(p15card, &objectid, &dummy);
			break;
		case SC_PKCS15_TYPE_PUBKEY:
			r = sc_pkcs15_find_pubkey_by_id(p15card, &objectid, &dummy);
			break;
		case SC_PKCS15_TYPE_PRKEY:
			r = sc_pkcs15_find_prkey_by_id(p15card, &objectid, &dummy);
			break;
	}

	return r;
}

/*
 * Check if the CA certificate is already present
 */
static int
is_cacert_already_present(struct sc_pkcs15_card *p15card, struct sc_pkcs15init_certargs *args) {
	sc_pkcs15_object_t	*objs[32];
	sc_pkcs15_cert_info_t	*cinfo;
	sc_pkcs15_cert_t	*cert;
	int			i, count, r;

	r = sc_pkcs15_get_objects(p15card, SC_PKCS15_TYPE_CERT_X509, objs, 32);
	if (r <= 0)
		return 0;

	count = r;
	for (i = 0; i < count; i++) {
		cinfo = (sc_pkcs15_cert_info_t *) objs[i]->data;

		if (!cinfo->authority)
			continue;
		if (args->label && objs[i]->label
		 && strcmp(args->label, objs[i]->label))
			continue;
		/* XXX we should also match the usage field here */

		/* Compare the DER representation of the certificates */
		r = sc_pkcs15_read_certificate(p15card, cinfo, &cert);
		if (r < 0 || !cert)
			continue;

		if (cert->data_len == args->der_encoded.len
			&& !memcmp(cert->data, args->der_encoded.value,
				     cert->data_len)) {
			sc_pkcs15_free_certificate(cert);
			return 1;
		}
		sc_pkcs15_free_certificate(cert);
		cert=NULL;
	}

	return 0;
}

static void
init_gost_params(struct sc_pkcs15init_keyarg_gost_params *params, EVP_PKEY *pkey) {
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && !defined(OPENSSL_NO_EC)
	EC_KEY *key;

	//assert(pkey);
	if (EVP_PKEY_id(pkey) == NID_id_GostR3410_2001) {
		key = (EC_KEY *) EVP_PKEY_get0(pkey);
		//assert(key);
		//assert(params);
		//assert(EC_KEY_get0_group(key));
		//assert(EC_GROUP_get_curve_name(EC_KEY_get0_group(key)) > 0);
		switch (EC_GROUP_get_curve_name(EC_KEY_get0_group(key))) {
		case NID_id_GostR3410_2001_CryptoPro_A_ParamSet:
			params->gostr3410 = SC_PKCS15_PARAMSET_GOSTR3410_A;
			break;
		case NID_id_GostR3410_2001_CryptoPro_B_ParamSet:
			params->gostr3410 = SC_PKCS15_PARAMSET_GOSTR3410_B;
			break;
		case NID_id_GostR3410_2001_CryptoPro_C_ParamSet:
			params->gostr3410 = SC_PKCS15_PARAMSET_GOSTR3410_C;
			break;
		}
	}
#else
	(void)params, (void)pkey; /* no warning */
#endif
}

static int
do_convert_bignum(sc_pkcs15_bignum_t *dst, const BIGNUM *src) {
	if (src == 0)
		return 0;
	dst->len = BN_num_bytes(src);
	dst->data = (u8 *) malloc(dst->len);
	BN_bn2bin(src, dst->data);
	return 1;
}

static int 
do_convert_private_key(struct sc_pkcs15_prkey *key, EVP_PKEY *pk) {
	switch (pk->type) {
	case EVP_PKEY_RSA: {
		struct sc_pkcs15_prkey_rsa *dst = &key->u.rsa;
		RSA *src = EVP_PKEY_get1_RSA(pk);

		key->algorithm = SC_ALGORITHM_RSA;
		if (!do_convert_bignum(&dst->modulus, src->n)
		 || !do_convert_bignum(&dst->exponent, src->e)
		 || !do_convert_bignum(&dst->d, src->d)
		 || !do_convert_bignum(&dst->p, src->p)
		 || !do_convert_bignum(&dst->q, src->q))
			util_fatal("Invalid/incomplete RSA key.\n");
		if (src->iqmp && src->dmp1 && src->dmq1) {
			do_convert_bignum(&dst->iqmp, src->iqmp);
			do_convert_bignum(&dst->dmp1, src->dmp1);
			do_convert_bignum(&dst->dmq1, src->dmq1);
		}
		RSA_free(src);
		break;
		}
	case EVP_PKEY_DSA: {
		struct sc_pkcs15_prkey_dsa *dst = &key->u.dsa;
		DSA *src = EVP_PKEY_get1_DSA(pk);

		key->algorithm = SC_ALGORITHM_DSA;
		do_convert_bignum(&dst->pub, src->pub_key);
		do_convert_bignum(&dst->p, src->p);
		do_convert_bignum(&dst->q, src->q);
		do_convert_bignum(&dst->g, src->g);
		do_convert_bignum(&dst->priv, src->priv_key);
		DSA_free(src);
		break;
		}
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && !defined(OPENSSL_NO_EC)
	case NID_id_GostR3410_2001: {
		struct sc_pkcs15_prkey_gostr3410 *dst = &key->u.gostr3410;
		EC_KEY *src = (EC_KEY *) EVP_PKEY_get0(pk);

		//assert(src);
		key->algorithm = SC_ALGORITHM_GOSTR3410;
		//assert(EC_KEY_get0_private_key(src));
		do_convert_bignum(&dst->d, EC_KEY_get0_private_key(src));
		break;
		}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10000000L && !defined(OPENSSL_NO_EC) */
	default:
		util_fatal("Unsupported key algorithm\n");
	}

	return 0;
}

static int 
do_convert_public_key(struct sc_pkcs15_pubkey *key, EVP_PKEY *pk) {
	switch (pk->type) {
	case EVP_PKEY_RSA: {
		struct sc_pkcs15_pubkey_rsa *dst = &key->u.rsa;
		RSA *src = EVP_PKEY_get1_RSA(pk);

		key->algorithm = SC_ALGORITHM_RSA;
		if (!do_convert_bignum(&dst->modulus, src->n)
		 || !do_convert_bignum(&dst->exponent, src->e))
			util_fatal("Invalid/incomplete RSA key.\n");
		RSA_free(src);
		break;
		}
	case EVP_PKEY_DSA: {
		struct sc_pkcs15_pubkey_dsa *dst = &key->u.dsa;
		DSA *src = EVP_PKEY_get1_DSA(pk);

		key->algorithm = SC_ALGORITHM_DSA;
		do_convert_bignum(&dst->pub, src->pub_key);
		do_convert_bignum(&dst->p, src->p);
		do_convert_bignum(&dst->q, src->q);
		do_convert_bignum(&dst->g, src->g);
		DSA_free(src);
		break;
		}
#if OPENSSL_VERSION_NUMBER >= 0x10000000L && !defined(OPENSSL_NO_EC)
	case NID_id_GostR3410_2001: {
		struct sc_pkcs15_pubkey_gostr3410 *dst = &key->u.gostr3410;
		EC_KEY *eckey = (EC_KEY *) EVP_PKEY_get0(pk);
		const EC_POINT *point;
		BIGNUM *X, *Y;
		int r = 0;

		//assert(eckey);
		point = EC_KEY_get0_public_key(eckey);
		if (!point)
			return SC_ERROR_INTERNAL;
		X = BN_new();
		Y = BN_new();
		if (X && Y && EC_KEY_get0_group(eckey))
			r = EC_POINT_get_affine_coordinates_GFp(EC_KEY_get0_group(eckey),
					point, X, Y, NULL);
		if (r == 1) {
			dst->xy.len = BN_num_bytes(X) + BN_num_bytes(Y);
			dst->xy.data = (u8 *) malloc(dst->xy.len);
			if (dst->xy.data) {
				BN_bn2bin(Y, dst->xy.data);
				BN_bn2bin(X, dst->xy.data + BN_num_bytes(Y));
				r = sc_mem_reverse(dst->xy.data, dst->xy.len);
				if (!r)
					r = 1;
				key->algorithm = SC_ALGORITHM_GOSTR3410;
			}
			else
				r = -1;
		}
		BN_free(X);
		BN_free(Y);
		if (r != 1)
			return SC_ERROR_INTERNAL;
		break;
		}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10000000L && !defined(OPENSSL_NO_EC) */
	default:
		util_fatal("Unsupported key algorithm\n");
	}

	return 0;
}

static int 
do_convert_cert(sc_pkcs15_der_t *der, X509 *cert) {
	u8	*p;

	der->len = i2d_X509(cert, NULL);
	der->value = p = (u8 *) malloc(der->len);
	i2d_X509(cert, &p);
	return 0;
}

}
