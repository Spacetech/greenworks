// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_STEAM_CALLBACKS_H_
#define SRC_STEAM_CALLBACKS_H_

#include "nan.h"
#include "steam/isteamnetworking.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/steam_api.h"
#include "steam/steamnetworkingtypes.h"

#define SETUP_STEAM_CALLBACK_DECLARATION(name, type) \
    Nan::Callback* name##Callback;                   \
    STEAM_CALLBACK(SteamCallbacks, name, type, m_Callback##name)

#define SETUP_STEAM_CALLBACK_MEMBER(name) \
    m_Callback##name(this, &SteamCallbacks::name)

namespace greenworks
{
    class SteamCallbacks
    {
      public:
        SteamCallbacks();

        SETUP_STEAM_CALLBACK_DECLARATION(OnGameOverlayActivated, GameOverlayActivated_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnGameJoinRequested, GameRichPresenceJoinRequested_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnLobbyCreated, LobbyCreated_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnLobbyEntered, LobbyEnter_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnLobbyChatUpdate, LobbyChatUpdate_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnLobbyJoinRequested, GameLobbyJoinRequested_t);
        // SETUP_STEAM_CALLBACK_DECLARATION(OnSteamRelayNetworkStatus, SteamRelayNetworkStatus_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnP2PSessionRequest, P2PSessionRequest_t);
        SETUP_STEAM_CALLBACK_DECLARATION(OnP2PSessionConnectFail, P2PSessionConnectFail_t);
    };
} // namespace greenworks

#endif // SRC_STEAM_CALLBACKS_H_
