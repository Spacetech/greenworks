// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "steam_callbacks.h"

#include "napi.h"
#include "uv.h"
#include "v8.h"

#include "greenworks_utils.h"

namespace greenworks
{
    SteamCallbacks::SteamCallbacks()
        : SETUP_STEAM_CALLBACK_MEMBER(OnGameOverlayActivated),
          SETUP_STEAM_CALLBACK_MEMBER(OnGameJoinRequested),
          SETUP_STEAM_CALLBACK_MEMBER(OnLobbyCreated),
          SETUP_STEAM_CALLBACK_MEMBER(OnLobbyEntered),
          SETUP_STEAM_CALLBACK_MEMBER(OnLobbyChatUpdate),
          SETUP_STEAM_CALLBACK_MEMBER(OnLobbyJoinRequested),
          SETUP_STEAM_CALLBACK_MEMBER(OnP2PSessionRequest),
          SETUP_STEAM_CALLBACK_MEMBER(OnP2PSessionConnectFail)
    {
    }

    void SteamCallbacks::OnGameOverlayActivated(GameOverlayActivated_t* pCallback)
    {
        if (!OnGameOverlayActivatedCallback.IsEmpty())
        {
            Napi::Env env = OnGameOverlayActivatedCallback.Env();

            OnGameOverlayActivatedCallback.Call({Napi::Boolean::New(env, pCallback->m_bActive ? true : false)});
        }
    }

    void SteamCallbacks::OnGameJoinRequested(GameRichPresenceJoinRequested_t* pCallback)
    {
        if (!OnGameJoinRequestedCallback.IsEmpty())
        {
            Napi::Env env = OnGameJoinRequestedCallback.Env();

            OnGameJoinRequestedCallback.Call({Napi::String::New(env, pCallback->m_rgchConnect)});
        }
    }

    void SteamCallbacks::OnLobbyCreated(LobbyCreated_t* pCallback)
    {
        if (!OnLobbyCreatedCallback.IsEmpty())
        {
            Napi::Env env = OnLobbyCreatedCallback.Env();

            OnLobbyCreatedCallback.Call({
                Napi::Boolean::New(env, pCallback->m_eResult == k_EResultOK),
                Napi::String::New(env, utils::uint64ToString(pCallback->m_ulSteamIDLobby)),
                Napi::Number::New(env, pCallback->m_eResult),
            });
        }
    }

    void SteamCallbacks::OnLobbyEntered(LobbyEnter_t* pCallback)
    {
        if (!OnLobbyEnteredCallback.IsEmpty())
        {
            Napi::Env env = OnLobbyEnteredCallback.Env();

            OnLobbyEnteredCallback.Call({
                Napi::Boolean::New(env, pCallback->m_EChatRoomEnterResponse == k_EChatRoomEnterResponseSuccess),
                Napi::String::New(env, utils::uint64ToString(pCallback->m_ulSteamIDLobby)),
                Napi::Number::New(env, pCallback->m_EChatRoomEnterResponse),
            });
        }
    }

    void SteamCallbacks::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback)
    {
        if (!OnLobbyChatUpdateCallback.IsEmpty())
        {
            Napi::Env env = OnLobbyChatUpdateCallback.Env();

            OnLobbyChatUpdateCallback.Call({
                Napi::String::New(env, utils::uint64ToString(pCallback->m_ulSteamIDLobby)),
                Napi::String::New(env, utils::uint64ToString(pCallback->m_ulSteamIDUserChanged)),
                Napi::Number::New(env, pCallback->m_rgfChatMemberStateChange),
            });
        }
    }

    void SteamCallbacks::OnLobbyJoinRequested(GameLobbyJoinRequested_t* pCallback)
    {
        if (!OnLobbyJoinRequestedCallback.IsEmpty())
        {
            Napi::Env env = OnLobbyJoinRequestedCallback.Env();

            OnLobbyJoinRequestedCallback.Call({
                Napi::String::New(env, utils::uint64ToString(pCallback->m_steamIDLobby.ConvertToUint64())),
            });
        }
    }

    void SteamCallbacks::OnP2PSessionRequest(P2PSessionRequest_t* pCallback)
    {
        if (!OnP2PSessionRequestCallback.IsEmpty())
        {
            Napi::Env env = OnP2PSessionRequestCallback.Env();

            OnP2PSessionRequestCallback.Call({
                Napi::String::New(env, utils::uint64ToString(pCallback->m_steamIDRemote.ConvertToUint64())),
            });
        }
    }

    void SteamCallbacks::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback)
    {
        if (!OnP2PSessionConnectFailCallback.IsEmpty())
        {
            Napi::Env env = OnP2PSessionConnectFailCallback.Env();

            OnP2PSessionConnectFailCallback.Call({
                Napi::String::New(env, utils::uint64ToString(pCallback->m_steamIDRemote.ConvertToUint64())),
                Napi::Number::New(env, pCallback->m_eP2PSessionError),
            });
        }
    }

} // namespace greenworks
