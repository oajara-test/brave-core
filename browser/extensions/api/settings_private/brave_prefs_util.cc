/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/extensions/api/settings_private.h"

namespace extensions{

namespace settings_api = api::settings_private;

const PrefsUtil::TypedPrefMap& BravePrefsUtil::GetWhitelistedKeys() {
  // Static cache, similar to parent class
  static PrefsUtil::TypedPrefMap* s_brave_whitelist = nullptr;
  if (s_brave_whitelist)
    return *s_brave_whitelist;
  s_brave_whitelist = new PrefsUtil::TypedPrefMap();
  // Start with parent class whitelist
  const auto chromium_prefs = PrefsUtil::GetWhitelistedKeys();
  s_brave_whitelist->insert(chromium_prefs.begin(), chromium_prefs.end());
  // Add Brave values to the whitelist
#if !defined(OS_CHROMEOS)
  // import data
  (*s_brave_whitelist)[::prefs::kImportDialogCookies] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
  (*s_brave_whitelist)[::prefs::kImportDialogStats] =
      settings_api::PrefType::PREF_TYPE_BOOLEAN;
#endif
  // appearance prefs
  (*s_brave_whitelist)[kLocationBarIsWide] =
    settings_api::PrefType::PREF_TYPE_BOOLEAN;
  return *s_brave_whitelist;
}

}
