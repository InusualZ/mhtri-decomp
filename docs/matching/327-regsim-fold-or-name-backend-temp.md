---
id: 327
title: A regsim plan that says fold r(-) into a local means drop the local; declare r(-) names the value
status: works
problem: regsim's EXACT plan carries an edit line (`fold r43(-) into local r35(i)` or `declare r40(@4112) as a local`) that `--apply-local` refuses to write, so a row with an exact colouring plan stays short
tags: [allocator, source-shape]
applies: [Wii/1.3]
demo: 
---

# 327. A regsim plan that says fold r(-) into a local means drop the local; declare r(-) names the value

**Problem.** `mt.py regsim run <unit> <fn>` prints an EXACT plan whose first line is an `edit:`: either `fold r43(-) into
local r34(ch)` or `declare r40(@4112) as a local at declaration position 0`. `--apply-local` and `--apply-order` write
nothing for these (the tool says "nothing to write"), so the plan reads as unreachable and the row stays at 98-99 %.

**Why it happens.** `r(-)` is a backend temporary: the register the compiler gave an expression result (`add r43,r3,r0`,
`lwz r40,r3,0`). A named local that merely holds that value is a *second* virtual register copy-propagated into it, so it
takes a different number and colours differently. `fold` says retail's value has no separate named variable; `declare`
says retail's value was a named variable the source kept.

**How to work it.** For `fold`: delete the local and spell the expression at each use (`self->channels[i].sendLeds()`
instead of `ch = &self->channels[i]; ch->sendLeds()`). For `declare`: add a function-scope local of the value's type, hold
the expression in it before the use (`pane = (Pane*)n->mKey; if (DynamicCast<Bounding>(pane))`), and declare it at the
plan's position (position 0 = first declared). Re-measure; keep the plan's declaration order for the other locals.

**Result.** homebutton/tiHKBManager: hkbManagerSetMap 98.73 -> 100 (`fold`, the `ch` local dropped),
hkbGuiSetBoundingTriggerTargets 99.25 -> 100 (`declare`, a `Pane* pane` local first). A `declare` of a call-result copy
(`mr r41,r3` in lobby/lb_pane_ui `fn_801ED464`, `mr r37,r32` in `lb_screen26_picks_draw`) did not move the row: the
optimiser forwards a local assigned from a call result or a parameter, so only values the plan names with an
expression (`lwz`, `add`) are reachable this way.

**Example.**

```
/* before */  HkbChannel* ch = &self->channels[i];  if (map != ch->mapIndex) { ch->mapIndex = map; ... }  ch->sendLeds();
/* after  */  if (map != self->channels[i].mapIndex) { self->channels[i].mapIndex = map; ... }  self->channels[i].sendLeds();
```
