# JOST Launcher v2: maintenance guide

A TinyLauncher-style menu that starts WHDLoad slaves through `jst`, in C, for
**Kickstart 1.3 only** (A500, CDTV). Same look and config as the Blitz Basic
v0.x (github.com/youenchene/JOST_Launcher). User docs: `dist/jl.readme`.

Two executables, both go in `C:`:
- `jl` (`src/stub/stub.c`, ~8K): what the user runs. A resident loop: load
  `jl-menu`, unload it, start the chosen slave through `jst`, load the menu again.
- `jl-menu` (everything else in `src/`): the menu. It hands the choice back in a
  `tJlShared` block (`src/shared.h`), whose address the stub passes as `jl=<hex>`.
  It also runs standalone.

Only the stub stays in memory while a game runs (13K measured). Keep it that
small: that's the point of v2.

## The loop

```sh
tools/check                 # unit tests, build, test disk, emulator tests on every profile
tools/check -p a500         # one profile
tools/check --update        # also re-record goldens: only for an intended visual change, say which
```

`agk` is the [Amiga Game Kit](https://github.com/codebase/amiga-game-kit)
CLI: on `PATH`, in a sibling checkout (`../amiga-game-kit`), or `$AGK`. Inputs
that can't be in the repo (Workbench 1.3 ADF, WHDLoad games, jst, ROMs) are set
in `tools/env.local`: copy `tools/env.example`.

Always go through `tools/check`: the harness counts `tests/*.agk`
and `agk.toml` as sources, and `tools/mkdisk.py` has to rebuild the test disk
after every build.

Emulator tests (`tests/*.agk`) assert on the `AGK ...` lines the launcher
prints (`src/dbg.c`, free on real hardware). `expect-serial` matches the whole
log after the run, so put the order you're checking into the regex.

Screens:
- `tools/cellmap.py SHOT.png` maps a screenshot to text cells.
- `tools/ocr.py SHOT.png` reads topaz text, e.g. a `jst` error on the CLI.
  A launch that "does nothing" is usually a message hidden behind the menu:
  screenshot right at `returned=` and OCR it.

Other tools:
- `tools/bench.py [-p PROFILE]`: v0.3 vs v2 timings and free memory for the game.
- `tests/real/` + `tools/mkgamedisk.py`: a launch with the real `jst` and a real
  game (run by hand, see `tests/real/launch.agk`).
- `tools/mkdemo.py --open [--a500-1mchip|--cdtv8|--cdtv]`: FS-UAE demo.

Profiles (in the kit's `harness/agk/profiles.py`): `a500` (512K chip + 512K
slow), `cdtv` (the same with ECS Agnus), `cdtv8` (1MB chip + 8MB fast, the
target CDTV), `a500-1mchip`.

## Where things go

- **Portable rules** (no Amiga headers, host unit-tested in `tests/unit/`):
  `inventory`, `config`, `nav`, `view`, `input`, `scanfmt`. Put new logic here
  with a unit test, and add the file to `unit_sources` in `agk.toml`.
- **Amiga side**, thin: `ui` (screen, copper), `hwinput`, `sfx`, `scan`,
  `store` (inventory file), `launch`, `app` (the session), `sys` (1.3 helpers).
- **Test-only helpers** in `tests/tools/`: `palfix`, `memreport`. They go on
  test disks, never in a release.

## Kickstart 1.3 rules

- **V34 APIs only.** No `AllocVec`, `CreateMsgPort`, `ReadArgs`, `RunCommand`
  (they're V36+). `src/sys.c` has the 1.3 equivalents.
- **Chip RAM through `AllocMem(MEMF_CHIP)`**, never `__chip`: with this
  toolchain the chip flag of the hunk gets lost, and the data lands in fast RAM
  (silent sounds, broken pointer).
- **NDK macro names clash** with ordinary identifiers: `CUSTOM` (use `HWREGS`
  for `0xDFF000`) and `textPen`. Build errors that don't make sense are usually
  one of these.

## Launching (src/launch.c)

- `jst` is `LoadSeg`ed and called directly on a 4K stack (`jlCallSeg`), the way
  the 1.3 shell runs a command.
- **Arguments:** on 1.3, `jst` reads them with BCPL `RdArgs` from the input
  stream's buffer, not from `a0`/`d0`. `callCommand` fakes that buffer with a
  `NIL:` handle. Without it `jst` prints "slave name is required".
- **Reload `jst` for every launch:** it isn't reentrant, so caching it would
  carry its variables into the next game.
- **BCPL commands can't be called this way** (e.g. 1.3's `Echo`): they need the
  shell's BCPL environment and hang. `jst_command` must be a normal program. The
  tests use `memreport` as the stand-in.
- **The game's folder must exist**, or the launch fails with "Could not start"
  (v0.x ran `jst` anyway).
- `Execute()` through a shell is the fallback when `LoadSeg` fails.

## Compatibility with v0.x

- **Inventory:** `name;path;slave` per line. v0.x files load as they are.
- **Sort order:** the file is saved back in name order at a moment the user
  waits anyway (after a scan, on launch or quit), so later starts skip the sort.
  `tests/launch.agk` checks it.
- **Folder view group:** the parent of the slave's directory.
- **Look:** keys, the remote, the palette, the layout (two columns of 29 rows)
  and the copper gradients follow `jl.bba` from v0.x. The goldens in
  `tests/golden/` guard them.

## Emulator quirks (not launcher bugs)

- **vAmiga + Kickstart 1.3 run NTSC** (200 rows) on a PAL machine, which clips
  the screen. `palfix` in each test disk's startup-sequence corrects it.
- **FS-UAE doesn't boot 1.3 from a directory hard drive**, so the demo boots
  from `build/demo/boot.adf`.
- **FS-UAE's CDTV model** has no DF0 and still stops at "insert a disc". Use
  `--cdtv8` (an A500 with the CDTV's memory) instead.
- **Long operations must keep the harness's frame clock ticking**: call
  `dbgTick()` during scans and file writes, or the harness decides the program
  hung.
- **Quitting a game with its quit key doesn't return in the emulator** (seen
  with Bubble Bobble and `jst` on a 68000).

## Local changes to the kit

These are in a local checkout of the [kit](https://github.com/codebase/amiga-game-kit), not upstream. Redo them on a fresh
checkout:
- **vAmiga:** built with Apple clang, because Homebrew LLVM's headers clash
  with the macOS SDK.
- **Python:** `tools/agk` and `tools/agk-mcp` run `python3.12` (they need 3.11+).
- **ROMs:** `roms/kick13.rom` must have a name without spaces (RetroShell
  splits paths).
- **Profiles:** `cdtv`, `cdtv8` and `a500-1mchip` added to `profiles.py`.

## Release

Update the version in `src/app.h` (`JL_VERSION`), `src/stub/stub.c` (`$VER`)
and `dist/jl.readme` (`Version:` and a `## X.Y` section: it becomes the release
notes), then push a tag `vX.Y`. `.github/workflows/release.yml` runs
`tools/release` (no kit needed) and publishes `jl.lha`, `jl`, `jl-menu`,
`jl.readme` and `jl-config.cfg`. It fails if the tag and the versions differ.

To publish on Aminet (an update of `util/misc/jl`), follow `AMINET_UPLOAD.md`
once the GitHub release is out and tested on real hardware.
