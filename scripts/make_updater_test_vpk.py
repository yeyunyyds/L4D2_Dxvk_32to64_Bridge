"""Build an explicitly unsigned LOCAL TEST VPK; VERSION may be edited afterwards."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile

SOURCES = {
    "client": "bin/dxvk_d3d9.dll",
    "host32": "bin/.l4d2bridge/L4D2Bridge32.exe",
    "host64": "bin/.l4d2bridge/L4D2Bridge64.exe",
    "readme": "UPDATE-README.md",
}


def make(source, output, version, build_id, notes):
    if not re.fullmatch(r"(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)", version):
        raise ValueError("Expected stable numeric version")
    bodies = {slot: (source / path).read_bytes() for slot, path in SOURCES.items() if slot != "readme"}
    bodies["readme"] = notes.encode("utf-8")
    files = []
    for slot, body in bodies.items():
        if slot != "readme" and build_id.encode() not in body:
            raise ValueError(f"Missing matching build identity: {slot}")
        files.append({"slot": slot, "source": SOURCES[slot], "bytes": len(body), "sha256": hashlib.sha256(body).hexdigest()})
    manifest = {"schema": 1, "product": "LDB", "mode": "local-test-editable-version", "build_id": build_id, "files": files}
    with zipfile.ZipFile(output, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("VERSION", version + "\n")
        archive.writestr("UPDATE-MANIFEST.json", json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
        for slot, body in bodies.items():
            archive.writestr(SOURCES[slot], body)
        for filename in ("LICENSE", "THIRD_PARTY.md"):
            if (source / filename).is_file():
                archive.write(source / filename, filename)
        if (source / "licenses").is_dir():
            for item in sorted((source / "licenses").rglob("*")):
                if item.is_file():
                    archive.write(item, item.relative_to(source).as_posix())


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--version", required=True)
    p.add_argument("--build-id", required=True)
    p.add_argument("--notes", default="LDB 本地更新链路测试：替换匹配三件套和本介绍，不更改配置。")
    a = p.parse_args()
    make(a.source, a.output, a.version, a.build_id, a.notes)
