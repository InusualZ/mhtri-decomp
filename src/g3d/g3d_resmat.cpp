/*
 * nw4r g3d: g3d_resmat.cpp - the `ResMat`/`ResTexSrt` resource TU, `.text`
 * 0x800947A4-0x80098D5C (140 functions, 0x45B8 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal
 * `g3d/g3d_resmat.cpp`.  Naming - evidence class 1, a `__FILE__` string:
 * the first body's `nw4r::db::Panic` assert passes `.data` 0x80590D78 = "g3d_resmat.cpp"
 * (`python tools/symbols/dumpmap.py lookup 0x80590D78`), and the next function outside this
 * range - 0x80098D5C - starts `g3d_resnode.cpp` (tudiscover seam, class `source`).  So the
 * range is exactly the `g3d_resmat.cpp` TU and both edges are proven seams.  The module is
 * `g3d` (the registered `g3d_res*` neighbours) and the extension `.cpp` is the `__FILE__`
 * suffix.  `tools/units/langcheck.py` agrees: C++, conclusive.
 *
 * The map owns two mangled symbols in this range - `ResTexSrt::SetEffectMtx` and its const
 * `GetEffectMtx` twin (`Q34nw4r3g3d9ResTexSrt`) - and 138 bare `fn_XXXXXXXX` stems.  The
 * mangled pair is written through its owner class (`g3d/g3d_resmat.h`, rule 9);
 * the rest keep the map's stems (Naming note below).
 *
 * Sections: `.text` 0x800947A4-0x80098D5C, `extab` 0x800094E0-0x80009850 (110 8-byte
 * unwind-only records) and `extabindex` 0x8002232C-0x80022854 (110 12-byte records).  The
 * extab extent is the contiguous run that starts at the previous TU's end (0x80009108) and
 * ends exactly where `gx/fn_8009AA78.c`'s extab begins (0x800099F0); our 110 records sit in
 * its middle, in `.text` order.  No `.ctors`/`.dtors` word points into the range.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` on the range's addresses: only the two
 * `ResTexSrt` manglings resolve to a real name, every other entry is an `fn_` placeholder),
 * so there is no real name to recover and stylelint's rule 7 refuses the landing without
 * this line.
 *
 * Residuals are recorded per function as the reconstruction proceeds (see the unit notes);
 * the measured percentages are in the outbox.
 */

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"           /* ResHandle (rule 1) */
#include "g3d/g3d_resmat.h"           /* nw4r::g3d::ResTexSrt, our owner type (rule 9) */
#include "g3d/fn_80075DCC.h"               /* fn_800768DC, owner g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/fn_800680CC.h"               /* fn_8006E2AC, owner g3d/fn_800680CC.cpp (rule 2) */
#include "g3d/g3d_state.h"                 /* fn_80087870, owner g3d/g3d_state.cpp (rule 2) */
#include "fn_8004CAD8.h"                   /* mtx34_identity, owner fn_8004CAD8.cpp (rule 2) */
#include "unsplit/g3d.h"                   /* fn_8007100C, no registered owner (rule 2) */

/* nw4r::db::Panic - the assert failure handler (variadic).  Called through its namespace
 * owner, never its mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pMsg, ...);

} /* namespace db */
} /* namespace nw4r */

/* The `.data` strings the retail asserts pass by address (unsplit `.data`). */
extern u8 lbl_80590D78[]; /* "g3d_resmat.cpp"                                                      */
extern u8 lbl_80590D88[]; /* "NW4R:Failed assertion IsValid()"                                    */
extern u8 lbl_80590DA8[]; /* "NW4R:Failed assertion id >= GX_TEXMAP0 && id <= GX_TEXMAP7"         */
extern u8 lbl_80590E70[]; /* "NW4R:Failed assertion idx >= 1 && idx <= 3"                          */
extern u8 lbl_80590EBC[]; /* "NW4R:Failed assertion id <= 7"                                       */
extern u8 lbl_80590DE4[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"                              */
extern u8 lbl_80590E0C[]; /* "NW4R:Failed assertion !(flag & 0xffffff00)"                         */
extern u8 lbl_805914B4[]; /* assert message used by the ResTexSrt resolvers                       */
extern u8 lbl_805914D0[]; /* assert file used by the ResTexSrt resolvers                          */
/* File-scope constants the range's accessors return by address. */
extern u8 lbl_80591210[];
extern u8 lbl_80591470[];
extern u8 lbl_805914A8[];
extern u8 lbl_805914E0[];

/* The `ResTexSrt` backing record `ResTexSrt::SetEffectMtx`/`GetEffectMtx` index.  Only this unit
 * uses it.  The 8 effect-matrix slots start at +0xAB (a 1-byte "matrix present" flag per slot,
 * then the 0x30-byte MTX34), stride 0x34. */
struct ResTexSrtEffect {
    /* +0x00 */ s8 a;                 /* the first per-slot bound byte (read by fn_800954FC's first out) */
    /* +0x01 */ s8 b;                 /* a signed clamp byte */
    /* +0x02 */ u8 c;                 /* a wrap-mode byte */
    /* +0x03 */ u8 flag;              /* bit 0: an effect matrix is present for this slot */
    /* +0x04 */ u8 mtx[0x30];          /* the 0x30-byte effect matrix (stored unaligned, as nw4r does) */
}; /* size: 0x34 */

struct ResTexSrtData {
    /* +0x00 */ u8 pad_0x00[0xA8];
    /* +0xA8 */ ResTexSrtEffect effect[8];
}; /* size: 0x248 - lower bound (only the effect table is reached) */

/* Forward declarations for this unit's own functions (C linkage: the map's names are the plain
 * `fn_XXXXXXXX` stems, so a C++ definition would mangle them away). */
extern "C" {
void* fn_80094868(ResHandle* pSelf);
void* fn_80094934(ResHandle* pSelf);
u8* fn_800947A4(ResHandle* pSelf, u32 id);
u8* fn_80094870(ResHandle* pSelf, u32 id);
u32 fn_8009493C(ResHandle* pSelf, u32 id);
void fn_80094A10(ResHandle* pSelf, u32 id);
void fn_80094AD4(ResHandle* pSelf, u32 id);
}

/* --- the `ResTexSrt` effect-matrix pair (rule 9: defined through the owner class) -------------- */

bool nw4r::g3d::ResTexSrt::SetEffectMtx(u32 id, const nw4r::math::MTX34* pMtx)
{
    if (id < 8) {
        ResTexSrtEffect* pEffect = &((ResTexSrtData*)fn_8006E2AC(this))->effect[id];

        if (pMtx != NULL) {
            fn_8007100C(pEffect->mtx, pMtx);
            pEffect->flag &= ~1;
        } else {
            mtx34_identity(pEffect->mtx);
            pEffect->flag |= 1;
        }
        return true;
    }
    return false;
}

bool nw4r::g3d::ResTexSrt::GetEffectMtx(u32 id, nw4r::math::MTX34* pMtx) const
{
    if (pMtx != NULL && id < 8) {
        const ResTexSrtEffect* pEffect = &((const ResTexSrtData*)fn_80087870((void*)this))->effect[id];

        fn_8007100C(pMtx, pEffect->mtx);
        return true;
    }
    return false;
}

/* --- the `ResMat` handle accessor cluster (0x800947A4-0x80094B00) ------------------------------ */

/* `ptr()`: the resource pointer the handle stores. */
extern "C" void* fn_80094868(ResHandle* pSelf)
{
    return pSelf->mpData;
}

/* A second `ptr()` copy for the sibling handle type (byte-identical to fn_80094868). */
extern "C" void* fn_80094934(ResHandle* pSelf)
{
    return pSelf->mpData;
}

/* The `id`-th texture-map sub-resource pointer: 8 slots of 0x20 bytes at +0x4 of the resource. */
extern "C" u8* fn_800947A4(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768DC(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 384, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 385, (const char*)lbl_80590DA8);
    }
    if (fn_800768DC(pSelf) && id <= 7) {
        return (u8*)fn_80094868(pSelf) + id * 0x20 + 4;
    }
    return NULL;
}

/* The same indexer for the sibling handle type. */
extern "C" u8* fn_80094870(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768DC(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 400, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 401, (const char*)lbl_80590DA8);
    }
    if (fn_800768DC(pSelf) && id <= 7) {
        return (u8*)fn_80094934(pSelf) + id * 0x20 + 4;
    }
    return NULL;
}

/* The `id`-th bit of the resource's flag word. */
extern "C" u32 fn_8009493C(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768DC(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 416, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 417, (const char*)lbl_80590DA8);
    }
    if (fn_800768DC(pSelf) && id <= 7) {
        u32 word = *(u32*)fn_80094868(pSelf);
        return (word & (1u << id)) != 0;
    }
    return 0;
}

/* Set the `id`-th bit of the resource's flag word. */
extern "C" void fn_80094A10(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768DC(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 432, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 433, (const char*)lbl_80590DA8);
    }
    if (fn_800768DC(pSelf) && id <= 7) {
        u32* pWord = (u32*)fn_80094934(pSelf);
        *pWord |= 1u << id;
    }
}

/* Clear the `id`-th bit of the resource's flag word. */
extern "C" void fn_80094AD4(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768DC(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 444, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 445, (const char*)lbl_80590DA8);
    }
    if (fn_800768DC(pSelf) && id <= 7) {
        u32* pWord = (u32*)fn_80094934(pSelf);
        *pWord &= ~(1u << id);
    }
}

/* --- the `ResTex` handle accessor cluster (0x80094CB8-0x8009509C) ------------------------------- */

/* A third `ptr()` copy for the sibling handle type. */
extern "C" void* fn_80094D7C(ResHandle* pSelf)
{
    return pSelf->mpData;
}

/* A fourth `ptr()` copy for the sibling handle type. */
extern "C" void* fn_80094E48(ResHandle* pSelf)
{
    return pSelf->mpData;
}

/* The `id`-th 0xC-byte sub-resource pointer of the sibling handle. */
extern "C" u8* fn_80094CB8(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768F0(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 490, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 491, (const char*)lbl_80590DA8);
    }
    if (fn_800768F0(pSelf) && id <= 7) {
        return (u8*)fn_80094D7C(pSelf) + id * 0xC + 4;
    }
    return NULL;
}

/* The same indexer for the fourth handle type. */
extern "C" u8* fn_80094D84(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768F0(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 506, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 507, (const char*)lbl_80590DA8);
    }
    if (fn_800768F0(pSelf) && id <= 7) {
        return (u8*)fn_80094E48(pSelf) + id * 0xC + 4;
    }
    return NULL;
}

/* The `id`-th bit of the third handle's flag word. */
extern "C" u32 fn_80094E50(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768F0(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 522, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 523, (const char*)lbl_80590DA8);
    }
    if (fn_800768F0(pSelf) && id <= 7) {
        u32 word = *(u32*)fn_80094D7C(pSelf);
        return (word & (1u << id)) != 0;
    }
    return 0;
}

/* Set the `id`-th bit of the fourth handle's flag word. */
extern "C" void fn_80094F24(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768F0(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 538, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 539, (const char*)lbl_80590DA8);
    }
    if (fn_800768F0(pSelf) && id <= 7) {
        u32* pWord = (u32*)fn_80094E48(pSelf);
        *pWord |= 1u << id;
    }
}

/* Clear the `id`-th bit of the fourth handle's flag word. */
extern "C" void fn_80094FE8(ResHandle* pSelf, u32 id)
{
    u32 valid;

    if (!fn_800768F0(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 550, (const char*)lbl_80590D88);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 551, (const char*)lbl_80590DA8);
    }
    if (fn_800768F0(pSelf) && id <= 7) {
        u32* pWord = (u32*)fn_80094E48(pSelf);
        *pWord &= ~(1u << id);
    }
}

/* --- the range's small `_ac.h` copy/accessor inlines, out of line --------------------------------- */

/* 4-byte name + one word (the material the copy-assigns carry). */
struct ResName4 {
    /* +0x00 */ u8 name[4];
    /* +0x04 */ u32 value;
}; /* size: 0x8 */

/* 4-byte name + two words. */
struct ResName4x2 {
    /* +0x00 */ u8 name[4];
    /* +0x04 */ u32 a;
    /* +0x08 */ u32 b;
}; /* size: 0xC */

/* Four signed halfwords. */
struct ResHalf4 {
    /* +0x00 */ s16 v[4];
}; /* size: 0x8 */

/* Ten words (one collection record). */
struct ResWord10 {
    /* +0x00 */ u32 v[10];
}; /* size: 0x28 */

/* The four `ptr()` copies of the remaining one-word handles (all `lwz r3,0(r3)`). */
extern "C" void* fn_800955A0(ResHandle* pSelf) { return pSelf->mpData; }
extern "C" void* fn_800957B4(ResHandle* pSelf) { return pSelf->mpData; }
extern "C" void* fn_80095830(ResHandle* pSelf) { return pSelf->mpData; }
extern "C" void* fn_80095D4C(ResHandle* pSelf) { return pSelf->mpData; }
extern "C" void* fn_80097898(ResHandle* pSelf) { return pSelf->mpData; }
extern "C" void* fn_80097924(ResHandle* pSelf) { return pSelf->mpData; }

/* The `== NULL` test of the same handle family. */
extern "C" u32 fn_800978AC(ResHandle* pSelf) { return pSelf->mpData != NULL; }

/* The handle-word setters. */
extern "C" void fn_80097450(ResHandle* pSelf, void* pData) { pSelf->mpData = pData; }
extern "C" void fn_80098374(ResHandle* pSelf, void* pData) { pSelf->mpData = pData; }
extern "C" void fn_800984F8(ResHandle* pSelf, void* pData) { pSelf->mpData = pData; }
extern "C" void fn_80098614(ResHandle* pSelf, void* pData) { pSelf->mpData = pData; }

/* The four file-scope constant accessors (each returns a `.data` address). */
extern "C" u8* fn_8009521C(void) { return lbl_805914E0; }
extern "C" u8* fn_800957A8(void) { return lbl_805914A8; }
extern "C" u8* fn_80095CDC(void) { return lbl_80591470; }
extern "C" u8* fn_800978A0(void) { return lbl_80591210; }

/* The out-of-line struct copy-assigns: a byte-wise copy of the 4-byte name, then the word fields
 * (MWCC emits the byte/halfword loop unrolled, which is why they are written as explicit loops). */
extern "C" void fn_80095718(ResName4* pDst, const ResName4* pSrc)
{
    int i;
    for (i = 0; i < 4; i++) {
        pDst->name[i] = pSrc->name[i];
    }
    pDst->value = pSrc->value;
}

extern "C" void fn_80095C44(ResName4x2* pDst, const ResName4x2* pSrc)
{
    int i;
    for (i = 0; i < 4; i++) {
        pDst->name[i] = pSrc->name[i];
    }
    pDst->a = pSrc->a;
    pDst->b = pSrc->b;
}

extern "C" void fn_8009639C(ResHalf4* pDst, const ResHalf4* pSrc)
{
    int i;
    for (i = 0; i < 4; i++) {
        pDst->v[i] = pSrc->v[i];
    }
}

extern "C" void fn_80097158(ResWord10* pDst, const ResWord10* pSrc) { *pDst = *pSrc; }

/* A sub-resource offset resolved against the handle's word. */
extern "C" u8* fn_80097458(ResHandle* pSelf, u32 offset)
{
    u8* pBase = (u8*)pSelf->mpData;

    if (offset != 0) {
        return pBase + offset;
    }
    return NULL;
}

/* --- the `ResTlut`-style tail-call thunks (0x80098CC0-0x80098CF8) --------------------------------- */

extern "C" void fn_80098CC0(void* pSelf) { fn_80089844(pSelf, 0); }
extern "C" void fn_80098CC8(void* pSelf) { fn_800897D8(pSelf, 0); }
extern "C" void fn_80098CD0(void* pSelf) { fn_8008976C(pSelf, 0); }
extern "C" void fn_80098CD8(void* pSelf) { fn_80089700(pSelf, 0); }
extern "C" void fn_80098CE0(void* pSelf) { fn_80089694(pSelf, 0); }
extern "C" void fn_80098CE8(void* pSelf) { fn_80089624(pSelf, 0); }

/* --- the `ResTexSrt` per-slot accessor pair (0x80095438-0x8009559C) ----------------------------- */

extern "C" u32 fn_80095438(void* pSelf, u32 id, u32 c, u32 a, u32 b)
{
    if (id < 8 && c < 256) {
        ResTexSrtEffect* pEffect = &((ResTexSrtData*)fn_8006E2AC(pSelf))->effect[id];

        pEffect->c = (u8)c;
        pEffect->a = (a <= 31) ? (s8)a : (s8)-1;
        pEffect->b = (b <= 127) ? (s8)b : (s8)-1;
        return 1;
    }
    return 0;
}

extern "C" u32 fn_800954FC(void* pSelf, u32 id, u32* pC, s32* pA, s32* pB)
{
    if (id < 8) {
        ResTexSrtEffect* pEffect = &((ResTexSrtData*)fn_80087870(pSelf))->effect[id];

        if (pC != NULL) {
            *pC = pEffect->c;
        }
        if (pA != NULL) {
            *pA = pEffect->a;
        }
        if (pB != NULL) {
            *pB = pEffect->b;
        }
        return 1;
    }
    return 0;
}

/* --- the `ResTexObj` byte/word setters and getters (0x800955A0-0x800959AF) ---------------------- */

/* The per-texture header fn_800955A0 resolves (only the fields the range touches are named). */
struct ResTexObjData {
    /* +0x00 */ u8 pad_0x00[2];
    /* +0x02 */ u8 wrapMode;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u32 value;
}; /* size: 0x8 - lower bound */

/* The signed-bound pair fn_80095830/0x800957B4 resolve. */
struct ResTlutObjData {
    /* +0x00 */ u8 format;
    /* +0x01 */ s8 maxIndex;
    /* +0x02 */ s8 field2;
    /* +0x03 */ u8 pad_0x03;
}; /* size: 0x4 - lower bound */

extern "C" void fn_800955A8(void* pSelf, u32 wrapMode)
{
    if (!fn_800768C8(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 742, (const char*)lbl_80590D88);
    }
    if (fn_800768C8(pSelf)) {
        ((ResTexObjData*)fn_800955A0((ResHandle*)pSelf))->wrapMode = (u8)wrapMode;
    }
}

extern "C" void fn_80095620(void* pSelf, u32 value)
{
    if (!fn_800768C8(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 758, (const char*)lbl_80590D88);
    }
    if (fn_800768C8(pSelf)) {
        ((ResTexObjData*)fn_800955A0((ResHandle*)pSelf))->value = value;
    }
}

extern "C" u32 fn_800957BC(void* pSelf)
{
    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 795, (const char*)lbl_80590D88);
    }
    if (fn_80076974(pSelf)) {
        return ((ResTlutObjData*)fn_80095830((ResHandle*)pSelf))->format;
    }
    return 0;
}

extern "C" void fn_80095838(void* pSelf, u32 value)
{
    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 810, (const char*)lbl_80590D88);
    }
    if (fn_80076974(pSelf)) {
        ResTlutObjData* pData = (ResTlutObjData*)fn_800957B4((ResHandle*)pSelf);

        if (value <= 127) {
            pData->maxIndex = (s8)value;
        } else {
            pData->maxIndex = -1;
        }
    }
}

extern "C" s32 fn_800958C8(void* pSelf)
{
    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 825, (const char*)lbl_80590D88);
    }
    if (fn_80076974(pSelf)) {
        return ((ResTlutObjData*)fn_80095830((ResHandle*)pSelf))->maxIndex;
    }
    return -1;
}

/* The third signed byte of the same header. */
extern "C" s32 fn_80095940(void* pSelf)
{
    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 853, (const char*)lbl_80590D88);
    }
    if (fn_80076974(pSelf)) {
        return ((ResTlutObjData*)fn_80095830((ResHandle*)pSelf))->field2;
    }
    return -1;
}

/* One palette entry the 1..3 index selects: a byte id at +0x3 and a signed value at +0x7. */
struct ResTlutPaletteEntry {
    /* +0x00 */ u8 pad_0x00[3];
    /* +0x03 */ u8 id;
    /* +0x04 */ u8 pad_0x04[3];
    /* +0x07 */ s8 value;
}; /* size: 0x8 */

/* Set the palette entry `idx` (1..3) with the texture id and the clamped value. */
extern "C" void fn_800959B8(void* pSelf, u32 idx, u32 id, s32 value)
{
    u32 valid;

    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 875, (const char*)lbl_80590D88);
    }
    if (!(idx >= 1 && idx <= 3)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 879, (const char*)lbl_80590E70);
    }
    valid = (id <= 7);
    if (!valid) {
        nw4r::db::Panic((const char*)lbl_80590D78, 882, (const char*)lbl_80590EBC);
    }
    if (fn_80076974(pSelf) && idx - 1 <= 2 && id <= 7) {
        ResTlutPaletteEntry* pEntry =
            (ResTlutPaletteEntry*)((u8*)fn_800957B4((ResHandle*)pSelf) + idx);

        pEntry->id = (u8)id;
        if ((s8)value < 0) {
            pEntry->value = -1;
        } else {
            pEntry->value = (u8)value;
        }
    }
}

/* Read the palette entry `idx` (1..3) back out. */
extern "C" void fn_80095ADC(void* pSelf, u32 idx, u32* pId, u8* pValue)
{
    if (!fn_80076974(pSelf)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 903, (const char*)lbl_80590D88);
    }
    if (!(idx >= 1 && idx <= 3)) {
        nw4r::db::Panic((const char*)lbl_80590D78, 907, (const char*)lbl_80590E70);
    }
    if (fn_80076974(pSelf) && idx - 1 <= 2) {
        ResTlutPaletteEntry* pEntry =
            (ResTlutPaletteEntry*)((u8*)fn_800957B4((ResHandle*)pSelf) + idx);

        if (pId != NULL) {
            *pId = pEntry->id;
        }
        if (pValue != NULL) {
            *pValue = pEntry->value;
        }
    }
}
