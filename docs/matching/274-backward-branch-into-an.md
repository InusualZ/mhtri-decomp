---
id: 274
title: A backward branch into an if-body: a one-case switch on the tested value, its default inside the loop the body heads
status: works
problem: retail tests `x == K`, falls into a tail that leaves, and later code jumps backwards to the tail's first instruction; with no switch around it there is no case label for idea 252's loop, so the goto-free source repeats the tail
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [213, 241, 252]
---

# 274. A backward branch into an if-body: a one-case switch on the tested value, its default inside the loop the body heads

**Problem.** Retail reads `cmpwi rX,K; bne REST; TAIL...; b exit; REST: ...; b TAIL`: an `if (x == K)` whose body
leaves, and a later path (after more statements) that branches back to the body's first instruction. Idea 252 makes
such a target a loop head, but it needs a `case` label to enter the loop's middle at `REST`; an `if` has none, so the
goto-free source repeats the tail at the later site and every instruction after it shifts.

**Why it happens.** MWCC compiles a `switch` with one `case` and a `default` to exactly `cmpwi rX,K; bne default`,
with the case body falling in after the compare (measured: no `beq`/`b` pair, no jump table). A `default:` label may
sit inside a nested loop of the switch body, so the `if` becomes a dispatch whose fall-through is the loop head and
whose other arm lands in the loop's middle; a `continue` from the later site is the backward branch.

**How to work it.** Replace `if (x == K) { TAIL; break; } REST;` with

```
switch (x) {
case K:
    for (;;) {
        TAIL;
        break;
    default:
        REST;              /* the late site: `continue;` */
        break;
    }
    break;
}
```

and keep a one-line comment above the switch. `TAIL`'s `break` leaves the loop and the `break` after it the switch,
so the code after the original `if` must be the enclosing statement's exit (here the end of the outer case). It only
works for an equality test against a constant; an `&&` chain as the switch value is materialised into a register
(measured: `li r3,1` / `cmpwi r3,0` / `li r4,1` / `cmpwi r4,1` around the chain), so a tail entered from the false
arms of a multi-test condition stays idea 213's wall.

**Result.** `quest/quest_entry` (Wii/1.3, -O3 + file-wide `optimization_level 4`, nopeephole): `quest_main_step`
90.69 -> 93.80 % (3028 -> 2940 B of 2832): the clear tail after `goal == 1` heads the loop and the time-out's
window-clear path `continue`s to it (the same register also colours `left` as retail does). Its fail tail, entered
from the false exits of a four-test `&&` chain, stays a wall.

**Example.**

```
switch (goal) {
case 1:
    for (;;) {
        clear_tail();
        break;
    default:
        tick();
        if (timed_out() && window_ok()) {
            mark_clear();
            continue;          /* retail: b back to the clear tail */
        }
        rest();
        break;
    }
    break;
}
```
