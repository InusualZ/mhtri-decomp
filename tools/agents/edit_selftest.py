#!/usr/bin/env python3
"""Self-test for `tools/agents/edit.py`: replace/normalise/check on LF, CRLF and mixed files in a temp dir.

    python tools/agents/edit_selftest.py
"""
from __future__ import annotations

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(HERE))
import edit  # noqa: E402
import unitutil  # noqa: E402

EDIT = os.path.join(HERE, "edit.py")


def _ok(label, got, want, failures):
    if got == want:
        print("ok    " + label)
        return failures
    print("FAIL  %s\n        got:  %r\n        want: %r" % (label, got, want))
    return failures + 1


def _put(path, data: bytes):
    with open(path, "wb") as fh:
        fh.write(data)


def _get(path) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _run(*args, cwd=None):
    p = subprocess.run([sys.executable, EDIT, *args], capture_output=True, cwd=cwd)
    return p.returncode, (p.stdout + p.stderr).decode("utf-8", "replace")


def _replace(tmp, content: bytes, old: bytes, new: bytes, *extra):
    f, a, b = (os.path.join(tmp, n) for n in ("f.c", "old.txt", "new.txt"))
    _put(f, content)
    _put(a, old)
    _put(b, new)
    rc, out = _run("replace", f, "--old-file", a, "--new-file", b, *extra)
    return rc, out, _get(f)


def selftest() -> int:
    failures = 0
    with unitutil.temp_dir() as tmp:
        old, new = b"int a;\nint b;\n", b"int a;\nint c;\nint d;\n"

        rc, out, got = _replace(tmp, b"x\nint a;\nint b;\ny\n", old, new)
        failures = _ok("LF file: replaced, stays LF", (rc, got), (0, b"x\nint a;\nint c;\nint d;\ny\n"), failures)
        failures = _ok("... prints a unified diff", "+int d;" in out and "--- a/" in out, True, failures)

        rc, out, got = _replace(tmp, b"x\r\nint a;\r\nint b;\r\ny\r\n", old, new)
        failures = _ok("CRLF file, LF needle: matched, result stays CRLF",
                       (rc, got), (0, b"x\r\nint a;\r\nint c;\r\nint d;\r\ny\r\n"), failures)

        rc, out, got = _replace(tmp, b"x\r\nint a;\r\nint b;\r\ny\r\n", old.replace(b"\n", b"\r\n"),
                                new.replace(b"\n", b"\r\n"))
        failures = _ok("CRLF file, CRLF needle", (rc, got), (0, b"x\r\nint a;\r\nint c;\r\nint d;\r\ny\r\n"),
                       failures)

        mixed = b"top\nint a;\r\nint b;\r\nmid\nint a;\nint b;\nend\r\n"
        rc, out, got = _replace(tmp, mixed, old, new, "--count", "2")
        failures = _ok("mixed file: each region keeps its own ending, the rest is byte-identical",
                       (rc, got), (0, b"top\nint a;\r\nint c;\r\nint d;\r\nmid\nint a;\nint c;\nint d;\nend\r\n"),
                       failures)

        rc, out, got = _replace(tmp, mixed, old, new)
        failures = _ok("two matches with the default count: refused, nothing written",
                       (rc != 0, got, "line(s) 2, 5" in out), (True, mixed, True), failures)

        rc, out, got = _replace(tmp, b"x\r\ny\r\n", old, new)
        failures = _ok("no match: refused, nothing written", (rc != 0, got, "found 0" in out),
                       (True, b"x\r\ny\r\n", True), failures)

        rc, out, got = _replace(tmp, b"a = 1;\r\nb = 2;\r\n", b"a = 1;", b"a = 10;")
        failures = _ok("single-line span keeps the file's ending", (rc, got), (0, b"a = 10;\r\nb = 2;\r\n"),
                       failures)

        rc, out, got = _replace(tmp, b"x\nk\n", b"k\n", b"")
        failures = _ok("an empty replacement deletes the span", (rc, got), (0, b"x\n"), failures)

        rc, out, got = _replace(tmp, "a é\r\nb\r\n".encode("utf-8"), b"b\n", b"c\n")
        failures = _ok("UTF-8 bytes are untouched", (rc, got), (0, "a é\r\nc\r\n".encode("utf-8")), failures)

        # normalise
        crlf, lf, mx, binary = (os.path.join(tmp, n) for n in ("c.txt", "l.txt", "m.txt", "b.bin"))
        _put(crlf, b"a\r\nb\r\n")
        _put(lf, b"a\nb\n")
        _put(mx, b"a\r\nb\nc\r\n")
        _put(binary, b"\0\r\n\0")
        rc, out = _run("normalise", crlf, lf, mx, binary)
        failures = _ok("normalise: CRLF and mixed become LF", (_get(crlf), _get(lf), _get(mx)),
                       (b"a\nb\n", b"a\nb\n", b"a\nb\nc\n"), failures)
        failures = _ok("... a binary file is skipped", _get(binary), b"\0\r\n\0", failures)
        failures = _ok("... and the report names which were CRLF/mixed",
                       ("was crlf" in out, "was mixed" in out, "already LF" in out, "skip (binary)" in out),
                       (True, True, True, True), failures)

        # check: a real git repo whose index is LF and whose working tree is not
        repo = os.path.join(tmp, "repo")
        os.makedirs(repo)
        git = ["git", "-c", "core.autocrlf=false", "-C", repo]
        subprocess.run(git + ["init", "-q"], check=True, capture_output=True)
        _put(os.path.join(repo, ".gitattributes"), b"* text=auto eol=lf\n")
        _put(os.path.join(repo, "good.c"), b"a\nb\n")
        _put(os.path.join(repo, "bad.c"), b"a\r\nb\r\n")
        _put(os.path.join(repo, "mix.c"), b"a\r\nb\nc\n")
        subprocess.run(git + ["add", "-A"], check=True, capture_output=True)
        rc, out = _run("check", cwd=repo)
        failures = _ok("check: lists the CRLF and mixed files, not the LF one",
                       (rc, "bad.c" in out, "mix.c" in out, "good.c" in out), (1, True, True, False), failures)
        rc, out = _run("check", "--fix", cwd=repo)
        failures = _ok("check --fix: normalises them", (_get(os.path.join(repo, "bad.c")),
                                                        _get(os.path.join(repo, "mix.c"))),
                       (b"a\nb\n", b"a\nb\nc\n"), failures)
        rc, out = _run("check", cwd=repo)
        failures = _ok("check afterwards: clean", rc, 0, failures)
    return failures


def main() -> int:
    failures = selftest()
    print("%s: %d failure(s)" % ("FAILED" if failures else "passed", failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
