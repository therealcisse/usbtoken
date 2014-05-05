/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
* 
* Provides a sample service class that derives from the service base class - 
* CServiceBase. The sample service logs the service start and stop 
* information to the Application event log, and shows how to run the main 
* function of the service in a thread pool worker thread.
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "usbtoken.h"

#include <windows.h>
#include <process.h>

#include <commdlg.h>
#include <shellapi.h>
#include <commctrl.h>
#include <strsafe.h>
#include <direct.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <winbase.h>
#include <windowsx.h>
#include <sstream>
#include <string>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"

#include "client_app.h"
#include "client_handler.h"
#include "string_util.h"

#include "inc/watcher.h"

#include "ep_pkcs11_scheme.h"

#pragma endregion

#define PREF_EPSILON_BASE L"Epsilon SARL"
#define APP_NAME L"Epsilon Token Manager"

#define PACKVERSION(major, minor) MAKELONG(minor, major)

#define MAX_LOADSTRING 100

// Global Variables
DWORD g_appStartupTime;
HINSTANCE g_hInst;                // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];      // the main window class name
char szWorkingDir[MAX_PATH];  // The current working directory

#if NTDDI_VERSION >= NTDDI_VISTA

SC_HANDLE g_scm;
SC_HANDLE g_service;

#endif

HANDLE m_SvcWatcherHnd;

// The global ClientHandler reference.
extern CefRefPtr<ClientHandler> g_handler;

//// Registry access functions
//
//// Registry key strings
//#define PREF_APPSHELL_BASE    L"Software"
//#define PREF_WINPOS_FOLDER    L"Window Position"
//#define PREF_LEFT       L"Left"
//#define PREF_TOP        L"Top"
//#define PREF_WIDTH        L"Width"
//#define PREF_HEIGHT       L"Height"
//#define PREF_RESTORE_LEFT   L"Restore Left"
//#define PREF_RESTORE_TOP    L"Restore Top"
//#define PREF_RESTORE_RIGHT    L"Restore Right"
//#define PREF_RESTORE_BOTTOM   L"Restore Bottom"
//#define PREF_SHOWSTATE      L"Show State"
//
//// Window state functions
//void SaveWindowRect(HWND hWnd);
//void RestoreWindowRect(int& left, int& top, int& width, int& height, int& showCmd);
//void RestoreWindowPlacement(HWND hWnd, int showCmd);// Registry access functions
void EnsureTrailingSeparator(LPWSTR pRet);
void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet);
bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret);
bool WriteRegistryInt (LPCWSTR pFolder, LPCWSTR pEntry, int val);
//
//// Registry key strings
//#define PREF_APPSHELL_BASE    L"Software"
//#define PREF_WINPOS_FOLDER    L"Window Position"
//#define PREF_LEFT       L"Left"
//#define PREF_TOP        L"Top"
//#define PREF_WIDTH        L"Width"
//#define PREF_HEIGHT       L"Height"
//#define PREF_RESTORE_LEFT   L"Restore Left"
//#define PREF_RESTORE_TOP    L"Restore Top"
//#define PREF_RESTORE_RIGHT    L"Restore Right"
//#define PREF_RESTORE_BOTTOM   L"Restore Bottom"
//#define PREF_SHOWSTATE      L"Show State"
//
//// Window state functions
//void SaveWindowRect(HWND hWnd);
//void RestoreWindowRect(int& left, int& top, int& width, int& height, int& showCmd);
//void RestoreWindowPlacement(HWND hWnd, int showCmd);

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const IDM_SERVICE_STOPPED  = WM_APP + 2;
UINT const NOTIFICATION_ICON_ID = WM_APP + 3;

UINT_PTR const HIDEBROWSER_TIMER_ID = 1;

#if NTDDI_VERSION > NTDDI_VISTA  
// Use a guid to uniquely identify our icon
class __declspec(uuid("{3886E88A-87F9-4623-BCBD-AEB3410F1028}")) NotificationIcon;

#endif

// Forward declarations of functions included in this code module:
ATOM        MyRegisterClass(HINSTANCE hInstance);
BOOL        InitInstance(HINSTANCE, int);
LRESULT CALLBACK  WndProc(HWND, UINT, WPARAM, LPARAM);

void                ShowContextMenu(int, HWND, POINT);
BOOL                AddNotificationIcon(HWND const &);
// BOOL                ShowStartNotification();
BOOL                DeleteNotificationIcon(HWND const &);
// BOOL                ShowTokenRemovedBalloon();
// BOOL                ShowPINBlockedBalloon();
// BOOL                ShowTokenInsertedBalloon();
// BOOL                RestoreTooltip();

BOOL DoStopSvc(SC_HANDLE);

// Formats a message string using the specified message and variable
// list of arguments.
LPWSTR GetLastErrorMessage();

#if NTDDI_VERSION >= NTDDI_VISTA

void CALLBACK ServiceNotifyCallback(void*);

#endif

#if NTDDI_VERSION < NTDDI_VISTA

HANDLE CreatePipe();

#endif

unsigned __stdcall WatchSvc(void*);

// BOOL MyPostQuitMessage(UINT message = 0x0);

void HideWnd(HWND);
void ShowWnd(HWND);

#if defined(OS_WIN)
// Add Common Controls to the application manifest because it's required to
// support the default tooltip implementation.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")  // NOLINT(whitespace/line_length)
#pragma comment(lib, "comctl32.lib")

#endif

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  
  CefMainArgs main_args(hInstance);
  CefRefPtr<ClientApp> app(new ClientApp);

  // Execute the secondary process, if any.
  int exit_code = CefExecuteProcess(main_args, app.get());
  if (exit_code >= 0)
    return exit_code;

  // Retrieve the current working directory.
  if (_getcwd(szWorkingDir, MAX_PATH) == NULL)
    szWorkingDir[0] = 0;

  // Parse command line arguments. The passed in values are ignored on Windows.
  AppInitCommandLine(0, NULL);

  CefSettings settings;

  // Populate the settings based on command line arguments.
  AppGetSettings(settings, app);

  settings.multi_threaded_message_loop = true;

  // Initialize CEF.
  CefInitialize(main_args, settings, app.get());

  // Register the scheme handler.
  epsilon::schemes::InitPKCS11Scheme();  

  HACCEL hAccelTable;

  // Initialize global strings
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadString(hInstance, IDC_USBTOKEN, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization
  if (!InitInstance(hInstance, SW_SHOWDEFAULT))
    return FALSE;

  hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_USBTOKEN));

  int result = 0;

  if (!settings.multi_threaded_message_loop) {
    // Run the CEF message loop. This function will block until the application
    // recieves a WM_QUIT message.
    CefRunMessageLoop();
  } else {
    MSG msg;

    // Run the application message loop.
    while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    result = static_cast<int>(msg.wParam);
  }

  // Shut down CEF.
  CefShutdown();

  return result;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  // UNREFERENCED_PARAMETER(nCmdShow);

  HWND hWnd;

  g_hInst = hInstance; // Store instance handle in our global variable

  RECT lpRect = {600, 100, 425, 415};
  AdjustWindowRect(&lpRect, (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) & ~WS_MAXIMIZEBOX, FALSE);

   hWnd = CreateWindow(szWindowClass, szTitle,
       (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN) & ~WS_MAXIMIZEBOX,
       lpRect.left, lpRect.top, lpRect.right, lpRect.bottom,
       NULL, NULL, g_hInst, NULL);
        
  //DWORD const dwStyle = WS_POPUP | WS_THICKFRAME | WS_OVERLAPPEDWINDOW | WS_MAXIMIZEBOX | WS_SYSMENU;
  //// adjust the window size to take the frame into account
  //AdjustWindowRectEx(&lpRect, dwStyle, FALSE, WS_EX_TOOLWINDOW);

  //hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szWindowClass, szTitle, dwStyle,
  //    lpRect.left, lpRect.top, lpRect.right, lpRect.bottom, NULL, NULL, g_hInst, NULL);        

  if(!hWnd)
    return FALSE;

  //ShowWindow(hWnd, nCmdShow);
  //UpdateWindow(hWnd);  

  return TRUE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style      = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc  = WndProc;
  wcex.cbClsExtra   = 0;
  wcex.cbWndExtra   = 0;
  wcex.hInstance    = hInstance;
  
  wcex.hIcon      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_USBTOKEN));
  wcex.hCursor    = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName = NULL; //MAKEINTRESOURCE(IDC_USBTOKEN);
  wcex.lpszClassName  = szWindowClass;
  wcex.hIconSm    = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassEx(&wcex);
}

DWORD GetAssemblyVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    static DWORD dwVersion = 0;

    if(dwVersion != 0)
      return dwVersion; //been here

    /* For security purposes, LoadLibrary should be provided with a fully qualified 
       path to the DLL. The lpszDllName variable should be tested to ensure that it 
       is a fully qualified path before it is used. */
    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

        /* Because some DLLs might not implement this function, you must test for 
           it explicitly. Depending on the particular DLL, the lack of a DllGetVersion 
           function can be a useful indicator of the version. */

        if(pDllGetVersion)
        {
            DLLVERSIONINFO2 dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.info1.cbSize = sizeof(dvi);

			hr = (*pDllGetVersion)(&dvi.info1);

            if(SUCCEEDED(hr))
               dwVersion = PACKVERSION(dvi.info1.dwMajorVersion, dvi.info1.dwMinorVersion);
        }
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

BOOL AddNotificationIcon(HWND const &hHnd)
{
    NOTIFYICONDATA nid = {0};
	nid.hWnd = hHnd;

	DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

	if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
        nid.cbSize = sizeof (NOTIFYICONDATA);
    
#if NTDDI_VERSION >= NTDDI_VISTA  
  else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
        nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);    
#endif
    
	else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
	else 
        nid.cbSize = NOTIFYICONDATA_V1_SIZE;

#if NTDDI_VERSION > NTDDI_VISTA  
  nid.guidItem = __uuidof(NotificationIcon);  
#else
	nid.uID = NOTIFICATION_ICON_ID;
#endif

    // add the icon, setting the icon, tooltip, and callback message.
    // the icon will be identified with the GUID
#if NTDDI_VERSION >= NTDDI_VISTA  
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_SHOWTIP | NIF_TIP;
#else
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
#endif  

    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
  	nid.dwState = 0;
  	nid.dwStateMask = NIS_HIDDEN;

#if NTDDI_VERSION >= NTDDI_VISTA  
    LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_SMALL), LIM_SMALL, &nid.hIcon);
#else
    nid.hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
#endif  
        
    LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
    Shell_NotifyIcon(NIM_ADD, &nid);

    // NOTIFYICON_VERSION_4 is prefered    
#if NTDDI_VERSION >= NTDDI_VISTA  
    nid.uVersion = NOTIFYICON_VERSION_4;
#else
    nid.uVersion = NOTIFYICON_VERSION;
#endif      

    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void ShowWnd(HWND hwnd)
{

  SetForegroundWindow(hwnd);
  ShowWindow(hwnd, SW_SHOW);
  //UpdateWindow(hwnd);
}

void HideWnd(HWND hwnd)
{
    ShowWindow(hwnd, SW_HIDE);

    // immediately after hiding the flyout we don't want to allow showing it again, which will allow clicking
    // on the icon to hide the flyout. If we didn't have this code, clicking on the icon when the flyout is open
    // would cause the focus change (from flyout to the taskbar), which would trigger hiding the flyout
    // (see the WM_ACTIVATE handler). Since the flyout would then be hidden on click, it would be shown again instead
    // of hiding.
    SetTimer(hwnd, HIDEBROWSER_TIMER_ID, GetDoubleClickTime(), NULL);
}

void ShowContextMenu(int type, HWND hwnd, POINT pt)
{
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(type));
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }
        DestroyMenu(hMenu);
    }
}

#if NTDDI_VERSION < NTDDI_VISTA

HANDLE CreatePipe()
{
	union maxSize
	  {
		  UINT _1;
	  };
  
  return ::CreateNamedPipe(L"\\\\.\\pipe\\epTokend",
                    PIPE_ACCESS_DUPLEX, 
                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
                    PIPE_UNLIMITED_INSTANCES, sizeof maxSize, 
                    sizeof maxSize, NMPWAIT_USE_DEFAULT_WAIT, NULL);
}

#endif

#if NTDDI_VERSION >= NTDDI_VISTA

void CALLBACK ServiceNotifyCallback(void* param)
{
  SERVICE_NOTIFY* sn = static_cast<SERVICE_NOTIFY*>(param);

  // Check the notification results
  if(sn->dwNotificationStatus == ERROR_SUCCESS && (sn->dwNotificationTriggered & SERVICE_NOTIFY_STOP_PENDING))
    ::PostMessage(g_handler->GetMainHwnd(), WM_COMMAND, IDM_EXIT, IDM_SERVICE_STOPPED);
}

#endif

unsigned __stdcall WatchSvc(void *lpParam) {

#if NTDDI_VERSION >= NTDDI_VISTA

  g_scm = ::OpenSCManager(
    NULL,                              // local SCM
    SERVICES_ACTIVE_DATABASE,
    SC_MANAGER_ENUMERATE_SERVICE);

  g_service = ::OpenService(
    g_scm,
    L"epTokend",
    SERVICE_STOP | SERVICE_QUERY_STATUS);

  SERVICE_NOTIFY serviceNotify = 
    { SERVICE_NOTIFY_STATUS_CHANGE, 
      ServiceNotifyCallback };

  ::NotifyServiceStatusChange(
    g_service,
    SERVICE_NOTIFY_STOP_PENDING,
    &serviceNotify);

  ::SleepEx(INFINITE, TRUE); // Wait for the notification 

#else

  HANDLE hPipe = CreatePipe();
  if (NULL == hPipe || hPipe == INVALID_HANDLE_VALUE)
    goto terminated;

  if(::ConnectNamedPipe(hPipe, NULL) || ::GetLastError() == ERROR_PIPE_CONNECTED)
  {
    BYTE buffer[sizeof UINT] = {0}; 
    UINT uMessage = 0;   
    
    while (true) 
    {
		DWORD lpNumberOfBytesRead = 0;
      if (::ReadFile(hPipe, &buffer, sizeof UINT, &lpNumberOfBytesRead, 0) && sizeof UINT == lpNumberOfBytesRead)
      {
        uMessage = *((UINT*)&buffer[0]);
        
        // The processing of the received data
        if(uMessage == 0x0) {
          ::PostMessage(g_handler->GetMainHwnd(), WM_COMMAND, IDM_EXIT, IDM_SERVICE_STOPPED);
          break; //Terminated from the service
        }
      }
      else
      {   
      }

      ::Sleep(1000);  // Simulate some lengthy operations. 
    }

    ::DisconnectNamedPipe(hPipe);
  }
  else
  {
  }

terminated :

#endif

  _endthreadex(0);
  return 0; 
}  

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT  - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  PAINTSTRUCT ps;
  HDC hdc;

   static BOOL s_fCanShowWnd = TRUE;

  switch (message)
  {

    case WM_CREATE: {

      // add the notification icon
      AddNotificationIcon(hWnd);
      // ShowStartNotification();

      // Create the single static handler class instance
      g_handler = new ClientHandler();
      g_handler->SetMainHwnd(hWnd);

      // Create the child windows used for navigation
      RECT rect;

      GetClientRect(hWnd, &rect);

      CefWindowInfo info;
      CefBrowserSettings settings;

      // Populate the settings based on command line arguments.
      AppGetBrowserSettings(settings);

      //settings.file_access_from_file_urls_allowed = true;
        //settings.universal_access_from_file_urls_allowed = true;

//      settings.accelerated_2d_canvas_disabled = false;
//      settings.accelerated_compositing_disabled = false;
     // settings.accelerated_filters_enabled = true;
      //settings.accelerated_layers_disabled =  false;
     // settings.accelerated_painting_enabled = true;
      //settings.accelerated_plugins_disabled = true;
      //settings.accelerated_video_disabled = true;
      //settings.application_cache_disabled = false;
   //   settings.databases_disabled = true;
      //settings.developer_tools_disabled = true;
 //     settings.plugins_disabled = true;
   //   settings.fullscreen_enabled = false;
    //  settings.javascript_open_windows_disallowed = true;
     // settings.plugins_disabled = true;
  //    settings.webgl_disabled = false;
//      settings.java_disabled = true;

      // Initialize window info to the defaults for a child window
      info.SetAsChild(hWnd, rect);

      // Creat the new child browser window
      CefBrowserHost::CreateBrowser(info, g_handler.get(),
          g_handler->GetStartupURL(), settings);

      g_handler->app_running_ = true;
      epsilon::TokenWatcher::Init(g_handler);

      m_SvcWatcherHnd = (HANDLE)_beginthreadex(NULL, 0, &WatchSvc, NULL, 0, NULL);

      return 0;
    }

    case WM_COMMAND: {

      CefRefPtr<CefBrowser> browser;
        
      if (g_handler.get())
          browser = g_handler->GetBrowser();

        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
          // case IDM_TOKEN_INSERTED:
          //   ShowTokenInsertedBalloon();
          //           break;

          // case IDM_TOKEN_REMOVED:
          //   ShowTokenRemovedBalloon();
          //           break;

          // case IDM_PIN_BLOCKED:
          //           ShowPINBlockedBalloon();
          //           break;

          // case IDM_SHOW_TOKEN_MGR: //options
          //     // placeholder for an options dialog
          //     MessageBox(hWnd,  L"Display the options dialog here.", L"Options", MB_OK);
          //     break;

          case IDM_BROWSER_SHOW:
            if (!IsWindowVisible(hWnd)) 
              ShowWnd(hWnd);
            break;

          case IDM_BROWSER_HIDE:
            if (IsWindowVisible(hWnd))
              HideWnd(hWnd);
            break;

          case IDM_EXIT:{
            if (g_handler.get()) {

              g_handler->app_running_ = false;
              
              CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
              if (browser.get()) {
              // Let the browser window know we are about to destroy it.
              browser->GetHost()->ParentWindowWillClose();
              //HideWnd(hWnd);
              //return 0;
              }
            }
          //if (g_handler.get()) {
                //g_handler->QuittingApp(true);
              //g_handler->DispatchCloseToNextBrowser();
            //} else {

            if(m_SvcWatcherHnd) {
              TerminateThread(m_SvcWatcherHnd, 0);
              CloseHandle(m_SvcWatcherHnd);
              m_SvcWatcherHnd = NULL;
            }              

            if(lParam != IDM_SERVICE_STOPPED) {
              
			try {

              DoStopSvc(
				  
#if NTDDI_VERSION >= NTDDI_VISTA

				  g_service

#else
				  NULL
				  
#endif

				  );

			} catch(...) {}

                      
            }
            
#if NTDDI_VERSION >= NTDDI_VISTA

			if(g_service)
				::CloseServiceHandle(g_service);
            
			if(g_scm)
				::CloseServiceHandle(g_scm);

#endif            

			DestroyWindow(hWnd);
            DeleteNotificationIcon(hWnd);            

            PostQuitMessage(0);                
            //}
            break;
          }

          default:
            return DefWindowProc(hWnd, message, wParam, lParam);          

          //case ID_WARN_CONSOLEMESSAGE:
          //  if (g_handler.get()) {
          //    std::wstringstream ss;
          //    ss << L"Console messages will be written to "
          //        << std::wstring(CefString(g_handler->GetLogFile()));
          //    MessageBox(hWnd, ss.str().c_str(), L"Console Messages",
          //        MB_OK | MB_ICONINFORMATION);
          //  }
          //  return 0;
        }
    }
    break;

    case WMAPP_NOTIFYCALLBACK: {
		
      switch (LOWORD(lParam))
      {
        case NIN_SELECT:
		case NIN_KEYSELECT:
		case WM_LBUTTONUP:
          // for NOTIFYICON_VERSION_4 clients, NIN_SELECT is prerable to listening to mouse clicks and key presses
          // directly.
          if (IsWindowVisible(hWnd))
          {
             HideWnd(hWnd);
             s_fCanShowWnd = FALSE;
          }
          else if (s_fCanShowWnd) {
             ShowWnd(hWnd);
          }

          break;

        case NIN_BALLOONTIMEOUT:
          // RestoreTooltip();
          break;

        case NIN_BALLOONUSERCLICK:
          // RestoreTooltip();
          // placeholder for the user clicking on the balloon.
          // MessageBox(hWnd, L"The user clicked on the balloon.", L"User click", MB_OK);
          break;

        case WM_CONTEXTMENU:
		case WM_RBUTTONUP:
          {
			POINT pt;// = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };			
			GetCursorPos(&pt);

            if (IsWindowVisible(hWnd)) {
				::SetForegroundWindow(hWnd);
				ShowContextMenu(IDC_CONTEXTMENU_HIDE, hWnd, pt);
			}
            else
              ShowContextMenu(IDC_CONTEXTMENU_SHOW, hWnd, pt);
          }
          break;
      }
      break;
    }
    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      // TODO: Add any drawing code here...
      EndPaint(hWnd, &ps);
      break;

    case WM_SETFOCUS:
      if (g_handler.get() && g_handler->GetBrowser()) {
        // Pass focus to the browser window
        CefWindowHandle hWnd =
            g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hWnd)
          PostMessage(hWnd, WM_SETFOCUS, wParam, NULL);
      }
      return 0;

    case WM_SIZE:
      {
        //if (wParam == SIZE_MINIMIZED && g_handler.get() &&
        //    g_handler->GetBrowser()) {
        //  CefWindowHandle hWnd =
        //      g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        //  //if (hWnd)
        //  //  HideWnd(hWnd);
        //}

        //// Minimizing resizes the window to 0x0 which causes our layout to go all
        //// screwy, so we just ignore it.
        //if (wParam != SIZE_MINIMIZED && g_handler.get() &&
        //    g_handler->GetBrowser()) {
        //  CefWindowHandle hwnd =
        //      g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        //  if (hwnd) {
        //    //// Resize the browser window and address bar to match the new frame
        //    //// window size
        //    //RECT rect;
        //    //GetClientRect(hWnd, &rect);
        //    ////rect.top += URLBAR_HEIGHT;

        //    ////int urloffset = rect.left + BUTTON_WIDTH * 4;

        //    //HDWP hdwp = BeginDeferWindowPos(1);
        //    ////hdwp = DeferWindowPos(hdwp, editWnd, NULL, urloffset,
        //    //  //0, rect.right - urloffset, URLBAR_HEIGHT, SWP_NOZORDER);
        //    //hdwp = DeferWindowPos(hdwp, hwnd, NULL,
        //    //  rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
        //    //  SWP_NOZORDER);
        //    //EndDeferWindowPos(hdwp);
        //  }
        //}
      }
      break;

    case WM_ERASEBKGND:
      if (g_handler.get() && g_handler->GetBrowser()) {
        CefWindowHandle hwnd =
            g_handler->GetBrowser()->GetHost()->GetWindowHandle();
        if (hwnd) {
          // Dont erase the background if the browser window has been loaded
          // (this avoids flashing)
          return 0;
        }
      }
      break;

    case WM_CLOSE:
      //if (g_handler.get()) {
        //CefRefPtr<CefBrowser> browser = g_handler->GetBrowser();
        //if (browser.get()) {
          // Let the browser window know we are about to destroy it.
          //browser->GetHost()->ParentWindowWillClose();
      HideWnd(hWnd);
        //}
      //}
      break;

    case WM_TIMER:
      switch(wParam) { 
        case IDM_BROWSER_SHOW: {
            PostMessage(hWnd, WM_COMMAND, IDM_BROWSER_SHOW, NULL);
            KillTimer(hWnd, IDM_BROWSER_SHOW);
          }
          break;
        
        case HIDEBROWSER_TIMER_ID: {
            // please see the comment in HideWnd() for an explanation of this code.
            KillTimer(hWnd, HIDEBROWSER_TIMER_ID);
            s_fCanShowWnd = TRUE;
          }
          break;
      }
      break;      

    case WM_DESTROY:
      // TODO: happens when we call DestroyWindow   
      return 0;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

BOOL IsWinVistaOrLater()
{
    // Initialize the OSVERSIONINFOEX structure.
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 0;

    // Initialize the condition mask.
    DWORDLONG dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

    // Perform the test.
    return VerifyVersionInfo(&osvi, 
                             VER_MAJORVERSION | VER_MINORVERSION,
                             dwlConditionMask);
}

BOOL DeleteNotificationIcon(HWND const &hWnd)
{
    NOTIFYICONDATA nid = {0};
	nid.hWnd = hWnd;

	DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

	if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
        nid.cbSize = sizeof (NOTIFYICONDATA);
    
#if NTDDI_VERSION >= NTDDI_VISTA  
	else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
        nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);    
#endif

	else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
        nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
	else 
        nid.cbSize = NOTIFYICONDATA_V1_SIZE;

    nid.uFlags = NIF_GUID;

#if NTDDI_VERSION > NTDDI_VISTA  
  nid.guidItem = __uuidof(NotificationIcon);
#else
	nid.uID = NOTIFICATION_ICON_ID;
#endif

    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

// BOOL ShowTokenRemovedBalloon()
// {
//     // Display a low ink balloon message. This is a warning, so show the appropriate system icon.
//     NOTIFYICONDATA nid = {0};

//   DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

//   if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof (NOTIFYICONDATA);
    
//   else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);
    
//   else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
//         nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
//   else 
//         nid.cbSize = NOTIFYICONDATA_V1_SIZE;

//     nid.uFlags = NIF_INFO | NIF_GUID;
// #if NTDDI_VERSION > NTDDI_VISTA  
//   nid.guidItem = __uuidof(NotificationIcon); 
//#else
//	nid.uID = NOTIFICATION_ICON_ID;
// #endif
//     // respect quiet time since this balloon did not come from a direct user action.
//     nid.dwInfoFlags = NIIF_WARNING | NIIF_RESPECT_QUIET_TIME;
//     LoadString(g_hInst, IDS_TOKEN_REMOVED_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
//     LoadString(g_hInst, IDS_TOKEN_REMOVED_TEXT, nid.szInfo, ARRAYSIZE(nid.szInfo));
//     return Shell_NotifyIcon(NIM_MODIFY, &nid);
// }

// BOOL ShowPINBlockedBalloon()
// {
//     // Display an out of ink balloon message. This is a error, so show the appropriate system icon.
//     NOTIFYICONDATA nid = {0};

//   DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

//   if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof (NOTIFYICONDATA);
    
//   else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);
    
//   else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
//         nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
//   else 
//         nid.cbSize = NOTIFYICONDATA_V1_SIZE;

//     nid.uFlags = NIF_INFO | NIF_GUID;
// #if NTDDI_VERSION > NTDDI_VISTA  
//   nid.guidItem = __uuidof(NotificationIcon);
//#else
//	nid.uID = NOTIFICATION_ICON_ID;
// #endif
//     nid.dwInfoFlags = NIIF_ERROR;
//     LoadString(g_hInst, IDS_PIN_BLOCKED_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
//     LoadString(g_hInst, IDS_PIN_BLOCKED_TEXT, nid.szInfo, ARRAYSIZE(nid.szInfo));
//     return Shell_NotifyIcon(NIM_MODIFY, &nid);
// }

// BOOL ShowTokenInsertedBalloon()
// {
//     // Display a balloon message for a print job with a custom icon
//     NOTIFYICONDATA nid = {0};

//   DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

//   if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof (NOTIFYICONDATA);
    
//   else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);
    
//   else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
//         nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
//   else 
//         nid.cbSize = NOTIFYICONDATA_V1_SIZE;

//     nid.uFlags = NIF_INFO | NIF_GUID;
// #if NTDDI_VERSION > NTDDI_VISTA  
//   nid.guidItem = __uuidof(NotificationIcon); 
//#else
//	nid.uID = NOTIFICATION_ICON_ID;
// #endif
//     nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
//     LoadString(g_hInst, IDS_TOKEN_INSERTED_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
//     LoadString(g_hInst, IDS_TOKEN_INSERTED_TEXT, nid.szInfo, ARRAYSIZE(nid.szInfo));
//     LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_USBTOKEN), LIM_LARGE, &nid.hBalloonIcon);
//     return Shell_NotifyIcon(NIM_MODIFY, &nid);
// }

// BOOL ShowStartNotification()
// {
//     // Display a balloon message for a print job with a custom icon
//     NOTIFYICONDATA nid = {0};

//   DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

//   if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof (NOTIFYICONDATA);
    
// #if NTDDI_VERSION > NTDDI_VISTA  
// 	else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);    
// #endif    
//   else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
//         nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
//   else 
//         nid.cbSize = NOTIFYICONDATA_V1_SIZE;

//     nid.uFlags = NIF_INFO | NIF_GUID;
// #if NTDDI_VERSION > NTDDI_VISTA  
//   nid.guidItem = __uuidof(NotificationIcon);  
//#else
//	nid.uID = NOTIFICATION_ICON_ID;
// #endif
//     nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
//     LoadString(g_hInst, IDS_EP_TOKEND_START_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
//     LoadString(g_hInst, IDS_EP_TOKEND_START_TEXT, nid.szInfo, ARRAYSIZE(nid.szInfo));
//     LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_USBTOKEN), LIM_LARGE, &nid.hBalloonIcon);
//     return Shell_NotifyIcon(NIM_MODIFY, &nid);
// }

// BOOL RestoreTooltip()
// {
//     // After the balloon is dismissed, restore the tooltip.
//     NOTIFYICONDATA nid = {0};

//   DWORD ullVersion = GetAssemblyVersion(L"shell32.dll");

//   if(ullVersion > MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof (NOTIFYICONDATA);
    
//   else if(ullVersion == MAKEDLLVERULL(6, 0, 0, 0))
//         nid.cbSize = sizeof(NOTIFYICONDATA_V3_SIZE);
    
//   else if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0))
//         nid.cbSize = NOTIFYICONDATA_V2_SIZE;
    
//   else 
//         nid.cbSize = NOTIFYICONDATA_V1_SIZE;

//     nid.uFlags = NIF_SHOWTIP | NIF_GUID;
// #if NTDDI_VERSION > NTDDI_VISTA  
//   nid.guidItem = __uuidof(NotificationIcon);  
//#else
//	nid.uID = NOTIFICATION_ICON_ID;
// #endif
//     return Shell_NotifyIcon(NIM_MODIFY, &nid);
// }

// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

// Purpose: 
//   Stops the service.
//
BOOL DoStopSvc(SC_HANDLE mService)
{
	BOOL bLocal = TRUE;
	SC_HANDLE scm;
	SC_HANDLE hService;

	if(mService == NULL || INVALID_HANDLE_VALUE == mService) {

	  scm = ::OpenSCManager(
		NULL,                              // local SCM
		SERVICES_ACTIVE_DATABASE,
		SC_MANAGER_ENUMERATE_SERVICE);

	if (scm == NULL) 
		goto cleanup;

	  hService = ::OpenService(
		scm,
		L"epTokend",
		SERVICE_STOP | SERVICE_QUERY_STATUS);

	if (hService == NULL) 
		goto cleanup;
		
	} else {

		hService = mService;
		bLocal = FALSE;
	}

    SERVICE_STATUS_PROCESS ssp;
    // DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    // DWORD dwTimeout = 30000; // 30-second time-out
    // DWORD dwWaitTime;

    // Make sure the service is not already stopped.

    if(!QueryServiceStatusEx(
            hService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
    {
        wprintf(L"QueryServiceStatusEx failed (%d)\n", GetLastError()); 
        return FALSE;
    }

    if(ssp.dwCurrentState == SERVICE_STOPPED || ssp.dwCurrentState == SERVICE_STOP_PENDING)
    {
        wprintf(L"Service is already stopped or stopping.\n");
        return TRUE;
    }

    // If a stop is pending, wait for it.

    // while(ssp.dwCurrentState == SERVICE_STOP_PENDING) 
    // {
    //     wprintf(L"Service stop pending...\n");

    //     // Do not wait longer than the wait hint. A good interval is 
    //     // one-tenth of the wait hint but not less than 1 second  
    //     // and not more than 10 seconds. 
 
    //     dwWaitTime = ssp.dwWaitHint / 10;

    //     if(dwWaitTime < 1000)
    //         dwWaitTime = 1000;
    //     else if(dwWaitTime > 10000)
    //         dwWaitTime = 10000;

    //     Sleep(dwWaitTime);

    //     if(!QueryServiceStatusEx(
    //              hService, 
    //              SC_STATUS_PROCESS_INFO,
    //              (LPBYTE)&ssp, 
    //              sizeof(SERVICE_STATUS_PROCESS),
    //              &dwBytesNeeded))
    //     {
    //         wprintf(L"QueryServiceStatusEx failed (%d)\n", GetLastError()); 
    //         return FALSE;
    //     }

    //     if(ssp.dwCurrentState == SERVICE_STOPPED)
    //     {
    //         wprintf(L"Service stopped successfully.\n");
    //         return FALSE;
    //     }

    //     if(GetTickCount() - dwStartTime > dwTimeout)
    //     {
    //         wprintf(L"Service stop timed out.\n");
    //         return FALSE;
    //     }
    // }

    // Send a stop code to the service.

    if(!ControlService(
            hService, 
            SERVICE_CONTROL_STOP, 
            (LPSERVICE_STATUS) &ssp))
    {
        wprintf(L"ControlService failed (%d)\n", GetLastError());
        return FALSE;
    }

    // Wait for the service to stop.

    // while (ssp.dwCurrentState != SERVICE_STOPPED) 
    // {
    //     Sleep(ssp.dwWaitHint);
    //     if(!QueryServiceStatusEx(
    //             hService, 
    //             SC_STATUS_PROCESS_INFO,
    //             (LPBYTE)&ssp, 
    //             sizeof(SERVICE_STATUS_PROCESS),
    //             &dwBytesNeeded))
    //     {
    //         wprintf(L"QueryServiceStatusEx failed (%d)\n", GetLastError());
    //         return FALSE;
    //     }

    //     if(ssp.dwCurrentState == SERVICE_STOPPED)
    //         break;

    //     if(GetTickCount() - dwStartTime > dwTimeout)
    //     {
    //         wprintf(L"Wait timed out\n");
    //         return FALSE;
    //     }
    // }

cleanup:

	if(bLocal == TRUE) {

		if(hService)
			::CloseServiceHandle(hService);

		if(scm)
			::CloseServiceHandle(scm);
	}

    return TRUE;
}

// Formats a message string using the specified message and variable
// list of arguments.
LPWSTR GetLastErrorMessage()
{
    LPWSTR pBuffer = NULL; 

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM | 
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, 
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  (LPWSTR)&pBuffer, 
                  0, 
                  NULL);

    return pBuffer;
}

/* Registry functions */

// Add trailing separator, if necessary
void EnsureTrailingSeparator(LPWSTR pRet)
{
  if (!pRet)
    return;

  int len = wcslen(pRet);
  if (len > 0 && wcscmp(&(pRet[len-1]), L"\\") != 0)
  {
    wcscat(pRet, L"\\");
  }
}

// Helper method to build Registry Key string
void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet)
{
  // Check for required params
  ASSERT(pBase && pApp && pRet);
  if (!pBase || !pApp || !pRet)
    return;

  // Base
  wcscpy(pRet, pBase);

  // Group (optional)
  if (pGroup && (pGroup[0] != '\0'))
  {
    EnsureTrailingSeparator(pRet);
    wcscat(pRet, pGroup);
  }

  // App name
  EnsureTrailingSeparator(pRet);
  wcscat(pRet, pApp);

  // Folder (optional)
  if (pFolder && (pFolder[0] != '\0'))
  {
    EnsureTrailingSeparator(pRet);
    wcscat(pRet, pFolder);
  }  
}

// get integer value from registry key
// caller can either use return value to determine success/fail, or pass a default to be used on fail
bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret)
{
  HKEY hKey;
  bool result = false;

  wchar_t key[MAX_PATH] = {0};
  GetKey(L"Software", PREF_EPSILON_BASE, APP_NAME, pFolder, (LPWSTR)&key);

  if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, KEY_READ, &hKey))
  {
    DWORD dwValue = 0;
    DWORD dwType = 0;
    DWORD dwCount = sizeof(DWORD);
    if (ERROR_SUCCESS == RegQueryValueEx(hKey, pEntry, NULL, &dwType, (LPBYTE)&dwValue, &dwCount))
    {
      result = true;
      ASSERT(dwType == REG_DWORD);
      ASSERT(dwCount == sizeof(dwValue));
      ret = (int)dwValue;
    }
    RegCloseKey(hKey);
  }

  if (!result)
  {
    // couldn't read value, so use default, if specified
    if (pDefault)
      ret = *pDefault;
  }

  return result;
}

// write integer value to registry key
bool WriteRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int val)
{
  HKEY hKey;
  bool result = false;

  wchar_t key[MAX_PATH] = {0};
  GetKey(L"Software", PREF_EPSILON_BASE, APP_NAME, pFolder, (LPWSTR)&key);

  if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, (LPCWSTR)key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
  {
    DWORD dwCount = sizeof(int);
    if (ERROR_SUCCESS == RegSetValueEx(hKey, pEntry, 0, REG_DWORD, (LPBYTE)&val, dwCount))
      result = true;
    RegCloseKey(hKey);
  }

  return result;
}

CefString ClientApp::GetCurrentLanguage() {

  // Read current language from registry in case user has changed his language
  int langID = 0;

  if(!GetRegistryInt(NULL, L"language", NULL, langID))
	  langID = GetUserDefaultUILanguage();

  if(langID != 1033 && langID != 1036)
	  langID = 1033;

  // Convert LANGID to a RFC 4646 language tag (per navigator.language)
  int langSize = GetLocaleInfo(langID, LOCALE_SISO639LANGNAME, NULL, 0);
  int countrySize = GetLocaleInfo(langID, LOCALE_SISO3166CTRYNAME, NULL, 0);

  wchar_t *lang = new wchar_t[langSize + countrySize + 1];
  wchar_t *country = new wchar_t[countrySize];

  GetLocaleInfo(langID, LOCALE_SISO639LANGNAME, lang, langSize);
  GetLocaleInfo(langID, LOCALE_SISO3166CTRYNAME, country, countrySize);

  // add hyphen
  wcscat(wcscat(lang, L"-"), country);
  std::wstring locale(lang);

  delete [] lang;
  delete [] country;

  return CefString(locale);
}

bool ClientHandler::SetLang(const int langID) {
	return bool(WriteRegistryInt(NULL, L"language", langID));
}
