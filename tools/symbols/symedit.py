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

`rename` is atomic, keeps the file's line endings, and prints just the one line it changed.  It
refuses when the old name is not defined exactly once or the new name is already taken, and it warns
about in-repo references to the old name (a rename is always two edits: this file *and* the source -
see AGENTS.md -> Conventions -> "Commenting and naming").
"""
import argparse
import os
import re
import sys

DEFAULT_FILE = "config/RMHE08/symbols.txt"
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
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


def rewrite(path, replacements, dry_run):
    """Replace name tokens on definition lines; returns the list of (old, new, line) changed."""
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as fh:
        text = fh.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(newline)
    changed = []
    for old, new in replacements.items():
        hits = [i for i, line in enumerate(lines) if LINE_RE.match(line) and parse_line(line)
                and parse_line(line)["name"] == old]
        if len(hits) != 1:
            raise SystemExit("refusing: %s is defined %d times" % (old, len(hits)))
        i = hits[0]
        lines[i] = re.sub(r"^%s(\s*=)" % re.escape(old), new + r"\1", lines[i], count=1)
        changed.append((old, new, lines[i]))
    if dry_run:
        return changed
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="") as fh:
        fh.write(newline.join(lines))
    os.replace(tmp, path)
    return changed


def rename(a, pairs):
    existing = {e["name"] for e in entries(a.file)}
    for old, new in pairs:
        if old not in existing:
            raise SystemExit("refusing: %s is not defined in %s" % (old, a.file))
        if new in existing and not a.force:
            raise SystemExit("refusing: %s is already defined in %s (use --force)" % (new, a.file))
        if not re.fullmatch(r"[A-Za-z_][\w.$]*", new):
            raise SystemExit("refusing: %s is not a valid symbol name" % new)
    changed = rewrite(a.file, dict(pairs), a.dry_run)
    for old, new, line in changed:
        print("%s %s -> %s" % ("would rename" if a.dry_run else "renamed", old, new))
        print("  %s" % line)
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
    add_common(ap)
    sub = ap.add_subparsers(dest="cmd", required=True)
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


if __name__ == "__main__":
    main()
