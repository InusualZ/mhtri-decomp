---
id: 199
title: A local with a constructor is initialised where it is declared, so declare it after the code retail runs first
status: works
problem: Retail runs a local's constructor stores (a zeroed nw4r::ut::Rect, four stfs) after a call that precedes its first use; ours runs them at the top of the block, before that call
tags: [source-shape]
applies: [GC/3.0a5.2]
demo: 
related: [138]
---

# 199. A local with a constructor is initialised where it is declared, so declare it after the code retail runs first

**Problem.** A record whose member has a constructor (`nw4r::ut::Rect` zeroes its four floats) shows its
`stfs` run in a different place: retail after a call (`bl hbmToFullWidth`), ours at the head of the loop body,
so the diff reads as four inserts and four deletes around an unchanged call.

**Why it happens.** MWCC emits the constructor at the declaration, and the scheduler does not move stores across
a call. A declaration at the top of a block runs the constructor before every statement of that block.

**How to work it.** Move the declaration down to just before the first use (C++ allows a declaration after
statements), or open a block there. The diff tells where: the first instruction retail runs after the
constructor's stores.

**Result.** `homebutton/hbm_text_panel` `hbmPanelDrawComposition` 97.75 -> 99.45 (the look-ahead loop's
`HbmCharBox box` declared after `if (self->inputLatched) c = hbmToFullWidth(c);`).

**Example.**

```
while ((c = *lookahead) != 0) {
    if (self->inputLatched) {
        c = hbmToFullWidth(c);
    }
    HbmCharBox box;          /* its Rect is zeroed here, after the call */

    box.code = c;
    ...
}
```
