#!/usr/bin/env python3
"""Report a working-tree file whose line endings differ from the blob git actually stores.

    python tools/units/checklf.py src/Pl/pl_act.cpp src/g3d/fn_8005AA28.cpp
    python tools/units/checklf.py                    # every path `git status` names
    python tools/units/checklf.py --rev HEAD <paths> # compare against HEAD instead of the index
    python tools/units/checklf.py --selftest

**Why this tool exists.** `.gitattributes` here is `* text=auto eol=lf`, and `git add` normalises on the
way in - so a lane that wrote a file with Python in **text mode** (the default `newline=None` turns every
`\\n` into `\\r\\n` on this host) can leave four sources CRLF in the working tree while the *blob* stays LF.
`git diff` then shows nothing at all (measured: after `git add`, `git status`, `git diff` and
`git diff --cached` are all empty while `f.py` on disk is `a\\r\\n`), so the file is committed, reviewed
and built as if nothing happened - until a tool whose anchor is written with `\\n` fails to match its own
file. Nothing in the tree compared the two, so this does.

What it compares, per path: the **working-tree bytes** against the **blob** (`git show :<path>` - the
index, what a commit would store; `--rev HEAD` for the committed blob), by line-ending **style**: CRLF
or a lone CR present on one side and not the other. A path whose line-ending style differs from its
blob's is reported; a pure content change (a different LF count on both sides) is not. A path git does
not know is compared against the repository's convention (LF)
instead. Lone CR (`\\r` with no `\\n`) is reported because a tool that splits on
`\\n` sees a mid-line return.

Exit status is the answer: 0 nothing differs, 1 at least one path differs, 2 a usage error.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def git(repo: str, *args: str) -> tuple[int, bytes, str]:
    """Run `git -C repo <args>`; `(returncode, stdout bytes, stderr text)`, never raising."""
    p = subprocess.run(["git", "-C", repo, *args], capture_output=True)
    return p.returncode, p.stdout, (p.stderr or b"").decode("utf-8", "replace")


def blob_of(repo: str, path: str, rev: str = ":") -> bytes | None:
    """The blob git stores for `path` at `rev` (`:` is the index), or None when git does not know it."""
    rc, out, _err = git(repo, "show", "%s%s" % (rev, path.replace(os.sep, "/")))
    return out if rc == 0 else None


def endings(data: bytes) -> dict:
    """`{"crlf", "lone_cr", "lf"}` counts for a byte string; `lf` is a bare `\\n`, not part of a CRLF."""
    crlf = data.count(b"\r\n")
    lone_cr = len(re.findall(rb"\r(?!\n)", data))
    return {"crlf": crlf, "lone_cr": lone_cr, "lf": data.count(b"\n") - crlf}


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
    """A finding for `path`, or None when its working-tree line-ending **style** matches the blob's.

    The comparison is by style, not by count: a file whose content legitimately changed has a different
    LF count from its blob and that is not a line-ending defect. What is a defect is a working tree that
    carries CRLF (or a lone CR) where the blob does not, or the reverse. `{"path", "working", "blob",
    "why"}` - `blob` is a description, or None for a path git does not know (untracked, judged against
    the repository's LF convention).
    """
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
    style_working = (bool(we["crlf"]), bool(we["lone_cr"]))
    style_blob = (bool(be["crlf"]), bool(be["lone_cr"]))
    if style_working == style_blob:
        return None
    why = "the working tree has %s; the blob has %s" % (_describe(we), _describe(be))
    if we["crlf"] and not be["crlf"]:
        why += (" - git normalises it away on `git add`, so `git diff` shows nothing and the CRLF is "
                "committed, reviewed and built as if it were not there")
    elif be["crlf"] and not we["crlf"]:
        why += " - the committed blob itself carries CRLF, which `.gitattributes` (eol=lf) forbids"
    return {"path": path, "working": _describe(we), "blob": _describe(be), "why": why}


def status_paths(repo: str) -> list[str]:
    """Every path `git status --porcelain` names (modified, added, untracked), repo-relative."""
    rc, out, _err = git(repo, "status", "--porcelain")
    if rc != 0:
        return []
    paths = []
    for line in out.decode("utf-8", "replace").splitlines():
        entry = line[3:].strip()
        if " -> " in entry:                       # a rename: judge the destination
            entry = entry.split(" -> ", 1)[1]
        if entry and not entry.startswith('"'):
            paths.append(entry)
    return paths


def selftest() -> int:
    import checklf_selftest
    return checklf_selftest.selftest()


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("paths", nargs="*", help="files to check (default: every path `git status` names)")
    ap.add_argument("--rev", default=":", help="the blob to compare against: `:` the index (default), "
                                               "or `HEAD`")
    ap.add_argument("--repo", default=ROOT, help="the repository to run git in (default: this tree)")
    ap.add_argument("--selftest", action="store_true", help="run the selftest and exit")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    paths = args.paths or status_paths(args.repo)
    findings = [f for f in (check_path(args.repo, p, args.rev) for p in paths) if f]
    for f in findings:
        print("%s: %s" % (f["path"], f["why"]))
    if findings:
        print("\n%d of %d path(s) have line endings that differ from their blob - `git diff` cannot "
              "show them; rewrite with `newline=\"\\n\"` (or a binary-mode write) and re-check."
              % (len(findings), len(paths)))
        return 1
    print("%d path(s) checked, all matching their blob's line endings" % len(paths))
    return 0


if __name__ == "__main__":
    sys.exit(main())
