#!/usr/bin/env python3
"""A real-game disk: Workbench 1.3 + jl + the real jst + one WHDLoad game
(Bubble Bobble, 184K), all on one floppy. Used by the headless real-launch
check (tests/real/) and as the FS-UAE demo boot disk.

    tools/mkgamedisk.py OUT.adf GAMEDIR   # GAMEDIR: an extracted game (from tools/mkdemo.py)
"""
import os
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkdisk  # noqa: E402
from mkdemo import JST_ZIP  # noqa: E402

STARTUP = mkdisk.STARTUP
# DH0: the FS-UAE demo drive, if mounted (a missing device is skipped quietly)
CONFIG = "scan_dir_1=DF0:Games\nscan_dir_2=DH0:Games\ninventory_file=S:jl-inventory.data\n"


def build(out, game):
    b = os.path.join(mkdisk.ROOT, "build")
    jst = os.path.join(b, "jst")
    with zipfile.ZipFile(JST_ZIP) as z, open(jst, "wb") as f:
        f.write(z.read("jst/bin/jst"))
    files = {"c/jl": os.path.join(b, "jl"), "c/palfix": os.path.join(b, "palfix"), "c/jst": jst,
             "s/startup-sequence": STARTUP, "s/jl-config.cfg": CONFIG}
    name = os.path.basename(game.rstrip("/"))
    dirs = ["Games", f"Games/{name}"]
    for d, subdirs, fs in os.walk(game):
        rel = os.path.relpath(d, game)
        for s in subdirs:
            dirs.append(os.path.normpath(f"Games/{name}/{rel}/{s}"))
        for f in fs:
            files[os.path.normpath(f"Games/{name}/{rel}/{f}")] = os.path.join(d, f)
    slave = next(f for f in os.listdir(game) if f.lower().endswith(".slave"))
    files["s/jl-inventory.data"] = f"{name};DF0:Games/{name};{slave}\n"   # ready to launch
    mkdisk.build_disk(os.path.abspath(out), files, dirs)
    print(f"built {out} with {name}")


def main():
    build(sys.argv[1], sys.argv[2])


if __name__ == "__main__":
    main()
