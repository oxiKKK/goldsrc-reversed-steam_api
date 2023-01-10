//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CALLBACK_MGR_H
#define CALLBACK_MGR_H
#pragma once

//-----------------------------------------------------------------------------
// 
// Callback manager C interface
// 
//-----------------------------------------------------------------------------

extern CCallbackMgr *GCallbackMgr();
extern void CallbackMgr_RegisterCallback(CCallbackBase *pCallback, int iCallback);
extern void CallbackMgr_UnregisterCallback(CCallbackBase *pCallback);
extern void CallbackMgr_RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall);
extern void CallbackMgr_UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall);
extern void CallbackMgr_RunCallbacks(HSteamPipe SteamPipe, bool bGameServerCallbacks);
extern void CallbackMgr_RegisterInterfaceFuncs(HMODULE hModule);
extern HSteamUser CallbackMgr_GetHSteamUserCurrent();

#endif