// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_GREENWORKS_UTILS_H_
#define SRC_GREENWORKS_UTILS_H_

#include <string>

// ReSharper disable once CppUnusedIncludeDirective
#include "napi.h"
#include "steam/steamtypes.h"
#include "uv.h"
#include "v8.h"

// 5mb
#define FILE_BUFFER_SIZE 5000000

namespace utils
{
void InitUgcQueryTypes(Napi::Env env, Napi::Object exports);

void InitUgcMatchingTypes(Napi::Env env, Napi::Object exports);

void InitUserUgcListSortOrder(Napi::Env env, Napi::Object exports);

void InitUserUgcList(Napi::Env env, Napi::Object exports);

void sleep(int milliseconds);

bool WriteFile(const std::string &target_path, char *content, int length);

std::string GetFileNameFromPath(const std::string &file_path);

bool UpdateFileLastUpdatedTime(const char *file_path, time_t time);

int64 GetFileLastUpdatedTime(const char *file_path);

std::string uint64ToString(uint64 value);

uint64 strToUint64(std::string);

void DebugLog(const char *format, ...);

} // namespace utils

#endif // SRC_GREENWORKS_UTILS_H_
