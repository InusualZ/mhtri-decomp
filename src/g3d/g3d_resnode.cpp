/*
 * g3d/g3d_resnode.cpp - nw4r g3d `ResNode` animation results (`SetAnmResult`/`GetAnmResult` by body) and setters.
 * RANGE. .text 0x80098D5C-0x80099400 (7 functions); extab, extabindex, .data 0x805915C0-0x80591618 (the three
 *   assert strings), .sdata2 0x80795F48-0x80795F50.  Right seam by class: fn_800993B4 calls `ResNode::ref`
 *   (fn_8005D218), fn_80099400 calls `ResShp::ref` (fn_80077674).
 * NAMES. Map stems; `mSubResOfs` is a GUESS (its target class is evidenced only by res_user_data_ctor's
 *   `g3d_resuser_ac.h`).
 * RESIDUALS. none; flipcheck: `.data` and `.sdata2` byte-identical.
 * SHAPES. File-scope `#pragma peephole off`: every `rlwinm` flag extract keeps retail's `cmpwi` (fn_80099134).
 *   `#pragma pool_data off`; the strings and the 1.0f/0.0f pool are literals, emitted in first-use order.
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_resnode.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma pool_data off

/* The target's `-O3` schedule is retail only with the peephole pass off: every flag extract keeps an
 * explicit `cmpwi` after the `rlwinm` instead of the folded record form `rlwinm.`. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The neighbours this unit calls (plain map stems), each owner named beside it. */
extern "C" {
void mtx34_identity(MTX34* pMtx);                               /* owner: src/fn_8004CAD8.cpp */
s32 res_node_is_valid(const ResHandle* pSelf);                     /* owner: src/g3d/fn_8005AA28.cpp */
void* res_handle_ptr(const ResHandle* pSelf);                   /* owner: src/g3d/fn_8005AA28.cpp */
ResNodeData* res_node_ptr(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
ResNodeData* res_node_ref_nonconst(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
VEC3* vec3_copy_construct(VEC3* pOut, const VEC3* pIn);              /* owner: src/g3d/g3d_anmscn.cpp */
void mtx34_rot_xyz_deg(f32* pOut, f32 x, f32 y, f32 z);            /* owner: src/g3d/g3d_resanmchr.cpp */
void* res_user_data_ctor(void* pOut, u32 value);                    /* owner: src/g3d/g3d_resanmcamera.cpp */
}

/* The unit's own functions, in address order (the forward declarations keep the source order
 * free).  They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {

void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult);
void fn_80098F6C(ResHandle* pSelf, AnmResult* pResult);
void fn_80099134(ResNodeData* pSelf);
void res_node_trans_set(ResHandle* pSelf, f32 x, f32 y, f32 z);
void res_node_rot_set(ResHandle* pSelf, f32 x, f32 y, f32 z);
u32 res_node_user_data_get(ResHandle* pSelf);
u32 fn_800993B4(ResHandle* pSelf, u32 ofs);

/* -------------------------------------------------------------------------------------------------- */

/* Apply one `AnmResult` to the node: the target reads the record's channel bits and rewrites the
 * scale/rotation-matrix/translation parts it selects from the node's own state. */
void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult) {
    if (pResult == NULL) {
        nw4r::db::Panic("g3d_resnode.cpp", 0x20, "NW4R:Pointer must not be NULL (pResult)");
    }
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic("g3d_resnode.cpp", 0x21, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf) != 0) {
        u32 flags = pResult->flags;
        ResNodeData* pData = res_node_ptr(pSelf);
        if ((flags & 0x80) != 0) {
            u32 dataFlags = pData->mFlags;
            if ((dataFlags & 0x8) != 0) {
                pResult->scale[2] = 1.0f;
                pResult->scale[1] = 1.0f;
                pResult->scale[0] = 1.0f;
                flags |= 0x18;
            } else {
                if ((dataFlags & 0x10) != 0) {
                    flags |= 0x10;
                } else {
                    flags &= ~0x10;
                }
                copyVec3((VEC3*)pResult->scale, (const VEC3*)pData->mScale);
                flags &= ~0xA;
            }
        }
        if ((flags & 0x100) != 0) {
            VEC3 translate;
            setVec3(&translate, pResult->mtx[3], pResult->mtx[7], pResult->mtx[11]);
            if ((pData->mFlags & 0x4) != 0) {
                mtx34_identity((MTX34*)pResult->mtx);
                flags |= 0x20;
            } else {
                mtx34_rot_xyz_deg(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
                flags &= ~0x26;
            }
            pResult->mtx[3] = translate.x;
            pResult->mtx[7] = translate.y;
            pResult->mtx[11] = translate.z;
            flags |= 0x80000000;
        }
        if ((flags & 0x200) != 0) {
            if ((pData->mFlags & 0x2) != 0) {
                pResult->mtx[11] = 0.0f;
                pResult->mtx[7] = 0.0f;
                pResult->mtx[3] = 0.0f;
                flags |= 0x40;
            } else {
                pResult->mtx[3] = pData->mTranslate[0];
                pResult->mtx[7] = pData->mTranslate[1];
                pResult->mtx[11] = pData->mTranslate[2];
                flags &= ~0x46;
            }
        }
        if ((flags & 0x20) != 0 && (flags & 0x40) != 0) {
            flags |= 0x4;
            if ((flags & 0x8) != 0) {
                flags |= 0x2;
            }
        }
        pResult->flags = flags & ~0x380;
    }
}

/* Fill one `AnmResult` from the node's own animation state (the pair's other direction). */
void fn_80098F6C(ResHandle* pSelf, AnmResult* pResult) {
    if (pResult == NULL) {
        nw4r::db::Panic("g3d_resnode.cpp", 0x7E, "NW4R:Pointer must not be NULL (pResult)");
    }
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic("g3d_resnode.cpp", 0x7F, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = res_node_ptr(pSelf);
        u32 flags = 0;
        u32 dataFlags = pData->mFlags;
        if ((dataFlags & 0x8) != 0) {
            flags |= 0x18;
            pResult->scale[2] = 1.0f;
            pResult->scale[1] = 1.0f;
            pResult->scale[0] = 1.0f;
        } else {
            if ((dataFlags & 0x10) != 0) {
                flags |= 0x10;
            }
            copyVec3((VEC3*)pResult->scale, (const VEC3*)pData->mScale);
        }
        if ((pData->mFlags & 0x4) != 0) {
            mtx34_identity((MTX34*)pResult->mtx);
            flags |= 0x20;
        } else {
            VEC3 rotate;
            copyVec3((VEC3*)pResult->rotate, vec3_copy_construct(&rotate, (const VEC3*)pData->mRotate));
            mtx34_rot_xyz_deg(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
        }
        if ((pData->mFlags & 0x2) != 0) {
            flags |= 0x40;
        } else {
            pResult->mtx[3] = pData->mTranslate[0];
            pResult->mtx[7] = pData->mTranslate[1];
            pResult->mtx[11] = pData->mTranslate[2];
        }
        if ((flags & 0x20) != 0 && (flags & 0x40) != 0) {
            flags |= 0x4;
            if ((flags & 0x8) != 0) {
                flags |= 0x2;
            }
        }
        flags |= 0x80000000;
        flags |= 0x1;
        if ((pData->mFlags & 0x20) != 0) {
            flags |= 0x400;
        }
        if ((pData->mFlags & 0x40) != 0) {
            flags |= 0x800;
        }
        pResult->flags = flags;
    }
}

/* Keep `ResNodeData`'s "all three channels are animated" bit coherent with the rotate/translate/scale
 * bits beside it. */
void fn_80099134(ResNodeData* pSelf) {
    u32 flags = pSelf->mFlags;
    if ((flags & 0x2) != 0 && (flags & 0x4) != 0 && (flags & 0x8) != 0) {
        pSelf->mFlags = flags | 0x1;
    } else {
        pSelf->mFlags &= ~0x1;
    }
}

/* Store the translate triple, setting the channel bit when the triple is zeroed. */
void res_node_trans_set(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic("g3d_resnode.cpp", 0xEF, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)res_handle_ptr(pSelf);
        if (0.0f == x && 0.0f == y && 0.0f == z) {
            pData->mFlags |= 0x2;
        } else {
            pData->mFlags &= ~0x2;
        }
        fn_80099134(pData);
        pData->mTranslate[0] = x;
        pData->mTranslate[1] = y;
        pData->mTranslate[2] = z;
    }
}

/* Store the rotate triple, setting the channel bit when the triple is zeroed. */
void res_node_rot_set(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic("g3d_resnode.cpp", 0x103, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)res_handle_ptr(pSelf);
        if (0.0f == x && 0.0f == y && 0.0f == z) {
            pData->mFlags |= 0x4;
        } else {
            pData->mFlags &= ~0x4;
        }
        fn_80099134(pData);
        pData->mRotate[0] = x;
        pData->mRotate[1] = y;
        pData->mRotate[2] = z;
    }
}

/* Resolve the node's sub-resource: the offset at +0x6C, from the node's own base. */
u32 res_node_user_data_get(ResHandle* pSelf) {
    ResNodeData* pData = res_node_ref_nonconst(pSelf);
    return fn_800993B4(pSelf, pData->mSubResOfs);
}

/* Build the one-word sub-resource handle at `base + ofs` (0 for a null offset) and return its word. */
u32 fn_800993B4(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        u32 present;
        return *(u32*)res_user_data_ctor(&present, base + ofs);
    }
    u32 absent;
    return *(u32*)res_user_data_ctor(&absent, 0);
}

}  /* extern "C" */
