---
id: 79
title: A reconstruction is not deferred for a low match score
status: works
problem: Twice in one session a measured call was made to leave something out because its bytes would score badly: a `.data` run a unit's own rows read (`arena_stage_config`, 0x24E0 B, whose three readers were rename-blocked) and a class's compiler-emitted vtable (`__vt__24NetworkSessionManagerPat` at 0x805FB0F0, 0x1C8 B with 112 non-zero slots fed by 49 functions while the class declared five overrides, so an emitted table would be about 5 % right). Both were deferred with careful reasoning and both were wrong.
tags: [measurement, process, data, vtable]
applies: []
demo:
---

# 79. A reconstruction is not deferred for a low match score

**Problem.** Twice in one session a measured call was made to leave something out because its bytes would score
badly: a `.data` run a unit's own rows read (`arena_stage_config`, 0x24E0 B, whose three readers were
rename-blocked) and a class's compiler-emitted vtable (`__vt__24NetworkSessionManagerPat` at 0x805FB0F0, 0x1C8 B
with 112 non-zero slots fed by 49 functions while the class declared five overrides, so an emitted table would be
about 5 % right). Both were deferred with careful reasoning and both were wrong.

**Why.** This project's success condition is source that recompiles to the **identical binary**, and the original's
source emitted these things: a vtable is a *consequence of writing the class*, and a data table is a *consequence
of writing the code that reads it*. `match_percent` is a progress metric over a reconstruction that is still
incomplete - it cannot be the arbiter of whether something gets reconstructed, and using it that way inverts the
campaign: the table stays unemitted, the class stays incomplete, the range stays owned by an `auto_*` band, and the
next lane re-derives the same claim. Deferring also confuses two different defects: emitting bytes in an
**unclaimed** range is an `ours-extra` defect, while claiming the range and emitting the same bytes is normal
owned work.

**Result.** Claim the range (exactly the owning symbol's extent - row 78) and emit whatever the current
reconstruction supports, with the rest recorded as a *residual*: the slots/bodies by address, the function each
should point at, and how many of them are right today. `arena_stage_config` was claimed after the overrule; the Pat
vtable's band was claimed and emitted the same way - five slots correct, the remaining 107 listed as the next
pass's work list, unit left `NonMatching` because a **claim is about ownership and a flip is about proof**. Two
rules follow: never leave a reconstructed artefact unowned because its score is small, and when the score is
genuinely the reason something looks unattractive, record the *work list* rather than the deferral.

