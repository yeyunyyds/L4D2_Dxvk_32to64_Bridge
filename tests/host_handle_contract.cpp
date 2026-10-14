// SPDX-License-Identifier: MIT
#include <windows.h>
#include <d3d9.h>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <unordered_map>
#include <cassert>
static void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Resource{unsigned calls=0;};
std::unordered_map<uint32_t,Resource*> gpD3DDevices;
struct Query{unsigned issues=0,releases=0;HRESULT Issue(DWORD){++issues;return S_OK;}ULONG Release(){++releases;return 0;}};
std::unordered_map<uint32_t,Query*> gpD3DQuery;
struct Header{uint32_t pHandle=1,command=1;}rpcHeader;
constexpr uint32_t currentUID=11;
struct DeviceBridge{inline static unsigned faults=0;static void reportFault(const char*,uint32_t,uint32_t){++faults;}static DWORD get_data(){return D3DISSUE_END;}};
static HRESULT reply=E_FAIL;
#define PULL(type,name) const auto name=static_cast<type>(DeviceBridge::get_data())
#define SEND_OPTIONAL_SERVER_RESPONSE(hr,uid) do{check(uid==currentUID,"reply UID changed");reply=hr;}while(0)
static void safeDestroy(Query* q,uint32_t){while(q&&q->Release()>0);}
#include "actual_host_handles.h"
int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 try{
  Resource first,second;
  macroDispatch();check(DeviceBridge::faults==1&&gpD3DDevices.empty(),"missing device inserted phantom entry");
  gpD3DDevices[1]=&first;macroDispatch();check(first.calls==1&&DeviceBridge::faults==1,"valid device dispatch changed");
  gpD3DDevices.erase(1);macroDispatch();check(DeviceBridge::faults==2&&gpD3DDevices.empty(),"stale device dispatched");
  gpD3DDevices[1]=&second;macroDispatch();check(first.calls==1&&second.calls==1,"recreated device reached old object");
  gpD3DDevices[1]=nullptr;macroDispatch();check(DeviceBridge::faults==3&&gpD3DDevices.size()==1,"null device entry accepted");
  Query query;gpD3DQuery[1]=&query;
  dispatchQueryIssue();check(reply==S_OK&&query.issues==1,"valid Query Issue changed");
  dispatchQueryDestroy();check(query.releases==1&&gpD3DQuery.empty(),"Query destroy changed");
  dispatchQueryDestroy();check(query.releases==1&&gpD3DQuery.empty(),"repeated Query destroy changed map");
  dispatchQueryIssue();check(reply==D3DERR_INVALIDCALL&&query.issues==1&&gpD3DQuery.empty(),"stale Query Issue reached backend");
  std::puts("HOST_HANDLES_PASS missing/stale/null/recreated device, Query Issue/destroy/repeat/UID");return 0;
 }catch(const std::exception& e){std::printf("FAIL: %s\n",e.what());return 1;}
}
