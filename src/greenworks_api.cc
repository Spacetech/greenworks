// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <sstream>
#include <string>
#include <vector>

#include "napi.h"
#include "steam/steam_api.h"
#include "uv.h"
#include "v8.h"

#include "greenworks_async_workers.h"
#include "greenworks_utils.h"
#include "greenworks_workshop_workers.h"
#include "steam_callbacks.h"

#define THROW_BAD_ARGS(msg) Napi::Error::New(env, msg).ThrowAsJavaScriptException()

#define SET_FUNCTION(function_name, function) exports.Set(function_name, Napi::Function::New(env, function))

#define SET_FUNCTION_TPL(function_name, function) tpl.Set(function_name, Napi::Function::New(env, function))

#define MESSAGE_CHANNEL 0

SteamCallbacks *steamCallbacks = nullptr;

Napi::Object GetSteamUserCountType(Napi::Env env, int type_id)
{
    Napi::Object account_type = Napi::Object::New(env);
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
    (account_type).Set("name", Napi::String::New(env, name));
    (account_type).Set("value", Napi::Number::New(env, type_id));
    return account_type;
}

Napi::Value InitAPI(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    bool success = SteamAPI_Init();

    if (success)
    {
        ISteamUserStats *steam_user_stats = SteamUserStats();
        steam_user_stats->RequestCurrentStats();
        steam_user_stats->RequestGlobalStats(1);

        if (steamCallbacks == nullptr)
        {
            // we should probably free this at somepoint
            steamCallbacks = new SteamCallbacks();
        }
    }

    return Napi::Boolean::New(env, success);
}

Napi::Value GetSteamId(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    CSteamID user_id = SteamUser()->GetSteamID();
    Napi::Object flags = Napi::Object::New(env);
    (flags).Set("anonymous", Napi::Boolean::New(env, user_id.BAnonAccount()));
    (flags).Set("anonymousGameServer", Napi::Boolean::New(env, user_id.BAnonGameServerAccount()));
    (flags).Set("anonymousGameServerLogin", Napi::Boolean::New(env, user_id.BBlankAnonAccount()));
    (flags).Set("anonymousUser", Napi::Boolean::New(env, user_id.BAnonUserAccount()));
    (flags).Set("chat", Napi::Boolean::New(env, user_id.BChatAccount()));
    (flags).Set("clan", Napi::Boolean::New(env, user_id.BClanAccount()));
    (flags).Set("consoleUser", Napi::Boolean::New(env, user_id.BConsoleUserAccount()));
    (flags).Set("contentServer", Napi::Boolean::New(env, user_id.BContentServerAccount()));
    (flags).Set("gameServer", Napi::Boolean::New(env, user_id.BGameServerAccount()));
    (flags).Set("individual", Napi::Boolean::New(env, user_id.BIndividualAccount()));
    (flags).Set("gameServerPersistent", Napi::Boolean::New(env, user_id.BPersistentGameServerAccount()));
    (flags).Set("lobby", Napi::Boolean::New(env, user_id.IsLobby()));

    Napi::Object result = Napi::Object::New(env);
    result.Set("flags", flags);
    result.Set("type", GetSteamUserCountType(env, user_id.GetEAccountType()));
    result.Set("accountId", Napi::Number::New(env, user_id.GetAccountID()));
    result.Set("staticAccountId", Napi::String::New(env, utils::uint64ToString(user_id.GetStaticAccountKey())));
    result.Set("steamId", Napi::String::New(env, utils::uint64ToString(user_id.ConvertToUint64())));
    result.Set("isValid", Napi::Number::New(env, user_id.IsValid()));
    result.Set("level", Napi::Number::New(env, SteamUser()->GetPlayerSteamLevel()));

    if (!SteamFriends()->RequestUserInformation(user_id, true))
    {
        result.Set("screenName", Napi::String::New(env, SteamFriends()->GetFriendPersonaName(user_id)));
    }
    else
    {
        std::ostringstream sout;
        sout << user_id.GetAccountID();
        result.Set("screenName", Napi::String::New(env, sout.str()));
    }

    return result;
}

Napi::Value RunCallbacks(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    SteamAPI_RunCallbacks();

    return env.Undefined();
}

Napi::Value SaveFilesToCloud(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsArray() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    Napi::Array files = info[0].As<Napi::Array>();
    std::vector<std::string> files_path;
    for (uint32_t i = 0; i < files.Length(); ++i)
    {
        if (!(files).Get(i).IsString())
            THROW_BAD_ARGS("Bad arguments");
        std::string string_array = (files).Get(i).ToString().Utf8Value();
        // Ignore empty path.
        if (string_array.length() > 0)
            files_path.push_back(string_array);
    }

    Napi::Function callback = info[1].As<Napi::Function>();

    (new FilesSaveWorker(callback, files_path))->Queue();
    return env.Undefined();
}

Napi::Value GetFileCount(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Number::New(env, SteamRemoteStorage()->GetFileCount());
}

Napi::Value GetFileNameAndSize(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    int iFile = info[0].As<Napi::Number>().Int32Value();

    int32 fileSize;

    const char *fileName = SteamRemoteStorage()->GetFileNameAndSize(iFile, &fileSize);

    Napi::Object fileObject = Napi::Object::New(env);

    (fileObject).Set("name", Napi::String::New(env, fileName));
    (fileObject).Set("size", Napi::Number::New(env, fileSize));

    return fileObject;
}

Napi::Value DeleteRemoteFile(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string file_name = info[0].ToString().Utf8Value();

    bool result = SteamRemoteStorage()->FileDelete(file_name.c_str());

    return Napi::Boolean::New(env, result);
}

Napi::Value IsCloudEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ISteamRemoteStorage *steam_remote_storage = SteamRemoteStorage();
    return Napi::Boolean::New(env, steam_remote_storage->IsCloudEnabledForApp());
}

Napi::Value IsCloudEnabledForUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ISteamRemoteStorage *steam_remote_storage = SteamRemoteStorage();
    return Napi::Boolean::New(env, steam_remote_storage->IsCloudEnabledForAccount());
}

Napi::Value EnableCloud(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1)
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    bool enable_flag = info[0].ToBoolean();
    SteamRemoteStorage()->SetCloudEnabledForApp(enable_flag);
    return env.Undefined();
}

Napi::Value GetCloudQuota(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    Napi::Function callback = info[0].As<Napi::Function>();

    (new CloudQuotaGetWorker(callback))->Queue();

    return env.Undefined();
}

Napi::Value ActivateAchievement(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    std::string achievement = info[0].ToString().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    (new ActivateAchievementWorker(callback, achievement))->Queue();

    return env.Undefined();
}

Napi::Value GetAchievement(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string achievement = info[0].ToString().Utf8Value();
    ;
    Napi::Function callback = info[1].As<Napi::Function>();

    (new GetAchievementWorker(callback, achievement))->Queue();
    return env.Undefined();
}

Napi::Value ClearAchievement(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    std::string achievement = info[0].ToString().Utf8Value();
    ;
    Napi::Function callback = info[1].As<Napi::Function>();

    (new ClearAchievementWorker(callback, achievement))->Queue();
    return env.Undefined();
}

Napi::Value GetAchievementNames(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    int count = static_cast<int>(SteamUserStats()->GetNumAchievements());
    Napi::Array names = Napi::Array::New(env, count);
    for (int i = 0; i < count; ++i)
    {
        (names).Set(i, Napi::String::New(env, SteamUserStats()->GetAchievementName(i)));
    }
    return names;
}

Napi::Value GetNumberOfAchievements(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    ISteamUserStats *steam_user_stats = SteamUserStats();
    return Napi::Number::New(env, steam_user_stats->GetNumAchievements());
}

Napi::Value GetCurrentBetaName(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    std::string betaNameStr = "public";

    char *betaName = new char[1024];
    if (SteamApps()->GetCurrentBetaName(betaName, 1024))
    {
        betaNameStr = std::string(betaName);
    }

    delete[] betaName;

    return Napi::String::New(env, betaNameStr);
}

Napi::Value GetCurrentGameLanguage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, SteamApps()->GetCurrentGameLanguage());
}

Napi::Value GetCurrentUILanguage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, SteamUtils()->GetSteamUILanguage());
}

Napi::Value GetCurrentGameInstallDir(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    uint32 appId = SteamUtils()->GetAppID();

    char *installDir = new char[1024];
    uint32 endPos = SteamApps()->GetAppInstallDir(appId, installDir, 1024);
    installDir[endPos] = '\0';

    std::string installDirRet = std::string(installDir);

    delete[] installDir;

    return Napi::String::New(env, installDirRet);
}

Napi::Value GetNumberOfPlayers(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    Napi::Function callback = info[0].As<Napi::Function>();

    (new GetNumberOfPlayersWorker(callback))->Queue();
    return env.Undefined();
}

Napi::Value IsGameOverlayEnabled(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, SteamUtils()->IsOverlayEnabled());
}

Napi::Value ActivateGameOverlay(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string option = info[0].ToString().Utf8Value();

    SteamFriends()->ActivateGameOverlay(option.c_str());

    return env.Undefined();
}

Napi::Value ActivateGameOverlayInviteDialog(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID lobbyId(utils::strToUint64(steamIdString));

    SteamFriends()->ActivateGameOverlayInviteDialog(lobbyId);
    return env.Undefined();
}

Napi::Value ActivateGameOverlayToWebPage(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string option = info[0].ToString().Utf8Value();
    SteamFriends()->ActivateGameOverlayToWebPage(option.c_str());
    return env.Undefined();
}

Napi::Value OnGameOverlayActive(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnGameOverlayActivatedCallback.IsEmpty())
    {
        steamCallbacks->OnGameOverlayActivatedCallback.Reset();
    }

    steamCallbacks->OnGameOverlayActivatedCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value OnGameJoinRequested(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnGameJoinRequestedCallback.IsEmpty())
    {
        steamCallbacks->OnGameJoinRequestedCallback.Reset();
    }

    steamCallbacks->OnGameJoinRequestedCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value SetRichPresence(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string key = info[0].ToString().Utf8Value();
    std::string value = info[1].ToString().Utf8Value();

    if (key.length() > k_cchMaxRichPresenceKeyLength)
    {
        THROW_BAD_ARGS("Key too long");
    }

    if (value.length() > k_cchMaxRichPresenceValueLength)
    {
        THROW_BAD_ARGS("Value too long");
    }

    bool success = SteamFriends()->SetRichPresence(key.c_str(), value.c_str());

    return Napi::Boolean::New(env, success);
}

Napi::Value ClearRichPresence(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    SteamFriends()->ClearRichPresence();

    return env.Undefined();
}

Napi::Value FileShare(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    std::string file_name = info[0].ToString().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    (new FileShareWorker(callback, file_name))->Queue();
    return env.Undefined();
}

Napi::Value PublishWorkshopFile(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 6 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString() ||
        !info[4].IsArray() || !info[5].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    std::string file_name = info[0].ToString().Utf8Value();
    std::string image_name = info[1].ToString().Utf8Value();
    std::string title = info[2].ToString().Utf8Value();
    std::string description = info[3].ToString().Utf8Value();

    Napi::Array tagsArray = info[4].As<Napi::Array>();
    std::vector<std::string> tags;
    for (uint32_t i = 0; i < tagsArray.Length(); ++i)
    {
        if (!(tagsArray).Get(i).IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string string_array = (tagsArray).Get(i).ToString().Utf8Value();
        if (string_array.length() > 0)
        {
            tags.push_back(string_array);
        }
    }

    Napi::Function callback = info[5].As<Napi::Function>();

    (new PublishWorkshopFileWorker(callback, file_name, image_name, title, description, tags))->Queue();

    return env.Undefined();
}

Napi::Value UpdatePublishedWorkshopFile(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 7 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString() ||
        !info[4].IsString() || !info[5].IsArray() || !info[6].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    PublishedFileId_t published_file_id = utils::strToUint64(info[0].ToString().Utf8Value());
    std::string file_name = info[1].ToString().Utf8Value();
    std::string image_name = info[2].ToString().Utf8Value();
    std::string title = info[3].ToString().Utf8Value();
    std::string description = info[4].ToString().Utf8Value();

    Napi::Array tagsArray = info[5].As<Napi::Array>();
    std::vector<std::string> tags;
    for (uint32_t i = 0; i < tagsArray.Length(); ++i)
    {
        if (!(tagsArray).Get(i).IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        std::string string_array = (tagsArray).Get(i).ToString().Utf8Value();
        if (string_array.length() > 0)
        {
            tags.push_back(string_array);
        }
    }

    Napi::Function callback = info[6].As<Napi::Function>();

    (new UpdatePublishedWorkshopFileWorker(callback, published_file_id, file_name, image_name, title, description,
                                           tags))
        ->Queue();

    return env.Undefined();
}

Napi::Value UGCGetItems(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(info[0].As<Napi::Number>().Int32Value());
    EUGCQuery ugc_query_type = static_cast<EUGCQuery>(info[1].As<Napi::Number>().Int32Value());

    Napi::Function callback = info[2].As<Napi::Function>();

    (new QueryAllUGCWorker(callback, ugc_matching_type, ugc_query_type))->Queue();
    return env.Undefined();
}

Napi::Value UGCGetUserItems(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 4 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    EUGCMatchingUGCType ugc_matching_type = static_cast<EUGCMatchingUGCType>(info[0].As<Napi::Number>().Int32Value());
    EUserUGCListSortOrder ugc_list_order = static_cast<EUserUGCListSortOrder>(info[1].As<Napi::Number>().Int32Value());
    EUserUGCList ugc_list = static_cast<EUserUGCList>(info[2].As<Napi::Number>().Int32Value());

    Napi::Function callback = info[3].As<Napi::Function>();

    (new QueryUserUGCWorker(callback, ugc_matching_type, ugc_list, ugc_list_order))->Queue();

    return env.Undefined();
}

Napi::Value UGCDownloadItem(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    UGCHandle_t download_file_handle = utils::strToUint64(info[0].ToString().Utf8Value());
    std::string download_dir = info[1].ToString().Utf8Value();

    Napi::Function callback = info[2].As<Napi::Function>();

    (new DownloadItemWorker(callback, download_file_handle, download_dir))->Queue();

    return env.Undefined();
}

Napi::Value UGCSynchronizeItems(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string download_dir = info[0].ToString().Utf8Value();

    Napi::Function callback = info[1].As<Napi::Function>();

    (new SynchronizeItemsWorker(callback, download_dir))->Queue();
    return env.Undefined();
}

Napi::Value UGCShowOverlay(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    std::string steam_store_url;
    if (info.Length() < 1)
    {
        uint32 appId = SteamUtils()->GetAppID();
        steam_store_url = "http://steamcommunity.com/app/" + utils::uint64ToString(appId) + "/workshop/";
    }
    else
    {
        if (!info[0].IsString())
        {
            THROW_BAD_ARGS("Bad arguments");
        }
        std::string item_id = info[0].ToString().Utf8Value();
        steam_store_url = "http://steamcommunity.com/sharedfiles/filedetails/?id=" + item_id;
    }

    SteamFriends()->ActivateGameOverlayToWebPage(steam_store_url.c_str());
    return env.Undefined();
}

Napi::Value UGCUnsubscribe(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }
    PublishedFileId_t unsubscribed_file_id = utils::strToUint64(info[0].ToString().Utf8Value());
    Napi::Function callback = info[1].As<Napi::Function>();

    (new UnsubscribePublishedFileWorker(callback, unsubscribed_file_id))->Queue();
    return env.Undefined();
}

Napi::Value UGCStartPlaytimeTracking(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsArray())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    PublishedFileId_t published_file_id[100];

    Napi::Array array = info[0].As<Napi::Array>();

    auto length = array.Length();

    if (length >= 100)
    {
        THROW_BAD_ARGS("Too many items");
    }

    for (uint32_t i = 0; i < length; ++i)
    {
        if (!(array).Get(i).IsNumber())
        {
            THROW_BAD_ARGS("Bad arguments");
        }

        published_file_id[i] = utils::strToUint64(array.Get(i).ToString().Utf8Value());
    }

    SteamUGC()->StartPlaytimeTracking(published_file_id, length);

    return env.Undefined();
}

Napi::Value UGCStopPlaytimeTracking(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    SteamUGC()->StopPlaytimeTrackingForAllItems();

    return env.Undefined();
}

Napi::Value CreateArchive(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 5 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsNumber() ||
        !info[4].IsFunction())
    {
        THROW_BAD_ARGS("bad arguments");
    }
    std::string zip_file_path = info[0].ToString().Utf8Value();
    std::string source_dir = info[1].ToString().Utf8Value();
    std::string password = info[2].ToString().Utf8Value();
    int compress_level = info[3].As<Napi::Number>().Int32Value();

    Napi::Function callback = info[4].As<Napi::Function>();

    (new CreateArchiveWorker(callback, zip_file_path, source_dir, password, compress_level))->Queue();
    return env.Undefined();
}

Napi::Value ExtractArchive(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 4 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsFunction())
    {
        THROW_BAD_ARGS("bad arguments");
    }
    std::string zip_file_path = info[0].ToString().Utf8Value();
    std::string extract_dir = info[1].ToString().Utf8Value();
    std::string password = info[2].ToString().Utf8Value();

    Napi::Function callback = info[3].As<Napi::Function>();

    (new ExtractArchiveWorker(callback, zip_file_path, extract_dir, password))->Queue();

    return env.Undefined();
}

Napi::Value GetFriends(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    EFriendFlags friendFlags = EFriendFlags::k_EFriendFlagImmediate;

    int friendCount = SteamFriends()->GetFriendCount(friendFlags);

    Napi::Array friends = Napi::Array::New(env, static_cast<int>(friendCount));

    for (int i = 0; i < friendCount; i++)
    {
        CSteamID friendSteamId = SteamFriends()->GetFriendByIndex(i, friendFlags);
        if (friendSteamId.IsValid())
        {
            Napi::Object friendObject = Napi::Object::New(env);

            (friendObject)
                .Set("staticAccountId",
                     Napi::String::New(env, utils::uint64ToString(friendSteamId.GetStaticAccountKey())));
            (friendObject)
                .Set("steamId", Napi::String::New(env, utils::uint64ToString(friendSteamId.ConvertToUint64())));

            const char *friendName = SteamFriends()->GetFriendPersonaName(friendSteamId);
            if (friendName != NULL)
            {
                (friendObject).Set("name", Napi::String::New(env, friendName));
            }

            FriendGameInfo_t friendGameInfo;
            if (SteamFriends()->GetFriendGamePlayed(friendSteamId, &friendGameInfo))
            {
                (friendObject)
                    .Set("gameId", Napi::String::New(env, utils::uint64ToString(friendGameInfo.m_gameID.ToUint64())));
                (friendObject)
                    .Set("lobbyId", Napi::String::New(
                                        env, utils::uint64ToString(friendGameInfo.m_steamIDLobby.ConvertToUint64())));
            }

            (friends).Set(i, friendObject);
        }
    }

    return friends;
}

Napi::Value CreateLobby(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    int lobbyType = info[0].As<Napi::Number>().Int32Value();
    if (lobbyType < 0 || lobbyType > 3)
    {
        THROW_BAD_ARGS("bad arguments");
    }

    SteamMatchmaking()->CreateLobby((ELobbyType)lobbyType, 16);

    return env.Undefined();
}

Napi::Value LeaveLobby(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID lobbyId(utils::strToUint64(steamIdString));

    SteamMatchmaking()->LeaveLobby(lobbyId);

    return env.Undefined();
}

Napi::Value JoinLobby(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID lobbyId(utils::strToUint64(steamIdString));

    SteamMatchmaking()->JoinLobby(lobbyId);

    return env.Undefined();
}

Napi::Value SetLobbyType(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string lobbyIdString = info[0].ToString().Utf8Value();
    CSteamID lobbyId(utils::strToUint64(lobbyIdString));

    int lobbyType = info[1].As<Napi::Number>().Int32Value();
    if (lobbyType < 0 || lobbyType > 3)
    {
        THROW_BAD_ARGS("bad arguments");
    }

    bool success = SteamMatchmaking()->SetLobbyType(lobbyId, (ELobbyType)lobbyType);

    return Napi::Boolean::New(env, success);
}

Napi::Value GetLobbyData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string lobbyIdString = info[0].ToString().Utf8Value();
    std::string key = info[1].ToString().Utf8Value();

    CSteamID lobbyId(utils::strToUint64(lobbyIdString));

    const char *data = SteamMatchmaking()->GetLobbyData(lobbyId, key.c_str());

    return Napi::String::New(env, data);
}

Napi::Value SetLobbyData(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string lobbyIdString = info[0].ToString().Utf8Value();
    std::string key = info[1].ToString().Utf8Value();
    std::string value = info[2].ToString().Utf8Value();

    CSteamID lobbyId(utils::strToUint64(lobbyIdString));

    bool success = SteamMatchmaking()->SetLobbyData(lobbyId, key.c_str(), value.c_str());

    return Napi::Boolean::New(env, success);
}

Napi::Value GetLobbyOwner(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string lobbyIdString = info[0].ToString().Utf8Value();

    CSteamID lobbyId(utils::strToUint64(lobbyIdString));

    CSteamID lobbyOwner = SteamMatchmaking()->GetLobbyOwner(lobbyId);

    return Napi::String::New(env, utils::uint64ToString(lobbyOwner.ConvertToUint64()));
}

Napi::Value GetLobbyMembers(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("bad arguments");
    }

    std::string lobbyIdString = info[0].ToString().Utf8Value();

    CSteamID lobbyId(utils::strToUint64(lobbyIdString));

    int lobbyMembersCount = SteamMatchmaking()->GetNumLobbyMembers(lobbyId);

    Napi::Array lobbyMembers = Napi::Array::New(env, static_cast<int>(lobbyMembersCount));

    for (int i = 0; i < lobbyMembersCount; i++)
    {
        CSteamID lobbyMemberSteamId = SteamMatchmaking()->GetLobbyMemberByIndex(lobbyId, i);
        if (lobbyMemberSteamId.IsValid())
        {
            Napi::Object lobbyMemberObject = Napi::Object::New(env);

            (lobbyMemberObject)
                .Set("staticAccountId",
                     Napi::String::New(env, utils::uint64ToString(lobbyMemberSteamId.GetStaticAccountKey())));
            (lobbyMemberObject)
                .Set("steamId", Napi::String::New(env, utils::uint64ToString(lobbyMemberSteamId.ConvertToUint64())));

            const char *lobbyMemberName = SteamFriends()->GetFriendPersonaName(lobbyMemberSteamId);
            if (lobbyMemberName != NULL)
            {
                (lobbyMemberObject).Set("name", Napi::String::New(env, lobbyMemberName));
            }

            (lobbyMembers).Set(i, lobbyMemberObject);
        }
    }

    return lobbyMembers;
}

Napi::Value OnLobbyCreated(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnLobbyCreatedCallback.IsEmpty())
    {
        steamCallbacks->OnLobbyCreatedCallback.Reset();
    }

    steamCallbacks->OnLobbyCreatedCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value OnLobbyEntered(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnLobbyEnteredCallback.IsEmpty())
    {
        steamCallbacks->OnLobbyEnteredCallback.Reset();
    }

    steamCallbacks->OnLobbyEnteredCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value OnLobbyChatUpdate(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnLobbyChatUpdateCallback.IsEmpty())
    {
        steamCallbacks->OnLobbyChatUpdateCallback.Reset();
    }

    steamCallbacks->OnLobbyChatUpdateCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value OnLobbyJoinRequested(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnLobbyJoinRequestedCallback.IsEmpty())
    {
        steamCallbacks->OnLobbyJoinRequestedCallback.Reset();
    }

    steamCallbacks->OnLobbyJoinRequestedCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value GetStatInt(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string name = info[0].ToString().Utf8Value();
    ;
    int32 result = 0;
    if (SteamUserStats()->GetStat(name.c_str(), &result))
    {
        return Napi::Number::New(env, result);
    }

    return env.Undefined();
}

Napi::Value GetStatFloat(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string name = info[0].ToString().Utf8Value();
    ;
    float result = 0;
    if (SteamUserStats()->GetStat(name.c_str(), &result))
    {
        return Napi::Number::New(env, result);
    }

    return env.Undefined();
}

Napi::Value GetGlobalStatInt(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string name = info[0].ToString().Utf8Value();
    ;
    int64 result = 0;
    if (SteamUserStats()->GetGlobalStat(name.c_str(), &result))
    {
        return Napi::Number::New(env, (int32)result);
    }

    return env.Undefined();
}

Napi::Value GetGlobalStatFloat(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string name = info[0].ToString().Utf8Value();
    ;
    double result = 0;
    if (SteamUserStats()->GetGlobalStat(name.c_str(), &result))
    {
        return Napi::Number::New(env, result);
    }

    return env.Undefined();
}

Napi::Value SetStat(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || (!info[1].IsNumber()))
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string name = info[0].ToString().Utf8Value();
    if (info[1].IsNumber())
    {
        int32 value = info[1].As<Napi::Number>().Int32Value();
        return Napi::Boolean::New(env, SteamUserStats()->SetStat(name.c_str(), value));
    }
    else
    {
        double value = info[1].ToNumber().DoubleValue();
        return Napi::Boolean::New(env, SteamUserStats()->SetStat(name.c_str(), static_cast<float>(value)));
    }
}

Napi::Value StoreStats(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || (!info[0].IsFunction()))
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    Napi::Function callback = info[0].As<Napi::Function>();

    (new StoreUserStatsWorker(callback))->Queue();

    return env.Undefined();
}

Napi::Value ResetAllStats(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || (!info[0].IsBoolean()))
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    bool reset_achievement = info[0].ToBoolean();
    return Napi::Boolean::New(env, SteamUserStats()->ResetAllStats(reset_achievement));
}

Napi::Value InitRelayNetworkAccess(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    SteamNetworkingUtils()->InitRelayNetworkAccess();

    return env.Undefined();
}

Napi::Value GetRelayNetworkStatus(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    SteamRelayNetworkStatus_t status;
    SteamNetworkingUtils()->GetRelayNetworkStatus(&status);

    Napi::Object result = Napi::Object::New(env);

    result.Set("availabilitySummary", Napi::Number::New(env, status.m_eAvail));
    result.Set("availabilityNetworkConfig", Napi::Number::New(env, status.m_eAvailNetworkConfig));
    result.Set("availabilityAnyRelay", Napi::Number::New(env, status.m_eAvailAnyRelay));
    result.Set("debugMessage", Napi::String::New(env, status.m_debugMsg));

    return result;
}

Napi::Value AcceptSessionWithUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();

    SteamNetworkingIdentity steamNetworkingIdentity;
    steamNetworkingIdentity.SetSteamID64(utils::strToUint64(steamIdString));

    bool result = SteamNetworkingMessages()->AcceptSessionWithUser(steamNetworkingIdentity);

    return Napi::Boolean::New(env, result);
}

Napi::Value SendMessageToUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsTypedArray())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();

    Napi::Uint8Array array = info[1].As<Napi::TypedArray>().As<Napi::Uint8Array>();
    uint8_t *dst = array.Data();
    uint32 length = sizeof(uint8_t) * array.ByteLength();

    SteamNetworkingIdentity steamNetworkingIdentity;
    steamNetworkingIdentity.SetSteamID64(utils::strToUint64(steamIdString));

    EResult result = SteamNetworkingMessages()->SendMessageToUser(
        steamNetworkingIdentity, dst, length,
        k_nSteamNetworkingSend_ReliableNoNagle | k_nSteamNetworkingSend_AutoRestartBrokenSession, MESSAGE_CHANNEL);

    return Napi::Number::New(env, result);
}

Napi::Value CloseSessionWithUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();

    SteamNetworkingIdentity steamNetworkingIdentity;
    steamNetworkingIdentity.SetSteamID64(utils::strToUint64(steamIdString));

    bool result = SteamNetworkingMessages()->CloseSessionWithUser(steamNetworkingIdentity);

    return Napi::Boolean::New(env, result);
}

Napi::Value GetSessionConnectionInfo(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();

    SteamNetworkingIdentity steamNetworkingIdentity;
    steamNetworkingIdentity.SetSteamID64(utils::strToUint64(steamIdString));

    SteamNetConnectionInfo_t connectionInfo;
    SteamNetworkingMessages()->GetSessionConnectionInfo(steamNetworkingIdentity, &connectionInfo, nullptr);

    Napi::Object result = Napi::Object::New(env);

    result.Set("state", Napi::Number::New(env, connectionInfo.m_eState));
    result.Set("endReason", Napi::Number::New(env, connectionInfo.m_eEndReason));
    result.Set("connectionDescription", Napi::String::New(env, connectionInfo.m_szConnectionDescription));
    result.Set("endDebug", Napi::String::New(env, connectionInfo.m_szEndDebug));

    return result;
}

SteamNetworkingMessage_t *networkingMessage;

Napi::Value ReceiveMessagesOnChannel(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    uint32 length = info[0].ToNumber().Uint32Value();

    bool useProvidedArray = info.Length() > 1 && info[1].IsTypedArray();

    Napi::Uint8Array array;

    if (useProvidedArray)
    {
        array = info[1].As<Napi::TypedArray>().As<Napi::Uint8Array>();

        if (array.ByteLength() < length)
        {
            THROW_BAD_ARGS("Bad arguments");
        }
    }
    else
    {
        array = Napi::Uint8Array::New(env, length);
    }

    if (networkingMessage == nullptr)
    {
        networkingMessage = SteamNetworkingUtils()->AllocateMessage(0);
    }

    networkingMessage->m_pData = array.Data();
    networkingMessage->m_cbSize = sizeof(uint8_t) * length;

    int messageCount = SteamNetworkingMessages()->ReceiveMessagesOnChannel(MESSAGE_CHANNEL, &networkingMessage, 1);
    if (messageCount != 0)
    {
        auto steamIdRemoteString =
            Napi::String::New(env, utils::uint64ToString(networkingMessage->m_identityPeer.GetSteamID64()));

        if (useProvidedArray)
        {
            return steamIdRemoteString;
        }

        Napi::Object result = Napi::Object::New(env);
        result.Set("steamIdRemote", steamIdRemoteString);
        result.Set("data", array);
        return result;
    }

    return env.Undefined();
}

Napi::Value SetSteamNetworkingMessagesSessionRequestCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnSteamNetworkingMessagesSessionRequestCallback.IsEmpty())
    {
        steamCallbacks->OnSteamNetworkingMessagesSessionRequestCallback.Reset();
    }

    steamCallbacks->OnSteamNetworkingMessagesSessionRequestCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value SetSteamNetworkingMessagesSessionFailedCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnSteamNetworkingMessagesSessionFailedCallback.IsEmpty())
    {
        steamCallbacks->OnSteamNetworkingMessagesSessionFailedCallback.Reset();
    }

    steamCallbacks->OnSteamNetworkingMessagesSessionFailedCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value SetP2PSessionRequestCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnP2PSessionRequestCallback.IsEmpty())
    {
        steamCallbacks->OnP2PSessionRequestCallback.Reset();
    }

    steamCallbacks->OnP2PSessionRequestCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value SetP2PSessionConnectFailCallback(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsFunction())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    if (steamCallbacks == nullptr)
    {
        THROW_BAD_ARGS("Internal error");
    }

    if (!steamCallbacks->OnP2PSessionConnectFailCallback.IsEmpty())
    {
        steamCallbacks->OnP2PSessionConnectFailCallback.Reset();
    }

    steamCallbacks->OnP2PSessionConnectFailCallback = Napi::Persistent(info[0].As<Napi::Function>());

    return env.Undefined();
}

Napi::Value AcceptP2PSessionWithUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID steamIdRemote(utils::strToUint64(steamIdString));

    bool success = SteamNetworking()->AcceptP2PSessionWithUser(steamIdRemote);

    return Napi::Boolean::New(env, success);
}

Napi::Value SendP2PPacket(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsTypedArray())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID steamIdRemote(utils::strToUint64(steamIdString));

    Napi::Uint8Array array = info[1].As<Napi::TypedArray>().As<Napi::Uint8Array>();
    uint8_t *dst = array.Data();
    uint32 length = sizeof(uint8_t) * array.ByteLength();

    bool sent = SteamNetworking()->SendP2PPacket(steamIdRemote, dst, length, EP2PSend::k_EP2PSendReliable);

    return Napi::Boolean::New(env, sent);
}

Napi::Value IsP2PPacketAvailable(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    uint32 messageSize;
    bool result = SteamNetworking()->IsP2PPacketAvailable(&messageSize);
    if (result && messageSize > 0)
    {
        return Napi::Number::New(env, messageSize);
    }
    else
    {
        return env.Undefined();
    }
}

Napi::Value ReadP2PPacket(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    uint32 length = info[0].ToNumber().Uint32Value();

    bool useProvidedArray = info.Length() > 1 && info[1].IsTypedArray();

    uint8_t *dst;
    Napi::Uint8Array array;

    if (useProvidedArray)
    {
        array = info[1].As<Napi::TypedArray>().As<Napi::Uint8Array>();

        if (array.ByteLength() < length)
        {
            THROW_BAD_ARGS("Bad arguments");
        }
    }
    else
    {
        array = Napi::Uint8Array::New(env, length);
    }

    dst = array.Data();

    uint32 packetSize;
    CSteamID steamIdRemote;
    bool success = SteamNetworking()->ReadP2PPacket(dst, sizeof(uint8_t) * length, &packetSize, &steamIdRemote);
    if (success)
    {
        auto steamIdRemoteString = Napi::String::New(env, utils::uint64ToString(steamIdRemote.ConvertToUint64()));

        if (useProvidedArray)
        {
            return steamIdRemoteString;
        }
        else
        {
            Napi::Object result = Napi::Object::New(env);
            result.Set("steamIdRemote", steamIdRemoteString);
            result.Set("data", array);
            return result;
        }
    }
    else
    {
        return env.Undefined();
    }
}

Napi::Value GetP2PSessionState(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID steamIdRemote(utils::strToUint64(steamIdString));

    P2PSessionState_t sessionState;
    bool success = SteamNetworking()->GetP2PSessionState(steamIdRemote, &sessionState);
    if (success)
    {
        Napi::Object result = Napi::Object::New(env);
        result.Set("connectionActive", Napi::Number::New(env, sessionState.m_bConnectionActive));
        result.Set("connecting", Napi::Number::New(env, sessionState.m_bConnecting));
        result.Set("lastError", Napi::Number::New(env, sessionState.m_eP2PSessionError));
        result.Set("usingRelay", Napi::Number::New(env, sessionState.m_bUsingRelay));
        return result;
    }
    else
    {
        return env.Undefined();
    }
}

Napi::Value CloseP2PSessionWithUser(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
        THROW_BAD_ARGS("Bad arguments");
    }

    std::string steamIdString = info[0].ToString().Utf8Value();
    CSteamID steamIdRemote(utils::strToUint64(steamIdString));

    bool result = SteamNetworking()->CloseP2PSessionWithUser(steamIdRemote);

    return Napi::Boolean::New(env, result);
}

void InitUtilsObject(Napi::Env env, Napi::Object exports)
{
    // Prepare constructor template
    Napi::Object tpl = Napi::Object::New(env);

    SET_FUNCTION_TPL("createArchive", CreateArchive);
    SET_FUNCTION_TPL("extractArchive", ExtractArchive);

    (exports).Set("Utils", tpl);
}

void InitNetworkingObject(Napi::Env env, Napi::Object exports)
{
    // Prepare constructor template
    Napi::Object tpl = Napi::Object::New(env);

    SET_FUNCTION_TPL("initRelayNetworkAccess", InitRelayNetworkAccess);
    SET_FUNCTION_TPL("getRelayNetworkStatus", GetRelayNetworkStatus);

    // new
    SET_FUNCTION_TPL("acceptSessionWithUser", AcceptSessionWithUser);
    SET_FUNCTION_TPL("sendMessageToUser", SendMessageToUser);
    SET_FUNCTION_TPL("closeSessionWithUser", CloseSessionWithUser);
    SET_FUNCTION_TPL("getSessionConnectionInfo", GetSessionConnectionInfo);
    SET_FUNCTION_TPL("receiveMessagesOnChannel", ReceiveMessagesOnChannel);
    SET_FUNCTION_TPL("setSteamNetworkingMessagesSessionRequestCallback",
                     SetSteamNetworkingMessagesSessionRequestCallback);
    SET_FUNCTION_TPL("setSteamNetworkingMessagesSessionFailedCallback",
                     SetSteamNetworkingMessagesSessionFailedCallback);

    // old
    SET_FUNCTION_TPL("acceptP2PSessionWithUser", AcceptP2PSessionWithUser);
    SET_FUNCTION_TPL("isP2PPacketAvailable", IsP2PPacketAvailable);
    SET_FUNCTION_TPL("sendP2PPacket", SendP2PPacket);
    SET_FUNCTION_TPL("readP2PPacket", ReadP2PPacket);
    SET_FUNCTION_TPL("getP2PSessionState", GetP2PSessionState);
    SET_FUNCTION_TPL("closeP2PSessionWithUser", CloseP2PSessionWithUser);
    SET_FUNCTION_TPL("setP2PSessionRequestCallback", SetP2PSessionRequestCallback);
    SET_FUNCTION_TPL("setP2PSessionConnectFailCallback", SetP2PSessionConnectFailCallback);

    exports.Set("networking", tpl);
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
    // Common APIs.
    SET_FUNCTION("initAPI", InitAPI);
    SET_FUNCTION("getSteamId", GetSteamId);
    SET_FUNCTION("runCallbacks", RunCallbacks);

    // File APIs.
    SET_FUNCTION("getFileCount", GetFileCount);
    SET_FUNCTION("getFileNameAndSize", GetFileNameAndSize);
    SET_FUNCTION("deleteRemoteFile", DeleteRemoteFile);
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

    // Lobby api
    SET_FUNCTION("createLobby", CreateLobby);
    SET_FUNCTION("leaveLobby", LeaveLobby);
    SET_FUNCTION("joinLobby", JoinLobby);
    SET_FUNCTION("setLobbyType", SetLobbyType);
    SET_FUNCTION("getLobbyData", GetLobbyData);
    SET_FUNCTION("setLobbyData", SetLobbyData);
    SET_FUNCTION("getLobbyOwner", GetLobbyOwner);
    SET_FUNCTION("getLobbyMembers", GetLobbyMembers);
    SET_FUNCTION("onLobbyCreated", OnLobbyCreated);
    SET_FUNCTION("onLobbyEntered", OnLobbyEntered);
    SET_FUNCTION("onLobbyChatUpdate", OnLobbyChatUpdate);
    SET_FUNCTION("onLobbyJoinRequested", OnLobbyJoinRequested);

    utils::InitUgcMatchingTypes(env, exports);
    utils::InitUgcQueryTypes(env, exports);
    utils::InitUserUgcListSortOrder(env, exports);
    utils::InitUserUgcList(env, exports);

    // Utils related APIs.
    InitUtilsObject(env, exports);

    // Networking apis
    InitNetworkingObject(env, exports);

    return exports;
}

NODE_API_MODULE(addon, InitAll)
