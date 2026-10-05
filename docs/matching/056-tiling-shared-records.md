---
id: 56
title: Two lanes' views of one work record are merged by tiling, not by choosing a side
status: works
problem: Two lanes register neighbouring bands that both take the same work record, so both write `src/<area>/<rec>.h` and the second landing is an add/add conflict on a file already in main; neither side is a superset, so choosing one breaks the other unit and keeping both defines the record twice.
tags: [process]
applies: []
demo:
reviewed: 2026-09-29
related: [57, 72, 60]
---

# 56. Two lanes' views of one work record are merged by tiling, not by choosing a side

**Problem.** Two lanes register neighbouring bands that both take the same work record, so both write
`src/<area>/<rec>.h` - and the second landing hits an **add/add conflict on a file that already exists in
`main`**. Neither side is a superset (measured on `_AINPC_W`: 90 named members in the landed header vs 85 in the
newcomer, **75 identical in both name and offset**, 14 only in the landed one, 9 only in the newcomer) and one
offset carries two different names (`+0x005` was `sub_step` in the landed header, `field_0x005` in the other).
Choosing a side breaks the other unit; "keep both" is two definitions of one record.

**How it looks.** A merge conflict (or a landing refusal) on a shared header both lanes created; the two versions
name different subsets of the same struct, each correct for its own unit's functions.

**Why it works.** A record is a *layout*: a total map from offsets to fields. If every field keeps the offset it
was measured at and the bytes nobody names stay padding, both views can live in one struct - and the proof is not
the comment columns but the **rows**: every consumer's per-symbol percentage must come back exactly.

**How to work it.** Take the **live** header as the base (the landed lane's, whose consumers are already measured
against it). For each field the other lane names and the base does not: find the base **filler** covering that
offset (a plain `+0xNNN`-prefixed line whose body begins `u8 unused_0x...`), take the field's size **from the other
header's own layout** - the distance to its next member, never from the type (`nw3r::math::VEC3` is 12 bytes, and
two lanes' fields can sit inside one base filler), and split that filler into `[gap] [field] [gap]`. Then assert
the tiling: every filler's declared end equals the next member's offset, no two members share an offset, the last
member's offset and the struct's total are unchanged. For a shared offset the **base's name wins**, and the other
unit's source is renamed to that spelling (one rename, three sites, codegen-neutral). **Tool:**
`tools/units/recordmerge.py` implements exactly these rules:

```sh
python tools/units/recordmerge.py --base src/ai/ainpc.h --other worker/<slug>:src/ai/ainpc.h --out src/ai/ainpc.h
```

(`--dry-run` shows the per-group delta; `--take {other,base}` picks which side's declaration wins a same-offset conflict, default `other`.) It
refuses to write while anything is unresolved - a named member in the way, no room in the covering filler, a
same-offset rename, a member whose size is the struct total - and carries top-level declarations only the other
view has (reported). It is only the edit: **the proof remains the whole-tree build plus `ninja changes`.**

**The trap that costs you every row.** Splicing a field **without shrinking its filler** silently enlarges the
struct while the hand-written `/* +0xNNN */` comments still describe the old layout - the file reads correctly,
the build is clean, and *every* row of *every* consumer drifts a fraction (measured: `ai/fn_802C474C` 100.00 ->
99.94, `fn_802C4B68` 100.00 -> 99.91, its unit's `matched_code` 32.52 % -> 0.00 %). Nothing else catches it:
build, stylelint and the gate's structural checks all pass. So verify a record merge by **re-measuring the
consumers' rows** - `ninja changes` must print *no* line for a unit that already owned the record. Offsets in
comments are not evidence; percentages are.

**Result / Example.** (2026-09-26) `src/ai/ainpc.h`, the `_AINPC_W` record (0x49C bytes): after the merge 99
named members (90 base + 9 spliced), 48 fillers, last field `+0x498`, total unchanged. The newcomer's 12 rows then
reproduced its own report exactly (`fn_802C5D10` 99.0541, `fn_802C6690` 100.0000, unit 20.159) and the landed
band's rows did not move at all - which is what made the landing acceptable with one `_AINPC_W` definition
instead of two.

**When NOT to apply.** When one header really is a superset of the other, just take it. When two lanes disagree on
an offset's *size or type* (not only its name), the side whose code depends on the declaration wins (below).

**Evidence (a second and third view of the same record; moved here from idea 57, where it was first written).**

* **A layout calculator finds size errors.** These records are written as explicit pad arrays (`u8 pad_0xNNN[0xMMM
  - 0xNNN];`), which makes the layout mechanically checkable: a 30-line checker asserts that the offsets tile. It
  caught `field_0x216` declared `s32` where the code does a `lbz` - the field is **1** byte, so every later field
  sat 2 or 8 bytes out and ~20 functions each lost ~20 points. Take a field's size from the **access width in the
  code** (`lbz` = 1, `lhz` = 2, `lwz` = 4, `stb`/`sth` likewise), never from the type you guessed, and assert the
  tiling before you measure.
* **A header file can define more than one struct.** Keying members by offset alone matched `_AINPC_W`'s offsets
  against a smaller struct above it in the same file (`0x0`, `0xC`), so the "next member offset" that sizes a field
  came from the wrong struct - `active` was sized 12 B instead of 1 B, and every splice was refused for lack of
  room. Parse **per struct**: name each group, keep its own member list, compute every size and covering filler
  *within* the group. (A byte-wide filler is also written `u8 unused_0xNNN;` with no bracket; a tiling check that
  reads only the bracket form must not flag it.)
* **Compare the declaration, not just `(offset, name)`.** Fourteen differences: six were fillers the splices
  themselves superseded (expected), and one was a genuine conflict the `(offset, name)` rule skipped - main had
  `u8 field_0x3F8;`, the newcomer `u8 field_0x3F8[4];`, and the newcomer's code indexes it. The merged header
  compiled every *landed* consumer and not the newcomer's own source (`illegal operands 'unsigned char' [
  'unsigned char'`). **The test of a record merge is that both sides' sources compile** - whole-tree `ninja -k 0`
  at 0 FAILED, not just one consumer's object - **plus the rows**. Only the same-name-different-size class needs a
  decision, and the side whose *code* depends on the declaration has the better claim: here the array won, the
  scalar's neighbouring `unused_0x3F9` filler was dropped, and the whole-tree build plus an empty `ninja changes`
  proved both consumers and the newcomer.
* Run against the merge that produced this idea (`089491a7b`'s header and the `802d44f4` view) `recordmerge.py`
  reproduces the 36 splices, the 33 filler splits and the `0x3F8` decision exactly, and keeps one trailing comment
  the hand pass typed away.
