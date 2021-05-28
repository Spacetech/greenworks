// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "steam_async_worker.h"

#include "v8.h"

#include "greenworks_utils.h"

namespace greenworks
{
    SteamAsyncWorker::SteamAsyncWorker(Napi::Function& callback) : Napi::AsyncWorker(callback)
    {
    }

    SteamAsyncWorker::~SteamAsyncWorker()
    {
    }

    SteamCallbackAsyncWorker::SteamCallbackAsyncWorker(Napi::Function& callback) : SteamAsyncWorker(callback), is_completed_(false)
    {
    }

    SteamCallbackAsyncWorker::~SteamCallbackAsyncWorker()
    {
    }

    void SteamCallbackAsyncWorker::WaitForCompleted()
    {
        while (!is_completed_)
        {
            utils::sleep(100);
        }
    }

} // namespace greenworks
