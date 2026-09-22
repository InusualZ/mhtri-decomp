---
name: decompile-symbol
description: Turn one symbol (an address or a name) into source that compiles and is measured in mhtri-dtk - decompile it from the shared Ghidra runtime dump, write the file, register it in configure.py + splits.txt as Object(NonMatching, ...), measure it with objdiff, and hand the residual over. Use when asked to decompile or reconstruct a symbol or function from an address, to start a new translation unit from a symbol, or to make an unsplit region exist as source. Never links and never flips a unit to Matching.
license: MIT
compatibility: decomp-toolkit project layout; run it inside a git worktree, not the shared tree; needs the Ghidra MCP or the user's help to bring it up
metadata:
  author: mhtri-dtk
  tool: tools/units/symbolpreflight.py
---

# One symbol, from an address to a registered and measured unit

Input is **one symbol per run**, an address (`0x804DA598`) or a name (`RSOLink`). The output is four things
and nothing more:

1. source under `src/` (plus a header beside it when a shared declaration needs one),
2. an entry in `configure.py` as `Object(NonMatching, …)` and the ranges in `config/RMHE08/splits.txt`,
3. a measurement: the unit's per-symbol `match_percent` in `build/RMHE08/report.json`,
4. a residual handover - the unit's file header comment and a short report.

It is not a matcher. A unit is finished by `mwcc-unit-matching`; this skill makes it exist and be measured.
Never mark anything `Matching`: `NonMatching` keeps the original bytes in the link, so
`ninja build/RMHE08/ok` stays green by construction.

**Run inside a worktree** (`git worktree add -b <stream> ../mhtri-dtk.ws-<stream> <base>`) so a second
`ninja` cannot race this one and your dirty files are not another agent's. Never commit or push.

## 1. Pre-flight first - it is a gate, not advice

```sh
python tools/units/symbolpreflight.py <address|name>          # add --json for the full record
```

It reports the symbol, its section neighbours and the gap to them, the `splits.txt` unit that owns the
address, that unit's `configure.py` entry (`lib`, `mw_version`, `cflags`, `Matching`/`NonMatching`), a
**collision verdict** from the table below, and drafts of the two registration blocks. It writes nothing.

Read the verdict before anything else:

| severity | meaning |
| --- | --- |
| `escalate` | an anomaly that should not exist. Immediate attention, both records reported, nothing written. |
| `approve` | **analyse, propose, wait.** Say which attribution you believe and why, list the options with their numbers, then wait for approval before writing. |
| `proceed` | the owner's claim is weak or absent (an `auto_*` scaffold, a documented alias). Continue, and record the clash in the handover. |
| `never touch` | the owner is `Matching`; its bytes are linked in place of the original. |

The twelve kinds and what each one owes: **1** address inside a configured unit's range, **3** function
body overlapping a neighbour, **4** a data range already owned, **5** an `extab`/`extabindex`/`.ctors`/
`.dtors` fragment already owned → all `approve`; **2** inside an `auto_*` object → `proceed`; **6** the name exists at a
different address → `approve`; **7** two names at one address → `escalate` when two *function* names share
it, `proceed` for the alias groups this repo treats as normal (`lbl_*` beside a real name); **8** the owner
already matches → `never touch`; **9** a `fn_xxxxxxxx` name → `proceed`, and check the dump for the real
one; **10** the same C identifier in another unit → `proceed`, deduplicate through a header; **11** the
target is a thunk owned by the unit it forwards to → `approve`; **12** the proposed range abuts or overlaps
a neighbour → `approve`, and measure before and after.

A stop of kind 1 is not "report and give up": check whether the owning unit's source or object actually
**references** the symbol. If it does not, it probably does not belong there — propose the re-attribution
*with that reason* and ask.

## 2. Evidence, in this order

1. **Ghidra decompiler C** as the shape: `get_function_by_address` / `decompile_function` on the shared
   runtime dump (`docs/memory-dump.md`).
2. **The disassembly is authoritative**: every instruction-level decision is checked against
   `disassemble_function` / the target object. The decompiler is wrong in details often enough that the
   diff is the arbiter.
3. **Disassembly without the dump, for parallel work:** every function already has its target object in the
   build tree, so `build/binutils/powerpc-eabi-objdump.exe -d build/RMHE08/obj/<...>.o` is the authoritative
   disassembly and needs no Ghidra session. A Ghidra MCP session is per-agent: a subagent can be unable to
   reach it while you can, so hand parallel work the offline route.
4. **`m2c` as a second shape oracle, offline** (`tools/m2c`, a submodule): it decompiles the same
   disassembly to C and targets matching MWCC source, so it works with no Ghidra session and is the
   cross-check when the Ghidra shape does not look like the code.

   ```sh
   python tools/units/m2cinput.py build/RMHE08/obj/<unit>.o -f <symbol> -o build/tmp/<symbol>.s
   python tools/m2c/m2c.py -t ppc-mwcc-c --no-cache -f <symbol> build/tmp/<symbol>.s
   ```

   A unit that lives in `.init` (the runtime and boot code) needs `--section .init` on the first command: its
   default is `.text`, and the symbol is then simply "not found in the object".

   `m2cinput.py` exists because m2c wants GNU-as style asm, not `objdump -d` output; its docstring lists
   every rewrite and why (`@ha`/`@l`/`@sda21`, `loc_` labels, a mid-function tail call as `bl`+`blr`, the
   data-only `gap_*` blobs it has to drop). `--list` says what is in the object and marks the `bctr`
   (jump-table) functions; those work too - the table's bytes are read out of the original DOL and written
   into the output as `.data`, which is what m2c needs to rebuild the `switch`, and a table whose entries
   are not this function's case labels is refused with a warning rather than guessed at. If one is still
   refused, m2c aborts the *whole* file on that function: keep the others with `-f`. An instruction m2c
   has no pattern for (`mfcr`, `cmpwi cr1, …`) comes back as `M2C_ERROR(...)` inline: a visible gap in the
   shape, not a claim. Read the C as a starting shape and the disassembly as the arbiter - never as codegen
   evidence.
5. **Names, signatures and struct layouts** come from the dump (`get_struct_layout`, `get_function_signature`)
   - never treat the dump as codegen evidence, and never let it choose a compiler.
6. **If the Ghidra instance or program is unreachable: stop and ask the user.** Do not substitute guesses
   from a stale `symbols.txt`.

Then read the target: `python .agents/skills/mwcc-unit-matching/scripts/mt.py units` for what exists, and
the disassembly of the function you are writing (`mt.py diff -u <unit> <symbol>` once it compiles).

## 3. Write the source

* Path: `src/<Dir>/<file>.c`, following the module the dump reports, mirroring an existing sibling unit's
  directory (`src/Camellia/camellia.c`, `src/RSO/runtime.c`).
* Header: only when a shared declaration needs one. The project's common scalar types live in
  `include/types.h` - include it, never re-typedef `u8`/`u32`/`f32`/`BOOL` in a unit (a file that carries its
  own copy is a bug: the campaign moved three units onto the shared header, and the vendor exception is
  `src/Camellia/camellia.c`, whose `u32` is `unsigned int` and whose match rests on it). A declaration a
  *unit* shares internally goes beside the source (`src/<Dir>/<file>.h`), never in `include/`; an SDK type
  (`GXRenderModeObj`, `Vec`, `Mtx`, `OSHeapHandle`, ...) gets a `include/dolphin/` mirror rather than a fresh
  declaration per unit. Shared declarations are deduplicated through those headers, never repeated per unit.
* Names: use the dump's real name when it is known, rename through `tools/symbols/symedit.py` — **the map
  and the source in one edit** (skill: `symbol-map-editing`). Never invent a name: `fn_xxxxxxxx`/`unkNN`
  stay until the context supports one.
* **A generated name is the normal case, not the exception.** 20 502 of the 20 524 function symbols outside
  Camellia/RSO/Runtime are `fn_*`, and the runtime dump usually has only its own placeholder (`zz_..._`) for
  them. The name then stays generated and the **module evidence** comes from elsewhere: `Panic(__FILE__, line)`
  assert strings in the functions around it (they name the retail `.cpp`), the pooled string literals the
  region references, and clusters of mangled C++ names. That evidence picks the path — it is also what
  `tu-boundary-discovery` automates when it is available.
* Vendor sources keep their vendor formatting (`Camellia/` mirrors upstream). New project code follows the
  4-space-indent C style around it.
* **Section placement.** The compiler emits `.text` by default. When the symbol's section in the map is
  something else (`.init` happens: MSL runtime and boot code live there), objdiff pairs nothing and the unit
  reads as unmatched however good the code is. Force it on the definition —
  `__declspec(section ".init") void *memset(...)` — then prove it with `tools/elf/elfsect.py`: the object
  carries that section and `.rela.<section>`, and **no** `.mwcats.<section>` (which appears when the cats
  pragma is missing). Test on a scratch file with the *whole* flag list from `mt.py info` — a partial flag
  list invents differences of its own.
* The file's header comment is written on every unit (that is where the residual lives), kept to: what it
  is, its `.text` range and function order, where its flags and evidence live, the residual. No per-function
  inventory, no percentages, and a function's own comment is a one-line description with no symbol name in
  it.

## 4. Register

`configure.py` - a `config.libs` entry with the source as `Object(NonMatching, "<Dir>/<file>.c")`:

* `mw_version` is decided **per unit**. This repo has two outcomes in it: `Wii/1.3` (Camellia,
  Runtime.PPCEABI.H) and `GC/3.0a3` (RSO, playbook 17); the `Wii/1.0` in `configure.py` belongs to an unused
  helper, not to a registered unit. Take it from the neighbouring units and the dump's module, or sweep the
  matrix (`mt.py matrix`) and quote the numbers.
* **No commentary about the unit in `configure.py`.** A lib entry is `lib` / `mw_version` / `cflags` /
  `objects` and nothing else: the unit's notes belong in the unit's own file header comment, and the evidence
  in `docs/matching.md` and the playbook. The exception is a per-library `cflags_*` override, which may carry
  its instruction/size evidence beside it — that is flag evidence for a config value, never a description of
  the TU.
* `cflags`: start from `cflags_runtime`/`cflags_base`; sweep with `tools/flags/*.py` only when the diff is
  flag-shaped, and land a per-library `cflags_*` override with the instruction/size evidence in a comment.
  Never edit the shared `cflags_base` for one unit (non-negotiable 3).
* **Probe the `-O` level before registering, not after** (playbook 27). The schedule is a per-unit property:
  of the six units here, four want `-O3` and two want `-O4,p`. The target function sits inside the `auto_*`
  blob that covers its region at `offset = address - blob base`, so compiling your source under both levels
  and diffing the instruction window with `objdump` decides it without paying for a re-split. Do it before
  the first `configure.py` edit; `tools/flags/optsweep.py` reports frames only and cannot see order.

`splits.txt` - one line per section the TU owns, exact `start:`/`end:`:

```
<Dir>/<file>.c:
	.text       start:0x… end:0x…
	.rodata     start:0x… end:0x…
```

Include the small fragments a runtime unit owns (`.ctors`/`.dtors` with their `rename:.ctors$10` forms).
Ranges are **proposed by hand** from `splits.txt` and the section order, then measured before and after; a
range that lowers a match is reverted. (The discovery tool `tools/splits/tudiscover.py` is not in this
branch; when it lands, per the collision policy it is the *proposal* step, never the justification.)

## 5. Measure, and prove nothing regressed

```sh
ninja baseline                                                 # once, before the first registration
python configure.py && ninja build/RMHE08/src/<Dir>/<file>.o    # compiles the unit alone
ninja build/RMHE08/ok                                          # the link must stay green
rm build/RMHE08/report.json && ninja build/RMHE08/report.json   # the report is what the bar reads
ninja changes                                                  # this unit up, nothing else down
python .agents/skills/mwcc-unit-matching/scripts/mt.py info   -u <unit>
python .agents/skills/mwcc-unit-matching/scripts/mt.py diff   -u <unit> <symbol>
```

The `ok` target does **not** build a `NonMatching` object — those bytes are not linked, so always name the
object target explicitly. A `splits.txt` edit re-splits the whole DOL (minutes: 6–12 here), so **batch**
registrations: several sources and ranges first, one re-split and one report after them.

The bar, all of it: the unit appears in `report.json` with a **real per-symbol `match_percent`** (a
function entry with no `fuzzy_match_percent` is 0 % — and `complete_code_percent: 100` has coexisted with
`fuzzy_match_percent: 1.77`), a **new** unit additionally leaves every pre-existing unit's numbers
unchanged (a numeric gain is only required on later passes), no other unit or symbol regresses,
`ninja build/RMHE08/ok` is still green, and the object measured is the one the real ninja command line
produced — never a hand-written command. Per-unit verification detail: skill `objdiff-verify`.

Registering a range also re-attributes bytes between the new unit and the surrounding `auto_*` scaffolding, so
the DOL-wide totals move for bookkeeping reasons: judge by the unit's own per-symbol numbers, never the total.

**A number that disagrees with the object means a stale report.** `build/RMHE08/report.json` is generated
from the objects at report time, and a unit that had just measured 100 % read as `None` in it until the
report was regenerated. Delete it (or run `ninja changes`) before believing a number, and cross-check every
claim with `mt.py diff -u <unit> <symbol>`, which reads the objects directly.

## 6. Hand the residual over

* The unit's file header comment: the residual, the first divergence, the sizes, and where the flags live.
* A short report with the numbers: the unit's score before and after, the first divergent instruction, the
  residual region, and what was tried.
* `docs/matching.md`: a section in house style (Problem / Why try it / Result / Example) the moment an idea
  actually works — a win that only lives in chat is lost at the next compaction. Then regenerate the skill's
  copy in the same commit: `python .agents/skills/mwcc-unit-matching/scripts/sync_reference.py --check` has to
  come back clean, because `references/` is what a fresh session and **every subagent** load — when this rule
  was written it was 55 lines behind the playbook, so the knowledge existed and no agent would have seen it.
  Report in the handover what the run taught *and* what it tried that did not work: a `no` result saves the
  next session the same detour, and a unit-specific fact belongs in the unit's file header comment.
* `AGENTS.md`: **write it** when the change is a meaningful improvement to the system (a playbook row, a new
  idea, a corrected rule) and report the write in the handover. Keep the local-only block rules:
  `python tools/agents/localonly.py pull` before staging and `push` after a commit
  (skill: `agents-md-local-only`). It is not a free-for-all: another agent edits that file too, so only
  meaningful improvements, and never a speculative name or an unevidenced flag.
* **Prepare the commit — always, and stop there.** This is the last step of a run, because a finding that is
  not staged is a finding the next session re-derives:

  ```sh
  python tools/git/prepcommit.py            # classifies, stages the intended paths, writes the message
  python tools/git/prepcommit.py --split    # a one-concern-per-commit plan instead
  ```

  It stages explicit paths (never `git add -A`), refuses build output, `orig/`, `.lavish/`, `.pi/` and stray
  scratch, verifies the `main.dol` SHA-1 against `config/RMHE08/build.sha1` and reports it in the message, and
  writes the message with the measured results into the worktree's **private git dir**
  (`git rev-parse --git-path prepcommit_msg.txt`) - in a linked worktree `.git` is a *file*, so never
  hard-code `.git/...`.
  For `AGENTS.md` it pulls the LOCAL-ONLY block out before staging and pushes it straight back, so the working
  tree keeps its live section while the *index* holds the stripped blob (non-negotiable 8); it **refuses to
  stage the file** when the pull left no state to restore from, and warns when the block did not come back -
  both were real failures before the guard existed. **It never commits:** the user reviews
  `git diff --cached` and runs `git commit -F <that path>`. `--commit` executes it and is only for when they
  explicitly ask.
* **Apply each finding in the same session, uncommitted.** The user reviews the working tree: an idea that
  worked gets its `docs/matching.md` section *and* its `AGENTS.md` playbook row, a unit-specific residual goes
  into the unit's file header comment, a process or tooling fact goes into the affected skill or `docs/` page
  (e.g. the re-split cost in `docs/splits.md`, the stale-report rule in `objdiff-verify`). A finding that only
  lives in the reply is lost at the next compaction.

## 7. Never

Never mark `Matching` below 100 % · never touch `orig/RMHE08/**` · never change `mw_version` or a shared
`cflags` without instruction-level evidence (m2c's and Ghidra's C are shapes, not evidence) · never take a
range or a name another unit owns without the approval gate in §1 · never commit or push on your own
initiative (leave a *prepared* commit instead - §6) · never hand-edit `symbols.txt` (use `symedit.py`) ·
never edit another agent's files.
