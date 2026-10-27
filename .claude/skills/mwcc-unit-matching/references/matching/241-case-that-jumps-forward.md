---
id: 241
title: A case that jumps forward into the middle of the next case: put the next case's label inside an if (0) block
status: works
problem: Retail's switch case ends with a `b` into the middle of the following case (past that case's own test), so the shared tail exists once; the goto-free source repeats the tail in each case and the function grows by its size.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [183, 213, 219]
---

# 241. A case that jumps forward into the middle of the next case: put the next case's label inside an if (0) block

**Problem.** Retail lays cases 0, 1, 2 out in order, and case 0 finishes with a `b` that lands *after* case 1's
prefix (its "result arrived?" test), case 1 with a `b` past case 2's prefix: a chain of forward jumps into the
middle of the next case. The goto-free spelling repeats the shared tail in every case (MWCC has no tail-merge,
idea 213) and the function is the tails' size longer, with every later instruction shifted.

**Why it happens.** A `case` label may sit anywhere inside the switch body, including inside a nested block.
`if (0) { case 1: <prefix> }` is a block the front end must keep (its label is reachable), and the constant-false
test becomes the one unconditional `b` over it - exactly the jump retail has from the end of case 0 to the code
after case 1's prefix. A `break` inside the block still leaves the switch.

**How to work it.** Write the cases as one fall-through run: case 0's body, then `if (0) { case 1: <case 1's
prefix> }`, then the shared code, then `if (0) { case 2: <prefix> }`, then the next shared code and its `break`.
Keep a one-line comment above each `if (0)` saying which path skips the prefix. It only covers *forward* jumps into
a *later* case; a backward jump into an earlier case is still idea 213's wall.

**Result.** `menu/menu_plsearch` (Wii/1.3, -O4 by pragma, nopeephole): `plsearch_page19_step` 92.86 -> 99.64 %
(2260 -> 2140 B, the target's size; step 40's friend-sync / profile-resend / command-18 chain). The remainder is
register colouring.

**Example.**

```
case 0:
    sub = 1;
    if (need_sync) { sync(); break; }
    /* no sync needed: on to the resend */
    if (0) {
case 1:
        if (result == 0) break;
    }
    sub = 2;
    if (resend_needed) { resend(); break; }
    /* nothing to resend: on to the request */
    if (0) {
case 2:
        if (result == 0) break;
    }
    request();
    break;
```
