# Subagent profile tests

A profile is a prompt, so "it looks fine" is not evidence. Each profile is tested three ways, and this file is
the record. Re-run the checks after **any** profile edit - prompt drift is silent, and one draft already
mis-numbered the rule table (see T2 below).

| # | test | how | what counts as passing |
| --- | --- | --- | --- |
| T1 | discovery | `subagent({ action: "list" })` | the name shows as a **project** agent with its aliases (project agents are read from `.agents/agents/`; there is no install step) |
| T2 | recall | `python tools/agents/profileprobe.py <agent>...` then launch the printed call | the reader child, running nothing, states its job, its write limits **and the tell for being launched in MAIN**, the numbered rules **with the right numbers**, the pre-report verification **and that the `FAILED` count is the primary signal**, its report sections, and its role-specific rules - and names anything missing rather than inventing it |
| T3 | behaviour | give it a **real** task of that shape and judge the result against the gate | lands through `landbranch.sh` first try; MAIN untouched; the role-specific proof present (see below); no row lower than before |

T2 is the cheap test that has already paid: it caught the first `decompiler` draft mis-numbering section 6.5
(rule 1 described as the vtable rule, which is rule 10; rule 4 as "no duplicated records"; the field-offset rule
folded into rule 3). A profile that misdirects a worker is worse than no profile, so the probe is worth running
before trusting a profile with lanes.

## Role-specific T3 acceptance

* **decompiler** - a real unit: registration + bodies in one commit, `Object(NonMatching, ...)`, the unit lands
  through `landbranch.sh` **without a gate refusal**, MAIN's tree still clean, the budget converged (no
  single-row spelunking), and the report carries its `Verification` section.
* **merger** - a real refused-apply branch: the merged branch lands; the report carries the **zero-rows-moved**
  table (landed consumers of the merged header measured before/after); `git merge-base --is-ancestor main HEAD`
  is true (so `main` moving mid-task was re-merged); and the header was merged **by hand** - no `union.py` on a
  header.
* **fixer** - a real refused branch: exactly the refused items cleared, the diff no larger than the refusal, no
  row lower than before, improvements kept (matching policy rule 1), and the branch lands.

## Results

### T1 - discovery (2026-09-25)

**PASS.** `decompiler`, `merger`, `fixer` all listed as project agents with aliases; the harness surfaced their
`skills:` references as "proactive skill subagent suggestions". Discovery from `.agents/agents/` was proven by
moving the `~/.pi/agent/agents/` copy away and re-listing: the agent was still there, so the tracked file is the
live one and there is no install or sync step.

### T2 - recall (2026-09-25)

* `decompiler` - **FAIL, then fixed.** An earlier draft was probed and answered "rules 5 and 8 are not in my
  context", which on inspection was a symptom of a mis-numbered table. The table now matches `docs/plan.md` 6.5
  exactly and the profile carries the "are you sure you are in your worktree?" tell. Re-probed below.
* `merger`, `fixer` - first probe below.

### T3 - behaviour

Pending: assigned as real tasks arrive (the next free unit slot for `decompiler`, the next refused apply for
`merger`, the next refused branch for `fixer`). The `worker`-based baseline this is measured against: five
branches refused on rule 3/6/7 items, four hand-built merge lanes, one worker editing MAIN, and one lane
spending 130 turns / 1.36 Mt on a single unit.
