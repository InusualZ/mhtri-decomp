---
id: 148
title: A dense switch lowered to a linear compare chain where retail has a compare tree
status: works
problem: A switch of about six dense cases compiles in ours to a linear `cmpwi`/`beq` chain, while retail tests it with a binary compare tree (pivot compares with `bge`/`blt`).
tags: [source-shape, pragma]
applies: [Wii/1.3]
demo: 
related: [108, 109, 120]
---

# 148. A dense switch lowered to a linear compare chain where retail has a compare tree

**Problem.** A `switch` with a few dense cases is a linear chain of compares in ours; retail splits the case
range with pivot compares (`cmpwi 1; beq; bge; cmpwi 0; bge; b default; cmpwi 3; bge default`). Every branch after
the first compare differs.

**Why it happens.** MWCC's front-end optimizer turns a small switch into an `if` chain ("optimizing switch into
ifs") as part of its control-flow optimisation; retail's build did not, for these functions. The pass is gated by
the internal pragma `opt_optimizecontrolflow`.

**How to work it.** Bracket the function with `#pragma push` / `#pragma opt_optimizecontrolflow off` / `#pragma
pop` (idea 120). It keeps the backend's compare tree and, unlike `#pragma global_optimizer off`, leaves the rest of
the optimizer on: no frame pointer (`mr r31,r1`), no extra saves. File-wide it can cost other rows (measured -170 B
over 9 rows in `Network/network_pat_control`), so scope it. Probe a demo first: a scratch file with the switch and
the unit's flags shows the tree under the pragma.

**Result.** `Network/network_pat_control` (Wii/1.3, -O4 by pragma, nopeephole): `updateNetworkPatControl` 89.75
-> 90.38 % (+71 B) with the pragma alone (every nested `switch` a compare tree, as retail); `global_optimizer off`
scored 85.78 (frame pointer). The opt_* pragmas `opt_rebuildconditionals`, `opt_treetransformation`,
`opt_prelinearize`, `opt_repositioncode`, `opt_rebuildlogicals`, `opt_generateconditionalassignments`,
`opt_arithtransformation`, `opt_foldconstants`, `switch_tables`, `opt_dead_code` and `opt_pointer_analysis` off
keep the `beq` chain (demo).

**Example.**

```c
#pragma push
#pragma opt_optimizecontrolflow off
void updateNetworkPatControl(void) { switch (work->step) { case 0: ... case 1: ... case 2: ... } }
#pragma pop
```
