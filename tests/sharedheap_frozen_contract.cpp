// SPDX-License-Identifier: MIT
// Frozen main source, unchanged API and real Windows mappings.
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <stdexcept>
#include "util_sharedheap.h"
#include "util_guid.h"
bridge_util::Guid gUniqueIdentifier;
int main() {
 try {
  using bridge_util::SharedHeap;
  SharedHeap::init(); SharedHeap::addNewHeapSegment(262144);
  SharedHeap::allocate(1,0);
  if (SharedHeap::getBuf(999)) throw std::runtime_error("unknown heap lookup resolved to allocated data");
  return 0;
 }catch(const std::exception& e){std::printf("FAIL: %s\n",e.what());return 1;}
}
