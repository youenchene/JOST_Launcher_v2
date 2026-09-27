#!/usr/bin/env python3
"""A playable demo for FS-UAE: a bootable Workbench 1.3 hard drive directory
with jl, the real jst and a few real WHDLoad games, on an A500 with
Kickstart 1.3 (512K chip + 512K slow, like a stock A500 with trapdoor RAM).

    tools/mkdemo.py [--open] [--cdtv8|--cdtv]   # --cdtv8: A500 1MB chip + 8MB fast;
                                            # --cdtv: FS-UAE's CDTV model + CDTV ext ROM

Inputs (not in the repo): $JL_WB13_ADF, $JL_WHDLOAD (see mkdisk.py),
$JL_JST_ZIP (jst 7.1 zip), $JL_KICK13 (Kickstart 1.3 ROM).
Output: build/demo/DH0 (the drive) and build/demo/jl-demo.fs-uae.
"""
import os
import shutil
import subprocess
import sys
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkdisk  # noqa: E402

ROOT = mkdisk.ROOT
JST_ZIP = os.environ.get("JL_JST_ZIP", "/Volumes/Youen/Retro/Amiga/CDTV-HD/jst_7.1.zip")
CDTV_EXT = os.environ.get("JL_CDTV_EXT", os.path.expanduser(
    "~/Code/amiga/Kickstarts/CDTV Extended-ROM v1.0 (1991)(Commodore)(CDTV)[!].rom"))
KICK = os.environ.get("JL_KICK13", os.path.expanduser(
    "~/Code/amiga/Kickstarts/Kickstart v1.3 rev 34.5 (1987)(Commodore)(A500-A1000-A2000-CDTV).rom"))
GAMES = ["A/Arkanoid_v1.9_0954.lha", "B/BubbleBobble_v1.3_2518.lha", "G/GreatGianaSisters_v1.6_2945.lha",
         "L/Lemmings_v1.5_Files_2089.lha", "P/Pang_v2.2_0929.lha", "P/PinballDreams_v1.9_0377.lha"]

STARTUP = """c:SetPatch >NIL:
echo "booted" >DH0:booted
c:palfix
resident CLI L:Shell-Seg SYSTEM pure add
resident c:Execute pure
makedir ram:t
assign T: ram:t
path ram: c: add
jl
"""
CONFIG = "scan_dir_1=DH0:Games\ninventory_file=S:jl-inventory.data\n"


def docker(work, *cmd, check=True):
    return subprocess.run(["docker", "run", "--rm", "-u", f"{os.getuid()}:{os.getgid()}", "-v", f"{work}:/w",
                           "-w", "/w", mkdisk.IMAGE, *cmd], check=check, capture_output=True, text=True)


def main():
    demo = os.path.join(ROOT, "build", "demo")
    shutil.rmtree(demo, ignore_errors=True)
    os.makedirs(demo)
    shutil.copy(mkdisk.WB, os.path.join(demo, "wb.adf"))
    docker(demo, "xdftool", "wb.adf", "unpack", ".")
    os.remove(os.path.join(demo, "wb.adf"))
    for f in os.listdir(demo):
        if f.startswith("Workbench1.3.") or f == "boot.adf":
            os.remove(os.path.join(demo, f))
    os.rename(os.path.join(demo, "Workbench1.3"), os.path.join(demo, "DH0"))
    dh0 = os.path.join(demo, "DH0")
    for f in os.listdir(dh0):            # xdftool's metadata files: not for FS-UAE
        if f.startswith("Workbench1.3."):
            os.remove(os.path.join(dh0, f))
    b = os.path.join(ROOT, "build")
    for exe in ("jl", "palfix"):
        shutil.copy(os.path.join(b, exe), os.path.join(dh0, "c", exe))
    with zipfile.ZipFile(JST_ZIP) as z, open(os.path.join(dh0, "c", "jst"), "wb") as f:
        f.write(z.read("jst/bin/jst"))
    for name, text in (("startup-sequence", STARTUP), ("jl-config.cfg", CONFIG)):
        with open(os.path.join(dh0, "s", name), "w", newline="\n") as f:
            f.write(text)
    games = os.path.join(dh0, "Games")
    os.makedirs(games)
    script = " ; ".join(f'lha xq "/whd/{g}"' for g in GAMES)  # lha exits 1 on attribute warnings
    subprocess.run(["docker", "run", "--rm", "-u", f"{os.getuid()}:{os.getgid()}", "-v", f"{games}:/w",
                    "-v", f"{mkdisk.WHD}:/whd:ro", "-w", "/w", mkdisk.IMAGE, "sh", "-c", script],
                   capture_output=True, text=True)
    got = [d for d in os.listdir(games) if os.path.isdir(os.path.join(games, d))]
    if len(got) < len(GAMES):
        raise SystemExit(f"only {len(got)} of {len(GAMES)} games extracted: {got}")
    # Boot floppy (tested headless: tests/real): FS-UAE doesn't boot Kickstart
    # 1.3 from a directory drive, so df0 boots jl + jst + Bubble Bobble.
    from mkgamedisk import build
    build(os.path.join(demo, "boot.adf"), os.path.join(games, "BubbleBobble"), marker=True)
    for d, _, files in os.walk(dh0):     # drop the per-file .info icons of games: less to scan
        for f in files:
            if f.endswith(".uaem"):
                os.remove(os.path.join(d, f))
    # a500: the minimum (stock A500 + trapdoor). cdtv8: your CDTV (1MB chip
    # with the 8372A Agnus, 8MB fast).
    configs = {"jl-demo.fs-uae": "chip_memory = 512\nslow_memory = 512\n",
               "jl-demo-cdtv8.fs-uae": "chip_memory = 1024\nslow_memory = 0\nfast_memory = 8192\n"
                                       "uae_chipset = ecs_agnus\n"}
    configs["jl-demo-cdtv.fs-uae"] = (f"amiga_model = CDTV\nkickstart_ext_file = {CDTV_EXT}\n"
                                      "chip_memory = 1024\nfast_memory = 8192\n"
                                      "uae_floppy0type = 0\n"   # the CDTV preset has no DF0
                                      f"uae_floppy0 = {os.path.join(demo, 'boot.adf')}\n")
    for name, mem in configs.items():
        model = "" if "amiga_model" in mem else "amiga_model = A500\n"
        with open(os.path.join(demo, name), "w") as f:
            f.write(f"[fs-uae]\n{model}{mem}kickstart_file = {KICK}\n"
                    f"floppy_drive_count = 1\nfloppy_drive_0 = {os.path.join(demo, 'boot.adf')}\n"
                    f"hard_drive_0 = {dh0}\nhard_drive_0_label = DH0\njoystick_port_1 = keyboard\n")
    pick = "cdtv8" if "--cdtv8" in sys.argv else "cdtv" if "--cdtv" in sys.argv else None
    cfg = os.path.join(demo, f"jl-demo-{pick}.fs-uae" if pick else "jl-demo.fs-uae")
    print(f"built {os.path.relpath(demo, ROOT)}: {len(GAMES)} games, config {os.path.relpath(cfg, ROOT)}")
    if "--open" in sys.argv:
        subprocess.Popen(["open", "-a", "FS-UAE", "--args", cfg])


if __name__ == "__main__":
    main()
