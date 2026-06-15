// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_event_loop.h"

#include <unistd.h>

#include <utility>

namespace flutter {

TizenEventLoop::TizenEventLoop(std::thread::id main_thread_id,
                               CurrentTimeProc get_current_time,
                               TaskExpiredCallback on_task_expired)
    : main_thread_id_(main_thread_id),
      get_current_time_(get_current_time),
      on_task_expired_(std::move(on_task_expired)) {
  if (pipe(pipe_fds_) == 0) {
    pipe_channel_ = g_io_channel_unix_new(pipe_fds_[0]);
    pipe_watch_id_ = g_io_add_watch(
        pipe_channel_, G_IO_IN,
        [](GIOChannel* source, GIOCondition condition,
           gpointer data) -> gboolean {
          auto* self = static_cast<TizenEventLoop*>(data);
          char buf[64];
          read(self->pipe_fds_[0], buf, sizeof(buf));
          self->ExecuteTaskEvents();
          return G_SOURCE_CONTINUE;
        },
        this);
  }
}

TizenEventLoop::~TizenEventLoop() {
  if (pipe_watch_id_ > 0) {
    g_source_remove(pipe_watch_id_);
  }
  if (pipe_channel_) {
    g_io_channel_unref(pipe_channel_);
  }
  if (pipe_fds_[0] >= 0) {
    close(pipe_fds_[0]);
  }
  if (pipe_fds_[1] >= 0) {
    close(pipe_fds_[1]);
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
    int* pipe_write_fd = new int(pipe_fds_[1]);
    g_timeout_add(
        static_cast<guint>(flutter_duration / 1000000.0),
        [](gpointer data) -> gboolean {
          int* fd = static_cast<int*>(data);
          if (*fd >= 0) {
            char c = 1;
            write(*fd, &c, 1);
          }
          delete fd;
          return G_SOURCE_REMOVE;
        },
        pipe_write_fd);
  } else {
    if (pipe_fds_[1] >= 0) {
      char c = 1;
      write(pipe_fds_[1], &c, 1);
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
  std::vector<Task> local_expired_tasks;
  {
    std::lock_guard<std::mutex> lock(expired_tasks_mutex_);
    local_expired_tasks = std::move(expired_tasks_);
  }

  for (const Task& task : local_expired_tasks) {
    on_task_expired_(&task.task);
  }
}

}  // namespace flutter
