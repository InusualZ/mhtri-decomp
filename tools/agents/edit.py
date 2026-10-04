#!/usr/bin/env python3
"""Line-ending-safe text edits: `replace` (CRLF/LF-agnostic, count-asserted), `normalise`, `check` (tree vs index).
Spec: docs/tools/spec/edit.md. CLI: edit.py replace FILE (--old-file A | --old T) (--new-file B | --new T) [--count N] | normalise FILE... | check [--fix] | check --blob [PATH...] [--rev R] [--repo D]."""
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
    old = _read(args.old_file) if args.old_file is not None else text.c_unescape(args.old)
    new = _read(args.new_file) if args.new_file is not None else text.c_unescape(args.new)
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


# --- check --blob: one path's working-tree bytes against the blob git stores (was checklf.py) -----------------

def blob_of(repo: str, path: str, rev: str = ":") -> bytes | None:
    """The blob git stores for `path` at `rev` (`:` is the index, `HEAD` a commit), or None when git does not know
    it."""
    spec = rev if rev.endswith(":") else rev + ":"
    p = Git(repo).run_bytes("show", spec + path.replace(os.sep, "/"))
    return p.stdout if p.returncode == 0 else None


def endings(data: bytes) -> dict:
    """`{"crlf", "lone_cr", "lf"}` counts for a byte string (`lib.text.ending_counts`)."""
    return text.ending_counts(data)


def _describe(e: dict) -> str:
    bits = []
    if e["crlf"]:
        bits.append("%d CRLF" % e["crlf"])
    if e["lone_cr"]:
        bits.append("%d lone CR" % e["lone_cr"])
    if e["lf"]:
        bits.append("%d LF" % e["lf"])
    return ", ".join(bits) or "no line break"


def check_path(repo: str, path: str, rev: str = ":") -> dict | None:
    """`{"path", "working", "blob", "why"}` when `path`'s line-ending style (CRLF or lone CR present) differs
    from its blob's, else None; a path git does not know is judged against the repository's LF convention."""
    full = path if os.path.isabs(path) else os.path.join(repo, path)
    try:
        with open(full, "rb") as fh:
            working = fh.read()
    except OSError as exc:
        return {"path": path, "working": None, "blob": None, "why": "cannot read %s: %s" % (path, exc)}
    we = endings(working)
    blob = blob_of(repo, path, rev)
    if blob is None:
        if we["crlf"] or we["lone_cr"]:
            return {"path": path, "working": _describe(we), "blob": None,
                    "why": "the path is not in git (%s) - this repository stores LF "
                           "(`.gitattributes` is `* text=auto eol=lf`), so the working tree's line "
                           "endings would be normalised away on the next `git add` and never reviewed"
                           % ("HEAD" if rev == "HEAD" else "the index")}
        return None
    be = endings(blob)
    if (bool(we["crlf"]), bool(we["lone_cr"])) == (bool(be["crlf"]), bool(be["lone_cr"])):
        return None
    why = "the working tree has %s; the blob has %s" % (_describe(we), _describe(be))
    if we["crlf"] and not be["crlf"]:
        why += (" - git normalises it away on `git add`, so `git diff` shows nothing and the CRLF is "
                "committed, reviewed and built as if it were not there")
    elif be["crlf"] and not we["crlf"]:
        why += " - the committed blob itself carries CRLF, which `.gitattributes` (eol=lf) forbids"
    return {"path": path, "working": _describe(we), "blob": _describe(be), "why": why}


def status_paths(repo: str) -> list[str]:
    """Every path `git status --porcelain` names (modified, added, untracked; a rename's destination)."""
    p = Git(repo).run("status", "--porcelain")
    if p.returncode != 0:
        return []
    paths = []
    for line in p.stdout.splitlines():
        entry = line[3:].strip()
        if " -> " in entry:
            entry = entry.split(" -> ", 1)[1]
        if entry and not entry.startswith('"'):
            paths.append(entry)
    return paths


def check_blobs(repo: str, paths: list[str], rev: str = ":") -> int:
    """Print each path whose line endings differ from its blob; 1 when any does, else 0."""
    paths = paths or status_paths(repo)
    findings = [f for f in (check_path(repo, p, rev) for p in paths) if f]
    for f in findings:
        print("%s: %s" % (f["path"], f["why"]))
    if findings:
        print("\n%d of %d path(s) have line endings that differ from their blob - `git diff` cannot "
              "show them; rewrite with `newline=\"\\n\"` (or a binary-mode write) and re-check."
              % (len(findings), len(paths)))
        return 1
    print("%d path(s) checked, all matching their blob's line endings" % len(paths))
    return 0


def cmd_check(args) -> int:
    if args.blob:
        repo = args.repo or Git().toplevel() or os.getcwd()
        return check_blobs(repo, args.paths, args.rev)
    if args.paths:
        raise SystemExit("edit: check takes paths only with --blob")
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
    ap = argparse.ArgumentParser(description=(__doc__ or "").split("\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("replace", help="exact-text replace that preserves the file's line endings")
    r.add_argument("file")
    old = r.add_mutually_exclusive_group(required=True)
    old.add_argument("--old-file", help="file holding the text to find (LF or CRLF)")
    old.add_argument("--old", help="the text to find, with C-style escapes (\\n, \\t, \\xHH), in one argument")
    new = r.add_mutually_exclusive_group(required=True)
    new.add_argument("--new-file", help="file holding the replacement text")
    new.add_argument("--new", help="the replacement, with C-style escapes, in one argument")
    r.add_argument("--count", type=int, default=1, help="exact number of matches to require (default 1)")
    r.set_defaults(fn=cmd_replace)
    n = sub.add_parser("normalise", help="rewrite files to LF, reporting the CRLF/mixed ones")
    n.add_argument("files", nargs="+")
    n.set_defaults(fn=cmd_normalise)
    c = sub.add_parser("check", help="tracked files whose working-tree endings differ from the index")
    c.add_argument("--fix", action="store_true", help="normalise them to LF")
    c.add_argument("--blob", action="store_true",
                   help="compare each PATH's bytes with its blob instead (default: every path `git status` names)")
    c.add_argument("paths", nargs="*", help="with --blob: the files to check")
    c.add_argument("--rev", default=":", help="with --blob: `:` the index (default) or `HEAD`")
    c.add_argument("--repo", default=None, help="with --blob: the repository to run git in (default: this tree)")
    c.set_defaults(fn=cmd_check)
    args = ap.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
