// Host network policy native regression.
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

#include <cstdio>
#include <cstdlib>

#include "host_network_policy.h"

static void require(bool condition, const char* message) {
  if (!condition) {
    std::fprintf(stderr, "FAIL host-network-policy: %s error=%d\n", message, WSAGetLastError());
    std::exit(2);
  }
}

int main() {
  WSADATA data {};
  require(WSAStartup(MAKEWORD(2, 2), &data) == 0, "WSAStartup");

  sockaddr_in v4 {};
  v4.sin_family = AF_INET;
  v4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  v4.sin_port = htons(443);
  require(!l4d2_host_network::shouldBlockTcp443(reinterpret_cast<sockaddr*>(&v4), sizeof(v4), false), "disabled IPv4");
  require(l4d2_host_network::shouldBlockTcp443(reinterpret_cast<sockaddr*>(&v4), sizeof(v4), true), "enabled IPv4");
  v4.sin_port = htons(80);
  require(!l4d2_host_network::shouldBlockTcp443(reinterpret_cast<sockaddr*>(&v4), sizeof(v4), true), "IPv4 port 80");

  sockaddr_in6 v6 {};
  v6.sin6_family = AF_INET6;
  v6.sin6_addr = in6addr_loopback;
  v6.sin6_port = htons(443);
  require(l4d2_host_network::shouldBlockTcp443(reinterpret_cast<sockaddr*>(&v6), sizeof(v6), true), "enabled IPv6");
  require(!l4d2_host_network::shouldBlockTcp443(reinterpret_cast<sockaddr*>(&v6), sizeof(v6) - 1, true), "short IPv6");

  require(l4d2_host_network::installTcp443FailFast(true), "install");
  SOCKET socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
  require(socket != INVALID_SOCKET, "socket");
  v4.sin_port = htons(443);
  WSASetLastError(0);
  require(connect(socket, reinterpret_cast<sockaddr*>(&v4), sizeof(v4)) == SOCKET_ERROR
    && WSAGetLastError() == WSAENETUNREACH, "connect fail-fast");
  WSASetLastError(0);
  require(WSAConnect(socket, reinterpret_cast<sockaddr*>(&v4), sizeof(v4), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR
    && WSAGetLastError() == WSAENETUNREACH, "WSAConnect fail-fast");

  GUID id = WSAID_CONNECTEX;
  LPFN_CONNECTEX connectEx = nullptr;
  DWORD bytes = 0;
  require(WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof(id), &connectEx,
    sizeof(connectEx), &bytes, nullptr, nullptr) == 0 && connectEx, "ConnectEx lookup");
  WSASetLastError(0);
  require(!connectEx(socket, reinterpret_cast<sockaddr*>(&v4), sizeof(v4), nullptr, 0, nullptr, nullptr)
    && WSAGetLastError() == WSAENETUNREACH, "ConnectEx fail-fast");
  require(l4d2_host_network::blockedCount() == 3, "blocked count");

  closesocket(socket);
  WSACleanup();
  std::puts("PASS host-network-policy IPv4/IPv6 connect, WSAConnect and ConnectEx TCP443 fail-fast");
  return 0;
}
