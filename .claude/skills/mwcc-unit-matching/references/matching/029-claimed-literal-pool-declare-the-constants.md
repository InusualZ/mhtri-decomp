---
id: 29
title: A claimed literal pool: declare the constants, never define them
status: works
problem: a unit whose `.sdata2`/`.sdata` fragment is claimed in `splits.txt` still shows its pooled constants as `ARG` rows - the target loads `lbl_80795AC0@sda21`, our source loads a literal the compiler put in a pool of its own, and the names cannot pair. The obvious fix (define the constants in the source) is the wrong one: it rebuilds the pool, so the section and every load that references it changes.
tags: [sections, data, symbols]
applies: []
demo:
---

# 29. A claimed literal pool: declare the constants, never define them

Problem: a unit whose `.sdata2`/`.sdata` fragment is claimed in `splits.txt` still shows its pooled constants
as `ARG` rows - the target loads `lbl_80795AC0@sda21`, our source loads a literal the compiler put in a pool of
its own, and the names cannot pair. The obvious fix (define the constants in the source) is the wrong one: it
rebuilds the pool, so the section and every load that references it changes.

Why try it: the claim already put the target's pool *inside* the unit, so the map's names (`lbl_80795AC0`,
`lbl_80790E24`, ...) can be declared `extern` in the source and used as the operands of the loads. The compiler
then references the claimed address instead of pooling a new copy, and the rows pair by name.

Result: `main.cpp` (batch 3) declared its `.sdata2` constants (0x80795AA0-0x80795AD8) and its `.sdata` byte
instead of redefining them, which is what let the remaining float rows in `main`, `fn_8003FEBC`,
`fn_8003FF98` and `fn_8003FC64` be judged on their instructions rather than on a pool name.

Example: the `splits.txt` claim plus

```c
extern f32 lbl_80795AC0;   /* not `f32 lbl_80795AC0 = ...;` - that would rebuild the pool */
```

The claim is only half of it, and the pool's *extent* has to be read before claiming it (batch 6,
`Pl/pl_skill.cpp`): the unit's own run is bounded by its neighbours' (0x8079A030-0x8079A080, left by
`pl_master`'s last entry 0x8079A028, right by `pl_act`'s first 0x8079A080), and the entries it *shares* with
them (0.0f, 1.0f, 10.0f and the int->float magic) sit in the **preceding** unit's run, so no contiguous claim
can cover them. Claiming a pool the object does not emit is worse than not claiming it - the target's 80 B
section then pairs against nothing - so declare the unit's own constants `extern` and use them as load
operands (that alone took five `pl_skill` functions to 100 %, +0.072 pt for the unit, nothing worse). The two
implicit int->float magics the compiler emits on its own cannot be named from source at all: they are the
residual, and the header should say so rather than claim the range. Section 58 proves this, and gives the one case where the claim does link - a private pool entry.
