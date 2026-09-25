#!/usr/bin/env python3
"""Fast, deterministic self-test for tools/units/symbolpreflight.py.

    python tools/units/preflight_selftest.py

No build and no shelling out: it imports the pre-flight module and checks its report against a fixture
table of real repo data, so a symbols.txt / splits.txt / configure.py parser regression is caught in
milliseconds. The table is the contract - when the repo legitimately changes one of these symbols, the
row has to change with it.

Three rows have moved since the table was first written, all because the `.init` runtime was reconstructed,
never because the tool changed (`severity_for` is byte-identical to its first commit):

* `memset` at `.init:0x80004350` started as collision kind 2 (no owner) and became kind 1 (approve) once
  `Runtime.PPCEABI.H/memset.c` existed in splits.txt + configure.py. Commit 3e5a0727 then flipped that
  object to `Object(Matching, ...)` (the first linked object), so it is now kind 8 (`never touch`, owner
  `Runtime.PPCEABI.H/memset.c`): a Matching object's bytes are linked in place of the original, and
  re-attributing the region would move main.dol's hash for everyone.
* `memcpy` at `.init:0x80004000` was the kind-2 (no owner) representative. Commit 480f5b0d ("close the
  .init runtime") added `Runtime.PPCEABI.H/memcpy.c` to splits.txt and configure.py as Matching, so it
  too is now kind 8. The uncovered (kind 2) path is exercised instead by `CleanUpTracks`, a still-unsplit
  `.text` function.

A row is only a snapshot of the repo: when configure.py / splits.txt legitimately move one of these
symbols, the row has to move with it - but the change is checked against the commit that moved it, never
loosened to make the suite pass.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

if os.path.join(ROOT, "tools", "units") not in sys.path:
    sys.path.insert(0, os.path.join(ROOT, "tools", "units"))

import symbolpreflight as sp  # noqa: E402  (imported through the sys.path shim above)

# (label, target passed to report(), {field -> expected value}); field names come from facts().
ROWS = (
    ("RSOLink", "0x804DA598",
     {"name": "RSOLink", "kind": 1, "severity": "approve", "owner": "RSO/runtime.c"}),
    # 3e5a0727 flipped memset.c to Object(Matching): the bytes are linked in place -> kind 8, never touch.
    ("memset", "0x80004350",
     {"name": "memset", "kind": 8, "severity": "never touch", "owner": "Runtime.PPCEABI.H/memset.c"}),
    ("camellia_sp1110", "0x80570E98",
     {"name": "camellia_sp1110", "kind": 4, "severity": "approve", "owner": "Camellia/camellia.c"}),
    ("fn_804DA7E4", "0x804DA7E4",
     {"name": "fn_804DA7E4", "kind": 1, "severity": "approve", "problem_kinds": [9]}),
    # 480f5b0d closed the .init runtime: memcpy.c now owns 0x80004000 and is Matching -> kind 8.
    ("memcpy", "0x80004000",
     {"name": "memcpy", "kind": 8, "severity": "never touch", "owner": "Runtime.PPCEABI.H/memcpy.c"}),
    # The kind-2 (no splits.txt owner) path: still-unsplit .text, nothing claims this address.
    ("CleanUpTracks", "0x800938ec",
     {"name": "CleanUpTracks", "kind": 2, "severity": "proceed", "owner": None}),
    ("camellia_sp1110 boundary", "camellia_sp1110",
     {"next_name": "camellia_sp0222", "next_addr": 0x80571298}),
)


def split_symbol(text: str | None) -> tuple[str | None, int | None]:
    """Turn a report boundary string ("name @ 0xADDR") into (name, address); None-safe."""
    if not text:
        return None, None
    name, _, address = text.partition(" @ ")
    return name, int(address, 16)


def facts(data: dict) -> dict:
    """Flatten one report() result down to the fields the table asserts on."""
    if "error" in data:
        return {"error": data["error"]}
    previous_name, previous_addr = split_symbol(data["boundary"]["previous"])
    next_name, next_addr = split_symbol(data["boundary"]["next"])
    return {
        "name": data["symbol"]["name"],
        "kind": data["collision"]["kind"],
        "severity": data["collision"]["severity"],
        "owner": data["owner"]["unit"],
        "prev_name": previous_name,
        "prev_addr": previous_addr,
        "next_name": next_name,
        "next_addr": next_addr,
        "problem_kinds": [problem["kind"] for problem in data["problems"]],
    }


def main() -> int:
    checks = failures = 0
    print("symbolpreflight self-test")
    for label, target, expected in ROWS:
        try:
            observed = facts(sp.report(target))
        except Exception as exc:  # noqa: BLE001 - a raise is a failure to report, not to propagate
            observed = {"error": f"{type(exc).__name__}: {exc}"}
        errored = "error" in observed
        bad = []
        for field, want in expected.items():
            checks += 1
            got = observed.get(field)
            if errored or got != want:
                failures += 1
                bad.append((field, got, want))
        print(f"  {'FAIL' if bad else 'PASS'}  {label}  ({target})")
        if errored:
            print(f"        report() {observed['error']}")
        for field, got, want in bad:
            print(f"        {field}: observed={got!r} expected={want!r}")
    print(f"\n{checks - failures}/{checks} checks passed, {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
