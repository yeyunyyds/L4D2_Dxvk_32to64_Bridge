// SPDX-License-Identifier: MIT
#include "steam_ime_chain.h"
#include <string>
#include <thread>
#include <cstdio>
#include <cstdlib>
static std::string logs;
static void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static void log(const char* line){logs+=line;logs+='\n';SetLastError(0xDEAD);}
static LRESULT CALLBACK echo(HWND window,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_APP+76){require(w==0x321 && l==0x654,"call/return observers preserve dispatch arguments");return 0x789;}
 return DefWindowProcW(window,msg,w,l);
}
static HWND makeWindow(){WNDCLASSW wc{};wc.lpfnWndProc=echo;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"ImeChainFixture";require(RegisterClassW(&wc)!=0,"register Unicode fixture");return CreateWindowW(wc.lpszClassName,L"IME chain fixture",WS_OVERLAPPED,0,0,100,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);}
int main(int argc,char** argv){
 require(argc==2,"mode");const std::string mode=argv[1];
 if(mode=="mta"){
  require(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)),"MTA init");
  {l4d2_overlay::ImeChain chain(true,true,true,log);require(!chain.ownsActivation() && !chain.pumpReady(),"apartment conflict does not activate or install pump");}
  require(logs.find("com_hr=0x80010106")!=std::string::npos,"apartment conflict evidence");CoUninitialize();
 }else{
  const bool activate=mode=="activate" || mode=="pump";const bool pump=mode=="pump";
  // Construct before the window, as in production. Do not infer system COM
  // ownership after IMM starts TSF internally.
  {l4d2_overlay::ImeChain chain(mode!="disabled",activate,pump,log);
   if(activate)require(chain.ownsActivation(),"real Windows TSF Activate succeeded");
   if(pump)require(chain.pumpReady(),"real TSF message pump available");
   HWND window=makeWindow();require(window!=nullptr,"Unicode window");
   SetLastError(12345);chain.bind(window);require(GetLastError()==12345,"bind preserves LastError");
   require(SendMessageW(window,WM_APP+76,0x321,0x654)==0x789,"real hook chain preserves return");
   if(mode=="disabled")require(logs.empty(),"default mode inert");
   else{
    logs.clear();chain.message(1,window,WM_KEYDOWN,0,0);chain.message(1,window,WM_CHAR,'A',0);
    chain.message(1,window,WM_CHAR,0x4E2D,0);chain.message(1,window,WM_IME_STARTCOMPOSITION);
    chain.message(1,window,WM_IME_COMPOSITION,0,GCS_RESULTSTR);chain.message(1,window,WM_IME_ENDCOMPOSITION);
    chain.sample("fixture",true);require(logs.find("stage=queue-return")!=std::string::npos && logs.find("key=1 ascii=1 unicode=1 dead=0 start=1 composition=1 result=1 end=1")!=std::string::npos,"classified stage counts");
    require(logs.find("manager_present=")!=std::string::npos && logs.find("document_present=")!=std::string::npos,"actual TSF focus evidence");
    std::string before=logs;std::thread foreign([&]{chain.sample("foreign",true);chain.message(1,window,WM_CHAR,'B');});foreign.join();require(logs==before,"foreign thread never queries owned TSF or alters counts");
   }
   // Use a private thread message to check the real pump preserves contents and
   // WM_QUIT, without asserting any real IME composition from synthetic messages.
   const DWORD thread=GetCurrentThreadId();MSG msg{};PeekMessageW(&msg,nullptr,0,0,PM_NOREMOVE);
   while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){}
   require(PostThreadMessageW(thread,WM_APP+77,0x123,0x456)!=FALSE,"post marker");
   bool got=false;for(unsigned n=0;n<100 && !got;++n){require(chain.getMessage(&msg)>0,"retrieve marker");got=msg.message==WM_APP+77;if(got)require(msg.wParam==0x123 && msg.lParam==0x456,"pump preserves message arguments");}
   require(got,"marker reached");PostQuitMessage(33);require(chain.getMessage(&msg)==0 && msg.message==WM_QUIT && msg.wParam==33,"pump preserves quit");
   require(DestroyWindow(window)!=FALSE,"window destruction");
  }
  if(activate)require(logs.find("event=deactivate")!=std::string::npos,"own activation paired");
 }
 std::printf("PASS IME chain mode=%s actual TSF lifetime/apartment and queue/quit, classified counters; no real IME composition tested\n",mode.c_str());
}
