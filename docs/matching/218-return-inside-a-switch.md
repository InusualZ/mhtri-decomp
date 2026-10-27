---
id: 218
title: A return inside a switch case leaves a dead b-to-epilogue block between the compare tree and the next case body
status: todo
problem: Ours ends a switch's compare tree with `b <next body>` followed by an unreachable `b <epilogue>`; retail falls from the tree straight into the next body, and the goto spelling of the function has the same two extra branches.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [131, 183]
---

# 218. A return inside a switch case leaves a dead b-to-epilogue block between the compare tree and the next case body

**Problem.** A switch whose cases 19..24 `return` (skipping a post-switch call) compiles to a compare tree whose
`bge` goes straight to the epilogue, but the `return`'s own block (`b <epilogue>`) stays in the layout after the
tree, so the tree's fall-through needs a `b` over it: two instructions retail does not have. An early
`if (a || b) return;` in the same function shows the same shape as `bne <next>; b <epilogue>`.

**Why it happens.** Not proven. The block count in the `tools/mwcc-debugger` dumps is constant from
`after-unreachable-block-deletion` (the only unreachable-block pass) to the end, so the dead block was still
reachable there and a later pass redirected the tree's branch past it; the dumps print every branch target as the
same `@L` id (the label list is not decoded), so the pass cannot be named yet.

**How to work it.** Wrapping the early exit as `if (!a && !b) { ... }` removes the early-return pair (-4 B, +0.65).
For the in-switch `return` no spelling has been found: moving the return group first or last in the switch,
adding an explicit `default: break;`, ordering the outer cases differently and idea 203's re-dispatch all measured
the same or worse.

**Result.** `lobby/lb_npc` `lb_player_remote_follow` (Wii/1.3): with idea 183, 98.18 (692 B) with the early
`return`, 98.82 (688 B) with the wrapped body; the scratch `goto` spelling of the original also reads 98.18 at
692 B, so this residual is not the goto's.

**Example.**

```
switch (self->act_no) {
case 1: ... case 45: continue;          /* bge <post> */
case 19: ... case 24: return;           /* bge <epilogue>, and a dead `b <epilogue>` block */
}
break;                                   /* b <walk> over the dead block */
```
