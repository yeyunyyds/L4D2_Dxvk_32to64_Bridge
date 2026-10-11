// Additional assertions against the actual production adapter, not a mock.
#ifdef LDB_IPCV2_ADAPTER_TEST
int v2AdapterCases(const std::wstring& mode) {
  const size_t memory = 32 * sizeof(Header) + 256 + 128 * sizeof(uint32_t);
  // Reader must be able to initialize before the writer/process even exists.
  ReaderChannel reader("ModuleV2Adapter", memory, 32, 128);
  auto& channel = reader.commands->testChannel();
  auto& shared = channel.shared();
  const uint32_t seed = mode == L"v2-wrap" ? UINT32_MAX - 3 : 0;
  shared.producer.published.store(seed);
  shared.consumer.completed.store(seed);
  WriterChannel writer("ModuleV2Adapter", memory, 32, 128);
  if (mode == L"v2-build") {
    auto config = shared.config; config.build[2] ^= 1;
    bool rejected = false;
    try { ldb_ipc_v2::Channel wrong(gUniqueIdentifier.toString("ModuleV2AdapterCommand") + "-production", config, false); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "production v2 build mismatch refused");
    DWORD before = 0, after = 0;
    require(GetProcessHandleCount(GetCurrentProcess(), &before) != FALSE, "construction handles baseline");
    const auto* identity = bridge_data::buildIdentity;
    bridge_data::buildIdentity = "deliberately-incompatible-production-build";
    for (unsigned i = 0; i < 100; ++i) {
      bool failed = false;
      try { WriterChannel wrong("ModuleV2Adapter", memory, 32, 128); }
      catch (const std::runtime_error&) { failed = true; }
      require(failed, "IpcChannel construction rejects mismatch");
    }
    bridge_data::buildIdentity = identity;
    require(GetProcessHandleCount(GetCurrentProcess(), &after) != FALSE && before == after,
      "failed channel construction releases section and event handles");
  } else if (mode.rfind(L"v2-bad-", 0) == 0) {
    require(writer.commands->try_push({Commands::Bridge_Response,0,7,9}) == Result::Success, "malformed record fixture");
    auto& block = channel.block(seed);
    if (mode == L"v2-bad-block") block.bytes = 1024;
    else {
      ldb_ipc_v2::Record record {};
      std::memcpy(&record, channel.data(block), sizeof(record));
      record.bodyBytes = UINT32_MAX;
      std::memcpy(channel.data(block), &record, sizeof(record));
    }
    Result result;
    reader.commands->peek(result, 10);
    require(result == Result::Failure && shared.consumer.completed.load() == seed
      && shared.fault.load() && !gbBridgeRunning.load(), "bad Header rejected without credit");
  } else if (mode == L"v2-fault") {
    ReaderChannel otherReader("DeviceV2Other", memory, 32, 128);
    WriterChannel otherWriter("DeviceV2Other", memory, 32, 128);
    for (uint32_t i = 0; i < 31; ++i)
      require(writer.commands->try_push({Commands::Bridge_Response,0,i,i}) == Result::Success, "fault prefill");
    ldb_ipc_v2::Event::failNextWait = true;
    require(writer.commands->push({Commands::Bridge_Response,0,32,32}, 40) == Result::Failure, "OS wait failed");
    const auto published = shared.producer.published.load();
    require(!gbBridgeRunning.load() && shared.fault.load(), "sticky production session fault");
    require(otherReader.commands->testChannel().shared().fault.load()
      && otherWriter.commands->testChannel().shared().fault.load(), "fault cancels other Device/Module channels");
    require(writer.commands->try_push({Commands::Bridge_Response,0,33,33}) == Result::Failure
      && shared.producer.published.load() == published, "no post-failure publication");
  } else {
    for (uint32_t cycle = 0; cycle < 2000; ++cycle) {
      for (uint32_t i = 0; i < 31; ++i) {
        const auto n = cycle * 31 + i;
        require(writer.commands->try_push({Commands::Bridge_Response,0x8000,n,n ^ 0x5a5a}) == Result::Success,
          "exact original usable quota accepted");
      }
      require(writer.commands->try_push({Commands::Bridge_Response,0,0,0}) == Result::Failure, "original quota full");
      require(gbBridgeRunning.load() && !shared.fault.load(), "full try is retryable");
      for (uint32_t i = 0; i < 31; ++i) {
        const auto n = cycle * 31 + i;
        const auto before = shared.consumer.completed.load();
        Result result;
        const auto header = reader.commands->peek(result, 10);
        require(result == Result::Success && header.dataOffset == n && header.pHandle == (n ^ 0x5a5a)
          && header.flags == 0x8000 && shared.consumer.completed.load() == before, "peek holds credit");
        const auto popped = reader.commands->pull(result, 10);
        require(result == Result::Success && popped.dataOffset == n
          && shared.consumer.completed.load() == uint32_t(before + 1), "one pop one consume confirmation");
      }
    }
    require(shared.producer.published.load() == uint32_t(seed + 62000)
      && shared.consumer.completed.load() == uint32_t(seed + 62000), "counter wrap and reuse");
  }
  std::printf("IPC_V2_PRODUCTION_ADAPTER_PASS mode=%ls pointer_bits=%u\n", mode.c_str(), unsigned(sizeof(void*) * 8));
  return 0;
}
#endif
