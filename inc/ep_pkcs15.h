

#ifndef EP_EP_PKCS_15_H_
#define EP_EP_PKCS_15_H_

#include <string>

#include "stdlib.h"
#include "time.h"

#include "libopensc/pkcs15.h"
#include "pkcs15init/profile.h"

#include "include/cef_base.h"

#include "inc/pkcs15.h"

#include "inc/ep_thread_ctx.h"

#define DEFAULT_PROFILE_NAME "pkcs15+onepin"

namespace epsilon {

class TokenContext: public CefBase {
public:

  ~TokenContext() { ep_free_lock(); Destroy(); }
  
  static CefRefPtr<TokenContext> 
  GetGlobalContext (void) {
    static CefCriticalSection cs;

    class Guard {
     public:
      explicit Guard(CefCriticalSection *base) : base_(base) { base_->Lock(); }
      ~Guard() { base_->Unlock(); }
     private:
      CefCriticalSection* base_;
    };
    
    // First check
    if (instance_ == NULL) {
      // Ensure serialization (guard constructor acquires lock_).
      Guard guard_(&cs);

      // Double check.
      if (instance_ == NULL)
        instance_ = new TokenContext;
    }

    return instance_;
  }

  bool Bind(const char * = NULL);
  void Destroy();

  sc_profile *GetProfile() const;
  sc_card *GetCard() const;
  sc_context *GetSCContext() const;
  std::string GetProfileName();

  bool InContext() const;

  static bool isTokenPresent(sc_context **);

private:
  static CefRefPtr<TokenContext> instance_;

  TokenContext(const TokenContext&);
  void operator=(const TokenContext&);

  explicit TokenContext() : kProfileName(DEFAULT_PROFILE_NAME), in_context(false) { srand(time(NULL)); ep_init_lock(); }
  
  const std::string kProfileName;

  volatile bool in_context;

  sc_context *ctx;

  sc_profile *profile;

  sc_card *card;

  IMPLEMENT_REFCOUNTING(TokenContext);    
  //IMPLEMENT_LOCKING(TokenContext); 
};

sc_pkcs15_card *CreateP15Card(CefRefPtr<TokenContext>);
void DestroyP15Card(sc_pkcs15_card*);

sc_pkcs15_card *BindP15Card(CefRefPtr<TokenContext>);
void ReleaseP15Card(sc_pkcs15_card*); 

void GetTokenInfo(CefRefPtr<TokenContext>, ep_token_info*);

}

#endif // EP_PKCS_15_H_