// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/google_sign_in/features.h"

#include "base/feature_list.h"
#include "build/build_config.h"

namespace permissions::features {

// When enabled, Brave will prompt for permission on sites which want to use
// Google Sign In
BASE_FEATURE(kBraveGoogleSignInPermission,
             "BraveGoogleSignInPermission",
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace permissions::features
