# SPDX-License-Identifier: MIT
"""Extract the exact pre-fix production Presenter from frozen LDB main."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = "cafc0442446c502a0de0ebb52d5e73ccaae67516"
NAME = "bridge/src/server/overlay_presenter.h"


def main():
    patch = subprocess.check_output(
        ["git", "show", BASE + ":patches/l4d2-bridge.patch"], cwd=ROOT
    ).decode("utf-8")
    marker = f"diff --git a/{NAME} b/{NAME}\n"
    if patch.count(marker) != 1:
        raise ValueError("Expected one frozen Presenter")
    block = patch.split(marker, 1)[1].split("\ndiff --git ", 1)[0]
    metadata, body = block.split("@@ -0,0 +1,", 1)
    if "--- /dev/null\n" not in metadata:
        raise ValueError("Frozen Presenter must be an entirely added file")
    count, body = body.split(" @@\n", 1)
    lines = body.splitlines()
    if len(lines) != int(count) or any(not line.startswith("+") for line in lines):
        raise ValueError("Unexpected frozen Presenter layout")
    data = ("\n".join(line[1:] for line in lines) + "\n").encode("utf-8")
    out = ROOT / ".deps/input-baseline"
    target = out / NAME
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    (out / "source-manifest.json").write_text(json.dumps({
        "commit": BASE, "file": NAME, "sha256": hashlib.sha256(data).hexdigest(),
        "scope": "Unmodified pre-fix production header; test compiler/dependencies use candidate tree",
    }, indent=2) + "\n", encoding="utf-8")
    print("Prepared exact LDB input control:", BASE)


if __name__ == "__main__":
    main()
