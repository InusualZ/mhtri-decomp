/* g3d/g3d_calcmaterial.cpp - the `g3d_calcmaterial.cpp` TU's `.text` 0x8006EE78..0x8006F738.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with a symbol greps of
 * config/RMHE08/symbols.txt: every .text entry in 0x8006EE78..0x8007270C is a bare `fn_XXXXXXXX`).
 *
 * The file name is class-1 evidence: the `.data` string `lbl_8058D7E0` read out of
 * orig/RMHE08/sys/main.dol is exactly "g3d_calcmaterial.cpp", the `pFile` argument of the
 * `nw4r::db::Panic` assertion inside fn_8006EE78.  Module `g3d` (its registered neighbours are all
 * `g3d/*`); the `.cpp` suffix plus the `Panic__Q24nw4r2dbFPCciPCce`/C++ front-end make it C++.
 *
 * The seam at 0x8006F738 is proven the same way: 0x8006F738's first assertion cites
 * `lbl_8058D938` = "g3d_calcview.cpp" and the range after it is a different TU (registered
 * separately as `g3d/g3d_calcview.cpp`).  The TU before (0x8006EAC0..0x8006EE78) cites
 * "g3d_anmvis.cpp" and the next (0x8007270C..) cites "g3d_calcvtx.cpp".
 *
 * This file reconstructs the material-controller helpers: a chain of small pointer wrappers over the
 * global material table returned by fn_8005A9BC, whose inlined constructors (fn_8006F158 / fn_8006F298
 * assert 32-byte alignment, fn_8006F374 / fn_8006F41C / fn_8006F4BC assert 4-byte alignment) carry the
 * `g3d_resmat_ac.h` file string.  fn_8006EE78 walks the material list and drives per-material matrix
 * updates.
 *
 * Residuals (all still stubs in this commit): fn_8006EE78, fn_8006F5A0 and fn_8006F660 are the large
 * vector/matrix bodies; their per-symbol scores are in the outbox.
 */
#include "types.h"

/* The five assert wrappers below need the *un-fused* compare (retail keeps `clrlwi` + `cmpwi` and a
 * separate `add`/`blr`, where the peephole pass folds them into `clrlwi.`/`bnelr`), exactly as
 * g3d_basic.cpp found for its asserts. */
#pragma peephole off

namespace nw4r {
namespace db {
/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
}  // namespace db
}  // namespace nw4r

extern "C" {
/* The helpers this unit calls.  Every one is still a bare `fn_XXXXXXXX` in the map (C linkage) and
 * its address has no registered owner, so - as g3d_basic.cpp does for the same band - they are
 * declared here rather than in a header nobody owns yet (docs/plan.md 6.5 rule 2's named gap for an
 * unsplit address). */
/* Allocator/registry pair behind fn_8006F0DC / fn_8006F220. */
u32 fn_800941DC(void* p, u32 flag);
u32 fn_8009429C(void* p, u32 flag);
/* The two handle-array element constructors fn_8006F528 runs. */
void fn_80052BC0(void* p, u32 flag);
void fn_800534B0(void* p, u32 flag);
/* The global material-table accessor the accessor chain reads at +0x3C. */
void* fn_8005A9BC(void* self);

/* This unit's own bodies, in address order. */
void* fn_8006EE78(void* pMdl, void* pMatArray, void* pTexArray, void* pClrArray);
u32 fn_8006F0DC(void* p);
void fn_8006F0E4(void* p);
void* fn_8006F0E8(void* pDst, const void* pSrc);
void fn_8006F118(void* pDst, const void* pSrc);
u32 fn_8006F124(void* self);
u32* fn_8006F158(u32* pDst, u32 ptr);
void fn_8006F1BC(u32* pDst, u32 value);
void* fn_8006F1C4(void* self);
void* fn_8006F200(void* pHandle, u32 offset);
void fn_8006F21C(void* p);
u32 fn_8006F220(void* p);
void* fn_8006F228(void* pDst, const void* pSrc);
void fn_8006F258(void* pDst, const void* pSrc);
u32 fn_8006F264(void* self);
u32* fn_8006F298(u32* pDst, u32 ptr);
void fn_8006F2FC(u32* pDst, u32 value);
void* fn_8006F304(void* pDst, const void* pSrc);
void fn_8006F334(void* pDst, const void* pSrc);
u32 fn_8006F340(void* self);
u32* fn_8006F374(u32* pDst, u32 ptr);
void fn_8006F3D8(u32* pDst, u32 value);
void fn_8006F3E0(void* p);
void fn_8006F3E4(void* p);
u32 fn_8006F3E8(void* self);
u32* fn_8006F41C(u32* pDst, u32 ptr);
void fn_8006F480(u32* pDst, u32 value);
u32 fn_8006F488(void* self);
u32* fn_8006F4BC(u32* pDst, u32 ptr);
void fn_8006F520(u32* pDst, u32 value);
u32* fn_8006F528(u32* self);
void fn_8006F5A0(f32* pDst, const f32* pMtx, const f32* pVec);
void fn_8006F660(f32* pDst, const f32* pMtx, const f32* pVec);
}

/* The pooled file-name/assert strings this unit reads (declared, never defined - they live in the
 * original `.data`, which this unit does not claim). */
extern const char lbl_8058D7E0[]; /* "g3d_calcmaterial.cpp"                            .data */
extern const char lbl_8058D7F8[]; /* "NW4R:Failed assertion mdl.IsValid()"            .data */
extern const char lbl_8058D81C[]; /* "NW4R:Failed assertion !((u32)p & 0x1f)"         .data */
extern const char lbl_8058D848[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D858[]; /* "NW4R:Failed assertion !((u32)p & 0x1f)"         .data */
extern const char lbl_8058D880[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D890[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D8B8[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D8C8[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D8F0[]; /* "g3d_resmat_ac.h"                                .data */
extern const char lbl_8058D900[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"          .data */
extern const char lbl_8058D928[]; /* "g3d_resmat_ac.h"                                .data */

/* The 32-byte-aligned resource pointer wrapper fn_8006F158 builds and the 4-byte-aligned ones
 * (fn_8006F374 / fn_8006F41C / fn_8006F4BC).  The asserts are the g3d_resmat_ac.h inlines. */

extern "C" {

/* fn_8006F118: copy a 4-byte resource handle. */
void fn_8006F118(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}

/* fn_8006F1BC / fn_8006F2FC / fn_8006F334 / fn_8006F3D8 / fn_8006F480 / fn_8006F520: store a handle. */
void fn_8006F1BC(u32* pDst, u32 value) {
    *pDst = value;
}
void fn_8006F2FC(u32* pDst, u32 value) {
    *pDst = value;
}
void fn_8006F334(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}
void fn_8006F3D8(u32* pDst, u32 value) {
    *pDst = value;
}
void fn_8006F480(u32* pDst, u32 value) {
    *pDst = value;
}
void fn_8006F520(u32* pDst, u32 value) {
    *pDst = value;
}
void fn_8006F258(void* pDst, const void* pSrc) {
    *(u32*)pDst = *(u32*)pSrc;
}

/* The empty hooks. */
void fn_8006F0E4(void* p) {
    (void)p;
}
void fn_8006F21C(void* p) {
    (void)p;
}
void fn_8006F3E0(void* p) {
    (void)p;
}
void fn_8006F3E4(void* p) {
    (void)p;
}

/* fn_8006F200: base handle + offset, with offset 0 meaning the null resource. */
void* fn_8006F200(void* pHandle, u32 offset) {
    u32 base = *(u32*)pHandle;
    if (offset == 0) {
        return 0;
    }
    return (void*)(base + offset);
}

/* fn_8006F0DC / fn_8006F220: tail calls into the pool allocator/registry with a null flag. */
u32 fn_8006F0DC(void* p) {
    return fn_800941DC(p, 0);
}
u32 fn_8006F220(void* p) {
    return fn_8009429C(p, 0);
}

/* fn_8006F0E8 / fn_8006F228 / fn_8006F304: copy-construct and return the destination. */
void* fn_8006F0E8(void* pDst, const void* pSrc) {
    fn_8006F118(pDst, pSrc);
    return pDst;
}
void* fn_8006F228(void* pDst, const void* pSrc) {
    fn_8006F258(pDst, pSrc);
    return pDst;
}
void* fn_8006F304(void* pDst, const void* pSrc) {
    fn_8006F334(pDst, pSrc);
    return pDst;
}

/* The alignment-asserting resource pointer constructors (g3d_resmat_ac.h inlines).
 * `lbl_8058D848`/`lbl_8058D880`/`...` are the header file strings, `lbl_8058D81C`/`lbl_8058D858`/...
 * the "!((u32)p & mask)" messages. */
u32* fn_8006F158(u32* pDst, u32 ptr) {
    fn_8006F1BC(pDst, ptr);
    if (ptr & 0x1F) {
        nw4r::db::Panic(lbl_8058D880, 378, lbl_8058D858);
    }
    return pDst;
}
u32* fn_8006F298(u32* pDst, u32 ptr) {
    fn_8006F2FC(pDst, ptr);
    if (ptr & 0x1F) {
        nw4r::db::Panic(lbl_8058D848, 413, lbl_8058D81C);
    }
    return pDst;
}
u32* fn_8006F374(u32* pDst, u32 ptr) {
    fn_8006F3D8(pDst, ptr);
    if (ptr & 0x3) {
        nw4r::db::Panic(lbl_8058D8B8, 107, lbl_8058D890);
    }
    return pDst;
}
u32* fn_8006F41C(u32* pDst, u32 ptr) {
    fn_8006F480(pDst, ptr);
    if (ptr & 0x3) {
        nw4r::db::Panic(lbl_8058D8F0, 74, lbl_8058D8C8);
    }
    return pDst;
}
u32* fn_8006F4BC(u32* pDst, u32 ptr) {
    fn_8006F520(pDst, ptr);
    if (ptr & 0x3) {
        nw4r::db::Panic(lbl_8058D928, 40, lbl_8058D900);
    }
    return pDst;
}

/* fn_8006F1C4: the material table base plus the per-frame offset at +0x3C of the global from
 * fn_8005A9BC. */
/* The global material table `fn_8005A9BC` returns; only its per-frame offset at +0x3C is read here,
 * so the size is the minimum that covers it. */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x3C];
    /* +0x3C */ u32 offset_0x3C;
} G3dMaterialTable; /* size: 0x40 */

void* fn_8006F1C4(void* self) {
    G3dMaterialTable* pGlobal = (G3dMaterialTable*)fn_8005A9BC(self);
    return fn_8006F200(self, pGlobal->offset_0x3C);
}

/* The typed sub-resource accessors: build the assert-checked pointer and read its handle. */
u32 fn_8006F124(void* self) {
    u8* pBase = (u8*)fn_8006F1C4(self);
    u32 handle;
    return *fn_8006F158(&handle, (u32)(pBase + 32));
}
u32 fn_8006F264(void* self) {
    u8* pBase = (u8*)fn_8006F1C4(self);
    u32 handle;
    return *fn_8006F298(&handle, (u32)(pBase + 160));
}
u32 fn_8006F340(void* self) {
    u8* pGlobal = (u8*)fn_8005A9BC(self);
    u32 handle;
    return *fn_8006F374(&handle, (u32)(pGlobal + 424));
}
u32 fn_8006F3E8(void* self) {
    u8* pGlobal = (u8*)fn_8005A9BC(self);
    u32 handle;
    return *fn_8006F41C(&handle, (u32)(pGlobal + 324));
}
u32 fn_8006F488(void* self) {
    u8* pGlobal = (u8*)fn_8005A9BC(self);
    u32 handle;
    return *fn_8006F4BC(&handle, (u32)(pGlobal + 64));
}

/* fn_8006F528: construct the 8+8 entry handle arrays at +0x4 and +0x24 of the record. */
u32* fn_8006F528(u32* self) {
    u32* p = self + 1;
    u32* mid = self + 9;
    for (; p < mid; p++) {
        fn_80052BC0(p, 0);
    }
    for (; mid < self + 17; mid++) {
        fn_800534B0(mid, 0);
    }
    return self;
}

/* fn_8006EE78: walk the model's material list and drive the per-material matrix update.  Stub. */
void* fn_8006EE78(void* pMdl, void* pMatArray, void* pTexArray, void* pClrArray) {
    (void)pMdl;
    (void)pMatArray;
    (void)pTexArray;
    (void)pClrArray;
    return pMdl;
}

/* fn_8006F5A0 / fn_8006F660: paired-single matrix/vector builders (a compiler-cloned pair).  Stubs. */
void fn_8006F5A0(f32* pDst, const f32* pMtx, const f32* pVec) {
    (void)pDst;
    (void)pMtx;
    (void)pVec;
}
void fn_8006F660(f32* pDst, const f32* pMtx, const f32* pVec) {
    (void)pDst;
    (void)pMtx;
    (void)pVec;
}

}  // extern "C"
