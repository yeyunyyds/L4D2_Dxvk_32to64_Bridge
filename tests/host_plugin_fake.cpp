// SPDX-License-Identifier: MIT
// Copyright (c) 2026 yeyunyyds.
#include "ldb_host_plugin.h"

#include <cstring>

struct TestCounts {
  unsigned loaded;
  unsigned created;
  unsigned swapchainsCreated;
  unsigned presented;
  unsigned beforeReset;
  unsigned afterReset;
  unsigned swapchainsDestroyed;
  unsigned destroyed;
  unsigned shutdown;
  unsigned hostBits;
};

static TestCounts g_counts {};
static const LDBHostApiV1* g_host = nullptr;

static BOOL WINAPI onLoad(const LDBHostApiV1* host) {
  if (!host || host->struct_size != sizeof(*host) || host->abi_version != LDB_HOST_PLUGIN_ABI_VERSION
    || !host->host_directory || !host->host_version || !host->log) { return FALSE; }
  g_host = host;
  ++g_counts.loaded;
  g_counts.hostBits = host->host_bits;
  return TRUE;
}
static void WINAPI onCreated(IDirect3DDevice9*) { ++g_counts.created; }
static void WINAPI onSwapChainCreated(IDirect3DSwapChain9*) { ++g_counts.swapchainsCreated; }
static void WINAPI onPresent(IDirect3DDevice9*, const LDBHostPresentInfoV1* info) {
  if (info && info->struct_size == sizeof(*info) && info->swap_chain) { ++g_counts.presented; }
}
static void WINAPI beforeReset(IDirect3DDevice9*, const D3DPRESENT_PARAMETERS*, BOOL) { ++g_counts.beforeReset; }
static void WINAPI afterReset(IDirect3DDevice9*, HRESULT, BOOL) { ++g_counts.afterReset; }
static void WINAPI onSwapChainDestroyed(IDirect3DSwapChain9*) { ++g_counts.swapchainsDestroyed; }
static void WINAPI onDestroyed(IDirect3DDevice9*) { ++g_counts.destroyed; }
static void WINAPI onShutdown() {
  ++g_counts.shutdown;
  if (g_host && g_host->log) { g_host->log(LDB_HOST_LOG_INFO, "LDB_TEST_PLUGIN shutdown=1"); }
}

extern "C" BOOL WINAPI LDBHostPluginQuery(uint32_t requested, LDBHostPluginV1* plugin) {
  if (requested != LDB_HOST_PLUGIN_ABI_VERSION || !plugin || plugin->struct_size != sizeof(*plugin)) { return FALSE; }
  LDBHostPluginV1 result {};
  result.struct_size = sizeof(result);
  result.abi_version = LDB_HOST_PLUGIN_ABI_VERSION;
  result.name = "LDB host test plugin";
  result.version = "1";
  result.on_load = onLoad;
  result.on_device_created = onCreated;
  result.on_swapchain_created = onSwapChainCreated;
  result.on_before_present = onPresent;
  result.on_before_reset = beforeReset;
  result.on_after_reset = afterReset;
  result.on_swapchain_destroyed = onSwapChainDestroyed;
  result.on_device_destroyed = onDestroyed;
  result.on_shutdown = onShutdown;
  std::memcpy(plugin, &result, sizeof(result));
  return TRUE;
}

extern "C" const TestCounts* WINAPI LDBHostPluginTestCounts() { return &g_counts; }
