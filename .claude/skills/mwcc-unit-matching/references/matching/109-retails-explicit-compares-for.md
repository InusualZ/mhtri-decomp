---
id: 109
title: Retail's explicit compares for empty switch cases survive when the cases return and the default breaks
status: works
problem: Retail's switch keeps `cmpwi x,2 ; beq End ; cmpwi x,5 ; beq End` for two cases whose arm is empty, and ours folds those labels into the default, so the function is two compares (16 bytes) short.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
reviewed: 2026-10-07
related: [37]
---

# 109. Retail's explicit compares for empty switch cases survive when the cases return and the default breaks

**Problem.** A dispatcher with arms for some values and `case 2: case 5: default: return;` (or the same with
`break`) loses the compares for 2 and 5: MWCC folds a label whose arm equals the default's into the default, so the
chain stops at the last real case. Retail has the compares, so ours is shorter and every branch target after
them shifts.

**Why it happens.** The folding only fires when the empty arm and the default are the same block. Giving the two
arms different shapes keeps the labels: the empty cases `return` from the switch while the default `break`s to the
function's epilogue, so they are two blocks and the compares stay.

**How to work it.** Write the empty cases as `case 2: case 5: return;` and the default as `default: break;` (the
reverse, both returning, and a no-default `break` were measured and fold), then re-measure.

**Result.** `ef/eft013_fx` `fn_8010A7D4` (2026-10-07): 93.55 -> 100 %, 232 -> 248 bytes.

**Example.**

```
switch (type) { case 0: case 7: ...; return; case 1: ...; return;
case 2: case 5: return;   /* cmpwi 2 ; beq ; cmpwi 5 ; beq  kept */
default: break; }
```
