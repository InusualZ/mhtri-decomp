#!/usr/bin/env python3
"""Write C string literals and exact bytes from one quoted argument, without a heredoc; `--edit` is `edit.py replace`.
Spec: docs/tools/spec/escape.md. CLI: escape.py --escape T | --bytes T | --write FILE [--append] T | --edit FILE --old T --new T [--count N] | --selftest."""

from __future__ import annotations
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))

import argparse
import os
import sys

from tools.lib import text as libtext


def encode(text: str) -> str:
    """`raw text` -> a C string literal **body** (`lib.text.c_escape`)."""
    return libtext.c_escape(text)


def decode(text: str) -> bytes:
    """A C string literal body -> the exact bytes (`lib.text.c_unescape`; an unknown escape stays literal)."""
    return libtext.c_unescape(text)


def atomic_write(path: str, data: bytes, append: bool = False) -> None:
    libtext.atomic_write(path, data, append=append)


def edit(path: str, old: bytes, new: bytes, count: int = 1) -> int:
    """Replace `old` in `path`, asserting `count` matches (`lib.text.replace_bytes`: the needle matches across
    `\\n` or `\\r\\n` and the replacement takes the ending of the span it replaces - `edit.py replace`'s rule)."""
    if not old:
        raise SystemExit("--old is empty; refusing to replace every position")
    with open(path, "rb") as fh:
        data = fh.read()
    try:
        after, lines = libtext.replace_bytes(data, old, new, count)
    except libtext.MatchCountError as exc:
        found = len(exc.lines)
        raise SystemExit("REFUSED: %s contains %d match(es) of the old text, not the %d asserted - "
                         "nothing written (re-run with --count %d if that is right)"
                         % (path, found, count, found)) from None
    atomic_write(path, after)
    return len(lines)


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    # --- the two directions -----------------------------------------------------------------------
    check("encode escapes a newline", encode("a\nb"), "a\\nb")
    check("encode escapes a quote", encode('say "hi"'), 'say \\"hi\\"')
    check("encode escapes a backslash", encode("C:\\x"), "C:\\\\x")
    check("encode leaves printable text alone", encode("hello world"), "hello world")
    check("encode hexes a control char", encode("\x01"), "\\x01")
    check("decode turns \\n into one LF", decode("a\\nb"), b"a\nb")
    check("decode turns \\x41 into A", decode("\\x41"), b"A")
    check("decode turns \\101 into A (octal)", decode("\\101"), b"A")
    check("decode handles \\t and \\r", decode("\\t\\r"), b"\t\r")
    check("decode keeps an unknown escape literal", decode("a\\qb"), b"a\\qb")
    check("round trip", decode(encode("line1\nline2\t\"q\"\\")), b"line1\nline2\t\"q\"\\")

    # --- the filed hazard: a heredoc ate the `\n` in a C string literal ---------------------------
    with _tempdir() as tmp:
        target = os.path.join(tmp, "out.txt")
        atomic_write(target, decode('printf("a\\nb");\\n'))
        with open(target, "rb") as fh:
            data = fh.read()
        check("the written file carries a real LF", data, b'printf("a\nb");\n')
        check("... and not the two characters \\n", b"\\n" in data, False)

        # --- the match-count assertion (`--edit`) -------------------------------------------------
        path = os.path.join(tmp, "src.c")
        atomic_write(path, b"int a = X;\nint b = X;\n")
        check("edit refuses the wrong count", _raises(lambda: edit(path, b"X", b"Y", count=1)), True)
        with open(path, "rb") as fh:
            check("... and writes nothing", fh.read(), b"int a = X;\nint b = X;\n")
        check("edit replaces when the count matches", edit(path, b"X", b"Y", count=2), 2)
        with open(path, "rb") as fh:
            check("... byte-exactly", fh.read(), b"int a = Y;\nint b = Y;\n")
        check("edit refuses zero matches", _raises(lambda: edit(path, b"ZZZ", b"Y", count=1)), True)
        check("edit refuses an empty old", _raises(lambda: edit(path, b"", b"Y", count=1)), True)
        crlf = os.path.join(tmp, "crlf.c")
        atomic_write(crlf, b"int a;\r\nint b;\r\n")
        check("edit matches an LF needle in a CRLF file (edit.py replace's rule)",
              edit(crlf, b"int a;\nint b;\n", b"int a;\nint c;\n"), 1)
        with open(crlf, "rb") as fh:
            check("... and the replacement keeps the file's CRLF", fh.read(), b"int a;\r\nint c;\r\n")

    # --- the CLI, as a lane types it --------------------------------------------------------------
    import contextlib
    import io
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = main(["--escape", 'a\nb"c'])
    check("cli --escape exits 0", rc, 0)
    check("cli --escape prints the literal body", buf.getvalue(), 'a\\nb\\"c\n')
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = main(["--bytes", "a\\nb"])
    check("cli --bytes prints the bytes", (rc, buf.getvalue()), (0, "a\nb"))
    with _tempdir() as tmp:
        out = os.path.join(tmp, "sub", "lit.txt")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--write", out, "a\\nb"])
        check("cli --write exits 0", rc, 0)
        with open(out, "rb") as fh:
            check("cli --write writes the decoded bytes", fh.read(), b"a\nb")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--write", out, "--append", "c"])
        with open(out, "rb") as fh:
            check("cli --append appends", (rc, fh.read()), (0, b"a\nbc"))
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = main(["--edit", out, "--old", "a\\nb", "--new", "A\\nB"])
        with open(out, "rb") as fh:
            check("cli --edit replaces", (rc, fh.read()), (0, b"A\nBc"))
        # a bad count is a refusal with a non-zero exit, and the bytes do not move
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf):
            rc = _rc_of(lambda: main(["--edit", out, "--old", "nope", "--new", "x"]))
        check("cli --edit refuses a bad count", rc, 1)
        with open(out, "rb") as fh:
            check("... leaving the file", fh.read(), b"A\nBc")

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


class _tempdir:
    """`tempfile.TemporaryDirectory` without the extra import at module scope."""

    def __enter__(self):
        import tempfile
        self._t = tempfile.TemporaryDirectory()
        return self._t.name

    def __exit__(self, *exc):
        self._t.cleanup()
        return False


def _raises(fn) -> bool:
    try:
        fn()
        return False
    except SystemExit:
        return True


def _rc_of(fn) -> int:
    try:
        return fn() or 0
    except SystemExit as exc:
        return exc.code if isinstance(exc.code, int) else 1


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=(__doc__ or "").splitlines()[0])
    ap.add_argument("text", nargs="?", help="the text, with C-style escapes, in ONE quoted argument")
    ap.add_argument("--escape", action="store_true", help="encode TEXT into a C string literal body")
    ap.add_argument("--bytes", action="store_true", help="decode TEXT and print the exact bytes")
    ap.add_argument("--write", default=None, metavar="FILE", help="decode TEXT into FILE (byte-exact)")
    ap.add_argument("--append", action="store_true", help="with --write: append instead of replace")
    ap.add_argument("--edit", default=None, metavar="FILE", help="byte-level replace in FILE")
    ap.add_argument("--old", default=None, help="with --edit: the text to replace")
    ap.add_argument("--new", default=None, help="with --edit: the replacement")
    ap.add_argument("--count", type=int, default=1, help="with --edit: the exact match count asserted")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)

    if args.selftest:
        return selftest()
    if args.edit:
        if args.old is None or args.new is None:
            ap.error("--edit needs both --old and --new")
        found = edit(args.edit, decode(args.old), decode(args.new), count=args.count)
        print("edited %s: replaced %d match(es)" % (args.edit, found))
        return 0
    if args.write:
        if args.text is None:
            ap.error("--write needs the text as one quoted argument")
        atomic_write(args.write, decode(args.text), append=args.append)
        print("wrote %d byte(s) to %s" % (len(decode(args.text)), args.write))
        return 0
    if args.escape:
        if args.text is None:
            ap.error("--escape needs the text")
        sys.stdout.write(encode(args.text) + "\n")
        return 0
    if args.bytes:
        if args.text is None:
            ap.error("--bytes needs the text")
        data = decode(args.text)
        stream = getattr(sys.stdout, "buffer", None)
        if stream is not None:
            stream.write(data)
        else:                       # a redirected text stream (the selftest): bytes via latin-1
            sys.stdout.write(data.decode("latin-1"))
        return 0
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
