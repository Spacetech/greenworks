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
        account_type->Set(Nan::New("name").ToLocalChecked(), Nan::New(name).ToLocalChecked());
        account_type->Set(Nan::New("value").ToLocalChecked(), Nan::New(type_id));
        return account_type;
    }

    NAN_METHOD(InitAPI)
    {
        Nan::HandleScope scope;

        bool success = SteamAPI_Init();

        if (success)
        {
            ISteamUserStats* stream_user_stats = SteamUserStats();
            stream_user_stats->RequestCurrentStats();

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
        flags->Set(Nan::New("anonymous").ToLocalChecked(), Nan::New(user_id.BAnonAccount()));
        flags->Set(Nan::New("anonymousGameServer").ToLocalChecked(),
                   Nan::New(user_id.BAnonGameServerAccount()));
        flags->Set(Nan::New("anonymousGameServerLogin").ToLocalChecked(),
                   Nan::New(user_id.BBlankAnonAccount()));
        flags->Set(Nan::New("anonymousUser").ToLocalChecked(), Nan::New(user_id.BAnonUserAccount()));
        flags->Set(Nan::New("chat").ToLocalChecked(), Nan::New(user_id.BChatAccount()));
        flags->Set(Nan::New("clan").ToLocalChecked(), Nan::New(user_id.BClanAccount()));
        flags->Set(Nan::New("consoleUser").ToLocalChecked(), Nan::New(user_id.BConsoleUserAccount()));
        flags->Set(Nan::New("contentServer").ToLocalChecked(), Nan::New(user_id.BContentServerAccount()));
        flags->Set(Nan::New("gameServer").ToLocalChecked(), Nan::New(user_id.BGameServerAccount()));
        flags->Set(Nan::New("individual").ToLocalChecked(), Nan::New(user_id.BIndividualAccount()));
        flags->Set(Nan::New("gameServerPersistent").ToLocalChecked(),
                   Nan::New(user_id.BPersistentGameServerAccount()));
        flags->Set(Nan::New("lobby").ToLocalChecked(), Nan::New(user_id.IsLobby()));

        v8::Local<v8::Object> result = Nan::New<v8::Object>();
        result->Set(Nan::New("flags").ToLocalChecked(), flags);
        result->Set(Nan::New("type").ToLocalChecked(), GetSteamUserCountType(user_id.GetEAccountType()));
        result->Set(Nan::New("accountId").ToLocalChecked(), Nan::New<v8::Integer>(user_id.GetAccountID()));
        result->Set(Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(user_id.GetStaticAccountKey())).ToLocalChecked());
        result->Set(Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(user_id.ConvertToUint64())).ToLocalChecked());
        result->Set(Nan::New("isValid").ToLocalChecked(), Nan::New<v8::Integer>(user_id.IsValid()));
        result->Set(Nan::New("level").ToLocalChecked(), Nan::New<v8::Integer>(
                                                            SteamUser()->GetPlayerSteamLevel()));

        if (!SteamFriends()->RequestUserInformation(user_id, true))
        {
            result->Set(Nan::New("screenName").ToLocalChecked(),
                        Nan::New(SteamFriends()->GetFriendPersonaName(user_id)).ToLocalChecked());
        }
        else
        {
            std::ostringstream sout;
            sout << user_id.GetAccountID();
            result->Set(Nan::New("screenName").ToLocalChecked(), Nan::New<v8::String>(sout.str()).ToLocalChecked());
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

        std::string file_name(*(v8::String::Utf8Value(info[0])));
        std::string content(*(v8::String::Utf8Value(info[1])));
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
            if (!files->Get(i)->IsString())
                THROW_BAD_ARGS("Bad arguments");
            v8::String::Utf8Value string_array(files->Get(i));
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

        std::string file_name(*(v8::String::Utf8Value(info[0])));
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
        std::string achievement = (*(v8::String::Utf8Value(info[0])));
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

        std::string achievement = (*(v8::String::Utf8Value(info[0])));
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
        std::string achievement = (*(v8::String::Utf8Value(info[0])));
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
            names->Set(i, Nan::New(SteamUserStats()->GetAchievementName(i)).ToLocalChecked());
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
        std::string option(*(v8::String::Utf8Value(info[0])));
        SteamFriends()->ActivateGameOverlay(option.c_str());
        info.GetReturnValue().Set(Nan::Undefined());
    }

    NAN_METHOD(ActivateGameOverlayToWebPage)
    {
        Nan::HandleScope scope;
        if (info.Length() < 1 || !info[0]->IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string option(*(v8::String::Utf8Value(info[0])));
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

        std::string key(*(v8::String::Utf8Value(info[0])));
        std::string value(*(v8::String::Utf8Value(info[1])));

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
        std::string file_name(*(v8::String::Utf8Value(info[0])));
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
        std::string file_name(*(v8::String::Utf8Value(info[0])));
        std::string image_name(*(v8::String::Utf8Value(info[1])));
        std::string title(*(v8::String::Utf8Value(info[2])));
        std::string description(*(v8::String::Utf8Value(info[3])));

        v8::Local<v8::Array> tagsArray = info[4].As<v8::Array>();
        std::vector<std::string> tags;
        for (uint32_t i = 0; i < tagsArray->Length(); ++i)
        {
            if (!tagsArray->Get(i)->IsString())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            v8::String::Utf8Value string_array(tagsArray->Get(i));
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
            *(v8::String::Utf8Value(info[0])));
        std::string file_name(*(v8::String::Utf8Value(info[1])));
        std::string image_name(*(v8::String::Utf8Value(info[2])));
        std::string title(*(v8::String::Utf8Value(info[3])));
        std::string description(*(v8::String::Utf8Value(info[4])));

        v8::Local<v8::Array> tagsArray = info[5].As<v8::Array>();
        std::vector<std::string> tags;
        for (uint32_t i = 0; i < tagsArray->Length(); ++i)
        {
            if (!tagsArray->Get(i)->IsString())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            v8::String::Utf8Value string_array(tagsArray->Get(i));
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

        EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(
            info[0]->Int32Value());
        EUGCQuery ugc_query_type = static_cast<EUGCQuery>(info[1]->Int32Value());

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

        EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(
            info[0]->Int32Value());
        EUserUGCListSortOrder ugc_list_order = static_cast<EUserUGCListSortOrder>(
            info[1]->Int32Value());
        EUserUGCList ugc_list = static_cast<EUserUGCList>(info[2]->Int32Value());

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
            *(v8::String::Utf8Value(info[0])));
        std::string download_dir = *(v8::String::Utf8Value(info[1]));

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
        std::string download_dir = *(v8::String::Utf8Value(info[0]));

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
            std::string item_id = *(v8::String::Utf8Value(info[0]));
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
            *(v8::String::Utf8Value(info[0])));
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
            if (!array->Get(i)->IsNumber())
            {
                THROW_BAD_ARGS("Bad arguments");
            }

            published_file_id[i] = utils::strToUint64(*(v8::String::Utf8Value(array->Get(i))));
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
        std::string zip_file_path = *(v8::String::Utf8Value(info[0]));
        std::string source_dir = *(v8::String::Utf8Value(info[1]));
        std::string password = *(v8::String::Utf8Value(info[2]));
        int compress_level = info[3]->Int32Value();

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
        std::string zip_file_path = *(v8::String::Utf8Value(info[0]));
        std::string extract_dir = *(v8::String::Utf8Value(info[1]));
        std::string password = *(v8::String::Utf8Value(info[2]));

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

                friendObject->Set(Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendSteamId.GetStaticAccountKey())).ToLocalChecked());
                friendObject->Set(Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendSteamId.ConvertToUint64())).ToLocalChecked());

                const char* friendName = SteamFriends()->GetFriendPersonaName(friendSteamId);
                if (friendName != NULL)
                {
                    friendObject->Set(Nan::New("name").ToLocalChecked(), Nan::New(std::string(friendName)).ToLocalChecked());
                }

                FriendGameInfo_t friendGameInfo;
                if (SteamFriends()->GetFriendGamePlayed(friendSteamId, &friendGameInfo))
                {
                    friendObject->Set(Nan::New("gameId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendGameInfo.m_gameID.ToUint64())).ToLocalChecked());
                    friendObject->Set(Nan::New("lobbyId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(friendGameInfo.m_steamIDLobby.ConvertToUint64())).ToLocalChecked());
                }

                friends->Set(i, friendObject);
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

        int lobbyType = info[0]->Int32Value();
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

        std::string steamIdString(*(v8::String::Utf8Value(info[0])));
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

        std::string steamIdString(*(v8::String::Utf8Value(info[0])));
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

        std::string lobbyIdString(*(v8::String::Utf8Value(info[0])));
        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        int lobbyType = info[1]->Int32Value();
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

        std::string lobbyIdString(*(v8::String::Utf8Value(info[0])));
        std::string key(*(v8::String::Utf8Value(info[1])));

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

        std::string lobbyIdString(*(v8::String::Utf8Value(info[0])));
        std::string key(*(v8::String::Utf8Value(info[1])));
        std::string value(*(v8::String::Utf8Value(info[2])));

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

        std::string lobbyIdString(*(v8::String::Utf8Value(info[0]->ToString())));

        CSteamID lobbyId(utils::strToUint64(lobbyIdString));

        int lobbyMembersCount = SteamMatchmaking()->GetNumLobbyMembers(lobbyId);

        v8::Local<v8::Array> lobbyMembers = Nan::New<v8::Array>(static_cast<int>(lobbyMembersCount));

        for (int i = 0; i < lobbyMembersCount; i++)
        {
            CSteamID lobbyMemberSteamId = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyId, i);
            if (lobbyMemberSteamId.IsValid())
            {
                v8::Local<v8::Object> lobbyMemberObject = Nan::New<v8::Object>();

                lobbyMemberObject->Set(Nan::New("staticAccountId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(lobbyMemberSteamId.GetStaticAccountKey())).ToLocalChecked());
                lobbyMemberObject->Set(Nan::New("steamId").ToLocalChecked(), Nan::New<v8::String>(utils::uint64ToString(lobbyMemberSteamId.ConvertToUint64())).ToLocalChecked());

                const char* lobbyMemberName = SteamFriends()->GetFriendPersonaName(lobbyMemberSteamId);
                if (lobbyMemberName != NULL)
                {
                    lobbyMemberObject->Set(Nan::New("name").ToLocalChecked(), Nan::New(std::string(lobbyMemberName)).ToLocalChecked());
                }

                lobbyMembers->Set(i, lobbyMemberObject);
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

        std::string name = (*(v8::String::Utf8Value(info[0])));
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

        std::string name = (*(v8::String::Utf8Value(info[0])));
        float result = 0;
        if (SteamUserStats()->GetStat(name.c_str(), &result))
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

        std::string name = *(v8::String::Utf8Value(info[0]));
        if (info[1]->IsInt32())
        {
            int32 value = info[1].As<v8::Number>()->Int32Value();
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

    void InitUtilsObject(v8::Handle<v8::Object> exports)
    {
        // Prepare constructor template
        v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>();

        Nan::SetMethod(tpl, "createArchive", CreateArchive);
        Nan::SetMethod(tpl, "extractArchive", ExtractArchive);

        Nan::Persistent<v8::Function> constructor;
        constructor.Reset(tpl->GetFunction());

        exports->Set(Nan::New("Utils").ToLocalChecked(), tpl->GetFunction());
    }

    void init(v8::Handle<v8::Object> exports)
    {
        // Common APIs.
        exports->Set(Nan::New("initAPI").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(InitAPI)->GetFunction());
        exports->Set(Nan::New("getSteamId").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetSteamId)->GetFunction());
        exports->Set(Nan::New("runCallbacks").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(RunCallbacks)->GetFunction());

        // File APIs.
        exports->Set(Nan::New("saveTextToFile").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SaveTextToFile)->GetFunction());
        exports->Set(Nan::New("readTextFromFile").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ReadTextFromFile)->GetFunction());
        exports->Set(Nan::New("saveFilesToCloud").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SaveFilesToCloud)->GetFunction());

        // Cloud APIs.
        exports->Set(Nan::New("isCloudEnabled").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(IsCloudEnabled)->GetFunction());
        exports->Set(Nan::New("isCloudEnabledForUser").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(IsCloudEnabledForUser)->GetFunction());
        exports->Set(Nan::New("enableCloud").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(EnableCloud)->GetFunction());
        exports->Set(Nan::New("getCloudQuota").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetCloudQuota)->GetFunction());

        // Achievement APIs.
        exports->Set(Nan::New("activateAchievement").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ActivateAchievement)->GetFunction());
        exports->Set(Nan::New("clearAchievement").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ClearAchievement)->GetFunction());
        exports->Set(Nan::New("getAchievement").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetAchievement)->GetFunction());
        exports->Set(Nan::New("getAchievementNames").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetAchievementNames)->GetFunction());
        exports->Set(Nan::New("getNumberOfAchievements").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetNumberOfAchievements)->GetFunction());

        // Game APIs.
        exports->Set(Nan::New("getCurrentBetaName").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetCurrentBetaName)->GetFunction());
        exports->Set(Nan::New("getCurrentGameLanguage").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetCurrentGameLanguage)->GetFunction());
        exports->Set(Nan::New("getCurrentUILanguage").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetCurrentUILanguage)->GetFunction());
        exports->Set(Nan::New("getCurrentGameInstallDir").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetCurrentGameInstallDir)->GetFunction());
        exports->Set(Nan::New("getNumberOfPlayers").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetNumberOfPlayers)->GetFunction());
        exports->Set(Nan::New("isGameOverlayEnabled").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(IsGameOverlayEnabled)->GetFunction());
        exports->Set(Nan::New("activateGameOverlay").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ActivateGameOverlay)->GetFunction());
        exports->Set(Nan::New("activateGameOverlayToWebPage").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ActivateGameOverlayToWebPage)->GetFunction());
        exports->Set(Nan::New("onGameOverlayActive").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnGameOverlayActive)->GetFunction());
        exports->Set(Nan::New("onGameJoinRequested").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnGameJoinRequested)->GetFunction());

        // Workshop APIs
        exports->Set(Nan::New("fileShare").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(FileShare)->GetFunction());
        exports->Set(Nan::New("publishWorkshopFile").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(PublishWorkshopFile)->GetFunction());
        exports->Set(Nan::New("updatePublishedWorkshopFile").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UpdatePublishedWorkshopFile)->GetFunction());
        exports->Set(Nan::New("ugcGetItems").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCGetItems)->GetFunction());
        exports->Set(Nan::New("ugcGetUserItems").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCGetUserItems)->GetFunction());
        exports->Set(Nan::New("ugcDownloadItem").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCDownloadItem)->GetFunction());
        exports->Set(Nan::New("ugcSynchronizeItems").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCSynchronizeItems)->GetFunction());
        exports->Set(Nan::New("ugcShowOverlay").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCShowOverlay)->GetFunction());
        exports->Set(Nan::New("ugcUnsubscribe").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCUnsubscribe)->GetFunction());

        exports->Set(Nan::New("startPlaytimeTracking").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCStartPlaytimeTracking)->GetFunction());
        exports->Set(Nan::New("stopPlaytimeTracking").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(UGCStopPlaytimeTracking)->GetFunction());

        // Presence apis
        exports->Set(Nan::New("setRichPresence").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SetRichPresence)->GetFunction());
        exports->Set(Nan::New("clearRichPresence").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ClearRichPresence)->GetFunction());

        // Friend apis
        exports->Set(Nan::New("getFriends").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetFriends)->GetFunction());

        // Stat apis
        exports->Set(Nan::New("getStatInt").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetStatInt)->GetFunction());
        exports->Set(Nan::New("getStatFloat").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetStatFloat)->GetFunction());
        exports->Set(Nan::New("setStat").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SetStat)->GetFunction());
        exports->Set(Nan::New("storeStats").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(StoreStats)->GetFunction());
        exports->Set(Nan::New("resetAllStats").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(ResetAllStats)->GetFunction());

        // Lobby api
        exports->Set(Nan::New("createLobby").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CreateLobby)->GetFunction());
        exports->Set(Nan::New("leaveLobby").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(LeaveLobby)->GetFunction());
        exports->Set(Nan::New("joinLobby").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(JoinLobby)->GetFunction());
        exports->Set(Nan::New("setLobbyType").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SetLobbyType)->GetFunction());
        exports->Set(Nan::New("getLobbyData").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetLobbyData)->GetFunction());
        exports->Set(Nan::New("setLobbyData").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SetLobbyData)->GetFunction());
        exports->Set(Nan::New("getLobbyMembers").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(GetLobbyMembers)->GetFunction());
        exports->Set(Nan::New("onLobbyCreated").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnLobbyCreated)->GetFunction());
        exports->Set(Nan::New("onLobbyEntered").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnLobbyEntered)->GetFunction());
        exports->Set(Nan::New("onLobbyChatUpdate").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnLobbyChatUpdate)->GetFunction());
        exports->Set(Nan::New("onLobbyJoinRequested").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnLobbyJoinRequested)->GetFunction());

        utils::InitUgcMatchingTypes(exports);
        utils::InitUgcQueryTypes(exports);
        utils::InitUserUgcListSortOrder(exports);
        utils::InitUserUgcList(exports);

        // Utils related APIs.
        InitUtilsObject(exports);
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
