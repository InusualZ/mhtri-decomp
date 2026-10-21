# `lanecmd` - The one builder of a lane's `claude --agent .. -p ..` launch line and the resume line

<!-- generated from the module docstring of `tools/units/lanecmd.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

lanecmd.py - the one place a campaign lane's launch command is built.

## Users

CLAUDE.md (1); docs (5); a re-export of `lib.lanes.launch` (`backlog`, `queue` and `slots` import the lib directly since WP3e)

## CLI

```
python tools/units/lanecmd.py check [CWD] [--main M]   # may a lane be launched in CWD (default: the current directory)?
```
Flags: `check`'s `--main` (default: MAIN resolved from CWD with git).
Exit codes: `check` 0 when CWD is a claimed lane tree, 1 with the reason and the `slots.py acquire` remedy otherwise; with no
subcommand the help, 0.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: agent, cwd, task -> shell text.

## Invariants and rules

* The command is POSIX-shell text (git-bash on Windows). The task is single-quoted with `shlex.quote`, because tasks carry backticks and `$`; a task too long for a command line is staged as a file under `<main>/.pi/lanes/` and fed on stdin instead.
* Answering a lane's question is `claude --resume <session-id> -p "<ruling>"` in the same cwd (`resume_call`): the session id is chosen here, at launch, so the orchestrator never has to discover it.
* **The launch cwd is a claimed lane tree (2026-10-21).** `launch.cwd_problem(main, cwd)` accepts a slot of the pool that
  holds a claim - on a branch and carrying its `.used` marker, `pool.claim_problem`, the same rule `worktreehook arm --slot N`
  applies - or a worktree a claim record names; MAIN, a missing directory, an unclaimed slot and any other directory are
  problems naming the `slots.py acquire <unit> --slot N --branch B --worker W` remedy. `lane_call(..., cwd_check=)`:
  `"refuse"` (what `slots.spawn` passes) raises on a problem, `"warn"` (what `backlog.lane_task`/`debt_task` pass: their
  suggested lane's cwd defaults to MAIN) returns it as `cwd_warning` and the call stays one line; the default builds the
  line unchecked. `queue.spawn_line` keeps its own MAIN refusal (its dry run prints a line for a slot it has not claimed).
  Measured on the live pool: slot 4 (claimed by `queue.py`) passes; MAIN, the free slot 8 and slots 1 and 6 (on branches
  with no `.used` marker) exit 1.

## Lib dependencies

lanes.launch.

## Test contract

Tier: fixture.
`tools/tests/units/test_lanecmd.py`. `test_launch_cwd_check` builds a `GitFixture` MAIN with three slot worktrees (claimed,
detached, on a branch with no marker), a claim record's worktree and a stray directory; accepting a branch without its
marker fails 1 check.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* A lane is a headless Claude Code session: `claude --agent <profile> -p <task>` run **with its cwd at the lane's slot**. The Agent tool has no `cwd` parameter, and the slot design rests on "one directory per lane", so a lane is a separate `claude` process rather than an in-session subagent. Three producers used to print their own launch line (`slots.spawn`, `queue.spawn_line`, `backlog.lane_task`); they all call `lane_call` now, so the flags cannot drift between them.
