---
id: 185
title: A narrow parameter updated in place is narrowed lazily, a fresh narrow local at once
status: works
problem: Retail adds to an `s16`/`u16` value and then stores it, adds to it or passes it on with no `extsh`/`clrlwi` after the add, while our `s16 top = y - 8;` (or an `s32` local) emits the narrowing at the definition and again before every store.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 185. A narrow parameter updated in place is narrowed lazily, a fresh narrow local at once

**Problem.** Retail computes `y - 8`, `width & 0xFFF0` or `row + strip` into a register and uses it raw - `sth` with
no `extsh` in front, `add` with no `extsh` of the operand - and narrows only where a use needs the narrow value (a
call argument, a chained assignment). Our spelling with a fresh local narrows immediately (`addi; extsh`) or, with an
`s32` local, narrows before each `sth` under `-opt nopeephole`; with `s32` parameters the callers change instead.

**Why it happens.** MWCC keeps a *parameter* (and a narrow local updated by a compound assignment) in its widened
form after `x += c` / `x -= c` / `x &= m` and inserts the narrowing conversion only at a use that needs it; a fresh
declaration `s16 t = x + c;` is narrowed at the assignment. Stores (`sth`) need no narrowing, so the in-place form
stores raw.

**How to work it.** When the target shows an add/mask on a narrow parameter with no narrowing until a call or
chained store, update the parameter itself (`y -= 8; width &= 0xFFF0; if (width < 96) width = 96;`) instead of
introducing a local; for a running narrow total keep an `s16` local and update it with `+=`. Keep the declared
parameter types (`s16`/`u16`) so the callers do not change.

**Result.** `hud/layout`: `draw_prompt_frame` 92.65 -> 96.71 % (then 99.29 % with an `if/else` for the two constant
assignments), `window_body_draw` 94.43 -> 96.19 % (`x += 12; y += 8; width -= 24; height -= 16;`) -> 99.34 % with the
row counter an `s16` updated by `+=`, `window_frame_tex_draw` 94.01 -> 96.90 % (`width &= 0xFFF0`), every includer
of the unit's header byte-identical (`objsame`).

**Example.**

```
void draw_prompt_frame(s16 x, s16 y, u16 width, u8 mode, u8 kind)
{
    y -= 8;                  /* subi r25,r4,8 - no extsh; sth r25 raw later      */
    width &= 0xFFF0;         /* rlwinm r26,r5,0,16,27 - no clrlwi                 */
    if (width < 96) {
        width = 96;
    }
    ...
}
/* not: s32 top = y - 8;  (extsh r4,r4 first, then extsh before every sth)        */
```
