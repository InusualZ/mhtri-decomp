---
id: 132
title: A const source pointer lets loads pass the stores: drop the const to keep retail's interleave
status: works
problem: Retail interleaves loads from a source record with stores into the destination (`lbz`/`stb`/`lbz`/`stb`), where ours hoists every load ahead of the stores and the row reads as a scheduling residual.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [60, 122]
---

# 132. A const source pointer lets loads pass the stores: drop the const to keep retail's interleave

**Problem.** A copy from a parameter record into a hardware block or another record comes out with all the
loads first and all the stores after them, where retail alternates load and store. Opcodes and count agree; the
order is wrong from the first store on, so it looks like a `#pragma scheduling` question.

**Why it happens.** Through a `const T*` the compiler may assume the stores cannot change what it reads, so the
loads are free to move above them (here: past stores to an address-taken local and to the destination record).
Through a plain `T*` a store may alias the source, so every load stays behind the store before it - retail's
order. The SDK prototypes that show this pattern simply take a non-const pointer.

**How to work it.** When retail's copy is load/store interleaved and ours is all-loads-first, drop the `const`
from the source parameter in the prototype *and* the definition (the prototype lives in the owner's header,
rule 2; its consumers are then re-measured, idea 60). Remove any scheduling pragma that was added to imitate the
order and measure again: the pragma is usually no longer needed.

**Result.** Wii/1.3: `VI/vi3in1` `__VISetGammaCoef` 72.84 -> 100 with the non-const entry pointer and the
`#pragma scheduling off` removed; `AX/AXVPB` `AXSetVoiceLpf` 68.46 -> 100 and `AXSetVoiceAdpcmLoop` 80.79 -> 100
with the non-const prototype (the latter also copies the three fields one by one; a struct assignment through the
non-const pointer measured 74.71).

**The same licence deletes a dead store.** Wii/1.0 (`Pl/pl_hit_sphere` `pl_capsule_box_set`, 97.96 -> 100): retail
writes `box->reach = vec3_distance(...)` (`stfs f1,0x34(r31)`), forwards the reload as `frsp f1,f1`, loads
`capsule->radius` and stores the sum. Through a `const PlCapsule* capsule` the first backend
`common-subexpression-elimination` pass (mwcc-debugger dump 04, right after unreachable-block deletion) treats the
radius load as unable to alias the store and deletes the first store as dead - the row is 4 bytes short and the
`extabindex` size word reads 0xC0 against 0xC4. Through a plain `PlCapsule*` the same pass still forwards the reload
(`frsp`) but keeps the store. No per-function pragma restores it (`opt_dead_assignments`, `opt_common_subs`,
`opt_lifetimes`, `opt_propagation`, `opt_dead_code`, `global_optimizer`, `optimization_level 2` all measure the same),
and giving both parameters one type or embedding the source record in the destination does not either: the `const`
is the lever.

**Example.**

```c
void AXSetVoiceLpf(AXVPB* vpb, AXPBLpf* lpf);        /* not: const AXPBLpf* lpf */
void __VISetGammaCoef(VIGammaEntry* entry);          /* not: const VIGammaEntry* */
void pl_capsule_box_set(PlCapsule* capsule, PlCapsuleBox* box);   /* not: const PlCapsule*: the dead store stays */
```
