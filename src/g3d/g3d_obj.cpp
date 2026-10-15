/*
 * g3d/g3d_obj.cpp - nw4r g3d `G3dObj`: the destructor, the heap release, Destroy and the run-time type members the
 *   class's vtable carries.
 * RANGE. .text 0x8007B2D4-0x8007B424 (5 functions); extab 0x800084A0-0x800084C0, extabindex 0x80020D18-0x80020D48,
 *   .rodata 0x8056F668-0x8056F678 (the "G3dObj" type-name record GetTypeObj reads), .data 0x8058ED18-0x8058ED38
 *   (`__vt__Q34nw4r3g3d6G3dObj`).  The vtable sits right after `g3d/g3d_light.cpp`'s last string (0x8058ECF8), so the
 *   class's key function, the destructor at 0x8007B2D4, opens this TU; the vtable's own inline virtuals GetTypeObj
 *   and GetTypeName close it (IsDerivedFrom's kept copy is `g3d/g3d_anmchr.cpp`'s 0x80063964).  The next `.data`
 *   word, 0x8058ED38, is a second vtable whose first code slot (0x8007B7F0) lies above this one's (0x8007B2D4): the
 *   descending-vtable order breaks there, so `g3d/g3d_scninline.cpp` starts at 0x8007B424.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. nw4r's (the map's manglings).
 * RESIDUALS. Partial: Dealloc (8 bytes: the tail call to MEMFreeToAllocator).
 *   flipcheck: `.rodata` (the type-name record, declared not defined) and `.data` (the vtable) are claimed and not
 *   emitted.
 * SHAPES. The destructor's body is the heap release; the compiler adds the vtable store and the deleting tail.
 */

#include "types.h"
#include "g3d/g3d_obj.h"                /* nw4r::g3d::G3dObj, anm_typename_G3dObj (rule 1) */
#include "g3d/g3d_anmchr.h"             /* type_obj_set_name (rule 2) */
#include "MEM/MEMAllocFromAllocator.h"  /* MEMFreeToAllocator (rule 2) */

#pragma peephole off

/* 0x8007B2D4 (0x6C): returns the object's memory to the heap it came from. */
nw4r::g3d::G3dObj::~G3dObj() {
    Dealloc(mpHeap, this);
}

/* 0x8007B340 (0x8): returns a block to the allocator. */
void nw4r::g3d::G3dObj::Dealloc(MEMAllocator* pHeap, void* pBlock) { /* untyped: byte range */
    MEMFreeToAllocator(pHeap, pBlock);
}

/* 0x8007B348 (0x74): tells the parent the object is going away, then destroys it. */
void nw4r::g3d::G3dObj::Destroy() {
    G3dObj* pParent = GetParent();
    if (pParent != NULL) {
        pParent->G3dProc(G3DPROC_CHILD_DETACHED, 0, this);
    }
    delete this;
}

/* 0x8007B3BC (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::G3dObj::GetTypeObj() const {
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, anm_typename_G3dObj));
}

/* 0x8007B3EC (0x38): returns the type's name. */
const char* nw4r::g3d::G3dObj::GetTypeName() const {
    return GetTypeObj().GetTypeName();
}
