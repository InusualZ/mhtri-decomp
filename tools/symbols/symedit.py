#!/usr/bin/env python3
"""Query and surgically edit a dtk symbol map without loading it into context.

`config/RMHE08/symbols.txt` is ~65 700 lines / 4.5 MB and the per-module RSO maps add 4 462 more
lines.  Never paste those files into a prompt or read them whole - go through this script, which
prints only the lines you asked for and changes only the name token you asked it to change.

Usage (default file: config/RMHE08/symbols.txt, override with --file):

    symedit.py find   <regex> [--limit N] [--section .text] [--type function]
    symedit.py show   <name> [<name> ...]
    symedit.py at     <address> [--count N]          # symbols around an address, in order
    symedit.py range  <start> <end> [--section S]    # members of a split range
    symedit.py refs   <name> [--roots src include docs] [--code-only]
    symedit.py check                                 # duplicate names / addresses, bad lines
    symedit.py rename <old> <new> [--dry-run] [--force] [--no-refs]
    symedit.py rename-batch <file> [--dry-run]       # lines: "old new" (# comments allowed)
    symedit.py merge-batch <file> [--dry-run] [--no-refs]   # lines: "merge <phantom> <previous> <size>"
    symedit.py --selftest                            # the checks, against temp fixtures only

`rename` writes through `tools/units/sharedfiles.py` (docs/plan.md 7.12): the edit is a temp file +
`os.replace` transaction that restores the previous bytes exactly if it fails, and the map's own line
ending is preserved.  Before a byte is written it asserts the shape a rename depends on - the old name
is defined exactly once, on a line that parses as a map line, and the rewrite yields a line that parses
back as the new name.  Re-applying a rename that is already in the file is a no-op.  It refuses when
the new name is already taken (unless `--force`), when `--force` would still leave one name at two
addresses, and when the new name is not a valid symbol name; it warns about in-repo references to the
old name (a rename is always two edits: this file *and* the source - see CLAUDE.md -> Conventions ->
"Commenting and naming").

`merge-batch` is the other half of `tools/symbols/phantom.py` (docs/plan.md 7.9): a phantom is an
unnamed `fn_*` that is really the previous function's dead epilogue, so a merge grows the previous
symbol's `size:` and deletes the phantom's line.  Per row it refuses - before any write - unless both
symbols are defined exactly once, in the same section, the previous ends exactly at the phantom's
address, no other name sits at that address, the stated size is exactly the two sizes added, the two
scopes agree, and the phantom has no in-repo reference.  An already-merged row is a no-op.  When the
plan's previous name is stale (a rename landed after `phantom.py` ran), the refusal names the symbol
that actually ends at the phantom's address instead of guessing.
"""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import os
import re
import sys
from pathlib import Path

DEFAULT_FILE = "config/RMHE08/symbols.txt"
_UNITS = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "units")
_UNITS = os.path.normpath(_UNITS)
# `unitutil.py` (the invocation-tree resolver) lives one level up, beside `units/`
_TOOLS = os.path.normpath(os.path.join(_UNITS, os.pardir))
for _p in (_UNITS, _TOOLS):
    if _p not in sys.path:
        sys.path.insert(0, _p)

import sharedfiles as sf  # noqa: E402  the one writer for shared files - docs/plan.md 7.12
import unitutil as _uu  # noqa: E402  the invocation-tree resolver (`repo_root`)
from tools.lib.project import symbols as _sym  # noqa: E402  the one map parser and the rename/merge planner

# The tree a relative `--file` resolves against. `symedit` **writes** the map (rename/rename-batch), so a
# MAIN-hardcoded root means a lane that invoked MAIN's copy from its own worktree renamed symbols *in MAIN*
# - or, reading, refused to find a name the branch had just introduced. `unitutil.repo_root` is the
# invocation-first resolver the other tools use (`git rev-parse --show-toplevel`, falling back to this
# file's tree outside a worktree), so the map is the caller's tree's map.
REPO = _uu.repo_root()
LINE_RE = _sym.LINE_RE


def parse_line(line):
    """`lib.project.symbols.parse_line` as the dict this tool's importers read."""
    e = _sym.parse_line(line)
    return e.to_dict() if e else None


def entries(path):
    """Every map row as a dict (with `lineno`), streamed."""
    for e in _sym.SymbolMap(path).rows():
        yield e.to_dict()


def emit(items, as_json=False, limit=None):
    shown = items if limit is None else items[:limit]
    if as_json:
        import json
        print(json.dumps(shown, indent=2))
    else:
        for e in shown:
            print("%-40s %s:0x%08X  %s %s" % (e["name"], e["section"], e["address"], e["type"],
                                              ("size:0x%X" % e["size"]) if e["size"] else ""))
    if limit is not None and len(items) > limit:
        print("... (%d more, raise --limit)" % (len(items) - limit))


def cmd_find(a):
    emit([e.to_dict() for e in _sym.SymbolMap(a.file).find(a.pattern, a.section, a.type)], a.json, a.limit)


def cmd_show(a):
    want = set(a.names)
    hits = [e.to_dict() for e in _sym.SymbolMap(a.file).rows() if e.name in want]
    emit(hits, a.json)
    missing = want - {e["name"] for e in hits}
    if missing:
        print("not found: %s" % ", ".join(sorted(missing)), file=sys.stderr)
        return 1
    return 0


def infer_section(rows, want, section=None):
    """The section an `at` address belongs to when `--section` does not name one (`rows` are entry dicts)."""
    return _sym.infer_section(((e["section"], e["address"]) for e in rows), want, section)


def cmd_at(a):
    want = int(a.address, 0) if not re.fullmatch(r"[0-9a-fA-F]+", a.address) else int(a.address, 16)
    emit([e.to_dict() for e in _sym.SymbolMap(a.file).at(want, a.count, a.section)], a.json)
    return 0


def cmd_range(a):
    hits = _sym.SymbolMap(a.file).in_range(int(a.start, 0), int(a.end, 0), a.section)
    emit([e.to_dict() for e in hits], a.json, a.limit)
    return 0


REF_SUFFIXES = (".c", ".h", ".cpp", ".hpp", ".py", ".md", ".txt", ".yml", ".yaml")


def find_refs(names, roots, limit, exclude=None):
    """`name -> [(relpath, lineno, text)]` for every in-repo mention, in one walk of `roots`.

    The scan `rename` warns with and `merge-batch` refuses on: a name mentioned in the source is a
    caller a delete would break.  Bounded by `limit` hits per name, so the output cannot flood.
    """
    pats = {n: re.compile(r"\b%s\b" % re.escape(n)) for n in names}
    hits = {n: [] for n in names}
    for root in roots:
        base_root = os.path.join(REPO, root)
        if not os.path.isdir(base_root):
            continue
        for base, dirs, files in os.walk(base_root):
            dirs[:] = [d for d in dirs if d not in (".git", "build", "__pycache__")]
            for fn in files:
                if not fn.endswith(REF_SUFFIXES):
                    continue
                p = os.path.join(base, fn)
                if exclude and os.path.abspath(p) == os.path.abspath(exclude):
                    continue
                try:
                    with open(p, "r", encoding="utf-8", errors="replace") as fh:
                        for i, line in enumerate(fh, 1):
                            for name, pat in pats.items():
                                if len(hits[name]) < limit and pat.search(line):
                                    hits[name].append((os.path.relpath(p, REPO), i,
                                                       line.strip()[:160]))
                except OSError:
                    continue
    return hits


# A map symbol's name is ALSO a file name whenever a unit is registered under a generated path
# (`src/DWCi/fn_805113B0.c`, `include/Network/fn_803D3CE8.h`). Renaming the map row does NOT move that file -
# renaming a registered unit's file is a registration move, not a symbol rename - so a mention of the name
# *inside a path* must never be rewritten as an identifier: that breaks the `#include`, and the build is the
# only thing that would notice. `find_refs` matches with `\b`, and `/` and `.` are word boundaries, so a path
# IS reported as a "reference"; classifying the hits is what makes the list safe to act on.
SRC_SUFFIXES = (".c", ".h", ".cpp", ".hpp")


def ref_kinds(line, name):
    """The kinds of mention of `name` on `line` - a subset of {code, path, string, comment}."""
    kinds = set()
    for m in re.finditer(r"\b%s\b" % re.escape(name), line):
        before, after = line[:m.start()], line[m.end():]
        quoted = (before.count('"') % 2) or (before.count("'") % 2)
        commented = "//" in before or "/*" in before
        if (before.endswith(("/", "\\")) or after.startswith(("/", "\\"))
                or any(after.startswith(s) for s in SRC_SUFFIXES)):
            kinds.add("path")
        elif commented:
            kinds.add("comment")
        elif quoted:
            kinds.add("string")
        else:
            kinds.add("code")
    return kinds


def group_hits(name, hits):
    """`find_refs`' hits grouped for a reader who is about to rewrite them.

    `code` is the other half of a rename. `path` is a *file* name, not the symbol: rewriting it breaks the
    include. `mention` is a comment or a string literal - inert, and a rename may leave it alone. A hit whose
    name appears in a path is reported under `path`, which is the subset that does damage.
    """
    groups = {"code": [], "path": [], "mention": []}
    for hit in hits:
        kinds = ref_kinds(hit[2], name)
        if "code" in kinds:
            groups["code"].append(hit)
        if "path" in kinds:
            groups["path"].append(hit)
        elif kinds & {"string", "comment"}:
            groups["mention"].append(hit)
    return groups


REF_GROUPS = (
    ("code", "these are the other half of the rename"),
    ("path", "DO NOT rewrite inside a path - the FILE keeps its name; renaming a unit's file is a "
             "registration move"),
    ("mention", "inert (a comment or a string literal): a rename may leave these alone"),
)


def cmd_refs(a):
    hits = find_refs([a.name], a.roots, a.limit, a.file).get(a.name, [])
    if not hits:
        print("no references to %s outside %s" % (a.name, a.file))
        return 0
    groups = group_hits(a.name, hits)
    if a.code_only:
        for rel, lineno, text in groups["code"]:
            print("%s:%d: %s" % (rel, lineno, text))
        print("%d code reference(s) to %s" % (len(groups["code"]), a.name))
        return 0
    for label, note in REF_GROUPS:
        if not groups[label]:
            continue
        print("-- %s (%d): %s" % (label, len(groups[label]), note))
        for rel, lineno, text in groups[label]:
            print("   %s:%d: %s" % (rel, lineno, text))
    if groups["path"]:
        print("WARNING: %d mention(s) of %s are inside a PATH. A scripted word-boundary rewrite of the name "
              "would corrupt them (the file on disk keeps its name), and only a build would notice - use "
              "`--code-only` for the rewrite list." % (len(groups["path"]), a.name))
    if len(hits) >= a.limit:
        print("... (stopping at --limit %d)" % a.limit)
    return 0


def cmd_check(a):
    res = _sym.SymbolMap(a.file).check()
    bad = ["duplicate name %s at lines %s" % (name, list(lines)) for name, lines in res.duplicates]
    bad += ["unparsed line %d: %s" % (i, text) for i, text in res.unparsed]
    if bad:
        print("\n".join(bad[:a.limit]))
        print("%d problem(s), %d address alias group(s)" % (len(bad), res.aliases))
        return 1
    print("%s: %d symbols, no duplicate names, all lines parse (%d address alias group(s))"
          % (a.file, res.symbols, res.aliases))
    return 0


def _lib_call(fn, *args, **kwargs):
    """Run a `lib.project.symbols` planner with this tool's exit: a refusal is `SystemExit(message)` (a shape
    failure stays `ShapeError`, which is a `lib.text.AnchorError`)."""
    try:
        return fn(*args, **kwargs)
    except _sym.Refused as exc:
        raise SystemExit(str(exc))


def _rewrite_line(line, old, new):
    """The name token of one definition line, replaced - or `sf.AnchorError` if it is not that shape."""
    return _lib_call(_sym.rewrite_name, line, old, new)


def plan_rename(path, pairs, force=False):
    """Plan a batch of renames (no write): `(text, nl, lines, changed, applied)`, `changed` being
    `(old, new, index, new_line)` - `lib.project.symbols.SymbolMap.plan_rename` with this tool's exits."""
    plan = _lib_call(_sym.SymbolMap(path).plan_rename, pairs, force)
    return plan.text, plan.newline, list(plan.lines), list(plan.changed), list(plan.applied)


def _write_text(path, text, rename=None):
    """Write through the verified transaction (`lib.project.symbols.write_text`); a failure restores the bytes."""
    _sym.write_text(path, text, rename)


def apply_rename(path, nl, lines, changed):
    """Apply the planned edits and write once, in the file's own line ending."""
    plan = _sym.RenamePlan(str(path), "", nl, tuple(lines), tuple(changed), ())
    if plan.changed:
        _write_text(path, plan.render())


def _resize_line(line, name, new_size):
    """The `size:` field of one definition line, set to `new_size` - or `sf.AnchorError`."""
    return _lib_call(_sym.resize, line, name, new_size)


def plan_merge(path, rows, scan_refs=None):
    """Plan a batch of phantom merges (no write): `(text, nl, lines, grown, deleted, applied)`, `grown` being
    `index -> new line` - `lib.project.symbols.SymbolMap.plan_merge` with this tool's exits."""
    plan = _lib_call(_sym.SymbolMap(path).plan_merge, rows, scan_refs)
    return plan.text, plan.newline, list(plan.lines), dict(plan.grown), list(plan.deleted), list(plan.applied)


def apply_merge(path, nl, lines, grown, deleted):
    """Apply the planned growths and deletions and write once, in the file's own line ending."""
    plan = _sym.MergePlan(str(path), "", nl, tuple(lines), dict(grown), tuple(deleted))
    if plan.changed:
        _write_text(path, plan.render())


def merge(a, rows):
    if not rows:
        print("no merge rows in %s" % a.mapfile)
        return 0
    scan = None if a.no_refs else (lambda names: find_refs(names, a.roots, a.limit, a.file))
    _text, nl, lines, grown, deleted, applied = plan_merge(a.file, rows, scan)
    if not a.dry_run:
        apply_merge(a.file, nl, lines, grown, deleted)
    for _i, line in sorted(grown.items()):
        print("%s %s" % ("would grow" if a.dry_run else "grew", line))
    for i in sorted(deleted):
        print("%s %s" % ("would delete" if a.dry_run else "deleted", lines[i]))
    for phantom, previous, new_size in applied:
        print("no-op: %s -> %s already merged (size:0x%X)" % (phantom, previous, new_size))
    print("%s %d merged, %d deleted, %d already merged in %s"
          % ("dry-run:" if a.dry_run else "wrote", len(grown), len(deleted), len(applied), a.file))
    return 0


def read_merge_rows(mapfile):
    """Parse a `merge <phantom> <previous> <new_size_hex>` batch file (`#` comments allowed)."""
    rows = []
    with open(mapfile, "r", encoding="utf-8") as fh:
        for lineno, line in enumerate(fh, 1):
            line = line.split("#")[0].strip()
            if not line:
                continue
            tok = line.split()
            if len(tok) != 4 or tok[0].lower() != "merge":
                raise SystemExit("refusing: %s:%d is not 'merge <phantom> <previous> <size>': %s"
                                 % (mapfile, lineno, line[:80]))
            try:
                new_size = int(tok[3], 16)
            except ValueError:
                raise SystemExit("refusing: %s:%d has no hex size: %s" % (mapfile, lineno, tok[3]))
            if new_size <= 0:
                raise SystemExit("refusing: %s:%d has a non-positive size" % (mapfile, lineno))
            rows.append((tok[1], tok[2], new_size))
    return rows


def cmd_merge_batch(a):
    return merge(a, read_merge_rows(a.mapfile))


def rename(a, pairs):
    _text, nl, lines, changed, applied = plan_rename(a.file, pairs, a.force)
    if not a.dry_run:
        apply_rename(a.file, nl, lines, changed)
    for old, new, _i, line in changed:
        print("%s %s -> %s" % ("would rename" if a.dry_run else "renamed", old, new))
        print("  %s" % line)
    for old, new in applied:
        print("no-op: %s -> %s (already applied)" % (old, new))
    print("%s %d symbol(s) in %s" % ("dry-run:" if a.dry_run else "wrote", len(changed), a.file))
    if not a.no_refs:
        for old, _new in pairs:
            print("-- references to %s:" % old)
            cmd_refs(argparse.Namespace(name=old, roots=a.roots, limit=a.limit, file=a.file,
                                        code_only=False))
    return 0


def add_common(p, suppress=False):
    """Shared options, on the main parser and again on every subcommand (so both orders work)."""
    d = argparse.SUPPRESS if suppress else None
    p.add_argument("--file", default=d if suppress else DEFAULT_FILE,
                   help="symbol map (default %s)" % DEFAULT_FILE)
    p.add_argument("--json", action="store_true", default=d if suppress else False,
                   help="machine-readable output")
    p.add_argument("--limit", type=int, default=d if suppress else 40,
                   help="max lines printed (default 40)")
    p.add_argument("--roots", nargs="*", default=d if suppress else ["src", "include", "docs"],
                   help="where `refs` looks for references")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    add_common(ap)
    sub = ap.add_subparsers(dest="cmd", required=False)
    common = argparse.ArgumentParser(add_help=False)
    add_common(common, suppress=True)

    p = sub.add_parser("find", parents=[common], help="regex search over symbol names")
    p.add_argument("pattern")
    p.add_argument("--section")
    p.add_argument("--type", dest="type")
    p.set_defaults(func=cmd_find)

    p = sub.add_parser("show", parents=[common], help="exact lookup")
    p.add_argument("names", nargs="+")
    p.set_defaults(func=cmd_show)

    p = sub.add_parser("at", parents=[common], help="symbols around an address")
    p.add_argument("address")
    p.add_argument("--count", type=int, default=6)
    p.add_argument("--section", default=None)
    p.set_defaults(func=cmd_at)

    p = sub.add_parser("range", parents=[common], help="symbols inside an address range")
    p.add_argument("start")
    p.add_argument("end")
    p.add_argument("--section")
    p.set_defaults(func=cmd_range)

    p = sub.add_parser("refs", parents=[common], help="repo references to a name, classified")
    p.add_argument("name")
    p.add_argument("--code-only", action="store_true",
                   help="only the references a rename must update (no paths, no comment/string mentions)")
    p.set_defaults(func=cmd_refs)

    p = sub.add_parser("check", parents=[common], help="duplicate names, unparsed lines")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("rename", parents=[common], help="rename one symbol (atomic, one-line diff)")
    p.add_argument("old")
    p.add_argument("new")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--force", action="store_true", help="allow overwriting an existing name")
    p.add_argument("--no-refs", action="store_true", help="skip the reference scan")
    p.set_defaults(func=lambda a: rename(a, [(a.old, a.new)]))

    p = sub.add_parser("rename-batch", parents=[common],
                       help="rename many symbols from a file of 'old new' lines")
    p.add_argument("mapfile")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--force", action="store_true")
    p.add_argument("--no-refs", action="store_true")
    p.set_defaults(func=cmd_batch)

    p = sub.add_parser("merge-batch", parents=[common],
                       help="merge phantom fn_* rows from 'merge <phantom> <previous> <size>' lines")
    p.add_argument("mapfile")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--no-refs", action="store_true", help="skip the reference scan")
    p.set_defaults(func=cmd_merge_batch)

    a = ap.parse_args()
    if a.selftest:
        sys.exit(selftest())
    if not a.cmd:
        ap.print_help()
        sys.exit(0)
    if not os.path.isabs(a.file):
        a.file = os.path.join(REPO, a.file)
    sys.exit(a.func(a) or 0)


def cmd_batch(a):
    pairs = []
    with open(a.mapfile, "r", encoding="utf-8") as fh:
        for line in fh:
            line = line.split("#")[0].strip()
            if line:
                old, new = line.split()[:2]
                pairs.append((old, new))
    return rename(a, pairs)


# --------------------------------------------------------------------------------------------------
# selftest - every check runs against a fixture or a temp copy, never the real symbols.txt
# --------------------------------------------------------------------------------------------------
def selftest() -> int:
    import io
    import tempfile
    from contextlib import redirect_stdout

    fails: list[str] = []
    checks = 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    def raises(fn, exc=SystemExit):
        try:
            fn()
        except exc:
            return True
        return False

    def message(fn):
        try:
            fn()
        except SystemExit as e:
            return str(e)
        return ""

    def map_text(nl, rows):
        return nl.join("%s = %s;%s" % (n, loc, (" // " + c) if c else "")
                       for n, loc, c in rows) + nl

    def temps(root):
        return sorted(str(p) for p in Path(root).rglob("*" + sf.TMP_SUFFIX))

    def changed_lines(before, after, nl):
        b, a = before.split(nl), after.split(nl)
        return sum(1 for x, y in zip(b, a) if x != y) + abs(len(a) - len(b))

    rows = [("fn_80040598", ".text:0x80040598", "type:function size:0x10"),
            ("foo", ".text:0x80040600", "type:function size:0x4")]

    # --- line-ending round trip and the one-line diff ------------------------------------------
    for nl, label in (("\n", "LF"), ("\r\n", "CRLF")):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "symbols.txt"
            text = map_text(nl, rows)
            p.write_bytes(text.encode("utf-8"))
            before = p.read_bytes()
            _t, nl2, lines, changed, applied = plan_rename(p, [("foo", "bar")])
            check("plan %s: one edit, nothing applied" % label, (len(changed), len(applied)), (1, 0))
            check("plan %s: the line is rewritten" % label, changed[0][3],
                  "bar = .text:0x80040600; // type:function size:0x4")
            apply_rename(p, nl2, lines, changed)
            after = sf.read_text(p)
            check("apply %s: exactly one line changed" % label, changed_lines(text, after, nl), 1)
            check("apply %s: the ending survives" % label, sf.line_ending(after), nl)
            if nl == "\r\n":
                check("apply CRLF: no bare LF appeared", after.replace("\r\n", "").count("\n"), 0)
                check("apply CRLF: the ending count is unchanged", after.count("\r\n"), text.count("\r\n"))
                check("apply CRLF: the bytes carry CRLF", after.encode("utf-8").count(b"\r\n"),
                      before.count(b"\r\n"))
            else:
                check("apply LF: no CR appeared", "\r" in after, False)
                check("apply LF: the newline count is unchanged", after.count("\n"), text.count("\n"))
            check("apply %s: no temp left" % label, temps(tmp), [])

    # --- the write transaction: a failure leaves the previous bytes exactly ---------------------
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        text = map_text("\n", rows)
        p.write_bytes(text.encode("utf-8"))
        before = p.read_bytes()
        wanted = text.replace("foo", "bar")

        def boom(src, dst):
            raise OSError("injected failure")

        check("transaction: a failed replace raises", raises(lambda: _write_text(p, wanted, boom),
                                                             OSError), True)
        check("transaction: the previous bytes survive", p.read_bytes(), before)
        check("transaction: no temp file", temps(tmp), [])

        def corrupt(src, dst):                       # the rename lands, then the bytes go bad
            os.replace(src, dst)
            with open(dst, "ab") as fh:
                fh.write(b"X")

        check("rollback: a verification failure raises", raises(lambda: _write_text(p, wanted, corrupt),
                                                                IOError), True)
        check("rollback: the previous bytes are restored exactly", p.read_bytes(), before)
        check("rollback: no temp file", temps(tmp), [])

    # --- the shape/anchor gate ------------------------------------------------------------------
    check("shape: a line without '=' is refused",
          raises(lambda: _rewrite_line("foo .text:0x100;", "foo", "bar"), sf.AnchorError), True)
    check("shape: a line whose name is not the token is refused",
          raises(lambda: _rewrite_line("foo = .text:0x100;", "bar", "baz"), sf.AnchorError), True)
    check("shape: a map line rewrites", _rewrite_line("foo = .text:0x100; // c", "foo", "bar"),
          "bar = .text:0x100; // c")

    # --- the refusals, each leaving the file untouched ------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        text = map_text("\n", rows + [("bar", ".text:0x80040610", "type:function size:0x4")])
        p.write_bytes(text.encode("utf-8"))
        before = p.read_bytes()
        check("collision: an existing name is refused",
              raises(lambda: plan_rename(p, [("foo", "bar")])), True)
        check("collision: keeps the ambiguous-rename message",
              "is already defined" in message(lambda: plan_rename(p, [("foo", "bar")])), True)
        check("force: a same name at another address is refused",
              raises(lambda: plan_rename(p, [("foo", "bar")], force=True)), True)
        check("force: the message names both addresses",
              "two addresses" in message(lambda: plan_rename(p, [("foo", "bar")], force=True)), True)
        check("missing: an absent old name is refused",
              "is not defined" in message(lambda: plan_rename(p, [("nope", "x")])), True)
        check("invalid: a bad new name is refused",
              "not a valid symbol name" in message(lambda: plan_rename(p, [("foo", "1bad")])), True)
        check("refusal: the file is untouched", p.read_bytes(), before)
        check("refusal: no temp file", temps(tmp), [])

    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        text = map_text("\n", [("foo", ".text:0x80040600", "type:function size:0x4"),
                               ("bar", ".text:0x80040600", "type:function size:0x4")])
        p.write_bytes(text.encode("utf-8"))
        _t, _nl, _l, changed, _ap = plan_rename(p, [("foo", "bar")], force=True)
        check("force: an alias at the same address is allowed", len(changed), 1)

    # --- idempotency ----------------------------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", rows).encode("utf-8"))
        _t, nl, lines, changed, applied = plan_rename(p, [("foo", "bar")])
        apply_rename(p, nl, lines, changed)
        once = p.read_bytes()
        _t2, _nl2, _l2, changed2, applied2 = plan_rename(p, [("foo", "bar")])
        check("idempotent: the second plan has no edits", len(changed2), 0)
        check("idempotent: the pair is reported already applied", applied2, [("foo", "bar")])
        apply_rename(p, nl, lines, changed2)
        check("idempotent: the bytes do not move", p.read_bytes(), once)

    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", rows + [("baz", ".text:0x80040610",
                                             "type:function size:0x4")]).encode("utf-8"))
        _t, nl, lines, changed, _ap = plan_rename(p, [("foo", "bar"), ("baz", "qux")])
        check("batch: two edits in one pass", len(changed), 2)
        apply_rename(p, nl, lines, changed)
        once = p.read_bytes()
        _t2, _nl2, _l2, changed2, applied2 = plan_rename(p, [("foo", "bar"), ("baz", "qux")])
        check("batch: re-applying is a no-op",
              (len(changed2), sorted(n for _o, n in applied2)), (0, ["bar", "qux"]))
        apply_rename(p, nl, lines, changed2)
        check("batch: the bytes do not move", p.read_bytes(), once)

    # --- the command path (dry-run and real) ----------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", rows).encode("utf-8"))
        before = p.read_bytes()
        ns = argparse.Namespace(file=str(p), dry_run=True, force=False, no_refs=True, roots=[], limit=40)
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = rename(ns, [("foo", "bar")])
        check("dry-run: returns 0", rc, 0)
        check("dry-run: the file is untouched", p.read_bytes(), before)
        check("dry-run: it says what it would do", "would rename foo -> bar" in buf.getvalue(), True)

        ns.dry_run = False
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = rename(ns, [("foo", "bar")])
        check("rename: returns 0", rc, 0)
        check("rename: prints the line it changed", "bar = .text:0x80040600;" in buf.getvalue(), True)
        check("rename: reports one symbol written", "wrote 1 symbol(s)" in buf.getvalue(), True)
        check("rename: the new name is in the file", "bar = .text:0x80040600;" in sf.read_text(p), True)

    # --- phantom merges: the happy path (grow + delete), both line endings ------------------------
    mrows = [("prev", ".text:0x80041000", "type:function size:0x20"),
             ("fn_80041020", ".text:0x80041020", "type:function size:0x4"),
             ("after", ".text:0x80041024", "type:function size:0x8")]
    mrow = ("fn_80041020", "prev", 0x24)
    for nl, label in (("\n", "LF"), ("\r\n", "CRLF")):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "symbols.txt"
            text = map_text(nl, mrows)
            p.write_bytes(text.encode("utf-8"))
            _t, nl2, lines, grown, deleted, applied = plan_merge(p, [mrow])
            check("merge %s: one grown, one deleted, none applied" % label,
                  (len(grown), len(deleted), len(applied)), (1, 1, 0))
            check("merge %s: the grown line sets the size" % label, grown[0],
                  "prev = .text:0x80041000; // type:function size:0x24")
            check("merge %s: the deleted line is the phantom" % label, lines[deleted[0]],
                  "fn_80041020 = .text:0x80041020; // type:function size:0x4")
            apply_merge(p, nl2, lines, grown, deleted)
            after = sf.read_text(p)
            want = [("prev = .text:0x80041000; // type:function size:0x24"
                     if x == "prev = .text:0x80041000; // type:function size:0x20" else x)
                    for x in text.split(nl)
                    if x != "fn_80041020 = .text:0x80041020; // type:function size:0x4"]
            check("merge %s: exactly one line grew and one vanished" % label, after.split(nl), want)
            check("merge %s: the ending survives" % label, sf.line_ending(after), nl)
            if nl == "\r\n":
                check("merge CRLF: no bare LF appeared", after.replace("\r\n", "").count("\n"), 0)
            else:
                check("merge LF: no CR appeared", "\r" in after, False)
            check("merge %s: no temp left" % label, temps(tmp), [])
            once = p.read_bytes()
            _t2, nl3, lines2, grown2, deleted2, applied2 = plan_merge(p, [mrow])
            check("merge %s: the second plan is a no-op" % label,
                  (len(grown2), len(deleted2)), (0, 0))
            check("merge %s: the row is reported already merged" % label, applied2, [mrow])
            apply_merge(p, nl3, lines2, grown2, deleted2)
            check("merge %s: the bytes do not move" % label, p.read_bytes(), once)

    # --- the resize shape gate -------------------------------------------------------------------
    check("resize: a map line grows",
          _resize_line("prev = .text:0x80041000; // type:function size:0x20", "prev", 0x24),
          "prev = .text:0x80041000; // type:function size:0x24")
    check("resize: a line naming another symbol is refused",
          raises(lambda: _resize_line("other = .text:0x80041000; // type:function size:0x20",
                                      "prev", 0x24), sf.AnchorError), True)
    check("resize: a line without a size is refused",
          raises(lambda: _resize_line("prev = .text:0x80041000; // type:function", "prev", 0x24),
                 sf.AnchorError), True)

    # --- phantom merges: every refusal leaves the file untouched ---------------------------------
    def merge_plan(map_rows, rows_, scan=None):
        """Plan against a throwaway map; returns (refusal message or "", file_changed)."""
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "symbols.txt"
            text = map_text("\n", map_rows)
            p.write_bytes(text.encode("utf-8"))
            try:
                plan_merge(p, rows_, scan)
            except SystemExit as e:
                return str(e), p.read_bytes() != text.encode("utf-8")
            return "", p.read_bytes() != text.encode("utf-8")

    adj = [("prev", ".text:0x80041000", "type:function size:0x20"),
           ("fn_80041020", ".text:0x80041020", "type:function size:0x4")]
    cases = [
        ("a gap before the phantom",
         [("prev", ".text:0x80041000", "type:function size:0x1C"),
          ("fn_80041020", ".text:0x80041020", "type:function size:0x4")],
         [mrow], "ends at 0x8004101C"),
        ("a size that is not the two sizes added", adj,
         [("fn_80041020", "prev", 0x28)], "is not"),
        ("different sections",
         [("prev", ".init:0x80041000", "type:function size:0x20"),
          ("fn_80041020", ".text:0x80041020", "type:function size:0x4")],
         [mrow], "is in .init"),
        ("a scope mismatch",
         [("prev", ".text:0x80041000", "type:function size:0x20"),
          ("fn_80041020", ".text:0x80041020", "type:function size:0x4 scope:local")],
         [mrow], "disagree on scope"),
        ("an alias at the phantom's address",
         adj + [("alias", ".text:0x80041020", "type:function size:0x4")],
         [mrow], "shares its address"),
        ("a non-function previous",
         [("prev", ".text:0x80041000", "type:object size:0x20"),
          ("fn_80041020", ".text:0x80041020", "type:function size:0x4")],
         [mrow], "not both type:function"),
        ("a phantom that is another row's previous",
         adj + [("fn_80041024", ".text:0x80041024", "type:function size:0x4")],
         [mrow, ("fn_80041024", "fn_80041020", 0x28)], "appears twice"),
        ("the same phantom twice", adj, [mrow, mrow], "appears twice"),
        ("a phantom with no previous defined",
         [("fn_80041020", ".text:0x80041020", "type:function size:0x4")],
         [mrow], "is not defined"),
        ("a half-applied row (phantom gone, old size)",
         [("prev", ".text:0x80041000", "type:function size:0x20")],
         [mrow], "is absent but prev has size:0x20"),
        ("neither name defined", [], [mrow], "neither"),
    ]
    for name, map_rows, batch, needle in cases:
        msg, changed = merge_plan(map_rows, batch)
        check("merge refuses %s" % name, bool(msg) and needle in msg, True)
        check("merge refusal leaves the file alone (%s)" % name, changed, False)

    # the stale-plan diagnostic: a rename landed after phantom.py ran
    msg, _changed = merge_plan([("new_prev", ".text:0x80041000", "type:function size:0x20"),
                                ("fn_80041020", ".text:0x80041020", "type:function size:0x4")],
                               [("fn_80041020", "old_prev", 0x24)])
    check("merge: a stale previous name is refused", "is not defined" in msg, True)
    check("merge: the refusal names the symbol that ends at the phantom",
          "the symbol ending at 0x80041020 is new_prev (size:0x20)" in msg, True)

    # an absent phantom whose previous already carries the planned size is the re-apply, not a typo
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", [("prev", ".text:0x80041000",
                                       "type:function size:0x24")]).encode("utf-8"))
        _t, _nl, _l, grown2, deleted2, applied2 = plan_merge(p, [mrow])
        check("merge: an absent phantom with the new size is already applied",
              (len(grown2), len(deleted2), applied2), (0, 0, [mrow]))

    # the reference gate
    fake = lambda names: {n: [("src/x.c", 7, "call %s" % n)] for n in names}
    msg, changed = merge_plan(adj, [mrow], fake)
    check("merge: a referenced phantom is refused", "is referenced at src/x.c:7" in msg, True)
    check("merge: the reference refusal leaves the file alone", changed, False)
    msg, _c = merge_plan(adj, [mrow], lambda names: {n: [] for n in names})
    check("merge: a clean reference scan passes", msg, "")
    check("refs: a known name is found", len(find_refs(["symedit"], ["tools"], 3)["symedit"]) > 0, True)
    # The name must be unique per RUN, not merely unusual. This check asserts a repo-wide ABSENCE, so a
    # hardcoded made-up name is a static string another file may legitimately contain - a selftest fixture
    # in tools/units/callees.py carried exactly fn_99999999, and the check then failed for a reason that
    # had nothing to do with find_refs. Deriving it from the pid keeps it valid (fn_<8 hex>) and unique.
    ghost = "fn_%08X" % (os.getpid() & 0xFFFFFFFF)
    check("refs: a made-up name is not", find_refs([ghost], ["tools"], 3)[ghost], [])

    # --- the merge-batch file parser -------------------------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        bf = Path(tmp) / "batch.txt"
        bf.write_text("# a comment\n\nmerge fn_80041020 prev 0x24\n", encoding="utf-8")
        check("merge batch: comments and blanks are skipped", read_merge_rows(bf), [mrow])
        bf.write_text("merge fn_80041020 prev\n", encoding="utf-8")
        check("merge batch: a short line is refused",
              "is not 'merge" in message(lambda: read_merge_rows(bf)), True)
        bf.write_text("rename a b 0x4\n", encoding="utf-8")
        check("merge batch: a non-merge verb is refused",
              "is not 'merge" in message(lambda: read_merge_rows(bf)), True)
        bf.write_text("merge fn_80041020 prev zz\n", encoding="utf-8")
        check("merge batch: a non-hex size is refused",
              "has no hex size" in message(lambda: read_merge_rows(bf)), True)

    # --- the merge command path (dry-run and real) -----------------------------------------------
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", mrows).encode("utf-8"))
        bf = Path(tmp) / "batch.txt"
        bf.write_text("# comment\nmerge fn_80041020 prev 0x24\n\n", encoding="utf-8")
        before = p.read_bytes()
        ns = argparse.Namespace(file=str(p), dry_run=True, no_refs=True, roots=[], limit=40,
                                mapfile=str(bf))
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = cmd_merge_batch(ns)
        check("merge cmd: dry-run returns 0", rc, 0)
        check("merge cmd: dry-run leaves the file", p.read_bytes(), before)
        check("merge cmd: dry-run says what it would do",
              "would grow" in buf.getvalue() and "would delete" in buf.getvalue(), True)
        ns.dry_run = False
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = cmd_merge_batch(ns)
        check("merge cmd: returns 0", rc, 0)
        check("merge cmd: the phantom line is gone", "fn_80041020" in sf.read_text(p), False)
        check("merge cmd: the previous grew",
              "prev = .text:0x80041000; // type:function size:0x24" in sf.read_text(p), True)
        check("merge cmd: reports the counts",
              "wrote 1 merged, 1 deleted, 0 already merged" in buf.getvalue(), True)
        once = p.read_bytes()
        buf = io.StringIO()
        with redirect_stdout(buf):
            rc = cmd_merge_batch(ns)
        check("merge cmd: the second run is a no-op",
              "0 merged, 0 deleted, 1 already merged" in buf.getvalue(), True)
        check("merge cmd: the bytes do not move", p.read_bytes(), once)
        check("merge cmd: no temp left", temps(tmp), [])

    # --- the reference list classifies what it found: a map name is also a FILE name ------------------
    # The bug this closes: `find_refs` matches with `\b`, so `/` and `.` are boundaries and a path such as
    # `#include "DWCi/fn_805113B0.h"` was reported as an ordinary reference. A lane that rewrote every
    # reported reference corrupted the include - the file on disk keeps its name - and only a build noticed.
    check("a bare call is a code reference", ref_kinds("    bl fn_80040598", "fn_80040598"), {"code"})
    check("a quoted include is a path, not a symbol",
          ref_kinds('#include "DWCi/fn_805113B0.h"', "fn_805113B0"), {"path"})
    check("a path in a comment is a path",
          ref_kinds(" * see src/DWCi/fn_805113B0.c", "fn_805113B0"), {"path"})
    check("a backslash path is a path", ref_kinds(" * src\\DWCi\\fn_805113B0.c", "fn_805113B0"), {"path"})
    check("a directory mention is a path",
          ref_kinds(" * in include/Network/fn_803D3CE8.h", "fn_803D3CE8"), {"path"})
    check("a bare name with a source suffix is a path", ref_kinds("fn_805113B0.c", "fn_805113B0"),
          {"path"})
    check("a comment mention is inert", ref_kinds("/* the fn_805113B0 band */ x = 1;", "fn_805113B0"),
          {"comment"})
    check("a string mention is inert", ref_kinds('p = "fn_805113B0";', "fn_805113B0"), {"string"})
    check("a line with both reports both",
          ref_kinds('fn_80040598(); /* see fn_80040598.h */', "fn_80040598"), {"code", "path"})
    check("an unrelated line has no kinds", ref_kinds("int x = 1;", "fn_805113B0"), set())
    grouped = group_hits("fn_805113B0", [("a.h", 1, '#include "x/fn_805113B0.h"'),
                                         ("b.c", 2, "    bl fn_805113B0"),
                                         ("c.c", 3, "/* fn_805113B0 */")])
    check("group: code is the rewrite list", [h[0] for h in grouped["code"]], ["b.c"])
    check("group: a path is listed separately", [h[0] for h in grouped["path"]], ["a.h"])
    check("group: a mention is neither", [h[0] for h in grouped["mention"]], ["c.c"])

    # --- the map resolves against the TREE the command was run in ------------------------------------
    # `symedit` *writes* the map, so a MAIN-hardcoded root meant a lane that invoked MAIN's copy from its
    # own worktree renamed symbols in MAIN (and, reading, refused to find a name the branch had just
    # introduced). The default `--file` now resolves through `unitutil.repo_root` - the caller's git
    # worktree. This runs the real CLI from a throwaway git tree whose map is the only one that knows the
    # marker, so a file-location root cannot pass it.
    import subprocess
    marker = "zz_selftest_marker"
    with tempfile.TemporaryDirectory() as tmp:
        repo = Path(tmp) / "fake-wt"
        (repo / "config" / "RMHE08").mkdir(parents=True)
        (repo / "configure.py").write_text("# a repo\n", encoding="utf-8")
        (repo / "config" / "RMHE08" / "symbols.txt").write_text(
            map_text("\n", [(marker, ".text:0x80001000", "type:function size:0x4")]),
            encoding="utf-8")
        subprocess.run(["git", "init", "-q"], cwd=repo, capture_output=True)
        p = subprocess.run([sys.executable, os.path.abspath(__file__), "show", marker],
                           cwd=repo, capture_output=True, text=True, encoding="utf-8", errors="replace")
        check("the default map is the invocation tree's (its marker is found)", marker in (p.stdout or ""),
              True)
        check("... and the command succeeds there", p.returncode, 0)

    # --- `at` on a data address picks the data section, not `.text` -------------------------------
    # The filed bug: `at 0x80500000` defaulted to `.text`, so a data address above the whole `.text`
    # range landed at the END of `.text` and the tracer was handed the wrong rows. The section is
    # inferred from the map's own extents; an explicit `--section` is still honoured.
    at_rows = [("fn_80040598", ".text:0x80040598", "type:function size:0x10"),
               ("fn_80040600", ".text:0x80040600", "type:function size:0x4"),
               ("gData", ".data:0x80500000", "type:object size:0x10"),
               ("gNext", ".data:0x80500020", "type:object size:0x4")]
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "symbols.txt"
        p.write_bytes(map_text("\n", at_rows).encode("utf-8"))
        parsed = list(entries(str(p)))
        check("infer: a data address picks .data", infer_section(parsed, 0x80500000), ".data")
        check("infer: a text address picks .text", infer_section(parsed, 0x80040598), ".text")
        check("infer: an explicit section is not second-guessed",
              infer_section(parsed, 0x80500000, ".text"), ".text")
        check("infer: a gap address picks the nearest section",
              infer_section(parsed, 0x80510000), ".data")
        out = io.StringIO()
        ns = argparse.Namespace(address="0x80500000", count=2, section=None, file=str(p), json=False)
        with redirect_stdout(out):
            rc_at = cmd_at(ns)
        text = out.getvalue()
        check("at: a data address exits 0", rc_at, 0)
        check("at: the data row is shown", "gData" in text, True)
        check("at: no text row leaks in", "fn_80040598" in text, False)
        out = io.StringIO()
        ns = argparse.Namespace(address="0x80500000", count=2, section=".text", file=str(p),
                                json=False)
        with redirect_stdout(out):
            cmd_at(ns)
        check("at: an explicit --section still shows its own rows",
              "fn_80040598" in out.getvalue(), True)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    main()
