// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LOGGER_H_
#define EMBEDDER_LOGGER_H_

#include <pthread.h>

#include <cassert>
#include <sstream>
#include <string>

namespace flutter {

constexpr int kLogLevelDebug = 0;
constexpr int kLogLevelInfo = 1;
constexpr int kLogLevelWarn = 2;
constexpr int kLogLevelError = 3;
constexpr int kLogLevelFatal = 4;

class Logger {
 public:
  static void Start();
  static void Stop();

  static int GetLoggingLevel() { return logging_level_; }
  static void SetLoggingLevel(int level) { logging_level_ = level; }

  static void SetLoggingPort(int32_t port) { logging_port_ = port; }

  static void Print(int level, const std::string& message);

 private:
  explicit Logger();

  static void* Redirect(void* arg);
  static void* Forward(void* arg);

  static inline bool is_running_ = false;
  static inline int stdout_pipe_[2];
  static inline int stderr_pipe_[2];
  static inline int logging_pipe_[2];
  static inline pthread_t stdout_thread_;
  static inline pthread_t stderr_thread_;
  static inline pthread_t logging_thread_;

  static inline int logging_level_ = kLogLevelError;
  static inline int32_t logging_port_ = 0;
};

class LogMessage {
 public:
  LogMessage(int level, const char* file, const char* function, int line);
  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  std::ostringstream stream_;
  const int level_;
  const char* file_;
  const char* function_;
  const int line_;
};

}  // namespace flutter

#ifndef __MODULE__
#define __MODULE__ strrchr("/" __FILE__, '/') + 1
#endif

#define FT_LOG(level)                                                  \
  flutter::LogMessage(flutter::kLogLevel##level, __MODULE__, __func__, \
                      __LINE__)                                        \
      .stream()

#if defined(NDEBUG)
#define FT_ASSERT(assertion) ((void)0)
#define FT_ASSERT_NOT_REACHED() ((void)0)
#define FT_ASSERT_STATIC(assertion, reason)
#else
#define FT_ASSERT(assertion) assert(assertion)
#define FT_ASSERT_NOT_REACHED() assert(false)
#define FT_ASSERT_STATIC(assertion, reason) static_assert(assertion, reason)
#endif

#define FT_UNIMPLEMENTED() FT_LOG(Warn) << "UNIMPLEMENTED!"

#endif  // EMBEDDER_LOGGER_H_
