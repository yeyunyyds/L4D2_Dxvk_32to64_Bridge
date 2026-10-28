// SPDX-License-Identifier: MIT
// Copyright (c) 2026 yeyunyyds.
#pragma once

#include <windows.h>
#include <d3d9.h>
#include <stdint.h>

#define LDB_HOST_PLUGIN_ABI_VERSION 1u
#define LDB_HOST_PLUGIN_QUERY_EXPORT "LDBHostPluginQuery"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LDBHostLogLevel {
  LDB_HOST_LOG_INFO = 0,
  LDB_HOST_LOG_WARNING = 1,
  LDB_HOST_LOG_ERROR = 2
} LDBHostLogLevel;

typedef void (WINAPI* LDBHostLogFn)(LDBHostLogLevel level, const char* message);

typedef struct LDBHostApiV1 {
  uint32_t struct_size;
  uint32_t abi_version;
  uint32_t host_bits;
  uint32_t reserved;
  const wchar_t* host_directory;
  const char* host_version;
  LDBHostLogFn log;
} LDBHostApiV1;

typedef struct LDBHostPresentInfoV1 {
  uint32_t struct_size;
  uint32_t is_swapchain_present;
  HWND destination_window_override;
  const RECT* source_rect;
  const RECT* destination_rect;
  const RGNDATA* dirty_region;
  DWORD flags;
} LDBHostPresentInfoV1;

typedef BOOL (WINAPI* LDBHostPluginLoadFn)(const LDBHostApiV1* host);
typedef void (WINAPI* LDBHostPluginDeviceFn)(IDirect3DDevice9* device);
typedef void (WINAPI* LDBHostPluginPresentFn)(IDirect3DDevice9* device, const LDBHostPresentInfoV1* present);
typedef void (WINAPI* LDBHostPluginBeforeResetFn)(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS* parameters, BOOL extended);
typedef void (WINAPI* LDBHostPluginAfterResetFn)(IDirect3DDevice9* device, HRESULT result, BOOL extended);
typedef void (WINAPI* LDBHostPluginShutdownFn)(void);

typedef struct LDBHostPluginV1 {
  uint32_t struct_size;
  uint32_t abi_version;
  const char* name;
  const char* version;
  LDBHostPluginLoadFn on_load;
  LDBHostPluginDeviceFn on_device_created;
  LDBHostPluginPresentFn on_before_present;
  LDBHostPluginBeforeResetFn on_before_reset;
  LDBHostPluginAfterResetFn on_after_reset;
  LDBHostPluginDeviceFn on_device_destroyed;
  LDBHostPluginShutdownFn on_shutdown;
} LDBHostPluginV1;

// The Host initializes out_plugin.struct_size before calling this export.
// A plugin returns FALSE when it does not support requested_abi.
typedef BOOL (WINAPI* LDBHostPluginQueryFn)(uint32_t requested_abi, LDBHostPluginV1* out_plugin);

#ifdef __cplusplus
}
#endif
