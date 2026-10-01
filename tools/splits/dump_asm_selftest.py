#!/usr/bin/env python3
"""Self-test for the on-demand asm dump: `tools/splits/dump_asm.py` + the stamp in `tudiscover`.

The risk `write_asm: false` introduces is a dump that silently outlives the map it was generated from
(stale asm zeroes codegen fingerprints - see `asm_files()`'s docstring), so the two things worth
pinning down are the temp-config rewrite that keeps the repo's own `config.yml` untouched, and the
state machine in `tudiscover.asm_stamp_status`: missing / unstamped / fresh / stale / truncated.

Both read their inputs from module globals, so this points those at a temp tree: no dtk, no real dump,
no writes outside the temp directory.

    python tools/splits/dump_asm_selftest.py
"""

from __future__ import annotations

import json
import os
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import dump_asm as da  # noqa: E402
import tudiscover as td  # noqa: E402

FAIL = []


def check(name, got, want):
    if got != want:
        FAIL.append("%s: got %r, want %r" % (name, got, want))


def state():
    return td.asm_stamp_status()[0]


def write(path, text):
    """Text mode translates `\n` on Windows, which would corrupt the fixtures - write raw."""
    with open(path, "w", encoding="utf-8", newline="") as fh:
        fh.write(text)


def test_temp_config(tmp):
    cfg = tmp / "config.yml"
    write(cfg, "a: 1\r\nwrite_asm: false  # off on purpose\r\nb: 2\r\n")
    da.CONFIG = cfg
    out = da.temp_config()
    check("write_asm flipped", "write_asm: true  # off on purpose" in out, True)
    check("neighbour keys untouched", out.replace("\r\n", "\n"),
          "a: 1\nwrite_asm: true  # off on purpose\nb: 2\n")
    check("line endings preserved", "\r\n" in out, True)

    write(cfg, "write_asm: true\n")
    check("already true stays true", da.temp_config(), "write_asm: true\n")

    write(cfg, "a: 1\n")
    try:
        da.temp_config()
        check("missing key asserts", "no error", "AssertionError")
    except AssertionError:
        check("missing key asserts", "ok", "ok")


def test_stamp_states(tmp):
    asm = tmp / "asm"
    asm.mkdir(parents=True)
    symbols, splits, dol = (tmp / n for n in ("symbols.txt", "splits.txt", "main.dol"))
    for p, text in ((symbols, "sym"), (splits, "spl"), (dol, "dol")):
        p.write_text(text, encoding="utf-8")
    td.ASM_DIR = str(asm)
    td.ASM_STAMP = str(asm / ".stamp.json")
    td.SYMBOLS, td.SPLITS, td.DOL = str(symbols), str(splits), str(dol)

    check("empty dump is missing", state(), "missing")

    (asm / "unit.s").write_text(".fn x\n.endfn x\n", encoding="utf-8")
    check("dump without stamp", state(), "unstamped")

    td.write_asm_stamp()
    check("stamped dump is fresh", state(), "fresh")
    check("a dump read from outside the tree says it is MAIN's, read-only", "[MAIN's dump" in td.asm_stamp_status()[1], True)
    check("... and is reported as a fallback", td.dump_is_main_fallback(), True)
    check("stamp counts .s only", json.loads(open(td.ASM_STAMP, encoding="utf-8").read())["files"], 1)
    check("no temp stamp left behind", os.path.exists(td.ASM_STAMP + ".tmp"), False)

    symbols.write_text("sym renamed", encoding="utf-8")
    check("map edit makes it stale", state(), "stale")

    td.write_asm_stamp()
    splits.write_text("spl changed", encoding="utf-8")
    check("split edit makes it stale", state(), "stale")

    td.write_asm_stamp()
    dol.write_text("dol changed", encoding="utf-8")
    check("dol edit makes it stale", state(), "stale")

    td.write_asm_stamp()
    (asm / "unit2.s").write_text(".fn y\n.endfn y\n", encoding="utf-8")
    check("a new unit does not invalidate the inputs", state(), "fresh")

    td.write_asm_stamp()
    (asm / "unit2.s").unlink()
    check("lost files are truncated", state(), "truncated")

    td.write_asm_stamp()
    (asm / ".stamp.json").write_text("not json", encoding="utf-8")
    check("unreadable stamp", state(), "unstamped")

    td.ASM_DIR = str(tmp / "gone")
    check("absent directory is missing", state(), "missing")


def test_local_dump(tmp):
    """The tool that WRITES the dump never follows the MAIN fallback."""
    td.ASM_DIR = str(tmp / "main-dump")
    check("a dump outside the tree is a fallback", td.dump_is_main_fallback(), True)
    td.use_local_dump()
    check("use_local_dump points at the tree's own dump", (td.ASM_DIR, td.dump_is_main_fallback()), (td.LOCAL_ASM_DIR, False))
    check("... and its stamp", td.ASM_STAMP, os.path.join(td.LOCAL_ASM_DIR, ".stamp.json"))
    check("dump_asm's out_dir is the tree's own build dir, never MAIN's", da.BUILD_DIR, Path(td.LOCAL_ASM_DIR).parent)


def main():
    with tempfile.TemporaryDirectory(prefix="dump_asm_selftest-") as tmp:
        tmp = Path(tmp)
        test_temp_config(tmp)
        test_stamp_states(tmp / "state")
        test_local_dump(tmp)
    for line in FAIL:
        print("FAIL " + line)
    print("%d check(s) failed" % len(FAIL) if FAIL else "all checks passed")
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
