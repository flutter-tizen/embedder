// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_autofill.h"

#include <app_common.h>
#include <autofill_common.h>

#include <functional>
#include <memory>

#include "flutter/shell/platform/tizen/logger.h"

TizenAutofill::TizenAutofill() {
  InitailizeAutofill();
}

TizenAutofill::~TizenAutofill() {
  autofill_destroy(ah_);
}

void TizenAutofill::InitailizeAutofill() {
  autofill_create(&ah_);

  int ret;
  ret = autofill_connect(
      ah_,
      [](autofill_h ah, autofill_connection_status_e status, void* user_data) {
      },
      NULL);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << __FUNCTION__ << "connect_autofill_daemon error";
  }

  autofill_fill_response_set_received_cb(
      ah_,
      [](autofill_h ah, autofill_fill_response_h fill_response, void* data) {
        int count = 0;
        autofill_fill_response_get_group_count(fill_response, &count);
        autofill_fill_response_foreach_group(
            fill_response,
            [](autofill_fill_response_group_h group_h, void* user_data) {
              autofill_fill_response_group_foreach_item(
                  group_h,
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
                  group_h);
              return true;
            },
            &count);
        TizenAutofill::GetInstance().CallPopupCallback();
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

  for (auto hint : hints) {
    std::optional<autofill_hint_e> autofill_hint = ConvertAutofillHint(hint);
    if (autofill_hint.has_value()) {
      autofill_item_h item;
      autofill_item_create(&item);
      autofill_item_set_autofill_hint(item, autofill_hint.value());
      autofill_item_set_id(item, id.c_str());
      autofill_item_set_sensitive_data(item, false);
      autofill_view_info_add_item(view_info, item);
      autofill_item_destroy(item);
    }
  }

  int ret = autofill_fill_request(ah_, view_info);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "autofill_fill_request error";
  }
  autofill_view_info_destroy(view_info);

  response_items_.clear();
}

void TizenAutofill::RegisterAutofillItem(std::string view_id,
                                         AutofillItem item) {
  autofill_save_item_h si_h;

  autofill_save_item_create(&si_h);
  autofill_save_item_set_autofill_hint(si_h, item.hint_);
  autofill_save_item_set_id(si_h, item.id_.c_str());
  autofill_save_item_set_label(si_h, item.label_.c_str());
  autofill_save_item_set_sensitive_data(si_h, item.sensitive_data_);
  autofill_save_item_set_value(si_h, item.value_.c_str());

  char* app_id;
  app_get_id(&app_id);

  autofill_save_view_info_h svi_h;

  autofill_save_view_info_create(&svi_h);
  autofill_save_view_info_set_app_id(svi_h, app_id);
  autofill_save_view_info_set_view_id(svi_h, view_id.c_str());

  autofill_save_view_info_add_item(svi_h, si_h);

  if (app_id) {
    free(app_id);
  }

  int ret;

  ret = autofill_commit(ah_, svi_h);
  if (ret != AUTOFILL_ERROR_NONE) {
    FT_LOG(Error) << "autofill_commit error";
  }

  autofill_save_view_info_destroy(svi_h);
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
