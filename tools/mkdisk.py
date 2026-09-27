#!/usr/bin/env python3
"""Build the test disk build/jl.adf: a real Workbench 1.3 disk, trimmed,
with jl in C:, a test config, a large inventory and a small game tree.

    tools/mkdisk.py                 # after agk build (which writes build/jl)

Needs your own Workbench 1.3 ADF (not in the repo): $JL_WB13_ADF, default
/Volumes/Youen/Retro/Amiga/CDTV-HD/workbench133.adf. Runs xdftool in the
kit's Docker toolchain image. The inventory uses the game names of a WHDLoad
collection ($JL_WHDLOAD, optional) in scrambled order, like a real scan.
"""
import os
import random
import shutil
import subprocess
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WB = os.environ.get("JL_WB13_ADF", "/Volumes/Youen/Retro/Amiga/CDTV-HD/workbench133.adf")
WHD = os.environ.get("JL_WHDLOAD", "/Volumes/Youen/Retro/LeTerrier/Amiga/WHDLOAD/GAMES")
IMAGE = os.environ.get("AGK_TOOLCHAIN_IMAGE", "amigadev/crosstools:m68k-amigaos")
INVENTORY_SIZE = 1200

# Room for the test files: what a launcher disk doesn't need.
DELETE = ["s/startup-sequence", "Utilities", "Utilities.info", "Expansion", "Expansion.info", "Trashcan",
          "Trashcan.info", "Prefs/Preferences", "devs/narrator.device", "devs/printer.device",
          "devs/printers", "libs/translator.library", "fonts", "GVP", "System",
          "System.info", "Empty", "Empty.info", "c/Ed", "c/Edit", "c/DiskDoctor"]

# A small collection for the scan test: dirs, a 2-slave game, a skipped data
# dir, a slave at the top of the scan dir, mixed case.
GAME_TREE = [
    "Games/_PointAndClick/Loom/Loom.slave",
    "Games/_PointAndClick/MonkeyIsland2Fr/MonkeyIsland2Fr.Slave",
    "Games/_PointAndClick/indianaJones/IndyAtlantis.slave",
    "Games/_Educatif/ADIJunior/WB31_1.Slave",
    "Games/_Educatif/ADIJunior/WB31_32.Slave",
    "Games/_Educatif/ADIJunior/data/Hidden.slave",
    "Games/Zool/Zool.slave",
    "Games/Zool/Zool.info",
    "Games/Top.slave",
]

STARTUP = """c:SetPatch >NIL:
c:palfix
Addbuffers df0: 10
resident CLI L:Shell-Seg SYSTEM pure add
resident c:Execute pure
makedir ram:t
assign T: ram:t
path ram: c: add
jl
"""

CONFIG = """scan_dir_1=DF0:Games
inventory_file=S:jl-inventory.data
jst_command=echo
"""


def inventory_lines(path_fmt="DF0:Games/{l}/{n}"):
    names = []
    if os.path.isdir(WHD):
        for letter in sorted(os.listdir(WHD)):
            d = os.path.join(WHD, letter)
            if os.path.isdir(d):
                names += [(letter, f.split("_v")[0]) for f in os.listdir(d) if f.endswith(".lha")]
    if not names:  # no collection at hand: made-up names
        names = [("G", f"Game{i:04d}") for i in range(INVENTORY_SIZE)]
    names = sorted(set(names))
    names = random.Random(1).sample(names, min(INVENTORY_SIZE, len(names)))  # scrambled
    return [f"{n};{path_fmt.format(l=l, n=n)};{n}.slave\n" for l, n in names]


def build_disk(out_adf, files, dirs=(), empty_files=()):
    """WB 1.3 disk minus DELETE, plus files {amiga_path: host_path or str text}."""
    work = tempfile.mkdtemp(prefix="jldisk", dir=os.path.join(ROOT, "build"))
    try:
        shutil.copy(WB, os.path.join(work, "disk.adf"))
        cmds = []
        for p in DELETE:
            cmds += ["+", "delete", p, "all"]
        for i, (ami, src) in enumerate(files.items()):
            host = os.path.join(work, f"f{i}")
            if os.path.exists(src):
                shutil.copy(src, host)
            else:
                with open(host, "w", newline="\n") as f:
                    f.write(src)
            cmds += ["+", "write", f"f{i}", ami]
        open(os.path.join(work, "empty"), "w").close()
        for d in dirs:
            cmds += ["+", "makedir", d]
        for p in empty_files:
            cmds += ["+", "write", "empty", p]
        cp = subprocess.run(["docker", "run", "--rm", "-u", f"{os.getuid()}:{os.getgid()}",
                             "-v", f"{work}:/w", "-w", "/w", IMAGE, "xdftool", "disk.adf", *cmds[1:]],
                            capture_output=True, text=True)
        if cp.returncode != 0:
            raise SystemExit(f"xdftool failed:\n{cp.stdout}{cp.stderr}")
        os.makedirs(os.path.dirname(out_adf), exist_ok=True)
        shutil.move(os.path.join(work, "disk.adf"), out_adf)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def tree_dirs(paths):
    """Every parent directory of paths, parents first."""
    out = []
    for p in paths:
        parts = p.split("/")[:-1]
        for i in range(1, len(parts) + 1):
            d = "/".join(parts[:i])
            if d not in out:
                out.append(d)
    return out


def main():
    b = os.path.join(ROOT, "build")
    files = {"c/jl": os.path.join(b, "jl"), "c/palfix": os.path.join(b, "palfix"),
             "s/startup-sequence": STARTUP, "s/jl-config.cfg": CONFIG,
             "s/jl-inventory.data": "".join(inventory_lines())}
    build_disk(os.path.join(b, "jl.adf"), files, tree_dirs(GAME_TREE), GAME_TREE)
    print(f"built build/jl.adf (WB 1.3, {INVENTORY_SIZE} inventory lines, {len(GAME_TREE)} test files)")


if __name__ == "__main__":
    main()
