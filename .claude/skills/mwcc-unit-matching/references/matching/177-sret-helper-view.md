---
id: 177
title: An out-pointer vector helper whose result lands in a temporary: call it through a view that returns the vector
status: works
problem: The stack slots of the helper results (subVec3/addVec3 outputs copied with copyVec3) are ordered differently from retail, shifting every addi rN,r1,X
tags: [source-shape, allocator]
applies: []
demo: 
---

# 177. An out-pointer vector helper whose result lands in a temporary: call it through a view that returns the vector

**Problem.** Retail spells `v = a - b` as `subVec3(&tmp, &a, &b); copyVec3(&v, &tmp)` with `tmp` a compiler temporary;
writing `VEC3 tmp; subVec3(&tmp, ...)` names the temporary, and MWCC allocates named locals before temporaries, so the
slot of every result (and of every later local) moves: the diff shows `addi r3,r1,0x5c` against `0x14` all over a long
function.

**Why it happens.** The original's vector class had by-value operators whose out-of-line bodies take the result through
r3 (`subVec3(out, a, b)`), so each call made a temporary. MWCC places temporaries after the named locals, in source
order.

**How to work it.** Call the helper through a function-pointer view that returns the vector (the `_c1` convention):
`#define vec3_sub ((nw4r::math::VEC3 (*)(const VEC3*, const VEC3*))subVec3)` and write
`copyVec3(&v, &vec3_sub(&a, &b));`. The call is still a direct `bl subVec3` with r3 = the temporary. The same view
works for `addVec3`, `vec3_scale`, `vec3_div` and a C function that writes its result through r3
(`get_camera_target_pos`). Nested views follow MWCC's right-to-left argument evaluation (idea 161).

**Result.** camera/camera_main: camera_follow_update 97.55 -> 99.28 from the views alone (every slot matched); the whole
camera unit uses them.

**Example.**

```
#define vec3_sub ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
copyVec3(&diff, &vec3_sub(&work->shoulder.pos, &player_pos));
```
