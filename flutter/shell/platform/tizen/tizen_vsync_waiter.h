// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <Ecore.h>
#include <tdm_client.h>

#include <memory>
#include <mutex>

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
  void SendMessage(int event, intptr_t baton);

  static void RunVblankLoop(void* data, Ecore_Thread* thread);

  std::shared_ptr<TdmClient> tdm_client_;
  Ecore_Thread* vblank_thread_ = nullptr;
  Eina_Thread_Queue* vblank_thread_queue_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
