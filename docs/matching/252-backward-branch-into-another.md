---
id: 252
title: A backward branch into another case's code: a for (;;) headed by that code, with the jumping case's label inside the loop
status: works
problem: Retail places a case after other cases and ends it with a `b` back to the start of an earlier case's code (or a step's `state = 0` re-entry lands on step 0's code, not on the dispatch); the fall-through or re-dispatch spelling puts the case in the wrong place and re-tests the state.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [203, 213, 241]
---

# 252. A backward branch into another case's code: a for (;;) headed by that code, with the jumping case's label inside the loop

**Problem.** Retail's outer `switch` lays out case X's code (a shared body), then other cases, then case Y, whose
code ends with a `b` *backwards* to the first instruction of X's body. The fall-through spelling (`case Y: ...;
/* fallthrough */ case X:`) puts Y in front of X; the re-dispatch spelling (idea 203: `state = X; continue;` in a
`for (;;)` around the switch) re-tests the state. Both shift everything after them, and the register allocator
then colours the whole function differently.

**Why it happens.** A backward branch to the first instruction of a block is a loop back edge, and that block is
the loop's head. Idea 213's wall is a target in the *middle* of another case's code; a target at the *start* of
it can be a loop head. A `case` label may sit inside a nested block of the switch body (idea 241), including a
loop body, so the cases laid out between X and the back edge can live inside the loop.

**How to work it.** Make X's code the body of a `for (;;)` placed at X's case labels; move the cases that retail
lays out after X (up to and including Y) inside the loop, keeping their `case` labels at the switch's indentation;
close the loop after Y and follow it with `break;`. Every path of X's body must leave (`return`), Y's end falls to
the loop's back edge, and a `break` inside the loop leaves the loop, then the `break;` after it leaves the switch,
which is what each moved case's `break` did before. The same shape covers a nested step machine whose step N does
`step = 0; continue;`: put `for (;;)` right after `case 0:` so `continue` lands on step 0's code. Keep a one-line
comment above each such loop.

**Result.** `Network/network_pat_control` (Wii/1.3, -O4 by pragma, nopeephole): `updateNetworkPatControl`
92.69 -> 96.77 % (+464 B) with case 0x1F moved after 0x3/0xD inside a loop headed by the 0x19..0x1E body, then
96.77 -> 97.00 % (+26 B) with case 0x7's step 0 heading its step loop.

**Example.**

```
case 0x19: case 0x1A:
    for (;;) {                  /* the shared body heads the loop */
        if (...) { return; }
        update();
        return;
case 0x3:
        ...
        return;
case 0x1F:
        stepSubMachine();       /* falls to the back edge: re-enters the 0x19 body */
    }
    break;
```
