#!/usr/bin/env python3
"""Selftest for `tools/units/checklf.py` - a CRLF working tree over an LF blob, in a throwaway repo.

    python tools/units/checklf_selftest.py
    python tools/units/checklf.py --selftest

A real `git init` in a temporary directory, so the mechanism is the real one and no repository state is
touched: the file is committed with LF, the working tree is rewritten with CRLF, and the selftest pins
that `git diff` and `git diff --cached` are **empty** (the invisibility) while `check_path` still names
the file, the CRLF count and the reason. A restore, an untracked file and a lone-CR file close it out.
"""
from __future__ import annotations

import importlib.util
import contextlib
import io
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CHECKLF = os.path.join(HERE, "checklf.py")

CHECKS = 0
FAILURES: list[str] = []


def check(name: str, got, want) -> None:
    global CHECKS
    CHECKS += 1
    if got != want:
        FAILURES.append("%s: got %r, want %r" % (name, got, want))


def _load():
    spec = importlib.util.spec_from_file_location("checklf_under_test", CHECKLF)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _git(repo, *args):
    p = subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    return p.returncode, p.stdout


def selftest() -> int:
    cl = _load()
    with tempfile.TemporaryDirectory() as tmp:
        _git(tmp, "init", "-q")
        _git(tmp, "config", "core.autocrlf", "false")
        with open(os.path.join(tmp, ".gitattributes"), "w", newline="") as fh:
            fh.write("* text=auto eol=lf\n")
        path = os.path.join(tmp, "f.py")
        with open(path, "wb") as fh:
            fh.write(b"import os\nprint(1)\n")
        _git(tmp, "add", "-A")
        _git(tmp, "-c", "user.email=x@y", "-c", "user.name=x", "commit", "-q", "-m", "init")

        # 1. an LF working tree over an LF blob: nothing to report
        check("an LF working tree is clean", cl.check_path(tmp, "f.py"), None)

        # 2. the lane's accident: Python text-mode write turns the tree CRLF, the blob stays LF
        with open(path, "wb") as fh:
            fh.write(b"import os\r\nprint(1)\r\n")
        _git(tmp, "add", "-A")                     # what a lane does next
        _rc, diff = _git(tmp, "diff")
        _rc, cached = _git(tmp, "diff", "--cached")
        check("git diff is empty over the CRLF file (the invisibility)", diff, "")
        check("git diff --cached is empty too", cached, "")
        check("the blob on disk is still LF", cl.blob_of(tmp, "f.py"), b"import os\nprint(1)\n")

        finding = cl.check_path(tmp, "f.py")
        check("check_path reports the CRLF working tree", bool(finding), True)
        check("... naming the file", finding and finding["path"], "f.py")
        check("... the CRLF count", finding and "2 CRLF" in finding["working"], True)
        check("... the blob's endings", finding and "LF" in (finding["blob"] or ""), True)
        check("... and why git cannot show it",
              finding and "git normalises it away" in finding["why"], True)

        # 3. restore LF: clean again (the fix is a re-write, not a git command)
        with open(path, "wb") as fh:
            fh.write(b"import os\nprint(1)\n")
        check("restoring LF clears the finding", cl.check_path(tmp, "f.py"), None)

        # 4. an untracked file with CRLF is judged against the repository's convention
        untracked = os.path.join(tmp, "new.py")
        with open(untracked, "wb") as fh:
            fh.write(b"x = 1\r\n")
        finding = cl.check_path(tmp, "new.py")
        check("an untracked CRLF file is reported", bool(finding), True)
        check("... as not in git", finding and "not in git" in finding["why"], True)
        with open(untracked, "wb") as fh:
            fh.write(b"x = 1\n")
        check("an untracked LF file is clean", cl.check_path(tmp, "new.py"), None)

        # 5. lone CR is its own defect (a `\n`-split reader sees a mid-line carriage return)
        with open(path, "wb") as fh:
            fh.write(b"import os\rprint(1)\n")
        finding = cl.check_path(tmp, "f.py")
        check("a lone CR is reported", bool(finding), True)
        check("... as a lone CR", finding and "lone CR" in finding["working"], True)

        # 6. the classifier counts CRLF and LF separately (a mixed file is not "all LF")
        e = cl.endings(b"a\r\nb\nc\r\n")
        check("endings counts CRLF apart from LF", (e["crlf"], e["lf"]), (2, 1))

        # 6b. a pure content change (different LF count, same style) is NOT a line-ending finding
        with open(path, "wb") as fh:
            fh.write(b"import os\nprint(2)\nprint(3)\n")
        check("a changed LF file is not a line-ending finding", cl.check_path(tmp, "f.py"), None)

        # 7. the CLI exit status is the verdict
        with open(path, "wb") as fh:
            fh.write(b"import os\r\nprint(1)\r\n")
        with contextlib.redirect_stdout(io.StringIO()):
            rc = cl.main(["--repo", tmp, "f.py"])
        check("the CLI returns 1 on a finding", rc, 1)
        with open(path, "wb") as fh:
            fh.write(b"import os\nprint(1)\n")
        with contextlib.redirect_stdout(io.StringIO()):
            rc = cl.main(["--repo", tmp, "f.py"])
        check("the CLI returns 0 when clean", rc, 0)

    for failure in FAILURES:
        print("FAIL " + failure)
    print("ok - %d checks" % CHECKS)
    return 1 if FAILURES else 0


if __name__ == "__main__":
    sys.exit(selftest())
