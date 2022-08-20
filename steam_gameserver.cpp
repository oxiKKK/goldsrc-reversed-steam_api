//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// 
// Steam gameserver API
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Setups game server API and data in a 'safe' way.
//-----------------------------------------------------------------------------
bool SteamGameServer_InitSafe(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString)
{
	return SteamGameServer_Init_Internal(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString, true);
}

//-----------------------------------------------------------------------------
// Purpose: Setups game server API and data.
//-----------------------------------------------------------------------------
bool SteamGameServer_Init(uint32 unIP, uint16 usSteamPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString)
{
	return SteamGameServer_Init_Internal(unIP, usSteamPort, usGamePort, usQueryPort, eServerMode, pchVersionString, false);
}

//-----------------------------------------------------------------------------
// Purpose: Shutsdown gameserver API
//-----------------------------------------------------------------------------
void SteamGameServer_Shutdown()
{
	if (g_pSteamGameServer && g_pSteamGameServer->BLoggedOn())
		g_pSteamGameServer->LogOff();

	if (!g_pSteamClientGameServer)
		return;

	if (g_hSteamGameServerPipe && g_hSteamGameServerUser)
		g_pSteamClientGameServer->ReleaseUser(g_hSteamGameServerPipe, g_hSteamGameServerUser);

	g_pSteamGameServer = nullptr;

	if (g_hSteamGameServerPipe)
		g_pSteamClientGameServer->BReleaseSteamPipe(g_hSteamGameServerPipe);

	g_hSteamGameServerPipe = NULL;

	if (g_pSteamClientGameServer)
		g_pSteamClientGameServer->BShutdownIfAllPipesClosed();

	g_pSteamClientGameServer = nullptr;

	if (g_hSteamGameServerModule)
		SteamAPI_Shutdown_Internal(g_hSteamGameServerModule);

	g_hSteamGameServerModule = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Returns false if the server is initialized with no authentication.
//-----------------------------------------------------------------------------
bool SteamGameServer_BSecure()
{
	if (g_eGameServerMode == eServerModeNoAuthentication)
		return false;

	if (!g_pSteamGameServer)
		return false;

	return g_pSteamGameServer->BSecure();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of IPC calls made since the last time this 
//			function was called.
//-----------------------------------------------------------------------------
uint32 SteamGameServer_GetIPCCallCount()
{
	if (!g_pSteamGameServerUtils)
		return NULL;

	return g_pSteamGameServerUtils->GetIPCCallCount();
}

//-----------------------------------------------------------------------------
// Purpose: Returns game server SID. If the server is initialized with no 
//			authentication, then (1 << 56) is returned.
//-----------------------------------------------------------------------------
uint64 SteamGameServer_GetSteamID()
{
	if (g_eGameServerMode == eServerModeNoAuthentication)
		return (1ull << (64 - 8));

	if (!g_pSteamGameServer)
		return NULL;

	return g_pSteamGameServer->GetSteamID().ConvertToUint64();
}

//-----------------------------------------------------------------------------
// 
// Callback interface
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Runs callback on steam game server pipe
//-----------------------------------------------------------------------------
void SteamGameServer_RunCallbacks()
{
	if (g_hSteamGameServerPipe)
		Steam_RunCallbacks(g_hSteamGameServerPipe, true);
}
