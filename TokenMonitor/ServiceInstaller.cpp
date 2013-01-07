/****************************** Module Header ******************************\
* Module Name:  ServiceInstaller.cpp
* Project:      CppWindowsService
* Copyright (c) Microsoft Corporation.
* 
* The file implements functions that install and uninstall the service.
* 
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
* 
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma region "Includes"

#include "inc\config.h"

#include <stdio.h>
#include <string>
#include <windows.h>
#include <sddl.h>
#include "inc\ServiceInstaller.h"

#include "accctrl.h"
#include "aclapi.h"

#pragma endregion

// Formats a message string using the specified message and variable
// list of arguments.
LPWSTR GetLastErrorMessage();


BOOL ChangeDelayedAutoStart(
    SC_HANDLE service, BOOL delayed)
{
#if NTDDI_VERSION >= NTDDI_VISTA

  SERVICE_DELAYED_AUTO_START_INFO info = { delayed };

  return 0 != ::ChangeServiceConfig2(
      service, 
      SERVICE_CONFIG_DELAYED_AUTO_START_INFO, 
      &info);

#else

  return TRUE;

#endif
}


BOOL ChangeFailureActionsOnNonCrashFailures(
  SC_HANDLE service,
  BOOL failureActionsOnNonCrashFailures)
{
#if NTDDI_VERSION >= NTDDI_VISTA  

  SERVICE_FAILURE_ACTIONS_FLAG flag = { 
   failureActionsOnNonCrashFailures };

  return 0 != ::ChangeServiceConfig2(service,
    SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
    &flag);

#else

  return TRUE;

#endif  
}

BOOL ChangeRequiredPrivileges(
    SC_HANDLE service,
    const LPWSTR requiredPrivileges)
{
#if NTDDI_VERSION >= NTDDI_VISTA  


  SERVICE_REQUIRED_PRIVILEGES_INFO info = { requiredPrivileges };

  return 0 != ::ChangeServiceConfig2(
    service,
    SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO,
    &info);

#else

  return TRUE;

#endif
}

BOOL ChangeSidType(SC_HANDLE service,
  DWORD sidType)
{
#if NTDDI_VERSION >= NTDDI_VISTA  

  SERVICE_SID_INFO info = { sidType };

  return 0 != ::ChangeServiceConfig2(service,
    SERVICE_CONFIG_SERVICE_SID_INFO,
    &info);

#else

  return TRUE;

#endif  
}

//
//   FUNCTION: InstallService
//
//   PURPOSE: Install the current application as a service to the local 
//   service control manager database.
//
//   PARAMETERS:
//   * pszServiceName - the name of the service to be installed
//   * pszDisplayName - the display name of the service
//   * dwStartType - the service start option. This parameter can be one of 
//     the following values: SERVICE_AUTO_START, SERVICE_BOOT_START, 
//     SERVICE_DEMAND_START, SERVICE_DISABLED, SERVICE_SYSTEM_START.
//   * pszDependencies - a pointer to a double null-terminated array of null-
//     separated names of services or load ordering groups that the system 
//     must start before this service.
//   * pszAccount - the name of the account under which the service runs.
//   * pszPassword - the password to the account name.
//
//   NOTE: If the function fails to install the service, it prints the error 
//   in the standard output stream for users to diagnose the problem.
//
void InstallService(PWSTR pszServiceName, 
                    PWSTR pszDisplayName, 
          PWSTR pszDescription, 
                    DWORD dwStartType,
                    PWSTR pszDependencies, 
                    PWSTR pszAccount, 
                    PWSTR pszPassword) {

    // EXPLICIT_ACCESS      ea;
    // SECURITY_DESCRIPTOR  sd;
    // PSECURITY_DESCRIPTOR psd            = NULL;
    // PACL                 pacl           = NULL;
    // PACL                 pNewAcl        = NULL;
    // BOOL                 bDaclPresent   = FALSE;
    // BOOL                 bDaclDefaulted = FALSE;
    // DWORD                dwError        = 0;
    // DWORD                dwSize         = 0;
    // DWORD                dwBytesNeeded  = 0;

    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0) {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto cleanup;
    }

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | 
        SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL) {
        wprintf(L"OpenSCManager failed w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage());
        goto cleanup;
    }

    // Install the service into SCM by calling CreateService
    schService = CreateService(
        schSCManager,                   // SCManager database
        pszServiceName,                 // Name of service
        pszDisplayName,                 // Name to display
        (SERVICE_ALL_ACCESS | READ_CONTROL | WRITE_DAC) & ~SERVICE_PAUSE_CONTINUE, //SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | READ_CONTROL | WRITE_DAC,           // Desired access
        SERVICE_WIN32_OWN_PROCESS,      // Service type
        dwStartType,                    // Service start type
        SERVICE_ERROR_NORMAL,           // Error control type
        szPath,                         // Service's binary
        NULL,                           // No load ordering group
        NULL,                           // No tag identifier
        pszDependencies,                // Dependencies
        pszAccount,                     // Service running account
        pszPassword                     // Password of the account
       );

    if (schService == NULL) {
        wprintf(L"CreateService failed w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage());
        goto cleanup;
    }

    SERVICE_DESCRIPTION desc = { pszDescription };
    ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &desc);
 
    SC_ACTION actions[3];

    actions[0].Type  = SC_ACTION_RESTART;
    actions[0].Delay = 10000;
    
    actions[1].Type  = SC_ACTION_RESTART;
    actions[1].Delay = 30000;
    
    actions[2].Type  = SC_ACTION_NONE;

    SERVICE_FAILURE_ACTIONS sfa;
    sfa.dwResetPeriod = 86400;
    sfa.lpCommand     = NULL;
    sfa.lpRebootMsg   = NULL;
    sfa.cActions      = ARRAYSIZE(actions);
    sfa.lpsaActions   = actions;
 
    ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);

    ChangeDelayedAutoStart(schService, TRUE);
    ChangeFailureActionsOnNonCrashFailures(schService, TRUE);

  	// LPWSTR privileges = 
  	// 	SE_CHANGE_NOTIFY_NAME 
  	// 	SE_IMPERSONATE_NAME
  	// 	SE_TCB_NAME
  	// 	L"\0"
   //  ;

   // ChangeRequiredPrivileges(schService, privileges);

    // ChangeSidType(schService, SERVICE_SID_TYPE_RESTRICTED);

    // // Get the current security descriptor.

    // if (!QueryServiceObjectSecurity(schService,
    //             DACL_SECURITY_INFORMATION, 
    //             &psd,           // using NULL does not work on all versions
    //             0, 
    //             &dwBytesNeeded))
    // {
    //     if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    //     {
    //         dwSize = dwBytesNeeded;
    //         psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
    //                 HEAP_ZERO_MEMORY, dwSize);
    //         if (psd == NULL)
    //         {
    //             // Note: HeapAlloc does not support GetLastError.
    //             wprintf(L"HeapAlloc failed\n");
    //             goto cleanup;
    //         }
  
    //         if (!QueryServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, psd, dwSize, &dwBytesNeeded))
    //         {
    //             wprintf(L"QueryServiceObjectSecurity failed (%d)\n", GetLastError());
    //             goto cleanup;
    //         }
    //     }
    //     else 
    //     {
    //         wprintf(L"QueryServiceObjectSecurity failed (%d)\n", GetLastError());
    //         goto cleanup;
    //     }
    // }

    // // Get the DACL.

    // if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclDefaulted))
    // {
    //     wprintf(L"GetSecurityDescriptorDacl failed(%d)\n", GetLastError());
    //     goto cleanup;
    // }

    // // Build the ACE.

    // BuildExplicitAccessWithName(&ea, L"GUEST",
    //     SERVICE_START | SERVICE_STOP | READ_CONTROL | DELETE,
    //     SET_ACCESS, NO_INHERITANCE);

    // dwError = SetEntriesInAcl(1, &ea, pacl, &pNewAcl);
    // if (dwError != ERROR_SUCCESS)
    // {
    //     wprintf(L"SetEntriesInAcl failed(%d)\n", dwError);
    //     goto cleanup;
    // }

    // // Initialize a new security descriptor.

    // if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    // {
    //     wprintf(L"InitializeSecurityDescriptor failed(%d)\n", GetLastError());
    //     goto cleanup;
    // }

    // // Set the new DACL in the security descriptor.

    // if (!SetSecurityDescriptorDacl(&sd, TRUE, pNewAcl, FALSE))
    // {
    //     wprintf(L"SetSecurityDescriptorDacl failed(%d)\n", GetLastError());
    //     goto cleanup;
    // }

    // // Set the new DACL for the service object.

    // if (!SetServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, &sd))
    // {
    //     wprintf(L"SetServiceObjectSecurity failed(%d)\n", GetLastError());
    //     goto cleanup;
    // }
    // else wprintf(L"Service DACL updated successfully\n");    

    WCHAR szSD[] = L"D:"
           L"(A;;CCLCSWRPWPDTLOCRRC;;;SY)"           // default permissions for local system
           L"(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)"   // default permissions for administrators
           L"(A;;CCLCSWLOCRRC;;;AU)"                 // default permissions for authenticated users
           L"(A;;CCLCSWLOCRRC;;;BG)"                 // default permissions for built-in guests
           L"(A;;CCLCSWRPWPDTLOCRRC;;;PU)"           // default permissions for power users
           L"(A;;RP;;;IU)"                           // added permission: start service for interactive users
           ;

    // Define the SDDL for the DACL. This example sets 
    // the following access:
    //     Built-in guests are denied all access.
    //     Anonymous logon is denied all access.
    //     Authenticated users are allowed 
    //     read/write/execute access.
    //     Administrators are allowed full control.
    // Modify these values as needed to generate the proper
    // DACL for your application. 
    // WCHAR *szSD = L"D:"       // Discretionary ACL

    //     L"(A;OICI;GA;;;BG)"     // Allow full access to built-in guests

    //     L"(A;OICI;GA;;;AN)"     // Allow full access to anonymous logon

    //     L"(A;OICI;GA;;;AU)"     // Allow full access to authenticated users

    //     L"(A;OICI;GA;;;IU)"     // Allow full control to interactive users

    //     L"(A;OICI;GA;;;BA)";    // Allow full control to administrators

  PSECURITY_DESCRIPTOR psd;

  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &psd, NULL)) {
    wprintf(L"ConvertStringSecurityDescriptorToSecurityDescriptor failed(%d) - %s\n", GetLastError(), GetLastErrorMessage());
    goto cleanup;
  }

  if (!SetServiceObjectSecurity(schService, DACL_SECURITY_INFORMATION, psd)) {
    wprintf(L"SetServiceObjectSecurity failed(%d) - %s\n", GetLastError(), GetLastErrorMessage());
  }

  wprintf(L"%s is installed.\n", pszServiceName);

cleanup:  

    // Centralized cleanup for all allocated resources.
    if (schSCManager) {
      CloseServiceHandle(schSCManager);
      schSCManager = NULL;
    }

    if (schService) {
      CloseServiceHandle(schService);
      schService = NULL;
    }

    // if(NULL != pNewAcl)
      // LocalFree((HLOCAL)pNewAcl);

    // if(NULL != psd)
      // HeapFree(GetProcessHeap(), 0, (LPVOID)psd);    
}


//
//   FUNCTION: UninstallService
//
//   PURPOSE: Stop and remove the service from the local service control 
//   manager database.
//
//   PARAMETERS: 
//   * pszServiceName - the name of the service to be removed.
//
//   NOTE: If the function fails to uninstall the service, it prints the 
//   error in the standard output stream for users to diagnose the problem.
//
void UninstallService(PWSTR pszServiceName) {
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        wprintf(L"OpenSCManager failed w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage());
        goto Cleanup;
    }

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | 
        SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL) {
        wprintf(L"OpenService failed w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage());
        goto Cleanup;
    }

    // Try to stop the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus)) {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssSvcStatus)) {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                wprintf(L".");
                Sleep(1000);
            }
            else break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED) {
            wprintf(L"\n%s is stopped.\n", pszServiceName);
        }
        else {
            wprintf(L"\n%s failed to stop.\n", pszServiceName);
        }
    }

    // Now remove the service by calling DeleteService.
    if (!DeleteService(schService)) {
        wprintf(L"DeleteService failed w/err 0x%08lx(%s)\n", GetLastError(), GetLastErrorMessage());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (schSCManager) {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService) {
        CloseServiceHandle(schService);
        schService = NULL;
    }
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

//
// Purpose: 
//   Starts the service if possible.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID DoStartSvc(PWSTR szSvcName)
{
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    // Get a handle to the SCM database. 
 
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // servicesActive database 
        SC_MANAGER_CONNECT);  // full access rights 
 
    if (NULL == schSCManager) 
    {
        wprintf(L"OpenSCManager failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        return;
    }

    // Get a handle to the service.

    schService = OpenService(
        schSCManager,         // SCM database 
        szSvcName,            // name of service 
        SERVICE_START | SERVICE_QUERY_STATUS);  // full access 
 
    if (schService == NULL)
    { 
        wprintf(L"OpenService failed (%d) - %s\n", GetLastError(), GetLastErrorMessage()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Check the status in case the service is not stopped. 

    if (!QueryServiceStatusEx(
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // information level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // size needed if buffer is too small
    {
        wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }

    // Check if the service is already running. It would be possible 
    // to stop the service here, but for simplicity this example just returns. 

    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        wprintf(L"Cannot start the service because it is already running\n");
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if(dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status until the service is no longer stop pending. 
 
        if (!QueryServiceStatusEx(
                schService,                     // handle to service 
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE) &ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded))              // size needed if buffer is too small
        {
            wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
            CloseServiceHandle(schService); 
            CloseServiceHandle(schSCManager);
            return; 
        }

        if(ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                wprintf(L"Timeout waiting for service to stop\n");
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager);
                return; 
            }
        }
    }

    // Attempt to start the service.

    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL))      // no arguments 
    {
        wprintf(L"StartService failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }
    else wprintf(L"Service start pending...\n"); 

    // Check the status until the service is no longer start pending. 
 
    if (!QueryServiceStatusEx(
            schService,                     // handle to service 
            SC_STATUS_PROCESS_INFO,         // info level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // if buffer too small
    {
        wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        CloseServiceHandle(schService); 
        CloseServiceHandle(schSCManager);
        return; 
    }
 
    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth the wait hint, but no less than 1 second and no 
        // more than 10 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if(dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        // Check the status again. 
 
        if (!QueryServiceStatusEx(
            schService,             // handle to service 
            SC_STATUS_PROCESS_INFO, // info level
            (LPBYTE) &ssStatus,             // address of structure
            sizeof(SERVICE_STATUS_PROCESS), // size of structure
            &dwBytesNeeded))              // if buffer too small
        {
            wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
            break; 
        }
 
        if(ssStatus.dwCheckPoint > dwOldCheckPoint)
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    } 

    // Determine whether the service is running.

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        wprintf(L"Service started successfully.\n"); 
    }
    else 
    { 
        wprintf(L"Service not started. \n");
        wprintf(L"  Current State: %d\n", ssStatus.dwCurrentState); 
        wprintf(L"  Exit Code: %d\n", ssStatus.dwWin32ExitCode); 
        wprintf(L"  Check Point: %d\n", ssStatus.dwCheckPoint); 
        wprintf(L"  Wait Hint: %d\n", ssStatus.dwWaitHint); 
    } 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Stops the service.
//
// Parameters:
//   None
// 
// Return value:
//   None
//
VOID DoStopSvc(PWSTR szSvcName)
{
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    // Get a handle to the SCM database. 
 
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_CONNECT);  // full access rights 
 
    if (NULL == schSCManager) 
    {
        wprintf(L"OpenSCManager failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        return;
    }

    // Get a handle to the service.

    schService = OpenService(
        schSCManager,         // SCM database 
        szSvcName,            // name of service 
        SERVICE_STOP | 
        SERVICE_QUERY_STATUS | 
        SERVICE_ENUMERATE_DEPENDENTS);  
 
    if (schService == NULL)
    { 
        wprintf(L"OpenService failed (%d) - %s\n", GetLastError, GetLastErrorMessage());
        CloseServiceHandle(schSCManager);
        return;
    }    

    // Make sure the service is not already stopped.

    if(!QueryServiceStatusEx(
            schService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded))
    {
        wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage()); 
        goto cleanup;
    }

    if(ssp.dwCurrentState == SERVICE_STOPPED)
    {
        wprintf(L"Service is already stopped.\n");
        goto cleanup;
    }

    // If a stop is pending, wait for it.

    while(ssp.dwCurrentState == SERVICE_STOP_PENDING) 
    {
        wprintf(L"Service stop pending...\n");

        // Do not wait longer than the wait hint. A good interval is 
        // one-tenth of the wait hint but not less than 1 second  
        // and not more than 10 seconds. 
 
        dwWaitTime = ssp.dwWaitHint / 10;

        if(dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if(dwWaitTime > 10000)
            dwWaitTime = 10000;

        Sleep(dwWaitTime);

        if(!QueryServiceStatusEx(
                 schService, 
                 SC_STATUS_PROCESS_INFO,
                 (LPBYTE)&ssp, 
                 sizeof(SERVICE_STATUS_PROCESS),
                 &dwBytesNeeded))
        {
            wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage()); 
            goto cleanup;
        }

        if(ssp.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"Service stopped successfully.\n");
            goto cleanup;
        }

        if(GetTickCount() - dwStartTime > dwTimeout)
        {
            wprintf(L"Service stop timed out.\n");
            goto cleanup;
        }
    }

    // If the service is running, dependencies must be stopped first.

    // StopDependentServices();

    // Send a stop code to the service.

    if(!ControlService(
            schService, 
            SERVICE_CONTROL_STOP, 
            (LPSERVICE_STATUS) &ssp))
    {
        wprintf(L"ControlService failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
        goto cleanup;
    }

    // Wait for the service to stop.

    while (ssp.dwCurrentState != SERVICE_STOPPED) 
    {
        Sleep(ssp.dwWaitHint);
        if(!QueryServiceStatusEx(
                schService, 
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp, 
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded))
        {
            wprintf(L"QueryServiceStatusEx failed (%d) - %s\n", GetLastError(), GetLastErrorMessage());
            goto cleanup;
        }

        if(ssp.dwCurrentState == SERVICE_STOPPED)
            break;

        if(GetTickCount() - dwStartTime > dwTimeout)
        {
            wprintf(L"Wait timed out\n");
            goto cleanup;
        }
    }
    wprintf(L"Service stopped successfully\n");

cleanup:
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}