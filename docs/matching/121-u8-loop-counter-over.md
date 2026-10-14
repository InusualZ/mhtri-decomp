---
id: 121
title: A u8 loop counter over a one-element array keeps retail's unfolded index multiply
status: works
problem: retail keeps `li rZ,0` ... `mulli rX,rZ,<stride>; add rX,base,rX` for an element at index 0 that our code folds into a plain offset
tags: [source-shape]
applies: []
demo: 
---

# 121. A u8 loop counter over a one-element array keeps retail's unfolded index multiply

**Problem.** A store sequence into element 0 of an array (a per-channel record in a manager with one channel)
diffs only by two instructions: retail scales a register that holds zero (`mulli r4,r6,0x6c; add r4,r3,r4`,
with `r6` the same `li r6,0` the zero stores use) and stores through `r4`, while ours folds the index and
stores through the object's own register. The row reads 4 instructions short and the stores all show `ARG`.

**Why it happens.** A `for` loop whose bound is the array's length (1) is fully unrolled late, after constant
propagation has run, so the multiply by the counter's value survives as `mulli` of the zero register. With an
`int`/`u32`/`s16` counter MWCC folds it (the loop is simplified before the multiply is generated); with a `u8`
counter the induction is kept and the unrolled body keeps the multiply. An index written as the literal `0`, or
an `inline` accessor called with `0`, folds too; indexing by a field that was just stored as zero keeps the
`mulli` but reorders the set-up.

**How to work it.** When retail multiplies a known-zero register by a record stride, write the per-element part
as `for (u8 i = 0; i < N; i++)` over the array (N = the array's length in the type), the element-0-only stores
inside, and try the counter types in order `u8`, `u32`, `s16` - one instruction pair decides it.

**Result.** `homebutton/tiHKBManager`, `fn_8056C090` (the keyboard manager's constructor, one 0x6C-byte channel
at +0x14): literal index / `int` loop 87.17 % (112 of 120 B), `self->b00` index 70.57 %, `u8` loop counter
100 %.

**Example.**

```
for (i = 0; i < 1; i++) {      /* u8 i */
    self->active[i] = 0;        /* folds to stb rZ,1(r3) */
    ch = &self->channels[i];    /* keeps mulli r4,rZ,0x6c; add r4,r3,r4 */
    ch->w04 = 0;
}
```
