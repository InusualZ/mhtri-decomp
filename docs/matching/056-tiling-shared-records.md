---
id: 56
title: Two lanes' views of one work record are merged by tiling, not by choosing a side
status: works
problem: Two lanes register neighbouring bands that both take the same work record, so both write `include/<area>/<rec>.h` - and the second landing hits an **add/add conflict on a file that already exists in `main`**. Neither side is a superset (measured on `_AINPC_W`: 90 named members in the landed header vs 85 in the newcomer, **75 identical in both name and offset**, 14 only in the landed one, 9 only in the newcomer) and one offset carries two different names (`+0x005` was `sub_step` in the landed header, `field_0x005` in the other). Choosing a side breaks the other unit; "keep both" is two definitions of one record.
tags: [process]
applies: []
demo:
---

# 56. Two lanes' views of one work record are merged by tiling, not by choosing a side

**Problem.** Two lanes register neighbouring bands that both take the same work record, so both write
`include/<area>/<rec>.h` - and the second landing hits an **add/add conflict on a file that already exists in
`main`**. Neither side is a superset (measured on `_AINPC_W`: 90 named members in the landed header vs 85 in the
newcomer, **75 identical in both name and offset**, 14 only in the landed one, 9 only in the newcomer) and one
offset carries two different names (`+0x005` was `sub_step` in the landed header, `field_0x005` in the other).
Choosing a side breaks the other unit; "keep both" is two definitions of one record.

**Why try it.** A record is a *layout*: a total map from offsets to fields. If every field keeps the offset it
was measured at and the bytes nobody names stay padding, both views can live in one struct - and the proof is
not the comment columns but the **rows**: every consumer's per-symbol percentage must come back exactly.

**Result.** Take the **live** header as the base (the landed lane's, whose consumers are already measured against
it). For each field the other lane names and the base does not: find the base **filler** covering that offset (a
plain `+0xNNN`-prefixed line whose body begins `u8 unused_0x...`), take the field's size **from the other
header's own layout** - the distance to its next member, never from the type (`nw3r::math::VEC3` is 12 bytes, and
two lanes' fields can sit inside one base filler), and split that filler into `[gap] [field] [gap]`. Then assert
the tiling: every filler's declared end equals the next member's offset, no two members share an offset, the last
member's offset and the struct's total are unchanged. For a shared offset the **base's name wins**, and the other
unit's source is renamed to that spelling (one rename, three sites, codegen-neutral).

**The trap that costs you every row.** Splicing a field **without shrinking its filler** silently enlarges the
struct while the hand-written `/* +0xNNN */` comments still describe the old layout - the file reads correctly,
the build is clean, and *every* row of *every* consumer drifts a fraction (measured: `ai/fn_802C474C` 100.00 ->
99.94, `fn_802C4B68` 100.00 -> 99.91, its unit's `matched_code` 32.52 % -> 0.00 %). Nothing else catches it:
build, stylelint and the gate's structural checks all pass. So verify a record merge by **re-measuring the
consumers' rows** - `ninja changes` must print *no* line for a unit that already owned the record. Offsets in
comments are not evidence; percentages are.

**Example.** (2026-09-26) `include/ai/ainpc.h`, the `_AINPC_W` record (0x49C bytes): after the merge 99 named
members (90 base + 9 spliced), 48 fillers, last field `+0x498`, total unchanged. The newcomer's 12 rows then
reproduced its own report exactly (`fn_802C5D10` 99.0541, `fn_802C6690` 100.0000, unit 20.159) and the landed
band's rows did not move at all - which is what made the landing acceptable with one `_AINPC_W` definition
instead of two.
