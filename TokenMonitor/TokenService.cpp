/****************************** Module Header ******************************\
* Module Name:  SampleService.cpp
* Project:      TokenService
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

#include "inc\config.h"

#include <windows.h>

#include <commdlg.h>
#include <shellapi.h>
#include <commctrl.h>
#include <direct.h>
#include <sstream>
#include <string>

#include <wtsapi32.h>
#include <userenv.h>

#include <tchar.h>
#include <strsafe.h>

#include "inc\ServiceInstaller.h"
#include "inc\ServiceBase.h"
#include "inc\TokenService.h"
#include "inc\ThreadPool.h"

#pragma endregion

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "advapi32.lib")

// 
// Settings of the service
// 

// Internal name of the service
#define SERVICE_NAME             L"epTokend"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     L"Epsilon USB Token Manager Service Monitor"

// Description of the service
#define SERVICE_DESCRIPTION     L"Epsilon USB Token Manager Service Monitor"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_AUTO_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     L""

// The name of the account under which the service should run
// #define SERVICE_ACCOUNT          L"NT AUTHORITY\\LocalService"
#define SERVICE_ACCOUNT          L"LocalSystem"

// The password to the service account name
#define SERVICE_PASSWORD         L""

//#if NTDDI_VERSION >= NTDDI_VISTA

#define APP_CMD L"C:\\Program\ Files\ (x86)\\Epsilon\\Epsilon\ Token\ Manager\\epToken.exe"
//#define APP_CMD L"D:\\Users\\amadou\\Documents\\Visual\ Studio\ 2010\\Projects\\cef3\\Release\\epToken.exe"

//#else

#define APP_CMD_XP L"C:\\Program\ Files\\Epsilon\\Epsilon\ Token\ Manager\\epToken.exe"
//#define APP_CMD_XP L"D:\\Users\\amadou\\Documents\\Visual\ Studio\ 2010\\Projects\\cef3\\Release\\epToken.exe"

//#endif


#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
                    DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
                    DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
                    DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
                    WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
                    WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
                    WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
                    STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

HANDLE GetCurrentUserToken();
BOOL RunTokenMgr(LPWSTR, LPPROCESS_INFORMATION);

#if NTDDI_VERSION < NTDDI_VISTA

BOOL MyPostQuitMessage(UINT = 0x0);

#endif

BOOL IsWinVistaOrLater();

BOOL AddAceToWindowStation(HWINSTA, PSID);

BOOL AddAceToDesktop(HDESK, PSID);

BOOL GetLogonSID (HANDLE, PSID*);

VOID FreeLogonSID (PSID*);

// Formats a message string using the specified message and variable
// list of arguments.
LPWSTR GetLastErrorMessage();

// 
//  FUNCTION: wmain(int, wchar_t *[]) 
// 
//  PURPOSE: entrypoint for the application. 
//  
//  PARAMETERS: 
//    argc - number of command line arguments 
//    argv - array of command line arguments 
// 
//  RETURN VALUE: 
//    none 
// 
//  COMMENTS: 
//    wmain() either performs the command line task, or run the service. 
// 
int wmain(int argc, wchar_t *argv[]) 
{

    if (argc > 1) 
    { 
	     	size_t index  = (*argv[1] == L'-' || (*argv[1] == L'/')) ? ((*(argv[1] + 1) == L'-') ? 2 : 1) : 0;

        if (_wcsicmp(L"install", argv[1] + index) == 0) 
        { 
            // Install the service when the command is  
            // "-install" or "/install". 
            InstallService( 
                SERVICE_NAME,               // Name of service 
                SERVICE_DISPLAY_NAME,       // Name to display 
                SERVICE_DESCRIPTION,    // Description of the service
                SERVICE_START_TYPE,         // Service start type 
                SERVICE_DEPENDENCIES,       // Dependencies 
                SERVICE_ACCOUNT,            // Service running account 
                SERVICE_PASSWORD            // Password of the account 
                ); 
        } 
        else if (_wcsicmp(L"remove", argv[1] + index) == 0) 
        { 
            // Uninstall the service when the command is  
            // "-remove" or "/remove". 
            UninstallService(SERVICE_NAME); 
        } 
        else if (_wcsicmp(L"start", argv[1] + index) == 0) 
        {
            // Start the service when the command is         
            // "-start" or "--start" or "/start".
            DoStartSvc(SERVICE_NAME);
          
        } 
        else if (_wcsicmp(L"stop", argv[1] + index) == 0) 
        {
            // Stop the service when the command is 
            // "-stop" or "--stop" or "/stop".
            DoStopSvc(SERVICE_NAME);
          
        }        
    } 
    else 
    { 
        wprintf(L"Parameters:\n"); 
        wprintf(L" -install  to install the service.\n"); 
        wprintf(L" -remove   to remove the service.\n"); 
        wprintf(L" -start  to start the service.\n"); 
        wprintf(L" -stop  to stop the service.\n");
 
        TokenService svc(SERVICE_NAME); 
        if (!CServiceBase::Run(svc)) 
          wprintf(L"Service failed to run w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage()); 
    } 

  return 0;
}

TokenService::TokenService(PWSTR pszServiceName, 
                               BOOL fCanStop, 
                               BOOL fCanShutdown, 
                               BOOL fCanPauseContinue)
: m_fStopping(FALSE), CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue) 
{  
  // Create a manual-reset event that is not signaled at first to indicate 
  // the stopped signal of the service.
  if ((m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    throw GetLastError();
}


TokenService::~TokenService(void)
{
  if (m_hStoppedEvent)
  {
      CloseHandle(m_hStoppedEvent);
      m_hStoppedEvent = NULL;
  }
}


//
//   FUNCTION: TokenService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the 
//   service by the SCM or when the operating system starts (for a service 
//   that starts automatically). It specifies actions to take when the 
//   service starts. In this code sample, OnStart logs a service-start 
//   message to the Application log, and queues the main service function for 
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore, 
//   it usually polls or monitors something in the system. The monitoring is 
//   set up in the OnStart method. However, OnStart does not actually do the 
//   monitoring. The OnStart method must return to the operating system after 
//   the service's operation has begun. It must not loop forever or block. To 
//   set up a simple monitoring mechanism, one general solution is to create 
//   a timer in OnStart. The timer would then raise events in your code 
//   periodically, at which time your service could do its monitoring. The 
//   other solution is to spawn a new thread to perform the main service 
//   functions, which is demonstrated in this code sample.
//
void TokenService::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"TokenService in OnStart", 
        EVENTLOG_INFORMATION_TYPE);

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&TokenService::ServiceWorkerThread, this);
}

//
//   FUNCTION: TokenService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs 
//   on a thread pool worker thread.
//
void TokenService::ServiceWorkerThread(void)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"TokenService in ServiceWorkerThread", EVENTLOG_INFORMATION_TYPE);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION)); 

    if(RunTokenMgr(IsWinVistaOrLater() ? APP_CMD : APP_CMD_XP, &pi)) {

     if (pi.hProcess != INVALID_HANDLE_VALUE) 
     { 
      WaitForSingleObject(pi.hProcess, INFINITE); // The process will be invalide if we don't wait.
      CloseHandle(pi.hProcess); 
     } 

     if (pi.hThread != INVALID_HANDLE_VALUE)
      CloseHandle(pi.hThread);          
    }

    // while (!m_fStopping)
    // {

    //   ::Sleep(1000);  // Simulate some lengthy operations. 
    // }

    // WriteEventLogEntry(L"before CreatePipe", EVENTLOG_INFORMATION_TYPE);

    // HANDLE hPipe = CreatePipe();
    // if (NULL == hPipe || hPipe == INVALID_HANDLE_VALUE) {
    //     WriteErrorLogEntry(L"CreatePipe");
    //     goto terminated;
    // }

    // WriteEventLogEntry(L"CreatePipe returned successfully", EVENTLOG_INFORMATION_TYPE);

    // if(::ConnectNamedPipe(hPipe, NULL) || ::GetLastError() == ERROR_PIPE_CONNECTED)
    // {
    //   WriteEventLogEntry(L"ConnectNamedPipe returned successfully", EVENTLOG_INFORMATION_TYPE);

    //   BYTE buffer[sizeof UINT] = {0}; 
    //   UINT uMessage = 0;   
      
    //   while (!m_fStopping) 
    //   {
    //     if (::ReadFile(hPipe, &buffer, sizeof UINT, NULL, 0))
    //     {
    //       WriteEventLogEntry(L"ReadFile returned successfully", EVENTLOG_INFORMATION_TYPE);
    //       uMessage = *((UINT*)&buffer[0]);
          
    //       // The processing of the received data
    //       if(uMessage == 0x0)
    //         goto terminated; //Terminated from the client
    //     }
    //     else
    //     {   
    //       WriteErrorLogEntry(L"ReadFile");           
    //     }

    //     ::Sleep(1000);  // Simulate some lengthy operations. 
    //   }

    //   WriteEventLogEntry(L"m_fStopping = TRUE", EVENTLOG_INFORMATION_TYPE);  

    //   ::DisconnectNamedPipe(hPipe);
    //   MyPostQuitMessage(hPipe); // Terminated from the server
    // }
    // else
    // {
    //   WriteErrorLogEntry(L"ConnectNamedPipe");
    // }

  // // Periodically check if the service is stopping. 
  //   while (!m_fStopping) 
  //   {  
  //       BOOL bResult = ::ConnectNamedPipe(hPipe, 0);

  //       if (bResult || ::GetLastError() == ERROR_PIPE_CONNECTED)
  //       {
  //           BYTE buffer[sizeof UINT] = {0}; 
  //           DWORD read = 0;

  //           UINT uMessage = 0;

  //           if (::ReadFile(hPipe, &buffer, sizeof UINT, &read, 0))
  //           {
  //               uMessage = *((UINT*)&buffer[0]);
                
  //               // The processing of the received data
  //               if(uMessage == 0)
  //                 goto terminated; //Terminated from the client
  //           }
  //           else
  //           {              
  //           }

  //           ::DisconnectNamedPipe(hPipe);
  //       }
  //       else
  //       { 
  //       }

  //       ::Sleep(1000);  // Simulate some lengthy operations. 
  //   }

  //   MyPostQuitMessage(hPipe); // Terminated from the server
 
   // if (hPipe != INVALID_HANDLE_VALUE) 
   //  CloseHandle(hPipe); 
   
    // Signal the stopped event. 
    SetEvent(m_hStoppedEvent);
}


//
//   FUNCTION: TokenService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the 
//   service by SCM. It specifies actions to take when a service stops 
//   running. In this code sample, OnStop logs a service-stop message to the 
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with 
//   SERVICE_STOP_PENDING if the procedure is going to take long time. 
//
void TokenService::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"TokenService in OnStop", 
        EVENTLOG_INFORMATION_TYPE);

#if NTDDI_VERSION < NTDDI_VISTA
    MyPostQuitMessage();
#endif

  // Indicate that the service is stopping and wait for the finish of the 
  // main service function (ServiceWorkerThread).
  m_fStopping = TRUE;
  WaitForSingleObject(m_hStoppedEvent, INFINITE);
  // if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
	 //  exit(GetLastError()); //throw GetLastError();

  // exit(0);
}

//
//   FUNCTION: TokenService::OnShutdown()
//
//   PURPOSE: When implemented in a derived class, executes when the system 
//   is shutting down. Specifies what should occur immediately prior to the 
//   system shutting down.
//
void TokenService::OnShutdown()
{

    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"TokenService in OnShutdown", 
        EVENTLOG_INFORMATION_TYPE);

#if NTDDI_VERSION < NTDDI_VISTA
    MyPostQuitMessage();
#endif

  // Indicate that the service is stopping and wait for the finish of the 
  // main service function (ServiceWorkerThread).
  m_fStopping = TRUE;
  WaitForSingleObject(m_hStoppedEvent, INFINITE);
  // if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
  //   exit(GetLastError()); //throw GetLastError();

  // exit(0);
}

HANDLE GetCurrentUserToken()
{    
  HANDLE currentToken;
  HANDLE primaryToken = INVALID_HANDLE_VALUE;

#if NTDDI_VERSION >= NTDDI_VISTA

  int dwSessionId = WTSGetActiveConsoleSessionId();

#else

  int dwSessionId = 0;
  PWTS_SESSION_INFO pSessionInfo = NULL;
  DWORD dwCount = 0;
  BOOL found = FALSE;

  // Get the list of all terminal sessions 
  if(!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, 
                       &pSessionInfo, &dwCount))
    return INVALID_HANDLE_VALUE; 

  // look over obtained list in search of the active session
  for (DWORD i = 0; i < dwCount; ++i)
  {
    WTS_SESSION_INFO si = pSessionInfo[i];
    if (WTSActive == si.State)
    { 
      // If the current session is active – store its ID
      dwSessionId = si.SessionId;
      found = TRUE;
      break;
    }
  }   

  WTSFreeMemory(pSessionInfo);

  if(found == FALSE)
    return INVALID_HANDLE_VALUE;

#endif    


  // Get token of the logged in user by the active session ID
  if (!WTSQueryUserToken(dwSessionId, &currentToken)) {
    CServiceBase::GetService()->WriteErrorLogEntry(L"WTSQueryUserToken");
    return INVALID_HANDLE_VALUE;
  }

  if (!DuplicateTokenEx(currentToken, 
          TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS,
          NULL, SecurityImpersonation, TokenPrimary, &primaryToken)) {
    CServiceBase::GetService()->WriteErrorLogEntry(L"DuplicateTokenEx");
    return INVALID_HANDLE_VALUE;        
  }

  return primaryToken;
}

// BOOL EnablePrivilege(LPCTSTR lpszPrivilegeName, BOOL bEnable)
// {

//   HANDLE hToken;
//   TOKEN_PRIVILEGES tp;
//   LUID luid;

//   if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
//     return FALSE;

//   if(!LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid))
//     return TRUE;

//   tp.PrivilegeCount = 1;
//   tp.Privileges[0].Luid = luid;
//   tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

//   AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, NULL, NULL);
//   CloseHandle(hToken);

//   return (GetLastError() == ERROR_SUCCESS);
// }

BOOL RunTokenMgr(LPWSTR lpCmdLine, LPPROCESS_INFORMATION lppi)
{    

  HDESK    hdesk   = NULL;
  HWINSTA  hwinsta = NULL, hwinstaSave = NULL;

  PSID pSid = NULL;
  BOOL ret  = FALSE;

  // Get token of the current user
  HANDLE hToken = GetCurrentUserToken();    
  if (NULL == hToken || hToken == INVALID_HANDLE_VALUE)
    return FALSE;

  if ((hwinstaSave = GetProcessWindowStation()) == NULL)
    goto cleanup;    

  // Get a handle to the interactive window station.
  hwinsta = OpenWindowStation(
    L"WinSta0",                   // the interactive window station 
    FALSE,                       // handle is not inheritable
    READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL

  if (hwinsta == NULL) 
    goto cleanup;

  // To get the correct default desktop, set the caller's 
  // window station to the interactive window station.
  if (!SetProcessWindowStation(hwinsta))
    goto cleanup;

  // Get a handle to the interactive desktop.
  hdesk = OpenDesktop(
    L"Default",     // the interactive window station 
    0,             // no interaction with other desktop processes
    FALSE,         // handle is not inheritable
    READ_CONTROL | // request the rights to read and write the DACL
    WRITE_DAC | 
    DESKTOP_WRITEOBJECTS | 
    DESKTOP_READOBJECTS);

  // Restore the caller's window station.
  if (!SetProcessWindowStation(hwinstaSave)) 
    goto cleanup;

  if (hdesk == NULL) 
    goto cleanup;

  // Get the SID for the client's logon session.
  if (!GetLogonSID(hToken, &pSid)) 
    goto cleanup;

  // Allow logon SID full access to interactive window station.
  if (!AddAceToWindowStation(hwinsta, pSid) ) 
    goto cleanup;

  // Allow logon SID full access to interactive desktop.
  if (!AddAceToDesktop(hdesk, pSid) ) 
    goto cleanup;

  void* lpEnvironment; 

  // Get all necessary environment variables of logged in user
  // to pass them to the process
  if (CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE) == 0) {
      CServiceBase::GetService()->WriteErrorLogEntry(L"CreateEnvironmentBlock");
      goto cleanup;
  }

  // Impersonate client to ensure access to executable file.
  if (!ImpersonateLoggedOnUser(hToken) ) 
    goto cleanup;

  STARTUPINFO si = { sizeof(si) };
  ZeroMemory(&si, sizeof(STARTUPINFO)); 
  si.lpDesktop    = L"WinSta0\\Default";
  si.dwFlags      = STARTF_USESHOWWINDOW;
  si.wShowWindow  = SW_NORMAL;

  // Start the process on behalf of the current user 
  if(!(ret = CreateProcessAsUser(hToken, lpCmdLine, 
              NULL, NULL,  NULL, FALSE, 
              // CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT,
              NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,   // creation flags 
              lpEnvironment, NULL, &si, lppi)))
    CServiceBase::GetService()->WriteErrorLogEntry(L"CreateProcessAsUser"); 

  DestroyEnvironmentBlock(lpEnvironment);

  // Close the handle to the client's access token.
  if(hToken != INVALID_HANDLE_VALUE)
    CloseHandle(hToken);  

  // End impersonation of client.
  RevertToSelf();

  cleanup: 

  if (hwinstaSave != NULL)
    SetProcessWindowStation(hwinstaSave);

  // Free the buffer for the logon SID.

  if (pSid)
    FreeLogonSID(&pSid);

  // Close the handles to the interactive window station and desktop.

  if (hwinsta)
    CloseWindowStation(hwinsta);

  if (hdesk)
    CloseDesktop(hdesk);

  return ret;
}

#if NTDDI_VERSION < NTDDI_VISTA

BOOL MyPostQuitMessage(UINT message)
{
  HANDLE hPipe = INVALID_HANDLE_VALUE;
  while (true) 
  { 
    hPipe = ::CreateFile(L"\\\\.\\pipe\\epTokend", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    
    if (hPipe != INVALID_HANDLE_VALUE)
      break;

    // If any error except the ERROR_PIPE_BUSY has occurred,
    // we should return FALSE. 
    if (::GetLastError() != ERROR_PIPE_BUSY) 
      return FALSE;

    // The named pipe is busy. Let’s wait for 5 seconds. 
    if (!WaitNamedPipe(L"\\\\.\\pipe\\epTokend", 5000)) 
      return FALSE;
  } 

  if(hPipe == NULL || INVALID_HANDLE_VALUE == hPipe) // WaitNamedPipe might have failed
	  return FALSE;

	DWORD dwBytesWritten = 0;
  if (WriteFile(hPipe, (LPVOID)&message, sizeof UINT, &dwBytesWritten, 0) && sizeof UINT == dwBytesWritten) {
	CloseHandle(hPipe);
    return TRUE;
  }

  return FALSE;
}

#endif

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid)
{
   ACCESS_ALLOWED_ACE   *pace = NULL;
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl = NULL;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the DACL for the window station.

      if (!GetUserObjectSecurity(
             hwinsta,
             &si,
             psd,
             dwSidSize,
             &dwSdSizeNeeded)
      )
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
         psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
               GetProcessHeap(),
               HEAP_ZERO_MEMORY,
               dwSdSizeNeeded);

         if (psd == NULL)
            __leave;

         psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
               GetProcessHeap(),
               HEAP_ZERO_MEMORY,
               dwSdSizeNeeded);

         if (psdNew == NULL)
            __leave;

         dwSidSize = dwSdSizeNeeded;

         if (!GetUserObjectSecurity(
               hwinsta,
               &si,
               psd,
               dwSidSize,
               &dwSdSizeNeeded)
         )
            __leave;
      }
      else
         __leave;

      // Create a new DACL.

      if (!InitializeSecurityDescriptor(
            psdNew,
            SECURITY_DESCRIPTOR_REVISION)
      )
         __leave;

      // Get the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
            psd,
            &bDaclPresent,
            &pacl,
            &bDaclExist)
      )
         __leave;

      // Initialize the ACL.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if the DACL is not NULL.

      if (pacl != NULL)
      {
         // get the file ACL size info
         if (!GetAclInformation(
               pacl,
               (LPVOID)&aclSizeInfo,
               sizeof(ACL_SIZE_INFORMATION),
               AclSizeInformation)
         )
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse +
            (2*sizeof(ACCESS_ALLOWED_ACE)) + (2*GetLengthSid(psid)) -
            (2*sizeof(DWORD));

      // Allocate memory for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new DACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(
                     pNewAcl,
                     ACL_REVISION,
                     MAXDWORD,
                     pTempAce,
                    ((PACE_HEADER)pTempAce)->AceSize)
               )
                  __leave;
            }
         }
      }

      // Add the first ACE to the window station.

      pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
                  sizeof(DWORD));

      if (pace == NULL)
         __leave;

      pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
      pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
                   INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
      pace->Header.AceSize  = LOWORD(sizeof(ACCESS_ALLOWED_ACE) +
                   GetLengthSid(psid) - sizeof(DWORD));
      pace->Mask            = GENERIC_ACCESS;

      if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
         __leave;

      if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            (LPVOID)pace,
            pace->Header.AceSize)
      )
         __leave;

      // Add the second ACE to the window station.

      pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
      pace->Mask            = WINSTA_ALL;

      if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            (LPVOID)pace,
            pace->Header.AceSize)
      )
         __leave;

      // Set a new DACL for the security descriptor.

      if (!SetSecurityDescriptorDacl(
            psdNew,
            TRUE,
            pNewAcl,
            FALSE)
      )
         __leave;

      // Set the new security descriptor for the window station.

      if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free the allocated buffers.

      if (pace != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
   }

   return bSuccess;

}

BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
{
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl = NULL;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the security descriptor for the desktop object.

      if (!GetUserObjectSecurity(
            hdesk,
            &si,
            psd,
            dwSidSize,
            &dwSdSizeNeeded))
      {
         if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
         {
            psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                  GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  dwSdSizeNeeded );

            if (psd == NULL)
               __leave;

            psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                  GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  dwSdSizeNeeded);

            if (psdNew == NULL)
               __leave;

            dwSidSize = dwSdSizeNeeded;

            if (!GetUserObjectSecurity(
                  hdesk,
                  &si,
                  psd,
                  dwSidSize,
                  &dwSdSizeNeeded)
            )
               __leave;
         }
         else
            __leave;
      }

      // Create a new security descriptor.

      if (!InitializeSecurityDescriptor(
            psdNew,
            SECURITY_DESCRIPTOR_REVISION)
      )
         __leave;

      // Obtain the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
            psd,
            &bDaclPresent,
            &pacl,
            &bDaclExist)
      )
         __leave;

      // Initialize.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if NULL DACL.

      if (pacl != NULL)
      {
         // Determine the size of the ACL information.

         if (!GetAclInformation(
               pacl,
               (LPVOID)&aclSizeInfo,
               sizeof(ACL_SIZE_INFORMATION),
               AclSizeInformation)
         )
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse +
            sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid(psid) - sizeof(DWORD);

      // Allocate buffer for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new ACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(
                  pNewAcl,
                  ACL_REVISION,
                  MAXDWORD,
                  pTempAce,
                  ((PACE_HEADER)pTempAce)->AceSize)
               )
                  __leave;
            }
         }
      }

      // Add ACE to the DACL.

      if (!AddAccessAllowedAce(
            pNewAcl,
            ACL_REVISION,
            DESKTOP_ALL,
            psid)
      )
         __leave;

      // Set new DACL to the new security descriptor.

      if (!SetSecurityDescriptorDacl(
            psdNew,
            TRUE,
            pNewAcl,
            FALSE)
      )
         __leave;

      // Set the new security descriptor for the desktop object.

      if (!SetUserObjectSecurity(hdesk, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free buffers.

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
   }

   return bSuccess;
}

BOOL GetLogonSID(HANDLE hToken, PSID *ppsid) 
{

#if NTDDI_VERSION >= NTDDI_VISTA

  DWORD dwLength = 0;
  PTOKEN_GROUPS pGroups = NULL;
  if(NULL == ppsid || NULL == hToken)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  GetTokenInformation(hToken, TokenLogonSid, NULL, 0, &dwLength);
  if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return FALSE;

  if((pGroups = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), 0, dwLength)) == NULL)
    return FALSE;

  if(!GetTokenInformation(hToken, TokenLogonSid, pGroups, dwLength, &dwLength))
  {
    HeapFree(GetProcessHeap(), 0, pGroups);
    return FALSE;
  }

  RtlMoveMemory(pGroups, pGroups->Groups[0].Sid, GetLengthSid(pGroups->Groups[0].Sid));
  *ppsid = pGroups;

  return TRUE;

#else

   BOOL bSuccess = FALSE;
   DWORD dwIndex;
   DWORD dwLength = 0;
   PTOKEN_GROUPS ptg = NULL;

    // Verify the parameter passed in is not NULL.
    if (NULL == ppsid)
        goto Cleanup;

   // Get required buffer size and allocate the TOKEN_GROUPS buffer.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups 
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         0,              // size of buffer
         &dwLength       // receives required buffer size
      )) 
   {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
         goto Cleanup;

      ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, dwLength);

      if (ptg == NULL)
         goto Cleanup;
   }

   // Get the token group information from the access token.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups 
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         dwLength,       // size of buffer
         &dwLength       // receives required buffer size
         )) 
   {
      goto Cleanup;
   }

   // Loop through the groups to find the logon SID.

   for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) 
      if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
             ==  SE_GROUP_LOGON_ID) 
      {
      // Found the logon SID; make a copy of it.

         dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
         *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                     HEAP_ZERO_MEMORY, dwLength);
         if (*ppsid == NULL)
             goto Cleanup;
         if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) 
         {
             HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
             goto Cleanup;
         }
         break;
      }

   bSuccess = TRUE;

   Cleanup: 

   // Free the buffer for the token groups.

   if (ptg != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);

   return bSuccess;

#endif

}

VOID FreeLogonSID (PSID *ppsid) 
{
  HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
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