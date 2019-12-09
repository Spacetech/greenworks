// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "steam_callbacks.h"

#include "nan.h"
#include "v8.h"

#include "greenworks_utils.h"

namespace greenworks
{
	SteamCallbacks::SteamCallbacks()
	{
		OnGameOverlayActivatedCallback = nullptr;
		OnGameJoinRequestedCallback = nullptr;
		OnLobbyCreatedCallback = nullptr;
		OnLobbyEnteredCallback = nullptr;
		OnLobbyChatUpdateCallback = nullptr;
		OnLobbyJoinRequestedCallback = nullptr;
		OnSteamRelayNetworkStatusCallback = nullptr;
	}

	void SteamCallbacks::OnGameOverlayActivated(GameOverlayActivated_t *pCallback)
	{
		if (OnGameOverlayActivatedCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::Boolean>(pCallback->m_bActive ? true : false)
		};

		OnGameOverlayActivatedCallback->Call(1, argv);
	}

	void SteamCallbacks::OnGameJoinRequested(GameRichPresenceJoinRequested_t *pCallback)
	{
		if (OnGameJoinRequestedCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::String>(pCallback->m_rgchConnect).ToLocalChecked()
		};

		OnGameJoinRequestedCallback->Call(1, argv);
	}

	void SteamCallbacks::OnLobbyCreated(LobbyCreated_t *pCallback)
	{
		if (OnLobbyCreatedCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::Boolean>(pCallback->m_eResult == k_EResultOK),
			Nan::New(utils::uint64ToString(pCallback->m_ulSteamIDLobby)).ToLocalChecked(),
			Nan::New<v8::Integer>(pCallback->m_eResult)
		};

		OnLobbyCreatedCallback->Call(3, argv);
	}

	void SteamCallbacks::OnLobbyEntered(LobbyEnter_t *pCallback)
	{
		if (OnLobbyEnteredCallback == nullptr)
		{
			return;
		}
		
		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::Boolean>(pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess),
			Nan::New(utils::uint64ToString(pCallback->m_ulSteamIDLobby)).ToLocalChecked(),
			Nan::New<v8::Integer>(pCallback->m_EChatRoomEnterResponse)
		};

		OnLobbyEnteredCallback->Call(3, argv);
	}

	void SteamCallbacks::OnLobbyChatUpdate(LobbyChatUpdate_t *pCallback)
	{
		if (OnLobbyChatUpdateCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New(utils::uint64ToString(pCallback->m_ulSteamIDLobby)).ToLocalChecked(),
			Nan::New(utils::uint64ToString(pCallback->m_ulSteamIDUserChanged)).ToLocalChecked(),
			Nan::New(pCallback->m_rgfChatMemberStateChange)
		};

		OnLobbyChatUpdateCallback->Call(3, argv);
	}

	void SteamCallbacks::OnLobbyJoinRequested(GameLobbyJoinRequested_t *pCallback)
	{
		if (OnLobbyJoinRequestedCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New(utils::uint64ToString(pCallback->m_steamIDLobby.ConvertToUint64())).ToLocalChecked()
		};

		OnLobbyJoinRequestedCallback->Call(1, argv);
	}

	void SteamCallbacks::OnSteamRelayNetworkStatus(SteamRelayNetworkStatus_t *pCallback)
	{
		if (OnSteamRelayNetworkStatusCallback == nullptr)
		{
			return;
		}

		v8::Local<v8::Value> argv[] = {
			Nan::New<v8::Integer>(pCallback->m_eAvail),
			Nan::New<v8::Integer>(pCallback->m_eAvailNetworkConfig),
			Nan::New<v8::Integer>(pCallback->m_eAvailAnyRelay)
		};

		OnSteamRelayNetworkStatusCallback->Call(1, argv);
	}
	
} // namespace greenworks
