---
id: 127
title: An inline-asm block scheduled into the C prologue - scope pragma optimizewithasm off to the function
status: works
problem: The first instructions of an asm block appear above the C prologue and the C initialisers, and two asm compares/subtracts trade places - retail kept the asm in source order.
tags: [pragma, flags]
applies: [GC/3.0a5.2]
demo: 
related: [122]
---

# 127. An inline-asm block scheduled into the C prologue - scope pragma optimizewithasm off to the function

**Problem.** A C function whose body is one `asm { }` block over `register` locals shows the block's first
instructions (`ps_merge00`, `psq_lx`) hoisted above the `lis`/`li`/`stwu` of the C part, and inside the block a
`ps_cmpu0` moved ahead of a `ps_sub`. Retail keeps every asm instruction in written order and schedules only
the C instructions around the block's start.

**Why it happens.** With the global optimizer on, MWCC (GC/3.0a5.2 here) feeds inline asm through the
scheduler like compiled code. The compiler carries a pragma for exactly this - its string table lists
`optimizewithasm` beside `volatileasm` ("treat inlined assembly as volatile") - and retail was built with the asm
left alone. `#pragma scheduling off` is the wrong lever: it also freezes the C part (94.49 against 99.55).

**How to work it.** Scope it to the function:

```cpp
#pragma push
#pragma optimizewithasm off
MTX34* MTX34RotXYZFIdx(register MTX34* pOut, register f32 fx, register f32 fy, register f32 fz) { ... }
#pragma pop
```

`#pragma volatileasm on` measured the same. Then re-check the colouring of the `register` locals the block
reads: their declaration order still decides which GPR each one gets.

**Result.** `nw4r/math_types` `MTX34RotXYZFIdx` (0x160): 90.91 -> 99.55 (pragma) -> 99.89 (the table pointer
declared before the circle-pair pointer); the remaining word is the pair pointer's register (retail r0, ours r7).

**Example.**

```cpp
#pragma push
#pragma optimizewithasm off
f32 f(register f32 a) {
    register const f32* p = sTable;
    asm { psq_lx f0, 0, p, 0, 0 ... }
}
#pragma pop
```
