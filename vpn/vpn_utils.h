// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_VPN_VPN_UTILS_H_
#define BRAVE_VPN_VPN_UTILS_H_

#include <windows.h>
#include <string>

namespace brave_vpn {
// Register and setup DNS filters layer to the system, if the layer is already
// registered reuses existing.
bool AddWpmFilters(HANDLE engine_handle, const std::string& name);
// Opens a session to a filter engine.
HANDLE OpenWpmSession();
// Closes a session to a filter engine.
bool CloseWpmSession(HANDLE engine);
// Subscribes for RAS connection notification of any os vpn entry.
bool SubscribeRasConnectionNotification(HANDLE event_handle);
}  // namespace brave_vpn

#endif  // BRAVE_VPN_VPN_UTILS_H_
