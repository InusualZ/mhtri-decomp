# `applyorder` - Write the declaration orders regsim asks for, keep each only on a strict gain, over a row list

## Purpose

Takes the plans of `regsim` (the declaration order whose colouring gives retail's registers) and writes them into the
function's source - at function scope and at block scope, with initialisers, several names on one line and `for` header
variables - measures each spelling the way `unittry` does, keeps a strict gain and restores everything else byte for
byte, round after round, for one function or for a whole row list across unit-disjoint workers.

## Users

Unit lanes on a register-only residual (`decompiler`, `fixer`); the campaign's batch passes over `remaining_functions.csv`
rows. Reached as `mt.py regsim run UNIT FUNCTION --apply-order` and `mt.py regsim run --rows-file F --apply-order`
(`regsim.py run` delegates here); `tools/units/applyorder.py row|batch` is the same code.

## CLI

```
python .claude/skills/mwcc-unit-matching/scripts/mt.py regsim run UNIT FUNCTION --apply-order [--plans N] [--rounds N] [--dry-run] [--restore] [--no-refresh]
python .claude/skills/mwcc-unit-matching/scripts/mt.py regsim run --rows-file ROWS --apply-order [--workers N] [--restore] [--row-timeout S]
python tools/units/applyorder.py row UNIT FUNCTION [options]      # the same, as the module's own entry
python tools/units/applyorder.py batch ROWS [options]
```

* **A round** is `regsim run`'s dump + search of the source as it stands, then the best `--plans` plans (default 3) in
  order; each plan's spellings (below) are written, scored and kept or restored. After a keep the plans are stale, so
  the next round dumps again; the run ends when a round keeps nothing (`--rounds`, default 4, caps it). A plan whose
  order is the source's own (rewrite cost 0) is skipped.
* A spelling is **kept only when `unittry`'s judge says so**: the unit's matched bytes rise strictly and no row falls
  (the official report metric of a fresh object, `tools/units/unittry.py`'s `Measurer`). Else the source is restored
  byte for byte, mtime and line endings included (`unittry.Workspace`). A kept run rebuilds the unit's object and
  `build/RMHE08/report.json` (`--no-refresh` skips; refreshes are serialised by a lock file under `.pi/tmp/applyorder/`).
* `--dry-run` prints every spelling as a unified diff (and every refusal with its reason) and writes nothing.
  `--restore` measures and puts the original source back at the end (the table still says what won).
* **The rows file**: lines `size,unit,function,pct`. A function name may contain commas: the line is cut from the left
  twice and the pct from the right; a trailing `,0xADDRESS` column (`remaining_functions.csv`) is dropped; blank lines,
  `#` lines and a header are skipped. A row already at 100 % in `build/RMHE08/report.json` (read at the start, never the
  csv's own pct) is `FULL` and not run.
* **Workers**: rows are partitioned by unit into at most `--workers` (default 3) lists, every unit whole in one list, so
  two workers never write one unit; each row runs as a child `regsim.py run UNIT FUNCTION --apply-order --json` whose log
  is `.pi/tmp/applyorder/<unit>__<function>.log`. One line per row as it completes, then a summary.

## The spellings of a plan (`lib/regsimorder.py`)

| spelling | what it writes |
| --- | --- |
| `scoped` | each scope's declarations reordered among their own lines. A scope with no initialiser, or whose initialisers keep their relative order with no statement between the slots, swaps the lines. Otherwise the plain declarations go to the scope's first slot in the plan's order and each initialiser stays at its own line as an assignment (`T x = f(a, b);` becomes `T x;` up top and `x = f(a, b);` in place). |
| `hoist` | idea 324 (`localedit.hoist_order`): every local the plan names declared at function scope in the plan's order, block initialisers left as assignments. |
| `hoist+for` | the same with each `for` header variable taken along (`for (u16 i = 0;` becomes `for (i = 0;` with `u16 i;` declared at the top), only when the name is unique in the function. |

A line with several names (`s32 a, b;`, `u8* p, q = 2;`) that declares a moved local is first written one name per line
(`T *a, b;` keeps `b` a plain `T`): the same declarations in the same order.

### Refusals (the row is `BADLINE reason`, nothing written)

* an initialiser that uses a local the plan declares after it; two initialised locals the plan swaps (their calls would
  swap); an initialised local that would move across a statement - the split form is tried instead;
* a `const` or brace initialiser, and a class-typed local with an initialiser, in a scope that needs the split form;
* a class-typed local that would move across a statement (its constructor would run elsewhere);
* a local used before the line its declaration is moved to;
* a `for` header variable the plan declares elsewhere in a `scoped` spelling (only `hoist+for` moves it), a `for` header
  that declares several names, a `const` for variable, a name declared more than once in the function;
* a plan naming a local that has no declaration in the source, or whose dump names do not line up (the mapping is empty).

### Row statuses

`KEPT` (a gain: bytes matched, the spellings kept), `SAME` (the best plan is the source's own order, or no plan),
`REVERTED` (spellings measured, none gained), `FEWFS` (no scope declares two locals the plan could reorder), `BADLINE`
(every spelling refused, the first reason), `NODEF` (the source defines no such function), `FULL` (already 100 %),
`DRYRUN` (`--dry-run` showed spellings), `ERROR` (no dump, no measure, a timeout).

## Inputs and outputs

Reads the unit's source and `build/RMHE08/report.json`; writes only the unit's own source (kept) plus the dump under
`.pi/tmp/regsim-run/`, the row logs under `.pi/tmp/applyorder/` and, after a keep, the unit's object and the report.

## Invariants and rules

* One source file is ever written, the unit's own; a shared header is never touched.
* A kept spelling raised the unit's matched bytes and dropped no row; the source on disk is always the accepted state
  (a spelling is written, scored, then kept or restored before the next).
* The rule for a spelling is `lib/regsimorder`'s; the measure is `unittry`'s; the plans are `regsim`'s: this tool adds none.
* A refusal prints its reason and writes nothing.

## Why `--apply-order` said "nothing to write" before

`--apply-order` wrote only a plan that listed block-scope locals (idea 324) and refused a function-scope-only plan as
"the order alone is `--tryvar`'s"; `--tryvar` and `--apply-local` took the best plan only and `--apply-local` stopped at
"the best plan is exact with no edit - nothing to write" for any plan whose registers were right, even when its
declaration order was not the source's. A plan 2 or 3, a reorder inside one scope, or an initialised declaration was
therefore never tried. This tool tries plans 1-3 and every spelling, and re-searches after each keep.

## Lib dependencies

regsimorder, trialrun (the unittry measure), regsim (`source_locals`, `find_definition`), report, units, repo. `regsim.py run
--plans-json F` is the one call into the search: a round is a `regsim.py run` process whose plans come back as JSON (each
declaration with its vreg, name, line and scope), and `regsim.py run --apply-order` / `--rows-file` is a process that runs
`applyorder.py row|batch` - a tool never imports a tool (`tools/tests/lib/test_layering.py`).

## Test contract

Tier: fixture - `tools/tests/units/test_applyorder.py`: the row parser (commas in the function, the address column, a
header, the pct from the right), the unit-disjoint partition, `regsim.declarators`, each spelling shape (several names on a
line, an initialiser with commas, a `for` header, block scope) and every refusal above, the status classification, and the
keep/restore loop (`try_plans`) on a temporary CRLF source with a scripted measure: a gain is kept with its endings, a
no-gain is restored byte for byte, `--dry-run` writes nothing. The dump and the compile are not in a fixture; they are
measured on real rows (the lanes' BADLINE rows, reported with the change).

## Known gaps

The judge is `unittry`'s over the whole unit (matched bytes up, no row down), not the one row: a gain on another row of the
unit counts. A `for` variable of a brace-less `for` is scoped to the next
block by `regsim.source_locals` (an old limit). Stack-slot order of locals that live in memory (address taken) is not in
regsim's model, so a plan that is already exact cannot gain from it.
