# SPDX-License-Identifier: MIT
"""Build one verified field-test ZIP from matched Bridge and L4N artifacts."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile


BRIDGE_FILES = (
    "bin/dxvk_d3d9.dll",
    "bin/.l4d2bridge/L4D2Bridge32.exe",
    "bin/.l4d2bridge/L4D2Bridge64.exe",
)
PLUGIN_FILE = "bin/neko/plugins/L4D2BridgePlugin.dll"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def receipt(directory):
    return json.loads((directory / "BUILD-INFO.json").read_text(encoding="utf-8"))


def package(bridge, plugin, output, run_id, source_commit):
    bridge_info = receipt(bridge)
    plugin_info = receipt(plugin)
    if bridge_info.get("kind") != "matched-update":
        raise ValueError("Expected the matched Bridge update artifact")
    if plugin_info.get("kind") != "optional-l4n-plugin":
        raise ValueError("Expected the independent L4N artifact")
    if bridge_info.get("source_commit") != source_commit or plugin_info.get("source_commit") != source_commit:
        raise ValueError("Artifacts do not belong to the requested source commit")
    if bridge_info.get("version") != plugin_info.get("version"):
        raise ValueError("Bridge and plugin versions differ")
    if bridge_info.get("build_id") != plugin_info.get("build_id"):
        raise ValueError("Bridge and plugin build identities differ")

    sources = {name: bridge / name for name in BRIDGE_FILES}
    sources[PLUGIN_FILE] = plugin / PLUGIN_FILE
    declared = {**bridge_info.get("files", {}), **plugin_info.get("files", {})}
    if set(declared) != set(sources):
        raise ValueError("Unexpected binary set in source artifacts")
    for name, path in sources.items():
        if not path.is_file() or digest(path) != declared[name]:
            raise ValueError("Artifact checksum mismatch: " + name)

    version = bridge_info["version"]
    base = f"LDB-v{version}-host-tcp443-studio-menu-r{run_id}"
    staging = output / base
    if staging.exists():
        raise FileExistsError(staging)
    for name, path in sources.items():
        target = staging / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)

    manifest = {name: digest(staging / name) for name in sources}
    combined = {
        "version": version,
        "kind": "host-tcp443-studio-diagnostics-and-persistent-menu-field-test",
        "build_id": bridge_info["build_id"],
        "source_commit": source_commit,
        "actions_run_id": int(run_id),
        "files": manifest,
        "studiorender_profiles": {
            "thinflex": "03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6",
            "valve": "3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85",
        },
        "bridge_artifact": bridge_info,
        "plugin_artifact": plugin_info,
    }
    (staging / "BUILD-INFO.json").write_text(json.dumps(combined, indent=2) + "\n", encoding="utf-8")
    (staging / "SHA256.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    (staging / "TEST-OPTIONS.conf.txt").write_text(
        "# Merge only the options needed into bin/.l4d2bridge/bridge.conf, then restart L4D2.\n"
        "client.studioModelDiagnostics = True\n"
        "client.createDeviceTimeoutMs = 30000\n"
        "server.blockHostTcp443 = True\n",
        encoding="utf-8",
    )
    (staging / "INSTALL.zh-CN.md").write_text(
        f"# LDB v{version} Host TCP 443 快速失败 + Studio Hook + 持久化菜单测试包\n\n"
        "关闭游戏以及 L4D2Bridge32/L4D2Bridge64，备份并替换以下四个文件：\n\n"
        "- `bin/dxvk_d3d9.dll`\n"
        "- `bin/.l4d2bridge/L4D2Bridge32.exe`\n"
        "- `bin/.l4d2bridge/L4D2Bridge64.exe`\n"
        "- `bin/neko/plugins/L4D2BridgePlugin.dll`\n\n"
        "不要复制或覆盖 `bridge.conf`、DXVK 后端、ReShade 和 retention DB。"
        "需要测试时，将 `TEST-OPTIONS.conf.txt` 中所需的配置行合并到现有 `bridge.conf` 并重启。"
        "功能默认关闭。受限网络测试可启用 `server.blockHostTcp443=True`；它会让 Host 内所有 TCP 443 "
        "连接立即失败，因此 Steam 网页、Workshop 和其他 HTTPS 将不可用。\n\n"
        "`client.createDeviceTimeoutMs` 单位为毫秒，可在 1000–600000 范围调整，只控制 CreateDevice "
        "等待 Host 匹配回复。启动后可在日志核对网络策略安装结果、实际等待值，以及已匹配的 "
        "StudioRender 变体与哈希。\n\n"
        "菜单位置在移动时立即写入插件同目录的 `L4D2BridgePlugin.ini`。"
        "首次使用的默认位置位于屏幕左侧、垂直居中稍偏上；菜单内重置位置会删除已保存坐标。\n\n"
        "回退时关闭游戏和 Host，恢复上述四个备份文件。可删除 `L4D2BridgePlugin.ini` 复位菜单位置。\n",
        encoding="utf-8",
    )

    archive = output / (base + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path in sorted(staging.rglob("*")):
            if path.is_file():
                zf.write(path, path.relative_to(output).as_posix())
    checksum = digest(archive)
    (output / (archive.name + ".sha256")).write_text(f"{checksum}  {archive.name}\n", encoding="ascii")
    shutil.copy2(staging / "INSTALL.zh-CN.md", output / (base + "-INSTALL.zh-CN.md"))
    shutil.rmtree(staging)
    print(json.dumps({"archive": str(archive), "sha256": checksum, "build_id": combined["build_id"]}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bridge", type=Path, required=True)
    parser.add_argument("--plugin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--source-commit", required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    package(args.bridge, args.plugin, args.output, args.run_id, args.source_commit)
