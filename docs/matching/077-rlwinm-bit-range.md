---
id: 77
title: `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test, never an extend
status: works
problem: The target reads `lbz r0,..` then `rlwinm r3,r0,0,24,24` where our build emits a bare `lbz`; a dozen spellings fold to the short form, so the row looks like a front-end artefact - but the body was semantically wrong.
tags: [source-shape, measurement]
applies: [Wii/1.3]
demo: 077-rlwinm-bit-range.cpp
reviewed: 2026-09-29
related: [2, 21, 38, 39, 86]
---

# 77. `rlwinm x,x,0,MB,ME` keeps an inclusive BIT RANGE - so `MB=ME` is a single-bit test, never an extend

**Problem.** `quest_move_state_ck` measured **94.16666 %** and the lane's residual said "front-end artefact": the
target reads `lbz r0,0x22D4(r3)` then `rlwinm r3,r0,0,24,24` where our build emits a bare `lbz`. Twelve spellings
(u8/u32 locals, `(u8)`, `& 0xFF`, `> 0`, `!!x`, `x ? 1 : 0`, an 8-bit bitfield, a `u8*` view) were compiled with the
unit's own flags and all folded to the same short form, so the search was "exhausted" and the row was written off.
It was not a codegen residual: the body was **semantically wrong**, and the 94 % hid it.

**How it looks.** In the first-divergence view (idea 2) one extra instruction on the target side, right after a
byte load:

```
target:  lbz   r0,0x22D4(r3)          ours:  lbz   r3,0x22D4(r3)
         rlwinm r3,r0,0,24,24                 (nothing)
```

**Why it happens.** `rlwinm rA,rS,SH,MB,ME` (rotate left word immediate then AND with mask) keeps bits `MB..ME`
*inclusive*, numbered from the most significant bit (bit 0 = 0x80000000, bit 31 = 0x1). With `SH = 0` it is a plain
mask, and `MB = ME = 24` keeps exactly one bit: mask **0x00000080** - "is bit 24 set?", not a widening. Reading the
operands is the whole trick, and every alias hides them: `clrlwi rA,rS,n` is `rlwinm rA,rS,0,n,31` (keep the low
`32-n` bits), `srwi` and `slwi` are rotates with a mask. So a plain boolean test (`!= 0`) folds to `lbz` plus a
booleanize and *cannot* produce this `rlwinm`, which is why every `!= 0`-shaped spelling was doomed - none of them
*is* a mask.

**How to work it.** Read `MB`/`ME` first. `MB == ME` is a single-bit test (`x & (1 << (31-MB))`, so 24 -> `0x80`);
`MB = 0, ME = 31` is a plain copy; `ME = 31` with `SH = 0` and `MB > 0` is a low-bits mask (`clrlwi`); a middle range
(`MB=20, ME=23`) is a field mask (`x & 0x0F00`). Then cross-check the reading against a sibling that is already at
100 % and touches the same byte - here `quest_move_state_get` masks the same byte with `0x7F`, which confirms
bit 7 means "a state follows" and the low seven bits are the state itself. The demo shows each reading:

```
x & 0x80            lbz r0,8(r3)  rlwinm r3,r0,0,24,24     single bit, mask in place
(x & 0x80) != 0     lbz r0,8(r3)  rlwinm r3,r0,25,31,31    the same bit rotated down to bit 31
x != 0              lbz r3,8(r3)  neg/or/srwi ...          no rlwinm at all
x & 0x0F00          rlwinm r3,r3,0,20,23                   field mask
x & 0xFF            clrlwi r3,r3,24                        objdump prints this one as an alias
```

**When NOT to apply.** If the `rlwinm` has `SH != 0` with `MB..ME` covering a shifted field, it is an
extract/insert (`extrwi`, `insrwi`, a bitfield access), not a single-bit test; and a `rlwinm.` (record form) is a
compare-with-zero the peephole fused (idea 39). Do not use `MB == ME` to *invent* a mask the source never
had: confirm it with a sibling function or a call site.

**Result.** `(work->state_0x22D4 & 0x80) != 0` took the row to **100.00000 %** - byte- *and* relocation-identical,
no pragma needed. The fix also forced the row's **name** to change (`quest_move_state_ck` ->
`quest_move_state_valid_ck`, map row plus 11 call sites), because the old name came from the disproven body.
"Several spellings fold to the same short form" is evidence about the *spellings*, never about what the retained
instruction means.

**Evidence.** (Measured at the time.) The tree already contained the answer twice at 100 %:
`src/Pl/fn_8027D684.cpp` writes `(self->field_0x655 & 0x80) == 0` and compiles with the peephole **on**, and a
`src/camera` tail emits the target's whole sequence. Related: idea 86 (why the alias `extrwi` never shows up in an
objdump), 21 (fingerprints built from instruction forms), 38 (a masked store where retail stores raw).
