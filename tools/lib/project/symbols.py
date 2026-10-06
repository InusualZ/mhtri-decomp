"""The `symbols.txt` model: one line parser, a streaming map reader, lookups, and planned renames and merges.
Spec: docs/tools/spec/lib-project.md. CLI: none (library; `tools/symbols/symedit.py` is the CLI)."""
from __future__ import annotations

import os
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterable, Iterator

from tools.lib.text import AnchorError, Transaction, line_ending, read_text

#: One map row: `name = section:0xADDR; // attrs`. The address may omit `0x`; the comment is optional.
LINE_RE = re.compile(r"^(?P<name>[^\s=]+)\s*=\s*(?P<loc>[^;]+);\s*(?://\s*(?P<comment>.*))?$")
ADDR_RE = re.compile(r"^(?P<section>[.\w]+):(?:0x)?(?P<addr>[0-9a-fA-F]+)$")
_TYPE_RE = re.compile(r"type:(\S+)")
_SIZE_RE = re.compile(r"size:(0x[0-9a-fA-F]+)")
_SCOPE_RE = re.compile(r"scope:(\S+)")
#: `data:` is a token of its own: `.sdata:`/`.rodata:`/`.data:` in an address all contain the substring.
_KIND_RE = re.compile(r"(?<![\w.])data:(\S+)")
_ALIGN_RE = re.compile(r"align:(\S+)")
#: A name `plan_rename` accepts as the new spelling: what MWCC emits and dtk/objdiff carry (measured 2026-10-06 by a
#: re-split + link + objdiff read of `probe_tmpl__Q24nw4r9Probe<i,c>Fv`, `@GUARD@probe_guard__Fv@x` and
#: `__sinit_\probe_unit_cpp`: main.dol OK, all three named in the split object and in objdiff's target side) -
#: template arguments `<` `>` `,` (and `-` for a negative one), MWCC's `@LOCAL@`/`@GUARD@`/`@<n>` labels and the
#: backslash of `__sinit_\<file>_cpp`. Never whitespace, `=`, `;`, `:` or `/`: the map line could not parse back.
VALID_NAME_RE = re.compile(r"[A-Za-z_@$][\w.$@<>,\-\\]*")


def name_pattern(name: str) -> "re.Pattern[str]":
    """A reference to the map name `name` in source text: the name, not inside a longer identifier. `\\b` cannot do it -
    a name may open with `@` or close with `>`, where there is no word boundary to find."""
    return re.compile(r"(?<![\w$@])%s(?![\w$])" % re.escape(name))


class Refused(Exception):
    """A planned edit the map's rules forbid; nothing was written. The message is the refusal."""


class ShapeError(AnchorError, ValueError):
    """A line is not the shape an edit depends on (the anchor gate); nothing was written."""


@dataclass(frozen=True, slots=True)
class Symbol:
    """One parsed map row. `line` is the row without its line ending; `lineno` is 1-based (0 = unknown)."""
    name: str
    section: str
    address: int
    type: str
    size: int
    line: str
    lineno: int = 0

    @property
    def comment(self) -> str:
        """The attribute text after `//` (`type:function size:0x10 scope:global`)."""
        return self.line.partition("//")[2].lstrip()

    @property
    def scope(self) -> str:
        m = _SCOPE_RE.search(self.comment)
        return m.group(1) if m else ""

    @property
    def kind(self) -> str:
        """The `data:` attribute (`string`, `float`, `4byte`, ...), or `""`."""
        m = _KIND_RE.search(self.comment)
        return m.group(1) if m else ""

    @property
    def align(self) -> int | None:
        m = _ALIGN_RE.search(self.comment)
        return int(m.group(1), 0) if m else None

    @property
    def hidden(self) -> bool:
        return "hidden" in self.comment.split()

    @property
    def sized(self) -> bool:
        """Whether the row states a `size:` at all (`size` is 0 both for `size:0x0` and for no size)."""
        return _SIZE_RE.search(self.comment) is not None

    @property
    def end(self) -> int:
        return self.address + self.size

    def to_dict(self) -> dict:
        """The dict `symedit.parse_line`/`entries` have always returned (`lineno` only when known)."""
        d = {"name": self.name, "section": self.section, "address": self.address, "type": self.type,
             "size": self.size, "line": self.line}
        if self.lineno:
            d["lineno"] = self.lineno
        return d


def parse_line(line: str, lineno: int = 0) -> Symbol | None:
    """The one map-line parser: a `Symbol`, or None for a line that is not a map row."""
    text = line.rstrip("\n").rstrip("\r") if line.endswith("\n") else line
    m = LINE_RE.match(text)
    if not m:
        return None
    loc = ADDR_RE.match(m.group("loc").strip())
    if not loc:
        return None
    comment = m.group("comment") or ""
    kind = _TYPE_RE.search(comment)
    size = _SIZE_RE.search(comment)
    return Symbol(m.group("name"), loc.group("section"), int(loc.group("addr"), 16),
                  kind.group(1) if kind else "", int(size.group(1), 16) if size else 0, text, lineno)


def rename_pairs(removed: Iterable[str], added: Iterable[str]) -> dict[str, str]:
    """`{old: new}` from the removed and added lines of a map diff: a row whose `section:address` is the only one
    removed and the only one added at that place, under another name, is a rename (the address is the identity).
    A place with several rows on either side, or the same name on both, is not one."""
    def by_place(lines: Iterable[str]) -> dict[tuple[str, int], list[str]]:
        out: dict[tuple[str, int], list[str]] = {}
        for line in lines:
            row = parse_line(line)
            if row is not None:
                out.setdefault((row.section, row.address), []).append(row.name)
        return out

    old, new = by_place(removed), by_place(added)
    return {o[0]: n[0] for place, o in old.items() for n in [new.get(place)]
            if n and len(o) == 1 and len(n) == 1 and o[0] != n[0]}


def stem_renames(rows: Iterable[Symbol]) -> dict[str, str]:
    """`{generated stem: map name}` for every row whose name is not dtk's generated stem for its address (`fn_<ADDR>`
    for a function, `lbl_<ADDR>` otherwise): a source still spelling the stem means that row, renamed."""
    out: dict[str, str] = {}
    for r in rows:
        stem = "%s_%08X" % ("fn" if r.type == "function" else "lbl", r.address)
        if r.name != stem:
            out.setdefault(stem, r.name)
    return out


def write_text(path: str | os.PathLike, text: str, rename: Callable | None = None) -> None:
    """Write `text` through a `lib.text.Transaction` and read it back: a failure, or bytes on disk that are
    not the planned ones, restores the previous bytes exactly and re-raises. `rename` is the fault seam."""
    tx = Transaction(rename=rename)
    try:
        tx.write(Path(path), text)
        if read_text(path) != text:
            raise IOError("verification failed: %s does not hold what was written" % path)
    except BaseException:
        tx.rollback()
        raise
    finally:
        tx.cleanup()


# --- planned edits ------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class RenamePlan:
    """What `apply` would write: `changed` is `(old, new, index, new_line)`, `applied` the pairs already done."""
    path: str
    text: str
    newline: str
    lines: tuple[str, ...]
    changed: tuple[tuple[str, str, int, str], ...]
    applied: tuple[tuple[str, str], ...]

    def render(self) -> str:
        out = list(self.lines)
        for _old, _new, i, new_line in self.changed:
            out[i] = new_line
        return self.newline.join(out)


@dataclass(frozen=True)
class MergePlan:
    """What `apply` would write: `grown` is `index -> new line`, `deleted` the indices to drop."""
    path: str
    text: str
    newline: str
    lines: tuple[str, ...]
    grown: dict = field(default_factory=dict)
    deleted: tuple[int, ...] = ()
    applied: tuple[tuple[str, str, int], ...] = ()

    @property
    def changed(self) -> bool:
        return bool(self.grown or self.deleted)

    def render(self) -> str:
        drop = set(self.deleted)
        return self.newline.join(self.grown.get(i, ln) for i, ln in enumerate(self.lines) if i not in drop)


def rewrite_name(line: str, old: str, new: str) -> str:
    """The name token of one definition line replaced, or `ShapeError` when the line is not that shape.

    The line must parse as a map row naming `old` and the rewrite must parse back as `new`, so a rename can
    never silently change nothing.
    """
    if not LINE_RE.match(line) or not parse_line(line):
        raise ShapeError("line %r is not a map line" % line[:80])
    out = re.sub(r"^%s(\s*=)" % re.escape(old), lambda m: new + m.group(1), line, count=1)
    if out == line:
        raise ShapeError("line %r does not start with the name %r" % (line[:80], old))
    parsed = parse_line(out)
    if parsed is None or parsed.name != new:
        raise ShapeError("rewriting %r did not name %r" % (line[:80], new))
    return out


def resize(line: str, name: str, new_size: int) -> str:
    """The `size:` field of `name`'s definition line set to `new_size` (upper-case hex, as dtk writes it),
    or `ShapeError` when the line is not `name`'s definition with a positive size."""
    e = parse_line(line)
    if not e or e.name != name:
        raise ShapeError("line %r is not the definition of %r" % (line[:80], name))
    if e.size <= 0:
        raise ShapeError("line %r has no size: field to grow" % line[:80])
    out = re.sub(r"size:0x[0-9a-fA-F]+", "size:0x%X" % new_size, line, count=1)
    if out == line:
        raise ShapeError("line %r has no size: field to grow" % line[:80])
    e2 = parse_line(out)
    if not e2 or e2.name != name or e2.size != new_size:
        raise ShapeError("rewriting %r did not set size:0x%X" % (line[:80], new_size))
    return out


def scope_of(line: str) -> str:
    """The `scope:` token of a map line, or `""`."""
    m = _SCOPE_RE.search(line)
    return m.group(1) if m else ""


def _definitions(lines: Iterable[str]) -> dict[str, list[int]]:
    out: dict[str, list[int]] = {}
    for i, line in enumerate(lines):
        e = parse_line(line)
        if e:
            out.setdefault(e.name, []).append(i)
    return out


def _names_at_two_addresses(lines: Iterable[str], names: set[str]) -> dict[str, list[tuple[str, int]]]:
    seen: dict[str, set] = {}
    for line in lines:
        e = parse_line(line)
        if e and e.name in names:
            seen.setdefault(e.name, set()).add((e.section, e.address))
    return {n: sorted(a) for n, a in seen.items() if len(a) > 1}


def _ending_at(lines: Iterable[str], address: int) -> list[str]:
    out = []
    for line in lines:
        e = parse_line(line)
        if e and e.size and e.end == address:
            out.append("%s (size:0x%X)" % (e.name, e.size))
    return out


# --- the map ------------------------------------------------------------------------------------------------

def infer_section(rows: Iterable[tuple[str, int]], address: int, section: str | None = None) -> str:
    """The section `address` belongs to when none is named, from `(section, address)` rows: the section whose
    row extent contains it (the narrowest), else the one with a row nearest to it. An explicit `section` is
    never second-guessed; no rows means `.text`."""
    if section:
        return section
    extents: dict[str, list[int]] = {}
    for sec, addr in rows:
        r = extents.get(sec)
        if r is None:
            extents[sec] = [addr, addr]
        else:
            r[0] = min(r[0], addr)
            r[1] = max(r[1], addr)
    if not extents:
        return ".text"
    containing = [s for s, (lo, hi) in extents.items() if lo <= address <= hi]
    if containing:
        return min(containing, key=lambda s: extents[s][1] - extents[s][0])
    return min(extents, key=lambda s: min(abs(address - extents[s][0]), abs(address - extents[s][1])))


@dataclass(frozen=True)
class CheckResult:
    """`SymbolMap.check()`: names defined twice, lines that do not parse, and the alias-group count."""
    symbols: int
    duplicates: tuple[tuple[str, tuple[int, ...]], ...]
    unparsed: tuple[tuple[int, str], ...]
    aliases: int

    @property
    def ok(self) -> bool:
        return not self.duplicates and not self.unparsed


class SymbolMap:
    """A symbol map file, read row by row. No method returns the file's text: rows, lookups and plans only
    (non-negotiable 7 made structural). Lookups that need every row build their index once per instance."""

    def __init__(self, path: str | os.PathLike) -> None:
        self.path = str(path)
        self._rows: list[Symbol] | None = None

    def rows(self) -> Iterator[Symbol]:
        """Every parsed row, in file order (streamed on first use, then served from the instance's cache)."""
        if self._rows is not None:
            yield from self._rows
            return
        rows: list[Symbol] = []
        with open(self.path, "r", encoding="utf-8", errors="replace", newline="") as fh:
            for i, line in enumerate(fh, 1):
                e = parse_line(line.rstrip("\r\n"), i)
                if e:
                    rows.append(e)
                    yield e
        self._rows = rows

    def _all(self) -> list[Symbol]:
        if self._rows is None:
            for _ in self.rows():
                pass
        return self._rows or []

    def by_name(self) -> dict[str, list[Symbol]]:
        """`name -> [rows]` (a name with two rows is a defect `check()` reports)."""
        out: dict[str, list[Symbol]] = {}
        for e in self._all():
            out.setdefault(e.name, []).append(e)
        return out

    def by_section(self) -> dict[str, list[Symbol]]:
        """`section -> rows sorted by address`."""
        out: dict[str, list[Symbol]] = {}
        for e in self._all():
            out.setdefault(e.section, []).append(e)
        for rows in out.values():
            rows.sort(key=lambda e: e.address)
        return out

    def names(self) -> set[str]:
        return {e.name for e in self._all()}

    def find(self, pattern: str, section: str | None = None, type: str | None = None) -> list[Symbol]:
        """Rows whose name matches the regex `pattern` (and the section/type when given), file order."""
        rx = re.compile(pattern)
        return [e for e in self._all() if rx.search(e.name) and (not section or e.section == section)
                and (not type or e.type == type)]

    def in_range(self, lo: int, hi: int, section: str | None = None) -> list[Symbol]:
        """Rows with `lo <= address < hi` (in `section` when given), sorted by address."""
        hits = [e for e in self._all() if lo <= e.address < hi and (not section or e.section == section)]
        return sorted(hits, key=lambda e: e.address)

    def infer_section(self, address: int, section: str | None = None) -> str:
        """`infer_section` over this map's rows."""
        return infer_section(((e.section, e.address) for e in self._all()), address, section)

    def at(self, address: int, count: int = 6, section: str | None = None) -> list[Symbol]:
        """The rows around `address` in its section: `count` before the row at or below it, `count` after."""
        sec = self.infer_section(address, section)
        rows = sorted((e for e in self._all() if e.section == sec), key=lambda e: e.address)
        idx = [i for i, e in enumerate(rows) if e.address <= address]
        centre = idx[-1] if idx else 0
        return rows[max(0, centre - count):centre + count + 1]

    def check(self) -> CheckResult:
        """Duplicate names, unparsed lines (blank and `#`/`//` lines excepted) and the alias-group count."""
        seen_name: dict[str, list[int]] = {}
        seen_addr: dict[tuple[str, int], int] = {}
        unparsed = []
        with open(self.path, "r", encoding="utf-8", errors="replace", newline="") as fh:
            for i, line in enumerate(fh, 1):
                e = parse_line(line.rstrip("\r\n"), i)
                if e:
                    seen_name.setdefault(e.name, []).append(i)
                    seen_addr[(e.section, e.address)] = seen_addr.get((e.section, e.address), 0) + 1
                elif line.strip() and not line.startswith(("#", "//")):
                    unparsed.append((i, line.strip()[:100]))
        dups = tuple((n, tuple(ls)) for n, ls in seen_name.items() if len(ls) > 1)
        return CheckResult(len(seen_name), dups, tuple(unparsed), sum(1 for v in seen_addr.values() if v > 1))

    # --- planned edits ---------------------------------------------------------------------------------
    def _lines(self) -> tuple[str, str, list[str]]:
        text = read_text(self.path)
        nl = line_ending(text)
        return text, nl, text.split(nl)

    def plan_rename(self, pairs: Iterable[tuple[str, str]], force: bool = False) -> RenamePlan:
        """Plan a batch of renames; nothing is written and every gate runs here (`Refused`/`ShapeError`).

        A pair already applied (`old` absent, `new` defined) is the idempotent no-op; `old` and `new` both
        absent is a typo and refused; so is a taken `new` (unless `force`), an invalid name, a name defined
        more than once, a name twice in the batch, and a rename that leaves one name at two addresses.
        """
        text, nl, lines = self._lines()
        path = self.path
        defined = _definitions(lines)
        changed, applied, used = [], [], set()
        for old, new in pairs:
            if old == new:
                applied.append((old, new))
                continue
            hits = defined.get(old, [])
            if not hits:
                if new in defined:
                    applied.append((old, new))
                    continue
                raise Refused("refusing: %s is not defined in %s" % (old, path))
            if new in defined and not force:
                raise Refused("refusing: %s is already defined in %s (use --force)" % (new, path))
            if not VALID_NAME_RE.fullmatch(new):
                raise Refused("refusing: %s is not a valid symbol name" % new)
            if len(hits) != 1:
                raise Refused("refusing: %s is defined %d times" % (old, len(hits)))
            i = hits[0]
            if i in used:
                raise Refused("refusing: %s appears twice in the batch" % old)
            if lines[i] not in text:
                raise ShapeError("refusing: the %s definition line is not in %s" % (old, path))
            changed.append((old, new, i, rewrite_name(lines[i], old, new)))
            used.add(i)
        if changed:
            planned = list(lines)
            for _old, _new, i, new_line in changed:
                planned[i] = new_line
            dupes = _names_at_two_addresses(planned, {new for _old, new, _i, _l in changed})
            if dupes:
                name = sorted(dupes)[0]
                (s1, a1), (s2, a2) = dupes[name][0], dupes[name][1]
                raise Refused("refusing: %s would be defined at two addresses (%s:0x%08X, %s:0x%08X)"
                              % (name, s1, a1, s2, a2))
        return RenamePlan(path, text, nl, tuple(lines), tuple(changed), tuple(applied))

    def plan_merge(self, rows: Iterable[tuple[str, str, int]],
                   scan_refs: Callable[[list[str]], dict] | None = None) -> MergePlan:
        """Plan a batch of merges `(phantom, previous, new_size)`; nothing is written.

        Functions (the phantom epilogue): refused unless both are defined once, in one section, the previous
        ending exactly at the phantom, no other name at the phantom's address, the size being the two sizes
        added, both `type:function` with one scope, and `scan_refs` (when given) finding no reference to the
        phantom. Data (`type:object` both): the stray label may also sit INSIDE the object (`previous.address <
        phantom.address <= previous.end`), the size is the union's (`max(ends) - previous.address`), no other symbol
        may start inside it, and the scopes may differ (a dtk label carries none). `phantom=None` is a plain data
        resize of `previous`: refused when a function, or when another symbol of its section starts inside the new
        extent. One row per object per batch. An absent phantom whose previous already has `new_size` is the re-apply; with the old size it is a
        half-applied plan.
        """
        rows = list(rows)
        text, nl, lines = self._lines()
        path = self.path
        defined = _definitions(lines)
        by_addr: dict[tuple[str, int], list[str]] = {}
        for line in lines:
            e = parse_line(line)
            if e:
                by_addr.setdefault((e.section, e.address), []).append(e.name)
        refs = scan_refs([r[0] for r in rows]) if scan_refs else {}
        grown: dict[int, str] = {}
        deleted: list[int] = []
        applied: list[tuple[str, str, int]] = []
        used: set[str] = set()
        for phantom, previous, new_size in rows:
            if phantom is None:
                self._plan_resize(lines, defined, by_addr, previous, new_size, used, grown, applied)
                continue
            if phantom == previous:
                raise Refused("refusing: %s is its own previous symbol" % phantom)
            for name in (phantom, previous):
                if name in used:
                    raise Refused("refusing: %s appears twice in the batch" % name)
                used.add(name)
            ph_hits, pv_hits = defined.get(phantom, []), defined.get(previous, [])
            if not ph_hits:
                if not pv_hits:
                    raise Refused("refusing: neither %s nor %s is defined in %s" % (phantom, previous, path))
                if len(pv_hits) != 1:
                    raise Refused("refusing: %s is defined %d times" % (previous, len(pv_hits)))
                e = parse_line(lines[pv_hits[0]])
                if e is not None and e.size == new_size:
                    applied.append((phantom, previous, new_size))
                    continue
                raise Refused("refusing: %s is absent but %s has size:0x%X, not the planned 0x%X"
                              % (phantom, previous, e.size if e else 0, new_size))
            if len(ph_hits) != 1:
                raise Refused("refusing: %s is defined %d times" % (phantom, len(ph_hits)))
            ph = parse_line(lines[ph_hits[0]])
            if not pv_hits:
                cands = _ending_at(lines, ph.address)
                hint = ("; the symbol ending at 0x%08X is %s - the plan may be stale after a rename"
                        % (ph.address, ", ".join(cands[:3]))) if cands else ""
                raise Refused("refusing: %s is not defined in %s%s" % (previous, path, hint))
            if len(pv_hits) != 1:
                raise Refused("refusing: %s is defined %d times" % (previous, len(pv_hits)))
            pv = parse_line(lines[pv_hits[0]])
            if ph.section != pv.section:
                raise Refused("refusing: %s is in %s but %s is in %s"
                              % (phantom, ph.section, previous, pv.section))
            data = ph.type == "object" and pv.type == "object"
            if data:
                if not pv.address < ph.address <= pv.end:
                    raise Refused("refusing: %s (0x%08X) is neither inside %s (0x%08X-0x%08X) nor at its end"
                                  % (phantom, ph.address, previous, pv.address, pv.end))
                union = max(pv.end, ph.end) - pv.address
                if new_size != union:
                    raise Refused("refusing: 0x%X (the plan) is not 0x%X, the extent of %s with %s folded in"
                                  % (new_size, union, previous, phantom))
                inside = sorted(n for (sec, addr), names in by_addr.items() if sec == pv.section
                                and pv.address < addr < pv.address + new_size for n in names
                                if n not in (phantom, previous))
                if inside:
                    raise Refused("refusing: %s with %s folded in would also cover %s - fold that one first"
                                  % (previous, phantom, ", ".join(inside[:3])))
            else:
                if pv.end != ph.address:
                    raise Refused("refusing: %s ends at 0x%08X, not at %s's 0x%08X"
                                  % (previous, pv.end, phantom, ph.address))
                if pv.size + ph.size != new_size:
                    raise Refused("refusing: 0x%X (the plan) is not 0x%X + 0x%X (%s + %s)"
                                  % (new_size, pv.size, ph.size, previous, phantom))
                if ph.size <= 0:
                    raise Refused("refusing: %s has no size to merge" % phantom)
                if ph.type != "function" or pv.type != "function":
                    raise Refused("refusing: %s and %s are not both type:function (or both type:object)"
                                  % (previous, phantom))
                if scope_of(lines[ph_hits[0]]) != scope_of(lines[pv_hits[0]]):
                    raise Refused("refusing: %s and %s disagree on scope" % (previous, phantom))
            aliases = [n for n in by_addr.get((ph.section, ph.address), []) if n != phantom]
            if aliases:
                raise Refused("refusing: %s shares its address with %s" % (phantom, ", ".join(sorted(aliases)[:3])))
            hits = refs.get(phantom) or []
            if hits:
                rel, lineno, txt = hits[0]
                raise Refused("refusing: %s is referenced at %s:%d (%s)" % (phantom, rel, lineno, txt))
            if ph_hits[0] in grown:
                raise Refused("refusing: %s is another row's grown symbol" % phantom)
            # a label folded inside its object leaves the object's size as it is: only the label goes
            grown[pv_hits[0]] = (lines[pv_hits[0]] if new_size == pv.size
                                 else resize(lines[pv_hits[0]], previous, new_size))
            deleted.append(ph_hits[0])
        if set(deleted) & set(grown):
            raise Refused("refusing: a line is both grown and deleted")
        return MergePlan(path, text, nl, tuple(lines), grown, tuple(deleted), tuple(applied))

    def _plan_resize(self, lines, defined, by_addr, name, new_size, used, grown, applied) -> None:
        """One `plan_merge` resize row: set data object `name`'s size to `new_size` (see `plan_merge`)."""
        if name in used:
            raise Refused("refusing: %s appears twice in the batch" % name)
        used.add(name)
        hits = defined.get(name, [])
        if len(hits) != 1:
            raise Refused("refusing: %s is defined %d times" % (name, len(hits)))
        e = parse_line(lines[hits[0]])
        if e.type != "object":
            raise Refused("refusing: %s is type:%s - only a data object is resized (a function grows by a merge)"
                          % (name, e.type or "?"))
        if e.size == new_size:
            applied.append((None, name, new_size))
            return
        inside = sorted(n for (sec, addr), names in by_addr.items() if sec == e.section
                        and e.address < addr < e.address + new_size for n in names)
        if inside:
            raise Refused("refusing: 0x%X would cover %s - fold it first (`merge <label> <object> <size>`), or pick a smaller size"
                          % (new_size, ", ".join(inside[:3])))
        grown[hits[0]] = resize(lines[hits[0]], name, new_size)

    def apply(self, plan: RenamePlan | MergePlan, write: Callable[[str, str], None] | None = None) -> bool:
        """Write `plan` once (through `write(path, text)`, default `write_text`); False when it is a no-op."""
        if isinstance(plan, RenamePlan) and not plan.changed:
            return False
        if isinstance(plan, MergePlan) and not plan.changed:
            return False
        (write or write_text)(plan.path, plan.render())
        self._rows = None
        return True


def read(path: str | os.PathLike) -> SymbolMap:
    """`SymbolMap(path)`."""
    return SymbolMap(path)
