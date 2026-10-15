// SPDX-License-Identifier: MIT
// Native cross-bitness fixture for the actual Host coordinator and updater.
#include <windows.h>
#include <fstream>
#include <string>
static std::string mode;
static int WINAPI fixtureDialog(HWND,LPCWSTR,LPCWSTR,UINT flags){
  if((flags&MB_TYPEMASK)==MB_YESNO)return mode=="decline"?IDNO:IDYES;
  if((flags&MB_TYPEMASK)==MB_OKCANCEL)return mode=="cancel"?IDCANCEL:IDOK;
  return IDOK;
}
#define MessageBoxW fixtureDialog
#include "update_test_coordinator.h"
#undef MessageBoxW
static const char identity[]="l4d2-1.3.2+1111111111111111";
static LRESULT CALLBACK gameWindow(HWND window,UINT message,WPARAM w,LPARAM l){
  if(message==RegisterWindowMessageW(ldb_update_test::closeName)){
    if(mode!="linger")PostMessageW(window,WM_CLOSE,0,0);return 0;
  }
  if(message==WM_CLOSE){DestroyWindow(window);return 0;}
  if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
  return DefWindowProcW(window,message,w,l);
}
int main(int argc,char** argv){
  if(argc<3)return 2;mode=argv[2];
  if(std::string(argv[1])=="game"){
    WNDCLASSW cls{};cls.lpfnWndProc=gameWindow;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"LDBUpdaterFixture";
    if(!RegisterClassW(&cls))return 3;
    HWND window=CreateWindowW(cls.lpszClassName,L"LDB fixture",0,0,0,32,32,nullptr,nullptr,cls.hInstance,nullptr);if(!window)return 4;
    std::ofstream("game-window.txt")<<reinterpret_cast<uintptr_t>(window)<<" "<<identity;
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;
  }
  if(argc!=5)return 5;
  DWORD pid=static_cast<DWORD>(std::stoul(argv[3]));HWND window=reinterpret_cast<HWND>(static_cast<uintptr_t>(std::stoull(argv[4])));
  ldb_update_test::Handle game(OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,pid));if(game.h==nullptr)return 6;
  ldb_update_test::run(game.h,window,identity,"11111111111111111111111111111111",[](const char* line){std::ofstream("host-events.txt",std::ios::app)<<line<<"\n";});
  return 0;
}
