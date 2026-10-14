---
id: 122
title: Swapped epilogue loads after an inline-asm bctr: scope scheduling off to the function
status: works
problem: A small function that ends in an inline `asm { ... bctr }` matches except the epilogue - the target restores `lwz r31` before `lwz r0` (the reverse of MWCC's scheduled order).
tags: [pragma, flags]
applies: [Wii/1.3]
demo: 
related: [32, 120]
---

# 122. Swapped epilogue loads after an inline-asm bctr: scope scheduling off to the function

**Problem.** A small function that ends in an inline `asm { ... bctr }` matches except the epilogue - the target
restores `lwz r31,0xc(r1)` before `lwz r0,0x14(r1)`, ours the other way round, so objdiff marks two `ARG` rows.

**Why it happens.** The backend scheduler hoists the `lr` reload ahead of the callee-saved restore so `mtlr` can
issue sooner. Retail built the function without the scheduling pass, which leaves the epilogue in its emission order
(callee-saved first). The rest of the unit was scheduled, so it is a per-function setting.

**How to work it.** Bracket the one function with `#pragma push` / `#pragma scheduling off` / `#pragma pop`
(idea 32: a region leaks until it is reset). Put the pragmas above the function's comment so the
`0xADDR (0xSIZE)` comment stays directly above the definition (rule 15). Re-measure the whole unit. The same
lever on scheduled, register-heavy functions is much worse (measured 64-85 % on fifteen OS rows), so it is a
probe for the epilogue symptom only.

**Result.** `OS/OSExec` `OSExecJump` (0x3C, `ICFlashInvalidate` then `sync; isync; mtctr; bctr`) 99.2 -> 100 %;
no other row of the unit moved.

**Example.**

```c
#pragma push
#pragma scheduling off
/* 0x804CE220 (0x3C): flushes the instruction cache and jumps to `entry`. */
void OSExecJump(register u32 entry)
{
    ICFlashInvalidate();
    asm {
        sync
        isync
        mtctr entry
        bctr
    }
}
#pragma pop
```
