"""Extract actual Client updater WndProc/recovery logic, without duplicating it."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
source = root / ".deps/dxvk-remix/bridge/src/client"
window = (source / "window.cpp").read_text()
start = window.index("  // Local updater test only:")
end = window.index("  if(handleSteamCaptureNotification", start)
guard = (source / "d3d9_lss.cpp").read_text()
start_guard = guard.index("static bool updaterRecoveryPending()")
end_guard = guard.index("HRESULT LssDirect3DCreate9Ex", start_guard)
prelude = r'''
#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstdlib>
static void check(bool ok,const char* reason){if(!ok){std::fprintf(stderr,"%s\n",reason);std::exit(2);}}
static unsigned sent=0,dialogs=0;
struct Channel{ bool send(const char* name,uint32_t,uint32_t){check(std::string(name)=="LDB_UPDATER_LOCAL_TEST_REQUEST_V1","wrong request");++sent;return true;} } channel;
static Channel* gpServerMessageChannel=&channel;
struct Server{ DWORD GetProcessId(){return 123;} } server;
static Server* gpServer=&server;
static std::filesystem::path fakeHost;
static std::filesystem::path GetServerExecutablePath(){return fakeHost;}
static int WINAPI dialog(HWND,LPCWSTR,LPCWSTR,UINT){++dialogs;return IDOK;}
#define MessageBoxW dialog
'''
tail = r'''
int main(){
  BYTE keys[256]{};GetKeyboardState(keys);
  HWND window=CreateWindowW(L"STATIC",L"updater-test",0,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
  check(window!=nullptr,"window creation");
  check(dispatch(window,WM_KEYDOWN,VK_F10,0)==99 && sent==0,"plain F10 changed");
  keys[VK_CONTROL]=0x80;keys[VK_SHIFT]=0x80;SetKeyboardState(keys);
  check(dispatch(window,WM_KEYDOWN,VK_F10,0)==0 && sent==0,"test keydown leaked to game");
  check(dispatch(window,WM_KEYDOWN,VK_F10,0)==0 && sent==0,"repeat leaked");
  keys[VK_CONTROL]=0;keys[VK_SHIFT]=0;SetKeyboardState(keys);
  check(dispatch(window,WM_KEYUP,VK_F10,0)==0 && sent==1,"release order lost request");
  check(dispatch(window,WM_KEYUP,VK_F10,0)==99 && sent==1,"duplicate release resent");
  UINT close=RegisterWindowMessageW(L"LDB_UPDATER_LOCAL_TEST_CLOSE_V1");
  check(dispatch(window,close,124,0)==99,"foreign close accepted");
  check(dispatch(window,close,123,0)==0,"normal close not posted");
  MSG m{};check(PeekMessageW(&m,window,WM_CLOSE,WM_CLOSE,PM_REMOVE)!=FALSE,"missing asynchronous close");
  DestroyWindow(window);
  wchar_t temp[MAX_PATH]{};check(GetTempPathW(MAX_PATH,temp)>0,"temporary directory");
  auto folder=std::filesystem::path(temp)/(L"ldb-updater-client-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directory(folder);
  fakeHost=folder/L"L4D2Bridge64.exe";
  SetLastError(0x1234);check(!updaterRecoveryPending() && dialogs==0 && GetLastError()==0x1234,"absent recovery state changed semantics");
  auto journal=folder/L"LDBupdate-transaction.json";HANDLE f=CreateFileW(journal.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,0,nullptr);check(f!=INVALID_HANDLE_VALUE,"journal fixture");CloseHandle(f);
  SetLastError(0x1234);check(updaterRecoveryPending() && dialogs==1 && GetLastError()==0x1234,"unfinished journal did not block");
  std::filesystem::remove_all(folder);std::puts("UPDATER_CLIENT_PASS production-hotkey-close-recovery");return 0;
}
'''
Path(sys.argv[1]).write_text(prelude + guard[start_guard:end_guard] +
                           "static LRESULT dispatch(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam){\n(void)lParam;\n" +
                           window[start:end] + "return 99;\n}\n" + tail, encoding="utf-8")
