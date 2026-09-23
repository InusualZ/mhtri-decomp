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
    symedit.py refs   <name> [--roots src include docs]
    symedit.py check                                 # duplicate names / addresses, bad lines
    symedit.py rename <old> <new> [--dry-run] [--force] [--no-refs]
    symedit.py rename-batch <file> [--dry-run]       # lines: "old new" (# comments allowed)
    symedit.py --selftest                            # the checks, against temp fixtures only

`rename` writes through `tools/units/sharedfiles.py` (docs/plan.md 7.12): the edit is a temp file +
`os.replace` transaction that restores the previous bytes exactly if it fails, and the map's own line
ending is preserved.  Before a byte is written it asserts the shape a rename depends on - the old name
is defined exactly once, on a line that parses as a map line, and the rewrite yields a line that parses
back as the new name.  Re-applying a rename that is already in the file is a no-op.  It refuses when
the new name is already taken (unless `--force`), when `--force` would still leave one name at two
addresses, and when the new name is not a valid symbol name; it warns about in-repo references to the
old name (a rename is always two edits: this file *and* the source - see AGENTS.md -> Conventions ->
"Commenting and naming").
"""
import argparse
import os
import re
import sys
from pathlib import Path

DEFAULT_FILE = "config/RMHE08/symbols.txt"
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
_UNITS = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "units")
_UNITS = os.path.normpath(_UNITS)
if _UNITS not in sys.path:
    sys.path.insert(0, _UNITS)

import sharedfiles as sf  # noqa: E402  the one writer for shared files - docs/plan.md 7.12
LINE_RE = re.compile(r"^(?P<name>[^\s=]+)\s*=\s*(?P<loc>[^;]+);\s*(?://\s*(?P<comment>.*))?$")
ADDR_RE = re.compile(r"^(?P<section>[.\w]+):(?:0x)?(?P<addr>[0-9a-fA-F]+)$")


def parse_line(line):
    m = LINE_RE.match(line.rstrip("\n"))
    if not m:
        return None
    loc = ADDR_RE.match(m.group("loc").strip())
    if not loc:
        return None
    comment = m.group("comment") or ""
    kind = re.search(r"type:(\S+)", comment)
    size = re.search(r"size:(0x[0-9a-fA-F]+)", comment)
    return {
        "name": m.group("name"),
        "section": loc.group("section"),
        "address": int(loc.group("addr"), 16),
        "type": kind.group(1) if kind else "",
        "size": int(size.group(1), 16) if size else 0,
        "line": line.rstrip("\n"),
    }


def entries(path):
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        for i, line in enumerate(fh, 1):
            e = parse_line(line)
            if e:
                e["lineno"] = i
                yield e


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
    rx = re.compile(a.pattern)
    hits = [e for e in entries(a.file)
            if rx.search(e["name"]) and (not a.section or e["section"] == a.section)
            and (not a.type or e["type"] == a.type)]
    emit(hits, a.json, a.limit)


def cmd_show(a):
    want = set(a.names)
    hits = [e for e in entries(a.file) if e["name"] in want]
    emit(hits, a.json)
    missing = want - {e["name"] for e in hits}
    if missing:
        print("not found: %s" % ", ".join(sorted(missing)), file=sys.stderr)
        return 1
    return 0


def cmd_at(a):
    want = int(a.address, 0) if not re.fullmatch(r"[0-9a-fA-F]+", a.address) else int(a.address, 16)
    sec = a.section or ".text"
    all_e = sorted((e for e in entries(a.file) if e["section"] == sec),
                   key=lambda e: e["address"])
    idx = [i for i, e in enumerate(all_e) if e["address"] <= want]
    centre = idx[-1] if idx else 0
    lo = max(0, centre - a.count)
    emit(all_e[lo:centre + a.count + 1], a.json)
    return 0


def cmd_range(a):
    lo = int(a.start, 0)
    hi = int(a.end, 0)
    hits = [e for e in entries(a.file)
            if lo <= e["address"] < hi and (not a.section or e["section"] == a.section)]
    hits.sort(key=lambda e: e["address"])
    emit(hits, a.json, a.limit)
    return 0


def cmd_refs(a):
    pat = re.compile(r"\b%s\b" % re.escape(a.name))
    found = 0
    for root in a.roots:
        for base, dirs, files in os.walk(os.path.join(REPO, root)):
            dirs[:] = [d for d in dirs if d not in (".git", "build", "__pycache__")]
            for fn in files:
                if not fn.endswith((".c", ".h", ".cpp", ".hpp", ".py", ".md", ".txt", ".yml", ".yaml")):
                    continue
                p = os.path.join(base, fn)
                if os.path.abspath(p) == os.path.abspath(a.file):
                    continue
                try:
                    with open(p, "r", encoding="utf-8", errors="replace") as fh:
                        for i, line in enumerate(fh, 1):
                            if pat.search(line):
                                print("%s:%d: %s" % (os.path.relpath(p, REPO), i, line.strip()[:160]))
                                found += 1
                                if found >= a.limit:
                                    print("... (stopping at --limit %d)" % a.limit)
                                    return 0
                except OSError:
                    continue
    if not found:
        print("no references to %s outside %s" % (a.name, a.file))
    return 0


def cmd_check(a):
    seen_name, seen_addr, bad, aliases = {}, {}, [], 0
    for e in entries(a.file):
        seen_name.setdefault(e["name"], []).append(e)
        seen_addr.setdefault((e["section"], e["address"]), []).append(e)
    for name, es in seen_name.items():
        if len(es) > 1:
            bad.append("duplicate name %s at lines %s" % (name, [e["lineno"] for e in es]))
    for (sec, addr), es in seen_addr.items():
        if len(es) > 1:
            # several names at one address is normal in a dtk map (label aliases)
            aliases += 1
    with open(a.file, "r", encoding="utf-8", errors="replace", newline="") as fh:
        for i, line in enumerate(fh, 1):
            if line.strip() and not line.startswith(("#", "//")) and not parse_line(line):
                bad.append("unparsed line %d: %s" % (i, line.strip()[:100]))
    if bad:
        print("\n".join(bad[:a.limit]))
        print("%d problem(s), %d address alias group(s)" % (len(bad), aliases))
        return 1
    print("%s: %d symbols, no duplicate names, all lines parse (%d address alias group(s))"
          % (a.file, len(seen_name), aliases))
    return 0


def _rewrite_line(line, old, new):
    """The name token of one definition line, replaced - or `sf.AnchorError` if it is not that shape.

    This is the shape gate: the line must parse as a map line naming `old`, and the rewrite must yield
a line that parses back as `new`.  Anything else raises before a byte is written, so a rename can
never silently change nothing.
    """
    if not LINE_RE.match(line) or not parse_line(line):
        raise sf.AnchorError("line %r is not a map line" % line[:80])
    out = re.sub(r"^%s(\s*=)" % re.escape(old), lambda m: new + m.group(1), line, count=1)
    if out == line:
        raise sf.AnchorError("line %r does not start with the name %r" % (line[:80], old))
    if (parse_line(out) or {}).get("name") != new:
        raise sf.AnchorError("rewriting %r did not name %r" % (line[:80], new))
    return out


def _definitions(lines):
    """name -> the indices of the lines that define it, by parsed shape (never by substring)."""
    out = {}
    for i, line in enumerate(lines):
        e = parse_line(line)
        if e:
            out.setdefault(e["name"], []).append(i)
    return out


def _names_at_two_addresses(lines, names):
    """The `names` that appear at more than one (section, address) - the duplicate the map forbids."""
    seen = {}
    for line in lines:
        e = parse_line(line)
        if e and e["name"] in names:
            seen.setdefault(e["name"], set()).add((e["section"], e["address"]))
    return {n: sorted(a) for n, a in seen.items() if len(a) > 1}


def plan_rename(path, pairs, force=False):
    """Read the map and plan a batch of renames - no write, every gate runs here.

    Returns `(text, nl, lines, changed, applied)`: `changed` is `(old, new, index, new_line)` for the
    edits to make and `applied` the pairs whose rename is already in the file (the idempotent no-op).
    A pair whose `old` is absent *and* whose `new` is absent is a typo, not a re-apply, and is refused;
    so is a rename that would put one name at two addresses.
    """
    text = sf.read_text(path)
    nl = sf.line_ending(text)
    lines = text.split(nl)
    defined = _definitions(lines)
    changed, applied, used = [], [], set()
    for old, new in pairs:
        if old == new:
            applied.append((old, new))
            continue
        hits = defined.get(old, [])
        if not hits:
            if new in defined:
                applied.append((old, new))          # the rename is already in the file
                continue
            raise SystemExit("refusing: %s is not defined in %s" % (old, path))
        if new in defined and not force:
            raise SystemExit("refusing: %s is already defined in %s (use --force)" % (new, path))
        if not re.fullmatch(r"[A-Za-z_][\w.$]*", new):
            raise SystemExit("refusing: %s is not a valid symbol name" % new)
        if len(hits) != 1:
            raise SystemExit("refusing: %s is defined %d times" % (old, len(hits)))
        i = hits[0]
        if i in used:
            raise SystemExit("refusing: %s appears twice in the batch" % old)
        if sf.missing_anchors(text, [lines[i]]):
            raise sf.AnchorError("refusing: the %s definition line is not in %s" % (old, path))
        changed.append((old, new, i, _rewrite_line(lines[i], old, new)))
        used.add(i)
    if changed:
        planned = list(lines)
        for _old, _new, i, new_line in changed:
            planned[i] = new_line
        dupes = _names_at_two_addresses(planned, {new for _old, new, _i, _l in changed})
        if dupes:
            name = sorted(dupes)[0]
            (s1, a1), (s2, a2) = dupes[name][0], dupes[name][1]
            raise SystemExit("refusing: %s would be defined at two addresses (%s:0x%08X, %s:0x%08X)"
                             % (name, s1, a1, s2, a2))
    return text, nl, lines, changed, applied


def _write_text(path, text, rename=None):
    """Write through the shared-file transaction; a failure restores the previous bytes exactly."""
    path = Path(path)
    tx = sf.Transaction(rename=rename)
    try:
        tx.write(path, text)
        if sf.read_text(path) != text:              # the bytes on disk are the ones we planned
            raise IOError("verification failed: %s does not hold what was written" % path)
    except BaseException:
        tx.rollback()
        raise
    finally:
        tx.cleanup()


def apply_rename(path, nl, lines, changed):
    """Apply the planned edits and write once, in the file's own line ending."""
    if not changed:
        return
    for _old, _new, i, new_line in changed:
        lines[i] = new_line
    _write_text(path, nl.join(lines))


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
            cmd_refs(argparse.Namespace(name=old, roots=a.roots, limit=a.limit, file=a.file))
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

    p = sub.add_parser("refs", parents=[common], help="grep the repo for references to a name")
    p.add_argument("name")
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

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


if __name__ == "__main__":
    main()
