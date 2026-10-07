/*
 * ef/ef_animcurve.cpp - the effect library's key-frame animation curves: the tick resolvers, the `u8` (u16.16
 *   fixed-point) and `f32` interpolation kernels (type 0 linear, 1 cubic with bits 2/3 picking 0.0f or 1.5f inner
 *   terms, 2 hold, else `NW4R_ASSERT(0)`), the divider walk, the key searches, the random generator (`seed *
 *   0x343FD + 0x269EC3`) and name hash, the two 0x100-byte channel tables' getters, the resource splitter, the
 *   u8 / f32 / signed-f32 / pattern / child-creation curve evaluators, and the creation queue's constructor pair.
 * RANGE. .text 0x8009CDBC-0x800A3044 (26 functions, defined in retail order); extab 0x80009A98-0x80009B40,
 *   extabindex 0x80022BC0-0x80022CBC (21 records: the five leaf bodies have none), .data 0x80591C68-0x805922C0
 *   (the channel tables, then three copies of the `__FILE__` string "ef_animcurve.cpp" at 0x80591E68, 0x80592260
 *   and 0x805922AC), .sdata2 0x80795FB8-0x80795FF0.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps `clrlwi`/`extrwi` + `cmpwi` unfused) and
 *   `#pragma fp_contract off` (retail keeps `fmuls` + `fadds`/`fsubs` apart).
 * NAMES. The map has only `fn_` stems for the range; the parameter names are the source's own, from the pointer
 *   messages (`header`, `tick`, `tickF32`, `resDiv`, `final`, `mCmdList`, `target`, `key`, `divL`, `divH`, `pHead`,
 *   `random`, `randomTable`, `nameTable`, `pp`, `ptrFixed`, `ptrBase`). GUESS: the record names (`EfAnimCurveKey`,
 *   `EfAnimChildKey`, `EfAnimRandomRange`, ...) and the particle owner chain's field names (`mManager`,
 *   `mManagerEM`, `mManagerEF`, `mManagerES`, after nw4r::ef's spelling) come from the offsets these bodies read.
 *   GUESS: `ef_anim_curve_u8` (0x8009DEB0), `ef_anim_curve_f32` (0x8009F85C), `ef_anim_curve_rotate` (0x800A02F8):
 *   the u8, f32 and signed-f32 curve evaluators.
 *   GUESS: `ef_anim_curve_texture` (0x800A0D04), `ef_anim_curve_child` (0x800A1CA8): the texture-pattern and
 *   child-creation curve evaluators.
 *   GUESS: `ef_anim_latch_tex_type` (0x800A14A4), `ef_anim_tex_ramp` (0x800A1504): the texture key type's latch
 *   and the pattern ramp.
 *   GUESS: `ef_anim_rand_next` (0x8009EEDC): the random generator's LCG step.
 *   GUESS: `ef_anim_name_hash` (0x8009EEF4): hashes the seed, the curve's id, the key and the division.
 * RESIDUALS. 11 partial rows, every one written:
 *  - `ef_anim_latch_tex_type`: retail stores the masked byte before or-ing the new bits in and reloads `arg->mChannel`;
 *    ours drops the first store (no alias between the two records);
 *  - `ef_anim_curve_rotate`, `ef_anim_curve_f32`, `ef_anim_curve_u8`: register colouring only (the mask, the two record strides and
 *    the key pointer take other callee-saved registers; retail reuses one pointer for the key data and the
 *    random range, flags read at -0xA);
 *  - `fn_8009E854`, `fn_800A01C4`: float register colours of the cubic terms;
 *  - `fn_8009EF88`: the exact-key bool is narrowed (`clrlwi`) before its two stores;
 *  - `fn_8009D5C0`, `fn_8009CDBC`, `ef_anim_curve_texture`, `ef_anim_tex_ramp`: register colours only;
 *  - `ef_anim_curve_rotate` keeps the key record pointers beside their data pointers, one callee-saved register more than
 *    retail: `_savegpr_20`/`_restgpr_20` where retail calls `_savegpr_21`/`_restgpr_21`.
 *   flipcheck: `.data` claimed, not emitted (the strings would have to come out of literal pools: the
 *   `fn_800A12AC`/`fn_8009EF88` messages each carry their own copy of the file name after the main pool);
 *   `.sdata2` 0x10 of 0x38 (only the two conversion doubles MWCC re-emits); `.text` 0x6280 of 0x6288.
 * SHAPES. `ef_anim_name_hash` sums `d*K + c*K + (a*K + b*K) + 0x4BF53` in that grouping (retail's add order).
 * SHAPES. The `.sdata2` constants are referenced by name, never spelled as literals (a literal re-pools them).
 *   The kernels take the curve type last (retail evaluates it after the three floats). The key searches read
 *   their u16 keys `stride` bytes apart. `fn_800A2FA4` walks the queue entries with a bottom-tested loop.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef/ef_torus.h" /* fn_800C9DCC (rule 2) */
#include "ef/ef_particlemanager.h"
#include "ef/ef_animcurve.h" /* the unit's own header */
#include "ef/ef_creationqueue.h" /* CreationQueue and its two `Add` members (rule 2) */

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
    /* +0x00 */ u8 pad_0x00;
    /* +0x01 */ u8 mTarget;            /* the channel the curve drives: 'h', 'l', 'p', ... */
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u8 mFlags;
    /* +0x05 */ u8 mDivCount;
    /* +0x06 */ u16 mNameId;           /* the curve's id, hashed with the seed for its random keys */
    /* +0x08 */ u16 mTickCount;
    /* +0x0A */ u8 pad_0x0A[0x2];
    /* +0x0C */ u32 mKeyBytes;         /* the key block's size: the random block follows it */
    /* +0x10 */ u32 mRandomBytes;      /* the random block's size: the random table follows it */
    /* +0x14 */ u32 mRandomTableBytes; /* the random table's size: the name table follows it */
    /* +0x18 */ u8 pad_0x18[0x8];
}; /* size: 0x20 (the key block starts right after it) */

/* A value curve's key block header: its leading u16 key count (the records follow at +0x4). */
struct EfAnimKeyRange {
    /* +0x00 */ u16 mKeyCount;
}; /* size: 0x2 (lower bound: +0x00 is the highest offset read) */


/* A resolved key: a 4-bit value, a 2-bit type and the index of its word in the curve's name table. */
struct EfAnimKey {
    /* +0x00 */ u8 mValue;
    /* +0x01 */ u8 mType;
    /* +0x02 */ u16 mTick;
}; /* size: 0x4 */

/* One record of a pattern curve's key block: its tick, its flags (non-zero: random; bit 1 picks the
 * random table over the random block) and either the key itself or the random entry's index. */
struct EfAnimKeyEntry {
    /* +0x00 */ u16 mTick;
    /* +0x02 */ u8 mFlags;
    /* +0x03 */ u8 pad_0x03[0x9];
    /* +0x0C */ union {
        EfAnimKey mKey;
        u16 mRandomIndex;
    } mValue;
}; /* size: 0x10 */

struct EfAnimKeyBlock {
    /* +0x00 */ u16 mCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ EfAnimKeyEntry mEntry[1];
}; /* size: 0x14 (a variable-length block: mCount records) */

/* One record of the random block: a base key and the mode that randomises its type bits. */
struct EfAnimRandomEntry {
    /* +0x00 */ EfAnimKey mKey;
    /* +0x04 */ u8 mMode;
    /* +0x05 */ u8 pad_0x05[0x3];
}; /* size: 0x8 */

struct EfAnimRandomBlock {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ EfAnimRandomEntry mEntry[1];
}; /* size: 0xC (a variable-length block) */

/* The random table: a key count and the keys a random draw picks from. */
struct EfAnimRandomTable {
    /* +0x00 */ u16 mCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ EfAnimKey mKey[1];
}; /* size: 0x8 (a variable-length block: mCount keys) */

/* The name table: the words a key's mTick indexes (texture names, for the pattern curves). */
struct EfAnimNameTable {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mName[1];
}; /* size: 0x8 (a variable-length block) */

/* One record of a value curve's key block: its tick, its flags (non-zero: random; bit 1 picks the random
 * table over the random block), one interpolation type per channel, then either one f32 per selected
 * channel or the random entry's index (the record is 0xC + 4 * channels bytes long). */
struct EfAnimCurveKey {
    /* +0x00 */ u16 mTick;
    /* +0x02 */ u8 mFlags;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u8 mType[8];
    /* +0x0C */ union {
        f32 mValue[1];
        u8 mByte[1];
        u16 mRandomIndex;
    } mData;
}; /* size: 0x10 (a variable-length record: 0xC + 4 * channels, or 0xC + channels rounded to 2 for a u8
    * curve) */

/* A random byte: `mBase + mRange * (s16)(hash >> 16) / 32768`, clamped to [0, 255] (the u8 curves' random
 * records hold one pair per channel). */
struct EfAnimRandomByte {
    /* +0x00 */ u8 mBase;
    /* +0x01 */ u8 mRange;
}; /* size: 0x2 */

/* A random value: `mBase + mRange * (hash >> 16)` (the random records hold one pair per channel). */
struct EfAnimRandomRange {
    /* +0x00 */ f32 mBase;
    /* +0x04 */ f32 mRange;
}; /* size: 0x8 */

/* The owner chain a particle reaches its effect system's creation queue through: particle manager ->
 * emitter -> effect -> effect system (the queue sits at +0x10; `ef/ef_effect.cpp` executes it). */
struct EfAnimEffectSystem {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ CreationQueue mCreationQueue;
}; /* size: 0xC014 (lower bound: the queue is the last member read) */

struct EfAnimEffect {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfAnimEffectSystem* mManagerES;
}; /* size: 0x24 (lower bound) */

struct EfAnimEmitter {
    /* +0x00 */ u8 pad_0x00[0xBC];
    /* +0xBC */ EfAnimEffect* mManagerEF;
}; /* size: 0xC0 (lower bound) */

struct EfAnimParticleManager {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfAnimEmitter* mManagerEM;
}; /* size: 0x24 (lower bound) */

/* The particle fields the curve commands touch: three channel words, the channels' key values as
 * nibbles and their key types as bit pairs, the owning particle manager, the remaining life a child
 * creation inherits, the one-shot latch and the ramp phase. */
struct EfAnimParticle {
    /* +0x00 */ u8 pad_0x00[0x88];
    /* +0x88 */ u32 mChannel[3];
    /* +0x94 */ u16 mKeyValueBits;
    /* +0x96 */ u8 mKeyTypeBits;
    /* +0x97 */ u8 pad_0x97[0xC8 - 0x97];
    /* +0xC8 */ EfAnimParticleManager* mManager;
    /* +0xCC */ u8 pad_0xCC[0xE2 - 0xCC];
    /* +0xE2 */ u16 mLife;
    /* +0xE4 */ u8 mLatched;
    /* +0xE5 */ u8 mPhase;
}; /* size: 0xE6 (lower bound: +0xE5 is the highest offset read) */

/* A child-creation key's payload: the setting a queued creation copies and the name-table index of the
 * resource it creates. */
struct EfAnimChildSetting {
    /* +0x00 */ Setting mSetting;
    /* +0x0A */ u16 mNameIndex;
}; /* size: 0xC */

/* One record of a child-creation curve's key block: its tick, its flags (non-zero: random; bit 1 picks
 * the random table) and either the payload or the random entry's index. */
struct EfAnimChildKey {
    /* +0x00 */ u16 mTick;
    /* +0x02 */ u8 mFlags;
    /* +0x03 */ u8 pad_0x03[0x9];
    /* +0x0C */ union {
        EfAnimChildSetting mSetting;
        u16 mRandomIndex;
    } mData;
}; /* size: 0x18 */

struct EfAnimChildKeyBlock {
    /* +0x00 */ u16 mCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ EfAnimChildKey mEntry[1];
}; /* size: 0x1C (a variable-length block: mCount records) */

/* The child-creation random table: a payload count and the payloads a random draw picks from. */
struct EfAnimChildTable {
    /* +0x00 */ u16 mCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ EfAnimChildSetting mEntry[1];
}; /* size: 0x10 (a variable-length block: mCount payloads) */

/* The name hash's word, folded byte by byte. */
union EfAnimHash {
    u32 mWord;
    u8 mByte[4];
}; /* size: 0x4 */

/* The ramp record ef_anim_tex_ramp reads: its two curve endpoints as bytes, plus the flags byte whose bit 2
 * picks the wrap rule. */
struct EfAnimRamp {
    /* +0x00 */ u8 mEnd;
    /* +0x01 */ u8 mStart;
    /* +0x02 */ u8 pad_0x02;
    /* +0x03 */ u8 mFlags;
}; /* size: 0x4 */

/* The key searches walk a table of records `stride` bytes apart whose leading u16 is the key. */
#define EF_ANIM_KEY_AT(base, stride, i) (*(const u16*)((const u8*)(base) + (i) * (stride)))

/* Fires one child-creation key of ef_anim_curve_child's curve. */
#define EF_ANIM_FIRE(entry)                                                                        \
    fn_800A16C4((entry), seed, header, (const EfAnimNameTable*)nameTable,                          \
                (const EfAnimChildTable*)randomTable, pp, divA)

extern "C" {

/* 0x80463F04 - the range helper the fractional key lookup measures its distance with. */
f32 fabsf(f32 x);

/* The unit's functions, in retail order (each is defined below in the same order). */
void fn_8009CDBC(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv);
void fn_8009D5C0(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv, u8* final);
void ef_anim_curve_u8(const u8* mCmdList, u8* target, u32 step, u16 seed, u32 mode);
u8 fn_8009E854(u32 tick, u8 start, u8 end, u8 type);
void fn_8009EA4C(const EfAnimHeader* header, const EfAnimKeyRange* key, u32 lo, u32* divL,
                 u32* divH);
u32 ef_anim_rand_next(u32 seed);
u32 ef_anim_name_hash(u16 a, u16 b, u16 c, u32 d);
void fn_8009EF88(u32* out, u8* flag, u16* outLo, u16* outHi, s32 target, f32 frac,
                 const u16* ptrBase, u32 stride, s32 lo, s32 hi);
void fn_8009F22C(const u8* pHead, const EfAnimHeader** header, const u8** key, const u8** random,
                 const u8** randomTable, const u8** nameTable);
u8 fn_8009F834(u32 index);
u8 fn_8009F848(u32 index);
void ef_anim_curve_f32(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode);
f32 fn_800A01C4(f32 t, f32 a, f32 b, u8 type);
void ef_anim_curve_rotate(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode);
void ef_anim_curve_texture(const u8* mCmdList, EfAnimParticle* pp, u32 step, u16 seed, u32 mode,
                 const u8** nameTableOut, u32** target, EfAnimDivider* divider);
void fn_800A1290(EfAnimKey* dst, const EfAnimKey* src);
void fn_800A12AC(s32* out, s32 target, const u16* ptrBase, u32 stride, s32 lo, s32 hi, int flag);
void ef_anim_latch_tex_type(EfAnimParticle* self, const EfAnimDivider* arg);
void ef_anim_tex_ramp(EfAnimParticle* self, const EfAnimDivider* divider, const EfAnimRamp* ramp,
                 struct EfPmManager* manager, const EfAnimNameTable* nameTable, u32* target);
void fn_800A16C4(const EfAnimChildKey* ptrFixed, u16 seed, const EfAnimHeader* header,
                 const EfAnimNameTable* nameTable, const EfAnimChildTable* randomTable, EfAnimParticle* pp,
                 u32 div);
void ef_anim_curve_child(const u8* mCmdList, EfAnimParticle* pp, u32 step, u16 seed, u32 mode);
void fn_800A2504(const u8* mCmdList, u8* target, u32 step, u32 mode);
void fn_800A27B4(const EfAnimHeader* header, u32 step, u16* tick, u32 mode);
void fn_800A2CF0(const u8* mCmdList, f32* target, u32 step, u32 mode);
CreationQueue* fn_800A2FA4(CreationQueue* queue);
CreationQueueEntry* fn_800A3008(CreationQueueEntry* slot);

/* 0x8009CDBC - resolves the key tick, fractional tick and divisor of position `step` in a curve `mode`
 * steps long (`header` holds the tick count, flags and divisor count); a tick count <= 1 gives tick 0. */
void fn_8009CDBC(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv) {
    NW4R_POINTER_ASSERT(header, 0x87, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(tick, 0x88, lbl_80591EB4, lbl_80591E68);
    NW4R_POINTER_ASSERT(tickF32, 0x89, lbl_80591EE8, lbl_80591E68);
    NW4R_POINTER_ASSERT(resDiv, 0x8A, lbl_80591F20, lbl_80591E68);

    *resDiv = 0;
    u16 count = header->mTickCount;
    if (count <= 1) {
        *tick = 0;
        *tickF32 = lbl_80795FB8;
        *resDiv = step;
        return;
    }

    u8 flags = header->mFlags;
    if ((flags & 0x20) == 0 && header->mDivCount <= 1) {
        if ((flags & 0x80) == 0) {
            if (step >= count - 1) {
                *tick = count - 1;
            } else {
                *tick = step;
            }
            *tickF32 = *tick;
            return;
        }
        if (mode == 1) {
            *tickF32 = count - 1;
        } else {
            f32 t = (f32)step * ((f32)(count - 1) / (f32)(mode - 1));
            *tickF32 = t;
            if (t > (f32)(header->mTickCount - 1)) {
                *tickF32 = header->mTickCount - 1;
            }
        }
        *tick = *tickF32;
        return;
    }

    if ((flags & 0x80) == 0) {
        u32 div = step / (count - 1);
        *resDiv = div;
        u8 flags2 = header->mFlags;
        if ((flags2 & 0x40) == 0) {
            if ((flags2 & 0x20) == 0 && div >= header->mDivCount) {
                *tick = header->mTickCount - 1;
                *resDiv = (u8)(header->mDivCount - 1);
            } else {
                *tick = step - div * (header->mTickCount - 1);
            }
        } else if ((flags2 & 0x20) == 0 && div >= header->mDivCount) {
            if (header->mDivCount % 2 == 0) {
                *tick = 0;
            } else {
                *tick = header->mTickCount - 1;
            }
            *resDiv = (u8)(header->mDivCount - 1);
        } else if ((div & 1) == 0) {
            *tick = step - div * (header->mTickCount - 1);
        } else {
            *tick = (header->mTickCount - 1) * (div + 1) - step;
        }
        *tickF32 = *tick;
        return;
    }

    u32 last = mode - 1;
    if (step >= last) {
        if ((flags & 0x40) == 0 || header->mDivCount % 2 != 0) {
            *tick = count - 1;
        } else {
            *tick = 0;
        }
        *tickF32 = *tick;
        *resDiv = (u8)(header->mDivCount - 1);
        return;
    }

    s32 span = count - 1;
    f32 rate = (f32)header->mDivCount * ((f32)span / (f32)last);
    u32 div = (u32)((f32)step * rate / (f32)span);
    *resDiv = div;
    s32 span2 = header->mTickCount - 1;
    f32 frac = (f32)step * rate - (f32)(div * span2);
    if ((header->mFlags & 0x40) == 0) {
        *tickF32 = frac;
    } else if ((div & 1) == 0) {
        *tickF32 = frac;
    } else {
        *tickF32 = (f32)span2 - frac;
    }
    *tick = *tickF32;
}

/* 0x8009D5C0 (0x8F0): the key-index twin of fn_8009CDBC: resolves the key slot, its float copy and the
 * division of position `step` in a curve `mode` steps long; `*final` is set when a ping-pong curve ends on
 * its first key. */
void fn_8009D5C0(u32 mode, const EfAnimHeader* header, u32 step, u16* tick, f32* tickF32,
                 u32* resDiv, u8* final) {
    NW4R_POINTER_ASSERT(header, 0x12A, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(tick, 0x12B, lbl_80591EB4, lbl_80591E68);
    NW4R_POINTER_ASSERT(tickF32, 0x12C, lbl_80591EE8, lbl_80591E68);
    NW4R_POINTER_ASSERT(resDiv, 0x12D, lbl_80591F20, lbl_80591E68);
    NW4R_POINTER_ASSERT(final, 0x12E, lbl_80591F58, lbl_80591E68);

    *resDiv = 0;
    *final = 0;

    u8 flags = header->mFlags;
    u32 bit5 = flags & 0x20;

    if (bit5 == 0 && header->mDivCount <= 1) {
        if ((flags & 0x80) == 0) {
            if (step >= header->mTickCount) {
                *tick = header->mTickCount;
            } else {
                *tick = (u16)step;
            }
        } else {
            *tick = (u16)(header->mTickCount * step / mode);
        }
    } else if ((flags & 0x80) == 0) {
        u16 count = header->mTickCount;
        if (count <= 1 || (flags & 0x40) == 0) {
            u32 div = step / count;
            if (bit5 == 0 && div >= header->mDivCount) {
                *tick = count;
                *resDiv = (u8)(header->mDivCount - 1);
            } else {
                *tick = (u16)(step - div * count);
                *resDiv = div;
            }
        } else {
            u32 div = step / (count - 1);
            *resDiv = div;
            if ((header->mFlags & 0x20) == 0 && div >= header->mDivCount) {
                if (header->mDivCount % 2 == 0) {
                    if (div == header->mDivCount && step - div * (header->mTickCount - 1) == 0) {
                        *final = 1;
                    }
                    *tick = 0;
                } else {
                    *tick = (u16)(header->mTickCount - 1);
                }
                *resDiv = (u8)(header->mDivCount - 1);
            } else if ((div & 1) == 0) {
                *tick = (u16)(step - div * (header->mTickCount - 1));
            } else {
                *tick = (u16)((header->mTickCount - 1) * (div + 1) - step);
            }
        }
    } else if (step >= mode) {
        if ((flags & 0x40) == 0 || header->mDivCount % 2 != 0) {
            *tick = header->mTickCount;
        } else {
            *tick = 0;
        }
        *resDiv = (u8)(header->mDivCount - 1);
    } else {
        u16 count = header->mTickCount;
        if (count <= 1 || (flags & 0x40) == 0) {
            f32 rate = (f32)header->mDivCount * ((f32)count / (f32)mode);
            u32 div = (u32)((f32)step * rate / (f32)count);
            *resDiv = div;
            *tick = (f32)step * rate - (f32)(div * header->mTickCount);
        } else {
            s32 span = count - 1;
            f32 rate = (lbl_80795FD0 + (f32)span * (f32)header->mDivCount) / (f32)mode;
            u32 pos = (u32)((f32)step * rate);
            u32 div = pos / span;
            *resDiv = div;
            if (div >= header->mDivCount) {
                if (header->mDivCount % 2 == 0) {
                    if (step == mode - 1) {
                        *final = 1;
                    }
                    *tick = 0;
                } else {
                    *tick = (u16)(header->mTickCount - 1);
                }
                *resDiv = (u8)(header->mDivCount - 1);
            } else if ((div & 1) == 0) {
                *tick = (u16)(pos - div * (header->mTickCount - 1));
            } else {
                *tick = (u16)((header->mTickCount - 1) * (div + 1) - pos);
            }
        }
    }
    *tickF32 = (f32)*tick;
}

/* 0x8009DEB0 (0x9A4): the u8 twin of ef_anim_curve_f32: a baked curve (0xAB) is a table copy, otherwise an exact
 * key is copied (or drawn from its random byte range) and a key span is interpolated in u16.16 fixed point
 * with each channel's kernel type. */
void ef_anim_curve_u8(const u8* mCmdList, u8* target, u32 step, u16 seed, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x31C, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x31D, lbl_80591FC4, lbl_80591E68);

    if (mCmdList[0] == 0xAB) {
        fn_800A2504(mCmdList, target, step, mode);
        return;
    }

    u8 mask = mCmdList[3];
    const EfAnimHeader* header;
    const u8* key;
    const u8* random;
    const u8* randomTable;
    const u8* nameTable;
    f32 tickF32;
    u32 divL;
    u32 divH;
    u32 idx;
    u16 tick;
    u16 keyLo;
    u16 keyHi;
    u8 exact;

    u8 end = fn_8009F848(mask);
    u32 count = fn_8009F834(mask);
    if (count == 0) {
        return;
    }

    u32 stride = (count + 0xD) & ~1;
    fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
    u32 rstride = count * 2;
    fn_8009CDBC(mode, header, step, &tick, &tickF32, &divL);
    divH = divL;

    const u8* keys = key + 4;
    fn_8009EF88(&idx, &exact, &keyLo, &keyHi, tick, tickF32, (const u16*)keys, stride, 0,
                ((const EfAnimKeyRange*)key)->mKeyCount - 1);

    if (exact) {
        const EfAnimCurveKey* rec = (const EfAnimCurveKey*)(keys + idx * stride);
        const u8* src = rec->mData.mByte;
        if (rec->mFlags == 0) {
            for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
                if ((mask & bit) != 0) {
                    *target = *src++;
                }
                target++;
            }
            return;
        }

        u16 r = *(const u16*)src;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, divL);
        const EfAnimRandomByte* range;
        if ((rec->mFlags & 2) == 0) {
            range = (const EfAnimRandomByte*)(random + r * rstride + 4);
        } else {
            range = (const EfAnimRandomByte*)(randomTable +
                                              rstride * ((hash >> 16) % *(const u16*)randomTable) + 4);
            hash = ef_anim_rand_next(hash);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                u8 base = range->mBase;
                u8 scale = range->mRange;
                range++;
                s32 v = base + scale * (s16)(hash >> 16) / 32768;
                if (v < 0) {
                    v = 0;
                }
                if (v > 0xFF) {
                    v = 0xFF;
                }
                *target = v;
            }
            if ((header->mFlags & 4) == 0) {
                hash = ef_anim_rand_next(hash);
            }
            target++;
        }
        return;
    }

    fn_8009EA4C(header, (const EfAnimKeyRange*)key, idx, &divL, &divH);
    u32 t = (u32)(lbl_80795FD4 * (tickF32 - (f32)keyLo)) / (keyHi - keyLo);
    const EfAnimCurveKey* recA = (const EfAnimCurveKey*)(keys + idx * stride);
    const u8* srcA = recA->mData.mByte;
    const EfAnimCurveKey* recB = (const EfAnimCurveKey*)(keys + (idx + 1) * stride);
    const u8* srcB = recB->mData.mByte;
    const u8* type = recA->mType;
    BOOL randA = recA->mFlags != 0;
    BOOL randB = recB->mFlags != 0;

    if (!randA && !randB) {
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = fn_8009E854(t, *srcA, *srcB, *type);
                srcA++;
                srcB++;
            }
            type++;
            target++;
        }
        return;
    }

    if (randA && !randB) {
        u16 rA = *(const u16*)srcA;
        u32 hashA = ef_anim_name_hash(seed, header->mNameId, rA, divL);
        const EfAnimRandomByte* rangeA;
        if ((recA->mFlags & 2) == 0) {
            rangeA = (const EfAnimRandomByte*)(random + rA * rstride + 4);
        } else {
            rangeA = (const EfAnimRandomByte*)(randomTable +
                                              rstride * ((hashA >> 16) % *(const u16*)randomTable) + 4);
            hashA = ef_anim_rand_next(hashA);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                u8 baseA = rangeA->mBase;
                u8 scaleA = rangeA->mRange;
                rangeA++;
                s32 vA = baseA + scaleA * (s16)(hashA >> 16) / 32768;
                if (vA < 0) {
                    vA = 0;
                }
                if (vA > 0xFF) {
                    vA = 0xFF;
                }
                *target = fn_8009E854(t, vA, *srcB, *type);
                srcB++;
            }
            if ((header->mFlags & 4) == 0) {
                hashA = ef_anim_rand_next(hashA);
            }
            type++;
            target++;
        }
        return;
    }

    if (!randA && randB) {
        u16 rB = *(const u16*)srcB;
        u32 hashB = ef_anim_name_hash(seed, header->mNameId, rB, divH);
        const EfAnimRandomByte* rangeB;
        if ((recB->mFlags & 2) == 0) {
            rangeB = (const EfAnimRandomByte*)(random + rB * rstride + 4);
        } else {
            rangeB = (const EfAnimRandomByte*)(randomTable +
                                              rstride * ((hashB >> 16) % *(const u16*)randomTable) + 4);
            hashB = ef_anim_rand_next(hashB);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                u8 baseB = rangeB->mBase;
                u8 scaleB = rangeB->mRange;
                rangeB++;
                s32 vB = baseB + scaleB * (s16)(hashB >> 16) / 32768;
                if (vB < 0) {
                    vB = 0;
                }
                if (vB > 0xFF) {
                    vB = 0xFF;
                }
                *target = fn_8009E854(t, *srcA, vB, *type);
                srcA++;
            }
            if ((header->mFlags & 4) == 0) {
                hashB = ef_anim_rand_next(hashB);
            }
            type++;
            target++;
        }
        return;
    }

    u16 rA = *(const u16*)srcA;
    u32 hashA = ef_anim_name_hash(seed, header->mNameId, rA, divL);
    const EfAnimRandomByte* rangeA;
    if ((recA->mFlags & 2) == 0) {
        rangeA = (const EfAnimRandomByte*)(random + rA * rstride + 4);
    } else {
        rangeA = (const EfAnimRandomByte*)(randomTable +
                                          rstride * ((hashA >> 16) % *(const u16*)randomTable) + 4);
        hashA = ef_anim_rand_next(hashA);
    }
    u16 rB = *(const u16*)srcB;
    u32 hashB = ef_anim_name_hash(seed, header->mNameId, rB, divH);
    const EfAnimRandomByte* rangeB;
    if ((recB->mFlags & 2) == 0) {
        rangeB = (const EfAnimRandomByte*)(random + rB * rstride + 4);
    } else {
        rangeB = (const EfAnimRandomByte*)(randomTable +
                                          rstride * ((hashB >> 16) % *(const u16*)randomTable) + 4);
        hashB = ef_anim_rand_next(hashB);
    }
    for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
        if ((mask & bit) != 0) {
            u8 baseA = rangeA->mBase;
            u8 scaleA = rangeA->mRange;
            rangeA++;
            s32 vA = baseA + scaleA * (s16)(hashA >> 16) / 32768;
            if (vA < 0) {
                vA = 0;
            }
            if (vA > 0xFF) {
                vA = 0xFF;
            }
            if ((header->mFlags & 4) == 0) {
                hashA = ef_anim_rand_next(hashA);
            }
            u8 baseB = rangeB->mBase;
            u8 scaleB = rangeB->mRange;
            rangeB++;
            s32 vB = baseB + scaleB * (s16)(hashB >> 16) / 32768;
            if (vB < 0) {
                vB = 0;
            }
            if (vB > 0xFF) {
                vB = 0xFF;
            }
            if ((header->mFlags & 4) == 0) {
                hashB = ef_anim_rand_next(hashB);
            }
            *target = fn_8009E854(t, vA, vB, *type);
        } else if ((header->mFlags & 4) == 0) {
            hashA = ef_anim_rand_next(hashA);
            hashB = ef_anim_rand_next(hashB);
        }
        type++;
        target++;
    }
}

/* 0x8009E854 - the u16.16 fixed-point `u8` interpolation kernel: `tick` is t * 65536, the result is
 * clamped to [0, 255]. */
u8 fn_8009E854(u32 tick, u8 start, u8 end, u8 type) {
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
            return s0 + t * (t * (w * (f32)(int)(e - s)));
        } else {
            f32 c1 = ((type >> 2) & 1) == 0 ? lbl_80795FB8 : lbl_80795FE0;
            f32 c0 = ((type >> 3) & 1) == 0 ? lbl_80795FB8 : lbl_80795FE0;
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
            return r;
        }
    }
    case 2:
        return start;
    default:
        NW4R_ASSERT(0, 0x2C7, lbl_80592000, lbl_80591E68);
        return 0;
    }
}

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
    if (bit5 == 0 && header->mDivCount <= 1) {
        return;
    }
    u32 v = *divL;
    if ((v & 1) == 0 && (s32)(lo + 1) >= (s32)(key->mKeyCount - 1)) {
        if (bit5 != 0 || v < (u32)(header->mDivCount - 1)) {
            (*divH)++;
        }
    }

    u32 w = *divL;
    if ((w & 1) != 0 && lo == 0 && w != 0) {
        (*divL)++;
    }
}

/* 0x8009EEDC - one step of the curve random generator's LCG. */
u32 ef_anim_rand_next(u32 seed) {
    return seed * 0x343FD + 0x269EC3;
}

/* 0x8009EEF4 - the four-word name hash the sequence name table is keyed by. */
u32 ef_anim_name_hash(u16 a, u16 b, u16 c, u32 d) {
    EfAnimHash h;
    h.mWord = (u32)d * 0x7B929 + (u32)c * 0x371097E7 + ((u32)a * 0x3F81F635 + (u32)b * 0x30A74193) + 0x4BF53;
    h.mByte[2] ^= h.mByte[3];
    h.mByte[1] ^= h.mByte[2];
    h.mByte[0] ^= h.mByte[1];
    return h.mWord;
}

/* 0x8009EF88 - binary-searches fn_800A12AC's u16 key table for both bracketing keys (`outLo`/`outHi`);
 * `*flag` says whether `frac` lands exactly on the found key. */
void fn_8009EF88(u32* out, u8* flag, u16* outLo, u16* outHi, s32 target, f32 frac,
                 const u16* ptrBase, u32 stride, s32 lo, s32 hi) {
    NW4R_POINTER_ASSERT(ptrBase, 0x1F1, lbl_80592274, lbl_805922AC);

    s32 mid = (lo + hi) / 2;
    u8 atTarget = fabsf((f32)target - frac) < lbl_80795FEC;

    u16 keyLo = EF_ANIM_KEY_AT(ptrBase, stride, lo);
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
        *flag = atTarget;
        if (atTarget == 0) {
            *outHi = EF_ANIM_KEY_AT(ptrBase, stride, lo + 1);
        }
        return;
    }

    u16 keyHi = EF_ANIM_KEY_AT(ptrBase, stride, hi);
    *outHi = keyHi;
    if (keyHi <= target) {
        *out = hi;
        *flag = 1;
        return;
    }

    s32 key = EF_ANIM_KEY_AT(ptrBase, stride, mid);
    while (lo < mid) {
        if (target == key) {
            *out = mid;
            *flag = atTarget;
            if (atTarget != 0) {
                return;
            }
            *outLo = key;
            *outHi = EF_ANIM_KEY_AT(ptrBase, stride, *out + 1);
            return;
        } else if (key < target) {
            lo = mid;
            *outLo = key;
        } else {
            hi = mid;
            *outHi = key;
        }
        mid = (lo + hi) / 2;
        key = EF_ANIM_KEY_AT(ptrBase, stride, mid);
    }
    *out = lo;
    *flag = 0;
}

/* 0x8009F22C (0x608): splits a curve resource into its header, key block, random block, random table and
 * name table; each block starts where the previous one's size (from the header) ends. */
void fn_8009F22C(const u8* pHead, const EfAnimHeader** header, const u8** key, const u8** random,
                 const u8** randomTable, const u8** nameTable) {
    NW4R_POINTER_ASSERT(pHead, 0x70, lbl_805920B4, lbl_80591E68);
    NW4R_POINTER_ASSERT(header, 0x71, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(key, 0x72, lbl_80592018, lbl_80591E68);
    NW4R_POINTER_ASSERT(random, 0x73, lbl_805920E8, lbl_80591E68);
    NW4R_POINTER_ASSERT(randomTable, 0x74, lbl_80592120, lbl_80591E68);
    NW4R_POINTER_ASSERT(nameTable, 0x75, lbl_80592160, lbl_80591E68);

    *header = (const EfAnimHeader*)pHead;
    *key = pHead + sizeof(EfAnimHeader);
    *random = *key + (*header)->mKeyBytes;
    *randomTable = *random + (*header)->mRandomBytes;
    *nameTable = *randomTable + (*header)->mRandomTableBytes;
}

/* 0x8009F834 - the low 0x100-byte table lookup. */
u8 fn_8009F834(u32 index) {
    return lbl_80591C68[index & 0xFF];
}

/* 0x8009F848 - the high 0x100-byte table lookup. */
u8 fn_8009F848(u32 index) {
    return lbl_80591D68[index & 0xFF];
}

/* 0x8009F85C (0x968): evaluates an f32 curve at `step` into the channels its mask selects: a baked curve
 * (0xAB) is a table copy, otherwise an exact key is copied (or drawn from its random range) and a key
 * span is interpolated with each channel's kernel type, either end possibly random. */
void ef_anim_curve_f32(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x418, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x419, lbl_80591FC4, lbl_80591E68);

    if (mCmdList[0] == 0xAB) {
        fn_800A2CF0(mCmdList, target, step, mode);
        return;
    }

    u8 mask = mCmdList[3];
    const EfAnimHeader* header;
    const u8* key;
    const u8* random;
    const u8* randomTable;
    const u8* nameTable;
    f32 tickF32;
    u32 divL;
    u32 divH;
    u32 idx;
    u16 tick;
    u16 keyLo;
    u16 keyHi;
    u8 exact;

    fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
    fn_8009CDBC(mode, header, step, &tick, &tickF32, &divL);
    divH = divL;

    u8 end = fn_8009F848(mask);
    u8 count = fn_8009F834(mask);
    if (count == 0) {
        return;
    }

    u32 stride = count * 4 + 0xC;
    u32 rstride = count * 8;
    const u8* keys = key + 4;
    fn_8009EF88(&idx, &exact, &keyLo, &keyHi, tick, tickF32, (const u16*)keys, stride, 0,
                ((const EfAnimKeyRange*)key)->mKeyCount - 1);

    if (exact) {
        const EfAnimCurveKey* rec = (const EfAnimCurveKey*)(keys + idx * stride);
        const f32* src = rec->mData.mValue;
        if (rec->mFlags == 0) {
            for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
                if ((mask & bit) != 0) {
                    *target = *src++;
                }
                target++;
            }
            return;
        }

        u16 r = *(const u16*)src;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, divL);
        const EfAnimRandomRange* range;
        if ((rec->mFlags & 2) == 0) {
            range = (const EfAnimRandomRange*)(random + r * rstride + 4);
        } else {
            range = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hash >> 16) % *(const u16*)randomTable) + 4);
            hash = ef_anim_rand_next(hash);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 base = range->mBase;
                f32 scale = range->mRange;
                range++;
                *target = base + scale * (f32)(hash >> 16);
                if ((header->mFlags & 4) == 0) {
                    hash = ef_anim_rand_next(hash);
                }
            }
            target++;
        }
        return;
    }

    fn_8009EA4C(header, (const EfAnimKeyRange*)key, idx, &divL, &divH);
    f32 t = (tickF32 - (f32)keyLo) / (f32)(keyHi - keyLo);
    const EfAnimCurveKey* recA = (const EfAnimCurveKey*)(keys + idx * stride);
    const f32* srcA = recA->mData.mValue;
    const EfAnimCurveKey* recB = (const EfAnimCurveKey*)(keys + (idx + 1) * stride);
    const f32* srcB = recB->mData.mValue;
    const u8* type = recA->mType;
    BOOL randA = recA->mFlags != 0;
    BOOL randB = recB->mFlags != 0;

    if (!randA && !randB) {
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = fn_800A01C4(t, *srcA, *srcB, *type);
                srcA++;
                srcB++;
            }
            type++;
            target++;
        }
        return;
    }

    if (randA && !randB) {
        u16 r = *(const u16*)srcA;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, divL);
        const EfAnimRandomRange* range;
        if ((recA->mFlags & 2) == 0) {
            range = (const EfAnimRandomRange*)(random + r * rstride + 4);
        } else {
            range = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hash >> 16) % *(const u16*)randomTable) + 4);
            hash = ef_anim_rand_next(hash);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 base = range->mBase;
                f32 scale = range->mRange;
                range++;
                f32 a = base + scale * (f32)(hash >> 16);
                if ((header->mFlags & 4) == 0) {
                    hash = ef_anim_rand_next(hash);
                }
                *target = fn_800A01C4(t, a, *srcB, *type);
                srcB++;
            }
            type++;
            target++;
        }
        return;
    }

    if (!randA && randB) {
        u16 r = *(const u16*)srcB;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, divH);
        const EfAnimRandomRange* range;
        if ((recB->mFlags & 2) == 0) {
            range = (const EfAnimRandomRange*)(random + r * rstride + 4);
        } else {
            range = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hash >> 16) % *(const u16*)randomTable) + 4);
            hash = ef_anim_rand_next(hash);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 base = range->mBase;
                f32 scale = range->mRange;
                range++;
                f32 b = base + scale * (f32)(hash >> 16);
                if ((header->mFlags & 4) == 0) {
                    hash = ef_anim_rand_next(hash);
                }
                *target = fn_800A01C4(t, *srcA, b, *type);
                srcA++;
            }
            type++;
            target++;
        }
        return;
    }

    u16 rA = *(const u16*)srcA;
    u32 hashA = ef_anim_name_hash(seed, header->mNameId, rA, divL);
    const EfAnimRandomRange* rangeA;
    if ((recA->mFlags & 2) == 0) {
        rangeA = (const EfAnimRandomRange*)(random + rA * rstride + 4);
    } else {
        rangeA = (const EfAnimRandomRange*)(randomTable +
                                            rstride * ((hashA >> 16) % *(const u16*)randomTable) + 4);
        hashA = ef_anim_rand_next(hashA);
    }
    u16 rB = *(const u16*)srcB;
    u32 hashB = ef_anim_name_hash(seed, header->mNameId, rB, divH);
    const EfAnimRandomRange* rangeB;
    if ((recB->mFlags & 2) == 0) {
        rangeB = (const EfAnimRandomRange*)(random + rB * rstride + 4);
    } else {
        rangeB = (const EfAnimRandomRange*)(randomTable +
                                            rstride * ((hashB >> 16) % *(const u16*)randomTable) + 4);
        hashB = ef_anim_rand_next(hashB);
    }
    for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
        if ((mask & bit) != 0) {
            f32 baseA = rangeA->mBase;
            f32 scaleA = rangeA->mRange;
            rangeA++;
            f32 a = baseA + scaleA * (f32)(hashA >> 16);
            if ((header->mFlags & 4) == 0) {
                hashA = ef_anim_rand_next(hashA);
            }
            f32 baseB = rangeB->mBase;
            f32 scaleB = rangeB->mRange;
            rangeB++;
            f32 b = baseB + scaleB * (f32)(hashB >> 16);
            if ((header->mFlags & 4) == 0) {
                hashB = ef_anim_rand_next(hashB);
            }
            *target = fn_800A01C4(t, a, b, *type);
        }
        type++;
        target++;
    }
}

/* 0x800A01C4 - the f32 interpolation kernel: curve type `type` (bits 0-1) between `a` and `b` at
 * `t` in [0, 1].  Type 1's two inner control terms are switched by bits 2 and 3. */
f32 fn_800A01C4(f32 t, f32 a, f32 b, u8 type) {
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
            f32 c0 = ((type >> 3) & 1) == 0 ? lbl_80795FB8 : lbl_80795FE0;
            f32 c1 = ((type >> 2) & 1) == 0 ? lbl_80795FB8 : lbl_80795FE0;
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

/* 0x800A02F8 (0xA0C): the signed twin of ef_anim_curve_f32: a random record carries a sign byte after its
 * pairs, and a set byte negates the drawn values on a coin flip of the hash. */
void ef_anim_curve_rotate(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x51A, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x51B, lbl_80591FC4, lbl_80591E68);

    u8 mask = mCmdList[3];
    const EfAnimHeader* header;
    const u8* key;
    const u8* random;
    const u8* randomTable;
    const u8* nameTable;
    f32 tickF32;
    u32 divL;
    u32 divH;
    u32 idx;
    u16 tick;
    u16 keyLo;
    u16 keyHi;
    u8 exact;

    fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
    fn_8009CDBC(mode, header, step, &tick, &tickF32, &divL);
    divH = divL;

    u8 end = fn_8009F848(mask);
    u8 count = fn_8009F834(mask);
    if (count == 0) {
        return;
    }

    u32 stride = count * 4 + 0xC;
    u32 pairBytes = count * 8;
    u32 rstride = pairBytes + 4;
    const u8* keys = key + 4;
    fn_8009EF88(&idx, &exact, &keyLo, &keyHi, tick, tickF32, (const u16*)keys, stride, 0,
                ((const EfAnimKeyRange*)key)->mKeyCount - 1);

    if (exact) {
        const EfAnimCurveKey* rec = (const EfAnimCurveKey*)(keys + idx * stride);
        const f32* src = rec->mData.mValue;
        if (rec->mFlags == 0) {
            for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
                if ((mask & bit) != 0) {
                    *target = *src++;
                }
                target++;
            }
            return;
        }

        u16 r = *(const u16*)src;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, divL);
        const EfAnimRandomRange* range;
        if ((rec->mFlags & 2) == 0) {
            range = (const EfAnimRandomRange*)(random + r * rstride + 4);
        } else {
            range = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hash >> 16) % *(const u16*)randomTable) + 4);
            hash = ef_anim_rand_next(hash);
        }
        BOOL negate = FALSE;
        if (*((const u8*)range + pairBytes) != 0) {
            if ((hash & 0x10000) == 0) {
                negate = TRUE;
            }
            hash = ef_anim_rand_next(hash);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 base = range->mBase;
                f32 scale = range->mRange;
                range++;
                f32 v = base + scale * (f32)(hash >> 16);
                if (negate) {
                    v = -v;
                }
                *target = v;
            }
            target++;
            if ((header->mFlags & 4) == 0) {
                hash = ef_anim_rand_next(hash);
            }
        }
        return;
    }

    fn_8009EA4C(header, (const EfAnimKeyRange*)key, idx, &divL, &divH);
    f32 t = (tickF32 - (f32)keyLo) / (f32)(keyHi - keyLo);
    const EfAnimCurveKey* recA = (const EfAnimCurveKey*)(keys + idx * stride);
    const f32* srcA = recA->mData.mValue;
    const EfAnimCurveKey* recB = (const EfAnimCurveKey*)(keys + (idx + 1) * stride);
    const f32* srcB = recB->mData.mValue;
    const u8* type = recA->mType;
    BOOL randA = recA->mFlags != 0;
    BOOL randB = recB->mFlags != 0;

    if (!randA && !randB) {
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = fn_800A01C4(t, *srcA, *srcB, *type);
                srcA++;
                srcB++;
            }
            type++;
            target++;
        }
        return;
    }

    if (randA && !randB) {
        u16 rA = *(const u16*)srcA;
        u32 hashA = ef_anim_name_hash(seed, header->mNameId, rA, divL);
        const EfAnimRandomRange* rangeA;
        if ((recA->mFlags & 2) == 0) {
            rangeA = (const EfAnimRandomRange*)(random + rA * rstride + 4);
        } else {
            rangeA = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hashA >> 16) % *(const u16*)randomTable) + 4);
            hashA = ef_anim_rand_next(hashA);
        }
        BOOL negateA = FALSE;
        if (*((const u8*)rangeA + pairBytes) != 0) {
            if ((hashA & 0x10000) == 0) {
                negateA = TRUE;
            }
            hashA = ef_anim_rand_next(hashA);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 baseA = rangeA->mBase;
                f32 scaleA = rangeA->mRange;
                rangeA++;
                f32 vA = baseA + scaleA * (f32)(hashA >> 16);
                if (negateA) {
                    vA = -vA;
                }
                *target = fn_800A01C4(t, vA, *srcB, *type);
                srcB++;
            }
            type++;
            target++;
            if ((header->mFlags & 4) == 0) {
                hashA = ef_anim_rand_next(hashA);
            }
        }
        return;
    }

    if (!randA && randB) {
        u16 rB = *(const u16*)srcB;
        u32 hashB = ef_anim_name_hash(seed, header->mNameId, rB, divH);
        const EfAnimRandomRange* rangeB;
        if ((recB->mFlags & 2) == 0) {
            rangeB = (const EfAnimRandomRange*)(random + rB * rstride + 4);
        } else {
            rangeB = (const EfAnimRandomRange*)(randomTable +
                                               rstride * ((hashB >> 16) % *(const u16*)randomTable) + 4);
            hashB = ef_anim_rand_next(hashB);
        }
        BOOL negateB = FALSE;
        if (*((const u8*)rangeB + pairBytes) != 0) {
            if ((hashB & 0x10000) == 0) {
                negateB = TRUE;
            }
            hashB = ef_anim_rand_next(hashB);
        }
        for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                f32 baseB = rangeB->mBase;
                f32 scaleB = rangeB->mRange;
                rangeB++;
                f32 vB = baseB + scaleB * (f32)(hashB >> 16);
                if (negateB) {
                    vB = -vB;
                }
                *target = fn_800A01C4(t, *srcA, vB, *type);
                srcA++;
            }
            type++;
            target++;
            if ((header->mFlags & 4) == 0) {
                hashB = ef_anim_rand_next(hashB);
            }
        }
        return;
    }

    u16 rA = *(const u16*)srcA;
    u32 hashA = ef_anim_name_hash(seed, header->mNameId, rA, divL);
    const EfAnimRandomRange* rangeA;
    if ((recA->mFlags & 2) == 0) {
        rangeA = (const EfAnimRandomRange*)(random + rA * rstride + 4);
    } else {
        rangeA = (const EfAnimRandomRange*)(randomTable +
                                           rstride * ((hashA >> 16) % *(const u16*)randomTable) + 4);
        hashA = ef_anim_rand_next(hashA);
    }
    u16 rB = *(const u16*)srcB;
    u32 hashB = ef_anim_name_hash(seed, header->mNameId, rB, divH);
    const EfAnimRandomRange* rangeB;
    if ((recB->mFlags & 2) == 0) {
        rangeB = (const EfAnimRandomRange*)(random + rB * rstride + 4);
    } else {
        rangeB = (const EfAnimRandomRange*)(randomTable +
                                           rstride * ((hashB >> 16) % *(const u16*)randomTable) + 4);
        hashB = ef_anim_rand_next(hashB);
    }
    BOOL negateA = FALSE;
    if (*((const u8*)rangeA + pairBytes) != 0) {
        if ((hashA & 0x10000) == 0) {
            negateA = TRUE;
        }
        hashA = ef_anim_rand_next(hashA);
    }
    BOOL negateB = FALSE;
    if (*((const u8*)rangeB + pairBytes) != 0) {
        if ((hashB & 0x10000) == 0) {
            negateB = TRUE;
        }
        hashB = ef_anim_rand_next(hashB);
    }
    for (u16 bit = 1; bit <= end; bit = (u16)(bit << 1)) {
        if ((mask & bit) != 0) {
            f32 baseA = rangeA->mBase;
            f32 scaleA = rangeA->mRange;
            rangeA++;
            f32 vA = baseA + scaleA * (f32)(hashA >> 16);
            if (negateA) {
                vA = -vA;
            }
            f32 baseB = rangeB->mBase;
            f32 scaleB = rangeB->mRange;
            rangeB++;
            f32 vB = baseB + scaleB * (f32)(hashB >> 16);
            if (negateB) {
                vB = -vB;
            }
            *target = fn_800A01C4(t, vA, vB, *type);
        }
        type++;
        target++;
        if ((header->mFlags & 4) == 0) {
            hashA = ef_anim_rand_next(hashA);
            hashB = ef_anim_rand_next(hashB);
        }
    }
}

/* 0x800A0D04 (0x58C): evaluates a pattern curve ('h'/'l'/'p' channel) at `step`: finds the key, resolves a
 * random key through the seed hash, writes the key's name word to the particle's channel and records the
 * key's value and type bits. */
void ef_anim_curve_texture(const u8* mCmdList, EfAnimParticle* pp, u32 step, u16 seed, u32 mode,
                 const u8** nameTableOut, u32** target, EfAnimDivider* divider) {
    NW4R_POINTER_ASSERT(mCmdList, 0x669, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(pp, 0x66A, lbl_80592198, lbl_80591E68);

    const EfAnimHeader* header;
    const u8* key;
    const u8* random;
    const u8* randomTable;
    const u8* nameTable;
    f32 tickF32;
    u32 resDiv;
    s32 idx;
    EfAnimKey k;
    u16 tick;
    u8 final;
    u32 ch;
    u32* channel;

    fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
    fn_8009D5C0(mode, header, step, &tick, &tickF32, &resDiv, &final);

    switch (header->mTarget) {
    case 'h':
        channel = &pp->mChannel[0];
        ch = 0;
        break;
    case 'l':
        channel = &pp->mChannel[1];
        ch = 1;
        break;
    case 'p':
        channel = &pp->mChannel[2];
        ch = 2;
        break;
    default:
        return;
    }

    const EfAnimKeyEntry* entries = ((const EfAnimKeyBlock*)key)->mEntry;
    fn_800A12AC(&idx, tick, &entries->mTick, sizeof(EfAnimKeyEntry), 0,
                ((const EfAnimKeyBlock*)key)->mCount - 1, 0);
    divider->mKeyCount = ((const EfAnimKeyBlock*)key)->mCount - 1;

    const EfAnimKeyEntry* entry = &entries[idx];
    if (entry->mFlags == 0) {
        fn_800A1290(&k, &entry->mValue.mKey);
    } else {
        u16 r = entry->mValue.mRandomIndex;
        u32 hash = ef_anim_name_hash(seed, header->mNameId, r, resDiv);
        if ((entry->mFlags & 2) == 0) {
            const EfAnimRandomEntry* re = &((const EfAnimRandomBlock*)random)->mEntry[r];
            fn_800A1290(&k, &re->mKey);
            switch (re->mMode) {
            case 1:
                k.mType = (k.mType & 2) | ((hash >> 16) & 1);
                break;
            case 2:
                k.mType = (k.mType & 1) | ((hash >> 16) & 2);
                break;
            case 3:
                k.mType = (hash >> 16) & 3;
                break;
            }
        } else {
            const EfAnimRandomTable* table = (const EfAnimRandomTable*)randomTable;
            fn_800A1290(&k, &table->mKey[(hash >> 16) % table->mCount]);
        }
    }

    *channel = ((const EfAnimNameTable*)nameTable)->mName[k.mTick];
    NW4R_POINTER_ASSERT(channel, 0x6DC, lbl_80591FC4, lbl_80591E68);

    pp->mKeyValueBits &= (u16)~(0xF << (ch * 4));
    pp->mKeyValueBits |= (u16)((k.mValue & 0xF) << (ch * 4));
    pp->mKeyTypeBits &= (u8)~(3 << (ch * 2));
    pp->mKeyTypeBits |= (u8)((k.mType & 3) << (ch * 2));

    *target = channel;
    *nameTableOut = nameTable;
    divider->mSlotBase = k.mTick;
    divider->mType = k.mType;
    divider->mChannel = ch;
}

/* 0x800A1290 - copies a 4-byte key record (u8, u8, u16). */
void fn_800A1290(EfAnimKey* dst, const EfAnimKey* src) {
    dst->mValue = src->mValue;
    dst->mType = src->mType;
    dst->mTick = src->mTick;
}

/* 0x800A12AC - binary-searches a table of u16 keys (stride `stride` u16s) for `target` and return the
 * lower bracketing index in `*out`.  `flag` decides which side an exact hit narrows. */
void fn_800A12AC(s32* out, s32 target, const u16* ptrBase, u32 stride, s32 lo, s32 hi, int flag) {
    NW4R_POINTER_ASSERT(ptrBase, 0x24C, lbl_80592228, lbl_80592260);

    s32 mid = (lo + hi) / 2;

    if (target < EF_ANIM_KEY_AT(ptrBase, stride, lo) || (target == EF_ANIM_KEY_AT(ptrBase, stride, lo) && flag != 0)) {
        *out = lo;
        return;
    }
    if (EF_ANIM_KEY_AT(ptrBase, stride, hi) < target || (target == EF_ANIM_KEY_AT(ptrBase, stride, hi) && flag == 0)) {
        *out = hi;
        return;
    }
    s32 key = EF_ANIM_KEY_AT(ptrBase, stride, mid);
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
        key = EF_ANIM_KEY_AT(ptrBase, stride, mid);
    }
    *out = lo;
}

/* 0x800A14A4 - latches the one-shot flag and, the first time, folds the key type of `arg`'s channel into
 * the particle's key-type bit pairs. */
void ef_anim_latch_tex_type(EfAnimParticle* self, const EfAnimDivider* arg) {
    if (self->mLatched == 0) {
        self->mKeyTypeBits &= (u8)~(3 << (arg->mChannel * 2));
        self->mKeyTypeBits |= (u8)((arg->mType & 3) << (arg->mChannel * 2));
    }
    self->mLatched = 1;
}

/* 0x800A1504 - resolves a key's slot (`mPhase` folded into the divider's range) and returns the table word
 * at slot + 1 through `target`; ramp flag bit 2 picks calling the ramp helper or wrapping at the last key. */
void ef_anim_tex_ramp(EfAnimParticle* self, const EfAnimDivider* divider, const EfAnimRamp* ramp,
                 struct EfPmManager* manager, const EfAnimNameTable* nameTable, u32* target) {
    s32 span = ramp->mEnd - ramp->mStart;
    s32 idx = (u16)(divider->mSlotBase + self->mPhase % span);
    idx += ramp->mStart;

    if ((ramp->mFlags & 4) != 0) {
        if ((u16)idx == divider->mKeyCount - 1) {
            fn_800AB880(manager, (struct EfPmParticle*)self);
        }
    } else if ((u16)idx >= divider->mKeyCount - 1) {
        idx = (u16)(idx - (u16)(divider->mKeyCount - 1));
    }

    *target = nameTable->mName[(u16)idx];
    NW4R_POINTER_ASSERT(target, 0x70D, lbl_80591FC4, lbl_80591E68);
}

/* 0x800A16C4 (0x5E4): fires one child-creation key: resolves its payload (inline, or drawn from the random
 * table through the seed hash) and queues the named resource's creation on the particle's effect system;
 * the setting's kind picks the queue entry type. */
void fn_800A16C4(const EfAnimChildKey* ptrFixed, u16 seed, const EfAnimHeader* header,
                 const EfAnimNameTable* nameTable, const EfAnimChildTable* randomTable, EfAnimParticle* pp,
                 u32 div) {
    NW4R_POINTER_ASSERT(ptrFixed, 0x71E, lbl_805921CC, lbl_80591E68);
    NW4R_POINTER_ASSERT(header, 0x71F, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(nameTable, 0x720, lbl_80592160, lbl_80591E68);
    NW4R_POINTER_ASSERT(randomTable, 0x721, lbl_80592120, lbl_80591E68);
    NW4R_POINTER_ASSERT(pp, 0x722, lbl_80592198, lbl_80591E68);

    const EfAnimChildSetting* setting;
    if (ptrFixed->mFlags == 0) {
        setting = &ptrFixed->mData.mSetting;
    } else {
        u32 hash = ef_anim_name_hash(seed, header->mNameId, ptrFixed->mData.mRandomIndex, div);
        if ((ptrFixed->mFlags & 2) == 0) {
            NW4R_ASSERT(0, 0x744, lbl_80592000, lbl_80591E68);
        } else {
            if (randomTable->mCount == 0) {
                return;
            }
            setting = &randomTable->mEntry[(hash >> 16) % randomTable->mCount];
        }
    }

    EffectHandle* resource = (EffectHandle*)nameTable->mName[setting->mNameIndex];
    if (resource == 0) {
        return;
    }
    if (setting->mSetting.mArg4 == 0) {
        ef_creation_queue_add_type0(&pp->mManager->mManagerEM->mManagerEF->mManagerES->mCreationQueue,
                                    &setting->mSetting, (EffectManager*)pp, resource, pp->mLife, 0, 0);
    } else {
        ef_creation_queue_add_type1(&pp->mManager->mManagerEM->mManagerEF->mManagerES->mCreationQueue,
                                    &setting->mSetting, (EffectManager*)pp, resource, pp->mLife, 0, 0);
    }
}

/* 0x800A1CA8 (0x85C): fires every child-creation key the curve passes between `step` and `step + 1`: within
 * one division the keys between the two ticks, otherwise the rest of the first division, every key of
 * the divisions in between and the start of the last one; a ping-pong curve (flag bit 6) walks the odd
 * divisions backwards and skips the turning keys. */
void ef_anim_curve_child(const u8* mCmdList, EfAnimParticle* pp, u32 step, u16 seed, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x768, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(pp, 0x769, lbl_80592198, lbl_80591E68);

    const EfAnimHeader* header;
    const u8* key;
    const u8* random;
    const u8* randomTable;
    const u8* nameTable;
    f32 tickF32;
    u32 divA;
    u32 divB;
    s32 idx;
    u16 tickA;
    u16 tickB;
    u8 finalA;
    u8 finalB;
    const EfAnimChildKey* entry;

    fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
    fn_8009D5C0(mode, header, step, &tickA, &tickF32, &divA, &finalA);
    fn_8009D5C0(mode, header, step + 1, &tickB, &tickF32, &divB, &finalB);

    const EfAnimChildKey* keys = ((const EfAnimChildKeyBlock*)key)->mEntry;
    u32 div;
    if (divA == divB) {
        if (tickA == tickB) {
            if (finalA == 0) {
                return;
            }
            divB = divA + 1;
            tickB = 1;
        }
        if (tickA < tickB) {
            fn_800A12AC(&idx, tickA, &keys->mTick, sizeof(EfAnimChildKey), 0,
                        ((const EfAnimChildKeyBlock*)key)->mCount - 1, 1);
            for (entry = &keys[idx]; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
                if (entry->mTick >= tickA) {
                    if (tickB <= entry->mTick) {
                        return;
                    }
                    EF_ANIM_FIRE(entry);
                }
            }
        } else {
            fn_800A12AC(&idx, tickA, &keys->mTick, sizeof(EfAnimChildKey), 0,
                        ((const EfAnimChildKeyBlock*)key)->mCount - 1, 0);
            for (entry = &keys[idx]; idx >= 0; entry--, idx--) {
                if (tickA >= entry->mTick) {
                    if (entry->mTick <= tickB) {
                        return;
                    }
                    EF_ANIM_FIRE(entry);
                }
            }
        }
        return;
    }

    NW4R_ASSERT(divA < divB, 0x7D7, lbl_80592204, lbl_80591E68);

    if ((header->mFlags & 0x40) != 0) {
        if ((divA & 1) == 0) {
            fn_800A12AC(&idx, tickA, &keys->mTick, sizeof(EfAnimChildKey), 0,
                        ((const EfAnimChildKeyBlock*)key)->mCount - 1, 1);
            for (entry = &keys[idx]; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
                if (entry->mTick >= tickA) {
                    if (entry->mTick == header->mTickCount - 1) {
                        break;
                    }
                    EF_ANIM_FIRE(entry);
                }
            }
        } else {
            fn_800A12AC(&idx, tickA, &keys->mTick, sizeof(EfAnimChildKey), 0,
                        ((const EfAnimChildKeyBlock*)key)->mCount - 1, 0);
            for (entry = &keys[idx]; idx >= 0; entry--, idx--) {
                if (tickA >= entry->mTick) {
                    if (entry->mTick == 0) {
                        break;
                    }
                    EF_ANIM_FIRE(entry);
                }
            }
        }

        for (div = divA + 1; div < divB; div++) {
            if ((divA & 1) == 0) {
                idx = 0;
                for (entry = keys; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
                    if (entry->mTick == header->mTickCount - 1) {
                        break;
                    }
                    EF_ANIM_FIRE(entry);
                }
            } else {
                idx = ((const EfAnimChildKeyBlock*)key)->mCount - 1;
                for (entry = &keys[idx]; idx >= 0; entry--, idx--) {
                    if (entry->mTick == 0) {
                        break;
                    }
                    EF_ANIM_FIRE(entry);
                }
            }
        }

        if ((divB & 1) == 0) {
            idx = 0;
            for (entry = keys; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
                if (tickB <= entry->mTick) {
                    return;
                }
                EF_ANIM_FIRE(entry);
            }
        } else {
            idx = ((const EfAnimChildKeyBlock*)key)->mCount - 1;
            for (entry = &keys[idx]; idx >= 0; entry--, idx--) {
                if (tickB >= entry->mTick) {
                    return;
                }
                EF_ANIM_FIRE(entry);
            }
        }
        return;
    }

    fn_800A12AC(&idx, tickA, &keys->mTick, sizeof(EfAnimChildKey), 0,
                ((const EfAnimChildKeyBlock*)key)->mCount - 1, 1);
    for (entry = &keys[idx]; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
        if (entry->mTick >= tickA) {
            EF_ANIM_FIRE(entry);
        }
    }
    for (div = divA + 1; div < divB; div++) {
        idx = 0;
        for (entry = keys; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
            EF_ANIM_FIRE(entry);
        }
    }
    idx = 0;
    for (entry = keys; idx < ((const EfAnimChildKeyBlock*)key)->mCount; entry++, idx++) {
        if (tickB <= entry->mTick) {
            return;
        }
        EF_ANIM_FIRE(entry);
    }
}

/* 0x800A2504 - copies the `u8` channels the key's mask selects out of one key record. */
void fn_800A2504(const u8* mCmdList, u8* target, u32 step, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x92C, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x92D, lbl_80591FC4, lbl_80591E68);

    u8 mask = mCmdList[3];
    u8 end = fn_8009F848(mask);
    u32 count = fn_8009F834(mask);

    if (count != 0) {
        const EfAnimHeader* header;
        const u8* key;
        const u8* random;
        const u8* randomTable;
        const u8* nameTable;
        u16 idx;

        fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
        fn_800A27B4(header, step, &idx, mode);

        const u8* src = (const u8*)key + count * idx;
        u16 bit;
        for (bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = *src++;
            }
            target++;
        }
    }
}

/* 0x800A27B4 - the same tick resolution as fn_8009CDBC without the divisor/fraction outputs: the
 * `mode` argument is the curve's step count and the only result is the key tick. */
void fn_800A27B4(const EfAnimHeader* header, u32 step, u16* tick, u32 mode) {
    NW4R_POINTER_ASSERT(header, 0x8AB, lbl_80591E7C, lbl_80591E68);
    NW4R_POINTER_ASSERT(tick, 0x8AC, lbl_80591EB4, lbl_80591E68);

    u16 count = header->mTickCount;
    if (count <= 1) {
        *tick = 0;
        return;
    }

    u8 flags = header->mFlags;
    u32 bit5 = flags & 0x20;
    if (bit5 == 0 && header->mDivCount <= 1) {
        if ((flags & 0x80) == 0) {
            if (step >= count - 1) {
                *tick = count - 1;
                return;
            }
            *tick = step;
            return;
        }
        if (mode == 1) {
            *tick = count - 1;
            return;
        }
        s32 last = count - 1;
        f32 t = (f32)step * ((f32)last / (f32)(mode - 1));
        if (t > (f32)last) {
            t = (f32)last;
        }
        *tick = t;
        return;
    }

    if ((flags & 0x80) == 0) {
        u32 span = count - 1;
        u32 div = step / span;
        if ((flags & 0x40) == 0) {
            if (bit5 == 0 && div >= header->mDivCount) {
                *tick = span;
                return;
            }
            *tick = step - div * span;
            return;
        }
        if (bit5 == 0 && div >= header->mDivCount) {
            if (header->mDivCount % 2 == 0) {
                *tick = 0;
                return;
            }
            *tick = span;
            return;
        }
        if ((div & 1) == 0) {
            *tick = step - div * span;
            return;
        }
        *tick = span * (div + 1) - step;
        return;
    }

    u32 last = mode - 1;
    if (step >= last) {
        if ((flags & 0x40) == 0 || header->mDivCount % 2 != 0) {
            *tick = count - 1;
            return;
        }
        *tick = 0;
        return;
    }

    s32 span = count - 1;
    f32 rate = (f32)header->mDivCount * ((f32)span / (f32)last);
    u32 div = (u32)((f32)step * rate / (f32)span);
    f32 frac = (f32)step * rate - (f32)(div * span);
    if ((flags & 0x40) == 0) {
        *tick = frac;
        return;
    }
    if ((div & 1) == 0) {
        *tick = frac;
        return;
    }
    *tick = (f32)span - frac;
}

/* 0x800A2CF0 - the f32 twin of fn_800A2504: same mask walk, one f32 per selected channel. */
void fn_800A2CF0(const u8* mCmdList, f32* target, u32 step, u32 mode) {
    NW4R_POINTER_ASSERT(mCmdList, 0x950, lbl_80591F8C, lbl_80591E68);
    NW4R_POINTER_ASSERT(target, 0x951, lbl_80591FC4, lbl_80591E68);

    u8 mask = mCmdList[3];
    u8 end = fn_8009F848(mask);
    u32 count = fn_8009F834(mask);

    if (count != 0) {
        const EfAnimHeader* header;
        const u8* key;
        const u8* random;
        const u8* randomTable;
        const u8* nameTable;
        u16 idx;

        fn_8009F22C(mCmdList, &header, &key, &random, &randomTable, &nameTable);
        fn_800A27B4(header, step, &idx, mode);

        const f32* src = (const f32*)key + count * idx;
        u16 bit;
        for (bit = 1; bit <= end; bit = (u16)(bit << 1)) {
            if ((mask & bit) != 0) {
                *target = *src++;
            }
            target++;
        }
    }
}

/* 0x800A2FA4 - resets the whole creation queue: clear every slot's VEC3 tails and the count. */
CreationQueue* fn_800A2FA4(CreationQueue* queue) {
    CreationQueueEntry* slot = &queue->mEntry[0];
    do {
        fn_800A3008(slot);
        slot++;
    } while (slot < &queue->mEntry[0x400]);
    queue->mCount = 0;
    return queue;
}

/* 0x800A3008 - initialises a queue slot's two VEC3 tails. */
CreationQueueEntry* fn_800A3008(CreationQueueEntry* slot) {
    VEC3_ctor(&slot->mPos);
    VEC3_ctor(&slot->mVel);
    return slot;
}

} /* extern "C" */
