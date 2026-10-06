/*
 * g3d/g3d_state.cpp - the nw4r g3d `G3DState` cluster: GX state writes and the camera matrix table the
 *   `mtxID < NUM_CAMERA` asserts guard.
 * RANGE. .text 0x8008452C-0x80088E24 (167 functions); extab, extabindex, .ctors 0x8056F2D4-0x8056F2D8, .rodata
 *   0x8056F6D0-0x8056F710, .data 0x8058F750-0x8058FCE8 (opens on "g3d_state.cpp"), .bss 0x80682E80-0x80688420,
 *   .sdata 0x80791238-0x80791258, .sbss 0x807948F0-0x80794910, .sdata2 0x80795E70-0x80795E98.  Left seam:
 *   `g3d/g3d_scnroot.cpp`'s `.data` fragment ends at 0x8058F74A; the right edge is `g3d/g3d_resvtx.cpp`, whose
 *   fragment opens at 0x8058FCE8.
 * NAMES. Map stems (`fn_`/`dtor_`, no mangling to derive from); `mtx34_inverse` (0x800883C4) is a GUESS (the 3x4
 *   matrix inverse its body computes).
 * RESIDUALS. 136 functions unwritten (objdiff scores them zero) in 13 runs: 0x80084630-0x80085344,
 *   0x80085350-0x80085478, 0x800854B8-0x8008569C, 0x800856F4-0x80085AF8, 0x80085B6C-0x80085F3C,
 *   0x80085F60-0x80086118, 0x80086194-0x80086390, 0x80086398-0x80086610, 0x80086648-0x80086760,
 *   0x800867AC-0x800868A0, 0x800868F4-0x80086AA0, 0x80086AA8-0x80086FFC, 0x80087004-0x80088E24 (with the `.ctors`
 *   static constructor fn_80088AD0).
 *   fn_8008455C: one relocation argument differs.
 *   fn_80086610: the two masked flag tests fuse to the record form (`rlwinm.`) where retail keeps `rlwinm` + `cmpwi`
 *     (tried: an early-return chain, one `&&` expression, mask locals).
 *   fn_800868E4: retail masks the argument with `clrlwi 24` before the byte store, ours stores the low byte directly
 *     (tried: a `(u8)` cast, a `u8` local, a `u8` parameter).
 *   fn_800856A4, fn_800856B8: retail materialises `(flags & 0x20) == 0` with `rlwinm` + `cntlzw` + `srwi 5`; MWCC
 *     folds it to `extrwi` + `xori` for every spelling tried.
 *   flipcheck: `.text` 0x394 of 0x48F8; `.ctors`, `.bss`, `.sbss` and every data section are claimed and not emitted.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"              /* GXWGFifo, the 0xCC008000 write window (rule 1) */
#include "g3d/g3d_anmchr.h"    /* fn_8005DC24, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80063888.h"   /* fn_800639D0, owned by g3d/fn_80063888.cpp (rule 2) */
#include "fn_8004CAD8.h"       /* mtx34_identity/MTX34_ctor, owned by fn_8004CAD8.cpp (rule 2) */
#include "g3d/g3d_camera.h"    /* fn_80075390..fn_80075620, owned by g3d/g3d_camera.cpp (rule 2) */
#include "g3d/fn_80075DCC.h"    /* fn_8007B5F4/fn_8007BB8C, owned by g3d/fn_80075DCC.cpp (rule 2) */

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

/* One 4-byte `{u16, u16}` pair of the table at +0x04 (`fn_80085F3C`). */
struct StatePair {
    /* +0x00 */ u16 mA;
    /* +0x02 */ u16 mB;
}; /* size: 0x4 */

/* The pair table and its flag word (`fn_80085F3C`). */
struct StatePairTable {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ StatePair mPairs[8];
}; /* size: 0x24 (approximation - only the head is touched) */

/* The bit mask `fn_80086118` clears a bit of (one byte at +0x100). */
struct StateBitMask {
    /* +0x000 */ u8 pad_000[0x100];
    /* +0x100 */ u8 mMask;
}; /* size: 0x101 (approximation) */

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
void fn_80085350(StateFlags* pSelf, u8 count);
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
void fn_80086118(StateBitMask* pSelf, u32 bit);
void fn_80086138(StateWord3* pDst, const StateWord3* pSrc);
BOOL fn_80086154(const StateWord3* pA, const StateWord3* pB);
u32 fn_80086390(StateWord* pSelf);
void fn_80086610(StateFlags* pSelf, u8 arg);
u8 fn_80086640(StateByte* pSelf);
u32 fn_80086760(StateWord* pSelf);
u32* fn_80086768(void);
u8 fn_80086770(StateByte* pSelf);
void fn_800867A0(StateByte* pSelf);
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

void fn_80086118(StateBitMask* pSelf, u32 bit) {
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

u32 fn_80086390(StateWord* pSelf) {
    return pSelf->mWord;
}

/* The state's dirty-flush hook: only flush when the pending flags say so and the new value differs. */
void fn_80086610(StateFlags* pSelf, u8 arg) {
    if ((pSelf->mFlags & 2) != 0 && (pSelf->mFlags & 1) == 0 && arg != 0) {
        fn_80085350(pSelf, arg);
    }
}

u8 fn_80086640(StateByte* pSelf) {
    return pSelf->mByte;
}

u32 fn_80086760(StateWord* pSelf) {
    return pSelf->mWord;
}

u32* fn_80086768(void) {
    return &lbl_8079124C;
}

/* Read the byte and clear it, returning what it held. */
u8 fn_80086770(StateByte* pSelf) {
    u8 value = pSelf->mByte;
    fn_800867A0(pSelf);
    return value;
}

void fn_800867A0(StateByte* pSelf) {
    pSelf->mByte = 0;
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
