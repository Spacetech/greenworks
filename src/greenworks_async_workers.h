// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_GREENWORK_ASYNC_WORKERS_H_
#define SRC_GREENWORK_ASYNC_WORKERS_H_

#include <string>
#include <vector>

#include "steam/steam_api.h"

#include "steam_async_worker.h"

namespace greenworks
{
    class FileContentSaveWorker : public SteamAsyncWorker
    {
      public:
        FileContentSaveWorker(Napi::Function& callback, std::string file_name, std::string content);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;

      private:
        std::string file_name_;
        std::string content_;
    };

    class FilesSaveWorker : public SteamAsyncWorker
    {
      public:
        FilesSaveWorker(Napi::Function& callback, const std::vector<std::string>& files_path);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;

      private:
        std::vector<std::string> files_path_;
    };

    class FileReadWorker : public SteamAsyncWorker
    {
      public:
        FileReadWorker(Napi::Function& callback, std::string file_name);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;
        virtual void OnOK() override;

      private:
        std::string file_name_;
        std::string content_;
    };

    class CloudQuotaGetWorker : public SteamAsyncWorker
    {
      public:
        CloudQuotaGetWorker(Napi::Function& callback);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;
        virtual void OnOK() override;

      private:
        uint64 total_bytes_;
        uint64 available_bytes_;
    };

    class ActivateAchievementWorker : public SteamAsyncWorker
    {
      public:
        ActivateAchievementWorker(Napi::Function& callback, const std::string& achievement);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;

      private:
        std::string achievement_;
    };

    class GetAchievementWorker : public SteamAsyncWorker
    {
      public:
        GetAchievementWorker(Napi::Function& callback, const std::string& achievement);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;
        virtual void OnOK() override;

      private:
        std::string achievement_;
        bool is_achieved_;
    };

    class ClearAchievementWorker : public SteamAsyncWorker
    {
      public:
        ClearAchievementWorker(Napi::Function& callback, const std::string& achievement);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;
        virtual void OnOK() override;

      private:
        std::string achievement_;
        bool success_;
    };

    class GetNumberOfPlayersWorker : public SteamCallbackAsyncWorker
    {
      public:
        GetNumberOfPlayersWorker(Napi::Function& callback);
        void OnGetNumberOfPlayersCompleted(NumberOfCurrentPlayers_t* result,
                                           bool io_failure);
        // Override NanAsyncWorker methods.
        virtual void Execute() override;
        virtual void OnOK() override;

      private:
        int num_of_players_;
        CCallResult<GetNumberOfPlayersWorker, NumberOfCurrentPlayers_t> call_result_;
    };

    class CreateArchiveWorker : public SteamAsyncWorker
    {
      public:
        CreateArchiveWorker(Napi::Function& callback,
                            const std::string& zip_file_path,
                            const std::string& source_dir,
                            const std::string& password,
                            int compress_level);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;

      private:
        std::string zip_file_path_;
        std::string source_dir_;
        std::string password_;
        int compress_level_;
    };

    class ExtractArchiveWorker : public SteamAsyncWorker
    {
      public:
        ExtractArchiveWorker(Napi::Function& callback,
                             const std::string& zip_file_path,
                             const std::string& extract_path,
                             const std::string& password);

        // Override NanAsyncWorker methods.
        virtual void Execute() override;

      private:
        std::string zip_file_path_;
        std::string extract_path_;
        std::string password_;
    };

    class StoreUserStatsWorker : public SteamCallbackAsyncWorker
    {
      public:
        StoreUserStatsWorker(Napi::Function& callback);
        STEAM_CALLBACK(StoreUserStatsWorker,
                       OnUserStatsStored,
                       UserStatsStored_t,
                       result);

        // Override NanAsyncWorker methods.
        void Execute() override;
        void OnOK() override;

      private:
        uint64 game_id_;
        CSteamID steam_id_user_;
    };

} // namespace greenworks

#endif // SRC_GREENWORK_ASYNC_WORKERS_H_
