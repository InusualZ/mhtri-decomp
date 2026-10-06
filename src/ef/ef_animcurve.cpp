/*
 * ef/ef_animcurve.cpp - the effect library's key-frame animation curve: the tick resolver, the `u8` (u16.16
 *   fixed-point) and `f32` interpolation kernels (type 0 linear, 1 cubic with bits 2/3 picking 0.0f or 1.5f inner
 *   terms, 2 hold, else `NW4R_ASSERT(0)`), the divider walk, the key searches, the random generator (`seed *
 *   0x343FD + 0x269EC3`) and name hash, the two 0x100-byte divisor tables' getters, the key record copy, the flag
 *   latch, and the curve commands that feed `ef/ef_creationqueue.cpp`'s 0x400-entry queue.
 * RANGE. .text 0x8009CDBC-0x800A3044 (26 functions); extab 0x80009A98-0x80009B40, extabindex 0x80022BC0-0x80022CBC
 *   (21 records: the five leaf bodies have none), .data 0x80591C68-0x805922C0 (the divisor tables, then three
 *   copies of the `__FILE__` string "ef_animcurve.cpp" at 0x80591E68, 0x80592260 and 0x805922AC), .sdata2
 *   0x80795FB8-0x80795FF0.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps `clrlwi`/`extrwi` + `cmpwi` unfused) and
 *   `#pragma fp_contract off` (retail keeps `fmuls` + `fadds`/`fsubs` apart).
 * NAMES. The map has only `fn_` stems for the range; the parameter names are the source's own, from the pointer
 *   messages (`header`, `tick`, `tickF32`, `resDiv`, `final`, `mCmdList`, `target`, `key`, `divL`, `divH`, `pHead`,
 *   `random`, `randomTable`, `nameTable`, `pp`, `ptrFixed`, `ptrBase`).
 * RESIDUALS. 8 rows unwritten (minimal bodies): 0x8009D5C0-0x8009E854, 0x8009F22C-0x8009F834,
 *   0x8009F85C-0x800A01C4, 0x800A02F8-0x800A1290, 0x800A16C4-0x800A2504.  The source order differs from retail's
 *   (these eight are defined last), so our `.text`, extab and extabindex run in another order.
 *   13 partial rows:
 *  - `fn_8009CDBC` (ours 0x79C of 0x804): the `flags & 0x80` divisor path is short; retail's nested three-way `if`
 *    materialises a longer chain than the flat logic;
 *  - `fn_8009EA4C`: one instruction's operand order;
 *  - `fn_8009EEF4`: the five products are scheduled `a`/`b` before `d`/`c` (retail keeps the written order);
 *  - `fn_800A14A4` (ours 0x58 of 0x60): the `&` result is stored once where retail stores it twice, and the shift
 *    count is reused where retail recomputes it;
 *  - `fn_8009E854`: float register colours and two branch senses;
 *  - `fn_8009EF88` (ours 0x2C4 of 0x2A4), `fn_800A12AC` (ours 0x210 of 0x1F8): extra `slwi r0,r0,1` index scaling
 *    and `clrlwi` narrowings around the u16 key table;
 *  - `fn_800A01C4`: the type-bit tests are `rlwinm` + `beq` where retail has `extrwi` + `bne`, and two pool loads
 *    swap order;
 *  - `fn_800A1504`: a `clrlwi` + `cmplw` compare where retail compares signed (`cmpw`);
 *  - `fn_800A2504`, `fn_800A2CF0`: the u8 argument is narrowed straight into r25 where retail first saves it (`mr
 *    r27,r3`);
 *  - `fn_800A27B4` (ours 0x4E4 of 0x53C): 27 retail instructions are missing and one load is hoisted;
 *  - `fn_800A2FA4` (ours 0x68 of 0x64): our loop adds a pre-test `b`.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x10 of 0x38 (only the two conversion doubles MWCC
 *   re-emits); `.text` 0x2260 of 0x6288; extab 0x68 of 0xA8; extabindex 0x9C of 0xFC.
 * SHAPES. The `.sdata2` constants are referenced by name, never spelled as literals (a literal re-pools them).
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef/ef_torus.h" /* fn_800C9DCC (rule 2) */
#include "ef/ef_particlemanager.h"

#pragma peephole off
#pragma fp_contract off

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* This unit's own pooled file-name literal (a copy per region - MWCC emitted three). */
extern char lbl_80591E68[];
extern char lbl_80592260[];
extern char lbl_805922AC[];

/* The two 0x100-byte tables the curve's divisor lookups index (this unit's claimed `.data`, declared,
 * never defined). */
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

/* The `.sdata2` constants, referenced by name (a float literal would re-pool them in this object). */
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

/* fn_800C9DCC (fabsf) is declared by its owner's header `ef/ef_torus.h` (rule 2), so this unit includes it rather
 * than re-declaring it. */
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

/* 0x800A1290 - copies a 4-byte key record (u8, u8, u16). */
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

/* 0x800A3008 - initialises a queue slot's two VEC3 tails. */
EfAnimSlot* fn_800A3008(EfAnimSlot* slot) {
    VEC3_ctor(&slot->mPos);
    VEC3_ctor(&slot->mVel);
    return slot;
}

/* 0x800A2FA4 - resets the whole creation queue: clear every slot's VEC3 tails and the count. */
EfAnimSlotQueue* fn_800A2FA4(EfAnimSlotQueue* queue) {
    EfAnimSlot* slot = &queue->mSlot[0];
    while (slot < &queue->mSlot[0x400]) {
        fn_800A3008(slot);
        slot++;
    }
    queue->mCount = 0;
    return queue;
}

/* 0x800A14A4 - latches the one-shot flag and, the first time, fold a bit pair taken from `arg` into
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

/* 0x800A12AC - binary-searches a table of u16 keys (stride `stride` u16s) for `target` and return the
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

/* 0x800A1504 - resolves a key's slot (`mPhase` folded into the divider's range) and returns the table word
 * at slot + 1 through `target`; ramp flag bit 2 picks calling the ramp helper or wrapping at the last key. */
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

/* 0x800A2504 - copies the `u8` channels the key's mask selects out of one key record. */
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

/* 0x8009EA4C - advances the two divider counters one key step (header flag bit 6 enables it; `divH` moves
 * only at the last key or with bit 5, `divL` also when the step starts at division 0). */
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

/* 0x8009EF88 - binary-searches fn_800A12AC's u16 key table for both bracketing keys (`outLo`/`outHi`);
 * `*flag` says whether `frac` lands exactly on the found key. */
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

/* 0x8009CDBC - resolves the key tick, fractional tick and divisor of position `step` in a curve `mode`
 * steps long (`header` holds the tick count, flags and divisor count); a tick count <= 1 gives tick 0. */
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
 * The eight unwritten bodies, defined minimally with the signatures the range's call sites use
 * (fn_8009F85C's is `ef/ef_emitter.cpp`'s declaration).
 * ------------------------------------------------------------------------------------------------- */
void fn_8009D5C0(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv, u32* final) {
    /* Unwritten: five pointer guards (header/tick/tickF32/resDiv/final, lines 0x12A-0x12E). */
    (void)mode; (void)header; (void)step; (void)tickF32; (void)resDiv;
    *tick = 0;
    *final = 0;
}

void fn_8009DEB0(const void* mCmdList, void* target) {
    /* Unwritten: calls the unit's readers (the tick resolver, the kernels, the divider walk, the key
     * searches, the random generator, the table getters, fn_800A2504). */
    (void)mCmdList; (void)target;
}

void fn_8009F22C(const void* self, u32* outA, u32* outB, u32* outC, u32* outD, u32* outE) {
    /* Unwritten: six pointer guards (lines 0x70-0x75). */
    (void)self;
    *outA = 0; *outB = 0; *outC = 0; *outD = 0; *outE = 0;
}

void fn_8009F85C(void* rec, void* target, u32 life, u16 seed, s32 range) {
    /* Unwritten: the curve evaluator `ef/ef_emitter.cpp` calls (the tick resolver, the divider walk,
     * the random generator, the key searches, the f32 kernel, fn_800A2CF0). */
    (void)rec; (void)target; (void)life; (void)seed; (void)range;
}

void fn_800A02F8(const void* mCmdList, void* target) {
    /* Unwritten: calls the tick resolver, the divider walk, the random generator, the key searches,
     * the table getters and the f32 kernel. */
    (void)mCmdList; (void)target;
}

void fn_800A0D04(const void* mCmdList, void* target) {
    /* Unwritten: calls fn_8009D5C0, fn_8009EEF4, fn_8009F22C, fn_800A1290 and fn_800A12AC. */
    (void)mCmdList; (void)target;
}

void fn_800A16C4(const void* mCmdList, void* target) {
    /* Unwritten: calls fn_8009EEF4 and the creation queue's two `Add` entries. */
    (void)mCmdList; (void)target;
}

void fn_800A1CA8(const void* mCmdList, void* target) {
    /* Unwritten: carries the `div < nextDiv` assert (lbl_80592204) and calls fn_8009D5C0, fn_8009F22C
     * and fn_800A12AC. */
    (void)mCmdList; (void)target;
}

} /* extern "C" */
