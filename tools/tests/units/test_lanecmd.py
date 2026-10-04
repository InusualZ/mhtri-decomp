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


if __name__ == "__main__":
    raise SystemExit(testing.run(globals()))
