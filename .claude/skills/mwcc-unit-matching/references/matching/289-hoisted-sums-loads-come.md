---
id: 289
title: A hoisted sum's loads come out in reverse operand order: name the sum and the shared load, sum declared first
status: works
problem: One `add rA,rA,r0` where ours has `add rA,r0,rA` after the same two loads; every operand order of the two field reads either swaps the loads or the add's operands, and a named local for the sum moves it one register up
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 63, 165, 278]
---

# 289. A hoisted sum's loads come out in reverse operand order: name the sum and the shared load, sum declared first

**Problem.** `lwz r7,0x3f8(r3); lwz r0,0x3fc(r3); add r7,r7,r0` in retail, `add r7,r0,r7` in ours: a sum of two fields that
the condition compares against and the assignment below reuses (`if (a + b < x + y) y = x + y - a`). Spelling the condition
`x + y` swaps the two loads instead; spelling the assignment the other way swaps the registers; a named local for the sum
lands it in r8 with the other load in r7.

**Why it happens.** The frontend (`-O3`) hoists the repeated `x + y` into a temporary ahead of the comparison. The first
PCode dump (`tools/mwcc-debugger`, `after-unreachable-block-deletion`) shows the rule: the two loads are numbered in the
order of the *second* occurrence (the assignment), emitted in the **reverse** of the condition's operand order, and the
`add` reads them in the condition's order. Loads in operand order and the sum on the first load's register is therefore
not reachable from the hoisted shape. The non-hoisted shape (a named local `sum = x + y`) emits the loads in order and
the add right, but the local is numbered below the frontend's `@` temporary for the shared load of `a`, so that temporary
is coloured first and takes the register (ideas 63, 165, 244).

**How to work it.** Name both: the sum, then the shared load, as locals declared in that order (the earlier declaration is
the higher node, coloured first), and use the locals in the condition and the assignment:

```
u32 windowEnd;
u32 sentOffset;
...
windowEnd = p->windowOffset + p->windowSize;
sentOffset = p->sentOffset;
if (sentOffset + p->sentSize < windowEnd) {
    p->sentSize = windowEnd - sentOffset;
}
```

Declaring the shared load before the sum colours them the other way round. Probe the shapes in a trial (`mt.py tryvar`)
and read the first PCode dump when the load order is the residual: it tells hoisted from non-hoisted at a glance.

**Result.** `Network/NetworkCommunityPat` `writeProfileRange_50` 99.81 -> 100 % (the row's one instruction), the unit's
last `.text` byte before the flip.

**Example.**

```
/* retail                                   ours (hoisted, condition `size + offset`)   ours (named sum only)      */
lwz r7,0x3f8(r3)   ; offset                 lwz r7,0x3f8(r3)                           lwz r7,0x3f8(r3)
lwz r0,0x3fc(r3)   ; size                   lwz r0,0x3fc(r3)                           lwz r0,0x3fc(r3)
add r7,r7,r0                                add r7,r0,r7                               add r8,r7,r0
lwz r8,0x500(r3)   ; sentOffset             lwz r8,0x500(r3)                           lwz r7,0x500(r3)
```
