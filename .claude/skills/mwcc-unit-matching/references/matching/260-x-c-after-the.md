---
id: 260
title: x += c after the last use of x: a fresh local keeps the add in place where MWCC's frontend propagates x + c into every later use
status: works
problem: Retail updates a coordinate in place (`addi r22,r22,0xa`) right after its last plain use; ours has no add there and recomputes `x + 10` (or a folded `x + 0x3a`) at each later use or hoists it late, recolouring the loop.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [243, 244]
---

# 260. x += c after the last use of x: a fresh local keeps the add in place where MWCC's frontend propagates x + c into every later use

**Problem.** A local `s32 x` is used, then `x += 10;`, then used again (often inside a loop). Retail has the `addi`
at the `+=`; ours has none there: each later use reads `x + 10` (folded into other offsets, or hoisted out of the
loop after the next call), and the extra live value shifts the loop's colouring.

**Why it happens.** The frontend optimizer (before the first backend dump: `backend-03` of `mwcc_debugger.py`
already shows `addi rN,r44,0xa` at every use and no add at the `+=`) substitutes the right-hand side `x + 10` into
the uses of the redefined variable. A *new* variable assigned once from `x + 10` is not substituted.

**How to work it.** Write `text_x = x + 10;` with a new local and use `text_x` afterwards. The add then sits at the
assignment; retail's single register for both is a colouring question afterwards (run `regsim search`). Writing
loop-invariant derived values (`(s16)text_x - 10`) at their uses inside the loop instead of in pre-loop locals
lets MWCC's hoist place them where retail has them.

**Result.** `menu/menu_placeinfo` (Wii/1.3 -O3, nopeephole): `lb_city_list_draw` 96.79 -> 97.65 (then 98.79 with the
for-init pointer and the regsim order), `lb_city_list_ex_draw` +7.3 B, `lb_city_member_list_draw` +10.0 B,
`lb_community_list_draw` +13.6 B, `lb_community_list_ex_draw` +11.2 B, `draw_search_city_dialog` +12.4 B.
Spelling `x = x + 10`, moving the `+=` into the `for` init, or `int` for `s32` change nothing.

**Example.**

```
name_x = x + 0x14;
put_frame_label(name_x, ...);
text_x = name_x + 10;           /* not: name_x += 10; */
for (i = 0; i < n; i++) {
    put_frame_separator((s16)text_x - 10, y, ...);
    lb_font_print(text_x, ...);
}
```
