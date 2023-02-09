// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_autofill.h"

#include <app_common.h>
#include <autofill_common.h>

#include <functional>

#include "flutter/shell/platform/tizen/logger.h"

TizenAutofill::TizenAutofill() {
  InitailizeAutofill();
}

TizenAutofill::~TizenAutofill() {
  autofill_destroy(autofill_);
}

void TizenAutofill::InitailizeAutofill() {
  autofill_create(&autofill_);

  int ret = autofill_connect(
      autofill_,
      [](autofill_h autofill, autofill_connection_status_e status,
         void* user_data) {},
      nullptr);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "connect_autofill_daemon error";
  }

  autofill_fill_response_set_received_cb(
      autofill_,
      [](autofill_h autofill, autofill_fill_response_h fill_response,
         void* data) {
        int count = 0;
        autofill_fill_response_get_group_count(fill_response, &count);
        autofill_fill_response_foreach_group(
            fill_response,
            [](autofill_fill_response_group_h group, void* user_data) {
              autofill_fill_response_group_foreach_item(
                  group,
                  [](autofill_fill_response_item_h item, void* user_data) {
                    char* id = nullptr;
                    char* value = nullptr;
                    char* presentation_text = nullptr;

                    autofill_fill_response_item_get_id(item, &id);
                    autofill_fill_response_item_get_presentation_text(
                        item, &presentation_text);
                    autofill_fill_response_item_get_value(item, &value);

                    std::unique_ptr<AutofillItem> response_item =
                        std::make_unique<AutofillItem>();
                    response_item->label_ = std::string(presentation_text);
                    response_item->id_ = std::string(id);
                    response_item->value_ = std::string(value);

                    TizenAutofill::GetInstance().StoreResponseItem(
                        move(response_item));

                    if (id) {
                      free(id);
                    }

                    if (value) {
                      free(value);
                    }

                    if (presentation_text) {
                      free(presentation_text);
                    }

                    return true;
                  },
                  group);
              return true;
            },
            &count);
        TizenAutofill::GetInstance().OnPopup();
      },
      nullptr);

  response_items_.clear();
}

void TizenAutofill::RequestAutofill(std::vector<std::string> hints,
                                    std::string id) {
  char* app_id = nullptr;
  app_get_id(&app_id);

  autofill_view_info_h view_info = nullptr;
  autofill_view_info_create(&view_info);
  autofill_view_info_set_app_id(view_info, app_id);
  autofill_view_info_set_view_id(view_info, id.c_str());

  if (app_id) {
    free(app_id);
  }

  for (auto hint : hints) {
    std::optional<autofill_hint_e> autofill_hint = ConvertAutofillHint(hint);
    if (autofill_hint.has_value()) {
      autofill_item_h item = nullptr;
      autofill_item_create(&item);
      autofill_item_set_autofill_hint(item, autofill_hint.value());
      autofill_item_set_id(item, id.c_str());
      autofill_item_set_sensitive_data(item, false);
      autofill_view_info_add_item(view_info, item);
      autofill_item_destroy(item);
    }
  }

  int ret = autofill_fill_request(autofill_, view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "autofill_fill_request error";
  }
  autofill_view_info_destroy(view_info);

  response_items_.clear();
}

void TizenAutofill::RegisterAutofillItem(std::string view_id,
                                         AutofillItem item) {
  autofill_save_item_h save_item = nullptr;

  autofill_save_item_create(&save_item);
  autofill_save_item_set_autofill_hint(save_item, item.hint_);
  autofill_save_item_set_id(save_item, item.id_.c_str());
  autofill_save_item_set_label(save_item, item.label_.c_str());
  autofill_save_item_set_sensitive_data(save_item, item.sensitive_data_);
  autofill_save_item_set_value(save_item, item.value_.c_str());

  char* app_id;
  app_get_id(&app_id);

  autofill_save_view_info_h save_view_info = nullptr;

  autofill_save_view_info_create(&save_view_info);
  autofill_save_view_info_set_app_id(save_view_info, app_id);
  autofill_save_view_info_set_view_id(save_view_info, view_id.c_str());

  autofill_save_view_info_add_item(save_view_info, save_item);

  if (app_id) {
    free(app_id);
  }

  int ret = autofill_commit(autofill_, save_view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "autofill_commit error";
  }

  autofill_save_view_info_destroy(save_view_info);
}

std::optional<autofill_hint_e> TizenAutofill::ConvertAutofillHint(
    std::string hint) {
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
