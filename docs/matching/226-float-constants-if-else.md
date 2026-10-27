---
id: 226
title: Bit-selected float constants: declare them first and set them with if/else statements, not ternary initialisers
status: works
problem: Two float constants chosen by type bits (`c1 = bit2 ? 1.5f : 0.0f`, `c0 = bit3 ? ...`) and a parameter `t` computed before them take each other's volatile FPRs (ours t f7 / c1 f5, retail t f5 / c1 f7) while every instruction matches; moving the declarations or the ternaries does not move it.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [63, 124]
---

# 226. Bit-selected float constants: declare them first and set them with if/else statements, not ternary initialisers

**Problem.** A cubic kernel computes `t = tick / 65536.0f`, then two control terms picked by type bits, each
`lfs` of `0.0f` or `1.5f` behind an `extrwi`/`cmpwi`/`bne`. The instruction stream is retail's, but `t` and the
first constant swap registers (`fdivs f7` / `lfs f5` against retail's `fdivs f5` / `lfs f7`). Declaring `t`, `c0`
and `c1` at the case, at the function top or in either order, and flipping the ternaries' sense, all leave it.

**Why it happens.** Not traced with the debugger; the likely reading: a ternary initialiser makes each constant one merged value born at its join; two assignment
statements in an if/else give the variable its own colour first and the two `lfs` write it directly, which changes
the order the allocator meets the three values.

**How to work it.** Declare the constants without initialisers, in the order retail's source most likely had
(`c0` then `c1` here, the reverse of their evaluation order), and set each with an if/else statement:

```
f32 c0;
f32 c1;
if (((type >> 2) & 1) == 0) { c1 = 0.0f; } else { c1 = 1.5f; }
if (((type >> 3) & 1) == 0) { c0 = 0.0f; } else { c0 = 1.5f; }
```

The declaration order matters for this spelling: `c1` declared first scored 99.64, `c0` first 100.

**Result.** `ef/ef_animcurve` `ef_anim_interp_u8` (Wii/1.3, -O3, peephole off): 99.52 -> **100**.

**Example.** As above; the f32 twin of the kernel (`ef_anim_interp_f32`) matches with plain ternaries, so try this
only when the constants' FPRs are the residual.
