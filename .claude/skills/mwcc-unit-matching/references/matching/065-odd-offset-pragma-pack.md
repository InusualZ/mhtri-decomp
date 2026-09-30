---
id: 65
title: A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured
status: works
problem: A record has a `u32` at an odd offset (`stw r0, 0x4485(r31)`, then `0x4489`); declared plainly it aligns to `0x4488` and every later field moves, and declared as bytes the same source emits `stb` where retail has `stw`.
tags: [pragma, source-shape]
applies: [Network]
demo: 065-odd-offset-pragma-pack.cpp
reviewed: 2026-09-29
related: [72, 51]
---

# 65. A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured

**Problem.** `Network/fn_8041A87C`'s `profile_4485` is five `u32` starting at an **odd** offset
(`stw r0, 0x4485(r31)`, then `0x4489`, ...). Declared without a pragma the member silently aligns to `0x4488`
and every later field moves; declaring the region as bytes instead makes the same source emit `stb` where
retail has `stw`.

**How it looks.** A store or load of a word at an odd displacement in the target (`stw r4,1(r3)`); ours uses
the aligned neighbour or a byte access, and every later offset in the record is off by the padding.

**Why it happens.** A natural-alignment struct can never place a `u32` at an odd offset, so an odd 32-bit
offset is direct evidence the original was compiled with `#pragma pack(1)` (or a packed region). The pack is
the conformant spelling (rule 6 forbids reaching the field through pointer arithmetic). It is a *layout*
lever, so it moves the whole record and changes `sizeof` at once.

**How to work it.** Wrap the record in `#pragma pack(1)` ... `#pragma pack()`. Then re-measure **every**
function of the unit: an array or `field + 4` that used to step 4 bytes may now step differently (see below).

**When NOT to apply.** Only when the target shows the odd offset. If only some fields are unaligned, pack just
that record; packing everything hides real alignment padding the target does have.

**Result.** Measured at the time (2026-09): `startNegotiation` **93.39 -> 98.23**, the pack score-neutral
everywhere else. The follow-on trap: a `field + 4` that used to be a byte offset became **+16 bytes** (pointer
arithmetic on the typed array), so `checkPeerProfile` needed `(const u8*)profile_4485 + 4`.

**Example.**

```c
#pragma pack(1)                     /* the target's layout: u32 at 0x4485, not 0x4488 */
struct NetworkProfile {
    /* +0x4485 */ u32 flags;        /* stw r0, 0x4485(r31) */
    /* +0x4489 */ u32 state;
};
#pragma pack()
```

**Demonstration.** `065-odd-offset-pragma-pack.cpp` (`ideas.py demo-check 65`): a `u8` then a `u32` gives
`stw ...,4(r3)` and `sizeof` 8 plain, `stw ...,1(r3)` / `5(r3)` and `sizeof` 9 under `pack(1)`.
