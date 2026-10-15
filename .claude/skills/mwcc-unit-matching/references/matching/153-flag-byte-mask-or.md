---
id: 153
title: A flag-byte store as rlwinm + ori is a mask-and-or; a 1-bit bitfield gives rlwimi
status: todo
problem: Retail updates a visibility bit with `rlwinm rX,rX,0,24,30` (+ `ori`) before the `stb`, where a 1-bit bitfield gives ours an `rlwimi`.
tags: [source-shape]
applies: [GC/3.0a5.2]
demo: 
related: [77, 95]
---

# 153. A flag-byte store as rlwinm + ori is a mask-and-or; a 1-bit bitfield gives rlwimi

**Problem.** Setting or clearing one bit of a flags byte comes out as an `rlwimi` insert in ours; retail clears the
bit with `rlwinm rX,rX,0,24,30` and sets it with `ori`.

**Why it happens.** A bitfield assignment is an insert (`rlwimi`). The original wrote the byte explicitly:
`flags = (flags & 0xFE) | bit`, which is a mask (`rlwinm` with an inclusive bit range, idea 77) and an `or`.

**How to work it.** Where retail has the `rlwinm 24,30` (+ `ori`) pair, model the byte as a plain `u8` field and
write the mask-and-or; keep the bitfield only where retail uses `rlwimi`.

**Result.** Used in the homebutton band (GC/3.0a5.2) for the panes' visibility stores; before/after numbers were
not recorded.

**Example.**

```c
pane->flags = (u8)((pane->flags & 0xFE) | (visible ? 1 : 0));   /* rlwinm 24,30 ; ori ; stb */
```
