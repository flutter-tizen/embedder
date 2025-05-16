// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_utils.h"

#include <utils_i18n.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

double ComputePixelRatio(int32_t screen_dpi) {
  // The scale factor is computed based on the display DPI and the current
  // profile. A fixed DPI value (72) is used on TVs. See:
  // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
#ifdef TV_PROFILE
  double dpi = 72.0;
#else
  double dpi = static_cast<double>(screen_dpi);
#endif
  double scale_factor = dpi / 90.0 * kProfileFactor;
  return std::max(scale_factor, 1.0);
}

std::vector<LanguageInfo> GetPreferredLanguageInfo() {
  std::vector<LanguageInfo> languages;
  const char* locale;

  i18n_ulocale_set_default(getenv("LANG"));
  int ret = i18n_ulocale_get_default(&locale);
  if (ret != I18N_ERROR_NONE) {
    FT_LOG(Error) << "i18n_ulocale_get_default() failed.";
    return languages;
  }
  std::string locale_string(locale);
  size_t codeset_pos = locale_string.find(".");
  locale_string = locale_string.substr(0, codeset_pos);
  locale = locale_string.c_str();

  LanguageInfo info;
  char buffer[128] = {0};
  int32_t size;

  // The "language" field is required.
  ret = i18n_ulocale_get_language(locale, buffer, sizeof(buffer), &size);
  if (ret != I18N_ERROR_NONE || size == 0) {
    FT_LOG(Error) << "i18n_ulocale_get_language() failed.";
    return languages;
  }
  info.language = std::string(buffer, size);

  // "country", "script", and "variant" are optional.
  size = i18n_ulocale_get_country(locale, buffer, sizeof(buffer), &ret);
  if (ret == I18N_ERROR_NONE && size > 0) {
    info.country = std::string(buffer, size);
  }
  size = i18n_ulocale_get_script(locale, buffer, sizeof(buffer));
  if (size > 0) {
    info.script = std::string(buffer, size);
  }
  size = i18n_ulocale_get_variant(locale, buffer, sizeof(buffer));
  if (size > 0) {
    info.variant = std::string(buffer, size);
  }
  FT_LOG(Info) << "Device language: " << info.language << "_" << info.country;

  languages.push_back(info);
  return languages;
}

}  // namespace flutter
