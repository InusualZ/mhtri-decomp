---
id: 321
title: Reordering a whole TU: classify real vs deferred-inline functions from the target's relocations, then generate the source order
status: works
problem: Every row of a unit is at 100 % but its `.text` symbol order, `.data` string pools and extab differ from the target, so the object cannot flip; the helpers were written as plain functions at the top of the file.
tags: [source-shape, sections, process, tooling]
applies: [Wii/1.3]
demo: 
related: [163, 298, 80]
---

# 321. Reordering a whole TU: classify real vs deferred-inline functions from the target's relocations, then generate the source order

**Problem.** A unit whose rows all score 100 % still does not flip: `python tools/units/textorder.py <unit>` reports
most symbols out of order (`g3d/g3d_anmchr`: 142 of 150; `ef/ef_drawstripestrategy`: 40 of 61), the `.data` tail of
weak `@STRING@` pools is permuted, and flipcheck lists `.data`/`.sdata2` as claimed and not emitted. Objdiff pairs by
name, so none of this shows in a row score.

**Why it happens.** The original source has few real (non-inline) functions; every helper is an `inline` that the
compiler emits as a weak copy right after the function that first references it (idea 163). Pending references pop
last-in first-out (reverse order of first reference in the caller), and a popped function's own new references are
processed before the next sibling, so a chain reads as a depth-first walk of the call graph in reverse reference
order. Weak string pools land in reverse definition order, `.sdata` pools in the same direction, so a unit's data is
decided by the order its helpers are *defined*, not emitted. Plain functions written at the top of the file in
address order give none of this.

**How to work it.**
1. `python tools/units/textorder.py <unit> --refs`: each target function with the target functions it calls, tagged
   D (a caller earlier in the text) or R. A function with an earlier caller is a deferred inline unless it is a
   non-inline member the compiler cannot defer: constructors reached through placement `new` in a `Construct` function
   and private members called from a deferred constructor were real here, `asm` functions and `math_reciprocal`
   style leaf helpers are always real (an `asm` block inside an inline function compiles to nothing).
2. Make every D function `__declspec(noinline) inline` (an explicit-specialisation constructor is
   `template <> __declspec(noinline) inline ...`; under `-inline noauto` only the `inline` keyword allows inlining, so
   `noinline` keeps the `bl`). Define each before its first caller; keep the R functions in retail address order.
3. Compare with `textorder.py`; a pair of swapped helpers means their first-reference order in the caller's source is
   the other way round, a constructor emitted last means it is real, not deferred.
4. Data: replace `extern` string and float declarations by literals, add `#pragma pool_data off` when retail
   addresses each string with its own `lis`/`addi`, and define the helpers that own strings in retail's reverse data
   order. `.rodata` type-name records are `TypeNameData<sizeof(s) + 4 - sizeof(s) % 4>` globals; a work buffer is a
   plain global array; small strings that retail names in `.sdata` are file-scope `char[]` in retail's order.
5. extab: `python tools/units/extabdiff.py <unit>` pairs each function's record through the `extabindex` relocations.
   A larger record of ours is an extra cleanup state: do the pointer-assert test of a destructor through a
   `static inline` helper whose block locals are declared in reverse and assigned in order (`__dt__AnmObjChrNode`
   108 -> 28 B).
6. For a 100-150 function unit write the reorder as a script (lex the top-level items, key each definition by
   class and method against the demangled target names, emit others, D, R) - hand-moving blocks cost more than the
   generator.

**Result.** Wii/1.3: `ef/ef_drawstripestrategy` (61 rows) and `g3d/g3d_anmchr` (150 rows, 28560 B) both flipped from
NOT READY to READY with every row still at 100 % and `main.dol: OK` on the trial flip; text order, `.data`,
`.rodata`, `.sdata`, `.sdata2`, `.bss` and extab all match. The ef unit needed three of the four steps above in two
commits; g3d_anmchr's generator moved 88 helpers and 70 real functions.

**Example.**

```
__declspec(noinline) inline void ef_stripe_noop(void) {}   /* emitted right after ef_stripe_draw_ribbon */
extern "C" void ef_stripe_draw_ribbon(...) { ... ef_stripe_noop(); }
```
