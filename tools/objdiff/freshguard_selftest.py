#!/usr/bin/env python3
"""Selftest for the freshness half of `tools/lib/report.py` (was `tools/objdiff/freshguard.py`) - the shared "is this artefact older than its unit?" rule.

    python tools/objdiff/freshguard_selftest.py

Offline and fixture-only (a temporary tree with real files and mtimes; no compiler, no build, no
repository state). It pins the pieces both scorers stand on:

* `source_closure` - the unit's own file plus its in-tree headers, transitively, resolved through
  `include/` and `src/`, with a system header (`<string.h>`) never part of it;
* `newest` / `mtime` - the newest input, and `None` for a missing file;
* `freshness` - report mode checks the report against **both** the object and the newest source (the
  order-only `report.json` hazard); object mode checks the object against the newest source; the
  comparison is strict `<` so an equal whole-second stamp is current;
* `unit_reasons` - the one call a scorer that reads a prebuilt object makes, and that a **header** edit
  dates the object just as a `.cpp` edit does.
"""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import os
import tempfile

from tools.lib import report as fg  # the rule moved to lib.report; tools/objdiff/freshguard.py re-exports it

CHECKS = 0
FAILURES: list[str] = []


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


def selftest() -> int:
    check("mtime of a missing file is None", fg.mtime(os.path.join(tempfile.gettempdir(), "nope-xyz")),
          None)

    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src", "demo"))
        os.makedirs(os.path.join(tmp, "include", "demo"))
        src = os.path.join(tmp, "src", "demo", "unit.cpp")
        hdr = os.path.join(tmp, "include", "demo", "unit.h")
        deep = os.path.join(tmp, "include", "demo", "deep.h")
        obj = os.path.join(tmp, "build", "RMHE08", "src", "demo", "unit.o")
        os.makedirs(os.path.dirname(obj))
        with open(src, "w", encoding="utf-8") as fh:
            fh.write('#include "demo/unit.h"\n#include <string.h>\nint fn(void);\n')
        with open(hdr, "w", encoding="utf-8") as fh:
            fh.write('#include "demo/deep.h"\n')
        with open(deep, "w", encoding="utf-8") as fh:
            fh.write("/* deep */\n")
        with open(obj, "wb") as fh:
            fh.write(b"\x7fELF")

        closure = {os.path.relpath(p, tmp).replace("\\", "/") for p in fg.source_closure(src, tmp)}
        check("the closure holds the source", "src/demo/unit.cpp" in closure, True)
        check("... and the direct header", "include/demo/unit.h" in closure, True)
        check("... and the transitive header", "include/demo/deep.h" in closure, True)
        check("... but not an in-tree-missing system header",
              any(c.endswith("string.h") for c in closure), False)

        base = 1_000_000.0
        os.utime(src, (base, base))
        os.utime(hdr, (base + 10, base + 10))
        os.utime(deep, (base + 20, base + 20))
        newest = fg.newest([src, hdr, deep])
        check("newest() finds the most recent", os.path.basename(newest[0]), "deep.h")
        check("... with its mtime", newest[1], base + 20)
        check("newest() of nothing is None", fg.newest([]), None)

        # report mode: the report must be newer than the object AND the newest source
        check("report mode: a report newer than everything is current",
              fg.freshness(True, base + 100, base + 5, (src, base + 20), "r.json", "o.o"), [])
        check("report mode: a report older than the source is stale",
              len(fg.freshness(True, base + 15, base + 5, (src, base + 20), "r.json", "o.o")), 1)
        check("report mode: a report older than the object is stale",
              len(fg.freshness(True, base + 3, base + 5, None, "r.json", "o.o")), 1)
        check("report mode: a missing report is stale",
              len(fg.freshness(True, None, base + 5, None, "r.json", "o.o")), 1)
        check("both report reasons can fire at once",
              len(fg.freshness(True, base + 1, base + 5, (src, base + 20), "r.json", "o.o")), 2)

        # object mode (--measure / symdiff): the object against the newest source, strict `<`
        check("object mode: an object newer than every source is current",
              fg.freshness(False, None, base + 50, (src, base + 20), "r.json", "o.o"), [])
        check("object mode: a source newer than the object is stale",
              len(fg.freshness(False, None, base + 10, (src, base + 20), "r.json", "o.o")), 1)
        check("an equal whole-second stamp is current, not stale",
              fg.freshness(False, None, base + 20, (src, base + 20), "r.json", "o.o"), [])
        check("a missing object is stale",
              len(fg.freshness(False, None, None, (src, base + 20), "r.json", "o.o")), 1)

        # unit_reasons: the one call a prebuilt-object scorer makes; a header dates the object too
        os.utime(obj, (base + 50, base + 50))
        reasons, _newest = fg.unit_reasons(src, obj, tmp)
        check("a fresh object has no reasons", reasons, [])
        os.utime(hdr, (base + 60, base + 60))                       # edit only the header
        reasons, _newest = fg.unit_reasons(src, obj, tmp)
        check("a header newer than the object is stale", len(reasons), 1)
        check("... and the reason names the header", "unit.h" in reasons[0], True)

        # the printed reason shortens paths through `rel`
        short = fg.unit_reasons(src, obj, tmp, rel=lambda p: fg.rel_path(p, tmp))[0]
        check("the reason uses the rel() shortening", "src/demo" in short[0] or "unit.h" in short[0],
              True)
        check("rel_path shortens and uses forward slashes", fg.rel_path(os.path.join(tmp, "a", "b"),
                                                                        tmp), "a/b")

    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECKS)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
