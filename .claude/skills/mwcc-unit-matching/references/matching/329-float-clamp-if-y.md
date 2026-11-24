---
id: 329
title: A float clamp 'if (!(y >= lo)) y = lo' materialises the compare: write it as y = (y >= lo) ? y : lo
status: works
problem: A float `>=` test written as `if (!(y >= 0.0f)) y = 0.0f;` compiles to `fcmpo; cror eq,gt,eq; mfcr; extrwi.; bne` (the condition is built as a value and tested), where retail branches on `cror` directly with `bne; b`.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [141, 261]
---

# 329. A float clamp 'if (!(y >= lo)) y = lo' materialises the compare: write it as y = (y >= lo) ? y : lo

**Problem.** The diff shows two extra instructions (`mfcr r0` + `extrwi. r0,r0,1,2`) in front of a `bne` after
`fcmpo cr0,f31,f0; cror eq,gt,eq`, and retail has `bne; b` instead: a 4 B size gap in an otherwise matching function.

**Why it happens.** `>=` on floats is `cror eq,gt,eq` on cr0; MWCC turns the negated condition of an `if` into a
value (`mfcr`/`extrwi.`) before branching. The ternary form `y = (y >= lo) ? y : lo` branches on the cr bit straight away.

**How to work it.** Replace the negated `if` clamp with the ternary and measure the one row; `if (y >= lo) {} else
{ y = lo; }`, `if (y < lo)` and the inverted ternary measured 97.7 / 98.2 / 98.4 % on the same row.

**Result.** `homebutton/hbm_text_panel` (Wii/1.3 -O3 peephole off): `hbmPanelCursorUp` 764 -> 760 B (target 760 B),
97.68 -> 99.26 %.

**Example.**

```
/* before */  if (!(y >= 0.0f)) { y = 0.0f; }
/* after  */  y = (y >= 0.0f) ? y : 0.0f;
```
