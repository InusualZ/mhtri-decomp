# AGENTS.md

Guidance for AI agents (and humans) working in this repository.

## What this repository is

A **matching decompilation of Monster Hunter Tri** (Nintendo Wii, USA, disc ID **`RMHE08`**), built on the
[decomp-toolkit](https://github.com/encounter/decomp-toolkit) project template ([`encounter/dtk-template`](https://github.com/encounter/dtk-template)).

"Success" here means one specific thing: **the C/C++ source in `src/` recompiles to code that links into a
`main.dol` byte-identical to the original game**. It is not enough for code to compile, look correct, or
produce the same output — it must match the original instructions, relocations and section layout.

The original binary is split into relocatable objects by decomp-toolkit (no hand-written assembly, no game
assets in the repo), and the final `main.dol` is verified against `config/RMHE08/build.sha1`.

* Original binary: `orig/RMHE08/sys/main.dol` — SHA-1 `BF4850739478CAAEDFE675949EB7C28595A7FDE9`
* Current state: `src/` holds many registered units (see `docs/plan.md` §2 for the measured totals); the
  `src/auto/` units are moved to their final homes under the register-once rule (`docs/plan.md` §12), and a
  region still unclaimed in `symbols.txt` is a proposal backlog, not a defect.

## Non-negotiables

1. **Never modify `orig/RMHE08/**`.** It is the original game data and the ground truth for every diff.
   Read-only, always. It is **gitignored**, so nothing in git can restore it: never run a repository-wide
   clean (`git clean -xdf`, `rm -rf orig`, a "reset the tree" recipe) in this repo, and never let a subagent
   do it either. On 2026-09-26 `orig/RMHE08/sys/main.dol` and `orig/RMHE08/files/*.sel` were gone from MAIN
   and the build could not regenerate `build.ninja` (`orig/RMHE08/sys/main.dol not found`); they were restored
   from the safety copy at `../orig-backup/RMHE08/` and checked against the documented SHA-1
   `bf4850739478caaedfe675949eb7c28595a7fde9`. When the build reports the DOL missing, restore from that copy
   first - `sha1sum orig/RMHE08/sys/main.dol` must print that hash.
2. **Never commit build output or original files.** `build/`, `orig/RMHE08/**` (except `.gitkeep`),
   `*.dol`, `*.rel`, `*.elf`, `*.o`, `*.map`, `objdiff.json` and `compile_commands.json` are gitignored —
   keep it that way.
3. **Do not edit `configure.py` compiler flags, `mw_version` values or tool version tags to make something
   build.** Those settings change codegen for every translation unit. Changing them is only acceptable with
   concrete evidence (an instruction/size diff that points at the flag), and must be called out explicitly.
   See "Gotchas" for the incident that motivates this rule.
4. **Only mark an object `Object(Matching, ...)` when it actually matches.** Otherwise use `NonMatching`.
   A wrong `Matching` flag breaks the final DOL hash for everyone.
5. **Don't rename or delete symbols that already exist in `config/RMHE08/symbols.txt`** unless you have
   verified nothing else depends on them. Symbol names are referenced by `splits.txt`, the linker script
   and the analysis output.
6. **Commit only what the task covers, and never push.** History is never rewritten (`rebase`,
   `commit --amend`, `reset --hard`, force-push, deleting/moving tags) and nothing is ever pushed - `origin`
   is the upstream template, not a fork of this project. On **commit approval**: the project owner has granted
   the campaign's orchestrator **standing approval to commit its own work without asking per commit**
   (2026-09-21, docs/plan.md, "Commits"), so a `docs/plan.md` batch ends by committing - that is not a licence
   to commit *anything*: an experiment, a probe, a half-registered unit, another stream's file or a scratch
   artifact still stays uncommitted, and anything outside the campaign keeps the old rule (leave it in the
   working tree and report what you would commit).
7. **Never paste `config/RMHE08/symbols.txt` into a prompt/tool output.** It is ~65,700 lines / 4.5 MB.
   Grep it, slice it, or use `dtk`/objdiff; do not print it.
8. **Never commit the local-only block in this file.** Everything between `<!-- LOCAL-ONLY-BEGIN` and
   `<!-- LOCAL-ONLY-END -->` (the `## Current task / plan` section) is live agent working state, not repo
   content. **`land.py` handles it for you**: when AGENTS.md is in the batch it runs `localonly pull` before
   the commit and `push` in a `finally`, and it tells the block apart from a real edit
   (`agents_md_real_change`). So **never run `localonly.py pull` + `git checkout -- AGENTS.md` by hand** -
   that checkout silently reverts the real AGENTS.md edit you are landing; land the batch and let the tool
   do it. **A plain `git commit` is safe too** (2026-09-28): the pre-commit hook pulls the block out and
   re-stages AGENTS.md, and `post-commit` pushes it back (both in `tools/git/hooks/`, and the pending state
   is `.git/localonly-pending`); if the commit fails, the hook prints the one command that restores the
   block. Use the tool directly (`python tools/agents/localonly.py pull|push`, skill: `agents-md-local-only`)
   only for a manual AGENTS.md commit, and verify a revision with
   `git show HEAD:AGENTS.md | grep -c '^<!-- LOCAL-ONLY'` → `0`. (This rule's own prose mentions the
   markers, so anchor the match at line start; the tool matches whole marker lines for the same reason.)

## Matching policy: flags and source variants

Two working rules that apply to **every** unit, agreed with the project owner:

1. **Apply the best-scoring variant even if it is not a full match.** A source rewrite or flag change is
   worth landing as soon as it *measurably improves* the objdiff score (`ninja build/RMHE08/report.json`,
   per-symbol `match_percent`) and regresses nothing else. Do not hold a real improvement back waiting for
   100 %: record the residual diff (what still differs and why) **in the unit's file header comment**, so
   the next reader finds it where the code lives. The residual belongs in that one place, never as a
   per-function comment - see "Commenting and naming" under Conventions for what a function comment is for.
   Never land a change that makes any function worse. Landing means the unit's own source or
   `configure.py` carries the change and the repository rebuilds better - the skill does it with
   `python .agents/skills/mwcc-unit-matching/scripts/mt.py variants --apply <name>` followed by a forced
   rebuild; a probe-only winner is not progress.
2. **Evidence-backed flags live in `configure.py` as soon as they are proven**, even while the unit is
   still short of 100 %, so the repo always reflects the best known state. They must be **per library**
   (never edit `cflags_base`/`cflags_runtime` for everyone) and must be called out explicitly, as
   non-negotiable #3 requires, with the instruction/size evidence in a comment next to them.

A unit's flag evidence belongs next to its definition in `configure.py` (a per-library `cflags_*`
override below `cflags_runtime`), not in this file; this file only carries the policy.

The *how* - the ideas, the problem each one solves and whether it has been tried - is the playbook index
below.

## Operational mode: production runs

How the campaign is run while a batch of units is being produced (owner's instruction, 2026-09-24). It
supersedes "keep the queue full": the aim is **steady** throughput, not maximum throughput.

* **Six worker subagents at most, and that budget covers everything** - unit workers and tool fixes share it.
  Five units plus one fix is fine; six units plus a fix is not.
* **A slot is not filled while finished work sits unlanded** (owner, 2026-09-26). `queue.py next` refuses to
  claim a unit when any local branch holds content `main` lacks, and `queue.py list` reports the same state.
  The test is narrow - a file counts only when main's copy is a *strict subset* of the branch's - so the
  older pad names and comment wording every landed branch leaves behind do not block, and it is
  content-based, not commit-based, because the landing recipe cherry-picks content and so leaves every
  worker branch ahead of main by commits after its unit has landed. Remedy: land it (docs/plan.md §12's
  recipe), or - once the audit shows the delta is stale - `git worktree remove <wt> && git branch -D
  <branch>`. `--allow-unlanded <branch>` parks one on purpose.
* **A slot goes to a problem before it goes to a new unit.** When something breaks - a gate refusal, a blocked
  or waiting worker, a tool that cannot express what the work needs, MAIN's HEAD on the wrong branch, a hole
  in the queue - the next free slot fixes that. Only when nothing is outstanding does a freed slot take a new
  proposal.
* **Refill one slot per completion, not in waves.** Claim with `queue.py next`, launch that one worker, and let
  its completion free the slot. The 12-13 worker waves are what produced the repair work this policy exists to
  avoid: unions mangling struct bodies, declarations whose owner registered mid-wave, an empty `--message`.
* **When a wave is claimed, claim it with `queue.py next --count N` - never N adjacent proposals.** The
  stride takes `i, i+N, i+2N, ...` in the queue's address order, so no two workers hold adjacent proposals.
  Adjacency is the vector for almost every clash this campaign has had: it is what puts two workers on one
  translation unit (`proposal/8007270C` + `proposal/80073180`, both `g3d_calcvtx.cpp`), it is what produces
  the rule-2 boundary artefacts (a neighbour registers a symbol you declare), and neighbouring units share
  owner headers and types by construction. The stride makes that **provable** - two adjacent proposals can
  share a wave only if both indices are congruent mod N, impossible for N > 1 - where random selection would
  only make it unlikely (both halves of one TU in a wave about once in N tries). The caveat is locality:
  spreading costs cross-unit knowledge reuse, so prefer the spread *within* a band that the address order
  already gives, and do not defeat it by re-sorting the ready set (by score or otherwise).
* **Land one unit per commit, one unit at a time, from `main`.** Check `git rev-parse --abbrev-ref HEAD` prints
  `main` before landing - `land.py` now refuses otherwise, because a batch landed off `main` puts its commits
  on the wrong ref and slides the merge-base that `applybranch.sh` and the gate both resolve against.
* Keep `ninja build/RMHE08/ok` green and `orig/RMHE08/**` untouched as the invariant of every step (see
  Non-negotiables).
* **A lane is launched with the profile that matches its job - not with the generic `worker` (owner's
  instruction, 2026-09-26).** The project defines **four** profiles in `.agents/agents/` (tracked):
  **`decompiler`** is *unit work* - register a proposal at its final home and reconstruct its bodies;
  **`fixer`** is a refused gate or a measured regression on a branch; **`merger`** is a refused *apply* -
  two lanes' views of one record or type, and a fold (the `recordmerge.py` class); **`codereviewer`** is
  read-only review of source against section 6.5 and the conventions. `worker` stays the fallback for a task
  that is none of those, and `scout`/`planner`/`reviewer` are the read-only globals. `queue.py next` emits
  the profile in its paste-ready spawn (`agent: "decompiler"` for a proposal lane) and takes `--profile` for
  the rest; the roster is `docs/plan.md` 5.4.1. A lane launched with the wrong profile is not
  a cosmetic mistake: it is missing the rules its job is held to.
  **The launcher now decides both**: `python tools/units/slots.py spawn --kind KIND [--slot N]` maps
  `unit`->`decompiler`, `fix`->`fixer`, `merge`->`merger`, **`tooling`/`docs`->`worker`**,
  `review`->`codereviewer`, and `scout`/`plan` to the read-only globals; it takes a pooled slot by number and
  prints the paste-ready launch line with the "your tree" block. An unknown kind is refused rather than
  guessed. So a tooling or docs lane is a `worker` lane - launching it as `decompiler` hands it unit policy
  it can never satisfy, and a review lane as a writer risks the tree it reviews.
* **Land the orchestrator-side batch first: `land.py land --already-applied` stages what it finds in MAIN.**
  Measured 2026-09-26: an uncommitted `AGENTS.md` + `docs/plan.md` pair rode the `OS/FindContainHeap_.c` unit
  commit (`4fad00522`), because the gate's commit-sweep guard protects a path only when the base's
  `dirty_at_base` snapshot recorded it *and* the batch does not name it - it is a guard, not a guarantee, and
  it does not stop a tracked file that is dirty at `record-base` time. Same class as the `docs/plan.md` edit
  that rode unit commit `85ddd7b6`. Land the tools/docs batch first, then the lanes.

**KNOWN BUG, measured 2026-09-24 (booked a slot):** `queue.py next` offered `proposal/80063888_fn_80063888` a second
and third time while that proposal was already claimed and its worker was running, refusing each time with
"branch worker/80063888-fn-80063888-b806 already exists - the unit is claimed (or was never released)". So a
claimed proposal is still `ready` as far as selection is concerned. This is the pool re-hand bug on the *claim*
axis rather than the *covered* axis. Until it is fixed, `queue.py next` can waste turns refusing at random;
claiming with the claim registry checked first (`claims.py list`) is the workaround.

The steady loop, per unit:

1. `queue.py next` claims one proposal - one worktree, one branch, one brief - and prints the paste-ready spawn.
2. The worker registers its range at its final home and commits the bodies on its branch, measured.
3. Apply that branch with `.pi/bin/applybranch.sh` (the merge-base diff; the local-only block records why the
   two obvious alternatives lose work), and resolve the shared-file conflicts: `python
   tools/units/mergebranch.py resolve` resolves the classes it can (a comment that names a unit **file** is not
   a stale symbol; a comment-only `src/**` difference takes main's comment and the branch's code) and refuses
   the rest. Then `land.py record-base` -> `land.py land --units <claim>` -> `claims.py release`.
4. `ninja build/RMHE08/ok` green, then refill exactly that one slot.

**The loop closes with a review: `decomp -> review -> decomp`.** A committed branch is reviewed **before** it
lands, by a read-only review lane (`.agents/agents/codereviewer.md`, launched with `python
tools/units/slots.py spawn --kind review`), against the branch's own diff: `git diff main...<branch>`,
`python tools/units/stylelint.py --ref <branch>` (it judges a branch's committed tree without checking it out)
and per-function objdiff. The reviewer judges the dimensions its profile lists - honesty of the match claim
(a `.text` match is not a claim about the *object*), naming, placement, types, comments, codegen hygiene -
and it is a real second pair of eyes, not a re-run of the gate: it reads the source a human would read. Its
findings come back **itemised**, and they are then cleared **on the same branch** by a follow-up decompilation
pass (a `fixer` lane, or the decompiler lane that wrote it). Only then does the landing gate run. A review
that finds nothing says so explicitly - for a well-measured unit that is the common answer, and it is what
makes the loop worth its cost.

Two tool behaviours the loop leans on, both fixed this session: `queue.py` never offers a proposal whose range
a registered unit already covers, so re-attributing a region cannot re-hand landed work; and
`attribute.py queue <start> <end>` **rewrites** the queue file with only that region's proposals rather than
appending - run it over the whole unclaimed region (the file records this as `cap: 0`), or the rest of the
backlog disappears.

## Matching playbook (index of `docs/matching.md`)

`docs/matching.md` is the playbook for making a unit match its original object. Every idea in it is
indexed below. **The index is generated** from the playbook by `tools/agents/sync_playbook_index.py`, so it
cannot describe an idea the playbook does not hold and a duplicate section number is refused rather than
listed twice: the number is the section in `docs/matching.md`, the row is that section's title, and the
problem column is the opening of that section's own problem sentence (truncated at 220 characters). Only
**numbered** ideas get a row - a short unnumbered section (the paired-single note, the RSO worked example)
does not. Read the index as the map of what is already known when a unit is opened; the *work* of matching
one unit is "The core loop" below.

The same method is packaged as a project skill, `.agents/skills/mwcc-unit-matching/` (tracked - the
`.gitignore` excepts it), so an agent can load it on demand instead of reading the playbook every session: `SKILL.md` holds the loop and the idea list,
`references/` is *generated* from `docs/matching.md` (never edit it - run
`python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py`, or `--check` to detect staleness),
and `scripts/mt.py` forwards to the `tools/` helpers (`units`, `info`, `frames`, `matrix`, `sweep`,
`variants`, `shapes`, `diff`, `slots`, `sections`, `dwarf`). The playbook - not this index - is the
authority; the
index is derived from it.

All project **subagent profiles** live in `.agents/agents/` (tracked), discovered by the harness as *project*
agents. `decompiler.md` is the unit-work role: it inherits this file (`inheritProjectContext: true`), loads
the matching/verify/registration skills (`skills:`), and carries the rules that used to be hand-typed into
every launch. When a launch prompt and the profile disagree, **the profile wins** - so the rules live there,
not in the prompt. Which profile a lane gets, and why it is not a cosmetic choice, is the rule in
"Operational mode" below. A profile edit is not live until `tools/agents/install.sh` has copied
`.agents/agents/*.md` to `~/.pi/agent/agents/` (it refuses when the section 6.5 block is stale), because the
harness reads the installed copy - a worktree cut before the edit otherwise serves the old prompt.

All project skills live in one tracked folder, `.agents/skills/`, so every harness sees the same set:
`mwcc-unit-matching/` (this playbook), `symbol-map-editing/` (`tools/symbols/symedit.py` - look up, list by
range and rename symbols without ever loading `symbols.txt` into context), `agents-md-local-only/`
(`tools/agents/localonly.py` - pull the local-only section out of this file before a commit and push it
back after), `objdiff-verify/` (proving a unit really matches), `tu-boundary-discovery/`
(`tools/splits/tudiscover.py` - from one symbol address, work out which functions and data ranges form one
translation unit, before any source is written) and `decompile-symbol/` (`tools/units/symbolpreflight.py`,
plus `tools/units/m2cinput.py` for the `tools/m2c` decompiler - one symbol from an address to a registered,
measured unit).

<!-- PLAYBOOK-INDEX-BEGIN - generated from docs/matching.md by tools/agents/sync_playbook_index.py; do not edit by hand -->
Every **numbered** idea in the playbook is listed below - a short unnumbered section (the paired-single
note, the RSO worked example) has no row of its own. The number **is** the section in
`docs/matching.md`, the idea column is that section's title, and the problem column is the opening of
that section's own problem sentence, truncated at 220 characters - so this index cannot describe an idea the playbook does not have, and a duplicate section number is refused rather than listed twice.

| # | idea | problem it solves |
| --- | --- | --- |
| 1 | Use a per-unit instrument, not the project-wide check | The project-level pass/fail signal is useless while any object is `NonMatching`: `ninja build/RMHE08/ok` cannot pass, and the progress report's `complete_code_percent` says 100 % even when the code is wrong. A unit can... |
| 2 | Read the *first divergence*, never the percentage | `match_percent` is positional: one inserted or deleted instruction shifts every following instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as a function that is... |
| 3 | Read the target's disassembly, not just the diff | The diff shows *what* differs, not *what the original source looked like*. Several flags are only discoverable if you already know the target's code shape. |
| 4 | Ignore version/`.comment` hints; make codegen the oracle | A unit's `.comment` section and `mw_comment_version` in `config.yml` look like a compiler fingerprint and invite "we must be using the wrong compiler version". |
| 5 | Sweep one flag at a time, against the project's own command line | Several flags are usually in play at once (`-O4,p`, `-inline auto`, `-use_lmw_stmw on`, `-Cpp_exceptions off`, `-str ...,pool,...`), and it is unclear which single one explains which symptom. |
| 6 | Guard against stale objects when scripting the compiler | A scripted matrix run can report all compilers producing *identical* output - which cannot be true. Every run was diffing the same stale object, one the compiler never wrote. |
| 7 | Use scratch files to attribute an instruction choice | Two candidate source idioms produce visibly different instructions, and the full-unit diff cannot tell you whether the difference comes from the source idiom or from an optimizer pass. |
| 8 | Ask the compiler which optimizations are actually on | After a long sweep it is still unclear what a flag *set* resolves to: `-O3` and `-opt level=3,peephole` are the same thing, and one `-O` level can set five switches at once. |
| 9 | Enumerate the option space from the compiler, not from memory | Guessed spellings waste runs, and worse, they lie: some invented keywords are **silently accepted and ignored**, so "no effect" looks like evidence when it is noise. Others do not parse at all (an `-O` level cannot... |
| 10 | Do not use frame size as a success signal | When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant that produces the target's frame - and several do. |
| 11 | Do not read a large size gap as "different source" | The target object is often hundreds of bytes bigger than ours, and several functions sit at 0 %, which reads like the original source containing code ours does not have (different unrolling, extra rounds, extra... |
| 12 | Check that the flag's side effects still link | Some flags change the *relocations*, not just the instructions: a save idiom that calls runtime helpers, or a table base that changes how a symbol is addressed. Matching the code shape while referencing a symbol the... |
| 13 | Stop when the remaining diff is no longer flag-shaped | After the flags that clearly apply are found, a near-miss variant often remains: a flag combination that fixes one visible symptom (a frame size, a single instruction) while leaving the code different. |
| 14 | Prove the committed flag list reproduces the proven object | The flags eventually get written into `configure.py` as a per-library override. A hand-written list can silently differ from the command line that was tested - a leftover `-O4,p`, a duplicated `-inline auto`, a flag... |
| 15 | Pin a metric that does not drift | Per-symbol fuzzy percentages change between tool versions: the same two objects can score 82.72 % with one `objdiff-cli` build and 99.83 % with another. A decision made against one number is meaningless against the... |
| 16 | Scope optimizer settings per function with pragmas | The optimizer levers are command-line flags, i.e. global: `-opt level=4` fixes one function's frame but reorders instructions elsewhere, so a per-function codegen difference looks unreachable from the source side. |
| 17 | Cross-family version matrix: a unit's compiler is per unit, not per project | `mw_version` is set once per library in `configure.py` and every unit inherits the same one (the template default here is `Wii/1.3`). Prebuilt SDK libraries in particular were compiled by Nintendo with whatever compiler... |
| 18 | Named temporaries, declaration order and operand order steer the allocator | Once the opcodes, sizes and relocations all match, the residual is often nothing but register numbers, and it looks unreachable from the source side. |
| 19 | Loop shape decides the loop idiom | A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested loop; the wrong idiom adds or removes instructions and moves every later register. |
| 20 | Force a loop-invariant address through a `u32` local | Retail materialises a loop-invariant field address into a callee-saved register in the loop preheader (`addi r30,r26,84` then `lwz r0,0(r30)`); our build folds it into a load displacement (`lwz r4,0x54(r27)`). That... |
| 21 | Count record-form instructions to fingerprint peephole/scheduling per unit | The flags of one unit were inferred for the whole binary from "the DOL contains no `extrwi`" - wrong twice over: it is not a whole-binary property, and the detector could not see what it was looking for. |
| 22 | When retail's colouring is your exact mirror, stop | A residual that is nothing but register numbers invites another hundred source variants, each of which costs a compile-and-diff cycle and none of which can be reasoned about. |
| 23 | Data ranges in `splits.txt`: what objdiff can and cannot fix | A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range is not claimed in `splits.txt` (`@1841_80629B90` in the target vs our `lbl_80629B90`), so it looks like a one-line fix. |
| 24 | Merging a probe into the unit is its own step | Probes are measured standalone, in their own translation unit, so their numbers are not the unit's numbers - and a probe cannot see the unit's types. |
| 25 | Use the shared memory dump as a name/signature/struct oracle | Before a function can be matched it has to be *understood*, and this repo's `symbols.txt` has thousands of `fn_XXXX` names and no types at all. |
| 26 | The target's section is part of the match | A unit can be instruction-identical and relocation-identical and still measure as *unmatched*, because the code landed in the wrong section. The compiler emits `.text` by default; the map and the split object may say... |
| 27 | Same instructions, different order names the `-O` level - probe both, per unit | The instruction multiset and the relocations agree, but independent instructions are swapped or split across registers - typically the epilogue's `lwz r0,0x14(r1)` (LR reload) and the function's real last load. It reads... |
| 28 | A kept `bl` to a tiny static names the unit's inlining setting | The target calls a small file-local function the source could just as well inline (`bl fn_8003F554`), while our build inlines it away - the callee has no counterpart in our object and the caller comes out an instruction... |
| 29 | A claimed literal pool: declare the constants, never define them | A unit whose `.sdata2`/`.sdata` fragment is claimed in `splits.txt` still shows its pooled constants as `ARG` rows - the target loads `lbl_80795AC0@sda21`, our source loads a literal the compiler put in a pool of its... |
| 30 | A C++ unit's exception settings live in its object, not in the source | Every function of a unit matches instruction for instruction, and the *unit* still measures short because its target object carries `extab`/`extabindex` while ours carries none, or carries different records. It reads as... |
| 31 | A function unpaired by name measures 0 %, not 60 % | A unit's functions are written and their bytes are right, and the score stays near zero. The instinct is to re-read the code - the wrong place, because objdiff pairs functions **by symbol name**. |
| 32 | A pragma region is not local to the functions it covers | A function whose residual is a missing instruction - a `lis` the build CSE-ed away, a base re-materialised at a merge - needs a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it, but the *reset*... |
| 33 | Prefer the unit's flags over a per-function flag - a TU was compiled once | A function that will not match invites a scoped pragma (`optimization_level`, `peephole`, `scheduling`) aimed at that function alone. Three wins this session did exactly that. |
| 34 | A switch tail's constant returns are if-converted, so write the arms negated | A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as `return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours returns early per... |
| 35 | A dead copy chain steers the allocator's web priority | The residual is two live ranges sharing one register pair - retail colours them one way, we colour them the mirror - and no source shape, type, cast, statement order or flag moves it. `Pl/pl_master`'s `fn_8026F908` sat... |
| 36 | A flipped unit's unreferenced trailing function is trimmed by the linker | The unit's object is byte-identical, `flipcheck.py` is happy and `linkorder.py` says `LINK OK` - and the flip still breaks the DOL, by exactly the size of the object's *last* function, with every later section shifted.... |
| 37 | A switch's `default` arm goes first in the source | A `switch` whose default dispatches into a helper comes out a few bytes too big - 364 against the target's 360 - with the default body sitting in the middle of the compare chain and the function stuck around 94 %, even... |
| 38 | An `s16` parameter with a compound assignment is what makes a field store raw | A store into a narrow field (`u8`/`u16`) comes out masked - a `clrlwi` before the `stb` - where retail stores the value as it stands. The function sits at 82-94 % with one extra instruction and every later register... |
| 39 | A unit whose retail code keeps unfused peephole folds needs the peephole pass off | The unit's retail object keeps instructions our peephole pass folds away: a masked `clrlwi` before a narrowing store, a separate `clrlwi`+`cmpwi`, a record-form `clrlwi.` the target does not have, or an `li r0,<slot>` +... |
| 40 | A unit whose retail code keeps unfused multiply-adds needs `-fp_contract off` | Retail keeps `a*b + c` as two instructions (`fmuls` + `fadds`/`fsubs`) where our default `-fp_contract on` emits one fused `fmadds`/`fmsubs`, so the function is a few instructions short and every later register shifts.... |
| 41 | `#pragma optimization_level 1` does not turn the peephole off | A unit needs the peephole pass off and `#pragma optimization_level 1` looks like the way to get it: `-O1` resolves to `-opt level=1`, and the level's switch set reads as if it includes the peephole. It compiles, the... |
| 42 | A C++ free function needs `extern "C"` so objdiff can pair it by name | A `fn_*` function defined in a `.cpp` file measures 0 % while its bytes are right: MWCC mangles the free function (`fn_80073398__FP9ResHandle`) and objdiff pairs symbols by name, so neither side pairs and the function... |
| 43 | Retail's per-string `lis`/`addi` addressing means the unit was built with `-pool off` | Our string literals are addressed through one `@stringBase0` base register (one `lis`, then `addi` displacements) where retail materialises each string with its own `lis`/`addi` - 0x20 bytes of `.text` short, and the... |
| 44 | A string pool in `.data` means the build was not `-str readonly` | The unit's retail string pool sits in `.data`, but our `cflags` carry `-str reuse,pool,readonly`, so our strings land in `.rodata` and the section does not pair. It reads as a missing data range. |
| 45 | A hand-written string literal's `\n` becomes CRLF on this host | A data range's string pool cannot be written in-source because MWCC on this host translates the `\n` in the literals to CRLF, so the emitted bytes do not match the DOL's LF. It reads as a data-claim problem and invites... |
| 46 | A flipped unit's `.ctors$10` fragment is reordered by the linker | A unit's object is byte-identical, `flipcheck.py` says READY, and the flip still breaks the DOL on the unit's `.ctors$10`/`.dtors$15` words, shifting the merged `.ctors`/`.dtors` tables. It reads as a linker/ordering... |
| 47 | Automate the shape search: generate, compile, score and rank source variants | Every near-match residual in this project has been *codegen* - an allocator web order, a branch direction, a register colouring - and the source shape that reproduces it was found by hand: the outboxes record "~200... |
| 48 | A C++ unit's unmangled map name is not a reason for `extern "C"` | The unit is C++ - a `__FILE__` string names a `.cpp`, or a callee is mangled - but the symbol map spells its symbols unmangled (`fn_800CCCF8`), because dtk could not demangle them. objdiff pairs by symbol name, so a C++... |
| 49 | `extab` in a no-exceptions lib is a cheap C++ language signal | A unit's language has to be decided from evidence, not from convenience (`docs/plan.md`, "The language comes from the symbol"), because the extension decides the front-end (`-lang`) and the name objdiff pairs by. The... |
| 50 | An already-mangled map name must not be declared as a C++ identifier | The map carries a *real* C++ mangling - `Panic__Q24nw4r2dbFPCciPCce`, not a `fn_XXXXXXXX` placeholder - and a C++ source declares it with that spelling as an identifier: |
| 51 | A shared type, an extern and a mangled name each have one owner | Three defects are the same mistake in three shapes: an identifier that belongs to someone else is spelled out locally instead of reached through its owner. A type two units share is **copied** into each unit's source... |
| 52 | A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance | A unit whose registered ranges contain a vtable can score 100 % while its source only *views* that table (a struct of function pointers, a cast `extern`). For a `NonMatching` unit the bytes come from the DOL, so nothing... |
| 53 | A sparse switch's jump table is readable once its `.data` range is claimed | A unit whose switch has a compiler-emitted jump table measures just short of 100 % and its arms are unreachable from the `.text` alone: the table lives in `.data`, which the unit's `splits.txt` does not claim, so the... |
| 54 | Dolphin's `.map` names a jump table's OWNER and a `__FILE__` emitter | Section 53 says to claim a jump table's `.data` range, but not *whose* range it is - so the claim is a guess at the boundaries, and two lanes can claim one table or split one TU's data across two units. Separately, the... |
| 55 | An odd-start section claim cannot be linked with MWCC's alignment | A unit whose object is byte-identical to its target still refuses to flip: `flipcheck.py` says READY, the bytes match, and `ninja build/RMHE08/ok` still fails with a DOL in which the tables are four bytes late. It reads... |
| 56 | Two lanes' views of one work record are merged by tiling, not by choosing a side | Two lanes register neighbouring bands that both take the same work record, so both write `include/<area>/<rec>.h` - and the second landing hits an **add/add conflict on a file that already exists in `main`**. Neither... |
| 57 | A call-site mask means the callee's parameter is declared wider than the value | A wrapper (or any caller) sits at 50-90 % and the first divergence is one instruction at the `bl`: retail masks or sign-extends the argument (`clrlwi r4,r4,16`, `extsh r5,r5`, a `slwi`/`srawi` pair) and ours passes it... |
| 58 | A compiler-synthesised pool entry can be claimed only while your unit is its sole referencer | A unit's `.text` matches 100 % and it still cannot flip, because its object emits an 8-byte `.sdata2` entry - MWCC's implicit int->double magic (`0x4330000080000000`), which the compiler pools per TU - that the target's... |
| 59 | A flipped unit's `extab`/`extabindex` entries must carry the map's names, and a global binding | A C++ unit whose object is byte-identical to its target - every section compared, `flipcheck.py` saying READY, `extab` and `extabindex` included - cannot link once it is `Matching`: |
| 60 | The declaration set is part of the codegen - fold a declaration only where one is deleted | A unit sits at 100 %, you add one `#include` of a header that declares a callee the file already sees through another header, and a function drops by a hair - `fn_800CC5B0` went 96.79185 -> 96.78541 with a... |
| 61 | A kept `bl` inside one function: scope `#pragma dont_inline on` to it | A `switch` case calls a small file-local helper that retail keeps as a `bl`, but our `-inline auto` folds the callee's body into the case: the kept call disappears, the case grows by the callee's size and every later... |
| 62 | A `new` expression is not `operator new` plus a constructor call | `NetworkWiiMediator`'s `initializeNetworkMediator` measured 1.89 % and `reflectInit` 88.34 %, and the first divergence was one instruction from the top: retail has `bl __nw__FUl; mr r31,r3; cmplwi r3,0x0` where ours had... |
| 63 | A local's DECLARATION ORDER colours registers - locals are coloured before parameters | `NHTTP/NHTTP_bgnend`'s `NHTTPi_Startup` measured **0 %** with an instruction stream that was otherwise byte-equivalent: the message-group address was materialised late (`lis r31`) where retail materialises it first... |
| 64 | An unknown-size `extern` array is addressed absolutely - give it its size for the SDA form | A band declares `extern const char lbl_80793998[];` and the caller materialises the address with `lis r5, sym@ha` + `addi r5, r5, sym@l`, where the target has a single `li r5, sym@sda21`. The extra instruction shifts... |
| 65 | A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured | `Network/fn_8041A87C`'s `profile_4485` is five `u32` starting at an **odd** offset (`stw r0, 0x4485(r31)`, then `0x4489`, ...). Declared without a pragma the member silently aligns to `0x4488` and every later field... |
| 66 | A narrow RETURN TYPE is visible at the caller | The caller is one instruction off at the `bl`: the target stores the result with a raw `sth`, ours masks it first - a `clrlwi`/`extsh` retail does not have - and every later instruction shifts with it. It reads as... |
| 67 | A band's `.data` LOG STRINGS name their own emitters | A band arrives as a hundred `fn_XXXXXXXX` and the naming evidence is the runtime dump (playbook 25), a `__FILE__` string (54) or the neighbours' scheme - all of which can be absent at registration. The band's own... |
| 68 | A derived class's VTABLE is emitted where its KEY FUNCTION is defined | The unit reconstructs a derived class that must store the base's vtable pointer (`__vt__24NetworkSessionManagerPat`) without emitting a table of its own - the target's `.data` is the base table alone, 0x1C8 bytes.... |
| 69 | A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element | Declaring the four `Pat` record types as **classes** with a `NetworkSmallObject` member blew `NetworkSessionManagerPat`'s constructor from **252 to 544 bytes**: the arrays are built with `__construct_array`, and MWCC... |
| 70 | Data a unit uses that nobody owns is the unit's to claim - an `extern` for it is the defect | A unit reads or writes bytes the target object carries, the address is covered by **no** registered `splits.txt` range, and the only thing the source has for it is an `extern` declaration. The shape that prompted this... |
| 71 | The condition's POLARITY decides the exit - a ternary merges arms where `if/else` does not | A function whose logic and instruction set are right still misses by a register shuffle and an exit: the two arms do not merge, an early return arrives as a `beq`+`b` pair where retail has one `bne`, or a `memcmp` test... |
| 72 | The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity | Three residuals no statement order and no flag moves: MWCC peels a word off a copy because a reconstructed struct's `sizeof` is not the size the target's copy used; a loop test reads `cmplw` where retail has `cmpw`; a... |
| 73 | Never append `, ...` to a definition to dodge an argument-count mismatch | A retired object calls a function through a declaration with more arguments than the source signature carries, so the compiler refuses it. The tempting fix is to make the definition variadic (`void fn(int a, ...)`),... |
| 74 | A data range dtk calls link PADDING needs a `type:function` symbol over the claim | `Runtime.PPCEABI.H/TRK_interrupt_vectors` - the 8,000 B TRK interrupt-vector image in `.init` - matched byte-for-byte and **could not land**: `dtk dol split` classifies the range as link padding (the analyzer finds no... |
| 75 | `complete_code_percent` is a FLAG we set, not a measurement | `tools/project.py` writes `metadata.complete: true` into `objdiff.json` for every `Object(Matching, ...)`, and the report's unit row then reads `complete_code: 404, complete_code_percent: 100.0` **whatever the bytes... |
| 76 | A class hierarchy is traced from MANGLINGS, the CTOR's vtable store and SHARED SLOT OFFSETS - and a by-index accessor family is usually a container | Our source modelled four `Network` entities (`NetworkSessionManagerPat`, `NetworkPatSlot04`, `NetworkCommunityPat`, `NetworkLayerPat`) as four classes behind a family of `void*` accessors (`getXPat(self, index)`,... |
| 77 | `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test, never an extend | `quest_move_state_ck` measured **94.16666 %** and the lane's residual said "front-end artefact": the target reads `lbz r0,0x22D4(r3)` then `rlwinm r3,r0,0,24,24` where our build emits a bare `lbz`. Twelve spellings... |
| 78 | A data claim must cover the run the unit actually TOUCHES, not the extent the symbol happens to name | `NHTTP/d_nhttp`'s `.sbss` claim covered **4 B** (`0x80795880`, the list head) while the unit's own rows relocate three words of a **16-byte run** at `0x80795878-0x80795888`: two lazy-init flags (`li r0,1` / `stw`,... |
| 79 | A reconstruction is not deferred for a low match score | Twice in one session a measured call was made to leave something out because its bytes would score badly: a `.data` run a unit's own rows read (`arena_stage_config`, 0x24E0 B, whose three readers were rename-blocked)... |
<!-- PLAYBOOK-INDEX-END -->

The two tables below hold ideas with **no section of their own**: tried in one unit's context and failed,
or not tried at all. `no` means tried and it did not work (or does not apply) - it stays listed so nobody
re-runs it; `todo` means not tried yet. Neither table needs a status kept current: the moment an idea works
it earns a section, and the index above picks it up by number. A few `todo` rows were raised by the
`Camellia` flag hunt (`camellia_setup256`), which is closed - re-queue one only if the same shape reappears.

Ruled out so far - tried, and it did not work (details in `docs/matching.md`):

| idea | problem it solves | status |
| --- | --- | --- |
| Compiler-version matrix | "The original used a different compiler release" is the first suspicion and has to be closed once, per unit. | no |
| The rest of the `-opt` axis | An unknown sub-option might be what controls fusion or stack allocation. | no |
| `-Cpp_exceptions` | `extab`/`extabindex` presence suggests exceptions were on; it adds those sections, and with a `new` expression in the body it moves `.text` too - see row 62. | no |
| `-O4`/`-O4,p`/`-O2`, `-schedule off`, `-fp_contract off`, `-ipa off` | Another optimizer level or codegen switch might be the retail setting. | no |
| A paired-single op in a function (the SDK's vector library, `fn_8007270C`'s fill loop) | It reads as a codegen lever and eats flag and shape sweeps. Measured 2026-09-25: **176 of 19,916** functions contain one and **0** of ~2,450 matched functions do, so the frontend cannot emit the body-store form. Record it and move on. | no |

New ideas with no section yet (inexpensive to try, and they earn a section only if they work):

| idea | problem it solves | status |
| --- | --- | --- |
| Third historical source variant | Our `camellia_setup256` matches NSS and NetBSD textually, but the retail file may be a third variant (original NTT 1.2.0 / SDK copy). Diffing another copy's absorb region is the cheapest way to find the source shape the allocator liked. | no - NSS 3.19.1 / 3.24 / 3.28.4, the older NSS fetch and NetBSD NTT-derived are all statement-identical to ours (only `PRUint32`/`SUBL` naming and whitespace); no third variant exists in that lineage. |
| Perturbation probe | The slot outcome is sensitive to IR shape (level 4 flips the frame). Deliberately perturb one independent statement, watch whether `subL[29]` coalesces, then look for the natural source form that produces the same perturbation. | no - 22 statement/pragma perturbations tried; none flips the frame at level 3. The productive version of this idea turned out to be per-function pragmas (row 16). |
| Absorb `dw` / `CAMELLIA_RL1` IR shape | The level-4 near-miss changes exactly this chain, and it is the only region whose IR shape demonstrably moves the frame. Rewrites here (temps, ordering, expression form) are the highest-probability remaining lever. | no - five source forms tried *with* the level-4 pragma (split comma, RL1 temp, operand swap, `tl` rewrite, `tl` temp): the 12-row window is unchanged, so it is level-4 optimizer behaviour, not source shape. |
| Pragma combination search | With the level-4 pragma the frame is correct and only a 12-row window differs; a pragma that suppresses the level-4 reorder would finish the job. | todo - try `opt_lifetimes on`, `opt_common_subs on` explicitly, and level 4 combined with each honoured pragma. |
| Level-4 pragma + window source forms | The window is 12 rows of register choice plus `CAMELLIA_RL1` placement; a source form that makes level 4 emit the target order there would be a 100 % match. | todo - 5 forms tried, more structural rewrites of the whole kw4 chain remain. |

How to work the list:

1. **Work one idea at a time** and record *evidence* - numbers, sizes, first-divergence indices - not
   impressions. For a unit that does not match, walk the ideas in `docs/matching.md` in the order they were
   learned; the index above is the map, the sections are the detail.
2. When an idea works, it earns a `docs/matching.md` section in the house style - **Problem / Why it
   happens / How to work it / Result / Example**, short and to the point - and the index above updates
   itself: the section's number is the row's number. **Record it in the same session it works**: a win that
   exists only in a chat message or a scratch report is lost at the next compaction, and the next unit
   re-derives it.
3. The same commit brings the skill's copy with it: `python
   .agents/skills/mwcc-unit-matching/scripts/sync_reference.py --check` must come back clean, because
   `references/` is what a fresh session and every subagent actually load. The index itself is checked by
   `python tools/agents/sync_playbook_index.py --check` (wired into `tools/selftest.py`).
4. An idea that fails stays in the table below as `no`, with the evidence that killed it, so it is not
   re-run. A new idea goes there as `todo` first, with the problem it solves; a duplicate number is refused
   by the index tool, so a new section takes the next free number.
5. Unit-specific findings that are not playbook material (a residual diff, a known-bad flag) belong in
   the unit's own header comment, per the matching policy above.

## Repository layout

```
configure.py              Project config + build generator (compiler flags, libs, tool versions)
config/RMHE08/config.yml  Analyzer/build settings, DOL path + hash, selfile (RSO list)
config/RMHE08/symbols.txt Symbol map: name = section:address; // type/size/scope  (generated, hand-editable)
config/RMHE08/splits.txt  Which address ranges belong to which translation unit / section
config/RMHE08/build.sha1  SHA-1 of each built artifact — the pass/fail check for the whole project
src/                      Our C/C++ source; a unit is registered once, at its final home
                          src/<module>/<name>.<ext> (no src/auto/ bucket - docs/plan.md §12)
include/types.h            The project's common scalar types (u8..s64, f32/f64, BOOL/TRUE/FALSE/NULL) - one
                          definition, included by every unit that needs them; `cflags` already get `-i include`.
                          A declaration moves here the *second* time a unit needs it, never the first (a type
                          one unit uses belongs to that unit, or beside it like `src/Camellia/camellia.h`);
                          `src/Camellia/camellia.c` is the vendor exception and keeps its own typedefs.
                          An SDK type (`GXRenderModeObj`, `Vec`, `Mtx`, ...) gets a `dolphin/` mirror here
                          rather than a fresh declaration per unit.
orig/RMHE08/              Original game files (read-only, gitignored). main.dol, files/mh3.sel, ...
build/                    Everything generated: build.ninja, compilers/, tools/, RMHE08/ (gitignored)
tools/                    Tooling. dtk-template's scripts at the top level (project.py, download_tool.py, ...),
                          plus ours, grouped by what they do:
                            unitutil.py  shared unit/flag/ELF layer used by the tools below
                            flags/    compiler-flag and source-shape experiments (frame.py,
                                      mwcc_matrix.py, optsweep.py, tryvar.py,
                                      shapesearch.py + shapes.py,
                                      infer.py) + variants/<lib>.py data
                                      - see docs/matching.md
                            objdiff/  objdiff consumers (symdiff.py, slotmap.py)
                            elf/      object/DWARF readers (elfsect.py, dwarfmap.py)
                            splits/   TU boundary discovery (tudiscover.py): from one symbol address,
                                      work out which functions and data ranges form one translation
                                      unit - see the `tu-boundary-discovery` skill
                            symbols/  symbol-map proxy (symedit.py): look up, list by range and rename
                                      symbols in config/RMHE08/symbols.txt without loading it into
                                      context - see the `symbol-map-editing` skill
                            agents/   AGENTS.md housekeeping (localonly.py): pull the local-only
                                      working-state section out before a commit and push it back after
                                      - see the `agents-md-local-only` skill; subagent-profile sync
                                      (sync_profiles.py): generate each profile's section 6.5 rule
                                      block from docs/plan.md - `--check` exits non-zero when a
                                      profile is stale
                            units/    the `decompile-symbol` helpers (symbolpreflight.py: one symbol's
                                      owner and collision pre-flight; m2cinput.py: a target object's
                                      disassembly as `tools/m2c` input) plus their self-tests, and
                                      ledger.py: the campaign's progress and the next symbol to claim
                                      (docs/plan.md), vtableaudit.py: the rule 10 audit (an owned
                                      vtable our object does not emit, plus non-`.text` section-size
                                      completeness), declclash.py: the duplicate-declaration/clash scan
                                      of one file's include closure (a cross-unit lane's cost model -
                                      rule 2's `illegal overloading` class), and land.py's band-ownership
                                      warning (a batch that registers a range whose symbols a band header
                                      still declares - the same class), and callers.py: the whole-DOL caller
                                      index (`callers.py <address|name>` - who calls this function / who
                                      reads this data, address-keyed because the asm dump is stale)
                            mwcc-debugger/ the compiler's own IR: our own mwcceppc.exe under gdb, the PCode
                                      stream after each optimizer pass and the register allocator's
                                      decisions - locate/verify_pcode.py health-checks a dump against
                                      your object before anything it says is trusted
                                      - see the `decompiler` profile's IR section
                            mwlink-debugger/ the linker's own run (tools/mwlink_debugger.py): trace one unit
                                      through a real link (kept? where each section landed, how its
                                      symbols resolved, which relocations were applied), diagnose a
                                      failing link's phase stream, verify the map against the ELF,
                                      align a claimed start and read the `.ctors`/`.dtors` order -
                                      the link step is Wii/1.0, and main.elf is never written
                                      - see the `decompiler` profile's linker section
                            m2c/      matt-kempster/m2c as a git submodule - the offline decompiler
                                      units/m2cinput.py feeds - see the `decompile-symbol` skill
docs/                     Where all documentation lives — ours and dtk-template's. Anything worth
                          writing down goes here. Keep docs short and to the point, not dense.
                          matching.md is the matching playbook; its ideas are indexed in the
                          "Matching playbook" section above.
                          plan.md is the campaign plan: every symbol in symbols.txt, the four steps per
                          symbol, the 80 % bar for closing one, and the order to work in.
                          memory-dump.md documents the shared Ghidra runtime memory dump: real SDK
                          symbol names, annotated struct layouts and data contents, used as an
                          oracle for names/signatures/data (never for codegen) - see below.
                          rso-modules.md documents the RSO module format, the inventory and the
                          splitter blocker.
```

### External oracles

* **Shared Ghidra project `MH3Shared` / runtime memory dump `/DolphinDump85.raw.keep`** (a dump of the
game running, reachable through the `ghidra` MCP server). It is the quickest way to turn a region full
of `fn_XXXX` into named SDK functions, to get a function's real signature, to confirm a struct's field
offsets/names, or to read a data blob this repo does not own yet. Example: it named the `RSO/runtime`
unit's thunks `RSONotifyPreRSOLink`/`RSONotifyPostRSOLink`, its `fn_804DA834` `FindExportIndex` and its
`fn_804DAA24` `RSORelocate`, and its `RSOModule` layout matched every offset we had derived by hand.
Three rules: it is **not** codegen evidence (flags and the compiler family still come from diffing the
retail bytes, see playbook 17), a `splits.txt` range derived from it must be **measured before and
after** - claiming the RSO unit's string pool *lowered* a function by 1.35 points, claiming its jump
table raised another by 0.08 - and it is **read-only**: the project is shared with another effort, so we
consult it and never modify it (no renames, no types, no comments, no imports, no saves), and our names
go in `config/RMHE08/symbols.txt` through `symedit.py`, never into Ghidra. Details and the recipes:
`docs/memory-dump.md`.

Inside `build/RMHE08/`:

* `src/<Unit>.o` — **our** compiled object (the candidate; ninja target `build/RMHE08/src/...`)
* `obj/<Unit>.o` — the **original** object split out of the DOL (the target)
* `main.elf`, `main.dol`, `main.MAP`, `ldscript.lcf`, `report.json`, `progress.json`, `ok`

objdiff compares the `src/` candidate against the `obj/` target. Keeping those two straight is essential.

## Build & verify

Windows-friendly (this is the supported setup here): Python 3.12 + `ninja` on `PATH`. The toolchain
(binutils, Metrowerks compilers, dtk, objdiff-cli, sjiswrap) is downloaded automatically into `build/`.

```sh
python configure.py            # regenerate build.ninja + objdiff.json (needed after editing configure.py)
ninja                          # default target: build/RMHE08/progress.json (builds + verifies)
ninja build/RMHE08/ok          # build main.dol and check it against config/RMHE08/build.sha1  <-- the real test
ninja build/RMHE08/src/Camellia/camellia.o   # compile a single translation unit
```

Other useful targets:

| Target | What it does |
| --- | --- |
| `ninja all_source` | Compile all configured source files (no link) |
| `ninja build/RMHE08/report.json` | objdiff report for all units (progress + per-function diffs) |
| `ninja diff` | `dtk dol diff` — report symbols in the linked ELF that don't match |
| `ninja apply` | `dtk dol apply` — apply symbol/label info from the linked ELF back into `symbols.txt` |
| `ninja baseline` | Save the current report as a baseline for regression checks |
| `ninja changes` / `ninja changes_all` | Compare against the baseline (markdown: `regressions.md`) |
| `ninja tools` | (Re-)download the pinned toolchain |

Notes:

* `python configure.py --warn all|error` adds compiler warnings; the committed default has no `-W` flag.
  `--non-matching` builds "equivalent" code for extra units without linking them.
* On non-Windows hosts a `--wrapper` (wibo/wine) is required to run the Metrowerks compilers; on Windows
  they run natively.
* `mwcc_sjis` (sjiswrap) wraps the compiler, so keep source files UTF-8 with no BOM.
* If results look impossible, the build tree is probably stale: `rm -rf build/RMHE08` then
  `python configure.py && ninja`.
* The **`dol split` is the slow step** and it re-runs whenever `symbols.txt`, `splits.txt` or the DOL
  changes - so a rename costs a re-split, which is why renames ride one batch (see the batch rule in
  `docs/plan.md`). It is **~18 s as configured** and **200-400 s with the asm dump on**: the dump is on
  demand (`write_asm: false` + `python tools/splits/dump_asm.py`) because only `tudiscover` reads
  `build/RMHE08/asm/`. Measurements, the link-ordering pitfall and the recipes for reading `.ninja_log`:
  `docs/build-performance.md`.

Verifying whether a unit, function or symbol matches is its own procedure — per-symbol objdiff plus raw ELF
evidence, and a specific set of traps (`complete_code_percent` lies, `ninja build/RMHE08/ok` cannot isolate
one unit, a function missing from the report is 0 %). Follow skill **`.agents/skills/objdiff-verify/SKILL.md`**
(everything under `.pi/` is gitignored - the tracker holds no path there).

## The core loop: adding / matching a translation unit

1. **Find the unit.** Locate the function in `config/RMHE08/symbols.txt` (`grep`), get its address and size,
   and find the surrounding section ranges in `config/RMHE08/splits.txt`. If it is an unnamed `fn_XXXX`,
   look it up in the shared memory dump (`docs/memory-dump.md`) first - the real SDK name and signature
   usually come back in one query, and they tell you what the function is before you read a byte of code.
2. **Register it** in `config.libs` in `configure.py`: pick the right `mw_version` (this is a Wii title:
   `Wii/1.0` for REL-type code, `Wii/1.3` for runtime-style code — **not** the GC compilers) and an
   appropriate `cflags` group (`cflags_runtime` for runtime units, `cflags_base` otherwise).
   Start with `Object(NonMatching, "Dir/file.c")`.
3. **Create `src/Dir/file.c`** (and headers under `include/` if needed) - **in the same change as the
   registration, because a unit's source file is mandatory, and it is created with its functions, not just a
   header: the functions the unit is known to hold, with their decompiled bodies wherever they are recoverable
   and measured against the target.** A unit whose bodies cannot be written yet still gets its file, whose
   header says what it is, its range, why it sits there, what is unknown, and where its inventory and evidence
   live (`ledger.py unit <path>`, the map, `splits.txt`) - never a copied-out function list, which goes stale
   on the next range edit. Registering a unit without a source leaves the build warning about it and its object
   unbuildable - a bug, not a state.
4. **Add the splits** to `config/RMHE08/splits.txt`: one line per section with exact `start:`/`end:`
   addresses, including the small `.ctors`/`.dtors`/`.sdata` fragments that runtime units own (Wii linkers
   use `.ctors$10`, `.dtors$10`, `.dtors$15` — the GC 2.7+/Wii linker gives each constructor/destructor
   chain its own `$`-suffixed section, so a runtime unit owns its own fragment).
5. **Compile and diff:**
   `python configure.py && ninja build/RMHE08/src/Dir/file.o`, then produce/refresh the report and inspect
   the unit's per-function diff (objdiff GUI reads the generated `objdiff.json`). When it does not match,
   work the ideas the **Matching playbook** index points at, one idea at a time, and read the section it
   names instead of guessing at flags.
6. **Flip to `Object(Matching, ...)`** only once the unit matches (bytes/instructions + relocations).
7. **Prove it end-to-end:** `ninja build/RMHE08/ok` must finish green, i.e. `main.dol` matches
   `config/RMHE08/build.sha1`.

Keep changes small and verified. A micro-optimization for a function that was already matching is a
regression if the hash goes red.

## Gotchas learned in this repo

* **Compiler-flag drift is silent and fatal.** An earlier revision of `cflags_base` had `-O4,p`,
  `-inline auto`, `-Cpp_exceptions off` and `-RTTI off` removed (and `-use_lmw_stmw on` commented out).
  Nothing failed to compile; the resulting `main.dol` was just wrong. `Camellia` compiled 0x3244 bytes too
  large, and per-function size deltas summed to **exactly the total DOL growth (+12,868 bytes)** — that
  sum-vs-total check is a great way to diagnose "everything is slightly bigger" symptoms.
* **Don't trust a single objdiff number.** `complete_code_percent: 100.0` has been observed alongside
  `fuzzy_match_percent: 1.77` for the same unit, and again with 1.49 for Camellia. Cross-check
  `fuzzy_match_percent`, the per-function diff, and ultimately `build.sha1` / `ninja build/RMHE08/ok`.
  In the report JSON a *function* entry with **no** `fuzzy_match_percent` key is **0 %**, not 100 % — the
  unit's percent is exactly the sum of the listed partial matches over `total_code`, so arithmetic-check it.
* **`Object(Matching, …)` is a claim, not evidence, and it changes the link.** `NonMatching` is literally
  `False` in `configure.py` ("should not be linked"): those regions keep their original bytes, while a
  `Matching` unit's object is substituted in. A `Matching` flag on a wrong object is worse than no flag —
  and a failing `ninja build/RMHE08/ok` cannot tell you *which* unit is wrong.
* **A `.comment` version-byte difference means a different compiler build.** Dump it with
  `python tools/elf/elfsect.py <obj>` (also at `.agents/skills/objdiff-verify/scripts/elfsect.py`): the
  original Camellia object is `"CodeWarrior" 0e …`, our `Wii/1.3` build is `"CodeWarrior" 0f …`, and
  `config.yml`'s `mw_comment_version: 14` describes the original. Different version byte + `0 %`/size-very-
  different functions = suspect the compiler release, not the source.
* **`extab` / `extabindex` (and `.relaextabindex`) presence is flag evidence.** The original Camellia object
  has them; ours has none, i.e. the original TU was built with C++ exceptions enabled (or as C++) while our
  `cflags` group passes `-Cpp_exceptions off`. Treat it as a hypothesis to test on one unit, per rule 3.
* **`mw_comment_version: 14`** in `config.yml` must match the `.comment` section of the original objects.
  A mismatch makes the analyzer mis-identify the toolchain.
* **`quick_analysis: false`** is required while function boundaries are still being discovered; setting it
  to `true` skips boundary analysis and is only valid after analysis is complete and `symbols.txt` /
  `splits.txt` are generated.
* Sizes/addresses in `symbols.txt` and `splits.txt` are absolute addresses from the **unlinked** original
  DOL; they are not offsets, and section order matters (`.init`, `extab`, `extabindex`, `.text`, ...).
* Stale `build/` output causes false conclusions (an old object from different flags can look "matching").
  Prefer a clean rebuild of the specific unit, and `rm -rf build/RMHE08` when in doubt.
* Local agent scratch directories (`.lavish/` and everything under `.pi/` - notes, prompts, scratch) are
  gitignored; keep them that way and never add their contents to commits. `.agents/` is ignored **except**
`agents/` and
  its skills folder, which is tracked in full (`.agents/skills/`) so every harness shares one set of skills.

## Conventions

* **Commit messages** (only once a commit has been approved — see Non-negotiables rule 6): short imperative
  subject, area-prefixed, e.g. `Camellia: match Camellia_Ekeygen`, `RMHE08: refresh symbols.txt`,
  `configure.py: add REL flags`. Describe *why* when fixing a mismatch.
* **Keep generated/large churn separate.** A `symbols.txt` regeneration or an analyzer settings change gets
  its own commit; never mix it with source changes or unrelated formatting.
* **Naming and commenting** (see "Commenting and naming" below): use the real name when it is known, and
  treat dtk's generated `FUN_xxxxxxxx`/`fn_xxxxxxxx` name as a **placeholder to replace** (rule 7: derive the
  name from context, rename the map row and sweep the referrers in the same change) - leaving one in `src/` is
  a finding, not a resting place. Keep function comments descriptive. Vendor files keep vendor naming (e.g.
  `Camellia/` uses `CAMELLIA_*` constants and its original MPL-1.1 header — keep those intact).
* **Commenting and naming** (applies to every unit we write):
  * **A comment on top of a function is a short description of what the function does** - one or two lines,
    in the present tense ("Rebases the module's section pointers, then patches every import's relocation
    chain."). It must **not** carry the symbol's name and **not** a matching percentage: the name is already
    the identifier below it, and the percentage lives in the objdiff report (and changes on every build).
    Residuals, flag evidence, name provenance and anything else that is about the *unit* go in the unit's
    file header comment.
  * **The unit's file header comment is the one place for the unit's own notes** - what it is, the `.text`
    range and function order, where its flags/evidence live, the residuals, and any load-bearing source
    shapes. Keep it to the essentials: one line per fact, no repetition of what `configure.py`, the playbook
    or a report already says, and no per-function inventory (addresses, sizes and instruction counts are in
    `symbols.txt` and the objdiff report). A header that re-argues the flag hunt or tabulates every function
    is noise the next reader has to skip - the two existing units' headers are the length to aim for.
  * **Use the real name when it is known** (the retail symbol map, the shared memory dump in
    `docs/memory-dump.md`, the SDK), or a descriptive name when you clearly have a better one - but it has
    to fit the **naming scheme of the surrounding symbols**, especially where siblings are already named:
    in the `RSO/runtime` unit `RSOStaticLocateObject`/`RSOUnLocateObject` make `LocateObject` an obvious fit,
    and `RSORelocate`/`RSORelocateSmallDataSection`/`RSOUnLink`/`RSONotifyPreRSOLink` match the `RSO*` public
    API around them. **A generated name is not an acceptable resting place**: derive one from context - what the
    function does and who calls it, what the data holds and who reads it, the field's offset and the value stored
    there - and keep it in the surrounding symbols' scheme, because a name that reads like it belongs to another
    module is still worse than a dull one. When the context supports only a guess, **guess**, and mark it in the
    unit's header so a later pass can refine it. `fn_xxxxxxxx`/`lbl_xxxxxxxx`/`unkNN` left in `src/` is a defect.
  * **A rename is always two edits**: `config/RMHE08/symbols.txt` (which names the *target* object) and the
    source that defines/references it, in the same change - otherwise objdiff stops matching the symbol by
    name and reports it as 0 %. Do it through the proxy, never by hand:
    `python tools/symbols/symedit.py rename <old> <new> --dry-run` first, then for real; it refuses
    ambiguous renames, keeps the file's line endings, prints the one-line diff and lists the in-repo
    references that are the other half of the edit. Verify with `mt.py diff -u <unit> <symbol>` and keep the
    linked DOL hash unchanged. Never open or regenerate the map for this - it is 4.5 MB and must not enter
    an agent's context (skill: `symbol-map-editing`).
  * **Name `unk` variables, fields and parameters from the context they are used in** - what is stored,
    what it is compared against, which SDK type the offset belongs to, what the value is later passed to
    (e.g. `unk50` in a struct became `import_symbol_table_size` once the dump confirmed the layout, and an
    argument only ever used as a string pointer became `symbol`). Leaving `unk`/`unkNN` in place is fine and
    expected when the context does not support a name - do not invent one to fill the gap. **Exception: work done
    under the campaign plan (`docs/plan.md` §6.5) is held to a stricter standard** - there, no `fn_XXXXXXXX` or
    `unkNN` may survive in `src/`, every reconstructed type states its size, every field carries its offset and a
    context name (padding excepted), shared types live in one header, an `extern` lives with the unit that owns
    the symbol, and pointer arithmetic to reach a field is forbidden. `tools/units/stylelint.py` (roadmap 7.21)
    enforces those twelve rules at the campaign's land gate (the table is `docs/plan.md` section 6.5;
    regenerate the profiles with `tools/agents/sync_profiles.py` after a rule change).
* **Style:** match the file you're editing (vendor sources mirror upstream formatting; new project code
  follows the surrounding 4-space-indent C style). Files are UTF-8 and **LF - in the repository and in the
  working tree**: `.gitattributes` carries `* text=auto eol=lf`, and a clone must **not** set
  `core.autocrlf=true`, because `eol=lf` alone does *not* override it (measured 2026-09-28: the attribute
  was in effect and the checkout was still CRLF - every blob was already LF, only the working tree was
  not). The pre-commit hook normalises a staged CRLF *text* blob and warns when `core.autocrlf` is on; it
  refuses only a *binary* blob that carries a CR. The hook itself is per clone: `git config core.hooksPath
  tools/git/hooks`.
* **Documentation:** `docs/` is the home for all documentation — put new knowledge there instead of
  leaving it in chat, commit messages or code comments. Write straight to the point: setup steps, recipes
  and findings as short bullets, not dense prose or oversized files. Split into one file per topic rather
  than growing a single wall of text. dtk-template docs already in `docs/` stay authoritative for template
  behaviour; add project-specific notes alongside them instead of rewriting them.

## Before claiming success

* [ ] `ninja build/RMHE08/ok` passes (for anything affecting the linked DOL), or the change is explicitly
      described as unverified. **Check the `FAILED` count first**: `ok` is order-only and prints OK off a
      stale `main.dol`, so a `NonMatching` object that does not compile still looks green.
* [ ] For a single unit/symbol: the object compiled **and** its objdiff diff shows the claimed match level
      (per-symbol `match_percent`, equal section sizes) — see the `objdiff-verify` skill.
* [ ] `git status --short` shows only intended files (no `build/`, no `orig/`, no scratch dirs). A lone
      `M AGENTS.md` just means the local-only block differs, which is expected.
* [ ] The committed `AGENTS.md` has no local-only block: `git show HEAD:AGENTS.md | grep -c '^<!-- LOCAL-ONLY'`
      → `0`.
* [ ] `symbols.txt` / `splits.txt` edits are byte-clean for the lines you didn't mean to touch
      (`git diff --stat` sanity check — these files are huge; a symbol rename goes through
      `python tools/symbols/symedit.py rename`, so its diff is exactly one line per symbol).
* [ ] For source or `include/` work: `python tools/units/stylelint.py --diff main` adds no section 6.5
      violation - the land gate enforces the same row, so an added one refuses the batch.
* [ ] For a tool change: the suite is green - `python tools/selftest.py --changed main` (plain `--changed`
      selects nothing on a committed clean tree), and the land gate runs all of it as the row **"all tool
      selftests pass (except the parked list)"** (~30 s, before the build, so a failure refuses early). A
      failure is fixed or **parked** in `tools/selftests-known-failures.json` with a reason and a date - never
      ignored, and never silently skipped: a park whose test now *passes* is itself an error, so parked debt
      cannot rot.
* [ ] No new compiler flags / tool version changes smuggled in.
* [ ] A lane **commits its own fix on its own branch** - that is the deliverable (see "Operational mode");
      nothing beyond it is committed, and nothing is ever pushed (rule 6: only the orchestrator has standing
      approval, and it still never pushes). Staged vs. unstaged state reported clearly.
