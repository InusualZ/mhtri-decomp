"""The harness tests itself: Checker's shape, FixtureTree, GitFixture and the fixture tier's live-tree refusal."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import io
import os
import runpy
import subprocess
import tempfile
import textwrap
from pathlib import Path

from tools.lib import testing
from tools.lib.testing import Checker, FixtureTree, GitFixture, LiveTreeError
from tools import selftest as runner
from tools.units import sharedfiles

TIER = "fixture"


def _fresh() -> tuple[Checker, io.StringIO]:
    out = io.StringIO()
    return Checker("inner", out=out), out


def test_checker_counts_and_prints_one_shape(c):
    k, out = _fresh()
    k.check("eq", 1, 1)
    k.expect("truthy", [1])
    k.contains("in", "abc", "b")
    k.raises("raises", ZeroDivisionError, lambda: 1 / 0)
    c.check("a green checker returns 0", k.summary(), 0)
    c.check("... and prints `ok - N checks`", out.getvalue().splitlines()[-1], "ok - 4 checks")
    c.check("... which the runner counts", runner.parse_checks(out.getvalue()), 4)

    k, out = _fresh()
    k.check("eq", 1, 2)
    k.expect("truthy", 0, "zero is false")
    k.contains("in", "abc", "z")
    k.raises("raises", ZeroDivisionError, lambda: None)
    k.raises("wrong type", KeyError, lambda: 1 / 0)
    k.check("good", 3, 3)
    c.check("a red checker returns 1", k.summary(), 1)
    lines = out.getvalue().splitlines()
    c.check("... prints one FAIL line per failure, then the summary",
            [ln.split(":")[0] for ln in lines], ["FAIL"] * 5 + ["FAIL - 5 of 6 checks failed"])
    c.contains("... the FAIL line carries got and want", lines[0], "got 1 != want 2")
    c.check("... and the runner still counts every check", runner.parse_checks(out.getvalue()), 6)
    k, out = _fresh()
    k.skip("absent", "no build tree")
    c.check("a skip is not a check", (k.count, k.summary()), (0, 0))
    c.contains("... but it is reported", out.getvalue(), "skip - absent: no build tree")


def test_fixture_tree_layout(c):
    with FixtureTree() as tree:
        c.expect("the tree is outside the live repository",
                 not str(tree.root).lower().startswith(str(testing.LIVE_ROOT).lower()))
        for rel in ("configure.py", "config/RMHE08/symbols.txt", "config/RMHE08/splits.txt",
                    "config/RMHE08/config.yml", "src", "include", "build/RMHE08/obj", "build/RMHE08/src"):
            c.expect("the skeleton has %s" % rel, tree.path(rel).exists())
        tree.add_cflags("cflags_runtime", ["-O4,p", "-fp_contract on"])
        tree.add_unit("Dir/file.c", lib="dir", flag="Matching", cflags="cflags_runtime",
                      ranges={".text": (0x80004000, 0x80004100), ".data": (0x80500000, 0x80500010)})
        tree.add_unit("Dir/other.cpp", lib="dir")
        tree.add_symbol("do_thing", ".text", 0x80004000, 0x100)
        tree.add_symbol("thing_table", ".data", 0x80500000, 0x10, type="object", scope="local")
        tree.claim("Dir/other.cpp", ".text", 0x80004100, 0x80004180)

        ns = runpy.run_path(str(tree.path("configure.py")))
        objects = [(o["name"], o["completed"]) for lib in ns["config"].libs for o in lib["objects"]]
        c.check("configure.py is real Python with the registered objects", objects,
                [("Dir/file.c", True), ("Dir/other.cpp", False)])
        c.check("... and the lib names its cflags group", ns["config"].libs[0]["cflags"], ["-O4,p", "-fp_contract on"])
        c.check("splits.txt is read by today's parser (sharedfiles.parse_ranges)",
                sharedfiles.parse_ranges(tree.read("config/RMHE08/splits.txt")),
                [("Dir/file.c", ".text", 0x80004000, 0x80004100), ("Dir/file.c", ".data", 0x80500000, 0x80500010),
                 ("Dir/other.cpp", ".text", 0x80004100, 0x80004180)])
        c.check("symbols.txt rows are the map's shape, in address order",
                tree.read("config/RMHE08/symbols.txt").splitlines(),
                ["do_thing = .text:0x80004000; // type:function size:0x100 scope:global",
                 "thing_table = .data:0x80500000; // type:object size:0x10 scope:local"])
        c.contains("a unit gets its source file", tree.read("src/Dir/file.c"), "fixture unit")
        tree.add_object("Dir/file.o", b"\x7fELF", side="src")
        c.check("objects land under build/RMHE08/<side>", tree.path("build/RMHE08/src/Dir/file.o").read_bytes(), b"\x7fELF")
        report = tree.set_report({"dir/Dir/file": {"fuzzy_match_percent": 50.0, "total_code": 8,
                                                   "functions": {"a": (4, 100.0), "b": (4, None)}}})
        funcs = __import__("json").loads(report.read_text(encoding="utf-8"))["units"][0]["functions"]
        c.check("a None score has no fuzzy_match_percent key (the 0 % convention)",
                ["fuzzy_match_percent" in f for f in funcs], [True, False])
        c.raises("an unknown section is refused", ValueError, tree.claim, "Dir/file.c", ".bogus", 0, 4)
        c.raises("an empty range is refused", ValueError, tree.claim, "Dir/file.c", ".text", 8, 8)
        root = tree.root
    c.expect("the context manager removes a tree it created", not root.exists())
    c.raises("a fixture inside the live repository is refused before anything is written", ValueError,
             FixtureTree, testing.LIVE_ROOT / "build" / "tmp" / "fixture")


def test_git_fixture(c):
    with GitFixture() as g:
        g.init()
        first = g.commit({"a.txt": "one\n"}, "first")
        c.check("commit returns the new head", g.head(), first)
        c.check("init names the branch main", g.current_branch(), "main")
        a, b = g.conflict("f.txt", "from a\n", "from b\n")
        c.check("conflict leaves the current branch alone", g.current_branch(), "main")
        g.checkout(a)
        merged = g.run("merge", "--no-edit", b)
        c.expect("merging the two conflict branches conflicts", merged.returncode != 0, merged.stdout)
        c.check("... on the named path", g.git("diff", "--name-only", "--diff-filter=U").split(), ["f.txt"])
        g.git("merge", "--abort")
        g.checkout("main")
        wt_parent = Path(tempfile.mkdtemp(prefix="git-fixture-wt-"))
        try:
            wt = g.worktree(wt_parent / "lane", "worker/x")
            c.check("a worktree is on its new branch", g.git("rev-parse", "--abbrev-ref", "HEAD", cwd=wt).strip(),
                    "worker/x")
            g.git("worktree", "remove", "--force", str(wt))
        finally:
            __import__("shutil").rmtree(wt_parent, ignore_errors=True)
        g.commit({"a.txt": None}, "delete")
        c.check("a None file is deleted", g.git("ls-files").split(), ["f.txt"])
        c.raises("a git failure raises with git's message", RuntimeError, g.git, "rev-parse", "no-such-ref")
    c.raises("a git fixture inside the live repository is refused", ValueError,
             GitFixture, testing.LIVE_ROOT / "x")


def _run_child(body: str, tier: str) -> subprocess.CompletedProcess:
    """A throwaway test module in a temp dir, run as the runner runs one (cwd = its temp dir)."""
    tmp = Path(tempfile.mkdtemp(prefix="tier-child-"))
    mod = tmp / "test_child.py"
    mod.write_text("from tools.lib import testing\nTIER = %r\n%s\nraise SystemExit(testing.run(globals()))\n"
                   % (tier, textwrap.dedent(body)), encoding="utf-8")
    env = {k: v for k, v in os.environ.items() if k != testing.TIER_ENV}
    env["PYTHONPATH"] = str(testing.LIVE_ROOT)
    try:
        return subprocess.run([sys.executable, str(mod)], cwd=tmp, env=env, capture_output=True, text=True,
                              encoding="utf-8", errors="replace", timeout=120)
    finally:
        __import__("shutil").rmtree(tmp, ignore_errors=True)


LIVE_READ = '''
def test_reads_live(c):
    c.check("read", len((testing.LIVE_ROOT / "configure.py").read_text(encoding="utf-8")) > 0, True)
'''


def test_tiers(c):
    p = _run_child(LIVE_READ, "fixture")
    c.check("a fixture-tier test that reads the live tree fails the run", p.returncode, 1)
    c.contains("... naming the live-tree access", p.stdout, "may not touch the live tree")
    p = _run_child(LIVE_READ, "smoke")
    c.check("the same read passes in the smoke tier", (p.returncode, p.stdout.strip().splitlines()[-1:]),
            (0, ["ok - 1 checks"]))
    p = _run_child('''
def test_swallowed(c):
    try:
        open(testing.LIVE_ROOT / "configure.py").close()
    except Exception:
        pass
    c.check("nothing", 1, 1)
''', "fixture")
    c.check("a refused read the test swallows still fails the run", p.returncode, 1)
    p = _run_child('''
def test_root(c):
    testing.live_root()
''', "fixture")
    c.check("live_root() is refused under the fixture tier", p.returncode, 1)
    c.contains("... by the resolver seam", p.stdout, "testing.live_root()")
    p = _run_child('''
import subprocess, sys
def test_proc(c):
    subprocess.run([sys.executable, "-c", "pass"], cwd=str(testing.LIVE_ROOT))
''', "fixture")
    c.check("a process started in the live tree is refused", p.returncode, 1)
    p = _run_child('''
import json, os, subprocess, sys, tempfile
def test_allowed(c):
    d = tempfile.mkdtemp()
    with open(os.path.join(d, "x.txt"), "w") as fh:
        fh.write("x")
    c.check("a temp file is readable", open(os.path.join(d, "x.txt")).read(), "x")
    import tools.units.sharedfiles
    c.check("importing a tool reads its code", hasattr(tools.units.sharedfiles, "parse_ranges"), True)
    out = subprocess.run([sys.executable, "-c", "import os; print(os.environ.get('TOOLS_TEST_TIER'))"],
                         cwd=d, capture_output=True, text=True).stdout.strip()
    c.check("a child process inherits the tier", out, "fixture")
''', "fixture")
    c.check("temp files, tool imports and child processes outside the tree are allowed",
            (p.returncode, p.stdout.strip().splitlines()[-1:]), (0, ["ok - 3 checks"]))
    p = _run_child("", "nonsense")
    c.check("an unknown TIER is refused", p.returncode != 0 and "TIER must be one of" in p.stderr, True)



def test_rewrite_json_keeps_the_files_shape(c):
    import json
    with tempfile.TemporaryDirectory() as d:
        two = os.path.join(d, "two.json")
        text = '{\n  "$comment": "x",\n  "pending": [\n    "a",\n    "b",\n    "c"\n  ]\n}\n'
        Path(two).write_text(text, encoding="utf-8", newline="\n")
        c.check("the indent unit is read from the file", (testing.json_indent(text), testing.json_indent("{}")), (2, 1))
        data = json.loads(text)
        data["pending"] = ["a", "c"]
        testing.rewrite_json(two, data)
        c.check("a prune of an indent-2 file drops one line and changes nothing else",
                Path(two).read_text(encoding="utf-8"), text.replace('    "b",\n', ""))
        one = os.path.join(d, "one.json")
        Path(one).write_text('{\n "edges": [\n  "a -> b"\n ],\n "note": "café"\n}\n', encoding="utf-8")
        testing.rewrite_json(one, {"edges": [], "note": "café"})
        c.check("an indent-1 file keeps indent 1 and its raw non-ASCII",
                Path(one).read_text(encoding="utf-8"), '{\n "edges": [],\n "note": "café"\n}\n')


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
