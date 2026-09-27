#!/usr/bin/env python3
"""Print a screenshot as an 80x32 map of 8x8 text cells (hires 640x256).

    tools/cellmap.py SHOT.png [--rows]

Accepts the harness's full frame (build/agk*/.../NAME.png, 716x285) or a
640x512 capture like the v0.x README screenshots. Each cell shows its main
non-black colour: w white/grey, b blue, o orange, ? other, . black. With
--rows, prints the colour of every raster line at x=320 (the copper gradients).
Lets an agent compare layouts without looking at images.
"""
import sys
from PIL import Image


def load(path):
    im = Image.open(path).convert("RGB")
    w, h = im.size
    if (w, h) == (716, 285):      # vAmiga cutout: hires x = X - 62, line y = Y - 18
        return lambda x, y: im.getpixel((x + 62, y + 18))
    if (w, h) == (640, 512):      # line-doubled capture
        return lambda x, y: im.getpixel((x, y * 2))
    raise SystemExit(f"{path}: unexpected size {w}x{h}")


def kind(rgb):
    r, g, b = rgb
    if max(rgb) < 40:
        return "."
    if r > 200 and g > 200 and b > 200 or (abs(r - g) < 20 and abs(g - b) < 20):
        return "w"
    if b > r and b > 180:
        return "b"
    if r > 200 and g < 200 and b < 120:
        return "o"
    return "?"


def main():
    px = load(sys.argv[1])
    if "--rows" in sys.argv:
        for y in range(256):
            print(y, "%02x%02x%02x" % px(320, y))
        return
    for row in range(32):
        line = ""
        for col in range(80):
            counts = {}
            for y in range(row * 8, min(row * 8 + 8, 256)):
                for x in range(col * 8, col * 8 + 8):
                    k = kind(px(x, y))
                    if k != ".":
                        counts[k] = counts.get(k, 0) + 1
            line += max(counts, key=counts.get) if counts else "."
        print(f"{row:2d} {line}")


if __name__ == "__main__":
    main()
