# The production pipeline: how a batch is run

**This file is the authority for *how* a batch is run. `docs/plan.md` remains the authority for *what* is
worked and in what order.** When the two disagree, this file owns the mechanics and `plan.md` owns the queue.

The pipeline is the process by which one claim becomes one landed unit: a decompile pass, an independent
review pass, a bounded fix pass, a gate, and a landing. It exists because **a decompile pass cannot audit its
own work** — the author of a unit writes the residual list, the flag evidence and the prose in the unit header,
and the same author is the worst reader of those claims, because they encode the author's beliefs. A separate
read-only pass, with no stake in the branch, and its findings handed back as the next pass's brief, is what
catches the claim the code does not support.

The measured basis for everything here is `.pi/notes/pipeline-experiment-report.md` (the full experiment
report, including run 2 in §8b) and `.pi/notes/production-trial.md` (the round log with every filed item
F31–F43). Numbers are quoted from those artifacts; where something is unmeasured this file says so.

---

## 1. The three-phase loop, and when to use it

### 1.1 The loop

```
claim (opt-in --pipeline) -> PHASE 1 decompile -> PHASE 2 review -> PHASE 3 fix -> [one re-review] -> land
```

* **Phase 0 — claim.** One claim, one slot, one branch: `claims.py claim <unit> --pipeline --slot <n>`. The
  claim holds the slot for the **whole loop**, because phase 2 must read the branch where it lives — a slot is
  a *directory*, never a branch (see §1.3). `--pipeline` is opt-in: the loop cannot gate the rest of the fleet,
  and everything that does not opt in behaves exactly as it did before.
* **Phase 1 — decompile.** An ordinary `decompiler` lane works the unit as normal, with one changed
  expectation: **"done" means "reviewable"**, not "finished". The normal bar is unchanged — best-scoring
  variant, no regression, residuals in the header, `ninja build/RMHE08/ok` green — but the unit header's
  residual list and the outbox are now *input to phase 2*, so they are written to be read. The lane commits on
  its branch.
* **Phase 2 — review.** A `codereviewer` lane is launched **in the same slot, read-only by construction** (its
  profile's tool list carries no write/edit), pointed at the *branch*. It **verifies the branch's claims
  independently rather than repeating them**, and reports in its own ranked shape (defect | debt | taste;
  where; rule; evidence verbatim; fix). It is explicitly asked for **where the profile was wrong or useless**,
  which is how the reviewing profile itself improves. That report is a first-class output, not a side effect.
* **Phase 3 — fix.** A `decompiler` lane on the **same branch**, with the review's findings **as the brief**.
  No re-derivation, and an explicit instruction to say which findings it deliberately left and why.
* **The bounded re-review.** The plan allows review → fix → **one** re-review, then land. Skipping the
  re-review is legitimate for documentation-only findings, but it is **recorded, not implied**.
* **Supervision is the orchestrator's job, not the lanes'.** Each hand-off is driven: phase 1 completes → the
  review is launched in that slot on that branch → its findings are read → defects go back as a fix lane →
  re-review (or a recorded decision to skip) → land one unit per commit, gate on. Every phase's cost and every
  finding are recorded, because the experiment's metric is *what the review caught that decompile missed*.

### 1.2 The new rule this session's own failure produced: scope phase 3 by what changes bytes

**Scope phase 3 by "what changes bytes" first, then the record-only findings.** A six-defect fix list plus two
debts was handed to one lane as a single brief and it hit the 4-hour wall with **nothing committed** — the
relocation-level defect was the one that mattered, and it was buried in a list of documentation edits. A fix
brief must therefore be ordered: **byte-moving findings first, record-only findings after**, so that if the
lane runs out of wall it has committed the change that affects the artifact.

### 1.3 When to use the loop — and when not to

The loop is **opt-in per claim** (`--pipeline`). It is worth its cost when the flip is live and the claim is
high-stakes: a first flip of a unit, a unit whose work is doubtful (new bodies, a deliberate rule deviation, a
modelled layout), or a record that downstream lanes will read and trust. It is **not** worth it for a
documentation-only change, a mechanical rename sweep, or a tooling fix whose own selftest is the check — for
those the ordinary lane plus its gate is sufficient, and the loop's supervision cost is pure overhead.

The honest state of the evidence is in §1.4. **Do not quote the loop as "it catches wrong code" on the strength
of run 1** — run 1 caught wrong *records*, and run 2 (a doubtful unit) is what shows it catching real defects.

### 1.4 The measured verdict

The experiment has two complete runs, and they answer different questions.

**Run 1 — `Network/initNetworkSessionStable` (a correct flip).** The review found **five defects; all five were
in the record, zero in the code**, and the code was already byte-identical. Its value was **the honesty of the
record**, not the correctness of the artifact. That is a real, repeatable value — the code was verified
independently, from a fresh compile, before anyone trusted it — but it is a different value from "catches wrong
code". The run's own numbers:

* Phase 1 (`1b2b64038`): 99.92424 → **100.00000 %**; the manual `operator new` + null check became a real
  `new` expression, and `extab` became byte-identical including the `__dl__FPv` relocation at +0x14. It also
  found a **second defect the earlier review had not seen** — byte +0x79 (`lz r12,0(r31)` against retail's
  `r12,0(r3)`), which is **not source-shaped** (six spellings give the identical byte) and yielded to the
  optimizer pass: `#pragma peephole off` at file scope, with no new flag deviation.
* Phase 2 (`1223dc58`): verified the flip **from a fresh compile of the committed source** — three sections
  byte-identical, the relocation offsets *inside* `extab` checked, the claim sizes, `undefined_reference_problems`
  re-run by hand, and the DOL hash — then reproduced the header's own numbers.
* Phase 3 (`1d867c03f`): closed all five findings, each **re-measured rather than quoted**, and cleared the
  naming debt the review handed over as data (five vtable slot targets; four string-confirmed, one derived from
  use alone and recorded as such). Zero byte movement.
* The landing (`2e9a85e4c`): gate green, `ok` OK at the DOL hash, **34th flip on main**; ledger matched
  5,089 → 5,090, `.text` 580,752 → **581,016**. The flip count only moved on *landing* — the branch held it
  earlier, and quoting it before the landing would have been wrong.

**Run 2 — the DWCi band (`worker/dwci-band-9050`, doubtful work).** Two units, 19 new bodies,
`DWCi_NatNeg` 1.19 → 19.58 %, `DWCi_Np_CPUCopyFast` 2.60 → 6.20, no regression. The review returned **six
defects**, one of them **invisible to every score the project has**:

* **A relocation-level defect.** `DWCi_natNegTickIdleSockets` calls `DWCi_natNegPollReplies` where the target
  calls `DWCi_natNegPollRepliesOnce` — MWCC auto-inlined the 4-byte thunk, so the linked bytes differ while the
  instruction count agrees. It was found by a **per-symbol reloc-name multiset comparison** (six lines of
  Python). The header had attributed the row to register colouring, so a fixer following it would have hunted
  colouring forever. The fix is playbook 61 (`dont_inline` scoped to **one** function).
* **False evidence with the conclusion still right.** The deliberate rule-1 deviation (one `.sdata` symbol, two
  declarations, because a sizedness is a codegen input) is justified — but its written proof was wrong: the
  review measured the negotiator's own object as **ten** SDA21 relocations plus two ADDR16_HA/LO, not SDA21
  throughout. It also probed the mechanism and measured how rare the target's mixing is (**4 of 2970** target
  objects). So the deviation stands and the residual for the next pass is now named.
* Plus stale residual numbers from before the second commit; a naming paragraph that missed five renamed rows;
  a dead `fn_` row name and a "no placeholder survives" claim its own file falsifies; and three evidence
  sentences with addresses that point into other units or at instructions that do not exist.
* The "checked and clean" half is as valuable: **no regression independently (2296 units, 0 regressions, exactly
  2 advances)**, the four claimed byte-identical bodies verified as **byte *and* reloc identical**, and every
  modelled layout offset spot-checked against the target disassembly.

**The verdict, in the form it should be quoted:** *run 1 showed the loop polices the record; run 2 shows it
finds real defects.* Neither run alone is the claim.

**Later reviews (2026-09-28): 6 reviews, 6 material catches, none visible to any gate row.** A false 100 % (a
`bl` to the wrong symbol *name* - objdiff scores a reloc-name mismatch as equal); a gate hole (the rule-11 row
is COUNT-based, so an added `void*` hid behind nine removals); a struct that scored 100 % while `0xC` wrong for
17 rows (an overflowed pad); a regression in a function the branch never touched (`NWC24iSetRtcCounter`
44.17 -> 40.50, fixed to 92.24 by `dont_inline` scoped to one function); `stylelint --ref`'s vacuous clean.
Review judges the branch's own diff: `stylelint --diff main` **from MAIN**, per-function objdiff, and the
**relocation** view. A recorded residual is a landed win, not a finding; a review that finds nothing says so.

### 1.5 The honest limits (unmeasured, and not to be implied away)

* **Cost per phase is unmeasured.** Phase 1 and 3 are ordinary decompiler lanes; phase 2's report was 87 lines
  and its hand-rolled harness cost it ~15 minutes (its own filing). Wall-clock and token cost per phase are
  **not** recorded — the trial's tables are where they should accumulate, and until then no cost claim is
  supportable beyond "phase 2 is one review lane".
* **Whether a branch-only review suffices is untested.** Both runs gave the reviewer the decompile lane's own
  summary *and* the branch. A cheaper variant — hand the reviewer the branch alone — has never been run.
* **One re-review has never been exercised.** Run 1 skipped it legitimately (all findings were
  documentation-only), which means the loop's bound has not actually been tested either. The bound is a design
  decision, not yet a measured one.

---

## 2. The slot model, and its enforcement (E1–E3)

### 2.1 A slot holds a directory, never a branch

Six reusable slots sit beside the repo at stable sibling paths (`../mhtri-dtk.slot1` … `../mhtri-dtk.slot6`),
each holding a warm `build/RMHE08`, `orig/`, the toolchain and its initialised `tools/m2c` submodule across
rounds. A stable path is the point: the depfile's absolute header paths stay valid across a reset, and the
per-seed rewrite is a one-time cost per slot instead of a per-claim one. A slot is never removed, so its `tools/m2c` submodule is initialised once and inherited by every round. A **throwaway** worktree (`--no-slots`) has no toolchain or target object of its own, so `recompile.py` resolves `MAIN` through `git worktree list --porcelain` (first entry) and takes the compiler, binutils and the *target* object from `MAIN` — while the source it compiles and the object it writes stay in the invocation's own tree.

**A slot holds a DIRECTORY, never a branch.** Every claim cuts a *fresh* branch off `main`'s current tip, so a
slot's identity is its working directory and its warm build tree — the branch that happens to be checked out in
it is disposable. **A free slot *is* the concurrency cap.** There is no separate lane limit: the pool size is
the cap, and `queue.py next` / `acquire` refuses while all six slots are taken (naming each holder) instead of
constructing a seventh environment. `slots.py status` shows the pool; `slots.py verify` proves every kept tree
current.

**A slot is never trusted on faith.** `acquire` validates the kept `build/RMHE08` against MAIN's current
map/DOL with the same staleness guard the seeder uses (`config.json` vs `symbols.txt`/`splits.txt`/`main.dol`,
plus a byte comparison of `report.json`) and **re-seeds if it cannot be proven current** — a stale build tree is
a lie, and the most expensive failure this campaign has hit. `slots.py`'s `verify`/`status` are the readings;
the reset is **fail-closed**.

### 2.2 `.used`: two readings of one fact

A slot is occupied by one sentinel file, `.used`, read from two sources that **must agree**:

* `acquire` creates `.used` in the slot's worktree root **atomically** (`O_CREAT | O_EXCL`), so two racing
  acquires cannot both take one slot; `release` removes it; `status` reports `used`/`free` from it.
* The second source is the **worktree itself**: a slot whose worktree still has a branch checked out is **in
  use**, whatever any file says. The lock record is a convenience; the worktree is the truth.

On 2026-09-27 the two disagreed — `status` called slot 2 `free` while its worktree held
`worker/rule10-fix-14f8` — and that disagreement *was* the bug, so both readings are kept. A `.used` marker on a
*detached* worktree with no live claim is a crash remnant, named reclaimable rather than a permanent wedge.
`acquire` **falls through**: an occupied slot is skipped for the next genuinely free one, while an explicitly
named `--slot N` still refuses when it holds an **unlanded branch** — the check is on the *branch*, not merely
the sentinel, so a crashed lane's unlanded work is never silently overwritten.

### 2.3 The enforcement (E1–E3)

The one-lane-per-slot rule was recorded after a real error (F33: a claim was released on slot 5 while a lane was
still working in it, detaching its HEAD mid-run) and **recorded is not enforced**, so it is a tool change with
acceptance tests, not a note:

* **E1 — the owner label.** `.used` records its **owner label**, and a lane **stops** if the slot it is in is
  owned by a different claim *or by none*. A lane launched into an unowned or foreign slot detects it in its
  first turn and reports rather than working.
* **E2 — `release` fails closed.** `release` refuses when: the slot tree is **dirty**; a **running** subagent
  run has its `cwd` resolving into that slot; or the **HEAD holds commits no branch reaches**. It prints the
  reason and names the run. `--force` is the deliberate override, and the reason it is being overridden is
  printed.
* **E3 — the acceptance tests.** A fixture with a running run record whose `cwd` is the slot must **refuse** and
  name the run; the same record marked finished must proceed; a dirty tree must refuse; `--force` must override
  both; and a slot whose `.used` owner label differs from the claim must be refused at launch.

**Honest limit to state, because it is structural:** *a lane cannot be detected by a lock.* A lane is a
sequence of short-lived processes, so no lockfile can see one. The **run registry is the only live-lane
signal**, and it is the harness that owns it — the guard *reads* it, it does not pretend to see a process. This
is why the launch side is structural too: `queue.py next` and `claims.py claim` print the paste-ready spawn
line with **the claim's own worktree as `cwd`**, and a spawn line whose `cwd` resolves to MAIN is refused
outright rather than discouraged. Every brief opens with a **"your tree"** block naming the `cwd` and the
one-line self-check (`git rev-parse --show-toplevel` must equal the tree; if it does not, stop and report).

### 2.4 The lane profiles, and when each is used

| profile | used when |
| --- | --- |
| `surveyor` | a claim is surveyed **first**: what the unit needs and owns, and the ready-to-paste claim extension |
| `decompiler` | unit work: the body pass after a survey, and the phase-1 and phase-3 legs of the pipeline |
| `fixer` | a **refused gate**, or a measured regression — it fixes the named failure, not the neighbourhood |
| `merger` | a refused apply — but prefer `mergebranch.py` (§5) |
| `codereviewer` | the phase-2 review; **read-only by construction** (its tool list has no write/edit) |
| `worker` | anything else, and tooling |
| `scout` / `planner` / `reviewer` | read-only recon, planning, and review outside the pipeline |

A unit claim runs **four legs**: `surveyor` -> `decompiler` -> `codereviewer` -> `decompiler`. The surveyor goes
first because four lanes in one day each lost a round to a claim that was too small - an unowned pool, an unowned
table, a range that was two translation units, a seam re-cut done by hand - and each of those is cheaper to answer
before the body pass than during it. `queue.py next` emits the `surveyor` leg for a fresh claim; the decompiler and
the review are launched once the survey lands.

The rule-text blocks in these profiles are **machine-generated** from `docs/plan.md` §6.5 and the matching
skill: run `python tools/agents/sync_profiles.py --check` and never hand-edit between the markers.

* **The profile is not cosmetic.** Reconstruction/recon work runs as `decompiler`, never `worker`: three
  networking recon lanes launched as `worker` never saw the §6.5 naming/type policy and all three were
  refused for ~130 names. `slots.py spawn --kind KIND` decides the profile (`unit`->`decompiler`,
  `fix`->`fixer`, `merge`->`merger`, `tooling`/`docs`->`worker`, `review`->`codereviewer`); an unknown kind is
  refused.
* **Policy never lives only in hand-written duplication.** The profiles once taught the retired `rule 7
  deferred:` key and no rule 11 because nothing detected divergence from §6.5. `sync_profiles.py --check`
  (wired into `install.sh` and `profileprobe.py`, deliberately **not** the land gate) now fails on it, so a
  rule-change batch must also run `python tools/agents/sync_profiles.py`. A profile edit is live only after
  `tools/agents/install.sh`.

### 2.5 Slot and worktree hygiene

* **A lane working in MAIN costs a landing.** A lane launched with `cwd = MAIN` cloned into the repo root and
  the next landing was refused (*"paths outside the batch appeared during the build"*). Cure is structural: the
  spawn line's `cwd` is the claim's own slot, MAIN is refused, and `land` pre-flights MAIN for lane scratch
  (`.tmp-*`, `.ws-*`, `upstream/`) before the build. A stray untracked file in MAIN (`??`) blocks every landing:
  check `git status --short`, delete it once the slot copy is proven identical. Scratch belongs in `.pi/tmp/`.
* **A fresh worktree is missing every non-tracked payload.** `tools/m2c` (submodule, empty after `worktree
  add`: `git submodule update --init tools/m2c`), `build/binutils` (0 files; `ninja tools`), `orig/`. Slots and
  the seeder handle this; a hand-built worktree does not. `orig/` is **copied** below `ORIG_JUNCTION_MIN_BYTES`
  (64 MB; the repo's is 6.6 MB), because a junction's teardown emptied MAIN's originals twice.
* **Teardown.** `git worktree remove` cannot remove a tree holding a submodule; the manual path is `rm -rf
  <wt> && git worktree prune && git branch -D <branch>`, only after `LANDED` (R3) or proof the branch is
  applied, and only in MAIN. Never `claims.py release` from inside a worktree - it wiped one. A lane launched
  outside the registry leaves its branch behind: `claims.py release --branch <name>` (`--force` only once the
  gate said "no batch path is stageable"). Plain `git worktree remove` on a junctioned tree follows the
  junction into MAIN.
* **Check the brief slug against the worktree slug** when pasting several `queue.py next` launches: one was
  launched with the `80394038` task in the `803967f0` tree. `briefs/<slug>.md` in the task must equal the cwd's
  slug.
* **Rescue and resolve refs are a safety net.** `claims.release` parks `refs/rescue/<slug>` first; the audit
  runs at teardown (`rescue.py audit`: `redundant` is pruned, `landed-with-drift` is reported and never
  pruned, `unlanded`/`unknown` are named and kept; fail-open). A re-claim of a unit overwrites its slug's ref.
  Judge a `land/resolve-*` helper by its **content**, not by whether its unit is registered (the `--merged`
  test misleads). `refs/codex/*` is an external tool's; leave it alone. The measured result of the full audit:
  0 of 193 refs held unlanded work.
* **Classify a held branch for free:** `git merge-tree --write-tree main <b>` against `git rev-parse
  main^{tree}` - equal means fully applied. Never `git diff main <b>` (main moved; it shows a huge deletion).
  Read the held branch's own `build/RMHE08/report.json` before transplanting a body. `.pi/notes/
  held-branch-inventory.md` holds the per-branch verdicts.

---

## 3. Operating rules, each with the failure that produced it

These are not aphorisms. Every one of them cost a session something concrete, and the failure is recorded with
it so the rule can be argued with rather than obeyed. This is the section that pays for itself.

**R1 — A tool whose input lives in MAIN must be exercised in a worktree.**
The failure: `flipcheck.py` byte-checked through a **fixed** path, `MAIN/.pi/_flipcheck.bin`. A worktree does not
carry `MAIN/.pi`, so `objcopy` failed on **both** sides, `raw_section` returned `None`, and the comparison was
**skipped without a word** — 51 of 281 units reported READY against a true 33: **18 false READYs**, including
`NetworkPat` with 577 of 720 `.text` bytes mislaid. The lane that wrote the tool never saw it because it verified
in MAIN, where `.pi` exists; the defect only existed where lanes actually run. (F31.)

**R2 — A lane cannot be signalled by a lock, and a release must not run under a live lane.**
The failure: a claim on slot 5 was released while a lane was still working in that slot, and the release detached
its HEAD mid-run (the lane recovered and anchored its branch honestly). The slot layer does not know about lanes
launched outside the run registry, because a lane is a sequence of short-lived processes. The guard is E2 (§2.3):
`release` fails closed on a running run whose `cwd` resolves into the slot. (F33.)

**R3 — Guard the teardown on success only.**
The failure: teardown ran regardless of whether the landing had landed, so a slot was returned (and a branch
deleted) for work that had not committed. Guard it on the outcome — `if echo "$out" | grep -q '^LANDED'` — and do
nothing otherwise, so a failed landing leaves its slot and branch in place.

**R4 — Parallel lanes go in separate single-child launches.**
The failure: the workflow runner has **silently dropped children** when several were fanned out in one launch.
A missing child leaves no error and no outbox — the unit simply does not advance and the round looks idle. One
child per launch is the shape that survives.

**R5 — One writer per file; landings are serial on one `main`.**
The failure: two lanes touching the same shared file (or a worker committing to `main` itself) makes the batch
base invalid and the merge ambiguous. A worker edits only its unit's source and commits only on its branch; the
orchestrator owns the shared files and lands one unit per commit. A conflict is a **protocol violation**, not a
merge to resolve blindly. `main` does not move while a batch is open; if HEAD has moved before the land step the
batch aborts.

**R6 — `NonMatching` units are not linked, so `ok` stays green while an object fails to compile.**
The failure: a green `ninja build/RMHE08/ok` was read as "the batch builds", but `ok` links only the units that
are `Matching`; a `NonMatching` unit whose object does not compile is simply never reached by the link. Check the
**compile** result too — grep the build output for `^FAILED` — before believing `ok` means the batch is sound.

**R7 — A docs diff maps to no selftest, so run the doc-adjacent checks by hand.**
The failure: `selftest.py --changed` maps only `tools/**` diffs to selftests, so a docs/profiles batch reports
**GREEN with 0 tests** while the invariants it can actually break — `sync_profiles.py --check`,
`sync_reference.py --check` — are never run. So a documentation change runs those two by hand until
`--changed` maps docs to them. (F37 — the mapping is a known gap, not yet closed.)

**R8 — Long drafts go through the file tool, not a shell heredoc.**
The failure: a shell heredoc **truncates silently** when a second heredoc follows (a long playbook draft was lost
that way; the orchestrator hit the same class the same night). The file tool does not truncate; a heredoc gives
no warning, and a truncated draft that commits is worse than no draft. (F38.)

**R9 — The orchestrator never reads a whole lane report into context when the outbox and the gate carry the
evidence.**
The failure: reading full lane reports into the orchestrator's context is how it runs out of room to drive the
fleet, and it duplicates evidence that is already machine-readable. The outbox (`.pi/outbox/<slug>.json`) and the
gate are the evidence; the report is for a human and for a later session.

**R10 — A lane tree can be a commit behind `main`, and that silently invalidates a "known clean" answer.**
The failure: the section-gap lane was cut before the `initNetworkSessionStable` flip landed, so its **negative
control** emitted an 8-byte `extab` against the target's 24 and was not clean at all — the lane read "clean"
from a stale tree and nearly trusted it. Diagnose with `git rev-list --count HEAD..main`; if it is non-zero,
**fast-forward and rebuild** before trusting any known-answer test. A worktree that cannot be proven current is
not a source of truth. (This is the same law as §2.1's `acquire` staleness guard, and it rides the measurement
discipline in §7.)

**R11 — A verification pass must run where the lanes run.**
This is the umbrella the others sit under. R1 is its sharpest instance; R7 and R10 are the same shape one level
up (a check that never executes in the environment where it matters reports success). Before trusting any
"clean"/"green"/"READY", ask **where it printed that** and whether the tool's inputs existed there.

**R12 — A refused landing pollutes both trees.**
The failure: a REFUSED landing left a modified tracked file *and* MAIN's build tree holding its own split objects
(`config.json` predates the map), so the next batch's row reported *"no unit's split target object moved"* for a
unit it never touched. Cure: `git status` after every refusal; `ninja` in MAIN restores the tree (the `acquire`
currency check catches it); when a row blames a neighbour, `rm -rf build/RMHE08 && python configure.py &&
ninja`, then `ninja build/RMHE08/report.json` + `land.py record-base`.

**R13 — A gate hands out a work order; it must be complete.**
The failure: `rule7_defer_growth` listed `defs[:3]` and a batch that renamed exactly the 3 shown was refused
again (it defined 8; one unit had 33). Two lanes lost a unit of work each. A refusal message lists every
offender and leads with the count. Same class: a batch **must name the symbols its own units define** and
register at a **named path** (a `fn_XXXXXXXX.c` path is refused) - only references to other units' symbols
were ever deferrable, and now nothing is (§11).

**R14 — An edit anchored on a heading eats the heading.**
The failure: an `Edit` whose anchor was a `**HEADING.**` line replaced it, so the next paragraph lost its title.
Put the heading verbatim at the end of the replacement, then check the seam with `sed`. Also: anchors written
with `\n` silently fail on a CRLF file - assert the match count (§12).

---

## 4. The gate and the landing

**`tools/units/land.py`'s rows are the law.** The gate is not a suggestion and not a checklist to skim: it
refuses a batch, names the row that refused, and the landing either passes every row or does not happen.

* **`--units` takes units *or* batch paths.** A batch is named by its units, or by the paths it touches; both
  are valid, and the tool resolves them through the map rather than by guessing a directory layout.
* **A `BOOKKEEPING` refusal is not a gate failure.** When the named row is a bookkeeping row (a ledger or
  report row, not a code row), the response is to *re-check that row* and then use the documented
  `--no-outbox` if the row is genuinely bookkeeping-only. Do not treat it as a code failure, and do not let it
  train you to bypass the gate.
* **A real gate failure is never bypassed.** A refusal on a code row is a stop: fix the unit, re-measure, and
  land again. There is no "land it anyway".
* **One unit per commit.** The landing is one commit per unit, so a later bisect or revert names a unit, not a
  batch. Cherry-pick is the mechanism; a worker that left several commits is landed as its range rather than
  only its tip.
* **A whole branch lands in one command.** `python tools/units/land.py land --branch worker/<slug>` runs record-base, applies the branch with the registration union, gates it, commits and releases - the landing path when the branch is whole. Reach for `--units` only when the batch's paths are not the branch's registration diff.
* **The bootstrap exception.** A change **to the gate itself** cannot be gated by the gate it is changing, so
  it is committed **directly, path-limited** (the gate's own files and nothing else). This is the only landing
  that skips the gate, and its path-limit is what keeps the exception from becoming a habit.

**Gate rows that cost a round, and the cure for each**

* **`--units` must name what the row names.** The *"a neighbour's split target object moved"* row fires on a
  unit the batch may not have touched; name exactly the unit it names in `--units` (a rename does **not**
  re-range a neighbour - `verifyunit.target_object_fingerprint` drops names by design; the row's test is not the
  fingerprint alone, so do not guess at renames). A deliberate neighbour re-range needs `--units <neighbour>`
  **and** `--allow-regression <neighbour>` when a function legitimately leaves it (`--allow-regression` is
  refused as stale when not needed).
* **A path is not a unit.** `is_batch_path` says "path" on a file extension *or* something in the tree at that
  exact path; a unit's bare name is never a file. A tools-only/docs-only/comment-only batch names its paths (or,
  for a sweep with no registration diff, every unit) in `--units`; a tooling file left staged after a fixer batch
  needs its own commit (the gate warns *"the index holds N path outside this batch"*).
* **Expect a BOOKKEEPING bounce from a lane outbox** and reach for `--no-outbox` when the real rows are green:
  the outbox row validates one unit per branch (a 2+ unit branch can never satisfy it), attributes renames
  strictly, wants `flags_probed[].verdict` in `reject`/`adopt`/`inconclusive`, and rejects an unknown
  `config_requests` kind (`seam`). A header named in `--units` once demanded residual/flags for a non-unit path
  (`unit_units` fix pending; check: `land.py` outbox row).
* **Lane-side pre-check for rule 7:** `land.rule7_defer_growth(<worktree>, <base>)` and
  `land.band_ownership_warnings(...)` must both return `[]`; two lanes were refused after a full unit for not
  running them. Rule 2/7 sweeps also need `handoff.py --check` at 0 errors.
* **A committed scratch file refuses the batch** (`.tmp_dg.json`): remove it on the branch, never `--no-outbox`.
* **`--already-applied` and the commit-sweep guard.** A tracked file dirty at `record-base` rides the next unit
  commit unless the batch names it (`CLAUDE.md`/`docs/plan.md` did): land the orchestrator-side tools/docs batch
  first. `land` refuses off `main`. A commit to main mid-flight does not endanger a running lane (`record-base`
  re-reads main).
* **Landing a whole branch:** `land.py land --branch worker/<x>` (a `-named` branch too) derives `--units`; the
  orchestrator's whole landing is that one command - never read the lane report (R9).
* **` M <file>` with an empty `git diff` is EOL churn.** It refuses with *"main's tree is not clean"*;
  `git checkout -- <file>` settles it, but never when the edit is a real one. `CLAUDE.md` is an ordinary tracked
  file, so a real edit to it lands with `land.py land --units CLAUDE.md` like any other path.

Related, and enforced by a different tool: the compile must actually happen. `NonMatching` units are not
linked, so a green `ok` can coexist with an object that fails to compile — check `^FAILED` (R6). And the
landing's verification against the DOL hash is the arbiter that a `Matching` flip really links.

---

## 5. The merge procedure

**`tools/units/mergebranch.py resolve` brings `main` into a stale branch.** It is proven on three lanes and is
now the tool — the manual merge is superseded. Reach for it whenever a branch was cut before `main` moved and
must be brought current before landing.

* **`git apply --3way` is the wrong primitive.** It merges text; this repository's conflicts are almost never
  text-local. A `symbols.txt` row, a `splits.txt` range and a `configure.py` entry are *facts with an owner*,
  and a text merge can produce a file that is syntactically fine and semantically doubled.
* **The map is resolved by row replacement, never unioned.** A `symbols.txt` row is owned by exactly one unit;
  a union of two versions of the map can leave the same address claimed twice or a rename half-applied. The
  resolver replaces the row with the branch's intended owner, so ownership stays single-valued.
* **A `src/**` block is never unioned.** Source is one writer per file (R5). Unioning a `src/` block is how a
  marker-free file — one that happens not to carry conflict markers — is silently **skipped** and left at
  `main`'s version, taking the branch's work with it.
* **The rule-2 address sweep.** Because a declaration belongs with the unit that owns the symbol (§6.5 of the
  plan), a merge that moves a symbol's ownership must sweep the *references* by **address**, not by name — the
  asm dump is stale and the name may not have moved with the address.

**The two failure modes it exists to prevent** are the same defect at two scales: *a marker-free file left from
`main` being skipped* (the branch's version vanishes because it never conflicted), and — the expensive instance
— **157 header lines and a registration lost that way**. A merge that reports success has not necessarily
carried the branch; the resolver's job is to make the carried set explicit rather than inferred from git's exit
code.

**The by-hand classes, for when `mergebranch.py` blocks** (proven on three lanes; ~1.5 h and two content losses
is what the tool replaced):

* `TIP=$(git rev-parse HEAD); MB=$(git merge-base main $TIP); git merge main`, then `mkdir -p .pi/tmp`.
* `symbols.txt`: take main's file and re-apply the branch's rename pairs by exact row replacement; a rename
  collision resolves as both sides' new names and neither side's old ones, which keeps the line count equal to
  main's (a cheap invariant). `splits.txt` is a union of blocks - but a block's **position** decides dtk's link
  order (`lb_quest_board` anywhere but between its neighbours died with *Cyclic dependency encountered*).
* Other files: `git merge-file -p --diff3 <ours=git show main:f> <base=git show $MB:f> <theirs=git show $TIP:f>`.
  `git apply --3way` writes markers and **exits 0**, and refuses once the tree differs from the index.
* `include/unsplit/*.h`: the rule-2 sweep - drop every declaration whose address is inside a registered `.text`
  range and **move the ones a unit still uses into the owner's header** (rule 2 has no deferral); a `void*` in the
  new home needs `/* untyped: <reason> */`. `include/unsplit/menu.h` is the hot spot: two `menu` lanes in one wave
  collide, so prefer disjoint subsystems.
* **After merging, re-check every name the branch references**: a rename is two edits and a merge resolves only
  the map half (`fn_803754F4` -> `em020_aim_target_ck` left a source call to a dead name; the gate caught it only
  at link). For each `fn_[0-9A-F]{8}` in the branch's changed files, if it is no longer a map name use the
  address's current one.
* Finish with `python configure.py && ninja -k 0` (grep `^FAILED`), `rm -f build/RMHE08/ok && ninja
  build/RMHE08/ok`, `stylelint.py --diff <merge-base>`.
* **Verify a merge by the measurement, not by a green build**: a `NonMatching` unit's object is not linked, so a
  clean-looking merge (an `isSubState_*` vs `queryPatOpeningFlag*` naming collision; `configure.py` cflags lost)
  sat green until someone disbelieved a score. Never skip a conflicted file for having no markers.

---

## 6. The feedback loop

The loop is what makes the profiles and the tools improve from use, and it is a **round-level obligation**, not
a courtesy.

* **The registers.** Two: `tools/units/backlog.py` (fed by `.pi/outbox/*.json` and `.pi/notes/*.md`) is the
  work register, and `docs/tooling-requests.md` is the tooling register. A finding goes in one of them, not
  into a lane report that nothing reads.
* **A novel row needs two distinct filers.** One lane's annoyance stays a *note*; the same row filed
  independently by a **second, distinct** lane is what promotes it to due work. This is why "the first filer's
  report is enough" is wrong: it is the second filer that proves the finding is not one lane's local
  misunderstanding.
* **Feedback is assigned, never merely stored.** A row left **unassigned** at the end of a round is a **failure
  of that round**. Recording a finding and not routing it is the same as not filing it — the register's value is
  that every row has a home and a decision.
* **A tool that misled a lane is the most valuable report in the channel.** A tool that merely failed is
  annoying; a tool that *succeeded while lying* (R1's 18 false READYs) is dangerous, because it was trusted.
  That class of report is the one to promote first.
* **Every tool section in a profile carries its own channel.** The `mwcc-debugger-gaps.md` /
  `mwlink-debugger-gaps.md` pattern: the profile's section on a tool names the channel where that tool's own
  gaps are filed, and the loop is *written into the profile* rather than left to the lane to remember.

---

## 7. Measurement discipline

The campaign's scores are its evidence, so the metric is treated as a single artifact with one definition.

* **One implementation of the official metric.** `measure.py` and `recompile.py --measure` are **the same
  primitive** now — a duplicate scorer was deleted so there is exactly one implementation of the official
  metric (the drift that once printed 0.36 points low on RSO/runtime is closed by construction). A number whose
  producer is ambiguous is not evidence.
* **Resolution is invocation-first, and it prints the path it used, with its kind.** `recompile.py` prints the
  absolute path it resolved and labels it `[worktree-split]` / `[registered]` / `[auto-fallback]`, and refuses
  when the target is missing rather than inventing one. A resolution that silently follows the *invocation's*
  tree is the whole point — the class of bug where a tool reads `MAIN`'s target while claiming to measure the
  branch (F40) or reads `MAIN`'s `symbols.txt` on a branch that renamed its own symbols (F43) is the same
  failure: resolution that did not follow the invocation.
* **Cite sections, not object sizes.** Object size is **not a reproducible number**: the `.symtab` FILE symbol
  carries the source *name*, so the same source is **1232 / 1216 / 1224 B** under three different names. A
  cited size can differ from the truth by the length of a filename, so a claim about a run quotes the
  **section** (`extab` 24 B, `.text` 264 B), never the object. (This nearly cost a false finding.)
* **A stale build tree is a lie.** `slots.py verify` (and `acquire`'s staleness guard, §2.1) prove a kept tree
  current before it is trusted; a wrong measurement from a stale tree is indistinguishable from a real
  regression. This is also why R10 — a tree a commit behind `main` — belongs here: a known-answer control run
  against a stale tree answers a question about the wrong source.
* **`report.json` is an order-only target.** It is a product of the build, not an input to it; treating it as
  an input is how a report and the objects it describes drift apart. After a source edit `ninja
  build/RMHE08/report.json` says "no work to do" and you read the **previous** build's scores (a lane read "all
  new functions score 0 %" for three iterations): delete it or touch a source first, then diff the whole-project
  report - it is the cheapest neighbour-regression check (playbook 60).
* **Resolve MAIN by `git rev-parse --git-common-dir`, never by the first `git worktree list` entry** (registration
  order): `symdiff.py` run from a slot once reported 0.91743 for a symbol the slot's own report had at 100.0 and it
  looked like a merge destroyed 67 functions. `recompile.py --measure` prints tree, target object + mtime and warns
  when the target is MAIN's under a worktree cwd; it resolves against MAIN's map - pass `--main .`.
* **Refuse an object older than its source.** Hand-built scorers silently measured a STALE object when a compile
  failed (two false "improvements"); `recompile.py` asserts the mtime advanced. Three lanes re-derived the same
  scorer in one session - use `measure.py` / `recompile.py --measure`, do not write a fourth.
* **A rename-only or `bl`-name mismatch scores equal.** objdiff scores a reloc-name mismatch as a match, so a
  false 100 % can hide a `bl` to the wrong symbol: compare a per-symbol **relocation-name multiset** (an open
  tool request - a target-vs-ours reloc summary; three lanes asked). A struct-size lint is likewise open (`sizeof`
  vs the strides the target's disassembly uses).
* **`report.json` carries NO evidence for an `Object(Matching, ...)` unit.** `metadata.complete: true` pins the
  unit at 100 (an intact pair and a one-byte-corrupted pair both read `fuzzy_match_percent: 100.0`; playbook 75),
  so the ledger's matched bytes do not move when a claim lands. The backstop is `verifyunit.py`'s **address-aware
  byte comparison** (a pad-named row resolves by address; a corrupted byte still refuses). Also: a fully matched
  row omits `fuzzy_match_percent` while a *function* entry with none is 0 % - arithmetic-check the unit percent.
* **Re-measure headline numbers, do not copy them.** `python tools/units/ledger.py` gives covered/closed/matched
  and the `.text` denominator (the DOL file size is never the denominator; dtk's `complete_code_percent` is not
  quoted). `datagap --unit` labels a `.text` overrun a size residual and prints only `ours-extra` by default -
  "0 unit(s) listed" means "no extra", not "no object".

---

## 8. Flip discipline

A flip is the highest-stakes claim in this repository — a wrong one links the wrong bytes into `main.dol` — so
it is verified, not asserted.

* **`flipcheck.py` READY is necessary, not sufficient.** READY is the *entry* condition: it says the unit is a
  candidate. It does not say the unit is correct, and it must not be quoted as if it did.
* **A flip is bytes *and* relocations.** Verify the relocation **offsets inside `extab`**, not its size: a
  record can have the **right size with its relocations at the wrong offsets** — `NetworkWiiMediator`'s `extab`
  is the correct `0x1a8` with three `dl` relocations at `+0x2c`/`+0x34`/`+0x54` instead of
  `+0x14`/`+0xac`/`+0xb4`. A size compare cannot see that, and neither can any score (F41).
* **A flipped unit's object is linked, so the DOL hash is the arbiter.** The final question is whether the
  linked `main.dol` is byte-identical; a section compare inside the object is evidence *toward* that, not a
  substitute for it.
* **The record's `Matching` claim is verified from a fresh compile**, never from the author's object. The
  author's object proves what the author compiled; a fresh compile of the committed source proves what the
  repository holds. (Run 1's phase 2 did exactly this, and it is what makes the flip trustworthy.)

* **A `.text` match is not a claim about the object.** `constructNetworkWiiMediator` read 100.00 % and was not
  flip-ready: `operator new(n)` + a null check lowers to the same 16 instructions but MWCC emits no cleanup, so
  `extab` was 8 B against the target's 24 B (playbook 62). Run `flipcheck.py` before believing a 100 % unit;
  it now also refuses the `@etb_`/`@eti_` class. In a lib built `-Cpp_exceptions off`, a file-scoped `#pragma
  exceptions on` is **required**, not stylistic; the deciding evidence is the call site's relocation.
* **The `@etb_`/`@eti_` class is fixed by a build step**, not source: `tools/elf/objextab.py` (chained after
  `objalign` in all four MWCC rules) renames the extab/extabindex entries to `splits.txt start + st_value` and
  sets the symtab binding global (playbook 59). Promoted symbols keep their index in the local range (`sh_info`
  untouched). `ef/ef_emform` is **not** in that class (row 46).
* **Row 46: the answer is the symbol, not the section.** The linker builds ctor/dtor entries from the
  `__*_reference` symbols; renaming an MWCC `.ctors$10` leaves the output byte-identical and renaming the symbols
  fails the link (catalogue id 205). A section-name compare cannot see this. Use `mwlink_debugger.py trace
  [--link]` (§9.7); the **link step is `Wii/1.0`** (`build.ninja` global `mw_version`), per-object compiles `Wii/1.3`.
* **`.init` and link-generated data.** dtk classifies the TRK vector image as link PADDING (`pad_NN_ADDR_init`); a
  `type:function` symbol over the exact claimed extent retires the pad symbol (playbook 74; a sized
  `type:object` made `dol split` fail with an overlap error). MWCC pads every object in `.init` to 8 bytes.
  The 164 B `_eti_init_info`/`_rom_copy_info`/`_bss_init_info`/`_ctors$99`/`_dtors$99` tables are unclaimable by
  design (`is_linker_generated_object()`) and inert; `__start.c`'s `extern`s stay correct.
* **`flipcheck` READY was measured wrong 2 of 5 times** (`g3d_resfile` `undefined: '@eti_...'`, `ef_emform`
  hash) - the DOL is the only proof. A flip lane that hits a hash move quotes the `ninja diff` / `undefined:`
  diagnostic verbatim rather than guessing. The flip count moves on **landing**, not on the branch.

One further, still-open limit worth stating plainly: `flipcheck.py`'s relocation half **does not run for an
already-`Matching` unit**, so a reviewer of a landed flip must re-run that check by hand (a filed tool gap,
F31's sibling class).

---

## 9. The tool roster

One line per tool: what it is, and **use it when …**. Paths are relative to `MAIN`. Where a tool is still
being built, the line says so; a tool named here without a mechanism is named because the campaign uses it, not
because it is finished.

### 9.1 The lane, the claim and the environment

| tool | use it when … |
| --- | --- |
| `tools/units/claims.py` | **any lane starts**: claim a unit (`--pipeline` opts into the three-phase loop, §1) — the claim cuts a fresh branch off `main`'s tip and holds a slot for the whole loop; `release` returns the slot and **fails closed** (E2, §2.3). |
| `tools/units/slots.py` | **you need a lane directory**: `init` builds the six-slot pool, `status`/`verify` read the pool and prove a kept tree current — *a stale build tree is a lie* (§7), so reach for `verify` the moment a measurement is suspect; the reset is fail-closed. |
| `tools/units/queue.py` | **you are choosing the next item**: `next` picks a unit, takes a free slot and prints the paste-ready spawn line with the claim's own worktree as `cwd`; it refuses while all six slots are taken, and refuses a `cwd` that resolves to MAIN. |
| `tools/units/brief.py` | **you are launching a lane**: it generates `tools/units/briefs/<unit>.md`, the one file the worker reads — unit, inventory, residuals, decided items, the task, and the rules copied verbatim from the plan. *If the brief does not say it, it is not a rule.* |

### 9.2 The gate, the merge and the landing

| tool | use it when … |
| --- | --- |
| `tools/units/land.py` | **you are about to put a batch on `main`**: its rows are the law (§4). `--units` takes units *or* batch paths; `--no-outbox` is for a bookkeeping row, never for a code refusal; its `--diff` grandfathers existing rule findings and refuses *added* ones. |
| `tools/units/mergebranch.py` | **a branch was cut before `main` moved**: `resolve` brings `main` in by **row replacement** (never a union, never a `src/**` union), with the rule-2 **address** sweep — it supersedes the manual merge procedure (§5). |
| `tools/units/flipcheck.py` | **you are deciding whether a unit may be flipped**: READY is **necessary, not sufficient**, and its relocation half does not run for an already-`Matching` unit (§8). |
| `tools/elf/objalign.py` | **a unit's claimed start is not 8-aligned**: clamp the compiled section's `sh_addralign` so mwld stops rounding the section up and shifting every later section (a build-step ELF fix). |
| `tools/elf/objextab.py` | **a `Matching` unit's exception tables need the map's names**: give the MWCC object the `@etb_…`/`@eti_…` names `dtk dol split` synthesises, so the link resolves once the unit is the only definition (a build-step ELF fix). |

### 9.3 The measurement

| tool | use it when … |
| --- | --- |
| `tools/units/measure.py` | **you are searching**: score a whole unit in one compile and one `objdiff report`, printed as a per-symbol table. |
| `tools/units/recompile.py --measure` | **you are proving one symbol**: one symbol per run, usable from any worktree; resolution is invocation-first and prints the path it used with its kind (`[worktree-split]` / `[registered]` / `[auto-fallback]`). Together with `measure.py` it is the **one** implementation of the official metric (§7). |
| `tools/units/sectiongap.py` | **a flip is doubtful**: it prints each differing section **with its relocations**, not just a size — the `extab`/`extabindex` gaps `datagap.py` skips, and the right-size/wrong-offset record no size check can see (F39/F41; being built). |
| `tools/units/datagap.py` | **a unit reads 100 % but may not be the target**: it compares the *data* sections' sizes, so you see the extra `.sdata2`/string pool objdiff does not count. |

### 9.4 The record, the map, the language

| tool | use it when … |
| --- | --- |
| `tools/units/symbolpreflight.py` | **before any source is written**: answer who owns an address and what a registration would touch — offline and bounded. |
| `tools/symbols/symedit.py` | **you need a map row**: query and surgically edit `symbols.txt` (~65 700 lines) without loading it into context; the map row and the source change together. |
| `tools/symbols/dumpmap.py` | **you want a rename candidate**: resolve `symbols.txt` names against the runtime dump's Dolphin map. |
| `tools/splits/tudiscover.py` | **you need a TU boundary**: propose the translation-unit seam around an address, offline, from the data-section referrer runs. |
| `tools/units/callees.py` | **you are about to write bodies**: name a unit's callees — every generated symbol its bodies reference, its owner, the call shape — before rule 7 bites. |
| `tools/units/callers.py` | **you need "who calls / who reads this"**: the whole-DOL caller index, **address-keyed** because the `asm/` dump is stale (rule 12's evidence tool; the session called it the single most useful recon tool). |
| `tools/units/dataclaim.py --unit U` | **rule 12 bit you (an `extern` of unowned data)**: every data symbol U references but does not own, its census sharers, and the exact `splits.txt` claim / named data-only unit to paste. Read-only; it never writes `splits.txt`. |
| `tools/units/unwindcut.py <unit> <cut>` | **you are re-cutting a seam** (moving a unit's right edge to a function boundary): the `extabindex`/`extab` partition at the cut with the sum check, the paste-ready lines for both halves, and the `.ctors`/`.dtors` words the cut obliges you to drop. Read-only; it refuses a cut that is not a function boundary. |
| `tools/units/langcheck.py` | **a unit's language is in question**: decide C vs C++ from evidence (a mangled definition, a `.cpp` `__FILE__` string), never from convenience. |
| `tools/units/recordmerge.py` | **two lanes each hold a view of the same record header**: fold them into one definition with the checks the hand passes lacked. |

#### 9.4.1 Claiming data: rule 12's three shapes, and the two recipes lanes kept re-deriving

Rule 12 (`docs/plan.md` §6.5) refuses an `extern` of data **no registered `splits.txt` range covers**,
and unlike rule 7 there is no rename remedy — the only remedies are ownership changes. There are exactly
three shapes, and `dataclaim.py --unit U` decides between them mechanically (the address, the section, the
map symbol and its **extent**, the registered owner, who else reads it, and the claim to paste):

**The inverse — un-claiming a `.text` range obliges dropping the `.ctors`/`.dtors` words that point into it.**
Releasing a `.text` range (a seam re-cut) obliges dropping every `.ctors`/`.dtors` word **whose target
function leaves with the cut**, because dtk derives that word's claim from the unit that owns the
constructor; a surviving word makes `dtk dol split` refuse `Mismatched splits for .ctors 4:0x8056F374
(menu/menu_message.cpp) and function 3:0x802ABC4C (auto_fn_802ABC4C_text)` rather than mis-split, and
dropping the claim lets dtk re-derive it from the tail. `tools/units/unwindcut.py <unit> <cut>` names the
words and prints both halves' lines (`.pi/notes/menu-seam-recut-4622.md`).

1. **Ordinary.** The range is free: claim it into the unit being worked. The claim must cover the
   **whole** map symbol extent (`symbols.txt`'s `size:`), and a claim's `end:` must be **4-aligned** — a
   partial `.sdata`/`.sdata2` claim does not link.
2. **Span.** The unit already owns a run of that section: claim the **contiguous span, gap included**.
3. **Named data-only unit.** A pool/table several units read: register a **named** unit that owns it.

The first is `docs/plan.md` rule 12 itself. The other two are the recipes worth spelling out, each with
the failure it prevents.

**Recipe 1 — the span claim (a second run of one section).** A unit that claims several runs of one
section must own the bytes *between* them. Claiming a band's jump tables but not the unnamed blobs
between them splits the range against dtk's own `auto_<n>_<addr>_data` unit; that unit then lands *inside*
the claiming unit's address range and `dtk dol split` stops with
`Cyclic dependency encountered while resolving link order: <unit> -> auto_<n>_<addr>_data` (playbook 53,
`fn_80429B94`). Either claim every run of that section, or drop a run entirely; a *leading* or *trailing*
unclaimed run is harmless (one direction, no cycle) — only a run **between** two of the unit's own does
this. The fix is one line: merge the two ranges into the span that covers both, gap included.
`dataclaim.py --unit U` prints that line whenever `U` already owns a run below the symbol.

**Recipe 2 — the named data-only unit (a shared pool/table).** When a pool several units read has no
owner, do **not** claim it into one reader: a partial `.sdata2`/`.sdata` claim cannot be linked, and if
the bytes are not one unit's own the claim would also be wrong. Register a **named data-only unit**: a
`splits.txt` range covering the whole pool, a `symbols.txt` name, and a source file that **defines
nothing**. That is legitimate, not a cheat: the unit is `NonMatching`, so the original bytes stay in the
binary and the **DOL is untouched**, while the range gains an owner — strictly better than the anonymous
`auto_XX_data` unit dtk would otherwise create. The consumers then `#include` the owner's header and
declare into it (rule 2's home). One owner per pool.

```
# splits.txt
Pl/sdata2_pool.cpp:
	.sdata2     start:0x80799E00 end:0x8079A000

# symbols.txt
Pl/sdata2_pool = .sdata2:0x80799E00; // type:object size:0x200

# src/Pl/sdata2_pool.cpp   - NonMatching, defines nothing
```

The failure this prevents is the inverse of recipe 1's: claiming a shared pool (or a `.sdata2` sub-range)
into one reader either fails to link or re-attributes bytes another unit also emits. `dataclaim.py --unit U`
names the sharers from `callers.py`'s census and prints the block above whenever a referenced pool is read
by a unit other than `U`.

### 9.5 The rules and the audits

| tool | use it when … |
| --- | --- |
| `tools/units/stylelint.py` | **you changed `src/` or `include/`**: rules 1–12 with `file:line`; `--diff <ref>` is the gate's add-only comparison, `--budget` a debt read. |
| `tools/units/vtableaudit.py` | **a unit owns a code-pointer run**: find a vtable it owns but does not emit and a hand-written `+0x00` table store — rule 10 made mechanical, with `--diff` at the gate. |
| `tools/units/declclash.py` | **a cross-unit lane hits `(10197) illegal function overloading`**: list the function names declared more than once with *different text* in one include closure, before any source is edited. |

### 9.6 The registers and the suite

| tool | use it when … |
| --- | --- |
| `tools/units/backlog.py` | **you file or work the backlog**: one ranked register built from `.pi/outbox/*.json` and `.pi/notes/*.md`, with the credit ledger (one resolved item per new proposal claim). |
| `tools/units/tooling.py` | **you file a tooling gap**: turn the reports' "Tooling and environment" entries into the ranked `docs/tooling-requests.md`, clustered by demand. |
| `tools/selftest.py` | **before claiming green**: `--changed` maps `tools/**` diffs to selftests — a **docs** diff maps to **none**, so run `sync_profiles.py --check` and `sync_reference.py --check` by hand until the mapping lands (R7). |

### 9.7 The compiler and the linker

| tool | use it when … |
| --- | --- |
| `tools/mwcc-debugger/` + `locate/verify_pcode.py` | **a body differs and the source shapes are exhausted**: drive `mwcceppc.exe` under gdb, dump the per-pass PCode, and classify which optimizer pass is responsible. |
| `tools/mwlink_debugger.py` | **the DOL hash moved after a flip**: interrogate `mwldeppc.exe` — trace / diagnose / align / phases. **In production** per its six-point gate: health-checked against `main.MAP`/`main.elf`, `main.elf` never written, 260/260 sampled inputs, with its own gaps note. |

**Debugger detail worth knowing.** `mwcc-debugger`: run `verify_pcode.py` on the last dump first (`MATCH` or a
real `FAIL`), then on the dump before the divergence for a `PASS-DELTA` that **names the pass**
(`after-peephole` for the `lbzu` fusion), and write that pass into the unit header's residual; a missing gdb is
one `fetch_gdb.py` line. `mwlink_debugger.py`: `trace` (kept/dropped, section addresses with bytes read back from
the ELF, symbol resolution, decoded relocations, ctor/dtor rank), `trace --link` (the build's own link line into
scratch, byte-identical to `main.elf`), `phases [--prove]`, `messages` (the linker's message catalogue lives in PE
RT_STRING resources - why `strings` finds nothing), `verify`. **No `mwldeppc.exe` carries a CodeView blob** (31
checked), so the compiler's lever is absent for the linker. Gaps channels: `.pi/notes/mwcc-debugger-gaps.md`,
`.pi/notes/mwlink-debugger-gaps.md` (§6).

### 9.8 Audit and recovery

| tool | use it when … |
| --- | --- |
| `tools/units/rescue.py audit` | **you want to know if any `refs/rescue/*` holds unlanded work**: classifies `redundant` / `landed-with-drift` / `unlanded` / `unknown` (~40 s); `--prune` deletes only `redundant`. |
| `tools/units/verifyunit.py` | **a `Matching` claim needs a byte-level backstop**: address-aware comparison, since `report.json` carries no evidence for `Matching` units (§7). |
| `tools/units/backlog.py triage` | **the register may hold ghosts**: classifies open items resolved / stale / open from evidence and parks only what it can prove (§11). |
| `tools/units/ledger.py` | **you need a headline number**: covered / closed / matched, re-measured, never quoted from a doc. |

---

## 10. The coordinator protocol: brief, outbox, handoff

Moved here from `docs/plan.md` §5, which is now a pointer. Workers are separate processes that inherit nothing
from the orchestrator's context, so **everything that used to be a prompt or a habit becomes a file with a
format.**

### 10.1 The brief is the worker's only input

`tools/units/brief.py <unit>` writes `tools/units/briefs/<unit>.md`, containing:

1. the **unit**: path, lib, `mw_version`, the real cflags, object and target paths, the `.text` range;
2. the **inventory**: every symbol it owns, with size and current measured %;
3. the **residuals**: the unit header's existing table (what differs and why) — so a re-brief never re-derives
   settled work;
4. the **decided items**: flags landed for this lib, seams already settled, data ranges deliberately not
   claimed — so a worker does not re-litigate them;
5. the **task**: the functions to write, in address order;
6. the **rules**: the hard rules, the measurement loop, the evidence order, the flag policy, and what it may
   not touch.

Part 6 is copied **verbatim** from the plan's rules (§8) plus the skill that owns the step, so there is one
canonical source and no drift. **If the brief does not say it, it is not a rule.** Spawning a worker is then
one line: *read this brief, do it, write your report.*

### 10.2 The worker's output is data, not prose

| # | artefact | consumed by |
| --- | --- | --- |
| 1 | the source it owns, compiled and measured, committed on its branch (**one commit**) | the build, the merge |
| 2 | `MAIN/.pi/outbox/<slug>.json` (slug = branch minus `worker/`) — per-symbol %, unit %, residual, **config requests** (range/seam/rename/flag/shared-file with evidence), **flag probes** (numbers + verdict), blockers, and the command it measured with | the orchestrator and `land.py`, which can refuse a batch from it alone |
| 3 | `MAIN/.pi/notes/<slug>.md` — the full evidence trail | a later session, or a re-brief of the same unit |
| 4 | a ≤ 15-line digest in the reply | human review |
| 5 | the claim released (slot returned, branch deleted after the merge) | other lanes |

Why JSON and not prose: four workers writing four prose reports is exactly how the orchestrator ends up
reconciling numbers by hand — the failure mode this protocol exists to remove. Evidence lives in **MAIN**, not
the worktree: `.pi/` is destroyed by `git worktree remove`, so a worker writes `MAIN/.pi/outbox/<slug>.json`
and `MAIN/.pi/notes/<slug>.md` and the evidence outlives the lane.

### 10.3 The handoff is the worker's final message

A worker is a headless `claude --agent <profile> -p` process (`tools/units/lanecmd.py` builds the command,
with cwd at the slot): when it exits, its **last assistant message** is the run's output. A pane-launched worker
delivers the same last message into its pane, plus its outbox, and the orchestrator wakes on the pane going
idle. Either way there is **no completion tool to call**, so a brief that asked for one would name a tool the
worker does not have. A hand-written brief must carry the final-message instruction too. A lane that needs a
ruling ends its turn with the request; the orchestrator answers with `claude --resume <session-id> -p "<ruling>"`.

### 10.4 Who runs what

**Only the orchestrator runs bare `ninja`, the split, the link and `ok`.** Workers compile their own object
through `recompile.py` (direct compiler invocation, mtime asserted, section sizes printed) and measure with the
metric. Otherwise four processes fight over one build tree, and a green `ok` can come from a stale link.
**A lane never builds its own environment**: the orchestrator claims through `queue.py next` / `claims.py
claim`, which takes a free slot, resets it, cuts a fresh branch off `main`'s current tip and verifies the kept
build tree; the worker starts in the directory it is handed. The land gate **pre-flights MAIN before it
builds**, naming foreign/untracked paths *before* the expensive gate — the refusal is unchanged (a path that
appears during the build is still refused), only delivered earlier.

### 10.5 Failure modes and their handling

| failure | detected by | handling |
| --- | --- | --- |
| two workers, one unit | the branch already exists (`claims`) or the slot is locked (`slots`) | the loser takes the next unclaimed unit or the next free slot |
| a slot's kept build tree is stale | `slots.py verify`, or `acquire`'s guard | **re-seed from MAIN before handing over**; refuse if it still cannot be proven current |
| a worker dies mid-unit | stale worktree / branch never merged / no outbox entry | inspect its tree, re-issue the brief (brief step 1 compiles and measures first, so a half-written source is caught) |
| a worker edits a shared file | `land.py verify` diffs the tree against the expected file set | reject the merge, restore the file, re-brief; its other work survives |
| a worker measures a stale object | `recompile.py` asserts the mtime advanced | rerun; discard the numbers |
| a worker's number disagrees with the report | the orchestrator re-measures every symbol it claims | the orchestrator's measurement wins; the difference is investigated, never averaged |
| a worker wants a flag nobody else agrees with | the flag rule | it stays a *probe* in the outbox until a second unit or a source pragma backs it |
| a merge conflict | `git cherry-pick` stops | protocol violation: resolve, re-measure, record |
| a worktree cannot resolve the toolchain/target | `recompile.py` fails with the missing path | fail loudly — never silently compile nothing |
| the unit is only partially matched | the outbox says so and its score is below `main`'s | **measure before merging**: worse → drop and re-brief; better → merge, record the residual, mark `partial` |
| `main` moved while the worker ran | the cherry-pick conflicts, the worker's base is old, or `land.py` refuses to union the batch's header | the worker runs **`tools/units/mergebranch.py resolve` inside its own worktree** - main is merged into the held branch there, resolved by class, proven and committed, so the lane keeps working and one branch still lands - and the orchestrator re-measures after the apply regardless. The by-hand dance is no longer the path: `git apply --3way` refuses as soon as the working tree differs from the index, and inferring "already resolved" from the absence of markers once skipped a file left from main and silently dropped **157 header lines and a registration** |
| a claim cannot be released | `git worktree remove` fails (`Permission denied` / "Device or resource busy") | a live pane is sitting in the worktree; teardown first (§10.8) |
| a claimed seam is wrong | the unit's functions will not match | revisit the seam while the unit is small — matching settles the boundary |

### 10.6 A worker does not fan out subagents

A lane is one agent in one worktree: no profile carries the `Agent` tool, so a worker cannot spawn subagents
and a brief must not tell it to. A unit too big for one lane is split by the orchestrator into more claims
(more slots), each with its own branch, brief and commit — never inside a lane. One branch, one commit and one
writer per file therefore hold by construction, and every score in the handoff is the lane's own measurement.

### 10.7 Acknowledgement, heartbeats and timeouts

A terminal multiplexer cannot tell "finished" from "never started": both read as *idle*. Three mechanisms
remove the ambiguity:

* **Acknowledge first.** A worker's first action, before it reads the target disassembly, is
  `python tools/units/claims.py ack <unit> --agent <name> --pane <pane>`, writing `MAIN/.pi/ack/<slug>.json`.
  No ack within two minutes is an unambiguous failure, and the claim can be reclaimed immediately.
* **Heartbeat per iteration.** The same command with `--progress <symbol>` after each function measured. The
  list is the difference between "quiet because it is thinking" and "quiet because it stopped" — one second
  per function.
* **Status and timeout.** `claims.py status` reports `unacked` / `stalled` / `working` / `done` per claim,
  combining the ack file with what cannot lie — the commits on the branch (`rev-list --count <base>..<branch>`)
  and the outbox's existence — and exits non-zero when anything is unhealthy. `claims.py timeout [<unit>]
  [--apply]` reclaims the unhealthy ones: **it copies the branch to `refs/rescue/<slug>` first**, then removes
  the worktree, deletes the branch and drops the claim, so a timed-out worker loses the lock but never its work.

The orchestrator's rule of thumb: **an ack or an artefact, never a hunch.**

### 10.8 Teardown is part of the round

After the handoff and after the integration: close the worker's pane, **then** `claims.py release <unit>`. A
live pane holds its worktree as its `cwd`, and Windows refuses to delete a directory a process is in
("Device or resource busy"). A **tool-spawned worker has no pane**, so that step is skipped and `release`
returns the slot to `main`'s tip, deletes the branch and clears the lock; for a throwaway worktree
(`--no-slots`) it removes the worktree instead. The rescue ref the release parked the un-merged commits at is
audited on the spot: `redundant` is pruned, drift is reported, `unlanded`/`unknown` are named loudly and kept.
**Nothing in a round constructs a worktree by hand.**

### 10.9 Orchestration hazards

* **Parallel launches: one child per launch, and never a workflow child with `cwd = MAIN`.** The MAIN-cwd child of
  a three-child workflow never appeared and produced no error (R4). A read-only MAIN lane is a plain single
  headless launch (`tools/units/lanecmd.py`).
* **A lane that needs a ruling ends its turn with the request** and is resumed by `claude --resume <session-id>
  -p "<ruling>"`.
* **A slot goes to a problem before a new unit; a refused slot is refilled with a `fixer`, not a claim** (the
  queue refuses while a branch holds unlanded work). `queue.py next --count N` strides the address order, so no
  two wave lanes hold adjacent proposals (adjacency is the vector for shared-TU and rule-2 boundary clashes).
* **`attribute.py queue <start> <end>` REWRITES the queue file.** Run it over the whole unclaimed region (`cap:
  0`), or add briefs with `brief.py`; a partial run deletes the rest of the pool. Claimed proposals were once
  re-offered by `queue.py next`: check `claims.py list` first if it refuses at random.
* **`tools/splits/tudiscover.py` needs the asm dump** (`python tools/splits/dump_asm.py`; `write_asm: false` by
  default; `dol split` is then 200-400 s instead of ~18 s).
* **A `Matching` flip is one unit per commit, byte-identical, with a green `ok`** (§8); a lane on an exhausted
  row records the residual with both measurements and does not use `goto` (rule 8).

---

## 11. Owner rulings (standing policy, dated)

These are decisions, not lessons. Do not relax one without the owner.

| date | ruling |
| --- | --- |
| 2026-09-21 | The orchestrator has standing approval to commit its own campaign work without per-commit asking (rule 6); nothing is ever pushed, history is never rewritten. |
| 2026-09-24 | **Steady, not maximum, throughput.** Six lanes at most, covering unit workers and tool fixes together; refill one slot per completion, not in waves. |
| 2026-09-26 | A slot is not filled while finished work sits unlanded (`queue.py next` refuses; `--allow-unlanded <branch>` parks one on purpose). A lane gets the profile that matches its job (§2.4). |
| 2026-09-27 | **Budget is not a concern - keep lanes working.** Never cut a lane short for turns, tokens or wall-clock, never steer one to "wrap up"; if it looks like it is thrashing, report it and let it run. Only a genuinely exhausted row (every conformant shape tried and measured) stops a row. The published budget rule in the profiles is the worker's own judgement, not the orchestrator's reason to interrupt. |
| 2026-09-27 | **Slot pool approved.** Six reusable slots; a slot holds a directory, never a branch; every claim cuts a fresh branch off main's tip; a free slot is the concurrency cap; the reset is verified and fail-closed (§2). "A lane never builds its own environment." |
| 2026-09-27 | **Backlog policy v2 - ratio + triage.** One resolved backlog item (`range`, `shared-file`, `flag`, `naming`, `band-header`, `untyped`, open tooling rows) per new proposal claim; `done` earns a credit, `parked` earns none, a claim is free while the register is clean (`ledger["free"]`), `--ignore-backlog` is a deliberate override. `rename` requests are not backlog (the landing applies them). Triage is evidence-based: no evidence, stays open. |
| 2026-09-27 | **Naming: no exemption.** No `fn_`/`lbl_`/`loc_`/`unkNN` may survive in `src/`, whoever owns the symbol; the `rule 7 deferred:` key, the `src/auto/` key, the bodyless-file key and the own/foreign `lbl_` split are all deleted. The one grandfathering is the gate's `--diff` (existing findings never block, an added one refuses). Existing debt is register items ("work on them slowly" is the ratio). A dump cannot name them (26,065 of 30,992 dump entries are placeholders), so a name comes from context. |
| 2026-09-27 | **Inventing a context-derived name is sanctioned**: nothing is unnameable; the honest part is the **mark** (in the owner unit's header, else the file that uses it most), not refusing to name. A rename propagates to every referrer in one batch. |
| 2026-09-27 | **Rule 11: `void *` is banned** in parameter and return types; the exemption is a per-DECLARATION `/* untyped: <reason> */` (byte range, opaque handle, caller-owned payload), never a per-file key; never a cast; locals out of scope. Heterogeneous call sites mean the *sites* are wrong: settle the type once and let the compiler enumerate mismatches. A map name is the linkage contract, but a helper's **type** is not constrained by the binary. |
| 2026-09-27 | **Rule 7 covers data; rule 2 covers unowned symbols** (a local `extern` of an unowned symbol belongs in an `include/unsplit/` band header; rule 12 later made an unowned *data* extern a claim instead). A lane that needs an unnamed neighbour renames it, sweeps every reference and re-measures - naming, not deferral. |
| 2026-09-28 | **`decomp -> review -> decomp` is the standing procedure** (§1): a branch is reviewed read-only before it lands and findings are cleared on the same branch. |
| 2026-09-27 | **Every tool section in a profile carries a feedback loop**: a gaps note as a register source, check it before filing (a repeat is a vote), the report's tooling line, and a tool that *misled* is the most valuable report. A novel row ranks only with two distinct filers (§6). |
| 2026-09-27 | The `@etb_`/`@eti_` post-compile build step (`objextab.py`) was owner-approved; changing compiler flags, `mw_version` or tool tags still needs concrete evidence and an explicit call-out (rule 3). |
| 2026-09-28 | The `.init` TRK-image request was landed; the residual 164 B is closed as unclaimable (§8). Parked decisions still standing: `memcpy.c`/`memset.c` stay separate; `-func_align 4` waits for `Runtime.PPCEABI.H`'s next pass; one `Matching` flip per commit. |
| 2026-09-29 | **No `--allow-rule12` (and no `--no-outbox` for a code row) unless the owner rules.** `land.py --allow-rule12 <token>` is a recorded allowance and using it is the owner's call, not a lane's. The owner ruled once, for `DWCi_natProbeStatus` (`.sbss` 0x80795818, landed with `DWCi/DWCi_NatNeg`); its claim is scheduled in `.pi/data-requests.json` until the unregistered `fn_8050C770` band, which writes the word, becomes a unit. For `network_transport` the owner chose the real fix (peer classes + a `.data` claim) over an exemption. |
| standing | A rule enforced by remembering is not a rule: a rule change ships with its tool row (`stylelint`, `vtableaudit`, `sync_profiles`) in the same batch. |

---

## 12. Tooling and environment traps

Grouped by cause; each cost a lane ~30 min the first time.

* **git-bash / MSYS.** `recompile.py --measure` and `measure.py` are unusable from git-bash: `absolutize()` turns
  cmd's `/c` into `C:\c`, an interactive cmd runs and no object is written (the "`-o` is a DIRECTORY" message
  misattributes it; `MSYS_NO_PATHCONV` / `MSYS2_ARG_CONV_EXCL` do not help). Use PowerShell/cmd, or `ninja` +
  `tools/objdiff/symdiff.py -u <unit>`; an MSYS-safe whole-unit probe took 0.3 s.
* **Line endings.** Files are LF in the repo and working tree (`* text=auto eol=lf`; a clone must not set
  `core.autocrlf=true`, which overrides `eol=lf`). Older notes said `include/**`/`configure.py` are CRLF - if a
  scripted anchor with `\n` fails to match, `assert` the match count and read/write with `newline=""`.
* **Encoding.** Source stays UTF-8 with no BOM (`sjiswrap`); MWCC on this host turns a literal `\n` into CRLF
  (playbook 45); a Windows `PermissionError [WinError 5]` was seen once in a selftest on a fresh temp worktree
  (AV/indexer, not reproduced).
* **Shell.** A heredoc **truncates silently** when a second one follows (R8); prefer the file tool for long text.
  `m2c` needs `-t ppc-mwcc-c` (the default MIPS rejects `stwu`) and its `.s` needs a space after each comma.
* **Staleness.** `report.json` and `build/` are only current if you proved it (§7, R10, R12); `ok` prints OK off a
  stale `main.dol` and never sees an uncompiled `NonMatching` object (R6). `build/binutils`
  may be empty (`ninja tools`); `symdiff.py -u <unit>` lists every symbol (a bulk scorer over `report.json` was
  hand-written twice).
* **MAIN's build tree.** Anything that reads it treats it read-only: a seed lane once deleted `.ninja_deps` /
  `.ninja_log` and restored them with a full `ninja`. The merge warns about unreachable loose objects every time
  (harmless; `git prune` is open).
* **`symedit.py` cannot split a merged map row** (two functions in one row): a direct two-line edit is needed
  (a `split` subcommand is wanted). `dumpmap.py` emits junk duplicates (open).
* **Lint quirks.** Rule 3 wants the canonical `size: 0xNN`; rule 6 false-positives on `(u32)(*(u8 *)p - 252)`
  (write the dereference); a `#pragma` in a shared header leaks into every including TU (keep pragmas in the `.c`/
  `.cpp`; a lint is wanted; playbook 32/60); `stylelint --diff <ref>` auto-selects the merge base when the ref is
  not an ancestor, and a `--ref` that judges 0 files exits 2.
* **Harness.** A worktree cut before a profile edit serves the old prompt until `tools/agents/install.sh`.
  `handoff.py --check` may resolve MAIN's stale map (open). `docs/tooling-requests.md` is regenerated by
  `tooling.py`.
