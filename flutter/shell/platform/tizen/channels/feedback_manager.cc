// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "feedback_manager.h"

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

FeedbackManager::FeedbackManager() {
  int ret = feedback_initialize();
  if (ret != FEEDBACK_ERROR_NONE) {
    FT_LOG(Error) << "feedback_initialize() failed with error: "
                  << get_error_message(ret);
    return;
  }
  initialized_ = true;
}

FeedbackManager::~FeedbackManager() {
  if (initialized_) {
    feedback_deinitialize();
  }
}

void FeedbackManager::Play(feedback_type_e type, feedback_pattern_e pattern) {
  if (!initialized_) {
    return;
  }
  int ret = feedback_play_type(type, pattern);
  if (ret == FEEDBACK_ERROR_PERMISSION_DENIED) {
    FT_LOG(Error)
        << "Permission denied. Add the http://tizen.org/privilege/haptic "
           "privilege to tizen-manifest.xml to use haptic feedbacks.";
  } else if (ret != FEEDBACK_ERROR_NONE) {
    FT_LOG(Error) << "feedback_play_type() failed with error: "
                  << get_error_message(ret);
  }
}

}  // namespace flutter
