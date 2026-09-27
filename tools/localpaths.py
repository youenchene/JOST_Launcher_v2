"""Paths to things that are not in the repo (your own ROMs, disks, games).

Read from the environment, which tools/env.local (gitignored, copy
tools/env.example) fills in: KEY=VALUE lines, # for comments.
"""
import os
import shutil

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(TOOLS)


def _load_env_local():
    path = os.path.join(TOOLS, "env.local")
    if not os.path.exists(path):
        return
    for line in open(path):
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1)
            os.environ.setdefault(k.strip(), os.path.expanduser(v.strip().strip('"')))


_load_env_local()


def agk():
    """The Amiga Game Kit CLI: $AGK, `agk` on PATH, or ../amiga-game-kit."""
    return (os.environ.get("AGK") or shutil.which("agk")
            or os.path.join(os.path.dirname(ROOT), "amiga-game-kit", "tools", "agk"))


def need(key, what):
    """A required input path, with a clear error if it isn't configured."""
    v = os.environ.get(key)
    if not v or not os.path.exists(v):
        raise SystemExit(f"{what}: set {key} in tools/env.local (see tools/env.example)"
                         + (f" - not found: {v}" if v else ""))
    return v


def optional(key, default=None):
    return os.environ.get(key, default)


def kick13():
    """Kickstart 1.3 ROM: $JL_KICK13, else the kit's roms/kick13.rom."""
    return os.environ.get("JL_KICK13") or os.path.join(
        os.path.dirname(os.path.dirname(agk())), "roms", "kick13.rom")
