#!/usr/bin/env python3
"""lanecmd.py - the one place a campaign lane's launch command is built.

A lane is a headless Claude Code session: `claude --agent <profile> -p <task>` run **with its cwd at the
lane's slot**.  The Agent tool has no `cwd` parameter, and the slot design rests on "one directory per lane",
so a lane is a separate `claude` process rather than an in-session subagent.  Three producers used to print
their own launch line (`slots.spawn`, `queue.spawn_line`, `backlog.lane_task`); they all call `lane_call`
now, so the flags cannot drift between them.

The command is POSIX-shell text (git-bash on Windows).  The task is single-quoted with `shlex.quote`, because
tasks carry backticks and `$`; a task too long for a command line is staged as a file under
`<main>/.pi/lanes/` and fed on stdin instead.

Answering a lane's question is `claude --resume <session-id> -p "<ruling>"` in the same cwd (`resume_call`):
the session id is chosen here, at launch, so the orchestrator never has to discover it.

  python tools/units/lanecmd.py --selftest
"""
from __future__ import annotations

import argparse
import os
import shlex
import sys
import uuid

#: Longer than this and the task goes to a file on stdin (Windows caps a command line near 32 K characters).
INLINE_LIMIT = 8000
#: Overridable, so a host where `claude` is not on PATH names the binary once.
BIN_ENV = "CLAUDE_BIN"
#: `acceptEdits` auto-approves file edits; `Bash` is allowed by name because a lane's whole job is running
#: the project's tools.  Override with LANE_PERMISSION_MODE / LANE_ALLOWED_TOOLS (or pass them in).
MODE_ENV = "LANE_PERMISSION_MODE"
TOOLS_ENV = "LANE_ALLOWED_TOOLS"


def _fwd(path: str) -> str:
    return path.replace("\\", "/")


def lanes_dir(main: str) -> str:
    """`<main>/.pi/lanes` - gitignored scratch where a long task is staged."""
    return os.path.join(main, ".pi", "lanes")


def stage_task(main: str, key: str, task: str) -> str:
    """Write `task` to `<main>/.pi/lanes/<key>.task.md` and return the path (forward slashes)."""
    os.makedirs(lanes_dir(main), exist_ok=True)
    path = os.path.join(lanes_dir(main), "%s.task.md" % key)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(task.rstrip("\n") + "\n")
    return _fwd(path)


def lane_call(agent: str, cwd: str, task: str, name: str | None = None, session_id: str | None = None,
              main: str | None = None, key: str | None = None, log_file: str | None = None) -> dict:
    """The launch command for one lane, plus the ids the orchestrator needs to follow it.

    Returns `{"call", "session_id", "task_file", "log_file"}`.  `task_file`/`log_file` are `None` when the
    task went inline / no log was asked for.  `main` + `key` are needed only to stage an over-long task; a
    task that long with no `main` is refused rather than truncated.
    """
    session_id = session_id or str(uuid.uuid4())
    parts = [os.environ.get(BIN_ENV) or "claude", "--agent", shlex.quote(agent)]
    if name:
        parts += ["--name", shlex.quote(name)]
    parts += ["--session-id", session_id,
              "--permission-mode", shlex.quote(os.environ.get(MODE_ENV) or "acceptEdits"),
              "--allowedTools", shlex.quote(os.environ.get(TOOLS_ENV) or "Bash")]
    task_file = None
    if len(task) > INLINE_LIMIT:
        if not (main and key):
            raise SystemExit("REFUSED lane_call: the task is %d characters (limit %d) and there is no MAIN/key "
                             "to stage it under" % (len(task), INLINE_LIMIT))
        task_file = stage_task(main, key, task)
        parts += ["-p", "<", shlex.quote(task_file)]
    else:
        parts += ["-p", shlex.quote(task)]
    if log_file:
        parts += [">", shlex.quote(_fwd(log_file)), "2>&1"]
    call = "cd %s && %s" % (shlex.quote(_fwd(cwd)), " ".join(parts))
    return {"call": call, "session_id": session_id, "task_file": task_file, "log_file": log_file}


def resume_call(cwd: str, session_id: str, message: str) -> str:
    """The command that answers a lane: resume its session in its own cwd with the orchestrator's ruling."""
    return "cd %s && %s --resume %s -p %s" % (shlex.quote(_fwd(cwd)), os.environ.get(BIN_ENV) or "claude",
                                              session_id, shlex.quote(message))


def selftest() -> int:
    import tempfile
    fails, n = [], 0

    def check(name, got, want):
        nonlocal n
        n += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    saved = {k: os.environ.pop(k, None) for k in (BIN_ENV, MODE_ENV, TOOLS_ENV)}
    try:
        r = lane_call("fixer", "C:\\a\\b.slot1", "fix `x` and $HOME", name="fixer-x", session_id="sid-1")
        check("cwd is forward-slashed and quoted first", r["call"].startswith("cd C:/a/b.slot1 && claude "), True)
        check("agent flag", "--agent fixer" in r["call"], True)
        check("name flag", "--name fixer-x" in r["call"], True)
        check("session id is the one given", "--session-id sid-1" in r["call"] and r["session_id"] == "sid-1", True)
        check("permission mode defaults to acceptEdits", "--permission-mode acceptEdits" in r["call"], True)
        check("Bash is allowed by name", "--allowedTools Bash" in r["call"], True)
        check("a task with backticks and $ is single-quoted", "-p 'fix `x` and $HOME'" in r["call"], True)
        check("no task file for a short task", r["task_file"], None)
        r2 = lane_call("worker", "/s", "it's", session_id="s2")
        check("an embedded quote survives shlex", shlex.split(r2["call"].split("-p ", 1)[1])[0], "it's")
        check("a fresh session id is a uuid", len(lane_call("worker", "/s", "x")["session_id"]), 36)
        r3 = lane_call("worker", "/s", "x", log_file="C:\\l\\o.log")
        check("a log file redirects both streams", r3["call"].endswith("> C:/l/o.log 2>&1"), True)
        os.environ[MODE_ENV] = "plan"
        os.environ[BIN_ENV] = "C:/bin/claude.exe"
        r4 = lane_call("worker", "/s", "x")
        check("the mode and binary are overridable",
              "--permission-mode plan" in r4["call"] and "&& C:/bin/claude.exe " in r4["call"], True)
        os.environ.pop(MODE_ENV)
        os.environ.pop(BIN_ENV)
        big = "y" * (INLINE_LIMIT + 1)
        try:
            lane_call("worker", "/s", big)
            check("an over-long task with nowhere to stage is refused", "no error", "SystemExit")
        except SystemExit:
            check("an over-long task with nowhere to stage is refused", True, True)
        with tempfile.TemporaryDirectory() as tmp:
            r5 = lane_call("worker", "/s", big, main=tmp, key="slot1")
            check("an over-long task is staged and fed on stdin", "-p < " in r5["call"], True)
            check("... the file holds the whole task",
                  open(r5["task_file"], encoding="utf-8").read().rstrip("\n"), big)
        check("resume answers in the lane's cwd",
              resume_call("C:\\a", "sid-1", "yes"), "cd C:/a && claude --resume sid-1 -p yes")
    finally:
        for k, v in saved.items():
            if v is not None:
                os.environ[k] = v
    if fails:
        print("FAIL %d/%d" % (len(fails), n))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % n)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.selftest:
        return selftest()
    ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
