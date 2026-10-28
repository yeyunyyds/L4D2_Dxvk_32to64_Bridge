"""Offline checks for the opt-in Host plugin contract and published SDK copy."""
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
PATCH = (ROOT / "patches/l4d2-bridge.patch").read_text(encoding="utf-8")


def added_file(path: str) -> str:
    marker = f"diff --git a/{path} b/{path}\n"
    section = PATCH.split(marker, 1)[1].split("\ndiff --git ", 1)[0]
    lines = section.splitlines()
    start = next(i for i, line in enumerate(lines) if line.startswith("@@ ")) + 1
    return "\n".join(line[1:] for line in lines[start:] if line.startswith("+") and not line.startswith("+++")) + "\n"


class HostPluginContractTests(unittest.TestCase):
    def test_public_sdk_is_the_exact_header_compiled_into_host(self):
        public = (ROOT / "plugins/host-sdk/ldb_host_plugin.h").read_text(encoding="utf-8")
        self.assertEqual(added_file("bridge/include/ldb_host_plugin.h"), public)
        self.assertIn("LDB_HOST_PLUGIN_ABI_VERSION 1u", public)
        self.assertIn('LDB_HOST_PLUGIN_QUERY_EXPORT "LDBHostPluginQuery"', public)

    def test_disabled_initialization_returns_before_any_filesystem_work(self):
        manager = added_file("bridge/src/server/host_plugins.h")
        initialize = manager.split("void initialize(bool enabled", 1)[1].split("bool enabled()", 1)[0]
        disabled = initialize.index("if (!enabled) { return; }")
        self.assertLess(disabled, initialize.index("GetModuleFileNameW"))
        self.assertLess(disabled, initialize.index("FindFirstFileW"))
        self.assertNotIn("std::thread", manager)
        self.assertNotIn("Sleep(", manager)

    def test_runtime_default_and_present_gate_remain_opt_in(self):
        config = (ROOT / "config/bridge.conf").read_text(encoding="utf-8")
        self.assertIn("server.plugins = False", config)
        self.assertIn('Config::getOption<bool>("server.plugins", false)', PATCH)
        self.assertIn("if (gHostPlugins.wantsPresent())", PATCH)
        self.assertIn("host_plugin_include_path = include_directories('../../include')", PATCH)
        self.assertNotIn("server.plugins = True", config)


if __name__ == "__main__":
    unittest.main()
