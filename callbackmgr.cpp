//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "steam_api_pch.h"

// Set inside CCallbackMgr constructor and destructor. True if the class has been
// instantiated and the constructor was called. False if the class object has been
// destroyed and the destructor was called.
static bool s_bCallbackManagerInitialized = false;

// Mutex lock for callback dispatch
static bool s_bRunningCallbacks = false;

//-----------------------------------------------------------------------------
// 
// Callback manager class
// 
//-----------------------------------------------------------------------------

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
	void Register(CCallbackBase *pCallback, int iCallback);
	void Unregister(CCallbackBase *pCallback);

	void RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall);
	void UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall);

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
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCallbackMgr::CCallbackMgr() :
	// Callbacks for game server and steam client
	m_SteamCallback(nullptr, nullptr),
	m_SteamGameServerCallback(nullptr, nullptr), 

	// Exported callback entries from steamclient module
	pfnSteam_BGetCallback(nullptr), 
	pfnSteam_FreeLastCallback(nullptr),
	pfnSteam_GetAPICallResult(nullptr),
	pfnSteam_CallbackDispatchMsg(nullptr),

	// Communication to the steam client
	m_hSteamPipe(NULL),
	m_hSteamUser(NULL)
{
	// API call maps
	m_CallbackMap.clear();
	m_APICallMap.clear();

	s_bCallbackManagerInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCallbackMgr::~CCallbackMgr()
{
	s_bCallbackManagerInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: Adds new callback entry to the map
//-----------------------------------------------------------------------------
void CCallbackMgr::Register(CCallbackBase* pCallback, int iCallback)
{
	// Tell that we are registered
	pCallback->m_nCallbackFlags |= pCallback->k_ECallbackFlagsRegistered;
	pCallback->m_iCallback = iCallback;

	m_CallbackMap.insert(std::make_pair(iCallback, pCallback));
}

//-----------------------------------------------------------------------------
// Purpose: Looks for a specific callback inside the map, and if there's a match, 
//			matched callback entry will be erased from the map.
//-----------------------------------------------------------------------------
void CCallbackMgr::Unregister(CCallbackBase *pCallback)
{
	// If already unregistered there's no need to do it again
	if (!(pCallback->m_nCallbackFlags & CCallbackBase::k_ECallbackFlagsRegistered))
		return;

	// Mark as unregistered so we don't then process unregisterd callback
	pCallback->m_nCallbackFlags &= ~CCallbackBase::k_ECallbackFlagsRegistered;

	// Find matched callback and unregister it from the list
	auto Iter = m_CallbackMap.find(pCallback->GetICallback());
	if (Iter != m_CallbackMap.end())
	{
		if (Iter->second == pCallback)
		{
			if (Iter == m_CallbackMap.begin())
				Iter++;

			m_CallbackMap.erase(Iter);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds new call result to the map
//-----------------------------------------------------------------------------
void CCallbackMgr::RegisterCallResult(CCallbackBase* pCallback, SteamAPICall_t hAPICall)
{
	m_APICallMap.insert(std::make_pair(hAPICall, pCallback));
}

//-----------------------------------------------------------------------------
// Purpose: Looks for a specific APICall handle entry and erases it from the map.
//-----------------------------------------------------------------------------
void CCallbackMgr::UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall)
{
	// If already unregistered there's no need to do it again
	if (!(pCallback->m_nCallbackFlags & CCallbackBase::k_ECallbackFlagsRegistered))
		return;

	// Mark as unregistered so we don't then process unregisterd api call
	pCallback->m_nCallbackFlags &= ~CCallbackBase::k_ECallbackFlagsRegistered;

	// Find matched api call and unregister it from the list
	auto Iter = m_APICallMap.find(pCallback->GetICallback());
	if (Iter != m_APICallMap.end())
	{
		if (Iter->second == pCallback)
		{
			if (Iter == m_APICallMap.begin())
				Iter++;

			m_APICallMap.erase(Iter);
			return;
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Register internal steamclient callback API and each callback object.
//-----------------------------------------------------------------------------
void CCallbackMgr::RegisterInterfaceFuncs(HMODULE hModule)
{
	pfnSteam_BGetCallback = reinterpret_cast<pfnSteam_BGetCallback_t>(GetProcAddress(hModule, "Steam_BGetCallback"));
	pfnSteam_FreeLastCallback = reinterpret_cast<pfnSteam_FreeLastCallback_t>(GetProcAddress(hModule, "Steam_FreeLastCallback"));
	pfnSteam_GetAPICallResult = reinterpret_cast<pfnSteam_GetAPICallResult_t>(GetProcAddress(hModule, "Steam_GetAPICallResult"));

	// Manually register callbacks for both clients
	m_SteamCallback.Register(this, &CCallbackMgr::OnSteamAPICallCompleted);
	m_SteamGameServerCallback.Register(this, &CCallbackMgr::OnSteamAPICallCompleted);
}

//-----------------------------------------------------------------------------
// Purpose: Routine that is called on APICall completion. It's responsible for
//			executing the callback and then for unregistering it.
//-----------------------------------------------------------------------------
void CCallbackMgr::OnSteamAPICallCompleted(SteamAPICallCompleted_t *pCompletedSteamAPICall)
{
	void*			pCallbackData;
	bool			bIOFailed;
	CCallbackBase*	pCallbackBase;
	int				iCallbackSize;
	SteamAPICall_t	hAPICall;

	hAPICall = pCompletedSteamAPICall->m_hAsyncCall;
	
	auto APICall = m_APICallMap.find(hAPICall);
	if (APICall == m_APICallMap.end())
		return;

	pCallbackBase = APICall->second;
	iCallbackSize = pCallbackBase->GetCallbackSizeBytes();
	bIOFailed = false;

	pCallbackData = malloc(iCallbackSize);

	// Try to dispatch the callback
	if (pfnSteam_GetAPICallResult(m_hSteamPipe, hAPICall, pCallbackData, iCallbackSize, pCallbackBase->GetICallback(), &bIOFailed))
	{
		pCallbackBase->Run(pCallbackData, bIOFailed, hAPICall);
	}

	free(pCallbackData);

	// We don't need it no more
	UnregisterCallResult(pCallbackBase, hAPICall);
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches all sheduled callbacks depending on the communcation 
//			pipe. Whenether callbacks are dispatched is controlled by internal
//			steamclient API - pfnSteam_BGetCallback() function. After the callback
//			is dispatched, it's freed by calling pfnSteam_FreeLastCallback().
//-----------------------------------------------------------------------------
void CCallbackMgr::RunCallbacks(HSteamPipe hSteamPipe, bool bGameServerCallbacks)
{
	CallbackMsg_t CallbackMsg;

	if (!pfnSteam_BGetCallback || !pfnSteam_FreeLastCallback)
		return;

	// Cannot dispatch when already running
	if (s_bRunningCallbacks != false)
		return;

	s_bRunningCallbacks = true;
	m_hSteamPipe = hSteamPipe;

	// Execute callbacks till there's no more left
	while (pfnSteam_BGetCallback(hSteamPipe, &CallbackMsg))
	{
		m_hSteamUser = CallbackMsg.m_hSteamUser;

		// Call exception or non-exception cared callback dispatcher
		DispatchCallback(&CallbackMsg, bGameServerCallbacks);

		if (pfnSteam_FreeLastCallback)
			pfnSteam_FreeLastCallback(hSteamPipe);
	}

	m_hSteamPipe = NULL;
	s_bRunningCallbacks = false;
}

//-----------------------------------------------------------------------------
// Purpose: Executes exception-care or nonexception-care dispatch routine.
//-----------------------------------------------------------------------------
void CCallbackMgr::DispatchCallback(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks)
{
	if (g_bCatchExceptionsInCallbacks != false)
	{
		DispatchCallbackTryCatch(pCallbackMsg, bGameServerCallbacks);
	}
	else
	{
		DispatchCallbackNoTryCatch(pCallbackMsg, bGameServerCallbacks);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches all sheduled callbacks with try & catch exception handling.
//-----------------------------------------------------------------------------
void CCallbackMgr::DispatchCallbackTryCatch(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks)
{
	CCallbackBase*	pCallback;
	bool			bGameServer;

	try
	{
		bGameServer = false;

		// Look for callbacks with identical indexes and try to dispatch them
		auto Iter = m_CallbackMap.find(pCallbackMsg->m_iCallback);
		if (Iter != m_CallbackMap.end())
		{
			pCallback = Iter->second;

			if (bGameServerCallbacks == ((pCallback->m_nCallbackFlags & CCallbackBase::k_ECallbackFlagsGameServer) >> 1))
			{
				bGameServer = true;
				pCallback->Run(pCallbackMsg->m_pubParam);
			}
		}

		if (pfnSteam_CallbackDispatchMsg)
			pfnSteam_CallbackDispatchMsg(pCallbackMsg, bGameServer != false);
	}
	catch (...)
	{
#ifdef REGS_FIXES
		__debugbreak();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches all sheduled callbacks without try & catch exception 
//			handling. 
//-----------------------------------------------------------------------------
void CCallbackMgr::DispatchCallbackNoTryCatch(CallbackMsg_t *pCallbackMsg, bool bGameServerCallbacks)
{
	CCallbackBase*	pCallback;
	bool			bGameServer;
	
	bGameServer = false;

	// Look for callbacks with identical indexes and try to dispatch them
	auto Iter = m_CallbackMap.find(pCallbackMsg->m_iCallback);
	if (Iter != m_CallbackMap.end())
	{
		pCallback = Iter->second;

		if (bGameServerCallbacks == (((pCallback->m_nCallbackFlags & CCallbackBase::k_ECallbackFlagsGameServer) >> 1) == 1))
		{
			bGameServer = true;
			pCallback->Run(pCallbackMsg->m_pubParam);
		}
	}

	if (pfnSteam_CallbackDispatchMsg)
		pfnSteam_CallbackDispatchMsg(pCallbackMsg, bGameServer != false);
}

//-----------------------------------------------------------------------------
// 
// Callback manager C interface
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Singleton access
//-----------------------------------------------------------------------------
CCallbackMgr *GCallbackMgr()
{
	static CCallbackMgr CallbackManager;
	return &CallbackManager;
}

//-----------------------------------------------------------------------------
// Purpose: Adds new callback to the already existing map.
//-----------------------------------------------------------------------------
void CallbackMgr_RegisterCallback(CCallbackBase *pCallback, int iCallback)
{
	GCallbackMgr()->Register(pCallback, iCallback);
}

//-----------------------------------------------------------------------------
// Purpose: Finds already existing callback inside the map and erases it.
//-----------------------------------------------------------------------------
void CallbackMgr_UnregisterCallback(CCallbackBase *pCallback)
{
	if (s_bCallbackManagerInitialized != true)
		return;

	GCallbackMgr()->Unregister(pCallback);
}

//-----------------------------------------------------------------------------
// Purpose: Adds new call result to the already existing map.
//-----------------------------------------------------------------------------
void CallbackMgr_RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall)
{
	GCallbackMgr()->RegisterCallResult(pCallback, hAPICall);
}

//-----------------------------------------------------------------------------
// Purpose: Unregisters API call result from the map.
//-----------------------------------------------------------------------------
void CallbackMgr_UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t hAPICall)
{
	if (s_bCallbackManagerInitialized != true)
		return;

	GCallbackMgr()->UnregisterCallResult(pCallback, hAPICall);
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches a set of callbacks on specific pipe.
//-----------------------------------------------------------------------------
void CallbackMgr_RunCallbacks(HSteamPipe SteamPipe, bool bGameServerCallbacks)
{
	GCallbackMgr()->RunCallbacks(SteamPipe, bGameServerCallbacks);
}

//-----------------------------------------------------------------------------
// Purpose: Registers interface routines located inside specified module.
//-----------------------------------------------------------------------------
void CallbackMgr_RegisterInterfaceFuncs(HMODULE hModule)
{
	GCallbackMgr()->RegisterInterfaceFuncs(hModule);
}

//-----------------------------------------------------------------------------
// Purpose: Returns handle to steam user used by callback manager.
//-----------------------------------------------------------------------------
HSteamUser CallbackMgr_GetHSteamUserCurrent()
{
	return GCallbackMgr()->m_hSteamUser;
}