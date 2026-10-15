"""Run the production updater/coordinator across x86 game -> x86/x64 fixture Host."""
import ctypes
import hashlib
import json
import re
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
KERNEL = ctypes.WinDLL("kernel32", use_last_error=True)
KERNEL.OpenProcess.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_uint32]
KERNEL.OpenProcess.restype = ctypes.c_void_p
KERNEL.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
KERNEL.CloseHandle.argtypes = [ctypes.c_void_p]


def wait_helper(pid):
    handle = KERNEL.OpenProcess(0x100000, False, pid)
    if not handle:
        assert ctypes.get_last_error() == 87, "Cannot open helper for exit confirmation"
        return
    try:
        assert KERNEL.WaitForSingleObject(handle, 20000) == 0, "Updater did not terminate"
    finally:
        KERNEL.CloseHandle(handle)


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
        shutil.copy2(NATIVE / "client-fixture.dll", root / "bin/dxvk_d3d9.dll")
        if mode == "alias-client":
            shutil.copy2(NATIVE / "client-fixture.dll", root / "bin/alias_bridge.dll")
        shutil.copy2(NATIVE / "fixture-x86.exe", host_dir / "L4D2Bridge32.exe")
        shutil.copy2(NATIVE / "fixture-x64.exe", host_dir / "L4D2Bridge64.exe")
        shutil.copy2(NATIVE / "LDBupdater.exe", host_dir / "LDBupdater.exe")
        (host_dir / "README.md").write_text("OLD README")
        (host_dir / "bridge.conf").write_text("DO NOT CHANGE")
        (host_dir / "LDBupdater-test.ini").write_text("[LDBUpdaterTest]\nEnabled=1\nWorkshopId=1\n")
        workshop = root / "left4dead2/addons/workshop"
        workshop.mkdir(parents=True)
        make(root, workshop / "1.vpk", "1.3.2" if mode == "same" else "1.3.1" if mode == "older" else "1.3.3", IDENTITY, "UPDATED README")
        game = subprocess.Popen([str(root / "left4dead2.exe"), "game", mode], cwd=root)
        updater_done = False
        helper_pid = None
        try:
            wait_for(lambda: (root / "game-window.txt").exists() and (root / "game-window.txt").stat().st_size > 0)
            hwnd = (root / "game-window.txt").read_text().split()[0]
            host = subprocess.Popen([str(host_dir / f"L4D2Bridge{bits}.exe"), "host", mode, str(game.pid), hwnd], cwd=root)
            host.wait(timeout=45)
            events = (root / "host-events.txt").read_text()
            match = re.search(r"updater-pid=(\d+)", events)
            assert match, events
            helper_pid = int(match[1])
            if mode in ("alias-client", "same", "older"):
                assert "armed-awaiting" not in events, events
                if mode != "alias-client":
                    assert "no-new-version" in events, events
                assert (host_dir / "README.md").read_text() == "OLD README"
                ctypes.windll.user32.PostMessageW(int(hwnd), 0x10, 0, 0)
            elif mode in ("decline", "cancel"):
                assert "armed-awaiting" not in events, events
                assert ("LDB_UPDATE_TEST declined" if mode == "decline" else "LDB_UPDATE_TEST cancelled") in events, events
                assert (host_dir / "README.md").read_text() == "OLD README"
                assert not (host_dir / "LDBupdate-package.sha256").exists()
                ctypes.windll.user32.PostMessageW(int(hwnd), 0x10, 0, 0)
            elif mode in ("linger", "crash"):
                assert "armed-awaiting" in events, events
                time.sleep(0.5)
                assert game.poll() is None
                assert (host_dir / "README.md").read_text() == "OLD README", "Installed before game exit"
                if mode == "crash":
                    game.kill()
                else:
                    ctypes.windll.user32.PostMessageW(int(hwnd), 0x10, 0, 0)
            else:
                assert "armed-awaiting" in events, events
            game.wait(timeout=20)
            if mode in ("decline", "cancel", "same", "older", "alias-client", "crash"):
                wait_helper(helper_pid)
                wait_for(lambda: not list((host_dir / "LDBupdate").glob("*")))
                assert (host_dir / "README.md").read_text() == "OLD README"
                assert not (host_dir / "LDBupdate-package.sha256").exists()
            else:
                wait_for(lambda: (host_dir / "LDBupdater.log").exists() and "UPDATE_OK" in (host_dir / "LDBupdater.log").read_text())
                assert (host_dir / "README.md").read_text() == "UPDATED README"
                assert (host_dir / "LDBupdate-package.sha256").read_text() == hashlib.sha256((workshop / "1.vpk").read_bytes()).hexdigest()
                assert not (host_dir / "LDBupdate-transaction.json").exists()
                assert not list((host_dir / "LDBupdate").glob("*"))
            assert (host_dir / "bridge.conf").read_text() == "DO NOT CHANGE"
            wait_helper(helper_pid)
            logs = (host_dir / "LDBupdater.log").read_text(errors="replace")
            if mode == "crash":
                assert "Game crashed or returned an unexpected exit code" in logs, logs
            elif mode == "alias-client":
                assert "Standard loaded Client module was not found" in logs, logs
            else:
                assert "UPDATE_ERROR" not in logs, "Helper reported a hidden session failure"
            updater_done = True
        finally:
            if game.poll() is None:
                game.kill()
                game.wait()
            if not updater_done:
                # Let managed exception unwinding flush its diagnostics before escalation.
                if helper_pid:
                    try:
                        wait_helper(helper_pid)
                    except AssertionError:
                        subprocess.run(["taskkill", "/F", "/PID", str(helper_pid)], capture_output=True)
                        wait_helper(helper_pid)
                for log in (host_dir / "LDBupdater.log", host_dir / "LDBupdater-startup.log"):
                    if log.exists():
                        print(log.name + "\n" + log.read_text(errors="replace"), file=sys.stderr)
            evidence = NATIVE / "evidence" / f"{bits}-{mode}"
            evidence.mkdir(parents=True, exist_ok=True)
            for source in (root / "host-events.txt", host_dir / "LDBupdater.log", host_dir / "LDBupdater-startup.log"):
                if source.exists():
                    shutil.copy2(source, evidence / source.name)
        return {"bits": bits, "mode": mode, "passed": True}


if __name__ == "__main__":
    results = [case(bits, mode) for bits in (32, 64) for mode in ("decline", "cancel", "accept", "linger", "same", "older", "crash", "alias-client")]
    (NATIVE / "sessions.json").write_text(json.dumps(results, indent=2))
    print("UPDATER_SESSIONS_PASS", len(results))
