#!/usr/bin/env python3
"""Fast, deterministic self-test for tools/units/ledger.py.

    python tools/units/ledger_selftest.py

No build, no `ninja` and no repository state: the ledger's input is text, so the fixture below is one tiny
`symbols.txt` / `splits.txt` / `configure.py` / `report.json` set, injected through the constructor. That
keeps the contract explicit - which symbols count as covered, when a score counts as closed, which symbol
`next` picks, and when the report is too old to believe - instead of re-deriving it from whatever the tree
happens to contain today.

The real-data counterpart is `symbolpreflight`'s self-test, which pins the parsers these views stand on.
"""
from __future__ import annotations

import json
import os
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import ledger as led  # noqa: E402  (imported through the sys.path shim above)


def entry(name: str, section: str, address: int, kind: str, size: int) -> dict:
    return {"name": name, "section": section, "address": address, "type": kind, "size": size, "lineno": 1}


# A unit owns the first function, and the report scores it 85 - closed, but not matching.
SYMBOLS = [
    entry("fn_80004000", ".init", 0x80004000, "function", 0x100),
    entry("named_thing", ".init", 0x80004100, "function", 0x40),
    entry("glob", ".data", 0x80005000, "object", 4),
    entry("lbl_80006000", ".rodata", 0x80006000, "label", 0),
]
SPLITS = [
    {"unit": "Mod/unit.c", "ranges": [{"section": ".init", "start": 0x80004000, "end": 0x80004100, "rename": None}]}
]
CONFIGURED = {
    "Mod/unit.c": {
        "flag": "NonMatching",
        "path": "Mod/unit.c",
        "lib": "mod",
        "mw_version": "Wii/1.3",
        "cflags": "cflags_base",
    }
}
REPORT = {
    "version": 2,
    "measures": {"total_units": 2, "matched_functions": 1, "matched_code": "12", "total_code": "100"},
    "units": [
        {"name": "main/Mod/unit", "functions": [{"name": "fn_80004000", "fuzzy_match_percent": 85.0}]},
        {"name": "main/auto_00_80004000_init", "functions": [{"name": "named_thing"}]},
    ],
}


def fixture() -> led.Ledger:
    by_name = {symbol["name"]: symbol for symbol in SYMBOLS}
    by_section: dict[str, list[dict]] = {}
    for symbol in SYMBOLS:
        by_section.setdefault(symbol["section"], []).append(symbol)
    for entries in by_section.values():
        entries.sort(key=lambda symbol: symbol["address"])
    return led.Ledger(
        symbols=(by_name, by_section, {}),
        splits=SPLITS,
        configured=CONFIGURED,
        report=REPORT,
        config_path=os.path.join(ROOT, "build", "RMHE08", "does-not-exist.json"),
    )


def rows():
    ledger = fixture()

    totals = ledger.totals()
    yield "functions counted", totals["functions"], 2
    yield "data symbols counted", totals["data_symbols"], 2
    yield "covered", totals["claimed_functions"], 1
    yield "closed at 85", totals["closed"], 1
    yield "nothing partial", totals["partial"], 0
    yield "the rest is unclaimed", totals["unclaimed"], 1
    yield "units registered", totals["units_registered"], 1

    # A function entry without the score key is 0 %, not 100 %.
    yield "missing score is zero", ledger.score({"unit": "Mod/unit.c"}, entry("named_thing", ".init", 0x80004100, "function", 0x40)), 0.0

    nxt = ledger.next(10)
    yield "next is the unclaimed function", [(row["name"], row["section"]) for row in nxt], [("named_thing", ".init")]
    yield "next skips generated names with --named", ledger.next(10, named_only=True)[0]["name"], "named_thing"
    yield "next can include data", [row["name"] for row in ledger.next(10, "data")], ["glob", "lbl_80006000"]
    yield "next stops at the limit", len(ledger.next(1, "all")), 1

    unit = ledger.unit("Mod/unit.c")
    yield "unit by source path", (unit["unit"], unit["closed"], unit["partial"]), ("Mod/unit.c", 1, 0)
    yield "unit by report name", ledger.unit("main/Mod/unit")["unit"], "Mod/unit.c"
    yield "unit without extension", ledger.unit("Mod/unit")["unit"], "Mod/unit.c"
    yield "unknown unit", ledger.unit("Nope/nope.c"), None
    yield "unit resolves its build", (unit["configured"]["lib"], unit["configured"]["flag"]), ("mod", "NonMatching")
    yield "unit symbols carry scores", [(s["name"], s["score"]) for s in unit["symbols"]], [("fn_80004000", 85.0)]
    yield "modules table has one registered row", [row["module"] for row in ledger.modules()], ["Mod", "(unclaimed)"]

    # Names that carry no information yet, and names that do.
    yield "generated names", [bool(led.GENERATED_RE.match(name)) for name in
                              ("fn_80004000", "lbl_80006000", "jumptable_805CEF78", "RSOLink")], [True, True, True, False]
    yield "report name mapping", led.report_name("Runtime.PPCEABI.H/memset.c"), "main/Runtime.PPCEABI.H/memset"
    yield "bare unit name", led.bare("main/Mod/unit"), "Mod/unit"


def stale_rows() -> int:
    """The one input that is a build output: an old report must be called out, not trusted."""
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        report = os.path.join(tmp, "report.json")
        source = os.path.join(tmp, "splits.txt")
        with open(report, "w") as fh:
            fh.write("{}")
        with open(source, "w") as fh:
            fh.write("")
        now = time.time()
        os.utime(report, (now - 600, now - 600))
        os.utime(source, (now, now))
        checks = (
            ("stale report detected", led.stale(report, (source,)), True),
            ("missing report is stale", led.stale(os.path.join(tmp, "nope.json"), (source,)), True),
        )
        os.utime(report, (now + 600, now + 600))
        checks += (("fresh report accepted", led.stale(report, (source,)), False),)
    for label, got, want in checks:
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures


def objects_rows() -> int:
    """Which split object holds an address - the one fact `next` adds for step 2 of the campaign."""
    failures = 0
    units = [
        {"name": "auto_03_80010000_text", "code_size": 0x100, "data_size": 0},
        {"name": "auto_03_80020000_text", "code_size": 0, "data_size": 0},
        {"name": "auto_03_80030000_text", "code_size": 0x80, "data_size": 0},
        {"name": "auto_03_80040000_text", "code_size": 0, "data_size": 0},
        {"name": "Camellia/camellia", "code_size": 0, "data_size": 0},
    ]
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "config.json")
        with open(path, "w") as fh:
            json.dump({"units": units}, fh)
        objects = led.Objects(path)
    checks = (
        ("inside a sized object", os.path.basename(objects.covering(".text", 0x80010040) or ""), "auto_03_80010000_text.o"),
        ("past the end of it is nothing", objects.covering(".text", 0x80010100), None),
        ("unsized object runs to its neighbour", os.path.basename(objects.covering(".text", 0x8002FFF0) or ""), "auto_03_80020000_text.o"),
        ("the neighbour takes over at its own start", os.path.basename(objects.covering(".text", 0x80030000) or ""), "auto_03_80030000_text.o"),
        ("the last unsized object is open ended", os.path.basename(objects.covering(".text", 0x80049000) or ""), "auto_03_80040000_text.o"),
        ("unknown section", objects.covering(".data", 0x80010000), None),
    )
    for label, got, want in checks:
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    return failures


def main() -> int:
    failures = 0
    for label, got, want in rows():
        if got == want:
            print(f"ok    {label}")
        else:
            failures += 1
            print(f"FAIL  {label}\n        got:  {got!r}\n        want: {want!r}")
    failures += stale_rows() + objects_rows()
    print(f"{'FAILED' if failures else 'passed'}: {failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
