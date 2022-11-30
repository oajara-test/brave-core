/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/crash/core/app/crashpad.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"

#define BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE \
  process_type == brave_vpn::kBraveVPNHelperProcessType ||
#include "src/components/crash/core/app/crashpad.cc"
#undef BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE
