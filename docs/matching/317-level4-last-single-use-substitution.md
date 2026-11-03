---
id: 317
title: Level 4 forward-substitutes a variable's last single-use definitions - an extra one moves the window
status: works
problem: Under optimization_level 4 one XOR chain comes out reassociated, `(a ^ b) ^ c` where retail keeps `t = b ^ c; a ^ t`, and anchoring that group only moves the reassociation to another group.
tags: [source-shape, pragma]
applies: [Camellia]
demo: 
related: [16, 35, 90, 92]
---

# 317. Level 4 forward-substitutes a variable's last single-use definitions - an extra one moves the window

**Problem.** At level 4 a group like `tr = subr(26) ^ RL1(dw); SubkeyR(23) = subr(22) ^ tr;` comes out as
`(subr(22) ^ subr(26)) ^ RL1(dw)`. Retail keeps `tr` in a register for that group and flattens only the next one.
Rewriting the group just makes an earlier group flatten instead (idea 90).

**Why it happens.** The flattening is already present in the **first PCode dump** (`backend-03`), so it comes from the
frontend optimizer, not a backend pass. That optimizer splits each local into webs. `tr` normally gets one web per
definition (`r34`, `r53`, `r54`, `r55` ...). Its **last two single-use definitions** in the function get no web: they are
forward-substituted into their one use, and that is when the XOR chain is reassociated. The measurements, all from the
dump 03 webs:

* vendor source: `tr`'s groups 23 and 26 are substituted, groups 7-18 get webs;
* group 26 written with `w1` instead of `tr`: groups 18 and 23 are substituted (the window moves back one group);
* group 7 written with `w1`: `w1`'s group 7 is substituted, and `tr`'s groups 23 and 26 are still substituted;
* `tr` used in the final multi-use RL8 statement: nothing is substituted, so groups 23 and 26 both keep webs.

**How to work it.** Get the debugger's dump 03 and find which definitions have no web. To move the window forward by one,
add **one more single-use definition of the same variable after the last group**, where substituting it emits the same
code: `tr = subr(26) ^ subr(28); SubkeyR(27) = tr;` instead of `SubkeyR(27) = subr(26) ^ subr(28);`. That definition takes
one of the two substitutions. The group before it is then the only other one flattened, which is what retail has.

**Result.** `Camellia/camellia` `camellia_setup256`: 99.80576 -> 100 % (6 differing rows -> 0). The vendor (NTT) spelling
of the SubkeyR(23) group came back, and the dead-duplicate anchor from idea 35 was dropped. The unit is 10/10 functions
and flipcheck is READY once its config request lands. The trial flip gave `main.dol: OK`.

**Example.**

```c
#pragma optimization_level 4
    dw = tl & subl(25), tr = subr(23) ^ CAMELLIA_RL1(dw);   /* retail flattens this one */
    CamelliaSubkeyR(26) = tr ^ subr(27);
    CamelliaSubkeyL(27) = subl(26) ^ subl(28);
    tr = subr(26) ^ subr(28);                               /* takes the second substitution */
    CamelliaSubkeyR(27) = tr;
```
