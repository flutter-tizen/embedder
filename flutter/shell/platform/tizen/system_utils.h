// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_SYSTEM_UTILS_H_
#define EMBEDDER_SYSTEM_UTILS_H_

#include <string>
#include <vector>

namespace flutter {

#if defined(MOBILE_PROFILE)
constexpr double kProfileFactor = 0.7;
#elif defined(TV_PROFILE)
constexpr double kProfileFactor = 2.0;
#else
constexpr double kProfileFactor = 1.0;
#endif

// Components of a system language/locale.
struct LanguageInfo {
  std::string language;
  std::string country;
  std::string script;
  std::string variant;
};

// Returns the scale factor calculated based on the display DPI and the current
// profile.
double ComputePixelRatio(int32_t screen_dpi);

// Returns the list of user-preferred locales, in preference order,
// parsed into LanguageInfo structures.
std::vector<LanguageInfo> GetPreferredLanguageInfo();

}  // namespace flutter

#endif  // EMBEDDER_SYSTEM_UTILS_H_
