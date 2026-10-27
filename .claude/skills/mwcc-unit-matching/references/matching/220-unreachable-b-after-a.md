---
id: 220
title: An unreachable b after a nested switch is the inner switch's default: return
status: works
problem: Retail has a second `b <end>` right after the last case of each inner switch (dead code), ours goes straight to the next outer case's dispatch.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [109, 183, 218]
---

# 220. An unreachable b after a nested switch is the inner switch's default: return

**Problem.** An outer `switch` whose cases each hold an inner `switch` (a jump table) followed by `break` compiles,
in retail, to each inner switch's last case `b <end>` plus one more, unreachable `b <end>` before the next outer
case. Ours omits the second `b`, so every outer case shifts by 4 bytes and the row scores in the mid-90s while
every instruction looks identical.

**Why it happens.** The unreachable `b` is the outer case's `break`. MWCC keeps that block when the inner switch's
default path does not reach it: with `default: return;` the inner switch's range check and its default both go to
the epilogue, so no edge enters the `break` block from the inner switch, and the block survives as dead code (the
same dead-`b` shape idea 218 describes for a `return` inside a switch). With no `default`, or `default: break;`, the
default edge runs into the `break` block, and MWCC threads and removes it.

**How to work it.** Give each inner jump-table switch `default: return;` and keep the outer `break`. A compare-tree
inner switch (two or three cases) may have no dead block in retail: add the `default` there only where the target
shows the extra `b`, and measure each one.

**Result.** `ef/eft_res` `eft_area_effects_set` (Wii/1.3, `cflags_main`, 2500 B, 22 outer cases): 95.528 -> 100.
`default: break;`, a `return` in every inner or outer case, an inline helper per outer case and a `do { } while (0)`
around the inner switch all left it at 95.528. Adding the `default` to map 10's compare-tree switch added a `b`
retail does not have (99.84).

**Example.**

```c
switch (map) {
case 1:
    switch (area) {
    case 0: fn_a(); break;
    case 7: fn_b(); break;
    case 12: fn_c(); break;
    default:
        return;     /* keeps the outer break's dead `b <end>` */
    }
    break;
...
}
```
