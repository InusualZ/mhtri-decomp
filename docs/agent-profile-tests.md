# Subagent profile tests

A profile is a prompt, so "it looks fine" is not evidence. Each profile is tested four ways, and this file is
the record. Re-run the checks after **any** profile edit - prompt drift is silent, and one draft already
mis-numbered the rule table (see T2 below).

| # | test | how | what counts as passing |
| --- | --- | --- | --- |
| T0 | freshness | `python tools/agents/sync_profiles.py --check` | exits 0: every profile's generated section 6.5 block matches `docs/plan.md` section 6.5. A rule change that forgot the profiles fails here (`install.sh` and `profileprobe.py` both run it first) |
| T1 | discovery | `subagent({ action: "list" })`, run from **MAIN** *and* from a fresh worktree | the name shows as a **project** agent with its aliases. Project agents are read from the **cwd's** `.claude/agents/`, so a lane in a worktree cut *before* a profile edit sees the old prompt - and the user-scope copy (`~/.claude/agents/`, consulted for every cwd) is what closes that gap. **Run `tools/agents/install.sh` after every profile edit.** T1's original "there is no install step" was proven only from MAIN and is wrong for worktrees |
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

### Profile edit - the seam rule (2026-09-26)

`decompiler` gained a pre-registration rule: **a registered range is not necessarily a TU** - settle the seam from
*data* before writing bodies, because a fragment can never match (the TU's pooled constants, jump tables and
`__FILE__` string belong to the larger unit), and never compensate for the missing pool with a local literal or a
re-declared symbol. The profile carries the evidence order (one-copy `__FILE__` string decisive, `.sdata2` pins
reliable vs `.data` pins candidate-only, `extab`/`extabindex` tiling), the `_<fnaddr>s_<file>_` correction (the
prefix is not the emitter), and the worked case: `menu_infomation.cpp`'s one-copy string is cited on both sides of
0x8030D338 and 0x80313E24, so three registered ranges were one TU with its seam at 0x80308FB4..0x8031A6C0.

Checks: **T1 PASS** (still listed as a project agent with its aliases). **T2** - two probes were launched
(`.pi/workflows/probe-decompiler.js`); both PASSed the pre-existing checklist (job, the MAIN tell, all ten 6.5
rules with the right numbers, the verification order, the FAILED-count primacy, the report sections, the rule-7
nuance, the best-variant policy, the three levers) but **neither stated the seam rule** - and that was a probe
defect, not a profile one:

* the probe's **task text** never asked about the seam (the checklist did, but the checklist only judges), so the
  question was added; a probe that cannot ask about a rule cannot test it.
* `emit(...)` sent `status: r.status`, which the runner does not populate - so every probe reported **workflow
  failed** even when the child completed. The child's answer was still saved, but a later reader sees a failed
  run and the recall evidence disappears behind it. `emit` now sends `key` and `out` only.

The edit also exposed the T1 row's stale claim: a profile edit has to be installed to user scope, or a lane
launched into a pre-edit worktree keeps the old prompt - which is exactly how this rule would have failed to
reach the worker that needed it.

**Re-probe: PASS (2026-09-26).** With the question actually asked, the child stated the seam rule in full - the
range may be a fragment, a fragment can never match, a failed check is reported as a seam re-draw rather than
written against, the one-copy `__FILE__` string is the decisive datum and an edge cited on both sides of it is
false, and `_<fnaddr>s_<file>_` gives the file name only, never the emitter. The run also reported **completed**
rather than failed, so the recall evidence is readable from the run itself - which is the point of the probe.

### Profile edit - the data sections are part of the registration (2026-09-26)

`decompiler` gained the rule that a unit's **data sections belong to the registration**, not to a later lane: the
data its own functions reference and own (private pool entries, jump tables, `__FILE__` strings, tables) is claimed
and emitted in the same change that writes the bodies, and an `extern` for data the unit owns is a defect. The old
framing was "**Encouraged** ... if the unit's code is at 100 %", so a unit at 88 % simply left the data; it is now
a requirement, and the **mandatory verification block** carries
`python tools/units/datagap.py --unit <stem>` - no `ours-extra` row, and no `target-extra` row for a section the
block claims.

Why the rule was needed, measured over the tree: **242 registered units with a source, only 23 claim any data
section, and 171 of the remaining 219 declare data `extern`s** - so most units are not their target object in
data and `matched_data` cannot rise. `enemy/fn_8018B3B8` (194 externs, no data claim) reports `ours-extra .data
1116 B (target 0 B), .rela.data 3348 B (target 0 B)`: our object invents a `.data` section the target object does
not have. `dataclaim.py` decides a *proposed* run (it serves the attribution queue), so `datagap.py` is the check a
worker runs.

T2 re-probe (2026-09-26): **the check is recalled, the rule's framing is not.** The child's mandatory-verification
list now includes `datagap.py --unit <stem>` with both directions (no `ours-extra`, no `target-extra` on a section
it claims) and it still states the seam rule in full - but it did **not** restate the rule itself, that the data is
claimed *in the registration* and that an `extern` for data the unit owns is a defect. That is the same probe
defect the seam rule had: question 4 elicits the *check* and question 3 elicits *numbered* rules, and this rule is
unnumbered. The probe now asks it directly (question 9).

**Re-probe: PASS (2026-09-26).** Asked directly, the child states the whole rule - the data is claimed *in the
registration commit, in the same change as the bodies* (private pool entries, jump tables, `__FILE__` strings,
tables); an `extern` for data its own functions own is a defect because the object is then not the target object and
`matched_data` stays at 0; and it gives both directions of the `datagap` row (`ours-extra` = it defined something
the original TU did not own; `target-extra` on a claimed section = it does not emit what the target has). The seam
rule is still stated in full alongside it, and the run reports completed.

### T1 - discovery (2026-09-25)

**PASS.** `decompiler`, `merger`, `fixer` all listed as project agents with aliases; the harness surfaced their
`skills:` references as "proactive skill subagent suggestions". Discovery from `.claude/agents/` was proven by
moving the `~/.claude/agents/` copy away and re-listing: the agent was still there, so the tracked file is the
live one and there is no install or sync step.

**Correction (2026-09-26).** That conclusion holds **from MAIN only**. The harness discovers project agents under
the **cwd's** `.agents/`, and every campaign lane runs in a worktree cut at its claim time - so a worktree created
before a profile edit sees the *old* prompt, silently. The user-scope copy (`~/.claude/agents/`, consulted for
every cwd) is what closes that gap, so `tools/agents/install.sh` must run after **every** profile edit. The
original test moved the user-scope copy away while cwd was MAIN, which cannot distinguish the two scopes.

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

    - user:    ~/.claude/agents            (consulted for every cwd)
    - user:    ~/.agents                     (present but empty)
    - project: <worktree>/.agents            (present but empty)
    - project: <worktree>/.pi/agents         (absent)

The harness discovers project agents under **`<cwd>/.agents`**, and **every campaign lane runs in a `git
worktree`** cut from the `main` of its claim time - so a worktree created before the profile commit sees none of
them, and the launch fails. Running `subagent list` from MAIN hides this completely: the profiles are there and
they work.

Fix: **`tools/agents/install.sh`** copies `.claude/agents/*.md` into `~/.claude/agents/` (user scope, consulted
for every cwd). `.claude/agents/` stays the reviewed source of truth. **Run it after every profile edit**, then
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

* **`merger` - PASS** (2026-09-25, as a rescue). The lane inherited an in-progress merge abandoned by a
  90-minute timeout (`MERGE_HEAD` set, `include/enemy/ENEMY_WORK.h` left `UU`, 17 paths staged, base 15 commits
  stale). It did exactly what the profile licenses: **aborted and redid** the merge instead of patching a
  half-understood partial resolution, then re-merged twice more as `main` advanced mid-task. The header's three
  hunks were hand-unioned - both sides splitting the same fillers, so each region was rewritten by hand in
  ascending offset order and **re-padded to the base byte total** - and the M1 completeness check confirmed all
  17 units `main` had registered since the base present exactly once in both `configure.py` and `splits.txt`
  (181 registrations vs 182 splits blocks, no duplicates, no registration without a block). For the proof it
  built a **full report of the merged tree against a full report of `main`'s tree in the same worktree** (not a
  proxy), **reproduced the two regressions the timed-out lane had found**, bisected them to a *variadic*
  `fn_8012EC60` spelling in the band header that changed the call convention of two landed units, and fixed them
  (`fn_8019DB9C` 93.75 -> 100.0000, `fn_801CA004` 86.67 -> 87.76923 - both restored to their main values).
  Final: **33 landed units, 1,729 rows compared, 0 lower, 0 higher.** It also investigated rather than inherited
  the stale `.ctors` line (dtk rewrites that fragment itself, so it is now committed). The gate landed it.

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

### Profile edit - naming (2026-09-26)

`decompiler` gained the rule that **naming is part of the unit's work, not a later pass**: real function names from
the runtime dump or the map (a rename is **two** edits, map + source, or objdiff pairs nothing), fields named from
their offset and use context, statics/globals named from what they hold - with `fn_XXXXXXXX`/`lbl_XXXXXXXX`/`unkNN`
left only where the evidence genuinely has no name, and said so in the unit header. `rule 7 deferred: <reason>` is a
last resort with a concrete reason, covers the `fn_` half only, and `grep -rn "rule 7 deferred" src/` is its
complete list.

Why: measured over `src/`, `fn_XXXXXXXX` appears **44600** times (definitions and cross-unit calls together),
`lbl_XXXXXXXX` **15333**, `unk*` 1283, and **218 of the 234 files** that carry any of them also carry a
`rule 7 deferred` declaration - the escape became the default. The T2 probe now asks for the rule (question 10) and
the checklist carries it.

### Profile edit - no placeholder is a resting place (2026-09-26, owner's ruling)

**There is no excuse for leaving `fn_`/`lbl_`/`unk`** - when the dump and the map give no real name, derive one
from context. This **reverses** the earlier convention ("a speculative name is a bug; leave a generated name in
place"), which was written down in four places; all four now say the same thing:

* `CLAUDE.md` -> Conventions -> "Commenting and naming" (the convention itself),
* `decompiler.md` (the profile's naming section and its `Commenting and naming` bullet),
* `.claude/skills/decompile-symbol/SKILL.md` (the "never a speculative name" clause),
* `.claude/skills/symbol-map-editing/SKILL.md` (the "`fn_xxxxxxxx` beats a speculative name" clause).

The rule: the real name when it is known; otherwise one **derived from context** - what the function does and who
calls it, what the data holds and who reads it, the field's offset and the value stored there - kept in the
surrounding symbols' scheme. When the context supports only a **guess, guess**, and mark it in the unit header so a
later pass can refine it. A generated `fn_`/`lbl_`/`unk` left in `src/` is a **defect**, and a unit being written
does not use `rule 7 deferred` - that escape exists for units registered before this rule, and
`grep -rn "rule 7 deferred" src/` is its complete list (218 of the 234 files carrying a placeholder today).

T2: question 11 asks it and the checklist carries it; verdict recorded when the probe returns.

**Re-probe: PASS (2026-09-26).** Asked directly, the child states both halves: names come "from evidence
(runtime dump real name -> map -> derived-from-context guess, **guess marked in the header**)", "`fn_`/`lbl_`/`unk`
is never the resting place", a rename's second edit is the source "in the same change, via `symedit.py`", and - the
part that closes the escape - "**`rule 7 deferred` may not be used on a unit I am writing**". Its policy answer
(item 7) independently repeats that a unit it writes may not use the escape at all. The seam (item 8) and data
(item 9) rules are still stated in full, and the run reports completed.

### Profile edit - the section 6.5 rules are generated, not hand-copied (2026-09-27)

**The drift, measured.** Both profiles still documented `rule 7 deferred: <reason>` as a legal escape. Commit
`4f3cb4ea1` deleted that key: rule 7 now fires on every `fn_XXXXXXXX` / `lbl_XXXXXXXX` / `loc_XXXXXXXX` / `unkNN`
identifier in `src/`, **whoever owns it**, with no exemption and no deferral - the land gate's `--diff` is the only
grandfather. Neither profile mentioned rule 11 (`void *`). The cause was duplication: the brief is generated and
cannot drift, but both profiles *inlined* the rule text by hand (`decompiler.md` across ~110 lines), and nothing
detected the copy going stale.

file:line before -> after:

* `decompiler.md:202` - "A file-wide `rule 7 deferred:` ... defers the `fn_` half only" -> **gone**: the rule list
  it sat in is now the generated block (the block's rule 7 line ends "There is no exemption and no deferral").
* `decompiler.md:279-285` - "`rule 7 deferred: <reason>` is legal **only for references to OTHER units' unrenamed
  symbols** ... It covers the `fn_` half only" -> `decompiler.md:259-264`: "the rule has **no exemption and no
  deferral** ... a `rule 7 deferred: <reason>` comment exempts nothing (that key no longer exists - do not re-add
  it) ... what you cannot fix you **report** ... the only grandfather is the land gate's `--diff`".
* `fixer.md:102` - "`rule 7 deferred: <reason>` is legal only for references to OTHER units' unrenamed `fn_`
  symbols" -> `fixer.md:100-106`: "There is **no deferral**: a `rule 7 deferred: <reason>` comment exempts nothing
  ... report it and leave the reference; the gate's `--diff` grandfathers an existing finding only".
* `merger.md:66` - "the gate refuses a written unit behind a `rule 7 deferred` escape" -> "the gate refuses a
  batch that leaves its own unit's symbols generated ..., a `rule 7 deferred` comment exempts nothing".
* `tools/agents/profileprobe.py` asked its probes what a `rule 7 deferred` comment "does and does not defer" and
  checked for "rule 7 deferral defers the fn_ half only" - both now state the current law.

**The mechanism.** `tools/agents/sync_profiles.py` injects a block between
`<!-- SECTION-6.5-RULES-BEGIN ... -->` / `<!-- SECTION-6.5-RULES-END -->` in `decompiler.md` and `fixer.md`: one
line per row of `docs/plan.md` section 6.5's rule table (number, title, meaning verbatim - so rule 2's
`include/unsplit/` and rule 7's "no exemption and no deferral" cannot be summarised away) plus the section's
enforcement sentences, selected from its enforcement paragraph so the audit table never leaks. Everything outside
the markers stays hand-written prose.

**Wiring.** `python tools/agents/sync_profiles.py --check` exits non-zero when a profile is stale;
`tools/agents/install.sh` and `tools/agents/profileprobe.py` both run it first and refuse, so a section-6.5 edit
that forgets the profiles fails the profile tests instead of silently installing a stale prompt. It is deliberately
**not** a `land.py` gate row: a worker landing an unrelated unit must not be refused over a stale profile.

Checks (2026-09-27): `--check` **0** (both profiles in sync, rules 1-10);
`python tools/agents/sync_profiles_selftest.py` - **38 checks, green** - a fixture table including a rule 11 with
its `/* untyped: <reason> */` marker (proves the pickup needs no code change), the marker refusal cases, a
deliberately stale profile reported stale and then in sync, and both real profiles against the real plan.

**Rule 11 was NOT in `docs/plan.md` section 6.5 when this ran** (the table read rules 1-10), so the generated
block carries rules 1-10. Because the generator parses the table, the rule-11 landing makes both profiles stale
and `--check`/`install.sh` fail until `python tools/agents/sync_profiles.py` is re-run - the continuation must
regenerate, and nothing else needs editing.

T1/T2/T3 are re-run after this edit; T2's probe questions were corrected with it.

### Profile edit - the 2026-09-28 findings, where they change what a lane must do (2026-09-28)

**What was annotated, and into which profile.** Only the findings that change a *lane's* behaviour - the
orchestrator-side facts stay in CLAUDE.md and the notes. Into all four: the launch/slot mechanism and the
profile mapping (`python tools/units/slots.py spawn --kind KIND [--slot N]`, with `tooling`/`docs`->`worker`),
the slot-reuse warning ("it built here before" is not evidence about this round), and "never run
`claims.py release`". Into `decompiler`: `unitscore.py` and `pairgap.py` plus the corrected metric reading (a
row with no `fuzzy_match_percent` key is 0 %, and the value is *matched / target instructions*, so a "0 %" row
with a big size gap is usually **one** matched instruction); `recompile.py --measure` works from git-bash (the
old `cmd /c` trap is fixed) and now refuses a stale object/split; the **claim -> rule 2** mechanism (a claim
moves ownership for every symbol in the range, so a band header's declaration becomes an owned-symbol finding
and the declarations move into the owner's header); and the `--diff` blind spot that makes that the lane's own
check. Into `fixer`: whole-row measurement via `unitscore.py`, the same staleness rule, `selftest.py --changed
main` (plain `--changed` selects nothing on a committed clean tree) and `stylelint.py --ref <branch>`. Into
`merger`: the two `mergebranch.py` false refusals it now resolves (a comment that names a
`src/<module>/fn_XXXXXXXX.c` **path** is not a symbol reference; a comment-only `src/**` difference resolves to
main's comment block plus the branch's code) with the genuine cases that must still block, and its residual
risks. Into `codereviewer`: `pairgap.py` and the metric reading, `stylelint.py --ref <branch>`, and the claim's
blind spot as a review target - the case the gate's grandfathering cannot see.

**Three defects the T2 probe found while verifying the edit - all fixed in the same batch.**

* **`merger.md` carried no generated section-6.5 block at all**, while citing rules 2/6/7/10 by number in its own
  prose (its M6 *is* rule 2; `vtableaudit.py --diff main` *is* rule 10). Measured, not assumed: the first merger
  probe answered *"there is no prose for project-wide rule 2/6/7/10/11 in my context - only their names"*.
  `sync_profiles.py`'s `PROFILES` tuple excluded it deliberately ("it keeps its own hand-written prose") -
  which is exactly the drift that tool exists to prevent, because a hand-typed merged header is where rules 3-5
  are broken. It is now generated like the others: the block is identical in all four, and the tool's own
  selftest went **45 -> 48 checks with no test edited** (the profile list is data-driven, so the count is the
  evidence that the new profile is really covered).
* **`codereviewer.md` dimension 5 carried an orphaned sentence fragment.** An earlier edit replaced the middle
  of a sentence and left *"reference units' is itself a finding: it is noise the next reader has to skip."*
  dangling under a parenthetical. The probe reported it as *"the two 'reference units' ... are named nowhere,
  so dimension 5 is unenforceable as written"* - a prompt defect that reads as garbage and had survived review.
  The sentence is now whole.
* **A contradiction in the note this very batch added**: it told a reviewer to write its report to
  `.pi/notes/<slug>.md`, while a reviewer has **no `write`/`edit` tool by design** - and the codereviewer probe
  caught it ("the scratch/notes write my instructions promise is impossible"). The profile now states the
  reason and names `bash` as the whole of the reviewer's write surface.

**Checks (2026-09-28).** T0: `--check` **0**, all four profiles in sync with `docs/plan.md` section 6.5
(rules 1-12); `python tools/agents/sync_profiles_selftest.py` **48 checks, green**. T1: `subagent list` shows
all four as project agents with the expected grants (`codereviewer` read-only: read/grep/find/ls/bash).
T2: run twice - four children, then two after the fixes. Every child recalled its job, the write limit **and
the repo-root tell**, rules 1-12 with the right numbers, its report sections, and the **new** annotations: the
decompiler child repeated `mhtri-dtk.slotN` (or `.ws-<claim>`), the fixer and merger children both repeated
`selftest.py --changed main`, the merger child quoted M7's new comment clause verbatim in meaning, and the
codereviewer listed `pairgap.py`. The second-round merger child answered the section-6.5 block as *"confident,
from the generated block"* - the exact gap round one had found. All four children correctly observed that they
were launched in MAIN and said they would stop and report; that is T2 exercising the tell, not a defect.

*(Superseded: the index has since left CLAUDE.md for `.claude/skills/mwcc-unit-matching/references/index.md`; what follows is the historical record.)*

**Residual gaps the probes named (recorded, not fixed here).** (1) CLAUDE.md's playbook index has a
**duplicate row number** - 48 appears twice (the variadic-definition row and the `extern "C"` row) and 52
follows 57 - so "playbook 48" was ambiguous in a finding's evidence line. **Fixed the same day**, with the
index generator: the out-of-place section is now **73**, `docs/matching.md`'s numbers are unique and
contiguous from 1 (asserted on the real plan by `sync_playbook_index_selftest.py`, which is what would have
caught it), the one live citation of that section (`.pi/notes/802a6624-fn-802a6624-d9e7.md`, the
float-varargs trap) was repointed, and CLAUDE.md's index is now generated rather than hand-copied. (2) The profiles
carry the playbook *index*, never `docs/matching.md`, so a lane can cite a row but not read it. (3) A
`rename`-class finding has no `backlog.py` kind, so a reviewer's rename request is picked up by nothing.

## Harness migration: pi -> Claude Code (2026-09-29)

Everything above was measured under the pi harness. Under Claude Code the profiles live in `.claude/agents/*.md`
(frontmatter: `name`, `description`, `tools`, `skills`; pi-only keys such as `aliases`, `inheritProjectContext`
and `timeoutMs` are gone), project instructions are `CLAUDE.md`, and a lane is a headless
`claude --agent <profile> -p` run with its cwd at the slot (`tools/units/lanecmd.py`). Read the T1/T2 rows above
with these substitutions:

| then | now |
| --- | --- |
| `subagent({ action: "list" })` / `subagent list` | `claude agents`, run from MAIN and from a fresh worktree |
| user scope `~/.pi/agent/agents/` | `~/.claude/agents/` (`tools/agents/install.sh`) |
| `contact_supervisor` (a blocking request) | end the turn with the request; the orchestrator answers with `claude --resume <session-id> -p "<ruling>"` |
| T2 probe as a `.pi/workflows/*.js` script | `python tools/agents/profileprobe.py <agent>` prints `claude --agent <agent> --tools "" -p < .pi/probes/probe-<agent>.md` |

The profiles have not been re-probed under Claude Code: run T1 and T2 before trusting them with a lane.
