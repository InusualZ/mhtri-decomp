"""lib.text: endings, the byte-exact replace, atomic writes, Transaction and the anchors."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile
from pathlib import Path

from tools.lib import testing, text

TIER = "fixture"


def test_endings(c):
    c.check("classes", [text.endings(d) for d in (b"a\nb\n", b"a\r\nb\r\n", b"a\r\nb\n", b"a\rb\r", b"ab", b"a\rb\n")],
            ["lf", "crlf", "mixed", "cr", "none", "mixed"])
    c.check("lone_cr=False ignores a bare CR (edit.py's reading)",
            [text.endings(d, lone_cr=False) for d in (b"a\rb\r", b"a\rb\n", b"a\r\nb\r")], ["none", "lf", "crlf"])
    c.check("dominant picks the majority", (text.dominant(b"a\r\nb\r\nc\n"), text.dominant(b"a\nb\nc\r\n"),
                                            text.dominant(b"x")), (b"\r\n", b"\n", b"\n"))
    c.check("to_ending normalises both ways", (text.to_ending(b"a\r\nb\n", b"\r\n"), text.to_lf(b"a\r\nb\r\n")),
            (b"a\r\nb\r\n", b"a\nb\n"))
    c.check("line_ending / with_ending on text", (text.line_ending("a\r\nb"), text.line_ending("a\nb"),
                                                  text.with_ending("a\r\nb\n", "\r\n")), ("\r\n", "\n", "a\r\nb\r\n"))
    c.check("ending_counts counts CRLF apart from a bare LF and a lone CR",
            text.ending_counts(b"a\r\nb\nc\r\nd\re"), {"crlf": 2, "lf": 1, "lone_cr": 1})
    c.check("... and nothing in a file with no break", text.ending_counts(b"ab"), {"crlf": 0, "lf": 0, "lone_cr": 0})


def test_c_escapes(c):
    c.check("c_escape: newline, quote, backslash, control, printable",
            [text.c_escape(s) for s in ("a\nb", 'say "hi"', "C:\\x", "\x01", "hello world")],
            ["a\\nb", 'say \\"hi\\"', "C:\\\\x", "\\x01", "hello world"])
    c.check("c_unescape: \\n, \\x41, \\101 (octal), \\t\\r",
            [text.c_unescape(s) for s in ("a\\nb", "\\x41", "\\101", "\\t\\r")], [b"a\nb", b"A", b"A", b"\t\r"])
    c.check("an unknown escape and a bare \\x stay literal",
            (text.c_unescape("a\\qb"), text.c_unescape("\\xg")), (b"a\\qb", b"\\xg"))
    c.check("a trailing backslash is kept", text.c_unescape("a\\"), b"a\\")
    c.check("the round trip is exact", text.c_unescape(text.c_escape("line1\nline2\t\"q\"\\")),
            b"line1\nline2\t\"q\"\\")


def test_replace_bytes(c):
    crlf = b"int a;\r\nint b;\r\nint c;\r\n"
    out, lines = text.replace_bytes(crlf, b"int a;\nint b;\n", b"int A;\nint B;\n")
    c.check("an LF needle matches a CRLF file and the replacement takes CRLF", out, b"int A;\r\nint B;\r\nint c;\r\n")
    c.check("... at line 1", lines, [1])
    mixed = b"x\r\ny\nx\ny\n"
    out, lines = text.replace_bytes(mixed, b"x\ny", b"X\nY", count=2)
    c.check("a mixed file keeps each region's own ending", out, b"X\r\nY\nX\nY\n")
    c.check("... two matches, lines 1 and 3", lines, [1, 3])
    out, _ = text.replace_bytes(b"a = 1;\r\nb = 2;\r\n", b"1", b"one\ntwo")
    c.check("a span with no break takes the dominant ending", out, b"a = one\r\ntwo;\r\nb = 2;\r\n")
    err = c.raises("zero matches refuse", text.MatchCountError, text.replace_bytes, crlf, b"nope", b"x")
    c.check("... naming no line", err and err.lines, [])
    err = c.raises("more than count refuse", text.MatchCountError, text.replace_bytes, crlf, b"int", b"x")
    c.check("... naming every line", err and err.lines, [1, 2, 3])
    c.raises("an empty needle refuses", ValueError, text.replace_bytes, crlf, b"", b"x")


def test_atomic_write(c):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "sub", "f.txt")
        text.atomic_write(p, b"a\r\nb")
        c.check("bytes are written exactly, directories created", Path(p).read_bytes(), b"a\r\nb")
        text.atomic_write(p, "x\ny\n")
        c.check("a str is UTF-8 with no newline translation", Path(p).read_bytes(), b"x\ny\n")
        text.atomic_write(p, b"z", append=True)
        c.check("append appends", Path(p).read_bytes(), b"x\ny\nz")
        c.check("no temp file is left", sorted(os.listdir(os.path.dirname(p))), ["f.txt"])
        q = os.path.join(tmp, "new.txt")
        text.atomic_write(q, b"n", append=True)
        c.check("append to a missing file creates it", Path(q).read_bytes(), b"n")


def test_transaction_restores_exactly(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        a, b = root / "a.txt", root / "deep" / "dir" / "b.txt"
        a.write_bytes(b"one\r\n")

        def flaky(src, dst):
            if Path(dst).name == "b.txt":
                raise OSError("injected failure")
            os.replace(src, dst)

        created = root / "made" / "c.txt"
        tx = text.Transaction(rename=flaky)
        try:
            tx.write(a, "one\r\ntwo\r\n")
            tx.write(created, "c\n")
            tx.write(b, b"b\r\n")
        except OSError:
            tx.rollback()
        finally:
            tx.cleanup()
        c.check("the first file is restored byte for byte", a.read_bytes(), b"one\r\n")
        c.check("a file the transaction created is removed, with the directory it made",
                (created.exists(), created.parent.exists()), (False, False))
        c.check("the failed one was never created, nor its directories", (b.exists(), (root / "deep").exists()),
                (False, False))
        c.check("no temp files left", sorted(str(p) for p in root.rglob("*" + text.TMP_SUFFIX)), [])
        tx = text.Transaction()
        tx.write(a, "new\n")
        tx.cleanup()
        c.check("a clean transaction commits", a.read_bytes(), b"new\n")


def test_anchors(c):
    conf = "config.libs = [\r\n]\r\n"
    c.check("missing_anchors names what is absent", text.missing_anchors(conf, ["config.libs = [", "nope"]), ["nope"])
    c.raises("a missing anchor raises before any write", text.AnchorError, text.insert_after_anchor, conf, "nope", "x\n")
    new, ins = text.insert_after_anchor(conf, "config.libs = [", "    lib,\n", present="lib,")
    c.check("the insert lands in the text's own CRLF", (ins, "    lib,\r\n" in new, "\n" in new.replace("\r\n", "")),
            (True, True, False))
    again, ins2 = text.insert_after_anchor(new, "config.libs = [", "    lib,\n", present="lib,")
    c.check("present= makes it idempotent", (again, ins2), (new, False))
    base = "main/foo.c:\n\t.text start:0x1000 end:0x1100\n"
    blk = ("auto/a.c", "auto/a.c:\n\t.text start:0x2000 end:0x2100\n")
    out, n = text.append_blocks(base, [blk])
    out2, n2 = text.append_blocks(out, [blk])
    c.check("append_blocks appends once", (n, n2, out2.count("auto/a.c:")), (1, 0, 1))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
