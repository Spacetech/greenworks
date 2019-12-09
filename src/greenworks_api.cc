// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <sstream>
#include <string>
#include <vector>

#include "nan.h"
#include "steam/steam_api.h"
#include "v8.h"

#include "greenworks_async_workers.h"
#include "greenworks_utils.h"
#include "greenworks_workshop_workers.h"
#include "steam_callbacks.h"

#define SET_FUNCTION(function_name, function) \
    Nan::Set(exports, Nan::New(function_name).ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(function)).ToLocalChecked())

#define SET_FUNCTION_TPL(function_name, function) \
    Nan::SetMethod(tpl, function_name, function)

greenworks::SteamCallbacks* steamCallbacks = nullptr;

namespace
{
#define THROW_BAD_ARGS(msg)       \
    do                            \
    {                             \
        Nan::ThrowTypeError(msg); \
        return;                   \
    } while (0);

    v8::Local<v8::Object> GetSteamUserCountType(int type_id)
    {
        v8::Local<v8::Object> account_type = Nan::New<v8::Object>();
        std::string name;
        switch (type_id)
        {
        case k_EAccountTypeAnonGameServer:
            name = "k_EAccountTypeAnonGameServer";
            break;
        case k_EAccountTypeAnonUser:
            name = "k_EAccountTypeAnonUser";
            break;
        case k_EAccountTypeChat:
            name = "k_EAccountTypeChat";
            break;
        case k_EAccountTypeClan:
            name = "k_EAccountTypeClan";
            break;
        case k_EAccountTypeConsoleUser:
            name = "k_EAccountTypeConsoleUser";
            break;
        case k_EAccountTypeContentServer:
            name = "k_EAccountTypeContentServer";
            break;
        case k_EAccountTypeGameServer:
            name = "k_EAccountTypeGameServer";
            break;
        case k_EAccountTypeIndividual:
            name = "k_EAccountTypeIndividual";
            break;
        case k_EAccountTypeInvalid:
            name = "k_EAccountTypeInvalid";
            break;
        case k_EAccountTypeMax:
            name = "k_EAccountTypeMax";
            break;
        case k_EAccountTypeMultiseat:
            name = "k_EAccountTypeMultiseat";
            break;
        case k_EAccountTypePending:
            name = "k_EAccountTypePending";
            break;
        }
        Nan::Set(account_type, Nan::New("name").ToLocalChecked(), Nan::New(name).ToLocalChecked());
        Nan::Set(account_type, Nan::New("value").ToLocalChecked(), Nan::New(type_id));
        return account_type;
    }

    NAN_METHOD(InitAPI)
    {
        Nan::HandleScope scope;

        bool success = SteamAPI_Init();

        if (success)
        {
            ISteamUserStats* steam_user_stats = SteamUserStats();
            steam_user_stats->RequestCurrentStats();
            steam_user_stats->RequestGlobalStats(1);

            if (steamCallbacks == nullptr)
            {
                // we should probably free this at somepoint
                steamCallbacks = new greenworks::SteamCallbacks();
            }
        }

        info.GetReturnValue().Set(Nan::New(success));
    }

    NAN_METHOD(GetSteamId)
    {
        Nan::HandleScope scope;
        CSteamID user_id = SteamUser()->GetSteamID();
        v8::Local<v8::Object> flags = Nan::New<v8::Object>();
        Nan::Set(flags, Nan::New("anonymous").ToLocalChecked(), Nan::New(user_id.BAnonAccount()));
        Nan::Set(flags, Nan::New("anonymousGameServer").ToLocalChecked(),
                 Nan::New(user_id.BAnonGameServerAccount()));
        Nan::Set(flags, Nan::New("anonymousGameServerLogin").ToLocalChecked(),
                 Nan::New(user_id.BBlankAnonAccount()));
        Nan::Set(flags, Nan::New("anonymousUser").ToLocalChecked(), Nan::New(user_id.BAnonUserAccount()));
        Nan::Set(flags, Nan::New("chat").ToLocalChecked(), Nan::New(user_id.BChatAccount()));
        Nan::Set(flags, Nan::New("clan").ToLocalChecked(), Nan::New(user_id.BClanAccount()));
        Nan::Set(flags, Nan::New("consoleUser").ToLocalChecked(), Nan::New(user_id.BConsoleUserAccount()));
        Nan::Set(flags, Nan::New("contentServer").ToLocalChecked(), Nan::New(user_id.BContentServerAccount()));
        Nan::Set(flags, Nan::New("gameServer").ToLocalChecked(), Nan::New(user_id.BGameServerAccount()));
        Nan::Set(flags, Nan::New("individual").ToLocalChecked(), Nan::New(user_id.BIndividualAccount()));
        Nan::Set(flags, Nan::New("gameServerPersistent").ToLocalChecked(), Nan::New(user_id.BPersistentGameServerAccount()));
        Nan::Set(flags, Nan::New("lobby").ToLocalChecked(), Nan::New(user_id.IsLobby()));

        v8::Local<v8::Object> result = Nan::New<v8::Object>();
        Nan::Set(result, Nan::New("flags").ToLocalChecked(), flags);
        Nan::Set(result, Nan::New("type").ToLocalChecked(), GetSteamUserCountType(user_id.GetEAccountType()));
        Nan::Set(result, Nan::New("accountId").ToLocalChecked(), Nan::New<v8::Integer>(user_id.GetAccountID()));
        Nan::Set(result, Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(user_id.GetStaticAccountKey())).ToLocalChecked());
        Nan::Set(result, Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(user_id.ConvertToUint64())).ToLocalChecked());
        Nan::Set(result, Nan::New("isValid").ToLocalChecked(), Nan::New<v8::Integer>(user_id.IsValid()));
        Nan::Set(result, Nan::New("level").ToLocalChecked(), Nan::New<v8::Integer>(SteamUser()->GetPlayerSteamLevel()));

        if (!SteamFriends()->RequestUserInformation(user_id, true))
        {
            Nan::Set(result, Nan::New("screenName").ToLocalChecked(),
                     Nan::New(SteamFriends()->GetFriendPersonaName(user_id)).ToLocalChecked());
        }
        else
        {
            std::ostringstream sout;
            sout << user_id.GetAccountID();
            Nan::Set(result, Nan::New("screenName").ToLocalChecked(), Nan::New<v8::String>(sout.str()).ToLocalChecked());
        }

        info.GetReturnValue().Set(result);
    }

    NAN_METHOD(RunCallbacks)
    {
        Nan::HandleScope scope;

        SteamAPI_RunCallbacks();

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(SaveTextToFile)
    {
        Nan::HandleScope scope;

        if (info.Length() < 3 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string file_name(*(Nan::Utf8String(info[0])));
        std::string content(*(Nan::Utf8String(info[1])));
        Nan::Callback* success_callback = new Nan::Callback(info[2].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 3 && info[3]->IsFunction())
            error_callback = new Nan::Callback(info[3].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::FileContentSaveWorker(success_callback,
                                                                    error_callback,
                                                                    file_name,
                                                                    content));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(SaveFilesToCloud)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsArray() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        v8::Local<v8::Array> files = info[0].As<v8::Array>();
        std::vector<std::string> files_path;
        for (uint32_t i = 0; i < files->Length(); ++i)
        {
            if (!Nan::Get(files, i).ToLocalChecked()->IsString())
                THROW_BAD_ARGS("Bad arguments");
            v8::String::Utf8Value string_array(Nan::Get(files, i).ToLocalChecked());
            // Ignore empty path.
            if (string_array.length() > 0)
                files_path.push_back(*string_array);
        }

        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());
        Nan::AsyncQueueWorker(new greenworks::FilesSaveWorker(success_callback,
                                                              error_callback,
                                                              files_path));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ReadTextFromFile)
    {
        Nan::HandleScope scope;

        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string file_name(*(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::FileReadWorker(success_callback,
                                                             error_callback,
                                                             file_name));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(IsCloudEnabled)
    {
        Nan::HandleScope scope;
        ISteamRemoteStorage* steam_remote_storage = SteamRemoteStorage();
        info.GetReturnValue().Set(Nan::New<v8::Boolean>(
            steam_remote_storage->IsCloudEnabledForApp()));
    }

    NAN_METHOD(IsCloudEnabledForUser)
    {
        Nan::HandleScope scope;
        ISteamRemoteStorage* steam_remote_storage = SteamRemoteStorage();
        info.GetReturnValue().Set(Nan::New<v8::Boolean>(
            steam_remote_storage->IsCloudEnabledForAccount()));
    }

    NAN_METHOD(EnableCloud)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1)
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        bool enable_flag = info[0]->BooleanValue();
        SteamRemoteStorage()->SetCloudEnabledForApp(enable_flag);
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetCloudQuota)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        Nan::Callback* success_callback = new Nan::Callback(info[0].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[1]->IsFunction())
            error_callback = new Nan::Callback(info[1].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::CloudQuotaGetWorker(success_callback,
                                                                  error_callback));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ActivateAchievement)
    {
        Nan::HandleScope scope;

        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string achievement = (*(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::ActivateAchievementWorker(
            success_callback, error_callback, achievement));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetAchievement)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string achievement = (*(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());
        Nan::AsyncQueueWorker(new greenworks::GetAchievementWorker(
            success_callback, error_callback, achievement));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ClearAchievement)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string achievement = (*(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::ClearAchievementWorker(
            success_callback, error_callback, achievement));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetAchievementNames)
    {
        Nan::HandleScope scope;
        int count = static_cast<int>(SteamUserStats()->GetNumAchievements());
        v8::Local<v8::Array> names = Nan::New<v8::Array>(count);
        for (int i = 0; i < count; ++i)
        {
            Nan::Set(names, i, Nan::New(SteamUserStats()->GetAchievementName(i)).ToLocalChecked());
        }
        info.GetReturnValue().Set(names);
    }

    NAN_METHOD(GetNumberOfAchievements)
    {
        Nan::HandleScope scope;
        ISteamUserStats* steam_user_stats = SteamUserStats();
        info.GetReturnValue().Set(steam_user_stats->GetNumAchievements());
    }

    NAN_METHOD(GetCurrentBetaName)
    {
        Nan::HandleScope scope;

        std::string betaNameStr = "public";

        char* betaName = new char[1024];
        if (SteamApps()->GetCurrentBetaName(betaName, 1024))
        {
            betaNameStr = std::string(betaName);
        }

        delete[] betaName;

        info.GetReturnValue().Set(Nan::New(betaNameStr).ToLocalChecked());
    }

    NAN_METHOD(GetCurrentGameLanguage)
    {
        Nan::HandleScope scope;
        info.GetReturnValue().Set(Nan::New(SteamApps()->GetCurrentGameLanguage()).ToLocalChecked());
    }

    NAN_METHOD(GetCurrentUILanguage)
    {
        Nan::HandleScope scope;
        info.GetReturnValue().Set(Nan::New(SteamUtils()->GetSteamUILanguage()).ToLocalChecked());
    }

    NAN_METHOD(GetCurrentGameInstallDir)
    {
        Nan::HandleScope scope;

        uint32 appId = SteamUtils()->GetAppID();

        char* installDir = new char[1024];
        uint32 endPos = SteamApps()->GetAppInstallDir(appId, installDir, 1024);
        installDir[endPos] = '\0';

        std::string installDirRet = std::string(installDir);

        delete[] installDir;

        info.GetReturnValue().Set(Nan::New(installDirRet).ToLocalChecked());
    }

    NAN_METHOD(GetNumberOfPlayers)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        Nan::Callback* success_callback = new Nan::Callback(info[0].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 1 && info[1]->IsFunction())
            error_callback = new Nan::Callback(info[1].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::GetNumberOfPlayersWorker(
            success_callback, error_callback));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(IsGameOverlayEnabled)
    {
        Nan::HandleScope scope;
        info.GetReturnValue().Set(Nan::New(SteamUtils()->IsOverlayEnabled()));
    }

    NAN_METHOD(ActivateGameOverlay)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string option(*(Nan::Utf8String(info[0])));
        SteamFriends()->ActivateGameOverlay(option.c_str());
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ActivateGameOverlayInviteDialog)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string steamIdString(*(Nan::Utf8String(info[0])));
        CSteamID lobbyId(utils::strToUint64(steamIdString));

        SteamFriends()->ActivateGameOverlayInviteDialog(lobbyId);
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ActivateGameOverlayToWebPage)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string option(*(Nan::Utf8String(info[0])));
        SteamFriends()->ActivateGameOverlayToWebPage(option.c_str());
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(OnGameOverlayActive)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnGameOverlayActivatedCallback != nullptr)
        {
            delete steamCallbacks->OnGameOverlayActivatedCallback;
            steamCallbacks->OnGameOverlayActivatedCallback = nullptr;
        }

        steamCallbacks->OnGameOverlayActivatedCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(OnGameJoinRequested)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnGameJoinRequestedCallback != nullptr)
        {
            delete steamCallbacks->OnGameJoinRequestedCallback;
            steamCallbacks->OnGameJoinRequestedCallback = nullptr;
        }

        steamCallbacks->OnGameJoinRequestedCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(SetRichPresence)
    {
        Nan::HandleScope scope;

        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string key(*(Nan::Utf8String(info[0])));
        std::string value(*(Nan::Utf8String(info[1])));

        if (key.length() > k_cchMaxRichPresenceKeyLength)
        {
            THROW_BAD_ARGS("Key too long");
        }

        if (value.length() > k_cchMaxRichPresenceValueLength)
        {
            THROW_BAD_ARGS("Value too long");
        }

        bool success = SteamFriends()->SetRichPresence(key.c_str(), value.c_str());

        info.GetReturnValue().Set(Nan::New(success));
    }

    NAN_METHOD(ClearRichPresence)
    {
        Nan::HandleScope scope;

        SteamFriends()->ClearRichPresence();

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(FileShare)
    {
        Nan::HandleScope scope;

        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string file_name(*(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::FileShareWorker(
            success_callback, error_callback, file_name));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(PublishWorkshopFile)
    {
        Nan::HandleScope scope;

        if (info.Length() < 6 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsString() || !info[3]->IsString() ||
            !info[4]->IsArray() || !info[5]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string file_name(*(Nan::Utf8String(info[0])));
        std::string image_name(*(Nan::Utf8String(info[1])));
        std::string title(*(Nan::Utf8String(info[2])));
        std::string description(*(Nan::Utf8String(info[3])));

        v8::Local<v8::Array> tagsArray = info[4].As<v8::Array>();
        std::vector<std::string> tags;
        for (uint32_t i = 0; i < tagsArray->Length(); ++i)
        {
            if (!Nan::Get(tagsArray, i).ToLocalChecked()->IsString())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            v8::String::Utf8Value string_array(Nan::Get(tagsArray, i).ToLocalChecked());
            if (string_array.length() > 0)
            {
                tags.push_back(*string_array);
            }
        }

        Nan::Callback* success_callback = new Nan::Callback(info[5].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 6 && info[6]->IsFunction())
            error_callback = new Nan::Callback(info[6].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::PublishWorkshopFileWorker(
            success_callback, error_callback, file_name, image_name, title,
            description, tags));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UpdatePublishedWorkshopFile)
    {
        Nan::HandleScope scope;

        if (info.Length() < 7 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsString() || !info[3]->IsString() || !info[4]->IsString() ||
            !info[5]->IsArray() || !info[6]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        PublishedFileId_t published_file_id = utils::strToUint64(
            *(Nan::Utf8String(info[0])));
        std::string file_name(*(Nan::Utf8String(info[1])));
        std::string image_name(*(Nan::Utf8String(info[2])));
        std::string title(*(Nan::Utf8String(info[3])));
        std::string description(*(Nan::Utf8String(info[4])));

        v8::Local<v8::Array> tagsArray = info[5].As<v8::Array>();
        std::vector<std::string> tags;
        for (uint32_t i = 0; i < tagsArray->Length(); ++i)
        {
            if (!Nan::Get(tagsArray, i).ToLocalChecked()->IsString())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            v8::String::Utf8Value string_array(Nan::Get(tagsArray, i).ToLocalChecked());
            if (string_array.length() > 0)
            {
                tags.push_back(*string_array);
            }
        }

        Nan::Callback* success_callback = new Nan::Callback(info[6].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 6 && info[7]->IsFunction())
            error_callback = new Nan::Callback(info[7].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::UpdatePublishedWorkshopFileWorker(
            success_callback, error_callback, published_file_id, file_name,
            image_name, title, description, tags));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCGetItems)
    {
        Nan::HandleScope scope;
        if (info.Length() < 3 || !info[0]->IsInt32() || !info[1]->IsInt32() ||
            !info[2]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(Nan::To<int32_t>(info[0]).FromJust());
        EUGCQuery ugc_query_type = static_cast<EUGCQuery>(Nan::To<int32_t>(info[1]).FromJust());

        Nan::Callback* success_callback = new Nan::Callback(info[2].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 3 && info[3]->IsFunction())
            error_callback = new Nan::Callback(info[3].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::QueryAllUGCWorker(
            success_callback, error_callback, ugc_matching_type, ugc_query_type));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCGetUserItems)
    {
        Nan::HandleScope scope;
        if (info.Length() < 4 || !info[0]->IsInt32() || !info[1]->IsInt32() ||
            !info[2]->IsInt32() || !info[3]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(Nan::To<int32_t>(info[0]).FromJust());
        EUserUGCListSortOrder ugc_list_order = static_cast<EUserUGCListSortOrder>(Nan::To<int32_t>(info[1]).FromJust());
        EUserUGCList ugc_list = static_cast<EUserUGCList>(Nan::To<int32_t>(info[2]).FromJust());

        Nan::Callback* success_callback = new Nan::Callback(info[3].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 4 && info[4]->IsFunction())
            error_callback = new Nan::Callback(info[4].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::QueryUserUGCWorker(
            success_callback, error_callback, ugc_matching_type, ugc_list,
            ugc_list_order));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCDownloadItem)
    {
        Nan::HandleScope scope;
        if (info.Length() < 3 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        UGCHandle_t download_file_handle = utils::strToUint64(
            *(Nan::Utf8String(info[0])));
        std::string download_dir = *(Nan::Utf8String(info[1]));

        Nan::Callback* success_callback = new Nan::Callback(info[2].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 3 && info[3]->IsFunction())
            error_callback = new Nan::Callback(info[3].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::DownloadItemWorker(
            success_callback, error_callback, download_file_handle, download_dir));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCSynchronizeItems)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string download_dir = *(Nan::Utf8String(info[0]));

        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::SynchronizeItemsWorker(
            success_callback, error_callback, download_dir));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCShowOverlay)
    {
        Nan::HandleScope scope;
        std::string steam_store_url;
        if (info.Length() < 1)
        {
            uint32 appId = SteamUtils()->GetAppID();
            steam_store_url = "http://steamcommunity.com/app/" +
                              utils::uint64ToString(appId) + "/workshop/";
        }
        else
        {
            if (!info[0]->IsString())
            {
                THROW_BAD_ARGS("Bad arguments");
            }
            std::string item_id = *(Nan::Utf8String(info[0]));
            steam_store_url = "http://steamcommunity.com/sharedfiles/filedetails/?id=" + item_id;
        }

        SteamFriends()->ActivateGameOverlayToWebPage(steam_store_url.c_str());
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCUnsubscribe)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        PublishedFileId_t unsubscribed_file_id = utils::strToUint64(
            *(Nan::Utf8String(info[0])));
        Nan::Callback* success_callback = new Nan::Callback(info[1].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 2 && info[2]->IsFunction())
            error_callback = new Nan::Callback(info[2].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::UnsubscribePublishedFileWorker(
            success_callback, error_callback, unsubscribed_file_id));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCStartPlaytimeTracking)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsArray())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        PublishedFileId_t published_file_id[100];

        v8::Local<v8::Array> array = info[0].As<v8::Array>();

        auto length = array->Length();

        if (length >= 100)
        {
            THROW_BAD_ARGS("Too many items");
        }

        for (uint32_t i = 0; i < length; ++i)
        {
            if (!Nan::Get(array, i).ToLocalChecked()->IsNumber())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            published_file_id[i] = utils::strToUint64(*(Nan::Utf8String(Nan::Get(array, i).ToLocalChecked())));
        }

        SteamUGC()->StartPlaytimeTracking(published_file_id, length);

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(UGCStopPlaytimeTracking)
    {
        Nan::HandleScope scope;

        SteamUGC()->StopPlaytimeTrackingForAllItems();

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(CreateArchive)
    {
        Nan::HandleScope scope;
        if (info.Length() < 5 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsString() || !info[3]->IsInt32() || !info[4]->IsFunction())
        {
            THROW_BAD_ARGS("bad arguments");
        }
        std::string zip_file_path = *(Nan::Utf8String(info[0]));
        std::string source_dir = *(Nan::Utf8String(info[1]));
        std::string password = *(Nan::Utf8String(info[2]));
        int compress_level = Nan::To<int32_t>(info[3]).FromJust();

        Nan::Callback* success_callback = new Nan::Callback(info[4].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 5 && info[5]->IsFunction())
            error_callback = new Nan::Callback(info[5].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::CreateArchiveWorker(
            success_callback, error_callback, zip_file_path, source_dir, password,
            compress_level));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ExtractArchive)
    {
        Nan::HandleScope scope;
        if (info.Length() < 4 || !info[0]->IsString() || !info[1]->IsString() ||
            !info[2]->IsString() || !info[3]->IsFunction())
        {
            THROW_BAD_ARGS("bad arguments");
        }
        std::string zip_file_path = *(Nan::Utf8String(info[0]));
        std::string extract_dir = *(Nan::Utf8String(info[1]));
        std::string password = *(Nan::Utf8String(info[2]));

        Nan::Callback* success_callback = new Nan::Callback(info[3].As<v8::Function>());
        Nan::Callback* error_callback = nullptr;

        if (info.Length() > 4 && info[4]->IsFunction())
            error_callback = new Nan::Callback(info[4].As<v8::Function>());

        Nan::AsyncQueueWorker(new greenworks::ExtractArchiveWorker(
            success_callback, error_callback, zip_file_path, extract_dir, password));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetFriends)
    {
        Nan::HandleScope scope;

        EFriendFlags friendFlags = EFriendFlags::k_EFriendFlagImmediate;

        int friendCount = SteamFriends()->GetFriendCount(friendFlags);

        v8::Local<v8::Array> friends = Nan::New<v8::Array>(static_cast<int>(friendCount));

        for (int i = 0; i < friendCount; i++)
        {
            CSteamID friendSteamId = SteamFriends()->GetFriendByIndex(i, friendFlags);
            if (friendSteamId.IsValid())
            {
                v8::Local<v8::Object> friendObject = Nan::New<v8::Object>();

                Nan::Set(friendObject, Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendSteamId.GetStaticAccountKey())).ToLocalChecked());
                Nan::Set(friendObject, Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendSteamId.ConvertToUint64())).ToLocalChecked());

                const char* friendName = SteamFriends()->GetFriendPersonaName(friendSteamId);
                if (friendName != NULL)
                {
                    Nan::Set(friendObject, Nan::New("name").ToLocalChecked(), Nan::New(std::string(friendName)).ToLocalChecked());
                }

                FriendGameInfo_t friendGameInfo;
                if (SteamFriends()->GetFriendGamePlayed(friendSteamId, &friendGameInfo))
                {
                    Nan::Set(friendObject, Nan::New("gameId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendGameInfo.m_gameID.ToUint64())).ToLocalChecked());
                    Nan::Set(friendObject, Nan::New("lobbyId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendGameInfo.m_steamIDLobby.ConvertToUint64())).ToLocalChecked());
                }

                Nan::Set(friends, i, friendObject);
            }
        }

        info.GetReturnValue().Set(friends);
    }

    NAN_METHOD(CreateLobby)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsNumber())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        int lobbyType = Nan::To<int32_t>(info[0]).FromJust();
        if (lobbyType < 0 || lobbyType > 3)
        {
            THROW_BAD_ARGS("bad arguments");
        }

        SteamMatchmaking()->CreateLobby((ELobbyType)lobbyType, 16);

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(LeaveLobby)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string steamIdString(*(Nan::Utf8String(info[0])));
        CSteamID lobbyId(utils::strToUint64(steamIdString));

        SteamMatchmaking()->LeaveLobby(lobbyId);

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(JoinLobby)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string steamIdString(*(Nan::Utf8String(info[0])));
        CSteamID lobbyId(utils::strToUint64(steamIdString));

        SteamMatchmaking()->JoinLobby(lobbyId);

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(SetLobbyType)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsNumber())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string lobbyIdString(*(Nan::Utf8String(info[0])));
        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        int lobbyType = Nan::To<int32_t>(info[1]).FromJust();
        if (lobbyType < 0 || lobbyType > 3)
        {
            THROW_BAD_ARGS("bad arguments");
        }

        bool success = SteamMatchmaking()->SetLobbyType(lobbyId, (ELobbyType)lobbyType);

        info.GetReturnValue().Set(Nan::New(success));
    }

    NAN_METHOD(GetLobbyData)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsString())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string lobbyIdString(*(Nan::Utf8String(info[0])));
        std::string key(*(Nan::Utf8String(info[1])));

        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        const char* data = SteamMatchmaking()->GetLobbyData(lobbyId, key.c_str());

        info.GetReturnValue().Set(Nan::New(std::string(data)).ToLocalChecked());
    }

    NAN_METHOD(SetLobbyData)
    {
        Nan::HandleScope scope;
        if (info.Length() < 3 || !info[0]->IsString() || !info[1]->IsString() || !info[2]->IsString())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string lobbyIdString(*(Nan::Utf8String(info[0])));
        std::string key(*(Nan::Utf8String(info[1])));
        std::string value(*(Nan::Utf8String(info[2])));

        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        bool success = SteamMatchmaking()->SetLobbyData(lobbyId, key.c_str(), value.c_str());

        info.GetReturnValue().Set(Nan::New(success));
    }

    NAN_METHOD(GetLobbyMembers)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("bad arguments");
        }

        std::string lobbyIdString(*(Nan::Utf8String(info[0]->ToString())));

        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        int lobbyMembersCount = SteamMatchmaking()->GetNumLobbyMembers(lobbyId);

        v8::Local<v8::Array> lobbyMembers = Nan::New<v8::Array>(static_cast<int>(lobbyMembersCount));

        for (int i = 0; i < lobbyMembersCount; i++)
        {
            CSteamID lobbyMemberSteamId = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyId, i);
            if (lobbyMemberSteamId.IsValid())
            {
                v8::Local<v8::Object> lobbyMemberObject = Nan::New<v8::Object>();

                Nan::Set(lobbyMemberObject, Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(lobbyMemberSteamId.GetStaticAccountKey())).ToLocalChecked());
                Nan::Set(lobbyMemberObject, Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(lobbyMemberSteamId.ConvertToUint64())).ToLocalChecked());

                const char* lobbyMemberName = SteamFriends()->GetFriendPersonaName(lobbyMemberSteamId);
                if (lobbyMemberName != NULL)
                {
                    Nan::Set(lobbyMemberObject, Nan::New("name").ToLocalChecked(), Nan::New(std::string(lobbyMemberName)).ToLocalChecked());
                }

                Nan::Set(lobbyMembers, i, lobbyMemberObject);
            }
        }

        info.GetReturnValue().Set(lobbyMembers);
    }

    NAN_METHOD(OnLobbyCreated)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnLobbyCreatedCallback != nullptr)
        {
            delete steamCallbacks->OnLobbyCreatedCallback;
            steamCallbacks->OnLobbyCreatedCallback = nullptr;
        }

        steamCallbacks->OnLobbyCreatedCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(OnLobbyEntered)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnLobbyEnteredCallback != nullptr)
        {
            delete steamCallbacks->OnLobbyEnteredCallback;
            steamCallbacks->OnLobbyEnteredCallback = nullptr;
        }

        steamCallbacks->OnLobbyEnteredCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(OnLobbyChatUpdate)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnLobbyChatUpdateCallback != nullptr)
        {
            delete steamCallbacks->OnLobbyChatUpdateCallback;
            steamCallbacks->OnLobbyChatUpdateCallback = nullptr;
        }

        steamCallbacks->OnLobbyChatUpdateCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(OnLobbyJoinRequested)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnLobbyJoinRequestedCallback != nullptr)
        {
            delete steamCallbacks->OnLobbyJoinRequestedCallback;
            steamCallbacks->OnLobbyJoinRequestedCallback = nullptr;
        }

        steamCallbacks->OnLobbyJoinRequestedCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetStatInt)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string name = (*(Nan::Utf8String(info[0])));
        int32 result = 0;
        if (SteamUserStats()->GetStat(name.c_str(), &result))
        {
            info.GetReturnValue().Set(result);
            return;
        }
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetStatFloat)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string name = (*(Nan::Utf8String(info[0])));
        float result = 0;
        if (SteamUserStats()->GetStat(name.c_str(), &result))
        {
            info.GetReturnValue().Set(result);
            return;
        }
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetGlobalStatInt)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string name = (*(Nan::Utf8String(info[0])));
        int64 result = 0;
        if (SteamUserStats()->GetGlobalStat(name.c_str(), &result))
        {
            info.GetReturnValue().Set((int32)result);
            return;
        }
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(GetGlobalStatFloat)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string name = (*(Nan::Utf8String(info[0])));
        double result = 0;
        if (SteamUserStats()->GetGlobalStat(name.c_str(), &result))
        {
            info.GetReturnValue().Set(result);
            return;
        }
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(SetStat)
    {
        Nan::HandleScope scope;
        if (info.Length() < 2 || !info[0]->IsString() || (!info[1]->IsNumber()))
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string name = *(Nan::Utf8String(info[0]));
        if (info[1]->IsInt32())
        {
            int32 value = Nan::To<int32_t>(info[1]).FromJust();
            info.GetReturnValue().Set(SteamUserStats()->SetStat(name.c_str(), value));
        }
        else
        {
            double value = info[1].As<v8::Number>()->NumberValue();
            info.GetReturnValue().Set(SteamUserStats()->SetStat(name.c_str(), static_cast<float>(value)));
        }
    }

    NAN_METHOD(StoreStats)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || (!info[0]->IsFunction()))
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        Nan::Callback* success_callback = new Nan::Callback(info[0].As<v8::Function>());

        Nan::Callback* error_callback = nullptr;
        if (info.Length() > 1 && info[1]->IsFunction())
        {
            error_callback = new Nan::Callback(info[1].As<v8::Function>());
        }

        Nan::AsyncQueueWorker(new greenworks::StoreUserStatsWorker(success_callback, error_callback));
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ResetAllStats)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || (!info[0]->IsBoolean()))
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        bool reset_achievement = info[0]->BooleanValue();
        info.GetReturnValue().Set(SteamUserStats()->ResetAllStats(reset_achievement));
    }

    NAN_METHOD(InitRelayNetworkAccess)
    {
        Nan::HandleScope scope;

        SteamNetworkingUtils()->InitRelayNetworkAccess();
        
        info.GetReturnValue().Set(Nan::Undefined());
    }
    
    NAN_METHOD(SetRelayNetworkStatusCallback)
    {
        Nan::HandleScope scope;

        if (info.Length() < 1 || !info[0]->IsFunction())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        if (steamCallbacks == nullptr)
        {
            THROW_BAD_ARGS("Internal error");
        }

        if (steamCallbacks->OnSteamRelayNetworkStatusCallback != nullptr)
        {
            delete steamCallbacks->OnSteamRelayNetworkStatusCallback;
            steamCallbacks->OnSteamRelayNetworkStatusCallback = nullptr;
        }

        steamCallbacks->OnSteamRelayNetworkStatusCallback = new Nan::Callback(info[0].As<v8::Function>());

        info.GetReturnValue().Set(Nan::Undefined());
    }
    
    void InitUtilsObject(v8::Local<v8::Object> exports)
    {
        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>();

        SET_FUNCTION_TPL("createArchive", CreateArchive);
        SET_FUNCTION_TPL("extractArchive", ExtractArchive);

        Nan::Persistent<v8::Function> constructor;
        constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
        Nan::Set(exports, Nan::New("Utils").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
    }

    void InitNetworkingObject(v8::Local<v8::Object> exports)
    {
        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>();

        SET_FUNCTION_TPL("initRelayNetworkAccess", InitRelayNetworkAccess);
        SET_FUNCTION_TPL("setRelayNetworkStatusCallback", SetRelayNetworkStatusCallback);

        Nan::Persistent<v8::Function> constructor;
        constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
        Nan::Set(exports, Nan::New("Networking").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
    }

    void init(v8::Local<v8::Object> exports)
    {
        // Common APIs.
        SET_FUNCTION("initAPI", InitAPI);
        SET_FUNCTION("getSteamId", GetSteamId);
        SET_FUNCTION("runCallbacks", RunCallbacks);

        // File APIs.
        SET_FUNCTION("saveTextToFile", SaveTextToFile);
        SET_FUNCTION("readTextFromFile", ReadTextFromFile);
        SET_FUNCTION("saveFilesToCloud", SaveFilesToCloud);

        // Cloud APIs.
        SET_FUNCTION("isCloudEnabled", IsCloudEnabled);
        SET_FUNCTION("isCloudEnabledForUser", IsCloudEnabledForUser);
        SET_FUNCTION("enableCloud", EnableCloud);
        SET_FUNCTION("getCloudQuota", GetCloudQuota);

        // Achievement APIs.
        SET_FUNCTION("activateAchievement", ActivateAchievement);
        SET_FUNCTION("clearAchievement", ClearAchievement);
        SET_FUNCTION("getAchievement", GetAchievement);
        SET_FUNCTION("getAchievementNames", GetAchievementNames);
        SET_FUNCTION("getNumberOfAchievements", GetNumberOfAchievements);

        // Game APIs.
        SET_FUNCTION("getCurrentBetaName", GetCurrentBetaName);
        SET_FUNCTION("getCurrentGameLanguage", GetCurrentGameLanguage);
        SET_FUNCTION("getCurrentUILanguage", GetCurrentUILanguage);
        SET_FUNCTION("getCurrentGameInstallDir", GetCurrentGameInstallDir);
        SET_FUNCTION("getNumberOfPlayers", GetNumberOfPlayers);
        SET_FUNCTION("isGameOverlayEnabled", IsGameOverlayEnabled);
        SET_FUNCTION("activateGameOverlay", ActivateGameOverlay);
        SET_FUNCTION("activateGameOverlayInviteDialog", ActivateGameOverlayInviteDialog);
        SET_FUNCTION("activateGameOverlayToWebPage", ActivateGameOverlayToWebPage);
        SET_FUNCTION("onGameOverlayActive", OnGameOverlayActive);
        SET_FUNCTION("onGameJoinRequested", OnGameJoinRequested);

        // Workshop APIs
        SET_FUNCTION("fileShare", FileShare);
        SET_FUNCTION("publishWorkshopFile", PublishWorkshopFile);
        SET_FUNCTION("updatePublishedWorkshopFile", UpdatePublishedWorkshopFile);
        SET_FUNCTION("ugcGetItems", UGCGetItems);
        SET_FUNCTION("ugcGetUserItems", UGCGetUserItems);
        SET_FUNCTION("ugcDownloadItem", UGCDownloadItem);
        SET_FUNCTION("ugcSynchronizeItems", UGCSynchronizeItems);
        SET_FUNCTION("ugcShowOverlay", UGCShowOverlay);
        SET_FUNCTION("ugcUnsubscribe", UGCUnsubscribe);

        SET_FUNCTION("startPlaytimeTracking", UGCStartPlaytimeTracking);
        SET_FUNCTION("stopPlaytimeTracking", UGCStopPlaytimeTracking);

        // Presence apis
        SET_FUNCTION("setRichPresence", SetRichPresence);
        SET_FUNCTION("clearRichPresence", ClearRichPresence);

        // Friend apis
        SET_FUNCTION("getFriends", GetFriends);

        // Stat apis
        SET_FUNCTION("getStatInt", GetStatInt);
        SET_FUNCTION("getStatFloat", GetStatFloat);
        SET_FUNCTION("getGlobalStatInt", GetGlobalStatInt);
        SET_FUNCTION("getGlobalStatFloat", GetGlobalStatFloat);
        SET_FUNCTION("setStat", SetStat);
        SET_FUNCTION("storeStats", StoreStats);
        SET_FUNCTION("resetAllStats", ResetAllStats);

        utils::InitUgcMatchingTypes(exports);
        utils::InitUgcQueryTypes(exports);
        utils::InitUserUgcListSortOrder(exports);
        utils::InitUserUgcList(exports);

        // Utils related APIs.
        InitUtilsObject(exports);

        // Networking apis
        InitNetworkingObject(exports);
    }
} // namespace

#if defined(_WIN32)
#if defined(_M_IX86)
NODE_MODULE(greenworks_win32, init)
#elif defined(_M_AMD64)
NODE_MODULE(greenworks_win64, init)
#endif
#elif defined(__APPLE__)
#if defined(__x86_64__) || defined(__ppc64__)
NODE_MODULE(greenworks_osx64, init)
#else
NODE_MODULE(greenworks_osx32, init)
#endif
#elif defined(__linux__)
#if defined(__x86_64__) || defined(__ppc64__)
NODE_MODULE(greenworks_linux64, init)
#else
NODE_MODULE(greenworks_linux32, init)
#endif
#endif
