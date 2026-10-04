---
id: 99
title: An aggregate copy's single/pair grouping names the record's members (an s64 inside a record)
status: works
problem: a word-by-word copy function mixes single `lwz/stw` moves with `lwz r5; lwz r0; stw r5; stw r0` pairs at irregular offsets
tags: [source-shape]
applies: []
demo: 
---

# 99. An aggregate copy's single/pair grouping names the record's members (an s64 inside a record)

**Problem.** A copy function (`*dst = *src` on a 0x40-byte record) moves some words alone and others as
high-word-first pairs at irregular offsets (single at +0x0, pair at +0x4, single at +0xC, pair at +0x10, ...).
Spelling the record as `u32 words[16]` pairs everything; member-by-member `u32` assignments move everything
alone; neither reaches the grouping.

**Why it happens.** MWCC's aggregate copy follows the record's member types: a 4-byte member is one
`lwz`/`stw`, an 8-byte `s64` member is a two-register move.  A bare `s64` member is moved low register first
(`lwz r0,+4 / lwz r5,+8 / stw r5 / stw r0` in the wrong order); an `s64` wrapped in a one-member record is
moved high word first (`lwz r5; lwz r0; stw r5; stw r0`), which is the retail order.  An `s64` is 8-aligned,
so a record that keeps it at a 4-aligned offset needs `#pragma pack(push, 4)`.

**How to work it.** Read the grouping off the target as the member list: every pair is an 8-byte member,
every single a 4-byte one.  Declare the pairs as a one-member record around an `s64`, pack the outer record
to 4 when the pairs sit at 4-aligned offsets, and copy with `*dst = *src`.

**Result.** `Network/network_pat_control` `copyPatSettings` (0x80431420, 132 B): `u32 words[16]` 68.24 %,
member-wise `u32` 74.27 %, bare `s64` members (pack 4) 98.55 %, `s64` wrapped in a record (pack 4) 100 %.

**Example.**

```
#pragma pack(push, 4)
typedef struct PatSettingsPair { s64 value_0x00; } PatSettingsPair;
typedef struct PatSettings {
    u32 word_0x00; PatSettingsPair pair_0x04; u32 word_0x0C; PatSettingsPair pair_0x10; /* ... */
} PatSettings;
#pragma pack(pop)
void copyPatSettings(PatSettings* dst, const PatSettings* src) { *dst = *src; }
```
