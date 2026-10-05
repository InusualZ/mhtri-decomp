"""Ownership: who owns an address or a symbol, from the symbol map, the splits and dtk's auto objects.
Spec: docs/tools/spec/lib-project.md. CLI: none (library)."""
from __future__ import annotations

import bisect
import json
import os
import re
from dataclasses import dataclass
from typing import Any, Callable, Iterable

from tools.lib.git import Git
from tools.lib.project.splits import Splits
from tools.lib.project.symbols import SymbolMap
from tools.lib.repo import VERSION

SYMBOLS_REL = "config/%s/symbols.txt" % VERSION
SPLITS_REL = "config/%s/splits.txt" % VERSION
CONFIG_JSON_REL = "build/%s/config.json" % VERSION
#: dtk names a split object after its section and start: `auto_03_802AE0C4_text`.
AUTO_OBJECT_RE = re.compile(r"^auto_\d+_([0-9a-fA-F]{8})_(\w+)$")
#: The four answers `owner_of` gives.
STATES = ("reconstructed", "registered", "auto", "unsplit")
#: The unsplit band's directory - the home of a declaration no registered unit owns. The one spelling of the path:
#: the owner's 2026-10-05 ruling moves it beside the sources (`src/unsplit/`), and that move edits only this line.
BAND_ROOT = "src/unsplit"
#: The band before the move (`include/unsplit`): still *classified* as the band, because a reader of a ref older than
#: the move (the gate's `--diff` back side) must read the band there the way it reads it here - never written.
LEGACY_BAND_ROOT = "include/unsplit"
BAND_ROOTS = (BAND_ROOT, LEGACY_BAND_ROOT)
#: What a header is: a file with one of these suffixes, wherever it lives (`src/**`; `include/**` before the move).
HEADER_SUFFIXES = (".h", ".hpp", ".hh")


def is_band_header(rel: str) -> bool:
    """Whether `rel` is a header in the unsplit band (`BAND_ROOT`, or `LEGACY_BAND_ROOT` in a pre-move tree)."""
    rel = rel.replace("\\", "/")
    return rel.startswith(tuple(r + "/" for r in BAND_ROOTS)) and rel.endswith(HEADER_SUFFIXES)


def band_root(root: str | os.PathLike | None = None) -> str:
    """The band directory of a tree, relative to it: `BAND_ROOT`, where a tool *writes* a band header (the move's
    dual-layout tolerance is gone; `root` is kept so a caller names the tree it means)."""
    return BAND_ROOT


def module_name(unit: str) -> str:
    """The module of a unit key: its directory, or its stem for a root-level file."""
    unit = unit.replace("\\", "/")
    d = unit.rsplit("/", 1)[0] if "/" in unit else ""
    return d or os.path.splitext(unit)[0]


@dataclass(frozen=True)
class Owner:
    """`owner_of`'s answer. `range` is `(start, end)` of the claim (or of the auto object, when it is known)."""
    state: str                       # reconstructed | registered | auto | unsplit
    unit: str | None = None          # the splits.txt key (registered/reconstructed) or the auto object name
    section: str | None = None
    range: tuple[int, int | None] | None = None
    band: str | None = None          # for an unsplit address: the module of the units bracketing it


class AutoObjects:
    """dtk's per-section `auto_*` split objects (`build/RMHE08/config.json`, else the object file names): an
    object runs to its recorded size, or - without one - to the next object of its section."""

    def __init__(self, entries: Iterable[tuple[str, int, int]]) -> None:
        self.ranges: dict[str, list[tuple[int, int | None, str]]] = {}
        rows = []
        for name, size in entries:
            m = AUTO_OBJECT_RE.match(name)
            if m:
                rows.append(("." + m.group(2), int(m.group(1), 16), size, name))
        for section, address, size, name in sorted(rows):
            self.ranges.setdefault(section, []).append([address, address + size if size else None, name])
        for section, ranges in self.ranges.items():
            for i in range(len(ranges) - 1):
                if ranges[i][1] is None:
                    ranges[i][1] = ranges[i + 1][0]
            self.ranges[section] = [tuple(r) for r in ranges]

    @classmethod
    def from_config(cls, config: dict | None, object_names: Iterable[str] = ()) -> "AutoObjects":
        """From dtk's parsed `config.json` (`units[].name`, `code_size + data_size`), else from object names."""
        if config:
            return cls((u.get("name", ""), u.get("code_size", 0) + u.get("data_size", 0))
                       for u in config.get("units", ()))
        return cls((n, 0) for n in object_names)

    @classmethod
    def load(cls, config_json: str | os.PathLike | None, obj_dir: str | os.PathLike | None = None) -> "AutoObjects":
        config = None
        if config_json and os.path.exists(config_json):
            with open(config_json, "r", encoding="utf-8") as fh:
                config = json.load(fh)
        names: list[str] = []
        if config is None and obj_dir and os.path.isdir(obj_dir):
            names = [os.path.splitext(n)[0] for n in os.listdir(obj_dir) if n.endswith(".o")]
        return cls.from_config(config, names)

    def covering(self, section: str, address: int) -> tuple[int, int | None, str] | None:
        for start, end, name in self.ranges.get(section, ()):
            if start <= address and (end is None or address < end):
                return start, end, name
        return None


def symbol_index(rows: Iterable[Any]) -> dict[str, list[tuple[str, int, str]]]:
    """`name -> [(section, address, type)]` from map rows (`Symbol`s) - the shape the index keeps."""
    out: dict[str, list[tuple[str, int, str]]] = {}
    for e in rows:
        out.setdefault(e.name, []).append((e.section, e.address, e.type))
    return out


class Ownership:
    """`symbol -> owning unit` and `address -> owner`, from the map rows and the splits ranges.

    `symbols` is `name -> [(section, address, type)]`; `ranges` is `section -> [(start, end, unit)]`. Two views
    of one set of rows: `resolve(name)` answers what a *name* means; `resolution_at(section, address)` what an
    *address* meant whatever its row is called (a rename keeps the address). `auto` adds dtk's auto objects
    for `owner_of`; `root` lets `owner_of` tell a reconstructed unit (its source exists) from a registered one.
    """

    def __init__(self, symbols: dict, ranges: dict, auto: AutoObjects | None = None,
                 root: str | os.PathLike | None = None) -> None:
        self.symbols = symbols
        self.ranges = {s: sorted(v) for s, v in ranges.items()}
        self.auto = auto
        self.root = None if root is None else str(root)
        self._at_address: dict | None = None
        self._reach: dict[str, tuple[list[int], list[int]]] = {}

    # --- construction -----------------------------------------------------------------------------------
    @classmethod
    def from_texts(cls, symbols_text: str, splits_text: str, **kw: Any) -> "Ownership":
        """From the two files' texts (what `git show` returns for a ref)."""
        from tools.lib.project.symbols import parse_line
        rows = (e for e in (parse_line(ln) for ln in symbols_text.splitlines()) if e is not None)
        return cls(symbol_index(rows), Splits.parse(splits_text).by_section(), **kw)

    @classmethod
    def from_files(cls, symbols_path: str | os.PathLike, splits_path: str | os.PathLike, **kw: Any) -> "Ownership":
        return cls(symbol_index(SymbolMap(symbols_path).rows()), Splits.read(splits_path).by_section(), **kw)

    @classmethod
    def load(cls, root: str | os.PathLike, auto: bool = False) -> "Ownership | None":
        """The tree's index, parsed once per (map, splits) mtime; None when either file is absent."""
        root = str(root)
        sym = os.path.join(root, SYMBOLS_REL)
        spl = os.path.join(root, SPLITS_REL)
        if not os.path.exists(sym) or not os.path.exists(spl):
            return None
        key = (cls, sym, spl, os.path.getmtime(sym), os.path.getmtime(spl), auto)
        if key not in _CACHE:
            autos = None
            if auto:
                autos = AutoObjects.load(os.path.join(root, CONFIG_JSON_REL),
                                         os.path.join(root, "build", VERSION, "obj"))
            _CACHE[key] = cls.from_files(sym, spl, auto=autos, root=root)
        return _CACHE[key]

    @classmethod
    def at_ref(cls, root: str | os.PathLike, ref: str,
               show: Callable[[str, str], bytes | None] | None = None) -> "Ownership | None":
        """The index as of `ref`, so each side of a diff is judged by the map it was written against; `show(ref,
        rel)` returns a file's bytes at the ref or None (default `lib.git.Git(root).show`). Cached per (root, ref)."""
        if not ref:
            return cls.load(root)
        show = show or Git(root).show
        key = (cls, str(root), ref)
        if key not in _REF_CACHE:
            texts = []
            for rel in (SYMBOLS_REL, SPLITS_REL):
                data = show(ref, rel)
                if data is None:
                    return None
                texts.append(data.decode("utf-8", "replace"))
            _REF_CACHE[key] = cls.from_texts(texts[0], texts[1])
        return _REF_CACHE[key]

    # --- the address view -------------------------------------------------------------------------------
    def covering(self, section: str, address: int) -> tuple[int, int, str] | None:
        """The `(start, end, unit)` claim of `section` containing `address` (the first in address order)."""
        rows = self.ranges.get(section)
        if not rows:
            return None
        if section not in self._reach:
            reach, top = [], -1
            for _s, e, _u in rows:
                top = max(top, e)
                reach.append(top)
            self._reach[section] = ([s for s, _e, _u in rows], reach)
        starts, reach = self._reach[section]
        j = bisect.bisect_right(starts, address) - 1
        hit = None
        while j >= 0 and reach[j] > address:
            s, e, u = rows[j]
            if s <= address < e:
                hit = rows[j]
            j -= 1
        return hit

    def band_of(self, section: str, address: int) -> str | None:
        """The module of the registered units bracketing `address` in `section`, or None when the nearest unit
        below and the nearest above name different modules (the bands interleave) or one side is missing."""
        rows = self.ranges.get(section)
        if not rows:
            return None
        lo = hi = None
        for start, _end, unit in rows:
            if start <= address:
                lo = unit
            elif hi is None:
                hi = unit
        if lo is None or hi is None:
            return None
        a, b = module_name(lo), module_name(hi)
        return a if a == b else None

    module = band_of

    def owner_of(self, section: str, address: int) -> Owner:
        """Who owns `(section, address)`: a registered unit (`reconstructed` when `root` is known and its source
        exists, else `registered`), an `auto` object, or nobody (`unsplit`, with the bracketing band)."""
        hit = self.covering(section, address)
        if hit is not None:
            start, end, unit = hit
            state = "registered"
            if self.root is not None and os.path.exists(os.path.join(self.root, "src", unit.replace("\\", "/"))):
                state = "reconstructed"
            return Owner(state, unit, section, (start, end))
        if self.auto is not None:
            obj = self.auto.covering(section, address)
            if obj is not None:
                return Owner("auto", obj[2], section, (obj[0], obj[1]))
        return Owner("unsplit", None, section, None, self.band_of(section, address))

    # --- the name view ----------------------------------------------------------------------------------
    def name_at(self, section: str, address: int) -> str | None:
        """The map's row name at an address (a name with duplicate rows is skipped), or None."""
        if self._at_address is None:
            index: dict = {}
            for name, entries in self.symbols.items():
                if len(entries) == 1:
                    index.setdefault((entries[0][0], entries[0][1]), name)
            self._at_address = index
        return self._at_address.get((section, address))

    def resolve(self, name: str) -> dict | None:
        """None when `name` is not in the map; else `{"kind": "dup"}`, an `owned` dict (unit, section, address,
        type) or an `unsplit` dict (section, address, type, module)."""
        entries = self.symbols.get(name)
        if not entries:
            return None
        if len(entries) != 1:
            return {"kind": "dup"}
        section, address, type_ = entries[0]
        hit = self.covering(section, address)
        if hit is not None:
            return {"kind": "owned", "unit": hit[2], "section": section, "address": address, "type": type_}
        return {"kind": "unsplit", "section": section, "address": address, "type": type_,
                "module": self.band_of(section, address)}

    def resolution_at(self, section: str, address: int) -> dict | None:
        """`resolve` of whatever row sits at `(section, address)`, or None when the map has none."""
        name = self.name_at(section, address)
        return self.resolve(name) if name else None

    def leaf_header_owner(self, rel: str, names: list[str]) -> str | None:
        """The unit that owns the header `rel` as a **leaf header**, or None (section 6.5 rule 2's one other owner
        spelling): `<module>/<symbol>.h` - a header anywhere outside the band (`include/` today, `src/` after the
        2026-10-05 move) - named for a symbol it declares (`names`), and declaring only
        symbols one registered unit defines. Strict: one unresolved, unowned or duplicate name, or symbols of two
        units, and it is not a leaf. One implementation: stylelint's rule 2 and integrate's already-applied test."""
        rel = rel.replace("\\", "/")
        if not rel.endswith(HEADER_SUFFIXES) or is_band_header(rel) or not names:
            return None
        if os.path.splitext(os.path.basename(rel))[0] not in names:
            return None
        units = set()
        for name in names:
            r = self.resolve(name)
            if r is None or r["kind"] != "owned":
                return None
            units.add(r["unit"])
        return units.pop() if len(units) == 1 else None

    def unit_of_symbol(self, name: str) -> str | None:
        """The registered unit owning `name`, or None (absent, duplicated or unsplit)."""
        r = self.resolve(name)
        return r["unit"] if r and r.get("kind") == "owned" else None

    def symbols_of_unit(self, unit: str, section: str | None = None) -> list[str]:
        """The map names inside `unit`'s claims (of `section` when given), in address order."""
        claims = [(s, a, b) for s, rows in self.ranges.items() for a, b, u in rows
                  if u == unit and (section is None or s == section)]
        hits = []
        for name, entries in self.symbols.items():
            for sec, address, _t in entries:
                if any(sec == s and a <= address < b for s, a, b in claims):
                    hits.append((address, name))
        return [n for _a, n in sorted(hits)]


_CACHE: dict = {}
_REF_CACHE: dict = {}


def load(root: str | os.PathLike, auto: bool = False) -> Ownership | None:
    """`Ownership.load(root)`."""
    return Ownership.load(root, auto=auto)


def owner_label(resolution: dict | None, source_exists: Callable[[str | None], bool]) -> tuple[str, str, str | None]:
    """`(label, state, unit)` for one `Ownership.resolve` result - the owner vocabulary `callers`/`callees` print:
    `reconstructed` (a registered unit with a source under `src/`), `registered` (a claim, no source yet), `unsplit`
    (no registered owner; the label names the band when the bracketing claims agree), `duplicate` or `unmapped`."""
    if resolution is None:
        return ("not in the symbol map", "unmapped", None)
    kind = resolution.get("kind")
    if kind == "dup":
        return ("duplicate row in the symbol map", "duplicate", None)
    if kind == "unsplit":
        module = resolution.get("module")
        where = "unsplit address"
        if module:
            where = "unsplit (%s)" % module
        return (where, "unsplit", None)
    unit = resolution.get("unit")
    if source_exists(unit):
        return (unit, "reconstructed", unit)
    return (unit + " (no source yet)", "registered", unit)


def source_exists(root: str | os.PathLike) -> Callable[[str | None], bool]:
    """A predicate: does the unit named in `splits.txt` have its file under `<root>/src/`?"""
    def exists(unit: str | None) -> bool:
        if not unit:
            return False
        return os.path.exists(os.path.join(root, "src", unit.replace("\\", "/")))
    return exists
