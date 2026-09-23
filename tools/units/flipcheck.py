"""Check whether a unit can be flipped to Object(Matching, ...): can our object fill the region?

A flip replaces the original bytes with our compiled object, so the object has to provide every section the
unit's `splits.txt` entry claims - same size, same alignment. Comparing against the *target object* is not
enough: dtk's split object is itself incomplete (a unit can claim extab/extabindex/data ranges that no single
object in the build emits), which is how a byte-identical object still scrambles main.dol.

Usage:
    python tools/units/flipcheck.py                 # every registered unit
    python tools/units/flipcheck.py <unit> [...]    # named units
    python tools/units/flipcheck.py --selftest      # the `.comment` check, against fixtures only
Exit status is non-zero when any unit is not flip-ready.
"""
from __future__ import annotations

import argparse
import os
import re
import struct
import subprocess
import sys

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OBJDUMP = os.path.join(MAIN, "build", "binutils", "powerpc-eabi-objdump.exe")
SPLITS = os.path.join(MAIN, "config", "RMHE08", "splits.txt")
SRC = os.path.join(MAIN, "build", "RMHE08", "src")

# the unit spelling rule has exactly one definition (`claims.norm_unit`), so `flipcheck.py runtime.c` and
# `flipcheck.py runtime` name the same unit and the same object (aliased: `claims` is a local function here)
sys.path.insert(0, os.path.join(MAIN, "tools"))
from units import claims as claims_mod  # noqa: E402

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


# Row 36 (docs/matching.md): `dol split` writes the target objects with `export_all: true`, which stamps
# `active_flags=0x08` (force-active / export) on every entry of the `.comment` symbol table, while MWCC writes
# 0x00. The linker honours the flag, so a symbol the target exports and our object does not - *and that
# nothing in the link references* - is deadstripped, taking its extab/extabindex with it and shifting every
# later section. The comparison must pair entries by symbol name: the two `.comment` tables are in ELF
# symbol-table order and the orders differ (measured on `sys_mem`: the target groups the extab symbols first,
# ours interleaves the 0-size labels). A symbol counts as referenced when any object relocates it from a
# code/data section, or when the linker script's FORCEACTIVE block roots it; only an extabindex entry (which
# covers the function and cannot root it) does not.
COMMENT_HEADER = 0x2C
ACTIVE_EXPORT = 0x08
# Relocation sections whose target is bookkeeping: an extabindex entry points at the function it covers, so
# it cannot keep that function alive on its own. `.ctors`/`.dtors` DO root their targets (a static ctor).
BOOKKEEPING_SECTIONS = ("extab", "extabindex")
# A symbol defined in one of these is fragment/metadata data, not trimmable code.
FRAGMENT_PREFIXES = ("extab", "extabindex", ".ctors", ".dtors")
LDSCRIPT = os.path.join(MAIN, "build", "RMHE08", "ldscript.lcf")
# mwldeppc's default entry symbol: the linker roots it, but it is not in FORCEACTIVE and no object relocates it.
ENTRY_SYMBOLS = ("__start",)


def elf_sections(path: str) -> tuple[list[str], dict[str, bytes]]:
    """([section names in shndx order], {name: raw bytes}) read straight from the ELF.

    `raw_section` above goes through objcopy, whose `-O binary` drops non-allocatable sections - `.comment`
    is one of them - so the flag table has to be read from the file itself.
    """
    if not os.path.exists(path):
        return [], {}
    data = open(path, "rb").read()
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] != 2:      # ELF32, big-endian
        return [], {}
    shoff, = struct.unpack_from(">I", data, 0x20)
    shentsize, = struct.unpack_from(">H", data, 0x2E)
    shnum, = struct.unpack_from(">H", data, 0x30)
    shstrndx, = struct.unpack_from(">H", data, 0x32)
    if not shoff or not shnum or shstrndx >= shnum:
        return [], {}
    raw = []
    for i in range(shnum):
        name, _typ, _flags, _addr, offset, size, _link, _info, _align, _entsize = struct.unpack_from(
            ">IIIIIIIIII", data, shoff + i * shentsize)
        raw.append((name, offset, size))
    stroff = raw[shstrndx][1]

    def name_at(o: int) -> str:
        end = data.find(b"\0", stroff + o)
        return data[stroff + o:end].decode("latin1")

    order = [name_at(n) for n, _, _ in raw]
    return order, {order[i]: data[raw[i][1]:raw[i][1] + raw[i][2]] for i in range(shnum)}


def comment_symbols(path: str) -> list[dict] | None:
    """The `.comment` symbol table as [{name, size, section, active_flags}], or None without a `.comment`.

    One 8-byte entry per ELF symbol, starting at 0x2C: `[align:4][visibility:1][active_flags:1][pad:2]`
    (docs/comment_section.md). The table follows ELF symbol-table order, so callers pair entries by name.
    """
    order, secs = elf_sections(path)
    com = secs.get(".comment")
    symtab = secs.get(".symtab")
    strtab = secs.get(".strtab")
    if com is None:
        return None
    if symtab is None or strtab is None or len(com) < COMMENT_HEADER:
        return []
    out = []
    for i in range((len(com) - COMMENT_HEADER) // 8):
        entry = com[COMMENT_HEADER + 8 * i:COMMENT_HEADER + 8 * i + 8]
        name = size = shndx = 0
        if (i + 1) * 16 <= len(symtab):
            name, _value, size, _info, _other, shndx = struct.unpack_from(">IIIBBH", symtab, i * 16)
        end = strtab.find(b"\0", name) if name < len(strtab) else -1
        out.append({"name": strtab[name:end].decode("latin1") if end != -1 else "",
                    "size": size,
                    "section": order[shndx] if shndx < len(order) else "",
                    "active_flags": entry[5]})
    return out


def code_references_in(path: str) -> set[str]:
    """The symbol names `path` references from a non-bookkeeping section (code, data or ctors)."""
    _order, secs = elf_sections(path)
    symtab = secs.get(".symtab")
    strtab = secs.get(".strtab")
    if symtab is None or strtab is None:
        return set()
    names = []
    for i in range(len(symtab) // 16):
        name, _value, _size, _info, _other, _shndx = struct.unpack_from(">IIIBBH", symtab, i * 16)
        end = strtab.find(b"\0", name) if name < len(strtab) else -1
        names.append(strtab[name:end].decode("latin1") if end != -1 else "")
    refs = set()
    for sec, data in secs.items():
        if not sec.startswith(".rela") or sec[5:].startswith(BOOKKEEPING_SECTIONS):
            continue
        for i in range(len(data) // 12):
            _off, info, _add = struct.unpack_from(">IIi", data, i * 12)
            index = info >> 8
            if index < len(names) and names[index]:
                refs.add(names[index])
    return refs


def forced_active(path: str = LDSCRIPT) -> set[str]:
    """The FORCEACTIVE symbols in the linker script - roots the linker will not deadstrip."""
    if not os.path.exists(path):
        return set()
    out: set[str] = set()
    inside = False
    for line in open(path, encoding="utf-8", errors="replace"):
        stripped = line.strip()
        if stripped.startswith("FORCEACTIVE"):
            inside = True
            continue
        if inside:
            if stripped == "}":
                break
            if stripped and stripped != "{" and not stripped.startswith(("/*", "//")):
                out.add(stripped.split()[0])
    return out


def code_reference_index(roots: list[str]) -> tuple[set[str], int]:
    """Every symbol name the link references from code/data, and how many objects were read."""
    refs: set[str] = set()
    count = 0
    for root in roots:
        for dirpath, _dirs, files in os.walk(root):
            for f in sorted(files):
                if f.endswith(".o"):
                    count += 1
                    refs |= code_references_in(os.path.join(dirpath, f))
    return refs, count


def comment_trim_risks(unit: str, obj_path: str, src_path: str,
                       refs: set[str]) -> tuple[list[str], int, bool]:
    """Row 36: target-exported symbols our object leaves un-exported that nothing in the link references.

    Returns (problems, exported symbols examined, whether both `.comment` sections were readable).
    """
    target = comment_symbols(obj_path)
    ours = comment_symbols(src_path)
    if target is None or ours is None:
        return [], 0, False
    mine: dict[str, list[int]] = {}
    for entry in ours:
        mine.setdefault(entry["name"], []).append(entry["active_flags"])
    problems = []
    checked = 0
    for entry in target:
        if not entry["active_flags"] & ACTIVE_EXPORT:
            continue
        checked += 1
        name = entry["name"]
        if not name or entry["size"] == 0:
            continue                     # a 0-size label has nothing to trim
        if entry["section"].startswith(FRAGMENT_PREFIXES):
            continue                     # extab/extabindex/.ctors/.dtors data is a fragment, not trimmable code
        flags = mine.get(name)
        if flags is None:
            continue                     # absent from our object: a missing-symbol problem, not a trim one
        if any(f & ACTIVE_EXPORT for f in flags):
            continue                     # already exported
        if name in refs:
            continue                     # referenced from code/data somewhere: the linker keeps it
        problems.append("%s: .comment marks %s (0x%X bytes) force-active (0x08) but our object does not, and "
                        "no code/data relocation in the link references it - the linker will deadstrip it and "
                        "shift every later section (row 36); mark it __declspec(export)"
                        % (unit, name, entry["size"]))
    return problems, checked, True


def check(unit: str, claim: dict[str, tuple[int, int]], refs: set[str] | None) -> tuple[list[str], list[str]]:
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

    # row 36: a byte-identical object can still break the DOL if the linker deadstrips a trailing function
    # our `.comment` does not force-active. Needs the whole link's reference set, so it is passed in.
    flag_problems, checked, compared = ([], 0, False)
    if refs is not None:
        flag_problems, checked, compared = comment_trim_risks(
            unit, os.path.join(MAIN, "build", "RMHE08", "obj", unit + ".o"),
            os.path.join(SRC, unit + ".o"), refs)
    problems += flag_problems
    if compared and not flag_problems:
        notes.append(".comment: no un-exported symbol at deadstrip risk (row 36, %d target-exported symbol(s) "
                     "checked)" % checked)
    return problems, notes


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Is a unit ready to flip to Object(Matching, ...)? Checks the object against the claim "
                    "its splits.txt entry makes (sections, sizes, alignment) and against the target object's "
                    "bytes. The DOL itself remains the only proof.")
    ap.add_argument("units", nargs="*")
    ap.add_argument("--selftest", action="store_true", help="run the self-test and exit")
    args = ap.parse_args()
    if args.selftest:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import flipcheck_selftest
        return flipcheck_selftest.selftest()

    refs, objects = code_reference_index([os.path.join(MAIN, "build", "RMHE08", "obj"), SRC])
    if objects:
        refs |= forced_active() | set(ENTRY_SYMBOLS)
    else:
        refs = None

    all_claims = claims()
    if args.units:
        wanted = {}
        for u in args.units:
            key = claims_mod.norm_unit(u.strip("/"))
            wanted[key] = all_claims.get(key)
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
        problems, notes = check(unit, claim, refs)
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
