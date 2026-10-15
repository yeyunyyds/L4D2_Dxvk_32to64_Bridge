// SPDX-License-Identifier: MIT
#include <windows.h>
extern "C" __declspec(dllexport) const char* updaterFixtureIdentity(){return "l4d2-1.3.2+1111111111111111";}
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}
