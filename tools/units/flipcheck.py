"""Check whether a unit can be flipped to Object(Matching, ...): can our object fill the region?

A flip replaces the original bytes with our compiled object, so the object has to provide every section the
unit's `splits.txt` entry claims - same size, same alignment. Comparing against the *target object* is not
enough: dtk's split object is itself incomplete (a unit can claim extab/extabindex/data ranges that no single
object in the build emits), which is how a byte-identical object still scrambles main.dol.

Usage:
    python tools/units/flipcheck.py                 # every registered unit
    python tools/units/flipcheck.py <unit> [...]    # named units
Exit status is non-zero when any unit is not flip-ready.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OBJDUMP = os.path.join(MAIN, "build", "binutils", "powerpc-eabi-objdump.exe")
SPLITS = os.path.join(MAIN, "config", "RMHE08", "splits.txt")
SRC = os.path.join(MAIN, "build", "RMHE08", "src")

# section names may or may not start with a dot: extab/extabindex do not.
SEC_RE = re.compile(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+[0-9a-f]+\s+2\*\*(\d+)")
CLAIM_RE = re.compile(r"^\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)(?:\s+rename:(\S+))?")
IGNORE = (".comment", ".note.split", ".symtab", ".strtab", ".shstrtab", ".rela")
# Fragments the *compiler* generates as a side effect of the unit's code: the exception tables and the
# constructor/destructor reference words. A matched unit produces them, so they are part of its match and are
# checked byte-for-byte below like any other section - contributing one is not a reason to withhold a flip.
# (The target object is dtk's synthesised object, so its *in-object section order* is dtk's, not the original
# compiler's, and comparing the two orders says nothing.)
COMPILER_GENERATED = ("extab", "extabindex", ".ctors", ".dtors")


def sections(path: str) -> dict[str, tuple[int, int]]:
    """{section name: (size, align exponent)} for a compiled object."""
    if not os.path.exists(path):
        return {}
    out = subprocess.run([OBJDUMP, "-h", path], capture_output=True, text=True).stdout
    res = {}
    for line in out.splitlines():
        m = SEC_RE.match(line)
        if m and not m.group(1).startswith(IGNORE):
            res[m.group(1)] = (int(m.group(2), 16), int(m.group(3)))
    return res


def claims() -> dict[str, dict[str, tuple[int, int]]]:
    """{unit: {section name as the object spells it: (claimed size, align exponent)}} from splits.txt."""
    units: dict[str, dict[str, tuple[int, int]]] = {}
    unit = None
    for line in open(SPLITS, encoding="utf-8"):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if not line[0].isspace():
            unit = re.sub(r"\.(c|cpp|cp|cc)$", "", line.split(":")[0].strip())
            units.setdefault(unit, {})
            continue
        m = CLAIM_RE.match(line)
        if m and unit:
            name = m.group(4) or m.group(1)          # rename:.ctors$10 -> .ctors$10
            size = int(m.group(3), 16) - int(m.group(2), 16)
            units[unit][name] = (size, 2)            # splits.txt states ranges, not alignment
    return units


def unit_name_for(path: str) -> str:
    """build/RMHE08/src/Network/NetworkWiiMediator.o -> Network/NetworkWiiMediator"""
    return os.path.relpath(path, SRC).replace("\\", "/")[:-2]


def raw_section(path: str, name: str) -> bytes | None:
    """The raw bytes of one section, via objcopy (None when the section is absent)."""
    if not os.path.exists(path):
        return None
    tmp = os.path.join(MAIN, ".pi", "_flipcheck.bin")
    r = subprocess.run([os.path.join(MAIN, "build", "binutils", "powerpc-eabi-objcopy.exe"),
                        "-O", "binary", "--only-section=" + name, path, tmp],
                       capture_output=True)
    if r.returncode != 0 or not os.path.exists(tmp):
        return None
    data = open(tmp, "rb").read()
    os.remove(tmp)
    return data


def check(unit: str, claim: dict[str, tuple[int, int]]) -> tuple[list[str], list[str]]:
    ours = sections(os.path.join(SRC, unit + ".o"))
    if not ours:
        return ["no compiled object (build/RMHE08/src/%s.o) - compile it first" % unit], []
    problems = []
    notes: list[str] = []
    for name, (size, _) in sorted(claim.items()):
        got = ours.get(name)
        if got is None:
            problems.append("splits.txt claims %s (0x%X) but the object emits no such section - "
                            "flipping drops %d bytes from the link and shifts everything after it"
                            % (name, size, size))
        elif got[0] != size:
            problems.append("%s: object is 0x%X, splits.txt claims 0x%X (%+d)" % (name, got[0], size, got[0] - size))
    for name in sorted(set(ours) - set(claim)):
        problems.append("%s (0x%X) is in the object but not claimed by splits.txt - "
                        "it will be linked somewhere the original had nothing" % (name, ours[name][0]))

    generated = sorted(n for n in ours if n.startswith(COMPILER_GENERATED))
    if generated:
        notes.append("compiler-generated fragments, byte-checked above: %s"
                     % ", ".join("%s 0x%X" % (n, ours[n][0]) for n in generated))

    # sizes and alignment matching is not enough: the bytes have to be the original's too.
    for name in sorted(set(ours) & set(claim)):
        mine = raw_section(os.path.join(SRC, unit + ".o"), name)
        tgt = raw_section(os.path.join(MAIN, "build", "RMHE08", "obj", unit + ".o"), name)
        if mine is None or tgt is None:
            continue
        if mine != tgt:
            at = next((i for i in range(min(len(mine), len(tgt))) if mine[i] != tgt[i]),
                      min(len(mine), len(tgt)))
            problems.append("%s: bytes differ from the target object at +0x%X (ours %02x, target %02x) - "
                            "the object is not the original's code"
                            % (name, at, mine[at] if at < len(mine) else 0, tgt[at] if at < len(tgt) else 0))
    return problems, notes


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Is a unit ready to flip to Object(Matching, ...)? Checks the object against the claim "
                    "its splits.txt entry makes (sections, sizes, alignment) and against the target object's "
                    "bytes. The DOL itself remains the only proof.")
    ap.add_argument("units", nargs="*")
    args = ap.parse_args()

    all_claims = claims()
    if args.units:
        wanted = {u: all_claims.get(u) for u in args.units}
        missing = [u for u, c in wanted.items() if c is None]
        for u in missing:
            print("%s: no splits.txt entry" % u)
        wanted = {u: c for u, c in wanted.items() if c is not None}
    else:
        wanted = {}
        for root, _, files in os.walk(SRC):
            for f in sorted(files):
                if f.endswith(".o"):
                    u = unit_name_for(os.path.join(root, f))
                    if u in all_claims:
                        wanted[u] = all_claims[u]

    bad = 0
    for unit, claim in sorted(wanted.items()):
        problems, notes = check(unit, claim)
        if problems:
            bad += 1
            print("NOT READY  %s" % unit)
            for p in problems:
                print("   - %s" % p)
        else:
            print("READY      %s (%d section(s) match the claim)" % (unit, len(claim)))
            for n in notes:
                print("   . %s" % n)
    print("\n%d of %d unit(s) ready" % (len(wanted) - bad, len(wanted)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
