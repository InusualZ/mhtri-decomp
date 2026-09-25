/*
 * nw4r g3d: g3d_resfile.cpp - the `ResFile` translation unit, `.text` 0x80093990-0x800947A4
 * (53 functions, 0xE14 B).
 *
 * Naming - which evidence class decided it.  Class 1 decides: the range's own `.data` pool holds the
 * bare source-file name `g3d_resfile.cpp` (lbl_80590BA0 at 0x80590BA0), which is the file argument of
 * the `nw4r::db::Panic` assert at the head of fn_80093990 ("NW4R:Failed assertion CheckRevision()",
 * lbl_80590BB0).  The `Panic__Q24nw4r2dbFPCciPCce` relocation is a C++ mangling, so the unit is
 * `src/g3d/g3d_resfile.cpp` in the existing g3d lib (Wii/1.3, cflags_g3d).  Class 2 FAILS:
 * `dumpmap.py lookup 0x80093990` answers a `zz_XXXXXXXX_` placeholder, not a name.
 *
 * What the unit is.  The `ResFile` resource-file container: fn_80093990 / fn_80093AA0 / fn_80093B0C
 * walk the file's ten resource categories (count + item accessors, `fn_80092588`/`fn_80092584` and its
 * nine siblings), checking each entry (`fn_80094044` and the sibling revision checks) or releasing it.
 * The tail of the range (fn_80094164 onward) is the out-of-line copy of the `g3d_resmat_ac.h` inlined
 * accessors the TU instantiates - `ResMatPix` / `ResMatTevColor` / `ResMatIndMtxAndScale` /
 * `ResMatTexCoordGen` `ref()` guards plus their GX-copy wrappers.
 *
 * Section claim: `.text` 0x80093990-0x800947A4, `extab` 0x800093A8-0x800094E0, `extabindex`
 * 0x80022158-0x8002232C.  The boundaries are the functions before (fn_800938EC, `CleanUpTracks`) and
 * after (fn_800947A4, the first body of the next TU, which cites "g3d_resmat.cpp").
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80093990`, which answers a `zz_` placeholder, and with
 * config/RMHE08/symbols.txt, whose every `.text` entry in 0x80093990..0x800947A4 is a bare
 * `fn_XXXXXXXX`).  The map's stems stand and are used as the identifiers.
 *
 * Measurement path: registered here for the first time, so MAIN has no split object for the range;
 * the source compiles with the g3d lib's real command line and each symbol is scored with objdiff
 * `report generate` against the retired per-range objects under build/RMHE08/obj/.
 *
 * Reconstruction status: all 53 functions reconstructed and measured at 100.00 % (official report
 * metric) against the retired per-range split objects under build/RMHE08/obj/; the object's `.text`
 * (0xE14), `extab` (0x138) and `extabindex` (0x1D4) sizes all equal the target's.  No residual.  The
 * one codegen lever is the `.sdata` `"ref"` member-name string: it must be declared as a *sized*
 * 4-byte array (`extern char lbl_80791294[4];`) so MWCC's small-data heuristic emits the target's
 * `li rN, @sda21` form instead of `lis`/`addi` (the same lever as `g3d/g3d_anmvis.cpp`).
 */

#include "types.h"
#include "gx.h"                  /* GXWGFifo (rule 1) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "unsplit/g3d.h"         /* the ten category accessors (rule 2) */
#include "g3d/fn_80075DCC.h"     /* fn_80076xxx/0x8007B878 (rule 2) */
#include "g3d/fn_800680CC.h"     /* fn_800695EC/0x8006CDBC/0x8006993C/0x8006E6B4 (rule 2) */
#include "g3d/fn_80063888.h"     /* fn_8006584C/0x80063FD0 (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* fn_8006F158/0x8006F298 (rule 2) */
#include "g3d/g3d_anmvis.h"      /* fn_8006EC3C (rule 2) */
#include "g3d/g3d_anmchr.h"      /* fn_800618BC (rule 2) */
#include "g3d/g3d_calcview.h"    /* fn_800700C0 (rule 2) */
#include "g3d/g3d_calcvtx.h"     /* fn_800734E4 (rule 2) */
#include "g3d/g3d_state.h"       /* fn_80089690 (rule 2) */
#include "fn_80059550.h"         /* fn_8005A9BC (rule 2) */

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* The panic file/format strings the target references as map symbols (unsplit `.data`). */
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
/* The `ResFile` release helpers and the SDK display-list entry point.  Their addresses sit in the
 * unclaimed run between the registered `g3d_resfile.cpp` and the `gx` unit, so the bracketing bands
 * name different modules and there is no sound header home (rule 2's named gap); they stay declared
 * here.  `GXCallDisplayList` is the SDK's own symbol. */
extern "C" void fn_80098ACC(void* p);
extern "C" void fn_80098CF0(void* p);
extern "C" u32 fn_8009A360(void* p, u32 value);
extern "C" u32 fn_8009A3CC(void* p);
extern "C" u32 fn_8009A6C0(void* p);
extern "C" void fn_8009A720(void* p);
extern "C" void fn_8009A748(void* pDst, void* pSrc, u32 size);
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
extern "C" u32 fn_80094164(ResHandle* p);
extern "C" u32 fn_800941C8(ResHandle* p);
extern "C" const char* fn_800941D0(void);
extern "C" u32 fn_80094224(ResHandle* p);
extern "C" u32 fn_80094288(ResHandle* p);
extern "C" const char* fn_80094290(void);
extern "C" u32 fn_800942E4(ResHandle* p);
extern "C" u32 fn_80094348(ResHandle* p);
extern "C" const char* fn_80094350(void);
extern "C" u32 fn_800943A4(ResHandle* p);
extern "C" u32 fn_80094408(ResHandle* p);
extern "C" const char* fn_80094410(void);
extern "C" void fn_80094464(u32 addr, u32 size);
extern "C" u32 fn_8009447C(ResHandle* p);
extern "C" u32 fn_800944E0(ResHandle* p);
extern "C" u32 fn_80094530(ResHandle* p);
extern "C" u32 fn_80094594(ResHandle* p);
extern "C" u32 fn_8009459C(ResHandle* p);
extern "C" u32 fn_80094600(ResHandle* p);
extern "C" u32 fn_80094608(ResHandle* p);
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
        fn_80098ACC(&item);
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

/* 0x80093AA0 - release every entry of the first category through `fn_80098CF0`. */
extern "C" void fn_80093AA0(ResHandle* pSelf) {
    u32 count = fn_80092588(pSelf);
    for (u32 i = 0; i < count; i++) {
        ResHandle item;
        item.mpData = fn_80092584(pSelf, i);
        fn_80098CF0(&item);
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
    return ((ResRevisionWord*)fn_800618BC(p))->revision;
}

extern "C" u32 fn_80094044(ResHandle* p) {
    return fn_80094070(p) == 11;
}

extern "C" u32 fn_80094070(ResHandle* p) {
    return ((ResRevisionWord*)(u32)fn_800700C0(p))->revision;
}

/* 0x80094094 - resolve the global material table and read the resource at its +0x4 offset. */
extern "C" u32 fn_80094094(ResHandle* pSelf) {
    return fn_800940D0(pSelf, ((ResMaterialTable*)(u32)fn_8005A9BC(pSelf))->field_0x4);
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
extern "C" void fn_8009411C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_80094164(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)data, 0x20);
    } else {
        fn_80089690((void*)data, 0x20);
    }
}

/* 0x80094164 - the checked `ResMatPix::ref()` accessor. */
extern "C" u32 fn_80094164(ResHandle* pSelf) {
    if (fn_80076800(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591400, 0x154, lbl_805913E4, fn_800941D0(), lbl_80791294);
    }
    return fn_800941C8(pSelf);
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
extern "C" void fn_800941DC(ResHandle* pSelf, s32 flag) {
    u32 data = fn_80094224(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)data, 0x80);
    } else {
        fn_80089690((void*)data, 0x80);
    }
}

/* 0x80094224 - the checked `ResMatTevColor::ref()` accessor. */
extern "C" u32 fn_80094224(ResHandle* pSelf) {
    if (fn_80076750(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591398, 0x17A, lbl_80591378, fn_80094290(), lbl_8079129C);
    }
    return fn_80094288(pSelf);
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
extern "C" void fn_8009429C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_800942E4(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)data, 0x40);
    } else {
        fn_80089690((void*)data, 0x40);
    }
}

/* 0x800942E4 - the checked `ResMatIndMtxAndScale::ref()` accessor. */
extern "C" u32 fn_800942E4(ResHandle* pSelf) {
    if (fn_8006E6B4(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591328, 0x19D, lbl_80591308, fn_80094350(), lbl_807912A4);
    }
    return fn_80094348(pSelf);
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
extern "C" void fn_8009435C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_800943A4(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)data, 0xA0);
    } else {
        fn_80089690((void*)data, 0xA0);
    }
}

/* 0x800943A4 - the checked `ResMatTexCoordGen::ref()` accessor. */
extern "C" u32 fn_800943A4(ResHandle* pSelf) {
    if (fn_8007673C(pSelf) == 0) {
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

/* 0x8009441C - `ResMatPix` `ref()` plus a display-list call or the inline pipe writer. */
extern "C" void fn_8009441C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_8009447C(pSelf);

    if (flag != 0) {
        GXCallDisplayList((void*)data, 0x20);
    } else {
        fn_80094464(data, 0x20);
    }
}

/* 0x80094464 - the inline `GXCallDisplayList` pipe command (opcode 0x40, then address and size). */
extern "C" void fn_80094464(u32 addr, u32 size) {
    GXWGFifo.u8 = 0x40;
    GXWGFifo.u32 = addr;
    GXWGFifo.u32 = size;
}

/* 0x8009447C - the checked `ResMatPix::ref()` accessor (second `_ac.h` instantiation). */
extern "C" u32 fn_8009447C(ResHandle* pSelf) {
    if (fn_80076800(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591430, 0x154, lbl_80591410, fn_800941D0(), lbl_80791290);
    }
    return fn_800944E0(pSelf);
}

/* 0x800944E0 - the `ref()` word. */
extern "C" u32 fn_800944E0(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x800944E8 - `ResMatTevColor` `ref()` plus a display-list call or the inline pipe writer. */
extern "C" void fn_800944E8(ResHandle* pSelf, s32 flag) {
    u32 data = fn_80094530(pSelf);

    if (flag != 0) {
        GXCallDisplayList((void*)data, 0x80);
    } else {
        fn_80094464(data, 0x80);
    }
}

/* 0x80094530 - the checked `ResMatTevColor::ref()` accessor (second instantiation). */
extern "C" u32 fn_80094530(ResHandle* pSelf) {
    if (fn_80076750(pSelf) == 0) {
        nw4r::db::Panic(lbl_805913C8, 0x17A, lbl_805913A8, fn_80094290(), lbl_80791298);
    }
    return fn_80094594(pSelf);
}

/* 0x80094594 - the `ref()` word. */
extern "C" u32 fn_80094594(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8009459C - the checked `ResMatIndMtxAndScale::ref()` accessor (second instantiation). */
extern "C" u32 fn_8009459C(ResHandle* pSelf) {
    if (fn_8006E6B4(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591358, 0x19D, lbl_80591338, fn_80094350(), lbl_807912A0);
    }
    return fn_80094600(pSelf);
}

/* 0x80094600 - the `ref()` word. */
extern "C" u32 fn_80094600(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094608 - the checked `ResMatTexCoordGen::ref()` accessor (second instantiation). */
extern "C" u32 fn_80094608(ResHandle* pSelf) {
    if (fn_8007673C(pSelf) == 0) {
        nw4r::db::Panic(lbl_805912E0, 0x201, lbl_805912C0, fn_80094410(), lbl_807912A8);
    }
    return fn_8009466C(pSelf);
}

/* 0x8009466C - the `ref()` word. */
extern "C" u32 fn_8009466C(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x80094674 - copy 0x20 bytes of the `ResMatPix` block and return the 0x20-aligned handle. */
extern "C" u32 fn_80094674(ResHandle* pSelf, u32 pDst) {
    fn_8009A748((void*)pDst, (void*)fn_800944E0(pSelf), 0x20);

    u32 handle;
    return *fn_80076794(&handle, pDst);
}

/* 0x800946C0 - copy 0x80 bytes of the `ResMatTevColor` block and return the 0x20-aligned handle. */
extern "C" u32 fn_800946C0(ResHandle* pSelf, u32 pDst) {
    fn_8009A748((void*)pDst, (void*)fn_80094594(pSelf), 0x80);

    u32 handle;
    return *fn_8006F158(&handle, pDst);
}

/* 0x8009470C - copy 0x40 bytes of the `ResMatIndMtxAndScale` block and return the 0x20-aligned
 * handle. */
extern "C" u32 fn_8009470C(ResHandle* pSelf, u32 pDst) {
    fn_8009A748((void*)pDst, (void*)fn_80094600(pSelf), 0x40);

    u32 handle;
    return *fn_8006F298(&handle, pDst);
}

/* 0x80094758 - copy 0xA0 bytes of the `ResMatTexCoordGen` block and return the 0x20-aligned handle. */
extern "C" u32 fn_80094758(ResHandle* pSelf, u32 pDst) {
    fn_8009A748((void*)pDst, (void*)fn_8009466C(pSelf), 0xA0);

    u32 handle;
    return *fn_800766D0(&handle, pDst);
}

/*
 * Residuals: none.  Every symbol in the range is byte-identical to the target (100.00 % on the
 * official report metric; .text/extab/extabindex sizes equal the target's).
 */
