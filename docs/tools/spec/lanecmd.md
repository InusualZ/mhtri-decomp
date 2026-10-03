# `lanecmd` - The one builder of a lane's `claude --agent .. -p ..` launch line and the resume line

<!-- generated from the module docstring of `tools/units/lanecmd.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

lanecmd.py - the one place a campaign lane's launch command is built.

## Users

CLAUDE.md (1); docs (5); imported by `backlog`, `queue`, `slots`

## CLI

Flags: `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: agent, cwd, task -> shell text.

## Invariants and rules

* The command is POSIX-shell text (git-bash on Windows). The task is single-quoted with `shlex.quote`, because tasks carry backticks and `$`; a task too long for a command line is staged as a file under `<main>/.pi/lanes/` and fed on stdin instead.
* Answering a lane's question is `claude --resume <session-id> -p "<ruling>"` in the same cwd (`resume_call`): the session id is chosen here, at launch, so the orchestrator never has to discover it.
* python tools/units/lanecmd.py --selftest

## Lib dependencies

lanes.launch.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_lanecmd.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* A lane is a headless Claude Code session: `claude --agent <profile> -p <task>` run **with its cwd at the lane's slot**. The Agent tool has no `cwd` parameter, and the slot design rests on "one directory per lane", so a lane is a separate `claude` process rather than an in-session subagent. Three producers used to print their own launch line (`slots.spawn`, `queue.spawn_line`, `backlog.lane_task`); they all call `lane_call` now, so the flags cannot drift between them.
