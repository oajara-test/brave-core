/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_

#include "components/omnibox/browser/omnibox_edit_model.h"

class OmniboxView;
class OmniboxEditController;
class OmniboxClient;

class BraveOmniboxEditModel : public OmniboxEditModel {
 public:
  BraveOmniboxEditModel(OmniboxView* view,
                        OmniboxEditController* controller,
                        std::unique_ptr<OmniboxClient> client);
  virtual ~BraveOmniboxEditModel();
  BraveOmniboxEditModel(const BraveOmniboxEditModel&) = delete;
  BraveOmniboxEditModel& operator=(const BraveOmniboxEditModel&) = delete;

  void AdjustTextForCopy(int sel_min,
                         std::u16string* text,
                         GURL* url_from_text,
                         bool* write_url) override;

 private:
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_BRAVE_OMNIBOX_EDIT_MODEL_H_
