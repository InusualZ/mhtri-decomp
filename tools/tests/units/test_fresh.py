"""fresh.py and the adopting tools on fixture trees: `fresh.py status` (rows, exit code, --json), tudiscover's
`ensure_dump` (auto by default, --no-refresh warns, a failed refresh warns) and callers' `choose_dump` (a fresh dump
answers, a stale one falls back to the split objects unless refreshed, refused under FRESH=refuse)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import argparse
import contextlib
import io
import json
import os
import subprocess
import tempfile
from pathlib import Path

from tools.lib import artifacts, refs, testing
from tools.splits import tudiscover as td
from tools.units import callers, fresh

TIER = "fixture"
os.environ.pop(artifacts.ENV, None)       # the defaults are what is tested, never the caller's FRESH


def _tree(root: Path, objects: bool = True) -> None:
    """Map, splits, DOL, a one-file dump stamped against them, and (optionally) one split object."""
    for rel, text in (("config/RMHE08/symbols.txt", "a = .text:0x80001000; // type:function size:0x4\n"),
                      ("config/RMHE08/splits.txt", "u.c:\n\t.text start:0x80001000 end:0x80001004\n"),
                      ("orig/RMHE08/sys/main.dol", "dol"),
                      ("build/RMHE08/asm/u.s", ".fn a, global\n.endfn a\n")):
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text, encoding="utf-8")
    if objects:
        (root / "build/RMHE08/obj").mkdir(parents=True, exist_ok=True)
        (root / "build/RMHE08/obj/u.o").write_bytes(b"\x7fELF")
    refs.DumpStamp.for_tree(str(root)).write()


def _stale(root: Path) -> None:
    (root / "config/RMHE08/symbols.txt").write_text("a = .text:0x80001000; // renamed\n", encoding="utf-8")


def _restamp_runner(root: Path, calls: list):
    def run(argv, cwd):
        calls.append(argv)
        refs.DumpStamp.for_tree(str(root)).write()
        return subprocess.CompletedProcess(argv, 0, "", "")
    return run


def _failing_runner(calls: list):
    def run(argv, cwd):
        calls.append(argv)
        return subprocess.CompletedProcess(argv, 2, "", "build/tools/dtk.exe is missing\n")
    return run


@contextlib.contextmanager
def _td_pinned(root: Path):
    """Point tudiscover's lazy paths at the fixture tree, and restore them."""
    names = ("ROOT", "SYMBOLS", "SPLITS", "LOCAL_ASM_DIR", "ASM_DIR", "DOL", "CACHE", "ASM_STAMP")
    saved = {n: td.__dict__[n] for n in names if n in td.__dict__}
    td.ROOT = str(root)
    td.SYMBOLS, td.SPLITS = str(root / "config/RMHE08/symbols.txt"), str(root / "config/RMHE08/splits.txt")
    td.LOCAL_ASM_DIR = td.ASM_DIR = str(root / "build/RMHE08/asm")
    td.ASM_STAMP = str(root / "build/RMHE08/asm/.stamp.json")
    td.DOL = str(root / "orig/RMHE08/sys/main.dol")
    td.CACHE = str(root / "build/tmp/tudiscover/graph.json")
    try:
        yield
    finally:
        for n in names:
            td.__dict__.pop(n, None)
        td.__dict__.update(saved)


def test_tudiscover_ensure_dump(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _tree(root)
        with _td_pinned(root):
            calls: list = []
            out = io.StringIO()
            st = td.ensure_dump(argparse.Namespace(fresh=None), out=out, runner=_restamp_runner(root, calls))
            c.check("a fresh dump: nothing runs, nothing is printed", (st.state, calls, out.getvalue()),
                    ("fresh", [], ""))
            _stale(root)
            st = td.ensure_dump(argparse.Namespace(fresh=None), out=out, runner=_restamp_runner(root, calls))
            c.check("a stale dump is refreshed by default (auto), once", (st.state, st.refreshed, len(calls)),
                    ("fresh", True, 1))
            c.contains("... with dump_asm.py", calls[0][-1].replace("\\", "/"), "tools/splits/dump_asm.py")
            c.check("... and the tool now reads its own dump", td.ASM_DIR, td.LOCAL_ASM_DIR)
            _stale(root)
            (root / "config/RMHE08/symbols.txt").write_text("again\n", encoding="utf-8")
            calls.clear()
            out = io.StringIO()
            st = td.ensure_dump(argparse.Namespace(fresh="warn"), out=out, runner=_restamp_runner(root, calls))
            c.check("--no-refresh uses the stale dump", (st.state, calls), ("stale", []))
            c.contains("... and says so", out.getvalue(), "WARNING: asm-dump is stale")
            out = io.StringIO()
            st = td.ensure_dump(argparse.Namespace(fresh=None), out=out, runner=_failing_runner(calls))
            c.check("a refresh that cannot run does not stop the tool", st.state, "stale")
            c.contains("... it warns with the reason", out.getvalue(), "dtk.exe is missing")


def test_tudiscover_graph_check(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _tree(root)
        with _td_pinned(root):
            state, msg = td.graph_check()
            c.check("no cache is missing", state, "missing")
            td.build_graph(*td.load_map())
            c.check("a just-built cache is fresh", td.graph_check()[0], "fresh")
            _stale(root)
            state, msg = td.graph_check()
            c.check("an edited map makes the graph stale", state, "stale")
            c.contains("... naming what changed", msg, "symbols")


def test_callers_choose_dump(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _tree(root)
        asm = os.path.join(tmp, "build", "RMHE08", "asm")
        _dir, files, note = callers.choose_dump(tmp, None, out=io.StringIO())
        c.check("a fresh dump answers", (len(files), note), (1, None))
        _stale(root)
        _dir, files, note = callers.choose_dump(tmp, None, out=io.StringIO())
        c.check("a stale dump is not refreshed by default", os.path.normcase(_dir), os.path.normcase(asm))
        c.contains("... the split objects answer, carrying the stamp's message", note or "", "symbols changed")
        try:
            callers.choose_dump(tmp, "refuse", out=io.StringIO())
            c.fail("FRESH=refuse refuses a stale dump", "no exception")
        except artifacts.StaleArtifact as exc:
            c.contains("FRESH=refuse refuses a stale dump", str(exc.code), "asm-dump is stale")
        calls: list = []
        _dir, files, note = callers.choose_dump(tmp, "auto", out=io.StringIO(), runner=_restamp_runner(root, calls))
        c.check("--refresh rebuilds it and the dump answers", (len(calls), note), (1, None))
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _tree(root, objects=False)
        _stale(root)
        _dir, files, note = callers.choose_dump(tmp, None, out=io.StringIO())
        c.check("no split objects: the stale dump answers (and says so, as before)", (len(files), note), (1, None))


def test_fresh_cli(c):
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        _tree(root)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = fresh.TOOL.run(fresh.main, ["--root", tmp, "--json"], parser=fresh.build_parser())
        data = json.loads(buf.getvalue())
        rows = {r["name"]: r for r in data["artifacts"]}
        c.check("status lists every registered artifact", sorted(rows), sorted(artifacts.REGISTRY))
        c.check("the fixture's dump is fresh", rows["asm-dump"]["state"], "fresh")
        c.check("no build.ninja: the manifest is missing", rows["manifest"]["state"], "missing")
        c.check("... so the exit is 1", rc, 1)
        c.check("a row carries its refresh command and cost", (rows["asm-dump"]["command"], bool(rows["asm-dump"]["cost"])),
                ("python tools/splits/dump_asm.py", True))
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = fresh.TOOL.run(fresh.main, ["status", "asm-dump", "--root", tmp], parser=fresh.build_parser())
        c.check("one fresh artifact: exit 0", rc, 0)
        c.contains("the table names it", buf.getvalue(), "asm-dump")
        c.raises("an unknown artifact is refused", SystemExit, fresh.TOOL.run, fresh.main,
                 ["status", "nope", "--root", tmp], parser=fresh.build_parser())


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
