---
id: 77
title: `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test, never an extend
status: works
problem: `quest_move_state_ck` measured **94.16666 %** and the lane's residual said "front-end artefact": the target reads `lbz r0,0x22D4(r3)` then `rlwinm r3,r0,0,24,24` where our build emits a bare `lbz`. Twelve spellings (u8/u32 locals, `(u8)`, `& 0xFF`, `> 0`, `!!x`, `x ? 1 : 0`, an 8-bit bitfield, a `u8*` view) were compiled with the unit's own flags and all folded to the same short form, so the search was "exhausted" and the row was written off. It was not a codegen residual: the body was **semantically wrong**, and the 94 % hid it.
tags: [source-shape, measurement]
applies: []
demo:
---

# 77. `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test, never an extend

**Problem.** `quest_move_state_ck` measured **94.16666 %** and the lane's residual said "front-end artefact": the
target reads `lbz r0,0x22D4(r3)` then `rlwinm r3,r0,0,24,24` where our build emits a bare `lbz`. Twelve spellings
(u8/u32 locals, `(u8)`, `& 0xFF`, `> 0`, `!!x`, `x ? 1 : 0`, an 8-bit bitfield, a `u8*` view) were compiled with the
unit's own flags and all folded to the same short form, so the search was "exhausted" and the row was written off.
It was not a codegen residual: the body was **semantically wrong**, and the 94 % hid it.

**Why try it.** `rlwinm rA,rS,SH,MB,ME` *keeps bits MB..ME inclusive* of the value, so `MB=ME=24` is the question
"is bit 24 set?" - mask **0x00000080** - not a widening. The tell is in the encodings, not the score, and the tree
already contained the answer twice at 100 %: `src/Pl/fn_8027D684.cpp:139` writes `(self->field_0x655 & 0x80) == 0`
and compiles with the peephole **on**, and `src/camera/fn_802B5C58.cpp:544` emits the target's entire tail. Two
further consequences of reading the pair correctly: a plain boolean test folds to `lbz` + booleanize (which is why
every `!= 0`-shaped spelling was doomed - none of them *is* a mask), and the `& 0x80` reading is forced by the
sibling `quest_move_state_get` at 100 %, which masks the same byte with `0x7F` because bit 7 is "a state follows"
and the low seven bits are the state itself.

**Result.** `(work->state_0x22D4 & 0x80) != 0` took the row to **100.00000 %** - byte- *and* relocation-identical,
no pragma needed. The fix also forced the row's **name** to change (`quest_move_state_ck` ->
`quest_move_state_valid_ck`, map row plus 11 call sites), because the old name came from the disproven body. The
rule for the next residual of this shape: read `MB`/`ME` first - `MB == ME` is a single-bit test, `MB=0,ME=31` is a
plain copy, and `ME-MB+1 < 8` is a field extraction. "Several spellings fold to the same short form" is evidence
about the *spellings*, never about what the retained instruction means.
