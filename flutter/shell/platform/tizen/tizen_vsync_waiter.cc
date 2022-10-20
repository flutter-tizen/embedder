// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"

#include <eina_thread_queue.h>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr int kMessageQuit = -1;
constexpr int kMessageRequestVblank = 0;

struct Message {
  Eina_Thread_Queue_Msg head;
  int event;
  intptr_t baton;
};

}  // namespace

TizenVsyncWaiter::TizenVsyncWaiter(FlutterTizenEngine* engine) {
  tdm_client_ = std::make_shared<TdmClient>(engine);

  vblank_thread_ = ecore_thread_feedback_run(RunVblankLoop, nullptr, nullptr,
                                             nullptr, this, EINA_TRUE);
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  tdm_client_->OnEngineStop();

  SendMessage(kMessageQuit, 0);

  if (vblank_thread_) {
    ecore_thread_cancel(vblank_thread_);
    vblank_thread_ = nullptr;
  }
}

void TizenVsyncWaiter::AsyncWaitForVsync(intptr_t baton) {
  SendMessage(kMessageRequestVblank, baton);
}

void TizenVsyncWaiter::SendMessage(int event, intptr_t baton) {
  if (!vblank_thread_ || ecore_thread_check(vblank_thread_)) {
    FT_LOG(Error) << "Invalid vblank thread.";
    return;
  }

  if (!vblank_thread_queue_) {
    FT_LOG(Error) << "Invalid vblank thread queue.";
    return;
  }

  void* ref;
  Message* message = static_cast<Message*>(
      eina_thread_queue_send(vblank_thread_queue_, sizeof(Message), &ref));
  message->event = event;
  message->baton = baton;
  eina_thread_queue_send_done(vblank_thread_queue_, ref);
}

void TizenVsyncWaiter::RunVblankLoop(void* data, Ecore_Thread* thread) {
  auto* self = reinterpret_cast<TizenVsyncWaiter*>(data);

  std::weak_ptr<TdmClient> tdm_client = self->tdm_client_;
  if (!tdm_client.lock()->IsValid()) {
    FT_LOG(Error) << "Invalid tdm_client.";
    ecore_thread_cancel(thread);
    return;
  }

  Eina_Thread_Queue* vblank_thread_queue = eina_thread_queue_new();
  if (!vblank_thread_queue) {
    FT_LOG(Error) << "Invalid vblank thread queue.";
    ecore_thread_cancel(thread);
    return;
  }
  self->vblank_thread_queue_ = vblank_thread_queue;

  while (!ecore_thread_check(thread)) {
    void* ref;
    Message* message = static_cast<Message*>(
        eina_thread_queue_wait(vblank_thread_queue, &ref));
    if (message->event == kMessageQuit) {
      eina_thread_queue_wait_done(vblank_thread_queue, ref);
      break;
    }
    intptr_t baton = message->baton;
    eina_thread_queue_wait_done(vblank_thread_queue, ref);

    if (tdm_client.expired()) {
      break;
    }
    tdm_client.lock()->AwaitVblank(baton);
  }

  if (vblank_thread_queue) {
    eina_thread_queue_free(vblank_thread_queue);
  }
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
  auto* self = reinterpret_cast<TdmClient*>(user_data);
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
