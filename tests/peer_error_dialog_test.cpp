// SPDX-License-Identifier: MIT
#include "peer_error_dialog.h"

#include <iostream>
#include <string>

int main() {
  const std::string report =
    "process_path=C:\\game\\L4D2Bridge64.exe\n"
    "preceding_bridge_fault reason=reply-deadline command=17 uid=44 tick_ms=123\n"
    "exception_code=0xc0000005 exception_name=EXCEPTION_ACCESS_VIOLATION\n"
    "access_type=read target_address=0x8\n"
    "current_command=CreateTexture opcode=17\n"
    "client_call=IDirect3DDevice9::CreateTexture\n"
    "studio_model kind=active model=\"models/test.mdl\"\n"
    "frame0 address=0x123 module=studiorender.dll offset=0x6896f\n"
    "termination=TerminateProcess exit_code=0xe04c3441\n";
  const auto summary = l4d2_peer_error::summarizeForTest(report);
  for (const auto* required : {"reply-deadline", "0xc0000005", "access_type=read",
                               "current_command=CreateTexture", "client_call=IDirect3DDevice9",
                               "models/test.mdl", "studiorender.dll", "termination=TerminateProcess"}) {
    if (summary.find(required) == std::string::npos) { return 1; }
  }
  if (summary.find("process_path=") != std::string::npos) { return 2; }
  std::cout << "peer-error-summary-ok\n";
  return 0;
}
