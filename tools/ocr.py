#!/usr/bin/env python3
"""Read the text on an Amiga screenshot (topaz 8, hires, e.g. the CLI).

    tools/ocr.py SHOT.png [ROM]    # full frame from the harness (716x285)

Glyphs come from the topaz 8 font in the Kickstart ROM ($JL_KICK13, see tools/env.example). Lets an
agent read CLI messages (jst errors...) without looking at images.
"""
import os
import struct
import sys
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import localpaths  # noqa: E402

ROM = localpaths.kick13()
BASE = 0xFC0000


def topaz(rom):
    d = open(rom, "rb").read()
    cands = []
    name = d.find(b"topaz.font")
    while name >= 0:
        ptr = struct.pack(">I", BASE + name)
        i = d.find(ptr)
        while i >= 0:
            cands.append(i)
            i = d.find(ptr, i + 1)
        name = d.find(b"topaz.font", name + 1)
    for i in cands:
        tf = i - 10                      # ln_Name is at offset 10 of the TextFont
        # the ROM copy has no 4-byte mn_ReplyPort gap: tf_YSize at +16
        ysize, xsize = struct.unpack(">H", d[tf + 16:tf + 18])[0], struct.unpack(">H", d[tf + 20:tf + 22])[0]
        if ysize == 8 and xsize == 8:
            lo, hi = d[tf + 28], d[tf + 29]
            data, mod, loc = struct.unpack(">IHI", d[tf + 30:tf + 40])
            glyphs = {}
            for c in range(lo, hi + 1):
                off, w = struct.unpack(">HH", d[loc - BASE + (c - lo) * 4:loc - BASE + (c - lo) * 4 + 4])
                rows = []
                for y in range(8):
                    row = 0
                    for x in range(8):
                        bit = off + x
                        byte = d[data - BASE + y * mod + bit // 8] if x < w else 0
                        row = (row << 1) | ((byte >> (7 - bit % 8)) & 1 if x < w else 0)
                    rows.append(row)
                glyphs[tuple(rows)] = chr(c)
            return glyphs
    raise SystemExit("topaz 8 not found in ROM")


def main():
    glyphs = topaz(sys.argv[2] if len(sys.argv) > 2 else ROM)
    im = Image.open(sys.argv[1]).convert("RGB")
    px = lambda x, y: im.getpixel((x + 62, y + 18))
    best = None
    for oy, ox in [(oy, ox) for oy in range(8) for ox in range(8)]:
        lines, hits = [], 0
        for row in range((256 - oy) // 8):
            line = ""
            for col in range(79):
                pts = [[px(ox + col * 8 + x, oy + row * 8 + y) for x in range(8)] for y in range(8)]
                flat = [c for r in pts for c in r]
                bg = max(set(flat), key=flat.count)   # the cell's paper colour
                cell = tuple(sum(1 << (7 - x) for x in range(8) if pts[y][x] != bg) for y in range(8))
                ch = glyphs.get(cell)
                if ch is None:
                    inv = tuple(~r & 0xFF for r in cell)
                    ch = glyphs.get(inv, "?" if any(cell) else " ")
                hits += ch not in "? "
                line += ch
            lines.append(line.rstrip())
        if best is None or hits > best[0]:
            best = (hits, lines)
    print("\n".join(l for l in best[1] if l.strip()))


if __name__ == "__main__":
    main()
