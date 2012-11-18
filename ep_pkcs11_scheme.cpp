// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "ep_pkcs11_scheme.h"

#include <algorithm>
#include <string>
#include "include/cef_browser.h"
#include "include/cef_callback.h"
#include "include/cef_frame.h"
#include "include/cef_resource_handler.h"
#include "include/cef_response.h"
#include "include/cef_request.h"
#include "include/cef_scheme.h"
#include "resource_util.h"
#include "string_util.h"
#include "util.h"

#include "include/wrapper/cef_stream_resource_handler.h"

#if defined(OS_WIN)
#include "resource.h"
#endif

namespace epsilon {

namespace {

// Implementation of the factory for for creating schema handlers.
class PKCS11SchemeHandlerFactory : public CefSchemeHandlerFactory {
 public:
  // Return a new scheme handler instance to handle the request.
  virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
                                               CefRefPtr<CefFrame> frame,
                                               const CefString& scheme_name,
                                               CefRefPtr<CefRequest> request)
                                               OVERRIDE {
    REQUIRE_IO_THREAD();

	std::string url = request->GetURL();
	if (url == "pkcs11://epsilon.ma/" || url == "pkcs11://epsilon.ma/#" || url == "pkcs11://epsilon.ma/#/" || strstr(url.c_str(), "index.html") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_INDEX);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/html", stream);

	} 
	
    if (strstr(url.c_str(), "application.js") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_APPLICATION_JS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/javascript", stream);
    }

    if (strstr(url.c_str(), "bootstrap.min.js") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_BOOTSTRAP_JS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/javascript", stream);
    }

    if (strstr(url.c_str(), "jquery-ui-1.8.23.custom.min.js") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_JQUERY_UI_JS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/javascript", stream);
    }

    if (strstr(url.c_str(), "application.css") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_APPLICATION_CSS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/css", stream);
    }

    if (strstr(url.c_str(), "bootstrap.min.css") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_BOOTSTRAP_CSS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/css", stream);
    }

    if (strstr(url.c_str(), "font-awesome.css") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_FONTAWESOME_CSS);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/css", stream);
    }

    if (strstr(url.c_str(), "fontawesome-webfont.woff") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_FONTAWESOME_WOFF);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("application/x-font-woff", stream);
    }

    if (strstr(url.c_str(), "fontawesome-webfont.ttf") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_FONTAWESOME_TTF);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("application/x-font-truetype", stream);
    }

	if (strstr(url.c_str(), "fontawesome-webfont.svg#FontAwesome") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader(IDS_FONTAWESOME_SVG);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("image/svg+xml", stream);
    }

    if (strstr(url.c_str(), "loader.gif") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_LOADER);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("image/gif", stream);
    }

    if (strstr(url.c_str(), "loading.gif") != NULL) {
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_LOADING);
      ASSERT(stream.get());
      return new CefStreamResourceHandler("image/gif", stream);
    }

	else if (strstr(url.c_str(), "logo.png") != NULL) {
      // Load the response image
#if defined(OS_WIN)
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDC_USBTOKEN);
      ASSERT(stream.get());      

	  return new CefStreamResourceHandler("image/jpg", stream);
      //DWORD dwSize;
      //LPBYTE pBytes;
      //if (LoadBinaryResource(IDC_USBTOKEN, dwSize, pBytes)) {
      //  data_ = std::string(reinterpret_cast<const char*>(pBytes), dwSize);
      //  handled = true;
      //  // Set the resulting mime type
      //  mime_type_ = "image/jpg";
      //}
#elif defined(OS_MACOSX) || defined(OS_LINUX)
      // Show the binding contents
      CefRefPtr<CefStreamReader> stream =
		  GetBinaryResourceReader(IDS_LOGO);
      ASSERT(stream.get());      

	  return new CefStreamResourceHandler("image/png", stream);
      //if (LoadBinaryResource("logo.png", data_)) {
      //  handled = true;
      //  // Set the resulting mime type
      //  mime_type_ = "image/png";
      //}
#else
#error "Unsupported platform"
#endif
    }

    return NULL;
  }

  IMPLEMENT_REFCOUNTING(PKCS11SchemeHandlerFactory);
};

}  // namespace

namespace schemes {

void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar,
                           std::vector<CefString>& cookiable_schemes) {
  registrar->AddCustomScheme("pkcs11", true, false, false);
}

void InitPKCS11Scheme() {
  CefRegisterSchemeHandlerFactory("pkcs11", "epsilon.ma", new PKCS11SchemeHandlerFactory);
}

}


}  // namespace scheme_test
