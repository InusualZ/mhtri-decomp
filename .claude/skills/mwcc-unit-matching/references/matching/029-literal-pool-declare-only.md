---
id: 29
title: A claimed literal pool: declare the constants, never define them
status: works
problem: A unit whose `.sdata2`/`.sdata` fragment is claimed still shows its pooled constants as `ARG` rows - the target loads `lbl_80795AC0@sda21`, ours loads a literal the compiler pooled itself - and defining the constants rebuilds the pool.
tags: [data, sections]
applies: [Wii/1.3]
demo: 029-literal-pool-declare-only.cpp
reviewed: 2026-09-29
related: [12, 23, 58, 64]
---

# 29. A claimed literal pool: declare the constants, never define them

**Problem.** A unit whose `.sdata2`/`.sdata` fragment is claimed in `splits.txt` still shows its pooled
constants as `ARG` rows - the target loads `lbl_80795AC0@sda21` (a small-data-area load: a 16-bit offset from
the base register r2/r13, written `@sda21`), our source loads a literal the compiler put in a pool of its own, and
the names cannot pair. The obvious fix (define the constants in the source) is the wrong one: it rebuilds the
pool, so the section and every load that references it changes.

**How it looks.** The instruction (`lfs f0,0(0)`) matches, the relocation names differ: target
`R_PPC_EMB_SDA21 lbl_80795AC0`, ours `R_PPC_EMB_SDA21 @NN` (the compiler's anonymous pool entry).

**Why it works.** The claim already put the target's pool *inside* the unit, so the map's names (`lbl_80795AC0`,
`lbl_80790E24`, ...) can be declared `extern` in the source and used as the operands of the loads. The compiler
then references the claimed address instead of pooling a new copy, and the rows pair by name.

**How to work it.** Claim the pool's run in `splits.txt`, then declare each constant the unit's own code uses
(`extern f32 lbl_80795AC0;`) and write the expression against the symbol. Read the pool's *extent* before
claiming it: the unit's own run is bounded by its neighbours' (`Pl/pl_skill`: 0x8079A030-0x8079A080, left by
`pl_master`'s last entry 0x8079A028, right by `pl_act`'s first 0x8079A080), and the entries it *shares* with them
(0.0f, 1.0f, 10.0f and the int->float magic) sit in the **preceding** unit's run, so no contiguous claim can
cover them.

```c
extern f32 lbl_80795AC0;   /* not `f32 lbl_80795AC0 = ...;` - that would rebuild the pool */
```

**Demonstration.** `029-literal-pool-declare-only.cpp` (`ideas.py demo-check 29`): `x * lbl_pool_const` loads
through a relocation naming `lbl_pool_const` and adds nothing to `.sdata2`, while `x * 2.5f` makes a 4-byte pool
entry of the object's own (`.sdata2` = 4) named `@NN`; both functions are 0xC bytes.

**When NOT to apply.** Claiming a pool the object does not emit is worse than not claiming it - the target's 80 B
section then pairs against nothing. The two implicit int->float magics the compiler emits on its own cannot be
named from source at all: they are the residual, and the unit header should say so rather than claim the range
(idea 58 gives the one case where such an entry can be claimed: your unit is its sole referencer). A *partial*
`.sdata2` claim does not link at all (idea 23).

**Evidence** (dated 2026-09-2x). `main.cpp` (batch 3) declared its `.sdata2` constants (0x80795AA0-0x80795AD8) and
its `.sdata` byte instead of redefining them, which let the remaining float rows in `main`, `fn_8003FEBC`,
`fn_8003FF98` and `fn_8003FC64` be judged on their instructions rather than on a pool name. `Pl/pl_skill.cpp`
(batch 6): declaring its own constants `extern` and using them as load operands took five functions to 100 %,
+0.072 pt for the unit, nothing worse.
