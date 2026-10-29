# SPDX-License-Identifier: MIT
"""Package, verify, and update checksums for the optional L4N plugin asset."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DLL = "bin/neko/plugins/L4D2BridgePlugin.dll"


def machine(data):
    if data[:2] != b"MZ" or len(data) < 64:
        raise ValueError("L4N plugin is not a PE file")
    offset = int.from_bytes(data[60:64], "little")
    if data[offset:offset + 4] != b"PE\0\0":
        raise ValueError("L4N plugin has no PE signature")
    return int.from_bytes(data[offset + 4:offset + 6], "little")


def package(dll, output):
    version = (ROOT / "VERSION").read_text().strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Optional patch requires a stable VERSION")
    binary = Path(dll).read_bytes()
    if machine(binary) != 0x14C:
        raise ValueError("L4N plugin must be x86")
    files = {
        DLL: binary,
        "VERSION": (version + "\n").encode(),
        "LICENSE": (ROOT / "LICENSE").read_bytes(),
        "THIRD_PARTY.md": (ROOT / "THIRD_PARTY.md").read_bytes(),
        "docs/L4N-BRIDGE-CONTROLS.md": (ROOT / "docs/L4N-BRIDGE-CONTROLS.md").read_bytes(),
        "README.md": (
            f"# Optional L4N Bridge controls {version}\n\n"
            "Close L4D2, then merge `bin/neko/plugins` into the game directory. "
            "This optional package replaces only `L4D2BridgePlugin.dll`; it does not replace "
            "Bridge, DXVK, ReShade, retention data, or `bridge.conf`.\n"
        ).encode(),
    }
    hashes = {DLL: hashlib.sha256(binary).hexdigest()}
    receipt = {
        "version": version,
        "kind": "optional-l4n-plugin",
        "distribution": "release",
        "source_commit": os.environ.get("GITHUB_SHA", "local"),
        "build_run_id": int(os.environ.get("GITHUB_RUN_ID", "0")),
        "files": hashes,
    }
    files["SHA256.json"] = (json.dumps(hashes, indent=2) + "\n").encode()
    files["BUILD-INFO.json"] = (json.dumps(receipt, indent=2) + "\n").encode()
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    if output.exists():
        raise FileExistsError(output)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    verify(output)


def read(path):
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError("Corrupt optional plugin ZIP")
        files = {}
        for item in archive.infolist():
            name = PurePosixPath(item.filename)
            if item.is_dir():
                continue
            if name.is_absolute() or ".." in name.parts or item.filename in files:
                raise ValueError("Unsafe or duplicate ZIP member")
            files[item.filename] = archive.read(item)
    return files


def verify(path):
    files = read(path)
    allowed_bin = [name for name in files if name.startswith("bin/")]
    if allowed_bin != [DLL] or machine(files[DLL]) != 0x14C:
        raise ValueError("Optional asset must contain only the x86 plugin binary")
    receipt = json.loads(files["BUILD-INFO.json"])
    hashes = json.loads(files["SHA256.json"])
    expected = hashlib.sha256(files[DLL]).hexdigest()
    if receipt["kind"] != "optional-l4n-plugin" or receipt["files"] != hashes or hashes != {DLL: expected}:
        raise ValueError("Invalid optional plugin receipt")
    if files["VERSION"].decode().strip() != receipt["version"]:
        raise ValueError("Optional plugin version mismatch")


def update_sums(asset, existing, output):
    asset = Path(asset)
    target = asset.name
    digest = hashlib.sha256(asset.read_bytes()).hexdigest()
    entries = []
    seen = set()
    for line in Path(existing).read_text().splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  (\S+)", line)
        if not match or match.group(2) in seen:
            raise ValueError("Invalid existing SHA256SUMS.txt")
        seen.add(match.group(2))
        entries.append((match.group(2), match.group(1)))
    if target not in seen:
        entries.append((target, digest))
    else:
        entries = [(name, digest if name == target else value) for name, value in entries]
    Path(output).write_text("".join(f"{value}  {name}\n" for name, value in entries))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    build = sub.add_parser("package")
    build.add_argument("--dll", required=True, type=Path)
    build.add_argument("--output", required=True, type=Path)
    check = sub.add_parser("verify")
    check.add_argument("zip", type=Path)
    sums = sub.add_parser("update-sums")
    sums.add_argument("--asset", required=True, type=Path)
    sums.add_argument("--existing", required=True, type=Path)
    sums.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.command == "package":
        package(args.dll, args.output)
    elif args.command == "verify":
        verify(args.zip)
    else:
        update_sums(args.asset, args.existing, args.output)
