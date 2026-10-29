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
  assert(std::strcmp(plugin->GetVersion(), "native-hud-1") == 0);
  assert(plugin->RequestHudMenu(true) == nullptr);
  assert(plugin->RequestHudMenu(false) == nullptr);
  assert(GetProcAddress(module, "L4D2BridgeHudToggle") != nullptr);
  puts("PASS: L4N is loader/device entry only; native HUD does not publish a KeyValues menu");
  FreeLibrary(module);
  return 0;
}
