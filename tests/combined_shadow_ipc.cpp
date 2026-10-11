// Actual production Shadow allocator -> actual production IPC -> integrity
// dispatch fixture. No DXVK/GPU, no per-command clock or VA scan.
#include "ipc_fixture.h"
#include "client/buffer_shadow.h"
#include <type_traits>
#include <psapi.h>
using Device = Bridge<BridgeId::Device>;
using Module = Bridge<BridgeId::Module>;
using AllocatedShadow = decltype(l4d2_buffer::allocate(1, l4d2_memory::Kind::Vertex, true));
using Shadow = std::conditional_t<std::is_pointer_v<AllocatedShadow>, std::unique_ptr<uint8_t[]>, AllocatedShadow>;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
uint64_t wall() {
  LARGE_INTEGER time {}, frequency {}; QueryPerformanceCounter(&time); QueryPerformanceFrequency(&frequency);
  return uint64_t(time.QuadPart / frequency.QuadPart) * 1000000000ull
    + uint64_t(time.QuadPart % frequency.QuadPart) * 1000000000ull / uint64_t(frequency.QuadPart);
}
uint64_t cpu() {
  FILETIME created {}, exited {}, kernel {}, user {};
  require(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user) != FALSE, "process CPU");
  return (((uint64_t(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime)
    + ((uint64_t(user.dwHighDateTime) << 32) | user.dwLowDateTime)) * 100;
}
void snapshot(const char* stage, unsigned round) {
  l4d2_observation::memoryMonitoring = true;
  const auto scan = l4d2_memory::scanAddressSpace();
  l4d2_observation::memoryMonitoring = false;
  require(scan.complete && scan.limit > uint64_t(3) * 1024 * 1024 * 1024, "complete LAA Client AV scan");
  std::printf("{\"kind\":\"va\",\"stage\":\"%s\",\"round\":%u,\"free\":%llu,\"largest\":%llu,\"mapped\":%llu}\n",
    stage, round, static_cast<unsigned long long>(scan.free), static_cast<unsigned long long>(scan.largestFree),
    static_cast<unsigned long long>(scan.mappedCommitted + scan.mappedReserved));
}
int wmain(int argc, wchar_t** argv) {
  try {
    require(argc == 2 && gUniqueIdentifier.setGuid(&argv[1]), "session arguments");
    bridge_data::buildIdentity = "combined-shadow-ipc-production-fixture";
    constexpr unsigned pairs = 16384, rounds = 3;
#ifdef REMIX_BRIDGE_CLIENT
    snapshot("before-channels", 0);
    WriterChannel writer("DeviceClient2Server", 96u << 20, 3072, 3072);
    ReaderChannel reader("DeviceServer2Client", 32u << 20, 10, 25);
    WriterChannel moduleWriter("ModuleClient2Server", 4u << 20, 5, 25);
    ReaderChannel moduleReader("ModuleServer2Client", 4u << 20, 5, 25);
#else
    ReaderChannel reader("DeviceClient2Server", 96u << 20, 3072, 3072);
    WriterChannel writer("DeviceServer2Client", 32u << 20, 10, 25);
    ReaderChannel moduleReader("ModuleClient2Server", 4u << 20, 5, 25);
    WriterChannel moduleWriter("ModuleServer2Client", 4u << 20, 5, 25);
#endif
    Device::install(&writer, &reader); Module::install(&moduleWriter, &moduleReader);
    const auto session = gUniqueIdentifier.toString();
    HANDLE ready = CreateEventA(nullptr, TRUE, FALSE, (session + "LifecycleReady").c_str());
    require(ready != nullptr, "ready event");
#ifdef REMIX_BRIDGE_CLIENT
    require(WaitForSingleObject(ready, 10000) == WAIT_OBJECT_0, "Host ready");
    snapshot("channels-live", 0);
    struct Payload { Shadow data; uint32_t bytes; };
    std::vector<Payload> buffers; buffers.reserve(pairs * 2);
    for (unsigned round = 0; round < rounds; ++round) {
      auto beginCpu = cpu(), begin = wall();
      for (unsigned i = 0; i < pairs * 2; ++i) {
        const uint32_t size = i % 2 ? 16384 : 65536;
        buffers.push_back({ Shadow(l4d2_buffer::allocate(size,
          i % 2 ? l4d2_memory::Kind::Index : l4d2_memory::Kind::Vertex, true)), size });
        std::memset(buffers.back().data.get(), uint8_t((i + round) % 251), size);
      }
      const auto creation = wall() - begin, creationCpu = cpu() - beginCpu;
      snapshot("shadows-live", round);
      beginCpu = cpu(); begin = wall();
      for (unsigned i = 0; i < buffers.size(); ++i) {
        const auto& p = buffers[i];
        Device::Command command(Commands::Bridge_Response, 0, 0,
          bridge_data::payload(p.bytes, p.data.get(), round, i, p.bytes));
        require(command.finish() == Result::Success, "actual Shadow upload");
      }
      const auto submission = wall() - begin, submissionCpu = cpu() - beginCpu;
      const auto uid = static_cast<UID>(Module::Command::get_uid());
      { Module::Command done(Commands::Bridge_Response, 0, 0, bridge_data::fields(round));
        require(done.finish() == Result::Success, "completion request"); }
      require(Module::waitForCommand(Commands::Bridge_Response, 10000, nullptr, true, uid) == Result::Success,
        "Host finished reading all Shadow uploads");
      require(Module::get_data() == round, "completion round"); Module::pop_front();
      const auto complete = wall() - begin;
      beginCpu = cpu(); begin = wall(); buffers.clear();
      const auto release = wall() - begin, releaseCpu = cpu() - beginCpu;
      snapshot("shadows-released-channels-live", round);
      std::printf("{\"kind\":\"timing\",\"round\":%u,\"logical_bytes\":1342177280,\"creation_first_write_ns\":%llu,\"creation_cpu_ns\":%llu,\"submission_ns\":%llu,\"submission_cpu_ns\":%llu,\"upload_complete_ns\":%llu,\"release_ns\":%llu,\"release_cpu_ns\":%llu}\n",
        round, static_cast<unsigned long long>(creation), static_cast<unsigned long long>(creationCpu),
        static_cast<unsigned long long>(submission), static_cast<unsigned long long>(submissionCpu),
        static_cast<unsigned long long>(complete), static_cast<unsigned long long>(release), static_cast<unsigned long long>(releaseCpu));
    }
#else
    require(SetEvent(ready) != FALSE, "ready signal");
    for (unsigned round = 0; round < rounds; ++round) {
      const auto beginCpu = cpu(), begin = wall();
      for (unsigned i = 0; i < pairs * 2; ++i) {
        require(Device::waitForCommand() == Result::Success, "Host wait"); Device::pop_front();
        require(Device::get_data() == round * pairs * 2 + i, "Host UID order");
        const auto p = Device::get_packet<3>();
        const uint32_t size = i % 2 ? 16384 : 65536;
        require(p.fields[0] == round && p.fields[1] == i && p.fields[2] == size
          && p.bytes == size && p.data, "Host upload layout");
        const auto expected = uint8_t((i + round) % 251);
        const auto* bytes = static_cast<const uint8_t*>(p.data);
        require(std::all_of(bytes, bytes + size, [&](uint8_t b) { return b == expected; }), "full Shadow payload integrity");
        Device::end_read_data();
      }
      const auto elapsed = wall() - begin, elapsedCpu = cpu() - beginCpu;
      require(Module::waitForCommand() == Result::Success, "completion marker");
      const auto header = Module::pop_front(); (void)header;
      const auto uid = Module::get_data();
      const auto p = Module::get_packet<1>(false); require(p.fields[0] == round, "marker round");
      Module::end_read_data();
      { Module::Command reply(Commands::Bridge_Response, uid); reply.send_data(round);
        require(reply.finish() == Result::Success, "completion reply"); }
      std::printf("{\"kind\":\"host-timing\",\"round\":%u,\"wall_ns\":%llu,\"cpu_ns\":%llu}\n", round,
        static_cast<unsigned long long>(elapsed), static_cast<unsigned long long>(elapsedCpu));
    }
#endif
    CloseHandle(ready);
    std::printf("COMBINED_SHADOW_IPC_PASS bits=%u\n", unsigned(sizeof(void*) * 8));
    return 0;
  } catch (const std::exception& error) { std::fprintf(stderr, "COMBINED_SHADOW_IPC_FAIL %s\n", error.what()); return 1; }
}
