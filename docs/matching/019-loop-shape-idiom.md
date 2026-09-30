---
id: 19
title: Loop shape decides the loop idiom
status: works
problem: A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested loop; the wrong idiom adds or removes instructions and moves every later register.
tags: [source-shape]
applies: [Wii/1.3, GC/3.0a3]
demo: 019-loop-shape-idiom.cpp
reviewed: 2026-09-29
related: [18, 20, 71, 72]
---

# 19. Loop shape decides the loop idiom

**Problem.** A loop can compile to a `mtctr`/`bdnz` countdown, to a compare-and-branch, or to a bottom-tested
loop; the wrong idiom adds or removes instructions and moves every later register.

**How it looks.** The instructions right after the loop bound is computed differ: retail has a record-form shift
(`srawi. r5,r4,2`, `.` = the instruction also sets condition register 0) followed by `ble`, where ours has a
separate `cmpwi`/`cmplwi` first, or retail uses a counted `mtctr`/`bdnz` loop where ours has `cmpw`/`blt`
with an index.

**Why it works.** MWCC picks the idiom from the source's shape, and it shows in the first three instructions
after the bound is computed, so it is cheap to check before touching anything else. `while (count-- > 0)`
becomes a record-form shift + `ble` + `mtctr`/`bdnz`; `for (i = 0; i < n; i++)` becomes a `cmpw` + `blt`
index loop; an explicit second induction variable (`for (i = 1, off = sizeof(RSOSection); ...)`) is what
produces retail's separate index and offset registers.

**How to work it.** Try the three canonical shapes (`while (n--)`, `for (i = n; i > 0; i--)`, `for (i = 0; i <
n; i++)`), plus a two-variable `for` when retail keeps an index *and* an offset. Fix the shape first; the
percentage often does not move until the colouring is fixed too (idea 18): the idiom and the colouring are two
separate residuals.

**Demonstration.** `019-loop-shape-idiom.cpp` (`ideas.py demo-check 19`), one summing loop in three shapes, all at
`-O4,p`: `while (count-- > 0)` compiles to `srawi.` + `ble` + `mtctr` with no separate compare (0x88 B). **The
text this idea used to carry did not hold as written:** `for (i = count; i > 0; i--)` is *not* the same code -
it comes out 0xB0 B with several extra `cmpwi`/`ble` guards - and the up-counting `for (i = 0; i < count; i++)`
with `p[i]` is 0xD8 B. All three still end in `bdnz`, so "mtctr" alone is not a discriminator; compare the entry
test and the size. (The measurement was done with a pointer walk in the loop; an indexed body may differ.)

**When NOT to apply.** A loop with a call inside, or with a data-dependent exit, is not chosen by this
lever; the unrolling seen in the demo (eight-fold at `-O4,p`) also depends on the optimization level, so compare
under the unit's real flags (idea 16 for scoping it).

**Evidence** (`RSO/runtime`, dated 2026-09-2x). `fn_804DA7E4`: the `mtctr`/`bdnz` idiom and the `srwi.` entry
test only appeared with `while (count--)` (a `for (i = 0; i < count; i++)` gave `cmplwi` + `ble` and one extra
instruction). `RSOStaticLocateObject` reproduced retail's `r15..r31` colouring (and `_savegpr_15` instead of
`_savegpr_14`) only with the two-variable `for`.
