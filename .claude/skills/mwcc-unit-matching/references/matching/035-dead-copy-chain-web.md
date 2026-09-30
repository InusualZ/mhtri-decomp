---
id: 35
title: A dead copy chain steers the allocator's web priority
status: works
problem: The residual is two live ranges sharing one register pair - retail colours them one way, we colour them the mirror - and no source shape, type, cast, statement order or flag moves it. The function sits at ~98.9 % with the same 39 instructions; only the register pair differs.
tags: [allocator, source-shape]
applies: [Wii/1.3]
demo:
reviewed: 2026-09-29
related: [18, 22, 33, 63]
---

# 35. A dead copy chain steers the allocator's web priority

**Problem.** The residual is two live ranges sharing one register pair - retail colours them one way, we colour
them the mirror - and no source shape, type, cast, statement order or flag moves it. `Pl/pl_master`'s
`fn_8026F908` sat at 98.85 % (9 bytes) with the same 39 instructions; only the register pair differed.

**How it looks.** The instruction *mnemonics* are identical and so is the length; every difference is a register
number (`r4`/`r6` swapped for the two values), typically a few adjacent instructions. That is the state idea 22
tells you to stop at - this idea is the one thing to try before stopping, once ideas 18 and 63 are exhausted.

**Why it happens.** The register allocator colours *webs* (a web is one value's set of connected definitions and
uses) in web-list order, and the IR's *copy* webs participate in that order even when the copies are dead. A chain
of dead copies of the competing value, plus one separate live load of it, flips which web is coloured first
without changing a single emitted instruction. It is the **copy count** that matters: chains of one or two
copies, and equally many fresh loads, do nothing.

**How to work it.**

1. Confirm the residual really is a mirror-colouring of the same instructions (idea 22) and that declaration
   order, temporaries and operand order (18, 63) are tried.
2. Add the value's copies *before* its real use, as a chain, then a separate live load that the code uses:

```c
u32 classCopy0 = self->weaponClass;   /* dead */
u32 classCopy1 = classCopy0;          /* dead */
u32 classCopy2 = classCopy1;          /* dead: a three-deep chain */
u32 weaponClass = self->weaponClass;  /* the one live load */
if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1) { ... }
```

3. Re-measure; vary the chain length (only some lengths flip the order). If no length works, record the residual.

**When NOT to apply.** This is a matching *trick*, not the original source - retail had no dead copies. Record it
as such in the unit's header comment, and reach for a flag or a real source shape when one exists (idea 33).
It is a last resort after the cheaper allocator levers, and it is easy to over-fit: if adding the chain moves a
*different* register pair, the mirror was not this one.

**Result.** `fn_8026F908` 98.85 -> **100 %**, the unit's `.text` byte-identical (0x45A0, 0 differing bytes, 24/24
functions 100 %, and the object links - flip 11). Measured at the time: ~200 source shapes, all 30 toolchain
compilers and every `-opt` keyword were measured and ruled out first; `scheduling on` / `-O4` do flip the register
but reorder the block.

**Demo.** None yet: the flip depends on a specific pair of competing webs inside a larger function, and a
minimal object that reproduces it has not been found (open: a minimal reproduction would need to be measured, not
guessed).

**Evidence.** Unit `Pl/pl_master` (the function is now `pl_act_param_tier_ck`; its unit header carries the full
probe log).
