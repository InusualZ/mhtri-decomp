---
id: 275
title: Two backward heads from one case: nest the 252 loops, break re-enters the outer head, and a case lands past a head's prefix with if (0) { case X: } else { prefix }
status: works
problem: retail's switch has two shared tails each sitting after a case's own code and each entered backwards from many later cases (some cases jump to one, some to the other, one case to both); idea 252's single loop re-enters one head only, and a case whose code must land on a head without running the head's prefix has no loop-free spelling
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [213, 241, 252, 274]
---

# 275. Two backward heads from one case: nest the 252 loops, break re-enters the outer head, and a case lands past a head's prefix with if (0) { case X: } else { prefix }

**Problem.** A big item-effect `switch` lays out item 0x2C's effect, then the "use motion" tail T1 (`0x162`/`0x133`
+ a call, `b exit`), then thirty cases that each end in `b T1` (backwards); later it lays out item 0x1A's stamina
gain, then the "drink motion" tail T2 (`0x162`/`0x12E`, `b exit`), and the cases after T2 end in `b T2` *or* `b T1`
(item 0xF and 0x10 go to T1, 0x1C and 0xD0 to T2, 0x91 to both). Two earlier cases (0x177, 0x1B) jump *onto* a tail
while skipping the statement just before it (`ef_inst_spawn(0)` before T1, `pl_stamina_add(0x12C)` before T2). The
goto-free source repeated both tails per case (6092 B vs 5600, 87.93 %), and the `default` went to an after-switch
copy where retail's goes straight to the epilogue.

**Why it happens.** Idea 252 makes a backward target a `for (;;)` head and keeps the later cases inside the loop, so
`continue` is the back edge. Inside a loop nested in another, `break` leaves the inner loop and falls to the **end of
the outer loop's body, which is the outer loop's back edge**; jump chaining retargets the `break`'s branch straight to
the outer head, so one nesting level gives a case two backward targets (`continue` = inner head, `break` = outer head)
plus `return`. A head's prefix is skipped by the same `if (0) { case X: ... }` block idea 241 uses, with the prefix in
its `else`: the `if (0)` lowers to one `b` into the else arm (chained past the nested `if (0)` blocks to the prefix),
the block's end falls to the join, which is the loop head. A loop whose body ends in a `return`/`continue` emits no
back-edge branch (unreachable-block deletion), and a `break;`/`continue;` written right after a loop is the chained
target of every `break` inside it, so a third loop (item 0xD0's effect, re-entered by item 0xD6) nests inside the
second and its exit is spelled by the `continue;` after it.

**How to work it.**

1. Dump retail's block list (leaders, terminators, `BACK` for a target below the block) and group every backward
   `b` by target: each target is a loop head; the heads nest in layout order (T1 before T2 before 0xD0).
2. Write T1's code as the body of a `for (;;)` right after the case that falls into it; every later case ends in
   `continue;` (T1), `return;` (retail `b exit`) or falls into the next loop.
3. At T2, open a second `for (;;)` inside the first; cases after it end in `continue;` (T2) or `break;` (T1).
   Close the loops at the switch's end; no code follows the switch, so `default` is the epilogue.
4. A case that must land on a head without its prefix: `A; if (0) { case X: B; } else { if (0) { case Y: C; } prefix; }`
   then the loop - A chains to `prefix`, B falls to the head, C runs `prefix` first.
5. A case re-entered from only one later case (0xD0 from 0xD6) heads a third loop containing that case, followed by
   `continue;` so the head's own `break` chains to T2.
6. Keep the arithmetic of what is **not** reachable: from the innermost loop a case has exactly two non-return
   targets. A case needing three heads (item 0x91: T1 x3, T2 x2, and 0x1C's else arm, 0xF's start, 0x179's start)
   keeps the sites a third head would cost; the measured alternative (a loop headed at 0x179 around T2's loop, 0x91
   outside it) scored the same bytes.

**Result.** `Pl/pl_act_step` (Wii/1.3, -O3, nopeephole): `pl_act_net_hook_main` 87.93 -> 95.29 % (6092 -> 5824 B of
5600) with the two nested loops, the 0xD0 loop and the two `if (0) {} else {}` prefix skips; -> 95.66 % with the
bonus-item result as one `switch` (`case 0: case 1: case 2:` lowers to `cmplwi r0,2; ble`) and item 0x14B's 241 skip;
-> 95.88 % with `me` declared before `partner`; -> 96.13 % with a `weight` local and a per-function
`#pragma opt_common_subs off` (idea 208: retail re-reads the weight row in the loop body). The remainder is the three
item-0x91 sites above (216 B).

**Example.**

```
case 0x2C:
    effect();
    if (0) {
case 0x177:
        clear_wait();           /* lands on the use motion, skipping the effect SE */
    } else {
        if (0) {
case 0xD:
            heal();
        }
        effect_se();            /* the prefix: 0x168 chains straight here */
    }
    for (;;) {                  /* T1: the use motion */
        use_motion(); return;
case 0xE:
        heal(); continue;       /* b T1 */
case 0x1B:
        stamina(); if (0) { case 0x1A: se(); stamina_big(); }
        for (;;) {              /* T2: the drink motion */
            drink_motion(); return;
case 0x1C:
            if (f) { stamina(); continue; }     /* b T2 */
            return;
case 0xF:
            heal(); break;      /* leaves T2's loop: T1's back edge, chained to T1 */
case 0xD0:
            for (;;) { heal(); break;           /* chains to the `continue` below: b T2 */
case 0xD6:
                if (bonus) continue;            /* b 0xD0 */
                return;
            }
            continue;
        }
    }
}
```
