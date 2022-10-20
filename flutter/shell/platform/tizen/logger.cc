// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "logger.h"

#include <dlog.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>

namespace {

constexpr char kLogTag[] = "ConsoleMessage";

std::string GetLevelName(int level) {
  switch (level) {
    case flutter::kLogLevelDebug:
      return "D";
    default:
      return "I";
    case flutter::kLogLevelWarn:
      return "W";
    case flutter::kLogLevelError:
      return "E";
    case flutter::kLogLevelFatal:
      return "F";
  }
}

log_priority LevelToPriority(int level) {
  switch (level) {
    case flutter::kLogLevelDebug:
      return DLOG_DEBUG;
    default:
      return DLOG_INFO;
    case flutter::kLogLevelWarn:
      return DLOG_WARN;
    case flutter::kLogLevelError:
      return DLOG_ERROR;
    case flutter::kLogLevelFatal:
      return DLOG_FATAL;
  }
}

}  // namespace

namespace flutter {

void* Logger::Redirect(void* arg) {
  int* pipe = static_cast<int*>(arg);
  ssize_t size;
  char buffer[4096];

  while ((size = read(pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
    buffer[size] = 0;
    Print(pipe == stdout_pipe_ ? kLogLevelInfo : kLogLevelError,
          std::string(buffer));
  }
  return nullptr;
}

void* Logger::Forward(void* arg) {
  if (logging_port_ == 0) {
    return nullptr;
  }

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    FT_LOG(Error) << "Error opening a socket.";
    return nullptr;
  }
  int optval = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(logging_port_);
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    FT_LOG(Error) << "Error on binding: " << strerror(errno);
    close(fd);
    return nullptr;
  }
  if (listen(fd, 1) < 0) {
    FT_LOG(Error) << "Error listening to incoming connection: "
                  << strerror(errno);
    close(fd);
    return nullptr;
  }

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int new_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
  if (new_fd < 0) {
    FT_LOG(Error) << "Error on accept: " << strerror(errno);
    close(fd);
    return nullptr;
  }
  FT_LOG(Info) << "Connection accepted on port: " << logging_port_;
  close(fd);

  if (write(new_fd, "ACCEPTED", 8) < 0) {
    FT_LOG(Error) << "Error writing to socket: " << strerror(errno);
    close(new_fd);
    return nullptr;
  }
  ssize_t size;
  while ((size = splice(logging_pipe_[0], nullptr, new_fd, nullptr, 4096,
                        SPLICE_F_MOVE)) > 0) {
  }
  if (size < 0) {
    FT_LOG(Error) << "Error writing to socket: " << strerror(errno);
  }
  close(new_fd);
  return nullptr;
}

void Logger::Start() {
  if (is_running_) {
    FT_LOG(Info) << "The threads have already started.";
    return;
  }
  if (pipe(stdout_pipe_) < 0 || pipe(stderr_pipe_) < 0 ||
      pipe(logging_pipe_) < 0) {
    FT_LOG(Error) << "Failed to create pipes.";
    return;
  }
  is_running_ = true;

  if (dup2(stdout_pipe_[1], 1) < 0 || dup2(stderr_pipe_[1], 2) < 0) {
    FT_LOG(Error) << "Failed to duplicate file descriptors.";
    return;
  }
  if (pthread_create(&stdout_thread_, 0, Redirect, stdout_pipe_) != 0 ||
      pthread_create(&stderr_thread_, 0, Redirect, stderr_pipe_) != 0 ||
      pthread_create(&logging_thread_, 0, Forward, nullptr) != 0) {
    FT_LOG(Error) << "Failed to create threads.";
    return;
  }
  if (pthread_detach(stdout_thread_) != 0 ||
      pthread_detach(stderr_thread_) != 0 ||
      pthread_detach(logging_thread_) != 0) {
    FT_LOG(Warn) << "Failed to detach threads.";
  }
}

void Logger::Stop() {
  if (!is_running_) {
    return;
  }
  is_running_ = false;

  close(stdout_pipe_[0]);
  close(stdout_pipe_[1]);
  close(stderr_pipe_[0]);
  close(stderr_pipe_[1]);
  close(logging_pipe_[0]);
  close(logging_pipe_[1]);
}

void Logger::Print(int level, const std::string& message) {
  // Write the message to the logging pipe so that it can be forwarded to the
  // connected host.
  if (logging_port_ > 0 && level >= kLogLevelInfo) {
    std::string prefix = "[" + GetLevelName(level) + "] ";
    std::string formatted = prefix + message + "\n";
    write(logging_pipe_[1], formatted.c_str(), formatted.size());
  }

  // Also print to dlog for convenience.
  log_priority priority = LevelToPriority(level);
#ifdef TV_PROFILE
  // dlog_print(..) which is an alias of __dlog_print(LOG_ID_APPS, ..) is not
  // valid on TV devices.
  __dlog_print(LOG_ID_MAIN, priority, kLogTag, "%s", message.c_str());
#else
  dlog_print(priority, kLogTag, "%s", message.c_str());
#endif
}

LogMessage::LogMessage(int level,
                       const char* file,
                       const char* function,
                       int line)
    : level_(level), file_(file), function_(function), line_(line) {
  stream_ << file_ << ": " << function_ << "(" << line_ << ") > ";
}

LogMessage::~LogMessage() {
  if (level_ < Logger::GetLoggingLevel()) {
    return;
  }
  Logger::Print(level_, stream_.str());

  if (level_ >= kLogLevelFatal) {
    abort();
  }
}

}  // namespace flutter
