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

**Example.**

```c
void AXSetVoiceLpf(AXVPB* vpb, AXPBLpf* lpf);        /* not: const AXPBLpf* lpf */
void __VISetGammaCoef(VIGammaEntry* entry);          /* not: const VIGammaEntry* */
```
