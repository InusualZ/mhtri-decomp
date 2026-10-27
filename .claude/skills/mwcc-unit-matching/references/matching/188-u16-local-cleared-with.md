---
id: 188
title: A u16 local cleared with a 16-bit constant keeps retail's andi. and its use-site clrlwi
status: works
problem: Retail clears bits of a u16 local with `andi. rN,rN,0xFFF7` / `rlwinm rN,rN,0,16,30` and masks it with `clrlwi r0,rN,16` at each test, where ours clears with `rlwinm r0,rN,0,29,27` + `clrlwi rN,r0,16` and folds the tests into one `rlwinm`.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
reviewed: 2026-10-27
related: [57, 95]
---

# 188. A u16 local cleared with a 16-bit constant keeps retail's andi. and its use-site clrlwi

**Problem.** A `u16` flag local copied from a field and then cleared bit by bit (`mask &= ~8`, `mask = (mask & ~1) | 2`)
compiles to a 32-bit `rlwinm` plus a `clrlwi` that re-narrows the local after every write, and every later
`(mask & bit)` test folds the narrowing into its `rlwinm`. Retail writes the local with `andi. rN,rN,0xFFF7` (or
`rlwinm rN,rN,0,16,30`), never re-narrows it, and puts a separate `clrlwi r0,rN,16` in front of each test - one
instruction more per test, two fewer per write, so the function differs in size as well as in shape.

**Why it happens.** `~8` is the 32-bit `0xFFFFFFF7`: the result can carry high bits, so the front end narrows the
local again after the write and then knows it is narrow at the tests. A 16-bit constant (`0xFFF7`) gives a result
that already fits, so the write is a plain `andi.` and no narrowing is recorded - the test then narrows the value
itself.

**How to work it.** When the writes are `andi.`/`rlwinm 16,N` and the tests start with `clrlwi r0,rN,16`, spell
every clear with the 16-bit constant (`mask &= 0xFFF7`, `mask = (mask & 0xFFFE) | 2`) and keep the local `u16`.

**Result.** `Pl/pl_hit` `hit_damage_em` (Wii/1.3, level 4): 97.66 -> 99.13 % (3972 -> 3984 B of 3988); the remaining
4 bytes were a `u8` status local and an unsigned shift.

**Example.**

```
u16 mask = hit->mask;
mask &= ~8;             /* rlwinm r0,r25,0,29,27 ; clrlwi r25,r0,16 ; later rlwinm r0,r25,0,30,30 */
mask &= 0xFFF7;         /* andi. r22,r22,0xfff7 ; later clrlwi r0,r22,16 ; rlwinm r0,r0,0,30,30  (retail) */
```
