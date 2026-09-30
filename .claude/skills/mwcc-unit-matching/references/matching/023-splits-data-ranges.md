---
id: 23
title: Data ranges in `splits.txt`: what objdiff can and cannot fix
status: works
problem: A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range is not claimed in `splits.txt` (`@1841_80629B90` vs `lbl_80629B90`), so it looks like a one-line fix.
tags: [sections, data, measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [26, 29, 53, 54, 55, 58, 70, 78]
---

# 23. Data ranges in `splits.txt`: what objdiff can and cannot fix

**Problem.** A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range is
not claimed in `splits.txt` (`@1841_80629B90` in the target vs our `lbl_80629B90`), so it looks like a one-line
fix.

**How it looks.** Every instruction of a function is right, and the rows still differ on a `lis`/`addi`/`lwz`
whose operand is a data symbol: the target names it `@1841_80629B90` (a name dtk synthesised because the range
belongs to the unit) and ours names it `lbl_80629B90` (an undefined reference to the map's name).

**Why it can backfire.** objdiff matches a *defined* data symbol by (section, offset) but an *undefined* one by
name. Claiming a range therefore fixes rows only when both sides end up defined at the same offset - and defining
a symbol changes what dtk emits. Claiming the RSO unit's string pool made dtk drop the target's `R_PPC_NONE` pool
relocations (they carry the pool-relative addends), so objdiff could no longer pair the pool-relative
instructions at all: 99.36 % -> 98.01 %.

**How to work it.**

1. Measure the unit before the claim, then edit `splits.txt`.
2. **Force a re-split** - `rm build/RMHE08/config.json` - because a claim edit that never re-runs the split links
   the old object and reports a false green (`ninja`'s "no work to do" is not proof the claim was applied; that
   cost two bisect rounds).
3. Measure after; check that the linked DOL hash did not change (`ninja build/RMHE08/ok`).
4. Claim data the object *emits*; re-split when it only *references* something. (Idea 70 is the rule that says
   which unowned data a unit must claim; idea 78 says to claim the whole run.)

**Result** (measured at the time). Claiming the compiler-generated jump table (`.data 0x80629C08..0x80629C40`) was
worth +0.077 % on `RSOStaticLocateObject`; claiming the string pool at `0x80629B90` cost 1.35 % on `fn_804DABF0`
and was reverted. Two more jump tables confirmed it: `.data 0x805C5FA0..0x805C5FC4` took `Pl/pl_master`'s
`fn_8026CC7C` from 99.9946 to **100 %** and `.data 0x8060E8A0..0x8060E8E4` did the same for
`Gecko_ExceptionPPC.cp`'s `ExPPC_NextAction` - both were short only by the *relocation's* symbol, the emitted
table was byte-equal all along. The counter-example: the same object's `.bss fragmentinfo` (0x806F4B48,
referenced 6 times) must **not** be claimed - the target's `.bss` is 0x180 B and our object emits none, so the
claim pairs a section against nothing and the unit's `matched_data` collapses. There the fix was a plain
re-split: dtk names an unclaimed reloc target from the map, and the target object had been split before the
symbol was renamed.

**Refinement (2026-09-27, `hud/layout.cpp`): a *partial* `.sdata2` claim is not linkable.** The lane claimed
`.data` 0x805D5798..0x805D5B48, `.sdata` 0x807927B0..0x807927C0 and `.sdata2` 0x8079A8C4..0x8079A8E0 beside its
`.text`/extab/extabindex claims. With the `.sdata2` claim, `ninja build/RMHE08/ok` dies inside `mwldeppc.exe`
with the generic `internal linker error: File: 'ELF_gen.c' Line 2802`; dropping only that claim makes it green
(all four combinations measured), and `.data`/`.sdata` claims are harmless. The target's pool run is 28 B and our
object emits its own 16 B - the pool words are *declared*, never defined (idea 29) - so the claim was a promise
about a section our object still contributes to. **Rule:** claim `.sdata2` only when our object emits no pool of
its own (or exactly the run); for a partially-written unit leave the pool to its auto unit. Idea 58 refines this
for a compiler-synthesised pool entry: the claim links only while your unit is the sole referencer of the address.

**Refinement (2026-09-28, arena-task lane) - `.sdata2` is MERGED across objects, so a shared pool label is not
an owner signal; and the asm dump is stale, so resolve by address.**

* **MWLD merges identical `.sdata2` constants across objects.** Of **7245** `.sdata2` labels, **640 are cited by
  more than one registered unit** - e.g. 0x8079C520 (`50.0f`), 0x8079C524 (`60.0f`) and 0x8079C528 (the int->double
  magic `0x4330000080000000`) are each cited by `menu/arena_result` **and** `quest/quest_entry` **and**
  `enemy/em_pop`. So a `.sdata2` **label pair is not evidence of a common TU or of one owner** - the tool's
  `.sdata2` evidence class is invalid for this project, and the `.sdata2` referrer-run seam test needs that
  caveat. `.data`/`.sdata` sharing is *not* affected: **12 of 2193** `.sdata` and **82 of 11208** `.data` labels are
  multi-cited, and all of those are plausible genuine globals. Dolphin's `.map` local-symbol prefixes
  (`_80444a34s_a_hou_back1_80607220`) are **noise, not an owner signal** (idea 54 covers what the map does name).
* **`build/RMHE08/asm/` is stale relative to the map.** A `bl` whose callee has since been renamed still prints
  the *old* label, so `grep` for the new name finds nothing. Resolve a callee by **address**, and use
  `python tools/units/callers.py <address|name>` for "who calls / who reads this" - it is address-keyed, rebuilt
  from the current map and cached (`callers.py --help`).

**When NOT to apply.** A claim does not fix a residual that is register or opcode shaped; and never claim data
another registered unit owns (include its header) or bytes the target object does not carry at all.

**Evidence.** `RSO/runtime`, `Pl/pl_master`, `Gecko_ExceptionPPC.cp`, `hud/layout` and the arena-task lane
(the last one's notes live in the campaign's untracked `.pi/notes/`, so the figures above are the durable
record). All numbers dated 2026-09-2x.
