/*
 * g3d/g3d_calcworld.cpp - nw4r g3d node/matrix world pass (g3d_calc_world, g3d_calc_skinning) and its node-table accessors.
 * RANGE. .text 0x800736F8-0x800746DC (30 functions); extab, extabindex, .data 0x8058E178-0x8058E430 (g3d_calc_world
 *   passes "g3d_calcworld.cpp"; the inline helpers pass "g3d_resnode_ac.h".."g3d_resmdl_ac.h"),
 *   .sdata 0x807911D0-0x807911E0, .sdata2 0x80795DC0-0x80795DC8.  The six accessors res_mdl_info_num_view_mtx..fn_800746D4 carry no
 *   data reference: the candidate cut 0x800746DC puts them here, 0x80074620 is the alternative.
 * NAMES. camera_ctor is a GUESS (the evidence follows).
 *   Map stems.
 *   world_mtx_attr_not_scale_uniform is a GUESS, world_mtx_attr_not_scale_one is a GUESS,
 *   world_mtx_attr_scale_uniform is a GUESS, world_mtx_attr_scale_one is a GUESS and world_mtx_attr_root_mtx is a
 *   GUESS (the world-matrix attribute bit helpers), g3d_calc_world is a GUESS and g3d_calc_skinning is a GUESS (the
 *   node-tree and node-mix byte-code runners), res_mdl_get_info is a GUESS and res_mdl_info_num_view_mtx is a GUESS
 *   (the ResMdlInfo handle and its view-matrix count).  ResMdl::ref/ptr (0x800740A8/0x8007410C, the non-const
 *   accessors), ICalcWorldCallback::ExecCallbackC/B/A (0x80073D30/0x80073DC0/0x80073F64, the empty default
 *   callbacks) and NodeMtxRec's implicit assignment (0x80073FA0) are the compiler's names for the weak copies.
 * RESIDUALS. The map still spells 0x80073D30, 0x80073DC0, 0x80073F64, 0x80073FA0, 0x800740A8 and 0x8007410C by their
 *   stems; every row is 100 % once they carry the manglings above (other units' vtables reference the three
 *   callbacks, so the flip needs the renames first).
 *   NodeMtxRec is a view of nw4r::g3d::ChrAnmResult (g3d/g3d_anmchr.h) whose flat fields give retail's memberwise copy.
 *   g3d_calc_skinning: the paired-single accumulators are register variables used by inline `asm` blocks (retail's
 *   f31-f26).
 * SHAPES. fn_80073CE0/fn_80073D34/fn_80073F00 call ICalcWorldCallback's ExecCallbackC/B/A virtually (the canonical
 *   `r12` dispatch) with the model handle by value; g3d_calc_world calls AnmObjChr::GetResult virtually.
 *   The helpers between g3d_calc_world and g3d_calc_skinning are inline functions defined ahead of g3d_calc_world under
 *   `#pragma dont_inline on`, and g3d_calc_world is compiled under it too: each weak copy lands after its first caller
 *   in reverse first-use order (the empty ExecCallback defaults after the dispatcher that calls them virtually), and
 *   their strings form retail's weak tail in reverse definition order (ResMdl::ref, the manipulator constructor,
 *   ResNode's matrix-id getter); ResMdl::ref passes `__FUNCTION__` (the "ref" `.sdata` object).
 *   g3d_calc_world: the byte code is a `while` over the opcode with an `if`/`else` (the duplicate opcode asserted),
 *   the byte-code pointer is tested once more only on the fetch path (an early return), the node's matrix id is
 *   updated through a reference, the scale vectors are indexed as `VEC3`s, the singleness flag is a `bool`.
 *   `#pragma peephole off` around fn_8007403C keeps retail's masked compare (playbook 32); file-scope
 *   `#pragma fp_contract off`.
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */


#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_anmchr.h" /* res_node_ref_nonconst (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::math::MTX34Scale (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_8006FDCC..mtx34_copy_ps (rule 2) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ICalcWorldCallback, FuncObjCalcWorld (rule 1) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma pool_data off

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

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)            */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void vec3_add_ps(void* pOut, const void* pA, const void* pB);
extern "C" s32 res_node_is_valid(const void* p);
extern "C" f32* fn_8005CED0(void);
extern "C" u32* fn_8005CEDC(void);
extern "C" ResNodeData* res_node_ptr(const void* p);
extern "C" void chr_anm_result_init(void* pOut);
extern "C" void* fn_8008E1C0(void* pOut, const void* pIn);
extern "C" void fn_8008F148(void* pOut, const void* pIn);
extern "C" void fn_80098D5C(void* pA, const void* pB);
extern "C" void fn_80098F6C(void* pA, void* pOut);
extern "C" s32 fn_800D77B0(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);
extern "C" s32 fn_800D7D24(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The three-word accessor `fn_800736F8` fills in: the value (three floats) and the flag word. */
struct VtxAttrRef {
    /* +0x0 */ u32 mUnused00;
    /* +0x4 */ f32* mpValue;
    /* +0x8 */ u32* mpFlags;
}; /* size: 0xC */

namespace nw4r {
namespace g3d {

/* The node matrix, scale and attribute word a world callback's second slot may edit (fn_80073DC4 builds it and
 * asserts each pointer).  size: 0xC */
struct WorldMtxManip {
    /* +0x0 */ math::MTX34* mpM;
    /* +0x4 */ math::VEC3* mpS;
    /* +0x8 */ u32* mpAttr;
};

}  // namespace g3d
}  // namespace nw4r

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

/* The model's resource data (`ResMdl::ref` hands it back); +0x4C is the node table's owning object. */
struct ResMdlData {
    /* +0x00 */ u32 mUnk00[19];
    /* +0x4C */ u32 mNodeTableKey;
}; /* size: 0x50 */


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
                 nw4r::g3d::AnmObjChr* pNodeCallback, nw4r::g3d::FuncObjCalcWorld* pNodeCallback2, u32 flags);
inline void fn_80073CE0(nw4r::g3d::FuncObjCalcWorld* pSelf, void* pMtx, s32* pMtxID);
inline void fn_80073D34(nw4r::g3d::FuncObjCalcWorld* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5);
inline nw4r::g3d::WorldMtxManip* fn_80073DC4(nw4r::g3d::WorldMtxManip* pSelf, void* pM, void* pS, void* pAttr);
inline u32 fn_80073E80(u32 value, u32 low);
inline u32 fn_80073E8C(void* pSelf);
inline void fn_80073F00(nw4r::g3d::FuncObjCalcWorld* pSelf, u32 id, s32* pMtx, s32* pMtxID);
inline nw4r::math::VEC3* addVec3To(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn);
inline s32 fn_8007403C(u32 flags);
inline s32 fn_80074050(const void* p);
inline s32 res_mdl_get_info(ResHandle* pMdl);
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
        *pSelf->mpFlags = world_mtx_attr_scale_uniform(*pSelf->mpFlags);
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

/* The helpers retail emits as weak copies after g3d_calc_world, defined ahead of it and kept out of line: each
 * copy lands after its first caller, and their strings come out in reverse definition order after the TU's own. */
#pragma push
#pragma dont_inline on

/* The `ResMdl` root, with the `g3d_resmdl_ac.h` validity assert. */
inline nw4r::g3d::ResMdlData& nw4r::g3d::ResMdl::ref() {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmdl_ac.h", 120, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    }
    return *ptr();
}

inline nw4r::g3d::WorldMtxManip* fn_80073DC4(nw4r::g3d::WorldMtxManip* pSelf, void* pM, void* pS, void* pAttr) {
    pSelf->mpM = (nw4r::math::MTX34*)pM;
    pSelf->mpS = (nw4r::math::VEC3*)pS;
    pSelf->mpAttr = (u32*)pAttr;
    if (pM == NULL) {
        nw4r::db::Panic("g3d_calcworld.h", 46, "NW4R:Pointer must not be NULL (m)");
    }
    if (pS == NULL) {
        nw4r::db::Panic("g3d_calcworld.h", 47, "NW4R:Pointer must not be NULL (s)");
    }
    if (pAttr == NULL) {
        nw4r::db::Panic("g3d_calcworld.h", 48, "NW4R:Pointer must not be NULL (attr)");
    }
    return pSelf;
}

inline u32 fn_80073E8C(void* pSelf) {
    if (!res_node_is_valid(pSelf)) {
        nw4r::db::Panic("g3d_resnode_ac.h", 90, "NW4R:Failed assertion IsValid()");
    }
    if (res_node_is_valid(pSelf)) {
        return ((ResNodeData*)res_node_ptr(pSelf))->mMtxID;
    }
    return 0;
}

/* Notify the callback object's `+0x14` vtable slot that one node's matrix is ready. */
inline void fn_80073CE0(nw4r::g3d::FuncObjCalcWorld* pSelf, void* pMtx, s32* pMtxID) {
    if ((pSelf->mTiming & 4) != 0) {
        pSelf->mpCallback->ExecCallbackC((nw4r::math::MTX34*)pMtx, *(nw4r::g3d::ResMdl*)pMtxID, pSelf);
    }
}

/* Notify the callback object's `+0x10` vtable slot for the matching node id. */
inline void fn_80073D34(nw4r::g3d::FuncObjCalcWorld* pSelf, u32 id, void* pMtx, void* pVec, s32* pMtxID, s32* pArg5) {
    if (id == pSelf->mNodeID && (pSelf->mTiming & 2) != 0) {
        nw4r::g3d::WorldMtxManip arg;
        fn_80073DC4(&arg, pMtx, pVec, pMtxID);
        pSelf->mpCallback->ExecCallbackB(&arg, *(nw4r::g3d::ResMdl*)pArg5, pSelf);
    }
}

inline u32 fn_80073E80(u32 value, u32 low) {
    return (value & 0xFFFFFF00) | low;
}

/* Notify the callback object's `+0xC` vtable slot for the matching node id. */
inline void fn_80073F00(nw4r::g3d::FuncObjCalcWorld* pSelf, u32 id, s32* pMtx, s32* pMtxID) {
    if (id == pSelf->mNodeID && (pSelf->mTiming & 1) != 0) {
        pSelf->mpCallback->ExecCallbackA((nw4r::g3d::ChrAnmResult*)pMtx, *(nw4r::g3d::ResMdl*)pMtxID, pSelf);
    }
}

inline nw4r::math::VEC3* addVec3To(nw4r::math::VEC3* pOut, const nw4r::math::VEC3* pIn) {
    vec3_add_ps(pOut, pOut, pIn);
    return pOut;
}

/* `peephole off` keeps retail's generic `!= 0` conversion (`rlwinm r3,r3,0,4,4; neg; or; srwi`), which `-O3`
 * folds into one `rlwinm r3,r3,5,31,31` (playbook 32). */
inline s32 fn_8007403C(u32 flags) {
    return (flags & 0x08000000) != 0;
}

inline s32 fn_80074050(const void* p) {
    return res_mdl_info_data(p)->mNumNode;
}

/* Look one entry up in the model's node table (`ResMdl::ref` hands back the model data, +0x4C is the
 * table's owning object). */
inline s32 res_mdl_get_info(ResHandle* pMdl) {
    ResMdlData* pData = (ResMdlData*)&reinterpret_cast<nw4r::g3d::ResMdl*>(pMdl)->ref();
    s32 idx;
    return *fn_80070054(&idx, &pData->mNodeTableKey);
}

inline nw4r::g3d::ResMdlData* nw4r::g3d::ResMdl::ptr() {
    return mpData;
}

} /* extern "C" */

/* ICalcWorldCallback's default callbacks: empty, emitted after the dispatchers that call them virtually. */
inline void nw4r::g3d::ICalcWorldCallback::ExecCallbackA(ChrAnmResult*, ResMdl, FuncObjCalcWorld*) {
}

inline void nw4r::g3d::ICalcWorldCallback::ExecCallbackB(WorldMtxManip*, ResMdl, FuncObjCalcWorld*) {
}

inline void nw4r::g3d::ICalcWorldCallback::ExecCallbackC(math::MTX34*, ResMdl, FuncObjCalcWorld*) {
}

extern "C" {

#pragma pop

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-callback helpers                                                      */
/* ------------------------------------------------------------------------------------------------ */

#pragma push
#pragma dont_inline on
/* Walk a `ResByteCodeData` node tree: build each node's matrix record, compute its world matrix
 * (`fn_800D77B0` / `fn_800D7D24`), then scale the matrices the byte code marked. */
void g3d_calc_world(u8* pMtxArray, s32* pMtxIDs, u8* pByteCode, const void* pMtx, ResHandle* pMdl,
                 nw4r::g3d::AnmObjChr* pNodeCallback, nw4r::g3d::FuncObjCalcWorld* pNodeCallback2, u32 flags) {
    s32 mayaDisable;
    bool singleMtx;
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
    chr_anm_result_init(&rec);
    NodeMtxRec* pRec = NULL;
    if (pCode == NULL) {
        pCode = (u8*)(void*)reinterpret_cast<const nw4r::g3d::ResMdl*>(pMdl)->GetResByteCode("NodeTree");
        if (pCode == NULL) {
            return;
        }
    }
    s18 = res_mdl_get_info(pMdl);
    singleMtx = (fn_80074050(&s18) == 1);
    mayaDisable = fn_8007403C(flags);
    f32* pScale = fn_8005CED0();
    pScale[0] = pScale[1] = pScale[2] = 1.0f;
    mtx34_copy_ps((Mtx34*)pMtxArray, (const Mtx34*)pMtx);
    u32* pMtxIDList = fn_8005CEDC();
    u32 numMtx = 0;
    *pMtxIDs = (s32)flags;
    u8 code;
    while ((code = pCode[0]) != 1) {
        if (code == 2) {
            u32 nodeID = (pCode[1] << 8) + pCode[2];
            u32 targetID = (pCode[3] << 8) + pCode[4];
            s14 = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(pMdl)->GetResNode(nodeID).mpData;
            res_node_copy_ctor(&s1C, (u32*)&s14);
            u32 mtxID = fn_8006FDCC(&s1C);
            if (numMtx >= 0x800) {
                nw4r::db::Panic("g3d_calcworld.cpp", 137, "NW4R:Failed assertion numMtxID < detail::workmem::WORKMEM_NUMMTXID");
            }
            pMtxIDList[numMtx++] = mtxID;
            u8* pDstMtx = pMtxArray + mtxID * 0x30;
            f32* pDstScale = pScale + mtxID * 3;
            u8* pSrcMtx = pMtxArray + targetID * 0x30;
            f32* pSrcScale = pScale + targetID * 3;
            s32 prev = pMtxIDs[targetID];
            if (pNodeCallback != NULL) {
                pRec = (NodeMtxRec*)pNodeCallback->GetResult((nw4r::g3d::ChrAnmResult*)&rec,
                                                             res_node_get_id((const ResHandle*)&s1C));
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
                    rec = *pRec;
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
                    rec = *pRec;
                    pRec = &rec;
                }
                s10 = *(s32*)pMdl;
                fn_80073F00(pNodeCallback2, nodeID, (s32*)&rec, &s10);
            }
            if (singleMtx != 0) {
                pMtxIDs[mtxID] =
                    fn_800D77B0(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
            } else if ((pRec->mUnk00 & 0x400) != 0) {
                nw4r::db::Panic("g3d_calcworld.cpp", 245, "NW4R_G3D_MAYA_DISABLE must be disabled.");
                pMtxIDs[mtxID] =
                    fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
            } else {
                pMtxIDs[mtxID] =
                    fn_800D7D24(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec);
            }
            s32& id = pMtxIDs[mtxID];
            id = fn_80073E80(id, fn_80073E8C(&s1C));
            if (pNodeCallback2 != NULL) {
                sC = *(s32*)pMdl;
                fn_80073D34(pNodeCallback2, nodeID, pDstMtx, pDstScale, &id, &sC);
            }
            pCode += 5;
        } else {
            if (code != 6) {
                nw4r::db::Panic("g3d_calcworld.cpp", 273, "NW4R:Failed assertion c == ResByteCodeData::MTXDUP");
            }
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
        }
    }
    for (u32 i = 0; i < numMtx; i++) {
        u32 mtxID = pMtxIDList[i];
        if (mtxID >= 0x800) {
            nw4r::db::Panic("g3d_calcworld.cpp", 294, "NW4R:Failed assertion mtxID < detail::workmem::WORKMEM_NUMMTXID");
        }
        if (1.0f != ((const nw4r::math::VEC3*)pScale)[mtxID].x || 1.0f != ((const nw4r::math::VEC3*)pScale)[mtxID].y ||
            1.0f != ((const nw4r::math::VEC3*)pScale)[mtxID].z) {
            nw4r::math::MTX34Scale((nw4r::math::MTX34*)(pMtxArray + mtxID * 0x30),
                                   (const nw4r::math::MTX34*)(pMtxArray + mtxID * 0x30),
                                   &((const nw4r::math::VEC3*)pScale)[mtxID]);
        }
    }
    if (pNodeCallback2 != NULL) {
        s8 = *(s32*)pMdl;
        fn_80073CE0(pNodeCallback2, pMtxArray, &s8);
    }
}
#pragma pop


u32 world_mtx_attr_root_mtx(void) {
    return 0xF0000000;
}

/* A blend weight: the byte code's big-endian bit pattern read back as a float.  size: 0x4 */
union SkinningWeight {
    /* +0x0 */ u32 bits;
    /* +0x0 */ f32 value;
};

/* 0x8007411C (0x504): runs the model's node-mix byte code: each skinned matrix the weighted sum of the node matrices
 * in model space (paired-single accumulators), each envelope matrix its node matrix times the node's inverse bind
 * matrix; then the scale attributes of the envelope matrices by the model's envelope mode. */
void g3d_calc_skinning(nw4r::math::MTX34* pWorldMtxArray, u32* pWorldMtxAttribArray, const nw4r::g3d::ResMdl* pMdl,
                       const u8* pByteCode) {
    SkinningWeight weight;
    ResHandle infoCopy;
    ResHandle node;
    ResHandle info;
    if (pByteCode == NULL) {
        pByteCode = (const u8*)pMdl->GetResByteCode("NodeMix");
        if (pByteCode == NULL) {
            return;
        }
    }
    nw4r::math::MTX34* pModelMtx = (nw4r::math::MTX34*)g3d_workmem_skinning_mtx();
    while (pByteCode[0] != 1) {
        if (pByteCode[0] == 3) {
            u32 mtxID = (pByteCode[1] << 8) + pByteCode[2];
            u32 numBlend = pByteCode[3];
            pWorldMtxAttribArray[mtxID] = world_mtx_attr_root_mtx();
            pByteCode += 4;
            register f32 acc0 = 0.0f;
            register f32 acc1;
            register f32 acc2;
            register f32 acc3;
            register f32 acc4;
            register f32 acc5;
            asm {
                ps_merge00 acc0, acc0, acc0
                ps_merge00 acc1, acc0, acc0
                ps_merge00 acc2, acc0, acc0
                ps_merge00 acc3, acc0, acc0
                ps_merge00 acc4, acc0, acc0
                ps_merge00 acc5, acc0, acc0
            }
            for (u32 i = 0; i < numBlend; i++) {
                u32 nodeID = (pByteCode[0] << 8) + pByteCode[1];
                if (nodeID >= 2048) {
                    nw4r::db::Panic("g3d_calcworld.cpp", 0x18C,
                                    "NW4R:Failed assertion mtxID < detail::workmem::WORKMEM_NUMSKINNINGMTX");
                }
                weight.bits = (pByteCode[5] | (pByteCode[4] << 8)) | ((pByteCode[2] << 24) | (pByteCode[3] << 16));
                register f32 w = weight.value;
                register const nw4r::math::MTX34* pSrc = &pModelMtx[nodeID];
                asm {
                    psq_l f1, 0(pSrc), 0, 0
                    psq_l f2, 8(pSrc), 0, 0
                    psq_l f3, 16(pSrc), 0, 0
                    psq_l f4, 24(pSrc), 0, 0
                    psq_l f5, 32(pSrc), 0, 0
                    psq_l f6, 40(pSrc), 0, 0
                    ps_madds0 acc0, f1, w, acc0
                    ps_madds0 acc1, f2, w, acc1
                    ps_madds0 acc2, f3, w, acc2
                    ps_madds0 acc3, f4, w, acc3
                    ps_madds0 acc4, f5, w, acc4
                    ps_madds0 acc5, f6, w, acc5
                }
                pWorldMtxAttribArray[mtxID] &= pWorldMtxAttribArray[nodeID];
                pByteCode += 6;
            }
            register nw4r::math::MTX34* pDst = &pWorldMtxArray[mtxID];
            asm {
                psq_st acc0, 0(pDst), 0, 0
                psq_st acc1, 8(pDst), 0, 0
                psq_st acc2, 16(pDst), 0, 0
                psq_st acc3, 24(pDst), 0, 0
                psq_st acc4, 32(pDst), 0, 0
                psq_st acc5, 40(pDst), 0, 0
            }
        } else {
            if (pByteCode[0] != 5) {
                nw4r::db::Panic("g3d_calcworld.cpp", 0x1C9, "NW4R:Failed assertion c == ResByteCodeData::EVPMTX");
            }
            u32 mtxID = (pByteCode[1] << 8) + pByteCode[2];
            u32 nodeID = (pByteCode[3] << 8) + pByteCode[4];
            if (mtxID >= 2048) {
                nw4r::db::Panic("g3d_calcworld.cpp", 0x1CD,
                                "NW4R:Failed assertion mtxID < detail::workmem::WORKMEM_NUMSKINNINGMTX");
            }
            node.mpData = pMdl->GetResNode(nodeID).mpData;
            mtx34_concat(&pModelMtx[mtxID], &pWorldMtxArray[mtxID],
                         (const Mtx34*)&res_node_ref_nonconst(&node)->mInvModelMtx);
            pByteCode += 5;
        }
    }
    info.mpData = (void*)res_mdl_info_handle((void*)pMdl);
    fn_80074668(&infoCopy, &info);
    switch (fn_80074644(&infoCopy)) {
    case 0:
        return;
    case 1: {
        u32 numEnvMtx = res_mdl_info_num_pos_nrm_mtx(&infoCopy) - pMdl->GetResNodeNumEntries();
        u32 numMtx = res_mdl_info_num_view_mtx(&infoCopy);
        for (u32 i = numMtx - numEnvMtx; i < numMtx; i++) {
            pWorldMtxAttribArray[i] |= 0x30000000;
        }
        break;
    }
    case 2: {
        u32 numEnvMtx = res_mdl_info_num_pos_nrm_mtx(&infoCopy) - pMdl->GetResNodeNumEntries();
        u32 numMtx = res_mdl_info_num_view_mtx(&infoCopy);
        for (u32 i = numMtx - numEnvMtx; i < numMtx; i++) {
            pWorldMtxAttribArray[i] &= ~0x20000000;
        }
        break;
    }
    default:
        nw4r::db::Panic("g3d_calcworld.cpp", 0x1F9, "EnvelopeMatrixMode is Invalid.\n");
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* g3d_calcworld.cpp: the node-table accessors                                                       */
/* ------------------------------------------------------------------------------------------------ */

u32 res_mdl_info_num_view_mtx(const ResHandle* pSelf) {
    return res_mdl_info_data(pSelf)->mNumMtx;
}

s32 fn_80074644(const ResHandle* pSelf) {
    return res_mdl_info_data(pSelf)->mEnvelopeMtxMode;
}

ResHandle* fn_80074668(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_80074698(pSelf, pRhs);
    return pSelf;
}

void fn_80074698(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

ResHandle* camera_ctor(ResHandle* pSelf, u32 value) {
    fn_800746D4(pSelf, value);
    return pSelf;
}

void fn_800746D4(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

} /* extern "C" */
