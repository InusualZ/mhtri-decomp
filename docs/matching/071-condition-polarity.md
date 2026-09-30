---
id: 71
title: The condition's POLARITY decides the exit; an assigned boolean gives `cntlzw`/`srwi.`
status: works
problem: A function whose logic and instructions are right still misses by a branch shape: an early return arrives as a `beq`+`b` pair where retail has one `bne`, or a `memcmp` test is `cmpwi`/`bne` where retail builds a boolean with `cntlzw`/`srwi.`.
tags: [source-shape]
applies: [DWCi]
demo: 071-condition-polarity.cpp
reviewed: 2026-09-29
related: [34, 18, 22]
---

# 71. The condition's POLARITY decides the exit; an assigned boolean gives `cntlzw`/`srwi.`

**Problem.** A function whose logic and instruction set are right still misses by a register shuffle and an
exit: an early return arrives as a `beq`+`b` pair where retail has one `bne`, or a `memcmp` test arrives as
`cmpwi`/`bne` where retail materialises a boolean with `cntlzw`/`srwi.`. It reads as an allocator problem
because every later register and offset shifts with the branch.

**How it looks.** One extra `b` (or one fewer) at the end of a conditional block, or, after a `bl memcmp`,
`cmpwi r3,0 / bne` in ours against `cntlzw r0,r3 / srwi. r0,r0,5 / beq` in retail (`cntlzw` counts leading
zeros: `cntlzw(x) >> 5` is `x == 0` as a 0/1 value; the `.` sets the condition register for the branch).

**Why it happens / what was measured.** These are *condition-polarity* choices. Reproduced on the real
compiler (2026-09-29, `-O4,p -inline auto -func_align 4`, Wii/1.3):

* **Exit shape - holds.** One `if (ok == 0) { ... }` block plus a trailing `return ok` gives retail's single
  `bne` exit; `if (ok != 0) return ok; ...` adds a `b` (`beq` + `b`), 4 bytes longer. It holds for both the
  ternary and the `if/else` producer of `ok`.
* **Assigned boolean - holds.** `s32 same = (memcmp(...) == 0); if (same)` is `cntlzw`/`srwi.`; both
  `if (memcmp(...) == 0)` and `if (!memcmp(...))` give `cmpwi`/`bne` (the two spellings compile alike). Used
  across another call the boolean becomes a branchless `neg/or/srawi/and` select.
* **Ternary vs `if/else` - NOT a lever of its own.** `ok = c ? f(x) : 0` and the `if (c) ok = f(x); else ok
  = 0;` spelling compiled **identically** (same bytes) under every flag set tried (`-O4,p`, `-O3`, `-O2`, `-O1`, `-opt nopeephole`, Wii/1.0 too), both in the scratch demo
  and on the project unit (`DWCi/DWCi_NatNeg`'s `DWCi_natNegTickIdleSockets`, rewritten as `if/else` and
  rebuilt: still 100.00 %). The original lane's "ternary alone 87.89 -> 92.59" is therefore not explained by
  the ternary; a neighbouring change in that same edit (the exit shape above, or a type/declaration change)
  is a hypothesis - the lane's diff would need re-reading to say which. Treat the ternary as harmless, not as a fix.
* **Argument arm order** (`f(13, value, 0)`, not `f(13, 0, value)`) is plain source fidelity, visible as
  register shuffles at the call; it is not compiler behaviour to demonstrate.

**How to work it.** At a branch-shaped residual, try in this order: the exit shape (single block +
trailing return vs early return), then the boolean materialisation, then argument order. Do not spend a
compile on ternary-vs-`if/else`.

**When NOT to apply.** The `cntlzw` form needs the boolean to be a *named local*; if the target only ever
tests the `memcmp` result once, `cmpwi`/`bne` is right and the assigned boolean would be a regression.

**Result.** Measured at the time (2026-09, DWCi band): `DWCi_natNegTickIdleSockets` 87.89 -> **92.59**, then
**96.55** with the `if (ok == 0)` + trailing-return spelling; `DWCi_npSetValueEx` 98.57 -> **100.00** from the
argument order alone.

**Example.**

```c
if (ok == 0) { ... }            /* one trailing `return ok` -> a single bne exit */
return ok;                      /* `if (ok != 0) return ok;` would give beq + b */

s32 same = (memcmp(a, b, n) == 0);   /* cntlzw / srwi. */
if (same) { ... }                    /* `if (memcmp(...) == 0)` gives cmpwi/bne */
```

**Demonstration.** `071-condition-polarity.cpp` (`ideas.py demo-check 71`): the assigned boolean
(`cntlzw` + `srwi.`) vs the direct and `!` forms (`cmpwi`); ternary and `if/else` the same size (0x74); the
single-exit form 0x74 vs the early-return form 0x78. Idea 34 (a switch tail's constant returns) is the same
family of "return shape" levers.
