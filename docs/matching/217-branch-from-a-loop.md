---
id: 217
title: A branch from a loop body to the code after the enclosing switch is not reachable goto-free: MWCC threads no flag or inline return
status: ruled-out
problem: Retail's search loop inside a switch case jumps from its found path straight to the code after the switch (skipping the post-loop code), and every goto-free spelling either adds a test or reorders the loop.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [145, 183, 101, 213]
---

# 217. A branch from a loop body to the code after the enclosing switch is not reachable goto-free: MWCC threads no flag or inline return

**Problem.** Inside `case 1:` a `for` loop searches; its found path ends with `b <after-switch>` while the loop's
normal exit falls into the post-loop code. `return` lands on the epilogue instead (one branch target differs),
and `break` only leaves the loop.

**Why it happens.** Measured with `tools/mwcc-debugger` on `lb_arm_wrestle_step` (Wii/1.3, the unit's real
command line): with a `found` flag the `li r26,1` on the found path and the `cmpi r26,0` after the loop are
present in every one of the 32 backend dumps from `after-unreachable-block-deletion` to `after-code-labels` - no
pass (constant propagation, global optimization, the branch passes) threads a branch whose condition is constant
on an incoming edge. An inlined `static inline` helper returning TRUE/FALSE keeps its return value and tests it
again (idea 101). Idea 213 found no tail-merge or cross-jump pass either. Idea 183's `continue` inside `do { } while (0)` is captured by the inner `for`. So the edge
exists only when the source has it.

**How to work it.** Do not spend variants on it. Measured spellings, best first: `return` (the residual: one
branch target), a flag (+16 B), `if (i >= count)` after a `break` (+12 B), an inlined helper (+16 B), the
post-loop code moved inside a `for (;;)` (same size, top-tested loop, far worse). Record the one-branch residual.

**Result.** `lobby/lb_npc` `lb_arm_wrestle_step`: `return` 99.99 (the residual); a scratch-only `goto` to the
post-switch test 100.0; flag 98.85 (1444 B); `if (i >= count)` 98.04 (1440 B); inline helper 98.67 (1444 B);
`for (;;)` with the exit test first 74.28.

**Example.**

```
case 1:
    for (i = 0; i < (u16)count; i++, plw++) {
        if (...) {
            ...;
            return;          /* retail: b <after-switch test>; residual of one branch target */
        }
    }
    <post-loop code>
    break;
```
