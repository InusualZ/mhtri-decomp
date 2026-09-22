# Campaign plan: every symbol in `symbols.txt`

The goal is one pass over the whole symbol map, symbol by symbol:

1. **attribute it** - discover the translation unit (TU) it belongs to, or move it to the one that should
   own it,
2. **decompile it** - write the source,
3. **match it** - ≥ 80 % per-symbol `match_percent` is the bar for closing the symbol,
4. **commit it**.

Nothing about the project's rules changes. `Matching` still means byte-identical (non-negotiable 4), so a
unit closed at 80 % stays `Object(NonMatching, ...)` and its bytes are not linked - which is what keeps
`ninja build/RMHE08/ok` green for the whole campaign. The 80 % bar is a bar for *closing a symbol*, not for
changing what "matching" means.

## Where this starts

| | |
| --- | --- |
| symbol map | 65,700 lines: **20,524 functions**, 45,176 data symbols (22,418 of those are
`extab`/`extabindex`/`.ctors`/`.dtors` fragments that come with the code unit that owns them) |
| split units | 13,615 in the report, of which the `auto_*_text.o` scaffolding is unowned |
| registered | 11 units in `splits.txt`, 12 registered objects, 7 C sources (+ 1 header) under `src/` |
| closed today | **24 of 20,524 functions at >= 80 %** (objdiff counts 19 matched), 21,836 of 5,437,392 `.text`
bytes (0.54 %) |

This is a ~20 000-function campaign: it is run in sessions, and the *repository* is the state - `splits.txt`,
`configure.py`, `report.json` and the units' own file header comments. Nothing that matters may live only in
a chat (the playbook says so, and this repo has already lost a finding that way). The numbers above come from
a regenerated report; `ledger.py` prints a warning when `report.json` predates `splits.txt`/`configure.py`,
because that is the one input here that is a build output and it describes a repository one session old.

## The loop, and the gate at each step

| # | step | tool | gate before moving on | left behind |
| --- | --- | --- | --- | --- |
| 1 | attribute | `tools/units/symbolpreflight.py`, `tools/splits/tudiscover.py` | the verdict is `proceed`, or `approve` with a written proposal the owner accepted | `splits.txt` ranges (+ `configure.py` entry) |
| 2 | decompile | Ghidra dump (`docs/memory-dump.md`) + `tools/units/m2cinput.py` → `tools/m2c` | it compiles, and the disassembly agrees with every instruction-level decision | `src/<Dir>/<file>.c` (+ header) |
| 3 | match | `mt.py diff/info`, `ninja changes`, report.json | the symbol's own `match_percent` ≥ 80, nothing else regressed, `ninja build/RMHE08/ok` still green | the residual in the unit's file header comment |
| 4 | commit | `tools/git/prepcommit.py` | the review sheet is read and approved (rule 6), and the batch's knowledge delta is written (see below) | one commit, and the ledger moves |

**Alignment and flags, learned the hard way in batch 1.** `-O4,p` implies `-func_align 16`, and the retail SDK
objects of `Runtime.PPCEABI.H` are all `.init align 2**2`: `memcpy.o`'s two functions are packed
contiguously, while `__start.o`'s are 16-byte aligned. Both layouts are reproducible (a
`#pragma function_align 4` in `memcpy.c`, the implied `-func_align 16` for `__start.c`), so no lib-wide flag
changes until a measurement says otherwise; if one lands, every unit in that lib has to be re-measured, since
function alignment moves every symbol after the first. A source pragma is also the honest place for a
*per-unit* difference like this - and note the inverse lesson: a pragma the reference SDK source carries
(`#pragma scheduling off` in `__init_data`) can itself be the residual, and no `-opt` sweep will show it.

Steps 2-3 are the two existing skills: `decompile-symbol` for "an address becomes a registered, measured
unit", `mwcc-unit-matching` for "this unit does not match yet". This plan only adds the order, the bar, and
the bookkeeping.

### 0. Once, before the first symbol

* **Autonomy (owner grant, 2026-09-21): the orchestrator runs the campaign without gates.** It decides the
  batch, the registrations, the flags, the queue below and the commits, and does not ask per item. It
  interrupts the owner only for a **pressing** issue, meaning one of:
  1. **the linked DOL is at risk** - anything that would change `config/RMHE08/build.sha1` outside the
     `NonMatching`-safe path;
  2. **loss or destruction** - deleting work, rewriting history, pushing, or an unrevertible bulk edit (a
     mass rename, a scaffold-wide range claim);
  3. **information only the owner has** - a reference source, a ROM, a tool that is not on this machine;
  4. **a premise change** - the bar, the scope, the vendor files, or an `AGENTS.md` rule the campaign would
     have to break;
  5. **the environment** - the owner's machine, disk or another session is in the way.
  Everything else - a symbol that resists, a flag that does not help, a table left unowned - is the campaign's
  own business, recorded (playbook, unit header, this file) rather than asked about.
* **The escalation queue, decided** (kept here so it is not re-litigated in every session):
  1. **`memcpy.c` and `memset.c` stay separate.** The retail member is one object (`__mem.o`), but this lib
     builds with `-inline auto`, and one file holding both `__fill_mem` and `memset` lets MWCC inline the
     helper into `memset` - a 100 % symbol spent to buy a file name. Revisit only if that lib's inline
     setting changes for another reason.
  2. **The TRK interrupt-vector table (0x80004380-0x800062B4) stays unowned.** Zero relocations, not
     expressible in C: reproducing it as `.long` data would match ~8 KB of bytes while recovering no method,
     against the repository's "no hand-written assembly" premise. Revisit only if byte coverage becomes the
     goal.
  3. **`-func_align 4` for `Runtime.PPCEABI.H` - landed** (batch 5, `cflags_ppceabi`): `-O4,p`'s implied 16 padded
     every function of that lib (three independent witnesses), and `__start.c`'s 16-byte function starts come
     from each function's own `.init` section, not from the alignment. Next pass on that lib should also claim
     the `.bss` fragment `fragmentinfo` (0x806F4B48-0x806F4CC8) and `Gecko_ExceptionPPC.cp`'s jump table
     (`.data 0x8060E8A0-0x8060E8E4`), both currently in the scaffold so their `lis`/`addi` displacements count
     as mismatches.
* **`Matching` flips** (decided): a unit is flipped only when it is byte-identical - every symbol 100 %,
  sections and relocations equal, proven per the `objdiff-verify` skill - **and** `ninja build/RMHE08/ok` stays
  green afterwards, one unit per commit and named in the message. Nothing qualifies today (best unit: 97.81 %
  fuzzy); the first flip is done alone so the commit before it is the revert point.
* **Commits need no per-batch approval** (owner grant, 2026-09-21): the orchestrator commits its own campaign
  work as it goes - one commit per batch, carrying the sources, the registration, the flags it proved and the
  knowledge delta. What is *not* committed is anything that is not a finished piece of work: probes, scratch,
  half-registered units, another stream's files. History is never rewritten and nothing is ever pushed.
* **Stop conditions** - the orchestrator does not stop because a batch ended; it stops when one of these is
  true, and each stop leaves the block, the ledger and `ok` consistent:
  1. the campaign target it was given ("the next N symbols") is closed - report the delta and the new totals;
  2. a gate blocks it: an escalation-queue item only the owner can decide, a regression it cannot fix, or a
     check that keeps failing (three attempts on one symptom is the limit - report the attempts, not a fourth);
  3. its own working budget runs out (context or turn): stop at a verified boundary - never mid-registration,
     with a dirty `splits.txt`/`configure.py` that has not been through a re-split and a report.

* **Build the ledger** (approved): `tools/units/ledger.py`, which *derives* progress from the repo instead of
  keeping its own state, so there is nothing to sync or commit -

  ```sh
  python tools/units/ledger.py             # totals, per-module table, units at >= 80 %
  python tools/units/ledger.py next 20     # the next 20 unclaimed symbols, in address order
  python tools/units/ledger.py unit <unit> # one unit's coverage and per-symbol score
  ```

  It reads `symbols.txt` through `symedit.py` (never by hand - rule 7), `splits.txt`, `configure.py` and
  `build/RMHE08/report.json`. `next` is what step 1 of each iteration consumes; without it there is no
  mechanical way to pick the next symbol out of a 65,700-line map.
  `tools/units/ledger_selftest.py` covers the views on fixtures (no build, no repository state).
* **Standing approval for step 4** (granted by the owner, then superseded by the autonomy grant above): the
  campaign commits without asking. `prepcommit.py` is used for its checks - explicit paths, refusals, the DOL
  gate, the LOCAL-ONLY round trip - not as an approval gate.
* **One worktree per stream** (`git worktree add -b <stream> ../mhtri-dtk.ws-<stream> <base>`), so a second
  `ninja` cannot race this one and dirty files are not shared.
* **`ninja baseline` once**, so `ninja changes` has something to compare against for the rest of the run.

## 1. Attribute the symbol

**The unit is the unit of work.** A `splits.txt` edit re-splits the whole DOL (6-12 minutes here), and
`configure.py` + `splits.txt` are the only expensive parts of the loop - the functions of a registered unit
are then cheap to iterate. So: register a unit once, then close *all* of its symbols before moving on. Batch
registrations; never pay a re-split for one function.

```sh
python tools/units/symbolpreflight.py <address|name>        # owner, collision verdict, drafts
python tools/splits/tudiscover.py at <address|name>         # TU boundary proposal when it is unowned
```

* **Already owned** (preflight kind 1 shows the address inside a registered unit): the question is not "who
  owns it" but "should it". Check whether that unit's source or object actually *references* the symbol; if
  it does not, propose the re-attribution with that reason.
* **Unowned** (`auto_*` scaffolding, kind 2): `tudiscover` proposes the TU - the functions *and* the data
  ranges (`.rodata`/`.sdata2` pools, `scope:local` anchors, jump tables). The proposal is the input, not the
  justification: every range is measured before and after (playbook 23 - a claimed range has *lowered* a
  function here before).
* **Name the unit** from module evidence, not from taste: `Panic(__FILE__, line)` strings in the surrounding
  code, pooled string literals, clusters of mangled C++ names, or the shared runtime dump
  (`docs/memory-dump.md`). A unit's path mirrors the module (`src/<Dir>/<file>.c`).
* **Claiming a unit's data ranges is what makes its relocations pair.** A `static` the retail file defines but
  nothing claims stays in the `auto_*` scaffold, where dtk names it `<name>_<address>` - and objdiff matches a
  relocation by name, so the owning unit's score stops just short (batch 1: 98.33 % and 97.5 % on two 3- and
  2-instruction accessors until the re-split after registration renamed it back). The catch is alignment: a
  `.sbss` split has to be 16-byte aligned (`Invalid alignment for split: ... .sbss 10:0x807953C9`), so a
  one-byte static that shares its 16-byte block with another unit's symbols **cannot** be claimed - accept the
  reloc-name residual and write it down in the unit header.
* **Never claim linker-generated data.** `_rom_copy_info` / `_bss_init_info` are "found in Linker Generated
  Symbol File" in `build/RMHE08/main.MAP`; MW ld emits them, no source can own them, and the DOL reproduces
  them byte-identically. A `preflight` draft for such an address is wrong: reject it.
* **A unit owns its `extab`/`extabindex` fragment, but check whose it is** before claiming one: the entry that
  a surrounding function's unwind record names belongs to *that* function's unit (`@etb_800066E0` in `.init` is
  `main`'s, three sections away from the `.init` code it sits next to).
* **`approve` and `never touch` are stop signs** (skill `decompile-symbol` §1): both are reported with
  numbers and wait for a human answer. `never touch` means the owner is `Matching` and its bytes are linked.
* Data-only symbols cannot be decompiled - they are claimed as ranges, which is what later lets the code
  around them match. That is a legitimate loop iteration; close it like any other.

## 2. Decompile it

Follow `decompile-symbol` §3; the two shape sources are the Ghidra decompiler C and, offline, `m2c`:

```sh
python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o -f <symbol> -o build/tmp/<symbol>.s
python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f <symbol> build/tmp/<symbol>.s
```

* The **disassembly is the arbiter**; both decompilers are shapes and neither is codegen evidence.
* Real names come from the dump, and a rename is `symedit.py` **plus the source in the same edit** (skill
  `symbol-map-editing`) - otherwise objdiff stops matching the symbol by name and reports 0 %.
* The unit's **file header comment** is written on the first commit that creates the file: what it is, its
  `.text` range and function order, where its flags/evidence live, the residual. Not a per-function
  inventory, not percentages.

## 3. Match it

```sh
python configure.py && ninja build/RMHE08/src/<Dir>/<file>.o   # the unit alone
rm build/RMHE08/report.json && ninja build/RMHE08/report.json   # the report is what the bar reads
ninja changes                                                   # this symbol up, nothing else down
python .agents/skills/mwcc-unit-matching/scripts/mt.py info    -u <unit>
python .agents/skills/mwcc-unit-matching/scripts/mt.py diff    -u <unit> <symbol>
```

The bar, all of it:

1. the symbol has a **real per-symbol score of at least 80** - in `build/RMHE08/report.json` (report version
   2, objdiff `v3.6.1`) that field is `fuzzy_match_percent` on the *function* entry, and a function entry
   **without** the key is 0 %, not 100 %; the unit-level `complete_code_percent` is not a score and has read
   100 % next to 1.77 %,
2. **no other symbol regressed** (`ninja changes`; the unit is judged by its own numbers, never by the
   DOL-wide totals, which move for bookkeeping reasons when a range is claimed),
3. `ninja build/RMHE08/ok` is still green (true by construction while the unit is `NonMatching`),
4. the object measured is the one the **real ninja command line** produced - never a hand-written compile,
5. the **residual is recorded** in the unit's file header comment: what still differs, the first divergent
   instruction, the sizes, and what was tried.

The metric is pinned to the objdiff version in `configure.py` (playbook 15): if that version is ever bumped,
re-baseline the closed symbols against the new report before trusting the bar again.

**Below 80 %**: land it only if it measurably improves the unit and regresses nothing (the project's
matching policy is already "land the best-scoring variant", and a `NonMatching` unit cannot hurt the link),
record the residual, and mark it `partial` in the ledger for a second pass. The playbook rows in `AGENTS.md`
are that second pass's todo list - work them one idea at a time, with evidence.

Re-check the two traps every time: a **stale `report.json`** lies (delete it before believing a number), and
**frame size is not progress** (playbook 10).

## 4. Commit it

```sh
python tools/git/prepcommit.py --message "<area>: <what this closes>"
git diff --cached          # read it
git commit -F .git/prepcommit_msg.txt
```

* One commit per closed symbol **or** per batch that was registered together (the owner confirmed per
  batch) - the same work, one concern. Subject style is the repo's: area-prefixed and imperative
  (`Pl/pl_act: match fn_8027BE0C`).
* `prepcommit.py` stages explicit paths only, refuses build/original/scratch output, verifies the `main.dol`
  SHA-1, and handles the `AGENTS.md` LOCAL-ONLY block (non-negotiable 8) - do not stage `AGENTS.md` by hand.
* The `AGENTS.md` local-only **`## Current task / plan` block is updated in the same commit** that closes a
  symbol: the symbol just closed, the next one, and the stream/worktree in flight. That is the one place a
  fresh session looks to resume.
* A playbook idea that worked becomes its `docs/matching.md` section **and** its `AGENTS.md` row in the same
  session; a unit-specific residual stays in the unit's header. A finding that only lives in a reply is lost
  at the next compaction.

## Goal: every symbol in its unit (attribution before matching)

The owner's goal (2026-09-21): **every symbol in `symbols.txt` belongs to the translation unit it came from** -
registered, ranged and named - before the matching work is finished. The 80 % bar still closes a symbol, but
attribution is now the campaign's **primary objective**: it is the cheap half (no source, no measurement) and it
is what makes everything else legible. With every address owned, a unit's score in `report.json` means
something, and a batch's source work has a home before it starts.

**What "attributed" means**: a `splits.txt` block whose ranges cover the symbol's section and address, a
`configure.py` entry for that unit (lib, `mw_version`, cflags, `Object(NonMatching, ...)`) **and the unit's
source file, whose existence is mandatory - not optional**. A stub is enough, and it is what makes the unit
real: it compiles, it appears in the build, and its header comment (what it is, its range, why it sits there,
what is still unknown and how reliable the seam is) is the note the next session needs. Nothing is registered
bare - a `Missing source file` warning from the build is a bug, not a state. The four registrations that
violated this were fixed the day the rule was written (`Pl/pl_{master,skill,act}.c`,
`Runtime.PPCEABI.H/__init_cpp_exceptions.cpp`). Ranges never overlap a registered unit's.

**A source file is created with its functions, not just a header.** The attribution pass registers a unit
*and* writes what has been discovered of it: the functions the unit is known to hold, with their decompiled
bodies wherever they are recoverable, measured against the target. A file that carries only a header comment is
a note, not a unit - `global_destructor_chain.c` is the worked example (registered with its range, its two
functions written from the SDK's listing and a 100 % each, in the same commit). Where a body cannot be written
yet the rule is narrower but still explicit: the header names where the unit's inventory and evidence live
(`ledger.py unit <path>`, the map, `splits.txt`) instead of copying a function list into the file, because the
inventory belongs to the map and a header that tabulates it goes stale on the next range edit.

**Naming**: the retail file name is only available where evidence exists (`Panic(__FILE__)`, a shared pool, the
runtime dump, an `extabindex` group). Everywhere else the unit gets an honest placeholder under `src/auto/`
(`src/auto/<first-symbol>.c`) with `progress_category: "auto"`, and its file header records *why* it sits where
it does (the `tudiscover` closure, the extab group, the pool it owns, and how reliable the seam is). A later
session renames it when evidence appears - map, source, `splits.txt` and `configure.py` in one edit.

**The method is bulk, not one scout per symbol**:

1. `python tools/units/attribute.py plan <start> <end>` - `tudiscover`'s proposals for every unclaimed address
   in the range, grouped into units, with the data ranges each would own and the boundary confidence
   (`certainly one TU` vs `weak signals` only).
2. `python tools/units/attribute.py apply <start> <end> [--dry-run]` - append the `splits.txt` blocks, the
   `configure.py` entries and **the unit's stub source in the same commit** (mandatory - see above), code
   ranges first, `extab`/`extabindex` placed from the decoded records, never overlapping an existing range.
3. One re-split per batch, then the ledger is the acceptance test: **`covered` must equal
   `20 524 - unclaimed`** for the range claimed, and `ninja changes` must show no *real* unit regressed (the
   `auto_*` scaffold losing symbols is expected and is not a regression).
4. **Data ranges are a second pass**, per batch, measured against the report before and after - claiming a
   range has lowered a matched function in this repo before (playbook 23). The code pass never touches them.
5. **Confidence is recorded, not hidden**: a unit whose seam rests only on weak signals is still applied (the
   extent settles as its functions match - that is the documented workflow) and its header says the seam is
   provisional.

Order: ascending address, module by module, so each batch's ranges are contiguous and its re-split buys the
most coverage. Source writing (steps 2-3 of the loop) follows behind, unit by unit, in the same order.

**Status (batch 6): the tool exists and the first pass is applied.** `tools/units/attribute.py` implements
`plan`/`apply` as above; the first unit is `src/auto/80040598_fn_80040598.cpp` (`.text`
0x80040598..0x800408A8, 17 functions, 784 B, seam unproven - the region offers no narrow strong boundary
evidence), taking the ledger to **300 covered / 20 224 unclaimed** with the linked DOL unchanged (`ok` green,
0 regressions). Three things the first pass taught, all now in the tool:

* **The partition is evidence-first, not seed-first.** Walking seed by seed with `tudiscover`'s closure turns
  a region with no anchors into one unit per *function* (17 units of 4-40 B here), which is certainly wrong.
  The right shape is: one proposal per maximal unclaimed run, cut only at seams a narrow (<= 4 cuts) strong
  observation pins, then repaired for TU shape - a piece under `--min-bytes` joins its neighbour, a piece over
  `--max-bytes` is split and *flagged as a guess*.
* **A claim the object does not emit is worse than no claim** (playbook 23): so `apply` writes `.text` only and
  prints the data runs it saw as comments, for the measured second pass. `extab`/`extabindex` follow the same
  rule even though they look mechanical - the fragments must pair by (section, offset) or they cost score.
* **Two silent-failure traps, both now loud.** `configure.py` is CRLF and `splits.txt` is LF, so a replacement
  anchored on the wrong line ending does nothing at all; and dtk refuses a `progress_category` that is not
  declared in `config.progress_categories` (`Progress category 'auto' missing...`), which fails the *next*
  `ninja` rather than the registration. Both are asserted in the tool, and `apply` is idempotent (a unit
already in `splits.txt` is skipped).

## The knowledge delta - the playbook is the real product

The campaign's output is not only closed symbols, it is the accumulated *method*. A trick that closed one
symbol here (or cost an hour to find) is worth more to the next 20,000 than the symbol it closed - and it is
worth nothing at all while it lives in a chat message or a subagent's reply. This repository has already lost
a finding that way, which is why "record it in the same session" is a rule in `AGENTS.md`; the campaign makes
it a gate instead of a good intention.

Every iteration owes a **knowledge delta**, and it has exactly four homes:

| what was learned | where it goes | who reads it next |
| --- | --- | --- |
| a matching idea - a code shape, a flag, an allocator rule | a section in `docs/matching.md` (house style: Problem / Why try it / Result / Example) **and** its row in the `AGENTS.md` "Matching playbook" table | every future session, and the `mwcc-unit-matching` skill |
| a workflow or tool fact - a gate, an order, a trap in our own tools | the skill that owns that step (`decompile-symbol`, `tu-boundary-discovery`, `symbol-map-editing`) | every agent running that step, subagents included |
| a unit's own residual - what still differs and why | that unit's file header comment | whoever touches that unit next |
| a campaign mechanic - registration, batching, orchestration | this file | the orchestrator of the next batch |

Three rules make that real rather than aspirational:

1. **A batch is not closed until its delta is written.** Step 4 of the loop ends with the delta, and the commit
   carries it - a batch whose commit holds only sources and ranges is a batch whose lesson is about to be
   paid for again. Write it *before* reviewing the staged diff: that is the moment the numbers are still in
   hand, and the diff is then the proof that the lesson is committed.
2. **The skills' generated references have to be in sync**, because a skill is what a fresh session and every
   subagent actually load:

   ```sh
   python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py           # regenerate
   python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py --check    # exit 1 when stale
   ```

   The day this rule was written, `references/playbook.md` was **55 lines behind** `docs/matching.md`: the
   playbook had grown and the skill had not, so the knowledge existed and no agent would ever have loaded it.
   The check belongs in the same commit as the playbook edit.
3. **A batch that found nothing new says so.** "Nothing new" is a legitimate delta (it means the playbook
   already covered it) and belongs in the local-only block; silence does not, because it is indistinguishable
   from not having looked.

## Parallelising the loop with subagents

Every step above is subagent-shaped, and the campaign is expected to run several at once. The rule is that
**analysis fans out, shared files do not**: a subagent may read anything, and may write only files no other
stream is writing.

| step | what fans out | what stays serial (the parent's job) |
| --- | --- | --- |
| 1 attribute | one subagent per unclaimed address: `symbolpreflight` + `tudiscover at`, returning a *proposal* (unit name, ranges, configure draft) and its evidence. Read-only, so it can run 8-wide. | applying the ranges to `splits.txt`/`configure.py`, and the re-split that follows |
| 2 decompile | one subagent per unit (or per function inside a large unit): the source file is its own; `m2cinput.py`/`m2c` and the Ghidra dump are read-only. | shared headers (one declaration, one place), and any `symbols.txt` rename (`symedit.py` edits one map) |
| 3 match | per unit, using `mt.py diff`/`symdiff` and the unit's own object - no global state. | regenerating `report.json` and `ninja changes`, which see the whole repo |
| 4 commit | nothing; it is one index per worktree. | staging, `prepcommit.py`, and the `AGENTS.md` LOCAL-ONLY round trip |

Practical rules, mostly already in the skills:

* **A worktree per stream** (`git worktree add -b <stream> ../mhtri-dtk.ws-<stream> <base>`) and therefore one
  build tree per stream: two `ninja` runs in one tree race, and `build/RMHE08/` is shared state. If two
  streams must share a worktree, give them disjoint source files and serialise every `ninja` call.
* **Never two writers on one file**: `splits.txt`, `configure.py`, `symbols.txt` and `AGENTS.md` are the
  four files the whole campaign has one of each.
* **A subagent cannot see the parent's Ghidra MCP session** unless it has its own - so hand it the offline
  route (`build/RMHE08/obj/<unit>.o` + `m2cinput.py` + `tools/m2c`), which is why that route is step 2 as
  written rather than an afterthought.
* **Ask for evidence, not prose**: the commands run, the numbers, the first divergence, and the files
  touched - a subagent's report is what the parent verifies with `mt.py diff` and `ninja changes` before it
  is committed under the parent's name. Ask it for **knowledge too**: what it tried that did *not* work is
  often the most valuable line in the reply (it becomes a `no` row or a note in the unit header, so nobody
  tries it again) - and if a worker had to re-derive something the playbook already knew, that is a playbook
  bug, fixed in the same commit.
* **Audit the shared files after every subagent, before trusting its report**:
  `git diff --stat config/RMHE08/splits.txt configure.py config/RMHE08/symbols.txt AGENTS.md`. A read-only
  brief is not a guarantee - in batch 2 a scout added ranges to `splits.txt` (correct, and kept, but it could
  as easily have been wrong) - and the orchestrator is the only writer that can be held to it.
* **Cap the reply.** Ask each subagent for a fixed-size answer ("HARD LIMIT 30 lines, tables only, no command
  transcripts"): the orchestrator's context is what a long campaign spends, and a 200-line report costs the
  next three batches. Ask for the numbers, the one-line evidence, and what it tried that did *not* work.
* **The re-split and the report are the global choke points** (6-12 minutes and a full-repo rebuild). Batch
  the work that needs them: many proposals in, one registration, one re-split, one report - then fan out
  again on step 2.

## The orchestrator

The orchestrator is a **role, not a fixed agent**: by default the main session plays it, and it can be
delegated in turn - one orchestrator per module, reporting to the campaign orchestrator. What has to be unique
is *who writes which file*, not who reads what. The role owns the four shared files, the re-split, the report,
the index and the ledger; what it must not do is the reading-heavy work itself.

**Its loop, once per iteration**

1. `ledger.py` - read the state, never memory: totals, `next N`, and the stale-report flag.
2. Pick the batch: one TU's unclaimed symbols, plus the data ranges that come with it (see "Order of work").
3. Fan out step 1 as read-only proposal subagents and collect their verdicts.
4. Decide. `proceed` is the orchestrator's call; `approve` and `never touch` go into the **escalation queue**
   for the human - in one batch, not one interruption per symbol.
5. Apply the registration itself (the shared files have one writer), then pay the one re-split.
6. Fan out step 2, one subagent per unit, each in its own worktree if it will also run `ninja`.
7. Re-derive every claim cheaply, per unit: `ledger.py unit`, a fresh report, `ninja changes`, `mt.py diff`.
   A number that does not reproduce is not a result.
8. Record the batch's **knowledge delta** (see "The knowledge delta": playbook row + `docs/matching.md`
   section, the skill that owns the step, the unit headers, or "nothing new" in the block), run
   `sync_reference.py --check`, then commit the batch (standing approval) and update the local-only block.
   Back to 1.

**What it has to weigh**

* **The playbook is the compounding asset, and the orchestrator is its librarian.** It owns the
  `AGENTS.md` table and `docs/matching.md`, it refuses to close a batch whose delta is unwritten, and it runs
  `sync_reference.py --check` so the skills keep carrying the knowledge - a stale generated reference is a bug
  that makes every future subagent pay again for something this campaign already knows.
* **Its own context is the scarce resource.** It reads the ledger, subagent summaries, `git diff --stat` and
  the numbers - not source files and not disassembly. Every read-heavy question is cheaper as a subagent's
  answer than as its own context.
* **Evidence over reports.** A summary is a claim; the orchestrator verifies it with the ledger and
  `mt.py diff` before committing it under its own name (skill `objdiff-verify`).
* **The escalation queue** collects the decisions it cannot make: a re-attribution (`approve`), a
  `never touch` owner, a policy question, a batch that would exceed the standing commit approval. Keep working
  on the rest; hand the queue over as a list with numbers.
* **Serialisation**: one writer for `splits.txt`/`configure.py`/`symbols.txt`/`AGENTS.md`, one `ninja` per
  build tree, one re-split at a time. The batch should therefore be as large as is safe - the re-split is the
  campaign's slowest step (**~200-400 s** measured; the link after it is ~75 s, and `docs/build-performance.md`
  has the breakdown and the knobs). Two things follow from that number, and both are the orchestrator's call:
  * **Renames and phantom merges ride the batch too.** A `symbols.txt` edit is itself a split dirty-check
    input, so N renames applied one at a time cost N re-splits while the batch re-splits once. Collect them
    with `tools/symbols/symedit.py rename-batch <file>` (one `old new` per line, per-line failure report) and
    re-split once - the same "one re-split per batch" rule step 3 of the attribution method already follows.
    The price is that objdiff verification of a rename moves to the batch boundary: verify every renamed
    symbol after the batch's split, before the commit.
  * **The asm dump is on demand, not on every split** (`config.yml`'s `write_asm: false`): only
    `tudiscover` reads `build/RMHE08/asm/`, so run `python tools/splits/dump_asm.py` once before an
    attribution session and let `tudiscover stats` tell you if it has gone stale since.
* **Blast radius**: everything lands `NonMatching`, so a bad batch cannot break the link - but it can break
  the *next* session's ability to measure. A half-registered unit, a `splits.txt` range with no `configure.py`
  entry, or a report that was not regenerated is what actually stops the loop.
* **Handover**: before a compaction or the end of a session, the local-only block says which batch is open,
  the ledger is the state, and anything worth keeping is in `docs/`, a skill or `AGENTS.md`. A finding that
  lives in a reply is lost.

**When the orchestrator delegates itself**

* **Per module**: a module with hundreds of symbols (`Pl/` is 189 registered already) gets its own
  orchestrator, worktree and commit stream; the campaign orchestrator then owns the plan, the four shared
  files and the cross-module view.
* **Per step**: a registration orchestrator that only applies proposals and re-splits, alongside one that
  only orchestrates decompilation - worth it when one step is the bottleneck (the report and the re-split
  usually are).
* **Never** two orchestrators writing one registration file, and never an orchestrator that commits from
  another's index - the index belongs to the worktree.

**Briefing a subagent** (the shape that keeps verification cheap)

* State the *one* artefact it returns, the read-only inputs it may use, the number it must report, and the
  files it may not touch (`splits.txt`, `configure.py`, `symbols.txt`, `AGENTS.md`, any `ninja`, any commit).
* Step 1: "run `symbolpreflight` + `tudiscover at` on `<address>`; return the verdict, the proposed unit path,
  the exact `splits.txt` block and `configure.py` entry, the evidence for each range, and what you could not
  decide."
* Step 2: "write `<src/path>` (+ a header only if a declaration is shared); return the file, the compile
  command and its output, the per-symbol score from a fresh report, the first divergent instruction, and the
  residual paragraph for the file header."

## Order of work

* **Ascending address**, because the DOL's layout groups a module's code and data: a TU discovered at one
  address usually runs into the next unclaimed symbols, and `tudiscover` works from an address.
* **Finish a TU before starting the next**, so one re-split covers all of its functions.
* **Size the batch by the re-split, not by a symbol count.** Register a whole island once - the range, and
  the `extab`/`extabindex`/data fragments that come with it - then fill its functions over several batches:
  a source-only change needs no re-split at all. Batch 2 registered `main.cpp` (38 functions, 4448 B) in one
  step and wrote 20 of them; the remaining 18 are pure source work on an already-claimed range.
* **Calibration first**: the first ~20 symbols should be small ones with known names (the runtime dump has
  real SDK names for a good fraction of them). They validate the loop and the 80 % bar cheaply; a
  `fn_*` blob nobody can name is the worst place to start.
* **Milestones**, recorded in the ledger output, not in prose: every 100 closed symbols, and every module's
  first unit. Re-check `ninja changes` at each milestone - a systematic regression is much cheaper to find
  then than 500 symbols later.

## Risks, and where they are already written down

| risk | where it is handled |
| --- | --- |
| reaching for flags to fix what is really source shape or liveness | playbook 13; `mwcc-unit-matching` |
| a `Matching` flag on a wrong object | non-negotiable 4; proof procedure in `objdiff-verify` |
| claiming a data range that lowers a function | playbook 23; measure before/after |
| a rename that splits the map and the source | skill `symbol-map-editing` (two edits, one change) |
| a stale report or a hand-written compile | `decompile-symbol` §5; `objdiff-verify` |
| four agents editing one file | one worktree per stream; `prepcommit.py` stages explicit paths |
| committing the local-only block | non-negotiable 8; `prepcommit.py`/`localonly.py` handle it |

## What "done" means

* Per symbol: attributed, decompiled, ≥ 80 %, committed, and counted by the ledger.
* Per batch: every symbol it set out to close is closed (or recorded `partial` with its residual), its ranges
  measure no worse than before, and its **knowledge delta** is committed - a playbook row, a skill line, a unit
  header, or an explicit "nothing new".
* Per unit: every symbol it owns is closed (or recorded `partial` with its residual), and its ranges measure
  no worse than before.
* Campaign: `python tools/units/ledger.py` reports no unclaimed symbol, and the remaining sub-100 % units
  are an explicit, listed second-pass queue - each with a residual in its own file header comment.
