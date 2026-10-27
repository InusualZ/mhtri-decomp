---
id: 227
title: A pointer plus a constant header plus a scaled index: the constant-first spelling sums into a scratch register
status: works
problem: Retail computes `p = base + i * n + 4` as `add r4,r4,r0` / `addi rDst,r4,0x4` (the sum in a scratch register), ours as `add rDst,r4,r0` / `addi rDst,rDst,0x4`; same instructions, one register different in each pair.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18]
---

# 227. A pointer plus a constant header plus a scaled index: the constant-first spelling sums into a scratch register

**Problem.** A record pointer past a 4-byte block header, `range = (T*)(random + r * rstride + 4)`, compiles to
`mullw`/`add`/`addi` in both objects, but ours adds into the destination register and then adds 4 in place, while
retail keeps the sum in a scratch register (`add r4,r4,r0`) and writes the destination with the `addi`.

**Why it happens.** Not traced with the debugger; the likely reading: MWCC folds the constant to the end in both spellings, and with the constant written last the
`add` result is the variable's own value and is coalesced with it, while with the constant written next to the base
the `add` result is an intermediate the allocator gives a scratch register.

**How to work it.** Write the constant beside the base: `random + 4 + r * rstride`, and for the other form of the
same address `randomTable + 4 + rstride * (...)`.

**Result.** `ef/ef_animcurve` (Wii/1.3, -O3, peephole off), both spellings applied at every site of the three curve
evaluators: `ef_anim_curve_u8` 99.01 -> 99.17, `ef_anim_curve_f32` 99.20 -> 99.37, `ef_anim_curve_rotate` 99.39 ->
99.54. A named temporary (`const u8* rnd = random + r * rstride; range = (T*)(rnd + 4);`) changes nothing (it is
copy-propagated).

**Example.**

```
range = (const EfAnimRandomRange*)(random + 4 + r * rstride);
```
