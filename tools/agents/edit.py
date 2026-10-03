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
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import difflib
import os
import sys

from tools.lib import text
from tools.lib.git import Git
from tools.lib.text import dominant, to_ending, to_lf  # noqa: F401 - the names this CLI has always exported


def _read(path: str) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _write(path: str, data: bytes) -> None:
    text.atomic_write(path, data)


def classify(data: bytes) -> str:
    """'lf', 'crlf', 'mixed' or 'none' (no line break) by counting `\\r\\n` against bare `\\n`."""
    return text.endings(data, lone_cr=False)


def find_matches(data: bytes, old: bytes) -> list[tuple[int, int, int]]:
    if not old:
        raise SystemExit("edit: the old text is empty")
    return text.find_matches(data, old)


def replace_bytes(data: bytes, old: bytes, new: bytes, count: int = 1) -> tuple[bytes, list[int]]:
    """(new file bytes, match line numbers). Raises SystemExit, changing nothing, when the count is wrong."""
    if not old:
        raise SystemExit("edit: the old text is empty")
    try:
        return text.replace_bytes(data, old, new, count)
    except text.MatchCountError as exc:
        raise SystemExit("edit: %s" % exc) from None


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


def drifted(root: str) -> list[tuple[str, str, str]]:
    """[(path, index class, working-tree class)] for tracked files whose endings differ from the index."""
    try:
        rows = Git(root).ls_files(eol=True)
    except Exception as exc:  # noqa: BLE001 - GitError: name git's own message
        raise SystemExit("edit: git ls-files --eol failed: " + str(getattr(exc, "stderr", exc)).strip())
    out = []
    for idx, wt, _attr, path in rows:
        if wt in ("-text", "none") or idx == wt:
            continue
        if idx in ("-text", "none"):
            continue
        out.append((path, idx, wt))
    return out


def cmd_check(args) -> int:
    root = Git().toplevel()
    if not root:
        raise SystemExit("edit: not in a git worktree")
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
