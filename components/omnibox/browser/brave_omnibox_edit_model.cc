/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_omnibox_edit_model.h"

BraveOmniboxEditModel::BraveOmniboxEditModel(
    OmniboxView* view,
    OmniboxEditController* controller,
    std::unique_ptr<OmniboxClient> client)
    : OmniboxEditModel(view, controller, std::move(client)) {}

void BraveOmniboxEditModel::AdjustTextForCopy(int sel_min,
                                              std::u16string* text,
                                              GURL* url_from_text,
                                              bool* write_url) {
  OmniboxEditModel::AdjustTextForCopy(sel_min, text, url_from_text, write_url);
}
