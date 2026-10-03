# `worktreehook` - PROTOTYPE: Claude Code WorktreeCreate/Remove hooks that hand out a slot against an arm token

<!-- generated from the module docstring of `tools/units/worktreehook.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

worktreehook.py - Claude Code's WorktreeCreate / WorktreeRemove hooks, backed by the slot pool.

## Users

CLAUDE.md (3); docs (1)

## CLI

Subcommands: `arm`, `disarm`, `status`, `create`, `remove`.
Flags: `--selftest`, `--slot`, `--ttl`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: hook JSON -> slot path.

## Invariants and rules

* PROTOTYPE. An Agent-tool subagent launched with `isolation: "worktree"` gets its working directory from the `WorktreeCreate` hook, so the hook can hand it a **slot** instead of a throwaway git worktree - which is how an in-session subagent gets the slot isolation the headless launcher (`lanecmd.py`) gets from `cd <slot>`.
* **Scoped by arming.** A project hook fires for every worktree Claude Code creates here, and most of those are not campaign lanes (a manual `--worktree`, an ad hoc isolated subagent). So the hook hands out a slot only against an **arm token**: the orchestrator runs `worktreehook.py arm N` right before launching N lanes, each `create` consumes one token (an atomic rename, so parallel hooks cannot share one) and tokens expire after `--ttl` seconds so a forgotten arm cannot capture a later launch. With no live token the hook does what Claude Code would have done itself - a plain git worktree under `.claude/worktrees/<name>` - and `remove` cleans that kind up the same way.
* **A token can name a slot** (`arm --slot 3`): the orchestrator has already claimed that slot for a unit (`queue.py next` acquires one and cuts the claim's branch), and the token binds the launch to it - `create` hands that very slot over instead of acquiring another, so the claim, its branch, its brief and the lane share one directory with no adoption step. A token with no slot means "any current free slot".
* create reads the hook JSON on stdin. Armed: takes the first free slot whose build tree is already **current**, cuts a placeholder branch `worker/hook-<stamp>-<pid>` off main's tip and prints the slot path (the only thing stdout may carry). Slot choice is `slots.acquire`'s own atomic `.used` sentinel, so parallel hooks get distinct slots. It never re-seeds a stale slot (a hook has a timeout): with no current free slot it fails naming the fix, and gives the token back. remove a slot path is released - unless `slots.unlanded_reason` says it holds unlanded work, in which case it exits 1 so Claude Code keeps the directory and the orchestrator runs `slots.py collect --release`. A plain worktree under `.claude/worktrees` is removed with `git worktree remove` (which refuses a dirty one). Anything else is not ours: exit 0.
* Every call appends the raw input and the decision to `<main>/.pi/lanes/hook.log`. Measured input fields: `cwd`, `hook_event_name`, `name` (`agent-<agentId>`), `prompt_id`, `scratchpad_dir`, `session_id`, `transcript_path` (create) and `worktree_path` (remove).
* python tools/units/worktreehook.py arm N [--ttl SECONDS] | arm --slot N | disarm | status python tools/units/worktreehook.py create|remove # stdin: the hook JSON python tools/units/worktreehook.py --selftest

## Lib dependencies

lanes.

## Test contract

Tier: smoke only (prototype).
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_worktreehook.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

prototype; the hook payload cannot identify the lane

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **The payload cannot identify the lane** (measured in hook.log, 2026-09-29): `name` is `agent-<agentId>`, an id the harness generates at launch, so the orchestrator cannot know it when it arms; `prompt_id` is shared by every Agent call issued in one assistant message (three parallel launches logged one id); there is no description, prompt text, branch or requested-name field. Nothing the launcher controls reaches the hook, so tokens cannot be keyed by lane and parallel launches take tokens in claim order (a lane briefed for slot 4 got slot 3). **Rule: a slot-bound token is armed and launched ONE AT A TIME** - `arm` refuses a second bound token, and any mix of bound and unbound tokens; unbound tokens (any free slot) may be armed N at a time. After each launch returns (its create hook has consumed the token) arm the next.
