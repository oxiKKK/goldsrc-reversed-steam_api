//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// Purpose: Modules for steam.dll and steamclient.dll
//-----------------------------------------------------------------------------
CModuleLoadWrapper g_MiniDumpSteamDllModule = { k_pszSteamModuleName, NULL, false };
CModuleLoadWrapper g_MiniDumpSteamClientDllModule = { k_pszSteamClientModuleName, NULL, false };

//-----------------------------------------------------------------------------
// 
// Module load wrapper
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Loads a dll within steam main directory. Which dll this function 
//			loads is set when global object instance of this class is initialized.
//-----------------------------------------------------------------------------
void CModuleLoadWrapper::Load()
{
	char szModulePath[MAX_PATH];
	bool bSteamClientPath;

	m_hModule = GetModuleHandle(m_pszModulePath);

	// True if we've located steam client path
	bSteamClientPath = ConfigureSteamClientPath(nullptr, NULL);

	// The module was loaded, no further actions needed
	if (m_hModule)
		return;

	// Load the module within steam main directory
	if (bSteamClientPath)
	{
		m_bValid = true;
		snprintf(szModulePath, sizeof(szModulePath), "%s%c%s", g_szSteamClientPath, PLATFORM_SLASH, m_pszModulePath);
		m_hModule = reinterpret_cast<HMODULE>(Steam_LoadModule(szModulePath));
	}

	if (!m_hModule)
		m_hModule = reinterpret_cast<HMODULE>(Steam_LoadModule(m_pszModulePath));
}

//-----------------------------------------------------------------------------
// 
// Steam API module code
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Parses AppID integer from the file. This file is usually steam_appid
//			text file.
// 
// Note:	This function exists only on windows builds.
//-----------------------------------------------------------------------------
uint32 GetSteamAppID(const char *szSteamAppID)
{
	FILE*	file;
	char	szBuf[256];
	uint32	appID;

	file = fopen(szSteamAppID, "rb");

	if (!file)
		return NULL;

	*szBuf = '\0';
	memset(szBuf + 1, NULL, sizeof(szBuf) - 1);

	fgets(szBuf, sizeof(szBuf), file);
	szBuf[sizeof(szBuf) - 1] = '\0';

	appID = atoi(szBuf);
	fclose(file);

	return appID;
}

//-----------------------------------------------------------------------------
// Purpose: Returns registry key handle depending on the name.
// 
// Inputs:	HKEY_LOCAL_MACHINE or HKLM
//			HKEY_CURRENT_USER  or HKCU
//			HKEY_CLASSES_ROOT  or HKCR
// 
// Note:	This function exists only on windows builds.
//-----------------------------------------------------------------------------
HKEY RegistryKeyByName(const char* pszName)
{
	if (!stricmp(pszName, "HKEY_LOCAL_MACHINE") || !stricmp(pszName, "HKLM"))
		return HKEY_LOCAL_MACHINE;

	if (!stricmp(pszName, "HKEY_CURRENT_USER") || !stricmp(pszName, "HKCU"))
		return HKEY_CURRENT_USER;

	if (!stricmp(pszName, "HKEY_CLASSES_ROOT") || !stricmp(pszName, "HKCR"))
		return HKEY_CLASSES_ROOT;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Retreives data from the registry entry with KEY_READ permission. 
//			The subkey that the data is searched in is HKEY_CURRENT_USER.
// 
// Note:	If the function fails, FALSE is returned, TRUE otherwise.
//			This function exists only on windows builds.
//-----------------------------------------------------------------------------
BOOL GetRegistryValue(LPCSTR lpSubKey, LPCSTR lpValueName, LPBYTE lpData, DWORD cbData)
{
	HKEY	hKey;
	LSTATUS lStatus;
	DWORD	dwType;

	// Get key for HKEY_CURRENT_USER
	hKey = RegistryKeyByName("HKCU");

	lStatus = RegOpenKeyExA(hKey, lpSubKey, NULL, KEY_READ, &hKey);

	if (lStatus == NO_ERROR)
	{
		lStatus = RegQueryValueExA(hKey, lpValueName, NULL, &dwType, lpData, &cbData);
		RegCloseKey(hKey);
	}

	return (lStatus == NO_ERROR) ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: Setups full directory to the steam client. The path is stored inside
//			g_szSteamClientPath[] global variable.
//-----------------------------------------------------------------------------
bool ConfigureSteamClientPath(const char *pszPath, uint32 u32Length)
{
	char	szClientPath[MAX_PATH];
	DWORD	cbData;
	HKEY	hKey, *phkResult;
	BOOL	bSuccess;
	DWORD	dwType;
	HMODULE	hSteamClient;

	if (strlen(g_szSteamClientPath) && !pszPath)
		return true;

	*szClientPath = '\0';
	memset(szClientPath + 1, NULL, sizeof(szClientPath) - 1);

	cbData = sizeof(szClientPath);

	// Get key for HKEY_CURRENT_USER
	hKey = RegistryKeyByName("HKCU");

	// Lookup registry for the already existing path
	bSuccess = GetRegistryValue("Software\\Valve\\Steam\\ActiveProcess", "SteamClientDll", (LPBYTE)szClientPath, cbData);

	// Check against last and first byte and get the path of steamclient.dll
	if ((szClientPath + strlen(szClientPath) + 1) == (szClientPath + 1))
	{
		hSteamClient = GetModuleHandleA(k_pszSteamClientModuleName);
		GetModuleFileNameA(hSteamClient, szClientPath, sizeof(szClientPath));
	}

	if (pszPath)
		strncpy((char *)pszPath, szClientPath, u32Length);

	// Copy over to global buffer
	strncpy(g_szSteamClientPath, szClientPath, sizeof(szClientPath));

	// Retain only directory
	Q_StripFilename(g_szSteamClientPath);

	return (bSuccess == TRUE) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Calls s_pfnSteamSetSteamID() routine.
//-----------------------------------------------------------------------------
void Steam_SetMinidumpSteamID(uint64 u64SteamID)
{
	printf("Steam_SetMinidumpSteamID:  Caching Steam ID:  %lld [API loaded %s]\n", u64SteamID, s_pfnSteamSetSteamID ? "yes" : "no");

	g_SteamMinidumpSID = u64SteamID;

	// Don't call minidump sid routine for steam.dll, because s_pfnSteamSetSteamID
	// is nullptr there!
	if (s_BreakpadInfo == STEAM_BREAKPAD_STEAM)
		return;

	if (!g_MiniDumpSteamClientDllModule.m_hModule)
		return;

	if (s_pfnSteamSetSteamID)
	{
		printf("Steam_SetMinidumpSteamID:  Setting Steam ID:  %lld\n", u64SteamID);
		s_pfnSteamSetSteamID(u64SteamID);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// 
// Note:	Originally called Sys_LoadModule(), but this would interfere with
//			our function located in public source file interface.cpp
//-----------------------------------------------------------------------------
HMODULE Steam_LoadModule(const char *pModuleName)
{
	LPWSTR	pwszBuffer;
	DWORD	dwWBufSize, dwNumChars;
	HMODULE hModule;

	hModule = NULL;

	dwWBufSize = strlen(pModuleName);
	pwszBuffer = new WCHAR[(2 * dwWBufSize) + 1];

	dwNumChars = MultiByteToWideChar(CP_UTF8, NULL, pModuleName, dwWBufSize, pwszBuffer, dwWBufSize + 1);

	if (dwNumChars != NULL)
	{
		if (dwNumChars < dwWBufSize - 1)
			dwWBufSize = dwNumChars;

		pwszBuffer[dwWBufSize] = '\0';

		hModule = LoadLibraryExW(pwszBuffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	}

	// Try ascii
	if (!hModule)
		hModule = LoadLibraryExA(pModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	delete[] pwszBuffer;
	return hModule;
}