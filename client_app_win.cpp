/*
 * Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * 
 */ 

#include "client_app.h"
#include "resource.h"
#include "include/cef_base.h"
#include "config.h"

#include <algorithm>
#include <MMSystem.h>
#include <ShlObj.h>
#include <string>

extern DWORD g_appStartupTime;

std::string ClientApp::GetExtensionJSSource() {
  extern HINSTANCE g_hInst;

  HRSRC hRes = FindResource(g_hInst, MAKEINTRESOURCE(IDS_USBTOKEN_EXTENSIONS), MAKEINTRESOURCE(BINARY));
  DWORD dwSize;
  LPBYTE pBytes = NULL;

  if(hRes) {
    HGLOBAL hGlob = LoadResource(g_hInst, hRes);
    if(hGlob) {
        dwSize = SizeofResource(g_hInst, hRes);
        pBytes = (LPBYTE)LockResource(hGlob);
    }
  }

  if (pBytes) {
    std::string res((const char *)pBytes, dwSize);
    return res;
  }

  return "";
}

double ClientApp::GetElapsedMilliseconds() {
    return (timeGetTime() - g_appStartupTime);
}