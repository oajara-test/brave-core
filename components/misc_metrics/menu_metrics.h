/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/timer/timer.h"
#include "brave/components/time_period_storage/weekly_storage.h"

class PrefRegistrySimple;
class PrefService;

namespace misc_metrics {

enum class MenuGroup {
  kTabWindow,
  kBraveFeatures,
  kBrowserViews,
};

extern const char kFrequentMenuGroupHistogramName[];
extern const char kMenuDismissRateHistogramName[];

class MenuMetrics {
 public:
  explicit MenuMetrics(PrefService* local_state);
  ~MenuMetrics();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void RecordMenuGroupAction(MenuGroup group);

  void RecordMenuShown();
  void RecordMenuDismiss();

 private:
  void RecordMenuDismissRate();

  void Update();

  base::flat_map<MenuGroup, double> menu_group_access_counts_;

  raw_ptr<PrefService> local_state_ = nullptr;
  WeeklyStorage menu_shown_storage_;
  WeeklyStorage menu_dismiss_storage_;

  base::RepeatingTimer update_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_MENU_METRICS_H_
