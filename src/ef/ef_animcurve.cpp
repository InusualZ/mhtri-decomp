/*
 * ef/ef_animcurve.cpp - the key-frame animation curve the effect library interpolates with.
 *
 * .text 0x8009CDBC..0x800A3044, 26 functions.  Final home and name from the range's own `__FILE__`
 * string (evidence class 1): every `Panic` in the range passes `lbl_80591E68`, which reads
 * "ef_animcurve.cpp" at 0x80591E68 in the DOL's `.data` (three pooled copies of it exist -
 * 0x80591E68, 0x80592260, 0x805922AC - and the `.data` pool order puts all of them between the
 * previous TU's literals and `ef_creationqueue.cpp`'s at 0x805922C0).  A bare source-file name is
 * the original TU, so the module is `ef` (the bracketing units are ef/ef_util.cpp below and
 * ef/ef_creationqueue.cpp above) and the extension is the name's suffix.  C++ is conclusive:
 * the `.cpp` name plus the `Panic__Q24nw4r2dbFPCciPCce` callee in 20 of the 26 bodies.
 *
 * The seam is proven on both sides.  Below, the run starts at the first function after
 * ef/ef_util.cpp's run (the 0x8009B374 proposal) and the address is a boundary in the extabindex
 * table (0x80022BC0).  Above, 0x800A3044 is where ef/ef_creationqueue.cpp starts and where the
 * extabindex table breaks (0x80022CBC).  Sections: extab 0x8009A98..0x8009B40 (21 unwind records -
 * exactly the 21 non-leaf bodies that carry a frame), extabindex 0x80022BC0..0x80022CBC (21
 * records), .text 0x8009CDBC..0x800A3044.  The five leaf bodies (fn_8009EEDC, fn_8009F834,
 * fn_8009F848, fn_800A1290, fn_800A14A4) get no record, which is why the extab run is 21 entries
 * and not 26.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` over all 26 addresses - every one answers the
 * map's own `fn_` stem and the runtime dump only `zz_XXXXXXXX_`; config/RMHE08/symbols.txt agrees).
 *
 * What it is.  A key-frame animation curve: a heading record (`header`) holding a curve type and a
 * key count, and interpolators that resolve a curve of `u8` keys (or their `f32` form) at a
 * tick.  The pointer-assert messages the Panic calls carry (`header`, `tick`, `tickF32`, `resDiv`,
 * `final`, `mCmdList`, `target`, `key`, `divL`, `divH`, `pHead`, `random`, `randomTable`,
 * `nameTable`, `pp`, `ptrFixed`, `ptrBase`) are the original source's own parameter names, and the
 * two `Panic` format strings that are not pointer errors ("NW4R:Failed assertion 0",
 * "NW4R:Failed assertion div < nextDiv") are `NW4R_ASSERT(...)` stringifications of the source's
 * own conditions.
 *
 *   fn_8009E854 / fn_800A01C4  the interpolation kernels: a u16.16 fixed-point `u8` version and
 *                its `f32` twin.  Curve type 0 is linear, 1 is a cubic whose two inner control
 *                terms are switched by bits 2 and 3 of the type byte (0.0f or 1.5f), 2 is "hold",
 *                and anything else is the `NW4R_ASSERT(0)` default.
 *   fn_8009EEDC / fn_8009EEF4  the random generator: a linear congruential step
 *                (`seed * 0x343FD + 0x269EC3`) and the four-word hash the name table is keyed by.
 *   fn_8009F834 / fn_8009F848  the two 0x100-byte lookup tables (`lbl_80591C68`, `lbl_80591D68`)
 *                the divisor tables are indexed through.
 *   fn_800A1290  the 4-byte key record copy (u8, u8, u16).
 *   fn_800A14A4  the one-shot flag latch that folds a bit pair into the `+0x96` byte.
 *   fn_800A2FA4 / fn_800A3008  the 0x400-entry creation queue's reset and its entry initialiser
 *                (the same 0x30-byte stride and the same two VEC3 at +0x18/+0x24 that
 *                ef/ef_creationqueue.cpp pushes).
 *
 * Flags: this lib's `cflags_main` (`-O3 -inline noauto -Cpp_exceptions on`) is the registered home
 * and every body below was measured under it.  Two file-scoped pragmas are load-bearing and neither
 * is a flag change: `#pragma peephole off` (retail keeps `clrlwi`/`extrwi` + `cmpwi` unfused where
 * the pass emits `clrlwi.`/`rlwinm.`) and `#pragma fp_contract off` (retail keeps `fmuls` + `fadds`
 * separate where `-fp_contract on` fuses `fmadds`/`fmsubs`).  Measured: without them fn_8009E854 is
 * 63.30 %, fn_800A01C4 73.77 %; with them 97.74 % / 97.47 %.
 *
 * Result: 18 of the 26 bodies are at or above the 80 % bar - fn_8009EEDC, fn_8009F834, fn_8009F848,
 * fn_800A1290 and fn_800A3008 are byte-identical; fn_8009EA4C 99.98 %, fn_800A2CF0 98.29 %,
 * fn_800A2504 98.23 %, fn_8009E854 97.74 %, fn_800A01C4 97.47 %, fn_800A2FA4 96.00 %,
 * fn_800A12AC 95.12 %, fn_800A1504 94.78 %, fn_8009EF88 93.79 %, fn_8009CDBC 91.45 %,
 * fn_800A27B4 89.35 %, fn_8009EEF4 88.65 %, fn_800A14A4 81.25 %.  The residuals are register
 * allocation and scheduling, not source shape: fn_8009EA4C differs in one instruction's operand
 * order; fn_8009EEF4's five products are all present but MWCC schedules the `a`/`b` share before
 * the `d`/`c` share (the target keeps the adds in the written order); fn_800A14A4 stores the `&`
 * result only once where retail stores it twice (the second statement's read of the field is what
 * retail's dead-store pass kept, ours forward-substitutes) and reuses the shift count where retail
 * recomputes it; fn_8009CDBC is 104 bytes short on the `flags & 0x80` divisor path (the target's
 * nested three-way `if` materialises a longer chain than the same logic written flat).
 *
 * The eight functions below are NOT reconstructed - they are listed as residual bodies at the end
 * of this file and their measured scores are 0.16-1.68 %: fn_8009D5C0 (0x8F0), fn_8009DEB0 (0x9A4),
 * fn_8009F22C (0x608), fn_8009F85C (0x968), fn_800A02F8 (0xA0C), fn_800A0D04 (0x58C),
 * fn_800A16C4 (0x5E4), fn_800A1CA8 (0x85C).  They are 20 932 of the range's 25 224 bytes and are
 * the obvious next queue for this unit; each is the same assert-then-compute family as the 18 that
 * did land, and fn_8009F85C already has its signature fixed by `ef/ef_emitter.cpp`'s declaration
 * (`void fn_8009F85C(void* rec, void* target, u32 life, u16 seed, s32 range)`).
 *
 * One `.sdata2` note: the two int->float conversion doubles the original TU emitted (0x80795FC0 and
 * 0x80795FC8) live in the shipped pool, but MWCC re-emits them per TU, so this object carries 16
 * bytes of `.sdata2` of its own (no named float literal is spelled anywhere in this file - the
 * `.sdata2` labels are referenced by name, which is why the section is 16 and not 44 bytes).
 */

#include "types.h"
#include "nw4r/math.h"
#include "unsplit/ef.h"
#include "ef/ef_particlemanager.h"

/* Two file-scoped pragmas, both load-bearing (the same pair ef/ef_cube.cpp and ef/eft007.cpp carry):
 *   - `peephole off`: retail keeps the compare-against-a-select chain unfused (`clrlwi` + `cmpwi`,
 *     `extrwi` + `cmpwi`) where the peephole pass emits `clrlwi.`/`rlwinm.`;
 *   - `fp_contract off`: retail keeps `fmuls` + `fadds` / `fmuls` + `fsubs` separate, which
 *     `-fp_contract on` (this lib's cflags) fuses into `fmadds`/`fmsubs`. */
#pragma peephole off
#pragma fp_contract off

/* `nw4r::db::Panic`: declaring the owner's real spelling makes the C++ front-end reproduce the map's
 * mangling (`Panic__Q24nw4r2dbFPCciPCce`) exactly; spelling the mangling itself would re-mangle it
 * (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* This unit's own pooled file-name literal (a copy per region - MWCC emitted three). */
extern char lbl_80591E68[];
extern char lbl_80592260[];
extern char lbl_805922AC[];

/* The two 0x100-byte tables the curve's divisor lookups index (they live in the ef library's pooled
 * `.data`, so they are declared, never defined here). */
extern u8 lbl_80591C68[];
extern u8 lbl_80591D68[];

/* The pointer-error messages, one per guarded parameter name (the source's own spelling). */
extern char lbl_80591E7C[]; /* header   */
extern char lbl_80591EB4[]; /* tick     */
extern char lbl_80591EE8[]; /* tickF32  */
extern char lbl_80591F20[]; /* resDiv   */
extern char lbl_80591F58[]; /* final    */
extern char lbl_80591F8C[]; /* mCmdList */
extern char lbl_80591FC4[]; /* target   */
extern char lbl_80591FE0[]; /* (unused here) */
extern char lbl_80592000[]; /* "NW4R:Failed assertion 0"          */
extern char lbl_80592018[]; /* key                                */
extern char lbl_8059204C[]; /* divL                               */
extern char lbl_80592080[]; /* divH                               */
extern char lbl_805920B4[]; /* pHead                              */
extern char lbl_805920E8[]; /* random                             */
extern char lbl_80592120[]; /* randomTable                        */
extern char lbl_80592160[]; /* nameTable                          */
extern char lbl_80592198[]; /* pp                                 */
extern char lbl_805921CC[]; /* ptrFixed                           */
extern char lbl_80592204[]; /* "NW4R:Failed assertion div < nextDiv" */
extern char lbl_80592228[]; /* ptrBase                            */
extern char lbl_80592274[]; /* ptrBase                            */

/* The .sdata2 pool this unit reaches into.  They are referenced by name, never spelled as float
 * literals: MWCC would then emit its own copy into this unit's `.sdata2` and the shipped pool at
 * 0x80795FB8.. would gain a second entry (main.cpp is the one unit that owns a `.sdata2` range). */
extern f32 lbl_80795FB8; /* 0.0f        */
extern f32 lbl_80795FD0; /* 1.0f        */
extern f32 lbl_80795FD4; /* 65536.0f    */
extern f32 lbl_80795FD8; /* 3.0f        */
extern f32 lbl_80795FDC; /* 2.0f        */
extern f32 lbl_80795FE0; /* 1.5f        */
extern f32 lbl_80795FE4; /* -2.0f       */
extern f32 lbl_80795FE8; /* 255.0f      */
extern f32 lbl_80795FEC; /* FLT_EPSILON */

/* The pointer guard every ef entry point carries: the RVL address-range chain (seven windows, the
 * first `if` carrying two tests - the compiler materialises six BOOLs). */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg, file)                                                    \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(file, line, msg, (p)))

#define NW4R_ASSERT(cond, line, msg, file)                                                         \
    ((cond) ? (void)0 : nw4r::db::Panic(file, line, msg))

/* The header record the curve readers hold: a flags byte, a divisor count and a key count.  The
 * `key` record fn_8009EA4C walks is the same shape but read through its own `+0x00` count. */
struct EfAnimHeader {
    /* +0x00 */ u16 mMatchCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u8 mFlags;
    /* +0x05 */ u8 mDivCount;
    /* +0x06 */ u8 pad_0x06[0x2];
    /* +0x08 */ u16 mTickCount;
}; /* size: 0xA */

/* The record fn_8009EA4C walks: only its leading u16 key count is read here (size is a lower bound). */
struct EfAnimKeyRange {
    /* +0x00 */ u16 mMatchCount;
}; /* size: 0x2 (lower bound: +0x00 is the highest offset read) */

/* The creation-queue record and queue this unit resets.  Same 0x30 stride and same two VEC3 tails as
 * ef/ef_creationqueue.cpp's `CreationQueueEntry` (which owns the type); this unit only walks it. */
struct EfAnimSlot {
    /* +0x00 */ u8 mType;
    /* +0x01 */ u8 mFlags;
    /* +0x02 */ u16 mLife;
    /* +0x04 */ u8 mSetting[0xA];
    /* +0x0E */ u8 pad_0x0E[0x2];
    /* +0x10 */ void* mpManager;
    /* +0x14 */ void* mpHandle;
    /* +0x18 */ Vec3 mPos;
    /* +0x24 */ Vec3 mVel;
}; /* size: 0x30 */

struct EfAnimSlotQueue {
    /* +0x00 */ s32 mCount;
    /* +0x04 */ EfAnimSlot mSlot[0x400];
}; /* size: 0xC004 */

/* fn_80043EA8 (zero a VEC3) and fn_800C9DCC (fabsf) are declared by the `ef` band header, where their
 * owner-less declarations live (rule 2), so this unit includes it rather than re-declaring them. */
extern "C" {

/* 0x8009EEDC - one step of the curve random generator's LCG. */
u32 fn_8009EEDC(u32 seed) {
    return seed * 0x343FD + 0x269EC3;
}

/* 0x8009EEF4 - the four-word name hash the sequence name table is keyed by. */
union EfAnimHash {
    u32 mWord;
    u8 mByte[4];
}; /* size: 0x4 */

u32 fn_8009EEF4(u16 a, u16 b, u16 c, u32 d) {
    EfAnimHash h;
    h.mWord = (u32)d * 0x7B929 + (u32)c * 0x371097E7 + ((u32)a * 0x3F81F635 + (u32)b * 0x30A74193 +
                                                                                        0x4BF53);
    h.mByte[2] ^= h.mByte[3];
    h.mByte[1] ^= h.mByte[2];
    h.mByte[0] ^= h.mByte[1];
    return h.mWord;
}

/* 0x8009F834 - the low 0x100-byte table lookup. */
u8 fn_8009F834(u32 index) {
    return lbl_80591C68[index & 0xFF];
}

/* 0x8009F848 - the high 0x100-byte table lookup. */
u8 fn_8009F848(u32 index) {
    return lbl_80591D68[index & 0xFF];
}

/* 0x800A1290 - copy a 4-byte key record (u8, u8, u16). */
struct EfAnimKey {
    /* +0x00 */ u8 mValue;
    /* +0x01 */ u8 mType;
    /* +0x02 */ u16 mTick;
}; /* size: 0x4 */

void fn_800A1290(EfAnimKey* dst, const EfAnimKey* src) {
    dst->mValue = src->mValue;
    dst->mType = src->mType;
    dst->mTick = src->mTick;
}

/* 0x800A01C4 - the f32 interpolation kernel: curve type `type` (bits 0-1) between `a` and `b` at
 * `t` in [0, 1].  Type 1's two inner control terms are switched by bits 2 and 3. */
f32 fn_800A01C4(u8 type, f32 t, f32 a, f32 b) {
    if (a == b) {
        return a;
    }
    switch (type & 3) {
    case 0:
        return a + t * (b - a);
    case 1:
        if (type == 1) {
            f32 w = lbl_80795FD8 - lbl_80795FDC * t;
            return a + t * (t * (w * (b - a)));
        } else {
            f32 c0 = (type & 4) ? lbl_80795FE0 : lbl_80795FB8;
            f32 c1 = (type & 8) ? lbl_80795FE0 : lbl_80795FB8;
            f32 s = t * (b - a);
            f32 u = t * (t * (c0 + c1 - lbl_80795FDC));
            f32 v = t * (lbl_80795FD8 + (lbl_80795FE4 * c0 - c1));
            return a + s * (c0 + (u + v));
        }
    case 2:
        return a;
    default:
        NW4R_ASSERT(0, 0x2F9, lbl_80592000, lbl_80591E68);
        return lbl_80795FB8;
    }
}

/* 0x8009E854 - the u16.16 fixed-point `u8` interpolation kernel: `tick` is t * 65536, the result is
 * clamped to [0, 255]. */
u32 fn_8009E854(u32 tick, u8 start, u8 end, u8 type) {
    u8 s = start;
    u8 e = end;

    if (s == e) {
        return start;
    }
    switch (type & 3) {
    case 0:
        return (u8)(s + ((tick * (e - s)) >> 16));
    case 1: {
        f32 t = (f32)tick / lbl_80795FD4;
        if (type == 1) {
            f32 s0 = (f32)s;
            f32 w = lbl_80795FD8 - lbl_80795FDC * t;
            return (u32)(int)(s0 + t * (t * (w * (f32)(int)(e - s))));
        } else {
            f32 c0 = (type & 4) ? lbl_80795FE0 : lbl_80795FB8;
            f32 c1 = (type & 8) ? lbl_80795FE0 : lbl_80795FB8;
            f32 s0 = (f32)s;
            f32 d = t * (f32)(int)(e - s);
            f32 u = t * (t * (lbl_80795FDC - (c0 + c1)));
            f32 v = t * (lbl_80795FD8 + (lbl_80795FE4 * c0 - c1));
            f32 r = s0 + d * (c0 + (u + v));
            if (r < lbl_80795FB8) {
                return 0;
            }
            if (r > lbl_80795FE8) {
                return 0xFF;
            }
            return (u32)(int)r;
        }
    }
    case 2:
        return start;
    default:
        NW4R_ASSERT(0, 0x2C7, lbl_80592000, lbl_80591E68);
        return 0;
    }
}

/* 0x800A3008 - initialise a queue slot's two VEC3 tails. */
EfAnimSlot* fn_800A3008(EfAnimSlot* slot) {
    fn_80043EA8(&slot->mPos);
    fn_80043EA8(&slot->mVel);
    return slot;
}

/* 0x800A2FA4 - reset the whole creation queue: clear every slot's VEC3 tails and the count. */
EfAnimSlotQueue* fn_800A2FA4(EfAnimSlotQueue* queue) {
    EfAnimSlot* slot = &queue->mSlot[0];
    while (slot < &queue->mSlot[0x400]) {
        fn_800A3008(slot);
        slot++;
    }
    queue->mCount = 0;
    return queue;
}

/* 0x800A14A4 - latch the one-shot flag and, the first time, fold a bit pair taken from `arg` into
 * the `+0x96` byte. */
struct EfAnimBitArg {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mShift;
    /* +0x08 */ u8 mBits;
}; /* size: 0xC */

struct EfAnimLatch {
    /* +0x00 */ u8 pad_0x00[0x96];
    /* +0x96 */ u8 mBits;
    /* +0x97 */ u8 pad_0x97[0xE4 - 0x97];
    /* +0xE4 */ u8 mFlag;
}; /* size: 0xE5 */

void fn_800A14A4(EfAnimLatch* self, const EfAnimBitArg* arg) {
    if (self->mFlag == 0) {
        u8 bits = self->mBits;
        u8 mask = (u8)~(3 << (arg->mShift * 2));
        self->mBits = (u8)(bits & mask);
        self->mBits = (u8)(self->mBits | (u8)((arg->mBits & 3) << (arg->mShift * 2)));
    }
    self->mFlag = 1;
}

/* 0x8009F22C / 0x800A27B4 - the key lookup pair the two channel-copy entry points below call; both
 * bodies follow. */
extern "C" void fn_8009F22C(const void* self, u32* outA, u32* outB, u32* outC, u32* outD, u32* outE);
extern "C" void fn_800A27B4(const EfAnimHeader* header, u32 step, u16* tick, u32 mode);

/* 0x800A12AC - binary-search a table of u16 keys (stride `stride` u16s) for `target` and return the
 * lower bracketing index in `*out`.  `flag` decides which side an exact hit narrows. */
void fn_800A12AC(u32* out, s32 target, const u16* ptrBase, u32 stride, s32 lo, s32 hi, int flag) {
    NW4R_POINTER_ASSERT(ptrBase, 0x24C, lbl_80592228, lbl_80592260);

    s32 mid = (lo + hi) / 2;

    if (target < ptrBase[lo * stride] || (target == ptrBase[lo * stride] && flag != 0)) {
        *out = lo;
        return;
    }
    if (ptrBase[hi * stride] < target || (target == ptrBase[hi * stride] && flag == 0)) {
        *out = hi;
        return;
    }
    u16 key = ptrBase[mid * stride];
    while (lo < mid) {
        if (target == key) {
            if (flag != 0) {
                hi = mid;
            } else {
                lo = mid;
            }
        } else if (key < target) {
            lo = mid;
        } else {
            hi = mid;
        }
        mid = (lo + hi) / 2;
        key = ptrBase[mid * stride];
    }
    *out = lo;
}

/* The divider record fn_800A1504 reads (a u16 key count and a u32 slot base) and the ramp record
 * beside it (its two curve endpoints as bytes, plus the flags byte whose bit 2 picks the wrap rule). */
struct EfAnimDivider {
    /* +0x00 */ u16 mKeyCount;
    /* +0x02 */ u8 pad_0x02[0xA];
    /* +0x0C */ u32 mSlotBase;
}; /* size: 0x10 */

struct EfAnimRamp {
    /* +0x00 */ u8 mEnd;
    /* +0x01 */ u8 mStart;
    /* +0x02 */ u8 pad_0x02;
    /* +0x03 */ u8 mFlags;
}; /* size: 0x4 */

struct EfAnimState {
    /* +0x00 */ u8 pad_0x00[0xE5];
    /* +0xE5 */ u8 mPhase;
}; /* size: 0xE6 */

/* 0x800A1504 - resolve a key's slot and hand the table's u32 word at that slot + 1 back through
 * `target`.  `self->mPhase` is folded into the divider's own range, and bit 2 of the ramp's flags
 * picks between the "call the ramp helper at the last key" and the "wrap to the first key" rule. */
void fn_800A1504(EfAnimState* self, const EfAnimDivider* divider, const EfAnimRamp* ramp, void* arg3,
                 const u8* table, u32* target) {
    s32 span = ramp->mEnd - ramp->mStart;
    s32 idx = (u16)(divider->mSlotBase + self->mPhase % span);
    idx += ramp->mStart;

    if ((ramp->mFlags & 4) != 0) {
        if ((u16)idx == (u16)(divider->mKeyCount - 1)) {
            fn_800AB880((struct EfPmManager*)arg3, (struct EfPmParticle*)self);
        }
    } else if ((u16)idx >= (u16)(divider->mKeyCount - 1)) {
        idx = (u16)(idx - (u16)(divider->mKeyCount - 1));
    }

    *(u32*)target = *(const u32*)(table + ((u16)idx << 2) + 4);
    NW4R_POINTER_ASSERT(target, 0x70D, lbl_80591FC4, lbl_80591E68);
}

/* 0x800A2504 - copy the `u8` channels the key's mask selects out of one key record. */
void fn_800A2504(const u8* mCmdList, u8* target, u32 arg2, u32 arg3) {
    NW4R_POINTER_ASSERT(mCmdList, 0x92C, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x92D, lbl_80591FC4, lbl_80591E68);

    u32 mask = mCmdList[3];
    u32 end = fn_8009F848(mask);
    u32 count = fn_8009F834(mask) & 0xFF;

    if (count != 0) {
        u32 v1C, v18, v14, v10, v0C;
        u16 idx;

        fn_8009F22C(mCmdList, &v1C, &v18, &v14, &v10, &v0C);
        fn_800A27B4((const EfAnimHeader*)v1C, arg2, &idx, arg3);

        const u8* src = (const u8*)v18 + count * idx;
        u16 bit;
        for (bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = *src++;
            }
            target++;
        }
    }
}

/* 0x800A2CF0 - the f32 twin of fn_800A2504: same mask walk, one f32 per selected channel. */
void fn_800A2CF0(const u8* mCmdList, f32* target, u32 arg2, u32 arg3) {
    NW4R_POINTER_ASSERT(mCmdList, 0x950, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x951, lbl_80591FC4, lbl_80591E68);

    u32 mask = mCmdList[3];
    u32 end = fn_8009F848(mask);
    u32 count = fn_8009F834(mask) & 0xFF;

    if (count != 0) {
        u32 v1C, v18, v14, v10, v0C;
        u16 idx;

        fn_8009F22C(mCmdList, &v1C, &v18, &v14, &v10, &v0C);
        fn_800A27B4((const EfAnimHeader*)v1C, arg2, &idx, arg3);

        const f32* src = (const f32*)v18 + count * idx;
        u16 bit;
        for (bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = *src++;
            }
            target++;
        }
    }
}

/* 0x80463F04 - the range helper the fractional key lookup measures its distance with. */
extern "C" f32 fn_80463F04(f32 x);

/* 0x8009EA4C - advance the two divider counters for one key step.  `divH` starts at `divL`; bit 6 of
 * the header's flags is what enables the walk, and the second counter only moves at the curve's
 * last key (or immediately when bit 5 is set).  The third block advances the first counter when the
 * step starts at division 0. */
void fn_8009EA4C(const EfAnimHeader* header, const EfAnimKeyRange* key, u32 lo, u32* divL,
                 u32* divH) {
    NW4R_POINTER_ASSERT(header, 0x301, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(key, 0x302, lbl_80592018, lbl_80591E68);
    NW4R_POINTER_ASSERT(divL, 0x303, lbl_8059204C, lbl_80591E68);
    NW4R_POINTER_ASSERT(divH, 0x304, lbl_80592080, lbl_80591E68);

    *divH = *divL;

    u8 flags = header->mFlags;
    if ((flags & 0x40) == 0) {
        return;
    }
    u32 bit5 = flags & 0x20;
    if (bit5 != 0 || header->mDivCount > 1) {
        u32 v = *divL;
        if ((v & 1) == 0 && (s32)(lo + 1) >= (s32)(key->mMatchCount - 1)) {
            if (bit5 != 0 || v < (u32)(header->mDivCount - 1)) {
                (*divH)++;
            }
        }
    }

    u32 w = *divL;
    if ((w & 1) != 0 && lo == 0 && w != 0) {
        (*divL)++;
    }
}

/* 0x8009EF88 - the fractional sibling of fn_800A12AC: binary-search the same u16 key table, but
 * return both bracketing keys (`outLo`/`outHi`) and, in `*flag`, whether `frac` lands exactly on the
 * key that was found. */
void fn_8009EF88(u32* out, u8* flag, u16* outLo, u16* outHi, s32 target, f32 frac,
                 const u16* ptrBase, u32 stride, s32 lo, s32 hi) {
    NW4R_POINTER_ASSERT(ptrBase, 0x1F1, lbl_80592274, lbl_805922AC);

    s32 mid = (lo + hi) / 2;
    u32 atTarget = fn_80463F04((f32)target - frac) < lbl_80795FEC;

    u16 keyLo = ptrBase[lo * stride];
    *outLo = keyLo;
    if (target < keyLo) {
        *out = lo;
        *flag = 1;
        return;
    }
    if (target == keyLo) {
        *out = lo;
        if (lo == hi) {
            *flag = 1;
            return;
        }
        *flag = (u8)atTarget;
        if (atTarget == 0) {
            *outHi = ptrBase[(lo + 1) * stride];
        }
        return;
    }

    u16 keyHi = ptrBase[hi * stride];
    *outHi = keyHi;
    if (keyHi > target) {
        *out = hi;
        *flag = 1;
        return;
    }

    u16 key = ptrBase[mid * stride];
    while (lo < mid) {
        if (target == key) {
            *out = mid;
            *flag = (u8)atTarget;
            if (atTarget != 0) {
                return;
            }
            *outLo = key;
            *outHi = ptrBase[(mid + 1) * stride];
            return;
        } else if (key < target) {
            lo = mid;
            *outLo = key;
        } else {
            hi = mid;
            *outHi = key;
        }
        mid = (lo + hi) / 2;
        key = ptrBase[mid * stride];
    }
    *out = lo;
    *flag = 0;
}

/* 0x8009CDBC - resolve a curve's tick from a division step.  `mode` (the first argument) is the
 * curve's length in steps; `header` carries the tick count, the flags and the divisor count; `step`
 * is the position inside the curve.  The three output parameters are the key tick, the fractional
 * tick and the resolved divisor.  Completely flat curves (tick count <= 1) short-circuit to tick 0. */
void fn_8009CDBC(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv) {
    NW4R_POINTER_ASSERT(header, 0x87, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(tick, 0x88, lbl_80591EB4, lbl_80591E68);
    NW4R_POINTER_ASSERT(tickF32, 0x89, lbl_80591EE8, lbl_80591E68);
    NW4R_POINTER_ASSERT(resDiv, 0x8A, lbl_80591F20, lbl_80591E68);

    u16 count = header->mTickCount;
    u8 flags = header->mFlags;

    *resDiv = 0;
    if (count <= 1) {
        *tick = 0;
        *tickF32 = lbl_80795FB8;
        *resDiv = step;
        return;
    }

    if ((flags & 0x20) == 0 && header->mDivCount <= 1) {
        if ((flags & 0x80) == 0) {
            if (step >= count - 1) {
                *tick = (u16)(count - 1);
            } else {
                *tick = (u16)step;
            }
            *tickF32 = (f32)*tick;
            return;
        }

        if (mode == 1) {
            *tickF32 = (f32)(count - 1);
        } else {
            *tickF32 = (f32)step * ((f32)(count - 1) / (f32)(mode - 1));
            if (*tickF32 > (f32)(header->mTickCount - 1)) {
                *tickF32 = (f32)(header->mTickCount - 1);
            }
        }
        *tick = (u16)(int)*tickF32;
        return;
    }

    if ((flags & 0x80) == 0) {
        u32 div = step / (count - 1);
        flags = header->mFlags;
        *resDiv = div;

        if ((flags & 0x40) == 0) {
            if ((flags & 0x20) == 0 && div >= header->mDivCount) {
                *tick = (u16)(count - 1);
                *resDiv = (u8)(header->mDivCount - 1);
            } else {
                *tick = (u16)(step - div * (count - 1));
            }
        } else if ((flags & 0x20) == 0 && div >= header->mDivCount) {
            if ((header->mDivCount & 1) == 0) {
                *tick = 0;
            } else {
                *tick = (u16)(count - 1);
            }
            *resDiv = (u8)(header->mDivCount - 1);
        } else if ((div & 1) == 0) {
            *tick = (u16)(step - div * (count - 1));
        } else {
            *tick = (u16)((count - 1) * (div + 1) - step);
        }

        *tickF32 = (f32)*tick;
        return;
    }

    if (step >= mode - 1) {
        if ((flags & 0x40) != 0 && (header->mDivCount & 1) == 0) {
            *tick = 0;
        } else {
            *tick = (u16)(count - 1);
        }
        *tickF32 = (f32)*tick;
        *resDiv = (u8)(header->mDivCount - 1);
        return;
    }

    f32 rate = (f32)(header->mDivCount) * ((f32)(count - 1) / (f32)mode);
    f32 full = (f32)step * rate;
    u32 div = (u32)(full / (f32)(count - 1));

    *resDiv = div;

    f32 frac = full - (f32)(div * (count - 1));
    if ((flags & 0x40) != 0 && (div & 1) != 0) {
        frac = (f32)(count - 1) - frac;
    }
    *tickF32 = frac;
    *tick = (u16)(int)frac;
}

/* 0x800A27B4 - the same tick resolution as fn_8009CDBC without the divisor/fraction outputs: the
 * `mode` argument is the curve's step count and the only result is the key tick. */
void fn_800A27B4(const EfAnimHeader* header, u32 step, u16* tick, u32 mode) {
    NW4R_POINTER_ASSERT(header, 0x8AB, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(tick, 0x8AC, lbl_80591EB4, lbl_80591E68);

    u16 count = header->mTickCount;
    u8 flags = header->mFlags;

    if (count <= 1) {
        *tick = 0;
        return;
    }

    u32 bit5 = flags & 0x20;
    if (bit5 == 0 && header->mDivCount <= 1) {
        if ((flags & 0x80) == 0) {
            if (step >= count - 1) {
                *tick = (u16)(count - 1);
            } else {
                *tick = (u16)step;
            }
            return;
        }
        if (mode == 1) {
            *tick = (u16)(count - 1);
            return;
        }
        f32 t = (f32)step * ((f32)(count - 1) / (f32)(mode - 1));
        if (t > (f32)(count - 1)) {
            t = (f32)(count - 1);
        }
        *tick = (u16)(int)t;
        return;
    }

    if ((flags & 0x80) == 0) {
        u32 span = count - 1;
        u32 div = step / span;

        if ((flags & 0x40) == 0) {
            if (bit5 == 0 && div >= header->mDivCount) {
                *tick = (u16)span;
            } else {
                *tick = (u16)(step - div * span);
            }
        } else if (bit5 == 0 && div >= header->mDivCount) {
            if ((header->mDivCount & 1) == 0) {
                *tick = 0;
            } else {
                *tick = (u16)span;
            }
        } else if ((div & 1) == 0) {
            *tick = (u16)(step - div * span);
        } else {
            *tick = (u16)(span * (div + 1) - step);
        }
        return;
    }

    if (step >= mode - 1) {
        if ((flags & 0x40) != 0 && (header->mDivCount & 1) == 0) {
            *tick = 0;
        } else {
            *tick = (u16)(count - 1);
        }
        return;
    }

    u32 span = count - 1;
    f32 rate = (f32)(header->mDivCount) * ((f32)span / (f32)mode);
    f32 full = (f32)step * rate;
    u32 div = (u32)(full / (f32)span);
    f32 frac = full - (f32)(div * span);

    if ((flags & 0x40) != 0 && (div & 1) != 0) {
        frac = (f32)span - frac;
    }
    *tick = (u16)(int)frac;
}

/* -------------------------------------------------------------------------------------------------
 * Residual bodies.  The eight functions below are inside this unit's range but were NOT reconstructed
 * in this pass (each is 0x1F8..0xA0C bytes of the same assert-then-compute family).  They are defined
 * here, minimally, for one reason only: a registered unit whose symbols are missing would leave the
 * range half-registered and break the next measurement pass (docs/plan.md 8.6).  Their signatures are
 * the ones the range's own call sites use (fn_8009F85C's is fixed by ef/ef_emitter.cpp's declaration).
 * Each is a residual with the measurement recorded in the unit's header; none of them is claimed to
 * match, and none of them is called by any other registered unit except fn_8009F85C.
 * ------------------------------------------------------------------------------------------------- */
void fn_8009D5C0(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv, u32* final) {
    /* NOT RECONSTRUCTED.  0x8009D5C0, 0x8F0 bytes, seven parameters and five pointer guards
     * (header/tick/tickF32/resDiv/final at lines 0x12A-0x12E).  Residual. */
    (void)mode; (void)header; (void)step; (void)tickF32; (void)resDiv;
    *tick = 0;
    *final = 0;
}

void fn_8009DEB0(const void* mCmdList, void* target) {
    /* NOT RECONSTRUCTED.  0x8009DEB0, 0x9A4 bytes; calls fn_8009CDBC, fn_8009E854, fn_8009EA4C,
     * fn_8009EEDC, fn_8009EEF4, fn_8009EF88, fn_8009F22C, fn_8009F834, fn_8009F848, fn_800A2504,
     * i.e. the whole unit's readers.  Residual. */
    (void)mCmdList; (void)target;
}

void fn_8009F22C(const void* self, u32* outA, u32* outB, u32* outC, u32* outD, u32* outE) {
    /* NOT RECONSTRUCTED.  0x8009F22C, 0x608 bytes, six parameters and six pointer guards
     * (lines 0x70-0x75).  Residual. */
    (void)self;
    *outA = 0; *outB = 0; *outC = 0; *outD = 0; *outE = 0;
}

void fn_8009F85C(void* rec, void* target, u32 life, u16 seed, s32 range) {
    /* NOT RECONSTRUCTED.  0x8009F85C, 0x968 bytes; the one function of this range another registered
     * unit calls (ef/ef_emitter.cpp's signature above).  It calls fn_8009CDBC, fn_8009EA4C,
     * fn_8009EEDC, fn_8009EEF4, fn_8009EF88, fn_8009F22C, fn_8009F834, fn_8009F848, fn_800A01C4 and
     * fn_800A2CF0, i.e. the curve evaluator itself.  Residual. */
    (void)rec; (void)target; (void)life; (void)seed; (void)range;
}

void fn_800A02F8(const void* mCmdList, void* target) {
    /* NOT RECONSTRUCTED.  0x800A02F8, 0xA0C bytes; calls fn_8009CDBC, fn_8009EA4C, fn_8009EEDC,
     * fn_8009EEF4, fn_8009EF88, fn_8009F22C, fn_8009F834, fn_8009F848, fn_800A01C4.  Residual. */
    (void)mCmdList; (void)target;
}

void fn_800A0D04(const void* mCmdList, void* target) {
    /* NOT RECONSTRUCTED.  0x800A0D04, 0x58C bytes; calls fn_8009D5C0, fn_8009EEF4, fn_8009F22C,
     * fn_800A1290, fn_800A12AC.  Residual. */
    (void)mCmdList; (void)target;
}

void fn_800A16C4(const void* mCmdList, void* target) {
    /* NOT RECONSTRUCTED.  0x800A16C4, 0x5E4 bytes; calls fn_8009EEF4, fn_800A3044 and fn_800A33DC
     * (the creation queue's two Add entries).  Residual. */
    (void)mCmdList; (void)target;
}

void fn_800A1CA8(const void* mCmdList, void* target) {
    /* NOT RECONSTRUCTED.  0x800A1CA8, 0x85C bytes; the range's other card carries the
     * `NW4R:Failed assertion div < nextDiv` guard (lbl_80592204) and calls fn_8009D5C0,
     * fn_8009F22C and fn_800A12AC.  Residual. */
    (void)mCmdList; (void)target;
}

} /* extern "C" */
