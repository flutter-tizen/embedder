// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

MessageLoop::MessageLoop() : quit_(false) {
  Init();
}
MessageLoop::~MessageLoop() {
  Quit();
}

void MessageLoop::Init() {
  quit_ = false;
  loop_thread_ = std::thread(&MessageLoop::Run, this);
  FT_LOG(Info) << "[MessageLoop] Thread started.";
}

void MessageLoop::Quit() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (quit_)
      return;
    quit_ = true;
  }
  cond_.notify_all();

  if (loop_thread_.joinable()) {
    loop_thread_.join();
    FT_LOG(Info) << "[MessageLoop] Thread joined and quit.";
  }
}

void MessageLoop::PostTask(Task task) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(std::move(task));
  }
  cond_.notify_one();
}

void MessageLoop::Run() {
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

TizenVsyncWaiter::TizenVsyncWaiter(FlutterTizenEngine* engine) {
  tdm_client_ = std::make_shared<TdmClient>(engine);
  message_loop_ = std::make_unique<MessageLoop>();
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  tdm_client_->OnEngineStop();
}

void TizenVsyncWaiter::AsyncWaitForVsync(intptr_t baton) {
  std::weak_ptr<TdmClient> tdm_client = tdm_client_;
  message_loop_->PostTask([tdm_client_weak = std::move(tdm_client), baton]() {
    if (auto tdm_client = tdm_client_weak.lock()) {
      tdm_client->AwaitVblank(baton);
    } else {
      FT_LOG(Error) << "tdm client is null, task cancelled";
    }
  });
}

TdmClient::TdmClient(FlutterTizenEngine* engine) {
  tdm_error ret;
  client_ = tdm_client_create(&ret);
  if (ret != TDM_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create a TDM client.";
    return;
  }

  output_ = tdm_client_get_output(client_, const_cast<char*>("default"), &ret);
  if (ret != TDM_ERROR_NONE) {
    FT_LOG(Error) << "Could not obtain the default client output.";
    return;
  }

  vblank_ = tdm_client_output_create_vblank(output_, &ret);
  if (ret != TDM_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create a vblank object.";
    return;
  }
  tdm_client_vblank_set_enable_fake(vblank_, 1);
  engine_ = engine;
}

TdmClient::~TdmClient() {
  if (vblank_) {
    tdm_client_vblank_destroy(vblank_);
    vblank_ = nullptr;
  }
  output_ = nullptr;
  if (client_) {
    tdm_client_destroy(client_);
    client_ = nullptr;
  }
}

bool TdmClient::IsValid() {
  return vblank_ && client_;
}

void TdmClient::OnEngineStop() {
  std::lock_guard<std::mutex> lock(engine_mutex_);
  engine_ = nullptr;
}

void TdmClient::AwaitVblank(intptr_t baton) {
  baton_ = baton;
  tdm_error ret = tdm_client_vblank_wait(vblank_, 1, VblankCallback, this);
  if (ret != TDM_ERROR_NONE) {
    FT_LOG(Error) << "tdm_client_vblank_wait failed with error: " << ret;
    return;
  }
  tdm_client_handle_events(client_);
}

void TdmClient::VblankCallback(tdm_client_vblank* vblank,
                               tdm_error error,
                               unsigned int sequence,
                               unsigned int tv_sec,
                               unsigned int tv_usec,
                               void* user_data) {
  auto* self = static_cast<TdmClient*>(user_data);
  FT_ASSERT(self != nullptr);

  std::lock_guard<std::mutex> lock(self->engine_mutex_);
  if (self->engine_) {
    uint64_t frame_start_time_nanos = tv_sec * 1e9 + tv_usec * 1e3;
    uint64_t frame_target_time_nanos = frame_start_time_nanos + 16.6 * 1e6;
    self->engine_->OnVsync(self->baton_, frame_start_time_nanos,
                           frame_target_time_nanos);
  }
}

}  // namespace flutter
