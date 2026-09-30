---
id: 71
title: The condition's POLARITY decides the exit - a ternary merges arms where `if/else` does not
status: works
problem: A function whose logic and instruction set are right still misses by a register shuffle and an exit: the two arms do not merge, an early return arrives as a `beq`+`b` pair where retail has one `bne`, or a `memcmp` test arrives as `cmpwi`/`bne` where retail materialises a boolean with `cntlzw`/`srwi.`. It reads as an allocator problem because every later register and offset shifts with the branch.
tags: [source-shape]
applies: [DWCi]
demo: 071-condition-polarity.cpp
---

# 71. The condition's POLARITY decides the exit - a ternary merges arms where `if/else` does not

**Problem.** A function whose logic and instruction set are right still misses by a register shuffle and an
exit: the two arms do not merge, an early return arrives as a `beq`+`b` pair where retail has one `bne`, or a
`memcmp` test arrives as `cmpwi`/`bne` where retail materialises a boolean with `cntlzw`/`srwi.`. It reads as
an allocator problem because every later register and offset shifts with the branch.

**Why try it.** All four spellings are *condition-polarity* choices, each measured on the DWCi band
(`.pi/notes/dwci-band-9050.md`, the lane's Header section):

* a **ternary** is what makes MWCC merge two arms: `ok = cond ? f(...) : 0` produced retail's `cmpwi r3,0` /
  `mr r30,r3` / `bne`, where the equivalent `if/else` produced a different merge;
* **one `if (ok == 0) { ... }` block plus a trailing `return ok`** gives retail's single `bne` exit, where
  `if (ok != 0) return ok;` gives a `beq`+`b` pair;
* an **assigned boolean** - `s32 same = (memcmp(...) == 0); if (same)` - is the only spelling that yields the
  target's boolean materialisation (`cntlzw`/`srwi.`); both `if (memcmp(...) == 0)` and `if (!memcmp(...))`
  give `cmpwi`;
* **argument arm order** is visible the same way: the host callback is `f(13, value, 0)`, not
  `f(13, 0, value)`.

**Result.** `DWCi_natNegTickIdleSockets` 87.89 -> **92.59** (the ternary alone), -> **96.55** with the
`if (ok == 0)` + trailing-return spelling; `DWCi_npSetValueEx` 98.57 -> **100.00** from the argument order
alone.

**Example.**

```c
/* three spellings of one test, three different exits */
ok = cond ? send(...) : 0;      /* cmpwi/mr/bne - the merged arm */
if (ok == 0) { ... }            /* ... plus one trailing `return ok` -> a single bne exit */
return ok;                      /* `if (ok != 0) return ok;` would give beq + b */

s32 same = (memcmp(a, b, n) == 0);   /* cntlzw / srwi. */
if (same) { ... }                    /* `if (memcmp(...) == 0)` gives cmpwi/bne */
```

**Demonstration.** `071-condition-polarity.cpp` (`ideas.py demo-check 71`). Reproduced: an assigned boolean gives
`cntlzw` + `srwi.` where `if (memcmp(...) == 0)` (and `if (!memcmp(...))`) give `cmpwi` + `bne`. **Not reproduced in
isolation:** the ternary and the `if/else` spelling of the merged arm compile identically (both 0x18 B), so the ternary
claim depends on the surrounding function.
