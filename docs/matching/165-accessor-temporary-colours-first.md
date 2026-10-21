---
id: 165
title: An inline accessor makes a local a compiler temporary that is coloured ahead of the user locals
status: works
problem: A value read in a loop sits in the lowest callee-saved register (r23) where retail has it high (r28), renumbering every other saved register; or a call's hoisted float constants load in the wrong order.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 63, 124]
---

# 165. An inline accessor makes a local a compiler temporary that is coloured ahead of the user locals

**Problem.** A value read in a loop (`tex = pp->texture[layer]`) takes the lowest saved register in ours and a high
one in retail, and every other saved register is renumbered around it; no declaration order moves it.

**Why it happens.** Read through a one-line `inline` accessor, the value is first the inline's return temporary, a
compiler web that enters colouring before the user locals (idea 63), so it gets the higher register. In the same way,
float arguments named as locals before a call fix the order their pool constants are loaded in.

**How to work it.** Read the value through a small `inline` accessor (`ef_particle_texture(pp, layer)`), and name a
call's float arguments as locals in retail's load order (`f32 max_lod = ...; f32 min_lod = 0.0f;`).

**Result.** `ef/ef_drawstrategyimpl` (Wii/1.3) `SetupGPTexture`: the LOD locals 97.31 -> 98.87, the accessor
98.87 -> 99.50 (`tex` r23 -> r28, the other saved registers to retail's numbers). Residual 99.50 (`tex` r28 vs r30).

**Example.**

```
inline EfTextureData* ef_particle_texture(EfDrawParticle* pp, int layer) { return pp->texture[layer]; }
tex = ef_particle_texture(pp, layer);   /* coloured first: r28; `pp->texture[layer]` gave r23 */
```
