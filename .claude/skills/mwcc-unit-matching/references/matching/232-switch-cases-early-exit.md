---
id: 232
title: A switch case's early exit shares the default's return - break on the pass test, fall through into default
status: works
problem: a case's `if (a || b || c) return; break;` plus `default: return;` leaves one `b <epilogue>` too many (the default's dead body, or the case's own return block) between the case and the next body
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [160, 218]
---

# 232. A switch case's early exit shares the default's return - break on the pass test, fall through into default

**Problem.** Retail's case tests branch straight to the epilogue (`beq exit` ... `bne join; b exit`) and the
dispatch's default goes to the epilogue too, with exactly one `b exit` block in the layout. Writing the case as
`if (a || b || c) return; break;` and the default as its own `default: return;` gives that case's `b exit` **and**
a second `b exit` block for the default body (or a `bne skip; b exit` pair per `if (x) return;`).

**Why it happens.** Every `return` statement is its own block ending in a branch to the epilogue. Branch threading
sends the dispatch straight to the epilogue, but the default's block stays in the layout. When the case falls
through into `default: return;` the two returns are one block, which is what retail has.

**How to work it.** Invert the case's exit test and `break` when it passes; let the case fall through into
`default:`, whose body is the one `return`:

```
case 1:
    q = lookup();
    if (q != 0 && q->lock != 1 && fade() != 2) {
        break;
    }
    /* falls through */
default:
    return;
case 2:                     /* a later case keeps its `if (x) return; break;` */
```

Placing `default:` before the last case (instead of after it) is the step that gives that last case's
`if (x == 1) return;` retail's `beq exit` fall-through into the post-switch code.

**Result.** `menu/menu_item` `menu_work_update` (3360 B): 99.398 -> 99.732 (the `||` form, idea 160, and a u16
field) -> 99.881 (`default:` before `case 2:`) -> 100.0 (case 1 falls through into `default: return;`).

**Example.**

```
switch (mode()) {
case 1:
    if (ok1() && ok2()) {
        break;
    }
    /* falls through */
default:
    return;
case 2:
    if (blocked()) {
        return;
    }
    break;
}
```
