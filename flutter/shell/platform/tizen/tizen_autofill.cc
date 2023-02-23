// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_autofill.h"

#include <app_common.h>
#include <autofill_common.h>

#include <functional>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

std::optional<autofill_hint_e> ConvertAutofillHint(std::string hint) {
  if (hint == "creditCardExpirationDate") {
    return AUTOFILL_HINT_CREDIT_CARD_EXPIRATION_DATE;
  } else if (hint == "creditCardExpirationDay") {
    return AUTOFILL_HINT_CREDIT_CARD_EXPIRATION_DAY;
  } else if (hint == "creditCardExpirationMonth") {
    return AUTOFILL_HINT_CREDIT_CARD_EXPIRATION_MONTH;
  } else if (hint == "creditCardExpirationYear") {
    return AUTOFILL_HINT_CREDIT_CARD_EXPIRATION_YEAR;
  } else if (hint == "email") {
    return AUTOFILL_HINT_EMAIL_ADDRESS;
  } else if (hint == "name") {
    return AUTOFILL_HINT_NAME;
  } else if (hint == "telephoneNumber") {
    return AUTOFILL_HINT_PHONE;
  } else if (hint == "postalAddress") {
    return AUTOFILL_HINT_POSTAL_ADDRESS;
  } else if (hint == "postalCode") {
    return AUTOFILL_HINT_POSTAL_CODE;
  } else if (hint == "username") {
    return AUTOFILL_HINT_ID;
  } else if (hint == "password") {
    return AUTOFILL_HINT_PASSWORD;
  } else if (hint == "creditCardSecurityCode") {
    return AUTOFILL_HINT_CREDIT_CARD_SECURITY_CODE;
  }
  FT_LOG(Error) << "Not supported autofill hint : " << hint;
  return std::nullopt;
}

bool StoreFillResponseItem(autofill_fill_response_item_h item,
                           void* user_data) {
  char* id = nullptr;
  char* value = nullptr;
  char* label = nullptr;

  autofill_fill_response_item_get_id(item, &id);
  autofill_fill_response_item_get_presentation_text(item, &label);
  autofill_fill_response_item_get_value(item, &value);
  std::unique_ptr<AutofillItem> response_item =
      std::make_unique<AutofillItem>();
  response_item->id = std::string(id);
  response_item->value = std::string(value);
  response_item->label = std::string(label);

  TizenAutofill* self = static_cast<TizenAutofill*>(user_data);
  self->StoreResponseItem(std::move(response_item));
  if (id) {
    free(id);
  }

  if (value) {
    free(value);
  }

  if (label) {
    free(label);
  }
  return true;
}

bool StoreForeachItem(autofill_fill_response_group_h group, void* user_data) {
  autofill_fill_response_group_foreach_item(group, StoreFillResponseItem,
                                            user_data);
  return true;
};

void ResponseReceived(autofill_h autofill,
                      autofill_fill_response_h fill_response,
                      void* user_data) {
  autofill_fill_response_foreach_group(fill_response, StoreForeachItem,
                                       user_data);
  TizenAutofill* self = static_cast<TizenAutofill*>(user_data);
  self->OnPopup();
};

autofill_save_item_h CreateSaveItem(const AutofillItem& item) {
  autofill_save_item_h save_item = nullptr;
  int ret = autofill_save_item_create(&save_item);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create autofill save item.";
    return nullptr;
  }

  autofill_save_item_set_autofill_hint(save_item, item.hint);
  autofill_save_item_set_id(save_item, item.id.c_str());
  autofill_save_item_set_label(save_item, item.label.c_str());
  autofill_save_item_set_sensitive_data(save_item, item.sensitive_data);
  autofill_save_item_set_value(save_item, item.value.c_str());

  return save_item;
}

autofill_save_view_info_h CreateSaveViewInfo(const std::string& view_id,
                                             const AutofillItem& item) {
  autofill_save_item_h save_item = CreateSaveItem(item);
  if (save_item == nullptr) {
    return nullptr;
  }

  char* app_id = nullptr;
  app_get_id(&app_id);

  autofill_save_view_info_h save_view_info = nullptr;
  int ret = autofill_save_view_info_create(&save_view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create autofill save view info.";
    return nullptr;
  }
  autofill_save_view_info_set_app_id(save_view_info, app_id);
  autofill_save_view_info_set_view_id(save_view_info, view_id.c_str());
  autofill_save_view_info_add_item(save_view_info, save_item);

  if (app_id) {
    free(app_id);
  }

  return save_view_info;
}

void AddItemsToViewInfo(autofill_view_info_h view_info,
                        const std::string& id,
                        const std::vector<std::string>& hints) {
  for (auto hint : hints) {
    std::optional<autofill_hint_e> autofill_hint = ConvertAutofillHint(hint);
    if (autofill_hint.has_value()) {
      autofill_item_h item = nullptr;
      int ret = autofill_item_create(&item);
      if (ret != AUTOFILL_ERROR_NONE) {
        FT_LOG(Error) << "Failed to create autofill item.";
        continue;
      }
      autofill_item_set_autofill_hint(item, autofill_hint.value());
      autofill_item_set_id(item, id.c_str());
      autofill_item_set_sensitive_data(item, false);
      autofill_view_info_add_item(view_info, item);
      autofill_item_destroy(item);
    }
  }
}

autofill_view_info_h CreateViewInfo(const std::string& id,
                                    const std::vector<std::string>& hints) {
  char* app_id = nullptr;
  app_get_id(&app_id);

  autofill_view_info_h view_info = nullptr;
  int ret = autofill_view_info_create(&view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create autofill view info.";
    return nullptr;
  }
  autofill_view_info_set_app_id(view_info, app_id);
  autofill_view_info_set_view_id(view_info, id.c_str());

  if (app_id) {
    free(app_id);
  }

  AddItemsToViewInfo(view_info, id, hints);

  return view_info;
}

}  // namespace

TizenAutofill::TizenAutofill() {
  Initialize();
}

TizenAutofill::~TizenAutofill() {
  autofill_fill_response_unset_received_cb(autofill_);
  autofill_destroy(autofill_);
}

void TizenAutofill::Initialize() {
  int ret = AUTOFILL_ERROR_NONE;
  if (!autofill_) {
    ret = autofill_create(&autofill_);
    if (ret != AUTOFILL_ERROR_NONE) {
      FT_LOG(Error) << "Failed to create autofill handle.";
      return;
    }
  }

  ret = autofill_connect(
      autofill_,
      [](autofill_h autofill, autofill_connection_status_e status,
         void* user_data) {
        TizenAutofill* self = static_cast<TizenAutofill*>(user_data);
        if (status == AUTOFILL_CONNECTION_STATUS_CONNECTED) {
          self->SetConnected(true);
        } else {
          self->SetConnected(false);
        }
      },
      this);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to connect to the autofill daemon.";
    autofill_destroy(autofill_);
    autofill_ = nullptr;
    return;
  }

  autofill_fill_response_set_received_cb(autofill_, ResponseReceived, this);

  response_items_.clear();
  is_initialized_ = true;
}

void TizenAutofill::RequestAutofill(const std::string& id,
                                    const std::vector<std::string>& hints) {
  if (!is_initialized_) {
    Initialize();
    return;
  }

  if (!is_connected_) {
    return;
  }

  autofill_view_info_h view_info = CreateViewInfo(id, hints);
  if (view_info == nullptr) {
    return;
  }

  int ret = autofill_fill_request(autofill_, view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to request autofill.";
  }
  autofill_view_info_destroy(view_info);

  response_items_.clear();
}

void TizenAutofill::RegisterItem(const std::string& view_id,
                                 const AutofillItem& item) {
  if (!is_initialized_) {
    Initialize();
    return;
  }

  if (!is_connected_) {
    return;
  }

  autofill_save_view_info_h save_view_info = CreateSaveViewInfo(view_id, item);
  if (save_view_info == nullptr) {
    return;
  }

  int ret = autofill_commit(autofill_, save_view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to register autofill item.";
  }

  autofill_save_view_info_destroy(save_view_info);
}

}  // namespace flutter
