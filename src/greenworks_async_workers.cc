// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "greenworks_async_workers.h"

#include "napi.h"
#include "steam/steam_api.h"
#include "uv.h"
#include "v8.h"

#include "greenworks_unzip.h"
#include "greenworks_utils.h"
#include "greenworks_zip.h"

#include <fstream>

FileContentSaveWorker::FileContentSaveWorker(Napi::Function &callback, std::string file_name, std::string content)
    : SteamAsyncWorker(callback), file_name_(file_name), content_(content)
{
}

void FileContentSaveWorker::Execute()
{
    if (!SteamRemoteStorage()->FileWrite(file_name_.c_str(), content_.c_str(), static_cast<int32>(content_.size())))
        SetError("Error on writing to file.");
}

FilesSaveWorker::FilesSaveWorker(Napi::Function &callback, const std::vector<std::string> &files_path)
    : SteamAsyncWorker(callback), files_path_(files_path)
{
}

void FilesSaveWorker::Execute()
{
    char *pBuffer = new char[FILE_BUFFER_SIZE];

    for (size_t i = 0; i < files_path_.size(); ++i)
    {
        auto filePath = files_path_[i];

        std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);
        if (!fileStream.is_open())
        {
            SetError("Failed to open file (1)");
            break;
        }

        std::string file_name = utils::GetFileNameFromPath(files_path_[i]);

        auto remoteFileHandle = SteamRemoteStorage()->FileWriteStreamOpen(file_name.c_str());
        if (remoteFileHandle == k_UGCFileStreamHandleInvalid)
        {
            SetError("Failed to open file write stream");
            break;
        }

        bool wroteChunk = true;

        while (wroteChunk && !fileStream.eof())
        {
            fileStream.read(pBuffer, FILE_BUFFER_SIZE);

            std::streamsize readLength = fileStream.gcount();

            wroteChunk = SteamRemoteStorage()->FileWriteStreamWriteChunk(remoteFileHandle, pBuffer, readLength);
        }

        if (!wroteChunk)
        {
            SetError("Failed to write chunk to file stream");
            break;
        }

        if (!SteamRemoteStorage()->FileWriteStreamClose(remoteFileHandle))
        {
            SetError("Failed to close file stream");
            break;
        }
    }

    delete[] pBuffer;
}

FileReadWorker::FileReadWorker(Napi::Function &callback, std::string file_name)
    : SteamAsyncWorker(callback), file_name_(file_name)
{
}

void FileReadWorker::Execute()
{
    ISteamRemoteStorage *steam_remote_storage = SteamRemoteStorage();

    if (!steam_remote_storage->FileExists(file_name_.c_str()))
    {
        SetError("File doesn't exist.");
        return;
    }

    int32 file_size = steam_remote_storage->GetFileSize(file_name_.c_str());

    char *content = new char[file_size + 1];
    int32 end_pos = steam_remote_storage->FileRead(file_name_.c_str(), content, file_size);
    content[end_pos] = '\0';

    if (end_pos == 0 && file_size > 0)
    {
        SetError("Error on reading file.");
    }
    else
    {
        content_ = std::string(content);
    }

    delete[] content;
}

void FileReadWorker::OnOK()
{
    Callback().Call({Env().Null(), Napi::String::New(Env(), content_)});
}

CloudQuotaGetWorker::CloudQuotaGetWorker(Napi::Function &callback)
    : SteamAsyncWorker(callback), total_bytes_(-1), available_bytes_(-1)
{
}

void CloudQuotaGetWorker::Execute()
{
    ISteamRemoteStorage *steam_remote_storage = SteamRemoteStorage();

    if (!steam_remote_storage->GetQuota(&total_bytes_, &available_bytes_))
    {
        SetError("Error on getting cloud quota.");
        return;
    }
}

void CloudQuotaGetWorker::OnOK()
{
    //     Napi::Value argv[] = {Napi::New(env, utils::uint64ToString(total_bytes_)),
    //                           Napi::New(env, utils::uint64ToString(available_bytes_))};
    Callback().Call({Env().Null(), Napi::Number::New(Env(), total_bytes_), Napi::Number::New(Env(), available_bytes_)});
}

ActivateAchievementWorker::ActivateAchievementWorker(Napi::Function &callback, const std::string &achievement)
    : SteamAsyncWorker(callback), achievement_(achievement)
{
}

void ActivateAchievementWorker::Execute()
{
    ISteamUserStats *steam_user_stats = SteamUserStats();

    steam_user_stats->SetAchievement(achievement_.c_str());
    if (!steam_user_stats->StoreStats())
    {
        SetError("Error on storing user achievement");
    }
}

GetAchievementWorker::GetAchievementWorker(Napi::Function &callback, const std::string &achievement)
    : SteamAsyncWorker(callback), achievement_(achievement), is_achieved_(false)
{
}

void GetAchievementWorker::Execute()
{
    ISteamUserStats *steam_user_stats = SteamUserStats();
    bool success = steam_user_stats->GetAchievement(achievement_.c_str(), &is_achieved_);
    if (!success)
    {
        SetError("Achivement name is not valid.");
    }
}

void GetAchievementWorker::OnOK()
{
    Callback().Call({Env().Null(), Napi::Boolean::New(Env(), is_achieved_)});
}

ClearAchievementWorker::ClearAchievementWorker(Napi::Function &callback, const std::string &achievement)
    : SteamAsyncWorker(callback), achievement_(achievement), success_(false)
{
}

void ClearAchievementWorker::Execute()
{
    ISteamUserStats *steam_user_stats = SteamUserStats();
    success_ = steam_user_stats->ClearAchievement(achievement_.c_str());
    if (!success_)
    {
        SetError("Achievement name is not valid.");
    }
    else if (!steam_user_stats->StoreStats())
    {
        SetError("Fails on uploading user stats to server.");
    }
}

void ClearAchievementWorker::OnOK()
{
    Callback().Call({Env().Null()});
}

GetNumberOfPlayersWorker::GetNumberOfPlayersWorker(Napi::Function &callback)
    : SteamCallbackAsyncWorker(callback), num_of_players_(-1)
{
}

void GetNumberOfPlayersWorker::Execute()
{
    SteamAPICall_t steam_api_call = SteamUserStats()->GetNumberOfCurrentPlayers();
    call_result_.Set(steam_api_call, this, &GetNumberOfPlayersWorker::OnGetNumberOfPlayersCompleted);

    WaitForCompleted();
}

void GetNumberOfPlayersWorker::OnGetNumberOfPlayersCompleted(NumberOfCurrentPlayers_t *result, bool io_failure)
{
    if (io_failure)
    {
        SetError("Error on getting number of players: Steam API IO Failure");
    }
    else if (result->m_bSuccess)
    {
        num_of_players_ = result->m_cPlayers;
    }
    else
    {
        SetError("Error on getting number of players.");
    }
    is_completed_ = true;
}

void GetNumberOfPlayersWorker::OnOK()
{
    Callback().Call({Env().Null(), Napi::Number::New(Env(), num_of_players_)});
}

CreateArchiveWorker::CreateArchiveWorker(Napi::Function &callback, const std::string &zip_file_path,
                                         const std::string &source_dir, const std::string &password, int compress_level)
    : SteamAsyncWorker(callback), zip_file_path_(zip_file_path), source_dir_(source_dir), password_(password),
      compress_level_(compress_level)
{
}

void CreateArchiveWorker::Execute()
{
    int result = zip(zip_file_path_.c_str(), source_dir_.c_str(), compress_level_,
                     password_.empty() ? nullptr : password_.c_str());
    if (result)
        SetError("Error on creating zip file.");
}

ExtractArchiveWorker::ExtractArchiveWorker(Napi::Function &callback, const std::string &zip_file_path,
                                           const std::string &extract_path, const std::string &password)
    : SteamAsyncWorker(callback), zip_file_path_(zip_file_path), extract_path_(extract_path), password_(password)
{
}

void ExtractArchiveWorker::Execute()
{
    int result = unzip(zip_file_path_.c_str(), extract_path_.c_str(), password_.empty() ? nullptr : password_.c_str());
    if (result)
        SetError("Error on extracting zip file.");
}

StoreUserStatsWorker::StoreUserStatsWorker(Napi::Function &callback)
    : SteamCallbackAsyncWorker(callback), result(this, &StoreUserStatsWorker::OnUserStatsStored)
{
}

void StoreUserStatsWorker::Execute()
{
    if (SteamUserStats()->StoreStats())
    {
        WaitForCompleted();
    }
    else
    {
        SetError("Error storing user stats");
    }
}

void StoreUserStatsWorker::OnUserStatsStored(UserStatsStored_t *result)
{
    if (result->m_eResult != k_EResultOK)
    {
        SetError("Error on storing user stats: " + std::to_string(result->m_eResult));
    }
    else
    {
        game_id_ = result->m_nGameID;
    }

    is_completed_ = true;
}

void StoreUserStatsWorker::OnOK()
{
    //     Napi::Value argv[] = {   Napi::New(env, utils::uint64ToString(game_id_))};
    Callback().Call({Env().Null(), Napi::Number::New(Env(), game_id_)});
}
