// Copyright (c) 2008-2009 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "resource_util.h"
#include "include/cef_stream.h"
#include "include/wrapper/cef_byte_read_handler.h"
#include "util.h"

#include "resource.h"

#if defined(OS_WIN)

bool LoadBinaryResource(int binaryId, DWORD &dwSize, LPBYTE &pBytes) {
	extern HINSTANCE g_hInst;

 //   WCHAR shortpath[MAX_PATH] = {0}, 
	//	  longpath[MAX_PATH] = {0};

	//HMODULE modhandle = GetModuleHandle(NULL);
	//GetModuleFileName(modhandle, longpath, MAX_PATH);
	//GetShortPathName(longpath, shortpath, MAX_PATH);
 //
	//HMODULE hLibrary = LoadLibrary(shortpath); 
	//if (hLibrary == NULL){
	//	printf("Error loading hlibrary\n");
	//	return false;
	//}

	//printf("longpath: %s\n", longpath);
	//printf("shortpath: %s\n", shortpath);
 //
	//if(IS_INTRESOURCE(binaryId) == FALSE){
	//	printf("%d is not a resource identifier...\n", binaryId);
	//	return false;
	//}

  HRSRC hRes = FindResource(g_hInst, MAKEINTRESOURCE(binaryId),
                            MAKEINTRESOURCE(256));

  DWORD err = GetLastError();

  if (hRes) {
	  HGLOBAL hGlob = LoadResource(g_hInst, hRes);
    if (hGlob) {
      dwSize = SizeofResource(g_hInst, hRes);
      pBytes = (LPBYTE)LockResource(hGlob);
      if (dwSize > 0 && pBytes)
        return true;
    }
  }

  return false;
}

CefRefPtr<CefStreamReader> GetBinaryResourceReader(int binaryId) {
  DWORD dwSize;
  LPBYTE pBytes;

  if (LoadBinaryResource(binaryId, dwSize, pBytes)) {
    return CefStreamReader::CreateForHandler(
        new CefByteReadHandler(pBytes, dwSize, NULL));
  }

  ASSERT(FALSE);  // The resource should be found.
  return NULL;
}

CefRefPtr<CefStreamReader> GetBinaryResourceReader(const char* resource_name) {
  // Map of resource labels to BINARY id values.
  static struct _resource_map {
    char* name;
    int id;
  } resource_map[] = {
    {"index.html", IDS_INDEX},
    
	{"application.js", IDS_APPLICATION_JS},
	{"bootstrap.min.js", IDS_BOOTSTRAP_JS},
	{"jquery-ui-1.8.23.custom.min.js", IDS_JQUERY_UI_JS},
    
	{"application.css", IDS_APPLICATION_CSS},
	{"bootstrap.min.css", IDS_BOOTSTRAP_CSS},
	{"font-awesome.css", IDS_FONTAWESOME_CSS},
    
	{"loader.gif", IDS_LOADER},
	{"loading.gif", IDS_LOADING},
	  {"", 0x00}
  };

  for (int i = 0; i < sizeof(resource_map)/sizeof(_resource_map); ++i) {
    if (!strcmp(resource_map[i].name, resource_name))
      return GetBinaryResourceReader(resource_map[i].id);
  }

  ASSERT(FALSE);  // The resource should be found.
  return NULL;
}

#endif  // OS_WIN
