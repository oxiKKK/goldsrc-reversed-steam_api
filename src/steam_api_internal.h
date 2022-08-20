//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef STEAM_API_INTERNAL_H
#define STEAM_API_INTERNAL_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: This is used by the internal steam api code.
//-----------------------------------------------------------------------------
extern ISteamClient*				g_pSteamClient;
extern ISteamUtils*					g_pSteamUtilsRunFrame;

extern HSteamPipe					g_hSteamPipe;
extern HSteamUser					g_hSteamUser;

//-----------------------------------------------------------------------------
// Purpose: Steam ContentServer
//-----------------------------------------------------------------------------
extern ISteamClient*				g_pSteamContentServerClient;
extern ISteamContentServer*			g_pSteamContentServer;
extern ISteamUtils*					g_pSteamContentServerUtils;

extern HSteamPipe					g_hSteamContentServerPipe;
extern HSteamUser					g_hSteamContentServerUser;

//-----------------------------------------------------------------------------
// Purpose: Steam GameServerAPI access interfaces
//-----------------------------------------------------------------------------
extern ISteamClient*				g_pSteamClientGameServer;
extern ISteamGameServer*			g_pSteamGameServer;
extern ISteamUtils*					g_pSteamGameServerUtils;
extern ISteamApps*					g_pSteamGameServerApps;
extern ISteamNetworking*			g_pSteamGameServerNetworking;
extern ISteamGameServerStats*		g_pSteamGameServerStats;
extern ISteamHTTP*					g_pSteamGameServerHTTP;

extern HSteamPipe					g_hSteamGameServerPipe;
extern HSteamUser					g_hSteamGameServerUser;

//-----------------------------------------------------------------------------
// Purpose: Minidump routine declarations
//-----------------------------------------------------------------------------
using pfnSteamMiniDumpFn_t = void(*)(uint32, void*, uint32);
using pfnSteamWriteMiniDumpSetComment_t = void(*)(const char*);
using pfnSteamSetSteamID_t = void(*)(int64);
using pfnSteamSetAppID_t = void(*)(AppId_t);
using pfnSteamClientMiniDumpInit_t = void(*)(AppId_t, const char*, const char*, bool, void*, PFNPreMinidumpCallback);
using pfnSteamMiniDumpInit_t = void(*)();

//-----------------------------------------------------------------------------
// 
// Internal global variables
// 
//-----------------------------------------------------------------------------

extern char				g_szSteamClientPath[MAX_PATH];
extern char				g_szSteamInstallPath[MAX_PATH];

extern HMODULE			g_hSteamClientModule;
extern HMODULE			g_hSteamGameOverlayRendererModule;

extern bool				g_bCatchExceptionsInCallbacks;

//-----------------------------------------------------------------------------
// Purpose: Module names that the SteamAPI module accesses
//-----------------------------------------------------------------------------
extern const char* k_pszSteamClientModuleName;
extern const char* k_pszSteamClientModule64Name;
extern const char* k_pszSteamModuleName;
extern const char* k_pszSteamUIModuleName;
extern const char* k_pszSteamConsoleModuleName;
extern const char* k_pszSteamGameOverlayRendererModuleName;
extern const char* k_pszSteamBinAudioModuleName;

//-----------------------------------------------------------------------------
// Purpose: Minidump routines
//-----------------------------------------------------------------------------
extern pfnSteamMiniDumpFn_t					s_pfnSteamMiniDumpFn;
extern pfnSteamWriteMiniDumpSetComment_t	s_pfnSteamWriteMiniDumpSetComment;
extern pfnSteamSetSteamID_t					s_pfnSteamSetSteamID;
extern pfnSteamSetAppID_t					s_pfnSteamSetAppID;

//-----------------------------------------------------------------------------
// 
// Breakpad data used by steamclient's Breakpad API.
// 
//-----------------------------------------------------------------------------

extern char						g_pchBreakpadVersion[64];
extern char						g_szBreakpadTimestamp[16];
extern bool						g_bBreakpadFullMemoryDumps;
extern void* 					g_pvBreakpadContext;
extern PFNPreMinidumpCallback	g_pfnBreakpadPreMinidumpCallback;

//-----------------------------------------------------------------------------
// Purpose: Values for s_BreakpadInfo
//-----------------------------------------------------------------------------
#define STEAM_BREAKPAD_STEAM		0
#define STEAM_BREAKPAD_STEAMCLIENT	1

extern AppId_t					g_BreakpadLastAppId;

extern bool						s_BreakpadInfo;

//-----------------------------------------------------------------------------
// 
// Minidump API
// 
//-----------------------------------------------------------------------------

extern uint64 g_SteamMinidumpSID;

//-----------------------------------------------------------------------------
// 
// Steam content server
// 
//----------------------------------------------------------------------------

extern HMODULE g_hSteamContentServerModule;

//-----------------------------------------------------------------------------
// 
// Steam game server
// 
//----------------------------------------------------------------------------

extern EServerMode g_eGameServerMode;

extern HMODULE g_hSteamGameServerModule;

//-----------------------------------------------------------------------------
// 
// Exported API declaration
// 
// Purpose: We have to do this because application can define following macro,
//			VERSION_SAFE_STEAM_API_INTERFACES, which basically defines that 
//			application developers can decide on which interface version they
//			want to use. Defining this declares class CSteamAPIContext which
//			enables this funcitonality. On the other hand, it hides declarations
//			for classic steam interface access routines such as SteamClient() or
//			SteamFriends() etc.
// 
//			Since we're building steam_api module, we cannot export just one thing, 
//			otherwise one developer could have custom interface versions, while
//			the other couldn't - because we didn't define this macro. This is an
//			issue that we have to take care of.
// 
//			In order to remedy this, we will declare our function declarations
//			that are compatible with these ones inside steam_api.h under the control
//			of that macro. This way, we can declare as well as define and export
//			both versions of this API, without taking care of safeness.
// 
//			This will create duplicated code, where we'll have two identical 
//			declarations of one function, but I cannot think of better design 
//			in this particular case.
// 
// Note:	We only declare functions that are controlled by safeness macro. There's
//			no need for use to declare other stuff that won't change if we define
//			or undefine that macro.
//-----------------------------------------------------------------------------

S_API bool SteamAPI_InitSafe();

#if defined(_PS3)
S_API bool SteamAPI_Init(SteamPS3Params_t *pParams);
#else
S_API bool SteamAPI_Init();
#endif

S_API ISteamUser* SteamUser();
S_API ISteamFriends* SteamFriends();
S_API ISteamUtils* SteamUtils();
S_API ISteamMatchmaking* SteamMatchmaking();
S_API ISteamUserStats* SteamUserStats();
S_API ISteamApps* SteamApps();
S_API ISteamNetworking* SteamNetworking();
S_API ISteamMatchmakingServers* SteamMatchmakingServers();
S_API ISteamRemoteStorage* SteamRemoteStorage();
S_API ISteamScreenshots* SteamScreenshots();
S_API ISteamHTTP* SteamHTTP();
#ifdef _PS3
S_API ISteamPS3OverlayRender *S_CALLTYPE SteamPS3OverlayRender();
#endif

S_API HSteamUser SteamAPI_GetHSteamUser();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSteamAPIContext
{
public:
	bool Init();
	void Clear();

public:
	ISteamUser					*m_pSteamUser;
	ISteamFriends				*m_pSteamFriends;
	ISteamUtils					*m_pSteamUtils;
	ISteamMatchmaking			*m_pSteamMatchmaking;
	ISteamUserStats				*m_pSteamUserStats;
	ISteamApps					*m_pSteamApps;
	ISteamMatchmakingServers	*m_pSteamMatchmakingServers;
	ISteamNetworking			*m_pSteamNetworking;
	ISteamRemoteStorage			*m_pSteamRemoteStorage;
	ISteamScreenshots			*m_pSteamScreenshots;
	ISteamHTTP					*m_pSteamHTTP;
#ifdef _PS3
	ISteamPS3OverlayRender *m_pSteamPS3OverlayRender;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: Clears all pointers
//-----------------------------------------------------------------------------
inline void CSteamAPIContext::Clear()
{
	m_pSteamUser = NULL;
	m_pSteamFriends = NULL;
	m_pSteamUtils = NULL;
	m_pSteamMatchmaking = NULL;
	m_pSteamUserStats = NULL;
	m_pSteamApps = NULL;
	m_pSteamMatchmakingServers = NULL;
	m_pSteamNetworking = NULL;
	m_pSteamRemoteStorage = NULL;
	m_pSteamHTTP = NULL;
	m_pSteamScreenshots = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: This function must be inlined so the module using steam_api.dll 
//			gets the version names they want.
//-----------------------------------------------------------------------------
inline bool CSteamAPIContext::Init()
{
	if (!SteamClient())
		return false;

	HSteamUser hSteamUser = SteamAPI_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamAPI_GetHSteamPipe();

	m_pSteamUser = SteamClient()->GetISteamUser(hSteamUser, hSteamPipe, STEAMUSER_INTERFACE_VERSION);
	if (!m_pSteamUser)
		return false;

	m_pSteamFriends = SteamClient()->GetISteamFriends(hSteamUser, hSteamPipe, STEAMFRIENDS_INTERFACE_VERSION);
	if (!m_pSteamFriends)
		return false;

	m_pSteamUtils = SteamClient()->GetISteamUtils(hSteamPipe, STEAMUTILS_INTERFACE_VERSION);
	if (!m_pSteamUtils)
		return false;

	m_pSteamMatchmaking = SteamClient()->GetISteamMatchmaking(hSteamUser, hSteamPipe, STEAMMATCHMAKING_INTERFACE_VERSION);
	if (!m_pSteamMatchmaking)
		return false;

	m_pSteamMatchmakingServers = SteamClient()->GetISteamMatchmakingServers(hSteamUser, hSteamPipe, STEAMMATCHMAKINGSERVERS_INTERFACE_VERSION);
	if (!m_pSteamMatchmakingServers)
		return false;

	m_pSteamUserStats = SteamClient()->GetISteamUserStats(hSteamUser, hSteamPipe, STEAMUSERSTATS_INTERFACE_VERSION);
	if (!m_pSteamUserStats)
		return false;

	m_pSteamApps = SteamClient()->GetISteamApps(hSteamUser, hSteamPipe, STEAMAPPS_INTERFACE_VERSION);
	if (!m_pSteamApps)
		return false;

	m_pSteamNetworking = SteamClient()->GetISteamNetworking(hSteamUser, hSteamPipe, STEAMNETWORKING_INTERFACE_VERSION);
	if (!m_pSteamNetworking)
		return false;

	m_pSteamRemoteStorage = SteamClient()->GetISteamRemoteStorage(hSteamUser, hSteamPipe, STEAMREMOTESTORAGE_INTERFACE_VERSION);
	if (!m_pSteamRemoteStorage)
		return false;

	m_pSteamScreenshots = SteamClient()->GetISteamScreenshots(hSteamUser, hSteamPipe, STEAMSCREENSHOTS_INTERFACE_VERSION);
	if (!m_pSteamScreenshots)
		return false;

	m_pSteamHTTP = SteamClient()->GetISteamHTTP(hSteamUser, hSteamPipe, STEAMHTTP_INTERFACE_VERSION);
	if (!m_pSteamHTTP)
		return false;

#ifdef _PS3
	m_pSteamPS3OverlayRender = SteamClient()->GetISteamPS3OverlayRender();
#endif

	return true;
}

extern CSteamAPIContext g_SteamAPIContext;

//-----------------------------------------------------------------------------
// 
// Game server API for un/safeness
// 
//-----------------------------------------------------------------------------

#ifndef _PS3
S_API bool SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
S_API bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
#else
S_API bool SteamGameServer_InitSafe(const SteamPS3Params_t *ps3Params, uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
S_API bool SteamGameServer_Init(const SteamPS3Params_t *ps3Params, uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString);
#endif

S_API ISteamGameServer *SteamGameServer();
S_API ISteamUtils *SteamGameServerUtils();
S_API ISteamNetworking *SteamGameServerNetworking();
S_API ISteamGameServerStats *SteamGameServerStats();
S_API ISteamHTTP *SteamGameServerHTTP();

//-----------------------------------------------------------------------------
// Purpose: Current version of GolSrc doesn't care about exporting this class, 
//			so we don't have to declare it
//-----------------------------------------------------------------------------
#if 0
S_API HSteamUser SteamGameServer_GetHSteamUser();

class CSteamGameServerAPIContext
{
public:
	CSteamGameServerAPIContext();
	void Clear();

	bool Init();

	ISteamGameServer *SteamGameServer() { return m_pSteamGameServer; }
	ISteamUtils *SteamGameServerUtils() { return m_pSteamGameServerUtils; }
	ISteamNetworking *SteamGameServerNetworking() { return m_pSteamGameServerNetworking; }
	ISteamGameServerStats *SteamGameServerStats() { return m_pSteamGameServerStats; }
	ISteamHTTP *SteamHTTP() { return m_pSteamHTTP; }
	ISteamUGC *SteamUGC() { return m_pSteamUGC; }

private:
	ISteamGameServer			*m_pSteamGameServer;
	ISteamUtils					*m_pSteamGameServerUtils;
	ISteamNetworking			*m_pSteamGameServerNetworking;
	ISteamGameServerStats		*m_pSteamGameServerStats;
	ISteamHTTP					*m_pSteamHTTP;
	ISteamUGC					*m_pSteamUGC;
};

inline CSteamGameServerAPIContext::CSteamGameServerAPIContext()
{
	Clear();
}

inline void CSteamGameServerAPIContext::Clear()
{
	m_pSteamGameServer = NULL;
	m_pSteamGameServerUtils = NULL;
	m_pSteamGameServerNetworking = NULL;
	m_pSteamGameServerStats = NULL;
	m_pSteamHTTP = NULL;
	m_pSteamUGC = NULL;
}

S_API ISteamClient *g_pSteamClientGameServer;
// This function must be inlined so the module using steam_api.dll gets the version names they want.
inline bool CSteamGameServerAPIContext::Init()
{
	if (!g_pSteamClientGameServer)
		return false;

	HSteamUser hSteamUser = SteamGameServer_GetHSteamUser();
	HSteamPipe hSteamPipe = SteamGameServer_GetHSteamPipe();

	m_pSteamGameServer = g_pSteamClientGameServer->GetISteamGameServer(hSteamUser, hSteamPipe, STEAMGAMESERVER_INTERFACE_VERSION);
	if (!m_pSteamGameServer)
		return false;

	m_pSteamGameServerUtils = g_pSteamClientGameServer->GetISteamUtils(hSteamPipe, STEAMUTILS_INTERFACE_VERSION);
	if (!m_pSteamGameServerUtils)
		return false;

	m_pSteamGameServerNetworking = g_pSteamClientGameServer->GetISteamNetworking(hSteamUser, hSteamPipe, STEAMNETWORKING_INTERFACE_VERSION);
	if (!m_pSteamGameServerNetworking)
		return false;

	m_pSteamGameServerStats = g_pSteamClientGameServer->GetISteamGameServerStats(hSteamUser, hSteamPipe, STEAMGAMESERVERSTATS_INTERFACE_VERSION);
	if (!m_pSteamGameServerStats)
		return false;

	m_pSteamHTTP = g_pSteamClientGameServer->GetISteamHTTP(hSteamUser, hSteamPipe, STEAMHTTP_INTERFACE_VERSION);
	if (!m_pSteamHTTP)
		return false;

	m_pSteamUGC = g_pSteamClientGameServer->GetISteamUGC(hSteamUser, hSteamPipe, STEAMUGC_INTERFACE_VERSION);
	if (!m_pSteamUGC)
		return false;

	return true;
}
#endif

//-----------------------------------------------------------------------------
// 
// Internal Steam API routines
// 
//-----------------------------------------------------------------------------

extern bool SteamAPI_InitInternal(bool safe);
extern ISteamClient* SteamAPI_Init_Internal(HMODULE* SteamModule, bool TryLocal);
extern void SteamAPI_Shutdown_Internal(HMODULE hSteamServerModule);

//-----------------------------------------------------------------------------
// 
// Steam game server internal API
// 
//-----------------------------------------------------------------------------

extern bool SteamGameServer_Init_Internal(uint32 unIP, uint16 usSteamPort, uint32 usGamePort, int usQueryPort, EServerMode eServerMode, const char* pchVersionString, bool bSafe);

//-----------------------------------------------------------------------------
// 
// Minidump internal API
// 
//-----------------------------------------------------------------------------

extern void Steam_LoadMinidumpInterface();
extern void Steam_ShutdownMinidumpInterface();

//-----------------------------------------------------------------------------
// 
// Game overlay renderer internal API
// 
//-----------------------------------------------------------------------------

extern bool Steam_LoadGameOverlayRenderer();

#endif