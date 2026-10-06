/*
 * g3d/g3d_state.cpp - the nw4r g3d `G3DState` cluster: GX state writes and the camera matrix table the
 *   `mtxID < NUM_CAMERA` asserts guard.
 * RANGE. .text 0x8008452C-0x80088E24 (167 functions); extab, extabindex, .ctors 0x8056F2D4-0x8056F2D8, .rodata
 *   0x8056F6D0-0x8056F710, .data 0x8058F750-0x8058FCE8 (opens on "g3d_state.cpp"), .bss 0x80682E80-0x80688420,
 *   .sdata 0x80791238-0x80791258, .sbss 0x807948F0-0x80794910, .sdata2 0x80795E70-0x80795E98.  Left seam:
 *   `g3d/g3d_scnroot.cpp`'s `.data` fragment ends at 0x8058F74A; the right edge is `g3d/g3d_resvtx.cpp`, whose
 *   fragment opens at 0x8058FCE8.
 * NAMES. Map stems (`fn_`/`dtor_`, no mangling to derive from) for the unwritten rows; `mtx34_inverse` (0x800883C4) is a
 *   GUESS (the 3x4 matrix inverse its body computes).  The material-state cache functions are GUESSES from their
 *   bodies and nw4r's G3DState: g3d_ind_mtx_op_init is a GUESS, g3d_ind_mtx_op_load is a GUESS,
 *   g3d_ind_mtx_op_set is a GUESS (the three indirect matrices),
 *   g3d_tex_coord_scale_load is a GUESS, g3d_state_set_mat_misc is a GUESS, g3d_zcomp_cache_set is a GUESS,
 *   g3d_state_load_tex_obj is a GUESS, g3d_tex_obj_cache_load is a GUESS, g3d_tex_obj_equal is a GUESS,
 *   g3d_state_load_tlut_obj is a GUESS, g3d_tlut_obj_cache_load is a GUESS, g3d_state_set_gen_mode is a GUESS,
 *   g3d_gen_mode_cache_set is a GUESS, g3d_state_load_tev is a GUESS, g3d_tex_coord_scale_set_tex_maps is a GUESS,
 *   g3d_gen_mode_cache_load is a GUESS, g3d_gd_set_gen_mode is a GUESS, g3d_tev_cache_update is a GUESS,
 *   g3d_state_load_mat_pix is a GUESS, g3d_state_load_mat_ind_mtx_dl is a GUESS, g3d_state_load_mat_ind_mtx is a
 *   GUESS; the globals g3d_state_gen_mode_cache, g3d_state_tex_coord_scale_cache, g3d_state_tex_obj_cache,
 *   g3d_state_tlut_obj_cache, g3d_state_dl_dirty, g3d_state_cached_tev, g3d_state_zcomp_cache and
 *   g3d_state_cull_mode_hw are GUESSES from their readers.  ResGenMode's GXGet* and ResTev's ref/ptr/GetClassName
 *   are nw4r's members.
 * RESIDUALS. 110 functions unwritten (objdiff scores them zero) in 9 runs: 0x80084630-0x80085160,
 *   0x8008540C-0x80085478, 0x800854B8-0x8008569C, 0x800856F4-0x80085AF8, 0x80085B6C-0x80085D4C,
 *   0x80085E48-0x80085F3C (g3d_tex_obj_cache_load: its 0x20-byte copy is `font/flfnt.cpp`'s unnamed fn_8005C50C),
 *   0x800869D4-0x80086A28 (its IsValid fn_80076750 is also called by ef/effect.cpp), 0x80086B2C-0x80086FFC,
 *   0x80087004-0x80088E24 (with the `.ctors` static constructor fn_80088AD0).
 *   fn_8008455C: one relocation argument differs.
 *   g3d_state_load_tex_obj, g3d_state_load_tlut_obj: the by-value copy's address is formed after the cache's
 *     (retail forms it first).
 *   g3d_tlut_obj_cache_load: retail strides the compared entry with its own offset register and the copied one
 *     with `mulli`; ours shares one pointer (tried: a separate offset counter, worse).
 *   g3d_tev_cache_update, g3d_state_load_mat_ind_mtx_dl, g3d_state_load_mat_ind_mtx: register order only.
 *   flipcheck: `.text` short of the claim; `.ctors`, `.bss`, `.sbss`, `.rodata` and `.sdata2` are claimed and not
 *     emitted, `.data` is 0xAC of 0x598 and `.sdata` 0x4 of 0x20 (the globals and the asserts' strings are declared,
 *     not defined).
 * SHAPES. File-scope `#pragma peephole off`: retail keeps every `rlwinm`/`clrlwi` + `cmpwi` and unfused
 *   `clrlwi` + `slwi` pairs (it took fn_800856A4/fn_800856B8/fn_80086610/fn_800868E4 to 100 with no row lost).
 *   The generation-mode word is `(ind << 16 | cull << 14) | ((tev - 1) << 10 | (gens | chans << 4))`, that
 *   grouping.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"              /* GXWGFifo, the 0xCC008000 write window (rule 1) */
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResGenMode (rule 2) */
#include "g3d/g3d_anmchr.h"    /* fn_8005DC24, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80063888.h"   /* fn_800639D0, owned by g3d/fn_80063888.cpp (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_identity/MTX34_ctor, owned by fn_8004CAD8.cpp (rule 2) */
#include "g3d/g3d_camera.h"    /* fn_80075390..fn_80075620, owned by g3d/g3d_camera.cpp (rule 2) */
#include "g3d/fn_80075DCC.h"    /* fn_8007B5F4/fn_8007BB8C, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/g3d_resshp.h"     /* nw4r::g3d::ResTev (rule 2) */
#include "g3d/g3d_state.h"
#include "gx/fn_8009AA78.h"     /* GDSetTexCoordScale2 (rule 2) */
#include "gx/GDSetIndTexMtx.h"  /* GDSetIndTexMtx (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */

#pragma peephole off
#include "RVLGX/GXSetTevOrder.h" /* GXLoadTlut, GXSetZCompLoc (rule 2) */

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)             */
/* ------------------------------------------------------------------------------------------------ */

/* The panic file/format strings the target references as map symbols, declared here rather than as
 * literals for the same reason `g3d_camera.cpp` does: `-str reuse` would pool a literal. */
extern const char lbl_8056F6D0[]; /* "ScnRoot" */
extern const char lbl_8058F750[]; /* "g3d_state.cpp" */
extern const char lbl_8058F80C[]; /* "NW4R:Failed assertion mtxID < NUM_CAMERA && mtxID >= 0" */

/* `.sdata` and `.sbss` globals the range's small accessors hand back the address of. */
extern u32 lbl_8079124C;

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The SDK entry points the target reaches with their own `lis`/`addi`; mtx34_identity/MTX34_ctor and
 * fn_80075390..fn_80075620 come from their owners' headers above. */
extern "C" void fn_80501658(void* p);

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The 4-byte-head resource handle `fn_8008569C`/`fn_800856CC` hand back the interior pointer of:
 * a `ResCommon<T>`-shaped wrapper whose payload starts one word in. */
struct G3DResRef {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mPayload;
}; /* size: 0x8 (approximation - only the two words are touched by this unit) */

/* One word/byte of the state the small accessors below read. */
struct StateWord {
    /* +0x00 */ u32 mWord;
}; /* size: 0x4 (approximation) */

/* The display-list dirty flag (`g3d_state_dl_dirty`): set when a cached state changed, read-and-cleared by the next
 * display-list call.  size: 0x4 */
struct G3dDlDirty {
    /* +0x00 */ bool dirty;
    /* +0x01 */ u8 pad_0x01[3];
};

struct StateByte {
    /* +0x00 */ u8 mByte;
}; /* size: 0x1 (approximation) */

struct StateByte3 {
    /* +0x00 */ u8 pad_00[3];
    /* +0x03 */ u8 mByte;
}; /* size: 0x4 (approximation) */

struct StateByte1 {
    /* +0x00 */ u8 pad_00[1];
    /* +0x01 */ u8 mByte;
}; /* size: 0x2 (approximation) */

/* The three-word value `fn_80086138` copies and `fn_80086154` compares. */
struct StateWord3 {
    /* +0x00 */ u32 mWords[3];
}; /* size: 0xC (approximation) */

/* One 4-byte `{u16, u16}` pair of the table at +0x04 (`fn_80085F3C`): a texture's width and height. */
struct StatePair {
    /* +0x00 */ u16 mA;
    /* +0x02 */ u16 mB;
}; /* size: 0x4 */

/* One texture-coordinate scale the cache writes (`g3d_tex_coord_scale_load`). */
struct StateCoordScale {
    /* +0x00 */ u16 s;
    /* +0x02 */ u16 t;
    /* +0x04 */ u8 pad_0x04[4];
}; /* size: 0x8 */

/* The texture-coordinate scale cache (`g3d_state_tex_coord_scale_cache`): the loaded textures' sizes, the scales
 * derived from them and the TEV's coordinate-to-texture-map table.  size: 0x74 */
struct StatePairTable {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ StatePair mPairs[8];
    /* +0x24 */ StateCoordScale scale[8];
    /* +0x64 */ union {
        u8 texMapID[8];
        u32 texMapWord[2];
    };
    /* +0x6C */ u8 pad_0x6C[8];
};

/* The texture-object cache (`g3d_state_tex_obj_cache`): the loaded objects and a valid bit per map.
 * size: 0x120 */
struct G3dTexObjCache {
    /* +0x000 */ GXTexObj texObj[8];
    /* +0x100 */ u8 mMask;
    /* +0x101 */ u8 pad_0x101[0x1F];
};

/* The TLUT-object cache (`g3d_state_tlut_obj_cache`): the loaded objects and a valid bit per slot.  size: 0x64 */
struct G3dTlutObjCache {
    /* +0x00 */ GXTlutObj tlut[8];
    /* +0x60 */ u16 validMask;
    /* +0x62 */ u8 pad_0x62[2];
};

/* The callback the material's indirect matrices are offered to before they load (its first virtual).
 * size: 0x4 */
class G3dIndMtxCallback {
public:
    virtual void Exec(struct G3dIndMtxOp* pOp) = 0;
};

/* The state object's flag word carrier, read by `fn_800856A4`/`fn_800856B8`. */
struct StateFlags {
    /* +0x00 */ u32 mFlags;
}; /* size: 0x4 (approximation) */

/* One 0x44-byte entry of the texture-matrix table at +0x2C (`fn_8008540C` constructs 0x80 of them,
 * `fn_800856D4` indexes them). */
struct StateTexMtxEntry {
    /* +0x00 */ u8 mData[0x44];
}; /* size: 0x44 */

/* The texture-matrix table `fn_800856D4` indexes: 0x80 entries from +0x2C. */
struct StateTexMtxTable {
    /* +0x00 */ u8 pad_00[0x2C];
    /* +0x2C */ StateTexMtxEntry mEntries[0x80];
}; /* size: 0x222C */

/* One 0xC-byte row of the byte table at +0x242C (`fn_80085B54`). */
struct StateByteRow {
    /* +0x00 */ u8 mBytes[0xC];
}; /* size: 0xC */

/* The two 0xC-byte-row tables and the index that selects one, as `fn_80085B54` walks them. */
struct StateByteTable {
    /* +0x00 */ u8 pad_00[0x10];
    /* +0x10 */ u32 mIndex;
    /* +0x14 */ u8 pad_14[0x2418];
    /* +0x242C */ StateByteRow mRows[0x80];
}; /* size: 0x2A2C */

/* The 0x2C-byte header `fn_80085478` re-initialises (the `-1`/`0` fill of one camera-state entry). */
struct StateEntryHead {
    /* +0x00 */ u8 pad_00[0x10];
    /* +0x10 */ s32 mUnk10;
    /* +0x14 */ s32 mUnk14;
    /* +0x18 */ s32 mUnk18;
    /* +0x1C */ s32 mUnk1C;
    /* +0x20 */ s32 mUnk20;
    /* +0x24 */ s8 mUnk24[8];
}; /* size: 0x2C (approximation - the caller's object continues past it) */

/* The object `fn_80085344` sets the validity byte of. */
struct StateValidFlag {
    /* +0x00 */ u8 mValid;
}; /* size: 0x1 (approximation) */

/* The object `fn_8008455C` dispatches on: its first word is a vtable and the call is slot +0x14. */
class G3dVtObject {
public:
    virtual void m00(); /* +0x00 */
    virtual void m04(); /* +0x04 */
    virtual void m08(); /* +0x08 */
    virtual void m0C(); /* +0x0C */
    virtual void m10(); /* +0x10 */
    virtual u32 m14();  /* +0x14 - the slot fn_8008455C calls */
}; /* size: 0x4 */

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

void* fn_8008452C(void);
u32 fn_8008455C(G3dVtObject* pSelf);
u32 fn_80084594(void* pSelf, u32* pArg);
void* fn_80084600(void);
void fn_80085344(StateValidFlag* pSelf);
void fn_80085478(StateEntryHead* pSelf);
u32* fn_8008569C(G3DResRef* pSelf);
BOOL fn_800856A4(StateFlags* pSelf);
BOOL fn_800856B8(StateFlags* pSelf);
u32* fn_800856CC(G3DResRef* pSelf);
StateTexMtxEntry* fn_800856D4(StateTexMtxTable* pSelf, u32 idx);
u8* fn_80085B00(u8* p, u8* pEnd, const s32* pValue);
s8* fn_80085B28(s8* p, s8* pEnd, const s8* pDelim);
s32 fn_80085AF8(u32 a, u32 b);
u8 fn_80085B54(StateByteTable* pSelf, u32 byteIdx);
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b);
void fn_80086118(G3dTexObjCache* pSelf, u32 bit);
void fn_80086138(StateWord3* pDst, const StateWord3* pSrc);
BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB);
void fn_80086610(StatePairTable* pSelf, u8 arg);
u8 fn_80086640(StateByte* pSelf);
bool fn_80086770(G3dDlDirty* pSelf);
void fn_800867A0(G3dDlDirty* pSelf);
u8 fn_80086AA0(StateByte3* pSelf);
u8 fn_80086FFC(StateByte1* pSelf);
void fn_800868D8(u32 value);
void fn_800868E4(u32 value);
void fn_800868A0(u32 value);

/* ------------------------------------------------------------------------------------------------ */
/* g3d_state.cpp                                                                                     */
/* ------------------------------------------------------------------------------------------------ */

/* The `ScnRoot` singleton lookup: `fn_8007BB8C` stores the found object through its out-parameter and
 * returns that parameter, so the result is the word it stored. */
void* fn_8008452C(void) {
    void* pScnRoot;
    return *fn_8007BB8C(&pScnRoot, lbl_8056F6D0);
}

/* The same lookup, emitted a second time for the state object's own caller. */
void* fn_80084600(void) {
    void* pScnRoot;
    return *fn_8007BB8C(&pScnRoot, lbl_8056F6D0);
}

/* A virtual dispatch on slot +0x14 whose result is handed to the `fn_8005DC24` unwrapper. */
u32 fn_8008455C(G3dVtObject* pSelf) {
    u32 value = pSelf->m14();
    return fn_8005DC24(&value);
}

/* Register `pArg`'s bound object with the state's table, and on a miss insert it under the current
 * `ScnRoot`. */
u32 fn_80084594(void* pSelf, u32* pArg) {
    void* pScnRoot = fn_80084600();
    if (fn_800639D0((u32**)pArg, (u32**)&pScnRoot)) {
        return 1;
    }
    u32 key = *pArg;
    return fn_8007B5F4(pSelf, &key);
}

void fn_80085344(StateValidFlag* pSelf) {
    pSelf->mValid = 1;
}

/* Re-initialise one camera-state entry: the -1 markers and the cleared words. */
void fn_80085478(StateEntryHead* pSelf) {
    pSelf->mUnk10 = -1;
    pSelf->mUnk14 = 0;
    pSelf->mUnk18 = 0;
    pSelf->mUnk1C = 0;
    pSelf->mUnk20 = 0;
    pSelf->mUnk24[7] = -1;
    pSelf->mUnk24[6] = -1;
    pSelf->mUnk24[5] = -1;
    pSelf->mUnk24[4] = -1;
    pSelf->mUnk24[3] = -1;
    pSelf->mUnk24[2] = -1;
    pSelf->mUnk24[1] = -1;
    pSelf->mUnk24[0] = -1;
}

u32* fn_8008569C(G3DResRef* pSelf) {
    return &pSelf->mPayload;
}

u32* fn_800856CC(G3DResRef* pSelf) {
    return &pSelf->mPayload;
}

/* `!(flags & 0x20)` - the target's `rlwinm`/`cntlzw`/`srwi` triple, so the masked value is
 * materialised first (inlining the test lets MWCC fold it to `extrwi`/`xori`). */
BOOL fn_800856A4(StateFlags* pSelf) {
    u32 masked = pSelf->mFlags & 0x20;
    return masked == 0 ? TRUE : FALSE;
}

/* `!(flags & 0x10)`, the same shape as fn_800856A4. */
BOOL fn_800856B8(StateFlags* pSelf) {
    u32 masked = pSelf->mFlags & 0x10;
    if (masked == 0) {
        return TRUE;
    }
    return FALSE;
}

/* The bounds-tested table accessor.  The target puts the NULL path last, so the guard is spelled
 * `if (idx <= 0x7F) { return ...; } return NULL;`. */
StateTexMtxEntry* fn_800856D4(StateTexMtxTable* pSelf, u32 idx) {
    if (idx <= 0x7F) {
        return &pSelf->mEntries[idx];
    }
    return NULL;
}

s32 fn_80085AF8(u32 a, u32 b) {
    return b - a;
}

u8 fn_80085B54(StateByteTable* pSelf, u32 byteIdx) {
    return pSelf->mRows[pSelf->mIndex].mBytes[byteIdx];
}

/* The two byte-scan loops of the state's key tables: walk `p` up to `pEnd` while the byte does not
 * match, and return where it stopped. */
u8* fn_80085B00(u8* p, u8* pEnd, const s32* pValue) {
    while (p != pEnd && *(s8*)p != *pValue) {
        p++;
    }
    return p;
}

s8* fn_80085B28(s8* p, s8* pEnd, const s8* pDelim) {
    while (p != pEnd && *p != *pDelim) {
        p++;
    }
    return p;
}

/* Store the pair into the table at `idx` and mark the table dirty (`(flags | 2) & ~1`). */
void fn_80085F3C(StatePairTable* pSelf, u32 idx, u16 a, u16 b) {
    pSelf->mPairs[idx].mA = a;
    pSelf->mPairs[idx].mB = b;
    pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
}

void fn_80086118(G3dTexObjCache* pSelf, u32 bit) {
    pSelf->mMask &= ~(1 << bit);
}

void fn_80086138(StateWord3* pDst, const StateWord3* pSrc) {
    *pDst = *pSrc;
}

BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB) {
    u32 equal = 0;
    if (pA->mWords[0] == pB->mWords[0] && pA->mWords[1] == pB->mWords[1] &&
        pA->mWords[2] == pB->mWords[2]) {
        equal = 1;
    }
    return equal;
}

} /* extern "C" */

/* 0x8008631C (0x74): returns the generation mode's cullMode, 3 for an empty handle. */
_GXCullMode nw4r::g3d::ResGenMode::GXGetCullMode() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xDF, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return (_GXCullMode)ptr()->cullMode;
    }
    return (_GXCullMode)3;
}

/* 0x80086398 (0x74): returns the generation mode's nInds, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumIndStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD8, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nInds;
    }
    return 0;
}

/* 0x8008640C (0x74): returns the generation mode's nTevs, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumTevStages() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xD1, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTevs;
    }
    return 0;
}

/* 0x80086480 (0x74): returns the generation mode's nChans, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumChans() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xCA, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nChans;
    }
    return 0;
}

/* 0x800864F4 (0x74): returns the generation mode's nTexGens, 0 for an empty handle. */
u8 nw4r::g3d::ResGenMode::GXGetNumTexGens() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_resmat_ac.h", 0xC3, "NW4R:Failed assertion IsValid()");
    }
    if (IsValid()) {
        return ptr()->nTexGens;
    }
    return 0;
}

/* 0x80086390 (0x8): returns the generation-mode block. */
const nw4r::g3d::ResGenModeData* nw4r::g3d::ResGenMode::ptr() const {
    return mpData;
}

extern "C" {

/* The state's dirty-flush hook: only flush when the pending flags say so and the new value differs. */
void fn_80086610(StatePairTable* pSelf, u8 arg) {
    if ((pSelf->mFlags & 2) != 0 && (pSelf->mFlags & 1) == 0 && arg != 0) {
        g3d_tex_coord_scale_load(pSelf, arg);
    }
}

u8 fn_80086640(StateByte* pSelf) {
    return pSelf->mByte;
}

} /* extern "C" */

/* 0x800866FC (0x64): returns the TEV block, panicking on a NULL handle. */
const nw4r::g3d::ResTevData& nw4r::g3d::ResTev::ref() const {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_restev_ac.h", 0x25, "%s::%s: Object not valid.", GetClassName(), "ref");
    }
    return *ptr();
}

/* 0x80086760 (0x8): returns the TEV block. */
const nw4r::g3d::ResTevData* nw4r::g3d::ResTev::ptr() const {
    return mpData;
}

/* 0x80086768 (0x8): returns the class name. */
const char* nw4r::g3d::ResTev::GetClassName() {
    return (const char*)&lbl_8079124C;
}

extern "C" {

/* Read the byte and clear it, returning what it held. */
bool fn_80086770(G3dDlDirty* pSelf) {
    bool value = pSelf->dirty;
    fn_800867A0(pSelf);
    return value;
}

void fn_800867A0(G3dDlDirty* pSelf) {
    pSelf->dirty = false;
}

u8 fn_80086AA0(StateByte3* pSelf) {
    return pSelf->mByte;
}

u8 fn_80086FFC(StateByte1* pSelf) {
    return pSelf->mByte;
}

/* The two write-gather-pipe stores (the window lives in `gx.h`, rule 1). */
void fn_800868D8(u32 value) {
    GXWGFifo.u32 = value;
}

void fn_800868E4(u32 value) {
    u8 narrow = value;
    GXWGFifo.u8 = narrow;
}

/* The `0x61`-tagged pipe command followed by its word. */
void fn_800868A0(u32 value) {
    fn_800868E4(0x61);
    fn_800868D8(value);
}

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */
/* The material-state caches: the indirect matrices, the z-compare location, the texture/TLUT object  */
/* caches, the generation mode and the TEV/pixel/indirect display lists                              */
/* ------------------------------------------------------------------------------------------------ */

/* The indirect-matrix operation a material's `ResMatIndMtxAndScale` fills: a flag bit per matrix that is set,
 * and the three matrices.  size: 0x94 */
struct G3dIndMtxOp {
    /* +0x00 */ u32 flags;
    /* +0x04 */ nw4r::math::MTX34 mtx[3];
};

/* The cached z-compare location (`lbl_80794908`).  size: 0x8 */
struct G3dZCompCache {
    /* +0x00 */ u32 flags;
    /* +0x04 */ u8 beforeTex;
};

/* The cached generation mode (`lbl_80682E80`): the four counts and the cull mode `ResGenMode` holds, and the
 * dirty/valid flags.  size: 0xC */
struct G3dGenModeCache {
    /* +0x00 */ u8 numTexGens;
    /* +0x01 */ u8 numChans;
    /* +0x02 */ u8 numTevStages;
    /* +0x03 */ u8 numIndStages;
    /* +0x04 */ s32 cullMode;
    /* +0x08 */ u32 flags;
};

extern "C" {

extern G3dGenModeCache g3d_state_gen_mode_cache;
extern StatePairTable g3d_state_tex_coord_scale_cache;
extern G3dTexObjCache g3d_state_tex_obj_cache;
extern G3dTlutObjCache g3d_state_tlut_obj_cache;
extern G3dDlDirty g3d_state_dl_dirty;
extern u32 g3d_state_cached_tev;
extern G3dZCompCache g3d_state_zcomp_cache;
extern const u8 g3d_state_cull_mode_hw[4];

/* The texture-object cache load (unwritten; its block copy is a `font/flfnt.cpp` helper). */
void g3d_tex_obj_cache_load(G3dTexObjCache* pSelf, nw4r::g3d::ResTexObj texObj);
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj);
void g3d_zcomp_cache_set(G3dZCompCache* pSelf, u8 beforeTex);
void g3d_gen_mode_cache_set(G3dGenModeCache* pSelf, nw4r::g3d::ResGenMode genMode);
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev);
void g3d_gen_mode_cache_load(G3dGenModeCache* pSelf);
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev);
void g3d_gd_set_gen_mode(u8 numTexGens, u8 numChans, u8 numTevStages, u8 numIndStages, u32 cullMode);

/* Reads the material's three indirect matrices into the operation, one flag bit per matrix found. */
G3dIndMtxOp* g3d_ind_mtx_op_init(G3dIndMtxOp* pSelf, const nw4r::g3d::ResMatIndMtxAndScale* pInd) {
    nw4r::math::MTX34* pMtx = pSelf->mtx;
    do {
        MTX34_ctor(pMtx);
        pMtx++;
    } while (pMtx < &pSelf->mtx[3]);
    pSelf->flags = 0;
    if (pInd->GXGetIndTexMtx(GX_ITM_0, &pSelf->mtx[0])) {
        pSelf->flags |= 1;
    }
    if (pInd->GXGetIndTexMtx(GX_ITM_1, &pSelf->mtx[1])) {
        pSelf->flags |= 2;
    }
    if (pInd->GXGetIndTexMtx(GX_ITM_2, &pSelf->mtx[2])) {
        pSelf->flags |= 4;
    }
    return pSelf;
}

/* Writes the operation's flagged matrices to the display list. */
void g3d_ind_mtx_op_load(G3dIndMtxOp* pSelf) {
    if (pSelf->flags & 1) {
        GDSetIndTexMtx(0, (const f32*)&pSelf->mtx[0]);
    }
    if (pSelf->flags & 2) {
        GDSetIndTexMtx(3, (const f32*)&pSelf->mtx[1]);
    }
    if (pSelf->flags & 4) {
        GDSetIndTexMtx(6, (const f32*)&pSelf->mtx[2]);
    }
}

/* Replaces the operation's matrix `id` (1..3) and flags it. */
void g3d_ind_mtx_op_set(G3dIndMtxOp* pSelf, s32 id, nw4r::math::MTX34* pMtx) {
    if (id == 1) {
        mtx34_copy(&pSelf->mtx[0], pMtx);
        pSelf->flags |= 1;
    } else if (id == 2) {
        mtx34_copy(&pSelf->mtx[1], pMtx);
        pSelf->flags |= 2;
    } else if (id == 3) {
        mtx34_copy(&pSelf->mtx[2], pMtx);
        pSelf->flags |= 4;
    }
}

/* Writes the texture-coordinate scales of the first `count` coordinates from the sizes of their textures. */
void g3d_tex_coord_scale_load(StatePairTable* pSelf, u8 count) {
    u8 i;
    for (i = 0; i < count; i++) {
        u8 map = pSelf->texMapID[i];
        if (map != 0xFF) {
            pSelf->scale[i].s = pSelf->mPairs[map].mA;
            pSelf->scale[i].t = pSelf->mPairs[pSelf->texMapID[i]].mB;
            GDSetTexCoordScale2(i, pSelf->scale[i].s, 0, 0, pSelf->scale[i].t, 0, 0);
        }
    }
    pSelf->mFlags |= 1;
}

/* Caches the material's z-compare location. */
void g3d_state_set_mat_misc(nw4r::g3d::ResMatMisc misc) {
    if (misc.IsValid()) {
        g3d_zcomp_cache_set(&g3d_state_zcomp_cache, misc.GXGetZCompLoc());
    }
}

/* Sets the z-compare location when it changed and marks the display list dirty. */
void g3d_zcomp_cache_set(G3dZCompCache* pSelf, u8 beforeTex) {
    if ((pSelf->flags & 1) == 0 || pSelf->beforeTex != beforeTex) {
        pSelf->flags |= 1;
        pSelf->beforeTex = beforeTex;
        GXSetZCompLoc(beforeTex);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

/* Loads the material's texture objects through the cache. */
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj) {
    if (texObj.IsValid()) {
        g3d_tex_obj_cache_load(&g3d_state_tex_obj_cache, texObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

/* Whether two texture objects are the same eight words. */
BOOL g3d_tex_obj_equal(const GXTexObj* pA, const GXTexObj* pB) {
    u32 equal = 0;
    if (pA->dummy[0] == pB->dummy[0] && pA->dummy[1] == pB->dummy[1] && pA->dummy[2] == pB->dummy[2] &&
        pA->dummy[3] == pB->dummy[3] && pA->dummy[4] == pB->dummy[4] && pA->dummy[5] == pB->dummy[5] &&
        pA->dummy[6] == pB->dummy[6] && pA->dummy[7] == pB->dummy[7]) {
        equal = 1;
    }
    return equal;
}

/* Loads the material's TLUT objects through the cache. */
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj) {
    if (tlutObj.IsValid()) {
        g3d_tlut_obj_cache_load(&g3d_state_tlut_obj_cache, tlutObj);
        fn_80085344((StateValidFlag*)&g3d_state_dl_dirty);
    }
}

/* Loads each valid TLUT that is not already cached, and invalidates the texture cached for that slot. */
void g3d_tlut_obj_cache_load(G3dTlutObjCache* pSelf, nw4r::g3d::ResTlutObj tlutObj) {
    u32 i = 0;
    do {
        if (tlutObj.IsValidTlut((_GXTlut)i)) {
            const GXTlutObj* pTlut = static_cast<const nw4r::g3d::ResTlutObj&>(tlutObj).GetTlut((_GXTlut)i);
            u16 bit = 1 << i;
            if ((pSelf->validMask & bit) == 0 || !fn_80086154((const StateWord3*)&pSelf->tlut[i],
                                                             (const StateWord3*)pTlut)) {
                pSelf->validMask |= bit;
                fn_80086138((StateWord3*)&pSelf->tlut[i], (const StateWord3*)pTlut);
                GXLoadTlut(pTlut, i);
                fn_80086118(&g3d_state_tex_obj_cache, i);
            }
        }
        i++;
    } while (i < 8);
}

/* Caches the material's generation mode. */
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode) {
    if (genMode.IsValid()) {
        g3d_gen_mode_cache_set(&g3d_state_gen_mode_cache, genMode);
    }
}

/* Copies each changed count or the cull mode into the cache, clearing the matching loaded flags. */
void g3d_gen_mode_cache_set(G3dGenModeCache* pSelf, nw4r::g3d::ResGenMode genMode) {
    if (pSelf->numTexGens != genMode.GXGetNumTexGens()) {
        pSelf->numTexGens = genMode.GXGetNumTexGens();
        pSelf->flags &= ~3;
    }
    if (pSelf->numChans != genMode.GXGetNumChans()) {
        pSelf->numChans = genMode.GXGetNumChans();
        pSelf->flags &= ~3;
    }
    if (pSelf->numTevStages != genMode.GXGetNumTevStages()) {
        pSelf->numTevStages = genMode.GXGetNumTevStages();
        pSelf->flags &= ~1;
    }
    if (pSelf->numIndStages != genMode.GXGetNumIndStages()) {
        pSelf->numIndStages = genMode.GXGetNumIndStages();
        pSelf->flags &= ~1;
    }
    if (pSelf->cullMode != genMode.GXGetCullMode()) {
        pSelf->cullMode = genMode.GXGetCullMode();
        pSelf->flags &= ~1;
    }
    if ((pSelf->flags & 4) == 0) {
        pSelf->flags = (pSelf->flags & ~3) | 4;
    }
}

/* Loads the material's TEV display list unless it is the one already loaded. */
void g3d_state_load_tev(nw4r::g3d::ResTev tev) {
    if (tev.IsValid()) {
        if (!g3d_tev_cache_update(&g3d_state_cached_tev, tev)) {
            g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
            tev.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
            g3d_tex_coord_scale_set_tex_maps(&g3d_state_tex_coord_scale_cache, tev);
            fn_80086610(&g3d_state_tex_coord_scale_cache, fn_80086640((StateByte*)&g3d_state_gen_mode_cache));
        }
    }
}

/* Copies the TEV's per-coordinate texture map table into the scale cache when it changed. */
void g3d_tex_coord_scale_set_tex_maps(StatePairTable* pSelf, nw4r::g3d::ResTev tev) {
    const nw4r::g3d::ResTevData& r = tev.ref();
    const u32* pMaps = (const u32*)r.texMapID;
    if (((u32)pMaps & 3) != 0) {
        nw4r::db::Panic(lbl_8058F750, 0x249, "NW4R:Failed assertion ((u32)x & 0x3) == 0");
    }
    if ((pSelf->mFlags & 2) == 0 || pMaps[0] != pSelf->texMapWord[0] || pMaps[1] != pSelf->texMapWord[1]) {
        pSelf->texMapWord[0] = pMaps[0];
        pSelf->texMapWord[1] = pMaps[1];
        pSelf->mFlags = (pSelf->mFlags | 2) & ~1;
    }
}

/* Writes the cached generation mode when it is complete but not yet loaded. */
void g3d_gen_mode_cache_load(G3dGenModeCache* pSelf) {
    if ((pSelf->flags & 4) != 0 && (pSelf->flags & 3) == 2) {
        g3d_gd_set_gen_mode(pSelf->numTexGens, pSelf->numChans, pSelf->numTevStages, pSelf->numIndStages,
                            pSelf->cullMode);
        pSelf->flags |= 1;
    }
}

/* Writes the BP generation-mode register (behind its mask command). */
void g3d_gd_set_gen_mode(u8 numTexGens, u8 numChans, u8 numTevStages, u8 numIndStages, u32 cullMode) {
    fn_800868A0(0xFE07FC3F);
    fn_800868A0(((numIndStages << 16) | (g3d_state_cull_mode_hw[cullMode] << 14)) |
                (((numTevStages - 1) << 10) | (numTexGens | (numChans << 4))));
}

/* Records `tev` as the loaded TEV, returning whether it already was. */
BOOL g3d_tev_cache_update(u32* pCached, nw4r::g3d::ResTev tev) {
    if (!tev.IsValid()) {
        nw4r::db::Panic(lbl_8058F750, 0x3B2, "NW4R:Failed assertion rhs.IsValid()");
    }
    if (*pCached == (u32)tev.ptr()) {
        return TRUE;
    }
    *pCached = (u32)tev.ptr();
    return FALSE;
}

/* Calls the material's pixel display list. */
void g3d_state_load_mat_pix(nw4r::g3d::ResMatPix pix) {
    if (pix.IsValid()) {
        g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
        pix.CallDisplayList(fn_80086770(&g3d_state_dl_dirty));
    }
}

/* Calls the material's indirect-matrix display list for the cached number of indirect stages. */
void g3d_state_load_mat_ind_mtx_dl(nw4r::g3d::ResMatIndMtxAndScale ind) {
    if (ind.IsValid()) {
        g3d_gen_mode_cache_load(&g3d_state_gen_mode_cache);
        bool dirty = fn_80086770(&g3d_state_dl_dirty);
        ind.CallDisplayList(fn_80086AA0((StateByte3*)&g3d_state_gen_mode_cache), dirty);
    }
}

/* Loads the material's indirect matrices: the display list, then the operation the callback may edit. */
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback) {
    if (ind.IsValid()) {
        G3dIndMtxOp op;
        g3d_state_load_mat_ind_mtx_dl(ind);
        nw4r::g3d::ResMatIndMtxAndScale copy = ind;
        g3d_ind_mtx_op_init(&op, &copy);
        pCallback->Exec(&op);
        g3d_ind_mtx_op_load(&op);
    }
}

} /* extern "C" */
