---
id: 162
title: A record copied as lfd/stfd pairs is a loop over an f64 view, not a struct assignment
status: works
problem: Retail copies a 0x30-byte record as six `lfd`/`stfd` pairs while ours copies it member by member (`lwz`/`stw`), or calls an out-of-line `__as__` under `dont_inline`.
tags: [source-shape, data]
applies: [Wii/1.3]
demo: 
related: [99, 163]
---

# 162. A record copied as lfd/stfd pairs is a loop over an f64 view, not a struct assignment

**Problem.** A record copy is a run of `lfd`/`stfd` pairs in retail. Ours copies the same bytes with word or
halfword moves, or (under `#pragma dont_inline on`) calls an out-of-line assignment operator.

**Why it happens.** A struct assignment in this compiler is memberwise: each member moves in its own type, and under
`dont_inline` the implicit `operator=` becomes a weak call. Doubleword moves only appear when the source moves `f64`s.

**How to work it.** Declare a view of the record as `f64 words[N]` (the record's size, rule 3) and copy it in a `for`
loop over the view; MWCC unrolls the loop into the `lfd`/`stfd` pairs. Keep the record's own type everywhere else.

**Result.** `g3d/g3d_fog` (Wii/1.3): `Fog::CopyTo` -> 100 with a `FogDataBlock { f64 words[6]; }` view (the
`FogData` assignment was memberwise, and out of line under the file's `dont_inline`); the unit is `Matching`.

**Example.**

```
struct FogDataBlock { /* +0x00 */ f64 words[6]; };    /* size: 0x30 */
for (int i = 0; i < 6; i++)
    ((FogDataBlock*)pDst)->words[i] = pSrc->words[i];  /* six lfd/stfd pairs */
```
