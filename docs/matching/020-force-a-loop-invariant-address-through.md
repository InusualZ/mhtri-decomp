---
id: 20
title: Force a loop-invariant address through a `u32` local
status: works
problem: Retail materialises a loop-invariant field address into a callee-saved register in the loop preheader (`addi r30,r26,84` then `lwz r0,0(r30)`); our build folds it into a load displacement (`lwz r4,0x54(r27)`). That costs one callee-saved register and changes the colouring of the whole function (`_savegpr_24` where retail has `_savegpr_23`).
tags: [allocator]
applies: []
demo:
---

# 20. Force a loop-invariant address through a `u32` local

**Problem.** Retail materialises a loop-invariant field address into a callee-saved register in the loop
preheader (`addi r30,r26,84` then `lwz r0,0(r30)`); our build folds it into a load displacement
(`lwz r4,0x54(r27)`). That costs one callee-saved register and changes the colouring of the whole
function (`_savegpr_24` where retail has `_savegpr_23`).

**Why try it.** It is a codegen choice, not an algorithmic one, and taking the address (`&p->field`) does
*not* prevent the fold. Routing the address through an integer type does.

**Result.** `fn_804DA6C8` 94.77 -> **100 %** by computing `(u32)pObject + 0x54`, parking it in a `u32 buf[1]`
local, and declaring `int i;` before `count`. Worth trying wherever retail shows a preheader
`addi rN,rM,<disp>`.

**Example**

```c
u32 buf[1];
buf[0] = (u32)pModule + 0x54;   /* retail: addi r30,r26,84 ; lwz r0,0(r30) - not lwz r0,0x54(r26) */
```
