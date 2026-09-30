---
id: 72
title: The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity
status: works
problem: Residuals no statement order and no flag moves: MWCC peels a word off a struct copy because `sizeof` is not the target's size; a loop test reads `cmplw` where retail has `cmpw`; a call site's register shuffle is wrong although the callee matches.
tags: [source-shape]
applies: [DWCi]
demo: 072-declared-shape-codegen.cpp
reviewed: 2026-09-29
related: [3, 57, 66, 65, 60]
---

# 72. The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity

**Problem.** Three residuals no statement order and no flag moves: MWCC peels a word off a copy because a
reconstructed struct's `sizeof` is not the size the target's copy used; a loop test reads `cmplw` (unsigned
compare) where retail has `cmpw` (signed); a call site's register shuffle is wrong although the callee's
body matches.

**How it looks.** (1) A struct copy loop (`lwzu`/`stwu` moving 8 bytes per turn, `bdnz`) is followed by an
extra `lwz`/`stw` pair in ours only; (2) the compare after a `bl` differs only `cmplw`/`cmpw`; (3) the
`mr r7,r4` / `mr r8,r5` argument shuffle before a `bl` has a different length.

**Why it happens.** Each is one declaration away from the target's codegen (measured on the DWCi band):

* **A struct's exact size is codegen.** The session copy needs `sizeof == 0xD8`; with any other size MWCC
  peels a word from the copy loop. Rule 3 asks for the size *annotation* - this is why the exact number matters
  and why an approximation marked as one still costs rows.
* **An `s32` loop index selects `cmpw`, not `cmplw`**, for `i < listCount(...)`.
* **Arity is visible at the call site.** `DWCi_initRuntime` takes **six** arguments, and only a six-argument
  declaration reproduces retail's `mr r7,r4` / `mr r8,r5` shuffle. Ideas 57 and 66 are the neighbouring cases
  (a parameter's or a return's *width*); this one names the callee's *count*.

**How to work it.** Before touching statements, check `sizeof` of every copied struct against the target's
copy loop (words moved = size / 4; an odd word count peels one), the signedness of loop indices against the
compare instruction, and the callee's parameter count against how many argument registers the call sets up.
Apply the change to the shared declaration and re-measure its other users (idea 60).

**When NOT to apply.** The peel is 4-byte granular and only for a *copy*; a struct that is only read field by
field has no loop to peel. The signedness lever needs the compare to be against the index itself; a
`u32`-returning bound can force `cmplw` regardless. Arity cannot be demonstrated on its own object (a wrong
count does not compile against a real prototype) - it is evidence from the target's call sequence.

**Result.** Measured at the time (2026-09): the `sizeof` fix took the session copy to **98.48**; the `s32`
index took `DWCi_NatNegEndSession` 83.42 -> **84.70**; the arity took `DWCi_npSetup` 96.29 -> **99.68**. All
three were one-line changes, none reachable from the diff alone.

**Example.**

```c
struct SessionCopy { /* size: 0xD8 */ ... };    /* not 0xD4: MWCC peels a word off the copy */

for (s32 i = 0; i < listCount(w); i++)          /* cmpw; a u32 i gives cmplw */

void DWCi_initRuntime(a, b, c, d, e, f);        /* SIX parameters - the call site proves the count */
```

**Demonstration.** `072-declared-shape-codegen.cpp` (`ideas.py demo-check 72`): a 54-word struct copies with
the two-words-per-turn loop and nothing else; a 53-word struct gets one extra `lwz` after the loop; an `s32`
index compares with `cmpw` and a `u32` index with `cmplw`. Arity is not demonstrated (see above).
(The earlier text cited "row 35's `s32` locals"; idea 35 is about dead copy chains and says nothing of that,
so the cross-reference was dropped.)
