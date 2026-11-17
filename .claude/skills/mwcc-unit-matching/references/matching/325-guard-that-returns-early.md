---
id: 325
title: A guard that returns early where retail shares one exit: wrap the rest in the positive test
status: works
problem: ours emits an extra `li r3,-1; b epilogue` pair (+8 B) at an early `return`, where retail's guard branches past the whole body to the one shared return
tags: [source-shape]
applies: []
demo: 
---

# 325. A guard that returns early where retail shares one exit: wrap the rest in the positive test

**Problem.** The diff shows `bne L` / `li r3,-1` / `b end` in ours against a single `beq end` in retail, and the
function is 8 bytes longer. Every later branch target is shifted.

**Why it happens.** `if (p == NULL) return result;` before a `switch` makes MWCC give the early exit its own
return block. Retail's source had `if (p != NULL) { switch ... }` and one `return result;` at the end.

**How to work it.** Invert the guard: wrap the rest of the body in `if (p != NULL) { ... }` and keep the single
trailing `return`. This is the goto-free spelling of the shared exit (rule 8). Related spellings that moved other
rows: a boolean flag written as `if (x != 0) v = 1; else v = 0;` where `v = x != 0 ? 1 : 0;` emitted the
`neg/or/srwi` idiom, and a chain of `==` tests against one value written as a `switch` when retail compares signed
(`cmpwi`) instead of unsigned (`cmplwi`).

**Result.** `hud_item_msg_push` 97.77 -> 100 (384 -> 376 B), `eft052_hold_row_set` 96.24 -> 100,
`eft052_place` 89.31 -> 99.71 (a `switch` on the map and area numbers).

**Example.**

```
/* before */ if (hud_msg_work == NULL) return result;  switch (kind) { ... }  return result;
/* after  */ if (hud_msg_work != NULL) { switch (kind) { ... } }              return result;
```
