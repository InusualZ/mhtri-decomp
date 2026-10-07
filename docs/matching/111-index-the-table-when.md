---
id: 111
title: Index the table when retail initialises the counter before the pointers
status: works
problem: a loop's set-up is reordered (retail `li rI,0` first, then the table bases) and every induction register is renamed, though the bodies are the same
tags: [source-shape, allocator]
applies: []
demo: 
---

# 111. Index the table when retail initialises the counter before the pointers

**Problem.** A draw loop over one or more constant tables (layout ids, labels) diffs only in its set-up and its
registers: retail emits `li rI,0` before the `li rP,table@sda21` / `addi rP,rP,table@l` of each walked table,
ours materialises the table pointers first, and every register of the loop is renamed (the row sits at
96-99.8 % with nothing but `ARG` and two-line `INSERT`/`DELETE` pairs at the top and bottom of the loop).

**Why it happens.** A pointer the source steps itself (`lsp = tbl; ... lsp++;`) is a plain variable,
initialised where the source writes it. An indexed access (`tbl[i]`) is strength-reduced by MWCC into an
induction pointer that is created *after* the counter and in the order of first use inside the loop, and the
allocator colours those inductions in creation order. This is the mirror image of idea 110: there retail
walked the pointer in the source; here it indexed.

**How to work it.**
1. Read the loop's set-up in the target: counter first, then the table bases -> rewrite each stepped pointer
   as `tbl[i]` and drop the pointer local; table bases first -> keep (or move into the `for` header) the
   stepped pointers. Several pointers that retail initialises in one order: write them in the `for` header in
   that order (`for (i = 0, pick = work->picks, label = ids, lsp = lsps; ...)`).
2. **Then re-try the declaration order** (idea 63) - after every other change, not once: removing a local or
   reshaping a loop renumbers the virtual registers, and an order that measured worse before can match now.
   Locals retail colours low (`active`, `cursor`, `str`, `filling`) go last; the first declared takes r31.

**Result.** `lobby/lb_quest_ui`, measured on the unit's object: `lb_kitchen_special_list_draw` 96.47 -> 100,
`lb_kitchen_list_draw` 98.40 -> 100, `lb_kitchen_confirm_draw` 97.19 -> 100, `lb_kitchen_course_draw`
99.30 -> 100, `lb_kitchen_special_pick_draw` 99.13 -> 100 (header form), `lb_trade_cost_draw` 99.15 -> 100,
`lb_trade_list_draw` 96.65 -> 98.62, `lb_kitchen_pick_draw` 98.05 -> 99.92; `lb_scene_eft_spawn` and
`lb_quest_board_model_spawn` (`work->models[i]` in place of a stepped `MHchar**`) -> 100. In most of these the
index form alone left a register rename that only the re-tried declaration order closed.

**Example.**

```
/* retail: li r29,0 ; li r28,lb_kitchen_row_lsp@sda21 ... */
for (i = 0; i < work->row_count; i++) {
    get_lsp_data(lb_kitchen_row_lsp[i], &pos);   /* not: lsp = lb_kitchen_row_lsp; ... *lsp ... lsp++ */
    ...
}
```
