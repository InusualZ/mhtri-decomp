---
id: 254
title: A binary tree of signed range compares on one value is a switch whose case labels spell out each run
status: works
problem: Retail tests one value with a `cmpwi K; bge` tree (`0x17`, `0xE`, `5`, then `0x13`/`0x32`/`0x1A`) and lays the arms out after the tree, where a hand-written nested `if` interleaves the tests and the arm bodies
tags: [source-shape]
applies: [Wii/1.0]
demo: 
related: [108, 148]
---

# 254. A binary tree of signed range compares on one value is a switch whose case labels spell out each run

**Problem.** Retail compares one value against a handful of thresholds in a balanced tree of `cmpwi K` / `bge`
pairs, and every arm (a shared body, a `return`, a `break`) sits after the whole tree. A nested `if` tree written
from the thresholds (`if (id >= 0x17) { if (id >= 0x32) ... }`) compiles to the same compares in a different order
with the bodies inlined between them, and swapping the `if`/`else` order changes nothing.

**Why it happens.** At `-O3` MWCC lowers a sparse `switch` by grouping consecutive case labels with the same target
into runs and binary-searching the run boundaries with signed compares; the tree is the switch's, not the
programmer's. A hand-written `if` tree is laid out statement by statement instead. (At `optimization_level 4` the
same switch becomes `subi`/`cmplwi` range tests instead - idea 108.)

**How to work it.** Read the runs off the tree: each leaf is a half-open interval of the value and a target (a
body, the epilogue for `return`, the switch exit for `break`). Write a `switch` that lists every label of every
run (`case 0x05: ... case 0x0D:` for `[5, 0xE)`), the shared body once, and the `return` runs as their own case
group; values in no run are the implicit default.

**Result.** `Pl/pl_act` (Wii/1.0, `-O3`): `fn_80279C20` 95.62 -> 98.74 (case 4's act tree, 12 labels to the shared
gate and 28 to `return`); with idea 255 on its guards, 99.40.

**Example.**

```c
switch (self->act_no) {
case 0x05: /* ... */ case 0x0D:
case 0x17: case 0x18: case 0x19:
    if (self->stage <= 2) {
        return;
    }
    break;
case 0x13: /* ... */ case 0x16:
case 0x1A: /* ... */ case 0x31:
    return;
}
```
