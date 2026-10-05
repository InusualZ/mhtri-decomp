"""lib.repo: the tree rule, MAIN, the input fallback, scratch/state/ground truth, and the live-tree choke point."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import hashlib
import os
import subprocess
import tempfile
from pathlib import Path

from tools.lib import repo, testing
from tools.lib.testing import LiveTreeError

TIER = "fixture"


def same(a, b) -> bool:
    return os.path.normcase(os.path.abspath(a)) == os.path.normcase(os.path.abspath(b))


@contextlib.contextmanager
def tier(name: str):
    """Run a block under another tier (the fallback rule is only reachable outside the fixture tier)."""
    before = testing.current_tier()
    testing.set_tier(name)
    try:
        yield
    finally:
        testing.set_tier(before)


@contextlib.contextmanager
def cwd(path):
    here = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(here)


def test_tree_rule(c):
    with testing.FixtureTree() as tree, tempfile.TemporaryDirectory() as tmp:
        sub = tree.root / "src" / "deep"
        sub.mkdir(parents=True)
        c.check("start= walks up to the fixture's configure.py", same(repo.repo_root(sub), tree.root), True)
        c.raises("no tree above start= exits", SystemExit, repo.repo_root, tmp)
        with testing.refusals_expected() as seen:
            c.raises("without start= the fixture tier refuses", LiveTreeError, repo.repo_root)
        c.check("... and the refusal was recorded", len(seen.refused), 1)
        packaged = repo.PACKAGED_ROOT
        repo.PACKAGED_ROOT = tree.root
        try:
            with cwd(tmp), testing.refusals_expected():
                c.raises("... even when the fallback would land on a fixture", LiveTreeError, repo.repo_root)
            with tier("smoke"), cwd(tmp):
                c.check("a cwd that is no tree falls back to the packaged copy", same(repo.repo_root(), tree.root), True)
            with testing.FixtureTree() as other, tier("smoke"), cwd(other.root):
                c.check("a cwd that is a tree (not git) wins", same(repo.repo_root(), other.root), True)
        finally:
            repo.PACKAGED_ROOT = packaged


def test_live_tree_choke_point(c):
    live = testing.LIVE_ROOT
    with testing.refusals_expected() as seen:
        c.raises("repo_root(start=<live>) is refused under the fixture tier", LiveTreeError, repo.repo_root, live)
        c.raises("resolve_input on the live tree is refused (an os.stat probe the audit hook cannot see)",
                 LiveTreeError, repo.resolve_input, "build/RMHE08/asm", live)
        c.raises("a Tree over the live root is refused", LiveTreeError, repo.Tree, live)
        c.raises("main_tree of the live root is refused", LiveTreeError, repo.main_tree, live)
        c.raises("scratch/state under the live root are refused", LiveTreeError, repo.scratch, "x", live)
        c.raises("a process given a live path is refused (a Windows command line is one string)", LiveTreeError,
                 subprocess.run, [sys.executable, "-c", "pass", str(live / "config")])
    c.check("every refusal was recorded", len(seen.refused), 6)


def test_is_served(c):
    with testing.refusals_expected() as seen:
        c.check("no tree is the served one under the fixture tier, and asking resolves nothing live",
                repo.is_served(str(testing.LIVE_ROOT)), False)
    c.check("... no refusal was needed", len(seen.refused), 0)
    with testing.FixtureTree() as tree:
        c.check("a fixture is never the served tree", repo.is_served(str(tree.root)), False)


def test_seam_tools_import_without_a_root(c):
    from tools.splits import dataorder, tudiscover  # noqa: F401 - their tree paths resolve on first use
    from tools.units import dataseams  # noqa: F401
    c.expect("tudiscover resolves no path at import", "ASM_DIR" not in vars(tudiscover) and "ROOT" not in vars(tudiscover))
    c.expect("dataorder resolves no path at import", "DOL" not in vars(dataorder) and not dataorder._PATHS)
    with testing.FixtureTree() as tree:
        tudiscover.ROOT = str(tree.root)            # an assigned path pins it; the others follow it
        try:
            c.check("tudiscover's paths follow an assigned ROOT",
                    (tudiscover._g("SYMBOLS"), tudiscover._g("LOCAL_ASM_DIR")),
                    (os.path.join(str(tree.root), "config", "RMHE08", "symbols.txt"),
                     os.path.join(str(tree.root), "build", "RMHE08", "asm")))
        finally:
            for name in tudiscover._LAZY:
                vars(tudiscover).pop(name, None)


def test_main_and_inputs(c):
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        fx.init()
        fx.commit({"configure.py": "", "build/RMHE08/obj/a.o": b"\x7fELF"}, "main")
        wt = fx.worktree(Path(tmp) / "wt", "lane")
        c.check("main_tree of a worktree is MAIN", same(repo.main_tree(wt), fx.root), True)
        c.check("main_tree of MAIN is MAIN", same(repo.main_tree(fx.root), fx.root), True)
        (fx.root / "only-main.bin").write_bytes(b"m")
        got = repo.resolve_input("only-main.bin", wt)
        c.check("resolve_input falls back to MAIN by path", same(got, fx.root / "only-main.bin"), True)
        c.check("... the worktree's own copy wins", same(repo.resolve_input("configure.py", wt), wt / "configure.py"), True)
        c.check("... and a path neither has names the worktree's",
                same(repo.resolve_input("nope.bin", wt), wt / "nope.bin"), True)
        alt = Path(tmp) / "alt"
        alt.mkdir()
        os.environ["MHTRI_MAIN"] = str(alt)
        try:
            c.check("$MHTRI_MAIN applies only when honoured", (same(repo.main_tree(wt, honour_env=True), alt),
                                                              same(repo.main_tree(wt), fx.root)), (True, True))
        finally:
            del os.environ["MHTRI_MAIN"]
        t = repo.Tree(wt)
        c.check("Tree: a linked worktree, not a slot", (t.is_worktree, t.is_slot, same(t.main, fx.root)),
                (True, False, True))
        c.check("Tree locations", [p.relative_to(t.root).as_posix() for p in
                                   (t.obj_dir, t.src_obj_dir, t.asm_dir, t.report_json, t.orig_dol, t.config_yml,
                                    t.objdiff_json)],
                ["build/RMHE08/obj", "build/RMHE08/src", "build/RMHE08/asm", "build/RMHE08/report.json",
                 "orig/RMHE08/sys/main.dol", "config/RMHE08/config.yml", "objdiff.json"])
        c.check("Tree.input uses the fallback", same(t.input("only-main.bin"), fx.root / "only-main.bin"), True)
        c.check("main_checkout of a worktree is MAIN", same(repo.main_checkout(wt), fx.root), True)
        (wt / "sub").mkdir()
        c.check("worktree_root from inside a worktree's subdirectory is the worktree, not MAIN",
                same(repo.worktree_root(wt / "sub"), wt), True)
        c.check("worktree_root of MAIN is MAIN", same(repo.worktree_root(fx.root), fx.root), True)
        try:
            repo.worktree_root(tmp)
            refused = None
        except SystemExit as exc:
            refused = str(exc)
        c.check("worktree_root outside git refuses and names the directory",
                bool(refused and "not a git worktree" in refused and os.path.basename(tmp) in refused), True)
    with testing.FixtureTree() as tree:
        c.check("main_tree outside git is None", repo.main_tree(tree.root), None)
        c.check("main_checkout of a non-repository copy is the copy itself",
                same(repo.main_checkout(tree.root), tree.root), True)
        with testing.FixtureTree() as other:
            real = repo.main_tree
            repo.main_tree = lambda root, honour_env=False: str(other.root)
            try:
                c.check("main_checkout takes main_tree's answer when it is a tree",
                        same(repo.main_checkout(tree.root), other.root), True)
            finally:
                repo.main_tree = real
        c.check("a fixture Tree is its own MAIN", same(repo.Tree(tree.root).main, tree.root), True)


def test_scratch_state_session(c):
    with testing.FixtureTree() as tree:
        p = repo.scratch("mytool", tree.root)
        c.check("scratch is build/tmp/<tool>, created", (p.relative_to(tree.root).as_posix(), p.is_dir()),
                ("build/tmp/mytool", True))
        c.check("state names the .pi file", repo.state("claims.json", tree.root).relative_to(tree.root).as_posix(),
                ".pi/claims.json")
        c.raises("state refuses a name off the one list", ValueError, repo.state, "random.json", tree.root)
        c.check("the heartbeat dir and the landing log are on the list (WP4)",
                [repo.state(n, tree.root).relative_to(tree.root).as_posix() for n in ("ack", "land-log.jsonl")],
                [".pi/ack", ".pi/land-log.jsonl"])
    d = repo.session_tmpdir()
    c.check("session_tmpdir is one directory per process", (repo.session_tmpdir() == d, os.path.isdir(d)), (True, True))
    env = dict(os.environ, PYTHONPATH=str(Path(repo.__file__).parents[2]))
    child = subprocess.run([sys.executable, "-c", "from tools.lib import repo; print(repo.session_tmpdir())"],
                           capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
    c.check("... and another process gets another", child.stdout.strip() not in ("", d), True)


def test_ground_truth(c):
    with testing.FixtureTree() as tree:
        dol = b"not really a dol"
        tree.write("orig/RMHE08/sys/main.dol", dol)
        tree.write(tree.config_dir / "config.yml", "object: orig/RMHE08/sys/main.dol\nhash: %s\n"
                   "selfile_hash: %s\n" % (hashlib.sha1(dol).hexdigest().upper(), "0" * 40))
        gt = repo.ground_truth(tree.root)
        c.check("ground_truth reads both pins, lowercased",
                gt, {"orig/RMHE08/sys/main.dol": hashlib.sha1(dol).hexdigest(), "orig/RMHE08/files/mh3.sel": "0" * 40})
        c.check("verify_ground_truth: the DOL matches, the missing selfile is reported",
                repo.verify_ground_truth(tree.root), [("orig/RMHE08/files/mh3.sel", "0" * 40, "MISSING")])
        tree.write("orig/RMHE08/sys/main.dol", b"changed")
        c.check("... a changed DOL is reported with its hash",
                [r[0] for r in repo.verify_ground_truth(tree.root)],
                ["orig/RMHE08/files/mh3.sel", "orig/RMHE08/sys/main.dol"])


def test_config_change(c):
    base = "# the DOL\nobject: orig/RMHE08/sys/main.dol\nhash: %s\nfill_gaps: true\n" % ("0" * 40)
    hint = base + "block_relocations:\n- target: extabindex:0x80020000\n  end: extabindex:0x80020010\n"
    blocks, problems = repo.config_blocks(hint)
    c.check("config_blocks: a column-0 list item belongs to the open key",
            blocks["block_relocations"][1:], ["- target: extabindex:0x80020000", "  end: extabindex:0x80020010"])
    c.check("... and the file reads cleanly", problems, [])
    c.check("a relocation hint may be added", repo.config_change(base, hint)["ok"], True)
    c.check("... and is named", repo.config_change(base, hint)["changed"], ["block_relocations"])
    c.check("add_relocations may change too",
            repo.config_change(base, base + "add_relocations:\n- source: 0x1\n")["ok"], True)
    c.check("a comment edit is no change", repo.config_change(base, base.replace("the DOL", "DOL"))["ok"], True)
    c.check("the DOL path may not change", repo.config_change(base, base.replace("main.dol", "x.dol"))["ok"], False)
    c.check("a removed key refuses", repo.config_change(base, base.replace("fill_gaps: true\n", ""))["ok"], False)
    c.check("a hint riding a frozen change refuses",
            repo.config_change(base, hint.replace("true", "false"))["changed"], ["block_relocations", "fill_gaps"])
    c.check("... and the verdict is a refusal", repo.config_change(base, hint.replace("true", "false"))["ok"], False)
    c.check("a deleted file refuses", repo.config_change(base, None)["ok"], False)
    c.check("an unreadable line refuses", repo.config_change(base, base + "stray\n")["ok"], False)


def test_moved_header(c):
    c.check("include/P lives at src/P since the move", repo.moved_header("include/Network/x.h"), "src/Network/x.h")
    c.check("the exception table wins", repo.moved_header("include/nw4r/g3d/g3d_resmat.h"), "src/g3d/g3d_resmat.h")
    c.check("a path outside include/ is unchanged", repo.moved_header("src/a/include/b.h"), "src/a/include/b.h")


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
