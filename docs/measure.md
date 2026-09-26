# `tools/units/measure.py` - the whole-unit measurer

`tools/units/recompile.py --measure <symbol>` answers **one** symbol per run: it recompiles the source and
issues two objdiff calls (the report for the score, the positional diff for the rows). For a *proof* that is
the right shape; for a *search* it is the wrong one. Workers looking for a shape wrote the same driver by
hand instead - the register's top request, **"Ship the worktree's scratch compile-and-score measurer", 61
votes** (`docs/tooling-requests.md`, key `scratch-measurer`), quoting `build/tmp/mp.py`,
`build/tmp/measure_all.py`, `build/tmp/scoreall.py`, `build/scratch/*.py`, `.pi/scratch/score.py`. `measure.py`
is that driver, shipped.

    python tools/units/measure.py <unit> [symbol] [--json] [--diff] [-q]

It compiles the unit once with the **real** command line - the construction is `recompile.py`'s
(`recompile.unit_tokens` -> MAIN's ninja, the worktree's ninja, then a same-lib sibling - plus
`recompile.compile_unit`, fresh-object assertion included), not a copy - then scores **every** symbol with a
single `objdiff report generate` over a one-unit project, the official `fuzzy_match_percent` the campaign's
`build/RMHE08/report.json` carries.

## What it adds over `recompile.py --measure`

| | `recompile.py --measure <sym>` | `measure.py <unit>` |
| --- | --- | --- |
| symbols per run | one | every symbol in the unit |
| compile | one per symbol | one |
| `report generate` calls | one per symbol (+ one `diff`) | one |
| output | one score | the unit's official measures, the `>= 80%` / `== 100%` counts, the `.text` totals, and the per-symbol table |
| run-to-run | none | a per-symbol **delta** vs the previous run (cache in `build/tmp/measure/`) |
| unit spelling | must carry the extension (`Camellia/camellia.c`) | `.c`/`.cpp` inferred from the worktree's `src/` |
| proposal target | MAIN's retired `auto_*_text` fallback | the worktree's **own split object** if it has one, then MAIN's registered object, then the fallback |
| symbol detail | the score only | `--diff` / a symbol focus prints the compact instruction-level mismatches |

`recompile.py --measure` stays the single-symbol proof; `measure.py` is for the round. `--json` makes it
scriptable from a sweep.

## Demonstration - `Camellia/camellia` (10 functions) in MAIN

    $ python tools/units/measure.py Camellia/camellia
    unit      Camellia/camellia.c
    command   main
    target    .../build/RMHE08/obj/Camellia/camellia.o  [registered]
    compiled  .../build/RMHE08/src/Camellia/camellia.o  (31720 bytes, fresh=True)
    score     99.96134 fuzzy   (unit, official report metric)
    functions 10 functions   9 == 100% (report matched_functions)   10 >= 80%   mean 99.98% of 10 scored
    text      24420 B target   24420 B ours

        score      delta   target    ours   symbol
           99.81%        .    4860    4860   camellia_setup256
          100.00%        .     496     496   Camellia_DecryptBlock
          100.00%        .      68      68   Camellia_Ekeygen
          100.00%        .     496     496   Camellia_EncryptBlock
          100.00%        .    3228    3228   camellia_decrypt128
          100.00%        .    4284    4284   camellia_decrypt256
          100.00%        .    3228    3228   camellia_encrypt128
          100.00%        .    4284    4284   camellia_encrypt256
          100.00%        .    3308    3308   camellia_setup128
          100.00%        .     168     168   camellia_setup192

`99.96134` is byte-for-byte the unit's `fuzzy_match_percent` in `build/RMHE08/report.json`, and
`camellia_setup256`'s `99.80576` is what `recompile.py --measure camellia_setup256` reports: the batched
report is the same metric, symbol for symbol (the selftest asserts both).

`recompile.py` answers the same unit one symbol at a time:

    $ python tools/units/recompile.py Camellia/camellia.c --measure camellia_setup256
      measure camellia_setup256: 99.80576% (official report metric; target 4860 B, ours 4860 B, paired=True)

and **cannot** be given the unit without its extension:

    $ python tools/units/recompile.py Camellia/camellia --measure camellia_setup256
    FAILED: Camellia/camellia   # Specified file '.../src/Camellia/camellia.cpp' not found

Wall time on the same tree: `measure.py Camellia/camellia` scores all 10 symbols in **0.76 s** (one compile,
one report); ten `recompile.py --measure <sym>` runs score the same 10 in **9.1 s** (ten compiles, twenty
objdiff calls). The loop's cost is now the compiler, not the measurer.

Running it twice is the search-loop form - the second run's scores are diffed against the cache:

    $ python tools/units/measure.py Camellia/camellia camellia_setup256 -q
    ...
    symbol    camellia_setup256    99.81%  target 4860 B  ours 4860 B   delta +0.00

`--diff` turns the focus into the rows a shape decision needs:

    $ python tools/units/measure.py Camellia/camellia camellia_setup256 --diff -q
    diff      1216 instructions, 6 differ  (arg_mismatch 4, delete 1, insert 1)
              0x7156   ARG_MISMATCH   target: add r6, r25, r6
                                        ours:   add r27, r25, r6
              ...

## In a worktree, for a proposal unit

Run it from the tree you are editing; the source, `-o` directory and `-i` order are that tree's
(`recompile.py`'s worktree path). If the worktree has not merged this tool yet, invoke MAIN's copy from the
worktree - the tool resolves the worktree from the *cwd*:

    $ cd <worktree>                       # ef/fn_8030681C is registered here, not on MAIN
    $ python <MAIN>/tools/units/measure.py ef/fn_8030681C -q
    unit      ef/fn_8030681C.cpp
    command   worktree
    target    .../ws-.../build/RMHE08/obj/ef/fn_8030681C.o  [worktree-split]
    compiled  .../ws-.../build/RMHE08/src/ef/fn_8030681C.o  (12256 bytes, fresh=True)
    score     14.14092 fuzzy   (unit, official report metric)
    functions 58 functions   9 == 100% (report matched_functions)   28 >= 80%   mean 92.93% of 28 scored
    text      27420 B target   4156 B ours

`recompile.py --measure fn_8030681C` on the same tree reads the same bytes through MAIN's retired
`auto_fn_8030681C_text.o` and reports `88.85714 %`, which is `measure.py ef/fn_8030681C fn_8030681C`'s
`88.86 %` for the same symbol. The unit has no split object on MAIN, so its target is resolved by address -
and the worktree's own split is preferred when it exists.

## Self-test

    python tools/units/measure_selftest.py

It pins the contract that made workers write this: a three-function unit issues **exactly one compile and
exactly one `report generate`**, even in the focused form; the worktree-split > registered > auto-fallback >
missing target order; the report's measures and the `>= 80%` / `== 100%` counts; the per-symbol delta and its
invalidation on a target change. With a build tree present it also asserts the unit score equals
`build/RMHE08/report.json` for a real unit.
