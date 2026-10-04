"""lib.git on a GitFixture: each call's shape, blobs byte-exact, the pathspec refusals, worktrees and merges."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import tempfile
from pathlib import Path

from tools.lib import testing
from tools.lib.git import Git, GitError, Worktree

TIER = "fixture"

# lib.git inherits the environment: give it the fixture's isolation and an identity for its own commits
os.environ.update(GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_NOSYSTEM="1", GIT_TERMINAL_PROMPT="0",
                  GIT_AUTHOR_NAME="fixture", GIT_AUTHOR_EMAIL="fixture@example.invalid",
                  GIT_COMMITTER_NAME="fixture", GIT_COMMITTER_EMAIL="fixture@example.invalid")


def test_refs_and_history(c):
    with testing.GitFixture() as fx:
        fx.init()
        first = fx.commit({"a.txt": "a\n"}, "first")
        fx.branch("topic", checkout=True)
        second = fx.commit({"b.txt": "b\n"}, "second")
        g = Git(fx.root)
        c.check("head and rev_parse", (g.head(), g.rev_parse("main"), g.rev_parse("no-such-ref")), (second, first, None))
        c.check("current_branch", g.current_branch(), "topic")
        c.check("merge_base / fork_point", (g.merge_base("main", "topic"), g.fork_point("topic")), (first, first))
        c.check("is_ancestor both ways", (g.is_ancestor("main", "topic"), g.is_ancestor("topic", "main")), (True, False))
        c.check("refs(prefix)", g.refs("refs/heads/"), {"refs/heads/main": first, "refs/heads/topic": second})
        c.check("branch_exists", (g.branch_exists("main"), g.branch_exists("nope")), (True, False))
        g.branch_create("made", "main")
        c.check("branch_create", g.rev_parse("made"), first)
        g.branch_delete("made", force=True)
        c.check("branch_delete", g.branch_exists("made"), False)
        g.update_ref("refs/rescue/x", first)
        c.check("update_ref writes", g.rev_parse("refs/rescue/x"), first)
        g.update_ref("refs/rescue/x", delete=True)
        c.check("... and deletes", g.rev_parse("refs/rescue/x"), None)
        c.check("diff_names", g.diff_names("main", "topic"), ["b.txt"])
        fx.git("mv", "b.txt", "c.txt")
        fx.git("commit", "-q", "-m", "rename")
        c.check("renames", g.renames("topic~1", "topic"), [("b.txt", "c.txt")])
        c.check("toplevel is forward-slashed", g.toplevel(), fx.root.as_posix())
        p = g.run("rev-parse", "--verify", "nope")
        c.check("run never raises by default", p.returncode != 0, True)
        err = c.raises("run(check=True) raises GitError", GitError, g.run, "rev-parse", "--verify", "nope", check=True)
        c.check("... carrying the argv and stderr", (err and err.argv[:2], bool(err and err.stderr)),
                (("rev-parse", "--verify"), True))
        c.check("out() returns stdout", g.out("rev-parse", "HEAD").strip(), g.head())
        c.check("ok()", (g.ok("rev-parse", "HEAD"), g.ok("rev-parse", "--verify", "nope")), (True, False))


def test_blobs_index_status(c):
    with testing.GitFixture() as fx:
        fx.init()
        fx.commit({"crlf.txt": b"one\r\ntwo\r\n", "dir/x.c": "x\n"}, "blobs")
        g = Git(fx.root)
        c.check("show returns the blob bytes unchanged (CRLF kept)", g.show("HEAD", "crlf.txt"), b"one\r\ntwo\r\n")
        c.check("show of an absent path is None", g.show("HEAD", "nope.txt"), None)
        c.check("show takes a backslashed path", g.show("HEAD", "dir\\x.c"), b"x\n")
        many = g.show_many("HEAD", ["crlf.txt", "nope.txt", "dir", "dir\\x.c"])
        c.check("show_many reads each blob as show does; an absent path or a tree is None",
                many, {"crlf.txt": b"one\r\ntwo\r\n", "nope.txt": None, "dir": None, "dir/x.c": b"x\n"})
        c.check("show_many of nothing runs nothing", g.show_many("HEAD", []), {})
        c.check("show_many at a bad ref is all None", g.show_many("no-such-ref", ["crlf.txt"]), {"crlf.txt": None})
        (fx.root / "dir" / "x.c").write_text("staged\n", encoding="utf-8", newline="\n")
        (fx.root / "new.txt").write_text("n\n", encoding="utf-8", newline="\n")
        c.raises("stage refuses an empty pathspec", ValueError, g.stage, [])
        g.stage(["dir/x.c"])
        c.check("cat_index reads the staged blob", g.cat_index("dir/x.c"), b"staged\n")
        c.check("cat_index of an untracked path is None", g.cat_index("new.txt"), None)
        c.check("status_porcelain", sorted(g.status_porcelain()), [("??", "new.txt"), ("M ", "dir/x.c")])
        c.check("ls_files", g.ls_files(), ["crlf.txt", "dir/x.c"])
        eol = {row[3]: row[:2] for row in g.ls_files(eol=True)}
        c.check("ls_files(eol=True) classes index and worktree", eol.get("crlf.txt"), ("crlf", "crlf"))
        g.unstage(["dir/x.c"])
        c.check("unstage", g.cat_index("dir/x.c"), b"x\n")
        g.stage(["dir/x.c", "new.txt"])
        with tempfile.TemporaryDirectory() as tmp:
            msg = Path(tmp) / "msg.txt"
            msg.write_text("commit through lib.git\n", encoding="utf-8")
            c.raises("commit refuses an empty pathspec", ValueError, g.commit, msg, [])
            sha = g.commit(msg, ["dir/x.c"])
        c.check("commit commits exactly the pathspec", (sha == g.head(), g.show("HEAD", "dir/x.c"),
                                                         g.show("HEAD", "new.txt")), (True, b"staged\n", None))


def test_worktrees_and_merges(c):
    with testing.GitFixture() as fx, tempfile.TemporaryDirectory() as tmp:
        fx.init()
        fx.commit({"f.txt": "base\n"}, "base")
        g = Git(fx.root)
        wt = Path(tmp) / "wt"
        g.worktree_add(wt, "lane")
        rows = g.worktree_list()
        c.check("worktree_list: MAIN first, then the lane",
                [(Path(r.path).name, r.branch) for r in rows], [(fx.root.name, "main"), ("wt", "lane")])
        c.expect("rows are Worktree values", all(isinstance(r, Worktree) for r in rows))
        c.check("common_dir of a worktree is MAIN's .git", Git(wt).common_dir(), (fx.root / ".git").as_posix())
        g.worktree_remove(wt, force=True)
        c.check("worktree_remove", [Path(r.path).name for r in g.worktree_list()], [fx.root.name])
        a, b = fx.conflict("f.txt", "ours\n", "theirs\n")
        c.check("merge_tree reports a conflict with a non-zero exit", g.merge_tree(a, b).returncode != 0, True)
        clean = fx.commit({"g.txt": "g\n"}, "unrelated")
        c.check("... and a clean merge with 0", g.merge_tree("main", clean).returncode, 0)
        base, ours, theirs = (Path(tmp) / n for n in ("base", "ours", "theirs"))
        base.write_bytes(b"1\n2\n3\n")
        ours.write_bytes(b"1\nX\n3\n")
        theirs.write_bytes(b"1\nY\n3\n")
        merged, conflicts = g.merge_file_diff3(ours, base, theirs)
        c.check("merge_file_diff3 counts the conflicts", conflicts, 1)
        c.contains("... and shows the base in diff3 style", merged, b"|||||||")
        c.check("... writing nothing", ours.read_bytes(), b"1\nX\n3\n")
        two, count = g.merge_bytes(b"1\nX\n3\nA\n5\n", b"1\n2\n3\n4\n5\n", b"1\nY\n3\nB\n5\n",
                                   labels=("ours", "base", "theirs"))
        c.check("merge_bytes: two conflict hunks are a count of 2, not an error", count, 2)
        c.check("... the labels name the sides in the markers",
                [ln for ln in two.split(b"\n") if ln[:7] in (b"<<<<<<<", b"|||||||", b">>>>>>>")][:3],
                [b"<<<<<<< ours", b"||||||| base", b">>>>>>> theirs"])
        c.check("... a clean merge is a count of 0", g.merge_bytes(b"1\nX\n3\n", b"1\n2\n3\n", b"1\n2\n3\n"),
                (b"1\nX\n3\n", 0))
        c.raises("... and a git failure (a missing input) raises", GitError, g.merge_file_diff3,
                 Path(tmp) / "missing", base, theirs)
        fx.checkout(a)
        fx.run("merge", b)
        c.check("unmerged_stages lists base/ours/theirs", sorted(g.unmerged_stages().get("f.txt", {})), [1, 2, 3])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
