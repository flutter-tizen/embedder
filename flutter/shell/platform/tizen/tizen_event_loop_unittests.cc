// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_event_loop.h"

#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace flutter {
namespace testing {

namespace {

using namespace std::chrono_literals;

uint64_t GetCurrentTimeNanos() {
  return static_cast<uint64_t>(g_get_monotonic_time()) * 1000;
}

FlutterTask MakeTask(uint64_t id) {
  FlutterTask task = {};
  task.task = id;
  return task;
}

}  // namespace

class TizenEventLoopTest : public ::testing::Test {
 protected:
  void SetUp() override {
    event_loop_ = std::make_unique<TizenEventLoop>(
        std::this_thread::get_id(), GetCurrentTimeNanos,
        [this](const FlutterTask* task) {
          executed_tasks_.push_back(task->task);
          execution_time_ = GetCurrentTimeNanos();
          execution_thread_ = std::this_thread::get_id();
          if (on_task_) {
            on_task_(*task);
          }
        });
  }

  void TearDown() override { event_loop_.reset(); }

  bool RunUntil(const std::function<bool()>& condition,
                std::chrono::milliseconds timeout = 1s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!condition() && std::chrono::steady_clock::now() < deadline) {
      g_main_context_iteration(nullptr, FALSE);
      std::this_thread::sleep_for(1ms);
    }
    return condition();
  }

  std::unique_ptr<TizenEventLoop> event_loop_;
  std::vector<uint64_t> executed_tasks_;
  uint64_t execution_time_ = 0;
  std::thread::id execution_thread_;
  std::function<void(const FlutterTask&)> on_task_;
};

TEST_F(TizenEventLoopTest, ExecutesPastAndImmediateTasksInPostingOrder) {
  const uint64_t now = GetCurrentTimeNanos();
  event_loop_->PostTask(MakeTask(1), now - 1);
  event_loop_->PostTask(MakeTask(2), now);
  event_loop_->PostTask(MakeTask(3), now);

  ASSERT_TRUE(RunUntil([this] { return executed_tasks_.size() == 3; }));
  EXPECT_EQ(executed_tasks_, (std::vector<uint64_t>{1, 2, 3}));
}

TEST_F(TizenEventLoopTest, ReschedulesEarlierTaskWithoutRunningItEarly) {
  const uint64_t now = GetCurrentTimeNanos();
  const uint64_t earlier_target = now + 20'000'000;
  event_loop_->PostTask(MakeTask(1), now + 500'000'000);
  event_loop_->PostTask(MakeTask(2), earlier_target);

  ASSERT_TRUE(RunUntil([this] { return !executed_tasks_.empty(); }, 250ms));
  EXPECT_EQ(executed_tasks_, (std::vector<uint64_t>{2}));
  EXPECT_GE(execution_time_, earlier_target);
}

TEST_F(TizenEventLoopTest, RunsWorkerThreadTaskOnMainThread) {
  std::thread worker(
      [this] { event_loop_->PostTask(MakeTask(1), GetCurrentTimeNanos()); });
  worker.join();

  ASSERT_TRUE(RunUntil([this] { return !executed_tasks_.empty(); }));
  EXPECT_EQ(execution_thread_, std::this_thread::get_id());
}

TEST_F(TizenEventLoopTest, SupportsReentrantPostTask) {
  on_task_ = [this](const FlutterTask& task) {
    if (task.task == 1) {
      event_loop_->PostTask(MakeTask(2), GetCurrentTimeNanos());
    }
  };
  event_loop_->PostTask(MakeTask(1), GetCurrentTimeNanos());

  ASSERT_TRUE(RunUntil([this] { return executed_tasks_.size() == 2; }));
  EXPECT_EQ(executed_tasks_, (std::vector<uint64_t>{1, 2}));
}

TEST_F(TizenEventLoopTest, DispatchesTasksInsideNestedLoop) {
  bool dispatched_in_nested_loop = false;
  on_task_ = [&](const FlutterTask& task) {
    if (task.task == 1) {
      event_loop_->PostTask(MakeTask(2), GetCurrentTimeNanos());
      dispatched_in_nested_loop =
          RunUntil([this] { return executed_tasks_.size() == 2; }, 250ms);
    }
  };
  event_loop_->PostTask(MakeTask(1), GetCurrentTimeNanos());

  ASSERT_TRUE(RunUntil([this] { return executed_tasks_.size() == 2; }));
  EXPECT_TRUE(dispatched_in_nested_loop);
  EXPECT_EQ(executed_tasks_, (std::vector<uint64_t>{1, 2}));
}

TEST_F(TizenEventLoopTest, SurvivesDestructionFromTask) {
  on_task_ = [this](const FlutterTask&) { event_loop_.reset(); };
  const uint64_t now = GetCurrentTimeNanos();
  event_loop_->PostTask(MakeTask(1), now - 1);
  event_loop_->PostTask(MakeTask(2), now);

  ASSERT_TRUE(RunUntil([this] { return !executed_tasks_.empty(); }));
  EXPECT_EQ(executed_tasks_, (std::vector<uint64_t>{1}));
}

TEST_F(TizenEventLoopTest, RemovesPendingSourceOnDestruction) {
  event_loop_->PostTask(MakeTask(1), GetCurrentTimeNanos());
  event_loop_.reset();

  g_main_context_iteration(nullptr, FALSE);
  EXPECT_TRUE(executed_tasks_.empty());
}

}  // namespace testing
}  // namespace flutter
