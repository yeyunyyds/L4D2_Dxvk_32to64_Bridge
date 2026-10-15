"""Assemble the explicit manual-trigger local updater experiment, never a Release."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from make_updater_test_vpk import make

ROOT = Path(__file__).resolve().parents[1]


def package():
    source = ROOT / "dist/l4d2-bridge-patch"
    receipt = json.loads((source / "BUILD-INFO.json").read_text())
    out = ROOT / "dist/ldb-updater-test"
    out.mkdir(parents=True, exist_ok=False)
    bootstrap = out / "bootstrap"
    for slot, relative in {
        "client": "bin/dxvk_d3d9.dll",
        "host32": "bin/.l4d2bridge/L4D2Bridge32.exe",
        "host64": "bin/.l4d2bridge/L4D2Bridge64.exe",
    }.items():
        data = (source / relative).read_bytes()
        if hashlib.sha256(data).hexdigest() != receipt["files"][relative]:
            raise ValueError(f"Unmatched source binary {slot}")
        target = bootstrap / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    host = bootstrap / "bin/.l4d2bridge"
    shutil.copy2(ROOT / "work/updater-native/LDBupdater.exe", host / "LDBupdater.exe")
    (host / "LDBupdater-test.ini").write_text(
        "; Explicit LOCAL TEST only. Set the numeric filename stem in ONE place.\n"
        "[LDBUpdaterTest]\nEnabled=1\nWorkshopId=\n", encoding="ascii")
    version = (ROOT / "VERSION").read_text().strip()
    make(bootstrap, out / "LOCAL-TEST.vpk", version, receipt["build_id"],
         "LDB 本地更新替换测试已完成。此 VERSION 可手动修改；三件套来自同一测试构建。配置、DXVK、插件、ReShade 和 retention DB 保留。")
    shutil.copy2(ROOT / "docs/UPDATER-LOCAL-TEST.zh-CN.md", out / "INSTALL.zh-CN.md")
    shutil.copy2(ROOT / "scripts/make_updater_test_vpk.py", out / "make_updater_test_vpk.py")
    files = {str(p.relative_to(out)).replace("\\", "/"): hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob("*") if p.is_file()}
    (out / "BUILD-INFO.json").write_text(json.dumps({
        "kind": "explicit-local-updater-test", "version": version,
        "build_id": receipt["build_id"], "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "automatic_game_ready_detection": False, "manual_trigger": "Ctrl+Shift+F10",
        "production_updates_enabled": False, "files": files,
    }, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(out)


if __name__ == "__main__":
    package()
