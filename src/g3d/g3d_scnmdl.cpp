/*
 * nw4r g3d: g3d_scnmdl.cpp - the ScnMdl scene-model object, its replaced-material (`mReplacement`)
 * buffers and the ScnMdl name-record cluster.
 * `.text` 0x8007C540-0x8007F0E4 (55 functions, 11172 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `8007C540`.
 *
 * Name - which evidence class decided it.  Class 1, a `__FILE__` string: the unit's own `.data`
 * fragment (0x8058EDA0-0x8058F0A0) opens on the bare source-file name `g3d_scnmdl.cpp`
 * (lbl_8058EDA0, 0x8058EDA0, referenced by fn_8007C540, by fn_8007E01C, by fn_8007E8B4 and by
 * fn_8007EE68), and the same fragment carries the inlined-assert header `g3d_resmdl_ac.h`
 * (lbl_8058F090, the `%s::%s: Object not valid.` guard's file argument in fn_8007D404) and the
 * `mReplacement.*Array` assert texts that name the class's own buffers
 * (`mReplacement.pixDLArray`, `.tevColorDLArray`, `.indMtxAndScaleDLArray`, `.texCoordGenDLArray`,
 * `.tevDataArray`).  The module is `g3d` (its link neighbours in config/RMHE08/splits.txt are all
 * nw4r g3d) and the `.cpp` suffix plus the C++ call sites make it C++ - `langcheck.py` agrees.
 *
 * Class 2 fails: `python tools/symbols/dumpmap.py lookup 0x8007C540` answers the placeholder
 * `zz_007c540_`, which is not evidence.
 *
 * Seams.  Left: 0x8007C540, where g3d/fn_80075DCC.cpp ends; it is that unit's proposal cap, not a
 * proven TU seam (its header says so).  Right: 0x8007F0E4, where g3d/g3d_scnmdlsmpl.cpp begins; it is
 * tudiscover's weak `codegen fingerprint change` cut AND the extent of this unit's proven match set
 * (`tudiscover.py at 0x8007C540`: "MATCH SET 48 functions, certainly one TU: 0x8007C540..0x8007EF1C").
 *
 * The 0x8007EF1C-0x8007F0E4 tail (7 functions) is allocated to this unit, not to a TU of its own:
 * its head function fn_8007EF1C is the ScnMdl *name-record reader* (the `*fn_800638B8(&local,
 * lbl_8056F678)` shape with lbl_8056F678 = "ScnMdl", the same shape g3d/g3d_anmvis.cpp's
 * fn_8006EE48 uses for "AnmObjVis"), and only the TU that defines ScnMdl can register that record.
 * The same argument assigns 0x800810DC-0x800813B8 to g3d_scnmdlsmpl.cpp ("ScnMdlSimple") and
 * 0x80082668-0x800827E4 to g3d_scnobj.cpp ("ScnObj"/"ScnLeaf"/"ScnGroup"); with that partition
 * `extab` (0x80008588-0x80008674), `extabindex` (0x80020E74-0x80020FB8) and `.text` are each
 * contiguous per unit and across the four units, which is the residual's own cross-check.
 *
 * Sections claimed: `.text` 0x8007C540-0x8007F0E4, `extab` 0x80008588-0x80008674 (236 B, 27 records:
 * 26 x 8 + 28 for fn_8007EE68), `extabindex` 0x80020E74-0x80020FB8 (324 B, 27 entries x 12).
 * The unit's `.data` fragment 0x8058EDA0-0x8058F0A0 (its `__FILE__` name, the assert texts, the
 * `%s::%s: Object not valid.` format, the ScnMdl vtable lbl_8058F028 and the `.rodata` name record
 * lbl_8056F678) and the `.sdata` word lbl_80791208 (`"ref"`) are NOT claimed here: a range may only
 * be claimed once the object emits it (docs/plan.md 8.4) and this pass reconstructs bodies only.
 * They are recorded in the outbox for the data pass.
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8007C540`, which answers the `zz_007c540_` placeholder,
 * and by reading every `.text` entry in 0x8007C540..0x8007F0E4 out of config/RMHE08/symbols.txt:
 * they are all bare `fn_XXXXXXXX`).  The one real name the range implies - `nw4r::g3d::ScnMdl` - is
 * a class name recovered from the `.rodata` name record and the assert texts; it is used as this
 * file's object type, never as a callable identifier.
 *
 * Measurement path: `python tools/units/recompile.py g3d/g3d_scnmdl.cpp --measure <symbol>` scores
 * each symbol against MAIN's retired per-function object that contains it (auto_fn_<ADDR>_text.o or
 * the auto_<nn>_<ADDR>_text.o run), compiled with the g3d lib's real command line (cflags_g3d).
 *
 * Reconstruction status: the small/medium bodies of the 0x8007D38C-0x8007F0CC span are written; the
 * seven large bodies (fn_8007C540, fn_8007D59C, fn_8007DDFC, fn_8007E01C, fn_8007E498, fn_8007E8B4,
 * fn_8007EA8C, fn_8007ED58) are registration stubs still to do.  Per-function scores: see the outbox
 * and .pi/notes/8007c540-fn-8007c540-567b.md.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* ResHandle, IS_VALID_PTR (rule 1) */
#include "nw4r/g3d/scnmdl.h"      /* nw4r::g3d::ScnMdl (rule 1: one definition, in include/) */
#include "g3d/fn_80063888.h"      /* fn_800638B8, fn_800639D0, G3dObj (rule 2: owner g3d/fn_80063888.cpp) */
#include "g3d/g3d_anmchr.h"       /* fn_8005D3E0, fn_8005DC24 (rule 2: owner g3d/g3d_anmchr.cpp) */
#include "g3d/fn_800680CC.h"      /* fn_800696E4, fn_800697A4 (rule 2: owner g3d/fn_800680CC.cpp) */
#include "g3d/fn_8005AA28.h"      /* fn_8005AB00 (rule 2: owner g3d/fn_8005AA28.cpp) */
#include "g3d/fn_80075DCC.h"      /* fn_8007B424..fn_800793A4 (rule 2: owner g3d/fn_80075DCC.cpp) */
#include "g3d/g3d_anmvis.h"      /* fn_8006ECB4/fn_8006ED84 (rule 2: owner g3d/g3d_anmvis.cpp) */
#include "g3d/g3d_calcview.h"     /* fn_8006FFBC/fn_8006FFC8 (rule 2: owner g3d/g3d_calcview.cpp) */
#include "g3d/g3d_calcvtx.h"       /* fn_8007270C/fn_800730D8/fn_800732F0 (rule 2: g3d_calcvtx.cpp) */
#include "g3d/g3d_scnmdlsmpl.h"   /* fn_8007F41C? and the ScnMdlSimple helpers (rule 2) */
#include "fn_80047398.h"          /* fn_800497AC (rule 2: owner fn_80047398.cpp) */

/* `ScnMdl` lives in its real namespace (rule 9's owner spelling); this unit's bodies name it short. */
using nw4r::g3d::ReplacementBlock;
using nw4r::g3d::ScnMdl;

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The panic file/format strings and data objects this unit's bodies reference.  They are `extern` map
 * labels, not literals: MWCC's `-str reuse` would pool a repeated literal into one blob addressed
 * through a shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058EDA0[]; /* "g3d_scnmdl.cpp" */
extern const char lbl_8058EDE4[]; /* "NW4R:Failed assertion ((u32)buf & 0x1f) == 0" */
extern const char lbl_8058EE14[]; /* "NW4R:Failed assertion pos.GetSize() == ResVtxPos(rep.vtxPosTable..." */
extern const char lbl_8058EE64[]; /* "...((u32)&mReplacement.pixDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEAC[]; /* "...((u32)&mReplacement.tevColorDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EEF8[]; /* "...((u32)&mReplacement.indMtxAndScaleDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF48[]; /* "...((u32)&mReplacement.texCoordGenDLArray[i] & 0x1f) == 0" */
extern const char lbl_8058EF98[]; /* "...((u32)&mReplacement.tevDataArray[i] & 0x1f) == 0" */
extern const char lbl_8058EFE0[]; /* "NW4R:Failed assertion !mpAnmObjShp" */
extern const char lbl_8058F004[]; /* "NW4R:Failed assertion !GetParent()" */
extern const char lbl_8058F028[]; /* the ScnMdl vtable (0x48 B) */
extern const char lbl_8058F070[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058F090[]; /* "g3d_resmdl_ac.h" */
extern char lbl_8056F678[];       /* the `.rodata` "ScnMdl" name record */
extern u32 lbl_80791208;          /* the `.sdata` word "ref" */

/* ------------------------------------------------------------------------------------------------ */
/* the object                                                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* The resource block `fn_800730D8`/fn_800732F0/fn_800696E4 hand back: its leading word only.  The
 * layout belongs to g3d/g3d_calcvtx.cpp (rule 1), which defines the full block. */
struct ResVtxBlockHead {
    /* +0x00 */ u32 mSize;
}; /* size: 0x04 */

/* ------------------------------------------------------------------------------------------------ */
/* this unit's own bodies, in address order (definitions follow)                                     */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

/* -------- the two g3d-band helpers with no registered owner (rule 2's documented gap: the band is
 * bracketed by a `g3d` unit and a `gx` unit, so it names no single module's header) -------- */
s32 fn_80097F80(void* pView);            /* 0x80097F80 - the model's node count */
s32 fn_80097F18(void* pView, u32 idx);   /* 0x80097F18 - one node handle of the table */
u32 fn_8009A2F4(void* pSelf, u32 flag);  /* 0x8009A2F4 - the pix-DL replacement's teardown */
u32 fn_8009435C(void* pSelf, u32 flag);  /* 0x8009435C - the tex-color-DL replacement's teardown */
u32 fn_8009411C(void* pSelf, u32 flag);  /* 0x8009411C - the ind-mtx/scale replacement's teardown */

u32 fn_8007D38C(u32 p);
u32 fn_8007D398(ResHandle* pSelf);
u32 fn_8007D3BC(ResHandle* pSelf);
u32 fn_8007D3E0(ResHandle* pSelf);
u32 fn_8007D404(ResHandle* pSelf);
u32 fn_8007D468(const ResHandle* pSelf);
u32 fn_8007D470(u32 p);
void fn_8007D47C(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
u32 fn_8007D568(ScnMdl* pSelf);
u32 fn_8007D570(ScnMdl* pSelf);
u32 fn_8007D588(ScnMdl* pSelf);
u32 fn_8007DB2C(ScnMdl* pSelf);
u32 fn_8007DB34(ScnMdl* pSelf);
u32* fn_8007DB3C(u32* pDst, const u32* pSrc);
void fn_8007DB6C(u32* pDst, const u32* pSrc);
u32* fn_8007DB78(u32* pDst, const u32* pSrc);
void fn_8007DBA8(u32* pDst, const u32* pSrc);
u32 fn_8007DBB4(ScnMdl* pSelf);
u32 fn_8007DBBC(ScnMdl* pSelf, u32 idx, u32 mask);
void fn_8007DBDC(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
u32 fn_8007DCB0(ScnMdl* pSelf);
u32 fn_8007DD8C(ScnMdl* pSelf);
void fn_8007DCB8(ScnMdl* pSelf, u32* pArg2, u32* pArg3);
void fn_8007DD94(ScnMdl* pSelf);
u32 fn_8007DDF4(ScnMdl* pSelf);
void fn_8007DF98(ScnMdl* pSelf, u32 type, u32 on);
u32 fn_8007DF9C(ScnMdl* pSelf, u32 type, u32 on);
u32 fn_8007DFDC(ScnMdl* pSelf, u32 type, u32* pOut);
u32 fn_8007E478(ScnMdl* pSelf);
u32 fn_8007E480(ScnMdl* pSelf);
u32 fn_8007E488(ScnMdl* pSelf);
void fn_8007E490(void);
void fn_8007E494(void);
void fn_8007E7FC(ScnMdl* pSelf);
void fn_8007EA08(ScnMdl* pSelf);
void* fn_8007EA10(ScnMdl* pSelf);
u32 fn_8007ECD8(ScnMdl* pSelf, u32 type);
u32 fn_8007ED28(ScnMdl* pSelf, u32 type);
u32 fn_8007ED40(ScnMdl* pSelf, u32 type);
u32 fn_8007ED58(ScnMdl* pSelf, void* pArg2, u32* pArg3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8,
                u32 a9, u32 a10, const ReplacementBlock* pReplacement, u32* pDLBuffer, u32 a13);
void* fn_8007EE68(ScnMdl* pSelf, s16 flag);
u32 fn_8007EF1C(void);
u32 fn_8007EF4C(G3dObj* pSelf);
u32 fn_8007EF84(void* pSelf, u32* pKey);
u32 fn_8007EFF0(void* pSelf, u32* pKey);
u32 fn_8007F05C(ScnMdl* pSelf);
u32 fn_8007F0BC(ScnMdl* pSelf);
u32 fn_8007F0CC(ScnMdl* pSelf, u32 id);

/* ------------------------------------------------------------------------------------------------ */
/* bodies                                                                                            */
/* ------------------------------------------------------------------------------------------------ */

/* 0x8007D38C - the block pointer aligned up to 4 (the `buf & 0x3` shape the asserts below test). */
u32 fn_8007D38C(u32 p) {
    return (p + 3) & 0xFFFFFFFC;
}

/* 0x8007D398 - the `ResVtxNrm` block's size word (`g3d_resvtx_ac.h`'s block accessor chain). */
u32 fn_8007D398(ResHandle* pSelf) {
    return ((ResVtxBlockHead*)fn_800730D8(pSelf))->mSize;
}

/* 0x8007D3BC - the `ResVtxClr` block's size word. */
u32 fn_8007D3BC(ResHandle* pSelf) {
    return ((ResVtxBlockHead*)fn_800732F0(pSelf))->mSize;
}

/* 0x8007D3E0 - the `ResVtxPos` block's size word. */
u32 fn_8007D3E0(ResHandle* pSelf) {
    return ((ResVtxBlockHead*)fn_800696E4(pSelf))->mSize;
}

/* 0x8007D404 - `ResCommon<ResMdl>::ref()`: the `g3d_resmdl_ac.h` inlined assert, then the handle. */
u32 fn_8007D404(ResHandle* pSelf) {
    if (fn_8006FFC8(pSelf) == 0) {
        nw4r::db::Panic(lbl_8058F090, 57, lbl_8058F070, fn_8006FFBC(), lbl_80791208);
    }
    return fn_8007D468(pSelf);
}

/* 0x8007D468 - the handle's resource pointer. */
u32 fn_8007D468(const ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8007D470 - the block pointer aligned up to 32 (the `buf & 0x1f` shape). */
u32 fn_8007D470(u32 p) {
    return (p + 31) & 0xFFFFFFE0;
}

/* 0x8007D47C - the model's per-node visibility pass: refresh the copied material, then either write the
 * per-node byte vector (the node buffer exists) or hand the node table to the animation object. */
void fn_8007D47C(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    fn_8007F41C(pSelf, pArg2, pArg3);
    if (fn_8007D588(pSelf) != 0 && fn_8007D570(pSelf) != 0) {
        fn_8007E7FC(pSelf);
    }
    if (fn_8007D568(pSelf) != 0) {
        if (pSelf->mReplacement.mpNodeVisible != 0) {
            u32 local = fn_8005AB00(pSelf);

            fn_8006ED84(pSelf->mReplacement.mpNodeVisible, &local, (u32*)fn_8007D568(pSelf));
            fn_8007C464(pSelf);
        } else {
            u32 local = fn_8005AB00(pSelf);

            fn_8006ECB4(&local, (void*)fn_8007D568(pSelf));
        }
    }
    fn_8007B564(pSelf, 4, pArg2, pArg3);
}

/* 0x8007D568 - the shape-animation object the node passes walk. */
u32 fn_8007D568(ScnMdl* pSelf) {
    return pSelf->mpAnmObjShp;
}

/* 0x8007D570 - `(mFlags & 1) != 0`, the visible bit. */
u32 fn_8007D570(ScnMdl* pSelf) {
    u32 flag = pSelf->mFlags & 1;

    return ((u32)(-(s32)flag) | flag) >> 31;
}

/* 0x8007D588 - `(mFlags & 2) == 0`, the shape-animation option's inverse. */
u32 fn_8007D588(ScnMdl* pSelf) {
    u32 flag = pSelf->mFlags & 2;

    return !flag;
}

/* 0x8007DB2C - the DL-buffer vertex count. */
u32 fn_8007DB2C(ScnMdl* pSelf) {
    return pSelf->mNumPixDL;
}

/* 0x8007DB34 - the indirect-matrix/scale DL count. */
u32 fn_8007DB34(ScnMdl* pSelf) {
    return pSelf->mNumIndMtxAndScaleDL;
}

/* 0x8007DB3C - the word copy that hands the destination back (the `mReplacement` setter's shape). */
u32* fn_8007DB3C(u32* pDst, const u32* pSrc) {
    fn_8007DB6C(pDst, pSrc);
    return pDst;
}

/* 0x8007DB6C - a one-word copy. */
void fn_8007DB6C(u32* pDst, const u32* pSrc) {
    *pDst = *pSrc;
}

/* 0x8007DB78 - the same copy shape for the second replacement buffer. */
u32* fn_8007DB78(u32* pDst, const u32* pSrc) {
    fn_8007DBA8(pDst, pSrc);
    return pDst;
}

/* 0x8007DBA8 - a one-word copy. */
void fn_8007DBA8(u32* pDst, const u32* pSrc) {
    *pDst = *pSrc;
}

/* 0x8007DBB4 - the tex-coord-gen DL count. */
u32 fn_8007DBB4(ScnMdl* pSelf) {
    return pSelf->mNumTevColorDL;
}

/* 0x8007DBBC - `(pSelf->mpDLBuffer[idx] & mask) != 0`, the per-entry option query. */
u32 fn_8007DBBC(ScnMdl* pSelf, u32 idx, u32 mask) {
    u32 value = pSelf->mpDLBuffer[idx] & mask;

    return ((u32)(-(s32)value) | value) >> 31;
}

/* 0x8007DBDC - the `pixDL` replacement pass: mark the array active, gather the model's counts and run
 * the draw-buffer builder, then mark it done. */
void fn_8007DBDC(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    u32 key;
    u32 handle;

    fn_8007B940(pSelf, 1, pArg2, pArg3);
    if (pArg3 != 0) {
        key = *pArg3;
    } else {
        key = fn_800497AC(pSelf);
    }
    handle = fn_8005AB00(pSelf);
    fn_800793A4((s32*)&handle, fn_80080B5C(pSelf), fn_80080BA8(pSelf), fn_80080C04(pSelf),
                fn_8007DCB0(pSelf), 0, (s32)&pSelf->mReplacement, key, handle);
    fn_8007B940(pSelf, 4, pArg2, pArg3);
}

/* 0x8007DCB0 - the tex-matrix count. */
u32 fn_8007DCB0(ScnMdl* pSelf) {
    return pSelf->mNumTexMtx;
}

/* 0x8007DCB8 - the `texCoordGen` replacement pass (the same shape as fn_8007DBDC, with the tex-coord
 * count in the parameter slot the other one gives to the pix-DL count). */
void fn_8007DCB8(ScnMdl* pSelf, u32* pArg2, u32* pArg3) {
    u32 key;
    u32 handle;

    fn_8007B8E4(pSelf, 1, pArg2, pArg3);
    if (pArg3 != 0) {
        key = *pArg3;
    } else {
        key = fn_800497AC(pSelf);
    }
    handle = fn_8005AB00(pSelf);
    fn_800793A4((s32*)&handle, fn_80080B5C(pSelf), fn_80080BA8(pSelf), fn_80080C04(pSelf), 0,
                fn_8007DD8C(pSelf), (s32)&pSelf->mReplacement, key, handle);
    fn_8007B8E4(pSelf, 4, pArg2, pArg3);
}

/* 0x8007DD8C - the tex-coord-generation count. */
u32 fn_8007DD8C(ScnMdl* pSelf) {
    return pSelf->mNumTexSrt;
}

/* 0x8007DD94 - the shape-blend driver hand-over: the model handle, the shape-animation object and the
 * replacement record's three tables. */
void fn_8007DD94(ScnMdl* pSelf) {
    if (fn_8007DDF4(pSelf) != 0) {
        u32 local = fn_8005AB00(pSelf);

        fn_8007270C(&local, (void*)fn_8007DDF4(pSelf), (const void**)pSelf->mReplacement.mpVtxPosTable,
                    (const void**)pSelf->mReplacement.mpClrTable,
                    (const void**)pSelf->mReplacement.mpTexTable);
    }
}

/* 0x8007DDF4 - the third replacement count. */
u32 fn_8007DDF4(ScnMdl* pSelf) {
    return pSelf->mpAnmObjShp;
}

/* 0x8007DF98 - a one-line tail thunk: pass every argument on to the shared option setter. */
void fn_8007DF98(ScnMdl* pSelf, u32 type, u32 on) {
    fn_80080A5C(pSelf, type, on);
}

/* 0x8007DF9C - the option setter for the one option this unit owns (bit 1 of mFlags, inverted); every
 * other option tail-calls the shared setter. */
u32 fn_8007DF9C(ScnMdl* pSelf, u32 type, u32 on) {
    if (type == 0x30001) {
        if (on != 0) {
            pSelf->mFlags &= ~2u;
        } else {
            pSelf->mFlags |= 2u;
        }
    } else {
        return fn_8007FFC4(pSelf, type, on);
    }
    return 1;
}

/* 0x8007DFDC - the matching option query. */
u32 fn_8007DFDC(ScnMdl* pSelf, u32 type, u32* pOut) {
    u32 flag;

    if (pOut == 0) {
        return 0;
    }
    if (type == 0x30001) {
        flag = pSelf->mFlags & 2;
        *pOut = !flag;
    } else {
        return fn_80080004(pSelf, type, pOut);
    }
    return 1;
}

/* 0x8007E478 - the pix-DL replacement's teardown (the shared destructor, flag 0). */
u32 fn_8007E478(ScnMdl* pSelf) {
    return fn_8009A2F4(pSelf, 0);
}

/* 0x8007E480 - the tex-color-DL replacement's teardown. */
u32 fn_8007E480(ScnMdl* pSelf) {
    return fn_8009435C(pSelf, 0);
}

/* 0x8007E488 - the indirect-matrix/scale replacement's teardown. */
u32 fn_8007E488(ScnMdl* pSelf) {
    return fn_8009411C(pSelf, 0);
}

/* 0x8007E490 - an empty body (`blr`): the range's no-op override. */
void fn_8007E490(void) {}

/* 0x8007E494 - an empty body (`blr`): the range's second no-op override. */
void fn_8007E494(void) {}

/* 0x8007E7FC - write one byte per node into the visible array (1 where the node is visible) and clear
 * the visible bit, which forces the next query to re-read the node table. */
void fn_8007E7FC(ScnMdl* pSelf) {
    u32 handle = fn_8005AB00(pSelf);
    u32 view;
    s32 numNodes;
    u32 i;

    fn_80077E34((s32)&view, &handle);
    numNodes = fn_80097F80(&view);
    if (pSelf->mReplacement.mpNodeVisible != 0) {
        for (i = 0; i < (u32)numNodes; i++) {
            s32 node = fn_80097F18(&view, i);

            if (fn_80078904((s32)&node) != 0) {
                pSelf->mReplacement.mpNodeVisible[i] = 1;
            } else {
                pSelf->mReplacement.mpNodeVisible[i] = 0;
            }
        }
    }
    pSelf->mFlags &= ~1u;
}

/* 0x8007EA08 - the animation-object setter's zero-filled subclass step. */
void fn_8007EA08(ScnMdl* pSelf) {
    fn_800649B4(pSelf, 4);
}

/* 0x8007EA10 - `DynamicCast`-shaped: resolve the caller's name record and hand the object back only
 * when the object's own type query accepts it. */
void* fn_8007EA10(ScnMdl* pSelf) {
    u32 ok = 0;

    if (pSelf != 0) {
        u32 local = fn_800697A4();

        if (pSelf->mpfn_0x08(&local) != 0) {
            ok = 1;
        }
    }
    return ok != 0 ? pSelf : 0;
}

/* 0x8007ECD8 - the option accessor that reads the shape-animation object instead of a flag. */
u32 fn_8007ECD8(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        u32 value = pSelf->mpAnmObjShp;

        pSelf->mpfn_0x38(value);
        return value;
    }
    return fn_800808C4(pSelf, type);
}

/* 0x8007ED28 - the `type == 5` fast path of the second option accessor. */
u32 fn_8007ED28(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        return pSelf->mpAnmObjShp;
    }
    return fn_800809A4(pSelf, type);
}

/* 0x8007ED40 - the same fast path for the setter side. */
u32 fn_8007ED40(ScnMdl* pSelf, u32 type) {
    if (type == 5) {
        return pSelf->mpAnmObjShp;
    }
    return fn_80080A00(pSelf, type);
}

/* 0x8007ED58 - the ScnMdl constructor: run the base constructor with the caller's two-word record, install
 * the vtable, clear the two empty-by-default members, take the DL buffer pointer, copy the 0x40-byte
 * replacement record in and store the trailing argument. */
u32 fn_8007ED58(ScnMdl* pSelf, void* pArg2, u32* pArg3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8,
                u32 a9, u32 a10, const ReplacementBlock* pReplacement, u32* pDLBuffer, u32 a13) {
    u32 args[3];

    args[2] = *pArg3;
    args[0] = a9;
    args[1] = a10;
    fn_80080C60(pSelf, pArg2, &args[2]);
    *(const char**)pSelf = lbl_8058F028;
    pSelf->mpAnmObjShp = 0;
    pSelf->mFlags = 0;
    pSelf->mpDLBuffer = pDLBuffer;
    pSelf->mReplacement = *pReplacement;
    pSelf->field_0x184 = a13;
    return (u32)pSelf;
}

/* 0x8007EE68 - the ScnMdl deleting destructor: install the vtable, assert the parent is gone, release
 * the shape animation object, run the base teardown and, for a positive flag, free the object. */
void* fn_8007EE68(ScnMdl* pSelf, s16 flag) {
    if (pSelf != 0) {
        *(const char**)pSelf = lbl_8058F028;
        if (fn_800600C0((u32*)pSelf) != 0) {
            nw4r::db::Panic(lbl_8058EDA0, 1627, lbl_8058F004);
        }
        if (pSelf->mpAnmObjShp != 0) {
            pSelf->mpfn_0x38(pSelf->mpAnmObjShp);
        }
        dtor_80080F7C(pSelf, 0);
        if ((s16)flag > 0) {
            fn_8005D3E0(pSelf);
        }
    }
    return pSelf;
}

/* 0x8007EF1C - the ScnMdl name record (`g3d_scnmdl.cpp`'s own type registration). */
u32 fn_8007EF1C(void) {
    void* local;

    return (u32)*fn_800638B8(&local, lbl_8056F678);
}

/* 0x8007EF4C - the vtable-dispatch wrapper: run the object's slot +0x14 and read the word back
 * through the `fn_8005DC24` helper. */
u32 fn_8007EF4C(G3dObj* pSelf) {
    u32 tmp = pSelf->vt->method_0x14(pSelf);

    return fn_8005DC24(&tmp);
}

/* 0x8007EF84 - one step of the name-record chain: resolve the ScnMdl record, compare the caller's key
 * against it and, on a miss, run the next step's insertion with a copy of the key word. */
u32 fn_8007EF84(void* pSelf, u32* pKey) {
    u32 res = fn_8007B764(pSelf);

    if (fn_800639D0((u32**)pKey, (u32**)&res)) {
        return 1;
    }
    {
        u32 local = *pKey;

        return fn_8007EFF0(pSelf, &local);
    }
}

/* 0x8007EFF0 - the chain's insertion step for an object that has no parent yet. */
u32 fn_8007EFF0(void* pSelf, u32* pKey) {
    u32 res = fn_8007B734(pSelf);

    if (fn_800639D0((u32**)pKey, (u32**)&res)) {
        return 1;
    }
    {
        u32 local = *pKey;

        return fn_8007BAF0(pSelf, &local);
    }
}

/* 0x8007F05C - drop the last material of the copied-material list (a no-op when the resource handle is
 * empty). */
u32 fn_8007F05C(ScnMdl* pSelf) {
    u32 count;

    if (fn_8007F0BC(pSelf) != 0) {
        return 0;
    }
    count = fn_8007B424(pSelf);
    pSelf->mpfn_0x38(count - 1);
    return 0;
}

/* 0x8007F0BC - `mResMdl == 0`: the resource handle is empty. */
u32 fn_8007F0BC(ScnMdl* pSelf) {
    return pSelf->mResMdl == 0;
}

/* 0x8007F0CC - dispatch the material id through slot +0x34 (the `CopiedMatAccess` constructor's
 * handle hand-over). */
u32 fn_8007F0CC(ScnMdl* pSelf, u32 id) {
    return pSelf->mpfn_0x34(pSelf->mResMdl, id);
}

} /* extern "C" */
