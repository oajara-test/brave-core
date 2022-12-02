/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/features.h"
namespace {

bool ShouldReplaceSecureDNSDisabledDescription() {
  if (!base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection))
    return false;
  return !g_browser_process->local_state()
              ->GetString(prefs::kBraveVpnDnsConfig)
              .empty();
}

}  // namespace

#define AddSecureDnsStrings AddSecureDnsStrings_ChromiumImpl

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)

// Use custom strings for diagnostic (crashes, hangs) reporting settings.
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF
#undef IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC
#define IDS_SETTINGS_ENABLE_LOGGING_PREF IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF
#define IDS_SETTINGS_ENABLE_LOGGING_PREF_DESC \
  IDS_BRAVE_DIAGNOSTIC_REPORTS_PREF_DESC

#include "src/chrome/browser/ui/webui/settings/shared_settings_localized_strings_provider.cc"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#undef AddSecureDnsStrings
namespace settings {

void AddSecureDnsStrings(content::WebUIDataSource* html_source) {
  AddSecureDnsStrings_ChromiumImpl(html_source);
  if (!ShouldReplaceSecureDNSDisabledDescription())
    return;
  static constexpr webui::LocalizedString kLocalizedStrings[] = {
      {"secureDnsDisabledForManagedEnvironment",
       IDS_SETTINGS_SECURE_DNS_DISABLED_BY_BRAVE_VPN}};

  html_source->AddLocalizedStrings(kLocalizedStrings);
}

}  // namespace settings

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN)
