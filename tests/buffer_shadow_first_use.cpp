// Supplemental equal-work allocation + first full write. Both variants execute
// the actual production allocation API; no COM or upload capture is involved.
#define LDB_PRESSURE_NATIVE_ALLOCATOR
#include "buffer_contract_support.h"
#include <iomanip>
uint64_t wall() {
  LARGE_INTEGER c {}, f {}; QueryPerformanceCounter(&c); QueryPerformanceFrequency(&f);
  return uint64_t(c.QuadPart / f.QuadPart) * 1000000000ull + uint64_t(c.QuadPart % f.QuadPart) * 1000000000ull / uint64_t(f.QuadPart);
}
uint64_t cpu() {
  FILETIME a {}, b {}, k {}, u {};
  if (!GetProcessTimes(GetCurrentProcess(), &a, &b, &k, &u)) std::terminate();
  return (((uint64_t(k.dwHighDateTime) << 32) | k.dwLowDateTime)
    + ((uint64_t(u.dwHighDateTime) << 32) | u.dwLowDateTime)) * 100;
}
int main(int argc, char** argv) {
  if (argc != 2) return 1;
  const bool small = std::string(argv[1]) == "small";
  const unsigned pairs = small ? 16384 : 128;
  std::vector<std::pair<ShadowOwner, size_t>> allocations;
  allocations.reserve(pairs * 2);
  auto beginCpu = cpu(), begin = wall();
  for (unsigned i = 0; i < pairs; ++i) {
    for (size_t bytes : {size_t(small ? 64 * 1024 : 8 * 1024 * 1024), size_t(small ? 16 * 1024 : 2 * 1024 * 1024)})
      allocations.emplace_back(ShadowOwner(l4d2_buffer::allocate(bytes, l4d2_memory::Kind::Vertex, true)), bytes);
  }
  const auto allocateNs = wall() - begin, allocateCpu = cpu() - beginCpu;
  beginCpu = cpu(); begin = wall();
  for (auto& p : allocations) std::memset(p.first.get(), 0x5a, p.second);
  const auto writeNs = wall() - begin, writeCpu = cpu() - beginCpu;
  for (const auto& p : allocations) if (p.first[0] != 0x5a || p.first[p.second - 1] != 0x5a) return 1;
  beginCpu = cpu(); begin = wall(); allocations.clear();
  const auto releaseNs = wall() - begin, releaseCpu = cpu() - beginCpu;
  std::cout << "{\"bytes\":1342177280,\"allocation_ns\":" << allocateNs
    << ",\"allocation_cpu_ns\":" << allocateCpu << ",\"first_write_ns\":" << writeNs
    << ",\"first_write_cpu_ns\":" << writeCpu << ",\"through_first_write_ns\":" << allocateNs + writeNs
    << ",\"through_first_write_cpu_ns\":" << allocateCpu + writeCpu
    << ",\"release_ns\":" << releaseNs << ",\"release_cpu_ns\":" << releaseCpu << "}\n";
  return 0;
}
