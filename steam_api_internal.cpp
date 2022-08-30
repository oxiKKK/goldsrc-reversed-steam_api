//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// 
// Internal global variables
// 
//-----------------------------------------------------------------------------

// Directory to steamclient.dll, without filename respectively.
char				g_szSteamClientPath[MAX_PATH] = { '\0' };

// Steam install path set inside SteamAPI_GetSteamInstallPath() function.
char				g_szSteamInstallPath[MAX_PATH] = { '\0' };

// Handles to loaded modules
HMODULE				g_hSteamClientModule = NULL;
HMODULE				g_hSteamGameOverlayRendererModule = NULL;

// Used for when application developers want to access different versions
// of steamclient API.
CSteamAPIContext	g_SteamAPIContext;

// We're allowing to catch exceptions inside callback handling code by default
bool				g_bCatchExceptionsInCallbacks = true;

//-----------------------------------------------------------------------------
// Purpose: Module names that the SteamAPI module accesses
//-----------------------------------------------------------------------------
const char* k_pszSteamClientModuleName = "steamclient.dll";
const char* k_pszSteamClientModule64Name = "steamclient64.dll";
const char* k_pszSteamModuleName = "steam.dll";
const char* k_pszSteamUIModuleName = "steamui.dll";
const char* k_pszSteamConsoleModuleName = "steamconsole.dll";
const char* k_pszSteamGameOverlayRendererModuleName = "gameoverlayrenderer.dll";
const char* k_pszSteamBinAudioModuleName = "bin\\audio.dll";

//-----------------------------------------------------------------------------
// Purpose: Minidump routines
//-----------------------------------------------------------------------------
pfnSteamMiniDumpFn_t				s_pfnSteamMiniDumpFn = nullptr;
pfnSteamWriteMiniDumpSetComment_t	s_pfnSteamWriteMiniDumpSetComment = nullptr;
pfnSteamSetSteamID_t				s_pfnSteamSetSteamID = nullptr;
pfnSteamSetAppID_t					s_pfnSteamSetAppID = nullptr;

//-----------------------------------------------------------------------------
// 
// Breakpad data used by steamclient's Breakpad API
// 
//-----------------------------------------------------------------------------

char					g_pchBreakpadVersion[64] = { '\0' };
char					g_szBreakpadTimestamp[16] = { '\0' };
bool					g_bBreakpadFullMemoryDumps = false;
void* 					g_pvBreakpadContext = nullptr;
PFNPreMinidumpCallback	g_pfnBreakpadPreMinidumpCallback = nullptr;

// Appid for breakpad routines. Is used to keep trach of appid value change.
AppId_t					g_BreakpadLastAppId = k_uAppIdInvalid;

// Informs about if we're loading from steam.dll (true) or steamclient.dll (false)
// See STEAM_BREAKPAD_* constants.
bool					s_BreakpadInfo = STEAM_BREAKPAD_STEAM;

//-----------------------------------------------------------------------------
// 
// Minidump API
// 
//-----------------------------------------------------------------------------

uint64 g_SteamMinidumpSID = NULL;

//-----------------------------------------------------------------------------
// 
// Steam content server
// 
//----------------------------------------------------------------------------

// Handle to steamclient module for content server client
HMODULE			g_hSteamContentServerModule;

//-----------------------------------------------------------------------------
// 
// Steam game server
// 
//----------------------------------------------------------------------------

// Authentication modes for game server
EServerMode		g_eGameServerMode;

// Handle to steamclient module for game server client
HMODULE			g_hSteamGameServerModule;

//-----------------------------------------------------------------------------
// 
// Internal Steam API routines
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Internal routine for steamAPI initialization. Introduces both safe
//			and unsafe mode intiialization. This isn't exposed to the user.
//-----------------------------------------------------------------------------
bool SteamAPI_InitInternal(bool safe)
{
	char		SteamAPPId[12];
	AppId_t		AppID;
	ISteamUser*	pSteamUser;
	ISteamUtils*pSteamUtils;

	// Already initialized
	if (g_pSteamClient != nullptr)
		return true;

	// Get steam client interface and module handle to steamclient.dll or steam.dll
	g_pSteamClient = SteamAPI_Init_Internal(&g_hSteamClientModule, false);

	if (!g_pSteamClient)
		return false;

	g_hSteamPipe = g_pSteamClient->CreateSteamPipe();
	g_hSteamUser = g_pSteamClient->ConnectToGlobalUser(g_hSteamPipe);

	g_pSteamUtilsRunFrame = nullptr;

	// These two are essential for us in the future, because without
	// them we cannot access individual steam client interfaces.
	if (!g_hSteamPipe || !g_hSteamUser)
	{
		SteamAPI_Shutdown();
		return false;
	}

	// Safe mode
	if (safe != false)
	{
		pSteamUtils = g_pSteamClient->GetISteamUtils(g_hSteamPipe, STEAMUTILS_INTERFACE_VERSION);
	}
	// Unsafe mode
	else
	{
		if (!g_SteamAPIContext.Init())
		{
			SteamAPI_Shutdown();
			return false;
		}

		pSteamUtils = g_SteamAPIContext.m_pSteamUtils;
	}

	// Try to retreive current app id
	AppID = k_uAppIdInvalid;

	if (pSteamUtils)
		AppID = pSteamUtils->GetAppID();

	if (!pSteamUtils || !AppID)
	{
		OutputDebugStringA("[S_API FAIL] SteamAPI_Init() failed; no appID found.\n"
						   "Either launch the game from Steam, or put the file steam_appid.txt containing the correct appID in your game folder.\n");

		SteamAPI_Shutdown();
		return false;
	}

	// If the steam app id weren't set already, we have to set it now
	if (!GetEnvironmentVariableA("SteamAppId", nullptr, NULL))
	{
		snprintf(SteamAPPId, sizeof(SteamAPPId), "%u", AppID);
		SteamAPPId[sizeof(SteamAPPId) - 1] = '\0';

		SetEnvironmentVariableA("SteamAppId", SteamAPPId);
	}

	SteamAPI_SetBreakpadAppID(AppID);

	CallbackMgr_RegisterInterfaceFuncs(g_hSteamClientModule);

	Steam_LoadMinidumpInterface();
	Steam_LoadGameOverlayRenderer();

	if (safe != false)
	{
		pSteamUser = g_pSteamClient->GetISteamUser(g_hSteamUser, g_hSteamPipe, STEAMUSER_INTERFACE_VERSION);

		if (pSteamUser)
			Steam_SetMinidumpSteamID(pSteamUser->GetSteamID().ConvertToUint64());
	}
	else
	{
		if (g_SteamAPIContext.m_pSteamUser)
			Steam_SetMinidumpSteamID(g_SteamAPIContext.m_pSteamUser->GetSteamID().ConvertToUint64());
		else
			Steam_SetMinidumpSteamID(0);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This version of internal initialization returns module handle to
//			the steamclient. If TryLocal is set, and the module couldn't be 
//			loaded, the function will try to search for the module inside local
//			(current active) path.
// 
// Note:	Also, g_szSteamClientPath is set upon calling ConfigureSteamClientPath()
//			if wasn't already.
//-----------------------------------------------------------------------------
ISteamClient* SteamAPI_Init_Internal(HMODULE* SteamModule, bool TryLocal)
{
	char SteamClientPath[MAX_PATH];
	char DebugBuffer[1024];

	if (!SteamModule)
		return false;

	*g_szSteamClientPath = '\0';
	*SteamClientPath = '\0';

	memset(SteamClientPath + 1, NULL, sizeof(SteamClientPath) - 1);

	// Try to get online running instance of steam
	if (ConfigureSteamClientPath(SteamClientPath, sizeof(SteamClientPath)))
	{
		if (SteamAPI_IsSteamRunning())
		{
			*SteamModule = Steam_LoadModule(SteamClientPath);

			if (!SteamModule)
			{
				snprintf(DebugBuffer, sizeof(DebugBuffer), "[S_API FAIL] SteamAPI_Init() failed; Steam_LoadModule failed to load: %s\n", SteamClientPath);
				DebugBuffer[sizeof(DebugBuffer) - 1] = '\0';
				OutputDebugStringA(DebugBuffer);
			}
		}
		else
		{
			OutputDebugStringA("[S_API FAIL] SteamAPI_Init() failed; SteamAPI_IsSteamRunning() failed.\n");
		}
	}

	// Try to get running instance of steam client, if possible
	if (!*SteamModule)
	{
		if (TryLocal)
			*SteamModule = Steam_LoadModule(k_pszSteamClientModuleName);

		if (!*SteamModule)
		{
			OutputDebugStringA("[S_API FAIL] SteamAPI_Init() failed; unable to locate a running instance of Steam, or a local steamclient.dll.\n");
			return false;
		}
	}

	return reinterpret_cast<ISteamClient*>(Sys_GetFactory(reinterpret_cast<CSysModule*>(*SteamModule))("SteamClient012", nullptr));
}

//-----------------------------------------------------------------------------
// Purpose: Frees steam module and frees minidump interface modules & data.
//-----------------------------------------------------------------------------
void SteamAPI_Shutdown_Internal(HMODULE hSteamServerModule)
{
	if (hSteamServerModule)
		FreeLibrary(hSteamServerModule);

	Steam_ShutdownMinidumpInterface();
}

//-----------------------------------------------------------------------------
// 
// Steam game server internal API
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Setups connection to the game server. Initialitzes global data
//			for the game server internal API.
//-----------------------------------------------------------------------------
bool SteamGameServer_Init_Internal(uint32 unIP, uint16 usSteamPort, uint32 usGamePort, int usQueryPort, EServerMode eServerMode, const char* pchVersionString, bool bSafe)
{
	uint32	unFlags;
	AppId_t	nGameAppId;

	g_eGameServerMode = eServerMode;
	
	// Locate and setup steam game server module
	g_pSteamClientGameServer = SteamAPI_Init_Internal(&g_hSteamGameServerModule, true);

	if (!g_pSteamClientGameServer)
		return false;

	// Set the local IP and Port to bind to. This must be called before CreateLocalUser().
	g_pSteamClientGameServer->SetLocalIPBinding(unIP, usSteamPort);

	// Create local user object for this server class object
	g_hSteamGameServerUser = g_pSteamClientGameServer->CreateLocalUser(&g_hSteamGameServerPipe, k_EAccountTypeGameServer);

	if (!g_hSteamGameServerUser || !g_hSteamGameServerPipe)
		return false;

	// Create game server object for us
	g_pSteamGameServer = g_pSteamClientGameServer->GetISteamGameServer(g_hSteamGameServerUser, g_hSteamGameServerPipe, STEAMGAMESERVER_INTERFACE_VERSION);

	if (!g_pSteamGameServer)
		return false;

	// Create game server utility object
	g_pSteamGameServerUtils = g_pSteamClientGameServer->GetISteamUtils(g_hSteamGameServerPipe, STEAMUTILS_INTERFACE_VERSION);

	if (!g_pSteamGameServerUtils)
		return false;

	// Create game server app object
	g_pSteamGameServerApps = g_pSteamClientGameServer->GetISteamApps(g_hSteamGameServerUser, g_hSteamGameServerPipe, STEAMAPPS_INTERFACE_VERSION);

	if (!g_pSteamGameServerApps)
		return false;

	// Create game server HTTP object
	g_pSteamGameServerHTTP = g_pSteamClientGameServer->GetISteamHTTP(g_hSteamGameServerUser, g_hSteamGameServerPipe, STEAMHTTP_INTERFACE_VERSION);

	if (!g_pSteamGameServerHTTP)
		return false;

	// Networking and server stats have to be established in non-safe mode
	if (bSafe != true)
	{
		// Create game server networking object
		g_pSteamGameServerNetworking = g_pSteamClientGameServer->GetISteamNetworking(g_hSteamGameServerUser, g_hSteamGameServerPipe, STEAMNETWORKING_INTERFACE_VERSION);

		if (!g_pSteamGameServerNetworking)
			return false;

		// Create game server stats object
		g_pSteamGameServerStats = g_pSteamClientGameServer->GetISteamGameServerStats(g_hSteamGameServerUser, g_hSteamGameServerPipe, STEAMGAMESERVERSTATS_INTERFACE_VERSION);

		if (!g_pSteamGameServerStats)
			return false;
	}

	unFlags = (g_eGameServerMode != eServerModeAuthenticationAndSecure) ? eServerModeInvalid : eServerModeAuthentication;

	if (g_eGameServerMode == eServerModeNoAuthentication)
		unFlags |= 32;

	// Try to obtain game APP identification number, we need this for breakpad and
	// game server initialization.
	nGameAppId = g_pSteamGameServerUtils->GetAppID();

	if (nGameAppId == k_uAppIdInvalid)
		return false;

	// Finally initialize game server by calling its internal API, if this fail, we have to return
	if (!g_pSteamGameServer->InitGameServer(unIP, usGamePort, usQueryPort, unFlags, nGameAppId, pchVersionString))
		return false;

	// While in safe mode, we can clear these out, they aren't needed at this point
	if (bSafe != false)
	{
		g_pSteamGameServer = nullptr;
		g_pSteamGameServerUtils = nullptr;
	}

	// Load interfaces we need and exit
	Steam_RegisterInterfaceFuncs(g_hSteamGameServerModule);
	SteamAPI_SetBreakpadAppID(nGameAppId);
	Steam_LoadMinidumpInterface();

	return true;
}

//-----------------------------------------------------------------------------
// 
// Minidump internal API
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Tries to locate all routines from the minidump interface API exposed
//			by either the steam.dll or steamclient.dll module.
//-----------------------------------------------------------------------------
void Steam_LoadMinidumpInterface()
{
	pfnSteamClientMiniDumpInit_t	pfnSteamClientMiniDumpInit;
	pfnSteamMiniDumpInit_t			pfnSteamMiniDumpInit;
	HMODULE							hSteamModule;

	// Try this first for steamclient.dll and if we fail, try steam.dll
	if (s_BreakpadInfo != STEAM_BREAKPAD_STEAM)
	{
		g_MiniDumpSteamClientDllModule.Load();
		hSteamModule = g_MiniDumpSteamClientDllModule.m_hModule;

		if (!hSteamModule)
			return;

		printf("Looking up breakpad interfaces from steamclient\n");

		// Breakpad_SteamWriteMiniDumpUsingExceptionInfoWithBuildId
		s_pfnSteamMiniDumpFn = reinterpret_cast<pfnSteamMiniDumpFn_t>(GetProcAddress(hSteamModule, "Breakpad_SteamWriteMiniDumpUsingExceptionInfoWithBuildId"));

		// Breakpad_SteamWriteMiniDumpSetComment
		s_pfnSteamWriteMiniDumpSetComment = reinterpret_cast<pfnSteamWriteMiniDumpSetComment_t>(GetProcAddress(hSteamModule, "Breakpad_SteamWriteMiniDumpSetComment"));

		// Breakpad_SteamSetSteamID
		s_pfnSteamSetSteamID = reinterpret_cast<pfnSteamSetSteamID_t>(GetProcAddress(hSteamModule, "Breakpad_SteamSetSteamID"));

		// Breakpad_SteamSetAppID
		s_pfnSteamSetAppID = reinterpret_cast<pfnSteamSetAppID_t>(GetProcAddress(hSteamModule, "Breakpad_SteamSetAppID"));

		// Breakpad_SteamMiniDumpInit
		pfnSteamClientMiniDumpInit = reinterpret_cast<pfnSteamClientMiniDumpInit_t>(GetProcAddress(hSteamModule, "Breakpad_SteamMiniDumpInit"));

		if (pfnSteamClientMiniDumpInit)
		{
			printf("Calling BreakpadMiniDumpSystemInit\n");
			pfnSteamClientMiniDumpInit(g_BreakpadLastAppId,
									   g_pchBreakpadVersion,
									   g_szBreakpadTimestamp,
									   g_bBreakpadFullMemoryDumps,
									   g_pvBreakpadContext,
									   g_pfnBreakpadPreMinidumpCallback);

			if (g_SteamMinidumpSID)
				Steam_SetMinidumpSteamID(g_SteamMinidumpSID);
		}
	}
	// Try to load from steam.dll
	else
	{
		g_MiniDumpSteamDllModule.Load();
		hSteamModule = g_MiniDumpSteamDllModule.m_hModule;

		if (!hSteamModule)
			return;

		// SteamWriteMiniDumpUsingExceptionInfoWithBuildId
		s_pfnSteamMiniDumpFn = reinterpret_cast<pfnSteamMiniDumpFn_t>(GetProcAddress(hSteamModule, "SteamWriteMiniDumpUsingExceptionInfoWithBuildId"));

		// SteamWriteMiniDumpSetComment
		s_pfnSteamWriteMiniDumpSetComment = reinterpret_cast<pfnSteamWriteMiniDumpSetComment_t>(GetProcAddress(hSteamModule, "SteamWriteMiniDumpUsingExceptionInfoWithBuildId"));

		s_pfnSteamSetSteamID = nullptr;

		pfnSteamMiniDumpInit = reinterpret_cast<pfnSteamMiniDumpInit_t>(GetProcAddress(hSteamModule, "SteamMiniDumpInit"));

		if (pfnSteamMiniDumpInit)
			pfnSteamMiniDumpInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shutdowns routines loaded from the minidump interaface. These routines
//			can be loaded both from steam and steamclient modules, so we have to
//			determine from where are these modules loaded from using breakpad
//			information variable and then free specific data respectively. Also
//			frees specific minidump steam library.
//-----------------------------------------------------------------------------
void Steam_ShutdownMinidumpInterface()
{
	s_pfnSteamMiniDumpFn = nullptr;
	s_pfnSteamWriteMiniDumpSetComment = nullptr;

	if (s_BreakpadInfo != STEAM_BREAKPAD_STEAM)
	{
		if (!g_MiniDumpSteamClientDllModule.m_bValid)
			return;

		g_MiniDumpSteamClientDllModule.m_bValid = false;
		if (!g_MiniDumpSteamClientDllModule.m_hModule)
			return;

		FreeLibrary(g_MiniDumpSteamClientDllModule.m_hModule);
		g_MiniDumpSteamClientDllModule.m_hModule = nullptr;
	}
	else
	{
		if (!g_MiniDumpSteamDllModule.m_bValid)
			return;

		g_MiniDumpSteamDllModule.m_bValid = false;
		if (!g_MiniDumpSteamDllModule.m_hModule)
			return;

		FreeLibrary(g_MiniDumpSteamDllModule.m_hModule);
		g_MiniDumpSteamDllModule.m_hModule = nullptr;
	}
}

//-----------------------------------------------------------------------------
// 
// Game overlay renderer internal API
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Tries to load steam overlay renderer library. Also if the steam client
//			path is invalid or hasn't been set yet, the function calls subroutine
//			ConfigureSteamClientPath() which setups path to the steam client dll.
//-----------------------------------------------------------------------------
bool Steam_LoadGameOverlayRenderer()
{
	char szGameOverlayRendererPath[MAX_PATH];

	g_hSteamGameOverlayRendererModule = GetModuleHandleA(k_pszSteamGameOverlayRendererModuleName);

	// If the module was loaded already before, we don't have to load it again
	if (g_hSteamGameOverlayRendererModule)
		return true;

	// Have to look for steam client path if this hasn't been done already
	ConfigureSteamClientPath(nullptr, NULL);

	// Append GOR module name to steam directory and try to load the module
	snprintf(szGameOverlayRendererPath, sizeof(szGameOverlayRendererPath), "%s\\%s", g_szSteamClientPath, k_pszSteamGameOverlayRendererModuleName);

	g_hSteamGameOverlayRendererModule = Steam_LoadModule(szGameOverlayRendererPath);

	if (g_hSteamGameOverlayRendererModule)
		return true;

	return false;
}
