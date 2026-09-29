---
id: 72
title: The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity
status: works
problem: Three residuals no statement order and no flag moves: MWCC peels a word off a copy because a reconstructed struct's `sizeof` is not the size the target's copy used; a loop test reads `cmplw` where retail has `cmpw`; a call site's register shuffle is wrong although the callee's body matches.
tags: [flags, source-shape, allocator]
applies: []
demo:
---

# 72. The declared SHAPE is a codegen input: a struct's exact size, an index's signedness, and arity

**Problem.** Three residuals no statement order and no flag moves: MWCC peels a word off a copy because a
reconstructed struct's `sizeof` is not the size the target's copy used; a loop test reads `cmplw` where retail
has `cmpw`; a call site's register shuffle is wrong although the callee's body matches.

**Why try it.** Each is one declaration away from the target's own codegen, all measured on the DWCi band
(`.pi/notes/dwci-band-9050.md`):

* **A struct's exact size is codegen.** The session copy needs `sizeof == 0xD8`; with any other size MWCC
  peels a word from the copy. Rule 3 asks for the size *annotation* - this is why the exact number matters
  and why an approximation marked as one still costs rows.
* **An `s32` loop index selects `cmpw`, not `cmplw`**, for `i < listCount(...)`. (Row 35's `s32` locals for
  equality tests are the same lever at `cmpwi`.)
* **Arity is visible at the call site.** `DWCi_initRuntime` takes **six** arguments, and only a six-argument
  declaration reproduces retail's `mr r7,r4` / `mr r8,r5` shuffle. Rows 57 and 66 are the neighbouring cases;
  this one names the callee's *count* rather than its width.

**Result.** The `sizeof` fix took the session copy to **98.48**; the `s32` index took
`DWCi_NatNegEndSession` 83.42 -> **84.70**; the arity took `DWCi_npSetup` 96.29 -> **99.68**. All three are
one-line source changes, and none of them is reachable from the diff alone.

**Example.**

```c
struct SessionCopy { /* size: 0xD8 */ ... };    /* not 0xD4: MWCC peels a word off the copy */

for (s32 i = 0; i < listCount(w); i++)          /* cmpw; a u32 i gives cmplw */

void DWCi_initRuntime(a, b, c, d, e, f);        /* SIX parameters - the call site proves the count */
```
