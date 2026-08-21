// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_EVENT_LOOP_H_
#define EMBEDDER_TIZEN_EVENT_LOOP_H_

#include <glib.h>

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

using CurrentTimeProc = uint64_t (*)();

class TizenEventLoop {
 public:
  using TaskExpiredCallback = std::function<void(const FlutterTask*)>;

  TizenEventLoop(std::thread::id main_thread_id,
                 CurrentTimeProc get_current_time,
                 TaskExpiredCallback on_task_expired);
  ~TizenEventLoop();

  TizenEventLoop(const TizenEventLoop&) = delete;
  TizenEventLoop& operator=(const TizenEventLoop&) = delete;

  bool RunsTasksOnCurrentThread() const;
  void PostTask(FlutterTask flutter_task, uint64_t flutter_target_time_nanos);

 private:
  struct Task {
    uint64_t target_time_nanos;
    uint64_t order;
    FlutterTask task;

    bool operator<(const Task& other) const {
      if (target_time_nanos == other.target_time_nanos) {
        return order > other.order;
      }
      return target_time_nanos > other.target_time_nanos;
    }
  };

  void ExecuteTaskEvents();
  void UpdateSourceReadyTimeLocked(uint64_t now);

  std::thread::id main_thread_id_;
  CurrentTimeProc get_current_time_;
  TaskExpiredCallback on_task_expired_;
  std::mutex task_queue_mutex_;
  std::priority_queue<Task> task_queue_;
  uint64_t task_order_ = 0;
  GSource* task_source_ = nullptr;
  // Detects destruction of this object by a task run in ExecuteTaskEvents.
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_EVENT_LOOP_H_
