"""MWCC's exception tables: which `extab`/`extabindex` records cover which `.text` functions of an object.
Spec: docs/tools/spec/lib-binary.md. CLI: none (library)."""
from __future__ import annotations

from tools.lib.binary.elf import STB_WEAK, STT_FUNC, STT_SECTION, Elf

#: One `extabindex` entry: the function's address, its size, the address of its `extab` record.
EXTABINDEX_ENTRY = 12


def weak_text_functions(elf: Elf) -> set[str]:
    """Names of the weak functions of size > 0 the object defines in `.text` - MWCC's out-of-line copies of inline
    functions, the ones a link can drop for another input's definition."""
    return {s.name for s in elf.symbols
            if s.name and s.defined and s.bind == STB_WEAK and s.type == STT_FUNC and s.size > 0
            and s.section == ".text"}


def unwind_entries(elf: Elf, functions: set[str]) -> dict[str, dict[int, int]]:
    """`{"extab": {offset: size}, "extabindex": {offset: size}}`: the unwind records of the `.text` functions named in
    `functions` - the records mwld drops together with a function it drops (a later weak copy).

    An `extabindex` entry is 12 bytes: +0 relocates the function it covers (by name, or `.text`'s section symbol plus
    the function's offset), +8 the `extab` record. The record's size is its sized symbol's, else the distance to the
    next record an entry names (or the section's end). Both dicts are empty when nothing matches."""
    out: dict[str, dict[int, int]] = {"extab": {}, "extabindex": {}}
    if not functions:
        return out
    syms = elf.symbols
    starts = {s.value for s in syms if s.defined and s.section == ".text" and s.name in functions}
    index = {r.offset: r for r in elf.relocs("extabindex")}
    extab = elf.section("extab")

    def target(r) -> tuple[str, int] | None:
        sym = syms[r.symbol] if r.symbol < len(syms) else None
        if sym is None or not sym.defined:
            return None
        return (sym.section, r.addend) if sym.type == STT_SECTION else (sym.section, sym.value + r.addend)

    records = sorted({place[1] for off, r in index.items() if off % EXTABINDEX_ENTRY == 8
                      for place in [target(r)] if place is not None and place[0] == "extab"})
    for offset, r in sorted(index.items()):
        if offset % EXTABINDEX_ENTRY:
            continue
        sym = syms[r.symbol] if r.symbol < len(syms) else None
        covered = target(r)
        if sym is None or covered is None or covered[0] != ".text":
            continue
        if sym.name not in functions and not (sym.type == STT_SECTION and covered[1] in starts):
            continue
        out["extabindex"][offset] = EXTABINDEX_ENTRY
        record = index.get(offset + 8)
        place = target(record) if record is not None else None
        if place is None or place[0] != "extab":
            continue
        size = next((s.size for s in syms if s.section == "extab" and s.value == place[1] and s.size
                     and s.type != STT_SECTION), 0)
        if not size:
            later = [o for o in records if o > place[1]]
            size = (later[0] if later else (extab.size if extab is not None else place[1])) - place[1]
        if size > 0:
            out["extab"][place[1]] = size
    return out
