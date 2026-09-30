---
id: 47
title: Automate the shape search: generate, compile, score and rank source variants
status: works
problem: Every near-match residual in this project has been *codegen* - an allocator web order, a branch direction, a register colouring - and the source shape that reproduces it was found by hand: the outboxes record "~200 source shapes tried", "a 360-permutation declaration-order sweep", "~30 non-volatile shapes". The comprehension was never the bottleneck; generating and *scoring* variants was.
tags: [tooling, source-shape]
applies: []
demo:
reviewed: 2026-09-29
related: [18, 19, 20, 34, 35, 37, 38, 63, 72]
---

# 47. Automate the shape search: generate, compile, score and rank source variants

**Problem.** Every near-match residual in this project has been *codegen* - an allocator web order, a branch
direction, a register colouring - and the source shape that reproduces it was found by hand: the outboxes record
"~200 source shapes tried", "a 360-permutation declaration-order sweep", "~30 non-volatile shapes". The
comprehension was never the bottleneck; generating and *scoring* variants was.

**How it looks.** A function sits at 90-99 % with the same opcodes, sizes and relocations as retail and only
register numbers (or one swapped pair of independent instructions) differ - the "register colouring" residual of
ideas 18/22/35/63. Nothing in the diff says which source spelling fixes it.

**Why it works.** The loop is mechanical: rewrite one function's body, compile it through the unit's *real* ninja
command line, score it with the official metric, keep what improves. `tools/flags/tryvar.py` does that for flags;
its source-side twin is `tools/flags/shapesearch.py` (generators in `tools/flags/shapes.py`).

**How to work it.**

* `shapesearch.py` locates one function in the unit's source, generates shapes of its body, compiles each into a
  scratch copy, scores it with `report generate`'s `fuzzy_match_percent`, drops variants whose object hash repeats
  and ranks the rest. Depth > 1 is a beam over the best parents. Nothing writes to `src/`; `--emit <label>` dumps
  the winning full source and its diff, which you then apply by hand.
* The generators (`--list-gens`, checked 2026-09-29): `switch` (default-first, arm order, `break`->`return`,
  ideas 34/37), `cond` (negate/swap branches, comparison sides), `compound` (compound assignment vs assignment vs
  `++`, idea 18), `stmt_order`, `decl_order` (idea 18/63), `loop_decl` (hoist a `for` declaration), `decl_type`
  (a local's type, `volatile`), `casts` (idea 38), `temps` (named temporaries), `ternary`, `loop` (idea 19),
  `field` (`p->f` vs `(*p).f`, idea 20) and `dead_copy` (idea 35).
* Read the result for what it tells you: a generator that wins names the lever (`loop_decl_top` = declaration order
  of the loop variable; `deadcopy_plain_x` = idea 35). A residual no generator moves is evidence the lever is not
  a shape at all (see below).

**Result (measured when the tool was introduced; the date was not recorded).** On `Pl/pl_act`'s 20 worst functions (depth 1, all generators,
12 jobs): **12/20 improved** in 83 s (~350 candidate compiles, ~0.3 s each, object-deduped), and two reached
100 % byte-identical:

* `fn_8027D40C` 96.228 -> **100.000**: `loop_decl_top` hoists the `for (s32 i = 0; ...)` declaration to the top of
  the body (`s32 i; s32 n = 0; for (i = 0; ...)`), which creates `i`'s live range before `n`'s and flips the r5/r6
  pair. `.text` 228/228 bytes identical.
* `Pl_get_gunner_vec__FP4_PLWP10_CP_VECTOR` 93.939 -> **100.000**: `deadcopy_plain_x` inserts `u32 dc_x = x;
  (void)dc_x;` before the last statement (idea 35's web-priority lever). `.text` 132/132 bytes identical.
* combined (all 12 winners applied at once): unit mean 98.888 -> 99.086, 82/115 functions at 100 %.

**When NOT to apply.** The negative results are as useful as the wins:

* `fn_8027BC48` (99.237) only reaches 99.395 by swapping `case 5`/`case 3`, because its residual is the order of
  the two *shared* `return 0`/`return 1` blocks, which no source shape here reaches.
* `fn_8027D050` (98.939) is an r8/r9 web-order coin-flip that neither declaration order nor a dead copy moves
  (idea 22: stop when retail's colouring is your exact mirror).
* `fn_8027AF88`/`fn_80276E08` show **no differing row** at `functionRelocDiffs=none` yet score 99.87/99.92: their
  residual is relocation-only (a pool name), so the shape search is the wrong tool and the `.sdata2` claim
  (ideas 23/29) is the right one.

Also not a substitute for reading the first divergence (idea 2): if the diff is a flag-shaped difference (idea 13),
sweeping shapes just burns compiles.

**Demo.** None: this is a tooling/process idea (the "demo" is running the tool on a unit with a real residual).

**Example.**

```
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --top 8      # one function
python tools/flags/shapesearch.py -u Pl/pl_act --scan 20                    # worst 20, summary
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --depth 3 --beam 16
python tools/flags/shapesearch.py -u Pl/pl_act -f fn_8027D40C --emit loop_decl:loop_decl_top_20
```

**Evidence.** Numbers above are from the `Pl/pl_act` run recorded with the tool's introduction; the unit and both
function names still exist in `symbols.txt` (checked 2026-09-29), but the scores were not re-run for this review
(the tool needs a warm build tree and the results depend on the unit's source at that time).
