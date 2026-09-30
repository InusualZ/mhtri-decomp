---
id: 13
title: Stop when the remaining diff is no longer flag-shaped
status: works
problem: After the flags that clearly apply are found, a near-miss variant often remains: a flag combination that fixes one visible symptom (a frame size, a single instruction) while leaving the code different.
tags: [flags, process]
applies: []
demo:
reviewed: 2026-09-29
related: [5, 10, 11, 16, 22]
---

# 13. Stop when the remaining diff is no longer flag-shaped

**Problem.** After the flags that clearly apply are found, a near-miss variant often remains: a flag combination
that fixes one visible symptom (a frame size, a single instruction) while leaving the code different.

**How it looks.** A variant matches the frame or the first few instructions, but a later window of the function
now differs in a way the target never does (instructions hoisted, reordered, or a different register colouring).

**Why it happens.** A near-miss deserves one look at *what* it changes, because that distinguishes "a flag I have
not found" from "the wrong flag that happens to fix one symptom". Flags act globally, so a flag that repairs one
place tends to disturb another.

**How to work it.** Inspect what the near-miss variant does to the instruction stream. If it reorders
instructions the target does not reorder, the flag is wrong and the residual belongs to source shape or liveness;
stop flag hunting there. Document the residual **once, in the unit's file header comment**, not as a comment on
the function it concerns: a function comment is a short description of what the function does, never its symbol
name, its match percentage or a residual (`CLAUDE.md`, Conventions). A scoped pragma (idea 16) is the
next lever only when a flag *is* right for most of the unit.

**When NOT to apply.** If the residual is a pure register-colouring mirror, see idea 22 (stop for a different
reason). If a flag combination has never been tried, that is not "no longer flag-shaped"; finish the sweep
(idea 5, idea 9).

**Result.** A bounded flag hunt and a recorded residual.

**Example**

```
-opt nopeephole,level=4 -> target frame -0x1d0, but two independent XORs get hoisted
                           (first-diff@958) -> the target's level is 3, not 4
```

**Evidence.** Camellia unit, `camellia_setup256`, measured early in the campaign.
