"""A long-path batch through the whole landing flow (gate -> stage -> commit -> release) under the 32,767-character
command-line limit, simulated on every OS (`testing.argv_limit`): it lands; the old command-line commit (the mutation)
is caught and ends as outcome `error` with a working recovery; an unrelated staged file stays uncommitted.

The 2026-10-05 header move (687 paths) passed every gate row and then crashed `git commit -- <paths>` with
`[WinError 206]`. The fixture is `test_gate_golden`'s, kind `bulk` (not one of its golden scenarios)."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import contextlib
import io
import json
import os
import shlex
import unittest.mock as mock

from tools.lib import proc, testing
from tools.tests.units.landing.test_gate_golden import BULK_N, Scenario, capture, git

TIER = "fixture"

BULK = Scenario("layout-bulk", "bulk", ["configure.py"], check_outbox=False)


def _via_land(main, units, base, dry_run=False, no_build=False, allow_regression=None, check_outbox=True,
              release_claims=True, no_selftests=False, **_kw):
    """`capture`'s `verify` slot filled by the whole landing flow, its stdout/stderr left to the caller."""
    from tools.units.landing import flow
    return flow.land(main, units, base, no_build, allow_regression, check_outbox=check_outbox,
                     release_claims=release_claims, no_selftests=no_selftests)


def _old_commit_pathspec(main, msg_file, stageable):
    """The pre-2026-10-05 commit step: the whole pathspec on the command line (the mutation the tests must catch)."""
    return proc.run(["git", "commit", "-F", msg_file, "--", *stageable], cwd=main)


def _log_rows(root):
    with open(os.path.join(root, ".pi", "land-log.jsonl"), encoding="utf-8") as fh:
        return [json.loads(line) for line in fh if line.strip()]


def _run_printed(root, argv):
    """Run a printed `git -C <main> ...` command with the fixture's isolated git config (its `core.autocrlf=false`,
    so a host's global `autocrlf=true` does not rewrite a restored file); None when it is not a git command."""
    return testing.GitFixture(root).run(*argv[1:]) if argv[:1] == ["git"] else None


def _count(paths, prefix):
    return sum(1 for p in paths if p.startswith(prefix))


def test_long_path_batch_lands(c):
    """1,500 renames (3,000 pathspec entries) land: one commit holding every new path and no old one."""
    with testing.temp_dir() as root, testing.argv_limit() as limit:
        got = capture(_via_land, BULK, root=root)
        tree = git(root, "ls-tree", "-r", "--name-only", "HEAD").split()
        status = git(root, "status", "--porcelain", "--untracked-files=all", "--", "src", "include").split("\n")
        log = _log_rows(root)
    c.check("the long-path batch lands (exit 0)", got["exit"], 0)
    c.check("... every gate row passed", sorted({r[1] for r in got["rows"] or []}), ["PASS"])
    c.check("... the commit holds all %d new paths" % BULK_N, _count(tree, "src/Bulk/"), BULK_N)
    c.check("... and none of the old ones", _count(tree, "include/Bulk/"), 0)
    c.check("... the batch is fully committed", [s for s in status if s.strip()], [])
    c.check("... the landing log says landed", (log[-1]["outcome"], log[-1]["refused_row"]), ("landed", None))
    c.check("... no launch was refused by the simulated limit", limit.refused, [])


def test_old_commit_is_an_error_with_a_recovery(c):
    """The mutation: the old command-line commit under the same limit must not crash - outcome `error` on the commit
    row in the landing log, MAIN unchanged with the batch staged, and the printed recovery commits the batch."""
    from tools.units.landing import flow
    out, err = io.StringIO(), io.StringIO()

    def land_loud(*a, **k):
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            return _via_land(*a, **k)
    with testing.temp_dir() as root, testing.argv_limit() as limit, \
            mock.patch.object(flow, "commit_pathspec", _old_commit_pathspec):
        got = capture(land_loud, BULK, root=root)
        refused = list(limit.refused)
        subject = git(root, "log", "-1", "--format=%s").strip()
        head = git(root, "rev-parse", "HEAD").strip()
        staged = git(root, "diff", "--cached", "--name-only").split()
        log = _log_rows(root)
        answer = [ln for ln in out.getvalue().splitlines() if ln.strip()]
        recovery = answer[-1].split("recover with: ", 1)[-1] if answer else ""
        argv = shlex.split(recovery)
        p = _run_printed(root, argv)
        tree = git(root, "ls-tree", "-r", "--name-only", "HEAD").split()
    c.check("the old spelling is refused by the simulated limit (the mutation is caught)", len(refused), 1)
    c.check("the landing exits 1, not a traceback", got["exit"], 1)
    c.check("... MAIN did not move (HEAD is still the fixture's base commit)", subject, "base")
    c.check("... the batch is left staged (every new path)", _count(staged, "src/Bulk/"), BULK_N)
    c.check("... the landing log records outcome error on the commit row",
            (log[-1]["outcome"], log[-1]["refused_row"]), ("error", flow.COMMIT_ROW))
    c.check("... with the error text and the batch's size",
            ("206" in log[-1].get("extra", {}).get("error", ""), log[-1].get("extra", {}).get("paths")),
            (True, 2 * BULK_N))
    c.check("... the answer line is an ERROR carrying the recovery",
            (answer[-1].startswith("ERROR ") if answer else False, "--pathspec-from-file=" in recovery), (True, True))
    c.check("... stderr names MAIN's state", "main is still at %s" % head[:9] in err.getvalue(), True)
    c.check("the printed recovery command commits the batch", p is not None and p.returncode, 0)
    c.check("... every new path and none of the old", (_count(tree, "src/Bulk/"), _count(tree, "include/Bulk/")),
            (BULK_N, 0))


def test_commit_step_crash_prints_a_working_abandon(c):
    """A commit step that raises (any exception) on the small header-move batch: exit 1, outcome `error`, and the
    printed abandon command restores every batch path to HEAD (the moved header back, the new path gone)."""
    from tools.tests.units.landing.test_gate_golden import SCENARIOS
    from tools.units.landing import flow
    move = next(s for s in SCENARIOS if s.name == "layout-move")
    out, err = io.StringIO(), io.StringIO()

    def raising(*_a, **_k):
        raise FileNotFoundError(2, "[WinError 206] The filename or extension is too long")

    def land_loud(*a, **k):
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            return _via_land(*a, **k)
    with testing.temp_dir() as root, mock.patch.object(flow, "commit_pathspec", raising):
        got = capture(land_loud, move, root=root)
        log = _log_rows(root)
        before = git(root, "status", "--porcelain", "--", "src", "include").split("\n")
        line = next((ln for ln in err.getvalue().splitlines() if "or abandon it" in ln), "")
        argv = shlex.split(line.split("):", 1)[-1])
        p = _run_printed(root, argv)
        after = git(root, "status", "--porcelain", "--", "src", "include").split("\n")
        exists = (os.path.exists(os.path.join(root, "include", "Net", "old.h")),
                  os.path.exists(os.path.join(root, "src", "Net", "old.h")))
    c.check("a raising commit step exits 1", got["exit"], 1)
    c.check("... logged as error on the commit row", (log[-1]["outcome"], log[-1]["refused_row"]),
            ("error", flow.COMMIT_ROW))
    c.check("... the batch was left applied", [s for s in before if s.strip()] != [], True)
    c.check("the printed abandon command succeeds", p is not None and p.returncode, 0)
    c.check("... and leaves no batch path changed", [s for s in after if s.strip()], [])
    c.check("... the header is back at its old path and gone from the new one", exists, (True, False))


def test_commit_pathspec_leaves_an_unrelated_staged_file(c):
    """Over the argv budget the commit still commits ONLY the batch's paths: an unrelated staged file stays staged."""
    from tools.units.landing import stage
    with testing.temp_dir() as root:
        fx = testing.GitFixture(root).init()
        names = ["src/Net/file_%03d.cpp" % i for i in range(40)]
        fx.commit({n: "base\n" for n in names + ["tools/units/other.py"]}, "base")
        for n in names + ["tools/units/other.py"]:
            with open(os.path.join(root, *n.split("/")), "w", encoding="utf-8", newline="\n") as fh:
                fh.write("edit\n")
        git(root, "add", "--", "tools/units/other.py")
        msg = os.path.join(root, ".git", "land_msg.txt")
        with open(msg, "w", encoding="utf-8") as fh:
            fh.write("game/net: the batch\n")
        with mock.patch.object(proc, "ARGV_BUDGET", 100):
            stage.stage_batch(root, names)
            p = stage.commit_pathspec(root, msg, names)
        c.check("the stdin pathspec commit succeeds", p.returncode, 0)
        c.check("... it read the pathspec from stdin", "--pathspec-file-nul" in p.args, True)
        c.check("... every batch path is committed", git(root, "diff", "--name-only", "HEAD~1", "HEAD").split(), names)
        c.check("... the unrelated staged file is NOT committed", git(root, "show", "HEAD:tools/units/other.py"),
                "base\n")
        c.check("... and is still staged", git(root, "diff", "--cached", "--name-only").split(),
                ["tools/units/other.py"])


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
