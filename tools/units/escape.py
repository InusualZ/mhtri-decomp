#!/usr/bin/env python3
"""Write C string literals and source text from the command line **without a heredoc**.

    python tools/units/escape.py --escape "raw text"          # -> the C string literal body
    python tools/units/escape.py --bytes "line1\\nline2"       # -> the exact bytes
    python tools/units/escape.py --write OUT.txt "a\\nb"       # write the exact bytes to a file
    python tools/units/escape.py --write OUT.txt --append "a"  # append instead of replace
    python tools/units/escape.py --edit FILE --old "a\\nb" --new "a\\tc" [--count N]
    python tools/units/escape.py --selftest

**Why this exists (the paste/heredoc hazard).** Three separate incidents in one day: a shell heredoc
silently ate a `\\n` in a rewritten C string literal and left a broken comment line in a landed file; a
lane's report had to be appended three times because bash truncated a long heredoc; and a working-tree
edit replaced two real source lines with a stray `L`. A heredoc is a second parser between you and the
bytes, and it is not the one that reads them.

This tool replaces it: pass the text as **one quoted argument with C-style escapes** (`\\n`, `\\t`,
`\\x41`, ...), and it writes the bytes itself. `--escape` is the other direction - type raw text and get
the literal body when you have to paste one. `--edit` is the safe rewrite: it operates on **bytes**,
reports the match count, and refuses when the count is not the one you asserted (the "assert the match
count" pattern) - so a replacement that matched zero or two places writes nothing instead of corrupting
the file.

The bytes are only ever Python `bytes`; nothing goes through a shell.
"""

from __future__ import annotations

import argparse
import os
import sys

# The two characters that always need escaping inside a C string literal, plus the control characters
# with a short spelling. Everything else non-printable becomes `\xHH`.
SHORT = {"\\": "\\\\", '"': '\\"', "\n": "\\n", "\t": "\\t", "\r": "\\r",
         "\a": "\\a", "\b": "\\b", "\f": "\\f", "\v": "\\v"}
UNSHORT = {"n": b"\n", "t": b"\t", "r": b"\r", "a": b"\a", "b": b"\b", "f": b"\f", "v": b"\v",
           "\\": b"\\", '"': b'"', "'": b"'", "0": b"\0"}
OCTAL = "01234567"


def encode(text: str) -> str:
    """`raw text` -> a C string literal **body** (no surrounding quotes) that decodes back to it."""
    out = []
    for ch in text:
        if ch in SHORT:
            out.append(SHORT[ch])
        elif 0x20 <= ord(ch) < 0x7F:
            out.append(ch)
        else:
            out.append("\\x%02x" % ord(ch))
    return "".join(out)


def decode(text: str) -> bytes:
    """A C string literal body -> the exact bytes (`\\n`, `\\t`, `\\xHH`, `\\NNN` all understood).

    An unknown escape is kept as the two literal characters it is - never silently dropped - so a typo
    surfaces in the output instead of vanishing.
    """
    out = bytearray()
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch != "\\" or i + 1 >= n:
            out += ch.encode("utf-8")
            i += 1
            continue
        nxt = text[i + 1]
        if nxt in UNSHORT:
            out += UNSHORT[nxt]
            i += 2
        elif nxt == "x":
            j = i + 2
            while j < n and j < i + 4 and text[j] in "0123456789abcdefABCDEF":
                j += 1
            if j == i + 2:
                out += b"\\x"          # a bare `\x`: keep it, do not eat the following character
                i += 2
            else:
                out.append(int(text[i + 2:j], 16) & 0xFF)
                i = j
        elif nxt in OCTAL:
            j = i + 1
            while j < n and j < i + 4 and text[j] in OCTAL:
                j += 1
            out.append(int(text[i + 1:j], 8) & 0xFF)
            i = j
        else:
            out += ("\\" + nxt).encode("utf-8")
            i += 2
    return bytes(out)


def atomic_write(path: str, data: bytes, append: bool = False) -> None:
    """Write `data` to `path` (byte-exact); a temp file + `os.replace` so a crash leaves one or the other."""
    parent = os.path.dirname(os.path.abspath(path))
    if parent:
        os.makedirs(parent, exist_ok=True)
    if append and os.path.exists(path):
        with open(path, "ab") as fh:
            fh.write(data)
        return
    tmp = path + ".tmp"
    with open(tmp, "wb") as fh:
        fh.write(data)
    os.replace(tmp, path)


def edit(path: str, old: bytes, new: bytes, count: int = 1) -> int:
    """Byte-level replace in `path`, asserting `count` matches. Returns the matches replaced."""
    if not old:
        raise SystemExit("--old is empty; refusing to replace every position")
    with open(path, "rb") as fh:
        data = fh.read()
    found = data.count(old)
    if found != count:
        raise SystemExit("REFUSED: %s contains %d match(es) of the old text, not the %d asserted - "
                         "nothing written (re-run with --count %d if that is right)"
                         % (path, found, count, found))
    atomic_write(path, data.replace(old, new))
    return found


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
