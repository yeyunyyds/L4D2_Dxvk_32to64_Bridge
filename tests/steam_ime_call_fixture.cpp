// SPDX-License-Identifier: MIT
#include <windows.h>
#include <imm.h>
#include <cstdint>
extern "C" __declspec(dllexport) uintptr_t ProbeIme(unsigned api,HWND window,HIMC context,DWORD* conversion,DWORD* sentence,DWORD argument) {
  switch(api){
  case 0:return reinterpret_cast<uintptr_t>(ImmCreateContext());
  case 1:return static_cast<uintptr_t>(ImmDestroyContext(context));
  case 2:return reinterpret_cast<uintptr_t>(ImmAssociateContext(window,context));
  case 3:return reinterpret_cast<uintptr_t>(ImmGetContext(window));
  case 4:return static_cast<uintptr_t>(ImmGetConversionStatus(context,conversion,sentence));
  case 5:return static_cast<uintptr_t>(ImmGetOpenStatus(context));
  case 6:return static_cast<uintptr_t>(ImmSetOpenStatus(context,static_cast<BOOL>(argument)));
  case 7:return reinterpret_cast<uintptr_t>(GetKeyboardLayout(argument));
  case 8:return static_cast<uintptr_t>(ImmGetProperty(reinterpret_cast<HKL>(context),argument));
  case 9:return static_cast<uintptr_t>(ImmGetCompositionStringW(context,argument,conversion,sizeof(DWORD)));
  case 10:return static_cast<uintptr_t>(ImmGetCompositionStringA(context,argument,conversion,sizeof(DWORD)));
  default:return 0;
  }
}
