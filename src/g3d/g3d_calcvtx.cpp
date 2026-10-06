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
 * RESIDUALS. fn_8007270C: retail blends with paired singles (`psq_l`, `ps_mul`, `ps_madd`); ours is scalar.
 *   .data: the object emits 0x6A of the claimed 0x410 and .sdata 0x4 of 0x18 (the driver's strings stay extern).
 *   The `ResCommon<T>` copy helpers (CopyResVtxClrHandle, CopyResVtxNrmHandle, fn_80069798) keep their stems.
 * SHAPES. the ShpAnm* constructors are complete: their work is the members' default constructions.
 *   the accessor copies are defined by the NW4R_G3D_RESOURCE_* macros (g3d/g3d_rescommon.h) in retail order.
 */

#include "types.h"
#include "nw4r/db_assert.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcvtx.h"
#include "g3d/fn_800680CC.h"
#include "unsplit/OS.h"

#pragma fp_contract off
#pragma pool_data off

/* The driver's panic strings, addressed one by one as the target does. */
extern const char lbl_8058DD68[]; /* "g3d_calcvtx.cpp" */
extern const char lbl_8058DD78[]; /* "NW4R:Pointer must not be NULL (pAnmObjShp)" */
extern const char lbl_8058DDA4[]; /* "NW4R:Failed assertion mdl.IsValid()" */
extern const char lbl_8058DDC8[]; /* "NW4R:Pointer must not be NULL (vtxPosTable)" */
extern const char lbl_8058DDF8[]; /* "NW4R:Failed assertion mdl.GetResVtxPos(vtxPosID)" */
extern const char lbl_8058DE48[]; /* "NW4R:Pointer must not be NULL (pResult)" */
extern const char lbl_8058DE70[]; /* "NW4R:Failed assertion resVtxPos.IsValid()" */
extern const char lbl_8058DE9C[]; /* "NW4R:Failed assertion resVtxPos.GetID() == vtxPosID" */
extern const char lbl_8058DED0[]; /* "NW4R:Pointer must not be NULL (pBaseVtx)" */
extern const char lbl_8058DEFC[]; /* "NW4R:Failed assertion vtxStride == VTX_STRIDE" */
extern const char lbl_8058DF2C[]; /* "NW4R:Failed assertion key.IsValid()" */
extern const char lbl_8058DF50[]; /* "NW4R:Failed assertion numKeyShape > 0" */
extern const char lbl_8058DF78[]; /* "NW4R:Failed assertion resVtxNrm.IsValid()" */
extern const char lbl_8058DFA4[]; /* "NW4R:Failed assertion resVtxClr.IsValid()" */

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

/* The animated-shape object's virtual that fills `pResult` and returns the blend for vertex array `id`. */
typedef const ShpAnmResult* (*ShpCalcFn)(void* pSelf, ShpAnmResult* pResult, s32 id);

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

#pragma peephole off
NW4R_G3D_RESOURCE_CTOR_ALIGNED(ResVtxClr, RESVTX_AC_FILE, 154)
#pragma peephole on

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
    ResMdl& mdl = *static_cast<ResMdl*>(pMdl);

    if (pAnmObjShp == NULL) {
        nw4r::db::Panic(lbl_8058DD68, 36, lbl_8058DD78);
    }
    if (!mdl.IsValid()) {
        nw4r::db::Panic(lbl_8058DD68, 37, lbl_8058DDA4);
    }
    s32 numVtxPos = (s32)mdl.GetResVtxPosNumEntries();

    for (s32 i = 0; i < numVtxPos; i++) {
        if (!fn_8006946C(pAnmObjShp, i)) {
            continue;
        }
        if (vtxPosTable == NULL) {
            nw4r::db::Panic(lbl_8058DD68, 49, lbl_8058DDC8);
        }
        ResVtxPos vtxPos = mdl.GetResVtxPos((u32)i);
        if (vtxPos.ptr() != vtxPosTable[i]) {
            nw4r::db::Panic(lbl_8058DD68, 50, lbl_8058DDF8);
        }
        ShpAnmResult result;
        const ShpAnmResult* pResult = ((ShpCalcFn*)(*(void**)pAnmObjShp))[14](pAnmObjShp, &result, i);
        if (pResult == NULL) {
            nw4r::db::Panic(lbl_8058DD68, 55, lbl_8058DE48);
        }
        if ((pResult->flags & 1) == 0) {
            continue;
        }

        if ((pResult->flags & 2) != 0) {
            ResVtxPos resVtxPos(&pResult->baseShapeVtxSet.resVtxPos);
            if (!resVtxPos.IsValid()) {
                nw4r::db::Panic(lbl_8058DD68, 66, lbl_8058DE70);
            }
            if (resVtxPos.GetID() != (u32)i) {
                nw4r::db::Panic(lbl_8058DD68, 67, lbl_8058DE9C);
            }
            ResVtxPos dst(const_cast<void*>(vtxPosTable[i]));
            f32* dstVtx = (f32*)dst.GetData();
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxPos.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 89, lbl_8058DED0);
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic(lbl_8058DD68, 91, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxPos key(&pResult->keyShape[k].vtxSet.resVtxPos);
                    if (!key.IsValid()) {
                        nw4r::db::Panic(lbl_8058DD68, 103, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 111, lbl_8058DF50);
            }
            u16 count = resVtxPos.GetNumVtxPos();
            f32* out = dstVtx;
            f32* end = out + count * 3;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 3;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 3;
                }
                for (s32 j = 0; j < 3; j++) {
                    out[j] = v[j];
                }
                out += 3;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 0xC);
        }

        if ((pResult->flags & 4) != 0 && vtxNrmTable != NULL) {
            ResVtxNrm resVtxNrm(&pResult->baseShapeVtxSet.resVtxNrm);
            if (!resVtxNrm.IsValid()) {
                nw4r::db::Panic(lbl_8058DD68, 0xC4, lbl_8058DF78);
            }
            ResVtxNrm dst(const_cast<void*>(vtxNrmTable[resVtxNrm.GetID()]));
            f32* dstVtx = (f32*)dst.GetData();
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxNrm.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 0xDA, lbl_8058DED0);
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic(lbl_8058DD68, 0xDC, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxNrm key(&pResult->keyShape[k].vtxSet.resVtxNrm);
                    if (!key.IsValid()) {
                        nw4r::db::Panic(lbl_8058DD68, 0xE8, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 0xF0, lbl_8058DF50);
            }
            u16 count = resVtxNrm.GetNumVtxNrm();
            f32* out = dstVtx;
            f32* end = out + count * 3;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 3;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 3;
                }
                for (s32 j = 0; j < 3; j++) {
                    out[j] = v[j];
                }
                out += 3;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 0xC);
        }

        if ((pResult->flags & 8) != 0 && vtxClrTable != NULL) {
            ResVtxClr resVtxClr(&pResult->baseShapeVtxSet.resVtxClr);
            if (!resVtxClr.IsValid()) {
                nw4r::db::Panic(lbl_8058DD68, 0x146, lbl_8058DFA4);
            }
            ResVtxClr dst(const_cast<void*>(vtxClrTable[resVtxClr.GetID()]));
            f32* dstVtx = (f32*)dst.GetData();
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pResult->baseShapeWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                resVtxClr.GetArray(&pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 0x15C, lbl_8058DED0);
                }
                if (vtxStride != 0x4) {
                    nw4r::db::Panic(lbl_8058DD68, 0x15E, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pResult->baseShapeWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pResult->numKeyShape; k++) {
                if (pResult->keyShape[k].weight != 0.0f) {
                    ResVtxClr key(&pResult->keyShape[k].vtxSet.resVtxClr);
                    if (!key.IsValid()) {
                        nw4r::db::Panic(lbl_8058DD68, 0x16A, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)key.GetData();
                    keys[numKeyShape].mWeight = pResult->keyShape[k].weight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 0x172, lbl_8058DF50);
            }
            u16 count = resVtxClr.GetNumVtxClr();
            f32* out = dstVtx;
            f32* end = out + count;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 1;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 1;
                }
                out[0] = v[0];
                out[1] = v[1];
                out[2] = v[2];
                out += 1;
            }
            DC::StoreRange(dst.GetData(), (u32)count * 4);
        }
    }
}
