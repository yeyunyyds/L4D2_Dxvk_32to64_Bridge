// SPDX-License-Identifier: MIT
// Copyright (c) 2026 yeyunyyds.
#include "host_plugins.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

struct TestCounts {
  unsigned loaded;
  unsigned created;
  unsigned presented;
  unsigned beforeReset;
  unsigned afterReset;
  unsigned destroyed;
  unsigned shutdown;
  unsigned hostBits;
};

static std::vector<std::string> g_log;
static void WINAPI testLog(LDBHostLogLevel, const char* message) { if (message) { g_log.emplace_back(message); } }
static void require(bool condition, const char* message) {
  if (!condition) { std::fprintf(stderr, "%s (error %lu)\n", message, GetLastError()); std::exit(2); }
}

int main() {
  constexpr const wchar_t* moduleName = L"ldb-host-plugin-test.dll";
  l4d2_host_plugins::Manager disabled(testLog);
  disabled.initialize(false, "disabled-test");
  require(!disabled.enabled() && disabled.loaded() == 0 && !disabled.wantsPresent(), "disabled manager stays inert");
  require(GetModuleHandleW(moduleName) == nullptr, "disabled manager does not inspect or load plugin directory");
  require(g_log.empty(), "disabled manager performs no plugin work or logging");

  l4d2_host_plugins::Manager enabled(testLog);
  enabled.initialize(true, "enabled-test");
  require(enabled.enabled() && enabled.loaded() == 1 && enabled.wantsPresent(), "enabled manager loads one valid plugin");
  HMODULE module = GetModuleHandleW(moduleName);
  require(module != nullptr, "test plugin remains loaded for Host lifetime");
  auto rawCounts = GetProcAddress(module, "LDBHostPluginTestCounts");
  const TestCounts* (WINAPI* counts)() = nullptr;
  std::memcpy(&counts, &rawCounts, sizeof(counts));
  require(counts != nullptr && counts()->loaded == 1 && counts()->hostBits == sizeof(void*) * 8,
    "plugin receives versioned Host API and actual architecture");

  auto* device = reinterpret_cast<IDirect3DDevice9*>(static_cast<uintptr_t>(1));
  D3DPRESENT_PARAMETERS parameters {};
  LDBHostPresentInfoV1 present { sizeof(present), FALSE, nullptr, nullptr, nullptr, nullptr, 0 };
  enabled.deviceCreated(device);
  enabled.beforePresent(device, present);
  enabled.beforeReset(device, &parameters, FALSE);
  enabled.afterReset(device, D3D_OK, FALSE);
  enabled.deviceDestroyed(device);
  const TestCounts beforeShutdown = *counts();
  require(beforeShutdown.created == 1 && beforeShutdown.presented == 1
    && beforeShutdown.beforeReset == 1 && beforeShutdown.afterReset == 1
    && beforeShutdown.destroyed == 1, "all D3D9 lifecycle callbacks are routed once");

  enabled.shutdown();
  require(GetModuleHandleW(moduleName) == nullptr, "shutdown unloads independent plugin DLL");
  bool sawShutdown = false;
  for (const auto& line : g_log) { if (line.find("LDB_TEST_PLUGIN shutdown=1") != std::string::npos) { sawShutdown = true; } }
  require(sawShutdown, "plugin shutdown callback runs before unload");
  enabled.shutdown();
  std::printf("PASS host plugin ABI: disabled-no-scan, opt-in load, lifecycle, shutdown (%zu-bit)\n", sizeof(void*) * 8);
  return 0;
}
