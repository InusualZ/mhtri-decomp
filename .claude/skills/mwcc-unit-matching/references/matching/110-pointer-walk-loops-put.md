---
id: 110
title: Pointer-walk loops put the counter increment before the pointer increment
status: works
problem: a counted loop's two `addi`s swap places (ours bumps the pointer before the counter), the only diff in the row
tags: [source-shape]
applies: []
demo: 
---

# 110. Pointer-walk loops put the counter increment before the pointer increment

**Problem.** A counted loop over an array is one instruction pair off: retail emits `addi rI,rI,1` then
`addi rP,rP,N` at the bottom of the loop, ours emits the pointer bump first. objdiff shows it as an insert plus a
delete, so a near-100 row (99.4-99.7 %) carries nothing else.

**Why it happens.** An indexed loop (`items[i].id`) is strength-reduced by MWCC into a pointer it increments
*before* the counter. A source loop that already walks the pointer in the `for` header
(`for (i = 0; i < n; i++, p++)`) keeps the source's order: counter, then pointer.

**How to work it.** Rewrite the loop to walk a pointer explicitly: `for (i = 0; i < N; i++, items++)` and
`items->id` / `items` in place of `items[i].id` / `&items[i]`. The same holds for a pointer bumped inside the body
(`cell++;` as the last statement): move it into the header after the counter.

**Result.** `menu/menu_result`: `q_result_items_left_ck` 99.71, `q_result_delivery_left_ck` 99.52,
`q_result_sell_total_get` 99.39, `q_result_items_sell` 99.46, `q_result_equip_cells_fill` 99.49 -> all 100;
`multi_box_phase_input` 98.79 -> 98.83 (the `for (...; i++, rec++)` form).

**Example.**

```
for (i = 0; i < 0x30; i++, items++) {      /* not: items[i].id ... */
    if (items->id != 0 && items->value > 0) {
        return 1;
    }
}
```
