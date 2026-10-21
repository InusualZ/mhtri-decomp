---
id: 166
title: A unit that owns its literal pool writes literals, not externs of the map's pool names
status: works
problem: Float rows read the map's pool names (`extern f32 lbl_...`) and come out with swapped `fmuls` operands, other registers, or a constant reloaded after every call, while the unit's own `.sdata2` is not emitted at all.
tags: [data, source-shape]
applies: [Wii/1.3]
demo: 
related: [29, 58, 94, 147, 174]
---

# 166. A unit that owns its literal pool writes literals, not externs of the map's pool names

**Problem.** A unit whose `.sdata2` range is its own declares the pool's constants `extern` (idea 29's old form). Its
float rows then differ from retail in operand order, register choice or reloads, and its `.sdata2` section is empty,
so flipcheck cannot pass it.

**Why it happens.** An `extern` object may alias any store or call, so MWCC reloads it after each one and does not
hoist it; a literal is a constant, hoisted and kept in a register as retail does. Once the unit owns the whole pool,
its literals rebuild exactly the target's pool: MWCC pools them in creation order. An inline-asm `lfs f0, 0.5f`
names a literal that joins the pool at that point, and a named `const` object is created at its definition (a
top-of-file constant comes first, a later one after the literals before it).

**How to work it.** Replace the externs with literals (and named `const`s where retail's order needs them), measure
the rows and byte-compare `.sdata2`. If a literal is pooled out of order, the pool needs a function retail
compiled before it (idea 147). Keep idea 29's declare-only form for a pool the unit shares or does not emit.

**Result.** Wii/1.3: `ef/ef_postfield` hit tests (`fmuls` operand order) to 100 and `.sdata2` byte-identical, unit
99.99 -> 100; `ef/ef_animcurve` `.sdata2` byte-identical; `ef/ef_particle` `ef_particle_param_init` stopped
reloading 2.0/1.0/100.0 and its `.sdata2` (0x40) came out byte-identical; `ef/ef_particlemanager`
`ef_field_random` registers fixed.

**Example.**

```
extern const f32 lbl_80795F10;    /* reloaded after every call; the unit's .sdata2 is empty */
x = y * lbl_80795F10;
x = y * 0.5f;                     /* hoisted; the pool is rebuilt in creation order */
```
