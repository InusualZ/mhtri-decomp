#!/usr/bin/env python3
"""Line-ending-safe text edits: `replace`, `normalise`, `check`.

    python tools/agents/edit.py replace FILE --old-file A --new-file B [--count N]
    python tools/agents/edit.py normalise FILE...
    python tools/agents/edit.py check [--fix]

Why this exists: slot working copies mix CRLF and LF files while the index is LF (`.gitattributes` has
`eol=lf`). A scripted exact-match edit (`str.replace` with `\\n`) silently matches nothing on a CRLF file,
and an edit that "works" can write LF lines into a CRLF file or the reverse. `escape.py --edit` normalises
the *needle* to LF only. This helper never guesses:

* `replace` reads FILE as bytes, matches OLD across `\\n` **or** `\\r\\n` (the texts come from files, so there
  is no shell escaping), converts NEW to the ending of the span it replaces (a mixed file keeps each region's
  own ending; a span with no line break takes the file's dominant ending), asserts the match count (default
  exactly 1; 0 or more than N is refused with the line numbers of the matches and nothing is written),
  writes back the same bytes everywhere else, and prints a unified diff.
* `normalise` rewrites the named text files to LF and reports which ones were CRLF/mixed.
* `check` lists tracked files in the working tree whose on-disk endings differ from the index
  (`git ls-files --eol`); `--fix` normalises them.
"""
from __future__ import annotations

import argparse
import difflib
import os
import re
import subprocess
import sys


def _read(path: str) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _write(path: str, data: bytes) -> None:
    tmp = path + ".edit.tmp"
    with open(tmp, "wb") as fh:
        fh.write(data)
    os.replace(tmp, path)


def to_lf(data: bytes) -> bytes:
    return data.replace(b"\r\n", b"\n")


def to_ending(data: bytes, ending: bytes) -> bytes:
    data = to_lf(data)
    return data if ending == b"\n" else data.replace(b"\n", ending)


def classify(data: bytes) -> str:
    """'lf', 'crlf', 'mixed' or 'none' (no line break) by counting `\\r\\n` against bare `\\n`."""
    crlf = data.count(b"\r\n")
    lf = data.count(b"\n") - crlf
    if crlf and lf:
        return "mixed"
    if crlf:
        return "crlf"
    return "lf" if lf else "none"


def dominant(data: bytes) -> bytes:
    crlf = data.count(b"\r\n")
    return b"\r\n" if crlf > data.count(b"\n") - crlf else b"\n"


def _pattern(old_lf: bytes) -> "re.Pattern[bytes]":
    lines = old_lf.split(b"\n")
    return re.compile(b"\r?\n".join(re.escape(x) for x in lines))


def find_matches(data: bytes, old: bytes) -> list[tuple[int, int, int]]:
    """[(start, end, 1-based line)] of the non-overlapping matches of `old` in `data`, either ending."""
    if not old:
        raise SystemExit("edit: the old text is empty")
    pat = _pattern(to_lf(old))
    return [(m.start(), m.end(), data.count(b"\n", 0, m.start()) + 1) for m in pat.finditer(data)]


def replace_bytes(data: bytes, old: bytes, new: bytes, count: int = 1) -> tuple[bytes, list[int]]:
    """(new file bytes, match line numbers). Raises SystemExit, changing nothing, when the count is wrong."""
    hits = find_matches(data, old)
    lines = [h[2] for h in hits]
    if len(hits) != count:
        raise SystemExit("edit: expected %d match(es), found %d%s" % (
            count, len(hits), " at line(s) " + ", ".join(map(str, lines)) if hits else ""))
    fallback = dominant(data)
    out, pos = [], 0
    for start, end, _line in hits:
        span = data[start:end]
        if b"\r\n" in span:
            ending = b"\r\n"
        elif b"\n" in span:
            ending = b"\n"
        else:
            ending = fallback
        out.append(data[pos:start])
        out.append(to_ending(new, ending))
        pos = end
    out.append(data[pos:])
    return b"".join(out), lines


def _diff(path: str, before: bytes, after: bytes) -> str:
    a = to_lf(before).decode("utf-8", "surrogateescape").splitlines(keepends=True)
    b = to_lf(after).decode("utf-8", "surrogateescape").splitlines(keepends=True)
    return "".join(difflib.unified_diff(a, b, "a/" + path, "b/" + path))


def cmd_replace(args) -> int:
    data = _read(args.file)
    old, new = _read(args.old_file), _read(args.new_file)
    after, lines = replace_bytes(data, old, new, args.count)
    kind = classify(data)
    _write(args.file, after)
    print("edit: %s - %d replacement(s) at line(s) %s; file endings %s, kept" % (
        args.file, len(lines), ", ".join(map(str, lines)), kind))
    sys.stdout.write(_diff(args.file, data, after) or "(no textual change)\n")
    return 0


def normalise_file(path: str) -> str | None:
    """Rewrite `path` to LF; return its former class ('crlf'/'mixed') or None when nothing changed."""
    data = _read(path)
    if b"\0" in data:
        return None
    kind = classify(data)
    if kind in ("crlf", "mixed"):
        _write(path, to_lf(data))
        return kind
    return None


def cmd_normalise(args) -> int:
    changed = 0
    for path in args.files:
        if b"\0" in _read(path):
            print("skip (binary)  %s" % path)
            continue
        kind = normalise_file(path)
        if kind:
            changed += 1
            print("%-14s %s" % ("was " + kind, path))
        else:
            print("%-14s %s" % ("already LF", path))
    print("normalised %d of %d file(s)" % (changed, len(args.files)))
    return 0


EOL_RE = re.compile(r"^i/(\S+)\s+w/(\S+)\s+attr/(.*?)\t(.*)$")


def drifted(root: str) -> list[tuple[str, str, str]]:
    """[(path, index class, working-tree class)] for tracked files whose endings differ from the index."""
    p = subprocess.run(["git", "ls-files", "--eol"], cwd=root, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if p.returncode != 0:
        raise SystemExit("edit: git ls-files --eol failed: " + (p.stderr or "").strip())
    out = []
    for line in p.stdout.splitlines():
        m = EOL_RE.match(line)
        if not m:
            continue
        idx, wt, _attr, path = m.groups()
        if wt in ("-text", "none") or idx == wt:
            continue
        if idx in ("-text", "none"):
            continue
        out.append((path, idx, wt))
    return out


def cmd_check(args) -> int:
    top = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True,
                         encoding="utf-8", errors="replace")
    if top.returncode != 0:
        raise SystemExit("edit: not in a git worktree")
    root = top.stdout.strip()
    rows = drifted(root)
    for path, idx, wt in rows:
        print("index %-6s working tree %-6s %s" % (idx, wt, path))
    if args.fix:
        fixed = sum(1 for path, _i, _w in rows if normalise_file(os.path.join(root, path)))
        print("check: normalised %d file(s) to LF" % fixed)
        return 0
    print("check: %d tracked file(s) differ from the index's line endings" % len(rows))
    return 1 if rows else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("replace", help="exact-text replace that preserves the file's line endings")
    r.add_argument("file")
    r.add_argument("--old-file", required=True, help="file holding the text to find (LF or CRLF)")
    r.add_argument("--new-file", required=True, help="file holding the replacement text")
    r.add_argument("--count", type=int, default=1, help="exact number of matches to require (default 1)")
    r.set_defaults(fn=cmd_replace)
    n = sub.add_parser("normalise", help="rewrite files to LF, reporting the CRLF/mixed ones")
    n.add_argument("files", nargs="+")
    n.set_defaults(fn=cmd_normalise)
    c = sub.add_parser("check", help="tracked files whose working-tree endings differ from the index")
    c.add_argument("--fix", action="store_true", help="normalise them to LF")
    c.set_defaults(fn=cmd_check)
    args = ap.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
