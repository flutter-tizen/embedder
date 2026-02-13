// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <tdm_client.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class FlutterTizenEngine;

class TdmClient {
 public:
  TdmClient(FlutterTizenEngine* engine);
  virtual ~TdmClient();

  bool IsValid();
  void OnEngineStop();
  void AwaitVblank(intptr_t baton);

 private:
  static void VblankCallback(tdm_client_vblank* vblank,
                             tdm_error error,
                             unsigned int sequence,
                             unsigned int tv_sec,
                             unsigned int tv_usec,
                             void* user_data);

  FlutterTizenEngine* engine_ = nullptr;
  std::mutex engine_mutex_;

  tdm_client* client_ = nullptr;
  tdm_client_output* output_ = nullptr;
  tdm_client_vblank* vblank_ = nullptr;

  intptr_t baton_ = 0;
};

class TizenVsyncWaiter {
 public:
  TizenVsyncWaiter(FlutterTizenEngine* engine);
  virtual ~TizenVsyncWaiter();
  void AsyncWaitForVsync(intptr_t baton);

 private:
  class MessageLoop {
   public:
    using Task = std::function<void()>;

    MessageLoop() : quit_(false) {
      loop_thread_ = std::thread(&MessageLoop::Run, this);
    }

    ~MessageLoop() { Quit(); }

    void PostTask(Task task) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (quit_) {
          return;
        }
        tasks_.push(std::move(task));
      }
      cond_.notify_one();
    }

    void Quit() {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (quit_) {
          return;
        }
        quit_ = true;
      }
      cond_.notify_all();

      if (loop_thread_.joinable()) {
        loop_thread_.join();
      }
    }

   private:
    void Run() {
      while (true) {
        Task task;
        {
          std::unique_lock<std::mutex> lock(mutex_);
          cond_.wait(lock, [this] { return !tasks_.empty() || quit_; });
          if (quit_) {
            break;
          }

          task = std::move(tasks_.front());
          tasks_.pop();
        }

        if (task) {
          task();
        }
      }
    }

    std::queue<Task> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> quit_;
    std::thread loop_thread_;
  };

  std::shared_ptr<TdmClient> tdm_client_;
  std::unique_ptr<MessageLoop> message_loop_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
