"""Target-object vs our-object comparisons: sections, bytes, symbols, relocations, undefined names, fingerprints.
Spec: docs/tools/spec/lib-objcompare.md. CLI: none (library)."""
from __future__ import annotations

import difflib
import hashlib
import json
import os
import re
from collections import Counter
from dataclasses import dataclass

from tools.lib import cache as _cache
from tools.lib import names as _names
from tools.lib.binary.elf import (SHN_UNDEF, SHT_NULL, SHT_RELA, SHT_STRTAB, SHT_SYMTAB, STB_GLOBAL,
                                  STB_WEAK, STT_FILE, Elf, ElfError, reloc_name)

#: Bookkeeping tables, not a unit's code or data: they move with the symbol table and the compiler version.
META_SECTIONS = frozenset({".comment", ".note.split", ".shstrtab", ".strtab", ".symtab", ".dynsym", ".dynstr"})
#: The data sections (a symbol-level `data` kind; `FLIP_DATA_SECTIONS` adds their relocation sections).
DATA_SECTIONS = frozenset({".data", ".sdata", ".sdata2", ".rodata", ".bss", ".sbss", ".ctors", ".dtors"})
FLIP_DATA_SECTIONS = DATA_SECTIONS | {".rela" + s for s in DATA_SECTIONS if s != ".sbss"}
#: Relocations into these are compiler bookkeeping (an extabindex entry covers a function, it cannot call one).
BOOKKEEPING_SECTIONS = ("extab", "extabindex")
#: mwldeppc defines the EABI small-data bases itself; `__start` is its default entry root.
EABI_LINKER_SYMBOLS = ("_SDA_BASE_", "_SDA2_BASE_")
ENTRY_SYMBOLS = ("__start",)
LINKER_SYMBOLS = EABI_LINKER_SYMBOLS + ENTRY_SYMBOLS
#: An lcf assignment (`_stack_addr = _stack_end + 0x10000;`) defines a symbol the linker supplies.
LINKER_ASSIGN_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_.]*)\s*=")
#: The link-symbol index cache (`build/` is gitignored, so it never dirties a tree).
LINK_INDEX_REL = os.path.join("build", "tmp", "undefrefs", "link-symbols.json")
LINK_INDEX_SCHEMA = 1
#: The relocation kinds relocdiff/relocaudit-era views name; every other kind renders numerically there
#: (Known gaps: the full `binary.elf` table would rename `R_PPC_11` -> `R_PPC_REL14` in their output).
LEGACY_NAMED_RELOCS = frozenset({1, 4, 5, 6, 10, 109})
#: The two string tables a symbol rename rewrites; the drift fingerprint skips them.
RENAME_FREE_SECTIONS = (".symtab", ".strtab")
#: Sections the touch fingerprint ignores (they move with names and the compiler version, never with code).
UNSTABLE_SECTIONS = (".symtab", ".strtab", ".shstrtab", ".comment")
#: `.rela<target>`: how MWCC spells a relocation section.
RELOC_PREFIX = ".rela"
MAX_RELOC_DIFFS = 6
MAX_RELOC_OFFSETS = 8
#: objdiff pairs by name but a pair whose sizes differ by more than this ratio reads as untouched.
OBJDIFF_SIZE_GAP = 1.5


def legacy_reloc_name(rtype: int, fallback: str = "R_PPC_%d") -> str:
    """The relocation name the legacy views print: named for `LEGACY_NAMED_RELOCS`, `fallback % rtype` otherwise."""
    return reloc_name(rtype) if rtype in LEGACY_NAMED_RELOCS else fallback % rtype


def load(obj) -> Elf:
    """`obj` as an `Elf`: a path, the bytes of an object, or an `Elf` already read."""
    return obj if isinstance(obj, Elf) else Elf.read(obj)


def be32(obj) -> Elf | None:
    """The ELF32 big-endian object with a section table at `obj`, or None (missing, unreadable, other)."""
    if isinstance(obj, (str, os.PathLike)) and not os.path.exists(obj):
        return None
    try:
        elf = load(obj)
    except (OSError, ElfError):
        return None
    if elf.ei_class != 1 or elf.ei_data != 2 or not elf.sections or elf.shstrndx >= elf.shnum:
        return None
    return elf


# --- bytes ----------------------------------------------------------------------------------------------------

def first_difference(mine: bytes, theirs: bytes) -> int | None:
    """The first offset where two byte strings differ, or None when their shared prefix is equal."""
    for i in range(min(len(mine), len(theirs))):
        if mine[i] != theirs[i]:
            return i
    return None


def differing_bytes(mine: bytes, theirs: bytes) -> int:
    """How many bytes differ, a length difference counting its extra bytes."""
    shared = min(len(mine), len(theirs))
    return sum(1 for i in range(shared) if mine[i] != theirs[i]) + abs(len(mine) - len(theirs))


# --- sections: sizes ------------------------------------------------------------------------------------------

def section_sizes(obj) -> dict[str, int]:
    """`{section: size}` for one object; an empty section is omitted."""
    return {s.name: s.size for s in load(obj).sections if s.size}


def size_gaps(target: dict[str, int], ours: dict[str, int], all_sections: bool = False):
    """`(ours_extra, target_extra)`, each a sorted `[(section, bigger size, smaller size)]`; metadata excluded
    unless `all_sections`. Pure."""
    names = set(target) | set(ours)
    if not all_sections:
        names -= META_SECTIONS
    extra, missing = [], []
    for name in sorted(names):
        t, o = target.get(name, 0), ours.get(name, 0)
        if o > t:
            extra.append((name, o, t))
        elif t > o:
            missing.append((name, t, o))
    return extra, missing


def object_sizes(obj) -> dict[str, tuple[int, int]]:
    """`{section: (size, align exponent)}` for the sections a flip compares (`objdump -h`'s view): every real
    section but the symbol/string/relocation tables, metadata and `.rela*` excluded; `{}` when unreadable."""
    elf = be32(obj)
    if elf is None:
        return {}
    out = {}
    for s in elf.sections:
        if s.type in (SHT_NULL, SHT_SYMTAB, SHT_STRTAB, SHT_RELA):
            continue
        if s.name.startswith((".comment", ".note.split", ".symtab", ".strtab", ".shstrtab", RELOC_PREFIX)):
            continue
        out[s.name] = (s.size, max(s.align, 1).bit_length() - 1)
    return out


def section_names(obj) -> list[str]:
    """Every named section of the object in header order (empty when unreadable)."""
    elf = be32(obj)
    return [s.name for s in elf.sections if s.name] if elf is not None else []


def section_data(obj, name: str) -> bytes | None:
    """The file bytes of section `name` (empty for NOBITS, as `objcopy -O binary` extracts it; None when absent).

    A repeated name (dtk writes two `.data` into one object) reads its LAST section, the one `object_sizes` sizes
    and the one `objcopy --only-section` leaves on top."""
    elf = be32(obj)
    found = [s for s in elf.sections if s.name == name] if elf is not None else []
    return found[-1].data if found else None


# --- sections: bytes and relocations --------------------------------------------------------------------------

def object_sections(obj, all_sections: bool = False) -> dict:
    """`{sections, order, relocs}` of one object: `sections` maps a content section to `{size, data}` (NOBITS keeps
    its size, no bytes), `order` is the header order, `relocs[target]` is `[(offset, type, symbol name)]` (a repeated
    relocation section name: the last wins). Metadata is omitted unless `all_sections`."""
    elf = load(obj)
    nsym = len(elf.symbols)
    order: list[str] = []
    sections: dict[str, dict] = {}
    relocs: dict[str, list] = {}
    by_rela: dict[int, list] = {}
    for r in elf.relocs():
        by_rela.setdefault(r.rela_index, []).append(
            (r.offset, r.type, r.symbol_name if r.symbol < nsym else "?%d" % r.symbol))
    for rela, _target in elf.rela_sections():
        relocs[rela.name[len(RELOC_PREFIX):]] = by_rela.get(rela.index, [])
    for s in elf.sections:
        if s.name.startswith(RELOC_PREFIX):
            continue
        if s.type == 0 or (not all_sections and (s.name in META_SECTIONS or s.name == "")):
            continue
        order.append(s.name)
        sections[s.name] = {"size": s.size, "data": s.data}
    return {"sections": sections, "order": order, "relocs": relocs}


def _fmt_offsets(pairs: list, limit: int = MAX_RELOC_OFFSETS) -> str:
    """`+0x14, +0xAC (R_PPC_ADDR32)` for a symbol's `(offset, type)` pairs, capped."""
    pairs = sorted(pairs)
    kinds = {typ for _off, typ in pairs}
    shown = pairs[:limit]
    if len(kinds) == 1 and kinds:
        body = ", ".join("+0x%X" % off for off, _typ in shown) + " (%s)" % reloc_name(next(iter(kinds)))
    else:
        body = ", ".join("+0x%X %s" % (off, reloc_name(typ)) for off, typ in shown)
    if len(pairs) > limit:
        body += ", ... (%d more)" % (len(pairs) - limit)
    return body


def _by_symbol(relocs: list) -> dict:
    out: dict[str, list] = {}
    for offset, typ, symbol in relocs:
        out.setdefault(symbol, []).append((offset, typ))
    return out


def reloc_reasons(ours: list, target: list, limit: int = MAX_RELOC_DIFFS) -> list[str]:
    """One section's relocation difference, paired by symbol name (the spelling a lane fixes); empty when equal."""
    ours_by, target_by = _by_symbol(ours), _by_symbol(target)
    reasons = []
    for name in sorted(set(ours_by) | set(target_by)):
        mine, theirs = ours_by.get(name), target_by.get(name)
        if mine is None:
            reasons.append("the target relocates `%s` at %s, ours does not" % (name, _fmt_offsets(theirs)))
        elif theirs is None:
            reasons.append("ours relocates `%s` at %s, the target does not" % (name, _fmt_offsets(mine)))
        elif sorted(mine) != sorted(theirs):
            reasons.append("relocations for `%s` differ: ours %s; target %s"
                           % (name, _fmt_offsets(mine), _fmt_offsets(theirs)))
    if len(reasons) > limit:
        reasons = reasons[:limit] + ["... and %d more relocation difference(s)" % (len(reasons) - limit)]
    return reasons


def section_reasons(ours: dict | None, target: dict | None, ours_relocs: list, target_relocs: list) -> list[str]:
    """Why one section differs, in reading order: size, bytes, relocations (`object_sections` entries)."""
    if ours is None:
        return ["missing from our object (the target's section is %d B)" % target["size"]]
    if target is None:
        return ["ours-extra: the target has no such section (%d B)" % ours["size"]]
    reasons = []
    if ours["size"] != target["size"]:
        delta = ours["size"] - target["size"]
        if delta > 0:
            reasons.append("ours-extra 0x%X (%d B): our section is longer" % (delta, delta))
        else:
            reasons.append("target-extra 0x%X (%d B): the target's section is longer" % (-delta, -delta))
    diff = first_difference(ours["data"], target["data"])
    if diff is not None:
        mine = ours["data"][diff] if diff < len(ours["data"]) else 0
        theirs = target["data"][diff] if diff < len(target["data"]) else 0
        reasons.append("bytes differ at +0x%X (ours %02x, target %02x) in %d of %d bytes"
                       % (diff, mine, theirs, differing_bytes(ours["data"], target["data"]),
                          max(len(ours["data"]), len(target["data"]))))
    reasons.extend(reloc_reasons(ours_relocs, target_relocs))
    return reasons


@dataclass(frozen=True)
class SectionGap:
    """One differing section: both sizes and the reasons (size, first byte and count, relocations)."""
    section: str
    ours: int
    target: int
    reasons: tuple[str, ...]

    @property
    def why(self) -> str:
        return "; ".join(self.reasons)

    def to_dict(self) -> dict:
        return {"section": self.section, "ours": self.ours, "target": self.target, "why": self.why}

    def finding(self, unit: str):
        from tools.lib.findings import Finding
        return Finding("section-gap", unit, 0, self.section, self.why)


def sections(target, ours, all_sections: bool = False) -> list[SectionGap]:
    """The differing sections in the target's header order (then ours-only ones); empty when the two objects agree
    on every compared section's size, bytes and relocations. Each side is an object or an `object_sections` dict."""
    t = target if isinstance(target, dict) else object_sections(target, all_sections)
    o = ours if isinstance(ours, dict) else object_sections(ours, all_sections)
    names = list(t["order"]) + [n for n in o["order"] if n not in t["sections"]]
    rows = []
    for name in names:
        mine, theirs = o["sections"].get(name), t["sections"].get(name)
        reasons = section_reasons(mine, theirs, o["relocs"].get(name, []), t["relocs"].get(name, []))
        if reasons:
            rows.append(SectionGap(name, mine["size"] if mine else 0, theirs["size"] if theirs else 0,
                                   tuple(reasons)))
    return rows


# --- sections: layout (the permutation classes) -----------------------------------------------------------------

def section_symbols(obj, name: str) -> dict[str, tuple[int, int]]:
    """`{symbol: (offset in the section, size)}` for the named symbols defined in section `name`."""
    elf = be32(obj)
    if elf is None or elf.section(".symtab") is None or elf.section(".strtab") is None:
        return {}
    nsec = len(elf.sections)
    return {s.name: (s.value, s.size) for s in elf.symbols
            if s.name and s.shndx < nsec and elf.sections[s.shndx].name == name}


def mislaid_layout(mine: bytes, tgt: bytes, ours: dict, theirs: dict):
    """`(symbols compared, differing bytes)` when two same-sized sections hold the same symbols permuted: every
    shared symbol identical at its own address, at least two of them, at least one moved; else None. `ours` and
    `theirs` are `section_symbols` maps."""
    if len(mine) != len(tgt) or not mine:
        return None
    compared, moved = 0, False
    for sym in sorted(set(ours) & set(theirs)):
        o_off, o_size = ours[sym]
        t_off, t_size = theirs[sym]
        if o_size == 0 and t_size == 0:
            continue
        if o_size != t_size or o_off + o_size > len(mine) or t_off + t_size > len(tgt):
            return None
        if mine[o_off:o_off + o_size] != tgt[t_off:t_off + t_size]:
            return None
        compared += 1
        moved = moved or o_off != t_off
    if compared < 2 or not moved:
        return None
    return compared, differing_bytes(mine, tgt)


def mislaid_order(mine: bytes, tgt: bytes, ours: dict, theirs: dict):
    """`(moved, compared, bytes the layout explains, bytes left over)` for the weaker permutation: equal sizes,
    every shared symbol the same size, the moved ones covering more than half the section and more differing bytes
    outside the shared symbols than inside them; else None."""
    if len(mine) != len(tgt) or not mine:
        return None
    compared, moved, moved_bytes, own_diff = 0, 0, 0, 0
    for sym in sorted(set(ours) & set(theirs)):
        o_off, o_size = ours[sym]
        t_off, t_size = theirs[sym]
        if o_size != t_size:
            return None
        if o_size == 0:
            continue
        if o_off + o_size > len(mine) or t_off + t_size > len(tgt):
            return None
        compared += 1
        if o_off != t_off:
            moved += 1
            moved_bytes += o_size
        own_diff += sum(1 for i in range(o_size) if mine[o_off + i] != tgt[t_off + i])
    total = differing_bytes(mine, tgt)
    if compared < 2 or not moved or moved_bytes * 2 <= len(mine) or total - own_diff <= own_diff:
        return None
    return moved, compared, total - own_diff, own_diff


def section_byte_reasons(name: str, mine: bytes, tgt: bytes, ours: dict, theirs: dict) -> list[str]:
    """The flip refusal lines for one differing section: first difference, count, and the permutation class
    (`ours`/`theirs` are the section's `section_symbols`). The wording is parsed by lanes: add-only."""
    at = next((i for i in range(min(len(mine), len(tgt))) if mine[i] != tgt[i]), min(len(mine), len(tgt)))
    span = max(len(mine), len(tgt))
    problems = ["%s: bytes differ from the target object at +0x%X (ours %02x, target %02x) - "
                "the object is not the original's code"
                % (name, at, mine[at] if at < len(mine) else 0, tgt[at] if at < len(tgt) else 0),
                "%s: %d of %d bytes differ from the target object" % (name, differing_bytes(mine, tgt), span)]
    perm = mislaid_layout(mine, tgt, ours, theirs)
    if perm is not None:
        problems.append("%s: the section is a permutation - every one of the %d symbol(s) defined in it has "
                        "its original bytes at its own address and the sizes agree, but the object's layout "
                        "is the source's definition order, not the address order; order (or forward-declare) "
                        "the source so the layout matches (%d of %d bytes mislaid)"
                        % (name, perm[0], perm[1], span))
        return problems
    order = mislaid_order(mine, tgt, ours, theirs)
    if order is not None:
        problems.append("%s: the section's layout is a permutation - %d of the %d symbol(s) defined in it sit "
                        "at a different address than the target's and every shared symbol keeps its size, and "
                        "%d of the %d differing bytes sit outside the symbols' own addresses (only %d differ "
                        "inside them), so the object's layout is the source's definition order, not the "
                        "address order; order (or forward-declare) the source so the layout matches"
                        % (name, order[0], order[1], order[2], differing_bytes(mine, tgt), order[3]))
    return problems


# --- symbols ----------------------------------------------------------------------------------------------------

CLASS_ORDER = ("size-gap", "missing", "extra")
MODE_CLASSES = {"gap": ("size-gap",), "missing": ("missing",), "extra": ("extra",), "all": CLASS_ORDER}


def section_kind(section: str) -> str:
    """`meta` | `code` | `data` | `other` for one section name."""
    if section in META_SECTIONS or section.startswith(RELOC_PREFIX) or section.startswith(".note"):
        return "meta"
    if section in (".text", ".init") or section.startswith(".text.") or section.startswith(".init."):
        return "code"
    if section in DATA_SECTIONS:
        return "data"
    return "other"


def wanted_kinds(scope: str, all_sections: bool = False) -> set[str]:
    """The section kinds a `--sections code|data|all` scope selects."""
    if scope == "code":
        return {"code"}
    if scope == "data":
        return {"data"}
    return {"code", "data", "other", "meta"} if all_sections else {"code", "data", "other"}


@dataclass(frozen=True)
class SymbolSize:
    """One defined symbol: its size and the section it lives in."""
    name: str
    size: int
    section: str


@dataclass(frozen=True)
class SymbolGap:
    """One `symbols` row: `cls` is size-gap / missing / extra; `delta` is 0.0 (equal) .. 1.0 (one side absent)."""
    cls: str
    name: str
    target_size: int
    ours_size: int
    section: str
    delta: float


def defined_symbols(obj, kinds: set[str]) -> dict[str, SymbolSize]:
    """`{name: SymbolSize}` for every named symbol `obj` defines in a section of the wanted kinds (a reserved
    section index - ABS, COMMON - has no section and is skipped; a repeated name: the last wins)."""
    elf = load(obj)
    nsec = len(elf.sections)
    out: dict[str, SymbolSize] = {}
    for s in elf.symbols:
        if not s.name or not s.shndx or s.shndx >= nsec:
            continue
        sname = elf.sections[s.shndx].name
        if section_kind(sname) not in kinds:
            continue
        out[s.name] = SymbolSize(s.name, s.size, sname)
    return out


def size_delta(target_size: int, ours_size: int) -> float:
    """`(bigger - smaller) / bigger`: 0.0 equal .. 1.0 one side zero; a 50 % gap = the smaller is under half."""
    hi = max(target_size, ours_size)
    if hi <= 0:
        return 0.0
    return (hi - min(target_size, ours_size)) / hi


def symbols(target: dict, ours: dict, threshold: float = 50.0, mode: str = "all") -> list[SymbolGap]:
    """The size-gap / missing / extra rows between two `{name: SymbolSize}` maps. `threshold` (percent) gates the
    size-gap class only (strictly greater); absent-on-one-side is always 100 % apart. Sorted by class, then the
    bigger size descending, then name."""
    classes = MODE_CLASSES[mode]
    frac = max(0.0, threshold) / 100.0
    rows: list[SymbolGap] = []
    for name in set(target) | set(ours):
        t, o = target.get(name), ours.get(name)
        if t and o:
            delta = size_delta(t.size, o.size)
            if delta > frac and "size-gap" in classes:
                rows.append(SymbolGap("size-gap", name, t.size, o.size, t.section or o.section, delta))
            continue
        if t and "missing" in classes:
            rows.append(SymbolGap("missing", name, t.size, 0, t.section, 1.0))
        elif o and "extra" in classes:
            rows.append(SymbolGap("extra", name, 0, o.size, o.section, 1.0))
    rows.sort(key=lambda r: (CLASS_ORDER.index(r.cls), -max(r.target_size, r.ours_size), r.name))
    return rows


def symbol_locations(obj) -> dict[str, tuple[str, int, int, bytes]]:
    """`{symbol: (section, offset, size, bytes)}` for every named symbol defined in a real section (a repeated
    name keeps the first definition) - the raw side of a per-symbol comparison, no objdiff involved."""
    elf = load(obj)
    out: dict[str, tuple[str, int, int, bytes]] = {}
    nsec = len(elf.sections)
    for s in elf.symbols:
        if not s.name or not s.shndx or s.name in out or s.shndx >= nsec:
            continue
        sec = elf.sections[s.shndx]
        out[s.name] = (sec.name, s.value, s.size, bytes(sec.raw[s.value:s.value + s.size]))
    return out


def symbol_rows(target, ours) -> dict[str, dict]:
    """Per symbol (keyed by the target's name): both sizes, both presences, byte identity and how ours was found.

    A name the candidate lacks is resolved by ADDRESS: the candidate symbol at the same section and offset (the
    equal-size one first; never a sizeless label) stands in with `resolved_by="address"` - dtk names a range it
    cannot attribute (`pad_*`), and objdiff can never pair it. `identical` stays strict: equal sizes and bytes."""
    t = symbol_locations(target)
    c = symbol_locations(ours)
    rows: dict[str, dict] = {}
    for name in set(t) | set(c):
        te, ce = t.get(name), c.get(name)
        resolved_by = "name" if ce is not None else None
        candidate_name = name if ce is not None else None
        if ce is None and te is not None:
            found = [(n, size, data) for n, (sec, off, size, data) in c.items()
                     if sec == te[0] and off == te[1] and size > 0]
            found.sort(key=lambda e: (e[1] != te[2], e[1]))
            if found:
                candidate_name, cand_size, cand_data = found[0]
                ce, resolved_by = (te[0], te[1], cand_size, cand_data), "address"
        rows[name] = {"target_size": te[2] if te else None, "candidate_size": ce[2] if ce else None,
                      "in_target": te is not None, "in_candidate": ce is not None,
                      "identical": bool(te and ce and te[2] == ce[2] and te[3] == ce[3]),
                      "candidate_name": candidate_name, "resolved_by": resolved_by}
    return rows


# --- relocations ------------------------------------------------------------------------------------------------

def _reloc_targets(elf: Elf) -> dict[int, str]:
    """`{rela section index: the section it applies to}` (`Elf.rela_sections`' rule: `sh_info` when it names a real
    section, else the name with `.rela` dropped)."""
    return {s.index: target for s, target in elf.rela_sections()}


def reloc_rows(obj) -> tuple[dict[str, list[tuple[int, str, int, int]]] | None, str | None]:
    """`({section: sorted [(offset, symbol, type, addend)]}, None)` or `(None, why)` when unreadable - None, not
    `{}`, so a missing build artefact is never a falsely clean unit."""
    path = obj if isinstance(obj, (str, os.PathLike)) else "<bytes>"
    try:
        elf = load(obj)
    except OSError as exc:
        return None, "cannot read %s: %s" % (path, exc)
    except ElfError:
        return None, "%s is not an ELF object: %s" % (path, "not an ELF object")
    targets = _reloc_targets(elf)
    out: dict[str, list[tuple[int, str, int, int]]] = {}
    for r in elf.relocs():
        section = targets.get(r.rela_index) or r.rela or "?"
        out.setdefault(section, []).append((r.offset, r.symbol_name or "", r.type, r.addend))
    for rows in out.values():
        rows.sort()
    return out, None


def reloc_classes(target: list, ours: list) -> dict:
    """The four-class diff of two `(offset, symbol, type, addend)` lists. Pure.

    Identity first (a multiset), so an unchanged relocation is never reported; the rest pair by offset:
    (c) same offset, different symbol; (d) same symbol, different type/addend; plus target-only and ours-only."""
    o, t = Counter(map(tuple, ours)), Counter(map(tuple, target))
    identical = sum((o & t).values())
    o_by, t_by = {}, {}
    for off, sym, typ, add in (o - t).elements():
        o_by.setdefault(off, []).append((sym, typ, add))
    for off, sym, typ, add in (t - o).elements():
        t_by.setdefault(off, []).append((sym, typ, add))
    only_target, only_ours, diffsym, diffattr = [], [], [], []
    for off in sorted(set(o_by) | set(t_by)):
        mine, theirs = o_by.get(off, []), t_by.get(off, [])
        while mine and theirs:
            a, b = mine.pop(0), theirs.pop(0)
            (diffsym if a[0] != b[0] else diffattr).append((off, a, b))
        only_ours.extend((off,) + x for x in mine)
        only_target.extend((off,) + x for x in theirs)
    return {"ours": len(ours), "target": len(target), "matched": identical,
            "only_target": only_target, "only_ours": only_ours,
            "different_symbol": diffsym, "different_attr": diffattr,
            "identical": not (only_target or only_ours or diffsym or diffattr)}


def relocs(target, ours, sections: list[str] | None = None) -> dict[str, dict]:
    """`{section: reloc_classes}` for every section either object relocates (narrowed to `sections`)."""
    t, err = reloc_rows(target)
    if err:
        raise ValueError(err)
    o, err = reloc_rows(ours)
    if err:
        raise ValueError(err)
    names = sorted(set(o) | set(t))
    if sections:
        names = [n for n in names if n in set(sections)]
    return {n: reloc_classes(t.get(n, []), o.get(n, [])) for n in names}


def owner_groups(obj) -> dict:
    """`{(section, owner): [(offset in owner, type name, symbol, addend)]}` in offset order; the owner is the
    defined symbol containing the relocated offset (None: grouped at the absolute offset)."""
    elf = load(obj)
    nsec = len(elf.sections)
    table: dict[str, list] = {}
    for s in elf.symbols:
        sec = elf.sections[s.shndx].name if 0 < s.shndx < nsec else None
        if s.name and sec and s.type in (0, 1, 2):
            table.setdefault(sec, []).append(s)
    for lst in table.values():
        lst.sort(key=lambda s: (s.value, -s.size))
    targets = _reloc_targets(elf)
    out: dict = {}
    for r in elf.relocs():
        tsec = targets.get(r.rela_index)
        own = None
        for s in table.get(tsec, ()):
            if s.value > r.offset:
                break
            if r.offset < s.value + max(s.size, 1):
                own = s
        key = (tsec, own.name if own else None)
        off = r.offset - own.value if own else r.offset
        out.setdefault(key, []).append((off, legacy_reloc_name(r.type, "type-%d"),
                                        r.symbol_name or "<section-symbol>", r.addend))
    for lst in out.values():
        lst.sort()
    return out


def by_owner(target, ours) -> tuple[int, int, list[str]]:
    """`(matching, total, lines)`: relocations aligned in ORDER within each owning symbol, so a moved function or
    a slid instruction is not a difference; same names at other offsets is a non-failing `note` line."""
    a, b = owner_groups(ours), owner_groups(target)
    lines: list[str] = []
    notes: list[str] = []
    matched = 0
    for key in sorted(set(a) | set(b), key=lambda k: (k[0] or "", k[1] or "")):
        sec, own = key
        x, y = a.get(key, []), b.get(key, [])
        if own and not x:
            lines.append("only in target  %s %s: absent from ours (%d relocation(s))" % (sec, own, len(y)))
            continue
        if own and not y:
            lines.append("only in ours    %s %s: absent from the target (%d relocation(s))" % (sec, own, len(x)))
            continue
        where = "%s %s" % (sec, own) if own else sec
        sm = difflib.SequenceMatcher(None, [i[1:] for i in x], [i[1:] for i in y], autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == "equal":
                matched += i2 - i1
                if [i[0] for i in x[i1:i2]] != [i[0] for i in y[j1:j2]]:
                    notes.append("note            %s: %d relocation(s) at different offsets, same names"
                                 % (where, i2 - i1))
                continue
            for k in range(max(i2 - i1, j2 - j1)):
                xi = x[i1 + k] if i1 + k < i2 else None
                yi = y[j1 + k] if j1 + k < j2 else None
                at = "+0x%x" % (xi or yi)[0]
                if xi and yi:
                    what = []
                    if xi[1] != yi[1]:
                        what.append("type %s vs %s" % (xi[1], yi[1]))
                    if xi[2] != yi[2]:
                        what.append("symbol %s vs %s" % (xi[2], yi[2]))
                    if xi[3] != yi[3]:
                        what.append("addend %+d vs %+d" % (xi[3], yi[3]))
                    lines.append("differs         %s%s: %s  (ours vs target)" % (where, at, "; ".join(what)))
                elif xi:
                    lines.append("extra in ours   %s%s: ours has %s %s%+d" % (where, at, xi[1], xi[2], xi[3]))
                else:
                    lines.append("missing in ours %s%s: target has %s %s%+d" % (where, at, yi[1], yi[2], yi[3]))
    total = max(sum(map(len, a.values())), sum(map(len, b.values())))
    return matched, total, lines + notes


# --- callees: symbol-name relocation differences per written function -------------------------------------------

#: A compiler-local name in OUR object: a pool/jump-table/string label (`@123`, `@stringBase0`, `@4@x`), a local
#: static's counter (`x$123`) or a section symbol. It names nothing the source chose, so it never is a callee.
LOCAL_LABEL_RE = re.compile(r"^(?:@|\.|<)|\$\d+")
#: The target's spelling of the same labels: the map's pool/jump-table names (`lbl_<ADDR>`, `jumptable_<ADDR>`) or a
#: compiler label the map kept. Dropped only one-for-one against a local label of ours with the same relocation type.
TARGET_LABEL_RE = re.compile(r"^(?:@|\.|<|jumptable_[0-9A-Fa-f]{8}$|lbl_[0-9A-Fa-f]{8}$)")
#: A function body this small in our object is a stub (`blr`, `li r3,0; blr`): not written, never judged.
STUB_MAX_BYTES = 8


def callee_kind(ours: str | None, target: str | None) -> str:
    """How one paired difference reads: `mangling` (the same stem, two manglings), `linkage` (the same stem, one side
    C), `callee` (a different symbol), or `extra`/`missing` (no counterpart)."""
    if target is None:
        return "extra"
    if ours is None:
        return "missing"
    so, st = _names.owner_stem(ours), _names.owner_stem(target)
    if so == st:
        return "mangling" if so != ours and st != target else "linkage"
    return "callee"


def _functions(elf: Elf) -> dict[str, tuple[int, int]]:
    """`{name: (value, size)}` of the defined functions in `.text`."""
    nsec = len(elf.sections)
    return {s.name: (s.value, s.size) for s in elf.symbols
            if s.name and 0 < s.shndx < nsec and elf.sections[s.shndx].name == ".text" and s.type == 2}


def callee_diffs(target, ours) -> list[dict]:
    """Per function both objects define in `.text` that ours has written (larger than `STUB_MAX_BYTES`): the
    relocation symbol names that differ, `[{function, diffs: [{kind, ours, target, offset}]}]` (only functions with
    a difference). The two name sequences are aligned per function (`difflib`, as `by_owner` does); the unaligned
    rows then cancel by name, so a relocation that only moved (a slid instruction) is not a difference; then each local label of ours (`LOCAL_LABEL_RE`) cancels one target pool or jump-table
    label (`TARGET_LABEL_RE`) of the same relocation type, and the leftover local labels are dropped (noise). The
    remaining names pair in offset order (`offset` is the target's, else ours', in the function); `callee_kind`
    classifies each pair."""
    o_elf, t_elf = load(ours), load(target)
    o_funcs, t_funcs = _functions(o_elf), _functions(t_elf)
    a, b = owner_groups(o_elf), owner_groups(t_elf)
    out = []
    for name in sorted(set(o_funcs) & set(t_funcs), key=lambda n: t_funcs[n][0]):
        if o_funcs[name][1] <= STUB_MAX_BYTES:
            continue
        mine = [(off, typ, sym) for off, typ, sym, _add in a.get((".text", name), [])]
        theirs = [(off, typ, sym) for off, typ, sym, _add in b.get((".text", name), [])]
        sm = difflib.SequenceMatcher(None, [r[2] for r in mine], [r[2] for r in theirs], autojunk=False)
        left_o, left_t = [], []
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op != "equal":
                left_o += mine[i1:i2]
                left_t += theirs[j1:j2]
        common = Counter(r[2] for r in left_o) & Counter(r[2] for r in left_t)   # a row that only moved
        for left in (left_o, left_t):
            budget = Counter(common)
            keep = []
            for row in left:
                if budget[row[2]]:
                    budget[row[2]] -= 1
                else:
                    keep.append(row)
            left[:] = keep
        locals_by_type = Counter(typ for _o, typ, sym in left_o if LOCAL_LABEL_RE.search(sym))
        left_o = [r for r in left_o if not LOCAL_LABEL_RE.search(r[2])]
        kept_t = []
        for row in left_t:
            if TARGET_LABEL_RE.search(row[2]) and locals_by_type[row[1]]:
                locals_by_type[row[1]] -= 1
                continue
            kept_t.append(row)
        diffs = []
        for i in range(max(len(left_o), len(kept_t))):
            o = left_o[i] if i < len(left_o) else None
            t = kept_t[i] if i < len(kept_t) else None
            diffs.append({"kind": callee_kind(o and o[2], t and t[2]), "ours": o and o[2], "target": t and t[2],
                          "offset": (t or o)[0]})
        if diffs:
            out.append({"function": name, "diffs": diffs})
    return out


# --- undefined names: what a flip would leave unresolved --------------------------------------------------------

def reloc_facts(obj) -> dict | None:
    """`{relocs, defined, refs}` for an object, or None when it cannot be read.

    `relocs` are `{section, target, offset, type, type_name, symbol, addend}` dicts (`target` = the section the
    relocation applies to); `defined` is `{name: (section, st_info)}` for every symbol the object defines, locals
    included; `refs` every name its non-bookkeeping relocations reference."""
    try:
        elf = load(obj)
    except (OSError, ElfError, ValueError):
        return None
    nsec = len(elf.sections)
    targets = _reloc_targets(elf)
    defined = {}
    for s in elf.symbols:
        if s.name and s.shndx:
            defined[s.name] = (elf.sections[s.shndx].name if 0 < s.shndx < nsec else None, s.info)
    nsym = len(elf.symbols)
    rows = []
    refs = set()
    for r in elf.relocs():
        target = targets.get(r.rela_index)
        symbol = r.symbol_name if r.symbol < nsym else None
        rows.append({"section": r.rela, "target": target, "offset": r.offset, "type": r.type,
                     "type_name": legacy_reloc_name(r.type, "type-%d"), "symbol": symbol, "addend": r.addend})
        if symbol and not (target or "").startswith(BOOKKEEPING_SECTIONS):
            refs.add(symbol)
    return {"relocs": rows, "defined": defined, "refs": refs}


def provides_global(entry) -> bool:
    """Whether a `defined` entry (`(section, st_info)`) has a binding the linker resolves across objects."""
    return bool(entry) and entry[1] >> 4 in (STB_GLOBAL, STB_WEAK)


def link_inputs(ninja: str) -> list[str] | None:
    """The object inputs on `main.elf`'s link line in the build file `ninja` (a tree's `build.ninja`), relative to
    the tree and `os.path.normpath`ed; None when there is no such file or no link edge. Only these objects matter
    for a link-wide question."""
    path = ninja
    if not os.path.exists(path):
        return None
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    for i, line in enumerate(lines):
        head = line.split(":", 1)[0]
        if not head.startswith("build ") or ": link " not in line or not head.rstrip().endswith("main.elf"):
            continue
        edge = [line]
        while edge[-1].rstrip().endswith("$"):
            i += 1
            edge.append(lines[i])
        inputs = []
        for token in " ".join(edge).replace("$", " ").split()[3:]:
            if token in ("|", "||"):
                break
            inputs.append(os.path.normpath(token.replace("\\", os.sep)))
        return inputs
    return None


def linker_assigned(ldscript: str) -> set[str]:
    """Names the linker script defines itself (`_stack_addr = ...;`): the link supplies them, no object does."""
    if not os.path.exists(ldscript):
        return set()
    out: set[str] = set()
    for line in open(ldscript, encoding="utf-8", errors="replace"):
        m = LINKER_ASSIGN_RE.match(line)
        if m:
            out.add(m.group(1))
    return out


def link_index(root: str, inputs: list[str] | None = None, cache_path: str | None = None,
               rebuild: bool = False) -> dict:
    """`{providers: {name: [input]}, ref_count: {name: n}, inputs: [input], refs: {input: names}}` over the link inputs.

    `providers` is the global/weak definition map, `ref_count` how many inputs reference each name, `refs` what
    each input references (empty for a missing or unreadable input). Per-input
    facts are cached (`LINK_INDEX_REL` under `root` by default) keyed by size+mtime; only changed inputs are
    re-read, and the cache is written only when an input appeared, vanished or moved."""
    inputs = (link_inputs(os.path.join(root, "build.ninja")) or []) if inputs is None else inputs
    cache_path = cache_path or os.path.join(root, LINK_INDEX_REL)
    cached = {}
    if not rebuild and os.path.exists(cache_path):
        try:
            loaded = json.load(open(cache_path, encoding="utf-8"))
            if isinstance(loaded, dict) and loaded.get("schema") == LINK_INDEX_SCHEMA:
                cached = loaded.get("inputs") or {}
        except (OSError, ValueError):
            cached = {}
    fresh: dict[str, dict] = {}
    changed = set(cached) != set(inputs)
    for rel in inputs:
        path = os.path.join(root, rel)
        sig = _cache.stat_key(path)
        if sig is None:
            continue
        old = cached.get(rel)
        if old and old.get("sig") == sig and "defined" in old and "refs" in old:
            fresh[rel] = old
            continue
        changed = True
        facts = reloc_facts(path)
        if facts is None:
            fresh[rel] = {"sig": sig, "defined": [], "refs": []}
            continue
        fresh[rel] = {"sig": sig, "defined": sorted(n for n, e in facts["defined"].items() if provides_global(e)),
                      "refs": sorted(facts["refs"])}
    providers: dict[str, list[str]] = {}
    ref_count: dict[str, int] = {}
    refs = {rel: set(fresh[rel]["refs"]) if rel in fresh else set() for rel in inputs}
    for rel, entry in fresh.items():
        for name in entry["defined"]:
            providers.setdefault(name, []).append(rel)
        for name in entry["refs"]:
            ref_count[name] = ref_count.get(name, 0) + 1
    if changed:
        try:
            os.makedirs(os.path.dirname(cache_path), exist_ok=True)
            tmp = cache_path + ".tmp"
            with open(tmp, "w", encoding="utf-8") as fh:
                json.dump({"schema": LINK_INDEX_SCHEMA, "inputs": fresh}, fh)
            os.replace(tmp, cache_path)
        except OSError:
            pass
    return {"providers": providers, "ref_count": ref_count, "inputs": inputs, "refs": refs}


def external_candidates(ours: dict, known: set[str]) -> list[tuple[str, int, str]]:
    """`(section, offset, name)` for every non-bookkeeping relocation of `ours` (a `reloc_facts`) whose name it
    does not define and `known` (map + linker names) does not name; first occurrence of each name."""
    out = []
    seen = set()
    for r in ours["relocs"]:
        name = r["symbol"]
        if not name or (r["target"] or "").startswith(BOOKKEEPING_SECTIONS):
            continue
        if name in ours["defined"] or name in known or name in seen:
            continue
        seen.add(name)
        out.append((r["target"], r["offset"], name))
    return out


def spelling_hint(section: str, offset: int, name: str, target: dict | None) -> tuple[str, str] | None:
    """`(target spelling, how)` for a name the target records differently: the one other name at the same
    relocation slot (`same offset`), else the one target name with the same linkage stem (`same stem`)."""
    if target is None:
        return None
    at = {r["symbol"] for r in target["relocs"]
          if r["symbol"] and r["target"] == section and r["offset"] == offset
          and not (r["target"] or "").startswith(BOOKKEEPING_SECTIONS)}
    other = sorted(x for x in at if x != name)
    if len(other) == 1:
        return other[0], "same offset"
    stem = _names.linkage_stem(name)
    same = sorted({n for n in (set(target["defined"]) | target["refs"]) if n != name and
                   _names.linkage_stem(n) == stem})
    if len(same) == 1:
        return same[0], "same stem"
    return None


def undefined(ours: dict, target: dict | None, *, map_set: set[str], providers: dict, ref_count: dict,
              target_rel: str, linker_set: set[str]) -> list[tuple[str, tuple[str, str] | None]]:
    """`[(name, spelling hint)]` our object relocates that no link input a flip keeps can define.

    A name is fine when our object defines it, the map or the linker names it, another link input (not the target,
    which a flip replaces) provides it, the target only references it too (already unresolved), or no input
    provides it while some input already references it (the link is already broken that way)."""
    known = map_set | linker_set
    target_refs = target["refs"] if target is not None else set()
    target_defined = target["defined"] if target is not None else {}
    target_providers = {target_rel} if target is not None else set()
    hits = []
    for section, offset, name in external_candidates(ours, known):
        if set(providers.get(name, ())) - target_providers:
            continue
        if name in target_refs and not provides_global(target_defined.get(name)):
            continue
        if not providers.get(name) and ref_count.get(name, 0) > 0:
            continue
        hits.append((name, spelling_hint(section, offset, name, target)))
    return hits


# --- linkage spellings (relocaudit's sweep) ---------------------------------------------------------------------

def linkage_sets(obj) -> tuple[set[str], set[str]] | None:
    """`(global/weak defined, undefined)` names of an ELF32 big-endian object, None when it is not one. Excluded:
    `STT_FILE`, `@`-prefixed compiler labels, local definitions, empty names."""
    try:
        elf = load(obj)
    except (OSError, ElfError):
        return None
    if elf.ei_class != 1 or elf.ei_data != 2:
        return None
    defined, undefined_ = set(), set()
    for s in elf.symbols:
        n = s.name
        if not n or n.startswith("@") or s.type == STT_FILE:
            continue
        if s.shndx == SHN_UNDEF:
            undefined_.add(n)
        elif s.bind in (STB_GLOBAL, STB_WEAK):
            defined.add(n)
    return defined, undefined_


def linkage_audit(our_defined, our_undefined, tgt_defined, tgt_undefined) -> dict:
    """The names our object emits that the target does not, under the same spelling. Pure.

    A target definition satisfies one of our references (a partial unit calls into its own TU). Four sorted lists of
    `{"our": name, "target": [target names with the same linkage stem]}`: `linkage_undefined`, `other_undefined`,
    `linkage_defined`, `other_defined` - a non-empty `target` list is the wrong-linkage signal."""
    tgt_names = set(tgt_defined) | set(tgt_undefined)

    def classify(ours, target_has):
        linkage, other = [], []
        for s in sorted(ours - target_has):
            cands = sorted(t for t in tgt_names if t != s and _names.linkage_stem(t) == _names.linkage_stem(s))
            (linkage if cands else other).append({"our": s, "target": cands})
        return linkage, other

    link_u, other_u = classify(set(our_undefined), tgt_names)
    link_d, other_d = classify(set(our_defined), set(tgt_defined))
    return {"linkage_undefined": link_u, "other_undefined": other_u,
            "linkage_defined": link_d, "other_defined": other_d}


# --- fingerprints -----------------------------------------------------------------------------------------------

def file_sha256(path: str) -> str:
    """The sha256 of a file's bytes (the raw half the gate pairs with `fingerprint` to tell a names-only change)."""
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def fingerprint(path: str) -> str:
    """A rename-insensitive content fingerprint of a split target object (the drift check).

    Every section but the two string tables (name, size, contents), plus the defined symbols' geometry
    `(value, size, type, section index)` with the names dropped: a `splits.txt` re-range moves bytes or addresses,
    a rename moves neither. A `SHT_NOBITS` section (`.bss`/`.sbss`) contributes no contents: dtk writes its
    header offset as 0, so its file slice is the ELF header, whose section-table offset moves with `.strtab`'s
    length. A file that is not an ELF object falls back to `raw:<sha256>`."""
    try:
        elf = Elf.read(path)
    except Exception:                                            # noqa: BLE001 - not an ELF (or truncated)
        return "raw:" + file_sha256(path)
    h = hashlib.sha256()
    for sec in elf.sections:
        if sec.name in RENAME_FREE_SECTIONS:
            continue
        h.update(("%s\0%08x\0" % (sec.name, sec.size)).encode())
        h.update(sec.data)
    for geom in sorted((s.value, s.size, s.type, s.shndx) for s in elf.symbols if s.name and s.shndx):
        h.update(("|%08x/%08x/%d/%d" % geom).encode())
    return h.hexdigest()


def touch_fingerprint(path: str, symbols: dict | None = None) -> dict | None:
    """`{body, ext}` of a compiled object (the TOUCHED rule), None when unreadable.

    `body` hashes everything but external targets (sections, defined symbols, relocations to defined symbols by
    their section/offset); `ext` is `{"section|offset|type|addend": [name, "SECTION:ADDR" | None]}`, the address
    from `symbols` (`{name: {section, address}}`, the tree's map; a name the map lacks resolves by its linkage
    stem). JSON-ready."""
    try:
        elf = Elf.read(path)
    except (OSError, ElfError, ValueError):
        return None
    symbols = symbols or {}
    nsec = len(elf.sections)
    h = hashlib.sha1()
    for sec in elf.sections:
        if sec.name in UNSTABLE_SECTIONS or sec.type == SHT_RELA or not sec.name:
            continue
        h.update(("S|%s|%d|%d|%d|%d|" % (sec.name, sec.type, sec.flags, sec.align, sec.size)).encode())
        h.update(sec.data)

    def section_of(s):
        return elf.sections[s.shndx].name if 0 < s.shndx < nsec else None

    defined = [s for s in elf.symbols if s.name and s.shndx]
    by_name = {s.name: s for s in defined}
    rows = ["D|%s|%d|%d|%d|%d" % (section_of(s), s.value, s.size, s.bind, s.type) for s in defined]
    targets = _reloc_targets(elf)
    nsym = len(elf.symbols)
    ext: dict[str, list] = {}
    for r in elf.relocs():
        name = (r.symbol_name if r.symbol < nsym else None) or ""
        tsec = targets.get(r.rela_index)
        own = by_name.get(name)
        if own is not None:
            rows.append("R|%s|%d|%d|%d|L|%s|%d" % (tsec, r.offset, r.type, r.addend, section_of(own), own.value))
            continue
        rows.append("R|%s|%d|%d|%d|X" % (tsec, r.offset, r.type, r.addend))
        entry = symbols.get(name) or symbols.get(_names.linkage_stem(name))
        ext["%s|%d|%d|%d" % (tsec, r.offset, r.type, r.addend)] = \
            [name, "%s:%d" % (entry["section"], entry["address"]) if entry else None]
    for row in sorted(rows):
        h.update(row.encode() + b"\n")
    return {"body": h.hexdigest(), "ext": ext}


def fingerprints_equal(a: dict, b: dict) -> bool:
    """Whether two `touch_fingerprint`s are the same object: equal bodies, the same external slots, and each slot's
    target the same address (both resolve) or the same spelling (one does not). Pure."""
    if a.get("body") != b.get("body") or set(a.get("ext") or {}) != set(b.get("ext") or {}):
        return False
    for key, (name_a, addr_a) in (a.get("ext") or {}).items():
        name_b, addr_b = b["ext"][key]
        if addr_a is not None and addr_b is not None:
            if addr_a != addr_b:
                return False
        elif name_a != name_b:
            return False
    return True

