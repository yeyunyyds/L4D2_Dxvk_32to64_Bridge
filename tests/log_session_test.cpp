// SPDX-License-Identifier: MIT
#include "log_session.h"

#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

int main() {
  const auto root = std::filesystem::temp_directory_path() /
    (L"LDB-session-\u8def\u5f84-" + std::to_wstring(GetCurrentProcessId()));
  const auto logs = root / L"L4D2BridgeLogs";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(logs, error);
  if (error) { return 1; }
  for (int i = 0; i < 4; ++i) {
    const auto old = logs / (L"old-session-" + std::to_wstring(i));
    std::filesystem::create_directories(old, error);
    std::filesystem::last_write_time(old, std::filesystem::file_time_type::clock::now() +
      std::chrono::seconds(i), error);
  }
  const auto active = logs / L"old-session-0";
  const auto activeMarker = active / L"session-active.lock";
  const auto activeEvidence = active / L"must-survive.txt";
  std::ofstream(activeEvidence) << "active";
  HANDLE activeLock = CreateFileW(activeMarker.c_str(), GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (activeLock == INVALID_HANDLE_VALUE) { return 2; }
  const std::string current = "485f03fd-6e68-49a0-adc1-fcda464fbebf";
  if (!l4d2_log_session::initialize(root, current, 3, true, "client", "test-build")) { return 3; }
  if (l4d2_log_session::directory() != logs / L"485f03fd-6e68-49a0-adc1-fcda464fbebf") { return 4; }
  size_t directories = 0;
  for (const auto& entry : std::filesystem::directory_iterator(logs)) {
    if (entry.is_directory()) { ++directories; }
  }
  if (directories != 3 || !std::filesystem::exists(l4d2_log_session::file(L"session-info.txt"))) { return 5; }
  if (!std::filesystem::exists(activeEvidence)) { return 6; }
  std::ifstream info(l4d2_log_session::file(L"session-info.txt"));
  const std::string content((std::istreambuf_iterator<char>(info)), std::istreambuf_iterator<char>());
  if (content.find("role=client") == std::string::npos || content.find("build=test-build") == std::string::npos) { return 7; }
  CloseHandle(activeLock);
  std::filesystem::remove_all(root, error);
  std::cout << "log-session-ok\n";
  return 0;
}
