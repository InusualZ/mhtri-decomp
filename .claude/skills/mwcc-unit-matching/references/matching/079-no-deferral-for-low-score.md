---
id: 79
title: A reconstruction is not deferred for a low match score
status: works
problem: A data run and a compiler-emitted vtable were both left out of a unit because their bytes would score badly (a table about 5 % right) - and both deferrals were wrong, because the score is not the arbiter of what gets reconstructed.
tags: [process]
applies: []
demo:
reviewed: 2026-09-29
related: [52, 68, 70, 76, 78]
---

# 79. A reconstruction is not deferred for a low match score

**Problem.** Twice in one session a measured call was made to leave something out because its bytes would score
badly: a `.data` run a unit's own rows read (`arena_stage_config`, 0x24E0 B, whose three readers were
rename-blocked) and a class's compiler-emitted vtable (`__vt__24NetworkSessionManagerPat` at 0x805FB0F0, 0x1C8 B
with 112 non-zero slots fed by 49 functions while the class declared five overrides, so an emitted table would be
about 5 % right). Both were deferred with careful reasoning and both were wrong.

**How it looks.** In a unit's notes or header: "deliberately not claimed/emitted: would lower the score", a range
kept owned by an `auto_*` band, an `extern` or a hand-modelled table standing in for the real thing. The `.data`
section of the unit reads as a small percentage precisely because the bytes were left out.

**Why it happens.** This project's success condition is source that recompiles to the **identical binary**, and the
original's source emitted these things: a vtable is a *consequence of writing the class*, and a data table is a
*consequence of writing the code that reads it*. `match_percent` is a progress metric over a reconstruction that is
still incomplete - it cannot be the arbiter of whether something gets reconstructed, and using it that way inverts
the campaign: the table stays unemitted, the class stays incomplete, the range stays owned by an `auto_*` band, and
the next lane re-derives the same claim. Deferring also confuses two different defects: emitting bytes in an
**unclaimed** range is an `ours-extra` defect, while claiming the range and emitting the same bytes is normal owned
work.

**How to work it.** Claim the range (exactly the owning symbol's extent - idea 78) and emit whatever the current
reconstruction supports, with the rest recorded as a *residual* in the unit header: the slots/bodies by address, the
function each should point at, and how many of them are right today. When the score is genuinely the reason
something looks unattractive, record the *work list* rather than the deferral. Keep the unit `NonMatching` until
the whole object is byte-identical: **a claim is about ownership, a flip is about proof** (idea 1's per-unit
instrument is what tells the two apart).

**When NOT to apply.** Nothing here licenses claiming what is not yours (another registered unit's data, idea 51/70's
boundary; compiler-synthesised bytes, idea 58) or emitting bytes into a range you do not claim. It also does not
say a reconstruction may be *guessed*: an emitted slot that is wrong is recorded as wrong in the residual, not left
looking right.

**Result.** `arena_stage_config` was claimed after the overrule; the Pat vtable's band was claimed and emitted the
same way - five slots correct, the remaining 107 listed as the next pass's work list, unit left `NonMatching`.
Two rules follow: never leave a reconstructed artefact unowned because its score is small, and record the *work
list* when the score is the objection.

**Evidence.** (Owner overrule, measured in one session; numbers are as of that day.) Related: 52 (a vtable we own
must be compiler-emitted), 68 (where a derived class's vtable is emitted), 70 and 78 (claim the run the unit
touches), 76 (tracing the class before writing it).
