// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "greenworks_utils.h"

#include <fstream>
#include <sstream>

#include "nan.h"
#include "steam/steam_api.h"

#include <stdio.h>

#if defined(_WIN32)
#include <sys/utime.h>
#include <windows.h>
#else
#include <unistd.h>
#include <utime.h>
#include <stdarg.h>
#endif

namespace utils
{
	void InitUgcMatchingTypes(v8::Local<v8::Object> exports)
	{
		v8::Local<v8::Object> ugc_matching_type = Nan::New<v8::Object>();
		Nan::Set(ugc_matching_type, Nan::New("Items").ToLocalChecked(), Nan::New(k_EUGCMatchingUGCType_Items));
		Nan::Set(ugc_matching_type, Nan::New("ItemsMtx").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Items_Mtx));
		Nan::Set(ugc_matching_type, Nan::New("ItemsReadyToUse").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Items_ReadyToUse));
		Nan::Set(ugc_matching_type, Nan::New("Collections").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Collections));
		Nan::Set(ugc_matching_type, Nan::New("Artwork").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Artwork));
		Nan::Set(ugc_matching_type, Nan::New("Videos").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Videos));
		Nan::Set(ugc_matching_type, Nan::New("Screenshots").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_Screenshots));
		Nan::Set(ugc_matching_type, Nan::New("AllGuides").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_AllGuides));
		Nan::Set(ugc_matching_type, Nan::New("WebGuides").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_WebGuides));
		Nan::Set(ugc_matching_type, Nan::New("IntegratedGuides").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_IntegratedGuides));
		Nan::Set(ugc_matching_type, Nan::New("UsableInGame").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_UsableInGame));
		Nan::Set(ugc_matching_type, Nan::New("ControllerBindings").ToLocalChecked(),
		                       Nan::New(k_EUGCMatchingUGCType_ControllerBindings));

		Nan::Persistent<v8::Object> constructor;
		constructor.Reset(ugc_matching_type);

		Nan::Set(exports, Nan::New("UGCMatchingType").ToLocalChecked(), ugc_matching_type);
	}

	void InitUgcQueryTypes(v8::Local<v8::Object> exports)
	{
		v8::Local<v8::Object> ugc_query_type = Nan::New<v8::Object>();
		Nan::Set(ugc_query_type, Nan::New("RankedByVote").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByVote));
		Nan::Set(ugc_query_type, Nan::New("RankedByPublicationDate").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByPublicationDate));
		Nan::Set(ugc_query_type, Nan::New("AcceptedForGameRankedByAcceptanceDate").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_AcceptedForGameRankedByAcceptanceDate));
		Nan::Set(ugc_query_type, Nan::New("RankedByTrend").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByTrend));
		Nan::Set(ugc_query_type, Nan::New("FavoritedByFriendsRankedByPublicationDate").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_FavoritedByFriendsRankedByPublicationDate));
		Nan::Set(ugc_query_type, Nan::New("CreatedByFriendsRankedByPublicationDate").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_CreatedByFriendsRankedByPublicationDate));
		Nan::Set(ugc_query_type, Nan::New("RankedByNumTimesReported").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByNumTimesReported));
		Nan::Set(ugc_query_type, Nan::New("CreatedByFollowedUsersRankedByPublicationDate").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_CreatedByFollowedUsersRankedByPublicationDate));
		Nan::Set(ugc_query_type, Nan::New("NotYetRated").ToLocalChecked(), Nan::New(k_EUGCQuery_NotYetRated));
		Nan::Set(ugc_query_type, Nan::New("RankedByTotalVotesAsc").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByTotalVotesAsc));
		Nan::Set(ugc_query_type, Nan::New("RankedByVotesUp").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByVotesUp));
		Nan::Set(ugc_query_type, Nan::New("RankedByTextSearch").ToLocalChecked(),
		                    Nan::New(k_EUGCQuery_RankedByTextSearch));

		Nan::Persistent<v8::Object> constructor;
		constructor.Reset(ugc_query_type);

		Nan::Set(exports, Nan::New("UGCQueryType").ToLocalChecked(), ugc_query_type);
	}

	void InitUserUgcList(v8::Local<v8::Object> exports)
	{
		v8::Local<v8::Object> ugc_list = Nan::New<v8::Object>();
		Nan::Set(ugc_list, Nan::New("Published").ToLocalChecked(), Nan::New(k_EUserUGCList_Published));
		Nan::Set(ugc_list, Nan::New("VotedOn").ToLocalChecked(), Nan::New(k_EUserUGCList_VotedOn));
		Nan::Set(ugc_list, Nan::New("VotedUp").ToLocalChecked(), Nan::New(k_EUserUGCList_VotedUp));
		Nan::Set(ugc_list, Nan::New("VotedDown").ToLocalChecked(), Nan::New(k_EUserUGCList_VotedDown));
		Nan::Set(ugc_list, Nan::New("WillVoteLater").ToLocalChecked(), Nan::New(k_EUserUGCList_WillVoteLater));
		Nan::Set(ugc_list, Nan::New("Favorited").ToLocalChecked(), Nan::New(k_EUserUGCList_Favorited));
		Nan::Set(ugc_list, Nan::New("Subscribed").ToLocalChecked(), Nan::New(k_EUserUGCList_Subscribed));
		Nan::Set(ugc_list, Nan::New("UsedOrPlayer").ToLocalChecked(), Nan::New(k_EUserUGCList_UsedOrPlayed));
		Nan::Set(ugc_list, Nan::New("Followed").ToLocalChecked(), Nan::New(k_EUserUGCList_Followed));

		Nan::Persistent<v8::Object> constructor;
		constructor.Reset(ugc_list);

		Nan::Set(exports, Nan::New("UserUGCList").ToLocalChecked(), ugc_list);
	}

	void InitUserUgcListSortOrder(v8::Local<v8::Object> exports)
	{
		v8::Local<v8::Object> ugc_list_sort_order = Nan::New<v8::Object>();
		Nan::Set(ugc_list_sort_order, Nan::New("CreationOrderDesc").ToLocalChecked(),
		                         Nan::New(static_cast<int>(k_EUserUGCListSortOrder_CreationOrderDesc)));
		Nan::Set(ugc_list_sort_order, Nan::New("CreationOrderAsc").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_CreationOrderDesc));
		Nan::Set(ugc_list_sort_order, Nan::New("TitleAsc").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_TitleAsc));
		Nan::Set(ugc_list_sort_order, Nan::New("LastUpdatedDesc").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_LastUpdatedDesc));
		Nan::Set(ugc_list_sort_order, Nan::New("SubscriptionDateDesc").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_SubscriptionDateDesc));
		Nan::Set(ugc_list_sort_order, Nan::New("VoteScoreDesc").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_VoteScoreDesc));
		Nan::Set(ugc_list_sort_order, Nan::New("ForModeration").ToLocalChecked(),
		                         Nan::New(k_EUserUGCListSortOrder_ForModeration));

		Nan::Persistent<v8::Object> constructor;
		constructor.Reset(ugc_list_sort_order);

		Nan::Set(exports, Nan::New("UserUGCListSortOrder").ToLocalChecked(), ugc_list_sort_order);
	}

	void sleep(int milliseconds)
	{
#if defined(_WIN32)
		Sleep(milliseconds);
#else
  usleep(milliseconds*1000);
#endif
	}

	bool WriteFile(const std::string& target_path, char* content, int length)
	{
		std::ofstream fout(target_path.c_str(), std::ios::out|std::ios::binary);
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

