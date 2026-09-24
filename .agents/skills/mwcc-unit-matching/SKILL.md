---
name: mwcc-unit-matching
description: Work the matching playbook for a translation unit whose compiled object does not match the original in an MWCC (GameCube/Wii) decomp-toolkit project - per-unit objdiff measurement, reading the first divergence, isolating compiler flags against the real ninja command line, attributing a symptom with scratch files, the frame-size and size-gap traps, and the evidence rules for claiming a win. Use when a unit's functions mismatch, when picking or sweeping compiler flags for a unit, or when asked to "match", "get to 100 %", or "fix the codegen" of a unit.
license: MIT
compatibility: decomp-toolkit project layout (configure.py, build.ninja, ninja, build/tools/objdiff-cli.exe, MWCC under build/compilers, split target objects under build/<version>/obj/). Tools are invoked through the repository's own tools/ directory.
metadata:
  author: mhtri-dtk
  canonical: docs/matching.md
  todo-table: AGENTS.md (section "Matching playbook")
  generated-references: references/ (regenerate with scripts/sync_reference.py)
---

# Matching an MWCC translation unit

A unit that compiles but does not match the original object is the normal state of a decompilation. This
skill is the method for closing that gap: measure one unit, read the *first* divergence, decide whether the
cause is a flag or the source, isolate one change at a time, and only claim a win with evidence.

## Read these first (they are the source of truth, not this file)

1. **`AGENTS.md` -> "Matching playbook"** - the index table of every idea in the playbook, the problem each
   one solves, the status column that doubles as the todo list, and the current target/step. Work that
   table; this skill is the method behind it.
2. **`docs/matching.md`** - the canonical playbook. `references/playbook.md` here is a generated copy;
   never edit it, regenerate it (see *Keeping in sync*).
3. **The unit's own file header comment** - the residual diff of a unit (what still differs and why) is
documented there, in one place. Per-function comments are short descriptions of what the function does and
carry **no** symbol name and **no** match percentage; the naming and commenting rules are in `AGENTS.md` ->
Conventions ("Commenting and naming").

## The loop

1. **Measure one unit** at instruction level - never the project-wide pass/fail:
   `python scripts/mt.py diff -u <unit> <symbol> 40` for a side-by-side listing, or
   `ninja build/<version>/report.json` and read `matched_functions X/Y` for that unit.
2. **Read the first divergence**, not the percentage (`match_percent` is positional - one extra
   instruction in the prologue reports ~0 %). Name the code shape it implies.
3. **Classify**: a fused instruction, a register-save idiom, a table base or a prologue is a *flag* lead;
   an algorithm, signature or unrolling difference is a *source* lead.
4. **Isolate** exactly one change, against the *real* command line for that unit (the tools read it from
   ninja, so `--flags-extra` overrides rather than guesses).
5. **Verify, then land it.** A winning change is not finished when it is measured - it is finished when it
   is in the unit's source and the *repository* builds better. Apply the winning rewrite with
   `python scripts/mt.py variants --apply <name>`, force a rebuild (`rm -f <unit obj> && ninja <unit obj>`;
   ninja can consider a same-second source edit up to date), and re-measure on the real object.
6. **Record the idea in the same session, before starting the next function.** Every idea that produced a
   win gets a row in `AGENTS.md`'s playbook table *and* a section in `docs/matching.md` in the house style -
   **Problem / Why try it / Result / Example** - with the numbers taken from the *unit's* object, then
   `python scripts/sync_reference.py` so `references/playbook.md` is not stale. Record it at the moment it
   works, not at the end of the session: a win that lives only in the chat or in a scratch report is lost at
   the next compaction and the next unit re-derives it from scratch. This step is part of the win.

## The ideas, in short

Full text with examples: `references/playbook.md` (same numbering as `AGENTS.md`'s table).

| # | idea | one-line why |
| --- | --- | --- |
| 1 | Per-unit instrument | the project-wide pass/fail cannot measure one unit |
| 2 | First divergence, not the percentage | positional match % makes "one instruction off" look like 0 % |
| 3 | Read the target's disassembly | the diff says *what* differs, not what code shape to look for |
| 4 | Codegen is the oracle, not `.comment` | a synthesized version fingerprint invites a dead-end version hunt |
| 5 | One flag at a time, real command line | hand-written commands drift; several flags in play hide the cause |
| 6 | Guard against stale objects | MWCC's `-o` is a directory; a stale object reports impossible results |
| 7 | Scratch files to attribute a symptom | can't tell a source idiom from an optimizer pass otherwise |
| 8 | Ask the compiler what is on (`-opt display`) | one `-O` level sets several switches |
| 9 | Enumerate options from `-help` | invented spellings are silently ignored |
| 10 | Frame size is not a success signal | 16-byte rounding makes frame hits look like answers |
| 11 | Size gap is not "different source" | aggressive flags *remove* instructions |
| 12 | Check the flag's relocations | the code shape can match while the symbols do not exist |
| 13 | Stop when the diff is not flag-shaped | a near-miss variant that fixes one symptom keeps you hunting |
| 14 | Prove the committed flags reproduce the object | the `cflags_*` list can silently differ from the tested command |
| 15 | Pin a metric that does not drift | fuzzy % changes between objdiff versions |
| 16 | Scope optimizer settings per function with pragmas | the `-opt` levers are global, so a per-function codegen difference looks unreachable from the source side |
| 17 | Cross-family version matrix | `mw_version` is inherited project-wide and the target's `.comment` is synthesized, so a unit built by a different toolchain looks like an unexplainable residual |
| 18 | Named temporaries, declaration order, operand order | the allocator colours live ranges from the source's temp structure, so a register-only residual *is* source-reachable |
| 19 | Loop shape decides the loop idiom | `mtctr`/`bdnz` only comes out of the right source shape, and the wrong shape shifts every later register |
| 20 | Loop-invariant address through a `u32` local | taking `&p->field` does not stop MWCC folding it into a displacement, which costs a callee-saved register |
| 21 | Record-form count as the peephole/scheduling fingerprint | `-opt` is per unit, and the old whole-DOL `extrwi` scan could not see the fused form at all |
| 22 | Stop when retail's colouring is your mirror image | only the allocator's web priority is left, so no source shape can help |
| 23 | `splits.txt` data ranges: what objdiff can and cannot fix | defining a symbol fixes name rows only by (section, offset) and can drop the target's `R_PPC_NONE` pool relocs |
| 24 | Merging a probe into the unit is its own step | probe numbers are not unit numbers, and one struct definition must serve every function |
| 25 | Shared memory dump as a name/signature/struct oracle | unnamed `fn_*` functions and untyped structs resolve in one query (`docs/memory-dump.md`) |

Already ruled out for this project (do not re-run): `references/ruled-out.md`.
A worked example of the whole loop: `references/worked-example.md`.

## Tools

One entry point, forwarding to the repository's `tools/` (they are versioned with the code and are
unit-agnostic; `-u` accepts `Lib/file`, `main/Lib/file`, `src/Lib/file.c` or an object path, and can be
omitted when the repo has a single unit with source):

```
python scripts/mt.py units                        # which units exist
python scripts/mt.py info  -u <unit>              # resolved paths + the exact ninja command line
python scripts/mt.py frames -u <unit> [--flags-extra "..."] [--versions ...]
python scripts/mt.py matrix -u <unit> [--flags-extra "..."] [versions...]
python scripts/mt.py sweep  -u <unit> [subs...]
python scripts/mt.py variants -u <unit> [--variants f.py] [names...]
python scripts/mt.py shapes -u <unit> [-f <function>] [--scan N] [--gens ...] [--depth N]  # source shapes
python scripts/mt.py diff   -u <unit> <symbol> [n] [--all]
python scripts/mt.py slots  -u <unit> <symbol> [--map] [--slot 0x64]
python scripts/mt.py sections -u <unit>
python scripts/mt.py dwarf  <obj> <function>
```

## External oracles: the shared memory dump (names, signatures, structs, data)

This repository also has access to a second, independent Ghidra project holding a **runtime memory dump
of the game** (`MH3Shared`, program `/DolphinDump85.raw.keep`, reachable through the `ghidra` MCP
server). For the matching loop it answers questions the retail object cannot:

* **The real name of a `fn_XXXX`.** Look it up before reading any code: `search_functions` (by pattern)
  or `get_function_by_address` (by address); one `mcpScript` fanning a list of addresses through
  `get_function_by_address` names a whole contiguous unit in a single call. In the `RSO/runtime` unit that
  yielded `RSONotifyPreRSOLink`/`RSONotifyPostRSOLink`/`RSONotifyModuleLoaded`/`RSONotifyModuleUnloaded`
  (the four 4-byte thunks), `LocateObject` (`fn_804D9B4C`), `RSOUnLink` (`fn_804DA6C8`), `FindExportIndex`
  (`fn_804DA834`), `RSORelocate` (`fn_804DAA24`) and `RSORelocateSmallDataSection` (`fn_804DABF0`) - which
  also identified what the unit *is* (the RSO linker), so the source could be written with real parameter
  types from the start.
* **Parameter and return types** (`get_function_signature`): e.g. `RSOLink(RSOModule*, RSOModule*, ...)`
  showed the second argument is the *exporting* module, not a private "relocation table".
* **Struct layouts with real field names** (`get_struct_layout`): the dump's `RSOModule` (88 bytes)
  confirmed every offset that unit had derived by hand, and let the source say
  `export_symbol_table_offset`/`import_symbol_names_offset` instead of `unk40`/`unk54`.
* **Data contents outside the unit's split** (`read_memory`): `.data:0x80629B90` is that unit's string
  pool, with its messages and function pointers visible.

Two guardrails, both learned the hard way:

1. **It is not codegen evidence.** Flags and the compiler family come from diffing our object against the
   retail bytes (idea 17). The dump's `PowerPC:BE:32:Gekko_Broadway` language is a CPU setting, not an
   answer to "which compiler built this".
2. **Its annotations mix real SDK identifiers with Ghidra placeholders** (`zz_04da7e4_`,
   `SPEC1_MakeStatus`), so trust a name only when its signature agrees with the code you are matching. And
   when you turn its data into `splits.txt` ranges, measure before *and* after: claiming the RSO string
   pool made a function 1.35 points worse (defining the symbol makes dtk drop the target's
   `R_PPC_NONE` pool relocations) while claiming its jump table was worth +0.08.

Details, recipes and worked examples: `docs/memory-dump.md`.

## Evidence rules (do not skip these)

* **A win that is not landed is not progress.** The point of the loop is that the unit's own source
  improves: apply the winning variant to `src/<Lib>/<file>.c` (`mt.py variants --apply <name>`), rebuild,
  and re-measure. Never leave a winner as a probe-only variant, and never record numbers taken from the
  probe object - the recorded evidence must come from the real unit. For a probe written by hand (rather
  than by the variant tool) the merge is its own step: unify the types with casts at the use sites and
  re-measure **every** function of the unit afterwards (idea 24).
* **A win that is not recorded is not progress either.** The idea goes into `AGENTS.md`'s table and a
  `docs/matching.md` section *in the same session* (loop step 6) - ideas discovered on the way to the
  target, not just the one that finished it.
* A flag change is only acceptable with a measured result: sizes and per-function match, never "it looks
  better". Frame size alone is not a result.
* Prefer `matched_functions X/Y` from the report over any fuzzy percentage, and state the tool version
  when quoting one. Compare like with like: the probe's *file-mode* match percentage and the project
  report's *project-mode* percentage can differ slightly (tens of a percent) for byte-identical objects.
* After `--apply` or a source restore, force the rebuild and check the object actually changed
  (`sha1sum`, or the frame via `mt.py frames --obj <obj>`). A stale object silently reports the previous
  result.
* One idea at a time, and record the *first-divergence index* - it is the thing that tells the next person
  where the change landed.
* Never edit `configure.py` for everyone: proven flags go into a per-library `cflags_*` override.
* If the idea fails, say so in the todo table as `no` so it is not re-run.

## Adding a new idea

1. Add a `todo` row to `AGENTS.md` -> "Matching playbook": the idea and the problem it solves.
2. Try it with the tools above, on one unit (`mt.py variants <name>` runs the probe).
3. **Land it if it wins.** `python scripts/mt.py variants --apply <name>` writes the rewrite into the
   unit's real source (line endings preserved; it refuses when the rewrite does not apply or changes
   nothing). Force a rebuild, re-measure on the real object, and only quote those numbers.
4. Record it: a section in `docs/matching.md` in the house style (Problem / Why try it / Result /
   Example, short) with the next free number, the row set to `done`, and the target/step updated in the
   local-only plan. Then run `python scripts/sync_reference.py`.
5. If it failed, leave it as `no` with a one-line note. A failed idea never touches the source.

## Keeping in sync

`references/` is generated from `docs/matching.md`. After any playbook edit run:

```
python scripts/sync_reference.py           # regenerate
python scripts/sync_reference.py --check   # exit 1 when stale (safe for hooks/CI)
```
