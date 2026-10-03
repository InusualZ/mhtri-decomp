# CLAUDE.md

Guidance for AI agents (and humans) working in this repository.

## What this repository is

A **matching decompilation of Monster Hunter Tri** (Nintendo Wii, USA, disc ID **`RMHE08`**), built on the
[decomp-toolkit](https://github.com/encounter/decomp-toolkit) project template ([`encounter/dtk-template`](https://github.com/encounter/dtk-template)).

"Success" here means one specific thing: **the C/C++ source in `src/` recompiles to code that links into a
`main.dol` byte-identical to the original game**. It is not enough for code to compile, look correct, or
produce the same output - it must match the original instructions, relocations and section layout.

The original binary is split into relocatable objects by decomp-toolkit (no hand-written assembly, no game
assets in the repo), and the final `main.dol` is verified against `config/RMHE08/build.sha1`.

* Original binary: `orig/RMHE08/sys/main.dol` - SHA-1 `BF4850739478CAAEDFE675949EB7C28595A7FDE9`
* Current state: the splits program is finished - `config/RMHE08/splits.txt` cuts the whole DOL into registered
  translation units with no unclaimed `.text` (`docs/splits-program.md`). A unit is registered **once**, at its final home
  `src/<module>/<name>.<ext>` (no `src/auto/` bucket); most are `NonMatching` shells being reconstructed. The work queue is
  `splits.txt` itself: the pool is the registered body-less units (`python tools/units/brief.py --pool`). The measured
  totals and the live in-flight state are not kept here: `docs/plan.md` section 2, `python tools/units/ledger.py`, and
  `.pi/state.md`.

## Where the state and the lessons live

* **Live campaign state** (in flight, parked decisions, open tool asks, how to re-measure the ledger) is `.pi/state.md` -
  gitignored; read it at the start of a session and refresh it rather than append.
* **Durable lessons, the owner's dated rulings, the gate, the merge procedure and the production-run policy** are in
  `docs/pipeline.md` (sections 3-5 rules/gate/merge, 11 rulings, 12 tooling traps, 13 production-run policy and gotcha history).
* **The plan and the section 6.5 rules** are `docs/plan.md`; **tool inventory and specs** are `docs/tools.md` -> `docs/tools/`.
* **History** is under `.pi/notes/` (never loaded into a lane's context).

## Non-negotiables

1. **Never modify `orig/RMHE08/**`.** It is the ground truth for every diff. Read-only, always, and **gitignored**, so nothing in
   git can restore it: never run a repository-wide clean (`git clean -xdf`, `rm -rf orig`, a "reset the tree" recipe), and never
   let a subagent do it. When the build reports the DOL missing, restore from `../orig-backup/RMHE08/` first -
   `sha1sum orig/RMHE08/sys/main.dol` must print `bf4850739478caaedfe675949eb7c28595a7fde9` (incident: `docs/pipeline.md` 13.3).
2. **Never commit build output or original files.** `build/`, `orig/RMHE08/**` (except `.gitkeep`), `*.dol`, `*.rel`, `*.elf`,
   `*.o`, `*.map`, `objdiff.json` and `compile_commands.json` are gitignored - keep it that way.
3. **Do not edit `configure.py` compiler flags, `mw_version` values or tool version tags to make something build.** They change
   codegen for every translation unit. Allowed only with concrete evidence (an instruction/size diff that points at the flag),
   called out explicitly (incident: `docs/pipeline.md` 13.3, "Compiler-flag drift").
4. **Only mark an object `Object(Matching, ...)` when it actually matches.** Otherwise `NonMatching`. A wrong `Matching` flag
   breaks the final DOL hash for everyone.
5. **Don't rename or delete symbols in `config/RMHE08/symbols.txt`** unless you have verified nothing else depends on them
   (`splits.txt`, the linker script and the analysis output name them).
6. **Commit only what the task covers, and never push.** History is never rewritten (`rebase`, `commit --amend`,
   `reset --hard`, force-push, deleting/moving tags); `origin` is the upstream template, not a fork. The owner granted the
   campaign's orchestrator **standing approval to commit its own work** (2026-09-21); that is not a licence to commit an
   experiment, a probe, a half-registered unit, another stream's file or a scratch artifact. Anything outside the campaign keeps
   the old rule: leave it in the tree and report what you would commit.
7. **Never paste `config/RMHE08/symbols.txt` into a prompt/tool output** (~65,700 lines / 4.5 MB). Grep it, slice it, or use
   `symedit.py` / `dtk` / objdiff.
8. **`CLAUDE.md` holds no live working state.** Session state, dated snapshots and incident logs go in `.pi/state.md`,
   `.pi/notes/` or `docs/pipeline.md`, never here.

## Matching policy: flags and source variants

Two working rules that apply to **every** unit, agreed with the project owner:

1. **Apply the best-scoring variant even if it is not a full match.** A source rewrite or flag change is worth landing as soon
   as it *measurably improves* the objdiff score (`ninja build/RMHE08/report.json`, per-symbol `match_percent`) and regresses
   nothing else. Record the residual diff in the unit's **file header comment** (never as a per-function comment). Never land a
   change that makes any function worse. Landing means the unit's own source or `configure.py` carries the change and the repo
   rebuilds better (`python .claude/skills/mwcc-unit-matching/scripts/mt.py variants --apply <name>` then a forced rebuild);
   a probe-only winner is not progress.
2. **Evidence-backed flags live in `configure.py` as soon as they are proven**, even while the unit is short of 100 %. They are
   **per library** (never edit `cflags_base`/`cflags_runtime` for everyone), called out explicitly (rule 3 above), with the
   instruction/size evidence in a comment next to them, below `cflags_runtime`.

The *how* is the playbook index in `docs/matching/index.md` (see "Matching playbook").

## Operational mode: production runs

The full policy, with its dated reasons, is `docs/pipeline.md` 13.1 and the rulings in section 11. The rules a lane needs:

* **Six lanes at most**, unit workers and tool fixes together; **refill one slot per completion**, not in waves.
* **A slot is not filled while finished work sits unlanded** (`queue.py next` refuses; `--allow-unlanded <branch>` parks one),
  and **a slot goes to a problem before a new unit** (a gate refusal, a blocked lane, a tool that cannot express the work).
* **Claim a wave with `queue.py next --count N`**, never N adjacent proposals (the stride keeps two lanes off one TU).
* **Land one unit per commit, one at a time, from `main`** (`git rev-parse --abbrev-ref HEAD` prints `main`; `land.py` refuses
  otherwise). Land the orchestrator-side tools/docs batch first (`land.py land --already-applied`), then the lanes.
* **A lane gets the profile that matches its job** - `.claude/agents/` (tracked): `decompiler` (unit work), `fixer` (a refused
  gate or measured regression), `merger` (a refused apply / a fold), `codereviewer` (read-only review), `worker` (tooling and
  docs; the fallback), plus the read-only globals `scout`/`planner`/`reviewer`. `python tools/units/slots.py spawn --kind KIND
  [--slot N]` decides and prints the launch line (`unit`, `fix`, `merge`, `tooling`/`docs`, `review`, `scout`/`plan`; an unknown
  kind is refused). When a prompt and the profile disagree, **the profile wins**. A profile edit is not live until
  `tools/agents/install.sh` has copied it to `~/.claude/agents/` (it refuses when the section 6.5 block is stale).
* A lane is a headless `claude --agent <profile> -p <task>` at a slot (`tools/units/lanecmd.py`); it cannot block on a question -
  it ends its turn with the request and is resumed. Harness details: `docs/pipeline.md` 13.2.
* Keep `ninja build/RMHE08/ok` green and `orig/RMHE08/**` untouched as the invariant of every step.

The steady loop, per unit:

1. `queue.py next` claims one registered body-less unit - one worktree, one branch, one brief - and prints the paste-ready spawn.
2. The worker reconstructs the bodies and commits them on its branch, measured.
3. **Review before landing**: a read-only `codereviewer` lane judges the branch's own diff (`git diff main...<branch>`,
   `python tools/units/stylelint.py --ref <branch>`, per-function objdiff) for honesty of the match claim, naming, placement,
   types, comments and codegen hygiene; findings come back itemised and are cleared on the same branch by a `fixer` (or the
   decompiler lane). A review that finds nothing says so. This is `decomp -> review -> decomp` (`docs/pipeline.md` 1).
4. Land with the one landing path, `python tools/units/land.py land --branch worker/<slug> [--units <claim>] [--message SUBJECT]`:
   it refuses a dirty tree, records the base, applies the branch's merge-base diff three-way, resolves a `configure.py` /
   `splits.txt` registration conflict with its own scoped union (`land.py resolve`), runs the gate, commits with a pathspec
   and releases the claim; a refusal leaves the tree as it was. A conflict it refuses (a shared header, a `src/**` file) is
   resolved first with `python tools/units/mergebranch.py resolve`. The gate and the merge procedure: `docs/pipeline.md` 4-5.
5. `ninja build/RMHE08/ok` green, then refill exactly that one slot.

## Matching playbook

`docs/matching/` is the playbook for making a unit match its original object: **one file per idea**, `NNN-slug.md`, with a
permanent id (`docs/matching.md` is only an entry page so old "playbook N" citations resolve). It is **not** loaded into every
lane: load the project skill **`mwcc-unit-matching`** (`.claude/skills/mwcc-unit-matching/`) when a unit's functions mismatch.

* **The index** (id, title, status, tags, the problem each solves) is the generated `docs/matching/index.md`;
  `python tools/agents/ideas.py find <symptom words>` searches it, `ideas.py show N` / `where N` open idea N, `ideas.py new` adds
  one (atomic id) and `ideas.py check` is the gate. The loop, tags and demo format: `docs/matching/README.md`.
* **Never edit the generated files.** `python tools/agents/sync_playbook_index.py` writes `index.md`;
  `python .claude/skills/mwcc-unit-matching/scripts/sync_reference.py` writes the skill's byte copy `references/matching/`;
  both take `--check` and run from `tools/selftest.py` whenever anything under `docs/matching/` changes.
* `python .claude/skills/mwcc-unit-matching/scripts/mt.py` forwards to the `tools/` helpers (`units`, `info`, `frames`,
  `matrix`, `sweep`, `variants`, `shapes`, `diff`, `slots`, `sections`, `dwarf`, `ideas`).
* **How to work an idea:** one at a time, with evidence (numbers, sizes, first-divergence indices). An idea that works earns its
  own `docs/matching/NNN-slug.md` in the same session with the next free id (ids are never renumbered); an idea that fails is
  recorded as `status: ruled-out`; unit-specific findings belong in the unit's header comment.

Project skills are one tracked folder, `.claude/skills/`: `mwcc-unit-matching/` (the playbook), `symbol-map-editing/`
(`tools/symbols/symedit.py`), `objdiff-verify/` (proving a unit matches), `tu-boundary-discovery/` (`tools/splits/tudiscover.py`:
which functions and data form one translation unit) and `decompile-symbol/` (`symbolpreflight.py`, `m2cinput.py` + `tools/m2c`).

## Repository layout

```
configure.py              Project config + build generator (compiler flags, libs, tool versions)
config/RMHE08/config.yml  Analyzer/build settings, DOL path + hash, selfile (RSO list)
config/RMHE08/symbols.txt Symbol map: name = section:address; // type/size/scope  (generated, hand-editable)
config/RMHE08/splits.txt  Which address ranges belong to which translation unit / section
config/RMHE08/build.sha1  SHA-1 of each built artifact - the pass/fail check for the whole project
src/                      Our C/C++ source: src/<module>/<name>.<ext>, a unit registered once at its final home
include/types.h           The project's common scalar types (u8..s64, f32/f64, BOOL/TRUE/FALSE/NULL), one definition.
                          A declaration moves here the *second* time a unit needs it, never the first; an SDK type
                          (`GXRenderModeObj`, `Vec`, `Mtx`, ...) gets a `dolphin/` mirror here. `src/Camellia/camellia.c`
                          is the vendor exception and keeps its own typedefs.
orig/RMHE08/              Original game files (read-only, gitignored). main.dol, files/mh3.sel, ...
build/                    Everything generated: build.ninja, compilers/, tools/, RMHE08/ (gitignored)
tools/                    Tooling: dtk-template's scripts at the top level, plus ours grouped by what they do (agents/,
                          units/, splits/, symbols/, flags/, objdiff/, elf/, git/, lib/, mwcc-debugger/, mwlink-debugger/,
                          m2c/). The inventory with one line per tool is docs/tools.md -> docs/tools/inventory.md.
docs/                     All documentation, ours and dtk-template's; short bullets, one file per topic. plan.md is the
                          campaign plan; pipeline.md the mechanics and rulings; matching/ the playbook; tools.md the tool
                          docs; memory-dump.md the shared Ghidra runtime dump; splits-program.md the splits record;
                          rso-modules.md, data-order-seams.md, pool-seams.md, build-performance.md the topic notes.
```

### External oracles

* **Shared Ghidra project `MH3Shared` / runtime memory dump `/DolphinDump85.raw.keep`** (via the `ghidra` MCP server) turns a
  region of `fn_XXXX` into named SDK functions, gives a function's real signature, confirms struct offsets and reads data this
  repo does not own yet (details and recipes: `docs/memory-dump.md`). Three rules: it is **not** codegen evidence (flags still
  come from diffing the retail bytes, playbook 17); a `splits.txt` range derived from it is **measured before and after**; and it
  is **read-only** - the project is shared, so no renames, types, comments, imports or saves, and our names go in `symbols.txt`
  through `symedit.py`, never into Ghidra.

Inside `build/RMHE08/`: `src/<Unit>.o` is **our** compiled object (the candidate); `obj/<Unit>.o` is the **original** split out of
the DOL (the target); also `main.elf`, `main.dol`, `main.MAP`, `ldscript.lcf`, `report.json`, `progress.json`, `ok`. objdiff
compares candidate against target - keep the two straight.

## Build & verify

Windows-friendly (the supported setup): Python 3.12 + `ninja` on `PATH`; the toolchain is downloaded into `build/`.

```sh
python configure.py            # regenerate build.ninja + objdiff.json (needed after editing configure.py)
ninja                          # default target: build/RMHE08/progress.json (builds + verifies)
ninja build/RMHE08/ok          # build main.dol and check it against config/RMHE08/build.sha1  <-- the real test
ninja build/RMHE08/src/Camellia/camellia.o   # compile a single translation unit
```

Other targets: `ninja all_source`, `build/RMHE08/report.json` (objdiff report), `diff`, `apply` (never for a rename: it brings
generated names back), `baseline`, `changes`/`changes_all`, `tools`. `--warn all|error` adds warnings; `--non-matching` builds
"equivalent" code for extra units without linking them. On non-Windows hosts a `--wrapper` (wibo/wine) is required for the
Metrowerks compilers. `mwcc_sjis` wraps the compiler, so keep source UTF-8 with no BOM. If results look impossible the tree is
stale: `rm -rf build/RMHE08` then `python configure.py && ninja`. The `dol split` is the slow step and re-runs whenever
`symbols.txt`, `splits.txt` or the DOL changes (`docs/build-performance.md`).

Verifying a unit, function or symbol is its own procedure (per-symbol objdiff plus raw ELF evidence, and a set of traps:
`complete_code_percent` lies, `ninja build/RMHE08/ok` cannot isolate one unit, a function missing from the report is 0 %): follow
**`.claude/skills/objdiff-verify/SKILL.md`**.

## The core loop: adding / matching a translation unit

1. **Find the unit.** Locate the function in `symbols.txt` (grep or `symedit.py`), get its address and size, and the surrounding
   ranges in `splits.txt`. For an unnamed `fn_XXXX`, query the shared memory dump first (`docs/memory-dump.md`).
2. **Register it** in `config.libs` in `configure.py`: the right `mw_version` (a Wii title: `Wii/1.0` for REL-type code,
   `Wii/1.3` for runtime-style code - **not** the GC compilers) and a `cflags` group (`cflags_runtime` for runtime units,
   `cflags_base` otherwise). Start with `Object(NonMatching, "Dir/file.c")`.
3. **Create `src/Dir/file.c`** in the same change as the registration: a unit's source file is mandatory and is created *with its
   functions* - decompiled bodies wherever recoverable and measured against the target. A unit whose bodies cannot be written yet
   still gets its file, whose header says what it is, its range, why it sits there, what is unknown and where its evidence lives
   (`ledger.py unit <path>`, the map, `splits.txt`) - never a copied-out function list.
4. **Add the splits** to `splits.txt`: one line per section with exact `start:`/`end:` addresses, including the small
   `.ctors`/`.dtors`/`.sdata` fragments runtime units own (`.ctors$10`, `.dtors$10`, `.dtors$15`).
5. **Compile and diff:** `python configure.py && ninja build/RMHE08/src/Dir/file.o`, then inspect the unit's per-function diff.
   When it does not match, work the playbook ideas one at a time instead of guessing at flags.
6. **Flip to `Object(Matching, ...)`** only once the unit matches (bytes/instructions + relocations), one unit per commit.
7. **Prove it end-to-end:** `ninja build/RMHE08/ok` must finish green.

Keep changes small and verified. A micro-optimization for a function that already matched is a regression if the hash goes red.

## Gotchas

The incident behind each is in `docs/pipeline.md` 13.3.

* **Compiler-flag drift is silent and fatal**; per-function size deltas summing to the DOL growth is the diagnostic.
* **Don't trust a single objdiff number**: cross-check per-symbol `match_percent`, the unit `fuzzy_match_percent`, and
  `build.sha1`. A function entry with no `fuzzy_match_percent` key is 0 %.
* **`Object(Matching, ...)` is a claim, not evidence, and it changes the link** (`NonMatching` is `False`: not linked).
* **`.comment` is generated by dtk**, not evidence about the original compiler (decide it by codegen, playbook 4 and 17);
  **`extab`/`extabindex` presence** in the target is flag evidence to test on one unit.
* **`quick_analysis: false`** while boundaries are being discovered; **sizes/addresses are absolute** addresses of the unlinked DOL.
* **Stale `build/` gives false conclusions**: rebuild the unit; `rm -rf build/RMHE08` when in doubt.
* Local scratch (`.lavish/`, `.pi/`) is gitignored and never committed; `.agents/` is ignored except its tracked skills folder
  (`.claude/skills/`).

## Conventions

* **Commit messages follow one convention**: `<category>: <message>`, then an optional long description (rule 6 governs approval).
  * **`<category>` names where the change lives, and mirrors the tree**: `game/<module>` (decompilation: the module is the `src/`
    directory - `game/network`, `game/quest`, `game/menu`, `game/hud`, `game/pl`, `game/enemy`, `game/ef`, `game/g3d`,
    `game/nw24`, `game/dwci`, `game/camellia`, `game/os` ...); `tools/<area>` (the `tools/` groupings - `tools/agents`,
    `tools/units`, `tools/git`, `tools/symbols`, `tools/splits` ... - and any script's stem at any depth, e.g. `tools/land`;
    the member set is derived from the tree); `config/<what>` (`configure.py`/`symbols.txt`/`splits.txt`-only: `config/flags`,
    `config/symbols`, `config/splits`); `docs/<topic>` (documentation that is not this file: `docs/plan`, `docs/pipeline`,
    `docs/matching`, `docs/tools`); `agents/<profile|policy>` (a subagent profile or this file, `agents/policy`);
    `repo/<area>` (`repo/readme`, `repo/license`, `repo/ci`, `repo/gitignore`). The list is **open and there is no catch-all**: if
    nothing fits, add a category. `python tools/git/commitlint.py --message "<subject>"` checks the shape.
  * **`<message>` is imperative, says what was made, and is at most 120 characters** ("match the vtable slots", "remove the count
    cap") - not the problem, not the investigation.
  * **A long description is optional and structural**: which files, units, symbols or rows changed, the measured numbers. It must
    **not** carry reasoning (no *why*, no alternatives, no account of the work); that belongs in the unit header, the plan docs,
    the outbox or `.pi/notes/`.
* **Keep generated/large churn separate.** A `symbols.txt` regeneration or an analyzer settings change gets its own commit.
* **Commenting and naming** (every unit we write):
  * **A comment on top of a function is a short description of what it does** - one or two lines, present tense. It must not
    carry the symbol's name nor a matching percentage. Residuals, flag evidence and name provenance go in the unit's file header.
  * **The unit's file header comment is the one place for the unit's own notes**: what it is, the `.text` range and function
    order, where its flags/evidence live, the residuals, load-bearing source shapes. One line per fact; no per-function inventory.
  * **Use the real name when known** (retail map, the memory dump, the SDK), or a descriptive name in the **naming scheme of the
    surrounding symbols**. dtk's `FUN_`/`fn_`/`lbl_`/`unkNN` names are placeholders to replace (section 6.5 rule 7): derive one from
    what the function does and who calls it, what the data holds and who reads it; when the context supports only a guess,
    **guess** and mark it in the unit header. Vendor files keep vendor naming (`Camellia/` keeps its MPL-1.1 header).
  * **A rename is always two edits**, `symbols.txt` (it names the *target* object) and the source that defines/references the
    symbol, in the same change, through the proxy: `python tools/symbols/symedit.py rename <old> <new> --dry-run` first. Verify with
    `mt.py diff -u <unit> <symbol>` and an unchanged DOL hash. Never open or regenerate the map for this.
  * **Work under the campaign plan (`docs/plan.md` 6.5) is held to a stricter standard**: no `fn_XXXXXXXX`/`unkNN` survives in
    `src/`, every reconstructed type states its size, every field carries its offset and a context name (padding excepted),
    shared types live in one header, an `extern` lives with the owner unit, no pointer arithmetic to reach a field.
    `tools/units/stylelint.py` enforces the thirteen rules at the land gate (regenerate profiles with
    `tools/agents/sync_profiles.py` after a rule change).
* **Style:** match the file you are editing; new project code is 4-space-indent C. Files are UTF-8 and **LF in the repository and
  the working tree** (`.gitattributes`: `* text=auto eol=lf`; a clone must not set `core.autocrlf=true`; the hook is per clone:
  `git config core.hooksPath tools/git/hooks`). Edit through `python tools/agents/edit.py replace FILE --old-file A --new-file B`
  when a working copy may be CRLF.
* **Documentation:** `docs/` is the home for all documentation; put new knowledge there, short and to the point, one file per
  topic. dtk-template docs already in `docs/` stay authoritative for template behaviour.

## Before claiming success

* [ ] `ninja build/RMHE08/ok` passes (for anything affecting the linked DOL), or the change is described as unverified. **Check the
      `FAILED` count first**: `ok` is order-only and prints OK off a stale `main.dol`.
* [ ] For a unit/symbol: the object compiled **and** its objdiff diff shows the claimed match level (per-symbol `match_percent`,
      equal section sizes) - the `objdiff-verify` skill.
* [ ] `git status --short` shows only intended files; `symbols.txt` / `splits.txt` edits are byte-clean for untouched lines
      (a rename goes through `symedit.py rename`, so its diff is one line per symbol).
* [ ] For source or `include/` work: `python tools/units/stylelint.py --diff main` adds no section 6.5 violation.
* [ ] For a tool change: `python tools/selftest.py --changed main` is green (the gate runs all of it as the row "all tool selftests
      pass (except the parked list)"); a failure is fixed or parked in `tools/selftests-known-failures.json` with a reason and a date.
* [ ] No new compiler flags / tool version changes smuggled in.
* [ ] A lane **commits its own work on its own branch** - that is the deliverable; nothing beyond it is committed, nothing is ever
      pushed. Staged vs. unstaged state reported clearly.
