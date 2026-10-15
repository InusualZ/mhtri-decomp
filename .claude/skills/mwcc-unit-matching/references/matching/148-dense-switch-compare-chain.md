---
id: 148
title: A dense switch lowered to a linear compare chain where retail has a compare tree
status: todo
problem: A switch of about six dense cases compiles in ours to a linear `cmpwi`/`beq` chain, while retail tests it with a binary compare tree (pivot compares with `bge`/`blt`).
tags: [source-shape, pragma]
applies: [Wii/1.3]
demo: 
related: [108, 109, 120]
---

# 148. A dense switch lowered to a linear compare chain where retail has a compare tree

**Problem.** A `switch` with about six dense cases is a linear chain of compares in ours; retail splits the case
range with pivot compares. Every branch after the first compare differs.

**Why it happens.** MWCC's front-end optimizer turns a small switch into an `if` chain ("optimizing switch into
ifs") when it judges it cheaper; retail's build did not, for these functions.

**How to work it.** In a scratch demo only `#pragma global_optimizer off` kept the compare tree; an if-chain, a
local selector, an extra far case, a `return` case and the other `opt_*` pragmas did not. Next step: bracket one
target function with the scoped pragma (idea 120) and re-measure the whole unit.

**Result.** Not yet measured on a unit. Targets: `WUD/wud` `WUDiLoadWiiFitSaveData` and `WUDiSyncFiber`
(Wii/1.3), whose retail switches are compare trees.

**Example.**

```c
#pragma push
#pragma global_optimizer off
static void WUDiSyncFiber(...) { switch (state) { case 0: ... case 5: ... } }
#pragma pop
```
