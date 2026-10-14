// SPDX-License-Identifier: MIT
// Copyright (c) 2026 yeyunyyds.
// Problem cases informed by NPCodex Nightly Build; see docs/IPC-UPLOAD-SAFETY.md.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "util_commands.h"
#include "ati_texture_layout.h"
#include "actual_host_handle_lookup.h"

static void require(bool value, const char* text) {
  if (!value) { throw std::runtime_error(text); }
}
namespace l4d2_api_wait { template<class F> auto backend(F&& f) { return f(); } }
namespace l4d2_data { template<class... A> void backend(A...) {} }
namespace l4d2_color {
std::atomic<bool> enabled{false};
struct Digest { void add(const void*, size_t) {} };
template<class... A> void record(const char*, A...) {}
template<class... A> void upload(A...) {}
}
struct Logger { static void err(const std::string&) {} static void info(const std::string&) {} };
template<class... A> std::string format_string(const char*, A...) { return {}; }
std::unordered_map<uint32_t,uint32_t> gColorParents;
struct DecodeFailure {};
static Header rpcHeader;
static const uint32_t currentUID = 77;
struct DeviceBridge {
  struct Blob { std::vector<uint8_t> bytes; uint32_t length; };
  inline static std::vector<uint32_t> words;
  inline static std::vector<Blob> blobs;
  inline static size_t word = 0, blob = 0;
  inline static unsigned faults = 0, unpins = 0;
  inline static bool failRead = false;
  inline static size_t failBlob = SIZE_MAX;
  static uint32_t get_data() {
    if (word >= words.size()) { throw DecodeFailure{}; }
    return words[word++];
  }
  static uint32_t get_data(void** out) {
    if (failRead || blob == failBlob || blob >= blobs.size()) { ++faults; throw DecodeFailure{}; }
    auto& value = blobs[blob++]; *out = value.bytes.empty() ? nullptr : value.bytes.data(); return value.length;
  }
  template<size_t N> struct Packet { std::array<uint32_t,N> fields; uint32_t bytes=0; void* data=nullptr; };
  template<size_t N> static Packet<N> get_packet(bool payload=true) {
    Packet<N> packet{}; for (auto& field:packet.fields) { field=get_data(); }
    if (payload) { packet.bytes=get_data(&packet.data); } return packet;
  }
  static void reportFault(const char*, uint32_t, uint32_t) { ++faults; }
  static void faultChannel() { ++faults; }
  static void* reservedData(uint32_t, uint32_t, uint32_t) { return blobs.back().bytes.data(); }
  static void unpinRead(uint32_t) { ++unpins; }
};
struct BridgeReadFailure {};
namespace SharedHeap {
inline std::array<uint8_t,256> bytes;
inline size_t capacity=256;
static uint8_t* getBuf(uint32_t id) { return id==1 ? bytes.data() : nullptr; }
static size_t getAllocationSize(uint32_t id) { return id==1 ? capacity : 0; }
}
struct TestResource {
  D3DRESOURCETYPE type;
  explicit TestResource(D3DRESOURCETYPE t):type(t){}
  D3DRESOURCETYPE GetType() const {return type;}
};
struct TestBuffer : TestResource {
  std::array<uint8_t,256> bytes;
  HRESULT result=S_OK, unlockResult=S_OK;
  unsigned locks=0, unlocks=0;
  bool nullBits=false;
  TestBuffer() : TestResource(D3DRTYPE_VERTEXBUFFER) { bytes.fill(0xa5); }
  HRESULT Lock(UINT,UINT,void** output,DWORD) { ++locks; *output=nullBits?nullptr:bytes.data(); return result; }
  HRESULT Unlock() { ++unlocks; return unlockResult; }
};
struct TestSurface : TestResource {
  std::array<uint8_t,256> bytes;
  HRESULT result=S_OK, unlockResult=S_OK;
  int32_t pitch=4;
  D3DFORMAT format=D3DFMT_A8R8G8B8;
  unsigned width=1,height=1;
  unsigned locks=0, unlocks=0, descriptions=0;
  bool nullBits=false;
  TestSurface() : TestResource(D3DRTYPE_SURFACE) { bytes.fill(0xa5); }
  HRESULT LockRect(D3DLOCKED_RECT* output,const RECT*,DWORD) { ++locks; *output={pitch,nullBits?nullptr:bytes.data()}; return result; }
  HRESULT UnlockRect() { ++unlocks; return unlockResult; }
  HRESULT GetDesc(D3DSURFACE_DESC* output) { ++descriptions; *output={}; output->Width=width; output->Height=height; output->Format=format; return S_OK; }
};
struct TestVolume : TestResource {
  std::array<uint8_t,256> bytes;
  HRESULT result=S_OK, unlockResult=S_OK;
  int32_t pitch=12, slice=32;
  unsigned locks=0, unlocks=0;
  bool nullBits=false;
  TestVolume() : TestResource(D3DRTYPE_VOLUME) { bytes.fill(0xa5); }
  HRESULT LockBox(D3DLOCKED_BOX* output,const D3DBOX*,DWORD) { ++locks; *output={pitch,slice,nullBits?nullptr:bytes.data()}; return result; }
  HRESULT UnlockBox() { ++unlocks; return unlockResult; }
  HRESULT GetDesc(D3DVOLUME_DESC* output) { *output={}; output->Format=D3DFMT_A8R8G8B8; return S_OK; }
};
struct TestVolumeTexture : TestVolume {
  TestVolumeTexture(){type=D3DRTYPE_VOLUMETEXTURE;}
  HRESULT LockBox(UINT,D3DLOCKED_BOX* output,const D3DBOX* box,DWORD flags) { return TestVolume::LockBox(output,box,flags); }
  HRESULT UnlockBox(UINT) { return TestVolume::UnlockBox(); }
  HRESULT GetLevelDesc(UINT,D3DVOLUME_DESC* output) { return GetDesc(output); }
};
std::unordered_map<uint32_t,TestResource*> gpD3DResources;
std::unordered_map<uint32_t,TestVolume*> gpD3DVolumes;
#define GET_HND(name) const uint32_t name=rpcHeader.pHandle
#define PULL_U(name) const uint32_t name=DeviceBridge::get_data()
#define PULL_D(name) PULL_U(name)
#define PULL_OBJ(type,name) void* name##_raw=nullptr; const auto name##_len=DeviceBridge::get_data(&name##_raw); auto* name=static_cast<type*>(name##_raw)
#ifdef HOST_ROW_UPLOADS
#undef SEND_ALL_LOCK_DATA_AT_ONCE
#else
#define SEND_ALL_LOCK_DATA_AT_ONCE
#endif
#ifdef FIXED_UPLOAD_SOURCE
#include "upload_safety.h"
#endif
#include "actual_upload_formats.h"
using namespace Commands;
#include "actual_upload_dispatch.h"

static void reset(Commands::D3D9Command command, uint16_t flags=0) {
  rpcHeader={command,flags,0,1}; DeviceBridge::words.clear(); DeviceBridge::blobs.clear();
  DeviceBridge::word=DeviceBridge::blob=0; DeviceBridge::faults=DeviceBridge::unpins=0; DeviceBridge::failRead=false;
  DeviceBridge::failBlob=SIZE_MAX;
}
template<class T> static void object(const T& value) {
  std::vector<uint8_t> bytes(sizeof(value)); std::memcpy(bytes.data(),&value,sizeof(value));
  DeviceBridge::blobs.push_back({std::move(bytes),sizeof(value)});
}
static void payload(unsigned bytes,unsigned declared=UINT32_MAX) {
  // Bytes after declared length are sentinels, never part of the upload.
  DeviceBridge::blobs.push_back({std::vector<uint8_t>((std::max)(bytes,16u),0x3c),declared==UINT32_MAX?bytes:declared});
}
static void bufferCase(bool index,const std::string& mode) {
  TestBuffer target; target.type=index?D3DRTYPE_INDEXBUFFER:D3DRTYPE_VERTEXBUFFER;
  const bool reserved=mode=="reserved-lock-failure" || mode=="reserved-valid" || mode=="missing-reserved" || mode=="wrong-type-reserved";
  reset(index?Commands::IDirect3DIndexBuffer9_Unlock:Commands::IDirect3DVertexBuffer9_Unlock,reserved?Commands::DataIsReserved:0);
  gpD3DResources[1]=&target; DeviceBridge::words={0,16,0};
  if(reserved) { DeviceBridge::words.push_back(0); }
  payload(16);
  if(mode=="lock-failure" || mode=="reserved-lock-failure") { target.result=E_FAIL; }
  if(mode=="null-bits") { target.nullBits=true; }
  if(mode=="unlock-failure") { target.unlockResult=E_FAIL; }
  if(mode=="zero") { DeviceBridge::words[1]=0; DeviceBridge::blobs.back().length=0; }
  if(mode=="missing" || mode=="missing-reserved"){gpD3DResources.erase(1);}
  if(mode=="wrong-type" || mode=="wrong-type-reserved"){target.type=D3DRTYPE_SURFACE;}
  const auto entries=gpD3DResources.size();
  dispatchUpload();
  if(mode=="missing" || mode=="missing-reserved" || mode=="wrong-type" || mode=="wrong-type-reserved"){
    require(DeviceBridge::faults==1 && !target.locks && gpD3DResources.size()==entries,"invalid buffer handle mutated map or reached backend");
    require(DeviceBridge::unpins==unsigned(reserved),"invalid buffer handle leaked reserved pin");return;
  }
  require(target.locks==1,"buffer lock count");
  if(FAILED(target.result)) {
    require(DeviceBridge::faults==1 && !target.unlocks && target.bytes[0]==0xa5,"failed buffer Lock copied or unlocked");
    require(DeviceBridge::unpins==unsigned(reserved),"failed reserved Lock leaked pin");
  } else {
    require(target.unlocks==1,"successful buffer Lock not balanced");
    require(DeviceBridge::faults==unsigned(FAILED(target.unlockResult)||target.nullBits),"buffer unlock failure hidden");
    require(DeviceBridge::unpins==unsigned(reserved),"successful reserved upload leaked pin");
    require(target.bytes[0]==(mode=="zero"||target.nullBits?0xa5:0x3c),"buffer contents");
  }
}
static void surfaceCase(const std::string& mode) {
  TestSurface target; reset(Commands::IDirect3DSurface9_UnlockRect);gpD3DResources[1]=&target;
  const bool rgb=mode=="rgb24",dxt=mode=="dxt1"||mode=="dxt5",partial=mode=="partial";
  const unsigned logical=dxt?(mode=="dxt1"?8:16):(rgb?3:4);
  RECT rect=partial?RECT{2,3,3,5}:RECT{0,0,1,1};object(rect);
  const unsigned sourcePitch=dxt?logical:4;
  target.pitch=dxt?logical+8:4;
  DeviceBridge::words={0,uint32_t(dxt?(mode=="dxt1"?D3DFMT_DXT1:D3DFMT_DXT5):(rgb?D3DFMT_R8G8B8:D3DFMT_A8R8G8B8)),sourcePitch};payload(sourcePitch*(partial?2:1));
  if(mode=="lock-failure") { target.result=E_FAIL; }
  if(mode=="unlock-failure") { target.unlockResult=E_FAIL; }
  if(mode=="short") { DeviceBridge::blobs.back().length=3; }
  if(mode=="bad-pitch") { target.pitch=2; }
  if(mode=="negative-pitch") { target.pitch=-4; }
  if(mode=="null-bits") { target.nullBits=true; }
  if(mode=="missing"){gpD3DResources.erase(1);}
  if(mode=="wrong-type"){target.type=D3DRTYPE_VERTEXBUFFER;}
  if(mode=="malformed-ati-rect"){DeviceBridge::blobs[0].length=0;DeviceBridge::blobs[0].bytes.clear();DeviceBridge::words[1]=l4d2_ati::kAti1;}
  const auto entries=gpD3DResources.size();
  dispatchUpload();
  if(mode=="missing"||mode=="wrong-type"||mode=="malformed-ati-rect"){
    require(DeviceBridge::faults==1 && !target.locks && !target.descriptions && gpD3DResources.size()==entries,"invalid surface handle or RECT reached backend");return;
  }
  require(DeviceBridge::blob==2,"surface payload not consumed");
  if(mode=="short") { require(DeviceBridge::faults==1 && !target.locks && target.bytes[0]==0xa5,"short surface payload reached backend");return; }
  require(target.locks==1,"surface lock count");
  if(FAILED(target.result)) {
    require(DeviceBridge::faults==1 && !target.unlocks && target.bytes[0]==0xa5,"failed surface Lock copied or unlocked");
  } else {
    require(target.unlocks==1,"surface Lock not balanced");
    const bool invalid=mode=="bad-pitch"||mode=="negative-pitch"||target.nullBits;
    require(DeviceBridge::faults==unsigned(invalid||mode=="unlock-failure"),"surface error hidden");
    if(invalid) {require(target.bytes[0]==0xa5,"invalid pitch wrote pixels");}
    else {require(target.bytes[logical-1]==0x3c && target.bytes[partial?8:logical]==0xa5,"Host widened small mip into padding");
      if(partial) {require(target.bytes[7]==0x3c,"partial surface lost second row");}}
  }
  require(!target.descriptions,"disabled color observer called GetDesc");
}
static void volumeCase(bool texture,const std::string& mode) {
  TestVolume volume;TestVolumeTexture tex;TestVolume& target=texture?static_cast<TestVolume&>(tex):volume;
  reset(texture?Commands::IDirect3DVolumeTexture9_UnlockBox:Commands::IDirect3DVolume9_UnlockBox);
  gpD3DVolumes[1]=&volume;gpD3DResources[1]=&tex;
  if(texture) {DeviceBridge::words.push_back(0);}
  D3DBOX box{0,0,2,2,0,2};object(box);
  const uint32_t fields[]={0,4,2,2,2};DeviceBridge::words.insert(DeviceBridge::words.end(),std::begin(fields),std::end(fields));
#ifdef HOST_ROW_UPLOADS
  for(unsigned i=0;i<4;++i) {payload(8);}
  if(mode=="short") {DeviceBridge::blobs.back().length=7;}
#else
  payload(32);if(mode=="short") {DeviceBridge::blobs.back().length=31;}
#endif
  if(mode=="lock-failure") {target.result=E_FAIL;}
  if(mode=="unlock-failure") {target.unlockResult=E_FAIL;}
  if(mode=="bad-pitch") {target.pitch=4;}
  if(mode=="bad-slice") {target.slice=16;}
  if(mode=="null-bits") {target.nullBits=true;}
  if(mode=="decode-failure") {DeviceBridge::failBlob=2;}
  if(mode=="missing"){if(texture){gpD3DResources.erase(1);}else{gpD3DVolumes.erase(1);}}
  if(mode=="wrong-type"){target.type=D3DRTYPE_SURFACE;}
  const auto entries=texture?gpD3DResources.size():gpD3DVolumes.size();
  try {dispatchUpload();}
  catch(const DecodeFailure&) {
    require(mode=="decode-failure" && target.locks==1 && target.unlocks==1,"decode error leaked successful backend Lock");return;
  }
  if(mode=="missing"||mode=="wrong-type"){
    require(DeviceBridge::faults==1 && !target.locks && (texture?gpD3DResources.size():gpD3DVolumes.size())==entries,"invalid volume handle mutated map or reached backend");return;
  }
  if(mode=="short") {
    require(DeviceBridge::faults==1,"short volume accepted");
#ifndef HOST_ROW_UPLOADS
    require(!target.locks,"short packed volume reached backend");
#else
    require(target.locks==1 && target.unlocks==1,"short row upload leaked backend Lock");
#endif
    return;
  }
  require(target.locks==1,"volume lock count");
  if(FAILED(target.result)) {require(DeviceBridge::faults==1 && !target.unlocks && target.bytes[0]==0xa5,"failed volume Lock copied or unlocked");}
  else {
    require(target.unlocks==1,"successful volume Lock not balanced");
    const bool invalid=mode=="bad-pitch"||mode=="bad-slice"||target.nullBits;
    require(DeviceBridge::faults==unsigned(invalid||mode=="unlock-failure"),"volume error hidden");
    if(invalid) {require(target.bytes[0]==0xa5,"bad volume pitch wrote pixels");}
    else {require(target.bytes[0]==0x3c && target.bytes[8]==0xa5 && target.bytes[32]==0x3c && target.bytes[52]==0xa5,"volume rows/slices/padding changed");}
  }
}
static void sharedCases() {
  for(bool index:{false,true}) {
    for(const auto& mode:{"valid","end-empty","overrun","offset","missing"}) {
      TestBuffer target; target.type=index?D3DRTYPE_INDEXBUFFER:D3DRTYPE_VERTEXBUFFER;
      reset(index?Commands::IDirect3DIndexBuffer9_Unlock:Commands::IDirect3DVertexBuffer9_Unlock,Commands::DataInSharedHeap);
      gpD3DResources[1]=&target; SharedHeap::capacity=16; SharedHeap::bytes.fill(0x7b);
      unsigned offset=4,bytes=12,id=1;
      if(std::string(mode)=="end-empty"){offset=16;bytes=0;}
      if(std::string(mode)=="overrun"){bytes=13;}
      if(std::string(mode)=="offset"){offset=17;bytes=0;}
      if(std::string(mode)=="missing"){id=999;}
      DeviceBridge::words={offset,bytes,0,id};
      dispatchUpload();
      const bool valid=std::string(mode)=="valid"||std::string(mode)=="end-empty";
      require(target.locks==unsigned(valid)&&target.unlocks==unsigned(valid),"shared buffer bounds reached backend");
      require(DeviceBridge::faults==unsigned(!valid),"shared buffer failure hidden");
      require(target.bytes[0]==(std::string(mode)=="valid"?0x7b:0xa5),"shared buffer contents changed");
    }
  }
  for(const auto& mode:{"valid","short","missing"}) {
    TestSurface target;target.width=2;target.height=2;
    reset(Commands::IDirect3DSurface9_UnlockRect,Commands::DataInSharedHeap);gpD3DResources[1]=&target;
    RECT rect{1,0,2,2};object(rect);
    SharedHeap::capacity=std::string(mode)=="short"?15:16;SharedHeap::bytes.fill(0x7b);
    DeviceBridge::words={0,uint32_t(D3DFMT_A8R8G8B8),8,std::string(mode)=="missing"?999u:1u};
    dispatchUpload();const bool valid=std::string(mode)=="valid";
    require(target.locks==unsigned(valid)&&target.unlocks==unsigned(valid),"partial shared surface bounds reached backend");
    require(DeviceBridge::faults==unsigned(!valid),"partial shared surface failure hidden");
    if(valid){require(target.bytes[0]==0x7b&&target.bytes[7]==0x7b&&target.bytes[8]==0xa5,"partial shared surface padding changed");}
  }
  for(const auto format:{l4d2_ati::kAti1,l4d2_ati::kAti2}) for(const auto& mode:{"valid","short","missing"}) {
    TestSurface target;target.width=4;target.height=4;target.format=static_cast<D3DFORMAT>(format);target.pitch=4;
    reset(Commands::IDirect3DSurface9_UnlockRect,Commands::DataInSharedHeap);gpD3DResources[1]=&target;
    RECT rect{0,0,4,4};object(rect);
    const unsigned bytes=format==l4d2_ati::kAti1?8:16;
    SharedHeap::capacity=std::string(mode)=="short"?bytes-1:bytes;SharedHeap::bytes.fill(0x7b);
    DeviceBridge::words={0,format,bytes,std::string(mode)=="missing"?999u:1u};
    dispatchUpload();const bool valid=std::string(mode)=="valid";
    require(target.locks==unsigned(valid)&&target.unlocks==unsigned(valid),"ATI shared allocation bounds reached backend");
    require(DeviceBridge::faults==unsigned(!valid),"ATI shared allocation failure hidden");
    require(target.bytes[0]==(valid?0x7b:0xa5),"ATI shared allocation copied invalid bytes");
  }
}
int main(int argc,char** argv) {
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
  try {
    const std::string mode=argc>1?argv[1]:"all";
    if(mode=="handles") {
      for(bool index:{false,true})for(const auto& c:{"missing","wrong-type","missing-reserved","wrong-type-reserved"})bufferCase(index,c);
      for(const auto& c:{"missing","wrong-type","malformed-ati-rect"})surfaceCase(c);
      volumeCase(false,"missing");volumeCase(true,"missing");volumeCase(true,"wrong-type");
    }
    else if(mode=="shared") {sharedCases();}
    else if(mode=="lock-failure-vb") {bufferCase(false,"lock-failure");}
    else if(mode=="lock-failure-ib") {bufferCase(true,"lock-failure");}
    else if(mode=="lock-failure-surface") {surfaceCase("lock-failure");}
    else if(mode=="lock-failure-volume") {volumeCase(false,"lock-failure");}
    else if(mode=="lock-failure-volume-texture") {volumeCase(true,"lock-failure");}
    else if(mode=="lock-failure") {bufferCase(false,mode);surfaceCase(mode);volumeCase(false,mode);}
    else if(mode=="small-mip") {surfaceCase("valid");surfaceCase("rgb24");}
    else {
      for(bool index:{false,true}) for(const auto& test:{"valid","lock-failure","reserved-valid","reserved-lock-failure","unlock-failure","null-bits","zero"}) {bufferCase(index,test);}
      for(const auto& test:{"valid","rgb24","dxt1","dxt5","partial","lock-failure","unlock-failure","short","bad-pitch","negative-pitch","null-bits"}) {surfaceCase(test);}
      for(bool texture:{false,true}) for(const auto& test:{"valid","lock-failure","unlock-failure","short","bad-pitch","bad-slice","null-bits"}) {volumeCase(texture,test);}
#ifdef HOST_ROW_UPLOADS
      volumeCase(false,"decode-failure");volumeCase(true,"decode-failure");
#endif
    }
    std::printf("HOST_UPLOAD_SAFETY_PASS mode=%s bits=%u\n",mode.c_str(),unsigned(sizeof(void*)*8));return 0;
  } catch(const std::exception& e) {std::fprintf(stderr,"HOST_UPLOAD_SAFETY_FAIL %s\n",e.what());return 2;}
  catch(...) {std::fprintf(stderr,"HOST_UPLOAD_SAFETY_FAIL unexpected decoding exception\n");return 2;}
}
