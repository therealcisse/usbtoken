// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_USBTOKEN_H___X
#define CEF_USBTOKEN_H___X
#pragma once

#include <string>
#include "include/cef_base.h"
#include "client_app.h"


class CefApp;
class CefBrowser;
class CefCommandLine;

// Returns the main browser window instance.
CefRefPtr<CefBrowser> AppGetBrowser();

// Returns the main application window handle.
CefWindowHandle AppGetMainHwnd();

// Returns the application working directory.
std::string AppGetWorkingDirectory();

// Initialize the application command line.
void AppInitCommandLine(int argc, const char* const* argv);

// Returns the application command line object.
CefRefPtr<CefCommandLine> AppGetCommandLine();

// Returns the application settings based on command line arguments.
void AppGetSettings(CefSettings& settings, CefRefPtr<ClientApp> app);

// Returns true if off-screen rendering is enabled via the command line
// argument.
bool AppIsOffScreenRenderingEnabled();

#endif  // CEF_USBTOKEN_H___
