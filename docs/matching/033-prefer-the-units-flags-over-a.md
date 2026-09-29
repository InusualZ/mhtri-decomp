---
id: 33
title: Prefer the unit's flags over a per-function flag - a TU was compiled once
status: works
problem: A function that will not match invites a scoped pragma (`optimization_level`, `peephole`, `scheduling`) aimed at that function alone. Three wins this session did exactly that.
tags: [flags, pragma]
applies: []
demo:
---

# 33. Prefer the unit's flags over a per-function flag - a TU was compiled once

**Problem.** A function that will not match invites a scoped pragma (`optimization_level`, `peephole`,
`scheduling`) aimed at that function alone. Three wins this session did exactly that.

**Why it is usually wrong.** A translation unit is compiled **once**, with **one** flag set. If the other
functions in the same unit already match, they are evidence that the unit's flags are right - so a function that
needs *different* flags is far more likely to be a **source** difference (shape, liveness, declaration order), a
**boundary/attribution** error, or a **stale target object**, than evidence of a per-function flag. A pragma that
fixes one function by changing codegen the original compiler never had is matching the bytes for the wrong reason,
and it will fight the rest of the unit (RSO's level-3 pragma costs `RSOUnLink` and `FindExportIndex` their 100 %;
`optimization_level`/`opt_*` pragmas are whole-function, so a mid-function switch is silently ignored anyway).

**Result / how to apply it.** When a set of symbols is *known* to belong to one TU and one function lags:

1. re-derive the **unit-level** flag from the functions that already match, and check the lagging function against
   it - if it needs something else, look at the *source* first;
2. check the three usual non-flag causes: a stale **target object** (re-split), a **boundary** error (the symbol
   belongs to another unit), and a **naming** mismatch (objdiff pairs by name);
3. only if the unit's flags are genuinely ambiguous - different functions demanding different levels, with
   byte-level evidence both ways - is a scoped pragma defensible, and it must then be recorded as a *suspected
   flag hack* in the unit's header, not as a solution.

**Example.** The RSO and `pl_skill` pragmas each closed a function while costing a sibling its 100 %, which is the
signature: one flag set cannot be right for a unit and wrong for one function inside it.