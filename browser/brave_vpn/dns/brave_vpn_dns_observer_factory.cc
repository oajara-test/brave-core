/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_factory.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/feature_list.h"
#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/features.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_manager.h"
#include "components/policy/core/common/cloud/machine_level_user_cloud_policy_store.h"
#include "components/policy/core/common/policy_map.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace brave_vpn {

// static
BraveVpnDnsObserverFactory* BraveVpnDnsObserverFactory::GetInstance() {
  return base::Singleton<BraveVpnDnsObserverFactory>::get();
}

BraveVpnDnsObserverFactory::~BraveVpnDnsObserverFactory() = default;

BraveVpnDnsObserverFactory::BraveVpnDnsObserverFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnDNSObserverService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* BraveVpnDnsObserverFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveVpnDnsObserverService(g_browser_process->local_state(),
                                        user_prefs::UserPrefs::Get(context));
}

// static
BraveVpnDnsObserverService* BraveVpnDnsObserverFactory::GetServiceForContext(
    content::BrowserContext* context) {
  if (!base::FeatureList::IsEnabled(
          brave_vpn::features::kBraveVPNDnsProtection)) {
    return nullptr;
  }
  DCHECK(IsBraveVPNEnabled());
  return static_cast<BraveVpnDnsObserverService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

void BraveVpnDnsObserverFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kBraveVPNShowDNSPolicyWarningDialog,
                                true);
}

}  // namespace brave_vpn
