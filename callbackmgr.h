//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CALLBACK_MGR_H
#define CALLBACK_MGR_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: Steam callbacks
//-----------------------------------------------------------------------------
typedef bool (*pfnSteam_BGetCallback_t)(HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg);
typedef void (*pfnSteam_FreeLastCallback_t)(HSteamPipe hSteamPipe);
typedef bool (*pfnSteam_GetAPICallResult_t)(HSteamPipe hSteamPipe, SteamAPICall_t hSteamAPICall, void* pCallback, int cubCallback, int iCallbackExpected, bool* pbFailed);
typedef bool (*pfnSteam_CallbackDispatchMsg_t)(CallbackMsg_t* pCallbackMessage, bool bGameServerCallbacks);

//-----------------------------------------------------------------------------
// Purpose: Callback management class
//-----------------------------------------------------------------------------
class CCallbackMgr
{
private:
	template<bool bGameServer>
	using SteamAPICallback = CCallback<CCallbackMgr, SteamAPICallCompleted_t, bGameServer>;

	template<class T>
	using CallbackMultimap = std::multimap<T, CCallbackBase*>;

public:
	CCallbackMgr();
	~CCallbackMgr();

public:
	void UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall);
	void Unregister(CCallbackBase *pCallback);

	void RegisterInterfaceFuncs(HMODULE hModule);

	void OnSteamAPICallCompleted(SteamAPICallCompleted_t *pCompletedSteamAPICall);

	// Callback dispatch
	void RunCallbacks(HSteamPipe hSteamPipe, bool bGameServerCallbacks);
	void DispatchCallback(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks);
	void DispatchCallbackTryCatch(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks);
	void DispatchCallbackNoTryCatch(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks);

public:
	// Call maps
	CallbackMultimap<int>				m_CallbackMap;
	CallbackMultimap<SteamAPICall_t>	m_APICallMap;

	// Callback steamclient API
	pfnSteam_BGetCallback_t 			pfnSteam_BGetCallback;
	pfnSteam_FreeLastCallback_t 		pfnSteam_FreeLastCallback;
	pfnSteam_GetAPICallResult_t 		pfnSteam_GetAPICallResult;
	
	// Communication
	HSteamUser 							m_hSteamUser;
	HSteamPipe 							m_hSteamPipe;

	// Callbacks
	pfnSteam_CallbackDispatchMsg_t 		pfnSteam_CallbackDispatchMsg;
	SteamAPICallback<false>				m_SteamCallback;
	SteamAPICallback<true>				m_SteamGameServerCallback;
};

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