// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_AUTOFILL_H_
#define EMBEDDER_TIZEN_AUTOFILL_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <autofill.h>

struct AutofillItem {
  autofill_hint_e hint_;
  bool sensitive_data_;
  std::string label_;
  std::string id_;
  std::string value_;
};

class TizenAutofill {
 public:
  static TizenAutofill& GetInstance() {
    static TizenAutofill instance = TizenAutofill();
    return instance;
  }

  void RequestAutofill(std::vector<std::string> hints, std::string id);

  void RegisterAutofillItem(std::string view_id, AutofillItem item);

  void StoreResponseItem(std::unique_ptr<AutofillItem> item) {
    response_items_.push_back(move(item));
  }

  void SetOnPopup(std::function<void()> on_popup) { on_popup_ = on_popup; }

  void SetOnCommit(std::function<void(std::string)> on_commit) {
    on_commit_ = on_commit;
  }

  void OnCommit(std::string str) { on_commit_(str); }

  void OnPopup() { on_popup_(); }

  const std::vector<std::unique_ptr<AutofillItem>>& GetAutofillItems() {
    return response_items_;
  }

 private:
  TizenAutofill();

  ~TizenAutofill();

  void InitailizeAutofill();

  std::optional<autofill_hint_e> ConvertAutofillHint(std::string hint);

  autofill_h autofill_;

  std::vector<std::unique_ptr<AutofillItem>> response_items_;

  std::function<void()> on_popup_;

  std::function<void(std::string)> on_commit_;
};

#endif
