

#ifndef EP_EP_PKCS_15_H_
#define EP_EP_PKCS_15_H_

#include <string>

#include "libopensc/pkcs15.h"
#include "pkcs15init/profile.h"

#include "include/cef_base.h"

#include "inc/pkcs15.h"

namespace epsilon {

class TokenContext: public CefBase {
public:

	~TokenContext() { if(in_context) Destroy(); }
	
	/*Use threads? */
	static CefRefPtr<TokenContext> GetGlobalContext() {	
		return CefRefPtr<TokenContext>(new TokenContext("pkcs15+onepin"));
	}

	bool Bind();
	void Destroy();

	sc_profile *GetProfile() const;
	sc_card *GetCard() const;
	std::string GetProfileName() const;

	void GetInfo(ep_token_info*) const;

	bool InContext() const;

private:

	TokenContext(const TokenContext&);
	void operator=(const TokenContext&);

	explicit TokenContext(char *kProfileName) : kProfileName(kProfileName), in_context(false) {}
	
	std::string kProfileName;

	bool in_context;

	sc_context *ctx;

	sc_profile *profile;

	sc_card *card;

	IMPLEMENT_REFCOUNTING(TokenContext);		
};

sc_pkcs15_card *CreateP15Card(const TokenContext *const);
void DestroyP15Card(sc_pkcs15_card*);

sc_pkcs15_card *BindP15Card(const TokenContext *const);
void ReleaseP15Card(sc_pkcs15_card*);	

}

#endif // EP_PKCS_15_H_