# SPDX-License-Identifier: MIT
"""Derive the audited Valve sample from the audited ThinFlex sample for CI only."""
import argparse
import hashlib
from pathlib import Path

THINFLEX = "03964dedcf8b7f4ebde24cd3d0738873d37c075a7a9b313dad001bb937f9d1b6"
VALVE = "3f5f5b0f539e8ad22bcfc4381be41571257c0c29e8061057682f9b8525ca7b85"
# One-based offsets and replacement bytes, recorded from the two user-supplied files.
CHANGES = [
    (239,"4"),(271,"0"),(315,"105"),(321,"36"),(322,"340"),
    (641,"0"),(642,"0"),(643,"0"),(644,"0"),(645,"0"),(646,"0"),
    (651,"0"),(654,"0"),(655,"0"),(677,"0"),(680,"0"),
    (1162,"20"),(1163,"47"),(1164,"0"),
    (1231,"232"),(1232,"72"),(1233,"0"),
    (18445,"232"),(18446,"72"),(18447,"0"),
    (53971,"232"),(53972,"72"),(53973,"0"),
    (66693,"232"),(66694,"72"),(66695,"0"),
    (67289,"232"),(67290,"72"),(67291,"0"),
]

def digest(data):
    return hashlib.sha256(data).hexdigest()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    data = bytearray(args.source.read_bytes())
    if digest(data) != THINFLEX:
        raise SystemExit("ThinFlex fixture identity mismatch")
    for offset, value in CHANGES:
        data[offset - 1] = int(value, 8)
    if digest(data) != VALVE:
        raise SystemExit("Derived Valve fixture identity mismatch")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
