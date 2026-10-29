// SPDX-License-Identifier: MIT
#include <windows.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include "../plugins/l4n/sdk/l4n_plugin.h"

int wmain(int argc, wchar_t** argv) {
  assert(argc == 2);
  const auto module = LoadLibraryW(argv[1]); assert(module);
  const auto get = reinterpret_cast<GetL4NPluginInstanceFunc>(GetProcAddress(module, "GetL4NPluginInstance")); assert(get);
  auto* plugin = get(); assert(plugin);
  assert(plugin->GetInterfaceVersion() == 2);
  assert(std::strcmp(plugin->GetName(), "L4D2 Bridge Native HUD") == 0);
  assert(std::strcmp(plugin->GetVersion(), "native-hud-3") == 0);
  assert(std::strcmp(plugin->RequestHudMenu(true), "LDB Control Panel") == 0);
  const char* menu = plugin->RequestHudMenu(false); assert(menu);
  assert(std::strstr(menu, "Open LDB Control Panel"));
  unsigned long long callback = 0;
  assert(sscanf_s(std::strstr(menu, "\"callback\""), "\"callback\" \"0x%llx\"", &callback) == 1);
  const auto open = reinterpret_cast<const char* (*)(void*)>(static_cast<uintptr_t>(callback));
  assert(open(nullptr) == nullptr);
  assert(GetProcAddress(module, "L4D2BridgeHudToggle") != nullptr);
  puts("PASS: L4N publishes one native-HUD launcher and no settings submenu");
  FreeLibrary(module);
  return 0;
}
