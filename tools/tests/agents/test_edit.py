"""edit.py: replace/normalise/check on LF, CRLF and mixed files, `replace --old/--new` (escape's `--edit`) and
`check --blob` (checklf's per-path check), each through the real CLI in a temp dir."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import subprocess
import tempfile

from tools.lib import testing

TIER = "fixture"
EDIT = str(pathlib.Path(__file__).resolve().parents[2] / "agents" / "edit.py")


def _put(path, data: bytes):
    with open(path, "wb") as fh:
        fh.write(data)


def _get(path) -> bytes:
    with open(path, "rb") as fh:
        return fh.read()


def _run(*args, cwd):
    p = subprocess.run([sys.executable, EDIT, *args], capture_output=True, cwd=cwd, env=testing._git_env())
    return p.returncode, (p.stdout + p.stderr).decode("utf-8", "replace")


def _replace(tmp, content: bytes, old: bytes, new: bytes, *extra):
    f, a, b = (os.path.join(tmp, n) for n in ("f.c", "old.txt", "new.txt"))
    _put(f, content)
    _put(a, old)
    _put(b, new)
    rc, out = _run("replace", f, "--old-file", a, "--new-file", b, *extra, cwd=tmp)
    return rc, out, _get(f)


def test_replace(c):
    with tempfile.TemporaryDirectory() as tmp:
        old, new = b"int a;\nint b;\n", b"int a;\nint c;\nint d;\n"
        rc, out, got = _replace(tmp, b"x\nint a;\nint b;\ny\n", old, new)
        c.check("LF file: replaced, stays LF", (rc, got), (0, b"x\nint a;\nint c;\nint d;\ny\n"))
        c.check("... prints a unified diff", "+int d;" in out and "--- a/" in out, True)
        rc, out, got = _replace(tmp, b"x\r\nint a;\r\nint b;\r\ny\r\n", old, new)
        c.check("CRLF file, LF needle: matched, result stays CRLF", (rc, got),
                (0, b"x\r\nint a;\r\nint c;\r\nint d;\r\ny\r\n"))
        rc, out, got = _replace(tmp, b"x\r\nint a;\r\nint b;\r\ny\r\n", old.replace(b"\n", b"\r\n"),
                                new.replace(b"\n", b"\r\n"))
        c.check("CRLF file, CRLF needle", (rc, got), (0, b"x\r\nint a;\r\nint c;\r\nint d;\r\ny\r\n"))
        mixed = b"top\nint a;\r\nint b;\r\nmid\nint a;\nint b;\nend\r\n"
        rc, out, got = _replace(tmp, mixed, old, new, "--count", "2")
        c.check("mixed file: each region keeps its own ending, the rest is byte-identical", (rc, got),
                (0, b"top\nint a;\r\nint c;\r\nint d;\r\nmid\nint a;\nint c;\nint d;\nend\r\n"))
        rc, out, got = _replace(tmp, mixed, old, new)
        c.check("two matches with the default count: refused, nothing written",
                (rc != 0, got, "line(s) 2, 5" in out), (True, mixed, True))
        rc, out, got = _replace(tmp, b"x\r\ny\r\n", old, new)
        c.check("no match: refused, nothing written", (rc != 0, got, "found 0" in out), (True, b"x\r\ny\r\n", True))
        rc, out, got = _replace(tmp, b"a = 1;\r\nb = 2;\r\n", b"a = 1;", b"a = 10;")
        c.check("single-line span keeps the file's ending", (rc, got), (0, b"a = 10;\r\nb = 2;\r\n"))
        rc, out, got = _replace(tmp, b"x\nk\n", b"k\n", b"")
        c.check("an empty replacement deletes the span", (rc, got), (0, b"x\n"))
        rc, out, got = _replace(tmp, "a é\r\nb\r\n".encode("utf-8"), b"b\n", b"c\n")
        c.check("UTF-8 bytes are untouched", (rc, got), (0, "a é\r\nc\r\n".encode("utf-8")))


def test_replace_inline_text(c):
    """`--old/--new` take C-style escapes in one argument - what `escape.py --edit` offered."""
    with tempfile.TemporaryDirectory() as tmp:
        f = os.path.join(tmp, "f.c")
        _put(f, b"x\r\nint a;\r\nint b;\r\n")
        rc, out = _run("replace", f, "--old", "int a;\\nint b;", "--new", "int a;\\n\\tint c;", cwd=tmp)
        c.check("an escaped LF needle matches a CRLF file and the result keeps CRLF", (rc, _get(f)),
                (0, b"x\r\nint a;\r\n\tint c;\r\n"))
        rc, out = _run("replace", f, "--old", "nope", "--new", "x", cwd=tmp)
        c.check("... a missing needle is refused, nothing written", (rc != 0, _get(f)),
                (True, b"x\r\nint a;\r\n\tint c;\r\n"))
        rc, out = _run("replace", f, "--old", "x", "--old-file", f, "--new", "y", cwd=tmp)
        c.check("--old and --old-file are exclusive (usage error)", rc, 2)
        rc, out = _run("replace", f, "--new", "y", cwd=tmp)
        c.check("... and one of them is required", rc, 2)


def test_normalise(c):
    with tempfile.TemporaryDirectory() as tmp:
        crlf, lf, mx, binary = (os.path.join(tmp, n) for n in ("c.txt", "l.txt", "m.txt", "b.bin"))
        _put(crlf, b"a\r\nb\r\n")
        _put(lf, b"a\nb\n")
        _put(mx, b"a\r\nb\nc\r\n")
        _put(binary, b"\0\r\n\0")
        rc, out = _run("normalise", crlf, lf, mx, binary, cwd=tmp)
        c.check("normalise: CRLF and mixed become LF", (_get(crlf), _get(lf), _get(mx)),
                (b"a\nb\n", b"a\nb\n", b"a\nb\nc\n"))
        c.check("... a binary file is skipped", _get(binary), b"\0\r\n\0")
        c.check("... and the report names which were CRLF/mixed",
                ("was crlf" in out, "was mixed" in out, "already LF" in out, "skip (binary)" in out), (True,) * 4)


def test_check_index(c):
    with testing.GitFixture() as fx:
        fx.init()
        repo = str(fx.root)
        _put(os.path.join(repo, ".gitattributes"), b"* text=auto eol=lf\n")
        _put(os.path.join(repo, "good.c"), b"a\nb\n")
        _put(os.path.join(repo, "bad.c"), b"a\r\nb\r\n")
        _put(os.path.join(repo, "mix.c"), b"a\r\nb\nc\n")
        fx.git("add", "-A")
        rc, out = _run("check", cwd=repo)
        c.check("check: lists the CRLF and mixed files, not the LF one",
                (rc, "bad.c" in out, "mix.c" in out, "good.c" in out), (1, True, True, False))
        rc, out = _run("check", "--fix", cwd=repo)
        c.check("check --fix: normalises them", (_get(os.path.join(repo, "bad.c")), _get(os.path.join(repo, "mix.c"))),
                (b"a\nb\n", b"a\nb\nc\n"))
        c.check("check afterwards: clean", _run("check", cwd=repo)[0], 0)
        c.check("paths without --blob are a refusal", _run("check", "good.c", cwd=repo)[0] != 0, True)


def test_check_blob(c):
    """A CRLF working tree over an LF blob is invisible to `git diff`; `check --blob` names it."""
    with testing.GitFixture() as fx:
        fx.init()
        repo = str(fx.root)
        fx.commit({".gitattributes": b"* text=auto eol=lf\n", "f.py": b"import os\nprint(1)\n"}, "init")
        c.check("an LF working tree over an LF blob is clean", _run("check", "--blob", "f.py", cwd=repo)[0], 0)
        _put(os.path.join(repo, "f.py"), b"import os\r\nprint(1)\r\n")
        fx.git("add", "-A")
        c.check("git diff and diff --cached are empty over the CRLF file (the invisibility)",
                (fx.git("diff"), fx.git("diff", "--cached")), ("", ""))
        rc, out = _run("check", "--blob", "f.py", cwd=repo)
        c.check("check --blob reports it: exit 1, the CRLF count, why git cannot show it",
                (rc, "2 CRLF" in out, "git normalises it away" in out), (1, True, True))
        _put(os.path.join(repo, "new.py"), b"x = 1\r\n")
        rc, out = _run("check", "--blob", cwd=repo)
        c.check("... with no path it checks every path `git status` names (an untracked CRLF file is judged "
                "against LF)", (rc, "new.py: the path is not in git" in out), (1, True))
        rc, out = _run("check", "--blob", "--repo", repo, "--rev", "HEAD", "f.py", cwd=tempfile.gettempdir())
        c.check("--repo and --rev HEAD are honoured", (rc, "2 CRLF" in out), (1, True))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
