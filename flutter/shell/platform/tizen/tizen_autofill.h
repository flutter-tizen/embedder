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

namespace flutter {

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

  void RequestAutofill(const std::string& id,
                       const std::vector<std::string>& hints);

  void RegisterItem(const std::string& view_id, const AutofillItem& item);

  void StoreResponseItem(std::unique_ptr<AutofillItem> item) {
    response_items_.push_back(move(item));
  }

  void SetConnected(bool connected) { is_connected_ = connected; };

  void SetOnPopup(std::function<void()> on_popup) { on_popup_ = on_popup; }

  void SetOnCommit(std::function<void(const std::string&)> on_commit) {
    on_commit_ = on_commit;
  }

  void OnCommit(const std::string& str) { on_commit_(str); }

  void OnPopup() { on_popup_(); }

  const std::vector<std::unique_ptr<AutofillItem>>& GetResponseItems() {
    return response_items_;
  }

 private:
  TizenAutofill();

  ~TizenAutofill();

  void Initialize();

  bool is_connected_ = false;

  bool is_initialized_ = false;

  autofill_h autofill_ = nullptr;

  std::vector<std::unique_ptr<AutofillItem>> response_items_;

  std::function<void()> on_popup_;

  std::function<void(const std::string&)> on_commit_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_AUTOFILL_H_
