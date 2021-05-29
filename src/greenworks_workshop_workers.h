// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_GREENWORK_WORKSHOP_WORKERS_H_
#define SRC_GREENWORK_WORKSHOP_WORKERS_H_

#define MAX_TAGS 100

#include "steam_async_worker.h"

#include <vector>

#include "steam/steam_api.h"

class FileShareWorker : public SteamCallbackAsyncWorker
{
  public:
    FileShareWorker(Napi::Function &callback, const std::string &file_path);
    void OnFileShareCompleted(RemoteStorageFileShareResult_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;
    virtual void OnOK() override;

  private:
    const std::string file_path_;
    UGCHandle_t share_file_handle_;
    CCallResult<FileShareWorker, RemoteStorageFileShareResult_t> call_result_;
};

class PublishWorkshopFileWorker : public SteamCallbackAsyncWorker
{
  public:
    PublishWorkshopFileWorker(Napi::Function &callback,

                              const std::string &file_path, const std::string &image_path, const std::string &title,
                              const std::string &description, const std::vector<std::string> &tags);
    void OnFilePublishCompleted(RemoteStoragePublishFileResult_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;
    virtual void OnOK() override;

  private:
    std::string file_path_;
    std::string image_path_;
    std::string title_;
    std::string description_;
    std::vector<std::string> tags_;
    const char *tags_cc[MAX_TAGS];

    PublishedFileId_t publish_file_id_;
    CCallResult<PublishWorkshopFileWorker, RemoteStoragePublishFileResult_t> call_result_;
};

class UpdatePublishedWorkshopFileWorker : public SteamCallbackAsyncWorker
{
  public:
    UpdatePublishedWorkshopFileWorker(Napi::Function &callback,

                                      PublishedFileId_t published_file_id, const std::string &file_path,
                                      const std::string &image_path, const std::string &title,
                                      const std::string &description, const std::vector<std::string> &tags);
    void OnCommitPublishedFileUpdateCompleted(RemoteStorageUpdatePublishedFileResult_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;

  private:
    PublishedFileId_t published_file_id_;
    std::string file_path_;
    std::string image_path_;
    std::string title_;
    std::string description_;
    std::vector<std::string> tags_;
    const char *tags_cc[MAX_TAGS];

    CCallResult<UpdatePublishedWorkshopFileWorker, RemoteStorageUpdatePublishedFileResult_t>
        update_published_file_call_result_;
};

// A base worker class for querying (user/all) ugc.
class QueryUGCWorker : public SteamCallbackAsyncWorker
{
  public:
    QueryUGCWorker(Napi::Function &callback,

                   EUGCMatchingUGCType ugc_matching_type);
    void OnUGCQueryCompleted(SteamUGCQueryCompleted_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void OnOK() override;

  protected:
    EUGCMatchingUGCType ugc_matching_type_;
    std::vector<SteamUGCDetails_t> ugc_items_;

    CCallResult<QueryUGCWorker, SteamUGCQueryCompleted_t> ugc_query_call_result_;
};

class QueryAllUGCWorker : public QueryUGCWorker
{
  public:
    QueryAllUGCWorker(Napi::Function &callback,

                      EUGCMatchingUGCType ugc_matching_type, EUGCQuery ugc_query_type);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;

  private:
    EUGCQuery ugc_query_type_;
};

class QueryUserUGCWorker : public QueryUGCWorker
{
  public:
    QueryUserUGCWorker(Napi::Function &callback,

                       EUGCMatchingUGCType ugc_matching_type, EUserUGCList ugc_list,
                       EUserUGCListSortOrder ugc_list_sort_order);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;

  private:
    EUserUGCList ugc_list_;
    EUserUGCListSortOrder ugc_list_sort_order_;
};

class DownloadItemWorker : public SteamCallbackAsyncWorker
{
  public:
    DownloadItemWorker(Napi::Function &callback,

                       UGCHandle_t download_file_handle, const std::string &download_dir);

    void OnDownloadCompleted(RemoteStorageDownloadUGCResult_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;

  private:
    UGCHandle_t download_file_handle_;
    std::string download_dir_;
    CCallResult<DownloadItemWorker, RemoteStorageDownloadUGCResult_t> call_result_;
};

class SynchronizeItemsWorker : public SteamCallbackAsyncWorker
{
  public:
    SynchronizeItemsWorker(Napi::Function &callback,

                           const std::string &download_dir);

    void OnUGCQueryCompleted(SteamUGCQueryCompleted_t *result, bool io_failure);
    void OnDownloadCompleted(RemoteStorageDownloadUGCResult_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;
    virtual void OnOK() override;

  private:
    size_t current_download_items_pos_;
    std::string download_dir_;
    std::vector<SteamUGCDetails_t> ugc_items_;
    std::vector<SteamUGCDetails_t> ugc_items_to_download;
    CCallResult<SynchronizeItemsWorker, RemoteStorageDownloadUGCResult_t> download_call_result_;
    CCallResult<SynchronizeItemsWorker, SteamUGCQueryCompleted_t> ugc_query_call_result_;
};

class UnsubscribePublishedFileWorker : public SteamCallbackAsyncWorker
{
  public:
    UnsubscribePublishedFileWorker(Napi::Function &callback,

                                   PublishedFileId_t unsubscribe_file_id);

    void OnUnsubscribeCompleted(RemoteStoragePublishedFileUnsubscribed_t *result, bool io_failure);

    // Override NanAsyncWorker methods.
    virtual void Execute() override;

  private:
    PublishedFileId_t unsubscribe_file_id_;

    CCallResult<UnsubscribePublishedFileWorker, RemoteStoragePublishedFileUnsubscribed_t> unsubscribe_call_result_;
};

#endif // SRC_GREENWORK_WORKSHOP_WORKERS_H_
