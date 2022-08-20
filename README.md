# GoldSrc reverse-engineered steam_api module

This repository contains \*original\* source code for the _steam_api_ module that is used within GoldSource engine. 

# Disclaimer

I've posted this code for __educational purposes only__, thus it is not compileable, unless you set up the entire project including linkage and all include files necessary. Meaning that if you know what you're doing - you can easily compile and run this module by yourself.

While reversing this module, originality is retained and no changes are made or added to the existing code.

# Corectness

Bugs may exist. The code was tested on the newest version of the game and everything works as should, thus it's more likely that the code is sufficient and bug-proof enough to work without any issues. However there still may be some edge cases that I've either wasn't able to test properly or that I've missed. So testing and feedback is much appreciated.

# Information

The module serves a purpose of a communication pipeline between the game and some services from the internal _steamclient_ API. 

It is also responsible for loading all the _steamclient_ interfaces (_ISteamClient_, _ISteamUtils_, _IsteamHTTP_, etc..) for a particular module that calls _SteamAPI_Init()_ function.


# Where is the API referenced inside the engine

### The Download Manager and CMultipleCallResults class

One reference would be inside the _DLM_. Upon receiving files from the server to be downloaded, the _DLM_ schedules these files to be downloaded simultaneously. The hardcoded limit for GS is 5 at the time inside the queue. 

#### Requesting a file to be downloaded
```c++
// inside DownloadManager::StartNewDownload()
if (SteamHTTP()->SendHTTPRequest(..., &hSteamAPICall))
{
	// Add call for the steamAPI to process it
	m_HTTPRequestCompleted.AddCall(hSteamAPICall);
}
else
{
    // Handle error and release request
}
```

First a download request is requested from the _SteamHTTP_ interface. If the request has been accepted, an API call we got from the request can be passed into the _CMultipleCallResults_ class that then calls the internal API of _steam_api_ module. 

#### Adding an APICall to the CallbackManager
```c++
template <class T, class P>
inline void CMultipleCallResults<T, P>::AddCall(SteamAPICall_t hSteamAPICall)
{
    // ...

	m_mapAPICalls.Insert(hSteamAPICall);

	SteamAPI_RegisterCallResult(this, hSteamAPICall);
}
```

This is where the communication between the engine and _steam_api_ occurs. The _steam_api's_ _Callback Manager_ class takes care of these APICall handles and does some more communcation with steamclient API before calling the _DownloadManager::OnHTTPRequestCompleted()_ dispatch routine.
