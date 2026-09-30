---
id: 34
title: A switch tail's constant returns are if-converted, so write the arms negated
status: works
problem: A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as `return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours returns early per case, and the function sits at ~94 % with the same opcodes in a different order.
tags: [source-shape]
applies: []
demo:
---

# 34. A switch tail's constant returns are if-converted, so write the arms negated

**Problem.** A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as
`return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours
returns early per case, and the function sits at ~94 % with the same opcodes in a different order.

**Why try it.** MWCC if-converts two constant return arms (`return 0` / `return 1`) into a branchless boolean,
so a body written `if (c) return 0; break;` compiles to an early `return 0` the target never has. Negating the
test (`if (!c) return 1; break;`) gives the compiler the *same* two arms but in the order it folds into the
branchless form, and the tail matches. The same class of shape - a case body that falls through to a shared
constant - has to be written the way the *tail* reads, not the way the condition reads.

**Result.** `Pl/pl_act`'s `fn_8027C208` 93.926 -> **99.967 %**, `.text` exact. The sibling shapes in the same
round: cases written in **body-address order** (not condition order) took `fn_8027A340` 94.234 -> 98.084 %, and
`s32` locals for equality tests (so the compiler emits `cmpwi`, not `cmplwi`) took `Pl_bari_ck`
84.415 -> 86.679 %. Unit 97.42033 -> **97.86864 %**, matched bytes unchanged, no flag change.

**Example.**

```c
/* target: the tail is `return 0`, the default `return 1`, every case falls through */
switch (id) {
case 0: case 4: case 2:            /* body-address order, not condition order */
    if (!allowed) return 1;        /* negated: gives MWCC its two constant arms */
    break;
default:
    return 1;
}
return 0;
```

**Refinement (2026-09-27): when the default returns the *same* constant as the tail, write `default: break;` - `default: return 0;` emits a second return-0 block.** The rule above is about MWCC if-converting
*two different* constant arms (`return 0` / `return 1`). When the constants are the *same* (the common
`return 1` for the allowed cases, `return 0` for everything else), the spelling decides how many copies of
the tail it emits:

```c
default:
    return 0;        /* a constant-return arm of its own -> its own block */
...
return 0;            /* the fall-through tail every `break` reaches -> a second block */
```

gives **two** return-0 blocks, while `default: break;` plus the one trailing `return 0;` makes every
non-returning case fall into the **same** tail block - which is what retail has.

**When the two forms differ, and how to tell from the target.** Count the constant-return blocks in the
switch's region of the target: retail with a shared tail has exactly **one** `li r3, 0` reaching `blr`,
with the case conditions branching to it, and no second `li r3, 0` at the default site. If our diff shows
an extra constant block sitting where the default arm compiles, our `default: return 0;` created it - change
that one word to `break`. (If the default's constant *differs* from the tail's, the two arms are the
if-conversion pair this row's negation rule already covers, and `default: return 1;` is right.)

**Measured** on all five switches of `Network/network_state.cpp`, each written `default: break;` + one
trailing `return 0;`: `handleNetworkState1` 0.23 -> **82.50230 %**, `handleNetworkState2` 0.34 ->
**91.28178 %**, `handleNetworkState2Fmp` 0.34 -> **81.01007 %** (the function scores with the batch's other
levers - the per-unit `-O3`, `#pragma exceptions on` and section 61's `dont_inline` pair).
