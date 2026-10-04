#!/usr/bin/env python3
"""Regenerate the split disassembly dump that `tudiscover` reads, and stamp it. Spec: docs/tools/spec/dump_asm.md.
CLI: dump_asm.py [--check | --dry-run]."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

from tools.splits import tudiscover as td  # path set above; owns ASM_DIR and the dump stamp

CONFIG = ROOT / "config" / td.GAME / "config.yml"
BUILD_DIR = Path(td.LOCAL_ASM_DIR).parent        # build/<game>, the split's `out_dir` (the tree's own, never MAIN's)
TMP_CONFIG = BUILD_DIR / "dump_asm.yml"          # a copy: never the repo's own config.yml
DTK = ROOT / "build" / "tools" / ("dtk.exe" if os.name == "nt" else "dtk")


def temp_config() -> str:
    """`config.yml` as text with `write_asm: true` - the repo carries no YAML dependency.

    Read and written with `newline=""` so the copy keeps the repo file's line endings (CRLF here):
    the value is the only thing that changes.
    """
    with open(CONFIG, "r", encoding="utf-8", newline="") as fh:
        text = fh.read()
    new, n = re.subn(r"(?m)^(write_asm:[ \t]*)\S+", r"\g<1>true", text)
    assert n == 1, "config.yml: expected exactly one `write_asm:` line, found %d" % n
    return new


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true",
                    help="report the dump's age and stamp; run nothing (exit 1 unless fresh)")
    ap.add_argument("--dry-run", action="store_true",
                    help="print the split command and the temp config it would write; run nothing")
    args = ap.parse_args(argv)
    td.use_local_dump()                               # read AND write this tree's dump, never the MAIN fallback

    state, msg = td.asm_stamp_status()
    print("asm dump           %s" % msg)
    if args.check:
        return 0 if state == "fresh" else 1
    if not DTK.is_file():
        print("%s is missing - run `ninja tools` to download the pinned toolchain"
              % DTK.relative_to(ROOT), file=sys.stderr)
        return 2

    cmd = [str(DTK), "dol", "split", "--no-update", str(TMP_CONFIG), str(BUILD_DIR)]
    if args.dry_run:
        print("would write        %s (config.yml with write_asm: true)"
              % TMP_CONFIG.relative_to(ROOT))
        print("would run          %s" % " ".join(str(c) for c in cmd))
        print("a dump is a full split: objects (skipped when unchanged) + the asm, then the stamp")
        return 0

    with open(TMP_CONFIG, "w", encoding="utf-8", newline="") as fh:
        fh.write(temp_config())
    before = len(td.dump_files())
    print("split              %s, %d .s file(s) now; this takes a full `dol split`"
          % (BUILD_DIR.relative_to(ROOT), before))
    t0 = time.time()
    try:
        proc = subprocess.run(cmd, cwd=ROOT)
    finally:
        try:
            TMP_CONFIG.unlink()
        except OSError:
            pass
    if proc.returncode != 0:
        print("`dtk dol split` failed (exit %d); the dump and its stamp are unchanged"
              % proc.returncode, file=sys.stderr)
        return proc.returncode

    after = len(td.dump_files())
    td.write_asm_stamp()
    print("dumped             %d .s file(s) in %.0f s (%+d)"
          % (after, time.time() - t0, after - before))
    if after < before:
        print("WARNING: the dump lost %d file(s) - a `write_asm: false` split may clean the directory;"
              " see docs/build-performance.md" % (before - after), file=sys.stderr)
    print("asm dump           %s" % td.asm_stamp_status()[1])
    return 0


if __name__ == "__main__":
    sys.exit(main())
