// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FEEDBACK_MANAGER_H_
#define EMBEDDER_FEEDBACK_MANAGER_H_

#include <feedback.h>

namespace flutter {

class FeedbackManager {
 public:
  static FeedbackManager& GetInstance() {
    static FeedbackManager instance;
    return instance;
  }

  FeedbackManager(const FeedbackManager&) = delete;
  FeedbackManager& operator=(const FeedbackManager&) = delete;

  void PlaySound() { Play(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_GENERAL); }

  void PlayTapSound() { Play(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TAP); }

  void Vibrate() { Play(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP); }

 private:
  explicit FeedbackManager();
  ~FeedbackManager();

  void Play(feedback_type_e type, feedback_pattern_e pattern);

  bool initialized_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_FEEDBACK_MANAGER_H_
