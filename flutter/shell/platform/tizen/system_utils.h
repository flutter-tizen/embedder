// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_SYSTEM_UTILS_H_
#define EMBEDDER_SYSTEM_UTILS_H_

#include <string>
#include <vector>

namespace flutter {

// Components of a system language/locale.
struct LanguageInfo {
  std::string language;
  std::string country;
  std::string script;
  std::string variant;
};

// Returns the list of user-preferred locales, in preference order,
// parsed into LanguageInfo structures.
std::vector<LanguageInfo> GetPreferredLanguageInfo();

}  // namespace flutter

#endif  // EMBEDDER_SYSTEM_UTILS_H_
