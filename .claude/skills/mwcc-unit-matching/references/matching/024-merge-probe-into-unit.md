---
id: 24
title: Merging a probe into the unit is its own step
status: works
problem: Probes are measured standalone, in their own translation unit, so their numbers are not the unit's numbers - and a probe cannot see the unit's types.
tags: [process, measurement]
applies: []
demo:
---

# 24. Merging a probe into the unit is its own step

**Problem.** Probes are measured standalone, in their own translation unit, so their numbers are not the
unit's numbers - and a probe cannot see the unit's types.

**Why try it.** The unit can only have one definition of a struct, so when two functions want different
field types the merge has to choose, and the choice is codegen-relevant: one field as `u8*` instead of
`u32` changed `add r6,r3,r0` into `add r6,r0,r3` and cost 0.3 % on that function.

**Result.** Give the struct the most specific pointer type and **cast at the individual use sites**; then
re-measure *every* function of the unit after the merge, because the unit's per-symbol table - not the
probe's - is what gets recorded. Merging the seven RSO reconstructions moved one function *up*
(`fn_804DABF0` 99.29 in the probe, 99.36 in-unit) and left the five byte-identical ones at 100 %.
