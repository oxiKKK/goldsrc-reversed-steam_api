//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CLIENT_MODULES_H
#define CLIENT_MODULES_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: Data structure to load the module set when globally initializing
//			object of this type.
//-----------------------------------------------------------------------------
class CModuleLoadWrapper
{
public:
	void Load();

public:
	const char* m_pszModulePath;
	HMODULE 	m_hModule;
	bool 		m_bValid;
};

extern CModuleLoadWrapper g_MiniDumpSteamDllModule;
extern CModuleLoadWrapper g_MiniDumpSteamClientDllModule;

//-----------------------------------------------------------------------------
// 
// Steam API module code
// 
//-----------------------------------------------------------------------------

extern uint32 GetSteamAppID(const char *szSteamAppID);
extern HKEY RegistryKeyByName(const char* pszName);
extern BOOL GetRegistryValue(LPCSTR lpSubKey, LPCSTR lpValueName, LPBYTE lpData, DWORD cbData);
extern bool ConfigureSteamClientPath(const char *pszPath, uint32 u32Length);
extern void Steam_SetMinidumpSteamID(uint64 u64SteamID);
extern HMODULE Steam_LoadModule(const char *pModuleName);

#endif