---
id: 247
title: An 8-byte run loaded as two words then stored in order is a struct-with-array block copy
status: works
problem: retail copies two adjacent words as lwz r5,8 / lwz r0,0xC / stw r5,8 / stw r0,0xC and field or u64 copies reorder it
tags: [source-shape]
applies: [quest/quest_entry]
demo: 
---

# 247. An 8-byte run loaded as two words then stored in order is a struct-with-array block copy

**Problem.** Retail copies an 8-byte run as two loads followed by two stores in address order (`lwz r5,8(r4)`,
`lwz r0,0xC(r4)`, `stw r5,8(r3)`, `stw r0,0xC(r3)`). Two field assignments interleave load/store pairs, two locals
come out in some other load order, a `u64` copy stores the high word last, and a struct of two named `u32`/`f32`
fields is copied field by field.

**Why it happens.** MWCC copies a small struct assignment as a block (all loads, then all stores) only when the struct
holds an array; a struct of scalar members is copied member by member.

**How to work it.** Copy the run through a struct whose one member is the array, `struct { u32 words[2]; }` (a
`u16 v[4]` behaves the same).

**Result.** quest/quest_entry: quest_element_copy 99.17 % -> 100 % (the `+0x08` and `+0x14` runs of `Q_ElementBlock`,
copied through `QuestElementPair`).

**Example.**

```
typedef struct { u32 words[2]; } Pair;
*(Pair*)&d->w8 = *(const Pair*)&s->w8;    /* lwz r5; lwz r0; stw r5; stw r0 */
```
