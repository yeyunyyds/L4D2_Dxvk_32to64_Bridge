// SPDX-License-Identifier: MIT
#include "steam_ime_native.h"
#include <cstdio>
#include <cstdlib>
#include <string>
static void require(bool ok, const char* reason) {
  if (!ok) { std::fprintf(stderr, "%s error=%lu\n", reason, GetLastError()); std::exit(1); }
}
int main(int argc, char** argv) {
  require(argc == 2, "test mode");
  const std::string mode = argv[1];
  if (mode == "disabled") {
    const HMODULE ctf = GetModuleHandleW(L"msctf.dll"); unsigned logs = 0;
    SetLastError(0x1234);
    { l4d2_overlay::ImeThread ime(false, [&](const char*) { ++logs; });
      require(!ime.active() && ime.comResult() == E_NOTIMPL && GetLastError() == 0x1234, "disabled init is inert"); }
    require(GetLastError() == 0x1234 && logs == 0 && GetModuleHandleW(L"msctf.dll") == ctf, "disabled teardown is inert");
  } else {
    const bool mta = mode == "mta";
    require(mta || mode == "sta", "known apartment mode");
    require(SUCCEEDED(CoInitializeEx(nullptr, mta ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED)), "foreign COM owner");
    SetLastError(0x1234);
    { l4d2_overlay::ImeThread ime(true);
      require(GetLastError() == 0x1234, "init preserves LastError");
      if (mta) { require(ime.comResult() == RPC_E_CHANGED_MODE && !ime.active(), "foreign MTA is untouched"); }
      else { require(ime.comResult() == S_FALSE && ime.active(), "existing STA activation"); }
    }
    require(GetLastError() == 0x1234, "teardown preserves LastError");
    APTTYPE apartment; APTTYPEQUALIFIER qualifier;
    require(SUCCEEDED(CoGetApartmentType(&apartment, &qualifier)), "foreign COM remains initialized");
    require(mta ? apartment == APTTYPE_MTA : (apartment == APTTYPE_STA || apartment == APTTYPE_MAINSTA), "foreign apartment retained");
    CoUninitialize();
  }
  std::printf("IME_NATIVE_PASS mode=%s bits=%zu\n", mode.c_str(), sizeof(void*) * 8); return 0;
}
