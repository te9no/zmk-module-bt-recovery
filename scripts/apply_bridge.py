#!/usr/bin/env python3
"""Explicit, version-locked integration. Never invoked by CMake."""
import pathlib
import subprocess
import sys
PIN = "4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85"
root = pathlib.Path(__file__).resolve().parents[1]
tree = pathlib.Path(sys.argv[1]).resolve()
def git(*args, check=True):
    return subprocess.run(["git", "-C", str(tree), *args], check=check,
                          capture_output=True, text=True)
if git("rev-parse", "HEAD").stdout.strip() != PIN:
    sys.exit("Unsupported ZMK commit; refusing to patch. Port/review the bridge first.")
patch = str(root / "patches/zmk-4dd95d7-bridge.patch")
if git("apply", "--reverse", "--check", patch, check=False).returncode == 0:
    print("Bridge already applied"); sys.exit(0)
check = git("apply", "--check", patch, check=False)
if check.returncode:
    sys.exit("Patch context differs; refusing partial/unsafe application.\n" + check.stderr)
git("apply", patch)
print("Applied pinned core guards and recovery bridge")
