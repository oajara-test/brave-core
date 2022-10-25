// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <windows.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "base/win/process_startup_helper.h"
#include "brave/vpn/service_main.h"

int main(int argc, char* argv[]) {
  // Initialize the CommandLine singleton from the environment.
  base::CommandLine::Init(0, nullptr);

  logging::LoggingSettings settings;
  logging::InitLogging(settings);

  // The exit manager is in charge of calling the dtors of singletons.
  base::AtExitManager exit_manager;

  // Make sure the process exits cleanly on unexpected errors.
  base::EnableTerminationOnHeapCorruption();
  base::EnableTerminationOnOutOfMemory();
  base::win::RegisterInvalidParamHandler();
  auto* command_line = base::CommandLine::ForCurrentProcess();
  base::win::SetupCRT(*command_line);

  // Run the service.
  brave_vpn::ServiceMain* service = brave_vpn::ServiceMain::GetInstance();
  if (!service->InitWithCommandLine(command_line))
    return -1;

  return service->Start();
}
