// SPDX-License-Identifier: MIT
#include "steam_window_diagnostics.h"
#include <vector>
#include <string>
#include <cstdlib>
using Diagnostic=l4d2_steam_window::Diagnostics;
using Probe=uintptr_t(*)(unsigned,HWND,HIMC,DWORD*,DWORD*,DWORD);
static std::vector<std::string> logs;
static unsigned calls[9]{};
static HWND window=nullptr;
static HIMC context=reinterpret_cast<HIMC>(uintptr_t(0x1234));
static HIMC previous=reinterpret_cast<HIMC>(uintptr_t(0x5678));
static void require(bool ok){if(!ok){std::fprintf(stderr,"IME original-call fixture assertion failed\n");std::exit(2);}}
static void logLine(const char* text){logs.emplace_back(text);SetLastError(0xDEAD);}
static void mark(unsigned api){++calls[api];SetLastError(0x100+api);}
static HIMC WINAPI create(){mark(0);return context;}
static BOOL WINAPI destroy(HIMC ctx){require(ctx==context);mark(1);return 7;}
static HIMC WINAPI associate(HWND w,HIMC ctx){require(w==window && ctx==context);mark(2);return previous;}
static HIMC WINAPI get(HWND w){require(w==window);mark(3);return context;}
static BOOL WINAPI conversion(HIMC ctx,LPDWORD c,LPDWORD s){require(ctx==context && c && s);*c=0x10;*s=0x20;mark(4);return 7;}
static BOOL WINAPI open(HIMC ctx){require(ctx==context);mark(5);return 0;}
static BOOL WINAPI setOpen(HIMC ctx,BOOL value){require(ctx==context && value==1);mark(6);return 7;}
static HKL WINAPI layout(DWORD thread){require(thread==99);mark(7);return reinterpret_cast<HKL>(uintptr_t(0x8040804));}
static DWORD WINAPI property(HKL h,DWORD index){require(h==reinterpret_cast<HKL>(context) && index==99);mark(8);return 0x40000;}
static void** findSlot(HMODULE module, const char* name) {
  const auto base = reinterpret_cast<BYTE*>(module);
  const auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  const auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
  auto descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
  for (; descriptor->Name; ++descriptor) {
    if (!descriptor->OriginalFirstThunk) continue;
    auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->OriginalFirstThunk);
    auto slots = reinterpret_cast<IMAGE_THUNK_DATA*>(base + descriptor->FirstThunk);
    for (; names->u1.AddressOfData; ++names, ++slots) {
      if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)
        && std::strcmp(reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData)->Name, name) == 0) return reinterpret_cast<void**>(&slots->u1.Function);
    }
  }
  return nullptr;
}
static void replace(void** slot, void* pointer) {
  require(slot != nullptr); DWORD protection = 0, ignored = 0;
  require(VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &protection) != FALSE);
  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot), pointer);
  require(VirtualProtect(slot, sizeof(void*), protection, &ignored) != FALSE);
}

int main(int argc,char** argv){
 require(argc==3);const bool enabled=std::string(argv[2])=="enabled";
 HMODULE module=LoadLibraryA(argv[1]);require(module!=nullptr);
 auto probe=reinterpret_cast<Probe>(GetProcAddress(module,"ProbeIme"));require(probe!=nullptr);
 window=CreateWindowW(L"STATIC",L"IME call observer",WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);require(window!=nullptr);
 const char* names[]={"ImmCreateContext","ImmDestroyContext","ImmAssociateContext","ImmGetContext","ImmGetConversionStatus","ImmGetOpenStatus","ImmSetOpenStatus","GetKeyboardLayout","ImmGetProperty"};
 void* originals[]={reinterpret_cast<void*>(&create),reinterpret_cast<void*>(&destroy),reinterpret_cast<void*>(&associate),reinterpret_cast<void*>(&get),reinterpret_cast<void*>(&conversion),reinterpret_cast<void*>(&open),reinterpret_cast<void*>(&setOpen),reinterpret_cast<void*>(&layout),reinterpret_cast<void*>(&property)};
 for(unsigned i=0;i<9;++i)replace(findSlot(module,names[i]),originals[i]);
 const wchar_t* name=sizeof(void*)==8?L"ime-calls-x64.dll":L"ime-calls-x86.dll";
 SetLastError(12345);Diagnostic::poll(true,window,window,logLine,name,false,"host",enabled);require(GetLastError()==12345);
 for(unsigned api=0;api<9;++api){
  DWORD c=0,s=0;const uintptr_t expected[]={reinterpret_cast<uintptr_t>(context),7,reinterpret_cast<uintptr_t>(previous),reinterpret_cast<uintptr_t>(context),7,0,7,0x8040804,0x40000};
  for(unsigned n=0;n<1000;++n){require(probe(api,window,context,&c,&s,api==6?1:99)==expected[api]);require(GetLastError()==0x100+api);}
  require(calls[api]==1000);if(api==4)require(c==0x10 && s==0x20);
 }
 unsigned seen=0;for(const auto& line:logs)if(line.find("L4D2_STEAM_IME_CALL api=")!=std::string::npos){++seen;require(line.find("presenter_thread=")!=std::string::npos && line.find("argument=")!=std::string::npos);}
 require(enabled ? seen>=9 && seen<=4096 : seen==0);
 Diagnostic::stop();for(unsigned i=0;i<9;++i)require(*findSlot(module,names[i])==originals[i]);
 require(DestroyWindow(window)!=FALSE);
 std::puts("PASS Steam IMM IAT observer x86/x64 ABI, exact originals, real arguments/results/buffers/LastError, disabled, bounded logs, restoration; no real composition tested");
}
