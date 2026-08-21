// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_event_loop.h"

#include <utility>
#include <vector>

namespace flutter {

namespace {

GSourceFuncs kTaskSourceFuncs = {
    nullptr,
    nullptr,
    [](GSource*, GSourceFunc callback, gpointer user_data) -> gboolean {
      return callback(user_data);
    },
    nullptr,
    nullptr,
    nullptr,
};

}  // namespace

TizenEventLoop::TizenEventLoop(std::thread::id main_thread_id,
                               CurrentTimeProc get_current_time,
                               TaskExpiredCallback on_task_expired)
    : main_thread_id_(main_thread_id),
      get_current_time_(get_current_time),
      on_task_expired_(std::move(on_task_expired)),
      task_source_(g_source_new(&kTaskSourceFuncs, sizeof(GSource))) {
  g_source_set_callback(
      task_source_,
      [](gpointer data) -> gboolean {
        static_cast<TizenEventLoop*>(data)->ExecuteTaskEvents();
        return G_SOURCE_CONTINUE;
      },
      this, nullptr);

  g_source_set_can_recurse(task_source_, TRUE);
  g_source_attach(task_source_, nullptr);
}

TizenEventLoop::~TizenEventLoop() {
  *alive_ = false;
  g_source_destroy(task_source_);
  g_source_unref(task_source_);
}

bool TizenEventLoop::RunsTasksOnCurrentThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void TizenEventLoop::PostTask(FlutterTask flutter_task,
                              uint64_t flutter_target_time_nanos) {
  std::lock_guard<std::mutex> lock(task_queue_mutex_);
  const bool should_reschedule =
      task_queue_.empty() ||
      flutter_target_time_nanos < task_queue_.top().target_time_nanos;
  task_queue_.push({flutter_target_time_nanos, task_order_++, flutter_task});
  if (should_reschedule) {
    UpdateSourceReadyTimeLocked(get_current_time_());
  }
}

void TizenEventLoop::ExecuteTaskEvents() {
  std::vector<FlutterTask> expired_tasks;
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    const uint64_t now = get_current_time_();
    while (!task_queue_.empty() && task_queue_.top().target_time_nanos <= now) {
      expired_tasks.push_back(task_queue_.top().task);
      task_queue_.pop();
    }
    UpdateSourceReadyTimeLocked(now);
  }

  std::shared_ptr<bool> alive = alive_;
  for (const FlutterTask& task : expired_tasks) {
    on_task_expired_(&task);
    if (!*alive) {
      return;
    }
  }
}

void TizenEventLoop::UpdateSourceReadyTimeLocked(uint64_t now) {
  if (task_queue_.empty()) {
    g_source_set_ready_time(task_source_, -1);
    return;
  }

  const uint64_t target_time_nanos = task_queue_.top().target_time_nanos;
  if (target_time_nanos <= now) {
    g_source_set_ready_time(task_source_, 0);
    return;
  }

  const uint64_t delay_nanos = target_time_nanos - now;
  const uint64_t delay_micros = delay_nanos / 1000 + (delay_nanos % 1000 != 0);
  g_source_set_ready_time(
      task_source_, g_get_monotonic_time() + static_cast<gint64>(delay_micros));
}

}  // namespace flutter
