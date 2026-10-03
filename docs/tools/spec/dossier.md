# `dossier` - One page of what the split target object already knows about a unit: `__FILE__` strings, mangled names, panic lines, literals, references, jump tables, layout, blanks

<!-- generated from the module docstring of `tools/units/dossier.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Extract everything a split object already knows about a unit - one page, before any source is written.

## Users

imported by `brief`, `callees`, `callers`, `datagap`, `relocdiff`, `undefrefs`, `vtableaudit`

## CLI

```
python tools/units/dossier.py <unit> [--json] [--out FILE] [--no-dump]
python tools/units/dossier.py --selftest
```
Flags: `--json`, `--no-dump`, `--out`, `--selftest`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: target .o, DOL, map, dump -> page/JSON.

## Invariants and rules

* What it reports, and what each trace is evidence for (the plan's table, mechanised):
| section | the trace | what it tells us |
| --- | --- | --- |
| source file & asserts | `__FILE__` strings in `.data`, read from the DOL | the original source name (`ef_line.cpp`), hence the module and language |
| symbols | mangled names in the object's symbol table | the language (C++ vs C) and the signature |
| panic line map | the `li r4, line` before each `Panic` call | the source line of each assert, in order |
| literals | the `.sdata2`/`.data` pool, in section order | the order the literals appear in the source |
| external references | relocations in `.text` | which named symbol each load/call refers to |
| jump tables | dense 4-byte `.data`/`.rodata` relocations | the switch structure, including the case count |
| section layout | the object's section headers | the exception/constructor structure and the unit's extent |
| blanks | what is *not* there | a name with no dump entry, a signature with no callers, a type with no size evidence |
* The other oracle (`D:/WiiExperiment/DumpSymbols.zip`, `docs/memory-dump.md`) is consulted for names only - never for codegen. A name it does not have is reported in the blanks section, not guessed.
* Read-only: no `ninja`, no compile, no link, no write to any shared file.

## Lib dependencies

binary, ppc, project, names.

## Test contract

Tier: fixture; smoke: the ef_line.cpp unit when its object exists.
Today's selftest (`tools/units/dossier_selftest.py`): The pure readers run against synthetic data (a hand-built `.text` with three asserts, a dense jump table, a two-literal pool, a one-section DOL, a fixture dump zip), so they need no build tree and no compiler. When `build/RMHE08/obj/auto/800CCFB0_fn_800CCFB0.o` exists - the `ef_line.cpp` unit whose `__FILE__`/panic/`Panic` traces motivated the tool - the whole pipeline is checked against it, and the `brief.py` integration is checked by rendering that brief and looking for the dossier block. `dossier.selftest()` holds the checks so this entry point and `dossier.py --selftest` cannot drift.
Target: `tools/tests/units/test_dossier.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

imports `brief` (a cycle) - design 5 breaks it

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* docs/plan.md, "The binary is the ground truth, and compilation is lossy" (2026-09-23). This is a *matching* decompilation, so the binary is the only authority - and compilation is lossy, so what survives in the split object is *evidence about the source*: the original file name, the language, the source line of every assert, the order of the literals, the switch structure, the unit's extent. The method is to extract the maximum from the object first and fill only the blanks it cannot answer.
* Six workers independently rediscovered the same peephole lever, one spent a round working out that its unit's `__FILE__` string was `ef_cube.cpp`, another propagated "`Panic` is variadic" through 13 call sites by hand. Every one of those traces was already in the object. This tool is the one page that carries them, and `brief.py` embeds it so a worker's brief *is* the dossier plus the task.
