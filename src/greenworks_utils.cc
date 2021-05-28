// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "greenworks_utils.h"

#include <fstream>
#include <sstream>

#include "napi.h"
#include "steam/steam_api.h"
#include "uv.h"

#include <stdio.h>

#if defined(_WIN32)
#include <sys/utime.h>
#include <windows.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <utime.h>
#endif

namespace utils
{
    void InitUgcMatchingTypes(Napi::Env env, Napi::Object exports)
    {
        Napi::Object ugc_matching_type = Napi::Object::New(env);
        (ugc_matching_type).Set("Items", Napi::Number::New(env, k_EUGCMatchingUGCType_Items));
        (ugc_matching_type).Set("ItemsMtx", Napi::Number::New(env, k_EUGCMatchingUGCType_Items_Mtx));
        (ugc_matching_type).Set("ItemsReadyToUse", Napi::Number::New(env, k_EUGCMatchingUGCType_Items_ReadyToUse));
        (ugc_matching_type).Set("Collections", Napi::Number::New(env, k_EUGCMatchingUGCType_Collections));
        (ugc_matching_type).Set("Artwork", Napi::Number::New(env, k_EUGCMatchingUGCType_Artwork));
        (ugc_matching_type).Set("Videos", Napi::Number::New(env, k_EUGCMatchingUGCType_Videos));
        (ugc_matching_type).Set("Screenshots", Napi::Number::New(env, k_EUGCMatchingUGCType_Screenshots));
        (ugc_matching_type).Set("AllGNumber::uides", Napi::Number::New(env, k_EUGCMatchingUGCType_AllGuides));
        (ugc_matching_type).Set("WebGuides", Napi::Number::New(env, k_EUGCMatchingUGCType_WebGuides));
        (ugc_matching_type).Set("IntegratedGuides", Napi::Number::New(env, k_EUGCMatchingUGCType_IntegratedGuides));
        (ugc_matching_type).Set("UsableInGame", Napi::Number::New(env, k_EUGCMatchingUGCType_UsableInGame));
        (ugc_matching_type).Set("ControllerBindings", Napi::Number::New(env, k_EUGCMatchingUGCType_ControllerBindings));
        (exports).Set("UGCMatchingType", ugc_matching_type);
    }

    void InitUgcQueryTypes(Napi::Env env, Napi::Object exports)
    {
        Napi::Object ugc_query_type = Napi::Object::New(env);
        (ugc_query_type).Set("RankedByVote", Napi::Number::New(env, k_EUGCQuery_RankedByVote));
        (ugc_query_type).Set("RankedByPublicationDate", Napi::Number::New(env, k_EUGCQuery_RankedByPublicationDate));
        (ugc_query_type).Set("AcceptedForGameRankedByAcceptanceDate", Napi::Number::New(env, k_EUGCQuery_AcceptedForGameRankedByAcceptanceDate));
        (ugc_query_type).Set("RankedByTrend", Napi::Number::New(env, k_EUGCQuery_RankedByTrend));
        (ugc_query_type).Set("FavoritedByFriendsRankedByPublicationDate", Napi::Number::New(env, k_EUGCQuery_FavoritedByFriendsRankedByPublicationDate));
        (ugc_query_type).Set("CreatedByFriendsRankedByPublicationDate", Napi::Number::New(env, k_EUGCQuery_CreatedByFriendsRankedByPublicationDate));
        (ugc_query_type).Set("RankedByNumTimesReported", Napi::Number::New(env, k_EUGCQuery_RankedByNumTimesReported));
        (ugc_query_type).Set("CreatedByFollowedUsersRankedByPublicationDate", Napi::Number::New(env, k_EUGCQuery_CreatedByFollowedUsersRankedByPublicationDate));
        (ugc_query_type).Set("NotYetRated", Napi::Number::New(env, k_EUGCQuery_NotYetRated));
        (ugc_query_type).Set("RankedByTotalVotesAsc", Napi::Number::New(env, k_EUGCQuery_RankedByTotalVotesAsc));
        (ugc_query_type).Set("RankedByVotesUp", Napi::Number::New(env, k_EUGCQuery_RankedByVotesUp));
        (ugc_query_type).Set("RankedByTextSearch", Napi::Number::New(env, k_EUGCQuery_RankedByTextSearch));

        (exports).Set("UGCQueryType", ugc_query_type);
    }

    void InitUserUgcList(Napi::Env env, Napi::Object exports)
    {
        Napi::Object ugc_list = Napi::Object::New(env);
        (ugc_list).Set("Published", Napi::Number::New(env, k_EUserUGCList_Published));
        (ugc_list).Set("VotedOn", Napi::Number::New(env, k_EUserUGCList_VotedOn));
        (ugc_list).Set("VotedUp", Napi::Number::New(env, k_EUserUGCList_VotedUp));
        (ugc_list).Set("VotedDown", Napi::Number::New(env, k_EUserUGCList_VotedDown));
        (ugc_list).Set("WillVoteLater", Napi::Number::New(env, k_EUserUGCList_WillVoteLater));
        (ugc_list).Set("Favorited", Napi::Number::New(env, k_EUserUGCList_Favorited));
        (ugc_list).Set("Subscribed", Napi::Number::New(env, k_EUserUGCList_Subscribed));
        (ugc_list).Set("UsedOrPlayer", Napi::Number::New(env, k_EUserUGCList_UsedOrPlayed));
        (ugc_list).Set("Followed", Napi::Number::New(env, k_EUserUGCList_Followed));

        (exports).Set("UserUGCList", ugc_list);
    }

    void InitUserUgcListSortOrder(Napi::Env env, Napi::Object exports)
    {
        Napi::Object ugc_list_sort_order = Napi::Object::New(env);
        (ugc_list_sort_order).Set("CreationOrderDesc", Napi::Number::New(env, static_cast<int>(k_EUserUGCListSortOrder_CreationOrderDesc)));
        (ugc_list_sort_order).Set("CreationOrderAsc", Napi::Number::New(env, k_EUserUGCListSortOrder_CreationOrderDesc));
        (ugc_list_sort_order).Set("TitleAsc", Napi::Number::New(env, k_EUserUGCListSortOrder_TitleAsc));
        (ugc_list_sort_order).Set("LastUpdatedDesc", Napi::Number::New(env, k_EUserUGCListSortOrder_LastUpdatedDesc));
        (ugc_list_sort_order).Set("SubscriptionDateDesc", Napi::Number::New(env, k_EUserUGCListSortOrder_SubscriptionDateDesc));
        (ugc_list_sort_order).Set("VoteScoreDesc", Napi::Number::New(env, k_EUserUGCListSortOrder_VoteScoreDesc));
        (ugc_list_sort_order).Set("ForModeration", Napi::Number::New(env, k_EUserUGCListSortOrder_ForModeration));

        (exports).Set("UserUGCListSortOrder", ugc_list_sort_order);
    }

    void sleep(int milliseconds)
    {
#if defined(_WIN32)
        Sleep(milliseconds);
#else
        usleep(milliseconds * 1000);
#endif
    }

    bool WriteFile(const std::string& target_path, char* content, int length)
    {
        std::ofstream fout(target_path.c_str(), std::ios::out | std::ios::binary);
        fout.write(content, length);
        return fout.good();
    }

    std::string GetFileNameFromPath(const std::string& file_path)
    {
        size_t pos = file_path.find_last_of("/\\");
        if (pos == std::string::npos)
            return file_path;
        return file_path.substr(pos + 1);
    }

    bool UpdateFileLastUpdatedTime(const char* file_path, time_t time)
    {
        DebugLog("Setting file last modified time for %s to %d\n", file_path, time);

        utimbuf utime_buf;
        utime_buf.actime = time;
        utime_buf.modtime = time;
        return utime(file_path, &utime_buf) == 0;
    }

    int64 GetFileLastUpdatedTime(const char* file_path)
    {
        struct stat st;
        if (stat(file_path, &st))
            return -1;
        return st.st_mtime;
    }

    std::string uint64ToString(uint64 value)
    {
        std::ostringstream sout;
        sout << value;
        return sout.str();
    }

    uint64 strToUint64(std::string str)
    {
        std::stringstream sin(str);
        uint64 result;
        sin >> result;
        return result;
    }

    void DebugLog(const char* format, ...)
    {
        // va_list arg;
        // FILE *pFile = fopen("greenworks.txt", "a");
        // va_start(arg, format);
        // vfprintf(pFile, format, arg);
        // va_end(arg);
        // fclose(pFile);
    }

} // namespace utils
