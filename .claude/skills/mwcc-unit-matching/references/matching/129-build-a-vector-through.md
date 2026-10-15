---
id: 129
title: Build a vector through a constructor, not member stores, when retail stages a temporary
status: works
problem: Retail stores a VEC2's components to a stack temporary (dead stores, or loads that read one component before the other) where ours writes the fields directly; frame size, scheduling and float registers drift.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
demo: 
related: [123, 124]
---

# 129. Build a vector through a constructor, not member stores, when retail stages a temporary

**Problem.** Around a two-float vector the diff shows one of: retail's frame is larger and every loop iteration
stores `0.0`/`1.0` to the same stack slots it never reads (`stfs f1,0x18(r1)` ... `stfs f0,0x24(r1)`), or two loads
of the vector's components come out in the other order (`lfs f3,0x44(r1)` before `lfs f2,0x40(r1)`), or a whole
1.5 KB function's float registers are coloured differently. Ours assigns `v.x = a; v.y = b;` field by field.

**Why it happens.** NW4R's `math::VEC2` has constructors (`VEC2(f32, f32)`), and the original wrote
`translate = math::VEC2(0, 0)` / `const math::VEC2 tSz(w, h)`. A user constructor makes the vector a
temporary object the compiler builds and then copies (or keeps in memory), which changes the frame, the order
the components are created in and with it the scheduling and the colouring. Member stores on a POD give the
optimizer two independent scalars instead.

**How to work it.** When the project's `VEC2` is a POD without constructors (`src/nw4r/math.h`), use a derived
stand-in with the constructors (`Point : math::VEC2` in `src/homebutton/hkb_material.h`: `Point(f32, f32)`,
`Point()`, a member-wise `operator=`) and build the value through it: `dst = Point(0.0f, 0.0f);` or
`Point tSz(width, height);`. Measure each site; an assignment *into* a `Point` from a `VEC2` (`Point p; p = v;`)
is a different shape and did not help the GetVtxPos copies.

**Result.** `homebutton/hkb_material` ReserveGXMem 78.20 -> 93.53 (the default SRTs, frame 0x40 -> 0x60);
`homebutton/hkb_window` DrawFrame 90.62 -> 100, DrawFrame4 99.97 -> 100, DrawFrame8 99.97 -> 100 (the
texture-coordinate helpers' `tSz`), unit 98.47 -> 99.94.

**Example.**

```
/* before */                         /* after */
math::VEC2 tSz;                      Point tSz(texSize.width, texSize.height);
tSz.x = texSize.width;
tSz.y = texSize.height;
```
