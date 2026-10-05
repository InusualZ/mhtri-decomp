"""dump_asm: the temp-config rewrite, the stamp's state machine (`tudiscover` over `lib.refs.DumpStamp`), and the
replace-the-dump cycle - a stale `.s` never survives a refresh, a failed split leaves the old dump and its stamp
alone, and nothing but the tree's own dump directories is ever deleted. dtk is a fake runner; every path is a
temp tree."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import io
import json
import os
import tempfile
from pathlib import Path
from types import SimpleNamespace

from tools.lib import testing
from tools.splits import dump_asm as da
from tools.splits import tudiscover as td

TIER = "fixture"
CONFIG = "object: orig/RMHE08/sys/main.dol\r\nwrite_asm: false  # off on purpose\r\nb: 2\r\n"


def _tree(tmp: Path) -> da.Tree:
    """A minimal tree: configure.py, the three split inputs, a CRLF config.yml and a fake dtk."""
    for rel, text in (("configure.py", "# fixture\n"), ("config/RMHE08/symbols.txt", "sym\n"),
                      ("config/RMHE08/splits.txt", "spl\n"), ("orig/RMHE08/sys/main.dol", "dol"),
                      ("build/tools/dtk.exe" if os.name == "nt" else "build/tools/dtk", "")):
        p = tmp / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text, encoding="utf-8")
    with open(tmp / "config/RMHE08/config.yml", "w", encoding="utf-8", newline="") as fh:
        fh.write(CONFIG)
    return da.tree_for(str(tmp))


def _fake_dtk(files, rc=0, during=None, calls=None):
    """A runner that writes `files` (rel -> text) under the split's out_dir/asm, then exits `rc`."""
    def run(cmd, cwd=None):
        if calls is not None:
            calls.append((list(cmd), cwd))
        out = Path(cmd[-1]) / "asm"
        for rel, text in files.items():
            (out / rel).parent.mkdir(parents=True, exist_ok=True)
            (out / rel).write_text(text, encoding="utf-8")
        (Path(cmd[-1]) / "obj").mkdir(parents=True, exist_ok=True)
        if during:
            during()
        return SimpleNamespace(returncode=rc)
    return run


def _names(d) -> set:
    return {os.path.relpath(p, d).replace("\\", "/") for p in da.refs.dump_files(str(d))}


def _quiet(tree, runner):
    return da.dump(tree, runner, out=io.StringIO(), err=io.StringIO())


def test_temp_config(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        out = da.temp_config(t)
        c.check("write_asm flipped, the comment kept", "write_asm: true  # off on purpose" in out, True)
        c.check("neighbour keys untouched", out.replace("\r\n", "\n"),
                "object: orig/RMHE08/sys/main.dol\nwrite_asm: true  # off on purpose\nb: 2\n")
        c.check("line endings preserved", "\r\n" in out, True)
        Path(t.config).write_bytes(b"write_asm: true\n")
        c.check("already true stays true", da.temp_config(t), "write_asm: true\n")
        Path(t.config).write_text("a: 1\n", encoding="utf-8")
        c.raises("a config with no write_asm line asserts", AssertionError, da.temp_config, t)


def test_the_tree_is_the_one_named(c):
    """Config, toolchain and output all come from `--root` - never from this file's tree."""
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        c.check("the dump is the tree's own build/RMHE08/asm", t.asm_dir, os.path.join(tmp, "build", "RMHE08", "asm"))
        c.check("... the config is the tree's", t.config, os.path.join(tmp, "config", "RMHE08", "config.yml"))
        c.check("... and so is dtk", os.path.dirname(t.dtk), os.path.join(tmp, "build", "tools"))
        calls = []
        _quiet(t, _fake_dtk({"a.s": "x"}, calls=calls))
        c.check("dtk runs in the tree, on the tree's temp config, into the tree's stage",
                (calls[0][1], calls[0][0][-2], calls[0][0][-1]), (tmp, t.tmp_config, t.stage))
        c.raises("a --root that is no tree is refused", SystemExit, da.tree_for, os.path.join(tmp, "config"))
        os.unlink(t.config)
        c.raises("a tree without config.yml is refused", SystemExit, da.tree_for, tmp)


def test_a_refresh_replaces_the_dump(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        asm = Path(t.asm_dir)
        (asm / "Gone").mkdir(parents=True)
        (asm / "Gone" / "old_unit.s").write_text("stale", encoding="utf-8")
        (asm / "kept.s").write_text("old bytes", encoding="utf-8")
        t.stamp().write()
        rc = _quiet(t, _fake_dtk({"kept.s": "new bytes", "Lib/unit.s": "y"}))
        c.check("the refresh succeeds", rc, 0)
        c.check("the dump is exactly what the split wrote (the stale unit is gone)", _names(asm), {"kept.s", "Lib/unit.s"})
        c.check("... with the new bytes", (asm / "kept.s").read_text(encoding="utf-8"), "new bytes")
        c.check("... stamped fresh, over the new file count", (t.stamp().status()[0],
                json.loads((asm / ".stamp.json").read_text(encoding="utf-8"))["files"]), ("fresh", 2))
        c.check("no retired dump and no staged asm left behind", da.leftovers(t), [])
        c.check("the temp config is removed", os.path.exists(t.tmp_config), False)


def test_a_failed_split_changes_nothing(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        asm = Path(t.asm_dir)
        asm.mkdir(parents=True)
        (asm / "unit.s").write_text("old", encoding="utf-8")
        stamp = t.stamp().write()
        rc = _quiet(t, _fake_dtk({"half.s": "partial"}, rc=3))
        c.check("dtk's exit code is returned", rc, 3)
        c.check("the old dump is untouched", (_names(asm), (asm / "unit.s").read_text(encoding="utf-8")),
                ({"unit.s"}, "old"))
        c.check("... and so is its stamp, which still describes it",
                (json.loads((asm / ".stamp.json").read_text(encoding="utf-8"))["utc"], t.stamp().status()[0]),
                (stamp["utc"], "fresh"))
        c.check("the partial staged asm is removed", da.leftovers(t), [])
        rc = _quiet(t, _fake_dtk({}))
        c.check("a split that wrote no .s is a failure too", (rc, _names(asm)), (1, {"unit.s"}))


def test_a_failed_swap_restores_the_old_dump(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        asm = Path(t.asm_dir)
        asm.mkdir(parents=True)
        (asm / "unit.s").write_text("old", encoding="utf-8")
        t.stamp().write()
        real, n = os.replace, []

        def flaky(a, b):
            n.append(a)
            if len(n) == 2:
                raise OSError("in use")
            return real(a, b)
        da.os.replace = flaky
        try:
            rc = _quiet(t, _fake_dtk({"new.s": "n"}))
        finally:
            da.os.replace = real
        c.check("a swap that cannot finish exits 2", rc, 2)
        c.check("... with the old dump and its fresh stamp back in place", (_names(asm), t.stamp().status()[0]),
                ({"unit.s"}, "fresh"))
        c.check("... and nothing left over", da.leftovers(t), [])


def test_the_stamp_records_the_inputs_read_before_the_split(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        sym = Path(tmp) / "config/RMHE08/symbols.txt"
        _quiet(t, _fake_dtk({"a.s": "x"}, during=lambda: sym.write_text("renamed mid-run", encoding="utf-8")))
        c.check("a map edit made while dtk ran reads as stale, never fresh", t.stamp().status()[0], "stale")


def test_only_owned_directories_are_deleted(c):
    with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as other:
        t, o = _tree(Path(tmp)), _tree(Path(other))
        build = t.build_dir
        ok = [t.asm_dir, t.stage, os.path.join(t.stage, "asm"), os.path.join(build, da.RETIRED + ".12")]
        c.check("the tree's dump, stage, staged asm and a retired dump are deletable",
                [da.removal_refusal(t, p) for p in ok], [None] * len(ok))
        bad = [build, os.path.join(build, "obj"), os.path.join(tmp, "src"), o.asm_dir,
               os.path.join(tmp, "asm"), os.path.join(build, "obj", "asm"), os.path.join(o.stage, "asm")]
        c.check("anything else is refused", [da.removal_refusal(t, p) is not None for p in bad], [True] * len(bad))
        os.makedirs(o.asm_dir)
        (Path(o.asm_dir) / "x.s").write_text("theirs", encoding="utf-8")
        c.raises("remove_owned refuses another tree's dump", SystemExit, da.remove_owned, t, o.asm_dir)
        c.check("... and deletes nothing there", _names(o.asm_dir), {"x.s"})
        os.makedirs(build, exist_ok=True)
        try:
            os.symlink(o.asm_dir, t.asm_dir, target_is_directory=True)
        except (OSError, NotImplementedError):
            c.skip("a linked asm directory is refused", "no symlink privilege here")
        else:
            c.contains("a linked asm directory is refused", da.removal_refusal(t, t.asm_dir), "link or junction")
            os.unlink(t.asm_dir)


def test_leftovers_are_cleared_first(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        retired = Path(t.build_dir) / (da.RETIRED + ".999")
        (retired / "sub").mkdir(parents=True)
        (retired / "sub" / "a.s").write_text("x", encoding="utf-8")
        (Path(t.stage) / "asm").mkdir(parents=True)
        (Path(t.stage) / "obj").mkdir(parents=True)
        c.check("an interrupted run's retired dump and staged asm are leftovers (the stage's objects are kept)",
                sorted(os.path.basename(p) for p in da.leftovers(t)), sorted(["asm", da.RETIRED + ".999"]))
        _quiet(t, _fake_dtk({"b.s": "y"}))
        c.check("a run clears them", (retired.exists(), _names(t.asm_dir)), (False, {"b.s"}))


def test_cli(c):
    with tempfile.TemporaryDirectory() as tmp:
        t = _tree(Path(tmp))
        out = io.StringIO()
        sys_stdout, sys.stdout = sys.stdout, out
        try:
            rc_check = da.main(["--root", tmp, "--check"])
            rc_dry = da.main(["--root", tmp, "--dry-run"], runner=_fake_dtk({"a.s": "x"}))
            nothing = _names(t.asm_dir) if os.path.isdir(t.asm_dir) else set()
            rc_run = da.main(["--root", tmp], runner=_fake_dtk({"a.s": "x"}))
            rc_check2 = da.main(["--root", tmp, "--check"])
        finally:
            sys.stdout = sys_stdout
        c.check("--check on no dump exits 1, --dry-run 0 and writes nothing", (rc_check, rc_dry, nothing), (1, 0, set()))
        c.check("a run exits 0 and --check then exits 0", (rc_run, rc_check2), (0, 0))
        c.contains("the run reports the replaced count", out.getvalue(), "1 .s file(s)")


def test_stamp_states(c):
    """`tudiscover`'s view of the stamp (`lib.refs.DumpStamp`): missing / unstamped / fresh / stale / truncated."""
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        asm = tmp / "asm"
        asm.mkdir(parents=True)
        symbols, splits, dol = (tmp / n for n in ("symbols.txt", "splits.txt", "main.dol"))
        for p, text in ((symbols, "sym"), (splits, "spl"), (dol, "dol")):
            p.write_text(text, encoding="utf-8")
        td.ROOT = str(tmp)
        td.ASM_DIR = str(asm)
        td.LOCAL_ASM_DIR = str(tmp / "local")
        td.ASM_STAMP = str(asm / ".stamp.json")
        td.SYMBOLS, td.SPLITS, td.DOL = str(symbols), str(splits), str(dol)
        state = lambda: td.asm_stamp_status()[0]  # noqa: E731

        c.check("empty dump is missing", state(), "missing")
        (asm / "unit.s").write_text(".fn x\n.endfn x\n", encoding="utf-8")
        c.check("dump without stamp", state(), "unstamped")
        td.write_asm_stamp()
        c.check("stamped dump is fresh", state(), "fresh")
        c.check("a dump read from outside the tree says it is MAIN's, read-only",
                "[MAIN's dump" in td.asm_stamp_status()[1], True)
        c.check("stamp counts .s only", json.loads(open(td.ASM_STAMP, encoding="utf-8").read())["files"], 1)
        c.check("no temp stamp left behind", os.path.exists(td.ASM_STAMP + ".tmp"), False)
        for p, text, what in ((symbols, "sym renamed", "map"), (splits, "spl changed", "split"),
                              (dol, "dol changed", "dol")):
            td.write_asm_stamp()
            p.write_text(text, encoding="utf-8")
            c.check("a %s edit makes it stale" % what, state(), "stale")
        td.write_asm_stamp()
        (asm / "unit2.s").write_text(".fn y\n.endfn y\n", encoding="utf-8")
        c.check("a new unit does not invalidate the inputs", state(), "fresh")
        td.write_asm_stamp()
        (asm / "unit2.s").unlink()
        c.check("lost files are truncated", state(), "truncated")
        (asm / ".stamp.json").write_text("not json", encoding="utf-8")
        c.check("unreadable stamp", state(), "unstamped")
        td.use_local_dump()
        c.check("use_local_dump points at the tree's own dump and its stamp",
                (td.ASM_DIR, td.ASM_STAMP, td.dump_is_main_fallback()),
                (str(tmp / "local"), os.path.join(str(tmp / "local"), ".stamp.json"), False))


def test_graph_stamp_keys_on_content(c):
    """`tudiscover`'s graph and `callers`' index key the dump by one rule, so `fresh.py status` gives one reason."""
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        asm = tmp / "build" / "RMHE08" / "asm"
        (asm / "Lib").mkdir(parents=True)
        unit = asm / "Lib" / "unit.s"
        unit.write_bytes(b".fn a\nbl b\n.endfn a\n")
        (tmp / "symbols.txt").write_bytes(b"sym\n")
        td.ROOT, td.ASM_DIR, td.SYMBOLS = str(tmp), str(asm), str(tmp / "symbols.txt")
        files = [str(unit)]
        s0 = td.graph_stamp(files)
        c.check("the graph's content key is lib.refs.dump_signature", s0["content"],
                da.refs.dump_signature(str(asm), files, str(tmp)))
        st = os.stat(unit)
        unit.write_bytes(b".fn a\nbl b\n.endfn a\n")
        os.utime(unit, ns=(st.st_atime_ns, st.st_mtime_ns + 2_000_000_000))
        c.check("a rewrite with the same bytes keeps the stamp", td.graph_stamp(files), s0)
        unit.write_bytes(b".fn a\nbl c\n.endfn a\n")
        s1 = td.graph_stamp(files)
        c.check("a same-size edit moves it (the old bytes-total key did not)",
                (s1["content"] != s0["content"], s1["files"] == s0["files"]), (True, True))
        c.check("... and graph_check words it as callers does", td.STAMP_WORDS["content"], da.refs.DUMP_CHANGED)


if __name__ == "__main__":
    sys.exit(testing.run(globals()))
