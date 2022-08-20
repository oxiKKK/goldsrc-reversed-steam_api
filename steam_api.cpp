//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// 
// Steam API interface
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Safe initialization for steamAPI code
//-----------------------------------------------------------------------------
bool SteamAPI_InitSafe()
{
	return SteamAPI_InitInternal(true);
}

//-----------------------------------------------------------------------------
// Purpose: Regular initialization for steamAPI code
//-----------------------------------------------------------------------------
bool SteamAPI_Init()
{
	return SteamAPI_InitInternal(false);
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down all code associated to steam API
//-----------------------------------------------------------------------------
void SteamAPI_Shutdown()
{
	g_pSteamUtilsRunFrame = nullptr;

	if (g_hSteamPipe && g_hSteamUser)
		g_pSteamClient->ReleaseUser(g_hSteamPipe, g_hSteamUser);

	g_hSteamUser = 0;

	// Set all pointers to NULL
	g_SteamAPIContext.Clear();

	if (g_hSteamPipe)
		g_pSteamClient->BReleaseSteamPipe(g_hSteamPipe);

	g_hSteamPipe = 0;

	if (g_pSteamClient)
		g_pSteamClient->BShutdownIfAllPipesClosed();

	g_pSteamClient = nullptr;

	SteamAPI_Shutdown_Internal(g_hSteamClientModule);
	g_hSteamClientModule = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Executes shell command to start the steam executable with same 
//			command-line parameters.
//-----------------------------------------------------------------------------
bool SteamAPI_RestartAppIfNecessary(uint32 unOwnAppID)
{
	char	szSteamAppID[32];
	DWORD	dwSteamAppID;
	char	szInstallPath[MAX_PATH];
	BOOL	bSuccess;
	char	szCmdLine[1024];
	LPSTR	lpszCmdLine, lpszStr;

	if (!unOwnAppID)
		return false;

	dwSteamAppID = GetEnvironmentVariableA("SteamAppId", szSteamAppID, sizeof(szSteamAppID));

	// Check against stored environmental variable, it must exist because of
	// later app launching through steam.exe client.
	if (dwSteamAppID - 1 <= 30 && atoi(szSteamAppID) != NULL)
		return false;

	// Extract app id from appid text file
	if (GetSteamAppID("steam_appid.txt") != NULL)
		return false;

	*szInstallPath = '\0';
	memset(szInstallPath + 1, NULL, sizeof(szInstallPath) - 1);

	// Try to look up for the install path in registry hive
	bSuccess = GetRegistryValue("Software\\Valve\\Steam", "InstallPath", (LPBYTE)szInstallPath, sizeof(szInstallPath));

	if (bSuccess != TRUE)
		return false;

	// Now we'll be at steam.exe directory
	strncat(szInstallPath, "\\steam.exe", (sizeof(szInstallPath) - 1) - strlen(szInstallPath));

	*szCmdLine = '\0';
	snprintf(szCmdLine, sizeof(szCmdLine), "-applaunch %u", unOwnAppID);
	szCmdLine[sizeof(szCmdLine) - 1] = '\0';

	lpszCmdLine = GetCommandLineA();

	if (lpszCmdLine && *lpszCmdLine)
	{
		if (*lpszCmdLine == '"')
		{
			lpszStr = lpszCmdLine + 1;

			// Locate token "
			while (*lpszStr != '"')
				lpszStr++;

			if (*lpszStr)
				lpszStr++;
		}

		// Locate whitespace token
		while (*lpszStr != ' ')
			lpszStr++;

		if (*lpszStr == ' ')
			strncat(szCmdLine, lpszStr, (sizeof(szCmdLine) - 1) - strlen(szCmdLine));
	}

	// Try to execute the app with commandline parameters
	bSuccess = ((uint32)ShellExecuteA(NULL, nullptr, szInstallPath, szCmdLine, nullptr, 1) > 32) ? TRUE : FALSE;

	return (bSuccess == TRUE) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if steam's active process is running.
//-----------------------------------------------------------------------------
bool SteamAPI_IsSteamRunning()
{
	DWORD	dwSteamPID, cbData;
	HANDLE	hProcess;
	DWORD	dwExitCode;
	BOOL	bIsRunning;

	cbData = sizeof(dwSteamPID);

	// Try to get steam process process id from registry
	GetRegistryValue("Software\\Valve\\Steam\\ActiveProcess", "pid", (LPBYTE)&dwSteamPID, cbData);

	// Open steam process and query informaton from it
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, NULL, dwSteamPID);

	if (!hProcess) // Not running
		return false;

	bIsRunning = FALSE;
	dwExitCode = NULL;

	// Check for exit code
	if (GetExitCodeProcess(hProcess, &dwExitCode) != ERROR_SUCCESS)
	{
		// No more data is available
		if (dwExitCode == ERROR_NO_MORE_ITEMS)
			bIsRunning = TRUE;
	}

	CloseHandle(hProcess);

	return (bIsRunning == TRUE) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Sets g_szSteamInstallPath global variable. If the function fails, 
//			the global variable is left nonset. The global variable is returned.
//-----------------------------------------------------------------------------
const char *SteamAPI_GetSteamInstallPath()
{
	HKEY	hKey;
	LSTATUS lStatus;
	DWORD	dwSteamPID, cbData;
	HANDLE	hProcess;
	BOOL	bIsRunning;
	DWORD	dwExitCode;

	*g_szSteamInstallPath = '\0';

	// Get key for HKEY_CURRENT_USER
	hKey = RegistryKeyByName("HKCU");

	lStatus = RegOpenKeyExA(hKey, "Software\\Valve\\Steam\\ActiveProcess", NULL, KEY_READ, &hKey);
	if (lStatus != NO_ERROR)
		return g_szSteamInstallPath;

	dwSteamPID = NULL;
	cbData = sizeof(dwSteamPID);

	// Locate pid of steam active process
	lStatus = RegQueryValueExA(hKey, "pid", NULL, NULL, (LPBYTE)&dwSteamPID, &cbData);
	if (lStatus != NO_ERROR)
		return g_szSteamInstallPath;
	
	// Open steam process and query informaton from it
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, NULL, dwSteamPID);

	if (hProcess)
	{
		dwExitCode = NULL;

		// Check for exit code
		if (GetExitCodeProcess(hProcess, &dwExitCode) != ERROR_SUCCESS)
		{
			// No more data is available
			if (dwExitCode == ERROR_NO_MORE_ITEMS)
				bIsRunning = TRUE;
		}

		// We don't need the process handle anymore at this point
		CloseHandle(hProcess);

		if (bIsRunning)
		{
			cbData = sizeof(g_szSteamInstallPath);

			// Try to locate steamclient path
			lStatus = RegQueryValueExA(hKey, "SteamClientDll", NULL, NULL, (LPBYTE)&dwSteamPID, &cbData);

			if (lStatus != NO_ERROR && cbData > NULL && strlen(g_szSteamInstallPath) > NULL)
			{
				Q_StripFilename(g_szSteamInstallPath);
			}
			else
			{
				// Invalid data
				*g_szSteamInstallPath = '\0';
			}
		}
	}

	RegCloseKey(hKey);
	return g_szSteamInstallPath;
}

//-----------------------------------------------------------------------------
// Purpose: Returns handle to theglobal variable g_hSteamUser.
//-----------------------------------------------------------------------------
HSteamUser SteamAPI_GetHSteamUser()
{
	return g_hSteamUser;
}

//-----------------------------------------------------------------------------
// Purpose: Returns handle to the global variable g_hSteamPipe.
//-----------------------------------------------------------------------------
HSteamPipe SteamAPI_GetHSteamPipe()
{
	return g_hSteamPipe;
}

//-----------------------------------------------------------------------------
// Purpose: TODO
//-----------------------------------------------------------------------------
void Steam_RegisterInterfaceFuncs(void* hModule)
{
	CallbackMgr_RegisterInterfaceFuncs(reinterpret_cast<HMODULE>(hModule));
}

//-----------------------------------------------------------------------------
// Purpose: TODO
//-----------------------------------------------------------------------------
void Steam_RunCallbacks(HSteamPipe SteamPipe, bool bGameServerCallbacks)
{
	CallbackMgr_RunCallbacks(SteamPipe, bGameServerCallbacks);
}

//-----------------------------------------------------------------------------
// 
// SteamAPI callback interface layer
// 
//	A wrapper around CCallbackMgr class.
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SteamAPI_RunCallbacks()
{
	ISteamUtils* pSteamUtils;

	if (g_hSteamPipe)
		CallbackMgr_RunCallbacks(g_hSteamPipe, false);

	if (!g_pSteamClient)
		return;

	pSteamUtils = g_pSteamClient->GetISteamUtils(g_hSteamPipe, STEAMUTILS_INTERFACE_VERSION);

	if (!g_pSteamUtilsRunFrame)
		g_pSteamUtilsRunFrame = pSteamUtils;

	if (g_pSteamUtilsRunFrame || pSteamUtils->GetAppID() != k_uAppIdInvalid)
		g_pSteamUtilsRunFrame->RunFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SteamAPI_RegisterCallback(CCallbackBase *pCallback, int iCallback)
{
	CallbackMgr_RegisterCallback(pCallback, iCallback);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SteamAPI_UnregisterCallback(CCallbackBase *pCallback)
{
	CallbackMgr_UnregisterCallback(pCallback);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SteamAPI_RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall)
{
	CallbackMgr_RegisterCallResult(pCallback, hAPICall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SteamAPI_UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall)
{
	CallbackMgr_UnregisterCallResult(pCallback, hAPICall);
}

//-----------------------------------------------------------------------------
// Purpose: Setter for global variable g_bCatchExceptionsInCallbacks. 
//-----------------------------------------------------------------------------
void SteamAPI_SetTryCatchCallbacks(bool bCatchCallbacks)
{
	g_bCatchExceptionsInCallbacks = bCatchCallbacks;
}

//-----------------------------------------------------------------------------
// 
// SteamAPI breakpad/minidump layer
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Surface layer breakpad api. Sets global parameters for the crash
//			handler so that we can then forward this data into internal breakpad
//			API that is inside steamclient.
// 
// Note:	Called when -nobreakpad isn't set.
//-----------------------------------------------------------------------------
void SteamAPI_UseBreakpadCrashHandler(char const *pchVersion, char const *pchDate, char const *pchTime, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback)
{
	const char* pszMonths[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int			iYear, iMonth, iDay, iHour, iMinute, iSecond;

	printf("Using breakpad crash handler\n");

	// Using breakpad API from steamclient.dll
	s_BreakpadInfo = STEAM_BREAKPAD_STEAMCLIENT;

	// Set our breakpad options, so that when calling steam breakpad internal API
	// we can pass these in set by the user.
	g_bBreakpadFullMemoryDumps = bFullMemoryDumps;
	strncpy(g_pchBreakpadVersion, pchVersion, sizeof(g_pchBreakpadVersion));
	g_pchBreakpadVersion[sizeof(g_pchBreakpadVersion) - 1] = '\0';
	g_pvBreakpadContext = pvContext;
	g_pfnBreakpadPreMinidumpCallback = m_pfnPreMinidumpCallback;

	// Search for match in months
	for (iMonth = 1; iMonth <= Q_ARRAYSIZE(pszMonths); iMonth++)
	{
		if (!strnicmp(pchDate, pszMonths[iMonth], 3))
			break;
	}

	// Format timestamp
	iDay = atoi(pchDate + 4);
	iYear = atoi(pchDate + 7);
	iHour = 0;
	iMinute = 0;
	iSecond = 0;

	sscanf(pchTime, "%02d:%02d:%02d", &iHour, &iMinute, &iSecond);
	_snprintf(g_szBreakpadTimestamp, sizeof(g_szBreakpadTimestamp), "%04d%02d%02d%02d%02d%02d", iYear, iMonth, iDay, iHour, iMinute, iSecond);
}

//-----------------------------------------------------------------------------
// Purpose: Tries to call s_pfnSteamSetAppID() and loads minidump interface if 
//			hasn't been loaded already.
//-----------------------------------------------------------------------------
void SteamAPI_SetBreakpadAppID(uint32 unAppID)
{
	// On change or initially
	if (g_BreakpadLastAppId != unAppID)
	{
		printf("Setting breakpad minidump AppID = %u\n", unAppID);
		g_BreakpadLastAppId = unAppID;
	}

	if (unAppID != NULL && !s_pfnSteamMiniDumpFn && s_BreakpadInfo != STEAM_BREAKPAD_STEAM)
	{
		printf("Forcing breakpad minidump interfaces to load\n");

		// Load minidump interface either from steamclient.dll
		Steam_LoadMinidumpInterface();
	}

	if (s_pfnSteamSetAppID)
		s_pfnSteamSetAppID(g_BreakpadLastAppId);
}

//-----------------------------------------------------------------------------
// Purpose: Tries to call s_pfnSteamMiniDumpFn() routine
//-----------------------------------------------------------------------------
void SteamAPI_WriteMiniDump(uint32 uStructuredExceptionCode, void* pvExceptionInfo, uint32 uBuildID)
{
	// Try to load the interface if we haven't already
	if (!s_pfnSteamMiniDumpFn)
		Steam_LoadMinidumpInterface();

	if (s_pfnSteamMiniDumpFn)
		s_pfnSteamMiniDumpFn(uStructuredExceptionCode, pvExceptionInfo, uBuildID);
}

//-----------------------------------------------------------------------------
// Purpose: Tries to call s_pfnSteamWriteMiniDumpSetComment()
//-----------------------------------------------------------------------------
void SteamAPI_SetMiniDumpComment(const char *pchMsg)
{
	// Try to load the interface if we haven't already
	if (!s_pfnSteamWriteMiniDumpSetComment)
		Steam_LoadMinidumpInterface();

	if (s_pfnSteamWriteMiniDumpSetComment)
		s_pfnSteamWriteMiniDumpSetComment(pchMsg);
}