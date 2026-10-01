#!/usr/bin/env python3
"""Regenerate the split disassembly dump that `tudiscover` reads.

`build/RMHE08/asm/` is dtk's disassembly of every split unit - one `.s` per unit, ~92 MB across
13 500 files. Nothing in the build reads it (`config.asm_dir = None`, and no ninja edge names it);
`tools/splits/tudiscover.py` does, so `config/RMHE08/config.yml` sets `write_asm: false` and this
tool produces the dump when a session needs it. That turns a cost the split paid on *every* run -
`symbols.txt` is one of its dirty-check inputs, so a rename re-dumps all of it, ~200 s of a ~380 s
`ninja` - into one full split per attribution session.

    python tools/splits/dump_asm.py              # dump, then stamp it
    python tools/splits/dump_asm.py --check      # report the dump's age only, no split (exit 1 if not fresh)
    python tools/splits/dump_asm.py --dry-run    # print the command it would run

`--no-update` keeps the hand-edited `symbols.txt`/`splits.txt` out of the run (dtk's own
"for build systems" mode), and the dump is stamped with the hashes of the three files it is a
function of, so `tudiscover stats` can say when it has outlived a symbol edit. That matters because
stale asm is silent: `asm_files()`'s docstring records a stale copy printing `bl fn_80456DD4` where
the canonical one prints `bl _savegpr_14`, which zeroes a codegen fingerprint.

Nothing here is written outside `build/<game>/` (`dump_asm.yml`, the dump itself, the stamp).
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "splits"))

import tudiscover as td  # noqa: E402  (path set above; owns ASM_DIR and the dump stamp)

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
