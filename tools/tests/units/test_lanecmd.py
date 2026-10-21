"""lanecmd: the launch line (cwd quoting, flags, a fresh session id, the task staged on stdin when too long) and
the resume line, through the tool's own re-exported names."""
import sys, pathlib; sys.path.insert(0, str(next(p for p in pathlib.Path(__file__).resolve().parents if (p / "tools" / "__init__.py").is_file())))
import os
import shlex
import tempfile

from tools.lib import testing
from tools.units import lanecmd

TIER = "fixture"
ENV = (lanecmd.BIN_ENV, lanecmd.MODE_ENV, lanecmd.TOOLS_ENV)


def _clean_env():
    return {k: os.environ.pop(k, None) for k in ENV}


def _restore(saved):
    for k in ENV:
        os.environ.pop(k, None)
    for k, v in saved.items():
        if v is not None:
            os.environ[k] = v


def test_launch_line(c):
    saved = _clean_env()
    try:
        r = lanecmd.lane_call("fixer", "C:\\a\\b.slot1", "fix `x` and $HOME", name="fixer-x", session_id="sid-1")
        c.check("cwd is forward-slashed and quoted first", r["call"].startswith("cd C:/a/b.slot1 && claude "), True)
        c.contains("agent flag", r["call"], "--agent fixer")
        c.contains("name flag", r["call"], "--name fixer-x")
        c.check("session id is the one given", ("--session-id sid-1" in r["call"], r["session_id"]), (True, "sid-1"))
        c.contains("permission mode defaults to acceptEdits", r["call"], "--permission-mode acceptEdits")
        c.contains("Bash is allowed by name", r["call"], "--allowedTools Bash")
        c.contains("a task with backticks and $ is single-quoted", r["call"], "-p 'fix `x` and $HOME'")
        c.check("no task file for a short task", r["task_file"], None)
        r2 = lanecmd.lane_call("worker", "/s", "it's", session_id="s2")
        c.check("an embedded quote survives shlex", shlex.split(r2["call"].split("-p ", 1)[1])[0], "it's")
        c.check("a fresh session id is a uuid", len(lanecmd.lane_call("worker", "/s", "x")["session_id"]), 36)
        r3 = lanecmd.lane_call("worker", "/s", "x", log_file="C:\\l\\o.log")
        c.check("a log file redirects both streams", r3["call"].endswith("> C:/l/o.log 2>&1"), True)
        os.environ[lanecmd.MODE_ENV] = "plan"
        os.environ[lanecmd.BIN_ENV] = "C:/bin/claude.exe"
        r4 = lanecmd.lane_call("worker", "/s", "x")
        c.check("the mode and binary are overridable",
                "--permission-mode plan" in r4["call"] and "&& C:/bin/claude.exe " in r4["call"], True)
    finally:
        _restore(saved)


def test_long_task_and_resume(c):
    saved = _clean_env()
    try:
        big = "y" * (lanecmd.INLINE_LIMIT + 1)
        c.raises("an over-long task with nowhere to stage is refused", SystemExit, lanecmd.lane_call, "worker", "/s", big)
        with tempfile.TemporaryDirectory() as tmp:
            r5 = lanecmd.lane_call("worker", "/s", big, main=tmp, key="slot1")
            c.contains("an over-long task is staged and fed on stdin", r5["call"], "-p < ")
            with open(r5["task_file"], encoding="utf-8") as fh:
                c.check("... the file holds the whole task", fh.read().rstrip("\n"), big)
            c.check("... under MAIN/.pi/lanes", os.path.dirname(r5["task_file"]).replace("\\", "/"),
                    lanecmd.lanes_dir(tmp).replace("\\", "/"))
        c.check("resume answers in the lane's cwd",
                lanecmd.resume_call("C:\\a", "sid-1", "yes"), "cd C:/a && claude --resume sid-1 -p yes")
    finally:
        _restore(saved)


def test_launch_cwd_check(c):
    """`cwd_problem` accepts a claimed slot (on a branch, `.used` marker present) and a claim record's worktree, and
    refuses MAIN, an unclaimed slot (detached, or on a branch with no marker) and any other directory; `lane_call`
    refuses on `cwd_check="refuse"` and only reports on `"warn"`, its call staying one line.
    Mutation: accepting a slot on a branch without its marker fails the "no marker" check."""
    import json
    from tools.lib.lanes import pool
    with tempfile.TemporaryDirectory() as tmp:
        fx = testing.GitFixture(os.path.join(tmp, "repo")).init()
        fx.commit({"a.txt": "a\n"}, "base")
        main = str(fx.root)
        os.makedirs(os.path.join(main, ".pi", "slots"), exist_ok=True)
        with open(os.path.join(main, ".pi", "slots", "pool.json"), "w", encoding="utf-8") as fh:
            json.dump({"count": 3}, fh)
        s1, s2, s3 = (pool.slot_dir(main, n) for n in (1, 2, 3))
        fx.git("worktree", "add", "-q", "-b", "lane/a", s1)
        fx.git("worktree", "add", "-q", "--detach", s2)
        fx.git("worktree", "add", "-q", "-b", "lane/c", s3)
        pool.mark_used(s1, "g3d/a", "lane/a", "lane/a")
        other = os.path.join(tmp, "elsewhere")
        claimed_wt = os.path.join(tmp, "claim-wt")
        os.makedirs(other)
        os.makedirs(claimed_wt)
        with open(os.path.join(main, ".pi", "claims.json"), "w", encoding="utf-8") as fh:
            json.dump({"g3d/b": {"worktree": claimed_wt, "branch": "worker/b"}}, fh)
        c.check("a claimed slot is a launch cwd", lanecmd.cwd_problem(main, s1), None)
        c.check("a claim record's worktree is a launch cwd", lanecmd.cwd_problem(main, claimed_wt), None)
        c.contains("MAIN is refused", lanecmd.cwd_problem(main, main) or "", "cwd is MAIN")
        c.contains("a detached slot is refused with the acquire remedy", lanecmd.cwd_problem(main, s2) or "",
                   "slots.py acquire <unit> --slot 2")
        c.contains("a slot on a branch with no marker is refused", lanecmd.cwd_problem(main, s3) or "",
                   "`.used` marker missing")
        c.contains("any other directory is refused", lanecmd.cwd_problem(main, other) or "",
                   "neither a slot of the pool nor a claim's worktree")
        c.contains("a missing directory is refused", lanecmd.cwd_problem(main, os.path.join(tmp, "nope")) or "",
                   "does not exist")
        c.raises("lane_call refuses an unclaimed cwd on cwd_check='refuse'", SystemExit, lanecmd.lane_call,
                 "worker", s2, "x", main=main, cwd_check="refuse")
        ok = lanecmd.lane_call("worker", s1, "x", main=main, cwd_check="refuse")
        c.check("... and builds the line for a claimed one", (ok["cwd_warning"], ok["call"].startswith("cd ")),
                (None, True))
        warned = lanecmd.lane_call("worker", main, "x", main=main, cwd_check="warn")
        c.check("'warn' reports the problem beside a one-line call",
                ("cwd is MAIN" in (warned["cwd_warning"] or ""), "\n" in warned["call"]), (True, False))
        import contextlib
        import io
        with contextlib.redirect_stdout(io.StringIO()):
            codes = (lanecmd.main(["check", s2, "--main", main]), lanecmd.main(["check", s1, "--main", main]))
        c.check("the CLI exits 1 on an unclaimed slot and 0 on a claimed one", codes, (1, 0))
        for s in (s1, s2, s3):
            fx.git("worktree", "remove", "--force", s)


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
