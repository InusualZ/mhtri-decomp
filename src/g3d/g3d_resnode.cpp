/*
 * g3d/g3d_resnode.cpp - nw4r g3d `ResNode` animation results (`SetAnmResult`/`GetAnmResult` by body) and setters.
 * RANGE. .text 0x80098D5C-0x80099400 (7 functions); extab, extabindex, .data 0x805915C0-0x80591618 (the three
 *   assert strings), .sdata2 0x80795F48-0x80795F50.  Right seam by class: fn_800993B4 calls `ResNode::ref`
 *   (fn_8005D218), fn_80099400 calls `ResShp::ref` (fn_80077674).
 * NAMES. Map stems; `mSubResOfs` is a GUESS (its target class is evidenced only by fn_8008A220's
 *   `g3d_resuser_ac.h`).
 * RESIDUALS. fn_80098D5C: register naming only (retail keeps the flag word in r31 and `flags & ~0x10` in r29,
 *   ours in r29/r28).  flipcheck: `.data` and `.sdata2` are claimed and not emitted.
 * SHAPES. File-scope `#pragma peephole off`: every `rlwinm` flag extract keeps retail's `cmpwi` (fn_80099134).
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_resnode.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

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

/* The pooled constants this unit reads, declared, not defined: the claimed `.data`/`.sdata2` are not emitted
 * yet. */
extern "C" {
extern const char lbl_805915C0[]; /* "g3d_resnode.cpp"                            .data */
extern const char lbl_805915D0[]; /* "NW4R:Pointer must not be NULL (pResult)"    .data */
extern const char lbl_805915F8[]; /* "NW4R:Failed assertion IsValid()"            .data */
extern const f32 lbl_80795F48;          /* 1.0f                                         .sdata2 */
extern const f32 lbl_80795F4C;          /* 0.0f                                         .sdata2 */
}

/* The neighbours this unit calls (plain map stems), each owner named beside it. */
extern "C" {
void mtx34_identity(MTX34* pMtx);                               /* owner: src/fn_8004CAD8.cpp */
s32 res_node_is_valid(const ResHandle* pSelf);                     /* owner: src/g3d/fn_8005AA28.cpp */
void* fn_8005AAE4(const ResHandle* pSelf);                   /* owner: src/g3d/fn_8005AA28.cpp */
ResNodeData* res_node_ptr(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
ResNodeData* fn_8005D218(const ResHandle* pSelf);            /* owner: src/g3d/g3d_anmchr.cpp */
VEC3* vec3_copy_construct(VEC3* pOut, const VEC3* pIn);              /* owner: src/g3d/fn_80063888.cpp */
void fn_8008C484(f32* pOut, f32 x, f32 y, f32 z);            /* owner: src/g3d/g3d_resanmchr.cpp */
void* fn_8008A220(void* pOut, u32 value);                    /* owner: src/g3d/g3d_resanmcamera.cpp */
}

/* The unit's own functions, in address order (the forward declarations keep the source order
 * free).  They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {

void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult);
void fn_80098F6C(ResHandle* pSelf, AnmResult* pResult);
void fn_80099134(ResNodeData* pSelf);
void fn_80099178(ResHandle* pSelf, f32 x, f32 y, f32 z);
void fn_80099278(ResHandle* pSelf, f32 x, f32 y, f32 z);
u32 fn_80099378(ResHandle* pSelf);
u32 fn_800993B4(ResHandle* pSelf, u32 ofs);

/* -------------------------------------------------------------------------------------------------- */

/* Apply one `AnmResult` to the node: the target reads the record's channel bits and rewrites the
 * scale/rotation-matrix/translation parts it selects from the node's own state. */
void fn_80098D5C(ResHandle* pSelf, AnmResult* pResult) {
    if (pResult == NULL) {
        nw4r::db::Panic(lbl_805915C0, 0x20, lbl_805915D0);
    }
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x21, lbl_805915F8);
    }
    if (res_node_is_valid(pSelf) != 0) {
        u32 flags = pResult->flags;
        ResNodeData* pData = res_node_ptr(pSelf);
        if ((flags & 0x80) != 0) {
            u32 dataFlags = pData->mFlags;
            if ((dataFlags & 0x8) != 0) {
                pResult->scale[2] = lbl_80795F48;
                pResult->scale[1] = lbl_80795F48;
                pResult->scale[0] = lbl_80795F48;
                flags |= 0x18;
            } else {
                u32 newFlags = flags & ~0x10;
                if ((dataFlags & 0x10) != 0) {
                    newFlags = flags | 0x10;
                }
                copyVec3((VEC3*)pResult->scale, (const VEC3*)pData->mScale);
                flags = newFlags & ~0xA;
            }
        }
        if ((flags & 0x100) != 0) {
            VEC3 translate;
            setVec3(&translate, pResult->mtx[3], pResult->mtx[7], pResult->mtx[11]);
            if ((pData->mFlags & 0x4) != 0) {
                mtx34_identity((MTX34*)pResult->mtx);
                flags |= 0x20;
            } else {
                fn_8008C484(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
                flags &= ~0x26;
            }
            pResult->mtx[3] = translate.x;
            pResult->mtx[7] = translate.y;
            pResult->mtx[11] = translate.z;
            flags |= 0x80000000;
        }
        if ((flags & 0x200) != 0) {
            if ((pData->mFlags & 0x2) != 0) {
                pResult->mtx[11] = lbl_80795F4C;
                pResult->mtx[7] = lbl_80795F4C;
                pResult->mtx[3] = lbl_80795F4C;
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
        nw4r::db::Panic(lbl_805915C0, 0x7E, lbl_805915D0);
    }
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x7F, lbl_805915F8);
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = res_node_ptr(pSelf);
        u32 flags = 0;
        u32 dataFlags = pData->mFlags;
        if ((dataFlags & 0x8) != 0) {
            flags |= 0x18;
            pResult->scale[2] = lbl_80795F48;
            pResult->scale[1] = lbl_80795F48;
            pResult->scale[0] = lbl_80795F48;
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
            fn_8008C484(pResult->mtx, pData->mRotate[0], pData->mRotate[1], pData->mRotate[2]);
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
void fn_80099178(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0xEF, lbl_805915F8);
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)fn_8005AAE4(pSelf);
        if (lbl_80795F4C == x && lbl_80795F4C == y && lbl_80795F4C == z) {
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
void fn_80099278(ResHandle* pSelf, f32 x, f32 y, f32 z) {
    if (res_node_is_valid(pSelf) == 0) {
        nw4r::db::Panic(lbl_805915C0, 0x103, lbl_805915F8);
    }
    if (res_node_is_valid(pSelf) != 0) {
        ResNodeData* pData = (ResNodeData*)fn_8005AAE4(pSelf);
        if (lbl_80795F4C == x && lbl_80795F4C == y && lbl_80795F4C == z) {
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
u32 fn_80099378(ResHandle* pSelf) {
    ResNodeData* pData = fn_8005D218(pSelf);
    return fn_800993B4(pSelf, pData->mSubResOfs);
}

/* Build the one-word sub-resource handle at `base + ofs` (0 for a null offset) and return its word. */
u32 fn_800993B4(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        u32 present;
        return *(u32*)fn_8008A220(&present, base + ofs);
    }
    u32 absent;
    return *(u32*)fn_8008A220(&absent, 0);
}

}  /* extern "C" */
