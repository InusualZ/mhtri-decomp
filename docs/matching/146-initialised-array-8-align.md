---
id: 146
title: An initialised array whose size is a multiple of 8 is 8-aligned, so a 4-aligned target object is not one array
status: works
problem: A `.data` run that the map calls one array is not byte-identical when emitted as one array - ours is padded to an 8-byte boundary where the target continues 4-aligned.
tags: [data, sections]
applies: [Wii/1.3]
demo: 
related: [55, 78, 80]
---

# 146. An initialised array whose size is a multiple of 8 is 8-aligned, so a 4-aligned target object is not one array

**Problem.** A unit's `.data` head is several initialised arrays. Emitting a run the map names as one object
(`wudFirmwareData`, 0x78 B) inserts 4 bytes of padding before it, and everything after it in the section shifts.

**Why it happens.** MWCC aligns an initialised array whose size is a multiple of 8 to 8 bytes. A target object
that starts 4-aligned right after a predecessor ending on a non-8-aligned address therefore cannot have a size
that is a multiple of 8: the run is really two (or more) objects whose sizes are not.

**How to work it.** When the start address of a `.data` object is 4- but not 8-aligned and ours pads it, look for a
split inside the run where the referrers index it (the code addresses `obj[8 + n]` and `table.records[n]` at
fixed offsets), and define separate arrays/structs whose sizes reproduce the target addresses. Rename and resize
the map rows in the same change.

**Result.** `WUD/wud` (Wii/1.3): the 0x78-byte run is a 0x5C-byte patch table (`WUDPatchTable`) and a 0x1C-byte
search-cancel command; with the five arrays in definition order the unit's `.data` is byte-identical (was 4333 of
4520 B).

**Example.**

```c
static u8 wudPatchImage[...];
static WUDPatchTable wudPatchTable = { ... };   /* 0x5C B: stays 4-aligned */
static u8 wudSearchCancelCommand[0x1C] = { ... };
```
