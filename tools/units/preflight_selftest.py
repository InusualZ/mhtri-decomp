#!/usr/bin/env python3
"""Fast, deterministic self-test for tools/units/symbolpreflight.py.

    python tools/units/preflight_selftest.py

No build and no shelling out: it imports the pre-flight module and checks its report against a fixture
table of real repo data, so a symbols.txt / splits.txt / configure.py parser regression is caught in
milliseconds. The table is the contract - when the repo legitimately changes one of these symbols, the
row has to change with it.

One row records a divergence from the original brief. `memset` at `.init:0x80004350` was specified as
collision kind 2 (no owner), but in this worktree it *is* owned: `Runtime.PPCEABI.H/memset.c` exists,
is registered in configure.py and splits.txt covers 0x80004350-0x80004380, so the report is kind 1
(approve, owner `Runtime.PPCEABI.H/memset.c`). That is a stale expectation, not a bug in the tool; the
kind-2 (uncovered) path is still exercised by the `memcpy` row.
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
    # Stale in the brief: a splits.txt block for this unit now covers the address (see the docstring).
    ("memset", "0x80004350",
     {"name": "memset", "kind": 1, "severity": "approve", "owner": "Runtime.PPCEABI.H/memset.c"}),
    ("camellia_sp1110", "0x80570E98",
     {"name": "camellia_sp1110", "kind": 4, "severity": "approve", "owner": "Camellia/camellia.c"}),
    ("fn_804DA7E4", "0x804DA7E4",
     {"name": "fn_804DA7E4", "kind": 1, "severity": "approve", "problem_kinds": [9]}),
    ("memcpy", "0x80004000",
     {"name": "memcpy", "kind": 2, "severity": "proceed", "owner": None}),
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
