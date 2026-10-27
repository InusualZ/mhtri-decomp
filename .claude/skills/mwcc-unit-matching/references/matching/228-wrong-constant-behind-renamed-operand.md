---
id: 228
title: A 100 % row can still load the wrong constant or string: read the values behind renamed operands
status: works
problem: A row reads 100 % (or high) while it loads a different float, int-to-float bias, assert file name or table string than retail - objdiff pairs `@5494` against `lbl_8079A68C` by position and never compares the bytes behind them.
tags: [measurement, data, relocations]
applies: []
demo: 
related: [29, 58]
---

# 228. A 100 % row can still load the wrong constant or string: read the values behind renamed operands

**Problem.** objdiff scores `lfs f1,@5494@sda21(r2)` against `lfs f1,lbl_8079A68C@sda21(r2)` as equal whatever the
two pool words hold, and an `addi`/`ADDR32` to a string or table the same way. A wrong literal therefore costs
nothing in the score - a `30.0f` where retail has `50.0f`, a typo-corrected string, the TU's file name where retail
cites a header's - and `rawsame.py` agrees because the relocated bits are masked.

**Why it happens.** Our pool symbols are compiler-generated (`@NNNN`) and retail's are the map's names, so every
pooled operand is a *renamed* pair; the score treats a rename as a match.

**How to work it.** Run `python tools/objdiff/relocdiff.py --values <unit>` and classify each line:

* `[bytes equal]` (the function is byte-identical): the value is wrong - fix it at its source.
* a partial row: compare the function's **loaded-constant sequence** (constants in load order) with retail's. Same
  sequence, shifted offsets = a false hit (code moved, a case body placed elsewhere, a reload ours does not CSE);
  a value that is in one sequence and not the other = a real wrong value.
* `[address]` base-pointer hits (`...data.0`, `Table+0x44`): read what the base reaches - a string pool whose
  prefix differs is layout, a record whose bytes differ is a wrong table entry.

Classes found so far, each fixed at its source:

* a wrong literal (`30.0f` for `50.0f`, `32.0` for `3.0f`, `0.0174532924f` for `0.130899697f` - 1 degree for 7.5);
* the int-to-float bias: `0x4330000080000000` is the **signed** conversion (`xoris` + bias), `0x4330000000000000` the
  unsigned one - an `s32` local, not `u32`;
* an assert in an inline member that retail defines in an accessor header (`g3d_resmat_ac.h`): its `Panic` cites the
  header's name, not the `.cpp`'s (retail even keeps two copies of the string, one per assert);
* a retail typo in a data table (`"..._Foucus_IN.brlan"`) that the source had corrected.

**Result.** 2026-10-27 census (`relocdiff --values` over 667 units): 122 hits in 22 units; the constfix lane fixed
`ai/ai_npc` `fn_802CF704` (50.0), `enemy/em022_prog` `em022_act6_rush` (3.0), `g3d/g3d_resmat`
`ResMat::GetResTexPlttInfo` (`g3d_resmat_ac.h`), `hud/cockpit_quest` `cockpit_item_tag_update` (pi/24),
`ef/effect` `eft_rand_unit` (signed bias: 94.5 -> 100 %) and `homebutton/hbm_kb_widget`'s animation table (the
`Foucus` typo); most partial-row hits were shifted pairings.

**Example.**

```
/* retail: lfs f1,lbl_8079A68C (50.0); ours @5494 (30.0) - the row read 100 % */
fn_802D4230(self, 50.0f);
```
