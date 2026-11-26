// SPDX-License-Identifier: MIT
#include "d3d9_privatedata.h"
#include <array>
#include <cstdlib>
#include <iostream>
#define CHECK(expr) do { if (!(expr)) { std::cerr << "line " << __LINE__ << ": " << #expr << '\n'; std::abort(); } } while (false)
struct FakeUnknown final : IUnknown {
  ULONG refs = 1;
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override {
    if (out) { *out = nullptr; }
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
  ULONG STDMETHODCALLTYPE Release() override { CHECK(refs > 0); return --refs; }
};
int main() {
  const GUID key{0x454d3250, 1, 2, {3,4,5,6,7,8,9,10}};
  const GUID other{0x454d3251, 1, 2, {3,4,5,6,7,8,9,10}};
  using Cache = std::array<uint32_t, 3>;
  constexpr DWORD bytes = static_cast<DWORD>(sizeof(Cache));
  constexpr DWORD ptrBytes = static_cast<DWORD>(sizeof(IUnknown*));
  const Cache original{0x454d3250, 0x5bf93fb7, 23};
  FakeUnknown object;
  IUnknown* iface = &object;
  {
    PrivateDataFactory data;
    Cache input = original, output{};
    DWORD size = bytes;
    CHECK(data.getData(key, &output, &size) == D3DERR_NOTFOUND);
    CHECK(data.freeData(key) == D3DERR_NOTFOUND);
    CHECK(data.setData(key, &input, bytes, 0) == D3D_OK);
    input.fill(0);
    for (int i = 0; i < 10000; ++i) {
      size = bytes;
      CHECK(data.getData(key, &output, &size) == D3D_OK);
      CHECK(size == bytes && output == original);
    }
    size = 0;
    CHECK(data.getData(key, nullptr, &size) == D3DERR_MOREDATA);
    CHECK(size == bytes);
    CHECK(data.getData(key, nullptr, &size) == D3D_OK);
    output.fill(0xfeedface);
    size = 4;
    CHECK(data.getData(key, &output, &size) == D3DERR_MOREDATA);
    CHECK(size == bytes && output[0] == 0xfeedface && output[2] == 0xfeedface);
    CHECK(data.getData(key, &output, nullptr) == D3DERR_INVALIDCALL);
    CHECK(data.setData(key, nullptr, bytes, 0) == D3DERR_INVALIDCALL);
    CHECK(data.setData(key, &input, bytes, 2) == D3DERR_INVALIDCALL);
    CHECK(data.setData(key, &input, bytes, 0) == D3D_OK);
    size = bytes;
    CHECK(data.getData(key, &output, &size) == D3D_OK && output == input);
    CHECK(data.setData(other, &original, bytes, 0) == D3D_OK);
    CHECK(data.setData(key, iface, 1, D3DSPD_IUNKNOWN) == D3DERR_INVALIDCALL);
    CHECK(data.setData(key, iface, ptrBytes, D3DSPD_IUNKNOWN) == D3D_OK);
    CHECK(object.refs == 2);
    size = 0;
    CHECK(data.getData(key, nullptr, &size) == D3DERR_MOREDATA && object.refs == 2);
    IUnknown* value = nullptr;
    size = ptrBytes;
    CHECK(data.getData(key, &value, &size) == D3D_OK && value == iface && object.refs == 3);
    value->Release();
    CHECK(data.setData(key, iface, ptrBytes, D3DSPD_IUNKNOWN) == D3D_OK && object.refs == 2);
    CHECK(data.setData(key, &original, bytes, 0) == D3D_OK && object.refs == 1);
    CHECK(data.freeData(key) == D3D_OK);
    size = bytes;
    CHECK(data.getData(other, &output, &size) == D3D_OK && output == original);
    CHECK(data.setData(key, iface, ptrBytes, D3DSPD_IUNKNOWN) == D3D_OK);
    CHECK(data.freeData(key) == D3D_OK && object.refs == 1);
    CHECK(data.setData(key, iface, ptrBytes, D3DSPD_IUNKNOWN) == D3D_OK);
  }
  CHECK(object.refs == 1);
  std::cout << "PrivateData contract passed\n";
}
