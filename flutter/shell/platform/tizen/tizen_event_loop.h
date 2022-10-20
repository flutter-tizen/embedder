// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_EVENT_LOOP_H_
#define EMBEDDER_TIZEN_EVENT_LOOP_H_

#include <Ecore.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"

namespace flutter {

typedef uint64_t (*CurrentTimeProc)();

class TizenEventLoop {
 public:
  using TaskExpiredCallback = std::function<void(const FlutterTask*)>;

  TizenEventLoop(std::thread::id main_thread_id,
                 CurrentTimeProc get_current_time,
                 TaskExpiredCallback on_task_expired);
  virtual ~TizenEventLoop();

  // Prevent copying.
  TizenEventLoop(const TizenEventLoop&) = delete;
  TizenEventLoop& operator=(const TizenEventLoop&) = delete;

  bool RunsTasksOnCurrentThread() const;

  void ExecuteTaskEvents();

  // Post a Flutter engine tasks to the event loop for delayed execution.
  void PostTask(FlutterTask flutter_task, uint64_t flutter_target_time_nanos);

  virtual void OnTaskExpired() = 0;

 protected:
  using TaskTimePoint = std::chrono::steady_clock::time_point;

  struct Task {
    uint64_t order;
    TaskTimePoint fire_time;
    FlutterTask task;

    struct Comparer {
      bool operator()(const Task& a, const Task& b) {
        if (a.fire_time == b.fire_time) {
          return a.order > b.order;
        }
        return a.fire_time > b.fire_time;
      }
    };
  };

  std::thread::id main_thread_id_;
  CurrentTimeProc get_current_time_;
  TaskExpiredCallback on_task_expired_;
  std::mutex task_queue_mutex_;
  std::priority_queue<Task, std::deque<Task>, Task::Comparer> task_queue_;
  std::vector<Task> expired_tasks_;
  std::mutex expired_tasks_mutex_;
  std::atomic<std::uint64_t> task_order_ = 0;

 private:
  Ecore_Pipe* ecore_pipe_ = nullptr;

  // Returns a TaskTimePoint computed from the given target time from Flutter.
  TaskTimePoint TimePointFromFlutterTime(uint64_t flutter_target_time_nanos);
};

class TizenPlatformEventLoop : public TizenEventLoop {
 public:
  TizenPlatformEventLoop(std::thread::id main_thread_id,
                         CurrentTimeProc get_current_time,
                         TaskExpiredCallback on_task_expired);
  virtual ~TizenPlatformEventLoop();

  virtual void OnTaskExpired() override;
};

class TizenRenderEventLoop : public TizenEventLoop {
 public:
  TizenRenderEventLoop(std::thread::id main_thread_id,
                       CurrentTimeProc get_current_time,
                       TaskExpiredCallback on_task_expired,
                       TizenRenderer* renderer);
  virtual ~TizenRenderEventLoop();

  virtual void OnTaskExpired() override;

 private:
  TizenRenderer* renderer_ = nullptr;
  std::atomic_bool has_pending_renderer_callback_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_EVENT_LOOP_H_
