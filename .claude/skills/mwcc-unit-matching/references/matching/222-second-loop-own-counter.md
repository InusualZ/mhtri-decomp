---
id: 222
title: Two loops sharing one counter swap two saved registers: give the second loop its own counter
status: works
problem: A function with two loops over the same counter variable has two saved registers swapped (the counter against a parameter or a result) while every instruction is otherwise identical; declaration orders do not move it.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [165]
---

# 222. Two loops sharing one counter swap two saved registers: give the second loop its own counter

**Problem.** `for (index = 0; ...) {...} for (index = 0; ...) {...}`: the counter takes r26 and a saved parameter
r29 where retail has them the other way round; `permdecl` over the declarations finds no better order.

**Why it happens.** One variable used by both loops is one live range web spanning both, and it is coloured with the
union of both loops' interference. Two variables are two webs, each coloured on its own, so the counter of the first
loop no longer competes with values that only live in the second.

**How to work it.** Give the second (or every later) loop its own counter variable of the same type
(`s8 j;`), then re-run `permdecl`/`hill` over the declarations - the separate web often needs one more order change
to land on retail's numbers. Also try naming a slot pointer that one loop derives as a block-local of that loop.

**Result.** `Network/NetworkSessionStable` `sendStream` 99.83 -> 100.00 (second loop on `j`);
`setNetworkConnectionEvent` 99.65 -> 99.97 (relay loop on `j`, the delivery loop's slot as a block-local `frame`,
then one declaration move found by `hill`).

**Example.**

```
for (index = 0; index < 4; index++) { selected[index] = ...; }
for (j = 0; j < 4; j++) { if (selected[j] != 0) { ... } }   /* was `index` again */
```
