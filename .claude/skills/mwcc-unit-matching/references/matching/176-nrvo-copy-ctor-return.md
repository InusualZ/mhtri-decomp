---
id: 176
title: A C++ accessor that builds its returned vector in the return slot: return a copy-constructible view of the type
status: works
problem: A function returning a POD vector/matrix by value keeps a local and copies it out (bigger frame, extra lwz/stw), where retail works on the sret pointer directly
tags: [source-shape]
applies: []
demo: 
---

# 176. A C++ accessor that builds its returned vector in the return slot: return a copy-constructible view of the type

**Problem.** Retail's `VEC3 get_camera_pos()` calls `VEC3_ctor` with the sret pointer (`mr r31,r3; bl VEC3_ctor`) and
hands `r31` to the callee that fills it; ours allocates `pos` on the stack and copies the twelve bytes into the return
slot at the end (frame 0x30 against 0x20, two `lwz`/`stw` pairs).

**Why it happens.** MWCC applies the named-return-value optimisation only when the returned class has a user-declared
copy constructor. The project's `nw4r::math::VEC3`/`MTX34` are plain aggregates, so the named local is copied out. The
original's vector class had constructors (the out-of-line `VEC3_ctor`, `setVec3`, `copyVec3` are its default
constructor, three-float constructor and assignment).

**How to work it.** A C++ function's mangling carries no return type (`get_camera_pos__Fv`), so the owner can return a
local view of the type that adds only an inline copy constructor, with the same layout:
`struct CamVec3 : nw4r::math::VEC3 { CamVec3() {} CamVec3(const CamVec3& o) { x = o.x; y = o.y; z = o.z; } };`.
Declare and define the accessor returning the view in the owner's TU (do not include a header that declares it with the
plain type); callers elsewhere keep the plain type - the ABI is the same sret pointer. A returned temporary of the view
type (`return cam_vec3_sub(&a, &b);` through an out-pointer helper view, idea 177) is built in the return slot too.

**Result.** camera/camera_main: get_camera_pos 59.42 -> 100, get_current_view_mtx 0 -> 100 (the `MTX34` view),
get_camera_direction 67.06 -> 100.

**Example.**

```
struct CamVec3 : nw4r::math::VEC3 { CamVec3() {} CamVec3(const CamVec3& o) { x = o.x; y = o.y; z = o.z; } };
CamVec3 get_camera_pos(void)
{
    CamVec3 pos;                 /* built in r3's slot: VEC3_ctor(r31) */
    VEC3_ctor(&pos);
    ...
    return pos;
}
```
