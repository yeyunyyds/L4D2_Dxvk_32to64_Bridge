"""Run the production updater/coordinator across x86 game -> x86/x64 fixture Host."""
import ctypes
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from make_updater_test_vpk import make

HERE = Path(__file__).resolve().parents[1]
NATIVE = HERE / "work/updater-native"
IDENTITY = "l4d2-1.3.2+1111111111111111"


def wait_for(predicate, timeout=20):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return
        time.sleep(0.05)
    raise AssertionError("Timed out waiting for fixture state")


def case(bits, mode):
    with tempfile.TemporaryDirectory(prefix="ldb-updater-session-") as temp:
        root = Path(temp)
        host_dir = root / "bin/.l4d2bridge"
        host_dir.mkdir(parents=True)
        shutil.copy2(NATIVE / "fixture-x86.exe", root / "left4dead2.exe")
        shutil.copy2(NATIVE / "fixture-x86.exe", root / "bin/dxvk_d3d9.dll")
        shutil.copy2(NATIVE / "fixture-x86.exe", host_dir / "L4D2Bridge32.exe")
        shutil.copy2(NATIVE / "fixture-x64.exe", host_dir / "L4D2Bridge64.exe")
        shutil.copy2(NATIVE / "LDBupdater.exe", host_dir / "LDBupdater.exe")
        (host_dir / "README.md").write_text("OLD README")
        (host_dir / "bridge.conf").write_text("DO NOT CHANGE")
        (host_dir / "LDBupdater-test.ini").write_text("[LDBUpdaterTest]\nEnabled=1\nWorkshopId=1\n")
        workshop = root / "left4dead2/addons/workshop"
        workshop.mkdir(parents=True)
        make(root, workshop / "1.vpk", "1.3.3", IDENTITY, "UPDATED README")
        game = subprocess.Popen([str(root / "left4dead2.exe"), "game", mode], cwd=root)
        updater_done = False
        try:
            wait_for(lambda: (root / "game-window.txt").exists() and (root / "game-window.txt").stat().st_size > 0)
            hwnd = (root / "game-window.txt").read_text().split()[0]
            host = subprocess.Popen([str(host_dir / f"L4D2Bridge{bits}.exe"), "host", mode, str(game.pid), hwnd], cwd=root)
            host.wait(timeout=45)
            events = (root / "host-events.txt").read_text()
            if mode in ("decline", "cancel"):
                assert "armed-awaiting" not in events, events
                assert (host_dir / "README.md").read_text() == "OLD README"
                assert not (host_dir / "LDBupdate-package.sha256").exists()
                ctypes.windll.user32.PostMessageW(int(hwnd), 0x10, 0, 0)
            elif mode == "linger":
                assert "armed-awaiting" in events, events
                time.sleep(0.5)
                assert game.poll() is None
                assert (host_dir / "README.md").read_text() == "OLD README", "Installed before game exit"
                ctypes.windll.user32.PostMessageW(int(hwnd), 0x10, 0, 0)
            else:
                assert "armed-awaiting" in events, events
            game.wait(timeout=20)
            if mode in ("decline", "cancel"):
                wait_for(lambda: not list((host_dir / "LDBupdate").glob("*")))
            else:
                wait_for(lambda: (host_dir / "LDBupdater.log").exists() and "UPDATE_OK" in (host_dir / "LDBupdater.log").read_text())
                assert (host_dir / "README.md").read_text() == "UPDATED README"
                assert (host_dir / "LDBupdate-package.sha256").read_text() == hashlib.sha256((workshop / "1.vpk").read_bytes()).hexdigest()
                assert not (host_dir / "LDBupdate-transaction.json").exists()
                assert not list((host_dir / "LDBupdate").glob("*"))
            assert (host_dir / "bridge.conf").read_text() == "DO NOT CHANGE"
            # Wait until the helper has released the log/remaining process resources.
            time.sleep(0.2)
            updater_done = True
        finally:
            if game.poll() is None:
                game.kill()
                game.wait()
            if not updater_done:
                if (host_dir / "LDBupdater.log").exists():
                    print((host_dir / "LDBupdater.log").read_text(), file=sys.stderr)
                # Do not leave a helper waiting on a failed fixture in CI.
                subprocess.run(["taskkill", "/F", "/IM", "LDBupdater.exe"], capture_output=True)
        return {"bits": bits, "mode": mode, "passed": True}


if __name__ == "__main__":
    results = [case(bits, mode) for bits in (32, 64) for mode in ("decline", "cancel", "accept", "linger")]
    (NATIVE / "sessions.json").write_text(json.dumps(results, indent=2))
    print("UPDATER_SESSIONS_PASS", len(results))
