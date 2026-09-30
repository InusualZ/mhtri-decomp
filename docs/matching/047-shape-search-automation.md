---
id: 47
title: Automate the shape search: generate, compile, score and rank source variants
status: works
problem: Every near-match residual in this project has been *codegen* - an allocator web order, a branch direction, a register colouring - and the source shape that reproduces it was found by hand: the outboxes record "~200 source shapes tried", "a 360-permutation declaration-order sweep", "~30 non-volatile shapes". The comprehension was never the bottleneck; generating and *scoring* variants was.
tags: [tooling, source-shape]
applies: []
demo:
---

# 47. Automate the shape search: generate, compile, score and rank source variants

**Problem.** Every near-match residual in this project has been *codegen* - an allocator web order, a
branch direction, a register colouring - and the source shape that reproduces it was found by hand: the
outboxes record "~200 source shapes tried", "a 360-permutation declaration-order sweep", "~30
non-volatile shapes". The comprehension was never the bottleneck; generating and *scoring* variants was.

**Why try it.** The loop is mechanical and `tools/flags/tryvar.py` already does it for flags. The
source-side twin is `tools/flags/shapesearch.py` (generators in `tools/flags/shapes.py`): it locates one
function in the unit's source, generates shapes of its body - declaration order and types, `for`-decl
hoisting, named temporaries, casts and signedness, statement order, compound assignment vs assignment,
field form vs pointer arithmetic, dead copies (35), switch tail and `default`-first (34/37), condition
and ternary form, loop shape - compiles each through the unit's *real* ninja command line into a scratch
copy, scores it with the official report metric (`report generate`'s `fuzzy_match_percent`), drops
variants whose object hash repeats, and ranks. Depth > 1 is a beam over the best parents. Nothing writes
to `src/`; `--emit <label>` dumps the winning full source and its diff.

**Result.** On `Pl/pl_act`'s 20 worst functions (depth 1, all generators, 12 jobs): **12/20 improved**
in 83 s (~350 candidate compiles, ~0.3 s each, object-deduped), and two reached 100 % byte-identical:

* `fn_8027D40C` 96.228 -> **100.000**: `loop_decl_top` hoists the `for (s32 i = 0; ...)` declaration to
  the top of the body (`s32 i; s32 n = 0; for (i = 0; ...)`), which creates `i`'s live range before
  `n`'s and flips the r5/r6 pair. `.text` 228/228 bytes identical, official 100.0.
* `Pl_get_gunner_vec__FP4_PLWP10_CP_VECTOR` 93.939 -> **100.000**: `deadcopy_plain_x` inserts
  `u32 dc_x = x; (void)dc_x;` before the last statement (row 35's web-priority lever). `.text` 132/132
  bytes identical.
* combined (all 12 winners applied at once): unit mean 98.888 -> 99.086, 82/115 functions at 100 %.

The negative results are as useful as the wins: `fn_8027BC48` (99.237) only reaches 99.395 by swapping
`case 5`/`case 3`, because its residual is the order of the two *shared* `return 0`/`return 1` blocks,
which no source shape here reaches; `fn_8027D050` (98.939) is an r8/r9 web-order coin-flip that neither
declaration order nor a dead copy moves; and `fn_8027AF88`/`fn_80276E08` show **no differing row** at
`functionRelocDiffs=none` yet score 99.87/99.92 - their residual is relocation-only (a pool name), so
the shape search is the wrong tool and the `.sdata2` claim (23/29) is the right one.

**Example.**

```
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --top 8      # one function
python tools/flags/shapesearch.py -u Pl/pl_act --scan 20                    # worst 20, summary
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --depth 3 --beam 16
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --emit loop_decl:loop_decl_top_20
```
