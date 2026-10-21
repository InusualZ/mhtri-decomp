/* g3d/g3d_calcvtx - nw4r g3d vertex calculation: the shape-blend driver fn_8007270C (three blend passes over the
 *   ResVtxPos/ResVtxNrm/ResVtxClr arrays) and the `g3d_resvtx_ac.h` accessor copies this TU keeps.
 * RANGE. .text 0x8007270C-0x800736F8 (35 functions); extab, extabindex, .data 0x8058DD68-0x8058E178 (opens on
 *   "g3d_calcvtx.cpp", fn_8007270C's assert), .sdata 0x807911B8-0x807911D0, .sdata2 0x80795DB8-0x80795DC0.
 * FLAGS. cflags_g3d (docs/g3d.md); file-scope `#pragma fp_contract off` (retail has no `fmadds`/`fmsubs`),
 *   `#pragma pool_data off` (every string gets its own `lis`/`addi`).
 * NAMES. The accessors are nw4r's (`g3d_resvtx.h`); the `const T*` constructors stand in for the out-of-line
 *   copy constructions retail emits (a real copy constructor would change the classes' return ABI); the
 *   ShpAnmVtxSet/ShpAnmKeyShape/ShpAnmResult constructors are GUESSES (the 0x218-byte shape-blend result);
 *   CopyResVtxClrHandle is a GUESS and CopyResVtxNrmHandle is a GUESS (the out-of-line handle copies the driver
 *   calls; the dump's GXInitLightColor there is a linker-folded duplicate).
 * RESIDUALS. fn_8007270C: register colouring only - the destination pointer and the key-collection counter swap
 *   r15/r16 (r16/r17 in the normal pass), and the weight compares load into f2 where retail uses f0; the blends are
 *   inline `asm` blocks of paired singles in the vertex loops (MWCC unrolls the key loop four times, as retail).
 *   .data/.sdata: the driver's strings are literals in first-use order, but the `_ac.h` accessor strings are not the inline tail yet (retail: weak `@STRING@` copies, one per inline
 *   instance, after the TU's own strings, playbook 80):
 *   the NW4R_G3D_RESOURCE_* copies are out-of-line definitions emitted ahead of the driver's.
 *   The `ResCommon<T>` copy helpers (CopyResVtxClrHandle, CopyResVtxNrmHandle, fn_80069798) keep their stems.
 * SHAPES. the ShpAnm* constructors are complete: their work is the members' default constructions.
 *   fn_8007270C: the model handle is used through the parameter (spilled to the frame's bottom slot), the
 *   GetResVtxPos assert compares a temporary handle, each pass's key array holds 33 entries (the base shape and 32
 *   keys), the key count is declared before the destination handle, the vertex count is a u32, the destination and
 *   end pointers are VEC3 pointers, and the key loop walks BlendKey pointers up to an end hoisted out of the vertex
 *   loop.
 *   the accessor copies are defined by the NW4R_G3D_RESOURCE_* macros (g3d/g3d_rescommon.h) in retail order.
 *   The unit compiles with `#pragma peephole off` throughout (retail keeps the unfused `clrlwi` + `cmpwi`, `extsh`,
 *   `addi r0` vtable-store and `mr r3` + `lwz r12,0(r3)` virtual-call forms; playbook idea 106).
 */

#include "types.h"
#include "nw4r/db_assert.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcvtx.h"
#include "g3d/g3d_anmshp.h"    /* anm_obj_shp_test_existence (rule 2) */
#include "g3d/g3d_obj.h"       /* nw4r::g3d::AnmObjShp (rule 2) */
#include "unsplit/OS.h"

#pragma peephole off

#pragma fp_contract off
#pragma pool_data off

/* The model validity check the driver asserts (owner g3d/g3d_anmchr.cpp). */

#define RESVTX_AC_FILE "g3d_resvtx_ac.h"

NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResVtxClrData)
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResVtxNrmData)
NW4R_G3D_RESCOMMON_CTOR(nw4r::g3d::ResVtxPosData)

namespace nw4r {
namespace g3d {

/* One shape's three vertex arrays.  size: 0xC */
struct ShpAnmVtxSet {
    ShpAnmVtxSet();

    /* +0x0 */ ResVtxPos resVtxPos;
    /* +0x4 */ ResVtxNrm resVtxNrm;
    /* +0x8 */ ResVtxClr resVtxClr;
};

/* One key shape of the blend and its weight.  size: 0x10 */
struct ShpAnmKeyShape {
    ShpAnmKeyShape();

    /* +0x0 */ ShpAnmVtxSet vtxSet;
    /* +0xC */ f32 weight;
};

/* The blend an animated shape hands back: the base shape and up to 32 key shapes.  size: 0x218 */
struct ShpAnmResult {
    ShpAnmResult();

    /* +0x000 */ u32 flags;   /* bit 0: valid; bits 1/2/3: blend positions/normals/colours */
    /* +0x004 */ s32 numKeyShape;
    /* +0x008 */ ShpAnmVtxSet baseShapeVtxSet;
    /* +0x014 */ f32 baseShapeWeight;
    /* +0x018 */ ShpAnmKeyShape keyShape[32];
};

/* The {vertex pointer, weight} pair the blend walks; the pointer is advanced in place per vertex.  size: 0x8 */
struct BlendKey {
    /* +0x0 */ const f32* mpVtx;
    /* +0x4 */ f32 mWeight;
};


/* 0x800730B4 (0x24): returns the number of colours. */
u16 ResVtxClr::GetNumVtxClr() const {
    return ref().numClr;
}

NW4R_G3D_RESOURCE_REF_CONST(ResVtxClr, RESVTX_AC_FILE, 154)
NW4R_G3D_RESOURCE_CLASS_NAME(ResVtxClr)

/* 0x80073148 (0x38): returns the colour data, or NULL when the block has none. */
/* untyped: byte range */
void* ResVtxClr::GetData() {
    ResVtxClrData& r = ref();
    return r.toClrArray != 0 ? reinterpret_cast<u8*>(&r) + r.toClrArray : NULL;
}

NW4R_G3D_RESOURCE_REF(ResVtxClr, RESVTX_AC_FILE, 154)
NW4R_G3D_RESOURCE_PTR(ResVtxClr)

NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxClr, RESVTX_AC_FILE, 154)

/* 0x80073258 (0x24): returns the array's ID. */
u32 ResVtxClr::GetID() const {
    return ref().id;
}

NW4R_G3D_RESOURCE_IS_VALID(ResVtxClr)

/* 0x80073290 (0x30): copies the handle `pRhs` holds. */
ResVtxClr::ResVtxClr(const ResVtxClr* pRhs) {
    CopyResVtxClrHandle(this, pRhs);
}

}  // namespace g3d
}  // namespace nw4r

/* 0x800732C0 (0xC): copies a ResVtxClr handle word. */
/* untyped: opaque handle */
extern "C" void CopyResVtxClrHandle(void* pDst, const void* pSrc) {
    *static_cast<u32*>(pDst) = *static_cast<const u32*>(pSrc);
}

namespace nw4r {
namespace g3d {

/* 0x800732CC (0x24): returns the number of normals. */
u16 ResVtxNrm::GetNumVtxNrm() const {
    return ref().numNrm;
}

NW4R_G3D_RESOURCE_REF_CONST(ResVtxNrm, RESVTX_AC_FILE, 98)
NW4R_G3D_RESOURCE_CLASS_NAME(ResVtxNrm)

/* 0x80073360 (0x38): returns the normal data, or NULL when the block has none. */
/* untyped: byte range */
void* ResVtxNrm::GetData() {
    ResVtxNrmData& r = ref();
    return r.toVtxNrmArray != 0 ? reinterpret_cast<u8*>(&r) + r.toVtxNrmArray : NULL;
}

NW4R_G3D_RESOURCE_REF(ResVtxNrm, RESVTX_AC_FILE, 98)
NW4R_G3D_RESOURCE_PTR(ResVtxNrm)
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxNrm, RESVTX_AC_FILE, 98)

/* 0x80073470 (0x24): returns the array's ID. */
u32 ResVtxNrm::GetID() const {
    return ref().id;
}

NW4R_G3D_RESOURCE_IS_VALID(ResVtxNrm)

/* 0x800734A8 (0x30): copies the handle `pRhs` holds. */
ResVtxNrm::ResVtxNrm(const ResVtxNrm* pRhs) {
    CopyResVtxNrmHandle(this, pRhs);
}

}  // namespace g3d
}  // namespace nw4r

/* 0x800734D8 (0xC): copies a ResVtxNrm handle word. */
/* untyped: opaque handle */
extern "C" void CopyResVtxNrmHandle(void* pDst, const void* pSrc) {
    *static_cast<u32*>(pDst) = *static_cast<const u32*>(pSrc);
}

namespace nw4r {
namespace g3d {

/* 0x800734E4 (0x4): stores a range out of the data cache, waiting for the store. */
/* untyped: byte range */
void DC::StoreRange(void* pStart, u32 size) {
    DCStoreRange(pStart, size);
}

/* 0x800734E8 (0x24): returns the number of positions. */
u16 ResVtxPos::GetNumVtxPos() const {
    return ref().numPos;
}

/* 0x8007350C (0x38): returns the position data, or NULL when the block has none. */
/* untyped: byte range */
void* ResVtxPos::GetData() {
    ResVtxPosData& r = ref();
    return r.toVtxPosArray != 0 ? reinterpret_cast<u8*>(&r) + r.toVtxPosArray : NULL;
}

NW4R_G3D_RESOURCE_REF(ResVtxPos, RESVTX_AC_FILE, 39)
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxPos, RESVTX_AC_FILE, 39)

/* 0x80073614 (0x60): constructs the base shape and the 32 key shapes with empty handles. */
ShpAnmResult::ShpAnmResult() {}

/* 0x80073674 (0x30): constructs a key shape with empty handles. */
ShpAnmKeyShape::ShpAnmKeyShape() {}

/* 0x800736A4 (0x4C): constructs the three handles empty. */
ShpAnmVtxSet::ShpAnmVtxSet() : resVtxPos((void*)NULL), resVtxNrm((void*)NULL), resVtxClr((void*)NULL) {}

NW4R_G3D_RESOURCE_PTR(ResVtxPos)

}  // namespace g3d
}  // namespace nw4r

/* 0x8007270C (0x9A8): blends each animated shape's base shape and key shapes over the position, normal and colour
 * arrays (passes for flag bits 1/2/3, differing only in the array class and the per-vertex width). */
extern "C" void fn_8007270C(void* pMdl, void* pAnmObjShp, const void** vtxPosTable, const void** vtxNrmTable,
                            const void** vtxClrTable) {
    using namespace nw4r::g3d;

    if (pAnmObjShp == NULL) {
        nw4r::db::Panic("g3d_calcvtx.cpp", 36, "NW4R:Pointer must not be NULL (pAnmObjShp)");
    }
    if (!static_cast<ResMdl*>(pMdl)->IsValid()) {
        nw4r::db::Panic("g3d_calcvtx.cpp", 37, "NW4R:Failed assertion mdl.IsValid()");
    }
    u32 numVtxPos = static_cast<ResMdl*>(pMdl)->GetResVtxPosNumEntries();

    for (u32 i = 0; i < numVtxPos; i++) {
        if (!anm_obj_shp_test_existence(pAnmObjShp, i)) {
            continue;
        }
        if (vtxPosTable == NULL) {
            nw4r::db::Panic("g3d_calcvtx.cpp", 49, "NW4R:Pointer must not be NULL (vtxPosTable)");
        }
        if (static_cast<ResMdl*>(pMdl)->GetResVtxPos(i).ptr() == vtxPosTable[i]) {
            nw4r::db::Panic("g3d_calcvtx.cpp", 50, "NW4R:Failed assertion mdl.GetResVtxPos(vtxPosID).ptr() != vtxPosTable[vtxPosID]");
        }
        ShpAnmResult result;
        const ShpAnmResult* pResult = static_cast<AnmObjShp*>(pAnmObjShp)->GetResult(&result, i);
        if (pResult == NULL) {
            nw4r::db::Panic("g3d_calcvtx.cpp", 55, "NW4R:Pointer must not be NULL (pResult)");
        }
        if ((pResult->flags & 1) == 0) {
            continue;
        }

        if ((pResult->flags & 2) != 0) {
            ResVtxPos resVtxPos(&pResult->baseShapeVtxSet.resVtxPos);
            if (!resVtxPos.IsValid()) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 66, "NW4R:Failed assertion resVtxPos.IsValid()");
            }
            if (resVtxPos.GetID() != (u32)i) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 67, "NW4R:Failed assertion resVtxPos.GetID() == vtxPosID");
            }
            s32 numKeyShape = 0;
            ResVtxPos dst(const_cast<void*>(vtxPosTable[i]));
            nw4r::math::VEC3* dstVtx = (nw4r::math::VEC3*)dst.GetData();
            BlendKey keys[33];
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxPos.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 89, "NW4R:Pointer must not be NULL (pBaseVtx)");
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 91, "NW4R:Failed assertion vtxStride == VTX_STRIDE");
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxPos key(&pResult->keyShape[k].vtxSet.resVtxPos);
                    if (!key.IsValid()) {
                        nw4r::db::Panic("g3d_calcvtx.cpp", 103, "NW4R:Failed assertion key.IsValid()");
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 111, "NW4R:Failed assertion numKeyShape > 0");
            }
            u32 count = resVtxPos.GetNumVtxPos();
            nw4r::math::VEC3* out = dstVtx;
            nw4r::math::VEC3* end = out + count;
            BlendKey* pKeyEnd = &keys[numKeyShape];
            while (out < end) {
                register f32 w = keys[0].mWeight;
                register const f32* pSrc = keys[0].mpVtx;
                asm {
                    psq_l f0, 0(pSrc), 0, 0
                    psq_l f1, 8(pSrc), 1, 0
                    ps_mul f0, f0, w
                    ps_mul f1, f1, w
                }
                keys[0].mpVtx = pSrc + 3;
                for (BlendKey* pKeyShape = &keys[1]; pKeyShape < pKeyEnd; pKeyShape++) {
                    register f32 wk = pKeyShape->mWeight;
                    register const f32* pKey = pKeyShape->mpVtx;
                    asm {
                        psq_l f3, 0(pKey), 0, 0
                        psq_l f4, 8(pKey), 1, 0
                        ps_madd f0, f3, wk, f0
                        ps_madd f1, f4, wk, f1
                    }
                    pKeyShape->mpVtx = pKey + 3;
                }
                register nw4r::math::VEC3* pOut = out;
                asm {
                    psq_st f0, 0(pOut), 0, 0
                    psq_st f1, 8(pOut), 1, 0
                }
                out++;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 0xC);
        }

        if ((pResult->flags & 4) != 0 && vtxNrmTable != NULL) {
            ResVtxNrm resVtxNrm(&pResult->baseShapeVtxSet.resVtxNrm);
            if (!resVtxNrm.IsValid()) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 0xC4, "NW4R:Failed assertion resVtxNrm.IsValid()");
            }
            s32 numKeyShape = 0;
            ResVtxNrm dst(const_cast<void*>(vtxNrmTable[resVtxNrm.GetID()]));
            nw4r::math::VEC3* dstVtx = (nw4r::math::VEC3*)dst.GetData();
            BlendKey keys[33];
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxNrm.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 0xDA, "NW4R:Pointer must not be NULL (pBaseVtx)");
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 0xDC, "NW4R:Failed assertion vtxStride == VTX_STRIDE");
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxNrm key(&pResult->keyShape[k].vtxSet.resVtxNrm);
                    if (!key.IsValid()) {
                        nw4r::db::Panic("g3d_calcvtx.cpp", 0xE8, "NW4R:Failed assertion key.IsValid()");
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 0xF0, "NW4R:Failed assertion numKeyShape > 0");
            }
            u32 count = resVtxNrm.GetNumVtxNrm();
            nw4r::math::VEC3* out = dstVtx;
            nw4r::math::VEC3* end = out + count;
            BlendKey* pKeyEnd = &keys[numKeyShape];
            while (out < end) {
                register f32 w = keys[0].mWeight;
                register const f32* pSrc = keys[0].mpVtx;
                asm {
                    psq_l f0, 0(pSrc), 0, 0
                    psq_l f1, 8(pSrc), 1, 0
                    ps_mul f0, f0, w
                    ps_mul f1, f1, w
                }
                keys[0].mpVtx = pSrc + 3;
                for (BlendKey* pKeyShape = &keys[1]; pKeyShape < pKeyEnd; pKeyShape++) {
                    register f32 wk = pKeyShape->mWeight;
                    register const f32* pKey = pKeyShape->mpVtx;
                    asm {
                        psq_l f3, 0(pKey), 0, 0
                        psq_l f4, 8(pKey), 1, 0
                        ps_madd f0, f3, wk, f0
                        ps_madd f1, f4, wk, f1
                    }
                    pKeyShape->mpVtx = pKey + 3;
                }
                register nw4r::math::VEC3* pOut = out;
                asm {
                    psq_st f0, 0(pOut), 0, 0
                    psq_st f1, 8(pOut), 1, 0
                }
                out++;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 0xC);
        }

        if ((pResult->flags & 8) != 0 && vtxClrTable != NULL) {
            ResVtxClr resVtxClr(&pResult->baseShapeVtxSet.resVtxClr);
            if (!resVtxClr.IsValid()) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 0x146, "NW4R:Failed assertion resVtxClr.IsValid()");
            }
            s32 numKeyShape = 0;
            ResVtxClr dst(const_cast<void*>(vtxClrTable[resVtxClr.GetID()]));
            f32* dstVtx = (f32*)dst.GetData();
            BlendKey keys[33];
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxClr.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 0x15C, "NW4R:Pointer must not be NULL (pBaseVtx)");
                }
                if (vtxStride != 0x4) {
                    nw4r::db::Panic("g3d_calcvtx.cpp", 0x15E, "NW4R:Failed assertion vtxStride == VTX_STRIDE");
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxClr key(&pResult->keyShape[k].vtxSet.resVtxClr);
                    if (!key.IsValid()) {
                        nw4r::db::Panic("g3d_calcvtx.cpp", 0x16A, "NW4R:Failed assertion key.IsValid()");
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic("g3d_calcvtx.cpp", 0x172, "NW4R:Failed assertion numKeyShape > 0");
            }
            u32 count = resVtxClr.GetNumVtxClr();
            f32* out = dstVtx;
            f32* end = out + count;
            BlendKey* pKeyEnd = &keys[numKeyShape];
            while (out < end) {
                register f32 w = keys[0].mWeight;
                register const f32* pSrc = keys[0].mpVtx;
                asm {
                    psq_l f0, 0(pSrc), 0, 2
                    psq_l f1, 2(pSrc), 0, 2
                    ps_mul f0, f0, w
                    ps_mul f1, f1, w
                }
                keys[0].mpVtx = pSrc + 1;
                for (BlendKey* pKeyShape = &keys[1]; pKeyShape < pKeyEnd; pKeyShape++) {
                    register f32 wk = pKeyShape->mWeight;
                    register const f32* pKey = pKeyShape->mpVtx;
                    asm {
                        psq_l f3, 0(pKey), 0, 2
                        psq_l f4, 2(pKey), 0, 2
                        ps_madd f0, f3, wk, f0
                        ps_madd f1, f4, wk, f1
                    }
                    pKeyShape->mpVtx = pKey + 1;
                }
                register f32* pOut = out;
                asm {
                    psq_st f0, 0(pOut), 0, 2
                    psq_st f1, 2(pOut), 0, 2
                }
                out += 1;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 4);
        }
    }
}
