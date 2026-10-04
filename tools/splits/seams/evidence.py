"""The TU-seam evidence the seam tools share: `.data` emission order, the literal pool model, source-file names.
Spec: docs/tools/spec/seams.md. CLI: none (the entry points are tudiscover, dataorder, dataseams, poolseams)."""
from __future__ import annotations

import collections
import re

from tools.lib import project as _project
from tools.lib.binary.dol import Dol as _Dol

GAME = "RMHE08"


# ---- the retail image ------------------------------------------------------------------------------------------------

class Image(_Dol):
    """Address -> bytes of the retail `main.dol`: `read(addr, n)` starts inside a segment and may run on past its end
    (`lib.binary.dol.Dol.bytes_from`), the reading every seam kind was measured with; `secs` is the segment table."""

    def __init__(self, path: str) -> None:
        with open(path, "rb") as handle:
            super().__init__(handle.read(), path)
        self.secs = [(s.address, s.size, s.offset) for s in self.segments]

    def read(self, addr: int, n: int) -> bytes | None:
        return self.bytes_from(addr, n)


def map_tables(path: str) -> tuple[dict, dict]:
    """`(functions, labels)`: name -> record from the map (never printed): every `.text` function with `addr`, `size`
    and `scope` (`local` or ''), every non-`.text` row as a label with `section`, `addr`, `size`, `kind` (the `data:`
    attribute) and `local`."""
    fns, labels = {}, {}
    for e in _project.SymbolMap(path).rows():
        if e.section == ".text" and e.type == "function":
            fns[e.name] = {"addr": e.address, "size": e.size, "scope": "local" if e.scope == "local" else ""}
        elif e.section != ".text":
            labels[e.name] = {"section": e.section, "addr": e.address, "size": e.size, "kind": e.kind,
                              "local": e.scope == "local"}
    return fns, labels


def map_rows(path: str) -> list[tuple[str, int, int | None, str]]:
    """`(section, address, size-or-None, name)` for every map row (streamed through `lib.project.SymbolMap`)."""
    return [(e.section, e.address, e.size if e.sized else None, e.name) for e in _project.SymbolMap(path).rows()]


def section_ranges(path: str, section: str = ".data") -> dict[int, tuple[str, int]]:
    """`start -> (unit, end)` for every registered range of `section` in `splits.txt`."""
    return {r.start: (r.unit, r.end) for r in _project.Splits.read(path).ranges if r.section == section}


def range_of(ranges: dict[int, tuple[str, int]], addr: int) -> tuple[str, int, int] | None:
    """`(unit, start, end)` of the first range (in file order) of `section_ranges`' shape that holds `addr`."""
    for start, (unit, end) in ranges.items():
        if start <= addr < end:
            return unit, start, end
    return None


# ---- .data emission order --------------------------------------------------------------------------------------------

VTABLE, STRING, DATA = "V", "S", "D"
PRINTABLE = set(range(32, 127)) | {9, 10, 13}
#: A `D` symbol this small between a vtable and the next symbol is alignment padding, not a new object.
PAD_MAX = 8
STRONG_KINDS = ("V->S", "zigzag")
#: The widest `V->S` gap (in symbols) whose boundary is cut (at the end of its inline tail); a wider one only warns.
NARROW = 8
#: Most non-header strings that may sit between two header names (or before the first one) of one inline tail.
TAIL_RUN_MAX = 3

HEADER_NAME_RE = re.compile(r"^[A-Za-z0-9_./\\-]*\.(?:h|hpp|inl)$")
#: A bare source-file name: the `__FILE__` of an out-of-line function's assert (an anchor, and the end of an inline tail).
SOURCE_NAME_RE = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_./\\-]*\.(?:c|cpp|cc|cxx|cp|c\+\+)$")


class Sym:
    """One `.data` symbol with its size, kind and (for a vtable) owner - the address of its first code slot."""

    __slots__ = ("addr", "size", "name", "kind", "owner", "text")

    def __init__(self, addr, size, name, kind=DATA, owner=None, text=None):
        self.addr, self.size, self.name, self.kind, self.owner, self.text = addr, size, name, kind, owner, text

    def as_dict(self):
        d = {"addr": self.addr, "size": self.size, "name": self.name, "kind": self.kind}
        if self.owner is not None:
            d["owner"] = self.owner
        return d


def is_header_name(text: str | None) -> bool:
    """A bare header file name (`g3d_resnode_ac.h`): the `__FILE__` of an assert in an INLINE function."""
    return bool(text) and bool(HEADER_NAME_RE.match(text.strip()))


def is_source_name(text: str | None) -> bool:
    """A bare source file name (`g3d_anmvis.cpp`): the `__FILE__` of an assert in an OUT-OF-LINE function."""
    return bool(text) and bool(SOURCE_NAME_RE.match(text.strip()))


def inline_tail(gap: list[Sym]) -> int:
    """How many leading symbols of a `V->S` gap are the first TU's inline tail (0 when nothing says so)."""
    last = run = 0
    for n, s in enumerate(gap):
        if s.kind != STRING or is_source_name(s.text):
            break
        if is_header_name(s.text):
            last, run = n + 1, 0
        else:
            run += 1
            if run > TAIL_RUN_MAX:
                break
    return last


def text_range(rows) -> tuple[int, int]:
    """The `.init`/`.text` span, from the map: a vtable slot must point into it."""
    text = [r for r in rows if r[0] in (".text", ".init")]
    return min(r[1] for r in text), max(r[1] + (r[2] or 0) for r in text)


def data_symbols(rows) -> list[tuple[int, int, str]]:
    """The `.data` rows as `(address, size, name)`, sizes filled from the next symbol when the map has none."""
    data = sorted((r for r in rows if r[0] == ".data"), key=lambda r: r[1])
    out = []
    for i, (_sec, addr, size, name) in enumerate(data):
        nxt = data[i + 1][1] if i + 1 < len(data) else None
        if not size:
            size = (nxt - addr) if nxt else 4
        if nxt and nxt > addr:
            size = min(size, nxt - addr)
        out.append((addr, size, name))
    return out


def classify(blob: bytes | None, tlo: int, thi: int) -> tuple[str, int | None]:
    """`(kind, owner)` of one symbol's bytes: a vtable (a `0, 0` header, then code pointers or zeros, at least one
    pointer), a string (printable, NUL-terminated, at least two characters) or data."""
    if not blob:
        return DATA, None
    if len(blob) >= 12 and len(blob) % 4 == 0:
        w = [int.from_bytes(blob[i:i + 4], "big") for i in range(0, len(blob), 4)]
        if w[0] == 0 and w[1] == 0 and all(x == 0 or tlo <= x < thi for x in w[2:]):
            ptr = [x for x in w[2:] if x]
            if ptr:
                return VTABLE, ptr[0]
    s = blob.rstrip(b"\0")
    if len(s) >= 2 and all(c in PRINTABLE for c in s) and sum(1 for c in s if c >= 32) >= 2:
        return STRING, None
    return DATA, None


def classify_all(rows, reader) -> list[Sym]:
    """Every `.data` symbol classified, in address order.  `reader.read(addr, n)` returns the retail bytes."""
    tlo, thi = text_range(rows)
    out = []
    for addr, size, name in data_symbols(rows):
        blob = reader.read(addr, size)
        kind, owner = classify(blob, tlo, thi)
        text = blob.rstrip(b"\0").decode("latin-1") if kind == STRING else None
        out.append(Sym(addr, size, name, kind, owner, text))
    return out


def seams(syms: list[Sym]) -> list[dict]:
    """Every seam after a vtable group: `{addr, kind, before, after, ...}` in address order (kinds in the spec)."""
    out = []
    for i, a in enumerate(syms):
        if a.kind != VTABLE:
            continue
        j = i + 1
        while j < len(syms) and syms[j].kind == DATA and syms[j].size <= PAD_MAX:
            j += 1
        if j >= len(syms):
            continue
        b = syms[j]
        row = {"addr": b.addr, "before": a.name, "after": b.name}
        if b.kind == STRING:
            k = j
            while k < len(syms) and syms[k].kind != VTABLE:
                k += 1
            tail = inline_tail(syms[j:k])
            if k < len(syms):
                row.update(kind="V->S", latest=syms[k].addr, width=k - j, tail=tail)
            else:
                row.update(kind="V->tail", width=k - j, tail=tail)
        elif b.kind == DATA:
            row["kind"] = "V->D"
        elif a.owner is not None and b.owner is not None and b.owner > a.owner:
            row["kind"] = "zigzag"
        else:
            continue
        out.append(row)
    return out


def tail_cut(syms: list[Sym], at: dict, row: dict) -> int:
    """Where a `V->S` gap is cut: the address of the first symbol after its inline tail (`at` = address -> index)."""
    return syms[min(at[row["addr"]] + row.get("tail", 0), len(syms) - 1)].addr


def strong_seams(syms: list[Sym]) -> list[dict]:
    """The strong rows of `seams` (`V->S`, `zigzag`); a `V->S` row also carries `cut` (`tail_cut`)."""
    found = [s for s in seams(syms) if s["kind"] in STRONG_KINDS]
    at = {x.addr: i for i, x in enumerate(syms)}
    for row in found:
        if row["kind"] == "V->S" and row["addr"] in at:
            row["cut"] = tail_cut(syms, at, row)
    return found


def zigzag_pairs(syms: list[Sym]) -> collections.Counter:
    """How many adjacent vtable pairs go `up` (a seam), `down` (the same TU) or `tie` (equal owners: no evidence)."""
    c = collections.Counter()
    for i, a in enumerate(syms):
        if a.kind != VTABLE:
            continue
        j = i + 1
        while j < len(syms) and syms[j].kind == DATA and syms[j].size <= PAD_MAX:
            j += 1
        if j < len(syms) and syms[j].kind == VTABLE and a.owner and syms[j].owner:
            c["up" if syms[j].owner > a.owner else ("down" if syms[j].owner < a.owner else "tie")] += 1
    return c


def fragments(syms: list[Sym], weak: bool = False) -> list[list[Sym]]:
    """Cut the run at every strong seam (and the weak `V->D`/`V->tail` ones when `weak`): one list per probable TU.
    A `V->S` gap of at most `NARROW` symbols is cut after its tail; a wider one is not cut."""
    at = {s.addr: i for i, s in enumerate(syms)}
    cuts = set()
    for row in seams(syms):
        if not (weak or row["kind"] in STRONG_KINDS):
            continue
        if row["kind"] == "V->S":
            if row["width"] > NARROW:
                continue
            cuts.add(tail_cut(syms, at, row))
        else:
            cuts.add(row["addr"])
    out, cur = [], []
    for s in syms:
        if s.addr in cuts and cur:
            out.append(cur)
            cur = []
        cur.append(s)
    if cur:
        out.append(cur)
    return out


def retail_symbols(symbols_path: str, dol_path: str) -> list[Sym]:
    """`classify_all` over the map and the retail image at these paths."""
    return classify_all(map_rows(symbols_path), Image(dol_path))


# ---- the literal pool --------------------------------------------------------------------------------------------------

LITERAL_SECTIONS = (".sdata2", ".sdata")
#: the int->float conversion constants: still a per-TU pool entry (the linker does not synthesise them), only counted
MAGIC = (bytes.fromhex("4330000080000000"), bytes.fromhex("4330000000000000"))
#: the `data:` kinds whose equal values prove two pool entries (a `4byte` word may be half of an 8-byte object)
VALUE_KINDS = ("float", "double")


def is_literal(section: str | None, size: int | None, kind: str | None, type_: str | None = "object") -> bool:
    """An object MWCC pools per TU: an `.sdata2` 4/8-byte object, or an `.sdata` string."""
    if section == ".sdata2":
        return (size or 0) in (4, 8) and type_ == "object"
    return section == ".sdata" and kind == "string"


def is_value_witness(section: str | None, size: int | None, kind: str | None, type_: str | None = "object") -> bool:
    """A pool literal whose bytes prove its identity: an `.sdata2` float/double (an `.sdata` string may be an initialised
    `char[]`, never pooled; an untyped word may be half of an 8-byte object the map cut in two)."""
    return section == ".sdata2" and kind in VALUE_KINDS and is_literal(section, size, kind, type_)


def components(pairs) -> list[set[str]]:
    """Connected components of two or more members over `pairs` of units (union-find), in first-seen order."""
    parent: dict[str, str] = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for a, b in pairs:
        parent[find(b)] = find(a)
    groups: dict[str, set[str]] = {}
    for u in list(parent):
        groups.setdefault(find(u), set()).add(u)
    return [g for g in groups.values() if len(g) >= 2]

