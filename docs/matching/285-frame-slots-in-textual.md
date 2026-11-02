---
id: 285
title: Frame slots in textual order: a vector helper retail calls with a hidden result pointer is a by-value callee whose results are temporaries
status: works
problem: A function's VEC3 locals all match except their stack offsets, which are permuted against retail; named locals, block-scoped or not, keep their order and the by-value call temporaries always land below them.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [249, 105]
---

# 285. Frame slots in textual order: a vector helper retail calls with a hidden result pointer is a by-value callee whose results are temporaries

**Problem.** `addi r3,r1,0x3c` where retail has `addi r3,r1,0x18`: the per-case `VEC3` temporaries of a switch
(`t0`..`t8`, the results of `addVec3`/`subVec3`/`vec3_scale`) and the temporaries of two by-value
`get_worldworld_pos` calls sit in the wrong frame slots. Retail's nine slots descend in exact textual order of
the expressions that create them, with the two call temporaries in the middle.

**Why it happens.** MWCC lays out a function's named locals (every scope, in declaration order, first declared
highest) and only then its compiler temporaries, in textual order of the expressions. Declaring the named vectors
inside the case blocks, or after the statements that create the call temporaries, changes nothing (measured:
byte-identical object). Retail's slots can only all be temporaries - and `addi r3,r1,0x24 ; addi r4,self+0x188 ;
addi r5,r1,0x18 ; bl addVec3` is exactly a by-value `VEC3 addVec3(VEC3*, VEC3*)` with the hidden result pointer
in r3, which is also why no `VEC3` constructor runs for them (a named `VEC3 t;` would call the out-of-line
constructor under `-inline noauto`). A named `VEC3 w = f();` is not an option either: MWCC copies the call's
temporary into `w` (six loads/stores) instead of returning into it, and so does a `static inline` by-value helper.

**How to work it.** Call the helpers through a by-value view whose arguments are the inputs only, and use the
result's address in place: `copyVec3(&dst, &addVec3_byval(&self->pos, &subVec3_byval(&there, &here)))` with
`#define addVec3_byval ((nw4r::math::VEC3 (*)(nw4r::math::VEC3*, nw4r::math::VEC3*))addVec3)`. Nested calls put
the outer result above the inner one (the outer temporary is created first), which is retail's order too. Keep
the owner's out-pointer declaration; the view is this unit's spelling (rule 2 is untouched: the symbol is still
the owner's).

**Result.** `enemy/em_common` `em_target_pos_set`: every frame-slot diff gone (98.19 -> 99.78 %, then 100 % with
idea 282).

**Example.**

```
#define addVec3_byval ((nw4r::math::VEC3 (*)(nw4r::math::VEC3*, nw4r::math::VEC3*))addVec3)
copyVec3(&self->vec_0x36C, &addVec3_byval(&self->pos, &dir));   /* was: addVec3(&t0, &self->pos, &dir); copyVec3(..., &t0); */
```
