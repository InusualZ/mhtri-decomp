#!/usr/bin/env python3
"""Is a prebuilt object (or the project report) older than the unit it is supposed to describe?

One implementation of the campaign's staleness rule, shared by every scorer that reads a **prebuilt**
artefact instead of compiling one:

    a tool that reports a score must refuse (or refuse loudly) when the object's mtime predates any
    source in that unit's include closure, and say which file is newer.

**The incident this is built around.** `build/RMHE08/report.json` is an **order-only** target of
`all_source` in `build.ninja`: after a source edit, `ninja build/RMHE08/report.json` prints "no work to
do" and the file still holds the PREVIOUS build's scores. A lane measured a stale object twice and
reported two "improvements" that were never built. `unitscore.py` was the first tool to refuse a stale
report (report mode) or a stale object (`--measure`); `symdiff.py -u <unit>` scored the prebuilt object
pair with no guard at all, which is the same lie under a different name.

**What "a unit's sources" means.** The unit's source file *and every in-tree header it reaches,
transitively*: a header is a build input too, so a lane that edited `include/unsplit/Pl.h` and did not
run ninja has an object as stale as one whose `.cpp` moved. System headers (`<string.h>`) are resolved
only inside the tree, so they never date a unit.

**Strict `<` on purpose.** `report.json` is written in the same whole second as the last object ninja
rebuilt (measured: report 07:21:39 / newest object 07:21:39 in a settled tree), so an equal stamp is
current and a report one second older than any input is not.

The functions here are pure file/mtime arithmetic - no objdiff, no unit resolution, no printing. Callers
own the wording and the exit code; `reasons` is a list of strings, `[]` meaning "current".
"""
from __future__ import annotations

import os
import re
import time

# a one-line `#include "x.h"` / `#include <x.h>`; comments are stripped first so a commented-out one is
# not counted as a dependency.
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
MAX_INCLUDE_DEPTH = 12


def mtime(path: str) -> float | None:
    """The file's mtime, or None when it does not exist (never an exception)."""
    try:
        return os.path.getmtime(path)
    except OSError:
        return None


def stamp(t: float | None) -> str:
    """A local timestamp for a printed report, `-` for a missing file."""
    return "-" if t is None else time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(t))


def stamp_json(t: float | None):
    """The same instant machine-readably: seconds since the epoch, or None."""
    return None if t is None else round(t, 3)


def freshness(use_report: bool, report_mtime: float | None, object_mtime: float | None,
              newest_source: tuple[str, float] | None, report_path: str, object_path: str,
              rel=lambda p: p) -> list[str]:
    """The reasons the numbers would be stale, worst first - `[]` means the run is current.

    `rel` shortens the paths for the printed reason; the JSON carries the absolute ones in its own
    fields. In report mode the report is checked against both the object and the newest source (owning
    the order-only hazard); otherwise the object itself is the artefact under test.
    """
    reasons: list[str] = []
    if object_mtime is None:
        reasons.append("the unit's object does not exist (%s): nothing was built for it in this tree, so "
                       "the report's rows cannot be this tree's" % rel(object_path))
    if use_report:
        if report_mtime is None:
            reasons.append("the report does not exist (%s)" % rel(report_path))
        else:
            if object_mtime is not None and report_mtime < object_mtime:
                reasons.append(
                    "the report (%s, %s) predates the unit's object (%s, %s) - it was written before the "
                    "last build of this unit" % (rel(report_path), stamp(report_mtime), rel(object_path),
                                                 stamp(object_mtime)))
            if newest_source is not None and report_mtime < newest_source[1]:
                reasons.append(
                    "the report (%s, %s) predates the newest source under the unit (%s, %s) - it holds "
                    "the PREVIOUS build's scores (report.json is an order-only target of all_source)"
                    % (rel(report_path), stamp(report_mtime), rel(newest_source[0]),
                       stamp(newest_source[1])))
    else:
        if object_mtime is not None and newest_source is not None and object_mtime < newest_source[1]:
            reasons.append(
                "the object (%s, %s) predates the newest source under the unit (%s, %s) - it was not "
                "rebuilt after the edit" % (rel(object_path), stamp(object_mtime), rel(newest_source[0]),
                                            stamp(newest_source[1])))
    return reasons


# --------------------------------------------------------------------------------------------------
# the unit's sources: the file plus its include closure, so a header edit dates the object too
# --------------------------------------------------------------------------------------------------

def includes_of(path: str) -> list[str]:
    """The `#include` targets named in a source file, comments removed so a commented one is not read."""
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return []
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return INCLUDE_RE.findall(text)


def rel_path(path: str, tree: str) -> str:
    """A path as it is printed: relative to the tree, with forward slashes on every host."""
    try:
        out = os.path.relpath(path, tree)
    except ValueError:                                 # different drives
        out = path
    return out.replace("\\", "/")


def _inside(path: str, root: str) -> bool:
    """`/a/b` inside `/a` - the check `os.path.commonpath` needs to be asked, not assumed."""
    try:
        return os.path.commonpath([os.path.normcase(os.path.abspath(path)),
                                   os.path.normcase(os.path.abspath(root))]) == \
            os.path.normcase(os.path.abspath(root))
    except ValueError:                                    # different drives
        return False


def resolve_include(name: str, from_dir: str, root: str) -> str | None:
    """Where MWCC would find `name` from `from_dir`: beside the includer, then `include/`, then `src/`.

    Only paths inside `root` are returned: a system header (`<string.h>`) is not a source under the
    unit and must not date it.
    """
    name = name.replace("\\", "/")
    for base in (from_dir, os.path.join(root, "include"), os.path.join(root, "src")):
        cand = os.path.normpath(os.path.join(base, *name.split("/")))
        if os.path.isfile(cand) and _inside(cand, root):
            return cand
    return None


def source_closure(src: str, root: str) -> list[str]:
    """The unit's source file and every in-tree header it reaches, transitively (deduplicated).

    A header is part of the unit's inputs - the object is rebuilt when it changes - so a stale verdict
    that ignored headers would miss "the lane edited `include/unsplit/Pl.h` and never ran ninja".
    """
    out: list[str] = []
    seen: set[str] = set()
    todo: list[tuple[str, int]] = [(os.path.abspath(src), 0)]
    while todo:
        path, depth = todo.pop(0)
        key = os.path.normcase(os.path.abspath(path))
        if key in seen:
            continue
        seen.add(key)
        out.append(path)
        if depth >= MAX_INCLUDE_DEPTH:
            continue
        for name in includes_of(path):
            hit = resolve_include(name, os.path.dirname(path), root)
            if hit and os.path.normcase(os.path.abspath(hit)) not in seen:
                todo.append((hit, depth + 1))
    return out


def newest(paths: list[str]) -> tuple[str, float] | None:
    """The `(path, mtime)` of the most recently modified of `paths`, or None for an empty list."""
    best: tuple[str, float] | None = None
    for p in paths:
        t = mtime(p)
        if t is None:
            continue
        if best is None or t > best[1]:
            best = (p, t)
    return best


def unit_reasons(src: str, obj: str, root: str, rel=lambda p: p) -> tuple[list[str], tuple[str, float] | None]:
    """The stale reasons for a unit's **prebuilt object** against its include closure (`[]` = current).

    The one call a scorer that reads `build/RMHE08/src/<unit>.o` needs: `obj` is that object, `src` the
    unit's source file. Returns `(reasons, newest_source)` so a caller can print the newest input too.
    """
    sources = source_closure(src, root)
    newest_source = newest(sources)
    return freshness(False, None, mtime(obj), newest_source, "", obj, rel=rel), newest_source
