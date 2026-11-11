#!/usr/bin/env python3
"""Compare a unit's exception-table records with the target's, one function at a time.
Spec: docs/tools/spec/extabdiff.md. CLI: extabdiff.py <unit>."""
from __future__ import annotations

import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import re

from tools.lib.binary import objdump

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME = "RMHE08"
RELOC_RE = re.compile(r"^([0-9a-f]+)\s+R_PPC_\w+\s+(\S+)")
DUMP_RE = re.compile(r"^ ([0-9a-f]+) ((?:[0-9a-f]{2,8} ?)+)")


def parse_symbols(symtab_text: str) -> dict[str, tuple[int, int]]:
    """`extab` record symbols of an `objdump -t` listing: name -> (offset, size)."""
    out = {}
    for line in symtab_text.splitlines():
        q = line.split()
        if len(q) >= 6 and "extab" in q and " O " in line:
            out[q[-1]] = (int(q[0], 16), int(q[q.index("extab") + 1], 16))
    return out


def parse_index(reloc_text: str) -> dict[int, dict[int, str]]:
    """The `extabindex` relocations of an `objdump -r` listing: entry -> {byte offset in the entry: symbol}."""
    ents: dict[int, dict[int, str]] = {}
    for line in reloc_text.splitlines():
        m = RELOC_RE.match(line)
        if m:
            off = int(m.group(1), 16)
            ents.setdefault(off // 12, {})[off % 12] = m.group(2)
    return ents


def parse_dump(dump_text: str) -> bytes:
    """The bytes of an `objdump -s` section dump."""
    raw = bytearray()
    for line in dump_text.splitlines():
        m = DUMP_RE.match(line)
        if m:
            raw += bytes.fromhex("".join(m.group(2).split()))
    return bytes(raw)


def pair_records(sym: dict[str, tuple[int, int]], ents: dict[int, dict[int, str]], raw: bytes) -> dict[str, bytes]:
    """function name -> its extab record bytes, paired through the index entry's two relocations."""
    out = {}
    for _, e in sorted(ents.items()):
        fn, ex = e.get(0), e.get(8)
        if ex in sym:
            off, size = sym[ex]
            out[fn] = raw[off:off + size]
    return out


def records(obj: str) -> dict[str, bytes]:
    tool = objdump.locate(ROOT) or "objdump"

    def run(*flags: str) -> str:
        return objdump.run_objdump(tool, list(flags), obj).stdout

    return pair_records(parse_symbols(run("-t")), parse_index(run("-r", "-j", "extabindex")), parse_dump(run("-s", "-j", "extab")))


def differing(target: dict[str, bytes], ours: dict[str, bytes]) -> list[tuple[str, int, int | None]]:
    """(function, target size, our size or None when missing) for every record that differs."""
    return [(fn, len(b), len(ours[fn]) if fn in ours else None) for fn, b in target.items() if ours.get(fn) != b]


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare a unit's extab records with the target's.")
    ap.add_argument("unit", help="unit key, e.g. g3d/g3d_anmchr")
    a = ap.parse_args()
    t = records(os.path.join(ROOT, "build", GAME, "obj", a.unit + ".o"))
    o = records(os.path.join(ROOT, "build", GAME, "src", a.unit + ".o"))
    bad = differing(t, o)
    for fn, tn, on in bad:
        print("%s: target %d B, ours %s" % (fn[:70], tn, on if on is not None else "missing"))
    print("differing %d of %d" % (len(bad), len(t)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
