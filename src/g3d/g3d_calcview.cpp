/* g3d/g3d_calcview.cpp - the `g3d_calcview.cpp` TU's `.text` 0x8006F738..0x8007270C.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with a grep of
 * config/RMHE08/symbols.txt: every .text entry in 0x8006EE78..0x8007270C is a bare `fn_XXXXXXXX`).
 *
 * The file name is class-1 evidence: the `.data` string `lbl_8058D938` read out of
 * orig/RMHE08/sys/main.dol is exactly "g3d_calcview.cpp", the `pFile` argument of the
 * `nw4r::db::Warning`/`Panic` calls in fn_8006F738, fn_8006F934, fn_8006FBBC, fn_80070134,
 * fn_80070410, fn_80070600, fn_80070820, fn_80071198 and fn_80071C48.  Module `g3d`; the `.cpp`
 * suffix plus the C++ call sites make it C++.
 *
 * Seams: 0x8006F738 (left, from `g3d_calcmaterial.cpp`) and 0x8007270C (right, where the range's
 * first `g3d_calcvtx.cpp` function fn_8007270C starts).  The billboard warning strings
 * (`lbl_8058D94C` .. `lbl_8058DA70`) in the same pool belong to this file.
 *
 * This file owns the view/billboard-matrix calculator: the per-billboard matrix builders
 * (fn_8006F738 / fn_8006F934 / fn_8006FBBC) and the material/model accessor chain they drive
 * (fn_8006FDCC .. fn_80070134).  The reconstruction is in progress: the small accessor wrappers
 * are matched below, the large paired-single bodies are still registration stubs and their
 * per-symbol scores are in the outbox.
 */
#include "types.h"

/* The alignment-assert wrappers need the un-fused compare (retail keeps `clrlwi` + `cmpwi`), exactly
 * as g3d/g3d_basic.cpp and g3d/g3d_calcmaterial.cpp found. */
#pragma peephole off

namespace nw4r {
namespace db {
/* `Panic`/`Warning`; the map names carry the C++ mangling, so they are called through their owner. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
void Warning(const char* pFile, int line, const char* pFmt, ...);
}  // namespace db
}  // namespace nw4r

/* The pooled file-name/assert strings and the two `.sdata` globals this unit reads (declared, never
 * defined - they live in the original `.data`/`.sdata`, which this unit does not claim). */
extern const char lbl_8058D938[];
extern const char lbl_8058D94C[];
extern const char lbl_8058D984[];
extern const char lbl_8058D9C4[];
extern const char lbl_8058D9FC[];
extern const char lbl_8058DA3C[];
extern const char lbl_8058DA70[];
extern const char lbl_8058DC58[];
extern const char lbl_8058DC78[];
extern const char lbl_8058DC8C[];
extern const char lbl_8058DCA8[];
extern const char lbl_8058DCB8[];
extern const char lbl_8058DCE0[];
extern const char lbl_8058DCF0[];
extern const char lbl_8058DCFC[];
extern const char lbl_8058DD18[];
extern const char lbl_8058DD28[];
extern const char lbl_8058DD58[];
extern u32 lbl_807911A8;
extern u32 lbl_807911AC;
extern u32 lbl_807911B0;

extern "C" {

/* -------- the helpers this unit calls (still unsplit `fn_XXXXXXXX`, C linkage) -------- */
f32 fn_80050BC0(f32 value);                      /* reciprocal-square-root / length helper */
u32* fn_8005D1AC(void* pDst, u32 value);         /* checked pointer wrapper */
u32 fn_8005AAEC(void* self);
u32* fn_8005D0C4(void* self);
void fn_80069CF4(void* p0, void* p1);

/* -------- this unit's own bodies -------- */
void fn_8006F738(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_8006F898(void* p0, void* p1, void* p2, void* p3, void* p4);
void fn_8006F8D4(void* p0, void* p1, void* p2, void* p3);
f32 fn_8006F908(const f32* pMtx, u32 idx);
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_8006FB60(void* p0, void* p1, void* p2, void* p3, f32 p4, f32 p5, f32 p6);
void fn_8006FBBC(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
u32 fn_8006FDCC(void* self);
void fn_8006FE40(void* self);
u32 fn_8006FE7C(void* self, u32 off);
u32 fn_8006FEC8(void* self, u32 index);
u32 fn_8006FF50(void* self);
u32 fn_8006FFB4(void* self);
const char* fn_8006FFBC(void);
u32 fn_8006FFC8(void* self);
u32 fn_8006FFDC(void* self);
u32 fn_80070020(void* self);
u32* fn_80070054(u32* pDst, u32 ptr);
void fn_800700B8(u32* pDst, u32 value);
u32 fn_800700C0(void* self);
u32 fn_80070124(void* self);
void* fn_8007012C(void);
void fn_80070134(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80070410(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80070600(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80070820(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80071008(void* p);
void fn_8007100C(void* pDst, const void* pSrc);
void fn_80071064(void* p);
void fn_800710A0(void* p);
void fn_800710B4(void* p);
void fn_800710BC(void* pDst, void* pA, void* pB);
void fn_80071130(void* p);
void fn_8007118C(void* p);
void fn_80071198(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_80071B70(void* p);
void fn_80071BD0(void* p);
void fn_80071BD4(void* p);
void fn_80071C34(void* p);
void fn_80071C38(void* p);
void fn_80071C3C(void* p);
void fn_80071C40(void* p);
void fn_80071C48(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7);
void fn_800726D0(void* p);

/* -------- reconstructed bodies -------- */

/* 0x8006FDCC - the model's node-list accessor. */
u32 fn_8006FDCC(void* self) {
    if (!fn_8005AAEC(self)) {
        nw4r::db::Panic(lbl_8058DC78, 83, lbl_8058DC58);
    }
    if (!fn_8005AAEC(self)) {
        return 0;
    }
    return *(u32*)((u8*)fn_8005D0C4(self) + 16);
}

/* 0x8006FE7C - a checked resource pointer from a base handle and an offset (0 -> null). */
u32 fn_8006FE7C(void* self, u32 off) {
    u32 base = *(u32*)self;
    if (off == 0) {
        u32 tmp;
        return *fn_8005D1AC(&tmp, 0);
    }
    u32 tmp;
    return *fn_8005D1AC(&tmp, base + off);
}

/* 0x8006F908 - the length of column `idx` of a 48-byte (12-float) matrix. */
f32 fn_8006F908(const f32* pMtx, u32 idx) {
    const f32* p = (const f32*)((const u8*)pMtx + idx * 4);
    f32 a = p[8] * p[8];
    f32 b = p[0] * p[0];
    f32 c = p[4] * p[4];
    return fn_80050BC0(a + (b + c));
}

/* 0x8006FFB4 - dereference the handle. */
u32 fn_8006FFB4(void* self) {
    return *(u32*)self;
}

/* 0x8006FFBC - the "NodeTree" name string. */
const char* fn_8006FFBC(void) {
    return lbl_8058DCF0;
}

/* 0x8006FFC8 - is the handle non-null. */
u32 fn_8006FFC8(void* self) {
    return *(u32*)self != 0;
}

/* 0x800700B8 - store the 4-byte handle. */
void fn_800700B8(u32* pDst, u32 value) {
    *pDst = value;
}

/* 0x80070054 - the 4-byte-aligned resource pointer constructor (g3d_resmdl_ac.h). */
u32* fn_80070054(u32* pDst, u32 ptr) {
    fn_800700B8(pDst, ptr);
    if (ptr & 0x3) {
        nw4r::db::Panic(lbl_8058DCE0, 57, lbl_8058DCB8);
    }
    return pDst;
}

/* 0x80070020 - the checked pointer the model info lives at. */
u32 fn_80070020(void* self) {
    u8* p = (u8*)fn_800700C0(self);
    u32 handle;
    return *fn_80070054((u32*)&handle, (u32)(p + 76));
}

/* 0x80070124 - dereference the handle. */
u32 fn_80070124(void* self) {
    return *(u32*)self;
}

/* 0x8007012C - the shared null resource record. */
void* fn_8007012C(void) {
    return &lbl_807911B0;
}

/* -------- registration stubs (reconstruction pending) -------- */
u32 fn_800700C0(void* self) {
    (void)self;
    return 0;
}
void fn_8006F738(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006F898(void* p0, void* p1, void* p2, void* p3, void* p4) {}
void fn_8006F8D4(void* p0, void* p1, void* p2, void* p3) {}
void fn_8006F934(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006FB60(void* p0, void* p1, void* p2, void* p3, f32 p4, f32 p5, f32 p6) {}
void fn_8006FBBC(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_8006FE40(void* self) {}
u32 fn_8006FEC8(void* self, u32 index) {
    (void)self;
    (void)index;
    return 0;
}
u32 fn_8006FF50(void* self) {
    (void)self;
    return 0;
}
u32 fn_8006FFDC(void* self) {
    (void)self;
    return 0;
}
void fn_80070134(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80070410(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80070600(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80070820(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80071008(void* p) {}
void fn_8007100C(void* pDst, const void* pSrc) {}
void fn_80071064(void* p) {}
void fn_800710A0(void* p) {}
void fn_800710B4(void* p) {}
void fn_800710BC(void* pDst, void* pA, void* pB) {}
void fn_80071130(void* p) {}
void fn_8007118C(void* p) {}
void fn_80071198(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_80071B70(void* p) {}
void fn_80071BD0(void* p) {}
void fn_80071BD4(void* p) {}
void fn_80071C34(void* p) {}
void fn_80071C38(void* p) {}
void fn_80071C3C(void* p) {}
void fn_80071C40(void* p) {}
void fn_80071C48(void* p0, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6, u32 p7) {}
void fn_800726D0(void* p) {}

}  // extern "C"
