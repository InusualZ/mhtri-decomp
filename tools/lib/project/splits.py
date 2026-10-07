"""The `splits.txt` model: a byte-exact round-tripping parse, range lookups and block edits.
Spec: docs/tools/spec/lib-project.md. CLI: none (library)."""
from __future__ import annotations

import bisect
import os
import re
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Iterable

#: dtk's section order (the `Sections:` legend of this project's `splits.txt`).
SECTION_ORDER = (".init", "extab", "extabindex", ".text", ".ctors", ".dtors", ".rodata", ".data", ".bss",
                 ".sdata", ".sbss", ".sdata2", ".sbss2")
#: A unit header: an unindented `name:` line, optionally followed by attributes (`name: comment:0`).
UNIT_RE = re.compile(r"^(?P<unit>\S.*?):(?:\s+(?P<attrs>\S.*?))?\s*$")
#: A range: an indented `section start:0x.. end:0x..` line with optional attributes (`rename:.ctors$10`).
RANGE_RE = re.compile(r"^\s+(?P<section>\S+)\s+start:0x(?P<start>[0-9A-Fa-f]+)\s+end:0x(?P<end>[0-9A-Fa-f]+)"
                      r"(?P<attrs>.*?)\s*$")
_RENAME_RE = re.compile(r"(?:^|\s)rename:(\S+)")


_CACHE: dict = {}


def stem(unit: str) -> str:
    """A unit key without its extension and a leading `/` (`Pl/pl_act.cpp` -> `Pl/pl_act`)."""
    return os.path.splitext(unit.replace("\\", "/").lstrip("/"))[0]


@dataclass(frozen=True)
class Range:
    """One claimed range: `[start, end)` of `section`, owned by `unit` (the key as `splits.txt` spells it)."""
    unit: str
    section: str
    start: int
    end: int
    attrs: str = ""

    @property
    def size(self) -> int:
        return self.end - self.start

    @property
    def rename(self) -> str | None:
        """The `rename:` attribute (dtk's `.ctors$10`-style object section name), or None."""
        m = _RENAME_RE.search(self.attrs)
        return m.group(1) if m else None

    @property
    def object_section(self) -> str:
        """The section name the unit's *object* carries: the `rename:` target, else the split's own section."""
        return self.rename or self.section

    def contains(self, address: int) -> bool:
        return self.start <= address < self.end

    def overlaps(self, lo: int, hi: int) -> bool:
        return self.start < hi and lo < self.end

    def line(self) -> str:
        """The canonical `splits.txt` row (what dtk writes)."""
        out = "\t%-11s start:0x%08X end:0x%08X" % (self.section, self.start, self.end)
        return out + (" " + self.attrs if self.attrs else "")


@dataclass(frozen=True)
class Block:
    """One unit's block: its header, its ranges and, while unedited, the exact lines it was parsed from."""
    unit: str
    attrs: str
    ranges: tuple[Range, ...]
    lines: tuple[str, ...] | None = None     # raw header + body lines; None once edited (rendered canonically)
    lead: tuple[str, ...] = ("",)            # the raw blank/comment lines before the header

    def render_lines(self) -> list[str]:
        if self.lines is not None:
            return [*self.lead, *self.lines]
        head = "%s:%s" % (self.unit, (" " + self.attrs) if self.attrs else "")
        return [*self.lead, head, *(r.line() for r in self.ranges)]


class Splits:
    """A parsed `splits.txt`. `render()` reproduces the parsed text byte for byte until a block is edited;
    the edit methods return a new `Splits` (the value is never mutated in place)."""

    def __init__(self, header: Iterable[str] = (), blocks: Iterable[Block] = (), newline: str = "\n",
                 trailer: Iterable[str] = (), final_newline: bool = True) -> None:
        self.header = tuple(header)
        self.blocks = tuple(blocks)
        self.newline = newline
        self.trailer = tuple(trailer)
        self.final_newline = final_newline
        self._index: dict[str, tuple[list[int], list[int], list[Range]]] | None = None

    # --- reading ---------------------------------------------------------------------------------------
    @classmethod
    def parse(cls, text: str) -> "Splits":
        """Parse a whole file or a fragment (a diff line, a conflict side): a fragment has no legend."""
        newline = "\r\n" if "\r\n" in text else "\n"
        lines = text.split(newline)
        final_newline = bool(lines) and lines[-1] == ""
        if final_newline:
            lines.pop()
        header: list[str] = []
        i = 0
        if lines and lines[0].startswith("Sections:"):
            header.append(lines[0])
            i = 1
            while i < len(lines) and lines[i][:1].isspace() and lines[i].strip():
                header.append(lines[i])
                i += 1
        blocks: list[Block] = []
        pending: list[str] = []
        cur: dict | None = None

        def close() -> None:
            if cur is not None:
                blocks.append(Block(cur["unit"], cur["attrs"], tuple(cur["ranges"]), tuple(cur["lines"]),
                                    tuple(cur["lead"])))

        for line in lines[i:]:
            stripped = line.strip()
            if not stripped or stripped.startswith(("#", "//")):
                pending.append(line)
                continue
            if not line[:1].isspace():
                m = UNIT_RE.match(line)
                if m and not line.startswith("Sections:"):
                    close()
                    cur = {"unit": m.group("unit"), "attrs": m.group("attrs") or "", "ranges": [],
                           "lines": [line], "lead": pending}
                    pending = []
                    continue
                pending.append(line)              # an unknown unindented line: kept, never a unit
                continue
            if cur is None:
                pending.append(line)              # a range outside any block: kept, never a claim
                continue
            cur["lines"].extend(pending)
            pending = []
            cur["lines"].append(line)
            m = RANGE_RE.match(line)
            if m:
                cur["ranges"].append(Range(cur["unit"], m.group("section"), int(m.group("start"), 16),
                                           int(m.group("end"), 16), m.group("attrs").strip()))
        close()
        return cls(header, blocks, newline, pending, final_newline)

    @classmethod
    def read(cls, path: str | os.PathLike) -> "Splits":
        """Parse the file at `path` (UTF-8, its own line endings kept)."""
        return cls.parse(Path(path).read_bytes().decode("utf-8", errors="replace"))

    @classmethod
    def cached(cls, path: str | os.PathLike) -> "Splits":
        """`read(path)`, parsed once per (path, mtime, size) - a `Splits` is never mutated, so sharing is safe."""
        st = os.stat(path)
        key = (os.path.abspath(path), st.st_mtime_ns, st.st_size)
        if key not in _CACHE:
            _CACHE.clear()
            _CACHE[key] = cls.read(path)
        return _CACHE[key]

    def render(self) -> str:
        """The file text; byte-identical to the parsed text for every unedited block."""
        out = list(self.header)
        for block in self.blocks:
            out.extend(block.render_lines())
        out.extend(self.trailer)
        text = self.newline.join(out)
        return text + self.newline if self.final_newline and out else text

    # --- the views -------------------------------------------------------------------------------------
    @property
    def units(self) -> list[str]:
        """Every unit key in file order (a duplicated key appears twice - that is a defect to report)."""
        return [b.unit for b in self.blocks]

    @property
    def ranges(self) -> list[Range]:
        """Every range in file order."""
        return [r for b in self.blocks for r in b.ranges]

    def sections(self) -> list[str]:
        """The section names of the `Sections:` legend, in order."""
        return [ln.split()[0] for ln in self.header[1:] if ln.split()]

    def block(self, unit: str) -> Block | None:
        """The block keyed exactly `unit`, else the one whose key has the same stem (either spelling)."""
        for b in self.blocks:
            if b.unit == unit:
                return b
        want = stem(unit)
        for b in self.blocks:
            if stem(b.unit) == want:
                return b
        return None

    def claims(self, unit: str) -> list[Range]:
        """The ranges of `unit` (exact key, else by stem), in file order."""
        b = self.block(unit)
        return [r for x in self.blocks if b is not None and x.unit == b.unit for r in x.ranges]

    def text_ranges(self) -> list[Range]:
        """Every `.text` range, in file order."""
        return [r for r in self.ranges if r.section == ".text"]

    def by_section(self) -> dict[str, list[tuple[int, int, str]]]:
        """`{section: [(start, end, unit)]}`, each list sorted - the shape the ownership index reads."""
        out: dict[str, list[tuple[int, int, str]]] = {}
        for r in self.ranges:
            out.setdefault(r.section, []).append((r.start, r.end, r.unit))
        return {s: sorted(v) for s, v in out.items()}

    def by_unit(self) -> dict[str, dict[str, tuple[int, int]]]:
        """`{unit: {section: (start, end)}}` in file order (the last range of a section wins)."""
        out: dict[str, dict[str, tuple[int, int]]] = {}
        for b in self.blocks:
            d = out.setdefault(b.unit, {})
            for r in b.ranges:
                d[r.section] = (r.start, r.end)
        return out

    def _section_index(self, section: str) -> tuple[list[int], list[int], list[Range]]:
        if self._index is None:
            groups: dict[str, list[Range]] = {}
            for r in self.ranges:
                groups.setdefault(r.section, []).append(r)
            index = {}
            for sec, rs in groups.items():
                rs.sort(key=lambda r: (r.start, r.end, r.unit))
                starts = [r.start for r in rs]
                reach, top = [], -1
                for r in rs:
                    top = max(top, r.end)
                    reach.append(top)
                index[sec] = (starts, reach, rs)
            self._index = index
        return self._index.get(section, ([], [], []))

    def covering(self, section: str, address: int) -> Range | None:
        """The range of `section` containing `address` (the first in address order if ranges overlap)."""
        starts, reach, rs = self._section_index(section)
        j = bisect.bisect_right(starts, address) - 1
        hit = None
        while j >= 0 and reach[j] > address:
            if rs[j].contains(address):
                hit = rs[j]
            j -= 1
        return hit

    def overlap(self, section: str | None, lo: int, hi: int) -> list[Range]:
        """Every range overlapping `[lo, hi)` (in `section`, or in any section when it is None), file order."""
        return [r for r in self.ranges if (section is None or r.section == section) and r.overlaps(lo, hi)]

    def neighbours(self, section: str, address: int) -> tuple[Range | None, Range | None]:
        """The ranges of `section` ending at or below `address` and starting above it (the bracketing claims)."""
        _starts, _reach, rs = self._section_index(section)
        prev = nxt = None
        for r in rs:
            if r.end <= address:
                prev = r
            elif r.start > address and nxt is None:
                nxt = r
        return prev, nxt

    # --- edits (each returns a new Splits) -------------------------------------------------------------
    def add_block(self, unit: str, ranges: Iterable[tuple[str, int, int] | Range], attrs: str = "",
                  before: str | None = None) -> "Splits":
        """A copy with a new block for `unit` (appended, or inserted before the block keyed `before`)."""
        rows = tuple(r if isinstance(r, Range) else Range(unit, r[0], r[1], r[2]) for r in ranges)
        rows = tuple(replace(r, unit=unit) for r in rows)
        new = Block(unit, attrs, rows, None, ("",))
        blocks = list(self.blocks)
        at = next((i for i, b in enumerate(blocks) if b.unit == before), len(blocks)) if before else len(blocks)
        blocks.insert(at, new)
        return Splits(self.header, blocks, self.newline, self.trailer, self.final_newline)

    def rename_unit(self, old: str, new: str) -> "Splits":
        """A copy with every block keyed `old` keyed `new` (its raw lines kept, only the header token changes)."""
        if not any(b.unit == old for b in self.blocks):
            raise KeyError("no block keyed %r" % old)
        blocks = []
        for b in self.blocks:
            if b.unit != old:
                blocks.append(b)
                continue
            lines = b.lines
            if lines is not None:
                lines = (new + lines[0][len(old):],) + lines[1:]
            blocks.append(Block(new, b.attrs, tuple(replace(r, unit=new) for r in b.ranges), lines, b.lead))
        return Splits(self.header, blocks, self.newline, self.trailer, self.final_newline)

    def remove_block(self, unit: str) -> "Splits":
        """A copy without the block(s) keyed `unit`."""
        blocks = [b for b in self.blocks if b.unit != unit]
        if len(blocks) == len(self.blocks):
            raise KeyError("no block keyed %r" % unit)
        return Splits(self.header, blocks, self.newline, self.trailer, self.final_newline)


def first_overlap(ranges: Iterable[Range | tuple[str, str, int, int]], start: int, end: int,
                  section: str | None = None, unit: str | None = None) -> Range | None:
    """The first claim overlapping `[start, end)` (in `section`, or in any when it is None), or None.

    `ranges` are `Range`s or `(unit, section, start, end)` tuples. A claim by the same `unit`, in `.text`,
    with the identical range is the idempotent re-apply of one block, not a collision.
    """
    for r in ranges:
        r = r if isinstance(r, Range) else Range(*r)
        if section is not None and r.section != section:
            continue
        if unit is not None and r.unit == unit and r.section == ".text" and (r.start, r.end) == (start, end):
            continue
        if r.overlaps(start, end):
            return r
    return None


def range_moves(before: Splits, after: Splits, section: str = ".text") -> list[tuple[str, str, int, int]]:
    """Where `section` changed hands between two `splits.txt` texts: `[(from_unit, to_unit, start, end)]`.

    Every address owned by one unit before and another after is a move; adjacent addresses with the same pair
    merge into one row. An address claimed on one side only (a new claim, a dropped one) is not a move."""
    cuts = sorted({p for s in (before, after) for r in s.ranges if r.section == section for p in (r.start, r.end)})
    out: list[list] = []
    for lo, hi in zip(cuts, cuts[1:]):
        a, b = before.covering(section, lo), after.covering(section, lo)
        if a is None or b is None or stem(a.unit) == stem(b.unit):
            continue
        if out and out[-1][0] == a.unit and out[-1][1] == b.unit and out[-1][3] == lo:
            out[-1][3] = hi
        else:
            out.append([a.unit, b.unit, lo, hi])
    return [tuple(row) for row in out]


def touched_units(before: Splits, after: Splits) -> set[str]:
    """The units whose claimed ranges differ between two `splits.txt` texts (any section; added or removed too)."""
    def claims(s: Splits) -> dict[str, set[tuple[str, int, int]]]:
        out: dict[str, set[tuple[str, int, int]]] = {}
        for r in s.ranges:
            out.setdefault(r.unit, set()).add((r.section, r.start, r.end))
        return out
    b, a = claims(before), claims(after)
    return {u for u in set(a) | set(b) if a.get(u) != b.get(u)}


def parse(text: str) -> Splits:
    """`Splits.parse(text)`."""
    return Splits.parse(text)


def read(path: str | os.PathLike) -> Splits:
    """`Splits.read(path)`."""
    return Splits.read(path)
