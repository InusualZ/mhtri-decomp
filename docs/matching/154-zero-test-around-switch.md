---
id: 154
title: A zero test wrapped around an error switch folds into the switch's compare tree
status: todo
problem: Retail's error-code switch has no separate `cmpwi r,0 ; beq` ahead of it - the zero test is one node of the compare tree - where ours tests zero first and then runs the tree.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [34, 37, 109]
---

# 154. A zero test wrapped around an error switch folds into the switch's compare tree

**Problem.** A handler maps an error code to a reply status with a `switch`, only when the code is non-zero.
Ours tests the code against zero and branches before the switch's compare tree; retail's tree includes the zero
comparison and has no separate test.

**Why it happens.** `if (error != 0) { switch (error) { ... } }` lets MWCC merge the guard into the switch's
range lowering, so the zero test becomes one compare of the tree; a guard written as an early return (or a
`case 0:` arm) stays a separate test.

**How to work it.** Write the switch inside `if (error != 0) { ... }` and measure. A sibling shape from the same
unit: a result variable assigned in each case plus `break` (one return after the switch), not a `return` per case
(`TRKDoStep`).

**Result.** Used in `TRK/msghndlr` `TRKDoWriteRegisters` (Wii/1.3, unit header SHAPES line) while the unit went
from 2 to 12 of 13 rows at 100 %; the lever's own before/after was not recorded.

**Example.**

```c
if (error != 0) {
    switch (error) {
    case 0x702:
        error = 0x15;
        break;
    case 0x700:
        error = 0x13;
        break;
    ...
    }
}
```
