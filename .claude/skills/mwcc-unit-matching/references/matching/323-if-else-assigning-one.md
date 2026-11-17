---
id: 323
title: An if/else assigning one local is laid out like retail's when written as a ternary on the inverted test
status: works
problem: Retail branches over the fall-through arm of a two-way size or kind pick (`bne` to the second arm, the first arm below it) where our `if (c) A else B` branches the other way (`beq`) and emits the arms in the opposite order, so a handful of instructions sit in the wrong block and the loads and registers after them shift.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [34, 209]
---

# 323. An if/else assigning one local is laid out like retail's when written as a ternary on the inverted test

**Problem.** Retail tests `lwz r3,-4(r31); clrlwi. r0,r3,31; bne L2` and falls into the arm that reads the object's own
field (`lwz r5,8(r3)`), with the other arm (`lwz r0,-8(r31); clrrwi; subi`) below the branch. Our `if (owner & 1) A; else B;`
emits `beq L2` and the arms the other way round. The rest of the function matches, but every later load that the block
order moves (and a hoisted `lwz r0`/`lwz r3` pair after it) reads as a mismatch.

**Why it happens.** MWCC lays an `if`/`else` out with the `then` arm as the fall-through. A ternary assigned to one
local keeps its true arm as the fall-through of the condition as written, so writing the condition inverted
(`!(owner & 1) ? B : A`) puts the arm retail falls into first, with the `bne` the target has.

**How to work it.** Keep the two arm expressions as they are, swap them and invert the test, assigned to the same local:
`size = !(owner & 1) ? ((FixedSizeBlock*)owner)->client_size : SubBlock_size(sb) - 8;`. The same spelling of the plain
`if (!(owner & 1)) { size = B; } else { size = A; }` measured no change (the compiler normalises the inverted `if`), and the
ternary with the test as written measured no change either: only the inverted ternary moves the arms. Try it with `tryvar`
on the one statement, and re-measure the whole unit.

**Result.** `MSL_C/alloc` (Wii/1.3): `free` 94.38 -> 100.00 (the `Block* prev` local declared before `next` took it from
92.93 to 94.38 first: a shared `block->prev` load, retail's r3, instead of two loads).

**Example.**

```
/* retail: bne to the SubBlock path, the client_size load falls through */
size = !(owner & 1) ? ((FixedSizeBlock*)owner)->client_size : SubBlock_size(sb) - 8;
```
