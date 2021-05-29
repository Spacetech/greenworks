// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_STEAM_ASYNC_WORKER_H_
#define SRC_STEAM_ASYNC_WORKER_H_

#include "napi.h"
#include "uv.h"
#include <stdarg.h>

// Extend NanAsyncWorker with custom error callback supports.
class SteamAsyncWorker : public Napi::AsyncWorker
{
  public:
    SteamAsyncWorker(Napi::Function &callback);
    ~SteamAsyncWorker();

    virtual void Execute() = 0;
    // virtual void OnOK() = 0;

  protected:
    void SetErrorEx(const char *format, ...)
    {
        char buffer[1024];

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 1024, format, args);

        SetError(buffer);

        va_end(args);
    }
};

// An abstract SteamAsyncWorker for Steam callback API.
class SteamCallbackAsyncWorker : public SteamAsyncWorker
{
  public:
    SteamCallbackAsyncWorker(Napi::Function &callback);
    ~SteamCallbackAsyncWorker();

    virtual void Execute() = 0;
    // virtual void OnOK() = 0;

    void WaitForCompleted();

  protected:
    bool is_completed_;
};

#endif // SRC_STEAM_ASYNC_WORKER_H_
