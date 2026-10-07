/*
 * g3d/g3d_calcworld.cpp - nw4r g3d node/matrix world pass (g3d_calc_world, g3d_calc_skinning) and its node-table accessors.
 * RANGE. .text 0x800736F8-0x800746DC (30 functions); extab, extabindex, .data 0x8058E178-0x8058E430 (g3d_calc_world
 *   passes "g3d_calcworld.cpp", lbl_8058E184; the fn_80073E80..world_mtx_attr_root_mtx helpers read lbl_8058E340..lbl_8058E420),
 *   .sdata 0x807911D0-0x807911E0, .sdata2 0x80795DC0-0x80795DC8.  The six accessors res_mdl_info_num_view_mtx..fn_800746D4 carry no
 *   data reference: the candidate cut 0x800746DC puts them here, 0x80074620 is the alternative.
 * NAMES. camera_ctor is a GUESS (the evidence follows).
 *   Map stems.
 *   world_mtx_attr_not_scale_uniform is a GUESS, world_mtx_attr_not_scale_one is a GUESS,
 *   world_mtx_attr_scale_uniform is a GUESS, world_mtx_attr_scale_one is a GUESS and world_mtx_attr_root_mtx is a
 *   GUESS (the world-matrix attribute bit helpers), g3d_calc_world is a GUESS and g3d_calc_skinning is a GUESS (the
 *   node-tree and node-mix byte-code runners), res_mdl_get_info is a GUESS and res_mdl_info_num_view_mtx is a GUESS
 *   (the ResMdlInfo handle and its view-matrix count).
 * RESIDUALS. Unwritten (objdiff scores it zero): g3d_calc_skinning.  Partial: fn_800736F8, g3d_calc_world, fn_80073CE0,
 *   fn_80073D34, fn_80073F00, addVec3To.
 *   flipcheck: `.text` 0xAD0 of 0xFE4; `.data` is claimed and not emitted; `.sdata` is 0x4 of 0x10, `.sdata2` 0x4
 *   of 0x8.
 * SHAPES. `#pragma peephole off` around fn_8007403C keeps retail's masked compare (playbook 32); file-scope
 *   `#pragma fp_contract off`.
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */


#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::math::MTX34Scale (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_8006FDCC..mtx34_copy_ps (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma peephole off

/* The target object contains no fused multiply-add at all while `cflags_g3d` passes
 * `-fp_contract on`, so the original file carried the pragma. File-scoped (see header). */
#pragma fp_contract off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace db


struct ResNodeData;
struct G3DWorkObj;

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)            */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void vec3_add_ps(void* pOut, const void* pIn);
extern "C" s32 res_node_is_valid(const void* p);
extern "C" f32* fn_8005CED0(void);
extern "C" u32* fn_8005CEDC(void);
extern "C" void* res_node_get_id(const void* p);
extern "C" ResNodeData* res_node_ptr(const void* p);
extern "C" void res_node_copy_ctor(void* pOut, const void* pIn);
extern "C" void fn_80061068(void* pOut);
extern "C" void* fn_8008E1C0(void* pOut, const void* pIn);
extern "C" void fn_8008F148(void* pOut, const void* pIn);
extern "C" void fn_80098D5C(void* pA, const void* pB);
extern "C" void fn_80098F6C(void* pA, void* pOut);
extern "C" s32 fn_800D77B0(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);
extern "C" s32 fn_800D7D24(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);

/* The panic file/format strings the target references as map symbols. They are extern here rather
 * than literals: MWCC's `-str reuse` would pool a literal into one blob and address it through a
 * shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058E178[];
extern const char lbl_8058E184[];
extern const char lbl_8058E198[];
extern const char lbl_8058E1E0[];
extern const char lbl_8058E208[];
extern const char lbl_8058E240[];
extern const char lbl_8058E320[];
extern const char lbl_8058E340[];
extern const char lbl_8058E354[];
extern const char lbl_8058E380[];
extern const char lbl_8058E390[];
extern const char lbl_8058E3B8[];
extern const char lbl_8058E3C8[];
extern const char lbl_8058E3F0[];
extern const char lbl_8058E400[];
extern const char lbl_8058E420[];

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The three-word accessor `fn_800736F8` fills in: the value (three floats) and the flag word. */
struct VtxAttrRef {
    /* +0x0 */ u32 mUnused00;
    /* +0x4 */ f32* mpValue;
    /* +0x8 */ u32* mpFlags;
}; /* size: 0xC */

/* The polymorphic node-callback object `fn_80073CE0`/`fn_80073D34`/`fn_80073F00` dispatch through: a
 * vtable at +0x0, a flag byte at +0x4 and the node id at +0x6. */
struct NodeCallback {
    /* +0x0 */ void** mpVtbl;
    /* +0x4 */ u8 mFlags;
    /* +0x5 */ u8 mUnk05;
    /* +0x6 */ u16 mNodeID;
}; /* size: 0x8 */

/* The three-word record `fn_80073DC4` packs for the vtable's `+0x10` slot: the node's matrix, its
 * three-float offset and the matrix id slot. */
struct MtxArg {
    /* +0x0 */ void* mpMtx;
    /* +0x4 */ void* mpVec;
    /* +0x8 */ void* mpMtxID;
}; /* size: 0xC */

/* One node's matrix record (`fn_80073FA0` copies the whole 0x4C): a small header and the 0x30-byte
 * matrix at +0x1C.  Only the field boundaries the copy emits are named. */
struct NodeMtxRec {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ f32 mUnk04[2];
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ f32 mUnk10[2];
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ f32 mMtx[12];
}; /* size: 0x4C */

/* The model's resource data (`fn_800740A8` hands it back); +0x4C is the node table's owning object. */
struct ResMdlData {
    /* +0x00 */ u32 mUnk00[19];
    /* +0x4C */ u32 mNodeTableKey;
}; /* size: 0x50 */

/* The engine's shared node/matrix work object `fn_8006FF50` returns. */
struct G3DWorkObj {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ s32 mNumNode;
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ u32 mUnk10;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ u32 mNumMtx;
    /* +0x20 */ u16 mUnk20;
    /* +0x22 */ u8 mUnk22;
    /* +0x23 */ u8 mEnvelopeMtxMode;
}; /* size: 0x24 */

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

void fn_800736F8(VtxAttrRef* pSelf, f32 x, f32 y, f32 z);
u32 world_mtx_attr_not_scale_uniform(u32 value);
u32 world_mtx_attr_not_scale_one(u32 value);
u32 world_mtx_attr_scale_uniform(u32 value);
u32 world_mtx_attr_scale_one(u32 value);
void g3d_calc_world(u8* pMtxArray, s32* pMtxIDs, u8* pByteCode, const void* pMtx, ResHandle* pMdl,
                 NodeCallback* pNodeCallback, NodeCallback* pNodeCallback2, u32 flags);
void fn_80073CE0(NodeCallback* pSelf, void* pMtx, s32* pMtxID);
void fn_80073D30(void);
void fn_80073D34(NodeCallback* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5);
void fn_80073DC0(void);
MtxArg* fn_80073DC4(MtxArg* pSelf, void* pM, void* pS, void* pAttr);
u32 fn_80073E80(u32 value, u32 low);
u32 fn_80073E8C(void* pSelf);
void fn_80073F00(NodeCallback* pSelf, u32 id, s32* pMtx, s32* pMtxID);
void fn_80073F64(void);
nw4r::math::VEC3* addVec3To(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn);
void fn_80073FA0(NodeMtxRec* pDst, const NodeMtxRec* pSrc);
s32 fn_8007403C(u32 flags);
s32 fn_80074050(const void* p);
s32 res_mdl_get_info(ResHandle* pMdl);
ResMdlData* fn_800740A8(ResHandle* pMdl);
void* fn_8007410C(ResHandle* pSelf);
u32 world_mtx_attr_root_mtx(void);
u32 res_mdl_info_num_view_mtx(const ResHandle* pSelf);
s32 fn_80074644(const ResHandle* pSelf);
void fn_80074698(ResHandle* pSelf, const ResHandle* pRhs);
ResHandle* fn_80074668(ResHandle* pSelf, const ResHandle* pRhs);
void fn_800746D4(ResHandle* pSelf, u32 value);
ResHandle* camera_ctor(ResHandle* pSelf, u32 value);

/* Store the three scale values and pick the scale-mode flag word: 0x40000000 when all three are 1.0f,
 * 0x10000000 then 0x3FFFFFFF when they are equal but not 1.0f, otherwise just clear the mode bits. */
void fn_800736F8(VtxAttrRef* pSelf, f32 x, f32 y, f32 z) {
    pSelf->mpValue[0] = x;
    pSelf->mpValue[1] = y;
    pSelf->mpValue[2] = z;
    if (x == y && x == z) {
        if (1.0f == x) {
            *pSelf->mpFlags = world_mtx_attr_scale_one(*pSelf->mpFlags);
            return;
        }
        u32* pFlags = pSelf->mpFlags;
        *pFlags = world_mtx_attr_scale_uniform(*pFlags);
        *pSelf->mpFlags = world_mtx_attr_not_scale_one(*pSelf->mpFlags);
        return;
    }
    *pSelf->mpFlags = world_mtx_attr_not_scale_uniform(*pSelf->mpFlags);
}

u32 world_mtx_attr_not_scale_uniform(u32 value) {
    return value & 0x0FFFFFFF;
}

u32 world_mtx_attr_not_scale_one(u32 value) {
    return value & 0x3FFFFFFF;
}

u32 world_mtx_attr_scale_uniform(u32 value) {
    return value | 0x10000000;
}

u32 world_mtx_attr_scale_one(u32 value) {
    return value | 0x40000000;
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-callback helpers                                                      */
/* ------------------------------------------------------------------------------------------------ */

/* Walk a `ResByteCodeData` node tree: build each node's matrix record, compute its world matrix
 * (`fn_800D77B0` / `fn_800D7D24`), then scale the matrices the byte code marked. */
void g3d_calc_world(u8* pMtxArray, s32* pMtxIDs, u8* pByteCode, const void* pMtx, ResHandle* pMdl,
                 NodeCallback* pNodeCallback, NodeCallback* pNodeCallback2, u32 flags) {
    s32 mayaDisable;
    u8 singleMtx;
    NodeMtxRec rec;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    s32 s1C;
    s32 s18;
    s32 s14;
    s32 s10;
    s32 sC;
    s32 s8;
    u8* pCode = pByteCode;
    fn_80061068(&rec);
    if (pCode == NULL) {
        pCode = (u8*)(void*)reinterpret_cast<const nw4r::g3d::ResMdl*>(pMdl)->GetResByteCode(lbl_8058E178);
    }
    if (pCode != NULL) {
        NodeMtxRec* pRec = NULL;
        s18 = res_mdl_get_info(pMdl);
        singleMtx = (fn_80074050(&s18) == 1);
        mayaDisable = fn_8007403C(flags);
        f32* pScale = fn_8005CED0();
        pScale[0] = 1.0f;
        pScale[1] = 1.0f;
        pScale[2] = 1.0f;
        mtx34_copy_ps((Mtx34*)pMtxArray, (const Mtx34*)pMtx);
        u32* pMtxIDList = fn_8005CEDC();
        u32 numMtx = 0;
        *pMtxIDs = (s32)flags;
        for (;;) {
            if (pCode[0] == 1) {
                break;
            }
            switch (pCode[0]) {
            case 2: {
                u32 nodeID = (pCode[1] << 8) + pCode[2];
                u32 targetID = (pCode[3] << 8) + pCode[4];
                s14 = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(pMdl)->GetResNode(nodeID).mpData;
                res_node_copy_ctor(&s1C, &s14);
                u32 mtxID = fn_8006FDCC(&s1C);
                if (numMtx >= 0x800) {
                    nw4r::db::Panic(lbl_8058E184, 137, lbl_8058E198);
                }
                pMtxIDList[numMtx++] = mtxID;
                u8* pDstMtx = pMtxArray + mtxID * 0x30;
                f32* pDstScale = pScale + mtxID * 3;
                u8* pSrcMtx = pMtxArray + targetID * 0x30;
                f32* pSrcScale = pScale + targetID * 3;
                s32 prev = pMtxIDs[targetID];
                if (pNodeCallback != NULL) {
                    void* (*pGetRec)(void*, NodeMtxRec*, void*) =
                        (void* (*)(void*, NodeMtxRec*, void*))pNodeCallback->mpVtbl[14];
                    pRec = (NodeMtxRec*)pGetRec(pNodeCallback, &rec, res_node_get_id(&s1C));
                }
                s32 reset;
                if (pNodeCallback == NULL || pRec->mUnk00 == 0) {
                    fn_80098F6C(&s1C, &rec);
                    pRec = &rec;
                    reset = 1;
                } else {
                    reset = 0;
                }
                if (nodeID != 0 && mayaDisable != 0 && reset == 0) {
                    VEC3_ctor(&vA);
                    VEC3_ctor(&vB);
                    if (pRec != &rec) {
                        fn_80073FA0(&rec, pRec);
                        pRec = &rec;
                    }
                    fn_8008E1C0(&rec, &vA);
                    rec.mUnk00 |= 0x200;
                    fn_80098D5C(&s1C, &rec);
                    fn_8008E1C0(&rec, &vB);
                    addVec3To(&vB, &vA);
                    fn_8008F148(&rec, &vB);
                }
                if (pNodeCallback2 != NULL) {
                    if (pRec != &rec) {
                        fn_80073FA0(&rec, pRec);
                        pRec = &rec;
                    }
                    s10 = *(s32*)pMdl;
                    fn_80073F00(pNodeCallback2, nodeID, (s32*)&rec, &s10);
                }
                if (singleMtx != 0) {
                    pMtxIDs[mtxID] =
                        fn_800D77B0(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                } else if ((pRec->mUnk00 & 0x400) != 0) {
                    nw4r::db::Panic(lbl_8058E184, 245, lbl_8058E1E0);
                    pMtxIDs[mtxID] =
                        fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                } else {
                    pMtxIDs[mtxID] =
                        fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
                }
                pMtxIDs[mtxID] = fn_80073E80(pMtxIDs[mtxID], fn_80073E8C(&s1C));
                if (pNodeCallback2 != NULL) {
                    sC = *(s32*)pMdl;
                    fn_80073D34(pNodeCallback2, nodeID, pDstMtx, pDstScale, &pMtxIDs[mtxID], &sC);
                }
                pCode += 5;
                break;
            }
            default:
                nw4r::db::Panic(lbl_8058E184, 273, lbl_8058E208);
                /* fallthrough */
            case 6: {
                u32 nodeID = (pCode[1] << 8) + pCode[2];
                u32 targetID = (pCode[3] << 8) + pCode[4];
                pMtxIDList[numMtx++] = nodeID;
                pMtxIDs[nodeID] = pMtxIDs[targetID];
                mtx34_copy_ps((Mtx34*)(pMtxArray + nodeID * 0x30), (const Mtx34*)(pMtxArray + targetID * 0x30));
                /* `pScale` is the frame's flat float array (`fn_8005CED0`), so the record is
                 * reached by index; the helper's own type is the real one (rule 11). */
                copyVec3((nw4r::math::VEC3*)(pScale + nodeID * 3),
                         (const nw4r::math::VEC3*)(pScale + targetID * 3));
                pCode += 5;
                break;
            }
            }
        }
        for (u32 i = 0; i < numMtx; i++) {
            u32 mtxID = pMtxIDList[i];
            if (mtxID >= 0x800) {
                nw4r::db::Panic(lbl_8058E184, 294, lbl_8058E240);
            }
            f32* pS = pScale + mtxID * 3;
            if (1.0f != pS[0] || 1.0f != pS[1] || 1.0f != pS[2]) {
                nw4r::math::MTX34Scale((nw4r::math::MTX34*)(pMtxArray + mtxID * 0x30),
                                       (const nw4r::math::MTX34*)(pMtxArray + mtxID * 0x30),
                                       (const nw4r::math::VEC3*)pS);
            }
        }
        if (pNodeCallback2 != NULL) {
            s8 = *(s32*)pMdl;
            fn_80073CE0(pNodeCallback2, pMtxArray, &s8);
        }
    }
}

/* Notify the callback object's `+0x14` vtable slot that one node's matrix is ready. */
void fn_80073CE0(NodeCallback* pSelf, void* pMtx, s32* pMtxID) {
    if ((pSelf->mFlags & 4) != 0) {
        s32 mtxID = *pMtxID;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[5];
        pNotify(pSelf->mpVtbl, pMtx, &mtxID, pSelf);
    }
}

void fn_80073D30(void) {
}

/* Notify the callback object's `+0x10` vtable slot for the matching node id. */
void fn_80073D34(NodeCallback* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5) {
    if (id == pSelf->mNodeID && (pSelf->mFlags & 2) != 0) {
        MtxArg arg;
        fn_80073DC4(&arg, pMtx, pVec, pMtxID);
        s32 mtxID = *pArg5;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[4];
        pNotify(pSelf->mpVtbl, &arg, &mtxID, pSelf);
    }
}

void fn_80073DC0(void) {
}

MtxArg* fn_80073DC4(MtxArg* pSelf, void* pM, void* pS, void* pAttr) {
    pSelf->mpMtx = pM;
    pSelf->mpVec = pS;
    pSelf->mpMtxID = pAttr;
    if (pM == NULL) {
        nw4r::db::Panic(lbl_8058E3F0, 46, lbl_8058E3C8);
    }
    if (pS == NULL) {
        nw4r::db::Panic(lbl_8058E3B8, 47, lbl_8058E390);
    }
    if (pAttr == NULL) {
        nw4r::db::Panic(lbl_8058E380, 48, lbl_8058E354);
    }
    return pSelf;
}

u32 fn_80073E80(u32 value, u32 low) {
    return (value & 0xFFFFFF00) | low;
}

u32 fn_80073E8C(void* pSelf) {
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic(lbl_8058E340, 90, lbl_8058E320);
    }
    if (res_node_is_valid(pSelf)) {
        return ((ResNodeData*)res_node_ptr(pSelf))->mMtxID;
    }
    return 0;
}

/* Notify the callback object's `+0xC` vtable slot for the matching node id. */
void fn_80073F00(NodeCallback* pSelf, u32 id, s32* pMtx, s32* pMtxID) {
    if (id == pSelf->mNodeID && (pSelf->mFlags & 1) != 0) {
        s32 mtxID = *pMtxID;
        void (*pNotify)(void*, void*, s32*, void*) = (void (*)(void*, void*, s32*, void*))pSelf->mpVtbl[3];
        pNotify(pSelf->mpVtbl, pMtx, &mtxID, pSelf);
    }
}

void fn_80073F64(void) {
}

nw4r::math::VEC3* addVec3To(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn) {
    vec3_add_ps(pOut, pIn);
    return pOut;
}

/* Copy one node's 0x4C-byte matrix record. */
void fn_80073FA0(NodeMtxRec* pDst, const NodeMtxRec* pSrc) {
    *pDst = *pSrc;
}

/* `peephole off` keeps retail's generic `!= 0` conversion (`rlwinm r3,r3,0,4,4; neg; or; srwi`), which `-O3`
 * folds into one `rlwinm r3,r3,5,31,31` (playbook 32). */
s32 fn_8007403C(u32 flags) {
    return (flags & 0x08000000) != 0;
}

s32 fn_80074050(const void* p) {
    return fn_8006FF50()->mNumNode;
}

/* Look one entry up in the model's node table (`fn_800740A8` hands back the model data, +0x4C is the
 * table's owning object). */
s32 res_mdl_get_info(ResHandle* pMdl) {
    ResMdlData* pData = fn_800740A8(pMdl);
    s32 idx;
    return *fn_80070054(&idx, &pData->mNodeTableKey);
}

/* The `ResMdl` root, with the `g3d_resmdl_ac.h` validity assert. */
ResMdlData* fn_800740A8(ResHandle* pMdl) {
    if (!reinterpret_cast<const nw4r::g3d::ResMdl*>(pMdl)->IsValid()) {
        nw4r::db::Panic(lbl_8058E420, 120, lbl_8058E400, nw4r::g3d::ResMdl::GetClassName(), "ref");
    }
    return (ResMdlData*)fn_8007410C(pMdl);
}

void* fn_8007410C(ResHandle* pSelf) {
    return pSelf->mpData;
}

u32 world_mtx_attr_root_mtx(void) {
    return 0xF0000000;
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-table accessors                                                       */
/* ------------------------------------------------------------------------------------------------ */

u32 res_mdl_info_num_view_mtx(const ResHandle* pSelf) {
    return fn_8006FF50()->mNumMtx;
}

s32 fn_80074644(const ResHandle* pSelf) {
    return fn_8006FF50()->mEnvelopeMtxMode;
}

void fn_80074698(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

ResHandle* fn_80074668(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_80074698(pSelf, pRhs);
    return pSelf;
}

void fn_800746D4(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

ResHandle* camera_ctor(ResHandle* pSelf, u32 value) {
    fn_800746D4(pSelf, value);
    return pSelf;
}

} /* extern "C" */
