---
id: 182
title: A flag handed to a bool parameter is a bool local
status: works
problem: ours converts a 0/1 local before every call to a `bool` parameter (`cntlzw`/`srwi` twice, hoisted out of a loop) where retail passes the register straight (`mr r4,rN`)
tags: [source-shape, allocator]
applies: []
demo: 
---

# 182. A flag handed to a bool parameter is a bool local

**Problem.** A draw function computes a few 0/1 flags (`pointer_off = chk_pointer() == 0`, `marked = 1`) and hands
them to a `bool` parameter (`GetMenuFontColor(bool, bool, bool, bool)`). With the flags typed `u32`/`s32`, ours
re-normalises each one at the call (`cntlzw`; `srwi` twice - `x != 0` as a value), and MWCC hoists those
conversions out of the loop into extra registers, so the whole function's register colouring shifts. Retail
passes the flag's register unchanged (`mr r4,r23`).

**Why it happens.** Converting an integer to `bool` is `x != 0`, which MWCC materialises as a value; a `bool`
local holds 0/1 already, so the call needs no conversion. The compare against the flag (`cmplwi rN,1`) is the
same either way, which is why the diff shows only the conversions and the register shift.

**How to work it.** Declare every local that only ever holds 0/1 and is passed to a `bool` parameter as `bool`.
Check the call sites in the retail listing: an argument loaded with `mr` from a register that was set by
`li 0`/`li 1` or by a compare is a `bool` local.

**Result.** `menu/menu_item_sub` (cflags_menu): the unit 94.48 -> 97.79 % in one change; `menu_item_sub_page16_draw`
92.00 -> 99.03, `menu_item_sub_page15_draw` 88.56 -> 96.00, `menu_item_sub_page14_card_draw` 89.85 -> 99.56,
`menu_item_sub_page09_rows_build` 89.62 -> 100, and `.text` 10256 -> 10048 B against retail's 10044 B.

**Example.**

```c
void row_draw(Slot* slot) {
    bool pointer_off;           /* u32 here costs two cntlzw/srwi per call */
    bool marked;

    pointer_off = chk_pointer() == 0;
    marked = slot->row == slot->cursor && pointer_off == 1;
    spr.color = GetMenuFontColor(1, marked, pointer_off, 0);   /* retail: mr r4,r23; mr r5,r25 */
}
```
