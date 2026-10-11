// Test only: actual production LockableBuffer and buffer_shadow, native x86 CRT.
#include <iomanip>
#include <stdexcept>
#include <malloc.h>

constexpr uint64_t MiB = 1024 * 1024;
bool nativeArrayControl = false;
template<typename T> class PressureBuffer : public LockableBuffer<T> {
  using Desc = std::conditional_t<std::is_same_v<T, IDirect3DVertexBuffer9>, D3DVERTEXBUFFER_DESC, D3DINDEXBUFFER_DESC>;
public:
  explicit PressureBuffer(const Desc& d) : LockableBuffer<T>(nullptr, nullptr, d) {}
  using LockableBuffer<T>::lock;
  using LockableBuffer<T>::unlock;
};
using VB = PressureBuffer<IDirect3DVertexBuffer9>;
using IB = PressureBuffer<IDirect3DIndexBuffer9>;
uint64_t av() {
  MEMORYSTATUSEX m {}; m.dwLength = sizeof(m);
  if (!GlobalMemoryStatusEx(&m)) throw std::runtime_error("GlobalMemoryStatusEx");
  return m.ullAvailVirtual;
}
struct Population {
  std::vector<std::unique_ptr<VB>> vb;
  std::vector<std::unique_ptr<IB>> ib;
  std::vector<std::unique_ptr<uint8_t[]>> arrayVB, arrayIB;
  uint64_t bytes = 0;
  void add(UINT vs, UINT is, bool dynamic) {
    if (nativeArrayControl) {
      arrayVB.emplace_back(l4d2_buffer::allocate(vs, l4d2_memory::Kind::Vertex, true)); bytes += vs;
      arrayIB.emplace_back(l4d2_buffer::allocate(is, l4d2_memory::Kind::Index, true)); bytes += is;
      return;
    }
    D3DVERTEXBUFFER_DESC v {}; v.Size = vs; v.Usage = D3DUSAGE_WRITEONLY | (dynamic ? D3DUSAGE_DYNAMIC : 0);
    v.Pool = D3DPOOL_DEFAULT; v.Format = D3DFMT_VERTEXDATA;
    D3DINDEXBUFFER_DESC i {}; i.Size = is; i.Usage = v.Usage; i.Pool = v.Pool; i.Format = D3DFMT_INDEX16;
    vb.push_back(std::make_unique<VB>(v)); bytes += vs;
    ib.push_back(std::make_unique<IB>(i)); bytes += is;
  }
  void clear() { vb.clear(); ib.clear(); arrayVB.clear(); arrayIB.clear(); bytes = 0; }
};
struct Pressure {
  std::vector<void*> regions;
  uint64_t reserved = 0;
  void make(unsigned target, bool fragmented) {
    while (fragmented || av() > uint64_t(target) * MiB) {
      void* p = VirtualAlloc(nullptr, 64 * MiB, MEM_RESERVE, PAGE_NOACCESS);
      if (!p) break;
      regions.push_back(p); reserved += 64 * MiB;
    }
    if (fragmented) {
      // Distributed, nonadjacent 64 MiB holes; remaining background is reserve-only.
      for (size_t j = 0; j < regions.size() && av() < uint64_t(target) * MiB; j += 2) {
        VirtualFree(regions[j], 0, MEM_RELEASE); regions[j] = nullptr; reserved -= 64 * MiB;
      }
    }
  }
  ~Pressure() { for (auto p : regions) if (p) VirtualFree(p, 0, MEM_RELEASE); }
};
void record(const char* phase, unsigned round, const Population& stat, const Population& dyn,
            uint64_t background, uint64_t locks = 0) {
  l4d2_observation::memoryMonitoring = true;
  const auto scan = l4d2_memory::scanAddressSpace();
  l4d2_observation::memoryMonitoring = false;
  PROCESS_MEMORY_COUNTERS_EX p {}; p.cb = sizeof(p);
  if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p), sizeof(p)))
    throw std::runtime_error("GetProcessMemoryInfo");
  uint64_t heapBusy = 0;
  bool heapComplete = false;
  const auto heap = reinterpret_cast<HANDLE>(_get_heap_handle());
  if (HeapLock(heap)) {
    PROCESS_HEAP_ENTRY entry {};
    while (HeapWalk(heap, &entry)) {
      if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) heapBusy += entry.cbData;
    }
    heapComplete = GetLastError() == ERROR_NO_MORE_ITEMS;
    HeapUnlock(heap);
  }
  std::cout << "{\"phase\":\"" << phase << "\",\"round\":" << round
    << ",\"static_shadow_bytes\":" << stat.bytes << ",\"dynamic_shadow_bytes\":" << dyn.bytes
    << ",\"objects\":" << stat.vb.size() + stat.ib.size() + dyn.vb.size() + dyn.ib.size()
      + stat.arrayVB.size() + stat.arrayIB.size() + dyn.arrayVB.size() + dyn.arrayIB.size()
    << ",\"native_array_control\":" << (nativeArrayControl ? "true" : "false")
    << ",\"available_virtual_bytes\":" << av() << ",\"free_scan_bytes\":" << scan.free
    << ",\"largest_free_bytes\":" << scan.largestFree << ",\"private_bytes\":" << p.PrivateUsage
    << ",\"private_reserved_bytes\":" << scan.privateReserved
    << ",\"private_committed_scan_bytes\":" << scan.privateCommitted
    << ",\"crt_heap_busy_bytes\":" << heapBusy
    << ",\"crt_heap_walk_complete\":" << (heapComplete ? "true" : "false")
    << ",\"working_set_bytes\":" << p.WorkingSetSize << ",\"reserved_background_bytes\":" << background
    << ",\"address_limit\":" << scan.limit << ",\"scan_complete\":" << (scan.complete ? "true" : "false")
    << ",\"lock_cycles\":" << locks << "}\n" << std::flush;
}
template<typename T> void cycle(T& buffer, DWORD flag) {
  void* p = nullptr;
  if (buffer.lock(0, 256, &p, flag) != S_OK || !p) throw std::runtime_error("Lock");
  std::memset(p, 0x5a, 256);
  if (buffer.unlock() != S_OK) throw std::runtime_error("Unlock");
  updates.clear(); // Discard fixture upload captures before measuring memory.
}
int main(int argc, char** argv) {
  try {
    if (argc != 4 || sizeof(void*) != 4) throw std::runtime_error("Expected x86: target-av-mib shape cycles");
    const unsigned target = static_cast<unsigned>(std::stoul(argv[1]));
    const std::string shape = argv[2]; const unsigned rounds = static_cast<unsigned>(std::stoul(argv[3]));
    nativeArrayControl = shape == "native-array-control";
    const bool manySmallBuffers = shape == "many-small" || nativeArrayControl, fragmented = shape == "fragmented";
    Population stat, dyn;
    record("empty", 0, stat, dyn, 0);
    Pressure pressure; pressure.make(target, fragmented);
    record("background", 0, stat, dyn, pressure.reserved);
    if (fragmented) {
      try {
        stat.add(96 * MiB, 2 * MiB, false);
        record("contiguous-probe-accepted", 0, stat, dyn, pressure.reserved);
      } catch (const std::bad_alloc&) {
        record("contiguous-probe-rejected", 0, stat, dyn, pressure.reserved);
      }
      stat.clear();
    }
    for (unsigned round = 1; round <= rounds; ++round) {
      record("before-map", round, stat, dyn, pressure.reserved);
      dyn.add(12 * MiB, 256 * 1024, true);
      record("dynamic-12.25mib", round, stat, dyn, pressure.reserved);
      for (auto targetMiB : {128u, 256u, 512u, 768u, 1024u, 1280u}) {
        bool failed = false;
        try {
          // Preserve upload-adapter headroom; do not call this an OOM threshold.
          while (stat.bytes < uint64_t(targetMiB) * MiB && av() > 64 * MiB)
            stat.add(manySmallBuffers ? 64 * 1024 : UINT(8 * MiB), manySmallBuffers ? 16 * 1024 : UINT(2 * MiB), false);
        } catch (const std::bad_alloc&) { failed = true; }
        record(failed ? "allocation-failed" : "static-growth", round, stat, dyn, pressure.reserved);
        if (failed) break;
      }
      for (unsigned i = 0; i < (nativeArrayControl ? 0u : 10000u); ++i) {
        cycle(*dyn.vb.front(), D3DLOCK_DISCARD); cycle(*dyn.ib.front(), D3DLOCK_NOOVERWRITE);
      }
      record("after-lock-phase", round, stat, dyn, pressure.reserved, nativeArrayControl ? 0 : 20000);
      // Independent dynamic-heavy stage: bounded and permitted to fail under low AV.
      stat.clear();
      record("static-released", round, stat, dyn, pressure.reserved);
      try { while (dyn.bytes < 512 * MiB) dyn.add(8 * MiB, 2 * MiB, true); }
      catch (const std::bad_alloc&) {}
      record("dynamic-growth", round, stat, dyn, pressure.reserved);
      dyn.clear();
      record("map-released", round, stat, dyn, pressure.reserved);
    }
    // Intentionally retain one map's objects, then create the next; this is NOT a demonstrated leak.
    stat.add(16 * MiB, 4 * MiB, false);
    record("retained-old-map", 0, stat, dyn, pressure.reserved);
    stat.add(16 * MiB, 4 * MiB, false);
    record("new-map-old-still-owned", 0, stat, dyn, pressure.reserved);
    stat.clear(); record("all-owned-objects-released", 0, stat, dyn, pressure.reserved);
    return 0;
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
