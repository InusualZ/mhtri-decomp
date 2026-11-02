---
id: 282
title: An inlined callee's locals are numbered before the webs of the functions inlined inside it: an outer local that must colour last needs the caller's own body
status: works
problem: A function that inlines a callee (and the callee's own inlined calls) matches every opcode but two or three callee-saved registers are permuted, the callee's outer local (an interrupt level) sits one register too high, and no declaration order, block scope, split initialiser, opt_lifetimes or inline_bottom_up setting moves it
tags: [source-shape, allocator, pragma]
applies: [Wii/1.3]
demo: 282-inlined-callees-locals-are.cpp
related: [243, 244, 278]
---

# 282. An inlined callee's locals are numbered before the webs of the functions inlined inside it: an outer local that must colour last needs the caller's own body

**Problem.** `__DVDPrepareReset` calls `__DVDPrepareResetAsync`, which the compiler inlines whole together with the
`DVDPause`/`DVDResume` it calls. Retail colours the cancel loop's `level` r31, `DVDResume`'s level r30 and the
outer `enabled` r29; ours gives `enabled` r30 and the resume level r29 (6 bytes). Every declaration order of the
callee's locals, a block-scoped `level`, split initialisers, `#pragma opt_lifetimes off`, `inline_bottom_up on` and a
dead first store leave the two registers swapped (measured, 20 variants).

**Why it happens.** `regalloc-gpr-pass-1-all.txt` (tools/mwcc-debugger) shows every value in the caller is a
frontend web `@NNNN`, and the inliner creates them in a fixed order: the callee's parameters, then the callee's locals
in **reverse declaration order** (last declared = lowest `@`), then - in code order - the webs of each function
inlined *inside* the callee, then the post-inlining promotions (a global read twice, `executing`). A lower `@` is a
higher virtual register, and the Chaitin cascade of idea 243 colours from the highest down, so the callee's own
locals always colour **before** the inner functions' webs: `enabled` (`@1742`) before `level` (`@1743`) before the
resume level (`@1749`). Retail's order - `level`, resume level, `enabled` - means retail's `enabled` was numbered
*below* both inner webs, which an inlined callee's local never is: it is the **caller's own named local**
(numbered below every `@` web, `regsim search` plan: "declare @1742 as a local at declaration position 0").

**How to work it.** When `regsim want` shows the inlined callee's outermost local one register too high and the
inner inlined webs one too low, give the caller its own copy of the callee's body with that local declared in the
caller, and keep the inner part as an inline helper so its locals stay webs numbered above the caller's. If the
standalone callee must keep matching too, it keeps its own local (as a helper web it would colour before the inner
function's web): share only the part below that local, passing the value in as a helper parameter - a helper
parameter is numbered with the helper's locals, i.e. above the webs of what the helper inlines.

**Result.** `DVD/dvd` (Wii/1.3, -O4,p): `__DVDPrepareReset` 99.74 -> 100 (unit 52/53 -> 53/53, report metric
99.99582 -> 100.0, flipcheck READY, trial flip `main.dol: OK`). The shared helper `CancelAllForReset(callback,
level)` keeps `__DVDPrepareResetAsync` at 100 (`level` its own local, the helper call's argument); the SDK-shaped
`DVDCancelAllAsync` with a `retVal` and a helper that owns `level` both drop the standalone copy to 96.55.

**Example.**

```
static inline void CancelAllForReset(DVDCBCallback callback, BOOL level) { DVDPause(); ... DVDResume(); OSRestoreInterrupts(level); }

void __DVDPrepareResetAsync(DVDCBCallback callback)   /* standalone: `level` is its own local (r30 below the resume web r31) */
{
    BOOL level;
    BOOL enabled;
    ...
        level = OSDisableInterrupts();
        CancelAllForReset(callback, level);
    ...
}

void __DVDPrepareReset(void)                          /* its own body: `enabled` the caller's local, coloured last (r29) */
{
    BOOL enabled;
    ...
    enabled = OSDisableInterrupts();
    ...
        CancelAllForReset(cbForPrepareReset, OSDisableInterrupts());   /* `level` a helper parameter: above the resume web */
    ...
    OSRestoreInterrupts(enabled);
}
```
