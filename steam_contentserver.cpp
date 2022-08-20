//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

//-----------------------------------------------------------------------------
// 
// Steam contentserver API
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Setups connection to the content server. Initialitzes global data
//			for the content server internal API.
//-----------------------------------------------------------------------------
bool SteamContentServer_Init(uint32 uContentServerID, uint32 unIP, uint16 usPort, uint16 usClientContentPort)
{
	// Locate and setup steam content server module
	g_pSteamContentServerClient = SteamAPI_Init_Internal(&g_hSteamContentServerModule, true);

	if (!g_pSteamContentServerClient)
		return false;

	// Set the local IP and Port to bind to. This must be called before CreateLocalUser().
	g_pSteamContentServerClient->SetLocalIPBinding(unIP, usPort);

	// Create local user object for this server class object
	g_hSteamContentServerUser = g_pSteamContentServerClient->CreateLocalUser(&g_hSteamContentServerPipe, k_EAccountTypeContentServer);

	if (!g_hSteamContentServerUser || !g_hSteamContentServerPipe)
		return false;

	// Create content server object for us
	g_pSteamContentServer = reinterpret_cast<ISteamContentServer*>(g_pSteamContentServerClient->GetISteamGenericInterface(
		g_hSteamContentServerUser, g_hSteamContentServerPipe, STEAMCONTENTSERVER_INTERFACE_VERSION));

	if (!g_pSteamContentServer)
		return false;

	// Create content server utility object
	g_pSteamContentServerUtils = g_pSteamContentServerClient->GetISteamUtils(g_hSteamContentServerPipe, STEAMUTILS_INTERFACE_VERSION);

	if (!g_pSteamContentServerUtils)
		return false;

	// Finally, log into content server
	g_pSteamContentServer->LogOn(uContentServerID);

	// Register callback functions to use to interact with the steam dll.
	Steam_RegisterInterfaceFuncs(g_hSteamContentServerModule);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shutsdown contentserver API
//-----------------------------------------------------------------------------
void SteamContentServer_Shutdown()
{
	if (g_pSteamContentServer && g_pSteamContentServer->BLoggedOn())
		g_pSteamContentServer->LogOff();

	if (!g_pSteamContentServerClient)
		return;

	if (g_hSteamContentServerPipe && g_hSteamContentServerUser)
		g_pSteamContentServerClient->ReleaseUser(g_hSteamContentServerPipe, g_hSteamContentServerUser);

	g_pSteamContentServer = nullptr;

	if (g_hSteamContentServerPipe)
		g_pSteamContentServerClient->BReleaseSteamPipe(g_hSteamContentServerPipe);

	g_hSteamContentServerPipe = NULL;

	if (g_pSteamContentServerClient)
		g_pSteamContentServerClient->BShutdownIfAllPipesClosed();

	g_pSteamContentServerClient = nullptr;

	if (g_hSteamContentServerModule)
		SteamAPI_Shutdown_Internal(g_hSteamContentServerModule);

	g_hSteamContentServerModule = nullptr;
}

//-----------------------------------------------------------------------------
// 
// Callback interface
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Executes contentserver callbacks
//-----------------------------------------------------------------------------
void SteamContentServer_RunCallbacks()
{
	if (g_hSteamContentServerPipe)
		Steam_RunCallbacks(g_hSteamContentServerPipe, 1);
}