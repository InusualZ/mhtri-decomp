"""checklf (a shim over `edit.py check --blob`): a CRLF working tree over an LF blob, in a throwaway repo."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os

from tools.agents import edit
from tools.lib import testing
from tools.units import checklf as cl

TIER = "fixture"


def _put(path, data: bytes):
    with open(path, "wb") as fh:
        fh.write(data)


def test_the_shim_is_edit(c):
    c.check("checklf's names are edit.py's one implementation",
            [getattr(cl, n) is getattr(edit, n) for n in ("check_path", "blob_of", "endings", "status_paths")],
            [True] * 4)


def test_crlf_over_lf(c):
    with testing.GitFixture() as fx:
        fx.init()
        tmp = str(fx.root)
        path = os.path.join(tmp, "f.py")
        fx.commit({".gitattributes": b"* text=auto eol=lf\n", "f.py": b"import os\nprint(1)\n"}, "init")
        c.check("an LF working tree is clean", cl.check_path(tmp, "f.py"), None)

        _put(path, b"import os\r\nprint(1)\r\n")      # the lane's accident: a text-mode write on Windows
        fx.git("add", "-A")
        c.check("git diff and diff --cached are empty over the CRLF file (the invisibility)",
                (fx.git("diff"), fx.git("diff", "--cached")), ("", ""))
        c.check("the blob is still LF", cl.blob_of(tmp, "f.py"), b"import os\nprint(1)\n")
        finding = cl.check_path(tmp, "f.py")
        c.check("check_path reports it: the file, the CRLF count, the blob's endings, why git cannot show it",
                (bool(finding), finding and finding["path"], finding and "2 CRLF" in finding["working"],
                 finding and "LF" in (finding["blob"] or ""), finding and "git normalises it away" in finding["why"]),
                (True, "f.py", True, True, True))
        c.check("... and against HEAD's blob too (`--rev HEAD`)",
                bool((cl.check_path(tmp, "f.py", "HEAD") or {}).get("blob")), True)

        _put(path, b"import os\nprint(1)\n")
        c.check("restoring LF clears the finding", cl.check_path(tmp, "f.py"), None)

        untracked = os.path.join(tmp, "new.py")
        _put(untracked, b"x = 1\r\n")
        finding = cl.check_path(tmp, "new.py")
        c.check("an untracked CRLF file is reported as not in git",
                (bool(finding), finding and "not in git" in finding["why"]), (True, True))
        _put(untracked, b"x = 1\n")
        c.check("an untracked LF file is clean", cl.check_path(tmp, "new.py"), None)

        _put(path, b"import os\rprint(1)\n")
        finding = cl.check_path(tmp, "f.py")
        c.check("a lone CR is reported as one", (bool(finding), finding and "lone CR" in finding["working"]),
                (True, True))
        e = cl.endings(b"a\r\nb\nc\r\n")
        c.check("endings counts CRLF apart from LF", (e["crlf"], e["lf"]), (2, 1))
        _put(path, b"import os\nprint(2)\nprint(3)\n")
        c.check("a changed LF file is not a line-ending finding", cl.check_path(tmp, "f.py"), None)

        _put(path, b"import os\r\nprint(1)\r\n")
        with contextlib.redirect_stdout(io.StringIO()):
            rc = cl.main(["--repo", tmp, "f.py"])
        c.check("the CLI returns 1 on a finding", rc, 1)
        _put(path, b"import os\nprint(1)\n")
        with contextlib.redirect_stdout(io.StringIO()):
            rc = cl.main(["--repo", tmp, "f.py"])
        c.check("the CLI returns 0 when clean", rc, 0)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
