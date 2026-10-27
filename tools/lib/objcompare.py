"""Target-object vs our-object comparisons: sections, bytes, symbols, relocations, undefined names, fingerprints.
Spec: docs/tools/spec/lib-objcompare.md. CLI: none (library)."""
from __future__ import annotations

import bisect
import difflib
import hashlib
import itertools
import json
import os
import re
from collections import Counter
from dataclasses import dataclass

from tools.lib import cache as _cache
from tools.lib import names as _names
from tools.lib.binary.elf import (SHN_UNDEF, SHT_NULL, SHT_RELA, SHT_STRTAB, SHT_SYMTAB, STB_GLOBAL, STT_SECTION,
                                  STB_LOCAL, STB_WEAK, STT_FILE, Elf, ElfError, reloc_name)

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
#: MWCC's numbered compiler-local names (`__arraydtor$9000`, `@1234` is a different spelling and not matched).
LOCAL_SUFFIX_RE = re.compile(r"\$\d+$")


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


#: Sections a trailing pad never excuses: code and its exception tables carry no alignment fill of their own.
PAD_NEVER = (".text", ".init", "extab", "extabindex")
#: The largest alignment a trailing pad may stand for.
PAD_MAX_ALIGN = 8
#: dtk's label for a padding run it assigned to a unit (`gap_09_8079398D_sdata`): fill, not content.
PAD_LABEL_PREFIX = "gap_"


def trailing_pad(section: str, ours_size: int, claim_size: int, align: int, target_bytes: bytes | None,
                 target_symbols: dict, start: int | None, next_aligns: list[int]) -> str | None:
    """Why a section `claim_size - ours_size` bytes short of its claim is only the alignment fill the link puts back,
    or None when it is not. All of: not `PAD_NEVER`; `align` (bytes, the larger of the two objects') at most
    `PAD_MAX_ALIGN` and `ours_size` rounded up to it is the claim; the target's tail is all zero (NOBITS: no bytes);
    no target symbol in the tail but dtk's `gap_` labels; and the next linked section of that name, at every
    alignment it may be linked with (`next_aligns`, its target's and ours'), starts exactly at the claim's end
    (`start` is the claim's address). An unknown next section is not accepted."""
    short = claim_size - ours_size
    if section in PAD_NEVER or short <= 0 or align > PAD_MAX_ALIGN or align < 1:
        return None
    if (ours_size + align - 1) // align * align != claim_size:
        return None
    if target_bytes is not None and (len(target_bytes) < claim_size or any(target_bytes[ours_size:claim_size])):
        return None
    inside = [n for n, (off, _size) in target_symbols.items()
              if ours_size <= off < claim_size and not n.startswith(PAD_LABEL_PREFIX)]
    if inside or start is None or not next_aligns:
        return None
    end = start + ours_size
    if any((end + a - 1) // a * a != start + claim_size for a in next_aligns):
        return None
    return ("%s: 0x%X of 0x%X bytes - the 0x%X-byte tail is alignment fill (target zero, no symbol, the next "
            "section aligned to %s starts at the claim's end)" % (section, ours_size, claim_size, short,
                                                                    "/".join(str(a) for a in sorted(set(next_aligns)))))


#: Sections a short claim is never excused in, even before a flipped successor: the exception tables.
LINKED_PAD_NEVER = ("extab", "extabindex", ".init")
#: The largest alignment a flipped successor may carry that the link pads up to (`-func_align 16` objects).
LINKED_PAD_MAX_ALIGN = 16
#: The largest alignment an UNFLIPPED successor may carry: its target object places itself at the alignment
#: splits.txt gives it (`align:32` on g3d/g3d_resanmscn's `.data`), so the link pads up to that.
UNFLIPPED_PAD_MAX_ALIGN = 32


def linked_trailing_pad(section: str, ours_size: int, claim_size: int, target_bytes: bytes | None,
                        target_symbols: dict, start: int | None, successor_aligns: list[int],
                        max_align: int = LINKED_PAD_MAX_ALIGN) -> str | None:
    """Why a section `claim_size - ours_size` bytes short of its claim is excused because its SUCCESSOR is a linked
    object: the link pads our section up to the successor's alignment. All of: not an exception table; the target's
    tail is all zero (NOBITS: no bytes) with no symbol in it but dtk's `gap_` labels; and the successor, at every
    alignment it carries (`successor_aligns`, each a power of two of at most `max_align`: `LINKED_PAD_MAX_ALIGN`
    for a flipped successor's own object, `UNFLIPPED_PAD_MAX_ALIGN` for an unflipped one's target object), starts
    exactly at the claim's end (`start` is the claim's address). No linked successor, no judgement."""
    short = claim_size - ours_size
    if section in LINKED_PAD_NEVER or short <= 0 or start is None or not successor_aligns:
        return None
    if any(a < 1 or a > max_align or a & (a - 1) for a in successor_aligns):
        return None
    if target_bytes is not None and (len(target_bytes) < claim_size or any(target_bytes[ours_size:claim_size])):
        return None
    if [n for n, (off, _size) in target_symbols.items()
            if ours_size <= off < claim_size and not n.startswith(PAD_LABEL_PREFIX)]:
        return None
    end = start + ours_size
    if any((end + a - 1) // a * a != start + claim_size for a in successor_aligns):
        return None
    return ("%s: 0x%X of 0x%X bytes - the 0x%X-byte tail is link padding (target zero, no symbol, the linked "
            "successor aligned to %s starts at the claim's end)" % (section, ours_size, claim_size, short,
                                                                      "/".join(str(a) for a in sorted(set(successor_aligns)))))


#: dtk's label for a leading padding run it assigned to a unit (`pad_10_80795090_sbss`: size, address, section).
LEAD_PAD_LABEL_RE = re.compile(r"^pad_[0-9A-Fa-f]+_[0-9A-Fa-f]{8}_")


def is_fill_label(name: str) -> bool:
    """Whether `name` is one of dtk's fill labels (`gap_...` or `pad_<size>_<address>_<section>`): fill, not content."""
    return name.startswith(PAD_LABEL_PREFIX) or bool(LEAD_PAD_LABEL_RE.match(name))


def leading_fill(section: str, ours_size: int, ours_align: int, claim_size: int, target_bytes: bytes | None,
                 target_symbols: dict, start: int | None, prev_end_ok: bool) -> int | None:
    """The size of the fill the link puts in FRONT of our section, or None when there is none to model.

    dtk can start a unit's claim at the previous object's end, before the unit's own aligned start (`DVD/dvd`'s
    `.sbss` claim 0x80795090 holds 0x10 bytes of fill before its 32-aligned data at 0x807950A0). Linking our object
    there, mwld places the section at the claim start rounded up to its alignment, so the lead comes back. All of:
    not an exception table or `.init`; `start` known and `ours_align` (bytes) a power of two; the lead (the start
    rounded up to `ours_align`, minus the start) is non-zero and `ours_size` plus it fits the claim; the target's
    lead bytes are all zero (NOBITS: no bytes); no target symbol starts inside the lead but dtk's fill labels; and
    `prev_end_ok` - the section linked before ours ends at the claim's start (the caller judges the predecessor)."""
    if section in LINKED_PAD_NEVER or start is None or ours_align < 2 or ours_align & (ours_align - 1):
        return None
    if not prev_end_ok:
        return None
    lead = (start + ours_align - 1) // ours_align * ours_align - start
    if lead <= 0 or ours_size + lead > claim_size:
        return None
    if target_bytes is not None and (len(target_bytes) < claim_size or any(target_bytes[:lead])):
        return None
    if [n for n, (off, _size) in target_symbols.items() if off < lead and not is_fill_label(n)]:
        return None
    return lead


#: Sections whose symbol offsets decide where every later object's data lands, so an object that matches in size
#: and bytes can still move the DOL: a spare global dead-stripped, a 4 B object where the original had 8 B, an order.
POSITIONED = (".bss", ".sbss", ".sbss2", ".sdata", ".sdata2")


def identity_symbols(m: dict) -> dict:
    """The entries of a `section_symbols` map whose name carries an identity: anonymous `@`-names, dtk's `gap_` fill
    labels, generated `lbl_`/`zz_` names (the target's spelling of compiler-pooled constants) and a function-local
    static's `name$NNN` ordinal are dropped."""
    return {n: v for n, v in m.items()
            if not n.startswith(("@", "lbl_", "zz_")) and not is_fill_label(n) and not re.search(r"\$\d+$", n)}


def data_symbol_gaps(ours: dict, theirs: dict, limit: int = 6, excused=frozenset()) -> list[str]:
    """Lines naming where our named symbols of one section sit at another offset than the target's, or a target
    symbol with a real name that our object does not define. `ours`/`theirs` are `section_symbols` maps; names
    without identity (`identity_symbols`) are skipped; a size is not compared (the section size and the offsets are
    what the link uses). A target name in `excused` (an `unnamed_spellings` name the caller has proved no other link
    input references) is not a missing definition."""
    o, t = identity_symbols(ours), identity_symbols(theirs)
    lines = ["%s: the target defines it at +0x%X (0x%X bytes), our object does not" % (n, t[n][0], t[n][1])
             for n in sorted(t, key=lambda n: t[n][0]) if n not in o and n not in excused]
    lines += ["%s: at +0x%X in our object, +0x%X in the target" % (n, o[n][0], t[n][0])
              for n in sorted(o, key=lambda n: o[n][0]) if n in t and o[n][0] != t[n][0]]
    return lines if len(lines) <= limit else lines[:limit] + ["... %d more" % (len(lines) - limit)]


def section_symbol_rows(obj, name: str) -> list[tuple[str, int, int, bool]]:
    """`[(symbol, offset, size, binds across objects)]` for every symbol defined in section `name`, unnamed ones
    included (they still cover bytes); section and file symbols are skipped."""
    elf = be32(obj)
    if elf is None or elf.section(".symtab") is None:
        return []
    nsec = len(elf.sections)
    return [(s.name, s.value, s.size, s.bind in (STB_GLOBAL, STB_WEAK)) for s in elf.symbols
            if s.type not in (STT_SECTION, STT_FILE) and 0 < s.shndx < nsec and elf.sections[s.shndx].name == name]


def unnamed_spellings(theirs: dict, ours_rows: list, mine: bytes | None, tgt: bytes | None) -> dict[str, tuple]:
    """`{target name: (offset, size)}` for the identity-carrying target symbols (`identity_symbols`) that our object
    does not define by name but emits as the same bytes at the same offset under no global name: the range lies inside
    both sections' bytes and is equal there, and every symbol of ours that overlaps it is local (MWCC's `@NNN` pool
    literal, a `static`) - the map's name for a word our compiler pools anonymously (`ef/ef_postfield`'s `ef_pf_*`
    `.sdata2` rows). `theirs` is the target's `section_symbols` map, `ours_rows` our `section_symbol_rows` with the
    offsets already shifted by any lead. A NOBITS section (no bytes), a zero-size row, a differing byte and a global
    or weak symbol of ours over the range all leave the name a missing definition; whether another link input
    references the name is the caller's check."""
    if not mine or not tgt:
        return {}
    ours_names = {n for n, _o, _s, _g in ours_rows}
    end = min(len(mine), len(tgt))
    out = {}
    for n, (off, size) in identity_symbols(theirs).items():
        if n in ours_names or size <= 0 or off < 0 or off + size > end or mine[off:off + size] != tgt[off:off + size]:
            continue
        if any(g for _n, o, s, g in ours_rows if o < off + size and off < o + max(s, 1)):
            continue
        out[n] = (off, size)
    return out


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
    cannot attribute (`pad_*`), and objdiff can never pair it. A compiler-local `stem$NNNN` neither name nor address finds
    resolves by STEM (`resolved_by="stem"`) when exactly one unpaired candidate shares it. `identical` stays strict:
    equal sizes and bytes."""
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
            elif LOCAL_SUFFIX_RE.search(name):
                # a compiler-local `stem$NNNN` (`__arraydtor$9000`): the number moves with every declaration the
                # TU sees, and objdiff pairs it by stem - so does this, when exactly one unpaired candidate shares it.
                stem = LOCAL_SUFFIX_RE.sub("", name)
                same = [n for n, e in c.items() if n not in t and e[0] == te[0] and LOCAL_SUFFIX_RE.search(n)
                        and LOCAL_SUFFIX_RE.sub("", n) == stem]
                if len(same) == 1:
                    candidate_name, ce, resolved_by = same[0], c[same[0]], "stem"
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


# --- relocation places: where each relocation resolves, in the link's address space -----------------------------

def moved_offset(moves: list[tuple[int, int, int]], offset: int) -> int | None:
    """Where `offset` of a section lands under `strip_moves`' `[(start, end, new start)]`; None inside a stripped span."""
    for lo, hi, new_lo in moves:
        if lo <= offset < hi:
            return new_lo + offset - lo
    return None


def reloc_places(obj, bases: dict[str, int], shifts: dict[str, int] | None = None,
                 addresses: dict[str, int] | None = None,
                 moves: dict[str, list[tuple[int, int, int]]] | None = None) -> dict[str, dict[int, list[tuple]]] | None:
    """`{applied section: {offset: [(type, place)]}}`: where every relocation of one object lands once it is linked
    at the claim, or None when the object cannot be read. Two objects can carry equal (masked) bytes and still point
    their relocations at different copies of identical data; comparing names cannot see it (ours says `@123`, the
    target `lbl_80793A10`), comparing places can.

    A place is `("addr", address)` when it is known: a named global or weak symbol with an `addresses` row (the map:
    the link resolves a name, and a weak copy the link drops resolves to the kept one), else a symbol the object
    defines in a section with a `bases` start (its claimed address) plus the symbol's value, the section's `shifts`
    (a leading fill) and the addend; an undefined name with an `addresses` row is that row plus the addend. Otherwise
    `("name", name, addend)` for an undefined name the map does not carry and `("in", section, offset)` for a local
    symbol in a section with no base. A section with `moves` (`strip_moves`: the link dead-strips part of it) has its
    offsets - the relocations' and the symbols' - mapped through them; a relocation inside a stripped span is dropped
    and a local symbol inside one resolves to None (not compared). Relocation offsets carry the section's shift too."""
    elf = be32(obj)
    if elf is None:
        return None
    shifts = shifts or {}
    addresses = addresses or {}
    moves = moves or {}

    def place_in(sec: str, offset: int) -> int | None:
        if sec in moves:
            return moved_offset(moves[sec], offset)
        return offset + shifts.get(sec, 0)

    syms = elf.symbols
    nsec = len(elf.sections)
    targets = _reloc_targets(elf)
    out: dict[str, dict[int, list[tuple]]] = {}
    for r in elf.relocs():
        applied = targets.get(r.rela_index) or r.section
        at = place_in(applied, r.offset)
        if at is None:
            continue
        sym = syms[r.symbol] if r.symbol < len(syms) else None
        place = None
        if sym is not None and sym.name and sym.bind in (STB_GLOBAL, STB_WEAK) and sym.name in addresses:
            place = ("addr", addresses[sym.name] + r.addend)
        elif sym is not None and 0 < sym.shndx < nsec:
            sec = elf.sections[sym.shndx].name
            if sym.type == STT_SECTION:
                value = place_in(sec, r.addend)
            else:
                value = place_in(sec, sym.value)
                value = None if value is None else value + r.addend
            if value is not None:
                place = ("addr", bases[sec] + value) if sec in bases else ("in", sec, value)
        elif sym is not None and sym.name:
            place = ("name", sym.name, r.addend)
        out.setdefault(applied, {}).setdefault(at, []).append((r.type, place))
    return out


def format_place(place: tuple | None) -> str:
    """One place of `reloc_places` as text: `0x80793A10`, `name+0x4`, `.sdata+0x8`."""
    if place is None:
        return "?"
    if place[0] == "addr":
        return "0x%08X" % place[1]
    head, off = place[1], place[2]
    return "%s%s" % (head, "%+#x" % off if off else "")


def reloc_place_diffs(ours: dict[int, list[tuple]], target: dict[int, list[tuple]]) -> list[tuple]:
    """`[(offset, ours [(type, place)], target [(type, place)])]` for every offset BOTH objects relocate where the
    two resolve to different places (or relocation types). An offset only one side relocates is not this question;
    a place None on either side (a local symbol inside a stripped span) is not compared. Pure."""
    out = []
    for off in sorted(set(ours) & set(target)):
        mine, theirs = sorted(ours[off], key=repr), sorted(target[off], key=repr)
        if any(p is None for _t, p in mine + theirs):
            continue
        if mine != theirs:
            out.append((off, mine, theirs))
    return out


# --- callees: symbol-name relocation differences per written function -------------------------------------------

#: A compiler-local name in OUR object: a pool/jump-table/string label (`@123`, `@stringBase0`, `@4@x`), a local
#: static's counter (`x$123`) or a section symbol. It names nothing the source chose, so it never is a callee.
LOCAL_LABEL_RE = re.compile(r"^(?:@|\.|<)|\$\d+")
#: The target's spelling of the same labels: the map's pool/jump-table names (`lbl_<ADDR>`, `jumptable_<ADDR>`) or a
#: compiler label the map kept. Dropped only one-for-one against a local label of ours with the same relocation type.
TARGET_LABEL_RE = re.compile(r"^(?:@|\.|<|jumptable_[0-9A-Fa-f]{8}$|lbl_[0-9A-Fa-f]{8}$)")
#: A function body this small in our object is a stub (`blr`, `li r3,0; blr`): not written, never judged.
STUB_MAX_BYTES = 8


def _label_addresses(elf: Elf) -> dict[str, tuple[str, int]]:
    """`{symbol name: (section name, value)}` of every defined symbol (the first definition of a name)."""
    nsec = len(elf.sections)
    out: dict[str, tuple[str, int]] = {}
    for s in elf.symbols:
        if s.name and 0 < s.shndx < nsec and s.name not in out:
            out[s.name] = (elf.sections[s.shndx].name, s.value)
    return out


def callee_kind(ours: str | None, target: str | None) -> str:
    """How one paired difference reads: `mangling` (the same stem, two manglings), `linkage` (the same stem, one side
    C), `callee` (a different symbol), or `extra`/`missing` (no counterpart). (`name-only` - the same address under
    another name - is decided by `callee_diffs`, which can see the addresses.)"""
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
    classifies each pair. Before that, one of our local labels and one target symbol of the same relocation type that
    resolve to the same address (section, symbol value + addend) pair as `name-only`: our compiler-local name against
    the map's name for the same data, a naming difference and no wrong callee."""
    o_elf, t_elf = load(ours), load(target)
    o_addr, t_addr = _label_addresses(o_elf), _label_addresses(t_elf)
    o_funcs, t_funcs = _functions(o_elf), _functions(t_elf)
    a, b = owner_groups(o_elf), owner_groups(t_elf)
    out = []
    for name in sorted(set(o_funcs) & set(t_funcs), key=lambda n: t_funcs[n][0]):
        if o_funcs[name][1] <= STUB_MAX_BYTES:
            continue
        mine = [(off, typ, sym, add) for off, typ, sym, add in a.get((".text", name), [])]
        theirs = [(off, typ, sym, add) for off, typ, sym, add in b.get((".text", name), [])]
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
        diffs = []

        def at(table: dict, row: tuple):
            base = table.get(row[2])
            return (base[0], base[1] + row[3]) if base else None
        for row in [r for r in left_o if LOCAL_LABEL_RE.search(r[2])]:
            here = at(o_addr, row)
            twin = next((t for t in left_t if here is not None and t[1] == row[1] and t[2] != row[2]
                         and not TARGET_LABEL_RE.search(t[2])
                         and at(t_addr, t) == here), None)
            if twin is not None:
                left_o.remove(row)
                left_t.remove(twin)
                diffs.append({"kind": "name-only", "ours": row[2], "target": twin[2], "offset": twin[0]})
        locals_by_type = Counter(typ for _o, typ, sym, _a in left_o if LOCAL_LABEL_RE.search(sym))
        left_o = [r for r in left_o if not LOCAL_LABEL_RE.search(r[2])]
        kept_t = []
        for row in left_t:
            if TARGET_LABEL_RE.search(row[2]) and locals_by_type[row[1]]:
                locals_by_type[row[1]] -= 1
                continue
            kept_t.append(row)
        for i in range(max(len(left_o), len(kept_t))):
            o = left_o[i] if i < len(left_o) else None
            t = kept_t[i] if i < len(kept_t) else None
            diffs.append({"kind": callee_kind(o and o[2], t and t[2]), "ours": o and o[2], "target": t and t[2],
                          "offset": (t or o)[0]})
        if diffs:
            out.append({"function": name, "diffs": diffs})
    return out


# --- values: what a renamed relocation target actually holds ----------------------------------------------------

#: The relocation kinds whose target is a datum an instruction or a table reads: the low half of an address, a
#: 16-bit address, a small-data access and a 32-bit address. `@ha`/`@hi` carry only the high half (their `@l`
#: partner is compared) and a branch's target is code (`callee_diffs`' question), so those are not read.
VALUE_RELOCS = frozenset({1, 3, 4, 109})          # ADDR32, ADDR16, ADDR16_LO, EMB_SDA21
#: How many bytes an address-taken target (`addi`, a table word) is read for when its symbol states no size.
VALUE_DEFAULT_WIDTH = 4
#: The longest datum one pair reads (a string or a table behind `addi`).
VALUE_MAX_WIDTH = 256
#: How many bytes of a longer datum `format_value` spells before `...` (the comparison reads all of them).
VALUE_SHOWN_BYTES = 24
#: FP loads/stores of a single (lfs/lfsu/stfs/stfsu) and of a double (lfd/lfdu/stfd/stfdu): shown as numbers.
_FP_SINGLE_OPS = frozenset({48, 49, 52, 53})
_FP_DOUBLE_OPS = frozenset({50, 51, 54, 55})
#: lmw/stmw move a run of words: their target is read as a datum, not as one access.
_MULTIPLE_OPS = frozenset({46, 47})
SHF_EXECINSTR = 0x4
#: How many symbols starting at or before a relocated offset are tried as its owner (aliases and nested labels).
OWNER_LOOKBACK = 8


def _owned_relocs(elf: Elf) -> tuple[dict, dict]:
    """`({(section, owner name): [(offset in owner, type, Symbol, addend, instruction word or None)]},
    {(section, owner name): the owner's bytes})` in offset order: the owner is the sized defined function/object
    containing the relocated offset (a relocation outside every sized symbol is not grouped); the word is the
    `.text` instruction the relocation patches."""
    nsec = len(elf.sections)
    table: dict[str, list] = {}
    for s in elf.symbols:
        sec = elf.sections[s.shndx].name if 0 < s.shndx < nsec else None
        if s.name and sec and s.type in (0, 1, 2) and s.size:
            table.setdefault(sec, []).append(s)
    for lst in table.values():
        lst.sort(key=lambda s: (s.value, -s.size))
    starts = {sec: [s.value for s in lst] for sec, lst in table.items()}
    targets = _reloc_targets(elf)
    data: dict[str, bytes] = {}
    syms = elf.symbols
    out: dict = {}
    body: dict = {}
    for r in elf.relocs():
        tsec = targets.get(r.rela_index)
        lst = table.get(tsec, ())
        own = None
        hi = bisect.bisect_right(starts.get(tsec, ()), r.offset)
        for s in reversed(lst[max(hi - OWNER_LOOKBACK, 0):hi]):   # the nearest start first; nesting is shallow
            if r.offset < s.value + s.size:
                own = s
                break
        if own is None or r.symbol >= len(syms):
            continue
        if tsec not in data:
            data[tsec] = elf.section_bytes(tsec) or b""
        key = (tsec, own.name)
        if key not in body:
            body[key] = data[tsec][own.value:own.value + own.size]
        word = None
        if tsec == ".text":
            raw = data[tsec][r.offset & ~3:(r.offset & ~3) + 4]
            word = int.from_bytes(raw, "big") if len(raw) == 4 else None
        out.setdefault(key, []).append((r.offset - own.value, r.type, syms[r.symbol], r.addend, word))
    for lst in out.values():
        lst.sort(key=lambda row: (row[0], row[1]))
    return out, body


def value_width(word: int | None, sizes: list[int]) -> tuple[int, str]:
    """`(bytes to read, how to show them)` for one relocated access. A D-form load/store reads its access width
    (`ppc.WIDTH`; an FP access shows as `f32`/`f64`, an integer one as `int`); anything else (`addi`, a table word,
    lmw/stmw) reads the datum: the smallest positive size of `sizes` (the two symbols' own sizes from the addend on;
    the larger one runs into its neighbour or its padding), else `VALUE_DEFAULT_WIDTH`, capped at `VALUE_MAX_WIDTH`
    (`bytes`). Pure."""
    from tools.lib import ppc  # noqa: PLC0415 - only the value pass decodes instructions
    op = None if word is None else word >> 26
    if op in ppc.WIDTH and op not in _MULTIPLE_OPS:
        return ppc.WIDTH[op], ("f32" if op in _FP_SINGLE_OPS else "f64" if op in _FP_DOUBLE_OPS else "int")
    size = min((n for n in sizes if n > 0), default=VALUE_DEFAULT_WIDTH)
    return min(size, VALUE_MAX_WIDTH), "bytes"


def _relocated_words(elf: Elf) -> dict[str, set[int]]:
    """`{section: offsets}` of every relocated field in the object's data sections: the bytes there are the
    relocation's business (zero in a compiled object, sometimes the linked address in a split one), not data."""
    out: dict[str, set[int]] = {}
    for r in elf.relocs():
        out.setdefault(r.section, set()).add(r.offset)
    return out


def _value_at(elf: Elf, sym, addend: int, width: int, dol, addresses: dict,
              relocated: dict[str, set[int]] | None = None) -> tuple[str, bytes | None, set[int]]:
    """What one relocation target holds: `("bytes", data, relocated)` from the object's own section (a defined
    symbol, or a section symbol at the addend) - `relocated` the datum's offsets that a relocation fills (each
    covers 4 bytes) - or from the DOL at the map's address plus the addend (an undefined name, nothing masked);
    `("code", ...)` for a target in executable code, `("bss", ...)` for uninitialised data (nothing to compare)
    and `("unresolved", ...)` for a name the map does not carry or a read past the section's end."""
    if sym.defined and 0 < sym.shndx < len(elf.sections):
        sec = elf.sections[sym.shndx]
        if sec.flags & SHF_EXECINSTR:
            return "code", None, set()
        if sec.is_nobits:
            return "bss", None, set()
        start = (0 if sym.type == STT_SECTION else sym.value) + addend
        data = sec.data[start:start + width]
        if start < 0 or len(data) != width:
            return "unresolved", None, set()
        hit = {off - start for off in (relocated or {}).get(sec.name, ()) if start - 3 <= off < start + width}
        return "bytes", data, hit
    address = addresses.get(sym.name) if sym.name else None
    if address is None or dol is None:
        return "unresolved", None, set()
    address += addend
    if dol.in_text(address):
        return "code", None, set()
    data = dol.bytes_at(address, width)
    return ("bytes", data, set()) if data is not None else ("bss", None, set())


def _masked(data: bytes, relocated: set[int]) -> bytes:
    """`data` with the 4 bytes at each relocated offset zeroed."""
    out = bytearray(data)
    for off in relocated:
        for i in range(max(off, 0), min(off + 4, len(out))):
            out[i] = 0
    return bytes(out)


def format_value(data: bytes, kind: str) -> str:
    """One datum as text: a single/double as its number and hex, a printable NUL-terminated string quoted, else
    hex. Pure."""
    import struct  # noqa: PLC0415
    if kind == "f32" and len(data) == 4:
        return "%r (0x%s)" % (struct.unpack(">f", data)[0], data.hex())
    if kind == "f64" and len(data) == 8:
        return "%r (0x%s)" % (struct.unpack(">d", data)[0], data.hex())
    text, _nul, rest = data.partition(b"\0")
    if kind == "bytes" and len(text) >= 2 and not any(rest) and all(32 <= c < 127 for c in text):
        return '"%s"' % text.decode("ascii")
    if len(data) > VALUE_SHOWN_BYTES:
        return "0x%s... (%d bytes)" % (data[:VALUE_SHOWN_BYTES].hex(), len(data))
    return "0x" + data.hex()


def _datum_size(sym, addend: int) -> int:
    """The bytes of `sym`'s own datum from `addend` on (0 for a section symbol or an unsized one)."""
    return 0 if sym.type == STT_SECTION or not sym.size else max(sym.size - addend, 0)


def value_diffs(target, ours, dol=None, addresses: dict | None = None) -> dict:
    """The relocation pairs whose targets differ by NAME, judged by the bytes each target holds.

    objdiff and `rawsame` score `lfs f1, @123@sda21(r2)` against `lfs f1, lbl_80796EE0@sda21(r2)` as equal whatever
    the two pool words hold. Per owner (function or data object) both objects define - a `.text` stub ours has not
    written (`STUB_MAX_BYTES`) excepted - a relocation of ours pairs with the target's at the SAME offset in the
    owner, of the same type and patching the same instruction word (relocated fields are zero in both objects; an
    aligner on the type sequence, or a pair across two different instructions, reads a diverged body as a false
    value mismatch - counted as `other-instruction`); every pair of a `VALUE_RELOCS` kind whose two symbol names differ is read
    on both sides (`value_width` bytes at the addend: the object's own section for a defined
    target, the DOL `dol` at `addresses[name]` for an undefined one) and the bytes compared - a word inside the datum
    that a relocation fills on either side is masked on both (a pointer is the relocation's question, not a value),
    and a datum made only of such words is skipped as `relocated`; `extab`/`extabindex` owners are not read.

    Returns `{pairs, same, differ: [{owner, section, offset, type, ours, ours_addend, target, target_addend,
    ours_value, target_value, width, kind, first_difference, owner_equal}], skipped: {code|bss|unresolved|relocated|
    other-instruction: n}}`: `pairs` counts the name-differing pairs read on both sides, `offset` is the offset in
    the owner, `kind` is `value_width`'s (`bytes`: an address-taken datum), `first_difference` the first differing
    byte of the datum, and `owner_equal` says the owner's two bodies are byte-identical (relocated fields are zero
    in both objects) - the case no score can see."""
    addresses = addresses or {}
    o_elf, t_elf = load(ours), load(target)
    (a, a_body), (b, b_body) = _owned_relocs(o_elf), _owned_relocs(t_elf)
    o_rel, t_rel = _relocated_words(o_elf), _relocated_words(t_elf)
    o_funcs = _functions(o_elf)
    out = {"pairs": 0, "same": 0, "differ": [], "skipped": Counter()}
    for key in sorted(set(a) & set(b), key=lambda k: (k[0] or "", k[1] or "")):
        sec, owner = key
        if sec == ".text" and owner in o_funcs and o_funcs[owner][1] <= STUB_MAX_BYTES:
            continue
        if sec.startswith(BOOKKEEPING_SECTIONS):         # unwind records: flipcheck compares those bytes directly
            continue
        theirs = {(row[0], row[1]): row for row in b[key]}
        for off, typ, osym, oadd, oword in a[key]:
            pair = theirs.get((off, typ))
            if pair is None:
                continue
            _toff, _ttyp, tsym, tadd, tword = pair
            oname, tname = osym.name or "<%s>" % osym.section, tsym.name or "<%s>" % tsym.section
            if typ not in VALUE_RELOCS or oname == tname:
                continue
            if oword != tword:                            # another instruction: the body diverged, not the datum
                out["skipped"]["other-instruction"] += 1
                continue
            width, kind = value_width(tword if tword is not None else oword,
                                      [_datum_size(tsym, tadd), _datum_size(osym, oadd)])
            ok, ov, om = _value_at(o_elf, osym, oadd, width, dol, addresses, o_rel)
            tk, tv, tm = _value_at(t_elf, tsym, tadd, width, dol, addresses, t_rel)
            if ok != "bytes" or tk != "bytes":
                out["skipped"][ok if ok != "bytes" else tk] += 1
                continue
            if om or tm:                                  # a relocated word is a pointer, not this datum's value
                ov, tv = _masked(ov, om | tm), _masked(tv, om | tm)
                if not any(ov) and not any(tv) and len(om | tm) * 4 >= width:
                    out["skipped"]["relocated"] += 1
                    continue
            out["pairs"] += 1
            if ov == tv:
                out["same"] += 1
                continue
            out["differ"].append({"owner": owner, "section": sec, "offset": off, "type": reloc_name(typ),
                                  "ours": oname, "ours_addend": oadd, "target": tname, "target_addend": tadd,
                                  "ours_value": format_value(ov, kind), "target_value": format_value(tv, kind),
                                  "width": width, "kind": kind, "first_difference": first_difference(ov, tv),
                                  "owner_equal": a_body[key] == b_body[key]})
    out["skipped"] = dict(out["skipped"])
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


# --- dead-stripping ---------------------------------------------------------------------------------------------

#: Sections whose symbols mwld may dead-strip one by one; every other section is kept whole.
STRIPPABLE_SECTIONS = (".text", ".data", ".rodata", ".sdata", ".sdata2", ".bss", ".sbss", ".sbss2")
#: Relocation targets whose references keep nothing alive: an extabindex entry points at the function it covers.
NON_ROOTING_SECTIONS = ("extabindex",)
#: `.comment`'s force-active (export) flag: the linker keeps such a symbol whatever references it.
FORCE_ACTIVE_FLAG = 0x08


def dead_symbols(obj, roots: set[str], removed: set[int] | None = None, kept: set[int] | None = None) -> list | None:
    """The defined symbols of one object that mwld's dead-strip removes, judged conservatively; None when unreadable.

    Live: a global/weak symbol named in `roots` (what the rest of the link references, FORCEACTIVE, the entry), a
    symbol `.comment` marks force-active, every byte of a section outside `STRIPPABLE_SECTIONS`, and - transitively -
    every symbol a live byte range relocates (an `extabindex` entry roots nothing; bytes of a strippable section no
    candidate covers are a live range). A relocation naming a section
    symbol, a 0-size label or a symbol that is not itself a strip candidate keeps its whole section, because the
    linker cannot tell which bytes behind it are used. Only a symbol of size > 0 in a strippable section can be dead.
    `removed` are the symbol indices the link drops before the strip (a weak copy another input defines first): a
    reference to one resolves to that other definition, so it keeps nothing of this object alive, and its own bytes
    root nothing; they are not in the result. `kept` are symbol indices the caller holds live whatever references
    them (a caller that counts a symbol's bytes keeps what it references too). Every doubt resolves to live: a wrong
    "live" refuses a flip, a wrong "dead" would pass a broken one."""
    removed = removed or set()
    elf = be32(obj)
    if elf is None or elf.symtab is None:
        return None
    syms = elf.symbols
    nsec = len(elf.sections)
    strippable = {s.index for s in elf.sections if s.name in STRIPPABLE_SECTIONS}
    cand = {s.index for s in syms
            if s.name and s.size > 0 and 0 < s.shndx < nsec and s.shndx in strippable and s.type != STT_SECTION}
    by_section: dict[int, list[int]] = {}
    for i in cand:
        by_section.setdefault(syms[i].shndx, []).append(i)
    relocs: dict[int, list[tuple[int, int]]] = {}
    all_relocs = elf.relocs()
    for rela, target in elf.rela_sections():
        if target.startswith(NON_ROOTING_SECTIONS) or not 0 < rela.info < nsec:
            continue
        relocs.setdefault(rela.info, []).extend((r.offset, r.symbol) for r in all_relocs if r.rela_index == rela.index)
    live: set[int] = set()
    whole: set[int] = set()
    work: list[tuple[int, int, int]] = []

    def keep_section(shndx: int) -> None:
        if shndx in whole:
            return
        whole.add(shndx)
        work.append((shndx, 0, elf.sections[shndx].size))
        live.update(by_section.get(shndx, ()))

    def keep(i: int) -> None:
        if i in live or i in removed or not 0 <= i < len(syms):
            return
        s = syms[i]
        if i in cand:
            live.add(i)
            work.append((s.shndx, s.value, s.value + s.size))
        elif 0 < s.shndx < nsec:
            keep_section(s.shndx)

    for s in elf.sections:
        if s.index and s.index not in strippable and s.type != SHT_RELA:
            work.append((s.index, 0, s.size))
    # bytes of a strippable section that no candidate covers belong to nothing the strip can take: they stay, and so
    # does whatever they relocate
    for shndx in strippable:
        spans = sorted((syms[i].value, syms[i].value + syms[i].size) for i in by_section.get(shndx, ()))
        starts = [lo for lo, _hi in spans]
        reach = list(itertools.accumulate((hi for _lo, hi in spans), max))
        for off, sym in relocs.get(shndx, ()):
            at = bisect.bisect_right(starts, off)
            if not at or reach[at - 1] <= off:
                keep(sym)
    for s in syms:
        if s.name in roots and s.bind in (STB_GLOBAL, STB_WEAK) and s.defined:
            keep(s.index)
    for i in kept or ():
        keep(i)
    comment = elf.comment
    for i, entry in enumerate(comment.entries if comment is not None else ()):
        if entry.active_flags & FORCE_ACTIVE_FLAG and i < len(syms) and syms[i].defined:
            keep(i)
    while work:
        shndx, lo, hi = work.pop()
        for off, sym in relocs.get(shndx, ()):
            if lo <= off < hi:
                keep(sym)
    return sorted((syms[i] for i in cand - live - removed), key=lambda s: (s.shndx, s.value))


def _align_up(value: int, align: int) -> int:
    return (value + align - 1) // align * align


def strip_section(obj, section: str, dead: list) -> bytes | None:
    """Section `section` of `obj` as the linker lays it out once the `dead` symbols are stripped, or None when that
    cannot be modelled (`strip_moves` decides the layout)."""
    got = strip_moves(obj, section, dead)
    return None if got is None else apply_moves(*got)


def apply_moves(data: bytes, moves: list[tuple[int, int, int]]) -> bytes:
    """`data` laid out by `strip_moves`' `[(start, end, new start)]`, the gaps zero-filled."""
    out = bytearray()
    for lo, hi, new_lo in moves:
        out += bytes(new_lo - len(out)) + data[lo:hi]
    return bytes(out)


def strip_moves(obj, section: str, dead: list) -> tuple[bytes, list[tuple[int, int, int]]] | None:
    """`(the section's bytes, [(start, end, new start)])`: where each kept symbol span of section `section` of `obj`
    lands once the `dead` symbols are stripped, ascending, or None when that cannot be modelled.

    Each kept symbol is placed at its alignment (its `.comment` entry's, else the section's) after the previous kept
    one, the gap zero-filled. The model is accepted only where the object itself already obeys it - the symbols of
    size > 0 do not overlap, the first starts at 0, each next one starts exactly at the previous end rounded up to its
    own alignment, every gap byte is zero, and the last one ends at the section's end, or short of it by zero fill
    that rounds it up to the section's alignment (MWCC pads `extab` after an 18-byte record) - so the kept layout
    differs from the object's only by the stripped symbols. Such a fill is kept as a last, empty move: the kept
    bytes are rounded up to the section's alignment the same way."""
    elf = be32(obj)
    sec = elf.section(section) if elf is not None else None
    if sec is None or sec.is_nobits:
        return None
    data = sec.data
    comment = elf.comment
    entries = comment.entries if comment is not None else ()
    gone = {s.index for s in dead}
    spans: dict[tuple[int, int], dict] = {}
    for s in elf.symbols:
        if s.shndx == sec.index and s.size > 0 and s.type != STT_SECTION:
            align = entries[s.index].align if s.index < len(entries) and entries[s.index].align else sec.align
            span = spans.setdefault((s.value, s.value + s.size), {"align": 1, "kept": False})
            span["align"] = max(span["align"], align, 1)
            span["kept"] = span["kept"] or s.index not in gone
    pos = 0
    for (lo, hi), span in sorted(spans.items()):
        if lo != (_align_up(pos, span["align"]) if pos else 0) or any(data[pos:lo]):
            return None
        pos = hi
    if pos != len(data) and (_align_up(pos, max(sec.align, 1)) != len(data) or any(data[pos:])):
        return None
    moves, end = [], 0
    for (lo, hi), span in sorted(spans.items()):
        if span["kept"]:
            new_lo = _align_up(end, span["align"])
            moves.append((lo, hi, new_lo))
            end = new_lo + hi - lo
    if pos != len(data) and end:
        moves.append((len(data), len(data), _align_up(end, max(sec.align, 1))))  # the object's own trailing fill
    return data, moves


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

