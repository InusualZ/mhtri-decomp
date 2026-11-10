---
id: 319
title: pragma pool_data off before the constructed globals keeps __sinit addressing each object by symbol
status: works
problem: a compiler-generated __sinit loads one `...bss.0` base and adds offsets where retail does a lis/addi per object
tags: [source-shape, sections]
applies: []
demo: 
---

# 319. pragma pool_data off before the constructed globals keeps __sinit addressing each object by symbol

**Problem.** A TU whose globals with constructors are defined at the foot of the file gets an `__sinit` that
materialises the TU's `.bss` once (`lis r3,...bss.0@ha; addi r31,r3,...bss.0@l`) and reaches every object as
`addi r3,r31,0x1024`; retail loads `lis`/`addi` of each object's own symbol.  objdiff scores the rows as argument
mismatches (91.88 % for light's `__sinit`).

**Why it happens.** `__sinit` is generated after the definitions, so MWCC knows the objects share one section and
pools their addresses behind the section anchor (the same mechanism as idea 128).  `#pragma pool_data off` stops
that pooling for the code generated after it.

**How to work it.** Put `#pragma pool_data off` directly above the constructed definitions at the foot of the file
(after every function, so only `__sinit` sees it), rebuild and re-measure the `__sinit` row.

**Result.** `light/light` `__sinit_\light_cpp` 91.88 -> 100 (0xA0), no other row of the unit moved.

**Example.**

```
#pragma pool_data off
LightPointWork light_point_work;
math::VEC3 light_dir_center(0.0f, 0.0f, 0.0f);
math::VEC3 light_lobby_dir[4] = { ... };
```
