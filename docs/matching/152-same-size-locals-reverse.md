---
id: 152
title: Same-size stack locals are placed in reverse declaration order, smaller ones first
status: todo
problem: Two stack locals' offsets are swapped against retail (`0x8(r1)` vs `0x10(r1)`), with the instructions otherwise identical.
tags: [allocator]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [63, 123]
---

# 152. Same-size stack locals are placed in reverse declaration order, smaller ones first

**Problem.** A function with several address-taken stack locals (vectors, colours, records) matches except that
their stack offsets are permuted.

**Why it happens.** Observed: MWCC places smaller locals at lower offsets first, and among locals of the same size
the one declared last gets the lowest offset.

**How to work it.** Read the stack offsets off retail, sort its locals by size, and declare the same-size ones in
the reverse of retail's address order.

**Result.** Observed in `homebutton/gui` `hbmGuiDrawQuad` (GC/3.0a5.2); not yet demonstrated nor measured as a
before/after. Wii/1.3: `g3d/g3d_draw1mat1shp` `Draw1Mat1ShpDirectly` orders its frame by size class, then by
declaration order inside a class: its fur pass declares the arrays and colours before the handles to get retail's
slots (unit header SHAPES); the remaining 8-byte slot offset there is a retail temporary (RESIDUALS), so the lever
is still unmeasured as a before/after.

**Example.**

```c
Vec2 b;     /* declared first: higher offset */
Vec2 a;     /* declared last:  lower offset */
```
