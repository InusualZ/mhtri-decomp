# Campaign plan: from 217 matched functions to a byte-identical `main.dol`

**This file is the contract.** The orchestrator follows it as written — 100 %, not "mostly" — and the end goal is
not "a lot of matched functions": it is **`main.dol` rebuilt from `src/` alone, byte-identical to
`config/RMHE08/build.sha1`** (`BF4850739478CAAEDFE675949EB7C28595A7FDE9`), with every unit we register sitting
at its final `src/<module>/<name>.<ext>` home - a function still unclaimed in `symbols.txt` is a *proposal no
worker has taken yet* (§6.1), not a defect.

Version 2.3 (2026-09-22), after **three rounds of subagent peer review** (findings kept in
`.pi/reviews/plan-r{1,2,3}-*.md`). Round 1 corrected the flip candidates, the per-module numbers, the overloaded
word "batch", the worktree measurement gap and the tools that do not exist yet; round 2 added the ground-truth
guard, the verify-then-commit order, the submodule trap and the honest size of the campaign; round 3 fixed the
remaining contradictions (the `.ninja_log` proof, the completion test, section 12, the cross-references).

**Version 2.4 (2026-09-24).** The `src/auto/` scaffolding bucket is retired: a unit is registered **once**, at
its final home, by the worker that works it (§12).

> **Status 2026-10-03.** The splits program is finished (`docs/splits-program.md`): `splits.txt` cuts the whole DOL into
> registered units (354) with no unclaimed `.text`, and the bulk tiler (`attribute.py`), its proposal queue and the
> `.pi/bin/*` landing scripts are retired (`docs/tools/retired.md`). Passages below that name `attribute.py`, a
> "proposal" or the "attribute" step are the history of how units were first registered; today the queue is the registered
> units that are not `Matching` (`queue.py list`) and the only landing path is `land.py land --branch`. How a unit is
> worked - the pair run, or a wave, its multi-lane form - is `docs/pipeline.md` §15, §1.3 and §14.

---

## 1. The end goal, and what "done" means

| level | done means |
| --- | --- |
| a **symbol** | attributed to its unit, decompiled, measured ≥ 80 % per-symbol `fuzzy_match_percent`, committed |
| a **unit** | every symbol it owns is closed or explicitly `partial` with its residual in the file header; its ranges measure no worse than before; its **object** is byte-identical if it is to be flipped |
| a **batch** | every symbol it set out to close is closed (or recorded `partial`), the ranges it claimed measure no worse, its **knowledge delta** is written, `ninja build/RMHE08/ok` is green, and it is committed |
| the **campaign** | (1) every **registered** unit sits at its final `src/<module>/<name>.<ext>` home and **no `auto/*` placeholder unit remains** - and `ledger.py`'s **unclaimed** lines are a *proposal backlog* (ranges attribution has offered, §6.1), not a defect; (2) every unit is named from evidence or is listed as a named exception in §10 with a reason; and (2b) **no unit's source has zero scored symbols** — a header-only stub is a note, not a unit, so `ledger.py` must show every unit contributing at least one measured function; (3) `rm -rf build/RMHE08 && python configure.py && ninja build/RMHE08/ok` passes **from a clean build tree**, so no stale split object can be carrying original bytes; (4) the DOL hash equals `config/RMHE08/build.sha1`, and that file's own sha1 still matches the value in `CLAUDE.md` (it is ground truth and is never edited - §8.1) |

**Why clause (3) is not paranoia.** Today `ninja build/RMHE08/ok` is green while 65 128 of 5 437 392 bytes are matched, because the 13 565 `auto_*` objects carry the original DOL bytes. A green `ok` therefore proves *nothing* about our source until the tree is rebuilt from scratch - and that is exactly the check the campaign must end on.

Two rules keep the bar honest and are **repository** rules, not campaign policy (non-negotiables 3 and 4):

* `Matching` means **byte-identical** — every symbol 100 %, sections and relocations equal, proven per the
  `objdiff-verify` skill. A unit closed at 80 % stays `Object(NonMatching, …)`, its bytes are not linked, and
  that is what keeps `ok` green for the whole campaign.
* The **80 % bar closes a symbol**; it never changes what "matching" means.

The last mile is a *linking* campaign: units flip to `Matching` one at a time, each flip verified alone, because
the DOL hash only starts depending on our objects at the first flip. **Start that early** (§7.6) — the linking
questions (`.ctors$10` placement, pool sharing across units, section order, `extab` pairing) are cheapest to find
while the unit count is small.

**A flip needs a byte-identical *object*, not a byte-identical library.** `Runtime.PPCEABI.H` is 19 of its 20
symbols at 100 % — `Gecko_ExceptionPPC.cp`'s `__register_fragment` is 93.68 % (a loop-preheader scheduling
tie-break, §10 item 6) — so the flip candidates are the six **objects** that are byte-identical: `memset.c`,
`memcpy.c`, `__start.c`, `__ppc_eabi_init.cpp`, `__init_cpp_exceptions.cpp`, `global_destructor_chain.c`. Each is
proven per `objdiff-verify` before it is flipped. Gecko's object is not a candidate until that residual is settled.

### 1.1 The phases, and the honest size of this

The campaign is not one loop; it is four phases with different economics, and confusing them is how a plan misallocates its effort:

| phase | what it does | cost per unit of progress |
| --- | --- | --- |
| **A. attribute** | (retired 2026-10-03: done by the splits program) propose ranges, seams and evidence; registration happened when a worker took a proposal (§12) | ~1 min per proposal, no source work |
| **B. decompile** | write bodies to >= 80 % | a bounded slice per lane (~20 functions or ~60 minutes, `docs/pipeline.md` §15.1) |
| **C. second pass** | push the 80-99.9 % band to 100 % | the playbook's idea list, per unit |
| **D. flip** | prove byte-identity per object, link it, keep `ok` green | minutes per object, but the DOL hash starts depending on us (`docs/pipeline.md` §15.3) |

**The honest arithmetic.** The plan's job is not to promise a date - it is to keep each round cheap (the protocol), to keep the method compounding (the knowledge delta) and to keep the queue measured (`ledger.py`, §2). **Phases C and D are on the critical path for the DOL**, not optional polish: a unit with any residual cannot flip, and the DOL cannot be byte-identical until every unit can.

## 2. Where we are

No totals are kept here: they are *derived*, never remembered.

* **The numbers**: `python tools/units/ledger.py` (covered / closed / matched, the `.text` denominator of §6.3) reads
  `symbols.txt` (through `symedit.py`), `splits.txt`, `configure.py` and `build/RMHE08/report.json`. A stale
  `report.json` lies — the ledger warns when it predates `splits.txt`/`configure.py`, and regenerating costs 1.5–3.5 s.
* **The queue**: the registered units that are not `Matching` (`python tools/units/queue.py list`); the flipped set is
  `configure.py`'s `Object(Matching, ...)` entries.
* **The flags**: per library in `configure.py`, each with a one-line pointer to its unit header's evidence; this plan
  asserts no default set (§8.2).
* **The tools**: `docs/tools/README.md`.
* **What is in flight** (claims, open pairs and waves, parked decisions): `.pi/state.md`; how to re-measure:
  Appendix B.

## 3. What a batch costs — and the batching law

Per **edge**, from `.ninja_log` (the first draft of the review used wall clocks taken under contention from a
second stream; these are the measured per-edge costs, and `docs/build-performance.md` has the breakdown):

| edge | measured | why it matters |
| --- | --- | --- |
| `dtk dol split` (`build/RMHE08/config.json`) | **16.8 / 16.8 / 27.1 / 46.5 s**; 200–400 s if `write_asm: true` | re-runs on *any* edit to `symbols.txt`/`splits.txt`/the DOL — it is an input of itself, so renames ride the batch |
| the link (`main.elf`) | **66.8 – 130.9 s** over 13 584 objects | the largest edge, paid once per batch whether one object changed or fifty |
| one source unit (`mwcceppc`) | **0.1 – 24 s** (our own log shows 0.13–1.20 s for the units rebuilt lately; `Pl/pl_skill.o` is the worst measured) | what a worker pays per iteration — and a worker pays *no* split and *no* link |
| `report.json` / `baseline.json` | 1.5 – 3.5 s | cheap enough to regenerate before believing anything |
| `main.dol` + `ok` | < 0.5 s | the real test, effectively free |
| the asm dump (`python tools/splits/dump_asm.py`) | **200 - 400 s** | not a build step (`write_asm: false`), but `tudiscover` reads it and **every batch's own `splits.txt` edit invalidates it** - which is a second, independent reason to batch attribution: run it once per attribution batch, check it with `--check` |

**Three different things get called "batch" — keep them apart:**

| term | what it is | size |
| --- | --- | --- |
| **registration batch** | the ranges and `configure.py` entries claimed in one go (one split, one ledger check) | **≤ 0.5 MB of `.text`** (owner's call) — ≈ 2 000 functions, a blast-radius ceiling, not a target |
| **work round** | what the lanes in flight do in parallel (at most six; a pair run is two, `docs/pipeline.md` §15) | a slice of ~20 functions per lane |
| **land batch** | everything that goes into one commit on `main` (one split, one link, one `ok`) | one or more work rounds, plus the renames/merges that ride them |

**The law, in five clauses:**

1. **One split and one link per land batch.** A batch boundary costs ~1.5–2.5 min of machine time; a rename-only
   batch that changes no object is ~20 s.
2. **Cap a registration batch at 0.5 MB of `.text`** (owner's call) — it binds the *attribution* pass, where one
   command can claim a whole region. The tiler that this cap bound is retired (2026-10-03); a window of `splits.txt` edits is
   now capped by the lane that makes it.
3. **A round is the pool's six slots wide** (owner, 2026-09-24: six lanes at most, refilled one per completion;
   `docs/pipeline.md` §2.1, §13.1). What that cap bounds is *coordination*, not CPU: only the orchestrator runs
   `configure.py`/`ninja`/the link, while a lane measures its own object with `recompile.py` inside its own slot
   (`docs/pipeline.md` §10.4). The orchestrator's verification time, not the machine's, is the scarce resource.
4. **Renames, phantom merges and range claims ride the same batch** as the source work, because each is a split
   dirty-check input. `symedit.py rename-batch <file>` collects renames; verify every renamed symbol *after* the
   batch's split, before the commit.
5. **Source-only changes never wait for a split** — they ride the next one for free, and an already-registered
   unit is pure source work (`main.cpp` was registered once and filled over three batches).

## 4. Roles and responsibilities

> **Tool status.** The tools 4-6 name exist and run: `recompile.py`, `claims.py`, `brief.py`, `handoff.py`,
> `land.py` (7.1-7.5), the data queue `dataqueue.py`/`dataclaim.py` (7.17), `stylelint.py` (7.21),
> `recordmerge.py` and `rescue.py` (7.27). The commands below are the ones actually run; 7 keeps the status of
> anything still open.

**The four shared files (`config/RMHE08/splits.txt`, `configure.py`, `config/RMHE08/symbols.txt`, `CLAUDE.md`)
have exactly one writer: the orchestrator.**

| responsibility | orchestrator (me) | worker (a lane in its slot) | owner | tooling |
| --- | --- | --- | --- | --- |
| pick the work (§12): a pair or a wave (`docs/pipeline.md` §1.3) | **owns** | — | — | `queue.py next --count 2`, `ledger.py` |
| a pair's dependency list and pre-pass (`docs/pipeline.md` §15) | **owns**: launches the read-only scout and scout review, lands each pre-pass batch alone | after the pre-pass, edits only its manifest's `owns`; a missed dependency is an integrator request | — | `docs/pair-briefs.md`, `land.py land --manifest` |
| register a unit: range, module/name, `configure.py` entry | **applies it on `main`** (one writer for the shared files) | **writes the registration in its own worktree**, for the unit it works | — | `symbolpreflight.py`, `tudiscover.py` |
| write a unit's source | reviews, integrates | **owns one unit, one file, one worktree** | — | `m2cinput.py`+`m2c`, Ghidra dump, `DumpSymbols.map` |
| measure a symbol | **re-measures every claim** | measures its own unit | — | `recompile.py`, `mt.py diff/info`, `symdiff.py` |
| land a compiler flag | **owns** — decides, and the decision needs §8.2's evidence | probes, reports numbers; may put a **one-unit** deviation in the source as a pragma (it owns that file) | — | the `configure.py` comment is the evidence; the pragma is the exception |
| claim a data range | **owns**, measured before/after | proposes with numbers | — | `dataclaim.py`, playbook 23 |
| run `ninja` / the split / the link / `ok` | **sole runner** | **never** (uses `recompile.py`) | — | `land.py verify` |
| git: branches, merges, conflicts | **owns** (`land.py land`, one unit per commit) | commits only on its branch; never merges `main` (`docs/pipeline.md` §11, 2026-10-07) | — | `land.py`, `mergebranch.py` |
| commit on `main` | **owns** | never | grants standing approval | `prepcommit.py` |
| knowledge delta (playbook row, skill line, unit header) | **owns the gate** | reports what it learned | — | `sync_reference.py --check` |
| type and naming discipline (§6.5) | **owns the gate**: lints every branch at the land step and refuses one that adds a violation | **writes to the rules** — its brief embeds them verbatim | — | `stylelint.py` (7.21) |
| decisions only the owner can make | escalates, does not guess | — | **owns** | §10 |
| DOL hash safety | **owns** | cannot touch it (branch, no `Matching`) | — | `prepcommit.py`, `land.py` |
| keep `main` free of worker commits | **owns**: the batch records `main`'s HEAD as its base (`land.py record-base`), and the gate refuses a batch whose base moved | must not commit, rebase or amend on `main` | — | `land.py` |

**What the orchestrator must not do:** read source files and disassembly as its default mode. It reads the
ledger, worker digests, `git diff --stat` and numbers; every read-heavy question is cheaper as a worker's answer.
It is the *librarian* of the compounding asset (§9), not the fastest decompiler.

## 5. The coordinator protocol (worktrees, branches, briefs, outbox)

**Moved to `docs/pipeline.md`, which is the authority for *how* a batch is run**; this file stays the authority for
*what* is worked and in what order (§3, §6, §12). The subsection numbers stay so old citations resolve:

* **How a unit is worked** - the pair run, and the wave as its multi-lane form: `docs/pipeline.md` §15, §1.3 and §14.
* Superseded outright: the manual merge (now `tools/units/mergebranch.py resolve`, and a lane never merges `main`:
  `docs/pipeline.md` §11, 2026-10-07), per-claim worktree construction (now the slot pool), cherry-pick landing (now
  `land.py land`), and the pi harness with its panes and `subagent_done` tool (the handoff is the final message).

### 5.1 The worktree layer — the branch *is* the claim

The slot pool, the `.used` sentinel, E1-E3 and slot hygiene: `docs/pipeline.md` §2. The launch line and in-session
lanes: `docs/pipeline.md` §13.2. Evidence collection (`slots.py collect`): `docs/pipeline.md` §10.2 and §12.

### 5.2 The worker's input — one generated file, nothing else

`docs/pipeline.md` §10.1 (the brief) and §10.3 (the final-message handoff).

### 5.3 The worker's output — data, not prose

`docs/pipeline.md` §10.2.

### 5.4 Who runs what, and the failure modes

`docs/pipeline.md` §10.4 and the failure-mode table §10.5.

### 5.4.1 The agent profiles

`docs/pipeline.md` §2.4 (which profile each leg runs, and `slots.py spawn --kind`).

### 5.5 A worker does not fan out subagents

`docs/pipeline.md` §10.6.

### 5.6 Acknowledgement, heartbeats and timeouts

`docs/pipeline.md` §10.7; teardown §10.8.

## 6. The loop - four steps, one gate each

> **A worker never runs `ninja`, the split, the link or `ok`** (§5.4). In this section the `ninja` commands are
> the orchestrator's; a worker's equivalent is `tools/units/recompile.py` (7.1). How the steps are run - claim,
> scout, pre-pass, lanes, review, landing - is `docs/pipeline.md` §15 (the pair run) and §1.3.

| # | step | tool | gate before moving on | left behind |
| --- | --- | --- | --- | --- |
| 1 | **attribute** | `symbolpreflight.py`, `tudiscover.py` | the verdict is `proceed`, or `approve` with a written proposal the owner accepted (§10) | the unit's range, seams and evidence (now already in `splits.txt`, §12) |
| 2 | **decompile** | the Ghidra dump (`docs/memory-dump.md`), `DumpSymbols.map`, `m2cinput.py` → `tools/m2c` | it compiles, and the disassembly agrees with every instruction-level decision | `src/<Dir>/<file>.c` (+ header beside it) |
| 3 | **match** | `recompile.py`, `mt.py diff/info`, `ninja changes`, `report.json` | the symbol's own score ≥ 80, nothing else regressed, `ok` green | the residual in the unit's file header |
| 4 | **commit** | `land.py land` (its gate is `land.py verify`) | the batch's knowledge delta is written and `sync_reference.py --check` is clean | one commit per unit, the ledger moves |

### 6.1 Attribute

```sh
python tools/units/symbolpreflight.py <address|name>   # owner, collision verdict, registration drafts
python tools/splits/tudiscover.py at <address|name>    # TU boundary proposal for an unowned address
#   the bulk tiler (`attribute.py plan`) is retired: every range is registered in `splits.txt`; a re-cut of one
#   region uses `tools/units/unwindcut.py` and `splitcheck.py --baseline` audits the result (docs/splits-program.md).
```

* **A unit is registered once, at its final `src/<module>/<name>.<ext>` home** (§12), inside the worker's own worktree so
  it can measure, and the orchestrator applies that registration on `main` with the batch's one re-split.
* **A registered unit has a source file; its bodies follow.** The file is created with the registration and its
  header says what it is, its range, why it sits there, what is unknown, and where its inventory lives
  (`ledger.py unit <path>`), never a copied function list. A registered unit whose source does not exist is a
  bug (the build warns `Missing source file`); a unit that still carries only a header at the end is a note, not
  a unit (§1 clause 2b).
* **Evidence-first partition.** A cut is made only at a seam a narrow (≤ 4 cuts) *strong* observation pins; one over the old size cap was flagged as a guess (the tiler that cut by size is retired). A
  region with no evidence stays **one** unit whose header says the seam is unproven — one function per file is
  certainly wrong, one file per region is only unproven.
* **Confidence is recorded, not hidden**, and a seam may only be claimed from `tudiscover`'s evidence kinds; a
  shared static or a call pattern is a *hint* for the header, not a seam.
* **Data is a second pass, measured - and it needs a queue, not comments.** The registration writes the data runs
  it saw into `splits.txt` as comments, which nothing reads. Roadmap 7.17 turns that into
  `tools/units/data-queue.json` (`{unit, section, start, end, labels, leak, density, verdict}`), which
  `dataclaim.py` consumes and `land.py` reports. A range our object does not emit must **not** be claimed
  (playbook 23): two of this campaign's three data decisions were "do not claim", and both would have cost
  score.
* **Registration must be transactional and never overlap.** Today the write is in place and only skips a unit that
  is already in `splits.txt`, so a crash between the two files leaves a half-registration — and `plan` over a whole
  `.text` produces 23 overlapping data spans today. Roadmap **7.20** fixes it: validate every proposal (no range
  overlaps a claimed one, no unit repeated), write `configure.py` and `splits.txt` through temp files and rename
  them, and restore both on any failure. Until then the recovery is
  `git checkout config/RMHE08/splits.txt configure.py` and re-running `plan`.
* **Run `python tools/splits/dump_asm.py --check` before `plan`/registration.** `tudiscover` reads the asm dump, and a
  stale dump is silent — it zeroes a codegen fingerprint and turns a seam proposal into a guess. The dump is
  invalidated by *any* edit to `symbols.txt`/`splits.txt`, including the batch's own.
* **Never claim linker-generated data** (`_rom_copy_info`, `_bss_init_info` — MW ld emits them; the DOL
  reproduces them byte-identically), and check whose `extab`/`extabindex` fragment it is before claiming one.

### 6.2 Decompile

```sh
python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o -f <symbol> -o build/tmp/<symbol>.s
python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f <symbol> build/tmp/<symbol>.s
```

The disassembly is the arbiter; both decompilers are shapes, neither is codegen evidence. A rename is
`symedit.py` **plus the source in the same edit**. The unit's file header is written on the commit that creates
the file: what it is, its range and function order, where its flags/evidence live, the residual — never a
per-function inventory and never percentages.

### 6.3 Match

```sh
python tools/units/recompile.py <unit>              # compile alone, no ninja, mtime asserted
python .claude/skills/mwcc-unit-matching/scripts/mt.py diff -u <unit> <symbol>
python tools/units/land.py verify                   # the batch gate: split, link, ok, regressions, ledger
```

The bar, all of it: (1) the symbol's own `fuzzy_match_percent` ≥ 80 — a function entry **without** the key is
0 %, and `complete_code_percent` is not a score (it has read 100 % next to 1.77 %); (2) no other symbol
regressed; (3) `ok` green; (4) the object measured came from the **real command line**, not a hand-written
compile; (5) the residual is in the unit header.

**What a match percentage is a fraction of.** The denominator is the game's **entire `.text`** - all of its code,
**5,449,596** bytes (`.init` 9,928 + `.text` 5,439,668, read off the linked ELF, the same artifact the numerator
is measured on). It is never the DOL binary (**6,683,744** bytes) and never a subset of the code - not the bytes
we have claimed, not the bytes in registered units. The report's own `measures.total_code` (5,437,424) is that
same universe minus unattributed padding, so quote it only when comparing like for like (it is what
`ledger.py`'s "matched / total of .text" line prints); when reporting progress to a human, say
"N % of the game's entire `.text`" and give the byte count so the fraction is unambiguous.

**"Re-measure the unit" means exactly this:** run `recompile.py <unit>` (or `ninja build/RMHE08/src/<unit>.o`
when I am the one at the keyboard), then `mt.py info -u <unit>` and `mt.py diff -u <unit> <symbol>` for every
symbol the batch claims, and compare against the numbers in the worker's outbox — a difference is investigated,
never averaged.

Below 80 %: land it only if it measurably improves the unit and regresses nothing, mark it `partial`, and put
the residual in the header — the `CLAUDE.md` playbook rows are that second pass's todo list.

### 6.4 Commit

One commit per unit, landed by `land.py land --branch <b> [--manifest <path>]` (`docs/pipeline.md` §4, §15.1 step
6); a pre-pass batch is its own commit, landed alone before its lanes. The subject follows `CLAUDE.md`'s
`<category>: <message>` convention; the commit carries the sources, the registration, the flags it proved and the
knowledge delta. `prepcommit.py` stages explicit paths only, refuses
build/original/scratch output, and verifies the DOL SHA-1. Live working state is kept in `.pi/state.md`, not in `CLAUDE.md`
(non-negotiable 8). History is
never rewritten; nothing is ever pushed.

### 6.5 Type and naming discipline — mandatory in phases B and C

Fifteen rules. **All fifteen are checked** - rules 1-9 and 11-15 by `tools/units/stylelint.py`, rule 10 by
`tools/units/vtableaudit.py` at the land gate (its violation count is the `r10` column of `stylelint.py --budget`). There is no "landing-review rule" here: that phrase was
retired 2026-09-27, when `src/Network/fn_803D3CE8.cpp`'s hand-written `self->vtable = &NetworkSessionManagerVTable;` sat
through a landing review under it - a rule whose only enforcement is a habit is not a rule. They are **not style preferences**: a wrong or
unnamed type is what makes
the *next* function in the same unit cost twice as much, and pointer arithmetic hides exactly the layout that
rules 3 and 4 exist to record.

| # | rule | what it means concretely |
| --- | --- | --- |
| 1 | **A shared type lives in one header** | a type more than one unit uses is defined **once** (in its owner's header beside the owner's source under `src/`, or in a hub header such as `src/types.h`) and *included* where needed — never copied. The existing convention applies: a declaration moves into a shared header the *second* time a unit needs it, never the first |
| 2 | **An extern lives with the TU that owns the symbol** | a **declaration** of a function or variable belongs in the source or header of the translation unit that **defines** it, and consumers include that. Re-declaring someone else's symbol in your own file "to save an include" is forbidden, and the finding is not the `extern` keyword: a plain prototype (`void foo(void);`) is the same defect, which is how the foreign declarations were actually written. The rule is read in two file classes - a source file and an ordinary header `src/<module>/*.h` (an owner's own `src/<module>/<stem>.h` beside its source is clean, which `_owns` must recognise or every owner's header reports itself) - and in the band `src/unsplit/*.h` the reading inverts: a declaration there of a symbol a registered unit **owns** is the finding, because the band is a fallback, not the owner. A symbol **no registered unit owns** (the map resolves it to an unsplit address) belongs in a band header under `src/unsplit/`, never a local `extern`. **A leaf header** is the one other owner's-header spelling: `src/<module>/<symbol>.h`, named for a symbol it declares and declaring only symbols one registered unit defines (for an owner whose full header redefines shared types and cannot be included beside its consumer); it counts as that unit's header, and adding a symbol of another unit or an unowned one makes it foreign again - there is no per-file exemption |
| 3 | **A reconstructed class/struct states its size** | every reconstructed type carries `/* size: 0xNN */`, traced from the evidence (allocations, `memset`/`memcpy` lengths, the object's `.data`/`.rel` records, the runtime dump). An approximation is allowed **only** if it is marked as one |
| 4 | **Every field carries its offset** | `/* +0x1C */` on the field, in ascending order, so the layout is readable at a glance and a reviewer can check it against the disassembly |
| 5 | **Every field has a name from its context** | what is stored, compared against, passed on. The **only** exception is a padding or unused field — present in the original object but untouched by the functions we match — which gets `pad_0xNN` / `unused_0xNN` **and keeps its offset** |
| 6 | **Pointer arithmetic to reach a field is forbidden** | `*(u32*)((u8*)self + 0x1C) = v;` is not acceptable; declare the type and write `self->field = v;`. A raw byte offset is allowed only where no field is being named (`memset`, a byte-wise copy, a `sizeof`/offset computation) — and even there prefer `offsetof(Type, field)` |
| 7 | **Symbols have proper names** | a function that arrives as `fn_XXXXXXXX` gets a name for **what it does** plus the naming scheme of its neighbours; a variable or field that arrives as `unkNN` gets a name for **what it holds** and where it is used; a data label (`lbl_XXXXXXXX` / `loc_XXXXXXXX`) gets a name from what it holds and where it is used, and the map row is renamed in the same change. **No generated name survives, in any `.c`/`.cpp`/`.h` (owner, 2026-10-05)** — an identifier with a `fn_`/`lbl_`/`loc_`/`dtor_`/`zz_` + eight-hex stem **anywhere** in it (`fn_X`, `view_fn_X`, `fn_X__FPv`, `fn_800FD864_fx`), an **address-named** identifier (a DOL address inside the name: `Panel805482CC`, `s_80276B58`, `Helper_80147CE0`), a bare `unkNN`, and every **file or directory name** spelled the same way (`fn_805113B0.cpp`, `src/fn_8004CAD8/`), whoever owns the symbol: this unit's, another unit's or an unowned one. Comments, string literals and `#include` lines do not count (an include path is retired by renaming its file). It is a **ratchet with no exemptions**: the gate refuses a finding new to its file by identity (rule 7, file, token), counts only fall, and a rename is credited; **a unit newly registered under a generated stem or directory is refused outright** (a GUESS name is allowed and marked in the unit header; a `--unit-rename` from a generated stem is credited). There is no exemption and no deferral  **The unblock is to NAME the callee, not to excuse it (owner, 2026-09-27).** A lane that needs an unnamed neighbour - a call to another unit's `fn_XXXXXXXX` - is expected to complete the work: rename that symbol in the map (with the evidence, from the runtime dump or the code), sweep every reference site in `src/` in the same change, put the declaration where section 6.5 says it belongs, and re-measure the owner and its consumers (playbook row 60: a shared header's declaration set is a codegen input). Rule 7 asks for the **name**; it never asked for the body, so naming unblocks the caller without touching who owns the code. Doing this is the lane's job, not a raised hand |
| 8 | **`goto` is forbidden** | No `goto`, and no label used as a control-flow device. Where a shared tail or a dispatch layout looks like it needs one, the conformant shapes are a **helper function**, a `switch` whose cases share a `break`, or a `for (;;)` with `break`/`continue` - and if none of them reproduces the target's codegen, that is a **residual to record with both measurements**, not a licence to use `goto`. The rule exists because the shape is unreadable in isolation (the target of a jump can be a hundred lines away) and it defeats the point of a reconstruction that someone has to read |
| 9 | **A mangled symbol is called through its owner** | a map name that carries an argument list (`Name__FP...`) or a class/namespace qualifier (`Name__Q34nw4r...`) is a **mangling**, i.e. a compiler spelling of a class member or a namespaced function, and must never be written as the callable identifier. Declare the owner (the class or namespace) and call `obj->method(args)` / `ns::function(args)`. The same holds for a **declaration** of the mangled spelling, which is where the C++ front-end mangles it a second time (playbook row 50); an `fn_XXXXXXXX` stem is the map's own placeholder, not a mangling, and is rule 7's to name |
| 10 | **A vtable we own is compiler output** | **this is a C++ project, and a pointer field at `+0x00` that points at a table of function pointers means the original was a class with inheritance** - model it as a class with `virtual` methods, never as a struct with a vtable member the code assigns by hand, because MWCC then emits the table *and* the store. A table of code pointers inside the unit's own registered ranges is **emitted by MWCC** from such a class - never written out entry by entry, never declared `extern`, never declared through a `void**` member. **The assignment is the discriminator**: reading slots through someone else's table is legal (rule 10 Case 2 - a table *outside* our ranges belongs to another TU: reference its `lbl_` symbol, and a struct of typed function pointers is the way to call a slot without dragging a class into the TU, since declaring the class would make MWCC emit a table into our object - extra bytes), while *writing* a `+0x00` function-pointer-table pointer from this unit's source is the violation, whatever the table is called on the right (`lbl_XXXXXXXX` and `NetworkSessionManagerVTable` are the same defect). **A table we wrote is not evidence of inheritance** - inheritance comes from the object's structure: the slot addresses read out of the DOL, the constructor's store, and the constructor/destructor chain |
| 11 | **No `void *` parameter or return type** | a `void` `*` in a function declaration's parameter list or return type is a finding by default: erasing the real type hides what the call sites are actually passing, and a heterogeneous call site is evidence the *sites* disagree, not that the declaration is untyped - name the type, and fix the sites. The only exemption is a **per-declaration** marker comment, `/* untyped: <reason> */`, on the declaration or the line above it, whose reason says which genuinely-untyped case it is: a byte range (`memcpy`-shaped), an opaque handle passed through, or a caller-owned payload, and a marker on the line above must **stand alone** (a trailing marker on one declaration never exempts the next). A `void *` **local variable** is out of scope (the lint counts them so the owner can decide), and the lint reads **declarations**, never a `(void*)p` cast in a body. `grep -rn "untyped:" src include` is the complete, reviewable list of exemptions - a file can never exempt itself, exactly as rule 7's per-file keys were removed. The rule is banned outright, so the existing tree is grandfathered only by `land.py`'s `--diff` (an existing finding never blocks a landing, an added one refuses) and the debt is filed as one `untyped` backlog item per file |
| 12 | **Data a unit uses and nobody owns is the unit's to claim and match** | when a unit reads or writes an address (or symbol) that **no registered `splits.txt` range covers**, the local `extern` is the finding - the unit **claims that range in its own `splits.txt`**, in the section the bytes live in, and then **matches it as part of its own object**, the bytes reconstructed so they byte-match the target. A header comment that says "no registered owner" is the finding naming itself. **Claim the whole run, with evidence**: the address, the referrers (`tools/units/callers.py <addr>` - address-keyed, because the asm dump is stale), the size (the target object's symbol size, or the emitted run) and the section boundary. The playbook caveats bind: a **partial** `.sdata2`/`.sdata` claim does not link - claim the pool only when our object emits none (playbook 23/29); a unit claiming several runs of one section must own the bytes **between** them, or an `auto_*_data` unit lands inside its range and `dtk dol split` dies with a link-order cycle (playbook 53); and a `.data` claim can make dtk drop the target's `R_PPC_NONE` pool relocs, so measure before *and* after (playbook 23). **Declare-never-define stays right when the range is ALREADY the unit's own** (playbook 29): there, *defining* the constants rebuilds the pool and moves the whole section, which is why the old advice exists - this rule targets the unowned case, where nothing is claimed at all. **Never claim what is not yours**: data another **registered** unit owns means include that owner's header (rule 2), and bytes the target object does not carry at all are compiler-synthesised (playbook 58 - claimable only while your unit is its sole referencer). The **test** is `datagap.py --unit` showing no target-extra for the claimed range, the unit's `flipcheck` reporting the data section byte-identical, and `ninja build/RMHE08/ok` green with the DOL unchanged. **Existing debt is a register item**: every current `extern` of unclaimed data is a `range` item (the register already has that kind - a claim IS a range change), so the credit ratio rations it like everything else |
| 13 | **A method is a member** | a free function named `<Type>_<name>` whose **first parameter** is `<Type>*`, `const <Type>*` or `<Type>&` (the `self`), for a class or struct the project defines, is a **member function spelled the C way** (`NetworkSingleTcp_send(NetworkSingleTcp* self, const u8* data, s32 size)` is `NetworkSingleTcp::send`). The compiler then emits the **mangled** name (`send__16NetworkSingleTcpFPCUcl`), so the map row must carry that mangling (objdiff pairs by name), the type's declaration lists the method, and call sites use `obj->name(...)`. **The fix**: declare `name` in the class, define `Type::name`, rename the map row to the compiler's mangling in the same change with `python tools/symbols/symedit.py rename` (a rename is two edits), and sweep the call sites; `this` arrives in r3 exactly like the old `self`, so it is codegen-neutral for a non-virtual method - measure it (playbook 60). `python tools/units/methodize.py <Type>` prints the plan (declaration, definition, call sites, proposed mangled name, map row) and a `symedit.py rename-batch` input. The only exemption is a **per-declaration** marker comment, `/* free: <reason> */`, on the declaration or the line above it (it must stand alone, and a file can never exempt itself, exactly as rule 7's and rule 11's per-file keys were removed): retail **C linkage is evidenced** (an unmangled name the shared dump or a caller's relocation proves) or the type is a **plain C struct from an SDK API**. Scope: `.cpp` sources and the headers they include (a C file has no members), and the type must have a definition the tool can see; a member definition `Type::name` and an already-mangled name are not findings, an `inline` free function is. **The static form** (owner, 2026-09-29): a free function named `<Type>_<name>` for such a type with **no** `Type*`/`Type&` first parameter (`GameSpyInterfaceThread_getInstance(void)`, `PatInterface_isReady(void)`, `NetworkSessionStable_setNotifyValue(u32)`) is a **static member** (`Type::name`): declare `static R name(args);` in the class, define `R Type::name(args)`, call `Type::name(...)`, and rename the map row to the same mangling as a plain member (no `this`, no `C`: `getInstance__22GameSpyInterfaceThreadFv`); the same `/* free: <reason> */` marker exempts it. The one measured carve-out: `<Type>_ctor`/`_dtor`/`_construct`/`_destruct` whose first parameter points at a *different* type is a C-style helper that initialises that type (`VEC3_ctor(MHTRI_PAD_VEC3*)`, `MTX34_ctor(MHTRI_MTX34*)`), not a static member of `<Type>`. Existing findings are grandfathered only by `land.py`'s `--diff` (an added one refuses), and the debt is filed as one `method` backlog item per file |
| 14 | **A codegen pragma lives in the TU that needs it** | a codegen `#pragma` (`peephole`, `optimization_level`, `inline`, `scheduling`, `pool`, `fp_contract`, ... - the list is `stylelint`'s `CODEGEN_PRAGMAS`) applies to the rest of every translation unit that reaches it, so one in a shared header changes code the header's author does not own (measured 2026-09-27 both ways: a unit matched only through a leaked `#pragma peephole off`, another lost rows until the pragma was restated where it was wanted). State it in the `.c`/`.cpp` that measured the dependency; `#pragma once`, `pack` and the diagnostic pragmas are not codegen pragmas. The lint numbered this check "rule 10" from 2026-09-27 until 2026-10-05, when 10 went back to the vtable rule above |
| 15 | **A comment says what is true now** | comment text carries facts about the code, never the campaign's history. **Refused** (add-only by identity, like every rule): a **stale path** - a retired `include/`, `src/auto`, `auto/<hex>_`, `proposal/`, `.pi/` or `docs/splits/phase4` path, or a retired tool's name (the list is `lib.comments.STALE_MARKERS`; a path the tree has is live, `.pi/` never is): name the live path or drop the reference; and a **wrong function-comment address**. **The address prefix is an allowed leading form**: a function comment may open `/* 0xAAAAAAAA (0xSS): <what it does> */` - `/*`, one space, `0x` and the eight hex digits of the function's `symbols.txt` address, optionally a space and its size in parentheses (`0x` hex or decimal), then `:` (the legacy `-` and `.` separators are read the same way); a comment directly above a definition that opens so must name that function's row and that row's size, or it is a finding. **Advisory** (the `r15` column of `--budget`, never refused): narrative markers (`phase 4`, `round N`, `pilot`, `lane`, `wave`, `next pass`, `header inherited`), dates, matching percentages, and a function comment that restates its own symbol. **Evidence prose lives in the unit's file header**, short and to the point (the template below this table); a function comment is one or two present-tense lines; `configure.py` carries a one-line pointer to the unit header, never the evidence itself |

**The unit file header** is the home of a unit's evidence (rule 15): one block comment at the top of the file, one line per fact, in this order, keeping only the labels that have something to say - no history, no dates, no percentages, no per-function inventory:

```c
/* <module>/<file> - <what it is, one line>.
 * RANGE. .text 0x8XXXXXXX-0x8XXXXXXX (<n> functions)[; the data sections it owns].
 * FLAGS. <its cflags group or per-library flag, and the evidence: configure.py, playbook NNN>.
 * NAMES. <name> is a GUESS (<the context it is read from>).
 * RESIDUALS. <function>: <what still differs, one line each> | none.
 * SHAPES. <a load-bearing source shape a later edit must keep>.
 */
```

**Before reporting: the review defect classes.** The read-only review found these in every batch of the 2026-10-06 wave; a lane checks its own branch for each before its final message:

- **Owners are mapped by function address**: a callee's owner is the unit whose `splits.txt` range holds that function's address, never the unit a retired range started at (`symedit.py at <addr>`, `callers.py <addr>`).
- **A 100 % row can still call the wrong thing**: a wrong callee, mangling or linkage hides behind a matching row; read the relocation targets (`python tools/objdiff/relocdiff.py --by-owner`), not the percentage.
- **An empty stub body is unwritten, not partial**: it never counts toward "bodies written" or a partial score.
- **The unwritten list is complete**: up to 20 runs as address ranges, otherwise the count, the largest and `sweepcomments.py --unit <unit>`.
- **Every flipcheck blocker is in RESIDUALS**, one line each.
- **Every GUESS name is marked** on the unit header's NAMES line.
- **A dump name is checked by address**: a signature match on a short body, or a linker-folded duplicate, is not evidence.
- **"Unowned"/"unclaimed" is verified** against `splits.txt` before it is written (`symedit.py at <addr>`, `datagap.py --unit <unit>`).
- **A function comment is third person, present tense** ("Returns the slot").

**These rules are part of phase C, not a separate chore.** The residual sweep already revisits every unit that
is not byte-identical; the conformance work (rules 1-15) rides the same pass, unit by unit, in address order.

**The `goto` backlog from the first protocol round (2026-09-23).** Four functions reached 100 % with a `goto`
shape before rule 8 existed, all in `Pl/pl_act.cpp` now: `fn_80278144` and `fn_80278310` (`goto ret1; ret0: return
0;`), `fn_8027BC48` (`Pl_motion_input_ck`, `switch` + `goto`) and `fn_80271BD4`/`fn_80271E0C` (label dispatch). They
were the lint's first reported entries: each needs a conformant shape that keeps the score, or a recorded residual
with the measurement that shows what the conformant shapes score. The unit header records the residuals and
`docs/pl.md` (Rule 8 shapes) the shapes that were tried, with their measurements.

**Enforcement is a tool, not a promise.** `tools/units/stylelint.py` (roadmap 7.21) reports each rule with
`file:line`, per unit and as a backlog, and **`land.py verify` refuses a batch that adds a violation** — a rule
enforced by remembering is not a rule. The rules apply to new work immediately; existing units are brought into
conformance as they are touched. **Rules 1, 2 and 9 are checked, not deferred**: rule 1 compares every `src/`
type definition, rule 9 rejects a mangled spelling used as a callable identifier, and rule 2 resolves every
**declaration** (the `extern` keyword *or* a plain prototype, in a `src/` file *or* an ordinary
`src/<module>/*.h` header) through `config/RMHE08/symbols.txt` + `splits.txt` to the unit that owns the
symbol - and when the map
resolves the symbol but **no** registered unit owns it, the local `extern` is itself the finding: the
declaration belongs in a band header under `src/unsplit/`. Where the registered bands bracketing an unsplit
address name different modules (a `sound` unit sits inside the `ef` band) no `<module>.h` is sound, so the
finding names the band directory rather than guess a header; a symbol the map does not contain at all, and a
duplicate map row, stay counted gaps because the map cannot judge them. Rule 7 has **no exemption and no
deferral**: every generated or address-named identifier, bare `unkNN` and generated file or directory name in
every `.c`/`.cpp`/`.h` is a finding, whoever owns the symbol, and `land.py` refuses a unit newly registered under a generated name. The **only** grandfather is the gate's `--diff`: an existing finding never blocks a
landing, while an *added* one refuses - so committed work is not revoked, and the mounted debt cannot grow. **A move is credited, a copy is
not**: an identity new to a file (rule, token, detail) is credited only when another file of the same batch *stopped*
carrying it, one credit per removal, and it is printed as `moved rule R <token>: <old file> -> <new file>` (also the
`--json` `moved` key) - never silently dropped; a copy that leaves the original intact, a removal of a different
rule, or a name that grew across the batch is still refused. A
file with no bodies is held to the rule too, and a `rule 7 deferred` comment exempts nothing. **Rule 11 is
checked the same way**: a `void *` parameter or return type is a finding by default and the only exemption is
a per-declaration `/* untyped: <reason> */` marker whose reason names which genuinely-untyped case it is - a
byte range, an opaque handle passed through, or a caller-owned payload - so `grep -rn "untyped:" src include`
is the complete list, and a file can never exempt itself. **Rule 10 is checked the same way**: `tools/units/vtableaudit.py` reports every owned-but-unemitted code-pointer run and every source write of a `+0x00` function-pointer-table pointer, `python tools/units/vtableaudit.py --diff <ref>` is the comparison the gate uses, and the row refuses a batch whose rule-10 set grows - add-only, exactly like the lint, because the tree already carries some. **Rule 12 is checked the same way**: `tools/units/stylelint.py` reports every `extern` of a data symbol that no registered `splits.txt` range covers - the unit that reads or writes the bytes claims the range and matches it - and the gate's `--diff` grandfathers the sites the tree already carries while refusing an *added* one. **Rule 15 is checked the same way**: `tools/units/stylelint.py` reports every stale path in comment text and every function comment whose `0xADDR (0xSIZE)` prefix is not the function's `symbols.txt` row, and the gate's `--diff` refuses an *added* one of either while its advisory classes (narrative markers, dates, percentages, self-names) are counted in `--budget` and never compared. **Rule 12 cannot see data nobody declares, so the gate has a second row for the same end (owner, 2026-09-29: "no data should be left behind")**: `tools/units/datagap.py --census` classifies every data address a registered unit's *target object* relocates against as claimed by this unit, claimed by another unit, or an **orphan** (no `splits.txt` range covers it), and the row "no batch unit's target object references data no claim covers" refuses a batch that ADDS an orphan pair for one of its own units or that shrinks a previously claimed byte (a recut). Pre-existing pairs are snapshotted at `record-base` and only reported; `--allow-orphan <addr>` is a recorded command-line allowance. **The row is strict for a touched unit (owner, 2026-09-29, "Yes, refuse (strict)")**: a batch that touches a unit (a unit is **touched** only by a REAL change (owner, 2026-09-29: "Only real changes"): the batch registers it, recuts it (its merged claims differ from the base's in any section) or changes its compiled object `build/RMHE08/src/<unit>.o` against the base's - section bytes/flags, defined symbols, relocation slots, or an external relocation's target, compared by address (a rename sweep whose relocations resolve to the same addresses touches nothing; `datagap.py --row <units> --touched-by` prints each unit's verdict and why). A source edit that compiles to the same object is not a touch; a unit with no object falls back to registered/recut) also refuses every *sole-owned* orphan pair of that unit, pre-existing or new - data exactly one registered object references and `callers.py` shows no unsplit reader for - unless the pair is not claimable on its own, which the tool computes and prints as `deferred: <class>` (`pool-synth`: `.sdata`/`.sdata2` our object still fills beyond the claim; `ambiguous-owner`: the text-window order of the neighbouring claims does not bracket the unit; `span-blocked`: a second range separated from the unit's main one by another owner's data, the playbook 53 cycle; `isolated-run`: a `.sdata`/`.sdata2` run with foreign neighbours and no claim of the unit's own). A deferred pair is reported, never refused - and the add-only half uses the same classification (2026-09-29): a NEW pair whose class is deferred is reported deferred, not refused, so a widened claim that re-splits the unit's target object does not turn deferred pairs into additions; everything else is refused, and `dataclaim.py --unit <unit>` prints the exact `splits.txt` edit. Each unit that still has refusable pairs is one `data-claim` backlog item (weight = pair count, §6.6). The naming and typing debt
is worked slowly through §6.6's backlog: `stylelint.py`'s rule-7 findings are one `naming` item per file, its
rule-2 findings one `band-header` item per file and its rule-11 findings one `untyped` item per file, and
the credit ratio rations new claims against them. The audit of 2026-09-22 says how much there is to bring:

| measure | count | rule |
| --- | --- | --- |
| auto-generated symbol names used in `src/` | **376** | 7 |
| pointer-arithmetic field accesses | **237** | 6 |
| fields still named `unk*` | **320** | 5 |
| struct fields that already carry an offset annotation | 52 | 4 (partial) |
| struct/class size annotations | **0** | 3 |
| types defined in more than one unit's source | **20** names, **68** extra definitions | 1 |
| `extern` declarations not in the owner's file | **126** foreign sites over 27 owner units, plus **209** unsplit sites the address band places in 6 modules | 2 |
| mangled spellings used as a callable identifier | **598** sites (**446** calls, **152** declarations), **83** names | 9 |
| `void *` parameter/return types with no `untyped` marker (measured 2026-09-27, when rule 11 was added: **5326** findings over **313** files, plus **259** `void *` locals the rule does not fire on) | **5326** | 11 |
| `extern` declarations of **data** that no registered range claims (measured 2026-09-28, when rule 12 was added: **4891** findings over **230** files - **3890** in `src/` over 188 files, **1001** in `include/` over 42; every one of them already a rule-2 unsplit site, and the commented subset is `.pi/notes/rule12-instances.md`) | **4891** | 12 |

A batch that touches a unit closes its rows in that table for that unit. The lint's backlog number is the
campaign's second burn-down (§7.11 is the first, bytes).

### 6.6 The backlog register — a credit ratio, and an evidence-based triage (owner, 2026-09-27)

**One resolved backlog item per new proposal claim.** The backlog was always filed - every lane's outbox
`config_requests` records what it found but was not allowed to change - but nothing tracked whether any of it
was ever done. `tools/units/backlog.py` is that register and `queue.py next` reads it. The first rule ("work
the backlog before any new proposal claim") was a hard gate: with 229 open items it held decompilation shut.
The owner replaced it with a **ratio** (option 3) and a **triage** of the pile (option 2).

* **What is backlog.** A `range` (a data run to claim), a `seam` (a code span whose boundary is in the wrong
  place, so the unit split needs re-drawing - its own kind, because "the cut belongs elsewhere" is a
  different ask from "claim this data"), a `shared-file` (a defect in a header a worker may not touch: a
  conflicting declaration, a `#pragma` that leaks, a wrong signature) and a `flag` (a compiler flag for a
  lib) - each open until a proposal pass re-draws it, it is fixed, or it is measured and adopted/rejected.
  The tooling/environment register `tooling.py` owns is read into the same list, never duplicated. **The
  naming debt is backlog too.** `stylelint.py`'s findings are aggregated **per file**: one `naming` item per
  file carrying rule-7 findings (`fn_XXXXXXXX` / `lbl_XXXXXXXX` / `loc_XXXXXXXX` / bare `unkNN`), one
  `band-header` item per file carrying rule-2 findings (an `extern` that belongs in the owner's header or
  `src/unsplit/`) and one `untyped` item per file carrying rule-11 findings (a `void *` parameter or
  return type with no marker). Each ask names the file, the rule and the count, and they are ordinary `open`
  items,
  so the ratio rations new claims against them with no special-casing - that is the owner's ruling's second
  half (2026-09-27): remove the exemption, but do not revoke committed progress; put the mounted debt in the
  backlog and work it slowly. **A `rename` is not backlog**: the landing applies it, so it is done when its
  batch lands, and carrying it would drown the signal.
* **A record is not a request.** Most `shared-file` entries are past-tense records of a change the branch
  already made ("Added one union member to the +0x328 union ..."); those default to `done`. Only an entry
  that states a live defect ("line 67 declares X while ... declares Y, so any TU that includes both fails
  with MWCC") defaults to `open`, and every item keeps the raw filing text so a wrong classification is
  visible. A `range` "SEAM UNPROVEN - registered whole as the brief proposed" is an admission, not a request,
  so it too is `done`.
* **Dedupe is the priority signal.** Items are keyed on `(kind, target, normalised-defect)`; the same defect
  filed by several lanes over weeks collapses into one item that records **how many lanes filed it and
  which**. The 703 requests in today's 266 outboxes are, after dedup and the record/request split, a few
  hundred distinct items - on 2026-09-27, 64 open `shared-file`, 130 open `range`, 26 open `flag` and 4 open
  tooling - with **22 open items filed by two or more independent lanes**: a signal that was thrown away
  before this register existed.
* **Ranking is filers, then an item's weight (a lint item's - `naming`/`band-header`/`untyped` - live finding
  count, so the high-traffic file leads), then recency, then `tooling.py`'s votes**, and each item shows its age. An item
  filed in an early phase may be stale because the code moved on - the register surfaces it, never drops it;
  a human or a lane parks it with `--set-status`.
* **Status is persistent and per item.** `open` / `done` / `parked` lives in `MAIN/.pi/backlog.json`
  (gitignored, like `claims.json`, so it survives a regeneration); set it with
  `python tools/units/backlog.py --set-status KEY done`, and `--check` exits 1 when the register is missing or
  stale.
* **The queue spends credits, it does not refuse outright (option 3 - a ratio, not a gate).** The register
  keeps a **credit ledger**. A resolved item (`done`) earns **1 credit**, a claim spends **`--ratio K`**
  credits (default 1 = one backlog item per claim), and the register starts with **1 credit** so the campaign
  can begin. `python tools/units/queue.py next` hands out the claim while the balance covers it and **records
  the claim in the ledger**; when it does not, it refuses and prints the balance with its derivation and the
  top item(s) with a ready-to-paste lane, exactly as the old refusal did. `--ignore-backlog` is the
  deliberate override and spends nothing; a wave (`--count N`) costs `N x ratio`. **`parked` earns no
  credit** - parking removes a ghost, it does not buy a claim; only `done` does, and both `--print` and the
  refusal say so. Earnings are **derived from the item statuses** (the count of resolved `done`) rather than
  stored as a counter, so the rule can never drift and enforcement never depends on the register file
  surviving a clean checkout; the claims handed out are the one part persisted. The guard order is branch
  (`HEAD` must be `main`), the credit balance, then unlanded. **A claim is free while the register is
  clean** (owner, 2026-09-27): the ledger rations against known backlog work, so with nothing to fix there is
  nothing to ration - charging anyway would let a clean stretch accrue negative credit and then demand
  catch-up resolutions the day items reappeared. A free claim is still counted in the ledger and shown on
  the balance line, so every claim handed out stays accounted for.
* **Triage the stale first, and never guess (option 2).** `python tools/units/backlog.py triage [--apply]`
  classifies every open item as `resolved` / `stale` / `open`, printing the check that proved each, and
  `--apply` writes `resolved` -> `done` and `stale` -> `parked`. `--apply` is idempotent and never flips a
  status a human already set. The evidence rule is per kind: a `range`/`seam` is `resolved` when its span is
  now **fully covered by a registered unit's range in `splits.txt`**; a `flag` is `resolved` when the lib's
  cflags group in `configure.py` already carries the requested flag; a `shared-file` is `stale` when its file
  no longer exists, and `resolved` when the file is present and the stated defect is **gone** (the named
  `#pragma` is no longer present, or the declaration's symbol is no longer named); a `naming`/`band-header`/
  `untyped` item is `resolved` when the file no longer carries that rule's findings, **re-linted with the same rule
  that filed it**, and carried forward from the published register so the fix can be proved after the item
  would otherwise have vanished; a `tooling` request is auto-decided only when it has a checkable artifact.
  **Anything that cannot be proved from the repository
  stays `open (no check)`** - a triage that guesses is worse than the pile it is triaging.

## 7. The tooling roadmap — build order, why, and the acceptance test

The review's accepted items, in the order that makes each one safe. Everything is `NonMatching`-safe by
construction: a tool can waste time, it cannot break the link.

| # | tool | why it exists (incident) | acceptance test | size |
| --- | --- | --- | --- | --- |
| 7.1 | `tools/units/recompile.py` (+ 7.15) | two workers measured objects the compiler never rewrote (1-second mtime granularity); a worker must not run `ninja` | delete the object, compile **without ninja**, assert the mtime moved, print section sizes and both object paths; resolve `MAIN` for the toolchain and the target object | ~60 |
| 7.2 | `tools/units/claims.py` | two independent processes must never take one unit; the branch is the lock, and a silent worker must be reclaimable (see the ack/heartbeat/timeout layer, §5.6) | `claim` takes a reusable **slot** and cuts the worktree+branch or refuses (7.26; `--no-slots` keeps the old construction path); `ack`/`status`/`timeout` cover liveness and reclaim (a timed-out branch is rescued to `refs/rescue/<slug>` first); `list` shows owner/age; `release` returns the slot and classifies the rescue ref it parks (`redundant` pruned and printed, drift reported, `unlanded`/`unknown` surfaced and kept) | ~140 |
| 7.3 | `tools/units/brief.py` | every fan-out cost a hand-written 40-line brief and each drifted | one file per unit with §5.2's six parts; idempotent | ~120 |
| 7.4 | `tools/units/handoff.py` | worker replies were inconsistently shaped; detail was lost to truncation | prints the digest skeleton; validates an outbox entry against the schema | ~60 |
| 7.5 | `tools/units/land.py` | the batch checklist was six manual commands and the regression scan was rewritten four times; a green `ok` can come from a stale link | `verify` **deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) before the run and requires both to be recreated**, then runs configure → split → report → regressions → `ok` → ledger delta (the knowledge-delta row was deleted 2026-10-05); refuses on a shared-file edit or an outbox violation; owns the baseline (7.16). The `.ninja_log` ordering idea does not work: a `NonMatching` batch never relinks, so `main.elf` never runs | ~240 |
| 7.6 | **first `Matching` flip** | `0 / 5 files linked` today; every linking question is untested and gets more expensive with every unit | one byte-identical **object** (§1) flipped alone in its commit, `ok` green, then the next | — |
| 7.7 | **done 2026-09-23** - `tools/symbols/dumpmap.py` (66 checks) | 48 367 real names/signatures sit in `DumpSymbols.zip` and using them means remembering the member, format and flags | `lookup <addr\|name>` and `join` (rename candidates, `zz_` confirmations, conflicts) against `symbols.txt` | ~80 |
| 7.8 | **done 2026-09-23** - `tools/units/dataclaim.py` (75 checks) | the riskiest edit class was reasoned out by hand three times | for each proposed run: target section size/bytes vs ours, verdict, expected effect | ~80 |
| 7.9 | **done 2026-09-23** - `tools/symbols/phantom.py` (60 checks) | five 4-byte `fn_*` were not functions (dead epilogues) and cost four functions their last 11 points | for every small unnamed `fn_*`, test whether the previous function's bytes include it → merge candidates | ~60 |
| 7.10 | **done 2026-09-23** - `prepcommit.py` warns when an improved batch carries no knowledge | the rule was enforced by me remembering | warns when a unit's score rose and no `src/` header, `docs/` file or `configure.py` comment changed in the same commit | ~40 |
| 7.11 | **done 2026-09-23** - `ledger.py` reports both burn-downs and a per-0x10000 `.text` view (8 checks) | `295 / 20 519` and `65 128 / 5 437 392` are two burn-downs and the second predicts the DOL | print both, plus a coarse per-0x10000-block view | ~40 |
| 7.12 | **done 2026-09-23** - `tools/units/sharedfiles.py` owns the writes (34 checks); `attribute.py` routes through it | CRLF vs LF silently voided two edits; an undeclared progress category failed the *next* `ninja` | one module owns the writes, asserts every anchor, is idempotent, runs `symbolpreflight`'s overlap check before a range lands | ~120 |
| 7.13 | **done 2026-09-23** - derived: 288 B (`sys_mem.cpp`) to 27436 B (`Pl/pl_act.cpp`), stated in the docstring | the two size defaults are a guess | derive them from the units we have measured (288 B … 27 KB) and say so in the docstring | ~20 |
| 7.14 | `attribute.py --max-total-bytes` | §3's 0.5 MB cap is not expressible today (only `--min-bytes`, `--max-bytes`, `--limit`) | a registration batch never exceeds the cap, and the tool says how many bytes it is about to claim | ~20 |
| 7.15 | worktree-safe measurement | a fresh worktree has no `build/` and cannot measure at all; this blocks the first 4-worker round | a worker in a worktree measures its own object against `MAIN`'s target and prints both paths | inside 7.1 |
| 7.16 | `land.py` owns the baseline | `ninja changes` compares against a `baseline.json` nobody refreshes, so a per-batch regression can hide | `verify` refreshes the baseline after a green batch and reports the batch's own delta | inside 7.5 |
| 7.17 | **done** - `tools/units/dataqueue.py` writes the queue; `dataclaim.py` (7.8) still has to consume it | data is 18.5 % of the DOL (1 233 640 B, 45 176 symbols, 720 B matched) and its queue is comments nothing reads | every data run `attribute.py` sees is in the queue with a verdict, and `land.py` reports the queue's size | ~40 |
| 7.18 | **ground-truth guard**: `prepcommit.py` refuses `config/RMHE08/build.sha1` and `config/RMHE08/config.yml`, and a tracked **`tools/git/hooks/pre-commit`** (enabled with `git config core.hooksPath tools/git/hooks` — local config, so 7.18 also states the checks that do *not* depend on a hook: `prepcommit.py`'s and `land.py`'s path refusals) refuses `orig/**` and `build/` on **any** commit path | `prepcommit.classify('config/RMHE08/build.sha1')` returns `stage` today: a worker or I could rewrite the DOL's expected hash and commit it, after which green `ok` means nothing | a staged `build.sha1` is refused, and the hash is checked against `orig/RMHE08/sys/main.dol`'s own sha1 | ~40 |
| 7.19 | link-order audit for flips | 13 584 objects link in 66-131 s now; with hundreds of `Matching` units the order, pool placement and symbol collisions become real | after a batch of flips, compare `main.MAP`'s section/symbol order against the original and diff the DOL | ~60 |
| 7.20 | transactional `attribute.py apply` | `apply` writes `splits.txt` first and can leave a half-registration; `plan` can propose overlapping data spans | no proposal overlaps a claimed range, both shared files are written via temp+rename, and a failure restores them | ~40 |
| 7.21 | `tools/units/stylelint.py` - the rules of §6.5 | 19 units carry 376 auto-generated names, 237 pointer-arithmetic field accesses, 320 `unk*` fields, **0** struct-size annotations, and now a `goto` backlog in `Pl/pl_act`/`Pl/pl_skill`; a rule enforced by remembering is not a rule | flags each rule as `file:line` (including `\bgoto\b` and an unmarked `void *` parameter or return), reports a per-unit backlog (`--budget`), and `land.py` refuses a batch that **adds** a violation | ~140 |
| 7.22 | **done** - `tools/units/datagap.py` compares a unit's **target object's section sizes with ours** (`build/RMHE08/obj/*.o` vs `build/RMHE08/src/*.o`) | a unit can read 100 % fuzzy and still emit data its original TU never had: our source defines a pooled constant or table where retail referenced a map symbol (the finding, 2026-09-26: the gap is **ours-extra** - `.sdata2 8B` on `Pl/fn_8026FFBC`, `.data 120B`/`548B` on `ef/eft002`/`ef/fn_800FD864`); the unit score hides it, so a flip looks blocked for no visible reason | `--flip-blockers` lists the units whose code already matches (>= 99 %) and whose only defect is data: **21** of 223 on 2026-09-26; the fix is playbook 29 (reference the map's symbol, never define it), since the target object has no such section to claim; 10 selftest checks | ~60 |
| 7.23 | **done** - `queue.py`'s **unlanded-branch guard**: `next` refuses to claim while any local branch holds content `main` lacks | the 2026-09-26 branch audit found a whole registered unit (`menu/fn_802E4978.cpp`, 511 + 186 + 115 lines) that no landing had ever taken, sitting on a worker branch while new lanes were being launched; finished work must not queue behind a new unit | `strictly_newer` (main's copy a strict subset - narrow on purpose, so the stale pads and comment wording a landed branch leaves behind do not block), `unlanded_branches`/`unlanded_error` wired into both spawn paths, `list` reports the state, `--allow-unlanded <branch>` parks one on purpose; 8 selftest checks | ~70 |
| 7.24 | **done** - a `decompiler` lane matches **data** as well as code: `.claude/agents/decompiler.md` gains a Data section and every brief gains 5d, and the old propose-do-not-claim line is inverted | objdiff's unit score does not count a wrong data section, so a lane could hand over a unit whose code matched and whose `.data`/`.sdata2` was ours-extra - the whole 20-unit flip-blocker list came from that blind spot, and no lane was ever asked about it | the lane measures with `datagap.py --unit`, claims what its object emits (private pool entries, unclaimed `.data`/`.ctors` ranges; never a shared entry - playbook 58), drops definitions it should only declare (playbook 29), and reports sections and bytes before/after; a code-complete unit with a claimable data gap is finished in the same lane and flipped | ~40 |
| 7.25 | **done** - the C++ **class shape** rule: `.claude/agents/decompiler.md` and every brief (section 5c) say a range the evidence calls a class is written as a class with member functions and real virtuals, not as a C struct plus free functions taking `self` | the owner caught a landed unit reconstructing a class that way - 403 `self->` uses in `Network/fn_8041A87C.cpp` while its own header records the target's string pool spelling `NetworkGameSpyInterface::`/`NetworkPeerGameSpy::`; MWCC only emits retail's canonical `lwz r12,0(r3)`/`lwz r12,<slot>` dispatch for a genuine `virtual`, so the shape is part of the match | both renderers carry 5c; a `fixer` lane reconverts the landed unit and records any function where the struct form measured better; 153 + 117 selftest checks green | ~40 |
| 7.26 | **done** - `tools/units/slots.py`: a fixed pool of reusable lane directories at stable paths, one lock each, a verified-and-fail-closed reset, wired into the claim path | per-claim construction costs per lane, its teardown cannot remove a worktree holding the `tools/m2c` submodule (the careless 2026-09-24 teardown destroyed a lane's branch), and the seeder rewrites `.ninja_deps`' absolute paths every time because each path is new; a reused slot also carries the previous round's build state, and a stale tree cost a full `rm -rf build/RMHE08` rebuild | `init`/`acquire`/`release`/`status`/`verify`; a slot holds a directory, never a branch (`acquire` cuts a fresh `checkout -B worker/<slug> <tip>`); a free slot is the concurrency cap (`queue.py next` refuses at six); the kept build tree is validated against MAIN's current map/DOL with the seeder's own guard plus a `report.json` comparison and re-seeded if it cannot be proven current; one lock per slot at `MAIN/.pi/slots/<n>.json`, stale locks reclaimed; 62 selftest checks | ~340 |
| 7.27 | **done** - `tools/units/rescue.py audit`: the `refs/rescue/*` safety net gets an audit, and `land.py resolve`'s `land/resolve-*` helper branch gets a teardown | 193 rescue refs had accumulated and nothing had ever looked at them; `land.py resolve` also left its helper branch (and its scratch worktree) behind, so a `land/*` ref outlived the batch it was made for (two were found from 2026-09-26) | every ref reports its date/subject, the unit(s) it registers (**the registration diff against the merge-base with `main`** - a whole-file name match against `main` matches every unit in the file), whether each is registered on `main` today and how the touched paths differ; verdicts `redundant`/`landed-with-drift`/`unlanded`/`unknown`; `--prune` deletes only `redundant` (printing each), is strictly read-only without it, and never touches drift/unlanded/unknown; a landing deletes its own resolve helper, visibly, only when the tip is contained by the branch or `main`, and refuses loudly otherwise; `claims.py release` runs the same classification on the ref it parks at teardown - `redundant` pruned and printed, `landed-with-drift` reported, `unlanded`/`unknown` surfaced with the ref, its unit(s) and its date and kept, never a gate; 28 checks in `rescue.py` + 36 in `claims.py` | ~180 |
| 7.28 | **done** - the per-lane measurement loop is fixed: `recompile.py`/`measure.py` run from git-bash, and no score can come from a stale object or the wrong tree | `absolutize()` rewrote cmd's `/c` to `C:\c` (`os.path.join(main, "/c")` is `C:/c`, and that drive-root artifact exists on this host), so the child was an *interactive* `cmd`, no object was written, and 31 lanes filed "make this work" in the register. Two siblings of the same failure: a hand-built scorer measured a STALE object twice when a compile failed and two invented "improvements" were reported as real, and the measuring tools rooted at their own file location / the first `git worktree list` entry, so a score run from a slot could print MAIN's number (`NetworkWiiMediator/dispatchReflectEvent` 0.91743 vs 100.0) | `absolutize()` never rewrites a `-`/`/` switch; every measurement path deletes the object before compiling and `object_is_fresh()` refuses an object older than its source; both chained post-processors (`objalign.py`, `objextab.py`) are retargeted to this tree's object; `unitutil.repo_root()` resolves the caller's `git rev-parse --show-toplevel` and `recompile.main_root()` the git common dir (the gate still runs from MAIN, unchanged); the hostile selftests break a source and assert `FAILED` with no score, and score a throwaway worktree and assert its number (100.00000, not MAIN's 99.80576) | ~80 |
| 7.29 | **done** - `measure.py --baseline`/`--against-main`/`--save`: every symbol of a unit with its before/after delta in **one call** | `symdiff.py -u <unit>` lists per-symbol scores but needs a re-invocation per symbol to see a diff, so a body lane spent minutes per iteration; the lanes' scratch scorers (`build/probe/score.py`) each re-derived "diff the probe's rows against the committed report" | one compile and one `report generate` score all N symbols (~0.3 s on `Camellia/camellia`); `--against-main` diffs each row against MAIN's `build/RMHE08/report.json`, `--baseline <file>` against a saved report or a `--save` file, `--save` writes this run for the next; a moved-down row is reported and the exit code is non-zero; the baseline selftest pins both shapes, the refusal of a unit the file lacks, and the delta direction | ~60 |
| 7.30 | **done 2026-09-27** - `tools/selftest.py`: one runner for every tool selftest, and the gate's `all tool selftests pass` row (measured ~29 s) | `measure_selftest.py` was red for weeks while 31 lanes filed "`recompile.py` is broken" - the tool's own test said so and nothing ran it. A selftest nobody runs is decoration, and the land gate ran only `land.py --selftest`, never the suite | discovers both shapes (`tools/**/*_selftest.py` and every tool exposing `--selftest`), **dedupes** a wrapper pair to one entry per tested tool (a genuine complementary pair, e.g. `ledger.py`/`ledger_selftest.py`, keeps both); runs in parallel with a per-test timeout that kills the whole process tree; one pass/fail/checks/duration table; **`git status --porcelain` identical before and after** (a selftest that writes into the repo is named); a **park list** (`tools/selftests-known-failures.json`, reason + date) so `green except N parked` never lets one old red hide a new one (a park that now passes is `STALE` and fails); `--json`, `--changed [REF]`, `--list`, `--no-dedupe`, and a `--selftest` that the suite itself runs; 51 entries / 3,345 checks / 29 s | ~260 |
| 7.31 | **done** - slot hygiene: `status` reads the worktree (not the lock file), `acquire` falls through, the spawn line carries the slot's `cwd`, every brief opens with a "your tree" self-check, and `land` pre-flights MAIN for foreign paths | a lane was launched with its cwd set to **MAIN**, so it cloned its upstream into `.tmp-mwcc/` inside the repository root and the next landing was refused with "paths outside the batch appeared during the build" - collateral damage to a *different* lane's batch, and only after the fact. `slots.py status` called that same slot `free` while its worktree held `worker/rule10-fix-14f8` (it believed the lock file), so a cap reasoning that trusted it was wrong; `acquire` refused instead of trying the next slot; nothing handed the lane a cwd | `status` derives **in use** from the worktree branch *and* the atomic `.used` sentinel (`acquire` marks it `O_EXCL`, `release` clears it, a crash remnant on a detached tree is reclaimable) - the two readings disagreed and both are kept; `acquire` skips an occupied slot and takes the next genuinely free one (an explicit `--slot N` still refuses an unlanded branch); `queue.py next`/`claims.py claim` print the paste-ready line with **the slot's path as `cwd`** and refuse a `cwd` that resolves to MAIN; every brief (unit and proposal) gains a "your tree" block with the `git rev-parse --show-toplevel` must-equal self-check and "STOP and report" if it does not; `land` reports foreign paths **before** the build, naming lane scratch (`.tmp-*`, `.ws-*`, `upstream/`) as the likely cause; selftests 62->78 (slots), 137->139 (queue), 169->172 (brief), 352->362 (land), 221 (claims, unchanged); the hostile case - a checked-out branch with no lock file and no marker - is reported in use | ~150 |
| 7.32 | **done 2026-09-28** - `tools/mwcc-debugger/`: the compiler's own IR | a residual that is **one instruction** and that neither a source shape nor a flag explains was unfalsifiable from the source side - "the allocator differs" is a description, not an attribution | drives our own `mwcceppc.exe` under gdb and dumps the PCode stream after each optimizer pass for the unit's **real** command line; `locate/verify_pcode.py` is the health check (`MATCH` on the final dump against the object; `PASS-DELTA` plus the first pass reaching the object on an early one); the profile carries the procedure, and `.pi/notes/mwcc-debugger-gaps.md` is the feedback channel | ~200 |
| 7.33 | **done 2026-09-28** - `tools/mwlink_debugger.py`: the linker's own run | a flip whose DOL hash moved, an `undefined:` at link time, a row-46 `.ctors`/`.dtors` ordering question, a row-36 trailing function the link drops or a row-55 alignment question all read as link mysteries, and "the linker reordered something" is not an attribution | derives every table from the linker binary instead of transcribing it (the `LoadStringA` message catalogue, 1248 phase anchors, the input-file record, the `*fill*` alignment site, the `.ctors`/`.dtors` priority list) and answers from a real link: `trace <unit>` (kept? where each section landed, relocations applied), `diagnose` (phase stream + catalogue ids), `verify` (map vs ELF, `--identity`), `align --unit`, `order`, `phases --prove`; the link step is `build.ninja`'s **Wii/1.0** (compiles are Wii/1.3) and every link is redirected into `build/scratch/` - **`main.elf` is never written**; 260/260 traced inputs and 13/13 sections MATCH, `records --prove` 2296/2296 names; `.pi/notes/mwlink-debugger-gaps.md` is the feedback channel | ~300 |
| 7.34 | **done 2026-09-28** - `tools/units/callers.py`: the whole-DOL caller index | `callees.py` answers "what does this unit call"; the inverse - "who calls this function, who reads this data" - had **no tool at all**, so it was answered with a hand-written grep over the 89 MB asm dump, which is **stale**: a `bl` whose callee was renamed still prints the old label, so grepping for the new name finds nothing | `python tools/units/callers.py <address\|name>` builds an **address-keyed** graph (a `bl`/`b` target decoded from the instruction's own displacement, names resolved per run from the current map) and reports `call`/`branch`/`addr`/`read`/`write`/`pointer` sites plus the inferred `r3` argument; cached in `build/tmp/callers/graph.json` and rebuilt when the dump changes - 245 258 references over 54 256 target addresses, ~1.3 s a query; 117 selftest checks, no dump needed | ~180 |

Rules for building them: **a tool that writes shared files goes through 7.12**; **every tool that mutates state
has a selftest** (`ledger_selftest.py`, `attribute_selftest.py`, `m2cinput_selftest.py` are the pattern); and **a
tool is not done until `land.py` calls it or the plan says who runs it**.

**Status (2026-09-23): the first five items are built, tested and committed.**

| item | state | evidence |
| --- | --- | --- |
| 7.18 ground-truth guard | **done** | `prepcommit.py` refuses `build.sha1`/`config.yml` (except `config.yml`'s `block_relocations`/`add_relocations` keys, 2026-10-03) and cross-checks the DOL hash; tracked `tools/git/hooks/pre-commit`; `tools/git/guard_selftest.py` (33 checks); both refusal paths exercised by hand |
| 7.1 + 7.15 `recompile.py` | **done** | compiles without ninja from a worktree that has **no `build/`**, asserts the mtime moved, prints section sizes, measures with `objdiff-cli -1/-2`; verified in MAIN (94.78261 %) and in a scratch worktree (identical, MAIN's object untouched) |
| 7.2 `claims.py` | **done** | branch-as-lock (`worker/<slug>`), registry in `MAIN/.pi/claims.json`, `list`/`release`/`expire`; refuses a second claim; 12 checks |
| 7.3 `brief.py` | **done** | six parts, rules extracted verbatim from §6.5/§8, inventory parsed in-process; for `Pl/pl_act`: 115 symbols, 5 below the bar, every score real |
| 7.4 `handoff.py` | **done** | outbox schema + validation (13 checks): unowned symbols, out-of-range percentages, a rename without evidence, a missing `measured_with` are all refused |
| 7.5 + 7.16 `land.py` | **done (build path dry-run only)** | 13 checks; the gate refuses on a moved base, a shared-file edit, a missing/invalid outbox, a regression, and requires `ok` to be recreated by *this* run; baseline refresh |
| 7.17 data queue / 7.20 transactional `apply` / 7.21 `stylelint.py` | **done 2026-09-23** | `dataqueue.py` writes the queue `brief.py` reads (3728 runs / 2.27 MB, 42 checks); `attribute.py` has `--max-total-bytes` (0.5 MB cap) and a transactional `apply` that restores byte-exactly on failure (103 checks); `stylelint.py` reports §6.5 rules 1-9 and 11 as `file:line` and the gate refuses a batch that adds one (218 checks) |

**Flip campaign (7.6), round 1 2026-09-23.** Six objects are flipped and committed, each alone in its commit -
`memset` (3e5a072), `NetworkWiiMediator` (6e0ed8c), `OSAlarm` (0806eae), `memcpy` (eadd4bd), `lobby_scene`
(a247195), `global_destructor_chain` (1e6f93c) - with `main.dol` still
`bf4850739478caaedfe675949eb7c28595a7fde9`. Two further candidates failed, and each failure wrote a rule the
proof has to contain (detail: `.pi/notes/flip-round-1.md`):

1. **Alignment is part of the proof.** `NetworkWiiMediator` and `OSAlarm` matched the target in every section
   size and every byte, and still broke the DOL: their `.text` was `align 2**4` (from `cflags_base`'s `-O4,p`
   implying `-func_align 16`) where retail's is `2**2`, so the linker rounded the object's start to the next
   16-byte boundary - `fn_80413F3C` moved to `0x80413F40`, every later symbol shifted by 4, 1 223 723 DOL bytes
   differed. Fixed per lib (`cflags_network`, `cflags_os`, commit b523f3d). Every retail object in the project
   is `.text`/`.init align 2**2`, so 4-byte alignment is the default expectation - and the two units that
   genuinely want 16-byte padding (`__start`, `__ppc_eabi_init`) want it from their own section pragmas.
2. **`extab`, `extabindex`, `.ctors$NN` and `.dtors$NN` are not an obstacle - they are part of the unit.**
   These sections are *compiler-generated*: their content is a side effect of the unit's code (exception tables
   for the try/catch and destructor shapes, reference words for the static initialisers), which is why
   `__init_cpp_exceptions.cpp` declares its three words through `#pragma section const_type` and carries a
   `static int fragmentID = -4` for `.sdata`. A matched unit therefore produces the matching fragment, and
   `flipcheck.py` verifies those fragments byte-for-byte like any other section - contributing one is not a
   reason to withhold a flip. **The earlier claim here (that such units cannot be flipped, and that the linker's
   object order was the cause) was wrong and is retracted.** dtk substitutes a `Matching` object *in place*:
   `build.ninja`'s input list has `src/lobby/lobby_scene.o` exactly where the target object was, and
   `src/Runtime.PPCEABI.H/memcpy.o`/`memset.o` adjacent to each other as before.

   **Resolved - and the honest account matters more than a rule.** The blocker does not reproduce. With a
   freshly compiled object the flipped link holds 92 `.ctors` and 3 `.dtors` words - the green counts, with
   `__init_cpp_exceptions` at `.ctors[0]` and `__fini_cpp_exceptions` at `.dtors[1]` - and the unit passes all
   fifteen gate checks (`2e15e1e`). Earlier measurements in this same session gave 91/2, and the investigation's
   suggested settling experiment (patch `SHF_INFO_LINK` 0x40 onto our `.rela.*` headers) came back
   **inconclusive**: the *unpatched* baseline already produced 92/3, so the flag is not the rule.

   What changed in between is a forced **re-split** - `build/RMHE08/config.json` and `dep` deleted, so dtk
   regenerated every synthesised target object. That is the playbook-row-31 trap the campaign has hit before: a
   target object built before a map edit keeps its old symbol spelling or layout, and the link then mixes it with
   ours. **Lesson for the flip campaign: re-split after any map change before judging a flip** - which also
   explains why `land.py`'s own gate (which re-splits) never reproduced the failure while the manual experiments
   (which did not) did.

   `sys_mem` is a **separate, still-open case**, not this one: it is byte-identical in every section including
   `extab`/`extabindex`, it is the only object in the link list for its unit, and flipping it still scrambles the
   DOL (5 536 639 differing bytes, `_eti_init_info` 0x8003F1C8 -> 0x8003F17C). Note that `g3d_resanmamblight`
   flips green while contributing `extab`/`extabindex` from a `src/` object, so "a concatenated section" is not
   the discriminator either - the investigation proved the `$NN`-versus-unsuffixed framing wrong and found
   `mwldeppc`'s built-in ctor/dtor table path in the binary, which is where to look next.

**`__ppc_eabi_init` is flipped** (`1a517a8`, with the per-file `-func_align 16` lib split in `895e72a`) - the
first flipped unit whose code lives in `.init` and the first needing a flag rather than a per-lib setting.
**`__start` is not an alignment problem**: 16 gives `.init` 0x2F8 against the retail 0x300 and 32 overshoots to
0x348, so the 8 missing bytes are trailing pad, and the flip fails at the link with `undefined: 'Debug_BBA'`.
The map has one entry for it - `scope:local`, `.sbss:0x807953C8`, used only by `__set_debug_bba`/`__get_debug_bba`
in this same file - and a local symbol cannot be referenced across objects, so it belongs to `__start.c` itself;
it is currently attributed to an auto `.sbss` blob, whose copy dtk spells `Debug_BBA_807953C8`. The fix is a
`splits.txt` claim plus a definition in the source: the second flip this round blocked by **attribution** rather
than by codegen, after the `.ctors` words.

That one is **solved**: the byte cannot be claimed (a `.sbss` split must be 16-byte aligned and the block also
holds `PowerCallback`/`ResetCallback`), and what actually broke was the *name* - `scope:local` makes the split
spell a local symbol `<name>_<address>`, so the scaffold defined `Debug_BBA_807953C8` while the two SDA21
relocations in `__start.o` named a symbol nothing defined. Declaring `Debug_BBA` **`scope:global`** in the map
(b08576d) keeps the plain name on both sides: the source goes back to the SDK's name, the relocations of our
object and the target's are identical, and the unit now reports **100.00 % (736/736 bytes)** - its recorded
residual is gone. **Rule: when a `scope:local` symbol has to be visible by name to another object, declare it
`scope:global` in `symbols.txt`** (or reference dtk's `<name>_<address>` spelling); `scope:local` is what makes
dtk disambiguate, and the map is a build input, not a claim about the original's symbol table.
   object covering those bytes, and object reordering (dtk substitutes in place).

`tools/units/flipcheck.py` checks all three conditions per unit (claim vs emitted sections, sizes/alignment,
and the bytes against the target object) and reports **7 of 19 ready**. Its one false positive is
`Runtime.PPCEABI.H/__init_cpp_exceptions`, which passes all three and still fails: `dtk dol diff` reports
`__init_cpp_exceptions_reference` expected at `0x8056F2C0` - the *first* `.ctors` entry - and ours holds a
different value there. **7.19 has now answered this**: the unit's code links at exactly the right address
(`objdump -t main.elf` shows `__init_cpp_exceptions` at 0x80457420) and its `.ctors$10` symbol
`__init_cpp_exceptions_reference` is *absent from the symbol table*, while `.ctors[0]` holds `fn_80046B94` and
`.dtors[1]` is 0 where retail has `__fini_cpp_exceptions`. The merged `.ctors`/`.dtors` sections concatenate
every object's fragment in **link command-line order**, so substituting our object for the target moves it in
that list and every later fragment shifts one slot - the table's contents change while its address and size do
not. **Consequence for the campaign: a flip is only safe for a unit that contributes no order-sensitive
fragment** (`.ctors$NN`/`.dtors$NN`); all six successful flips qualify, and `__init_cpp_exceptions` is the first
candidate that does not. The fix is in the link step (substitute in place, or re-order the merged fragments
after the link), not in the source - our object matches the target in sections, symbols, relocations and bytes.

**End-to-end dry round (2026-09-23).** claim → brief → measure inside the worktree → outbox → gate: `claims.py
claim` → `brief.py` → (in the worktree) `recompile.py --measure` → `handoff.py --check` → `land.py verify
--dry-run` exited **0** with all five cheap checks passing, then the claim was released. The heavy path
(configure → split → report → regressions → `ok` → baseline) has only been exercised as a *plan*; the first real
land is what runs it.

**Ordering constraint.** **7.18 comes first** - the ground-truth guard exists before any worker does, because it is the
only hole that can invalidate the whole campaign's evidence. Then 7.1 + 7.15 and 7.2 (a worker that cannot
measure, or two workers on one unit, is not a round), then 7.3 + 7.4, then 7.5 - the gate every later batch runs
through. 7.6 (the first flip) may happen before any of them: it needs no worker. — it needs no worker.

**The asm dump is a read-only input for attribution, not a build step.** `config.yml` sets `write_asm: false`,
so `build/RMHE08/asm/` is produced on demand (`python tools/splits/dump_asm.py`, 200–400 s) and stamped with the
sha1 of `symbols.txt`/`splits.txt`/the DOL. `tudiscover` reads it, so before an attribution batch the
orchestrator runs `python tools/splits/dump_asm.py --check` and regenerates when stale — a stale dump is
*silent* and zeroes a codegen fingerprint, which is why the stamp exists.

## 8. Invariants — the rules that do not change

**8.1 Repository rules (non-negotiable).** Never modify `orig/RMHE08/**`. **Never edit `config/RMHE08/build.sha1` or
`config/RMHE08/config.yml`** - they are the ground truth the campaign is measured against, and a rewritten hash
would make every later `ok` meaningless (7.18 enforces it). The one exception is `config.yml`'s relocation-analysis
keys (`block_relocations`, `add_relocations`) and comments, committable once measured (owner ruling 2026-10-03; the
hook and `prepcommit.py` refuse every other key). Never commit build output, original files or
scratch. Never change compiler flags/`mw_version`/tool tags to make something build - a flag change needs
instruction-level evidence and is called out explicitly. `Matching` only when byte-identical. Do not rename or
delete a map symbol unless nothing else depends on it. Never rewrite history, never push. Never print
`symbols.txt`.

**8.2 Flag policy.** A scoped `#pragma` in the source is for **one unit's** deviation; a **lib** flag is for when
**two or more units of that lib agree** on the real command line (that is how `Pl`'s four flags were settled:
three units, independently measured). Evidence lives in a comment next to the flag in `configure.py`. Every flag
change is re-measured across the whole lib (function alignment moves every symbol after the first).

**A flags audit is in flight (2026-09-28), and this policy is what it measures - the plan asserts no default
set.** The owner's hypothesis is that the per-file codegen pragmas in `src/` are a *symptom* of a mis-set lib
default, so a standard default set should fall out of the units that already match; that is **being measured,
not decided**. Its report is `.pi/notes/flags-audit-645d.md` (an unlanded lane when this was written): measured
end-to-end on **`Network`** and **`lobby`**, whose lib default (`cflags_base`'s `-Cpp_exceptions off`)
disagrees with 7/7 and 19/20 of their own targets; **probed and rejected** for `Runtime.PPCEABI.H`, whose probe
moved `main.dol`; and `-Cpp_exceptions off` is *correct* for the SDK/runtime libs, whose matching units carry
no `extab`. The honest reading today is that only **`#pragma exceptions`** is a candidate for a lib default:
`#pragma peephole off` and `#pragma fp_contract off` are deliberate per-file levers with their own playbook
rows (39 and 40), and their lib-wide alternatives are already registered backlog items. Census in this tree,
measured 2026-09-28: **24** files carry `#pragma exceptions`, **148** `#pragma peephole`, **31**
`#pragma fp_contract` (the audit's own starting census was 24 / 147 / 31). The default set is the audit's
*output*, not this plan's premise.

**8.3 Seam policy.** A boundary may be claimed only from `tudiscover`'s evidence kinds. Anything else — a shared
static, a call pattern — is a *hint* for the unit header. An unproven seam is allowed (the extent settles as its
functions match) and its header says so.

**8.4 Data policy.** Measure before *and* after every data claim; never claim a range our object does not emit;
never claim linker-generated data.

**8.5 Measurement policy.** The object measured must come from the real command line; a stale `report.json` lies
(regenerate, 1.5–3.5 s); frame size is not progress; a per-symbol score is the only score. **And a score must
come from an object this run wrote, in this tree**: `recompile.py`/`measure.py` delete the object before
compiling and refuse one older than its source, and the measuring tools root at the invocation's
`git rev-parse --show-toplevel` - a score printed from inside a worktree is that worktree's, never MAIN's
(7.28). When in doubt, the whole-tree check is one command and it is in §11.

**8.6 Blast radius.** Everything lands `NonMatching`, so a bad batch cannot break the link — but it can break the
*next* session's ability to measure. A half-registered unit, a range with no `configure.py` entry, or a report
that was not regenerated is what actually stops the loop.

## 9. The knowledge delta — the compounding asset, with a machine check

Every batch owes a delta, and it has exactly four homes:

| what was learned | where it goes | who reads it next |
| --- | --- | --- |
| a matching idea (code shape, flag, allocator rule) | an idea file `docs/matching/NNN-slug.md` (create it with `python tools/agents/ideas.py new`; the `index.md` row is generated by `sync_playbook_index.py`, and `ideas.py check` is the gate) | every future session and the `mwcc-unit-matching` skill |
| a workflow/tool fact (a gate, an order, a trap) | the skill that owns that step | every agent running that step, workers included |
| a unit's residual | that unit's file header | whoever touches that unit next |
| a campaign mechanic (registration, batching, orchestration) | this file | the next batch's orchestrator |

Three rules make it real: **a batch is not closed until its delta is written** (write it before reading the
staged diff — that is when the numbers are in hand); **the generated skill references must be in sync**
(`sync_reference.py --check` in the same commit, or the knowledge exists and no agent will load it — it once sat
55 lines behind); and **a batch that found nothing new says so** ("nothing new" is a legitimate delta; silence is
indistinguishable from not having looked).

From 7.10 this is a machine check, not a good intention: `prepcommit.py` warns when a unit improved and no
document changed. `land.py verify` no longer runs it: its knowledge-delta row (row 20) was deleted on 2026-10-05,
because a replay of 572 gated commits found it never fired; the gate still reports the ledger delta.

## 10. Escalation, grants and the decided queue

**Standing grants (owner, 2026-09-21).** The orchestrator runs the campaign without gates: it decides the batch,
the registrations, the flags, the queue and the commits, and does not ask per item. It interrupts the owner only
for a **pressing** issue: (1) the linked DOL is at risk outside the `NonMatching`-safe path; (2) loss or
destruction — deleting work, rewriting history, pushing, an unrevertible bulk edit; (3) information only the
owner has; (4) a premise change (the bar, the scope, the vendor files, an `CLAUDE.md` rule); (5) the
environment. Everything else is the campaign's own business, recorded rather than asked about.

**Decided queue — do not re-litigate:**

1. `memcpy.c` and `memset.c` stay separate (one file would let MWCC inline `__fill_mem` into `memset` and cost a
   100 % symbol).
2. The TRK interrupt-vector table (0x80004380–0x800062B4) was claimed and matched on 2026-09-28 as two
units (404 B + 7,596 B, both 100 %, DOL unchanged) - the owner asked for the last unclaimed `.init` bytes to
be completed.  This entry is kept as the record of the *old* decision and of why it looked final (zero
relocations, not expressible in C
   without hand-written assembly) — under §1's completion test it is simply a proposal no worker ever takes, not
   a blocker.
3. `-func_align 4` for `Runtime.PPCEABI.H` — **landed** (`cflags_ppceabi`), three independent witnesses.
4. `pl_act`'s `.sdata2` run (0x8079A080–0x8079A114) and `Gecko_ExceptionPPC.cp`'s `.bss fragmentinfo`
   (0x806F4B48) stay **unclaimed**: our objects do not emit those sections, so the claim pairs a section against
   nothing and loses score. The win came from `extern` declarations used as load operands (playbook 29).
5. `-sdata 0` for `Pl/pl_skill.cpp` is **ruled out** (purely harmful once the real fix landed; the absolute
   access is a property of that symbol's section, not of the unit's addressing mode).
6. `pl_master`'s `fn_8026F908` (9 bytes) is an allocator colouring tie-break — recorded, not chased.
7. Artifacts for the owner are **light-theme** (owner preference, 2026-09-22).

`.pi/state.md` is a **pointer to this file**, not a second queue: where the two disagree, this
file wins and the state file is corrected in the same commit.

**Escalation queue (open):** a re-attribution the `preflight` verdict calls `approve`; a `never touch` owner; a
policy question; a batch that would exceed the standing commit approval. Keep working on the rest and hand the
queue over as a list with numbers.

## 11. Verification, handover, and the stop conditions

**`land.py verify` is the land-batch gate** (`docs/pipeline.md` §4 lists its rows): `python configure.py`
→ `ninja` → `ninja build/RMHE08/report.json` → the regression scan (`ninja changes`, no measure down) →
`ninja build/RMHE08/ok` → `ledger.py` (the ledger delta, reported).  On top of that it runs the **cheap rows**
before the build: the ground-truth hash, the batch base, the scope/outbox/branch guards, the style lint, and
**`all tool selftests pass`** (`tools/selftest.py`, parked pre-existing failures aside; `--no-selftests` is the
documented fast path).

It must prove the `ok` it reads is *this* run's: `land.py` deletes `build/RMHE08/ok` (and `main.elf` when the
batch flips an object) **before** it starts and requires both to be recreated; it also checks every command's
exit code, `configure.py`'s included — a failed `configure.py` otherwise leaves a stale `build.ninja` and every
later number is meaningless. The `.ninja_log` ordering idea is **not** a substitute: a `NonMatching` batch never
relinks, so `main.elf` never runs and `ok` is the only edge that re-validates. Ordering still matters for
freshness (`ninja -t query build/RMHE08/main.elf` shows the `order_only` dependency on `config.json`).

**The whole-tree check is one command, and a lane can run it verbatim before it reports:**

```sh
ninja changes      # every unit whose score moved vs the baseline
```

**A non-empty diff is a row moving in a unit the batch did not touch.** A declaration change is part of codegen
(playbook 60): changing which header declares a callee shifts the TU's anonymous-pool ordering and can move a
row in a unit that has nothing to do with the symbol being renamed, and only a whole-tree diff shows it. A
folded-declaration batch that followed the net-zero rule moved 0 of 2,797 units; the two regressions the
type-fix lane caught were rows in unrelated units. **Investigate every line; never wave a non-empty diff
through.**

**And the style lint is part of the same gate** (`python tools/units/stylelint.py --diff <base>`): a batch may not
*add* a violation of §6.5, and it must not leave a violation in a unit it touched.

**The tool's own selftests are the same gate** (`tools/selftest.py`, roadmap 7.30): `measure_selftest.py` was red
for weeks while **31 lanes filed "`recompile.py` is broken"** - the tool's own test said so and nothing ran it. A
selftest nobody runs is decoration. One command runs them all (both shapes: `tools/**/*_selftest.py` and every
tool exposing `--selftest`), deduped to one entry per tested tool when one half is a wrapper of the other, in
parallel with a per-test timeout, and it **guards the tree**: `git status --porcelain` must be identical before
and after the run, so a selftest that writes into the repository is named, not tolerated. A *pre-existing*
failure is **parked** in `tools/selftests-known-failures.json` with a reason and a date; the runner reports
`green except N parked`, and a park whose test now passes is reported `STALE` and fails, so one old red cannot
hide every new one. The lane's fast loop and the recipe:

```sh
python tools/selftest.py --changed     # only the selftests of the tools THIS diff touches (fast)
python tools/selftest.py               # the whole suite
python tools/selftest.py --json        # machine-readable inventory (pass/fail/checks/duration per tool)
```

`land.py verify --no-selftests` skips the row outright if a landing must be fast.

**Handover.** Before a compaction or the end of a session: `.pi/state.md` says which batch is open, the
ledger is the state, and anything worth keeping is in `docs/`, a skill, `CLAUDE.md` or a unit header. A finding
that lives in a reply is lost — that has already happened once here.

**Stop conditions** — the orchestrator does not stop because a batch ended. It stops when (1) the campaign target
it was given is closed (report the delta and the new totals); (2) a gate blocks it — an escalation item, a
regression it cannot fix, or a check that keeps failing (three attempts on one symptom is the limit: report the
attempts, not a fourth); (3) its working budget runs out — and then only at a verified boundary: committed batch,
`splits.txt` through a re-split and a report, `ok` green, block updated.

## 12. The next batches, concretely

### The parked list: units that cannot be flipped yet (owner's call, 2026-09-23)

A unit may only be flipped to `Object(Matching, ...)` when its `.text` is byte-identical to the target object's
(rule 4, enforced by the flip gate). These are the registered units that are *measured* but not there yet. They are
**parked, not abandoned**: annotate and move on, and come back when there is new information (a flag, a source
shape, the memory dump, a tool). Do **not** spend a worker round re-deriving what the "why" column already records -
and when a slot frees, prefer a new unit, a new symbol or the next attribution batch over re-working a parked row.

| unit | score | why it cannot flip yet | what would unblock it |
| --- | --- | --- | --- |
| `main/sys_mem.cpp` | own object clean | **the flip itself fails**: `linkorder.py` says `LINK OK` (sections, sizes, bytes, relocations all fine), so the divergence is elsewhere in the link | the `ninja diff` divergence, or a second opinion on what the flip changes |
| `RSO/runtime.c` | 99.71 % | `RSORelocate` keeps 12 colouring rows at the TU's real level (4) against 9 at level 3; no unit flag and none of the 28 `-opt` sub-options separate them, so the residual is source | a source shape for the web-order tie-break (rows 18-22), or evidence the TU's level is 3 |
| `Camellia/camellia.c` | 99.79 % | `setup256`'s last 9 rows are a genuine two-way ambiguity: `setup128` is exactly 100 % at level 3 and 99.498 % at level 4, `setup256` is 9 rows + retail's frame at level 4 against 211 rows at level 3 | a third source variant, or an `-opt` combination that separates the two functions |
| `Pl/pl_skill.cpp` | 99.76 % | 13 of 197 functions below 100; the unit-level flag that replaces the pragma is byte-identical (landed), so the flag is now the unit's and the residual is unchanged | per-function shapes for the 13 (row 34's switch-tail family) |
| `Pl/pl_act.cpp` | 97.87 % | `Pl_bari_ck` has retail's out-of-line first-arm body; `fn_8027C208`'s 16-byte frame is locals retail never touches and is not source-reachable | a shape for the out-of-line arm |
| `Runtime.PPCEABI.H/Gecko_ExceptionPPC.cp` | 93.68 % | the exception/rename work landed; what remains is the long tail | a round per function |

The flipped units are `configure.py`'s `Object(Matching, ...)` entries (§2); the flip path is `docs/pipeline.md`
§15.3.

**The breadth queue is the pool** (§12, "Register once, at the final home"): the registered units that are not
`Matching` and that no worker has taken (`queue.py list`; until 2026-10-04 only the body-less ones). They have no score, so they are not parked rows - a unit joins this
table only once a worker has measured it.

### The binary is the ground truth, and compilation is lossy (owner's principle, 2026-09-23)

This is a *matching* decompilation, so the binary is the only authority - and because compilation is a **lossy**
process, what survives in it is *evidence about the source*, not an accident. The method: **extract the maximum from
the binary first, then fill only the blanks it cannot answer.**

What the binary actually carries, and what each trace is evidence *for*:

| the trace | what it tells us |
| --- | --- |
| `__FILE__` strings in `.data` | the original **source file name** (`ef_line.cpp`, `ef_point.cpp`, ...) - hence the module and the language |
| mangled symbols | the **language** (C++ vs C) and the **signature** |
| `Panic` line numbers | the **source line** of each assert - which orders the source and shows how much is missing |
| assert message text and format strings | the **semantics**, in the original author's words |
| the `.sdata2`/`.data` pool order | the **order the literals appear** in the source |
| relocations | which **named symbol** each load refers to |
| jump tables | the **switch structure**, including the case count |
| `extab`/`extabindex`, `.ctors`/`.dtors` | the **exception and constructor structure**, hence the function shapes |
| section sizes and the map | the **unit's extent** and every symbol's size |

The corollary is a working rule: when a tool or a brief can carry one of these, it should. A worker that has to
*rediscover* that a unit is `ef_cube.cpp`, or that a symbol is C++, is doing work the binary already did for us - and
this session showed six workers independently rediscovering the same peephole lever, which is the same waste in a
different place.

### Paste the queue's spawn line verbatim (2026-09-23)

The profile each leg runs is `docs/pipeline.md` §2.4; a pair run's legs are listed in `docs/pipeline.md` §15.1.

Two mistakes this session had one cause: **the orchestrator retyped what `queue.py next` had already produced.**

* it grepped the spawn line for the `cwd` to get the worktree path, dropped the `agent:` line, and spawned six
  general-purpose `worker`s where the queue said `decompiler` - losing the decompiler's prompt, its loaded skills and
  its acceptance role;
* it wrote the task text by hand, and added *"end your turn by calling `subagent_done`"* - an instruction **no agent can
  follow**, because none of them declares that tool. `brief.py` and `queue.py` do not say it (both have selftests
  asserting the phrase is absent: *the handoff is the final message*), and delivery happens through the extension's
  grace path regardless.

So: **`queue.py next` prints `agent:`, `name:`, `cwd:` and `task:` - paste all four.** Hand-typing them re-introduces
exactly the drift the queue exists to prevent, and both of the above were silent: the work still ran, it just ran with the
wrong agent and an impossible closing instruction. Adding hints to a pasted task is fine; rewriting it is not.

### Which agent to spawn (2026-09-23)

The profile table and the kind -> profile map are `docs/pipeline.md` §2.4. A `worker` is the *general-purpose* agent
and is right for tooling, probes and investigations - it is **not** the right agent for a unit lane, and using it
there loses the unit profile's prompt and its skill set.

### The two tools that changed how a round is worked (2026-09-23)

Both were built as experiments on `experiment/*` branches, tested by the owner, and integrated after approval.

**`tools/flags/shapesearch.py`** (with `shapes.py`; `mt.py shapes`) - the **source-shape searcher**. The residual on a
stubborn function is *codegen*, not comprehension: MWCC colours registers from the source's temporary structure, so the
same logic spelled differently scores 96 % or 100 %. It generates variants - 13 generators: statement and declaration
order, `for`-declaration hoisting, signedness and `volatile`, casts, compound assignment, ternary, named temporaries,
loop shape, switch arm order, `default`-first, `break` to `return C`, the field form, and row 35's dead copies -
compiles each with the unit's **real command line** into a scratch copy, scores each with the **official** metric,
dedupes by normalised source *and* object sha1, and ranks; depth > 1 is a beam search. `src/` is never written.

*On `Pl/pl_act`'s worst 20 it improved 12 and took two to 100.000 % byte-identical in 30 seconds* (550 candidates, 12
threads): `fn_8027D40C` by hoisting a declaration, `Pl_get_gunner_vec` by a dead copy. **A miss is informative**: a
function with *zero differing rows* that still scores 99.9 % is relocation naming, so the `.sdata2` claim is the fix,
not a shape (rows 23/29).

**`tools/flags/infer.py`** - the **flag inferencer**. Reads a split *target* object and names the flags its unit was
built with, each with evidence and a confidence: record forms and the `li r0,N; psq_lx` epilogue for the peephole,
fused FMA vs `fmuls`+`fadds` for `fp_contract`, per-symbol `lis`+`addi` vs a shared base for the pool, `stmw`/`lmw` vs
`_savegpr_*` for `use_lmw_stmw`, function starts and `gap_*` padding for `func_align`, and a `bl` to a tiny
same-object function for `inline`. **36 confident claims, 100 % correct, zero confident misses**, and it abstains on 78
units rather than guess. It answers the round that **fourteen** workers each spent rediscovering
`#pragma peephole off`. It decodes PPC by hand because GNU objdump mis-decodes Gekko paired-single as VMX.

It cannot infer the `-O` level, `mw_version`/compiler family (`.comment` is synthesised), `-Cpp_exceptions`,
`-schedule` or the C++ front-end - and it found that **no object in the project contains a fused FMA**, so
`-fp_contract` may be inert here and the seven workers who reported it may have been seeing something else.

### The leaked SDK source is forbidden (owner's rule, 2026-09-23)

The `oracle-mine` experiment indexed the leaked RVL SDK / `wii_development_package` archive and a public decomp
(`doldecomp/ogws`) to answer "does the original source for this symbol exist". **The owner has forbidden it**: no
corpus, no index, no fetcher, no query tool. The branch, the worktree, the fetcher and the index are deleted, and the
fetched trees were removed from `%TEMP%`.

Two things from that experiment are worth keeping as *facts about the terrain*, not as a licence:

* the archive's SDK half is **headers only** - so it offered signatures, types and layouts rather than bodies;
* `doldecomp/ogws` was **verified byte-identical to a public decompilation project**, not to Nintendo's source.

The existing oracles are unchanged and remain the sanctioned ones: the **shared Ghidra runtime memory dump**
(`docs/memory-dump.md` - names, signatures, struct layouts, data contents, explicitly *not* codegen evidence), the
**symbol map**, and the **original DOL itself**. If a future session wants a *public decompilation* as an oracle, that
is a **separate decision for the owner** - it is not covered by this experiment's removal, and it must be asked for
explicitly rather than assumed.

### The language comes from the symbol, not from our convenience (owner's rule, 2026-09-23)

A unit is **C++** when either of these says so:

* its **symbol is mangled** - `Panic__Q24nw4r2dbFPCciPCce` is C++, and so is anything whose map name carries a
  `__Q`/`Q24`/`__F`-style mangling instead of a plain C identifier;
* its **panic/log string names a `.cpp`** - the `__FILE__` assert strings are original source names, and five of the
  ones we have found end in `.cpp` (`ef_line.cpp`, `ef_point.cpp`, `ef_cube.cpp`, `ef_cylinder.cpp`, `ef_disc.cpp`), so
  a unit whose pool holds one is a C++ translation unit even when its `.text` reads like C.

That decides three things, and none of them is stylistic:

* **the file extension** - `.cpp`, not `.c`;
* **`-lang`** - the front-end changes, and the residual it leaves is *not* source-reachable: the `800CCFB0` round
  closed at 99.96 % with its last two rows attributed to "C-vs-C++ front-end", and `Panic`'s variadic `crclr
  4*cr1+eq` is emitted *only* for a C++ callee;
* **the name objdiff pairs by** - a C++ definition is mangled unless it is `extern "C"`, which is row 42 seen from the
  other side.

**No retro-fit under register-once.** The worker that registers a unit reads its `__FILE__` string *first*, so the
extension (`-lang`) is right from the unit's first build; the old flow let attribution pick an extension without
that evidence and parked the fix in a later promotion pass, which retired with the `auto/` bucket. A unit already
registered with the wrong extension is a defect to fix in place - rename, extension and `-lang` in one change, one
re-split.

### Register once, at the final home (owner, 2026-09-24)

The `src/auto/` bucket is **retired**. A translation unit is registered **once**, at its final
`src/<module>/<name>.<ext>` home, by the worker that works it. The old two-step (`attribute` -> `src/auto/<addr>_fn_<addr>`
stub -> later promotion to a real name and location) touched every unit twice and is gone, as is the tiler that proposed units.

* **The worker names the unit from the evidence it has.** In order: the `__FILE__` string in the pool (the
  original source name, hence the module and the language), then a real name from the shared dump
  (`docs/memory-dump.md`), then what the code does plus the naming scheme of its neighbours. **The map's
  `fn_XXXXXXXX` stem is not an outcome** (owner, 2026-09-26): when the evidence is thin, derive a name from the
  context and **mark the guess** in the unit header so a later pass can refine it - a generated `fn_`/`lbl_`/
  `unk` name left in `src/` is a defect, and a `rule 7 deferred` comment exempts nothing - the lint honours
  no key (the escape's files are pure comment text now), and the land gate's lint row refuses a batch that *adds*
  a generated name in `src/`; the old row that refused a grown escape or a unit at a generated file name was
  deleted 2026-10-05). **Inventing a module is
  forbidden** - a module comes from the `__FILE__` string, the dump or the subsystem, never from a guess.
* **Registration before measurement is the constraint.** A worker cannot score a unit that is not in the build
  graph, so it makes the registration - `splits.txt` range, `configure.py` entry, source file - **inside its own
  worktree** and measures there. The orchestrator applies that registration on `main` (the landing,
  `docs/pipeline.md` §4), so the shared files keep one writer.
* **One re-split per batch.** Registration and a rename both dirty the split, so registrations ride a batch the
  way renames do - **one re-split per batch**, the same cost the old attribution batch paid; the ledger and the
  brief pool follow the unit to its real path.
* **The object must stay byte-identical** across a move or rename - a name and a path change no instructions, so
  any byte difference is a bug in the move, not a matching change. Verify it, do not assume it.
* **There is no `src/auto/` left to migrate.** Measured 2026-09-28: the directory does not exist in the tree
  (the 36-unit promotion landed), so §1 clause (1)'s "no `auto/*` placeholder unit remains" is already
  satisfied and no bullet here has to schedule a move. A unit that still drifts under a placeholder path is a
  defect to fix **in place** - rename, extension and `-lang` in one change, one re-split - never a second
  registration.
* **A claim has one key.** The claim's key was once the unit's *spelling*, so
  `auto/X` and `auto/X.c` defeated the lock and two workers took one unit. `claims.norm_unit` now strips the source
  extension at claim and release (commit `c36ab86f`), so a unit has one key whatever it is named.

### A branch is never the only copy of work (2026-09-23)

`claims.py release` deletes the branch. That is right for a **landed** unit - its work is in `main` and the branch is
redundant - and wrong for an **unreported** one, where the branch is the only copy. The tool makes a
`refs/rescue/<slug>` ref before every branch deletion, and that is the difference between this being a scare and
being data loss: `auto/800E46E8` (185 functions, the campaign's largest single landing) was landed **from** its rescue
ref after a `--force` release had taken its branch.

The rules that follow:

* a rescue ref is **never deleted without proof it is redundant**: the only deletion is the audit's `redundant`
  verdict - the unit is on `main` *and* every touched path matches - whether it runs at teardown or
  `rescue.py audit --prune`; a ref the classifier cannot prove contained (drift, unlanded, unknown) is never
  touched, because a by-name check cannot see a unit renamed or absorbed into another file;
* `--force` on an unreported claim is defensible *only* because the ref exists - so the refusal message names the
  exact restore command, and the release says what it is about to remove before it removes it;
* after any `--force` release the recovery list is `git for-each-ref refs/rescue`, and the restore is
  `git branch worker/<slug> refs/rescue/<slug>`;
* a branch whose work is *in main* - merged, or cherry-picked and gated - may be deleted silently. That is the normal
  teardown, and it is why `land.py` releasing a landed unit is safe.

`tools/units/rescue.py audit` is what reads that safety net (193 refs by 2026-09-27, and nothing had ever
looked at them). For each ref it derives the unit(s) the ref registers from the ref's registration **diff
against its merge-base with `main`** - the `Object(...)` rows and `splits.txt` headers the ref *added*; a
whole-file name match against `main` matches every unit in the file - reports whether each unit is registered
on `main` today, and diffs the paths the ref touched against `main`. It then classifies: **`redundant`** (the
unit is on `main` and the touched paths match), **`landed-with-drift`** (`main` has moved on), **`unlanded`**
(no such unit on `main` - the ref may hold the only copy) and **`unknown`** (no merge-base, or nothing
parseable). `--prune` deletes **only** `redundant` refs and prints each one; without it the audit is strictly
read-only; `landed-with-drift`, `unlanded` and `unknown` are never touched. The classification is deliberately
conservative about renames: an `auto/` unit that migrated to its final home still fails the by-name check and
is surfaced as `unlanded` (and kept) rather than guessed about.

**The audit also runs at teardown, where the answer is still actionable** (2026-09-27). `claims.py release`
classifies the ref it parks - in the same step list, right after `git update-ref` - and acts on the verdict: a
**`redundant`** ref (the unit is on `main` and every touched path matches) is **pruned and the deletion
printed**; **`landed-with-drift`** is **reported and kept**, one line naming the ref and the drift size, because
drift can hide an unlanded hunk; **`unlanded`/`unknown`** are **surfaced loudly** - the ref, its unit(s) and its
date - and kept, because that is the case where the lane's work did not land and the ref may be the only copy.
The verdict is a **report, never a gate**: a release always completes, no verdict can fail one, and nothing is
pruned without proof of containment. The classification is `rescue.py`'s - `release` calls it, it does not
re-implement "is this on main". A teardown that finds a `redundant` ref prunes it (that is the one deletion
rule), and `--force`'s cost line says so instead of naming a ref that is gone. The lesson is *when* the check
runs: at release time the loss is actionable, 193 refs later it is archaeology.

`land.py resolve`'s helper branch has the same teardown rule. `scratch_resolve` parks the union on a
`land/resolve-<slug>-<pid>` branch in its scratch worktree so the caller can fast-forward the worker branch;
when that branch lands, `land --branch` deletes the helper **and prints the deletion**. The helper is deleted
only when its tip is provably contained by the branch or by `main`; one that carries a hand fix the branch
never took (2026-09-26: the `u32 mode` repair lived only on `land/resolve-8030681c-...-31048`) is refused
loudly and left alone, because it may be the only copy.

### Teardown is part of landing (owner's rule, 2026-09-23)

**When a worker finishes, its claim is released and its branch is removed** - and `claims.py` is the tool
that does it, because the claim owns the branch, the environment and the registry entry. With the slot pool
(§5.1) the environment is **returned to the pool, not destroyed**: its directory and its warm `build/RMHE08`
stay, only the branch (the lock) and the lock file go. Landing a unit is not finished until its teardown is: a
landed unit must not leave a claim, a merged branch or a registry entry behind, and the same is true of a worker
whose round produced nothing.

`claims.py release <unit>` is the one-shot: rescue ref -> **audit that ref** (`redundant` pruned and the
deletion printed, drift reported and kept, `unlanded`/`unknown` surfaced loudly with the ref, its unit(s) and
its date and kept - a verdict is reported, never enforced) -> **return the slot** (detach at
main's tip, clear the lock, refresh the warm tree) - or `git worktree remove --force` for a throwaway worktree -
-> `branch -D` -> `prune`, then the registry entry. It must be **idempotent and total** - every step says what it
did or why it was skipped (already gone, never existed) - because aborting on an
already-removed target is how the 2026-09-23 Camellia tangle happened: its claim could not be released, a
merged branch then blocked the re-claim, and a leftover directory blocked the new worktree, all three cleared
by hand. The one real refusal is a **live session**, which pins the worktree as its cwd on Windows (5.1) - that
stays an abort, naming the session.

The flow calls it, so nobody has to remember: `land.py` releases the claim of the unit it just gated, and
`claims.py release --all-merged` sweeps every finished worker in one command.

### Production mode (owner, 2026-09-23)

The campaign runs continuously and the orchestrator manages it: **the worker slots stay full and the queue is
worked without per-batch approval.** The loop is `docs/pipeline.md`'s: the pair run (§15: scout, scout
review, pre-pass, lanes, land), or a wave where its §1.3 says so; one unit per landing, one slot refilled
per completion (its §13.1). Renames, phantom merges, range claims and source work ride the same split (§3, item 4 of
the law); in a pair run every own-row `symbols.txt` rename rides the shared pre-pass batch (`docs/pipeline.md` §15.1).

Work is chosen by the ledger: the next residual unit by the per-module table, the next flip by `flipcheck.py` and
the reverse-reference check (`docs/pipeline.md` §15.3). The gates stay on (they have caught real regressions,
protocol bugs and a wrong premise); what the owner has removed is the *approval*, not the verification. A stop is
only for the plan's own conditions: the target closed, a check that will not pass after three attempts, or the
budget.

**The build order so far** (done, or retired): the ground-truth guard (7.18), the protocol tools (7.1-7.5, 7.15,
7.17, 7.20, 7.21), the first `Matching` flip (7.6), `dumpmap.py`, `dataclaim.py` and `phantom.py` (7.7-7.9), and
the attribution pass (retired 2026-10-03; the splits program did it).

**Follow-ups the tool work named:** migrate `tools/symbols/symedit.py::rewrite` to `lib.text` (`Transaction`; `sharedfiles.py` was retired in WP6) - it writes
`symbols.txt` with its own temp+replace and no anchor/overlap/idempotency gate, the highest-risk shared file -
and `tools/units/dataqueue.py::write_queue`, which duplicates the same primitive.

**What remains on the critical path:**

1. **Phase C, the residual sweep** - the 80-99.9 % band (a unit with any residual cannot flip; the parked list
   above names the measured ones). **The type and naming conformance of §6.5 rides this pass** - the same units,
   the same order - so a unit leaves phase C byte-identical *and* conformant.
2. **The data second pass** (7.8 + 7.17) - measured, one batch at a time, driven by the data queue rather than
   by comments in `splits.txt`.
3. **Phase D, the flip campaign** (7.19) - as units reach byte-identity, flip them (the flip path is
   `docs/pipeline.md` §15.3) with a link-order audit, until `ok` passes from a clean tree (S1).


## 13. Risks, and where each is handled

| risk | handled by |
| --- | --- |
| reaching for flags to fix what is really source shape or liveness | playbook 13, `mwcc-unit-matching` |
| a `Matching` flag on a wrong object | non-negotiable 4, `objdiff-verify`, each flip hash-checked alone (batches allowed, `docs/pipeline.md` §11 2026-10-13), §1's per-object rule |
| claiming a data range that lowers a function | playbook 23, §8.4, `dataclaim.py` |
| a rename that splits the map and the source | `symbol-map-editing` (two edits, one change), `symedit.py rename-batch` |
| a stale report or a hand-written compile | `recompile.py` (7.1), §8.5 |
| four agents editing one file | worktrees + branches (§5.1), one writer per unit, `land.py`'s file-set check; a pair run's pre-pass and the manifest row (`docs/pipeline.md` §14.4, §15) |
| a worker in a worktree cannot measure (no `build/`, no `build.ninja`) | 7.15 — `MAIN` resolution, explicit `--target`/`--flags`, the submodule init; the first worktree round does not start without it |
| a worker's commits silently dropped by a tip-only cherry-pick | `land.py land --branch` applies the branch's merge-base diff (`docs/pipeline.md` §4) |
| rewriting the DOL's ground truth (`build.sha1`) | 7.18's refusal + the sha1 cross-check against `orig/RMHE08/sys/main.dol` |
| a worker's diff landing on `main` before verification | `land.py land` gates before it commits; a refusal leaves the tree as it was (`docs/pipeline.md` §4) |
| a worktree that cannot be removed (submodule, untracked evidence) | no submodule in worktrees; evidence in `MAIN/.pi/`; `remove --force` + `branch -D` + `prune` (7.2) |
| overlapping or half-written registrations | `apply` transactional + the overlap check (§6.1) |
| a residual unit silently blocking the DOL | §1.1: phases C and D are on the critical path, and the residual class is scheduled, not hoped for |
| a green `ok` from stale split objects | §1's clean-tree completion test |
| a green `ok` from a stale link | §11's delete-and-recreate check in `land.py` (plus the exit-code checks) |
| the attribution pass claiming a wrong seam at scale | the 0.5 MB cap, evidence-first partition, unproven seams marked, and matching as the arbiter |
| the compiler-build residual (`0x0e` vs `0x0f` `.comment`) | recorded as a project-wide known difference; a unit that resists with everything else equal is a candidate for the version matrix, once |

## Appendix A — the incident → rule map

The fourteen incidents in `docs/process-review.md`, and the rule that now prevents each:

| incident | rule |
| --- | --- |
| 1 overlapping registration | §6.1 `symbolpreflight` verdict is a gate; 7.12 runs the overlap check |
| 2 CRLF/LF silent no-op | 7.12 (one writer, asserts every anchor) |
| 3 undeclared progress category | 7.12 + `land.py` runs `configure.py` before the split |
| 4 stale-object measurement | 7.1 `recompile.py` |
| 5 stale `report.json` | §8.5, `ledger.py`'s stale warning |
| 6 phantom `fn_*` symbols | 7.9 `phantom.py` |
| 7 the first partitioner's 17 one-function units | §6.1 evidence-first partition + selftest |
| 8 a size cap cutting inside a must-link anchor | `attribute_selftest.py` (23 checks) |
| 9 the wrong zip member | 7.7 `dumpmap.py` + `docs/memory-dump.md` |
| 10 an unsupported seam claim | §8.3 seam policy |
| 11 concurrent writers | §5.1 worktrees + branches; the pair pre-pass and manifest (`docs/pipeline.md` §15) |
| 12 manual verification | 7.5 `land.py` |
| 13 the knowledge delta by memory | 7.10 the machine check |
| 14 an out-of-band flag | §8.2 flag policy |

## Appendix B — the numbers, and how to re-measure them

```sh
python tools/units/ledger.py                 # totals, per-module, closed >= 80 %, unclaimed
python tools/selftest.py                      # every tool's own selftest; green except the parked list
python tools/units/ledger.py unit <unit>     # one unit's coverage and per-symbol score
python tools/units/ledger.py --json          # machine-readable
ninja build/RMHE08/report.json               # 1.5-3.5 s; the report is what the bar reads
ninja changes                                # whole-tree diff vs the baseline; a non-empty line
                                             # is a neighbour that moved -> investigate, never waive
python tools/units/measure.py <unit> --against-main   # the unit's rows with their delta vs MAIN's report
ninja build/RMHE08/ok                        # the DOL hash - the only test that matters
python - <<'EOF'                             # per-edge build costs, straight from .ninja_log
for l in open(".ninja_log"):
    p = l.split("\t")
    if len(p) >= 5 and p[3].endswith(("config.json", "main.elf")):
        print(int(p[1]) - int(p[0]), "ms", p[3])
EOF
```

Baseline for this plan: **295 attributed / 284 closed ≥ 80 % / 217 matched / 65 128 bytes**, commit `9d6b351`.
