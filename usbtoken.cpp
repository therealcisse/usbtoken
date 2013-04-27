// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "usbtoken.h"

#include <stdio.h>
#include <cstdlib>

#include <sstream>
#include <string>

#include "include/cef_app.h"
#include "include/cef_command_line.h"

#include "client_handler.h"
#include "client_app.h"
#include "client_switches.h"
#include "util.h"

namespace {

// Return the int representation of the specified string.
int GetIntValue(const CefString& str) {
  if (str.empty())
    return 0;

  std::string stdStr = str;
  return atoi(stdStr.c_str());
}

}  // namespace

CefRefPtr<ClientHandler> g_handler;
CefRefPtr<CefCommandLine> g_command_line;

CefRefPtr<CefBrowser> AppGetBrowser() {
  if (!g_handler.get())
    return NULL;
  return g_handler->GetBrowser();
}

CefWindowHandle AppGetMainHwnd() {
  if (!g_handler.get())
    return NULL;
  return g_handler->GetMainHwnd();
}

void AppInitCommandLine(int argc, const char* const* argv) {
  g_command_line = CefCommandLine::CreateCommandLine();
#if defined(OS_WIN)
  g_command_line->InitFromString(::GetCommandLineW());
#else
  g_command_line->InitFromArgv(argc, argv);
#endif
}

// Returns the application command line object.
CefRefPtr<CefCommandLine> AppGetCommandLine() {
  return g_command_line;
}

// Returns the application settings based on command line arguments.
void AppGetSettings(CefSettings& settings, CefRefPtr<ClientApp> app) {
  ASSERT(app.get());
  ASSERT(g_command_line.get());
  if (!g_command_line.get())
    return;

  CefString str;

#if defined(OS_WIN)
  settings.multi_threaded_message_loop =
      g_command_line->HasSwitch(usbtoken::kMultiThreadedMessageLoop);
#endif

  CefString(&settings.cache_path) =
      g_command_line->GetSwitchValue(usbtoken::kCachePath);

  // Retrieve command-line proxy configuration, if any.
//  bool has_proxy = false;
//  cef_proxy_type_t proxy_type = CEF_PROXY_TYPE_DIRECT;
//  CefString proxy_config;

//  if (g_command_line->HasSwitch(usbtoken::kProxyType)) {
//    std::string str = g_command_line->GetSwitchValue(usbtoken::kProxyType);
//    if (str == usbtoken::kProxyType_Direct) {
//      has_proxy = true;
//      proxy_type = CEF_PROXY_TYPE_DIRECT;
//    } else if (str == usbtoken::kProxyType_Named ||
//               str == usbtoken::kProxyType_Pac) {
//      proxy_config = g_command_line->GetSwitchValue(usbtoken::kProxyConfig);
//      if (!proxy_config.empty()) {
//        has_proxy = true;
//        proxy_type = (str == usbtoken::kProxyType_Named?
//                      CEF_PROXY_TYPE_NAMED:CEF_PROXY_TYPE_PAC_STRING);
//      }
//    }
//  }

//  if (has_proxy) {
    // Provide a ClientApp instance to handle proxy resolution.
//    app->SetProxyConfig(proxy_type, proxy_config);
//  }
}

bool AppIsOffScreenRenderingEnabled() {
  ASSERT(g_command_line.get());
  if (!g_command_line.get())
    return false;

  return g_command_line->HasSwitch(usbtoken::kOffScreenRenderingEnabled);
}

// Returns the application browser settings based on command line arguments.
// void AppGetBrowserSettings(CefBrowserSettings& settings) {
//   ASSERT(g_command_line.get());
//   if (!g_command_line.get())
//     return;

//   settings.remote_fonts =
//       g_command_line->HasSwitch(usbtoken::kRemoteFontsDisabled) ? STATE_DISABLED : STATE_ENABLED;

//   CefString(&settings.default_encoding) =
//       g_command_line->GetSwitchValue(usbtoken::kDefaultEncoding);

//   settings.encoding_detector =
//       g_command_line->HasSwitch(usbtoken::kEncodingDetectorEnabled) ? STATE_ENABLED : STATE_DISABLED;
//   settings.javascript =
//       g_command_line->HasSwitch(usbtoken::kJavascriptDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.javascript_open_windows =
//       g_command_line->HasSwitch(usbtoken::kJavascriptOpenWindowsDisallowed) ? STATE_DISABLED : STATE_ENABLED;
//   settings.javascript_close_windows =
//       g_command_line->HasSwitch(usbtoken::kJavascriptCloseWindowsDisallowed) ? STATE_DISABLED : STATE_ENABLED;
//   settings.javascript_access_clipboard =
//       g_command_line->HasSwitch(
//           usbtoken::kJavascriptAccessClipboardDisallowed) ? STATE_DISABLED : STATE_ENABLED;
//   settings.dom_paste =
//       g_command_line->HasSwitch(usbtoken::kDomPasteDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.caret_browsing =
//       g_command_line->HasSwitch(usbtoken::kCaretBrowsingDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.java =
//       g_command_line->HasSwitch(usbtoken::kJavaDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.plugins =
//       g_command_line->HasSwitch(usbtoken::kPluginsDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.universal_access_from_file_urls =
//       g_command_line->HasSwitch(usbtoken::kUniversalAccessFromFileUrlsAllowed) ? STATE_ENABLED : STATE_DISABLED;
//   settings.file_access_from_file_urls =
//       g_command_line->HasSwitch(usbtoken::kFileAccessFromFileUrlsAllowed) ? STATE_ENABLED : STATE_DISABLED;
//   settings.web_security =
//       g_command_line->HasSwitch(usbtoken::kWebSecurityDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.xss_auditor =
//       g_command_line->HasSwitch(usbtoken::kXssAuditorEnabled) ? STATE_ENABLED : STATE_DISABLED;
//   settings.image_load =
//       g_command_line->HasSwitch(usbtoken::kImageLoadingDisabled) ? STATE_DISABLED : STATE_ENABLED;
//   settings.shrink_standalone_images_to_fit =
//       g_command_line->HasSwitch(usbtoken::kShrinkStandaloneImagesToFit);
//   settings.site_specific_quirks_disabled =
//       g_command_line->HasSwitch(usbtoken::kSiteSpecificQuirksDisabled);
//   settings.text_area_resize_disabled =
//       g_command_line->HasSwitch(usbtoken::kTextAreaResizeDisabled);
//   settings.page_cache_disabled =
//       g_command_line->HasSwitch(usbtoken::kPageCacheDisabled);
//   settings.tab_to_links_disabled =
//       g_command_line->HasSwitch(usbtoken::kTabToLinksDisabled);
//   settings.hyperlink_auditing_disabled =
//       g_command_line->HasSwitch(usbtoken::kHyperlinkAuditingDisabled);
//   settings.user_style_sheet_enabled =
//       g_command_line->HasSwitch(usbtoken::kUserStyleSheetEnabled);

//   CefString(&settings.user_style_sheet_location) =
//       g_command_line->GetSwitchValue(usbtoken::kUserStyleSheetLocation);

//   settings.author_and_user_styles_disabled =
//       g_command_line->HasSwitch(usbtoken::kAuthorAndUserStylesDisabled);
//   settings.local_storage_disabled =
//       g_command_line->HasSwitch(usbtoken::kLocalStorageDisabled);
//   settings.databases_disabled =
//       g_command_line->HasSwitch(usbtoken::kDatabasesDisabled);
//   settings.application_cache_disabled =
//       g_command_line->HasSwitch(usbtoken::kApplicationCacheDisabled);
//   settings.webgl_disabled =
//       g_command_line->HasSwitch(usbtoken::kWebglDisabled);
//   settings.accelerated_compositing_disabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedCompositingDisabled);
//   settings.accelerated_layers_disabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedLayersDisabled);
//   settings.accelerated_video_disabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedVideoDisabled);
//   settings.accelerated_2d_canvas_disabled =
//       g_command_line->HasSwitch(usbtoken::kAcceledated2dCanvasDisabled);
//   settings.accelerated_painting_enabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedPaintingEnabled);
//   settings.accelerated_filters_enabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedFiltersEnabled);
//   settings.accelerated_plugins_disabled =
//       g_command_line->HasSwitch(usbtoken::kAcceleratedPluginsDisabled);
//   settings.developer_tools_disabled =
//       g_command_line->HasSwitch(usbtoken::kDeveloperToolsDisabled);
//   settings.fullscreen_enabled =
//       g_command_line->HasSwitch(usbtoken::kFullscreenEnabled);
// }
