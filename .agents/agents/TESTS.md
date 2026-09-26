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

* `decompiler` - **FAIL, then PASS.** An earlier draft answered "rules 5 and 8 are not in my context", which on
  inspection was a symptom of a mis-numbered table (rule 1 described as the vtable rule, which is rule 10; rule 4
  as "no duplicated records"; the field-offset rule folded into rule 3). The table now matches `docs/plan.md` 6.5
  exactly. Re-probe: all ten rules correct, the policy, the residual's home, three paying codegen levers, and that
  a rule-7 deferral covers the `fn_` half only. **PASS.**
* `merger` - first probe: honest and mostly right, but it reported that its merge rules "are NOT numbered". The
  rules are now M1-M8 (headers hand-union, re-pad, one member per offset, owner's declaration wins, and M8 "the
  claim release is the parent's, never `claims.py release`"). Re-probe: **PASS**, M1-M8 all held, plus the
  zero-rows proof, the struct-shrink hazard, and the FAILED-count primacy.
* `fixer` - first probe: it held rules 3-7 but reported "not given: the text of stylelint rules 1, 8+". A real
  gap - a fixer sent to clear a `goto` finding (and four units carry a pre-rule `goto` backlog) would not have
  known rule 8's conformant shapes. Rules 1, 8, 9, 10 are now in the profile. Re-probe: **PASS**, all ten rules
  with rule 8's three shapes and rule 9's placeholder nuance.

The probes also exposed a **contradiction in all three profiles**: each said "never write MAIN", while the
campaign requires the outbox/notes evidence *in* MAIN (`.pi/outbox/<slug>.json`, `.pi/notes/<slug>.md`). The rule
is now precise - MAIN's **tracked** files are read-only; those two gitignored evidence files are the job's
deliverable, and a later session reads them. `fixer` also gained the commit-message convention and "if you
believe the refusal is wrong, report it with evidence - never work around the gate".

### A defect the tests found before any lane used the profiles

T3's first launch failed outright: `Unknown agent: fixer`. The error's directory list *is* the finding:

    - user:    ~/.pi/agent/agents            (consulted for every cwd)
    - user:    ~/.agents                     (present but empty)
    - project: <worktree>/.agents            (present but empty)
    - project: <worktree>/.pi/agents         (absent)

The harness discovers project agents under **`<cwd>/.agents`**, and **every campaign lane runs in a `git
worktree`** cut from the `main` of its claim time - so a worktree created before the profile commit sees none of
them, and the launch fails. Running `subagent list` from MAIN hides this completely: the profiles are there and
they work.

Fix: **`tools/agents/install.sh`** copies `.agents/agents/*.md` into `~/.pi/agent/agents/` (user scope, consulted
for every cwd). `.agents/agents/` stays the reviewed source of truth. **Run it after every profile edit**, then
re-probe. Without this test, every profile-based lane would have failed to launch - i.e. the whole point of the
profiles.

### T3 - behaviour

* **`fixer` - PASS** (2026-09-25). The real refusal: `worker/801a9540-fn-801a9540-83fb` was refused for (a) a
  compile failure in `enemy/fn_8014A1BC.o` and (b) one section 6.5 violation (`+1 rule 9
  src/enemy/fn_801A9540.cpp`). The lane **reproduced both errors itself** instead of trusting the summary, fixed
  rule 9 at the *call site* (`__dl__FPv` is `operator delete`, whose C++ declaration was already in `sys_mem.h` -
  the spelling the sibling units use), and resolved the `(10563)` redeclaration by keeping MAIN's authoritative
  declaration, explaining why deleting the band copy instead would have widened the diff past the refusal. It held
  the diff to exactly its 13 paths, showed all 54 rows unchanged **row by row** against the branch's own outbox,
  and proved neutrality at the **object level** (`.text`/`.rela*`/`.symtab`/`extab` byte-identical; only
  `.strtab`'s MWCC `@NNN` label numbering differed). It used the profile's primary signal correctly (the `FAILED`
  count first). **The gate then landed it** (`2c4777b7c`, closed 4465 -> 4519). No prompt defect reported.
* **`decompiler`** - running on a real unit with a deliberately **short** prompt: the launch names the brief and
  the ack and the setup, and nothing else, because the profile is supposed to carry the rules.
* **`merger`** - pending: the next refused apply (the parked `worker/801b0010-...` branch is the natural case).

### The `decompiler` T3 "self-verification defect" was TOOLING, not the profile

Two `decompiler` lanes reported *"no new section 6.5 violation"* and were then refused by the gate for real
findings in their brand-new units. The cause, root-caused by a `fixer` lane that reproduced it three ways
(the exact verdict string, a temporary index, and a scratch repo): **`stylelint.py --diff <ref>` builds its
comparison set from `git diff --name-status`, and `git diff` cannot see an untracked file** - so a lane that runs
the lint before staging its new unit is checked against the wrong file set entirely, and a new unit's violations
are all additions, i.e. exactly what it cannot see. The profile's own ordering (lint, then commit) made that
certain for every new unit.

Fixed on both sides, because the tool is the real bug:

* `tools/units/stylelint.py` now includes untracked files under `src/` and `include/unsplit/` in the comparison
  set (selftest still 162 checks). Proved with a probe: an untracked `src/zz_lintprobe.c` containing a `goto` was
  invisible before and is now reported as `+1 rule 8`.
* the profile says so, so a future reader of a verdict knows what to check.

**Consequence for the test record: those two refusals are not evidence against the profile.** The lanes did what
they were told; the instruction was blind. What the refusals *did* show is that the gate catches it anyway - which
is why the tool fix matters for throughput, not for safety.

### Field comparison: `worker` vs `decompiler` on the same kind of task

The profiles are not only tested in isolation - the campaign kept running `worker` lanes beside `decompiler` ones,
which gives a direct comparison on real units.

* **`enemy/fn_801D80EC` (`worker`, 2026-09-25)** - all 49 rows, 18 byte-identical, unit 94.97 %, landed clean. Its
  own report names the difference: *"the enemy owner headers don't carry the call-site signatures this band needs,
  so the file declares them itself exactly as `enemy/fn_801D428C.cpp` does; four `shared-file` requests are in the
  outbox."* That is the rule-2 pattern the campaign has paid for all session: it passes the lint only because
  those symbols have no registered owner *yet*, and it becomes a clash the moment one lands - which is exactly how
  `801A9540` (clash in `fn_8014A1BC.o`) and `801BD6C0` (clash in `fn_80105314.cpp`) were both refused. The
  `decompiler` profile says instead: a declaration belongs in its owner's header, and an unowned symbol goes in
  `include/unsplit/<module>.h`.
* **What to watch in the `decompiler` lanes** (`801E0ADC`, `801EC9F8`, `801F3294`, `801F9CD4`, `802029B4`): whether
  they place declarations that way unprompted, and whether they land without a gate refusal. That outcome decides
  whether the rule-2 refusals we have been absorbing go away - and it is the honest measure of whether the
  profiles were worth building.

The `worker`-based baseline this is measured against: five branches refused on rule 3/6/7 items, four hand-built
merge lanes, one worker editing MAIN, and one lane spending 130 turns / 1.36 Mt on a single unit.
