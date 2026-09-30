---
id: 35
title: A dead copy chain steers the allocator's web priority
status: works
problem: The residual is two live ranges sharing one register pair - retail colours them one way, we colour them the mirror - and no source shape, type, cast, statement order or flag moves it. `Pl/pl_master`'s `fn_8026F908` sat at 98.85 % (9 bytes) with the same 39 instructions; only the register pair differed.
tags: [allocator, source-shape]
applies: []
demo:
---

# 35. A dead copy chain steers the allocator's web priority

**Problem.** The residual is two live ranges sharing one register pair - retail colours them one way, we colour them
the mirror - and no source shape, type, cast, statement order or flag moves it. `Pl/pl_master`'s `fn_8026F908` sat
at 98.85 % (9 bytes) with the same 39 instructions; only the register pair differed.

**Why try it.** The allocator colours webs in web-list order, and the IR's *copy* webs participate in that order even
when the copies are dead. A chain of dead copies of the competing value, plus one separate live load of it, flips
which web is coloured first without changing a single emitted instruction. It is the **copy count** that matters:
chains of one or two copies, and equally many fresh loads, do nothing.

**Result.** `fn_8026F908` 98.85 -> **100 %**, the unit's `.text` byte-identical (0x45A0, 0 differing bytes, 24/24
functions 100 %, and the object links - flip 11). ~200 source shapes, all 30 toolchain compilers and every `-opt`
keyword were measured and ruled out first; `scheduling on` / `-O4` do flip the register but reorder the block.

**Example.** The dead copies exist only to steer the allocator and are optimised away.

```c
u32 classCopy0 = self->weaponClass;   /* dead */
u32 classCopy1 = self->weaponClass;   /* dead */
u32 classCopy2 = self->weaponClass;   /* dead */
u32 weaponClass = self->weaponClass;  /* the one live load */
if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1) { ... }
```

**Honest note.** This is a matching *trick*, not the original source - retail had no dead copies. Record it as such
in the unit's header comment (done there), and reach for a flag or a real source shape when one exists (row 33).
