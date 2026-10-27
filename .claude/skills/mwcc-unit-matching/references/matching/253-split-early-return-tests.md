---
id: 253
title: Split early-return tests if-converted to subfc/subfe/andc: scope #pragma opt_rebuildconditionals off
status: works
problem: Retail tests a value twice with separate early returns (`cmpwi 0; bne; li r3,0; blr`, then a range test), while ours folds the pair into a branchless `subfc`/`subfe`/`andc` sequence, or the `||` spelling that avoids it puts a `beq` where retail has `bne`.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
related: [148, 120]
---

# 253. Split early-return tests if-converted to subfc/subfe/andc: scope #pragma opt_rebuildconditionals off

**Problem.** Retail writes `if (c == 0) return 0; if ((u32)(c - 0x80) > 0x3F) return 0; return 2;` and keeps both
tests as branches (`cmpwi r3,0; bne 1f; li r3,0; blr; 1: subi; cmplwi; ble ...`). Ours turns the pair into a
branchless select (`subfc`/`subfe`/`andc`), so the function is shorter and every later instruction shifts; the
`||` spelling that avoids the select branches on `beq` instead and scores lower still.

**Why it happens.** MWCC's front-end optimizer rebuilds conditionals (the internal `opt_rebuildconditionals`
pass) and merges two returns of constants into one conditional expression that the backend emits branchless.
Retail's build did not do that for this function.

**How to work it.** Write the retail shape (separate tests, separate returns) and bracket the function with
`#pragma push` / `#pragma opt_rebuildconditionals off` / `#pragma pop`. Probe it with a pragma sweep over the
function: the split form alone measures lower, the split form plus the pragma is the win. File-wide it can cost
other rows (`drawTextRuns` -58 B in the same unit), so scope it.

**Result.** `Network/network_pat_control` (Wii/1.3, -O4 by pragma, nopeephole): `getUtf8CharLength` 89.79 ->
100 % (+19.2 B); the split form without the pragma read 79.74.

**Example.**

```c
#pragma push
#pragma opt_rebuildconditionals off
s32 getUtf8CharLength(u8* bytes)
{
    ...
        if (second == 0) {
            return 0;
        }
        if ((u32)(second - 0x80) > 0x3F) {
            return 0;
        }
        return 2;
    ...
}
#pragma pop
```
