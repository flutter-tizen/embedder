// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_event_loop.h"

#include <utility>

#include "flutter/shell/platform/tizen/tizen_renderer_evas_gl.h"

namespace flutter {

TizenEventLoop::TizenEventLoop(std::thread::id main_thread_id,
                               CurrentTimeProc get_current_time,
                               TaskExpiredCallback on_task_expired)
    : main_thread_id_(main_thread_id),
      get_current_time_(get_current_time),
      on_task_expired_(std::move(on_task_expired)) {
  ecore_pipe_ = ecore_pipe_add(
      [](void* data, void* buffer, unsigned int nbyte) -> void {
        auto* self = reinterpret_cast<TizenEventLoop*>(data);
        self->ExecuteTaskEvents();
      },
      this);
}

TizenEventLoop::~TizenEventLoop() {
  if (ecore_pipe_) {
    ecore_pipe_del(ecore_pipe_);
  }
}

bool TizenEventLoop::RunsTasksOnCurrentThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void TizenEventLoop::ExecuteTaskEvents() {
  const TaskTimePoint now = TaskTimePoint::clock::now();
  {
    std::lock_guard<std::mutex> lock1(task_queue_mutex_);
    std::lock_guard<std::mutex> lock2(expired_tasks_mutex_);
    while (!task_queue_.empty()) {
      const Task& top = task_queue_.top();

      if (top.fire_time > now) {
        break;
      }

      expired_tasks_.push_back(task_queue_.top());
      task_queue_.pop();
    }
  }
  OnTaskExpired();
}

TizenEventLoop::TaskTimePoint TizenEventLoop::TimePointFromFlutterTime(
    uint64_t flutter_target_time_nanos) {
  const TaskTimePoint now = TaskTimePoint::clock::now();
  const uint64_t flutter_duration =
      flutter_target_time_nanos - get_current_time_();
  return now + std::chrono::nanoseconds(flutter_duration);
}

void TizenEventLoop::PostTask(FlutterTask flutter_task,
                              uint64_t flutter_target_time_nanos) {
  Task task;
  task.order = ++task_order_;
  task.fire_time = TimePointFromFlutterTime(flutter_target_time_nanos);
  task.task = flutter_task;
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    task_queue_.push(task);
  }

  const double flutter_duration =
      static_cast<double>(flutter_target_time_nanos) - get_current_time_();
  if (flutter_duration > 0) {
    ecore_timer_add(
        flutter_duration / 1000000000.0,
        [](void* data) -> Eina_Bool {
          auto* self = reinterpret_cast<TizenEventLoop*>(data);
          if (self->ecore_pipe_) {
            ecore_pipe_write(self->ecore_pipe_, nullptr, 0);
          }
          return ECORE_CALLBACK_CANCEL;
        },
        this);
  } else {
    if (ecore_pipe_) {
      ecore_pipe_write(ecore_pipe_, nullptr, 0);
    }
  }
}

TizenPlatformEventLoop::TizenPlatformEventLoop(
    std::thread::id main_thread_id,
    CurrentTimeProc get_current_time,
    TaskExpiredCallback on_task_expired)
    : TizenEventLoop(main_thread_id, get_current_time, on_task_expired) {}

TizenPlatformEventLoop::~TizenPlatformEventLoop() {}

void TizenPlatformEventLoop::OnTaskExpired() {
  for (const Task& task : expired_tasks_) {
    on_task_expired_(&task.task);
  }
  expired_tasks_.clear();
}

TizenRenderEventLoop::TizenRenderEventLoop(std::thread::id main_thread_id,
                                           CurrentTimeProc get_current_time,
                                           TaskExpiredCallback on_task_expired,
                                           TizenRenderer* renderer)
    : TizenEventLoop(main_thread_id, get_current_time, on_task_expired),
      renderer_(renderer) {
  static_cast<TizenRendererEvasGL*>(renderer_)->SetOnPixelsDirty([this]() {
    {
      std::lock_guard<std::mutex> lock(expired_tasks_mutex_);
      for (const Task& task : expired_tasks_) {
        on_task_expired_(&task.task);
      }
      expired_tasks_.clear();
    }
    has_pending_renderer_callback_ = false;
  });
}

TizenRenderEventLoop::~TizenRenderEventLoop() {}

void TizenRenderEventLoop::OnTaskExpired() {
  std::lock_guard<std::mutex> lock(expired_tasks_mutex_);
  if (!has_pending_renderer_callback_ && !expired_tasks_.empty()) {
    static_cast<TizenRendererEvasGL*>(renderer_)->MarkPixelsDirty();
    has_pending_renderer_callback_ = true;
  }
}

}  // namespace flutter
