---
id: 184
title: A run of identical stores to consecutive slots is a constant-count loop MWCC unrolls
status: works
problem: A long run of `stw r3,N(r31)` / `sth r0,N(rX)` with one value is matched by hand-written stores except for the scheduling of the value's `li` and the base register's `addi`, which land in the other order.
tags: [source-shape]
applies: [Wii/1.0]
demo: 
---

# 184. A run of identical stores to consecutive slots is a constant-count loop MWCC unrolls

**Problem.** A run of stores of one value to consecutive array slots (`li r3,0` then 48 `stw r3,N(r31)`, or four
`sth r0,0x18..0x1E(r31)` of `0x4D48`) looks like hand-written assignments, and writing them out explicitly gets
every store right but schedules the constant's `li` before the base register's `lis`/`addi` (or after the first
call), so the function stays at ~97 %.

**Why it happens.** MWCC fully unrolls a `for (i = 0; i < N; i++) a[i] = v;` with a small constant `N` into exactly
that store run, but the loop's constant and base are materialised by the loop's own setup, which the scheduler
orders differently from N separate statements.

**How to work it.** When retail shows an unbroken run of one value into consecutive slots, write the `for` loop
over the array (`counters_0x18[0x20]`, `counters_0x9C[0x10]`, `field_0x18[4]`) instead of the statements; keep the
index an `int`.

**Result.** `Pl/pl_motion` (Wii/1.0, `cflags_pl`): `vs_scene_work_init` 97.52 -> 100 % (the `system_w` ring of four
`0x4D48` halfwords as a loop), and both scene inits' 48-word counter clears reproduce retail's store runs as two
loops (`quest_scene_work_init` at 100 %).

**Example.**

```
for (i = 0; i < 4; i++) {
    system_w.field_0x18[i] = 0x4D48;   /* retail: addi r31,&system_w; li r0,0x4D48; sth x4 */
}
```
