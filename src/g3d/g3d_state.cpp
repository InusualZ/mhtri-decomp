/*
 * nw4r g3d: g3d_state.cpp - the `G3DState` cluster, `.text` 0x8008452C-0x80088E24 (167 functions),
 * plus its `extab` (0x80008ADC-0x80008DE0) and `extabindex` (0x8002148C-0x800218AC) records.  Phase 4 cut the
 * `ResVtx*` accessor tail (0x80088E24-0x800898B0, the second `.data` fragment below) into `g3d_resvtx.cpp`.
 *
 * Registered once, at its final home, from proposal `8008452C` - the 0x8008452C-0x800898B0 maximal
 * unclaimed run.  The evidence:
 *
 *  - **source name**: the run's own `__FILE__` string - `.data` 0x8058F750 holds `"g3d_state.cpp"`
 *    (18 references, the file argument of the `nw4r::db::Panic` asserts from fn_80084630 on, e.g.
 *    fn_80085C70's `mtxID < NUM_CAMERA` at line 0x6C5).  It is the name of the original source file,
 *    so module and name are decided (brief section 2 class 1); `langcheck`'s authority says C++.
 *  - **left seam** 0x8008452C: the enclosing attachment's own anchor `"g3d_anmroot.cpp"`-family ends
 *    there - `tudiscover at 0x80084300` gives the `g3d_scnroot.cpp` TU as 0x800827E4-0x8008452C with
 *    `.data` fragment 0x8058F530-0x8058F74A; this run's fragment starts at 0x8058F750, the next
 *    address.  The seam is a real TU boundary.
 *  - **right seam** 0x800898B0: the registered `g3d/g3d_resanm.c` starts exactly there; `tudiscover
 *    at 0x800894C8` calls it a *strong* cut (`.sdata2` lbl_80795EA0 -> lbl_80795EA8).
 *  - **interior**: a second `.data` fragment opens at 0x8058FCE8 (the `g3d_resvtx_ac.h` /
 *    `ResVtxFurVec` / `ResVtxTexCoord` strings), so the last ~35 functions are a second TU: the reconciled
 *    candidate cuts the run at 0x80088E24 and `g3d_resvtx.cpp` takes the tail (phase 4).
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `tools/symbols/dumpmap.py` over the range and the proposal's 202-entry inventory - every name is a
 * `fn_`/`dtor_` stem, no mangling, so no better name can be derived).
 *
 * **State of the unit.**  Registered and building; 31 of the run's 202 functions are reconstructed
 * here (26 byte-exact, one at 99.93 %, four partial - the table below).  The remaining 171 keep
 * their auto/ units until the next pass; they are not stubbed, because a wrong shape costs the next
 * reader more than a missing body (brief section 5: a function that resists is a residual, and this
 * file's header is where it is recorded).
 *
 * ```text
 * 100.00 %  fn_8008452C fn_80084594 fn_80084600 fn_80085344 fn_80085478 fn_8008569C fn_800856CC
 *           fn_800856D4 fn_80085AF8 fn_80085B00 fn_80085B28 fn_80085B54 fn_80085F3C fn_80086118
 *           fn_80086138 fn_80086154 fn_80086390 fn_80086640 fn_80086760 fn_80086768 fn_80086770
 *           fn_800867A0 fn_800868A0 fn_800868D8 fn_80086AA0 fn_80086FFC
 *  99.93 %  fn_8008455C  (56 B, paired - one relocation/argument differs)
 *  74.17 %  fn_80086610  (48 B target / 36 B ours: the two masked flag tests fuse to the record form
 *                        `rlwinm.`/`clwi.` instead of the target's separate `rlwinm` + `cmpwi`.  Tried:
 *                        early-return chain, one `&&` expression, materialised mask locals - all fuse.)
 *  71.25 %  fn_800868E4  (16 B target / 12 B ours: the target masks the argument with `clrlwi 24`
 *                        before the byte store; `GXWGFifo.u8 = value` stores the low byte directly.
 *                        Tried: `(u8)` cast, a `u8` local, a `u8` parameter - all stay 12 B.)
 *  56.00 %  fn_800856A4  56.00 %  fn_800856B8
 *           (20 B target / 16 B ours: the target materialises `(flags & 0x20) == 0` with `rlwinm` +
 *            `cntlzw` + `srwi 5`; MWCC folds the single-bit test to `extrwi` + `xori` no matter how it
 *            is spelled - bound first, ternary, `if`/`return`, `<= 0`.)
 * ```
 *
 * Not reconstructed yet: the run's larger bodies in address order - fn_80084630 (0x3E4), fn_80084B9C
 * (0x4A0), fn_80084A14 (0x188), fn_80085B6C (0x104), fn_800856F4 (0x3D8), fn_800861E0 (0x13C),
 * fn_8008715C (0x288), fn_80087680 (0x1F0), fn_80088AD0 (the `.ctors` static constructor, 0xD4),
 * fn_80088C28 (0x100) ... plus the tail accessors of the may-be-separate TU documented above.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"              /* GXWGFifo, the 0xCC008000 write window (rule 1) */
#include "unsplit/g3d.h"       /* unsplit g3d neighbours (rule 2) */
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

/* The SDK entry points the target reaches with their own `lis`/`addi` (their bracketing registered
 * units name different modules, so the unsplit band does not carry them - rule 2's named gap).
 * mtx34_identity/MTX34_ctor (owner fn_8004CAD8.cpp) and fn_80075390..fn_80075620 (owner
 * g3d/g3d_camera.cpp) are now declared in those owners' headers and #included above (rule 2). */
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

/* The two write-gather-pipe stores (the window lives in `include/gx.h`, rule 1). */
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
