"""movehdr on a temp git repo: the dry-run plan writes nothing; the apply moves, rewrites the exception's includers and
switches configure.py; a second run is a no-op; a collision, a case clash and an include that would change target
(the mutation: a header beside an includer that shadows the moved root header) are refused with nothing written."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import os

from tools.lib import testing
from tools.units import movehdr

TIER = "fixture"

CONFIGURE = ('config.asflags = [\n    "-I include",\n]\ncflags_base = [\n    "-i include",\n'
             '    f"-i build/{config.version}/include",\n]\n')
FILES = {
    "configure.py": CONFIGURE,
    "include/types.h": "typedef int s32;\n",
    "include/Net/net.h": '#include "types.h"\nstruct Net { s32 x; };\n',
    "include/lib/g/mat.h": '#include "types.h"\nvoid mat(void);\n',
    "src/Net/net.cpp": '#include "Net/net.h"\n#include "lib/g/mat.h"\nvoid f(void) {}\n',
    "src/g/mat.cpp": '#include "lib/g/mat.h"\r\nvoid mat(void) {}\r\n',
}
EXC = {"include/lib/g/mat.h": "src/g/mat.h"}


def _repo(extra=None):
    g = testing.GitFixture().init()
    files = dict(FILES)
    files.update(extra or {})
    g.commit(files, "base")
    return g


def _plan(g, exc=EXC):
    return movehdr.plan(str(g.root), exc)


def _run(g, *argv):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        rc = movehdr.TOOL.run(movehdr.main, ["--root", str(g.root), *argv], parser=_parser())
    return rc, out.getvalue()


def _parser():
    ap = movehdr.TOOL.parser()
    ap.add_argument("--exception", action="append", default=[])
    return ap


def test_dry_run_plans_and_writes_nothing(c):
    g = _repo()
    try:
        p = _plan(g)
        c.check("three headers move", sorted(p.moves),
                [("include/Net/net.h", "src/Net/net.h"), ("include/lib/g/mat.h", "src/g/mat.h"),
                 ("include/types.h", "src/types.h")])
        c.check("the exception's two includers are rewritten", sorted((f, l, o, n) for f, l, o, n in p.rewrites),
                [("src/Net/net.cpp", 2, "lib/g/mat.h", "g/mat.h"), ("src/g/mat.cpp", 1, "lib/g/mat.h", "g/mat.h")])
        c.check("both flags switch", p.config, [('"-i include",', '"-i src",'), ('"-I include",', '"-I src",')])
        c.check("nothing refused, every include resolved", (p.refused, p.unresolved, p.includes), (False, 0, 5))
        before = g.git("status", "--porcelain")
        rc, out = _run(g, "--dry-run", "--exception", "include/lib/g/mat.h=src/g/mat.h")
        c.check("--dry-run exits 0", rc, 0)
        c.contains("--dry-run prints the plan", out, "would move 3")
        c.check("--dry-run writes nothing", (g.git("status", "--porcelain"), (g.root / "include").is_dir()),
                (before, True))
    finally:
        g.cleanup()


def test_apply_then_idempotent(c):
    g = _repo()
    try:
        rc, _out = _run(g, "--exception", "include/lib/g/mat.h=src/g/mat.h")
        c.check("the apply exits 0", rc, 0)
        status = sorted(g.git("status", "--porcelain").splitlines())
        c.check("the moves are staged renames, the edits staged modifications", status,
                sorted(["M  configure.py", "R  include/Net/net.h -> src/Net/net.h",
                        "R  include/lib/g/mat.h -> src/g/mat.h", "R  include/types.h -> src/types.h",
                        "M  src/Net/net.cpp", "M  src/g/mat.cpp"]))
        c.check("include/ is gone", (g.root / "include").exists(), False)
        c.check("the includer now spells the exception's new path (CRLF kept)",
                (g.root / "src/g/mat.cpp").read_bytes(), b'#include "g/mat.h"\r\nvoid mat(void) {}\r\n')
        cfg = (g.root / "configure.py").read_text()
        c.check("configure.py carries -i src / -I src",
                ('"-i src",' in cfg, '"-I src",' in cfg, '"-i include"' in cfg or '"-I include"' in cfg),
                (True, True, False))
        g.git("commit", "-q", "-m", "move")
        p = _plan(g)
        c.check("a second run plans nothing", (p.moves, p.rewrites, p.config, p.refused), ([], [], [], False))
        c.check("... and reports the exception as done", p.done, [("include/lib/g/mat.h", "src/g/mat.h")])
        rc, out = _run(g, "--exception", "include/lib/g/mat.h=src/g/mat.h")
        c.check("... exits 0 saying so", (rc, "nothing to do" in out), (0, True))
    finally:
        g.cleanup()


def test_refusals(c):
    g = _repo({"src/types.h": "/* a unit-local copy */\n"})
    try:
        p = _plan(g)
        c.check("an existing destination is refused",
                (p.refused, [s for s in p.collisions if "src/types.h" in s] != []), (True, True))
        before = g.git("status", "--porcelain")
        rc, out = _run(g, "--exception", "include/lib/g/mat.h=src/g/mat.h")
        c.check("a refused plan exits 1 and writes nothing",
                (rc, "REFUSED" in out, g.git("status", "--porcelain"), (g.root / "include/types.h").is_file()),
                (1, True, before, True))
    finally:
        g.cleanup()
    g = _repo({"src/net/NET.h": "/* case clash */\n"})
    try:
        c.check("a case-insensitive clash is refused",
                [s for s in _plan(g).collisions if "Net/net.h" in s] != [], True)
    finally:
        g.cleanup()
    # the mutation: src/Net/types.h sits beside the moved net.h, so `#include "types.h"` found include/types.h before
    # (include/Net/ has none) and would find src/Net/types.h after - a changed target under the local-first order
    g = _repo({"src/Net/types.h": "typedef long s32;\n"})
    try:
        p = _plan(g)
        c.check("an include that would change target is refused",
                (p.refused, [s for s in p.changed if "include/Net/net.h:1" in s and "local-first" in s] != []),
                (True, True))
        c.check("... and the roots-first order is unaffected",
                [s for s in p.changed if "roots-first" in s], [])
    finally:
        g.cleanup()


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
