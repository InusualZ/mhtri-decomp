---
id: 65
title: A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured
status: works
problem: `Network/fn_8041A87C`'s `profile_4485` is five `u32` starting at an **odd** offset (`stw r0, 0x4485(r31)`, then `0x4489`, ...). Declared without a pragma the member silently aligns to `0x4488` and every later field moves; declaring the region as bytes instead makes the same source emit `stb` where retail has `stw`.
tags: [pragma, measurement]
applies: []
demo:
---

# 65. A 32-bit member at an ODD offset needs `#pragma pack(1)`, and the whole unit must be re-measured

**Problem.** `Network/fn_8041A87C`'s `profile_4485` is five `u32` starting at an **odd** offset
(`stw r0, 0x4485(r31)`, then `0x4489`, ...). Declared without a pragma the member silently aligns to `0x4488`
and every later field moves; declaring the region as bytes instead makes the same source emit `stb` where
retail has `stw`.

**Why try it.** An odd 32-bit offset is direct evidence the original was compiled with `#pragma pack(1)`, and
the pack is the conformant spelling (rule 6 forbids reaching the field through pointer arithmetic). It is a
*layout* lever, so it moves the whole record at once - measure the unit, not the function.

**Result.** `startNegotiation` **93.39 -> 98.23**, and re-measuring the unit showed the pack score-neutral
everywhere else. The follow-on is the trap: a `field + 4` that used to be a byte offset becomes **+16 bytes**,
so `checkPeerProfile` needed `(const u8*)profile_4485 + 4`. Pack first, then re-measure **every** function of
the unit - a field that moved is a regression the one moved function cannot show.

**Example.**

```c
#pragma pack(1)                     /* the target's layout: u32 at 0x4485, not 0x4488 */
struct NetworkProfile {
    /* +0x4485 */ u32 flags;        /* stw r0, 0x4485(r31) */
    /* +0x4489 */ u32 state;
};
```
