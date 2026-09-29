# CLAUDE.md

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

## Where the state and the lessons live

* **Live campaign state** (what is in flight, parked decisions, open tool asks, how to re-measure the ledger) is in
  `.pi/state.md` - gitignored, read it at the start of a session, refresh it rather than append.
* **Durable lessons and the owner's dated rulings** are in `docs/pipeline.md` (sections 3-5 for the rules, the gate
  and the merge procedure, 11 for the rulings, 12 for tooling and environment traps).
* **History** (what earlier sessions did and why) is `.pi/notes/claude-local-block-archive-2026-09-29.md` and the
  other files under `.pi/notes/`; none of it is loaded into a lane's context.

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
8. **`CLAUDE.md` holds no live working state.** It is an ordinary tracked file: session state, dated snapshots
   and incident logs go in `.pi/state.md` or `.pi/notes/`, never here.

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
   `python .claude/skills/mwcc-unit-matching/scripts/mt.py variants --apply <name>` followed by a forced
   rebuild; a probe-only winner is not progress.
2. **Evidence-backed flags live in `configure.py` as soon as they are proven**, even while the unit is
   still short of 100 %, so the repo always reflects the best known state. They must be **per library**
   (never edit `cflags_base`/`cflags_runtime` for everyone) and must be called out explicitly, as
   non-negotiable #3 requires, with the instruction/size evidence in a comment next to them.

A unit's flag evidence belongs next to its definition in `configure.py` (a per-library `cflags_*`
override below `cflags_runtime`), not in this file; this file only carries the policy.

The *how* - the ideas, the problem each one solves and whether it has been tried - is the playbook index
in `docs/matching/index.md` (see "Matching playbook").

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
  instruction, 2026-09-26).** The project defines **four** profiles in `.claude/agents/` (tracked):
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
  Measured 2026-09-26: an uncommitted `CLAUDE.md` + `docs/plan.md` pair rode the `OS/FindContainHeap_.c` unit
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
3. Apply that branch with `.pi/bin/applybranch.sh` (the merge-base diff, not a tip-only cherry-pick and not a
   plain two-way diff, which lose work), and resolve the shared-file conflicts: `python
   tools/units/mergebranch.py resolve` resolves the classes it can (a comment that names a unit **file** is not
   a stale symbol; a comment-only `src/**` difference takes main's comment and the branch's code) and refuses
   the rest. Then `land.py record-base` -> `land.py land --units <claim>` -> `claims.py release`.
4. `ninja build/RMHE08/ok` green, then refill exactly that one slot.

**The loop closes with a review: `decomp -> review -> decomp`.** A committed branch is reviewed **before** it
lands, by a read-only review lane (`.claude/agents/codereviewer.md`, launched with `python
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

## Matching playbook

`docs/matching/` is the playbook for making a unit match its original object: **one file per idea**,
`NNN-slug.md`, with a permanent id (`docs/matching.md` is only an entry page so old "playbook N" citations resolve).
It is **not** loaded into every lane: load the project skill **`mwcc-unit-matching`**
(`.claude/skills/mwcc-unit-matching/`, tracked) when a unit's functions mismatch.

* **The index** of every idea (id, title, status, tags, the problem it solves) is the generated
  `docs/matching/index.md`; `python tools/agents/sync_playbook_index.py --where N` prints idea N's path. The loop,
  the tag vocabulary and how ideas are recorded are in `docs/matching/README.md`; the ideas tried and ruled out
  are `ruled-out.md` and `todo.md` beside it.
* **Never edit the generated files.** `python tools/agents/sync_playbook_index.py` writes `index.md` (from the
  front matter of the idea files) and `python .claude/skills/mwcc-unit-matching/scripts/sync_reference.py` writes
  the skill's portable byte copy `references/matching/`; both take `--check` and both run from
  `tools/selftest.py` whenever anything under `docs/matching/` changes.
* `python .claude/skills/mwcc-unit-matching/scripts/mt.py` forwards to the `tools/` helpers (`units`, `info`,
  `frames`, `matrix`, `sweep`, `variants`, `shapes`, `diff`, `slots`, `sections`, `dwarf`).
* **How to work an idea:** one at a time, with evidence (numbers, sizes, first-divergence indices); an idea that
  works earns its own `docs/matching/NNN-slug.md` in the same session (front matter, then Problem / Why it
  happens / How to work it / Result / Example) with the next free id (ids are never renumbered); an idea that fails is recorded as ruled out so nobody re-runs it;
  unit-specific findings belong in the unit's header comment.

All project **subagent profiles** live in `.claude/agents/` (tracked), discovered by the harness as *project*
agents. `decompiler.md` is the unit-work role: it inherits this file (`inheritProjectContext: true`), loads
the matching/verify/registration skills (`skills:`), and carries the rules that used to be hand-typed into
every launch. When a launch prompt and the profile disagree, **the profile wins** - so the rules live there,
not in the prompt. Which profile a lane gets, and why it is not a cosmetic choice, is the rule in
"Operational mode" below. A profile edit is not live until `tools/agents/install.sh` has copied
`.claude/agents/*.md` to `~/.claude/agents/` (it refuses when the section 6.5 block is stale), because the
harness reads the installed copy - a worktree cut before the edit otherwise serves the old prompt.

All project skills live in one tracked folder, `.claude/skills/`, so every harness sees the same set:
`mwcc-unit-matching/` (the playbook and its index), `symbol-map-editing/` (`tools/symbols/symedit.py` - look up, list by
range and rename symbols without ever loading `symbols.txt` into context),
`objdiff-verify/` (proving a unit really matches), `tu-boundary-discovery/`
(`tools/splits/tudiscover.py` - from one symbol address, work out which functions and data ranges form one
translation unit, before any source is written) and `decompile-symbol/` (`tools/units/symbolpreflight.py`,
plus `tools/units/m2cinput.py` for the `tools/m2c` decompiler - one symbol from an address to a registered,
measured unit).

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
                                      - see docs/matching/
                            objdiff/  objdiff consumers (symdiff.py, slotmap.py)
                            elf/      object/DWARF readers (elfsect.py, dwarfmap.py)
                            splits/   TU boundary discovery (tudiscover.py): from one symbol address,
                                      work out which functions and data ranges form one translation
                                      unit - see the `tu-boundary-discovery` skill; dataorder.py:
                                      the `.data` emission-order seam evidence,
                                      docs/data-order-seams.md
                            symbols/  symbol-map proxy (symedit.py): look up, list by range and rename
                                      symbols in config/RMHE08/symbols.txt without loading it into
                                      context - see the `symbol-map-editing` skill
                            agents/   subagent-profile sync
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
                          matching/ is the matching playbook, one NNN-slug.md per idea, indexed in
                          matching/index.md (matching.md is a legacy entry page).
                          plan.md is the campaign plan: every symbol in symbols.txt, the four steps per
                          symbol, the 80 % bar for closing one, and the order to work in.
                          memory-dump.md documents the shared Ghidra runtime memory dump: real SDK
                          symbol names, annotated struct layouts and data contents, used as an
                          oracle for names/signatures/data (never for codegen) - see below.
                          rso-modules.md documents the RSO module format, the inventory and the
                          splitter blocker.
                          data-order-seams.md explains why a vtable followed by data in retail `.data` is a
                          TU seam (MWCC emits globals, strings, then vtables in reverse), the measured
                          result, and the plan that feeds it into tudiscover, dataclaim, flipcheck and
                          attribute (playbook row 80).
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

## Agent harness: Claude Code

This repository was converted from the pi agent harness to Claude Code on 2026-09-29. What lives where:

* **`CLAUDE.md`** (this file) is the project instructions file; it was `AGENTS.md`. Live working state stays out of it
  (non-negotiable 8).
* **`.claude/agents/*.md`** are the subagent profiles (frontmatter `name`, `description`, `tools`, `skills`).
  `tools/agents/install.sh` copies them to `~/.claude/agents/`, so a worktree cut before a profile edit still
  sees the current prompt. `codereviewer` has no `Write`/`Edit` in `tools:` - read-only by construction.
* **`.claude/skills/*`** are the project skills (`SKILL.md` per folder), loaded on demand.
* **A lane is a headless `claude --agent <profile> -p <task>` process with its cwd at a slot.** The Agent tool has
  no `cwd` parameter and the slot design is one directory per lane, so `tools/units/lanecmd.py` builds the launch
  line (`slots.py spawn`, `queue.py next` and `backlog.py` all print it). Its `--session-id` is what answers a
  lane's question: `claude --resume <session-id> -p "<ruling>"` in the same cwd. `CLAUDE_BIN`,
  `LANE_PERMISSION_MODE` and `LANE_ALLOWED_TOOLS` override the binary, the permission mode (default
  `acceptEdits`) and the allowed tools (default `Bash`).
* **A live lane is read from `~/.claude/sessions/<pid>.json`** (`slots.live_runs`): a record counts while its pid
  is alive, and `release`/`reclaim` refuse a slot a live session is working in.
* **Prototype - in-session subagents in slots.** `.claude/settings.json` points the `WorktreeCreate` /
  `WorktreeRemove` hooks at `tools/units/worktreehook.py`. The hook hands out a slot only **when armed**:
  run `python tools/units/worktreehook.py arm N` right before launching N lanes (Agent tool,
  `isolation: "worktree"`); each launch consumes one token (an exclusive-create claim, so parallel hooks get
  distinct tokens and distinct slots) and tokens expire after `--ttl` (default 900 s). An unarmed launch - a
  manual `--worktree`, an ad hoc isolated subagent - gets an ordinary git worktree under `.claude/worktrees/`
  and is removed the ordinary way. A slot needs a **current** build tree (the hook never re-seeds; it fails and
  returns the token). For a claimed unit, `worktreehook.py arm --slot N` binds the token to the slot
  `queue.py next` / `claims.py claim` already took, so the subagent works in the claim's own slot and branch
  (no adoption step). Claude Code did not call the remove hook after a clean subagent, so the
  orchestrator runs **`python tools/units/slots.py collect --path <the result's worktreePath> --release`** when a
  lane's result arrives: it copies the slot's `.pi/outbox/*.json` and `.pi/notes/*.md` into MAIN, reports the
  commits main lacks, and releases the slot only when nothing is unlanded.
* **`.pi/` is unchanged.** It is the campaign's own gitignored scratch (outbox, notes, claims, lane state), not
  the old harness's directory, and lanes and tools still write there.
* **A lane cannot block on a question.** The `contact_supervisor` channel is gone: a lane ends its turn with the
  decision request as its final report and waits to be resumed.

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
one unit, a function missing from the report is 0 %). Follow skill **`.claude/skills/objdiff-verify/SKILL.md`**
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
  `python tools/elf/elfsect.py <obj>` (also at `.claude/skills/objdiff-verify/scripts/elfsect.py`): the
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
  its skills folder, which is tracked in full (`.claude/skills/`) so every harness shares one set of skills.

## Conventions

* **Commit messages follow one convention**: `<category>: <message>`, then an optional long description. Approved
  the same way as anything else — see Non-negotiables rule 6.
  * **`<category>` names where the change lives, and mirrors the tree**:
    * **`game/<module>`** — decompilation work. The module is the `src/` directory: `game/network`, `game/quest`,
      `game/menu`, `game/hud`, `game/pl`, `game/enemy`, `game/ef`, `game/g3d`, `game/nw24`, `game/dwci`,
      `game/camellia`, `game/os`, `game/pl` …
    * **`tools/<area>`** — our tooling, by the name the tree gives it: the `tools/` groupings
      (`tools/agents`, `tools/units`, `tools/git`, `tools/symbols`, `tools/objdiff`, `tools/flags`,
      `tools/splits` …) and any script's stem at any depth (`tools/land`, `tools/stylelint`, `tools/slots`,
      from `tools/units/land.py`, …). The member set is derived from the tree, not a fixed list.
    * **`config/<what>`** — a `configure.py` / `symbols.txt` / `splits.txt`-only change: `config/flags`,
      `config/symbols`, `config/splits`.
    * **`docs/<topic>`** — documentation that is not this file: `docs/plan`, `docs/pipeline`, `docs/matching`.
    * **`agents/<profile|policy>`** — a subagent profile (`agents/decompiler`, `agents/surveyor`) or this file
      (`agents/policy`).
    * **`repo/<area>`** — the repository itself: `repo/readme`, `repo/license`, `repo/ci`, `repo/gitignore`.
    * The list is **open, and there is no catch-all**: if nothing fits, add a category and use it. A `misc`/`chore`
      default is how the convention dies.
  * **`<message>` is imperative, says what was made, and is at most 120 characters.** "match the vtable slots",
    "give the register a name-based weight", "remove the count cap" — not a description of the problem, and not of
    the investigation that found it.
  * **A long description is optional and structural.** Include it only when the subject cannot hold the work: which
    files, units, symbols or rows changed, the measured numbers, the sections or claims added. It must **not** carry
    reasoning — no *why* this decision, no alternatives considered, no account of the work. That belongs in the unit
    header, the plan docs, the outbox or `.pi/notes/`, where it sits beside the code rather than in `git log`.
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
* [ ] `git status --short` shows only intended files (no `build/`, no `orig/`, no scratch dirs).
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
