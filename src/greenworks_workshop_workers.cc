// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "greenworks_workshop_workers.h"

#include <algorithm>
#include <fstream>

#include "napi.h"
#include "steam/steam_api.h"
#include "uv.h"
#include "v8.h"

#include "greenworks_utils.h"

namespace
{
    Napi::Object ConvertToJsObject(Napi::Env env, const SteamUGCDetails_t& item)
    {
        Napi::Object result = Napi::Object::New(env);

        (result).Set("acceptedForUse", Napi::Boolean::New(env, item.m_bAcceptedForUse));
        (result).Set("banned", Napi::Boolean::New(env, item.m_bBanned));
        (result).Set("tagsTruncated", Napi::Boolean::New(env, item.m_bTagsTruncated));
        (result).Set("fileType", Napi::Number::New(env, item.m_eFileType));
        (result).Set("result", Napi::Number::New(env, item.m_eResult));
        (result).Set("visibility", Napi::Number::New(env, item.m_eVisibility));
        (result).Set("score", Napi::Number::New(env, item.m_flScore));

        (result).Set("file", Napi::String::New(env, utils::uint64ToString(item.m_hFile)));
        (result).Set("fileName", Napi::String::New(env, item.m_pchFileName));
        (result).Set("fileSize", Napi::Number::New(env, item.m_nFileSize));

        (result).Set("previewFile", Napi::String::New(env, utils::uint64ToString(item.m_hPreviewFile)));
        (result).Set("previewFileSize", Napi::Number::New(env, item.m_nPreviewFileSize));

        (result).Set("steamIDOwner", Napi::String::New(env, utils::uint64ToString(item.m_ulSteamIDOwner)));
        (result).Set("consumerAppID", Napi::Number::New(env, item.m_nConsumerAppID));
        (result).Set("creatorAppID", Napi::Number::New(env, item.m_nCreatorAppID));
        (result).Set("publishedFileId", Napi::String::New(env, utils::uint64ToString(item.m_nPublishedFileId)));

        (result).Set("title", Napi::String::New(env, item.m_rgchTitle));
        (result).Set("description", Napi::String::New(env, item.m_rgchDescription));
        (result).Set("URL", Napi::String::New(env, item.m_rgchURL));
        (result).Set("tags", Napi::String::New(env, item.m_rgchTags));

        (result).Set("timeAddedToUserList", Napi::Number::New(env, item.m_rtimeAddedToUserList));
        (result).Set("timeCreated", Napi::Number::New(env, item.m_rtimeCreated));
        (result).Set("timeUpdated", Napi::Number::New(env, item.m_rtimeUpdated));
        (result).Set("votesDown", Napi::Number::New(env, item.m_unVotesDown));
        (result).Set("votesUp", Napi::Number::New(env, item.m_unVotesUp));

        return result;
    }

    inline std::string GetAbsoluteFilePath(const std::string& file_path, const std::string& download_dir)
    {
        std::string file_name = file_path.substr(file_path.find_last_of("/\\") + 1);
        return download_dir + "/" + file_name;
    }
} // namespace

namespace greenworks
{
    FileShareWorker::FileShareWorker(Napi::Function& callback, const std::string& file_path)
        : SteamCallbackAsyncWorker(callback),
          file_path_(file_path)
    {
        share_file_handle_ = k_UGCHandleInvalid;
    }

    void FileShareWorker::Execute()
    {
        // Ignore empty path.
        if (file_path_.empty())
            return;

        std::string file_name = utils::GetFileNameFromPath(file_path_);

        if (!SteamRemoteStorage()->FileExists(file_name.c_str()))
        {
            SetError("File doesn't exist in remote storage");
            return;
        }

        SteamAPICall_t share_result = SteamRemoteStorage()->FileShare(
            file_name.c_str());
        call_result_.Set(share_result, this, &FileShareWorker::OnFileShareCompleted);

        // Wait for FileShare callback result.
        WaitForCompleted();
    }

    void FileShareWorker::OnFileShareCompleted(
        RemoteStorageFileShareResult_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError("Error on sharing file: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            share_file_handle_ = result->m_hFile;
        }
        else
        {
            SetErrorEx("Error on sharing file on Steam cloud %s, %s, %d", file_path_.c_str(), utils::GetFileNameFromPath(file_path_).c_str(), result->m_eResult);
        }
        is_completed_ = true;
    }

    void FileShareWorker::OnOK()
    {
        Callback().Call({Env().Null(), Napi::String::New(Env(), utils::uint64ToString(share_file_handle_))});
    }

    PublishWorkshopFileWorker::PublishWorkshopFileWorker(
        Napi::Function& callback,
        const std::string& file_path, const std::string& image_path,
        const std::string& title, const std::string& description,
        const std::vector<std::string>& tags) : SteamCallbackAsyncWorker(callback),
                                                file_path_(file_path),
                                                image_path_(image_path),
                                                title_(title),
                                                description_(description),
                                                tags_(tags)
    {
        publish_file_id_ = k_PublishedFileIdInvalid;
    }

    void PublishWorkshopFileWorker::Execute()
    {
        SteamParamStringArray_t tags;
        if (tags_.empty())
        {
            tags.m_nNumStrings = 0;
            tags.m_ppStrings = nullptr;
        }
        else
        {
            for (std::vector<std::string>::size_type i = 0; i < tags_.size(); i++)
            {
                tags_cc[i] = tags_[i].c_str();
            }

            tags.m_nNumStrings = tags_.size();
            tags.m_ppStrings = tags_cc;
        }

        std::string file_name = utils::GetFileNameFromPath(file_path_);
        std::string image_name = utils::GetFileNameFromPath(image_path_);
        SteamAPICall_t publish_result = SteamRemoteStorage()->PublishWorkshopFile(
            file_name.c_str(),
            image_name.empty() ? nullptr : image_name.c_str(),
            SteamUtils()->GetAppID(),
            title_.c_str(),
            description_.empty() ? nullptr : description_.c_str(),
            k_ERemoteStoragePublishedFileVisibilityPublic,
            &tags,
            k_EWorkshopFileTypeCommunity);

        call_result_.Set(publish_result, this,
                         &PublishWorkshopFileWorker::OnFilePublishCompleted);

        // Wait for FileShare callback result.
        WaitForCompleted();
    }

    void PublishWorkshopFileWorker::OnFilePublishCompleted(
        RemoteStoragePublishFileResult_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError("Error on publishing workshop file: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            publish_file_id_ = result->m_nPublishedFileId;
        }
        else
        {
            SetErrorEx("Error on publishing workshop file %d", result->m_eResult);
        }
        is_completed_ = true;
    }

    void PublishWorkshopFileWorker::OnOK()
    {
        Callback().Call({Env().Null(), Napi::String::New(Env(), utils::uint64ToString(publish_file_id_))});
    }

    UpdatePublishedWorkshopFileWorker::UpdatePublishedWorkshopFileWorker(
        Napi::Function& callback,
        PublishedFileId_t published_file_id, const std::string& file_path,
        const std::string& image_path, const std::string& title,
        const std::string& description,
        const std::vector<std::string>& tags) : SteamCallbackAsyncWorker(callback),
                                                published_file_id_(published_file_id),
                                                file_path_(file_path),
                                                image_path_(image_path),
                                                title_(title),
                                                description_(description),
                                                tags_(tags)
    {
    }

    void UpdatePublishedWorkshopFileWorker::Execute()
    {
        PublishedFileUpdateHandle_t update_handle =
            SteamRemoteStorage()->CreatePublishedFileUpdateRequest(
                published_file_id_);

        const std::string file_name = utils::GetFileNameFromPath(file_path_);
        const std::string image_name = utils::GetFileNameFromPath(image_path_);
        if (!file_name.empty())
            SteamRemoteStorage()->UpdatePublishedFileFile(update_handle,
                                                          file_name.c_str());
        if (!image_name.empty())
            SteamRemoteStorage()->UpdatePublishedFilePreviewFile(update_handle,
                                                                 image_name.c_str());
        if (!title_.empty())
            SteamRemoteStorage()->UpdatePublishedFileTitle(update_handle,
                                                           title_.c_str());
        if (!description_.empty())
            SteamRemoteStorage()->UpdatePublishedFileDescription(update_handle,
                                                                 description_.c_str());

        if (!tags_.empty())
        {
            SteamParamStringArray_t tags;
            if (tags_.empty())
            {
                tags.m_nNumStrings = 0;
                tags.m_ppStrings = nullptr;
            }
            else
            {
                for (std::vector<std::string>::size_type i = 0; i < tags_.size(); i++)
                {
                    tags_cc[i] = tags_[i].c_str();
                }

                tags.m_nNumStrings = tags_.size();
                tags.m_ppStrings = tags_cc;
            }

            SteamRemoteStorage()->UpdatePublishedFileTags(update_handle, &tags);
        }

        SteamAPICall_t commit_update_result =
            SteamRemoteStorage()->CommitPublishedFileUpdate(update_handle);
        update_published_file_call_result_.Set(commit_update_result, this,
                                               &UpdatePublishedWorkshopFileWorker::
                                                   OnCommitPublishedFileUpdateCompleted);

        // Wait for published workshop file updated.
        WaitForCompleted();
    }

    void UpdatePublishedWorkshopFileWorker::OnCommitPublishedFileUpdateCompleted(
        RemoteStorageUpdatePublishedFileResult_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError(
                "Error on committing published file update: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
        }
        else
        {
            SetErrorEx("Error on getting published file details %d", result->m_eResult);
        }

        is_completed_ = true;
    }

    QueryUGCWorker::QueryUGCWorker(Napi::Function& callback,
                                   EUGCMatchingUGCType ugc_matching_type)
        : SteamCallbackAsyncWorker(callback),
          ugc_matching_type_(ugc_matching_type)
    {
    }

    void QueryUGCWorker::OnOK()
    {
        Napi::Array items = Napi::Array::New(Env(), static_cast<int>(ugc_items_.size()));
        for (uint32_t i = 0; i < ugc_items_.size(); ++i)
            (items).Set(i, ConvertToJsObject(Env(), ugc_items_[i]));

        Callback().Call({Env().Null(), items});
    }

    void QueryUGCWorker::OnUGCQueryCompleted(SteamUGCQueryCompleted_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError("Error on querying all ugc: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            uint32 count = result->m_unNumResultsReturned;
            SteamUGCDetails_t item;
            for (uint32 i = 0; i < count; ++i)
            {
                SteamUGC()->GetQueryUGCResult(result->m_handle, i, &item);
                ugc_items_.push_back(item);
            }

            SteamUGC()->ReleaseQueryUGCRequest(result->m_handle);
        }
        else
        {
            SetErrorEx("Error on querying ugc %d", result->m_eResult);
        }

        is_completed_ = true;
    }

    QueryAllUGCWorker::QueryAllUGCWorker(Napi::Function& callback,
                                         EUGCMatchingUGCType ugc_matching_type,
                                         EUGCQuery ugc_query_type)
        : QueryUGCWorker(callback, ugc_matching_type),
          ugc_query_type_(ugc_query_type)
    {
    }

    void QueryAllUGCWorker::Execute()
    {
        uint32 app_id = SteamUtils()->GetAppID();
        uint32 invalid_app_id = 0;
        // Set "creator_app_id" parameter to an invalid id to make Steam API return
        // all ugc items, otherwise the API won't get any results in some cases.
        UGCQueryHandle_t ugc_handle = SteamUGC()->CreateQueryAllUGCRequest(
            ugc_query_type_, ugc_matching_type_, /*creator_app_id=*/invalid_app_id,
            /*consumer_app_id=*/app_id, 1);

        SteamAPICall_t ugc_query_result = SteamUGC()->SendQueryUGCRequest(ugc_handle);
        ugc_query_call_result_.Set(ugc_query_result, this, &QueryAllUGCWorker::OnUGCQueryCompleted);

        // Wait for query all ugc completed.
        WaitForCompleted();
    }

    QueryUserUGCWorker::QueryUserUGCWorker(Napi::Function& callback,
                                           EUGCMatchingUGCType ugc_matching_type,
                                           EUserUGCList ugc_list, EUserUGCListSortOrder ugc_list_sort_order)
        : QueryUGCWorker(callback, ugc_matching_type),
          ugc_list_(ugc_list),
          ugc_list_sort_order_(ugc_list_sort_order)
    {
    }

    void QueryUserUGCWorker::Execute()
    {
        uint32 app_id = SteamUtils()->GetAppID();

        UGCQueryHandle_t ugc_handle = SteamUGC()->CreateQueryUserUGCRequest(
            SteamUser()->GetSteamID().GetAccountID(),
            ugc_list_,
            ugc_matching_type_,
            ugc_list_sort_order_,
            app_id,
            app_id,
            1);
        SteamAPICall_t ugc_query_result = SteamUGC()->SendQueryUGCRequest(ugc_handle);
        ugc_query_call_result_.Set(ugc_query_result, this,
                                   &QueryUserUGCWorker::OnUGCQueryCompleted);

        // Wait for query all ugc completed.
        WaitForCompleted();
    }

    DownloadItemWorker::DownloadItemWorker(Napi::Function& callback,
                                           UGCHandle_t download_file_handle,
                                           const std::string& download_dir)
        : SteamCallbackAsyncWorker(callback),
          download_file_handle_(download_file_handle),
          download_dir_(download_dir)
    {
    }

    void DownloadItemWorker::Execute()
    {
        SteamAPICall_t download_item_result = SteamRemoteStorage()->UGCDownload(download_file_handle_, 0);
        call_result_.Set(download_item_result, this, &DownloadItemWorker::OnDownloadCompleted);

        // Wait for downloading file completed.
        WaitForCompleted();
    }

    void DownloadItemWorker::OnDownloadCompleted(
        RemoteStorageDownloadUGCResult_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError(
                "Error on downloading file: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            std::string target_path = GetAbsoluteFilePath(result->m_pchFileName,
                                                          download_dir_);

            int file_size_in_bytes = result->m_nSizeInBytes;
            char* content = new char[file_size_in_bytes];

            SteamRemoteStorage()->UGCRead(download_file_handle_,
                                          content, file_size_in_bytes, 0, k_EUGCRead_Close);
            if (!utils::WriteFile(target_path, content, file_size_in_bytes))
            {
                SetError("Error on saving file on local machine.");
            }

            delete[] content;
        }
        else
        {
            SetErrorEx("Error on downloading file %d", result->m_eResult);
        }
        is_completed_ = true;
    }

    SynchronizeItemsWorker::SynchronizeItemsWorker(Napi::Function& callback,
                                                   const std::string& download_dir)
        : SteamCallbackAsyncWorker(callback),
          current_download_items_pos_(0),
          download_dir_(download_dir)
    {
    }

    void SynchronizeItemsWorker::Execute()
    {
        uint32 app_id = SteamUtils()->GetAppID();

        UGCQueryHandle_t ugc_handle = SteamUGC()->CreateQueryUserUGCRequest(
            SteamUser()->GetSteamID().GetAccountID(),
            k_EUserUGCList_Subscribed,
            k_EUGCMatchingUGCType_Items,
            k_EUserUGCListSortOrder_SubscriptionDateDesc,
            app_id,
            app_id,
            1);
        SteamAPICall_t ugc_query_result = SteamUGC()->SendQueryUGCRequest(ugc_handle);
        ugc_query_call_result_.Set(ugc_query_result, this, &SynchronizeItemsWorker::OnUGCQueryCompleted);

        // Wait for synchronization completed.
        WaitForCompleted();
    }

    void SynchronizeItemsWorker::OnUGCQueryCompleted(SteamUGCQueryCompleted_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError("Error on querying all ugc: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            SteamUGCDetails_t item;
            for (uint32 i = 0; i < result->m_unNumResultsReturned; ++i)
            {
                SteamUGC()->GetQueryUGCResult(result->m_handle, i, &item);
                std::string target_path = GetAbsoluteFilePath(item.m_pchFileName,
                                                              download_dir_);
                int64 file_update_time = utils::GetFileLastUpdatedTime(
                    target_path.c_str());
                ugc_items_.push_back(item);

                // If the file is not existed or last update time is not equal to Steam,
                // download it.
                utils::DebugLog("%s. file modified time: %d. steamworks item updated time: %d\n", target_path.c_str(), file_update_time, item.m_rtimeUpdated);

                if (file_update_time == -1 || file_update_time != item.m_rtimeUpdated)
                {
                    utils::DebugLog("\tUpdating file\n");
                    ugc_items_to_download.push_back(item);
                }
            }

            // Start download the file.
            bool hasItemsToDownload = !ugc_items_to_download.empty();
            if (hasItemsToDownload)
            {
                // auto targetPath = GetAbsoluteFilePath(result->m_pchFileName, download_dir_);
                // SteamAPICall_t download_item_result = SteamRemoteStorage()->UGCDownloadToLocation(ugc_items_to_download[current_download_items_pos_].m_hFile, targetPath.c_str(), 0);
                SteamAPICall_t download_item_result = SteamRemoteStorage()->UGCDownload(ugc_items_to_download[current_download_items_pos_].m_hFile, 0);
                download_call_result_.Set(download_item_result, this, &SynchronizeItemsWorker::OnDownloadCompleted);
            }

            SteamUGC()->ReleaseQueryUGCRequest(result->m_handle);

            if (hasItemsToDownload)
            {
                // is_completed_ will be set to true once all the downloads complete
                return;
            }
        }
        else
        {
            SetErrorEx("Error on querying ugc %d", result->m_eResult);
        }

        is_completed_ = true;
    }

    void SynchronizeItemsWorker::OnDownloadCompleted(RemoteStorageDownloadUGCResult_t* result, bool io_failure)
    {
        if (io_failure)
        {
            SetError(
                "Error on downloading file: Steam API IO Failure");
        }
        else if (result->m_eResult == k_EResultOK)
        {
            std::string target_path = GetAbsoluteFilePath(result->m_pchFileName, download_dir_);

            std::ofstream fileStream(target_path.c_str(), std::ios::out | std::ios::binary);

            int fileSizeInBytes = result->m_nSizeInBytes;

            char* pBuffer = new char[FILE_BUFFER_SIZE];

            int readOffset = 0;

            while (readOffset < fileSizeInBytes)
            {
                int bytesRead = SteamRemoteStorage()->UGCRead(result->m_hFile, pBuffer, FILE_BUFFER_SIZE, readOffset, k_EUGCRead_ContinueReadingUntilFinished);
                if (bytesRead <= 0)
                {
                    break;
                }

                fileStream.write(pBuffer, bytesRead);

                readOffset += bytesRead;
            }

            delete[] pBuffer;

            // flush before checking the status
            fileStream.flush();

            bool is_save_success = fileStream.good();
            if (!is_save_success)
            {
                SetError("Error on saving file on local machine.");
                is_completed_ = true;
                return;
            }

            // close the stream now so we can update the modified time
            fileStream.close();

            int64 file_updated_time = ugc_items_to_download[current_download_items_pos_].m_rtimeUpdated;
            if (!utils::UpdateFileLastUpdatedTime(target_path.c_str(), static_cast<time_t>(file_updated_time)))
            {
                SetError("Error on update file time on local machine.");
                is_completed_ = true;
                return;
            }

            ++current_download_items_pos_;
            if (current_download_items_pos_ < ugc_items_to_download.size())
            {
                SteamAPICall_t download_item_result = SteamRemoteStorage()->UGCDownload(ugc_items_to_download[current_download_items_pos_].m_hFile, 0);
                download_call_result_.Set(download_item_result, this, &SynchronizeItemsWorker::OnDownloadCompleted);
                return;
            }
        }
        else
        {
            SetErrorEx("Error on downloading file %d", result->m_eResult);
        }
        is_completed_ = true;
    }

    void SynchronizeItemsWorker::OnOK()
    {
        Napi::Array items = Napi::Array::New(Env(), static_cast<int>(ugc_items_.size()));

        for (uint32_t i = 0; i < ugc_items_.size(); ++i)
        {
            Napi::Object item = ConvertToJsObject(Env(), ugc_items_[i]);
            bool is_updated = std::find_if(ugc_items_to_download.begin(), ugc_items_to_download.end(), [&](SteamUGCDetails_t const& item)
                                           { return item.m_hFile == ugc_items_[i].m_hFile; }) != ugc_items_to_download.end();
            (item).Set("isUpdated", Napi::Boolean::New(Env(), is_updated));
            (items).Set(i, item);
        }

        Callback().Call({Env().Null(), items});
    }

    UnsubscribePublishedFileWorker::UnsubscribePublishedFileWorker(
        Napi::Function& callback,
        PublishedFileId_t unsubscribe_file_id)
        : SteamCallbackAsyncWorker(callback),
          unsubscribe_file_id_(unsubscribe_file_id)
    {
    }

    void UnsubscribePublishedFileWorker::Execute()
    {
        SteamAPICall_t unsubscribed_result =
            SteamRemoteStorage()->UnsubscribePublishedFile(unsubscribe_file_id_);
        unsubscribe_call_result_.Set(unsubscribed_result, this,
                                     &UnsubscribePublishedFileWorker::OnUnsubscribeCompleted);

        // Wait for unsubscribing job completed.
        WaitForCompleted();
    }

    void UnsubscribePublishedFileWorker::OnUnsubscribeCompleted(
        RemoteStoragePublishedFileUnsubscribed_t* result, bool io_failure)
    {
        is_completed_ = true;
    }
} // namespace greenworks
