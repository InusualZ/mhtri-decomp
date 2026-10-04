"""The landing path's union of a registration conflict (by `unionprose`'s rule) and the four invariants a wrong
union breaks silently (`check_union`). Spec: docs/tools/spec/merge.md. CLI: none (module)."""
from __future__ import annotations

import collections

from tools.lib import project as _project
from tools.units.merge.unionprose import union_markers

#: The only two paths a registration append-conflict may touch (a header or `src/**` conflict is another class).
UNION_SCOPE = ("configure.py", "config/RMHE08/splits.txt")

#: The sections a merged map must never overlap in (where adjacent bands meet and a bad union shows first).
OVERLAP_SECTIONS = (".text", "extab", "extabindex")


def union_text_full(text: str, path: str = "") -> tuple[str, int, list[dict]]:
    """`union_markers` under the landing path's name: a `blocked` decision must be refused, never written."""
    return union_markers(text, path)


def union_text(text: str) -> tuple[str, int]:
    """`union_text_full` without the decisions: `(merged, hunks_resolved)`."""
    merged, hunks, _decisions = union_text_full(text)
    return merged, hunks


def union_file(path: str) -> int:
    """Union-resolve `path` in place; the number of hunks resolved (0 = no markers, or a refused file).

    A `blocked` hunk is not written: the file is left alone and a `REFUSED` line is printed. Every non-code
    hunk is reported so which rule fired is visible.
    """
    with open(path, encoding="utf-8", newline="") as fh:
        text = fh.read()
    merged, hunks, decisions = union_text_full(text, path)
    if hunks:
        for d in decisions:
            if d["class"] != "code":
                tag = "BLOCKED" if d.get("blocked") else "took %s" % d["took"]
                print("    %-32s hunk %d %-5s %-9s %s" % (path, d["hunk"], d["class"], tag, d["why"]))
    if hunks and any(d.get("blocked") for d in decisions):
        print("    %-32s REFUSED - a prose hunk has no superset; resolve it by hand" % path)
        return 0
    if hunks:
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(merged)
    return hunks


# --- the invariants a wrong union breaks silently ---------------------------------------------------------------

def split_units(text: str) -> list[str]:
    """The unit keys of a `splits.txt` text, in file order (the order is the address order)."""
    return _project.Splits.parse(text).units


def split_ranges(text: str) -> list[tuple[str, str, int, int]]:
    """`(unit, section, start, end)` for every range in a `splits.txt` text."""
    return [(r.unit, r.section, r.start, r.end) for r in _project.Splits.parse(text).ranges]


def configure_objects(text: str) -> list[str]:
    """Every one-line `Object(kind, "unit")` (no options) as a normalised string, in file order."""
    return [c.normalised() for c in _project.object_calls(text) if c.closed]


def object_names(text: str) -> list[str]:
    """Every unit name a `configure.py` text registers via a one-line `Object(kind, "unit")`, in file order."""
    return [c.path for c in _project.object_calls(text) if c.closed]


def duplicate_unit_keys(units: list[str]) -> list[str]:
    """The unit keys a merged `splits.txt` names more than once."""
    return sorted(unit for unit, n in collections.Counter(units).items() if n > 1)


def duplicate_objects(objects: list[str]) -> list[str]:
    """The `Object()` lines a merged `configure.py` names more than once."""
    return sorted(obj for obj, n in collections.Counter(objects).items() if n > 1)


def overlapping_ranges(rows: list[tuple[str, str, int, int]],
                       sections: tuple[str, ...] = OVERLAP_SECTIONS) -> list[tuple]:
    """Every overlapping pair in a named section: `(section, unit_a, sa, ea, unit_b, sb, eb)` (half-open ranges:
    touching is the seam, only `next.start < previous.end` is a defect)."""
    by_section: dict[str, list[tuple[int, int, str]]] = {}
    for unit, section, start, end in rows:
        if section in sections:
            by_section.setdefault(section, []).append((start, end, unit))
    out: list[tuple] = []
    for section, spans in by_section.items():
        spans.sort()
        for k in range(1, len(spans)):
            prev_start, prev_end, prev_unit = spans[k - 1]
            start, end, unit = spans[k]
            if start < prev_end:
                out.append((section, prev_unit, prev_start, prev_end, unit, start, end))
    return out


def check_union(main_splits: str, merged_splits: str,
                main_configure: str, merged_configure: str) -> list[str]:
    """Every silent breakage of a wrong union, as readable lines (empty = the union is sound), judged against
    main's own text: a duplicate unit key or `Object()` line, an overlap, a registration of main's dropped."""
    problems: list[str] = []
    for unit in duplicate_unit_keys(split_units(merged_splits)):
        problems.append("duplicate unit key in the merged splits.txt: %s" % unit)
    for obj in duplicate_objects(configure_objects(merged_configure)):
        problems.append("duplicate Object() line in the merged configure.py: %s" % obj)
    for section, ua, sa, ea, ub, sb, eb in overlapping_ranges(split_ranges(merged_splits)):
        problems.append("overlapping %s range in the merged splits.txt: %s 0x%X-0x%X and %s 0x%X-0x%X"
                        % (section, ua, sa, ea, ub, sb, eb))
    merged_units = set(split_units(merged_splits))
    for unit in split_units(main_splits):
        if unit not in merged_units:
            problems.append("a unit main registered is missing from the merged splits.txt: %s" % unit)
    merged_objects = set(configure_objects(merged_configure))
    for obj in configure_objects(main_configure):
        if obj not in merged_objects:
            problems.append("a unit main registered is missing from the merged configure.py: %s" % obj)
    return problems
