// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/google_sign_in/google_sign_in_permission_util.h"

#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/permissions/google_sign_in/features.h"
#include "components/permissions/permissions_client.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/url_pattern.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"

namespace permissions {

namespace {
constexpr char kGoogleAuthPattern[] =
    "https://accounts.google.com/o/oauth2/auth*";
constexpr char kFirebaseContentSettingsPattern[] =
    "https://[*.]firebaseapp.com/__/auth*";
// ContentSettingsPattern accepts [*.] as wildcard for subdomains, while
// URLPattern takes *.
constexpr char kFirebaseUrlPattern[] = "https://*.firebaseapp.com/__/auth*";

bool IsGoogleAuthUrl(const GURL& gurl, const bool check_origin_only) {
  static const std::vector<URLPattern> auth_login_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, kGoogleAuthPattern),
      URLPattern(URLPattern::SCHEME_HTTPS, kFirebaseUrlPattern),
  });
  return std::any_of(auth_login_patterns.begin(), auth_login_patterns.end(),
                     [&gurl, check_origin_only](URLPattern pattern) {
                       if (check_origin_only) {
                         return pattern.MatchesSecurityOrigin(gurl);
                       }
                       return pattern.MatchesURL(gurl);
                     });
}
}  // namespace

bool IsGoogleAuthRelatedRequest(const GURL& request_url,
                                const GURL& request_initiator_url) {
  return IsGoogleAuthUrl(request_url, false) &&
         !IsGoogleAuthUrl(request_initiator_url, true) &&
         !net::registry_controlled_domains::SameDomainOrHost(
             request_initiator_url,
             GURL(URLPattern(URLPattern::SCHEME_HTTPS, kGoogleAuthPattern)
                      .host()),
             net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool IsGoogleSignInFeatureEnabled() {
  return base::FeatureList::IsEnabled(
      permissions::features::kBraveGoogleSignInPermission);
}

bool IsGoogleSignInPrefEnabled(PrefService* prefs) {
  return prefs->FindPreference(kGoogleLoginControlType) &&
         prefs->GetBoolean(kGoogleLoginControlType);
}

blink::mojom::PermissionStatus GetCurrentGoogleSignInPermissionStatus(
    content::PermissionControllerDelegate* permission_controller,
    content::WebContents* contents,
    const GURL& request_initiator_url) {
  return permission_controller->GetPermissionStatusForOrigin(
      blink::PermissionType::BRAVE_GOOGLE_SIGN_IN,
      contents->GetPrimaryMainFrame(), request_initiator_url);
}

void Set3pCookieException(HostContentSettingsMap* content_settings,
                          const ContentSettingsPattern& embedding_pattern,
                          const ContentSetting& content_setting) {
  const std::vector<std::string> auth_content_settings_patterns(
      {kGoogleAuthPattern, kFirebaseContentSettingsPattern});
  // embedding_pattern is the website that is embedding the google sign in
  // auth pattern is auth.google.com or firebaseapp.com
  for (const auto& auth_url_pattern : auth_content_settings_patterns) {
    auto auth_pattern = ContentSettingsPattern::FromString(auth_url_pattern);
    content_settings->SetContentSettingCustomScope(
        auth_pattern, embedding_pattern, ContentSettingsType::BRAVE_COOKIES,
        content_setting);
  }
}

// Sets 3p cookie exception for Google Auth-related URLs, for a particular URL
// If the status == ASK i.e. user dismisses the prompt so we don't do anything
void HandleBraveGoogleSignInPermissionStatus(
    content::BrowserContext* context,
    const GURL& request_initiator_url,
    scoped_refptr<HostContentSettingsMap> content_settings,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  auto permission_status = permission_statuses[0];
  auto embedding_pattern =
      ContentSettingsPattern::FromURL(request_initiator_url);

  if (permission_status == blink::mojom::PermissionStatus::GRANTED) {
    // Add 3p exception for request_initiator_url for auth patterns
    Set3pCookieException(content_settings.get(), embedding_pattern,
                         CONTENT_SETTING_ALLOW);
  } else if (permission_status == blink::mojom::PermissionStatus::DENIED) {
    // Remove 3p exception for request_initiator_url for auth patterns
    Set3pCookieException(content_settings.get(), embedding_pattern,
                         CONTENT_SETTING_BLOCK);
  }
  return;
}

void CreateGoogleSignInPermissionRequest(
    bool* defer,
    content::PermissionControllerDelegate* permission_controller,
    content::RenderFrameHost* rfh,
    const GURL& request_initiator_url,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!rfh->IsDocumentOnLoadCompletedInMainFrame()) {
    return;
  }
  if (defer) {
    *defer = true;
  }
  permission_controller->RequestPermissionsForOrigin(
      {blink::PermissionType::BRAVE_GOOGLE_SIGN_IN}, rfh, request_initiator_url,
      true, std::move(callback));
}

bool GetPermissionAndMaybeCreatePrompt(
    content::WebContents* contents,
    const GURL& request_initiator_url,
    bool* defer,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        permission_result_callback,
    base::OnceCallback<void()> denied_callback) {
  // Check current permission status
  content::PermissionControllerDelegate* permission_controller =
      contents->GetBrowserContext()->GetPermissionControllerDelegate();
  auto current_status = GetCurrentGoogleSignInPermissionStatus(
      permission_controller, contents, request_initiator_url);

  switch (current_status) {
    case blink::mojom::PermissionStatus::GRANTED: {
      return true;
    }

    case blink::mojom::PermissionStatus::DENIED: {
      std::move(denied_callback).Run();
      return false;
    }

    case blink::mojom::PermissionStatus::ASK: {
      CreateGoogleSignInPermissionRequest(
          defer, permission_controller, contents->GetPrimaryMainFrame(),
          request_initiator_url, std::move(permission_result_callback));
      return false;
    }
  }
}

bool CanCreateWindow(content::RenderFrameHost* opener,
                     const GURL& opener_url,
                     const GURL& target_url) {
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(opener);

  HostContentSettingsMap* content_settings =
      permissions::PermissionsClient::Get()->GetSettingsMap(
          contents->GetBrowserContext());

  if (IsGoogleSignInFeatureEnabled() &&
      IsGoogleAuthRelatedRequest(target_url, opener_url)) {
    PrefService* prefs =
        user_prefs::UserPrefs::Get(contents->GetBrowserContext());
    if (!IsGoogleSignInPrefEnabled(prefs)) {
      return false;
    }

    return GetPermissionAndMaybeCreatePrompt(
        contents, opener_url, nullptr,
        base::BindOnce(
            &HandleBraveGoogleSignInPermissionStatus,
            contents->GetBrowserContext(), opener_url,
            base::WrapRefCounted<HostContentSettingsMap>(content_settings)),
        base::DoNothing());
  }
  // If not applying Google Sign-In permission logic, open window
  return true;
}

}  // namespace permissions
