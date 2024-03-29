// Copyright (c) 2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_SCHEME_TEST_H_
#define CEF_TESTS_CEFCLIENT_SCHEME_TEST_H_
#pragma once

#include <vector>
#include "include/cef_base.h"

class CefBrowser;
class CefSchemeRegistrar;

namespace epsilon {

	namespace schemes {
// Register the scheme.
void RegisterCustomSchemes(CefRefPtr<CefSchemeRegistrar> registrar,
                           std::vector<CefString>& cookiable_schemes);

// Create the scheme handler.
void InitPKCS11Scheme();
	}

}  // namespace epsilon

#endif  // CEF_TESTS_CEFCLIENT_SCHEME_TEST_H_
