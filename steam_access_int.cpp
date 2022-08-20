//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Accessor to  API.
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// Purpose: SteamAPI access interfaces
//-----------------------------------------------------------------------------
ISteamClient*				g_pSteamClient;
ISteamUtils*				g_pSteamUtilsRunFrame;

HSteamPipe					g_hSteamPipe;
HSteamUser					g_hSteamUser;

//-----------------------------------------------------------------------------
// Purpose: Steam ContentServerAPI access interfaces
//-----------------------------------------------------------------------------
ISteamClient*				g_pSteamContentServerClient;
ISteamContentServer*		g_pSteamContentServer;
ISteamUtils*				g_pSteamContentServerUtils;

HSteamPipe					g_hSteamContentServerPipe;
HSteamUser					g_hSteamContentServerUser;

//-----------------------------------------------------------------------------
// Purpose: Steam GameServerAPI access interfaces
//-----------------------------------------------------------------------------
ISteamClient*				g_pSteamClientGameServer;
ISteamGameServer*			g_pSteamGameServer;
ISteamUtils*				g_pSteamGameServerUtils;
ISteamApps*					g_pSteamGameServerApps;
ISteamNetworking*			g_pSteamGameServerNetworking;
ISteamGameServerStats*		g_pSteamGameServerStats;
ISteamHTTP*					g_pSteamGameServerHTTP;

HSteamPipe					g_hSteamGameServerPipe;
HSteamUser					g_hSteamGameServerUser;

//-----------------------------------------------------------------------------
// 
// SteamAPI access routines
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam client API.
//-----------------------------------------------------------------------------
ISteamClient* SteamClient()
{
	return g_pSteamClient;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam user API.
//-----------------------------------------------------------------------------
ISteamUser* SteamUser()
{
	return g_SteamAPIContext.m_pSteamUser;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam friends API.
//-----------------------------------------------------------------------------
ISteamFriends* SteamFriends()
{
	return g_SteamAPIContext.m_pSteamFriends;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam utility API.
//-----------------------------------------------------------------------------
ISteamUtils* SteamUtils()
{
	return g_SteamAPIContext.m_pSteamUtils;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam matchmaking API.
//-----------------------------------------------------------------------------
ISteamMatchmaking* SteamMatchmaking()
{
	return g_SteamAPIContext.m_pSteamMatchmaking;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam matchmaking servers API.
//-----------------------------------------------------------------------------
ISteamMatchmakingServers* SteamMatchmakingServers()
{
	return g_SteamAPIContext.m_pSteamMatchmakingServers;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam user stats API.
//-----------------------------------------------------------------------------
ISteamUserStats* SteamUserStats()
{
	return g_SteamAPIContext.m_pSteamUserStats;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam apps API.
//-----------------------------------------------------------------------------
ISteamApps* SteamApps()
{
	return g_SteamAPIContext.m_pSteamApps;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam networking API.
//-----------------------------------------------------------------------------
ISteamNetworking* SteamNetworking()
{
	return g_SteamAPIContext.m_pSteamNetworking;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam remote storage API.
//-----------------------------------------------------------------------------
ISteamRemoteStorage* SteamRemoteStorage()
{
	return g_SteamAPIContext.m_pSteamRemoteStorage;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam screenshots API.
//-----------------------------------------------------------------------------
ISteamScreenshots* SteamScreenshots()
{
	return g_SteamAPIContext.m_pSteamScreenshots;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam HTTP API.
//-----------------------------------------------------------------------------
ISteamHTTP* SteamHTTP()
{
	return g_SteamAPIContext.m_pSteamHTTP;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam pipe object handle.
//-----------------------------------------------------------------------------
HSteamPipe GetHSteamPipe()
{
	return g_hSteamPipe;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam user object handle.
//-----------------------------------------------------------------------------
HSteamUser GetHSteamUser()
{
	return g_hSteamUser;
}

//-----------------------------------------------------------------------------
// 
// Steam contentserver
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam content server API.
//-----------------------------------------------------------------------------
ISteamContentServer* SteamContentServer()
{
	return g_pSteamContentServer;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam content server utility API.
//-----------------------------------------------------------------------------
ISteamUtils* SteamContentServerUtils()
{
	return g_pSteamContentServerUtils;
}

//-----------------------------------------------------------------------------
// 
// Steam gameserver
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server API.
//-----------------------------------------------------------------------------
ISteamGameServer* SteamGameServer()
{
	return g_pSteamGameServer;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server utility API.
//-----------------------------------------------------------------------------
ISteamUtils* SteamGameServerUtils()
{
	return g_pSteamGameServerUtils;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server apps API.
//-----------------------------------------------------------------------------
ISteamApps *SteamGameServerApps()
{
	return g_pSteamGameServerApps;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server networking API.
//-----------------------------------------------------------------------------
ISteamNetworking *SteamGameServerNetworking()
{
	return g_pSteamGameServerNetworking;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server stats API.
//-----------------------------------------------------------------------------
ISteamGameServerStats *SteamGameServerStats()
{
	return g_pSteamGameServerStats;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam game server HTTP API.
//-----------------------------------------------------------------------------
ISteamHTTP *SteamGameServerHTTP()
{
	return g_pSteamGameServerHTTP;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam pipe object handle.
//-----------------------------------------------------------------------------
HSteamPipe SteamGameServer_GetHSteamPipe()
{
	return g_hSteamGameServerPipe;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor to steam user object handle.
//-----------------------------------------------------------------------------
HSteamUser SteamGameServer_GetHSteamUser()
{
	return g_hSteamGameServerUser;
}