---
id: 63
title: A local's DECLARATION ORDER colours registers - locals are coloured before parameters
status: works
problem: A function is byte-equivalent except that two callee-saved registers are swapped (retail `lis r30` for a table and the parameter in r31, ours the mirror), and no shape inside a statement moves it.
tags: [source-shape, allocator]
applies: [NHTTP]
demo: 063-declaration-order-colours.cpp
reviewed: 2026-09-29
related: [18, 22, 35]
---

# 63. A local's DECLARATION ORDER colours registers - locals are coloured before parameters

**Problem.** A function is instruction-for-instruction equivalent except that two callee-saved registers are
swapped: the message-group address is materialised late (`lis r31`) where retail materialises it first
(`lis r30`, with the parameter in r31), and that one swap shifts every use after it. It reads as allocator
luck.

**How it looks.** The diff is nothing but register numbers on a handful of rows (`r30`/`r31` mirrored), the
opcodes and relocations agree. Idea 22 says to stop when retail's colouring is your exact mirror *and* nothing
moves it; try this first.

**Why it happens.** MWCC's register allocator colours the **local** webs (a web is one variable's live
range) before the **parameter** webs, so the order the locals are declared in is the order their webs enter the
colouring: a local that must live in `r31` has to be written **before** its siblings. A parameter can never be
moved to the front (proved on the original unit: copying the parameter to a local first did not do it), because
its web is coloured after every local's however it is written. This is idea 18 with a mechanism: there
"declaration order" meant the operands of one expression; here it means the order of the declarations above
the body.

**How to work it.** Move the declaration of the local that should get the higher callee-saved register
(`r31`) to the top of the block, re-measure. Try the other permutations of the locals; it is cheap.

**When NOT to apply.** Only the *declaration* order matters, not the order of the assignments: in a
two-local scratch test (`phase`/`state`, both assigned after declaration) swapping the declarations changed
nothing. If the residual is not a pure register swap, this is not it.

**Result.** Measured at the time (2026-09), two independent filers:

* `NHTTPi_Startup` **0 -> 57.16 -> 93.85 %** with `const char* messages = NHTTPi_startupMessages;` declared
  **first** (unit 75.19 -> 89.36); `NWC24SuspendScheduler` / `NWC24ResumeScheduler` to **98.85 / 95.67 %**
  with `NWC24RequestWork* work = &sNwc24Work;` first;
* `Network/fn_8041A87C`'s `ConnectToAnybody` **99.40 -> 100** on `s32 phase; s32 state;` (retail `r31`/`r30`).

**Example.** The whole change is where the first line sits:

```c
s32 NHTTPi_Startup(u32 group)                 /* retail: `lis r30` for the table, parameter in r31 */
{
    const char* messages = NHTTPi_startupMessages;   /* FIRST - its web is coloured before the parameter's */
    u32 i;
    ...
}
```

**Demonstration.** `063-declaration-order-colours.cpp` (`ideas.py demo-check 63`): the same loop with the
table pointer declared before / after the counter: the pointer is `r31` in one and `r30` in the other, while the
parameter is `r29` in both.
