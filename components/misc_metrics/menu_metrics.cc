/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/menu_metrics.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace misc_metrics {

namespace {

const MenuGroup kAllMenuGroups[] = {
    MenuGroup::kTabWindow, MenuGroup::kBraveFeatures, MenuGroup::kBrowserViews};

const char kTabWindowPrefKey[] = "tab_window";
const char kBraveFeaturesPrefKey[] = "brave_features";
const char kBrowserViewsPrefKey[] = "browser_views";

constexpr base::TimeDelta kUpdateInterval = base::Days(1);

const char* GetMenuGroupPrefKey(MenuGroup group) {
  switch (group) {
    case MenuGroup::kTabWindow:
      return kTabWindowPrefKey;
    case MenuGroup::kBraveFeatures:
      return kBraveFeaturesPrefKey;
    case MenuGroup::kBrowserViews:
      return kBrowserViewsPrefKey;
  }
  NOTREACHED();
}

}  // namespace

const char kFrequentMenuGroupHistogramName[] =
    "Brave.Toolbar.FrequentMenuGroup";
const char kMenuDismissRateHistogramName[] = "Brave.Toolbar.MenuDismissRate";

MenuMetrics::MenuMetrics(PrefService* local_state)
    : local_state_(local_state),
      menu_shown_storage_(local_state, kMiscMetricsMenuShownStorage),
      menu_dismiss_storage_(local_state, kMiscMetricsMenuDismissStorage) {
  update_timer_.Start(FROM_HERE, kUpdateInterval, this, &MenuMetrics::Update);
  Update();
}

MenuMetrics::~MenuMetrics() = default;

void MenuMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsMenuGroupActionCounts);
  registry->RegisterListPref(kMiscMetricsMenuShownStorage);
  registry->RegisterListPref(kMiscMetricsMenuDismissStorage);
}

void MenuMetrics::RecordMenuGroupAction(MenuGroup group) {
  const char* group_pref_key = GetMenuGroupPrefKey(group);

  VLOG(2) << "MenuMetrics: recorded " << group_pref_key;

  DictionaryPrefUpdate update(local_state_, kMiscMetricsMenuGroupActionCounts);
  base::Value::Dict& update_dict = update->GetDict();
  for (MenuGroup it : kAllMenuGroups) {
    if (!menu_group_access_counts_.contains(it)) {
      double value =
          update_dict.FindDouble(GetMenuGroupPrefKey(it)).value_or(0);
      menu_group_access_counts_[it] = value;
    }
  }
  menu_group_access_counts_[group]++;
  update_dict.Set(GetMenuGroupPrefKey(group), menu_group_access_counts_[group]);
  const auto& result = base::ranges::max_element(
      menu_group_access_counts_.begin(), menu_group_access_counts_.end(),
      [](const auto& a, const auto& b) { return a.second < b.second; });
  if (result == menu_group_access_counts_.end()) {
    return;
  }
  int histogram_value = -1;
  switch (result->first) {
    case MenuGroup::kTabWindow:
      histogram_value = 0;
      break;
    case MenuGroup::kBraveFeatures:
      histogram_value = 1;
      break;
    case MenuGroup::kBrowserViews:
      histogram_value = 2;
      break;
    default:
      NOTREACHED();
      return;
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kFrequentMenuGroupHistogramName, histogram_value,
                             3);
}

void MenuMetrics::RecordMenuShown() {
  VLOG(2) << "MenuMetrics: menu shown";
  menu_shown_storage_.AddDelta(1);
  RecordMenuDismissRate();
}

void MenuMetrics::RecordMenuDismiss() {
  VLOG(2) << "MenuMetrics: menu dismiss";
  menu_dismiss_storage_.AddDelta(1);
  RecordMenuDismissRate();
}

void MenuMetrics::RecordMenuDismissRate() {
  double shown_sum = menu_shown_storage_.GetWeeklySum();
  double dismiss_sum = menu_dismiss_storage_.GetWeeklySum();

  int answer = 0;
  if (shown_sum != 0) {
    double rate = dismiss_sum / shown_sum;

    VLOG(2) << "MenuMetrics: menu dismiss rate: " << rate;

    if (rate < 0.25) {
      answer = 1;
    } else if (rate >= 0.25 && rate < 0.5) {
      answer = 2;
    } else if (rate >= 0.5 && rate < 0.75) {
      answer = 3;
    } else if (rate >= 0.75) {
      answer = 4;
    }
  }

  UMA_HISTOGRAM_EXACT_LINEAR(kMenuDismissRateHistogramName, answer, 5);
}

void MenuMetrics::Update() {
  RecordMenuDismissRate();
}

}  // namespace misc_metrics
