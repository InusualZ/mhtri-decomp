/*
 * g3d/g3d_resfile.cpp - nw4r g3d `ResFile` container checks/releases and the out-of-line `g3d_resmat_ac.h` accessors.
 * RANGE. .text 0x80093990-0x800947A4 (53 functions); extab, extabindex.  fn_80093990's `CheckRevision()` assert
 *   passes "g3d_resfile.cpp" (lbl_80590BA0); the next body, fn_800947A4, cites "g3d_resmat.cpp".
 * NAMES. res_mat_ind_mtx_copy_to is a GUESS; res_mat_ind_mtx_dc_store is a GUESS; res_mat_pix_copy_to is a GUESS;
 *   res_mat_pix_dc_store is a GUESS; res_mat_tev_color_copy_to is a GUESS; res_mat_tev_color_dc_store is a GUESS;
 *   res_mat_tex_coord_gen_copy_to is a GUESS; res_mat_tex_coord_gen_dc_store is a GUESS (the evidence follows).
 *   Map stems (the dump answers `zz_` placeholders); res_mat_{pix,tev_color,ind_mtx,tex_coord_gen}_dc_store
 *   and res_mat_{pix,tev_color,ind_mtx,tex_coord_gen}_copy_to are GUESSES (nw4r's DCStore/CopyTo, by the block
 *   sizes 0x20/0x80/0x40/0xA0 and the ScnMdl buffer refill's per-flag order); GXFastCallDisplayList is a GUESS (the inline that
 *   writes the call-display-list command straight into the GX FIFO).
 * RESIDUALS. none.  The flip links because `tools/elf/objextab.py` gives the object's extab/extabindex entries the
 *   map's `@etb_`/`@eti_` names with global binding (`@eti_800222FC` is referenced from the `.data` blob at
 *   0x8057C820; playbook 59).
 * SHAPES. The `.sdata` "ref" strings are sized externs (`extern char lbl_80791294[4];`): MWCC then emits
 *   `li rN, @sda21`, not `lis`/`addi`.
 */

#include "types.h"
#include "gx.h"                  /* GXWGFifo (rule 1) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "g3d/g3d_resanmtexsrt.h" /* the ten category accessors (rule 2) */
#include "g3d/fn_80075DCC.h"     /* fn_80076xxx/0x8007B878 (rule 2) */
#include "g3d/fn_800680CC.h"     /* fn_800695EC/0x8006CDBC/0x8006993C/0x8006E6B4 (rule 2) */
#include "g3d/fn_80063888.h"     /* fn_8006584C/0x80063FD0 (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* fn_8006F158/0x8006F298 (rule 2) */
#include "g3d/g3d_anmvis.h"      /* fn_8006EC3C (rule 2) */
#include "g3d/g3d_anmchr.h"      /* nw4r::g3d::ResAnmChr (rule 2) */
#include "g3d/g3d_calcview.h"    /* fn_800700C0 (rule 2) */
#include "g3d/g3d_state.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resfile.h"
#include "g3d/g3d_resmat.h"

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* The panic file/format strings the target references as map symbols: lbl_80590BA0/lbl_80590BB0 are in
 * `g3d/g3d_resanmtexsrt.cpp`'s `.data`, lbl_80591280/lbl_80591294 are unclaimed. */
extern const char lbl_80590BA0[]; /* "g3d_resfile.cpp" */
extern const char lbl_80590BB0[]; /* "NW4R:Failed assertion CheckRevision()" */
extern const char lbl_80591280[]; /* "ResMatTexCoordGen" */
extern const char lbl_80591294[]; /* "%s::%s: Object not valid." */
extern const char lbl_805912B0[]; /* "g3d_resmat_ac.h" */
extern const char lbl_805912C0[]; /* "%s::%s: Object not valid." */
extern const char lbl_805912E0[]; /* "g3d_resmat_ac.h" */
extern const char lbl_805912F0[]; /* "ResMatIndMtxAndScale" */
extern const char lbl_80591308[]; /* "%s::%s: Object not valid." */
extern const char lbl_80591328[]; /* "g3d_resmat_ac.h" */
extern const char lbl_80591338[]; /* "%s::%s: Object not valid." */
extern const char lbl_80591358[]; /* "g3d_resmat_ac.h" */
extern const char lbl_80591368[]; /* "ResMatTevColor" */
extern const char lbl_80591378[]; /* "%s::%s: Object not valid." */
extern const char lbl_80591398[]; /* "g3d_resmat_ac.h" */
extern const char lbl_805913A8[]; /* "%s::%s: Object not valid." */
extern const char lbl_805913C8[]; /* "g3d_resmat_ac.h" */
extern const char lbl_805913D8[]; /* "ResMatPix" */
extern const char lbl_805913E4[]; /* "%s::%s: Object not valid." */
extern const char lbl_80591400[]; /* "g3d_resmat_ac.h" */
extern const char lbl_80591410[]; /* "%s::%s: Object not valid." */
extern const char lbl_80591430[]; /* "g3d_resmat_ac.h" */

/* The `"ref"` member-name strings of the accessor asserts (unsplit `.sdata`).  Declared as *sized*
 * 4-byte arrays so MWCC's small-data heuristic emits the target's `li rN, @sda21` address form (an
 * unsized `extern const char[]` is assumed large and gets `lis`/`addi` - the same lever as
 * `g3d/g3d_anmvis.cpp`'s `lbl_807911A0`). */
extern char lbl_80791290[4];
extern char lbl_80791294[4];
extern char lbl_80791298[4];
extern char lbl_8079129C[4];
extern char lbl_807912A0[4];
extern char lbl_807912A4[4];
extern char lbl_807912A8[4];
extern char lbl_807912AC[4];

/* ------------------------------------------------------------------------------------------------ */
/* The `ResFile` release helpers (fn_8009A360 is `g3d/g3d_resshp.cpp`'s; declared here) and the SDK's
 * `GXCallDisplayList`. */
extern "C" u32 fn_8009A360(void* p, u32 value);
extern "C" u32 fn_8009A3CC(void* p);
extern "C" u32 fn_8009A6C0(void* p);
extern "C" void fn_8009A720(void* p);
extern "C" void GXCallDisplayList(void* pList, u32 size);

/* ------------------------------------------------------------------------------------------------ */
/* The types this unit names.                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The revision word each resource-type check reads out of a resolved block: only +0x8 is touched
 * here, so the two leading words are modelled as opaque padding (approximate prefix). */
typedef struct {
    /* +0x0 */ u32 pad_0x0;
    /* +0x4 */ u32 pad_0x4;
    /* +0x8 */ u32 revision;
} ResRevisionWord; /* size: 0xC (approximate - only +0x8 is evidenced) */

/* The global material table `fn_8005A9BC` returns; only +0x4 is read here. */
typedef struct {
    /* +0x0 */ u32 pad_0x0;
    /* +0x4 */ u32 field_0x4;
} ResMaterialTable; /* size: 0x8 (approximate - only +0x4 is evidenced) */

/* ------------------------------------------------------------------------------------------------ */
/* The unit's own functions, in address order.                                                        */
/* ------------------------------------------------------------------------------------------------ */

extern "C" u32 fn_80093B0C(ResHandle* pSelf);
extern "C" u32 fn_80093A98(void* p);
extern "C" u32 fn_80093E14(ResHandle* p);
extern "C" u32 fn_80093E40(ResHandle* p);
extern "C" u32 fn_80093E64(ResHandle* p);
extern "C" u32 fn_80093E90(ResHandle* p);
extern "C" u32 fn_80093EB4(ResHandle* p);
extern "C" u32 fn_80093EE0(ResHandle* p);
extern "C" u32 fn_80093F04(ResHandle* p);
extern "C" u32 fn_80093F30(ResHandle* p);
extern "C" u32 fn_80093F54(ResHandle* p);
extern "C" u32 fn_80093F80(ResHandle* p);
extern "C" u32 fn_80093FA4(ResHandle* p);
extern "C" u32 fn_80093FD0(ResHandle* p);
extern "C" u32 fn_80093FF4(ResHandle* p);
extern "C" u32 fn_80094020(ResHandle* p);
extern "C" u32 fn_80094044(ResHandle* p);
extern "C" u32 fn_80094070(ResHandle* p);
extern "C" u32 fn_800940D0(ResHandle* pSelf, u32 offset);
extern "C" u32 fn_800941C8(ResHandle* p);
extern "C" const char* fn_800941D0(void);
extern "C" u32 fn_80094288(ResHandle* p);
extern "C" const char* fn_80094290(void);
extern "C" u32 fn_80094348(ResHandle* p);
extern "C" const char* fn_80094350(void);
extern "C" u32 fn_800943A4(ResHandle* p);
extern "C" u32 fn_80094408(ResHandle* p);
extern "C" const char* fn_80094410(void);
extern "C" u32 fn_800944E0(ResHandle* p);
extern "C" u32 fn_80094594(ResHandle* p);
extern "C" u32 fn_80094600(ResHandle* p);
extern "C" u32 fn_8009466C(ResHandle* p);

/* 0x80093990 - the file-checked destructor walk: assert `CheckRevision()`, then release each of the
 * three object categories through its own helper. */
extern "C" void fn_80093990(ResHandle* pSelf) {
    if (fn_80093B0C(pSelf) == 0) {
        nw4r::db::Panic(lbl_80590BA0, 0xDF, lbl_80590BB0);
    }

    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        reinterpret_cast<nw4r::g3d::ResMdl*>(&item)->Init();
    }

    count = fn_80092B10(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092B0C(pSelf, i);
        fn_8009A720(&item);
    }

    count = fn_8009284C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092848(pSelf, i);
        fn_80093A98(&item);
    }
}

/* 0x80093A98 - release one entry of the third category (tail call). */
extern "C" u32 fn_80093A98(void* p) {
    return fn_8009A360(p, 0);
}

/* 0x80093AA0 - release every entry of the first category through `ResMdl::Terminate`. */
extern "C" void fn_80093AA0(ResHandle* pSelf) {
    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        reinterpret_cast<nw4r::g3d::ResMdl*>(&item)->Terminate();
    }
}

/* 0x80093B0C - `ResFile::CheckRevision()`: walk all ten categories and report whether every entry
 * passes its own revision check. */
extern "C" u32 fn_80093B0C(ResHandle* pSelf) {
    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        if (fn_80094044(&item) == 0) {
            return 0;
        }
    }

    count = fn_80092B10(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092B0C(pSelf, i);
        if (fn_8009A6C0(&item) == 0) {
            return 0;
        }
    }

    count = fn_8009284C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092848(pSelf, i);
        if (fn_8009A3CC(&item) == 0) {
            return 0;
        }
    }

    count = fn_80092CC4(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092CC0(pSelf, i);
        if (fn_80093FF4(&item) == 0) {
            return 0;
        }
    }

    count = fn_80092E78(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092E74(pSelf, i);
        if (fn_80093FA4(&item) == 0) {
            return 0;
        }
    }

    count = fn_8009302C(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80093028(pSelf, i);
        if (fn_80093F54(&item) == 0) {
            return 0;
        }
    }

    count = fn_800931E0(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_800931DC(pSelf, i);
        if (fn_80093F04(&item) == 0) {
            return 0;
        }
    }

    count = fn_80093394(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80093390(pSelf, i);
        if (fn_80093EB4(&item) == 0) {
            return 0;
        }
    }

    count = fn_80093548(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80093544(pSelf, i);
        if (fn_80093E64(&item) == 0) {
            return 0;
        }
    }

    count = fn_80093690(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_8009368C(pSelf, i);
        if (fn_80093E14(&item) == 0) {
            return 0;
        }
    }

    return 1;
}

/* The ten per-category revision checks: each compares the category's revision word against the
 * revision the loader accepts. */
extern "C" u32 fn_80093E14(ResHandle* p) {
    return fn_80093E40(p) == 5;
}

extern "C" u32 fn_80093E40(ResHandle* p) {
    return ((ResRevisionWord*)fn_8006584C(p))->revision;
}

extern "C" u32 fn_80093E64(ResHandle* p) {
    return fn_80093E90(p) == 4;
}

extern "C" u32 fn_80093E90(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_800695EC(p))->revision;
}

extern "C" u32 fn_80093EB4(ResHandle* p) {
    return fn_80093EE0(p) == 5;
}

extern "C" u32 fn_80093EE0(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006CDBC(p))->revision;
}

extern "C" u32 fn_80093F04(ResHandle* p) {
    return fn_80093F30(p) == 4;
}

extern "C" u32 fn_80093F30(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006993C(p))->revision;
}

extern "C" u32 fn_80093F54(ResHandle* p) {
    return fn_80093F80(p) == 4;
}

extern "C" u32 fn_80093F80(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_80063FD0(p))->revision;
}

extern "C" u32 fn_80093FA4(ResHandle* p) {
    return fn_80093FD0(p) == 4;
}

extern "C" u32 fn_80093FD0(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_8006EC3C(p))->revision;
}

extern "C" u32 fn_80093FF4(ResHandle* p) {
    return fn_80094020(p) == 5;
}

extern "C" u32 fn_80094020(ResHandle* p) {
    return reinterpret_cast<const nw4r::g3d::ResAnmChr*>(p)->ref().revision;
}

extern "C" u32 fn_80094044(ResHandle* p) {
    return fn_80094070(p) == 11;
}

extern "C" u32 fn_80094070(ResHandle* p) {
    return ((ResRevisionWord*)(u32)(u32)&reinterpret_cast<const nw4r::g3d::ResMdl*>(p)->ref())->revision;
}

/* 0x80094094 - resolve the global material table and read the resource at its +0x4 offset. */
extern "C" u32 fn_80094094(ResHandle* pSelf) {
    return fn_800940D0(pSelf, ((ResMaterialTable*)(u32)(u32)&reinterpret_cast<nw4r::g3d::ResMat*>(pSelf)->ref())->field_0x4);
}

/* 0x800940D0 - return the handle stored at `self->mpData + offset` (or the null handle at offset 0). */
extern "C" u32 fn_800940D0(ResHandle* pSelf, u32 offset) {
    u32 data = (u32)pSelf->mpData;

    if (offset != 0) {
        u32 found;
        return *(u32*)fn_8007B878((s32)&found, (s32)(data + offset));
    } else {
        u32 zero;
        return *(u32*)fn_8007B878((s32)&zero, 0);
    }
}

/* 0x8009411C - `ResMatPix` `ref()` plus the 0x20-byte range store. */
extern "C" void res_mat_pix_dc_store(ResHandle* pSelf, s32 flag) {
    u32 data = (u32)&reinterpret_cast<nw4r::g3d::ResMatPix*>(pSelf)->ref();

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)data, 0x20);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)data, 0x20);
    }
}

/* 0x80094164 - the checked `ResMatPix::ref()` accessor. */
nw4r::g3d::ResMatPixData& nw4r::g3d::ResMatPix::ref() {
    if (IsValid() == 0) {
        nw4r::db::Panic(lbl_80591400, 0x154, lbl_805913E4, fn_800941D0(), lbl_80791294);
    }
    return *(nw4r::g3d::ResMatPixData*)fn_800941C8((ResHandle*)this);
}

/* 0x800941C8 - the `ref()` word. */
extern "C" u32 fn_800941C8(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x800941D0 - the class-name string of the accessor's assert. */
extern "C" const char* fn_800941D0(void) {
    return lbl_805913D8;
}

/* 0x800941DC - `ResMatTevColor` `ref()` plus the 0x80-byte range store. */
extern "C" void res_mat_tev_color_dc_store(ResHandle* pSelf, s32 flag) {
    u32 data = (u32)&reinterpret_cast<nw4r::g3d::ResMatTevColor*>(pSelf)->ref();

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)data, 0x80);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)data, 0x80);
    }
}

/* 0x80094224 - the checked `ResMatTevColor::ref()` accessor. */
nw4r::g3d::ResMatTevColorData& nw4r::g3d::ResMatTevColor::ref() {
    if (!IsValid()) {
        nw4r::db::Panic(lbl_80591398, 0x17A, lbl_80591378, fn_80094290(), lbl_8079129C);
    }
    return *(nw4r::g3d::ResMatTevColorData*)fn_80094288((ResHandle*)this);
}

/* 0x80094288 - the `ref()` word. */
extern "C" u32 fn_80094288(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094290 - the class-name string of the accessor's assert. */
extern "C" const char* fn_80094290(void) {
    return lbl_80591368;
}

/* 0x8009429C - `ResMatIndMtxAndScale` `ref()` plus the 0x40-byte range store. */
extern "C" void res_mat_ind_mtx_dc_store(ResHandle* pSelf, s32 flag) {
    u32 data = (u32)&reinterpret_cast<nw4r::g3d::ResMatIndMtxAndScale*>(pSelf)->ref();

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)data, 0x40);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)data, 0x40);
    }
}

/* 0x800942E4 - the checked `ResMatIndMtxAndScale::ref()` accessor. */
nw4r::g3d::ResMatIndMtxAndScaleData& nw4r::g3d::ResMatIndMtxAndScale::ref() {
    if (IsValid() == 0) {
        nw4r::db::Panic(lbl_80591328, 0x19D, lbl_80591308, fn_80094350(), lbl_807912A4);
    }
    return *(nw4r::g3d::ResMatIndMtxAndScaleData*)fn_80094348((ResHandle*)this);
}

/* 0x80094348 - the `ref()` word. */
extern "C" u32 fn_80094348(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094350 - the class-name string of the accessor's assert. */
extern "C" const char* fn_80094350(void) {
    return lbl_805912F0;
}

/* 0x8009435C - `ResMatTexCoordGen` `ref()` plus the 0xA0-byte range store. */
extern "C" void res_mat_tex_coord_gen_dc_store(ResHandle* pSelf, s32 flag) {
    u32 data = fn_800943A4(pSelf);

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)data, 0xA0);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)data, 0xA0);
    }
}

/* 0x800943A4 - the checked `ResMatTexCoordGen::ref()` accessor. */
extern "C" u32 fn_800943A4(ResHandle* pSelf) {
    if (reinterpret_cast<const nw4r::g3d::ResMatTexCoordGen*>(pSelf)->IsValid() == 0) {
        nw4r::db::Panic(lbl_805912B0, 0x201, lbl_80591294, fn_80094410(), lbl_807912AC);
    }
    return fn_80094408(pSelf);
}

/* 0x80094408 - the `ref()` word. */
extern "C" u32 fn_80094408(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094410 - the class-name string of the accessor's assert. */
extern "C" const char* fn_80094410(void) {
    return lbl_80591280;
}

/* 0x8009441C (0x48): calls the pixel display list, through `GXCallDisplayList` when `bSync`. */
void nw4r::g3d::ResMatPix::CallDisplayList(bool flag) const {
    u32 data = (u32)&ref();

    if (flag != 0) {
        GXCallDisplayList((void*)data, 0x20);
    } else {
        GXFastCallDisplayList((const void*)data, 0x20);
    }
}

/* 0x80094464 - the inline `GXCallDisplayList` pipe command (opcode 0x40, then address and size). */
/* untyped: byte range */
extern "C" void GXFastCallDisplayList(const void* pList, u32 size) {
    GXWGFifo.u8 = 0x40;
    GXWGFifo.u32 = (u32)pList;
    GXWGFifo.u32 = size;
}

/* 0x8009447C - the checked `ResMatPix::ref()` accessor (second `_ac.h` instantiation). */
const nw4r::g3d::ResMatPixData& nw4r::g3d::ResMatPix::ref() const {
    if (IsValid() == 0) {
        nw4r::db::Panic(lbl_80591430, 0x154, lbl_80591410, fn_800941D0(), lbl_80791290);
    }
    return *(const nw4r::g3d::ResMatPixData*)fn_800944E0((ResHandle*)this);
}

/* 0x800944E0 - the `ref()` word. */
extern "C" u32 fn_800944E0(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x800944E8 - calls the TEV colour display list, through the FIFO or the fast path. */
void nw4r::g3d::ResMatTevColor::CallDisplayList(bool bSync) const {
    u32 data = (u32)&ref();

    if (bSync) {
        GXCallDisplayList((void*)data, 0x80);
    } else {
        GXFastCallDisplayList((const void*)data, 0x80);
    }
}

/* 0x80094530 - the checked `ResMatTevColor::ref()` accessor (second instantiation). */
const nw4r::g3d::ResMatTevColorData& nw4r::g3d::ResMatTevColor::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic(lbl_805913C8, 0x17A, lbl_805913A8, fn_80094290(), lbl_80791298);
    }
    return *(const nw4r::g3d::ResMatTevColorData*)fn_80094594((ResHandle*)this);
}

/* 0x80094594 - the `ref()` word. */
extern "C" u32 fn_80094594(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8009459C - the checked `ResMatIndMtxAndScale::ref()` accessor (second instantiation). */
const nw4r::g3d::ResMatIndMtxAndScaleData& nw4r::g3d::ResMatIndMtxAndScale::ref() const {
    if (IsValid() == 0) {
        nw4r::db::Panic(lbl_80591358, 0x19D, lbl_80591338, fn_80094350(), lbl_807912A0);
    }
    return *(const nw4r::g3d::ResMatIndMtxAndScaleData*)fn_80094600((ResHandle*)this);
}

/* 0x80094600 - the `ref()` word. */
extern "C" u32 fn_80094600(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094608 - the checked `ResMatTexCoordGen::ref()` accessor (second instantiation). */
const nw4r::g3d::ResMatTexCoordGenData& nw4r::g3d::ResMatTexCoordGen::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic(lbl_805912E0, 0x201, lbl_805912C0, fn_80094410(), lbl_807912A8);
    }
    return *(const nw4r::g3d::ResMatTexCoordGenData*)fn_8009466C((ResHandle*)this);
}

/* 0x8009466C - the `ref()` word. */
extern "C" u32 fn_8009466C(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094674 - copy 0x20 bytes of the `ResMatPix` block and return the 0x20-aligned handle. */
extern "C" u32 res_mat_pix_copy_to(ResHandle* pSelf, u32 pDst) {
    nw4r::g3d::detail::Copy32ByteBlocks((void*)pDst, (void*)fn_800944E0(pSelf), 0x20);

    return (u32)nw4r::g3d::ResMatPix((void*)pDst).mpData;
}

/* 0x800946C0 - copy 0x80 bytes of the `ResMatTevColor` block and return the 0x20-aligned handle. */
extern "C" u32 res_mat_tev_color_copy_to(ResHandle* pSelf, u32 pDst) {
    nw4r::g3d::detail::Copy32ByteBlocks((void*)pDst, (void*)fn_80094594(pSelf), 0x80);

    return (u32)nw4r::g3d::ResMatTevColor((void*)pDst).mpData;
}

/* 0x8009470C - copy 0x40 bytes of the `ResMatIndMtxAndScale` block and return the 0x20-aligned
 * handle. */
extern "C" u32 res_mat_ind_mtx_copy_to(ResHandle* pSelf, u32 pDst) {
    nw4r::g3d::detail::Copy32ByteBlocks((void*)pDst, (void*)fn_80094600(pSelf), 0x40);

    return (u32)nw4r::g3d::ResMatIndMtxAndScale((void*)pDst).mpData;
}

/* 0x80094758 - copy 0xA0 bytes of the `ResMatTexCoordGen` block and return the 0x20-aligned handle. */
extern "C" u32 res_mat_tex_coord_gen_copy_to(ResHandle* pSelf, u32 pDst) {
    nw4r::g3d::detail::Copy32ByteBlocks((void*)pDst, (void*)fn_8009466C(pSelf), 0xA0);

    return (u32)nw4r::g3d::ResMatTexCoordGen((void*)pDst).mpData;
}

