---
id: 181
title: A retail frsp before a float use is a store-to-load forward: read the value back from the field or the temporary
status: works
problem: retail rounds an f32 value it already holds (`frsp f0,f1; fneg f0,f0`, or `frsp` before a pane-translation store) and no cast or f64 spelling reproduces it
tags: [source-shape]
applies: []
demo: 
---

# 181. A retail frsp before a float use is a store-to-load forward: read the value back from the field or the temporary

**Problem.** Retail carries an `frsp` on a value that is already single precision - a parameter negated right after it
is stored (`stfs f1,X(r3); ... frsp f0,f1; fneg f0,f0`), or a loop accumulator copied into a struct member - and no
cast, `f64` parameter or helper on the *value* produces it.

**Why it happens.** MWCC forwards a store to a following load of the same address and keeps the value in its register,
but it models the `stfs` rounding with an `frsp` on the forwarded copy. The source read the value back from the
memory it had just written (the field, or a temporary struct member), not from the variable.

**How to work it.** Spell the use as a read of what was stored: `out = -self->position;` after
`self->position = value;`, or build the temporary the original built (a 2-float point constructed from the
variable, then a 3-float translation built from the point's members) so the read goes through it.

**Result.** homebutton/hbm_kb_cursor: `fn_80568004` 89.50 % -> 100 % (`-value` -> `-self->...float_0x100`); the
memo body loop (`hbmKbCursorDrawBody`, 0x134) matched at 100 % first try once the pane translation went through a
point temporary and an inline placement helper, and the same helper matched the footer and the scroll reset.

**Example.**

```
self->position = value;
if (self->mode == 2)
    out = -self->position;   /* frsp f0,f1; fneg f0,f0 - `-value` gives a bare fneg */
```
