"""unionguard: a union of a conflicted `git apply -3` passes only for disjoint additions; a refusal undoes the apply."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io

from tools.lib import testing
from tools.tests.units import merge_fixtures as mf
from tools.units.merge import unionguard as ug
from tools.units.merge import unionresolve as ur

TIER = "fixture"
mf.hermetic_git()


def quiet(fn, *args, **kwargs):
    with contextlib.redirect_stdout(io.StringIO()):
        return fn(*args, **kwargs)


def diverged(fx, base: dict, branch: dict, main: dict) -> str:
    """`main` and `branch` cut from one base commit and changed apart; returns the merge base."""
    fx.init()
    fx.commit(base, "base")
    fx.branch("branch", checkout=True)
    fx.commit(branch, "branch")
    fx.checkout("main")
    fx.commit(main, "main")
    return fx.git("merge-base", "main", "branch").strip()


def apply3(fx, base: str, branch: str = "branch") -> int:
    """`git apply -3 <merge-base diff>`, exactly as `land.py land --branch` does it."""
    import subprocess
    patch = subprocess.run(["git", "diff", "--binary", base, branch], cwd=str(fx.root), capture_output=True,
                           env=testing._git_env()).stdout
    return subprocess.run(["git", *fx.IDENTITY, "apply", "-3", "-"], cwd=str(fx.root), input=patch,
                          capture_output=True, env=testing._git_env()).returncode


def stage_conflict(fx, path: str, stages: dict) -> None:
    """An unmerged index entry built directly (`apply -3` cannot express a modify/delete conflict): stage 0
    cleared, then the given stages (bytes payload, so Windows does not append a CR to the path)."""
    import subprocess
    lines = ["0 0000000000000000000000000000000000000000\t%s\n" % path]
    lines += ["100644 %s %s\t%s\n" % (sha, stage, path) for stage, sha in sorted(stages.items())]
    p = subprocess.run(["git", "update-index", "--index-info"], cwd=str(fx.root), input="".join(lines).encode(),
                       capture_output=True, env=testing._git_env())
    if p.returncode:
        raise RuntimeError(p.stderr.decode("utf-8", "replace"))


def clean(fx) -> str:
    return fx.git("status", "--porcelain").strip()


def test_disjoint_additions_union(c):
    with testing.GitFixture() as fx:
        base = diverged(fx, {"reg.txt": "anchor\n", "keep.c": "int keep;\n"},
                        {"reg.txt": "anchor\nbranch added\n"}, {"reg.txt": "anchor\nmain added\n"})
        c.check("disjoint: the apply lands the conflict", apply3(fx, base) in (0, 1), True)
        c.check("... reg.txt is the conflict", sorted(ug.unmerged(str(fx.root))), ["reg.txt"])
        c.check("... the guard allows the union and leaves the conflict for it",
                (quiet(ug.report, str(fx.root), base, "branch", []), sorted(ug.unmerged(str(fx.root)))),
                (0, ["reg.txt"]))
        n = quiet(ur.union_file, str(fx.root / "reg.txt"))
        text = (fx.root / "reg.txt").read_text(encoding="utf-8")
        c.check("... and the one union rule keeps both additions, no markers",
                (n, "main added" in text and "branch added" in text, "<<<<<<<" in text), (1, True, False))


def test_both_sides_modified_refuses(c):
    with testing.GitFixture() as fx:
        base = diverged(fx, {"cfg.txt": "value = 0\n"}, {"cfg.txt": "value = 1\n"}, {"cfg.txt": "value = 2\n"})
        c.check("both-modified: the apply lands the conflict", apply3(fx, base) in (0, 1), True)
        root = str(fx.root)
        finding = ug.classify(root, "cfg.txt", ug.unmerged(root)["cfg.txt"], (set(), set()),
                              ug.rename_sets(root, base, "branch"))
        c.check("... flagged as an overlap", (finding["both_modified"], finding["unsafe"]), (True, True))
        c.check("... the guard refuses and undoes the apply", (quiet(ug.report, root, base, "branch", []), clean(fx)),
                (1, ""))


def test_two_hunks_are_classified_not_a_crash(c):
    """git merge-file exits with the conflict COUNT: two hunks used to read as a failure and raise."""
    with testing.GitFixture() as fx:
        lines = ["l%d\n" % i for i in range(12)]
        b = list(lines)
        m = list(lines)
        b[1], b[9] = "branch one\n", "branch nine\n"
        m[1], m[9] = "main one\n", "main nine\n"
        base = diverged(fx, {"two.txt": "".join(lines)}, {"two.txt": "".join(b)}, {"two.txt": "".join(m)})
        apply3(fx, base)
        root = str(fx.root)
        st = ug.unmerged(root)["two.txt"]
        c.check("a two-hunk overlap is classified (both sides modified the same region)",
                ug.three_way_overlap(root, st[1], st[2], st[3]), True)
        adds = ["l%d\n" % i for i in range(12)]
        ba, ma = list(adds), list(adds)
        ba.insert(2, "branch a\n"), ba.insert(10, "branch b\n")
        ma.insert(2, "main a\n"), ma.insert(10, "main b\n")
    with testing.GitFixture() as fx:
        base = diverged(fx, {"two.txt": "".join(adds)}, {"two.txt": "".join(ba)}, {"two.txt": "".join(ma)})
        apply3(fx, base)
        root = str(fx.root)
        st = ug.unmerged(root)["two.txt"]
        c.check("... and two disjoint insertions are still no overlap",
                ug.three_way_overlap(root, st[1], st[2], st[3]), False)


def test_delete_on_one_side_refuses(c):
    with testing.GitFixture() as fx:
        fx.init()
        fx.commit({"gone.c": "int a;\n", "reg.txt": "see gone.c\n"}, "base")
        fx.branch("branch", checkout=True)
        fx.commit({"gone.c": "int a;\nint b;\n"}, "branch edits gone.c")
        fx.checkout("main")
        fx.commit({"gone.c": None}, "main deletes gone.c")
        root = str(fx.root)
        base = fx.git("merge-base", "main", "branch").strip()
        stage_conflict(fx, "gone.c", {"1": fx.git("rev-parse", base + ":gone.c").strip(),
                                      "3": fx.git("rev-parse", "branch:gone.c").strip()})
        c.check("delete-one-side: the unmerged entry exists", "gone.c" in ug.unmerged(root), True)
        finding = ug.classify(root, "gone.c", ug.unmerged(root)["gone.c"], (set(), set()),
                              ug.rename_sets(root, base, "branch"))
        c.check("... flagged deleted by ours", finding["ours"], "deleted")
        c.check("... the guard refuses, leaving a clean tree", (quiet(ug.report, root, base, "branch", []), clean(fx)),
                (1, ""))


def test_rename_refuses(c):
    with testing.GitFixture() as fx:
        fx.init()
        fx.commit({"a.c": "int a;\n", "reg.txt": "see a.c\n"}, "base")
        fx.branch("branch", checkout=True)
        fx.git("mv", "a.c", "a.cpp")
        fx.commit({"reg.txt": "see a.cpp\n"}, "rename a.c -> a.cpp")
        fx.checkout("main")
        fx.commit({"a.c": "int a; // main edit\n"}, "main edits a.c")
        root = str(fx.root)
        base = fx.git("merge-base", "main", "branch").strip()
        c.check("rename: git sees the rename", "a.c" in ug.rename_sets(root, base, "branch")[0], True)
        stage_conflict(fx, "a.c", {"1": fx.git("rev-parse", base + ":a.c").strip(),
                                   "2": fx.git("rev-parse", "main:a.c").strip()})
        finding = ug.classify(root, "a.c", ug.unmerged(root)["a.c"], ug.rename_sets(root, base, "HEAD"),
                              ug.rename_sets(root, base, "branch"))
        c.check("... flagged renamed by theirs and deleted by theirs",
                ("theirs" in finding["renamed"], finding["theirs"]), (True, "deleted"))
        c.check("... the guard refuses, leaving a clean tree", (quiet(ug.report, root, base, "branch", []), clean(fx)),
                (1, ""))
        c.check("an unknown ref is no renames, not a crash", ug.rename_sets(root, "no-such-ref", "HEAD"), (set(), set()))


def test_refusal_undoes_the_apply(c):
    with testing.GitFixture() as fx:
        base = diverged(fx, {"cfg.txt": "value = 0\n", "shared.c": "int keep;\n"},
                        {"cfg.txt": "value = 1\n", "shared.c": "int keep;\nint from_branch;\n", "new.c": "int fresh;\n"},
                        {"cfg.txt": "value = 2\n"})
        c.check("cleanup: the apply lands the conflict", apply3(fx, base) in (0, 1), True)
        head_cfg = fx.git("show", "HEAD:cfg.txt")
        root = str(fx.root)
        c.check("... the guard refuses", quiet(ug.report, root, base, "branch", []), 1)
        c.check("... no unmerged entry, nothing staged or dirty", (ug.unmerged(root), clean(fx)), ({}, ""))
        c.check("... cfg.txt is HEAD's again, the clean hunk undone, the added file removed",
                ((fx.root / "cfg.txt").read_text(encoding="utf-8"), (fx.root / "shared.c").read_text(encoding="utf-8"),
                 (fx.root / "new.c").exists()), (head_cfg, "int keep;\n", False))


def test_no_cleanup_keeps_the_conflict(c):
    with testing.GitFixture() as fx:
        base = diverged(fx, {"cfg.txt": "value = 0\n"}, {"cfg.txt": "value = 1\n"}, {"cfg.txt": "value = 2\n"})
        apply3(fx, base)
        rc = quiet(ug.main, ["--cwd", str(fx.root), "--base", base, "--branch", "branch", "--no-cleanup"])
        c.check("no-cleanup: the exit stays non-zero, the conflict stays UU",
                (rc, sorted(ug.unmerged(str(fx.root))), fx.git("status", "--short").startswith("UU ")),
                (1, ["cfg.txt"], True))


def test_prose_rewrite_is_the_overlap_signal(c):
    prose = ("<<<<<<< ours\n * a `menu/fn_802A6624.cpp` paragraph\n||||||| base\n * the base paragraph\n=======\n"
             " * a `menu/menu_message.cpp` paragraph\n>>>>>>> theirs\n")
    additive = "<<<<<<< ours\nvoid fn_80002000(void);\n||||||| base\n=======\nvoid fn_80002040(void);\n>>>>>>> theirs\n"
    c.check("a non-empty base refuses, an empty base allows (the guard unions nothing itself)",
            (ug.has_base_region(prose), ug.has_base_region(additive)), (True, False))


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
