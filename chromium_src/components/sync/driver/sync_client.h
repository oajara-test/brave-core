/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_H_

#define OnLocalSyncTransportDataCleared                \
SetDefaultEnabledTypes(SyncService* sync_service) = 0; \
virtual void OnLocalSyncTransportDataCleared

#include "src/components/sync/driver/sync_client.h"
#undef OnLocalSyncTransportDataCleared

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_H_
