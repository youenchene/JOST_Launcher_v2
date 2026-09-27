#!/usr/bin/env python3
"""v0.3 (Blitz) vs v2 (C), same Workbench 1.3 disk, same inventory.

    tools/bench.py [-n INVENTORY_LINES] [-p PROFILE]   # after agk build

C:jst on both disks is tests/tools/memreport: "starting a game" reports free
memory and a 50 Hz timestamp. The startup-sequence runs it once before the
launcher (the baseline, and the harness's boot marker). The scenario takes a
screenshot every 10 frames to see when the list appears, then launches twice.
"""
import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkdisk  # noqa: E402
from cellmap import load, kind  # noqa: E402

ROOT = mkdisk.ROOT
AGK = os.environ.get("AGK", os.path.expanduser("~/Code/amiga/amiga-game-kit/tools/agk"))
V0 = os.path.join(ROOT, "bench", "jl-v0.3")
SHOT_EVERY, SHOTS = 10, 150
LAUNCH_WAIT = 1500

STARTUP = """c:SetPatch >NIL:
c:palfix
Addbuffers df0: 10
resident CLI L:Shell-Seg SYSTEM pure add
resident c:Execute pure
makedir ram:t
assign T: ram:t
path ram: c: add
jst
jl
"""
CONFIG = "scan_dir_1=DF0:Games\ninventory_file=S:jl-inventory.data\n"


def scenario():
    lines = []
    for i in range(SHOTS):
        lines += [f"wait {SHOT_EVERY}", f"screenshot s{i:02d}"]
    lines += ["press fire", f"wait {LAUNCH_WAIT}", "screenshot back", "press fire", f"wait {LAUNCH_WAIT}"]
    return "\n".join(lines) + "\n"


def list_rows(png):
    """Text rows (of 29) with blue list text: 0 = no list on screen."""
    px = load(png)
    rows = 0
    for r in range(29):
        y = 4 + r * 8 + 4
        rows += any(kind(px(x, y)) == "b" for x in range(24, 320, 2))
    return rows


def sorted_lines(lines):
    """Name order, as v2 keeps the file after its first run (steady state)."""
    return sorted(lines, key=lambda l: (l.split(";")[0].lower(), l.split(";")[0]))


def run(name, exe, n, profile, config=CONFIG, stub=None):
    proj = os.path.join(ROOT, "bench", name)
    os.makedirs(os.path.join(proj, "build"), exist_ok=True)
    with open(os.path.join(proj, "agk.toml"), "w") as f:
        f.write(f'name = "{name}"\nprofile = "{profile}"\nsync = "frames"\nboot = "mem=report"\n')
    b = os.path.join(ROOT, "build")
    mkdisk.INVENTORY_SIZE = n
    files = {"c/jl": exe, "c/palfix": os.path.join(b, "palfix"), "c/jst": os.path.join(b, "memreport"),
             "s/startup-sequence": STARTUP, "s/jl-config.cfg": config,
             "s/jl-inventory.data": "".join(sorted_lines(mkdisk.inventory_lines("DF0:s")))}
    if stub:  # SPIKE C: the stub is started, jl is its menu module
        files["c/jlstub"] = stub
        files["s/startup-sequence"] = STARTUP.replace("jl\n", "jlstub\n")
    mkdisk.build_disk(os.path.join(proj, "build", f"{name}.adf"), files)
    sc = os.path.join(proj, "bench.agk")
    with open(sc, "w") as f:
        f.write(scenario())
    out = os.path.join(proj, "build", "run")
    subprocess.run([AGK, "run", "--no-build", "--fresh", "-p", profile, "-o", out, "-f", sc, proj],
                   capture_output=True, text=True)
    serial = open(os.path.join(out, "serial.txt"), errors="replace").read()
    mems = [dict(re.findall(r"(\w+)=(\d+)", l)) for l in serial.splitlines() if "mem=report" in l]
    shown = next((i for i in range(SHOTS) if list_rows(os.path.join(out, f"s{i:02d}.png")) >= 20), None)
    return {"mems": mems, "shown": None if shown is None else (shown + 1) * SHOT_EVERY,
            "back_rows": list_rows(os.path.join(out, "back.png")), "size": os.path.getsize(exe)}


def report(name, r):
    m = r["mems"]
    kb = lambda v: f"{int(v) // 1024}K"
    print(f"\n{name}: executable {r['size']} bytes")
    print(f"  list on screen   : {r['shown'] / 50 if r['shown'] else 'never'} s after the startup-sequence reached it")
    if len(m) < 2:
        print("  launch           : jst never ran")
        return
    base = m[0]
    for i, g in enumerate(m[1:], 1):
        fire_t = int(base["t"]) + SHOTS * SHOT_EVERY + (i - 1) * (LAUNCH_WAIT + 1)
        print(f"  launch {i}: game gets chip {kb(g['chip'])} fast {kb(g['fast'])} "
              f"(boot: chip {kb(base['chip'])} fast {kb(base['fast'])}), "
              f"~{(int(g['t']) - fire_t) / 50:.1f} s after fire")
    print(f"  after 1st launch : {r['back_rows']} list rows on screen")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", type=int, default=250, help="inventory lines (v0.3 holds at most 255)")
    ap.add_argument("-p", default="a500")
    ap.add_argument("--modes", help="v2 launch modes only, e.g. 0,1,2 (spike)")
    ap.add_argument("--stub", action="store_true", help="spike C: resident stub + menu module")
    a = ap.parse_args()
    if a.stub:
        b = os.path.join(ROOT, "build")
        report("v2 stub + module", run("v2stub", os.path.join(b, "jl"), a.n, a.p, CONFIG, os.path.join(b, "jlstub")))
        return
    if a.modes:
        for m in a.modes.split(","):
            report(f"v2 launch_mode={m}", run(f"v2m{m}", os.path.join(ROOT, "build", "jl"), a.n, a.p,
                                            CONFIG + f"launch_mode={m}\n"))
        return
    for name, exe in (("v0", V0), ("v2", os.path.join(ROOT, "build", "jl"))):
        report(name, run(name, exe, a.n, a.p))


if __name__ == "__main__":
    main()
