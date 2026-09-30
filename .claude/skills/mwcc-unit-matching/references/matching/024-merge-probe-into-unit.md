---
id: 24
title: Merging a probe into the unit is its own step
status: works
problem: Probes are measured standalone, in their own translation unit, so their numbers are not the unit's numbers - and a probe cannot see the unit's types.
tags: [process, measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [16, 18, 60, 72]
---

# 24. Merging a probe into the unit is its own step

**Problem.** Probes are measured standalone, in their own translation unit, so their numbers are not the
unit's numbers - and a probe cannot see the unit's types.

**How it looks.** A function scores 100 % in a scratch file and drops (a little or a lot) when its body is pasted
into the unit's `.c`/`.cpp`; or two functions ported one after the other fight over one struct.

**Why it happens.** The unit can only have one definition of a struct, so when two functions want different
field types the merge has to choose, and the choice is codegen-relevant: one field as `u8*` instead of `u32`
changed `add r6,r3,r0` into `add r6,r0,r3` and cost 0.3 % on that function. The unit also has its own
declaration set and pragmas (ideas 32 and 60), which a standalone probe does not.

**How to work it.** Give the struct the most specific pointer type and **cast at the individual use sites**; then
re-measure *every* function of the unit after the merge, because the unit's per-symbol table - not the
probe's - is what gets recorded (`mt.py diff -u <unit> <symbol>`, `symdiff.py -u <unit>`).

**Result** (measured at the time). Merging the seven RSO reconstructions moved one function *up*
(`fn_804DABF0` 99.29 in the probe, 99.36 in-unit) and left the five byte-identical ones at 100 %.

**When NOT to apply.** A probe is still the right tool to *find* a shape (`tools/flags/tryvar.py`,
`shapesearch.py`); this idea is about the step after. If the merged unit is worse than its probes and the cause
is a shared type, that is a real trade-off to record in the unit's header, not a probe bug.

**Evidence.** `RSO/runtime`, dated 2026-09-2x.
