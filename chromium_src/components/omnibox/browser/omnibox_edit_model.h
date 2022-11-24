/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_

#define AdjustTextForCopy \
  dummy();                \
  virtual void AdjustTextForCopy
#include "src/components/omnibox/browser/omnibox_edit_model.h"
#undef AdjustTextForCopy

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_EDIT_MODEL_H_
