"""A function's raw bytes, target against ours, with only the relocated operand bits masked.
Spec: docs/tools/spec/lib-rawsame.md. CLI: none (library)."""
from __future__ import annotations

from dataclasses import dataclass, field

from tools.lib import objcompare
from tools.lib.binary.elf import STT_FUNC, Elf, reloc_name

#: the bits of the 32-bit instruction word a relocation of this type owns (the linker rewrites them). SDA21 owns the
#: low 16 bits only: its `rA` (the base register) is the compiler's choice and must match.
MASKS = {
    1: 0xFFFFFFFF, 24: 0xFFFFFFFF, 26: 0xFFFFFFFF,                        # ADDR32, UADDR32, REL32
    2: 0x03FFFFFC, 10: 0x03FFFFFC,                                        # ADDR24, REL24
    7: 0xFFFC, 8: 0xFFFC, 9: 0xFFFC, 11: 0xFFFC, 12: 0xFFFC, 13: 0xFFFC,  # ADDR14*, REL14*
    3: 0xFFFF, 4: 0xFFFF, 5: 0xFFFF, 6: 0xFFFF,                           # ADDR16, _LO, _HI, _HA
    32: 0xFFFF, 108: 0xFFFF, 109: 0xFFFF,                                 # SDAREL16, EMB_SDA2REL, EMB_SDA21
}
#: a relocation type this table does not know masks nothing: the whole word must match
MAX_DIFFS_PER_ROW = 4


@dataclass
class Row:
    """One function: `status` is `same`, `size`, `bytes`, `missing-target` or `missing-ours`; `diffs` is
    `[(offset, kind, target, ours)]` with `kind` `word` (hex words) or `reloc` (type names, `-` for none)."""
    name: str
    status: str
    target_size: int | None = None
    ours_size: int | None = None
    diffs: list[tuple[int, str, str, str]] = field(default_factory=list)

    @property
    def differs(self) -> bool:
        return self.status != "same"

    def line(self, unit: str = "") -> str:
        head = "%s%s" % (unit + ": " if unit else "", self.name)
        if self.status == "size":
            return "%s: size %s vs %s (target vs ours)" % (head, self.target_size, self.ours_size)
        if self.status.startswith("missing"):
            return "%s: %s" % (head, self.status.replace("-", " "))
        shown = ["+0x%X %s %s vs %s" % d for d in self.diffs[:MAX_DIFFS_PER_ROW]]
        more = len(self.diffs) - MAX_DIFFS_PER_ROW
        return "%s: %s%s" % (head, "; ".join(shown), " (+%d more)" % more if more > 0 else "")


def functions(elf: Elf) -> dict[str, tuple[int, int, int]]:
    """`{name: (section index, offset, size)}` of every defined function symbol (the first definition of a name)."""
    out: dict[str, tuple[int, int, int]] = {}
    nsec = len(elf.sections)
    for s in elf.symbols:
        if s.type == STT_FUNC and s.name and 0 < s.shndx < nsec and s.name not in out:
            out[s.name] = (s.shndx, s.value, s.size)
    return out


def word_relocs(elf: Elf) -> dict[int, dict[int, int]]:
    """`{section index: {word offset: relocation type}}`. A halfword relocation sits at the word's offset + 2 (the
    field it owns); the word it belongs to is the offset rounded down to four."""
    by_name: dict[str, int] = {}
    for sec in elf.sections:
        by_name.setdefault(sec.name, sec.index)
    out: dict[int, dict[int, int]] = {}
    for r in elf.relocs():
        index = by_name.get(r.section)
        if index is not None:
            out.setdefault(index, {})[r.offset & ~3] = r.type
    return out


def _word(blob: bytes, at: int) -> int:
    return int.from_bytes(blob[at:at + 4], "big")


def compare_function(name: str, t: Elf, o: Elf, tloc: dict, oloc: dict, trel: dict, orel: dict) -> Row:
    """The raw comparison of one function present in `tloc` and `oloc` (`functions`); relocation tables are
    `word_relocs`."""
    if name not in tloc:
        return Row(name, "missing-target", None, oloc[name][2])
    if name not in oloc:
        return Row(name, "missing-ours", tloc[name][2], None)
    tsec, tat, tsize = tloc[name]
    osec, oat, osize = oloc[name]
    if tsize != osize:
        return Row(name, "size", tsize, osize)
    tbytes, obytes = t.sections[tsec].raw, o.sections[osec].raw
    diffs: list[tuple[int, str, str, str]] = []
    for k in range(0, tsize, 4):
        rt = trel.get(tsec, {}).get(tat + k)
        ro = orel.get(osec, {}).get(oat + k)
        if rt != ro:
            diffs.append((k, "reloc", reloc_name(rt) if rt is not None else "-",
                          reloc_name(ro) if ro is not None else "-"))
            continue
        keep = ~MASKS.get(rt, 0) & 0xFFFFFFFF if rt is not None else 0xFFFFFFFF
        wt, wo = _word(tbytes, tat + k), _word(obytes, oat + k)
        if wt & keep != wo & keep:
            diffs.append((k, "word", "%08X" % wt, "%08X" % wo))
    return Row(name, "bytes" if diffs else "same", tsize, osize, diffs)


def compare(target, ours, names) -> list[Row]:
    """A `Row` per name in `names` (the functions to judge), in the given order: raw bytes and relocation types of
    the target object against ours, only relocated operand bits masked."""
    t, o = objcompare.load(target), objcompare.load(ours)
    tloc, oloc = functions(t), functions(o)
    trel, orel = word_relocs(t), word_relocs(o)
    return [compare_function(n, t, o, tloc, oloc, trel, orel) for n in names]
