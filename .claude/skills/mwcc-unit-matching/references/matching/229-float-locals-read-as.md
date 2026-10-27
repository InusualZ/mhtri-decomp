---
id: 229
title: Float locals read as bits and reassigned stay in memory when they are one local array
status: works
problem: retail stores two float results, reads their bits with lwz, then on a match stores one over the other and reloads both with lfs before the final op; ours forwards the floats in registers (fmr/frsp, no reload)
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 229. Float locals read as bits and reassigned stay in memory when they are one local array

**Problem.** A float pair is compared bit-wise (`*(u32*)&a == *(u32*)&b + 1`) and then one is assigned over the other.
Retail keeps both values in their stack slots: `stfs` after the compare, `lfs`/`stfs` for the second assignment and
`lfs`/`lfs` before the subtraction. Ours spells them as two scalar locals and forwards the register values instead
(`fmr f0,f1` / `frsp`, no reloads), so the function is shorter and the tail does not pair.

**Why it happens.** Taking the address of a scalar local still lets MWCC keep a register copy of it and forward
that copy into later reads. An array element is always read and written through memory, so every assignment is a
store and every read after it a reload - exactly retail's sequence.

**How to work it.** Put the pair into one local array (`f32 v[2]`), compute into `v[0]`/`v[1]`, read the bits through
`*(u32*)&v[0]`, and write the assignments and the final expression on the elements. Keep the bit copies as `u32`
locals as before.

**Result.** `Pl/pl_coll` `pl_tri2d_cross` (Wii/1.3, -O3, the Pl flags): 84.68 -> 100.0, 116 -> 124 B (= retail).
Pointer locals to the two scalars (`f32* lp = &lhs`) changed nothing; comparing `*(u32*)&lhs` directly lost 11 B.

**Example.**

```
f32 products[2];
u32 lhs_bits, rhs_bits;
products[0] = (b->x - a->x) * (c->y - a->y);
products[1] = (c->x - a->x) * (b->y - a->y);
lhs_bits = *(u32*)&products[0];
rhs_bits = *(u32*)&products[1];
if (lhs_bits == rhs_bits + 1) products[0] = products[1];
if (lhs_bits == rhs_bits - 1) products[0] = products[1];
return products[0] - products[1];
```
