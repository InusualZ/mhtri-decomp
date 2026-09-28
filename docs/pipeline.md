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
| `decompiler` | unit work: proposals, body passes, and the phase-1 and phase-3 legs of the pipeline |
| `fixer` | a **refused gate**, or a measured regression — it fixes the named failure, not the neighbourhood |
| `merger` | a refused apply — but prefer `mergebranch.py` (§5) |
| `codereviewer` | the phase-2 review; **read-only by construction** (its tool list has no write/edit) |
| `worker` | anything else, and tooling |
| `scout` / `planner` / `reviewer` | read-only recon, planning, and review outside the pipeline |

The rule-text blocks in these profiles are **machine-generated** from `docs/plan.md` §6.5 and the matching
skill: run `python tools/agents/sync_profiles.py --check` and never hand-edit between the markers.

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
* **`localonly.py pull` → land → `push`, and never patch the block while it is pulled out.** The local-only
  block in `AGENTS.md` is pulled out for a landing and restored immediately after; editing it while it is
  pulled out is how the block drifts.
* **The bootstrap exception.** A change **to the gate itself** cannot be gated by the gate it is changing, so
  it is committed **directly, path-limited** (the gate's own files and nothing else). This is the only landing
  that skips the gate, and its path-limit is what keeps the exception from becoming a habit.

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
  an input is how a report and the objects it describes drift apart.

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
| `tools/units/langcheck.py` | **a unit's language is in question**: decide C vs C++ from evidence (a mangled definition, a `.cpp` `__FILE__` string), never from convenience. |
| `tools/units/recordmerge.py` | **two lanes each hold a view of the same record header**: fold them into one definition with the checks the hand passes lacked. |

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

A tool-spawned worker is a `pi --mode json` child the `subagent` tool waits for: when it exits, its **last
assistant message** is returned to the orchestrator as the tool result. A pane-launched worker delivers the
same last message into its pane, plus its outbox, and the orchestrator wakes on the pane going idle. Either
way there is **no completion tool to call** — the old `subagent_done` sidecar belonged to an extension that is
no longer installed, so a brief that asked for that tool would name a tool the worker does not have. A
hand-written brief must carry the final-message instruction too.

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

### 10.6 A worker may fan out subagents — under the same rules

A unit is often several independent functions, so a worker may spawn its own subagents, under three rules that
are part of the brief:

* the subagents work **in the worker's worktree, on the worker's branch** — one branch, **one commit**, made by
  the worker; a subagent never commits;
* the worker assigns **disjoint files or functions** (one writer per file, inside a worker too) and is
  **accountable for everything its subagents produce** — it re-measures every claim they make, exactly as the
  orchestrator re-measures the worker's;
* every subagent is handed the rules (the brief's part 6) **verbatim**. A subagent that has not read them will
  name a field `unk4`, reach it with a pointer cast and use a `goto` — repair work charged to the worker.

The worker's handoff covers its subagents' work as its own: `measured_with` says how the numbers were obtained,
and `residual` covers whatever they left unfinished.

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
