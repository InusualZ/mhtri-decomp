---
id: 34
title: A switch tail's constant returns are if-converted, so write the arms negated
status: works
problem: A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as `return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours returns early per case, and the function sits at ~94 % with the same opcodes in a different order.
tags: [source-shape]
applies: [Wii/1.3]
demo: 034-switch-tail-negated-arms.cpp
reviewed: 2026-09-29
related: [37, 61, 71]
---

# 34. A switch tail's constant returns are if-converted, so write the arms negated

**Problem.** A `switch` whose default is `return 1` and whose allowed cases `break` leaves the target as
`return 0` looks like a missing arm: the diff shows the target loading a constant and branching while ours
returns early per case, and the function sits at ~94 % with the same opcodes in a different order.

**How it looks.** Same opcodes, different block order: retail has one shared `li r3,0 ; blr` tail with the case
tests branching to it, while ours has an extra constant-return block (`li r3,0 ; b`) sitting where the `default`
arm compiles, or returns early per case where retail falls through. Function size differs by one or two blocks
(8 bytes per extra `li`+`b`/`blr`).

**Why it happens.** MWCC merges identical constant-return blocks, and if-converts a return arm into a
branchless boolean (`cntlzw`+`srwi`, i.e. "is zero") **only** when the arms reduce to a boolean of the tested
value. What it does with the same source therefore depends on what is in the arms, and the switch's `default`
spelling decides whether the tail exists once or twice.

**How to work it.** Match the switch's *tail*, not the conditions: count the constant-return blocks in the target's
switch region and write the source so ours has the same number.

* **The arms call something (a real function body, not a bare boolean):** `default: return 0;` next to a trailing
  `return 0;` emits **two** return-0 blocks (the default's own plus the fall-through tail). `default: break;` plus
  one trailing `return 0;` makes every non-returning case reach the **same** tail block - what retail has when its
  `li r3,0` occurs once. Measured in the demo: `call_default_ret0` 0x6C B vs `call_default_break` 0x64 B (8 bytes = one extra block).
* **The arms reduce to a boolean of the tested value** (`if (!allowed) return 1; break;` + `return 0`): the
  direction *reverses*. `default: return 0` merges with the tail and the whole case test folds to a branchless
  `cntlzw`/`srwi` (0x2C B), while `default: break` keeps the explicit `li 1` / `li 0` blocks (0x30 B). So if
  retail shows the `cntlzw`/`srwi` form, the `default` must return the same constant as the tail.
* **Default and tail constants differ** (`default: return 1` with a `return 0` tail): those are the two arms
  MWCC if-converts as a pair, and `default: return 1;` is right.
* Write the `case` labels in **body-address order** (the order the bodies appear in the target), not the
  condition's order, and use `s32` locals for equality tests so the compare is `cmpwi`, not `cmplwi` (idea 72).

**When NOT to apply.** Do not negate a condition (`if (!c) return 1;` versus `if (c) return 0;`) expecting a
change: the demo's `arms_negated`/`arms_plain` (one spelling negated, semantically the same function) compile to
identical code. That "negated arms" step of the original idea was **not reproduced** on Wii/1.3, nor with the Pl
unit's flags (`-O3 -inline noauto -opt nopeephole`). The unit the idea came from,
`Pl/pl_act`'s `fn_8027C208`, no longer contains a negated arm (its cases read `if (...) return 1; break;`), so the
original evidence for negation is not checkable in the tree; treat it as unproven and try it only after the
default-spelling lever above.

**Example.**

```c
/* retail tail: every non-returning case reaches ONE `li r3,0 ; blr`, and the cases call helpers */
switch (id) {
case 0: case 4:                    /* body-address order, not condition order */
    if (chk(x)) return 1;
    break;
case 2:
    if (x > 3) return 1;
    break;
default:
    break;                         /* not `return 0;` - that emits a second return-0 block */
}
return 0;
```

**Result.** `Pl/pl_act`'s `fn_8027C208` 93.926 -> **99.967 %**, `.text` exact (measured then; the source has since
been rewritten). The sibling shapes in the same round: cases in **body-address order** took `fn_8027A340` 94.234 ->
98.084 %, and `s32` locals for equality tests took `Pl_bari_ck` 84.415 -> 86.679 %. Unit 97.42033 -> **97.86864 %**,
matched bytes unchanged, no flag change.

**Evidence (refinement of 2026-09-27).** Measured on all five switches of `Network/network_state.cpp`, each
written `default: break;` + one trailing `return 0;`: `handleNetworkState1` 0.23 -> **82.50230 %**,
`handleNetworkState2` 0.34 -> **91.28178 %**, `handleNetworkState2Fmp` 0.34 -> **81.01007 %** (the functions score
with the batch's other levers - the per-unit `-O3`, `#pragma exceptions on` and idea 61's `dont_inline` pair).

**Demonstration.** `034-switch-tail-negated-arms.cpp` (Wii/1.3, base cflags, `ideas.py demo-check 34`): the four
functions above are exactly the measurements quoted (0x2C / 0x30 for the boolean shape, 0x6C / 0x64 for the calling
shape, and equal 0x2C for negated vs plain arms). Verified 2026-09-29 also under `-O3 -inline noauto -opt nopeephole`
(same sizes). The earlier demo covered only the boolean shape, and its result was read as contradicting the
refinement; the two shapes are consistent - the direction depends on whether the arms fold to a boolean.
