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

  void SetPopupCallback(std::function<void()> callback) {
    popup_callback_ = callback;
  }

  void SetCommitCallback(std::function<void(std::string)> callback) {
    commit_callback_ = callback;
  }

  void OnCommit(std::string str) { commit_callback_(str); }

  void CallPopupCallback() {
    popup_callback_();
    // response_items_.clear();
    //  TODO : reponse_item_ must be cleared when popup is disappeared
  }

  const std::vector<std::unique_ptr<AutofillItem>>& GetAutofillItems() {
    return response_items_;
  }

 private:
  TizenAutofill();

  ~TizenAutofill();

  void InitailizeAutofill();

  // TODO : implement convert flutter hint to tizen hint function
  std::optional<autofill_hint_e> ConvertAutofillHint(std::string hint);

  autofill_h autofill_;

  std::vector<std::unique_ptr<AutofillItem>> response_items_;

  std::function<void()> popup_callback_;

  std::function<void(std::string)> commit_callback_;
};

#endif
