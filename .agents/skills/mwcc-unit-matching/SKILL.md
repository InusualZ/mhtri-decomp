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
3. **The unit's own header/source comments** - the residual diff of a unit is documented on top of the
   symbol that still differs (`src/<Lib>/<file>.h`, `src/<Lib>/<file>.c`).

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
   ninja can consider a same-second source edit up to date), and re-measure on the real object. Then record
   the evidence (sizes, per-function match, `matched_functions`, first-divergence index). Update the todo table's status column, and if the idea worked, write it into
   `docs/matching.md` in the house style - **Problem / Why try it / Result / Example**.

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
python scripts/mt.py diff   -u <unit> <symbol> [n] [--all]
python scripts/mt.py slots  -u <unit> <symbol> [--map] [--slot 0x64]
python scripts/mt.py sections -u <unit>
python scripts/mt.py dwarf  <obj> <function>
```

## Evidence rules (do not skip these)

* **A win that is not landed is not progress.** The point of the loop is that the unit's own source
  improves: apply the winning variant to `src/<Lib>/<file>.c` (`mt.py variants --apply <name>`), rebuild,
  and re-measure. Never leave a winner as a probe-only variant, and never record numbers taken from the
  probe object - the recorded evidence must come from the real unit.
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
