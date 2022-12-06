/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_dns_observer_service.h"

#include <unordered_map>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "brave/browser/brave_profile_prefs.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/secure_dns_util.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/dns/public/secure_dns_mode.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {
namespace {
const char kCustomServersURLs[] =
    "https://server1.com\nhttps://server2.com/{?dns}";
const char kCloudflareDnsProviderURL[] =
    "https://chrome.cloudflare-dns.com/dns-query";
}  // namespace

class BraveVpnDnsObserverServiceUnitTest : public testing::Test {
 public:
  BraveVpnDnsObserverServiceUnitTest() {}

  void SetUp() override {
    RegisterLocalState(local_state_.registry());
    RegisterUserProfilePrefs(profile_pref_service_.registry());
    brave_vpn::RegisterProfilePrefs(profile_pref_service_.registry());

    profile_pref_service_.registry()->RegisterBooleanPref(
        prefs::kBraveVPNShowDNSPolicyWarningDialog, true);
    dns_service_.reset(
        new BraveVpnDnsObserverService(local_state(), pref_service()));
    stub_resolver_config_reader_ =
        std::make_unique<StubResolverConfigReader>(&local_state_);
    SystemNetworkContextManager::set_stub_resolver_config_reader_for_testing(
        stub_resolver_config_reader_.get());
    dns_service_->SetPrefServiceForTesting(pref_service());
    dns_service_->SetVPNNotificationCallbackForTesting(base::DoNothing());
  }

  void TearDown() override {
    // BraveVpnDnsObserverService destructor must be called before the task
    // runner is destroyed.
    dns_service_.reset();
  }
  PrefService* local_state() { return &local_state_; }
  PrefService* pref_service() { return &profile_pref_service_; }

  void FireBraveVPNStateChange(mojom::ConnectionState state) {
    dns_service_->OnConnectionStateChanged(state);
  }

  bool WasVpnNotificationShownForMode(const std::string& mode,
                                      const std::string& servers) {
    bool callback_called = false;
    dns_service_->SetVPNNotificationCallbackForTesting(
        base::BindLambdaForTesting([&]() { callback_called = true; }));
    SetDNSMode(mode, servers);
    return callback_called;
  }

  void SetDNSMode(const std::string& mode, const std::string& doh_providers) {
    local_state()->SetString(::prefs::kDnsOverHttpsTemplates, doh_providers);
    local_state()->SetString(::prefs::kDnsOverHttpsMode, mode);
    SystemNetworkContextManager::GetStubResolverConfigReader()
        ->UpdateNetworkService(false);
  }

  bool WasPolicyNotificationShownForState(mojom::ConnectionState state) {
    bool callback_called = false;
    dns_service_->SetPolicyNotificationCallbackForTesting(
        base::BindLambdaForTesting([&]() { callback_called = true; }));
    FireBraveVPNStateChange(state);
    return callback_called;
  }

  void ExpectDNSMode(const std::string& mode,
                     const std::string& doh_providers) {
    auto dns_config = SystemNetworkContextManager::GetStubResolverConfigReader()
                          ->GetSecureDnsConfiguration(false);
    auto* current_mode = SecureDnsConfig::ModeToString(dns_config.mode());
    auto current_servers = dns_config.doh_servers().ToString();
    EXPECT_EQ(current_mode, mode);
    EXPECT_EQ(current_servers, doh_providers);
  }
  void SetManagedMode(const std::string& value) {
    local_state_.SetManagedPref(::prefs::kDnsOverHttpsMode,
                                std::make_unique<base::Value>(value));
  }

 private:
  std::unordered_map<std::string, std::string> policy_map_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVpnDnsObserverService> dns_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;
};

TEST_F(BraveVpnDnsObserverServiceUnitTest, AutoEnable) {
  // Browser DoH mode off -> override browser config and enable vpn
  local_state()->ClearPref(::prefs::kBraveVpnDnsConfig);
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode automatic -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode secure -> do not override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode automatic with custom servers
  // -> we override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, kCustomServersURLs);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode automatic with broken custom servers
  // -> override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, std::string());
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode secure with custom servers
  // -> do not override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, DoHConfiguredByPolicy) {
  SetManagedMode(SecureDnsConfig::kModeOff);

  SetDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");

  SetManagedMode(SecureDnsConfig::kModeAutomatic);

  SetDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_TRUE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");

  SetManagedMode(SecureDnsConfig::kModeSecure);
  SetDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, "");

  SetDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(WasPolicyNotificationShownForState(
      mojom::ConnectionState::DISCONNECTING));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::DISCONNECTED));
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCustomServersURLs);

  // Do not show dialog option enabled
  SetManagedMode(SecureDnsConfig::kModeOff);
  pref_service()->SetBoolean(prefs::kBraveVPNShowDNSPolicyWarningDialog, false);
  SetDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_FALSE(
      WasPolicyNotificationShownForState(mojom::ConnectionState::CONNECTED));
}

TEST_F(BraveVpnDnsObserverServiceUnitTest, NotifyIfUnblocked) {
  // Browser DoH mode secure -> do not override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTING);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  // Mode secure, servers changed, the dialog should not be shown
  // as we have secure mode.
  EXPECT_FALSE(WasVpnNotificationShownForMode(SecureDnsConfig::kModeSecure,
                                              kCustomServersURLs));

  // Mode changed to automatic, servers changed, the dialog should be shown.
  EXPECT_TRUE(
      WasVpnNotificationShownForMode(SecureDnsConfig::kModeAutomatic, ""));

  // No reaction for other settings changes as we have already notified use.
  EXPECT_FALSE(WasVpnNotificationShownForMode(SecureDnsConfig::kModeOff, ""));

  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeOff, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());

  // Browser DoH mode secure -> do not override browser config and enable vpn
  SetDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  FireBraveVPNStateChange(mojom::ConnectionState::CONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeSecure, kCloudflareDnsProviderURL);
  EXPECT_FALSE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
  // Mode secure, servers changed, the dialog should not be shown
  // as we have secure mode.
  EXPECT_FALSE(WasVpnNotificationShownForMode(SecureDnsConfig::kModeSecure,
                                              kCustomServersURLs));

  // Mode changed to off, servers changed, the dialog should be shown.
  EXPECT_TRUE(WasVpnNotificationShownForMode(SecureDnsConfig::kModeOff, ""));

  // No reaction for other settings changes as we have already notified use.
  EXPECT_FALSE(
      WasVpnNotificationShownForMode(SecureDnsConfig::kModeAutomatic, ""));

  FireBraveVPNStateChange(mojom::ConnectionState::DISCONNECTED);
  ExpectDNSMode(SecureDnsConfig::kModeAutomatic, "");
  EXPECT_TRUE(local_state()->GetString(::prefs::kBraveVpnDnsConfig).empty());
}

}  // namespace brave_vpn
