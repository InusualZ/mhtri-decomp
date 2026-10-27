---
id: 244
title: Read the colouring order from the mwcc-debugger: reverse declaration numbering, split webs, conversion temps
status: works
problem: Same-size rows whose only diff is two callee-saved (or volatile) registers swapped, where blind declaration permutes find nothing.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 63, 124, 165]
---

# 244. Read the colouring order from the mwcc-debugger: reverse declaration numbering, split webs, conversion temps

**Problem.** A row differs only in register numbers (`lbz r31, 0x8c3(r30)` vs retail `lbz r30, 0x8c3(r30)`, `mr r31, r3`
vs `mr r30, r3`), and a declaration permute of the function's top block finds nothing, because the swapped node is not
one of the declared locals.

**Why it happens.** `regalloc-gpr-pass-1-assigned.txt` (tools/mwcc-debugger) lists the nodes in the order they are
coloured, and on Wii/1.3 that order is, for nodes that simplify normally, *descending virtual register number*
(a node whose degree stays too high, typically a parameter live over the whole body, is coloured first). The numbers
come from three tiers: the function's named locals are numbered in **reverse textual declaration order** (the first
declared, block-locals included, gets the highest number), then the frontend's `@NNNN` temporaries, then backend
temporaries in emission order. Three source shapes therefore move a node across tiers:
* a function-scope local **reassigned in a later, disjoint region** (a second `level = OSDisableInterrupts();`) is
  split by the frontend into an `@NNNN` web numbered above every local, so it is coloured early and takes r31;
* a **converting initialiser** (`u8 handle = cb->devHandle` where the field is `s8`) leaves the load in a backend
  temporary (the `rlwinm` conversion copies it into the local), and that high-numbered temporary is the node;
* an inline accessor's return value is a temporary (idea 165) - spelling the expression in place makes it a local.

**How to work it.** Run `python tools/mwcc-debugger/mwcc_debugger.py --gdb <gdb> -a '<unit command line>' FUNC DIR`
(the full unit works; `fetch_gdb.py` installs a gdb in ~2 min), read `regalloc-gpr-pass-1-assigned.txt` (`name` on a
row is the variable; unnamed rows are backend temps, find them in the last `after-interference-graph` dump), write
the order retail needs, then: reorder declarations so the node that should be coloured first is declared first;
give a later critical section its own block-local; type a local like the field it copies; spell an accessor in place.

**Result.** `WPAD/wpad` (Wii/1.3, -O4,p): `wpadInitDoneCallback` 99.80 -> 100 and `wpadAbortCallback` 99.83 -> 100
(`s8 handle`), `wpadShutdownCallback` 99.91 -> 100 and `wpadSendWbcCommand` 99.24 -> 100 (block-local `level` for
the second section, then declaration order), `wpadSendCommand` 99.27 -> 100 (declaration order read from the
dump: `handle` before `cb`, `data` after `reportId`), `wpadPollSample` 99.06 -> 100 (sample spelled in place +
`cb` declared before `changed`). A previous pass had searched declaration orders blindly on the same rows.

**Example.**

```
} else {
    u32 level = OSDisableInterrupts();   /* was `level = ...` reusing the function's local: an @web, r31 */
    ...
s8 handle = cb->devHandle;               /* was `u8`: the conversion temp was coloured first and took r31 */
```
