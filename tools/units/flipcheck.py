"""Can our object fill every section the unit's splits.txt claims? Sizes, bytes, permutation, undefined refs.
Spec: docs/tools/spec/flipcheck.md. CLI: flipcheck.py [<unit> ...] [--verbose] [--json] [--root TREE] | --selftest."""
from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import re
import sys

from tools.lib import findings as _findings
from tools.lib import objcompare
from tools.lib import project as _project  # the splits / map readers
from tools.lib.lanes.naming import norm_unit  # the one unit spelling rule
from tools.units import dataseams  # `.data` emission-order seams: order-only / multi-TU diagnosis
from tools.units import poolseams  # literal pools as TU evidence: a pool difference a fold explains

MAIN = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPLITS = os.path.join(MAIN, "config", "RMHE08", "splits.txt")
SRC = os.path.join(MAIN, "build", "RMHE08", "src")
# the link's input list - the only objects a link-wide symbol/reference check may read (see `link_inputs`)
NINJA = os.path.join(MAIN, "build.ninja")
# the symbol map: a name with a row here is a definition the project has, whatever object emits it
SYMBOLS = os.path.join(MAIN, "config", "RMHE08", "symbols.txt")


def set_root(root: str) -> None:
    """Read another tree: every path this module reads (`MAIN`, `SPLITS`, `SRC`, `NINJA`, `SYMBOLS`, `LDSCRIPT`) is
    re-derived from `root`. The default is the tree this file lives in."""
    global MAIN, SPLITS, SRC, NINJA, SYMBOLS, LDSCRIPT
    MAIN = os.path.abspath(root)
    SPLITS = os.path.join(MAIN, "config", "RMHE08", "splits.txt")
    SRC = os.path.join(MAIN, "build", "RMHE08", "src")
    NINJA = os.path.join(MAIN, "build.ninja")
    SYMBOLS = os.path.join(MAIN, "config", "RMHE08", "symbols.txt")
    LDSCRIPT = os.path.join(MAIN, "build", "RMHE08", "ldscript.lcf")

# Fragments the *compiler* generates as a side effect of the unit's code: the exception tables and the
# constructor/destructor reference words. A matched unit produces them, so they are part of its match and are
# checked byte-for-byte below like any other section - contributing one is not a reason to withhold a flip.
# (The target object is dtk's synthesised object, so its *in-object section order* is dtk's, not the original
# compiler's, and comparing the two orders says nothing.)
COMPILER_GENERATED = ("extab", "extabindex", ".ctors", ".dtors")

# dtk's symbol map names a unit's exception-table fragments `@etb_<VA>` (extab) and `@eti_<VA>`
# (extabindex); MWCC emits the same fragments under anonymous ordinal names (`@905`) instead. When another
# *linked* object relocates the map name, only the target object `dol split` synthesises defines it, so a flip
# leaves it undefined and the link fails - the resfile-flip class (`.pi/notes/resfile-flip.md`).
MAP_FRAGMENT_PREFIXES = ("@etb_", "@eti_")


def sections(path: str) -> dict[str, tuple[int, int]]:
    """{section name: (size, align exponent)} for a compiled object (`objcompare.object_sizes`)."""
    return objcompare.object_sizes(path)


def emitted_sections(path: str) -> list[str]:
    """Every section name the object carries, ignored ones included, in section-header order."""
    return objcompare.section_names(path)


def missing_or_empty_object(unit: str, src_path: str, claim: dict[str, tuple[int, int]]) -> list[str]:
    """Why there is nothing to check: the object is absent, or it exists and emits nothing comparable.

    `sections()` returns `{}` for both, and the two must not read the same. A lane told "compile it first"
    for an object that exists - a bodyless unit whose object is `.comment` and nothing else
    (`NHTTP/NHTTP_os_RVL`, and `ninja -n` answers "no work to do") - rebuilds in a loop and stays stuck.
    The empty case says what it is and what the claim needs.
    """
    if not os.path.exists(src_path):
        return ["no compiled object (build/RMHE08/src/%s.o) - compile it first" % unit]
    claim_txt = ", ".join("%s 0x%X" % (n, size) for n, (size, _align) in sorted(claim.items())) or "no section"
    return ["the object (build/RMHE08/src/%s.o) exists but emits none of the sections this check compares "
            "(only %s): splits.txt claims %s, which a rebuild cannot supply (`ninja -n` reports no work to do)"
            % (unit, ", ".join(emitted_sections(src_path)) or "no section at all", claim_txt)]


def claims() -> dict[str, dict[str, tuple[int, int]]]:
    """{unit: {section name as the object spells it: (claimed size, align exponent)}} from splits.txt."""
    units: dict[str, dict[str, tuple[int, int]]] = {}
    for block in _project.Splits.read(SPLITS).blocks:
        claimed = units.setdefault(re.sub(r"\.(c|cpp|cp|cc)$", "", block.unit), {})
        for r in block.ranges:
            claimed[r.object_section] = (r.size, 2)    # rename:.ctors$10 -> .ctors$10; ranges, not alignment
    return units


_RANGES: dict = {}


def _range_index() -> tuple[dict, dict]:
    """`({(unit stem, object section): (start, end)}, {(section, start): unit stem})` from splits.txt, read once
    per `SPLITS` path."""
    if SPLITS not in _RANGES:
        own, starts = {}, {}
        for block in _project.Splits.read(SPLITS).blocks:
            stem = re.sub(r"\.(c|cpp|cp|cc)$", "", block.unit)
            for r in block.ranges:
                own[(stem, r.object_section)] = (r.start, r.end)
                starts[(r.section, r.start)] = (stem, r.object_section)
        _RANGES[SPLITS] = (own, starts)
    return _RANGES[SPLITS]


def pad_context(unit: str, section: str) -> tuple[int | None, list[int]]:
    """`(the claim's start, [alignments the next linked section may carry])` for a trailing-pad judgement: the
    registered range that starts where this claim ends, its target object's alignment and - when built - our
    object's. `(start, [])` when the next section is not a registered unit's (it is not judged)."""
    own, starts = _range_index()
    span = own.get((unit, section))
    if span is None:
        return None, []
    nxt = starts.get((section.split("$")[0], span[1]))
    if nxt is None:
        return span[0], []
    stem, objsec = nxt
    aligns = []
    for side in ("obj", "src"):
        got = sections(os.path.join(MAIN, "build", "RMHE08", side, stem + ".o")).get(objsec)
        if got is not None:
            aligns.append(1 << got[1])
    return span[0], aligns


def pad_note(unit: str, name: str, ours: tuple[int, int], size: int, obj_path: str) -> str | None:
    """`lib.objcompare.trailing_pad` for one section of this unit, with the target's bytes, symbols and alignment."""
    tgt = sections(obj_path).get(name)
    if tgt is None:
        return None
    align = 1 << max(ours[1], tgt[1])
    start, next_aligns = pad_context(unit, name)
    data = raw_section(obj_path, name)
    nobits = tgt is not None and data is not None and len(data) == 0 and size > 0
    return objcompare.trailing_pad(name, ours[0], size, align, None if nobits else data,
                                   section_symbols(obj_path, name), start, next_aligns)


def unit_name_for(path: str) -> str:
    """build/RMHE08/src/Network/NetworkWiiMediator.o -> Network/NetworkWiiMediator"""
    return os.path.relpath(path, SRC).replace("\\", "/")[:-2]


def raw_section(path: str, name: str) -> bytes | None:
    """The raw bytes of one section (None when the section is absent; empty for NOBITS)."""
    return objcompare.section_data(path, name)


differing_bytes = objcompare.differing_bytes      # a length difference counts its extra bytes
section_symbols = objcompare.section_symbols      # {symbol: (offset in the section, size)}


def mislaid_layout(mine: bytes, tgt: bytes, ours_path: str, obj_path: str, name: str):
    """(symbols compared, differing bytes) when two same-sized sections hold the same symbols permuted."""
    return objcompare.mislaid_layout(mine, tgt, section_symbols(ours_path, name), section_symbols(obj_path, name))


def mislaid_order(mine: bytes, tgt: bytes, ours_path: str, obj_path: str, name: str):
    """(moved, compared, bytes the layout explains, bytes left over) for the weaker permutation class."""
    return objcompare.mislaid_order(mine, tgt, section_symbols(ours_path, name), section_symbols(obj_path, name))


def section_byte_problems(name: str, mine: bytes, tgt: bytes, ours_path: str,
                          obj_path: str) -> list[str]:
    """The byte-level refusal lines for one section: the first difference, the count, the permutation."""
    return objcompare.section_byte_reasons(name, mine, tgt, section_symbols(ours_path, name),
                                           section_symbols(obj_path, name))


def data_seam_problems(unit: str, name: str, ours_path: str, obj_path: str, seams: list[dict] | None = None,
                       rng: tuple[int, int] | None = None) -> list[str]:
    """The `.data` emission-order diagnosis for a differing `.data`: `order-only` or the multi-TU line.

    A unit's `.data` that holds the target's symbols in another sequence reads as a plain byte/size mismatch
    everywhere else. `dataseams.seam_note` (docs/data-order-seams.md) names it - `order-only: the unit spans
    several TUs; seams: at 0x... / a boundary in [a, b)` - when the target's range contains a strong seam (a
    `V->S` gap, where the boundary position is uncertain, or a zigzag; never `V->tail`/`V->D`), and adds the
    multi-TU line when the section differs otherwise. Empty for another section and for a range with no seam.
    """
    if name != ".data":
        return []
    note = dataseams.seam_note(unit, ours_path, obj_path, name, seams, rng)
    return [note] if note else []


def pool_group_problems(unit: str, claim: dict, ours: dict, src_path: str, obj_path: str,
                        note: str | None = None) -> list[str]:
    """The pool-sharing explanation for a differing `.sdata2`/`.sdata`: our object holds a partial pool of a TU.

    MWCC emits one literal pool per TU and the linker does not merge pools (docs/pool-seams.md), so when the
    unit's pooled literals are also read by other registered units the target's pool is shared with them and ours
    can only be a part of it - the flip is blocked by the seam, not by the source.  `note` is `poolseams`' fold line
    (None = look it up; nothing when the unit is in no group).  One line per differing pool section.
    """
    out = []
    for name in (".sdata2", ".sdata"):
        if name not in claim:
            continue
        got = ours.get(name)
        mine, tgt = raw_section(src_path, name), raw_section(obj_path, name)
        if got is not None and got[0] == claim[name][0] and mine is not None and tgt is not None and mine == tgt:
            continue
        if note is None:
            note = poolseams.note_for_unit(MAIN, unit) or ""
        if note:
            out.append("%s: our object's pool is a partial pool of a TU that spans several registered units - %s; "
                       "fold the units (one TU, one pool) before expecting this section to match" % (name, note))
    return out


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
BOOKKEEPING_SECTIONS = objcompare.BOOKKEEPING_SECTIONS
# A symbol defined in one of these is fragment/metadata data, not trimmable code.
FRAGMENT_PREFIXES = ("extab", "extabindex", ".ctors", ".dtors")
LDSCRIPT = os.path.join(MAIN, "build", "RMHE08", "ldscript.lcf")
# mwldeppc defines the EABI small-data base symbols itself, and no object and no symbols.txt row carries
# them (`Runtime.PPCEABI.H/__start` references both); `__start` is its default entry root.
EABI_LINKER_SYMBOLS = objcompare.EABI_LINKER_SYMBOLS
ENTRY_SYMBOLS = objcompare.ENTRY_SYMBOLS
_elf = objcompare.be32   # the ELF32 big-endian object at a path, or None


def comment_symbols(path: str) -> list[dict] | None:
    """The `.comment` symbol table as [{name, size, section, active_flags}], or None without a `.comment`.

    One 8-byte entry per ELF symbol, starting at 0x2C: `[align:4][visibility:1][active_flags:1][pad:2]`
    (docs/comment_section.md). The table follows ELF symbol-table order, so callers pair entries by name.
    """
    elf = _elf(path)
    com = elf.section(".comment") if elf is not None else None
    if com is None:
        return None
    if elf.section(".symtab") is None or elf.section(".strtab") is None or len(com.raw) < COMMENT_HEADER:
        return []
    syms = elf.symbols
    out = []
    for i in range((len(com.raw) - COMMENT_HEADER) // 8):
        sym = syms[i] if i < len(syms) else None
        out.append({"name": sym.name if sym else "",
                    "size": sym.size if sym else 0,
                    "section": elf.section_name(sym.shndx) if sym else "",
                    "active_flags": com.raw[COMMENT_HEADER + 8 * i + 5]})
    return out


def object_symbols(path: str) -> tuple[set[str], dict[str, tuple[str, int]]]:
    """(names referenced from non-bookkeeping sections, {defined name: (section, st_info)}) in one read."""
    facts = objcompare.reloc_facts(path) if os.path.exists(path) else None
    if facts is None:
        return set(), {}
    return facts["refs"], facts["defined"]


def code_references_in(path: str) -> set[str]:
    """The symbol names `path` references from a non-bookkeeping section (code, data or ctors)."""
    return object_symbols(path)[0]


def provides_global(defined: dict[str, tuple[str, int]], name: str) -> bool:
    """Whether `name` is defined in the object with a binding the linker resolves across objects."""
    return objcompare.provides_global(defined.get(name))


def forced_active(path: str | None = None) -> set[str]:
    """The FORCEACTIVE symbols in the linker script - roots the linker will not deadstrip."""
    path = LDSCRIPT if path is None else path
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


def link_inputs() -> list[str] | None:
    """The object inputs on `main.elf`'s link line in `NINJA`, MAIN-relative (None if unknown)."""
    return objcompare.link_inputs(NINJA)


def linker_assigned(path: str | None = None) -> set[str]:
    """Names the linker script defines itself (`_stack_addr = ...;`): the link supplies them, no object does."""
    return objcompare.linker_assigned(LDSCRIPT if path is None else path)


def map_symbols(path: str | None = None) -> set[str]:
    """The symbol names `config/RMHE08/symbols.txt` carries a row for (the map's definition of a name)."""
    path = SYMBOLS if path is None else path
    if not os.path.exists(path):
        return set()
    return _project.SymbolMap(path).names()


def link_reference_context() -> dict | None:
    """{refs by path, reference count per name, provider paths per name} over the link inputs only
    (`objcompare.link_index`, cached per input)."""
    paths = link_inputs()
    if not paths:
        return None
    index = objcompare.link_index(MAIN, paths)
    return {"refs": index["refs"], "ref_count": index["ref_count"],
            "providers": {name: set(rels) for name, rels in index["providers"].items()}}


def undefined_reference_problems(unit: str, target_rel: str, obj_path: str, src_path: str,
                                 link_ctx: dict, map_rows: set[str]) -> list[str]:
    """The names our object relocates that the link would answer `undefined:` for once the flip lands.

    The rule is `objcompare.undefined` (defined here, a map row, another provider than the target, the target's
    own unresolved reference, an already-referenced unprovided name, the linker's own names are all fine);
    `target_rel` keys `link_ctx`, `obj_path` is the target object, `map_rows` is `map_symbols()`.
    """
    ours = objcompare.reloc_facts(src_path) if os.path.exists(src_path) else None
    if ours is None:
        return []
    target_refs = link_ctx["refs"].get(target_rel, set())
    facts = objcompare.reloc_facts(obj_path) if os.path.exists(obj_path) else None
    target = {"relocs": (facts or {}).get("relocs", []), "defined": (facts or {}).get("defined", {}),
              "refs": target_refs}
    providers = {name: sorted(rels) for name, rels in link_ctx["providers"].items()}
    hits = sorted(name for name, _hint in objcompare.undefined(
        ours, target, map_set=map_rows, providers=providers, ref_count=link_ctx["ref_count"],
        target_rel=target_rel, linker_set=linker_assigned() | set(EABI_LINKER_SYMBOLS)))
    if not hits:
        return []
    line = ("%s: %d referenced symbol(s) are defined by nothing a flip can use - %s - our object does not "
            "define them, `symbols.txt` carries no row and no link input other than the target object "
            "provides them, so the link answers `undefined: '%s'`"
            % (unit, len(hits), ", ".join(hits), hits[0]))
    variants = []
    for name in hits:
        near = sorted(x for x in target_refs if x.startswith(name) and x != name)
        if len(near) == 1:
            variants.append("%s -> `%s`" % (name, near[0]))
    if variants:
        line += " (the target object references the differently-spelled %s - match the map's spelling)"                 % ", ".join(variants)
    return [line]


def external_map_symbol_notes(unit: str, target_rel: str, src_path: str, self_refs: set[str],
                              ref_count: dict[str, int], providers: dict[str, set[str]]) -> list[str]:
    """Map symbols another *linked* object references that only the target object and the map name.

    `dol split` names a unit's extab/extabindex fragments after the map (`@etb_80008000`), while MWCC
    emits the same bytes under anonymous ordinals (`@905`). If any *other* link input relocates the
    map name, flipping the unit used to leave the name undefined and the link failed with
    `undefined: '@eti_800222FC'` - the resfile-flip class (`.pi/notes/resfile-flip.md`). **This is no
    longer a refusal**: `tools/elf/objextab.py`, chained into every MWCC rule, renames the entries to
    these names and sets the binding global, so a current object defines them itself (the check then
    says nothing). It is kept as a *note* for the one case left: an object that predates that step,
    or whose unit has no `splits.txt` entry to take the addresses from. A rename alone is not enough
    (the binding must be global too), and no source or flag can spell the name.

    `target_rel`, `self_refs`, `ref_count` and `providers` are all keyed by the link's MAIN-relative paths.
    """
    defined = object_symbols(os.path.join(MAIN, target_rel))[1]
    ours = object_symbols(src_path)[1]
    hits = []
    for name in sorted(defined):
        if not name.startswith(MAP_FRAGMENT_PREFIXES):
            continue
        if ref_count.get(name, 0) - (1 if name in self_refs else 0) <= 0:
            continue                          # no linked object other than the target references it
        if provides_global(ours, name):
            continue                          # our object can provide it to the link
        if providers.get(name, set()) - {target_rel}:
            continue                          # another input already defines it; the link still resolves
        hits.append(name)
    if not hits:
        return []
    subject = "it" if len(hits) == 1 else "them"
    return ["map symbol(s) %s are defined only by the target object that `dol split` synthesises, and "
            "another *linked* object references %s, so our object would have to define %s: MWCC emits "
            "the same fragment(s) under anonymous ordinal names and no source or flag can spell the "
            "map's. The extab/extabindex rename step (`tools/elf/objextab.py`, chained after objalign "
            "in every MWCC rule) renames them and sets the binding global - so this object was built "
            "without that step (or its unit has no splits.txt entry); a flip now would fail with "
            "`undefined: '%s'`. Informational, not a refusal: the resfile-flip class "
            "(.pi/notes/resfile-flip.md) is fixed in the build."
            % (", ".join(hits), subject, subject, hits[0])]


def comment_trim_risks(unit: str, obj_path: str, src_path: str,
                       refs: set[str]) -> tuple[list[str], int, bool]:
    """Row 36: target-exported symbols our object leaves un-exported that nothing in the link references.

    Returns (problems, exported symbols examined, whether both `.comment` sections were readable): one problem line
    per symbol, in the target's `.comment` order (`row36_lines` folds them into the one summary line `check` prints).
    """
    risks, checked, compared = comment_trim_candidates(obj_path, src_path, refs)
    return [ROW36_SYMBOL_LINE % (unit, name, size) for name, size in risks], checked, compared


#: The per-symbol row-36 line (`--verbose`, and `comment_trim_risks`' problems).
ROW36_SYMBOL_LINE = ("%s: .comment marks %s (0x%X bytes) force-active (0x08) but our object does not, and "
                     "no code/data relocation in the link references it - the linker will deadstrip it and "
                     "shift every later section (row 36); mark it __declspec(export)")
#: How many names the row-36 summary line spells before `...`.
ROW36_FIRST = 6


def row36_lines(unit: str, risks: list[tuple[str, int]], verbose: bool = False) -> list[str]:
    """The refusal lines for a unit's row-36 trim risks: ONE summary line with the count and the first
    `ROW36_FIRST` names (a 22-function unit used to print 22 near-identical lines), then - with `verbose` - the
    per-symbol lines. Nothing when there is no risk."""
    if not risks:
        return []
    names = [name for name, _size in risks]
    shown = ", ".join(names[:ROW36_FIRST]) + (", ... (first %d)" % ROW36_FIRST if len(names) > ROW36_FIRST else "")
    out = ["row 36: %d function(s) force-active in retail .comment, not in ours: %s - unreferenced in the link, "
           "so the linker deadstrips them and shifts every later section; mark each __declspec(export)%s"
           % (len(names), shown, "" if verbose or len(names) <= ROW36_FIRST else " (--verbose lists all)")]
    if verbose:
        out += [ROW36_SYMBOL_LINE % (unit, name, size) for name, size in risks]
    return out


def comment_trim_candidates(obj_path: str, src_path: str,
                            refs: set[str]) -> tuple[list[tuple[str, int]], int, bool]:
    """Row 36's judgement: `([(name, size)], exported symbols examined, both `.comment`s readable)`."""
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
        problems.append((name, entry["size"]))
    return problems, checked, True


def check(unit: str, claim: dict[str, tuple[int, int]], refs: set[str] | None,
          link_ctx: dict | None = None, map_rows: set[str] | None = None,
          verbose: bool = False) -> tuple[list[str], list[str]]:
    src_path = os.path.join(SRC, unit + ".o")
    target_rel = os.path.normpath(os.path.join("build", "RMHE08", "obj", unit + ".o"))
    obj_path = os.path.join(MAIN, target_rel)
    ours = sections(src_path)
    if not ours:
        return missing_or_empty_object(unit, src_path, claim), []
    problems = []
    notes: list[str] = []
    padded: dict[str, int] = {}             # section -> our size, where the shortfall is only alignment fill
    for name, (size, _) in sorted(claim.items()):
        got = ours.get(name)
        if got is None:
            problems.append("splits.txt claims %s (0x%X) but the object emits no such section - "
                            "flipping drops %d bytes from the link and shifts everything after it"
                            % (name, size, size))
        elif got[0] != size:
            pad = pad_note(unit, name, got, size, obj_path) if got[0] < size else None
            if pad:
                padded[name] = got[0]
                notes.append(pad)
                continue
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
        mine = raw_section(src_path, name)
        tgt = raw_section(obj_path, name)
        if mine is None or tgt is None:
            continue
        if name in padded:
            tgt = tgt[:padded[name]]        # the tail is fill (judged above): only our own bytes are compared
        if mine != tgt:
            problems += section_byte_problems(name, mine, tgt, src_path, obj_path)
            problems += data_seam_problems(unit, name, src_path, obj_path)

    problems += pool_group_problems(unit, claim, ours, src_path, obj_path)

    # row 36: a byte-identical object can still break the DOL if the linker deadstrips a trailing function
    # our `.comment` does not force-active. Needs the whole link's reference set, so it is passed in.
    flag_problems, checked, compared = ([], 0, False)
    if refs is not None:
        risks, checked, compared = comment_trim_candidates(obj_path, src_path, refs)
        flag_problems = row36_lines(unit, risks, verbose)
    problems += flag_problems
    if compared and not flag_problems:
        notes.append(".comment: no un-exported symbol at deadstrip risk (row 36, %d target-exported symbol(s) "
                     "checked)" % checked)

    # a flip can only provide what our object defines: a map symbol another linked object references, that
    # only the target object defines, used to be a hard link break (the resfile-flip class).  The build's
    # extab/extabindex rename step (tools/elf/objextab.py) now provides those names, so this is a note.
    if link_ctx is not None:
        if target_rel in link_ctx["refs"]:
            notes += external_map_symbol_notes(
                unit, target_rel, src_path,
                link_ctx["refs"].get(target_rel, set()), link_ctx["ref_count"], link_ctx["providers"])
            # the general relocation half: only a *flip* can break a reference, so this runs where the
            # target object is still a link input (a matched unit is already carrying our object).
            problems += undefined_reference_problems(
                unit, target_rel, obj_path, src_path, link_ctx,
                map_symbols() if map_rows is None else map_rows)
        else:
            notes.append("already Object(Matching) - %s is not a link input, so there is no flip to check"
                         % target_rel.replace(os.sep, "/"))
    return problems, notes


def link_setup() -> tuple[dict | None, set[str] | None, set[str]]:
    """`(link_ctx, refs, map_rows)` - what `check` needs for the link-wide rows (row 36 and the undefined
    references); `(None, None, set())` when the link inputs are unknown (no `build.ninja`)."""
    link_ctx = link_reference_context()
    if link_ctx is None:
        return None, None, set()
    return link_ctx, set(link_ctx["ref_count"]) | forced_active() | set(ENTRY_SYMBOLS), map_symbols()


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Is a unit ready to flip to Object(Matching, ...)? Checks the object against the claim "
                    "its splits.txt entry makes (sections, sizes, alignment) and against the target object's "
                    "bytes. The DOL itself remains the only proof.")
    ap.add_argument("units", nargs="*")
    ap.add_argument("--selftest", action="store_true", help="run the self-test and exit")
    ap.add_argument("--verbose", action="store_true",
                    help="row 36: one line per force-active symbol after the summary line (default: the summary)")
    ap.add_argument("--json", action="store_true",
                    help="the lib.findings schema: one row per unit, plus `units` {unit: {ready, problems, notes}} "
                         "and `missing`")
    ap.add_argument("--root", default=None, help="the tree to read (default: the tree this file lives in)")
    args = ap.parse_args()
    if args.selftest:
        from tools.units import flipcheck_selftest
        return flipcheck_selftest.selftest()
    if args.root:
        set_root(args.root)

    link_ctx, refs, map_rows = link_setup()

    all_claims = claims()
    if args.units:
        wanted = {}
        for u in args.units:
            key = norm_unit(u.strip("/"))
            wanted[key] = all_claims.get(key)
        missing = [u for u, c in wanted.items() if c is None]
        for u in missing if not args.json else ():
            print("%s: no splits.txt entry" % u)
        wanted = {u: c for u, c in wanted.items() if c is not None}
    else:
        missing = []
        wanted = {}
        for root, _, files in os.walk(SRC):
            for f in sorted(files):
                if f.endswith(".o"):
                    u = unit_name_for(os.path.join(root, f))
                    if u in all_claims:
                        wanted[u] = all_claims[u]

    bad = 0
    results = {}
    for unit, claim in sorted(wanted.items()):
        problems, notes = check(unit, claim, refs, link_ctx, map_rows, verbose=args.verbose)
        results[unit] = {"ready": not problems, "problems": problems, "notes": notes}
        if args.json:
            bad += bool(problems)
            continue
        if problems:
            bad += 1
            print("NOT READY  %s" % unit)
            for p in problems:
                print("   - %s" % p)
        else:
            print("READY      %s (%d section(s) match the claim)" % (unit, len(claim)))
            for n in notes:
                print("   . %s" % n)
    if args.json:
        rows = [_findings.Row.check(u, r["ready"], "; ".join(r["problems"]), "; ".join(r["notes"]))
                for u, r in results.items()]
        rows += [_findings.Row.check(u, False, "no splits.txt entry") for u in missing]
        print(_findings.render_json("flipcheck", _findings.Verdict.of(rows), units=results, missing=missing))
        return 1 if bad or missing else 0
    print("\n%d of %d unit(s) ready" % (len(wanted) - bad, len(wanted)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
