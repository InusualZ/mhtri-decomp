#!/usr/bin/env python3
"""Side-by-side instruction diff for one symbol of a unit.

Usage:
    python tools/objdiff/symdiff.py -u <unit>                             # list every symbol + score
    python tools/objdiff/symdiff.py -u <unit> <symbol> [n] [--all]        # runs objdiff for you
    python tools/objdiff/symdiff.py <diff.json> <symbol> [n] [--all]      # reuse an existing diff
    python tools/objdiff/symdiff.py -u <unit> --force-stale               # score a stale object anyway

`-u <unit>` scores the unit's **prebuilt** object (`build/RMHE08/src/<unit>.o`), so it refuses to print
numbers when a source or header under the unit is newer than that object (exit 1, naming the newer
file): a lane that measured a stale object reported two "improvements" that were never built. The rule
and its arithmetic live in `tools/objdiff/freshguard.py`, shared with `unitscore.py`; `--force-stale`
scores it anyway and says so on stderr.

A bare `-u <unit>` is the **first measurement of a unit in one command**: it lists every symbol the
unit owns with its official report score, worst first. It used to raise an IndexError traceback
(measured 2026-09-27: a lane lost its first turn to it), and the scores are the same
`fuzzy_match_percent` the single-symbol path prints, so the listing and the diff never disagree.

Left  = the -1 object, Right = the -2 object (file mode), or target/base in project mode
(`-p . -u <unit>`, where left = target and right = our build).

The `match` figure printed with `-u` is the **official report metric** (`report generate`'s
`fuzzy_match_percent` - what `build/RMHE08/report.json`, `ledger.py` and `land.py` read), with the
positional `diff` value shown beside it when it differs. A pre-existing `diff.json` has no unit context,
so that path prints the positional value and says so: objdiff's `match_percent` is positional (one
inserted/deleted instruction shifts every later instruction, so a single early divergence can report
~0 %) and its relocation default differs from the report's. Read the first divergence, not the percentage.

Every run writes its project, report and diff JSON under a **unique** temp directory
(`session_tmpdir()`), removed at exit.  The shared `build/tmp/unitutil/unitutil_report.json` was held by
another process twice and raised `PermissionError [WinError 5]`, costing a measurement round (2026-09-28);
a unique directory (with a transient-lock retry) removes the collision.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import atexit
import json
import os
import shutil
import sys
import tempfile
import time
from tools.lib import repo as librepo

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))  # tools/
_HERE = os.path.dirname(os.path.abspath(__file__))                 # tools/objdiff (freshguard lives here)
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
import unitutil as uu
import freshguard


def session_tmpdir() -> str:
    return librepo.session_tmpdir()


def stale_reasons(unit, root: str | None = None) -> list[str]:
    """Why scoring `unit`'s **prebuilt** object would report numbers that are not this build's.

    `-u <unit>` in both shapes scores `build/RMHE08/src/<unit>.o` as it sits on disk - one
    `report generate` (the listing) or one `report measure` (the per-symbol score). If a source or
    header under the unit is newer than that object, the score describes a build that no longer exists:
    a lane measured exactly that, twice, and reported two "improvements" that were never compiled. This
    is the same rule `unitscore.py` enforces (`tools/objdiff/freshguard.py` holds the arithmetic), applied
    to the tool that scores without one. `root` is the tree the include closure is resolved in (default
    `unitutil.ROOT`); the selftest passes a fixture tree.
    """
    root = root or uu.ROOT
    src = unit.src if os.path.isabs(unit.src) else os.path.join(root, unit.src)
    reasons, _newest = freshguard.unit_reasons(src, unit.obj, root,
                                              rel=lambda p: freshguard.rel_path(p, root))
    return reasons


def retry_transient(fn, attempts: int = 4):
    """Call `fn()`, retrying the transient Windows sharing violation (WinError 5) with backoff.

    The unique directory already removes the collision; this covers a transient antivirus/indexer lock
    on the file the caller just created, which `os.remove`/`open` can still raise once.
    """
    delay = 0.05
    last: PermissionError | None = None
    for i in range(attempts):
        try:
            return fn()
        except PermissionError as exc:
            last = exc
            if i + 1 < attempts:
                time.sleep(delay)
                delay *= 2
    raise (last if last else PermissionError("transient file lock"))


def cli():
    """(diff json path, symbol, unit), from `-u <unit> [<symbol>]` or `<diff.json> <symbol>`.

    A bare `-u <unit>` returns `(None, None, unit)`: the caller lists the unit's symbols instead of
    diffing one (see `list_symbols`).  Returning that shape - rather than indexing `rest[0]` - is what
    retires the IndexError a lane hit on its first measurement.
    """
    a = sys.argv[1:]
    for flag in ("-u", "--unit"):
        if flag in a:
            i = a.index(flag)
            if i + 1 >= len(a):
                raise SystemExit("usage: symdiff.py -u <unit> [<symbol>] [n] [--all]"
                                 " | symdiff.py <diff.json> <symbol> [n] [--all]")
            rest = a[:i] + a[i + 2:]
            unit = uu.resolve_unit(a[i + 1])
            if not rest or rest[0] == "--all":
                return None, None, unit
            symbol = rest[0]
            path, log = uu.objdiff(unit, symbol, out=os.path.join(session_tmpdir(), "diff.json"))
            if not path:
                raise SystemExit("objdiff failed: " + log)
            sys.argv = [sys.argv[0], path, symbol] + rest[1:]
            return path, symbol, unit
    if len(a) < 2:
        raise SystemExit("usage: symdiff.py -u <unit> [<symbol>] [n] [--all]"
                         " | symdiff.py <diff.json> <symbol> [n] [--all]")
    return a[0], a[1], None


def list_symbols(unit) -> int:
    """Every symbol `unit` owns with its official report score, worst first - the one-command recon.

    The metric is `report generate`'s `fuzzy_match_percent` (the same one `-u <unit> <symbol>` prints and
    `report.json` carries), never a fabricated 0.0: a target object that does not exist yet is an error
    that names the path and says why, so a proposal unit's first run is not read as "everything 0 %".
    """
    entries = retry_transient(lambda: uu.report_functions(unit.target, unit.obj, unit.name,
                                                          tmpdir=session_tmpdir()))
    if "_error" in entries:
        raise SystemExit(
            "cannot score %s: %s\n  target object: %s\n"
            "  (a proposal unit has no split object until its registration lands; check the target exists "
            "and the tree is built)" % (unit.name, entries["_error"], unit.target))
    rows = sorted(entries.values(),
                  key=lambda e: (e.get("fuzzy_match_percent")
                                 if e.get("fuzzy_match_percent") is not None else 0.0,
                                 e.get("name") or ""))
    print("== %s: %d symbol(s), report metric (fuzzy_match_percent), worst first" % (unit.name, len(rows)))
    print("%-44s %9s %9s  %s" % ("symbol", "size B", "match %", ""))
    for e in rows:
        pct = e.get("fuzzy_match_percent")
        size = e.get("size")
        label = "n/a" if pct is None else "%.5f" % pct
        flag = "" if (pct is not None and pct >= 100.0) else "  <- open"
        print("%-44s %9s %9s%s" % (e.get("name"), size if size is not None else "?", label, flag))
    return 0


def norm(d, side):
    """Sections dict with symbols normalized to the old nested shape.

    objdiff-cli v3.6.1 (pinned in configure.py) emits a flat top-level `symbols[]`
    array and only when a symbol argument was passed; older builds nested the
    entries under `sections[].symbols[]` with a `symbol` object.
    """
    syms = d[side].get("symbols")
    if not syms:
        return {s["name"]: s for s in d[side]["sections"]}
    out = {s["name"]: dict(s) for s in d[side]["sections"]}
    out.setdefault(".text", {"name": ".text"})["symbols"] = [
        {"symbol": {"name": e.get("name"), "size": e.get("size")},
         "instructions": e.get("instructions") or [],
         "match_percent": e.get("match_percent")} for e in syms]
    return out


def load(path):
    d = json.load(open(path))
    return norm(d, "left"), norm(d, "right")


def find(section, name):
    for e in section.get("symbols", []):
        if e["symbol"].get("name") == name:
            return e
    return None


def fmt(ins):
    if ins is None:
        return "--"
    i = ins.get("instruction")
    return i["formatted"] if i else "<" + str(ins.get("diff_kind", "?")) + ">"


def kind(ins):
    if ins is None:
        return "MISSING"
    if "instruction" not in ins:
        return "PH"
    k = ins.get("diff_kind")
    if k == "DIFF_ARG_MISMATCH":
        return "ARG"  # opcode matches, only an argument differs
    if k in (None, "DIFF_NONE"):
        return "=="
    return k.replace("DIFF_", "")


def official_match(unit, name):
    """The report metric for `name`, or None when it cannot be obtained (never a fabricated score)."""
    if unit is None:
        return None
    m = retry_transient(lambda: uu.report_measure(unit.target, unit.obj, name, unit.name,
                                                  tmpdir=session_tmpdir()))
    if "error" in m:
        return None
    return m.get("match_percent")


def main():
    argv = sys.argv[1:]
    force = "--force-stale" in argv
    if force:
        sys.argv = [sys.argv[0]] + [a for a in argv if a != "--force-stale"]
    path, name, unit = cli()
    if unit is not None:
        reasons = stale_reasons(unit)
        if reasons:
            print("freshness  STALE%s" % (" (forced)" if force else ""), file=sys.stderr)
            for reason in reasons:
                print("             - " + reason, file=sys.stderr)
            if not force:
                print("refused    a stale object would print numbers that are not this build's; nothing "
                      "shown.\n           rebuild (`ninja %s`) or pass --force-stale to score it anyway "
                      "(the verdict stays)." % unit.name, file=sys.stderr)
                return 1
            print("freshness  (forced by --force-stale)", file=sys.stderr)
    if name is None and unit is not None:
        return list_symbols(unit)
    n = int(sys.argv[3]) if len(sys.argv) > 3 and sys.argv[3].isdigit() else 30
    show_all = "--all" in sys.argv
    left, right = load(path)
    official = official_match(unit, name)
    for sec in (".text", ".rodata", ".data"):
        if sec not in left:
            continue
        le = find(left[sec], name)
        if not le:
            continue
        re_ = find(right.get(sec, {}), name)
        li = le.get("instructions") or []
        ri = (re_.get("instructions") or []) if re_ else []
        pos = le.get("match_percent")
        if official is None:
            match = "%s (positional diff - not the report metric; pass -u <unit> for the official score)" % pos
        elif official == pos or pos is None:
            match = "%s (report metric)" % official
        else:
            match = "%s (report metric; objdiff positional diff says %s)" % (official, pos)
        print(f"== {sec} {name}: target {le['symbol'].get('size')} B / {len(li)} ins, "
              f"ours {re_['symbol'].get('size') if re_ else '?'} B / {len(ri)} ins, "
              f"match {match}")
        print(f"{'i':>5} {'T':>4} {'kind':<12} {'target':<40} | {'ours':<40} ours-kind")
        limit = max(len(li), len(ri)) if show_all else min(n, max(len(li), len(ri)))
        for i in range(limit):
            l = li[i] if i < len(li) else None
            r = ri[i] if i < len(ri) else None
            print(f"{i:>5} {kind(l):>4} {kind(l):<12} {fmt(l):<40} | {fmt(r):<40} {kind(r)}")
        return
    print(f"symbol {name!r} not found")


if __name__ == "__main__":
    # `sys.exit(main())`: the freshness refusal returns 1, and until this was added the process exited
    # 0 regardless, so a refusal was visible only to a human reading stderr.
    sys.exit(main())
