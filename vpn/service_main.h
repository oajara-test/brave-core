/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VPN_SERVICE_MAIN_H_
#define BRAVE_VPN_SERVICE_MAIN_H_

#include <windows.h>
#include <memory>

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "brave/vpn/vpn_dns_handler.h"

namespace base {
class CommandLine;
}  // namespace base

namespace brave_vpn {

class ServiceMain : public brave_vpn::VpnDnsHandler {
 public:
  static ServiceMain* GetInstance();

  ServiceMain(const ServiceMain&) = delete;
  ServiceMain& operator=(const ServiceMain&) = delete;

  // This function parses the command line and selects the action routine.
  bool InitWithCommandLine(const base::CommandLine* command_line);

  // Start() is the entry point called by WinMain.
  int Start();

  // brave_vpn::VpnDnsHandler:
  void SignalExit() override;

 private:
  ServiceMain();
  ~ServiceMain() override;

  // This function handshakes with the service control manager and starts
  // the service.
  int RunAsService();

  // Runs the service on the service thread.
  void ServiceMainImpl();

  // Runs as a local server for testing purposes. RunInteractive returns an
  // HRESULT, not a Win32 error code.
  int RunInteractive();

  // The control handler of the service.
  static void WINAPI ServiceControlHandler(DWORD control);

  // The main service entry point.
  static void WINAPI ServiceMainEntry(DWORD argc, wchar_t* argv[]);

  // Calls ::SetServiceStatus().
  void SetServiceStatus(DWORD state);

  // Handles object registration, message loop, and unregistration. Returns
  // when all registered objects are released.
  HRESULT Run();

  // The action routine to be executed.
  int (ServiceMain::*run_routine_)();

  SERVICE_STATUS_HANDLE service_status_handle_;
  SERVICE_STATUS service_status_;
  base::WaitableEvent exit_signal_;
  base::OnceClosure quit_;
  friend class base::NoDestructor<ServiceMain>;
};

}  // namespace brave_vpn

#endif  // BRAVE_VPN_SERVICE_MAIN_H_
