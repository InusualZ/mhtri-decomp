---
id: 203
title: A case that jumps to another case's head: re-dispatch with state = N; continue in a for (;;)
status: works
problem: one switch case ends in a `b` to another case's first instruction (retail shares that case's whole body), and the goto-free copy of the body makes the function longer by the body's size
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [183, 145]
---

# 203. A case that jumps to another case's head: re-dispatch with state = N; continue in a for (;;)

**Problem.** Retail's switch has a case (`case 8`) that, on one condition, branches to the *first instruction of
another case* (`case 5`) - the original was a `goto` or a fall into the other case's label. Duplicating case 5's
body into case 8 (rule 8 forbids the `goto`) emits a second copy: the row is longer than the target by the body's
size and everything after the copy is shifted, so the row reads in the 80s.

**Why it happens.** MWCC does not tail-merge identical blocks, so the duplicate stays. A `for (;;)` around the
switch, dispatching on a local copy of the selector, lets the case set the local and `continue`: the body exists
once. MWCC does *not* thread the `continue` straight into the case (it loops back to the compare chain), so the
row keeps a small residual - but the length matches and the rest of the function realigns.

**How to work it.** Copy the selector into a local (`u8 state = work->state;`), wrap the switch in `for (;;) {
switch (state) { ... } break; }`, and in the case that jumps write `state = 5; continue;`. Every other case keeps
its `break` (which leaves the switch and then the loop). Measure; the remaining diff is the back-edge (`li r3,5 ;
b dispatch`) and the allocator's colouring of the loop-carried locals. Prefer idea 183 when the shared code comes
*after* the switch.

**Result.** `menu/menu_placeinfo` (Wii/1.3): `lb_city_select_draw` 87.03 -> 95.48 % (1164 -> 1044 B, the target's
size; the search-dialog body of state 5 is reached from state 8).

**Example.**

```
u8 state = work->state_0x000;

for (;;) {
    switch (state) {
    case 5:
        draw_search_city_dialog(work, 1);
        break;
    case 8:
        if (work->step_0x001 == 0) {
            ...;
            break;
        }
        state = 5;          /* retail: b case_5 */
        continue;
    }
    break;
}
```
