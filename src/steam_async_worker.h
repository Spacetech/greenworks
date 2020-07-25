// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_STEAM_ASYNC_WORKER_H_
#define SRC_STEAM_ASYNC_WORKER_H_

#include "nan.h"

namespace greenworks
{
    // Extend NanAsyncWorker with custom error callback supports.
    class SteamAsyncWorker : public Nan::AsyncWorker
    {
      public:
        SteamAsyncWorker(Nan::Callback* success_callback, Nan::Callback* error_callback);

        ~SteamAsyncWorker();

        // Override NanAsyncWorker methods:
        virtual void HandleErrorCallback() override;

      protected:
        Nan::Callback* error_callback_;

        void SetErrorMessageEx(const char* format, ...)
        {
            char buffer[1024];

            va_list args;
            va_start(args, format);
            vsnprintf(buffer, 1024, format, args);

            SetErrorMessage(buffer);

            va_end(args);
        }
    };

    // An abstract SteamAsyncWorker for Steam callback API.
    class SteamCallbackAsyncWorker : public SteamAsyncWorker
    {
      public:
        SteamCallbackAsyncWorker(Nan::Callback* success_callback,
                                 Nan::Callback* error_callback);

        void WaitForCompleted();

      protected:
        bool is_completed_;
    };
} // namespace greenworks

#endif // SRC_STEAM_ASYNC_WORKER_H_
