---
name: commit-review-gate
description: Show what is about to be committed and refuse until a human approves that exact staged content, through tools/agents/commitgate.py and a .git/hooks/pre-commit shim. Use before every commit in this repository - it enforces AGENTS.md rule 6 (nothing committed without explicit approval), blocks build/original/scratch paths (rule 2) and a stamped AGENTS.md (rule 8), and flags generated churn, configure.py edits and unmeasured source changes for review.
license: MIT
compatibility: git repository with a .git/hooks directory; Python 3, no network, state in the gitignored .pi/handoff/commit-approval.json.
metadata:
  author: mhtri-dtk
  tool: tools/agents/commitgate.py
---

# Review before every commit

Rule 6 says nothing is committed without explicit approval - and the review matters most exactly where
skipping it is tempting: a 4.5 MB `symbols.txt` churn, a `configure.py` flag change, a `build/` file
caught by `git add -f`. Habit is not enforcement, so the gate is a `pre-commit` hook plus a state file.
The hook prints the review sheet and **exits non-zero**, so the commit does not happen until the human
has approved the staged tree.

## Commands

| goal | command |
| --- | --- |
| see what would be committed | `python tools/agents/commitgate.py review [--diff-lines 150]` |
| what the hook runs | `python tools/agents/commitgate.py check` |
| record the human's approval | `python tools/agents/commitgate.py approve -m "<who said what>"` |
| state of staged tree / approval / hook | `python tools/agents/commitgate.py status` |
| install or remove the hook | `python tools/agents/commitgate.py install \| uninstall` |

The flow is: stage the change → `review` (or just `git commit`) → the sheet appears and the commit is
refused → the human reads it → `approve -m "<what they said>"` → commit again. `review` never blocks;
only `check` (and therefore the hook) does.

**Stage one concern at a time and commit the whole staged tree.** A pathspec commit
(`git commit -- <paths>`) is refused: git runs the hook against a restricted temporary index, so the
sheet being reviewed is not the staged tree the approval covers. Two commits from one stage means two
stage/approve/commit rounds - which is the point.

## What an approval binds to

The approval stores a hash of the **staged tree** (`git diff --cached --binary`), so staging anything
else afterwards invalidates it - you cannot approve a diff and then commit a different one. It expires
after 120 minutes, lives in the gitignored `.pi/handoff/commit-approval.json`, and `status` says whether
it is valid, stale or absent. `approve` refuses when nothing is staged, or when the stage contains a
hard block.

## Hard blocks (not approvable)

* a staged path under `build/`, `orig/`, `.pi/`, `.lavish/`, `.vscode/`, `__pycache__/`, or any
  `*.dol`/`*.rel`/`*.elf`/`*.o`/`*.map`/`*.a`/`*.pyc` - rule 2, build output and original files never
  get committed;
* a staged `AGENTS.md` that still contains a `LOCAL-ONLY` marker line - rule 8 (`pull` it first with
  `tools/agents/localonly.py`, see the `agents-md-local-only` skill).

## Warnings (review, then decide)

* `config/RMHE08/symbols.txt` / `splits.txt`: the changed-line count, and a loud warning past
  `--max-generated-lines` (200) because generated churn belongs in its own commit;
* `configure.py`: a flag or tool-version change needs the instruction/size evidence in a comment next
  to it (rule 3);
* anything under `config/**` or `src/`: `config/**` is also the other window's area, and a source change
  should come with a measurement (`objdiff-verify`).

The sheet also reprints the two checks that cannot be automated - `ninja build/RMHE08/ok` must pass for
anything touching the linked DOL, and no compiler flags may be smuggled in.

## Bypass

`COMMITGATE=off git commit ...` prints a warning and lets the commit through. That is for a human in a
hurry, and it is the only way past the gate: `approve` cannot override a hard block, and a reviewer who
is not in the loop can always see in `status` that no approval was recorded.

## Note for whoever works in the `General` window

The hook applies to every commit in this checkout, whoever makes it. The block message names the exact
command to approve, so nothing is lost - but a commit will not land until a human has run `approve`.
