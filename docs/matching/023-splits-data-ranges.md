---
id: 23
title: Data ranges in `splits.txt`: what objdiff can and cannot fix
status: works
problem: A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range is not claimed in `splits.txt` (`@1841_80629B90` in the target vs our `lbl_80629B90`), so it looks like a one-line fix.
tags: [sections, data, measurement]
applies: []
demo:
---

# 23. Data ranges in `splits.txt`: what objdiff can and cannot fix

**Problem.** A unit's near-miss rows are often just *symbol names* for data the unit owns but whose range
is not claimed in `splits.txt` (`@1841_80629B90` in the target vs our `lbl_80629B90`), so it looks like a
one-line fix.

**Why try it - and why it can backfire.** objdiff matches a *defined* data symbol by (section, offset) but
an *undefined* one by name. Claiming a range therefore fixes rows only when both sides end up defined at
the same offset - and defining a symbol changes what dtk emits. Claiming the RSO unit's string pool made
dtk drop the target's `R_PPC_NONE` pool relocations (they carry the pool-relative addends), so objdiff
could no longer pair the pool-relative instructions at all: 99.36 % -> 98.01 %.

**Result.** Claiming the compiler-generated jump table (`.data 0x80629C08..0x80629C40`) was worth +0.077 %
on `RSOStaticLocateObject`; claiming the string pool at `0x80629B90` cost 1.35 % on `fn_804DABF0` and was
reverted. Measure before *and* after, and check that the linked DOL hash did not change.

Two more jump tables confirmed it (batch 6), and one counter-example shows where the claim stops: claiming
`.data 0x805C5FA0..0x805C5FC4` took `Pl/pl_master`'s `fn_8026CC7C` from 99.9946 to **100 %** and
`.data 0x8060E8A0..0x8060E8E4` did the same for `Gecko_ExceptionPPC.cp`'s `ExPPC_NextAction` - both were
only ever short by the *relocation's* symbol, and the emitted table was byte-equal all along. The same
object's `.bss fragmentinfo` (0x806F4B48, referenced 6 times) must **not** be claimed: the target's `.bss` is
0x180 B and our object emits none, so the claim pairs a section against nothing and the unit's `matched_data`
collapses. There the fix was a plain re-split - dtk names an unclaimed reloc target from the map, and the
target object had been split before the symbol was renamed. Claim data that the object *emits*; re-split when
it only *references* something.

**Refinement (2026-09-27, `hud/layout.cpp`): a *partial* `.sdata2` claim is not linkable.** The lane claimed
`.data` 0x805D5798..0x805D5B48, `.sdata` 0x807927B0..0x807927C0 and `.sdata2` 0x8079A8C4..0x8079A8E0 beside its
`.text`/extab/extabindex claims. With the `.sdata2` claim, `ninja build/RMHE08/ok` dies inside `mwldeppc.exe`
with the generic `internal linker error: File: 'ELF_gen.c' Line 2802`; dropping only that claim makes it green,
and `.data`/`.sdata` claims are harmless (all four combinations measured). The target's pool run is 28 B and our
object emits its own 16 B - the pool words are *declared*, never defined (playbook 29) - so the claim was a
promise about a section our object still contributes to. **Rule:** claim `.sdata2` only when our object emits no
pool of its own (or exactly the run); for a partially-written unit leave the pool to its auto unit. `.data` and
`.sdata` claims are safe. This is the playbook-23 class (dtk dropping the target pool's `R_PPC_NONE` relocs).
It is also *the* reason to force a re-split when testing a claim - `rm build/RMHE08/config.json` - because a
claim edit that never re-runs the split links the old object and reports a false green (that cost two bisect
rounds here: `ninja`'s "no work to do" was not proof the claim had been applied). Section 58 refines this for a compiler-synthesised pool entry: the claim links only while your unit is the sole referencer of the address.

**Refinement (2026-09-28) - `.sdata2` is MERGED across objects, so a shared pool label is not an owner
signal; and the asm dump is stale, so resolve by address.** Two corrections, both measured on the arena-task
lane (`worker/arena-task-92fd`, `.pi/notes/arena-task-92fd.md`):

* **MWLD merges identical `.sdata2` constants across objects.** Of **7245** `.sdata2` labels, **640 are cited
  by more than one registered unit** - e.g. 0x8079C520 (`50.0f`), 0x8079C524 (`60.0f`) and 0x8079C528 (the
  int->double magic `0x4330000080000000`) are each cited by `menu/arena_result` **and** `quest/quest_entry`
  **and** `enemy/em_pop`, and 0x8079A3B8 (the same magic) by `Pl/fn_80295EF4` and `menu/menu_item`. So a
  `.sdata2` **label pair is not evidence of a common TU or of one owner** - the tool's `.sdata2` evidence
  class is invalid for this project (its only "strong" cuts in the arena band rest on exactly such a pair),
  and the `.sdata2` referrer-run seam test needs the caveat. `.data`/`.sdata` sharing is *not* affected:
  **12 of 2193** `.sdata` and **82 of 11208** `.data` labels are multi-cited, and all of those are plausible
  genuine globals. And Dolphin's `.map` local-symbol prefixes (`_80444a34s_a_hou_back1_80607220`) are
  **noise, not an owner signal**: the prefix is neither the referrer nor the owner
  (`_802a22a4s_menu_item.cpp_805cdfc8` is referenced from `fn_802A5444`/`fn_802A579C`/`fn_802A64B0`).
* **`build/RMHE08/asm/` is stale relative to the map.** A `bl` whose callee has since been renamed still
  prints the *old* label, so `grep` for the new name finds nothing and a hand search over the 89 MB dump
  answers wrongly. Resolve a callee by **address** (asm label -> address -> the current `symbols.txt` row),
  and use **`tools/units/callers.py <address|name>`** for "who calls / who reads this" - it is
  **address-keyed**, rebuilt from the current map, cached in `build/tmp/callers/graph.json`, ~1.3 s a query
  over 245 258 references / 54 256 target addresses, with 117 selftest checks (roadmap 7.34). What it
  replaces is exactly the hand grep this row's referrer test used to assume.

