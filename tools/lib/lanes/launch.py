"""The launch line: a lane is a headless `claude --agent <profile> -p <task>` run with its cwd at the slot; the
kind -> profile table; the resume line that answers a lane. Spec: docs/tools/spec/lib-lanes.md. CLI: none (library)."""
from __future__ import annotations

import os
import shlex
import uuid

#: Longer than this and the task is staged as a file and fed on stdin (Windows caps a command line near 32 K).
INLINE_LIMIT = 8000
BIN_ENV = "CLAUDE_BIN"
MODE_ENV = "LANE_PERMISSION_MODE"
TOOLS_ENV = "LANE_ALLOWED_TOOLS"

#: A lane's kind of work -> the agent profile it is launched with. Exhaustive: an unknown kind is refused.
KIND_PROFILE = {
    "unit": "surveyor",        # survey a claim, then reconstruct - the four-leg unit loop
    "fix": "fixer",            # a refused gate or a measured regression on a branch
    "merge": "merger",         # a refused apply
    "tooling": "worker",       # the fallback for a task that is none of the specific ones
    "docs": "worker",
    "review": "codereviewer",  # the tracked project review profile
    "scout": "scout",          # read-only
    "plan": "planner",         # read-only
}
#: Every profile a lane can be launched with (an override is validated against this).
PROFILES = tuple(sorted(set(KIND_PROFILE.values())))


def profile_for_kind(kind: str) -> str:
    """The agent profile for `kind`; SystemExit naming the valid kinds for an unknown one (never a guess)."""
    try:
        return KIND_PROFILE[kind]
    except KeyError:
        raise SystemExit("REFUSED: unknown kind %r - valid kinds are: %s" % (kind, ", ".join(KIND_PROFILE)))


def _fwd(path: str) -> str:
    return path.replace("\\", "/")


def lanes_dir(main: str) -> str:
    """`<main>/.pi/lanes` - gitignored scratch where a long task is staged."""
    return os.path.join(main, ".pi", "lanes")


def stage_task(main: str, key: str, task: str) -> str:
    """Write `task` to `<main>/.pi/lanes/<key>.task.md`; the path, forward-slashed."""
    os.makedirs(lanes_dir(main), exist_ok=True)
    path = os.path.join(lanes_dir(main), "%s.task.md" % key)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(task.rstrip("\n") + "\n")
    return _fwd(path)


def lane_call(agent: str, cwd: str, task: str, name: str | None = None, session_id: str | None = None,
              main: str | None = None, key: str | None = None, log_file: str | None = None) -> dict:
    """The POSIX-shell launch command -> `{"call", "session_id", "task_file", "log_file"}`. The task is
    single-quoted (`shlex.quote`); one over `INLINE_LIMIT` is staged under `main`/`key` (refused without them)."""
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


def your_tree_lines(lines: list[str], b: dict) -> None:
    """Append the brief's section 0 "your tree" block for `b["worktree"]` / `b["main"]`: the one directory the
    lane may write, and the `git rev-parse --show-toplevel` self-check that stops a mis-launched lane."""
    wt = (b.get("worktree") or "").replace("\\", "/")
    main = (b.get("main") or "").replace("\\", "/")
    lines.append("## 0 · Your tree, then acknowledge")
    lines.append("")
    if wt:
        lines.append("**Your tree is `%s`.** It is the only directory you may write to." % wt)
    else:
        lines.append("**Your tree is the worktree the spawn line's `cwd` names.** It is the only directory "
                     "you may write to.")
    lines.append("")
    lines.append("* Write **nothing** outside it - not MAIN, not a sibling slot, not a new directory at the "
                 "repository root. Scratch belongs under *that tree's own* ignored paths (`build/`, `*.ctx`, "
                 "`d<digits>.json` - whatever `.gitignore` already covers there). MAIN's working tree belongs "
                 "to the orchestrator, and an edit there blocks every other lane's landing gate.")
    lines.append("* **Self-check before you do any work.** Run this as your first tool call:")
    lines.append("")
    lines.append("```sh")
    lines.append("git rev-parse --show-toplevel     # must print exactly: %s" % (wt or "your tree"))
    lines.append("```")
    lines.append("")
    lines.append("If it prints anything else - MAIN%s, another slot, a `.ws-*` tree - **STOP and report it "
                 "instead of working.** A lane launched in the wrong tree silently pollutes MAIN; catching "
                 "it here costs one turn, missing it costs a refused landing."
                 % (" (`%s`)" % main if main else ""))
    lines.append("")
    lines.append("* **A command the harness refuses cannot be confirmed.** A subagent has no UI for the "
                 "\"dangerous command\" prompt, so a retry can never succeed - the lane that did this "
                 "sat on a blocked `bash` for minutes of its budget. Rewrite it as the narrowest explicit "
                 "command (one path, no recursive delete) or report it. Never `rm -rf`, `git clean -fdx` "
                 "or `git checkout .` in your tree - if the tree itself needs repairing, ask the "
                 "orchestrator.")


def teardown_lines(lines: list[str]) -> None:
    """Append the `claims.py release` ban (run from inside a worktree it wiped the directory)."""
    lines.append("")
    lines.append("**NEVER run `claims.py release`.** From inside a worktree it WIPED the directory and")
    lines.append("deregistered it while reporting \"teardown is INCOMPLETE, the claim is kept\" - that lane")
    lines.append("lost its tree, and it survived only because it had committed first; uncommitted work would")
    lines.append("not have. Teardown is the orchestrator's job. Leave your worktree where it is.")


def tree_block(main: str, path: str) -> str:
    """The "your tree" block a spawned lane gets: the brief's section-0 block, the release ban, do-not-land."""
    lines: list[str] = []
    your_tree_lines(lines, {"worktree": path, "main": main})
    teardown_lines(lines)
    lines.append("**Do not land.** A worker never runs `land.py` and never commits on `main` - landing is the")
    lines.append("orchestrator's job, exactly as teardown is.")
    return "\n".join(lines).strip("\n")


def resume_call(cwd: str, session_id: str, message: str) -> str:
    """The command that answers a lane: resume its session in its own cwd with the ruling."""
    return "cd %s && %s --resume %s -p %s" % (shlex.quote(_fwd(cwd)), os.environ.get(BIN_ENV) or "claude",
                                              session_id, shlex.quote(message))
