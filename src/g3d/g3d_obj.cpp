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
 * NAMES. nw4r's (the map's manglings).  g3d_obj_dealloc_tail (0x8007B344) is a GUESS: the 4-byte forwarder
 *   Dealloc branches to (`b` to the next word), the shape of `g3d/g3d_anmchr.cpp`'s Alloc/g3d_obj_alloc_tail pair.
 * RESIDUALS. Dealloc's map row covers 8 bytes (0x8007B340-0x8007B348) and pairs with our 4-byte Dealloc until it
 *   is split at +4 into g3d_obj_dealloc_tail (request filed); the object's bytes are retail's.
 *   flipcheck: `.rodata` (the type-name record, declared not defined) and `.data` (the vtable) are claimed and not
 *   emitted.
 * SHAPES. The destructor's body is the heap release; the compiler adds the vtable store and the deleting tail.
 */

#include "types.h"
#include "g3d/g3d_obj.h"                /* nw4r::g3d::G3dObj, anm_typename_G3dObj (rule 1) */
#include "g3d/g3d_anmchr.h"             /* type_obj_set_name (rule 2) */
#include "MEM/MEMAllocFromAllocator.h"  /* MEMFreeToAllocator (rule 2) */

#pragma peephole off

/* The "G3dObj" run-time type-name record: the name's size with its NUL, then the name. */
extern "C" const u8 anm_typename_G3dObj[] = {0, 0, 0, sizeof("G3dObj"), 'G', '3', 'd', 'O', 'b', 'j', 0};

/* 0x8007B2D4 (0x6C): returns the object's memory to the heap it came from. */
nw4r::g3d::G3dObj::~G3dObj() {
    Dealloc(mpHeap, this);
}

extern "C" {
/* untyped: byte range - the block returned to the allocator */
static void g3d_obj_dealloc_tail(MEMAllocator* pHeap, void* pBlock);
}

/* 0x8007B340: returns a block to the allocator. */
void nw4r::g3d::G3dObj::Dealloc(MEMAllocator* pHeap, void* pBlock) { /* untyped: byte range */
    g3d_obj_dealloc_tail(pHeap, pBlock);
}

extern "C" {
/* 0x8007B344: forwards the block to the allocator's free. */
/* untyped: byte range - the block returned to the allocator */
static void g3d_obj_dealloc_tail(MEMAllocator* pHeap, void* pBlock)
{
    MEMFreeToAllocator(pHeap, pBlock);
}
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
