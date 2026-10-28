---
id: 262
title: A step's early path that runs a later case's tail: wrap the rest of the case (and the cases between) in its else, so it falls into that case
status: works
problem: Retail's `case 0` (or `case 1`) sets the state and branches straight to a later case's body (`li r0,2; stb; b <case 2>`) and every other exit branches to one shared `mr r3,rN` return; ours repeats the later case's test inline, or returns a constant (`li r3,0; b epilogue`) at each exit.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [183, 241, 252]
---

# 262. A step's early path that runs a later case's tail: wrap the rest of the case (and the cases between) in its else, so it falls into that case

**Problem.** A step machine's `case 0: if (done == 1) { state = 2; <case 2's tail>; }` - retail has no copy of the
tail there, only `b` to case 2's code at the end of the switch. And where the function returns a `result` local,
retail ends every path with `b` to one `mr r3,r29`, while a `return result;` in each case becomes `li r3,0` (the
value is a known constant there) plus its own branch to the epilogue.

**Why it happens.** MWCC has no tail merge (idea 213), so the shared tail is written once in the original: the early
path *falls* into the later case. An `if (...) {A} else {B}` puts `A` first and ends it with a branch past `B`,
i.e. to whatever follows the `if` - so if the rest of the case and every case laid out before the target live in
`B`, the branch lands on the target case's label. A single `return result;` after the switch keeps `result` a
merge value, so each exit is a plain branch to one `mr`.

**How to work it.** `case 0: if (done == 1) { state = 2; } else { ...rest of case 0...; break; case 1: ...; break; }`
followed by `case 2: <tail>; break;` (the case labels inside the `else` stay legal, idea 241). Turn every
`return result;` into `break` and return once after the switch. When a second tail sits *after* the switch (reached by
`continue` in idea 183's `do { } while (0)`), make it the loop's condition so `break` can still reach the return:
`do { switch (...) {...} break; } while (tail_test() && (result = 1, 0));` - `continue` runs the tail, the
trailing `break` skips it.

**Result.** `lobby/lb_pane_ui` (Wii/1.3, -O3, peephole off): `fn_801F3294` 97.03 -> 100 (680 B, the else alone);
`lb_color_pane_step` 95.78 -> 99.55 (one exit, both done paths falling into case 2, the condition tail), then 100
with `optimization_level 4` scoped to it.

**Example.**

```
do {
    switch (self->phase) {
    case 0:
        if (self->done == 1) {
            self->phase = 2;                /* falls into case 2 */
        } else {
            if (self->done == 2) { self->phase = 3; continue; }
            ...
            break;
    case 1:
            ...
            break;
        }
    case 2:
        if (empty(page) == 1) { result = 2; }
        break;
    case 3:
        continue;
    }
    break;
} while (empty(page) == 1 && (result = 1, 0));
return result;
```
