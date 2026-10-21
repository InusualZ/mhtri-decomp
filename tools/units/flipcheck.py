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
from tools.lib.project import ownership as _ownership  # dtk's auto split objects
from tools.lib.binary.extab import unwind_entries  # the unwind records a dropped function takes with it
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


#: Units the caller flips together with the one being checked (the gate passes the batch): linked like `Matching`.
BATCH: set[str] = set()


def linked_units() -> set[str]:
    """Unit stems whose object the link takes from our build: `Object(Matching, ...)` in this tree's configure.py, and
    the units named on this run's command line (a batch flips them together)."""
    path = os.path.join(MAIN, "configure.py")
    got: set[str] = set()
    if os.path.isfile(path):
        try:
            got = {re.sub(r"\.(c|cpp|cp|cc)$", "", p) for p in _project.configure.load(path).matching_units()}
        except Exception:
            got = set()
    return got | set(BATCH)


def successor_of(unit: str, section: str) -> tuple[str, str] | None:
    """`(unit stem, object section)` of the registered range that starts where this claim ends, or None."""
    own, starts = _range_index()
    span = own.get((unit, section))
    return None if span is None else starts.get((section.split("$")[0], span[1]))


def padding_chain_break(stem: str, objsec: str) -> str | None:
    """Why a run of linked units does not hold its claimed addresses after a short successor, else None.

    A linked successor whose own built section is shorter than its claim leaves the next section to be placed by
    the linker, not by the claim: that next unit must be linked and 16-aligned too, and so on until a unit that
    fills its claim (or an unregistered/absent end). Returns the first unit that breaks the run."""
    own, starts = _range_index()
    linked = linked_units()
    seen = set()
    while (stem, objsec) not in seen:
        seen.add((stem, objsec))
        span = own.get((stem, objsec))
        built = sections(os.path.join(SRC, stem + ".o")).get(objsec)
        if span is None or built is None or built[0] >= span[1] - span[0]:
            return None
        nxt = starts.get((objsec.split("$")[0], span[1]))
        if nxt is None:
            return None
        standin = sections(os.path.join(MAIN, "build", "RMHE08", "obj", nxt[0] + ".o")).get(nxt[1])
        if nxt[0] not in linked and standin and standin[1] >= 4:
            return None  # an unflipped successor aligned to 16 by splits.txt places itself and fills its claim
        if nxt[0] not in linked:
            return "%s (short by 0x%X) is followed by unflipped %s: flip from the tail of the run" % (
                stem, span[1] - span[0] - built[0], nxt[0])
        stem, objsec = nxt
    return None


def linked_pad_note(unit: str, name: str, ours: tuple[int, int], size: int, obj_path: str) -> tuple[str | None, str | None]:
    """`(note, hint)`: the note when the shortfall is link padding before a linked successor (`linked_trailing_pad`),
    else the hint naming the unflipped successor whose flip would make it so (flip from the tail of each run)."""
    tgt = sections(obj_path).get(name)
    nxt = successor_of(unit, name)
    if tgt is None or nxt is None:
        return None, None
    stem, objsec = nxt
    start = pad_context(unit, name)[0]
    data = raw_section(obj_path, name)
    nobits = data is not None and len(data) == 0 and size > 0
    symbols = section_symbols(obj_path, name)
    body = None if nobits else data
    if stem in linked_units():
        built = sections(os.path.join(SRC, stem + ".o")).get(objsec)
        aligns = [1 << built[1]] if built else []
        note = objcompare.linked_trailing_pad(name, ours[0], size, body, symbols, start, aligns)
        if note:
            broken = padding_chain_break(stem, objsec)
            if broken:
                return None, ("the shortfall is link padding only while the run after it stays on its claimed addresses, "
                              "and %s" % broken)
        return note, None
    standin = sections(os.path.join(MAIN, "build", "RMHE08", "obj", stem + ".o")).get(objsec)
    if standin and start is not None:
        # an unflipped successor is its target object, which fills its own claim: it carries the alignment
        # splits.txt gives it (`align:N`), so the link pads this unit up to it without any chain
        note = objcompare.linked_trailing_pad(name, ours[0], size, body, symbols, start, [1 << standin[1]])
        if note:
            return note, None
    if start is not None and objcompare.linked_trailing_pad(name, ours[0], size, body, symbols, start, [16]):
        return None, ("the shortfall is link padding only once its successor %s is flipped (16-aligned); "
                      "flip from the tail of each run" % stem)
    return None, None


_AUTO: dict = {}


def auto_successor(section: str, address: int) -> tuple[str, str] | None:
    """`(object path, section)` of dtk's auto split object (`auto_<nn>_<ADDR>_<section>`) that starts at `address` in
    `section`, as this tree's split lists it (`build/RMHE08/config.json`; never a stale file in `obj/`), or None.
    Such an object is never flipped: the link places its target object at its own alignment."""
    if MAIN not in _AUTO:
        build = os.path.join(MAIN, "build", "RMHE08")
        _AUTO[MAIN] = _ownership.AutoObjects.load(os.path.join(build, "config.json"), None)
    base = section.split("$")[0]
    for start, _end, name in _AUTO[MAIN].ranges.get(base, ()):
        if start == address:
            return os.path.join(MAIN, "build", "RMHE08", "obj", name + ".o"), base
    return None


def pad_context(unit: str, section: str) -> tuple[int | None, list[int]]:
    """`(the claim's start, [alignments the next linked section may carry])` for a trailing-pad judgement: the
    registered range that starts where this claim ends, its target object's alignment and - when built - our
    object's; with no registered range there, dtk's auto object that starts there (`auto_successor`) and its
    alignment. `(start, [])` when neither is known (it is not judged)."""
    own, starts = _range_index()
    span = own.get((unit, section))
    if span is None:
        return None, []
    nxt = starts.get((section.split("$")[0], span[1]))
    if nxt is None:
        auto = auto_successor(section, span[1])
        if auto is None:
            return span[0], []
        got = sections(auto[0]).get(auto[1])
        return span[0], [1 << got[1]] if got is not None else []
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


def unreferenced_elsewhere(name: str, target_rel: str, link_ctx: dict) -> bool:
    """Whether no link input other than the target object references `name` (the reverse-reference count)."""
    self_refs = link_ctx["refs"].get(target_rel, set())
    return link_ctx["ref_count"].get(name, 0) - (1 if name in self_refs else 0) <= 0


def positioned_symbol_problems(claim: dict, ours: dict, src_path: str, obj_path: str,
                               leads: dict[str, int] | None = None, link_ctx: dict | None = None,
                               target_rel: str | None = None, notes: list[str] | None = None) -> list[str]:
    """Per positioned data section (`objcompare.POSITIONED`) the symbols whose offset, size or presence differs from
    the target's: equal sizes and bytes do not make the DOL hash hold when a spare global is dead-stripped or an
    object is 4 B instead of 8 B. `python tools/mwlink_debugger.py trace <unit>` names the cause. `leads` is `check`'s
    section -> leading fill: our symbols sit that much later in the link.

    A target name our object emits unnamed (`objcompare.unnamed_spellings`: equal bytes at the same offset, only local
    symbols of ours over them) is not missing when `link_ctx` shows no other link input references it - the map's
    name for an anonymous pool word; it is listed in `notes`. Without `link_ctx` nothing is excused. Where our code
    reaches those bytes is `relocation_place_problems`' check."""
    out = []
    leads = leads or {}
    for name in sorted(set(claim) & set(ours) & set(objcompare.POSITIONED)):
        lead = leads.get(name, 0)
        mine = {n: (off + lead, size) for n, (off, size) in section_symbols(src_path, name).items()}
        theirs = section_symbols(obj_path, name)
        excused: dict = {}
        if link_ctx is not None and target_rel is not None:
            data = raw_section(src_path, name)
            rows = [(n, off + lead, size, g) for n, off, size, g in objcompare.section_symbol_rows(src_path, name)]
            spelled = objcompare.unnamed_spellings(theirs, rows, bytes(lead) + data if data else data,
                                                   raw_section(obj_path, name))
            excused = {n: v for n, v in spelled.items() if unreferenced_elsewhere(n, target_rel, link_ctx)}
        gaps = objcompare.data_symbol_gaps(mine, theirs, excused=excused)
        if excused and notes is not None:
            notes.append("%s: %d target name(s) our object emits unnamed - same bytes at the same offset, only local "
                         "symbols of ours over them, no other link input references them: %s"
                         % (name, len(excused), ", ".join("%s +0x%X" % (n, excused[n][0])
                                                         for n in sorted(excused, key=lambda n: excused[n][0]))))
        if gaps:
            out.append("%s: symbol layout differs from the target (the link moves later data; `python "
                       "tools/mwlink_debugger.py trace <unit>` names the cause): %s" % (name, "; ".join(gaps)))
    return out


_MAP_ADDRESSES: dict = {}


def map_addresses() -> dict[str, int]:
    """`{name: address}` for every name `symbols.txt` defines exactly once (a duplicated name resolves nothing),
    read once per `SYMBOLS` path."""
    if SYMBOLS not in _MAP_ADDRESSES:
        rows: dict[str, list[int]] = {}
        if os.path.exists(SYMBOLS):
            for row in _project.SymbolMap(SYMBOLS).rows():
                rows.setdefault(row.name, []).append(row.address)
        _MAP_ADDRESSES[SYMBOLS] = {n: a[0] for n, a in rows.items() if len(a) == 1}
    return _MAP_ADDRESSES[SYMBOLS]


def _places_text(side: list, ours: list, target: list) -> str:
    """One side's `[(type, place)]` at an offset as text; each relocation type is named when the two sides' differ."""
    typed = [t for t, _p in ours] != [t for t, _p in target]
    return "/".join(objcompare.format_place(p) + (" (%s)" % objcompare.reloc_name(t) if typed else "")
                    for t, p in side)


#: How many differing relocations one section's refusal line spells before `...`.
RELOC_PLACE_FIRST = 4


def relocation_place_problems(unit: str, claim: dict, src_path: str, obj_path: str,
                              leads: dict[str, int] | None = None, moves: dict | None = None) -> list[str]:
    """One line per claimed section where a relocation at the same offset resolves to a different place (or type) in
    our object than in the target (`lib.objcompare.reloc_places`): the masked bytes can be equal while an accessor
    points at the other of two identical string copies (`g3d/g3d_resvtx`'s four `"ref"` copies, `g3d/g3d_scn`'s inline
    accessors), and the link then writes another address into the DOL. Both objects are placed at the claim
    (`splits.txt`'s starts), a named global through the map; `leads` shifts our sections and `moves` (the dead-strip
    model's `strip_moves` layouts) maps our offsets where the link strips symbols."""
    own, _starts = _range_index()
    bases = {sec: own[(unit, sec)][0] for sec in claim if (unit, sec) in own}
    addresses = map_addresses()
    mine = objcompare.reloc_places(src_path, bases, leads, addresses, moves)
    theirs = objcompare.reloc_places(obj_path, bases, None, addresses)
    if mine is None or theirs is None:
        return []
    out = []
    for name in sorted(set(claim) & set(mine) & set(theirs)):
        diffs = objcompare.reloc_place_diffs(mine[name], theirs[name])
        if not diffs:
            continue
        shown = ["+0x%X ours %s, target %s" % (off, _places_text(o, o, t), _places_text(t, o, t))
                 for off, o, t in diffs[:RELOC_PLACE_FIRST]]
        more = " (... %d more)" % (len(diffs) - RELOC_PLACE_FIRST) if len(diffs) > RELOC_PLACE_FIRST else ""
        out.append("%s: %d relocation(s) resolve to a different place or type than the target's at the same offset, "
                   "so the link writes other addresses even where the bytes are equal - %s%s"
                   % (name, len(diffs), "; ".join(shown), more))
    return out


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
    several TUs; seams: at 0x... / a boundary in [a, b)` - when a strong seam crosses the target's range
    (`dataseams.crossing`: a zigzag inside it, or a `V->S` gap whose later vtable is inside it too - a gap running on
    to the range's end is its own inline tail; never `V->tail`/`V->D`), and adds the
    multi-TU line when the section differs otherwise. Empty for another section and for a range with no seam.
    """
    if name != ".data":
        return []
    note = dataseams.seam_note(unit, ours_path, obj_path, name, seams, rng)
    return [note] if note else []


def pool_group_problems(unit: str, claim: dict, ours: dict, src_path: str, obj_path: str,
                        note: str | None = None, padded: dict[str, int] | None = None,
                        leads: dict[str, int] | None = None) -> list[str]:
    """The pool-sharing explanation for a differing `.sdata2`/`.sdata`: our object holds a partial pool of a TU.

    MWCC emits one literal pool per TU and the linker does not merge pools (docs/pool-seams.md), so when the
    unit's pooled literals are also read by other registered units the target's pool is shared with them and ours
    can only be a part of it - the flip is blocked by the seam, not by the source.  `note` is `poolseams`' fold line
    (None = look it up; nothing when the unit is in no group).  `padded` is `check`'s section -> our size where the
    shortfall was judged alignment fill: such a section matches when our bytes equal the target's prefix.  `leads`
    is the section -> leading fill `check` judged: our bytes are compared after that many zero bytes.  One line
    per differing pool section.
    """
    out = []
    padded = padded or {}
    leads = leads or {}
    for name in (".sdata2", ".sdata"):
        if name not in claim:
            continue
        got = ours.get(name)
        mine, tgt = raw_section(src_path, name), raw_section(obj_path, name)
        if mine and leads.get(name):
            mine = bytes(leads[name]) + mine
        if mine is not None and tgt is not None:
            if got is not None and got[0] == claim[name][0] and mine == tgt:
                continue
            if name in padded and mine == tgt[:padded[name]]:
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
            "providers": {name: set(rels) for name, rels in index["providers"].items()},
            "inputs": list(paths)}


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


def reverse_reference_problems(unit: str, target_rel: str, obj_path: str, src_path: str,
                               link_ctx: dict) -> list[str]:
    """The reverse-reference check (docs/pipeline.md 15.3): global names the target object defines, another link
    input relocates, and nothing a flip leaves in the link defines - our object does not provide them (global or
    weak) and no other input does - so the link answers `undefined: '<name>'` (gki_buffer's `.bss` label
    `lbl_8070DF3E`, read by `mh3_pad.o`). The `@etb_`/`@eti_` fragments stay `external_map_symbol_notes`' note."""
    target = object_symbols(obj_path)[1]
    ours = object_symbols(src_path)[1]
    self_refs = link_ctx["refs"].get(target_rel, set())
    hits = []
    for name in sorted(target):
        if not provides_global(target, name) or name.startswith(MAP_FRAGMENT_PREFIXES):
            continue
        if link_ctx["ref_count"].get(name, 0) - (1 if name in self_refs else 0) <= 0:
            continue                          # no other linked object references it
        if provides_global(ours, name) or link_ctx["providers"].get(name, set()) - {target_rel}:
            continue                          # our object, or another input, still defines it
        hits.append(name)
    if not hits:
        return []
    users = sorted({rel.replace(os.sep, "/").rsplit("/", 1)[-1] for rel, names in link_ctx["refs"].items()
                    if rel != target_rel and names & set(hits)})
    return ["%s: %d symbol(s) the target object defines and another link input references are not defined by our "
            "object - %s (referenced from %s) - so a flip fails with `undefined: '%s'`; define them (same name, "
            "global) or keep the unit unflipped (reverse-reference check, docs/pipeline.md 15.3)"
            % (unit, len(hits), ", ".join(hits), ", ".join(users[:6]) + (", ..." if len(users) > 6 else ""),
               hits[0])]


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


def anchored_sections(src_path: str) -> set[str]:
    """The sections our object's own code/data reaches through MWCC's section anchor (`...data.0`: a local, 0-size
    symbol at offset 0 that a relocation names with an addend; the code then indexes past it). The linker cannot
    tell which bytes behind an anchor are used, so it keeps the whole section: no symbol in it can be deadstripped."""
    elf = _elf(src_path)
    if elf is None:
        return set()
    anchors = {s.name: s.section for s in elf.symbols
               if s.name and s.name.startswith("...") and s.size == 0 and s.value == 0 and s.section}
    if not anchors:
        return set()
    rows, _ = objcompare.reloc_rows(src_path)
    out = set()
    for section, relocs in (rows or {}).items():
        if section.startswith(BOOKKEEPING_SECTIONS):
            continue
        out.update(anchors[sym] for _, sym, _, _ in relocs if sym in anchors)
    return out


def comment_trim_candidates(obj_path: str, src_path: str,
                            refs: set[str]) -> tuple[list[tuple[str, int]], int, bool]:
    """Row 36's judgement: `([(name, size)], exported symbols examined, both `.comment`s readable)`."""
    target = comment_symbols(obj_path)
    ours = comment_symbols(src_path)
    if target is None or ours is None:
        return [], 0, False
    anchored = anchored_sections(src_path)
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
        if entry["section"] in anchored:
            continue                     # our own code reaches its section through the anchor: kept whole
        problems.append((name, entry["size"]))
    return problems, checked, True


#: The section whose dead-stripped functions `deadstrip_model` takes out before the comparison.
STRIPPED_CODE = ".text"


def deadstrip_roots(unit: str, target_rel: str, link_ctx: dict) -> set[str]:
    """What keeps a symbol of `unit`'s object alive once it is flipped: every name another link input references,
    what the batch's own objects reference, the linker script's FORCEACTIVE block and the entry symbol. The unit's own
    objects are never "another input", so the answer is the same before the flip (the target object is the input) and
    after it (our object is): their references are what `dead_symbols` follows from the live symbols."""
    own = {target_rel, os.path.normpath(os.path.join("build", "RMHE08", "src", unit + ".o"))}
    roots: set[str] = set()
    for rel, names in link_ctx["refs"].items():
        if rel not in own:
            roots |= names
    for other in BATCH - {unit}:
        roots |= code_references_in(os.path.join(SRC, other + ".o"))
    return roots | forced_active() | set(ENTRY_SYMBOLS)


def _object_defines(path: str, name: str) -> bool:
    """Whether the object at `path` defines `name` with a binding the link resolves across objects (global or weak)."""
    facts = objcompare.reloc_facts(path) if os.path.exists(path) else None
    return bool(facts) and provides_global(facts["defined"], name)


def weak_duplicate_symbols(unit: str, src_path: str, obj_path: str, target_rel: str,
                           link_ctx: dict | None) -> tuple[list, list[tuple[str, str]]]:
    """`([our symbols the link drops], [(name, note)])`: weak definitions in our object that an EARLIER link input
    defines too.

    MWCC emits an inline function or an implicit vtable as a weak copy in every TU that uses it (`nw4r/ut_ResFontBase`
    carries `ut::Font`'s dtor and vtable, `nw4r/ut_CharWriter` `ut::Color`'s dtor); mwld keeps the first definition
    in link order and drops the later copies' bytes, which is why retail holds one (in `nw4r/ut_Font`, input 605, and
    `font/flfnt`, input 24 - both before the units that repeat them). A copy is dropped when all of these hold: it is
    weak, of size > 0, in a strippable section; the target object does not define the name (retail kept this unit's
    own copy otherwise); and a link input placed BEFORE ours defines it, global or weak - a unit of the batch counted
    as its built object at its link position. A copy that only a LATER input defines is kept (first wins: the link
    keeps ours) and so is one nobody else defines (`homebutton/hkb_window`'s `ut::Color` ctor); both stay counted like
    any other bytes, with a note. `([], [])` without the link order (`link_ctx["inputs"]`)."""
    inputs = (link_ctx or {}).get("inputs")
    elf = _elf(src_path)
    if not inputs or elf is None:
        return [], []
    src_rel = os.path.normpath(os.path.join("build", "RMHE08", "src", unit + ".o"))
    position = {rel: i for i, rel in enumerate(inputs)}
    own = position.get(target_rel, position.get(src_rel))
    if own is None:
        return [], []
    target_names = set(object_symbols(obj_path)[1])
    # the batch's other units are linked from their built objects, at their target objects' link positions
    batch = {os.path.normpath(os.path.join("build", "RMHE08", "obj", u + ".o")): u for u in BATCH - {unit}}
    dropped, notes = [], []
    for s in elf.symbols:
        if (s.bind != objcompare.STB_WEAK or not s.defined or s.size == 0 or s.name in target_names
                or s.section not in objcompare.STRIPPABLE_SECTIONS):
            continue
        earlier, later = [], []
        for rel in sorted((set(link_ctx["providers"].get(s.name, ())) | set(batch)) - {target_rel, src_rel}):
            if rel not in position:
                continue
            path = os.path.join(SRC, batch[rel] + ".o") if rel in batch else os.path.join(MAIN, rel)
            if rel in batch and not _object_defines(path, s.name):
                continue
            (earlier if position[rel] < own else later).append(rel)
        if earlier:
            dropped.append(s)
        elif later:
            notes.append((s.name, "%s: weak %s is defined again only by %s, which the link places after this "
                          "unit - the first definition wins, so the link keeps our copy and it is counted"
                         % (s.section, s.name, ", ".join(r.replace(os.sep, "/") for r in later))))
        else:
            notes.append((s.name, "%s: weak %s is defined by no other link input, so the link keeps our copy and it "
                          "is counted" % (s.section, s.name)))
    return dropped, notes


def dropped_unwind_symbols(src_path: str, functions: set[str]) -> list:
    """The `extab`/`extabindex` symbols of our object that cover the dropped weak `.text` copies `functions`
    (`lib.binary.extab.unwind_entries`): mwld drops a function's unwind entries with it, so a weak copy an earlier
    input defines takes its `extab` record and its 12-byte `extabindex` entry out of the link too - the same rule
    `tools/elf/objextab.py` names the remaining entries by."""
    elf = _elf(src_path) if functions else None
    if elf is None:
        return []
    entries = unwind_entries(elf, functions)
    return [s for s in elf.symbols if s.size > 0 and s.type != objcompare.STT_SECTION
            and s.value in entries.get(s.section, {})]


def deadstrip_model(unit: str, claim: dict, ours: dict, src_path: str, obj_path: str, target_rel: str,
                    link_ctx: dict | None, map_rows: set[str] | None,
                    moves: dict | None = None) -> tuple[dict[str, bytes], list[str], list[str]]:
    """`({section: our bytes as the link keeps them}, problems, notes)` for the symbols the link removes from our
    object: functions it dead-strips and weak copies it drops for an earlier definition (`weak_duplicate_symbols`).
    `moves`, when given, receives each rewritten section's `lib.objcompare.strip_moves` layout.

    A unit may define a function the retail linker stripped (a GUESS body written so its strings land in the pool):
    our object carries it, the target and `symbols.txt` do not, and mwld strips it again at link. Such a function is
    excluded when `lib.objcompare.dead_symbols` finds it dead (nothing another link input, the batch, FORCEACTIVE or a
    `.comment` export flag keeps, directly or through a live symbol's relocations), no target symbol and no map row
    carries its name, and `strip_section` can lay out the rest (the object already places each symbol at its
    alignment after the previous one, zero-filled). The remaining bytes then have to match the target byte for
    byte like any other. A dead WEAK data symbol with no target symbol and no map row (a base class's implicit
    vtable only its stripped weak dtor read - `homebutton/hkb_pane`) is one retail stripped too and leaves the layout
    the same way; any other data symbol the strip takes with it (a string or an object the target carries, only a
    stripped function read) is a problem: our object counts its bytes, the link drops them. A section absent from
    the result is compared as the object carries it; a note says why when a removal cannot be modelled.
    `({}, [], [])` without link context."""
    if link_ctx is None:
        return {}, [], []
    weak, kept_copies = weak_duplicate_symbols(unit, src_path, obj_path, target_rel, link_ctx)
    weak_names = {s.name for s in weak}
    unwind = dropped_unwind_symbols(src_path, {s.name for s in weak if s.section == STRIPPED_CODE})
    code, problems, weak_dead = [], [], []
    if STRIPPED_CODE in ours:
        dead = objcompare.dead_symbols(src_path, deadstrip_roots(unit, target_rel, link_ctx)) or []
        target_names = set(object_symbols(obj_path)[1])
        rows = map_symbols() if map_rows is None else map_rows
        code = [s for s in dead if s.section == STRIPPED_CODE and s.name not in target_names and s.name not in rows
                and s.name not in weak_names]
        data = [s for s in dead if s.section != STRIPPED_CODE and s.section in claim and s.name not in weak_names]
        # a weak copy (an implicit vtable, an inline's object) that is dead, absent from the target and the map is
        # one retail stripped too: it leaves the layout like a stripped function; anything else stays a refusal
        weak_dead = [s for s in data if s.bind == objcompare.STB_WEAK and s.name not in target_names
                     and s.name not in rows]
        data = [s for s in data if s.index not in {w.index for w in weak_dead}]
        if code and data:
            problems.append("%s: the link dead-strips %d data symbol(s) with the stripped function(s) %s - %s - which "
                            "our object counts and the link drops, so the linked section(s) fall short of the claim"
                            % (unit, len(data), ", ".join(s.name for s in code),
                               ", ".join("%s 0x%X B of %s" % (s.name, s.size, s.section) for s in data)))
    # a kept weak copy's note ("the link keeps our copy") is wrong once the dead-strip takes the copy anyway
    gone = {s.name for s in code} | {s.name for s in weak_dead}
    notes = [text for name, text in kept_copies if name not in gone]
    by_section: dict[str, tuple[list, list, list, list]] = {}
    for kind, symbols in enumerate((code, weak, weak_dead, unwind)):
        for s in symbols:
            by_section.setdefault(s.section, ([], [], [], []))[kind].append(s)
    kept: dict[str, bytes] = {}
    for section, (stripped, copies, dead_data, records) in sorted(by_section.items()):
        dead_txt = ("dead-stripped function(s) %s (no symbols.txt row, not in the target, nothing in the link "
                    "references them)" % ", ".join(s.name for s in stripped)) if stripped else ""
        weak_txt = ("weak copies %s that an earlier link input also defines (the link keeps the first definition)"
                    % ", ".join(s.name for s in copies)) if copies else ""
        data_txt = ("dead-stripped weak data %s (no symbols.txt row, not in the target, nothing the link keeps "
                    "references them)" % ", ".join(s.name for s in dead_data)) if dead_data else ""
        if records:
            data_txt = "the unwind entries %s of the dropped weak copies (the link drops them with the function)"                 % ", ".join(s.name for s in records)
        laid = objcompare.strip_moves(src_path, section, stripped + copies + dead_data + records)
        got = None if laid is None else objcompare.apply_moves(*laid)
        if got is None:
            if stripped and not copies:
                notes.append("%s: %s would be dead-stripped (no symbols.txt row, not in the target, nothing in the "
                             "link references them) but the section is not laid out as aligned functions with zero "
                             "gaps, so the stripped layout is not modelled"
                             % (section, ", ".join(s.name for s in stripped)))
            else:
                notes.append("%s: the link removes %s but the section is not laid out as aligned symbols with zero "
                             "gaps, so the stripped layout is not modelled"
                             % (section, " and ".join(filter(None, [dead_txt, weak_txt, data_txt]))))
            continue
        kept[section] = got
        if moves is not None:
            moves[section] = laid[1]
        notes.append("%s: 0x%X of the object's 0x%X bytes are %s; the 0x%X bytes the link keeps are compared"
                     % (section, ours[section][0] - len(got), ours[section][0],
                        " and ".join(filter(None, [dead_txt, weak_txt, data_txt])), len(got)))
    return kept, problems, notes


def predecessor_fills(unit: str, section: str) -> bool:
    """Whether the section the link places before this claim ends exactly at the claim's start: the registered
    range that ends there is an unflipped stand-in (it fills its own claim) or a linked object whose built section
    fills its claim. No registered predecessor: False (the lead is not judged)."""
    own, _starts = _range_index()
    span = own.get((unit, section))
    if span is None:
        return False
    base = section.split("$")[0]
    linked = linked_units()
    for (stem, objsec), (start, end) in own.items():
        if end != span[0] or objsec.split("$")[0] != base or (stem, objsec) == (unit, section):
            continue
        if stem not in linked:
            return True
        built = sections(os.path.join(SRC, stem + ".o")).get(objsec)
        return built is not None and built[0] >= end - start
    return False


def leading_fill_of(unit: str, name: str, ours: tuple[int, int], size: int, obj_path: str) -> int | None:
    """`lib.objcompare.leading_fill` for one section of this unit: the target's bytes and symbols, the claim's start
    and our object's alignment, the predecessor judged by `predecessor_fills`."""
    own, _starts = _range_index()
    span = own.get((unit, name))
    if span is None or sections(obj_path).get(name) is None:
        return None
    data = raw_section(obj_path, name)
    nobits = data is not None and len(data) == 0 and size > 0
    return objcompare.leading_fill(name, ours[0], 1 << ours[1], size, None if nobits else data,
                                   section_symbols(obj_path, name), span[0], predecessor_fills(unit, name))


def check(unit: str, claim: dict[str, tuple[int, int]], refs: set[str] | None,
          link_ctx: dict | None = None, map_rows: set[str] | None = None,
          verbose: bool = False) -> tuple[list[str], list[str]]:
    src_path = os.path.join(SRC, unit + ".o")
    target_rel = os.path.normpath(os.path.join("build", "RMHE08", "obj", unit + ".o"))
    obj_path = os.path.join(MAIN, target_rel)
    ours = sections(src_path)
    if not ours:
        return missing_or_empty_object(unit, src_path, claim), []
    moves: dict[str, list] = {}              # section -> where the dead-strip model moves our symbols
    kept, problems, notes = deadstrip_model(unit, claim, ours, src_path, obj_path, target_rel, link_ctx, map_rows,
                                            moves)
    stripped: dict[str, str] = {}
    if kept:
        ours = dict(ours)
        for name, data in kept.items():
            stripped[name] = ("dead-strips 0x%X B of function(s) from the object's 0x%X" if name == STRIPPED_CODE
                              else "drops 0x%X B of symbol(s) from the object's 0x%X") % (
                                  ours[name][0] - len(data), ours[name][0])
            ours[name] = (len(data), ours[name][1])
    leads: dict[str, int] = {}              # section -> fill the link puts before our bytes
    for name, (size, _) in sorted(claim.items()):
        got = ours.get(name)
        if got is None or got[0] >= size:
            continue
        lead = leading_fill_of(unit, name, got, size, obj_path)
        if lead:
            leads[name] = lead
            ours = dict(ours)
            ours[name] = (got[0] + lead, got[1])
            notes.append("%s: the 0x%X-byte lead is alignment fill (target zero, no symbol but dtk's fill labels, the "
                         "predecessor ends at the claim's start and our section aligned to %d starts 0x%X in) - our "
                         "0x%X bytes are compared at +0x%X" % (name, lead, 1 << got[1], lead, got[0], lead))
    padded: dict[str, int] = {}             # section -> our size, where the shortfall is only alignment fill
    sized: set[str] = set()                 # sections whose size line already refuses the flip
    for name, (size, _) in sorted(claim.items()):
        got = ours.get(name)
        if got is None:
            problems.append("splits.txt claims %s (0x%X) but the object emits no such section - "
                            "flipping drops %d bytes from the link and shifts everything after it"
                            % (name, size, size))
        elif got[0] != size:
            pad = pad_note(unit, name, got, size, obj_path) if got[0] < size else None
            hint = None
            if got[0] < size and not pad:
                pad, hint = linked_pad_note(unit, name, got, size, obj_path)
            if pad:
                padded[name] = got[0]
                notes.append(pad)
                continue
            if name in stripped:
                hint = "; ".join(filter(None, [hint, "after the link %s" % stripped[name]]))
            sized.add(name)
            problems.append("%s: object is 0x%X, splits.txt claims 0x%X (%+d)%s"
                            % (name, got[0], size, got[0] - size, " - " + hint if hint else ""))
    for name in sorted(set(ours) - set(claim)):
        problems.append("%s (0x%X) is in the object but not claimed by splits.txt - "
                        "it will be linked somewhere the original had nothing" % (name, ours[name][0]))

    generated = sorted(n for n in ours if n.startswith(COMPILER_GENERATED))
    if generated:
        notes.append("compiler-generated fragments, byte-checked above: %s"
                     % ", ".join("%s 0x%X" % (n, ours[n][0]) for n in generated))

    # sizes and alignment matching is not enough: the bytes have to be the original's too.
    for name in sorted(set(ours) & set(claim)):
        mine = kept[name] if name in kept else raw_section(src_path, name)
        tgt = raw_section(obj_path, name)
        if mine is None or tgt is None:
            continue
        if name in leads and mine:
            mine = bytes(leads[name]) + mine    # the lead is fill (judged above): zero, as the target's is
        if name in padded:
            tgt = tgt[:padded[name]]        # the tail is fill (judged above): only our own bytes are compared
        if mine != tgt:
            if name not in sized or objcompare.first_difference(mine, tgt) is not None:
                problems += section_byte_problems(name, mine, tgt, src_path, obj_path)
            # else: the bytes both objects have are equal and the size line already refuses the length
            problems += data_seam_problems(unit, name, src_path, obj_path)

    problems += pool_group_problems(unit, claim, ours, src_path, obj_path, padded=padded, leads=leads)
    problems += positioned_symbol_problems(claim, ours, src_path, obj_path, leads=leads, link_ctx=link_ctx,
                                           target_rel=target_rel, notes=notes)
    problems += relocation_place_problems(unit, claim, src_path, obj_path, leads=leads, moves=moves)

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
            problems += reverse_reference_problems(unit, target_rel, obj_path, src_path, link_ctx)
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
    BATCH.update(norm_unit(u.strip("/")) for u in args.units)

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
