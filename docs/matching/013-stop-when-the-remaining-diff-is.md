---
id: 13
title: Stop when the remaining diff is no longer flag-shaped
status: works
problem: After the flags that clearly apply are found, a near-miss variant often remains: a flag combination that fixes one visible symptom (a frame size, a single instruction) while leaving the code different.
tags: [flags]
applies: []
demo:
---

# 13. Stop when the remaining diff is no longer flag-shaped

**Problem.** After the flags that clearly apply are found, a near-miss variant often remains: a flag
combination that fixes one visible symptom (a frame size, a single instruction) while leaving the code
different.

**Why try it.** A near-miss deserves one look at *what* it changes, because that distinguishes "a flag I
have not found" from "the wrong flag that happens to fix one symptom".

**Result.** Inspect what the near-miss variant does to the instruction stream. If it reorders instructions
the target does not reorder, the flag is wrong and the residual belongs to source shape or liveness - stop
flag hunting there and document the residual instead (see the matching policy in `CLAUDE.md`). Document it
**once, in the unit's file header comment** - not as a comment on the function it concerns: a function
comment is a short description of what the function does, never its symbol name, its match percentage or a
residual (see `CLAUDE.md` -> Conventions).

**Example**

```
-opt nopeephole,level=4 -> target frame -0x1d0, but two independent XORs get hoisted
                           (first-diff@958) -> the target's level is 3, not 4
```
