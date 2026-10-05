/*
 * ef/ef_emitter.cpp - the nw4r::ef emitter / particle-manager object layer, `.text`
 * 0x800A6258..0x800A99B4 (phase 4: the emitter-side resource object's constructor, its sub-object constructor and its deleting
 * destructor, 0x800A6258..0x800A6350, came over from the old `ef_effectsystem.cpp`, which ends at 0x800A6258).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` for every one of the proposal's 49 symbols - each
 * resolves to `map=fn_XXXXXXXX`, and the shared dump names none of those addresses either).
 *
 * The name is evidence class 1: the range's own assert sites pass the bare `__FILE__` string
 * "ef_emitter.cpp" (`lbl_80592850`, read from the DOL at that address), so the module is `ef` and the
 * extension is that name's suffix.  `langcheck.py`'s evidence for C++: the `.cpp` name, the
 * `Panic__Q24nw4r2dbFPCciPCce` / `Warning__Q24nw4r2dbFPCciPCce` callees, and the range's own vtable
 * (`lbl_80592BB0`, eight words pointing at this file's functions).  The siblings' scheme
 * (`ef_particlemanager.cpp`, `ef_emitterform.cpp`, `ef_resource.cpp`) gives the file its final
 * spelling.
 *
 * The seam is proven on both sides: the run starts where the previous unit ends and stops exactly at
 * `ef/ef_emitterform.cpp`'s first instruction (0x800A99B4, in splits.txt).  `.data`, `.sdata2` and
 * `.sbss` stay unclaimed (the data pass owns them); the source declares them `extern`.
 *
 * What it is.  The emitter-side objects of the NintendoWare effect library.  The range's own vtable
 * at `.data 0x80592BB0` is this file's functions in order - fn_800A6414 (destroy), fn_800A6420
 * (retire), fn_800A6EC4 (create child), fn_800A7378 (create), fn_800A8A5C, fn_800A8D30, fn_800A8DF8,
 * fn_800A8F18 - and the three asserts the range carries name its two containers:
 *   - `UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_PARTICLEMANAGER` (0x400) - the object's
 *     own particle-manager list at +0xC0 (fn_800A6350, fn_800A6938, fn_800A8D30);
 *   - `UtlistSize(&mManagerEF->mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER` (0x200) - the
 *     emitter manager's list, reached through +0xBC and living at manager+0x24 (fn_800A6420).
 * `mManagerEF` is at +0xBC, the work record (`em`) at +0xB8, the parent at +0xF4, and the object's
 * own MTX34 at +0x124.
 *
 * Load-bearing source shapes:
 *   - the pointer guards are the `NW4R_POINTER_ASSERT` shape the ef shape units share (six
 *     materialised BOOLs); the range uses three file strings ("ef_emitter.cpp", "particle.h",
 *     "effect.h"), so the macro takes file and message, and the `__LINE__` immediates are
 *     reproduced with `#line`.
 *   - the 0x1020 / 0x820 frames are `NW4R_EF_MAX_PARTICLEMANAGER` / `NW4R_EF_MAX_EMITTER`-wide stack
 *     arrays (fn_800A6350, fn_800A6420, fn_800A6938, fn_800A8D30).
 *   - the manager's out-of-line hooks (fn_800A66AC, fn_800A723C) call through the delegate at
 *     manager+0xA0; the create path (fn_800A6EC4) calls through the object `fn_800A4420` returns,
 *     whose function table sits at +0x1C.  Both are spelled as function-pointer tables.
 *
 * Status: all 49 symbols reconstructed, none left as a stub.  46 of them are at or above the 80 %
 * bar and 20 are byte-identical; the unit is 91.29 % (weighted over the range's 13924 bytes).  The
 * three below the bar are
 *   fn_800A834C  71.66 %  the per-frame spawner: `#pragma fp_contract off` is needed for most of the
 *                         expression chain but the target fuses one `f2*f1 - f0` into `fmsubs`, and
 *                         the 0x48-hook/delegate path and the tail's field order still differ;
 *   fn_800A89A0  49.84 %  the 0x30-byte block copy: the target pairs its loads/stores through a
 *                         second temporary (`lwz r5; lwz r0; stw r5; stw r0`) where MWCC here emits
 *                         one `lwz/stw` per word, so the register colours differ through all 24
 *                         instructions;
 *   fn_800A8D18  40.00 %  the flag getter: the target masks then booleanises (`rlwinm; neg; or;
 *                         srwi`), MWCC here folds `(flags & 0x200) != 0` into `extrwi` for every
 *                         spelling tried (`bool` return, `== 0x200`, `(x >> 9) & 1`, `!= 0`).
 * The per-symbol numbers and the residuals are in `.pi/notes/800a6350-fn-800a6350-7a8c.md`.
 *
 * Rules 1/2/3/6/9: this file is clean (`stylelint.py --diff main` reports no new violation).  Two
 * cross-unit findings belong to the next sweep and are booked in the unit's outbox:
 *   - `src/ef/ef_point.cpp:73` defines its own partial `EfEmitter` (same nw4r class, 0x100-byte
 *     lower bound).  This file's type is `EfEmitterObj` so the batch adds no second definition, but
 *     the two must become one `ef/ef_emitter.h` definition (rule 1) once this unit has a
 *     header;
 *   - nine units declare this unit's symbols `extern` (`fn_800A8A08`, `fn_800A7F00`, `fn_800A8A04`,
 *     `fn_800A8C24`, `fn_800A8998`); those declarations now resolve to an owner and belong in that
 *     header (rule 2).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_particlemanager.h" /* fn_800AB9F4 / fn_800AE360 are that unit's (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "ef/ef_util.h" /* fn_8009CD64, owned by ef_util.cpp's range (rule 2) */
#include "draw_shape/fn_800532DC.h" /* fn_800532DC, owned by draw_shape.cpp's range (rule 2) */

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

extern "C" {

/* The pooled data this range references but does not own (`.data 0x80592850..0x80592CC0`,
 * `.sdata2 0x80796000..0x80796030`, `.sbss 0x80794920`).  Declared, never defined. */
extern const char lbl_80592850[]; /* "ef_emitter.cpp"                                        */
extern const char lbl_80592860[]; /* "NW4R:Failed assertion UtlistSize(&mActivityList..."
                                     " < NW4R_EF_MAX_PARTICLEMANAGER"                        */
extern const char lbl_805928BC[]; /* "NW4R:Failed assertion UtlistSize(&mManagerEF->..."
                                     " < NW4R_EF_MAX_EMITTER"                                */
extern const char lbl_8059291C[]; /* "NW4R:Pointer Error\ntarget(=%p) is not valid pointer." */
extern const char lbl_80592954[]; /* "NW4R:Pointer Error\neh(=%p) is not valid pointer."     */
extern const char lbl_80592988[]; /* "NW4R:Pointer Error\nef(=%p) is not valid pointer."     */
extern const char lbl_805929C0[]; /* "NW4R:Pointer Error\naParentEF(=%p) is not valid..."    */
extern const char lbl_805929F8[]; /* "NW4R:Failed assertion false"                           */
extern const char lbl_80592A14[]; /* "incomplete relocation (emitter:%s)"                    */
extern const char lbl_80592A38[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     */
extern const char lbl_80592A6C[]; /* "NW4R:Pointer Error\nmManagerEF(=%p) is not valid..."   */
extern const char lbl_80592AA8[]; /* "NW4R:Pointer Error\nresult(=%p) is not valid..."       */
extern const char lbl_80592AE0[]; /* "NW4R:Pointer Error\norig(=%p) is not valid pointer."   */
extern const char lbl_80592B14[]; /* "NW4R:Failed assertion result != orig"                  */
extern const char lbl_80592B3C[]; /* "NW4R:Failed assertion idx < GetNumParticleManager()"   */
extern const char lbl_80592B70[]; /* "NW4R:Pointer Error\nptr(=%p) is not valid pointer."    */
extern const char lbl_80592BD0[]; /* "NW4R:Pointer Error\nresult(=%p) is not valid..."       */
extern const char lbl_80592C08[]; /* "particle.h"                                            */
extern const char lbl_80592C14[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     */
extern const char lbl_80592C48[]; /* "effect.h"                                              */
extern const char lbl_80592C54[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     */
extern const char lbl_80592C88[]; /* "effect.h"                                              */
extern const char lbl_80592C94[]; /* "NW4R:Failed assertion num < NumEmitTrack()"            */
extern const char lbl_80592CC0[]; /* "res_emitter.h"                                         */
extern const f32 lbl_80796000;    /* 100.0f                                                  */
extern const f32 lbl_80796004;    /* 0.0f                                                    */
extern const f64 lbl_80796008;    /* 0x4330000000000000, the u32 -> f64 magic                */
extern const f64 lbl_80796010;    /* 0x4330000080000000, the s32 -> f64 magic                */
extern const f32 lbl_80796018;    /* -1.0f                                                   */
extern const f32 lbl_8079601C;    /* 1.0f                                                    */
extern const f32 lbl_80796020;    /* 2.0f                                                    */
extern const f32 lbl_80796024;    /* 65536.0f                                                */
extern const f32 lbl_80796028;    /* pi/2                                                    */
extern s32 lbl_80794920;          /* the "current manager" cache slot                        */

/* Callees outside this unit.  All are still `fn_*` in the symbol map and unsplit (no owner file to
 * move the declaration to - the rule-2 gap the campaign records for an unsplit address). */
u16 fn_800A4AF0(void* list);                       /* UtlistSize */
u16 fn_8009B374(void* list, void** buf, u16 size); /* UtlistGetArray */
void* fn_80501C60(void* list, void* node);         /* GetNext */
void* fn_80501C9C(void* list, u16 index);          /* GetNth */
void* fn_800A5250(void* list);                     /* GetFirst */
s32 fn_800A5248(void* node);
void fn_800A43E8(void* list, void* node);
void fn_800A45DC(void* list, void* node);
void fn_800A4A1C(void* list, void* node);
void fn_800A49B8(void* node);
void fn_800A4428(void* list);
void fn_800A444C(void* self);
void fn_800A4474(void* manager, void* self);
void* fn_800A4420(void* self);
void* fn_800A4864(void* p); /* walks to an object's chain head */
void fn_800A486C(void* manager, void* emitter);
u32 fn_800A485C(void* p);
void* fn_800A4654(void* manager, void* em, u8 flag, s32 mode);
void fn_800A337C(void* p);
void fn_800A3390(void* dst, const void* src);
void fn_800A3800(void* p);
void fn_800A5114(void* manager, s32 flag);
void fn_800A5900(void* random, u32 seed);
void* fn_800A5484(void* p);
void* fn_800A60C0(void* manager);
void fn_800A52E4(void* manager, void* cb, void* arg, s32 flag, void* self);
void fn_800AEE0C(void* dst, const void* src);
void fn_8035B998(void* p);
void fn_8009F85C(void* rec, void* target, u32 life, u16 seed, s32 range);
void* fn_800B2878(void);
void fn_8009B448(void* mtx, void* vec);
void fn_8009BF08(void* vec, void* out);
void fn_8009C040(void* vec, void* out);
void fn_8009B650(void* vec, void* mtx);
void fn_8009BCB4(void* mtx, void* vec);
void fn_8009CC20(void* out, void* mtx, void* in);
void fn_8009CCAC(void* out, void* mtx, void* in);
f32 sqrt_f32(f32 x);
void subVec3(void* dst, void* a, void* b);
void* fn_800508AC(void* vec);
f32 PSVECSquareDistance(void* a, void* b);
void mtx34_identity(void* mtx);
void fn_80051424(void* dst, void* src, f32 scale);
void assignVec3(void* dst, void* src);
void fn_800514FC(void* dst, void* mtx, void* vec);
void fn_8007100C(void* dst, void* src);
void fn_800710BC(void* dst, void* a, void* b);
void mtx34_inverse(void* mtx, void* in);
void fn_8009CA30(void* mtx, f32 x, f32 y, f32 z);
void fn_8009CBA0(void* dst, void* mtx, void* vec);
void fn_80501390(void* dst, void* mtx, void* vec);
f32 fn_80463E2C(f32 x);
s32 fn_8009C484(void* a, void* b);
}

/* -------------------------------------------------------------------------------------------------
 * Types.  Every record states its size; the offsets are the ones the bodies load or store.
 * ------------------------------------------------------------------------------------------------- */

/* The engine vector (the same layout as nw4r's, but the engine's own library type). */
typedef nw4r::math::VEC3 EfVec; /* size: 0x0C */

/* The `nw4r::ut::List` head: the walkers start at `head` and follow the runtime link offset. */
typedef struct EfList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 offset;
    /* +0x0A */ u16 size;
    /* +0x0C */ u16 linkOffset;
    /* +0x0E */ u16 field_0x0E;
    /* +0x10 */ void* field_0x10;
    /* +0x14 */ void* field_0x14;
} EfList; /* size: 0x18 */

/* The random block `fn_800A5900` seeds and `fn_800A8A08` steps. */
typedef struct EfRandom {
    /* +0x00 */ u32 state;
} EfRandom; /* size: 0x04 */

/* The parameter record the object is initialised from (`em` in fn_800A6A04's body).  Only the
 * offsets this unit reads are named; the record continues past them. */
typedef struct EfEmitterWork {
    /* +0x000 */ u32 flags;      /* the object's +0x20 */
    /* +0x004 */ u32 field_0x04; /* the object's +0x24 */
    /* +0x008 */ u16 life;       /* the life cap */
    /* +0x00A */ u16 field_0x0A;
    /* +0x00C */ u8 field_0x0C;
    /* +0x00D */ s8 scale_step;  /* per-frame scale step, in hundredths */
    /* +0x00E */ s8 rate;        /* per-frame spawn rate, in hundredths */
    /* +0x00F */ s8 scale_range; /* the scale's random range, in hundredths */
    /* +0x010 */ f32 scale;
    /* +0x014 */ u16 field_0x14;
    /* +0x016 */ u16 field_0x16;
    /* +0x018 */ u16 interval;
    /* +0x01A */ u8 mode;
    /* +0x01B */ u8 alpha;
    /* +0x01C */ f32 vec_0x1C[3];
    /* +0x028 */ f32 vec_0x28[3];
    /* +0x034 */ u16 field_0x34;
    /* +0x036 */ u8 field_0x36;
    /* +0x037 */ u8 field_0x37;
    /* +0x038 */ f32 float_0x38[7];
    /* +0x054 */ EfVec vec_0x54;
    /* +0x060 */ EfVec vec_0x60;
    /* +0x06C */ EfVec vec_0x6C;
    /* +0x078 */ EfVec vec_0x78;
    /* +0x084 */ u8 color_r; /* the three channels, in hundredths */
    /* +0x085 */ u8 color_g;
    /* +0x086 */ u8 color_b;
    /* +0x087 */ u8 field_0x87;
    /* +0x088 */ u32 field_0x088; /* its low half seeds the object's random block */
} EfEmitterWork; /* size: 0x8C (lower bound: the record continues past it) */

/* The parameter block `fn_800A3390` copies into the object's +0xFC. */
typedef struct EfEmitterParam {
    /* +0x00 */ s16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 pad_0x08[0x1C];
} EfEmitterParam; /* size: 0x24 (lower bound: the highest byte fn_800A3390 copies) */

/* A function table reached as `*(void***)((u8*)obj + offset)`. */
typedef struct EfVt {
    /* +0x00 */ void* slots[16];
} EfVt; /* size: 0x40 */

/* The object `fn_800A4420` returns: its table pointer sits at +0x1C. */
typedef struct EfHost {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EfVt* vtable;
} EfHost; /* size: 0x20 */

/* The effect-side resource block the manager keeps at `managerEF + 0x20`.  Its random block is at
 * +0xC01C, the reference position at +0xC020, a matrix at +0xC02C and two range floats at
 * +0xC05C/+0xC060.  It continues past them. */
typedef struct EfEffectData {
    /* +0x0000 */ u8 pad_0x0000[0x0C];
    /* +0x000C */ void* chain; /* an object whose own table is at +0x00 */
    /* +0x0010 */ u8 pad_0x0010[0xC00C];
    /* +0x0C1C */ u32 random;
    /* +0x0C20 */ EfVec ref_pos;
    /* +0x0C2C */ nw4r::math::MTX34 ref_mtx;
    /* +0x0C5C */ f32 range_a;
    /* +0x0C60 */ f32 range_b;
    /* +0x0C64 */ u8 pad_0x0C64[0x04];
} EfEffectData; /* size: 0xC68 (lower bound) */

/* The delegate the emitter manager keeps at +0xA0; its "set params" and "destroy" hooks are slots
 * 2 and 3. */
typedef struct EfManagerSlots {
    /* +0x00 */ void (*v00)(void);
    /* +0x04 */ void (*v04)(void);
    /* +0x08 */ void (*setParams)(void* manager, void* pm);
    /* +0x0C */ void (*destroy)(void* manager, void* eh);
} EfManagerSlots; /* size: 0x10 */

/* The emitter manager: the emitters live on its activity list at +0x24 and the effect resource it
 * was made from at +0x20. */
typedef struct EfEmitterManager {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfEffectData* effect;
    /* +0x24 */ EfList emitters; /* mActivityList.mActiveList */
    /* +0x3C */ u8 pad_0x3C[0x08];
    /* +0x44 */ void* field_0x44;
    /* +0x48 */ void* field_0x48; /* the retire-all hook the frame path calls */
    /* +0x4C */ u8 pad_0x4C[0x54];
    /* +0xA0 */ EfManagerSlots* delegate;
} EfEmitterManager; /* size: 0xA4 (lower bound) */

/* The object this unit works on: one `nw4r::ef` resource-backed object (the emitter and its
 * particle managers share this layout - it is the base both are reached through).  The table
 * pointer is installed by the sibling unit's constructor (`fn_800A6258`), so it stays padding. */
typedef struct EfEmitterObj {
    /* +0x000 */ void** vtable;
    /* +0x004 */ u8 pad_0x004[0x08];
    /* +0x00C */ s32 state; /* 0 = free, 1 = created, 2 = retiring */
    /* +0x010 */ u8 pad_0x010[0x10];
    /* +0x020 */ u32 flags;
    /* +0x024 */ u32 flags2; /* a pointer to the emitter's parameter record */
    /* +0x028 */ union {
        f32 scale;
        u32 scale_flags;
    } scale_0x028;
    /* +0x02C */ f32 scale_step;
    /* +0x030 */ u16 interval;    /* the frames between spawns */
    /* +0x032 */ u16 life_frames; /* the life a spawn starts with */
    /* +0x034 */ f32 rate;
    /* +0x038 */ f32 life;
    /* +0x03C */ u16 life_max;
    /* +0x040 */ f32 color_r;
    /* +0x044 */ f32 color_g;
    /* +0x048 */ f32 color_b;
    /* +0x04C */ f32 animate_0x4C[3];
    /* +0x058 */ f32 animate_0x58[3];
    /* +0x064 */ u8 flags3;
    /* +0x065 */ u8 alpha;
    /* +0x066 */ u8 field_0x066;
    /* +0x067 */ u8 field_0x067;
    /* +0x068 */ f32 float_0x068[7];
    /* +0x084 */ EfVec vec_0x84;
    /* +0x090 */ EfVec position;
    /* +0x09C */ EfVec vec_0x9C;
    /* +0x0A8 */ EfVec rotation;
    /* +0x0B4 */ u32 field_0x0B4;
    /* +0x0B8 */ EfEmitterWork* work;
    /* +0x0BC */ EfEmitterManager* managerEF;
    /* +0x0C0 */ EfList particles; /* mActivityList.mActiveList */
    /* +0x0D8 */ u8 pad_0x0D8[0x06];
    /* +0x0DE */ u16 field_0x0DE; /* the spawn countdown the frame tick uses */
    /* +0x0E0 */ u16 field_0x0E0; /* the spawn countdown the spawner uses */
    /* +0x0E2 */ u8 pad_0x0E2[0x02];
    /* +0x0E4 */ u32 field_0x0E4;
    /* +0x0E8 */ u16 field_0x0E8;
    /* +0x0EA */ u16 seed;
    /* +0x0EC */ EfRandom random;
    /* +0x0F0 */ void* field_0x0F0;
    /* +0x0F4 */ EfEmitterObj* parent;
    /* +0x0F8 */ void* orig;
    /* +0x0FC */ EfEmitterParam param;
    /* +0x120 */ u8 transform_dirty;
    /* +0x124 */ nw4r::math::MTX34 matrix;
} EfEmitterObj; /* size: 0x154 (lower bound: +0x124 is the highest field any body touches) */

/* The particle-manager view of the same object: the extra fields the create/sweep paths use. */
typedef struct EfPmView {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u8 pad_0x0C[0x14];
    /* +0x20 */ void* owner; /* the record fn_800A3800 releases */
    /* +0x24 */ void* param; /* the parameter record the manager was made from */
    /* +0x28 */ u32 flags;  /* bit0..bit2 inherited from the emitter */
    /* +0x2C */ u8 pad_0x2C[0x5C];
    /* +0x88 */ s8 field_0x88;
    /* +0x89 */ u8 field_0x89;
    /* +0x8A */ u8 dirty;
} EfPmView; /* size: 0x8B (lower bound) */

/* The length-prefixed blocks the emitter resource is walked through: `fn_800A8C24` steps 8 bytes in
 * from its own `offset`, `fn_800A8BF8` 4. */
typedef struct EfResHeader {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 offset;
} EfResHeader; /* size: 0x08 */

typedef struct EfChainHeader {
    /* +0x00 */ u32 offset;
} EfChainHeader; /* size: 0x04 */

/* The context block fn_800A9790 carries through fn_800A9784/fn_800A52E4. */
typedef struct EfWalkCtx {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* cb;
    /* +0x08 */ void* arg;
    /* +0x0C */ u8 flag;
    /* +0x0D */ u8 pad_0x0D[0x03];
} EfWalkCtx; /* size: 0x10 */

/* One emitted track record of the emitter resource: its flag byte, the emitter-relative byte offset
 * the spawner adds, and the two flag bytes the resource's array header carries. */
typedef struct EfTrack {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 offset;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 flags;
    /* +0x05 */ u8 pad_0x05[0x03];
} EfTrack; /* size: 0x08 */

/* The particle the manager walkers read (`ef/ef_particlemanager.cpp`'s EfParticleRec). */
typedef struct EfParticleRec {
    /* +0x000 */ u8 pad_0x000[0x0C];
    /* +0x00C */ s32 field_0x00C;
    /* +0x010 */ u8 pad_0x010[0x9C];
    /* +0x0AC */ EfVec offset;
    /* +0x0B8 */ EfVec position;
    /* +0x0C4 */ u8 pad_0x0C4[0x04];
    /* +0x0C8 */ void* manager;
} EfParticleRec; /* size: 0xCC (lower bound) */

/* The work record a resource-backed object is initialised from (`fn_800A4864` is its chain head). */
static inline EfEmitterWork* EfGetWork(void* p) {
    return (EfEmitterWork*)fn_800A4864(p);
}

#define NW4R_EF_MAX_PARTICLEMANAGER 0x400
#define NW4R_EF_MAX_EMITTER 0x200

/* The global `operator delete` (its compiler mangling is `__dl__FPv`; declaring that spelling would be
 * rule 9's violation - the sibling ef units spell it the same way). */
void operator delete(void* ptr) throw();

/* The +0x20 sub-object of the emitter-side object `fn_800A62C0` initialises: four transform vectors,
 * cleared in this order.  The offsets are relative to the sub-object. size: 0x94 */
typedef struct EfSysResourceSub {
    /* +0x00 */ u8 pad_0x00[0x64];
    /* +0x64 */ nw4r::math::VEC3 vec_0x84;
    /* +0x70 */ nw4r::math::VEC3 vec_0x90;
    /* +0x7C */ nw4r::math::VEC3 vec_0x9C;
    /* +0x88 */ nw4r::math::VEC3 vec_0xA8;
} EfSysResourceSub; /* size: 0x94 */

/* The emitter-side object `fn_800A6258` builds: the constructor's view of `EfEmitterObj` (the same object; the table sits at +0x1C
 * here).  Moved with the constructor from the old `ef_effectsystem.cpp` in phase 4. size: 0x154 (lower bound: +0x124 is the highest
 * field the constructor touches) */
typedef struct EfSysResourceObj {
    /* +0x000 */ u8 pad_0x000[0x01C];
    /* +0x01C */ void* vtable; /* the root base's table fn_800A4080 sets, then this class's */
    /* +0x020 */ EfSysResourceSub sub_0x020;
    /* +0x0B4 */ u8 pad_0x0B4[0x00C];
    /* +0x0C0 */ u8 particles[0x1C]; /* the activity-list record `fn_800A3FFC` initialises (EfEmitterObj's `particles`) */
    /* +0x0DC */ u8 pad_0x0DC[0x02C];
    /* +0x108 */ nw4r::math::VEC3 vec_0x108;
    /* +0x114 */ nw4r::math::VEC3 vec_0x114;
    /* +0x120 */ u8 pad_0x120[0x004];
    /* +0x124 */ nw4r::math::MTX34 mtx_0x124;
} EfSysResourceObj; /* size: 0x154 */

extern void* lbl_80592BA8[]; /* the emitter-side object's table (this unit's eight virtuals) */

extern "C" {
EfSysResourceObj* fn_800A6258(EfSysResourceObj* self);
EfSysResourceSub* fn_800A62C0(EfSysResourceSub* self);
void* fn_800A630C(void* self, s16 flag);
void* fn_800A3FFC(void* list, u32 linkOffset);
void fn_800A4080(void* self);
}
/* This unit's own symbols, in address order (the file defines them in that order). */
extern "C" {
u32 fn_800A6350(EfEmitterObj* self);
void fn_800A6414(EfEmitterObj* self);
void fn_800A6420(EfEmitterObj* self);
s32 fn_800A6554(EfEmitterManager* em, EfEmitterObj* target);
void fn_800A66A4(EfEmitterObj* self, u32 state);
void fn_800A66AC(EfEmitterManager* self, void* eh);
s32 fn_800A67E8(EfEmitterObj* self, EfParticleRec* target);
u32 fn_800A6938(EfEmitterObj* self);
s32 fn_800A6A04(EfEmitterObj* self, void* eh, EfEmitterManager* ef);
u32 fn_800A6E64(EfEmitterWork* work);
u16 fn_800A6E70(void* random);
void fn_800A6EA4(void* random);
s32 fn_800A6EC4(EfEmitterObj* self, EfEmitterManager* aParentEF, void* eh, u8 value);
void fn_800A723C(EfEmitterManager* self, void* pm);
EfEmitterObj* fn_800A7378(EfEmitterObj* self, void* em, const EfEmitterParam* params, EfParticleRec* pm,
                       s32 life_bonus, void* host);
s32 fn_800A7750(EfEmitterObj* self, void* eh, const EfEmitterParam* params, EfParticleRec* pm,
                s32 life_bonus, void* host);
EfVec* fn_800A7F00(EfParticleRec* self, EfVec* result);
void* fn_800A8040(EfEmitterObj* self, void* target, u32 a, u32 b, s8 c, u32 d, u8 e);
f32 fn_800A8220(void* a, void* b, f32 p1, f32 p2, f32 p3, f32 p4);
f32 fn_800A8300(void* a, void* b);
void fn_800A834C(EfEmitterObj* self, EfParticleRec* pm, void* mtx);
void* fn_800A8944(void* self);
void* fn_800A8968(EfEmitterObj* self);
void* fn_800A898C(void* dst, const void* src);
void* fn_800A8998(void* dst, s32 v);
void* fn_800A89A0(void* dst, const void* src);
f32 fn_800A8A04(f32 x);
f32 fn_800A8A08(u32* random);
void fn_800A8A5C(EfEmitterObj* self);
u16 fn_800A8BC8(void* res);
void* fn_800A8BF8(void* res);
void* fn_800A8C24(void* res);
void* fn_800A8C34(void* res, u16 index);
void* fn_800A8CB8(void* res);
u16 fn_800A8CE8(void* res);
u32 fn_800A8D18(EfEmitterObj* self);
void fn_800A8D30(EfEmitterObj* self);
void fn_800A8DF8(EfEmitterObj* self);
void fn_800A8F18(EfEmitterObj* self);
void* fn_800A90AC(void* dst, void* orig, u32 a, u32 b, s8 c, u32 d);
void* fn_800A94A4(EfEmitterObj* self, void* out);
void fn_800A95D8(EfEmitterObj* self);
void fn_800A96C0(EfPmView* pm);
u16 fn_800A9700(EfPmView* pm);
u16 fn_800A970C(EfEmitterObj* self);
void* fn_800A9714(EfEmitterObj* self, u16 index);
void fn_800A9784(EfEmitterObj* self, void* cb, void* arg, s32 flag);
void fn_800A9790(EfEmitterObj* self, EfWalkCtx* arg);
s32 fn_800A98D4(EfEmitterObj* self, void* cb, void* arg, s32 flag, s32 recurse);
}

/* -------------------------------------------------------------------------------------------------
 * Assert shapes
 * ------------------------------------------------------------------------------------------------- */

/* `NW4R_ASSERT(expr)`: the file is "ef_emitter.cpp" and the message is the literalised expression;
 * the `__LINE__` value comes from a `#line` directive at the call site (the sibling units' device). */
#define NW4R_ASSERT(expr, msg)                                                                     \
    if (!(expr))                                                                                   \
    nw4r::db::Panic(lbl_80592850, __LINE__, msg)

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (six materialised BOOLs), shared verbatim with
 * ef/ef_particlemanager.cpp and the ef shape units; the file differs by call site. */
#define NW4R_POINTER_ASSERT(file, ptr, msg)                                                  \
    {                                                                                        \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;   \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                 \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))          \
            ok6_ = FALSE;                                                                    \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                           \
            ok5_ = FALSE;                                                                    \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                 \
            ok4_ = FALSE;                                                                    \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                           \
            ok3_ = FALSE;                                                                    \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                           \
            ok2_ = FALSE;                                                                    \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                           \
            ok1_ = FALSE;                                                                    \
        if (!ok1_)                                                                           \
            nw4r::db::Panic(file, __LINE__, msg, (ptr));                                     \
    }

/* -------------------------------------------------------------------------------------------------
 * 0x800A6350 - retire every particle of every manager on this object's list.
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off

/* 0x800A6258 - the emitter-side resource object's constructor (this unit owns its layout). */
extern "C" EfSysResourceObj* fn_800A6258(EfSysResourceObj* self) {
    fn_800A4080(self);
    self->vtable = lbl_80592BA8;
    fn_800A62C0(&self->sub_0x020);
    fn_800A3FFC(&self->particles, 0x14);
    VEC3_ctor(&self->vec_0x108);
    VEC3_ctor(&self->vec_0x114);
    MTX34_ctor(&self->mtx_0x124);
    return self;
}

/* 0x800A62C0 - the resource object's +0x20 sub-object: clear its four transform vectors. */
extern "C" EfSysResourceSub* fn_800A62C0(EfSysResourceSub* self) {
    VEC3_ctor(&self->vec_0x84);
    VEC3_ctor(&self->vec_0x90);
    VEC3_ctor(&self->vec_0x9C);
    VEC3_ctor(&self->vec_0xA8);
    return self;
}

/* 0x800A630C - the resource object's deleting destructor. */
extern "C" void* fn_800A630C(void* self, s16 flag) {
    if (self != NULL && flag > 0) {
        operator delete(self);
    }
    return self;
}

#pragma peephole on

extern "C" u32 fn_800A6350(EfEmitterObj* self) {
    u32 total = 0;
#line 53
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, lbl_80592860);
    u16 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        total += fn_800AB9F4((struct EfPmManager*)list[i]);
    }
    return total;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6414 - the table's destroy slot.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A6414(EfEmitterObj* self) {
    fn_800A4474(self->managerEF, self);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6420 - the table's retire slot: drop the owned managers, then sweep the emitter manager's
 * list for this object's children.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A6420(EfEmitterObj* self) {
    if (self->field_0x0F0 != NULL) {
        self->field_0x0F0 = NULL;
    }
    if ((self->flags & 1) != 0) {
        fn_800A6350(self);
    }
    fn_800A6938(self);
    if ((self->flags & 1) != 0) {
#line 88
        NW4R_ASSERT(fn_800A4AF0(&self->managerEF->emitters) < NW4R_EF_MAX_EMITTER, lbl_805928BC);
        u16 size = fn_800A4AF0(&self->managerEF->emitters);
        void* list[NW4R_EF_MAX_EMITTER];
        u16 num = fn_8009B374(&self->managerEF->emitters, list, size);
        for (u16 i = 0; i < num; i++) {
            EfEmitterObj* eh = (EfEmitterObj*)list[i];
            EfEmitterObj* p = eh->parent;
            while (p != NULL) {
                if (p == self) {
                    fn_800A6350(eh);
                    if (eh->state == 1) {
                        fn_800A486C(self->managerEF, eh);
                    }
                    break;
                }
                p = p->parent;
            }
        }
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6554 - hand a manager to the emitter manager and retire it locally.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A6554(EfEmitterManager* em, EfEmitterObj* target) {
#line 118
    NW4R_POINTER_ASSERT(lbl_80592850, target, lbl_8059291C);
    fn_800A66AC(em, target);
    fn_800A3800(((EfPmView*)target)->owner);
    fn_800A45DC(&em->emitters, target);
    fn_800A66A4((EfEmitterObj*)target, 3);
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A66A4 - set an object's state word.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A66A4(EfEmitterObj* self, u32 state) {
    self->state = state;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A66AC - the emitter manager's destroy hook (delegate slot 3).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A66AC(EfEmitterManager* self, void* eh) {
#line 244
    NW4R_POINTER_ASSERT(lbl_80592C88, eh, lbl_80592C54);
    self->delegate->destroy(self, eh);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A67E8 - retire a manager if it is on the active list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A67E8(EfEmitterObj* self, EfParticleRec* target) {
#line 130
    NW4R_POINTER_ASSERT(lbl_80592850, target, lbl_8059291C);
    if (fn_800A5248(target) != 1) {
        return 0;
    }
    fn_800A4A1C(&self->particles, target);
    fn_800A49B8(target);
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6938 - retire this object's managers that are on the active list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800A6938(EfEmitterObj* self) {
    u32 total = 0;
#line 147
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, lbl_80592860);
    u16 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        void* target = list[i];
        if (fn_800A5248(target) == 1) {
            total += fn_800A67E8(self, (EfParticleRec*)target);
        }
    }
    return total;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6A04 - initialise an object from a parameter source (`eh`) and its emitter manager (`ef`).
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A6A04(EfEmitterObj* self, void* eh, EfEmitterManager* ef) {
#line 166
    NW4R_POINTER_ASSERT(lbl_80592850, eh, lbl_80592954);
#line 167
    NW4R_POINTER_ASSERT(lbl_80592850, ef, lbl_80592988);
    EfEmitterWork* work = EfGetWork(eh);
    self->field_0x0E4 = 0;
    self->state = 1;
    self->work = work;
    work = EfGetWork(work);
    self->field_0x0B4 = 0;
    self->field_0x0E8 = work->field_0x16;
    self->flags = work->flags;
    self->flags2 = work->field_0x04;
    self->life_max = work->life;
    self->field_0x0DE = work->field_0x14;
    self->seed = 0;
    self->color_g = (f32)work->color_g / lbl_80796000;
    self->color_r = (f32)work->color_r / lbl_80796000;
    self->color_b = (f32)work->color_b / lbl_80796000;
    self->scale_0x028.scale = work->scale;
    self->scale_step = (f32)work->scale_range / lbl_80796000;
    self->interval = work->interval;
    self->life_frames = work->field_0x34;
    self->rate = (f32)work->rate / lbl_80796000;
    self->life = lbl_80796004;
    self->transform_dirty = 1;
    copyVec3(&self->position, &work->vec_0x78);
    copyVec3(&self->vec_0x9C, &work->vec_0x60);
    copyVec3(&self->rotation, &work->vec_0x6C);
    self->flags3 = 3;
    self->alpha = 100;
    self->field_0x066 = work->field_0x36;
    self->field_0x067 = work->field_0x37;
    for (s32 i = 0; i < 7; i++) {
        self->float_0x068[i] = work->float_0x38[i];
    }
    copyVec3(&self->vec_0x84, &work->vec_0x54);
    fn_800B2878();
    self->seed = (u16)work->field_0x088;
    if (self->seed == 0) {
        self->seed = fn_800A6E70(&ef->effect->random);
    }
    fn_800A5900(&self->random, self->seed);
    self->animate_0x4C[0] = work->vec_0x1C[0];
    self->animate_0x4C[1] = work->vec_0x1C[1];
    self->animate_0x4C[2] = work->vec_0x1C[2];
    self->animate_0x58[0] = work->vec_0x28[0];
    self->animate_0x58[1] = work->vec_0x28[1];
    self->animate_0x58[2] = work->vec_0x28[2];
    self->transform_dirty = 1;
    self->orig = NULL;
    {
        u32 id = fn_800A6E64(work);
        EfHost* host = (EfHost*)fn_800A4420(ef->effect);
        self->field_0x0F0 =
            ((void* (*)(EfHost*, u32))host->vtable->slots[2])(host, id);
    }
    self->parent = NULL;
    self->managerEF = NULL;
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6E64 - the parameter record's byte at +0x04.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800A6E64(EfEmitterWork* work) {
    return work->field_0x04 & 0xFF;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6E70 - step a random block and return the high half of its state.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u16 fn_800A6E70(void* random) {
    fn_800A6EA4(random);
    return (u16)(*(u32*)random >> 16);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6EA4 - the generator's step (state = state * 0x343FD + 0x269EC3).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A6EA4(void* random) {
    u32 state = *(u32*)random;
    *(u32*)random = state * 0x343FD + 0x269EC3;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6EC4 - the table's create-child slot.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A6EC4(EfEmitterObj* self, EfEmitterManager* aParentEF, void* eh, u8 value) {
#line 246
    NW4R_POINTER_ASSERT(lbl_80592850, aParentEF, lbl_805929C0);
#line 247
    NW4R_POINTER_ASSERT(lbl_80592850, eh, lbl_80592954);
    fn_800A4864(eh);
    fn_800A444C(self);
    fn_800A4428(&self->particles);
    fn_800A6A04(self, eh, aParentEF);
    self->managerEF = aParentEF;
    {
        EfHost* host = (EfHost*)fn_800A4420(aParentEF->effect);
        EfPmView* created =
            ((EfPmView* (*)(EfHost*, EfEmitterObj*, void*))host->vtable->slots[4])(host, self, eh);
        if (created == NULL) {
#line 264
            nw4r::db::Panic(lbl_80592850, __LINE__, lbl_805929F8);
            return 0;
        }
        fn_800A337C(self->managerEF);
        fn_800A43E8(&self->particles, created);
        fn_800A66A4((EfEmitterObj*)created, 1);
        created->flags = 0;
        if ((EfGetWork(aParentEF)->flags & 0x20) != 0) {
            created->flags |= 1;
        }
        if ((EfGetWork(aParentEF)->flags & 0x40) != 0) {
            created->flags |= 2;
        }
        created->field_0x88 = *(s8*)((u8*)EfGetWork(aParentEF) + 0x1A);
        if ((EfGetWork(aParentEF)->flags & 0x400) != 0) {
            created->flags |= 4;
        }
        created->field_0x89 = value;
        fn_800A723C(self->managerEF, created);
    }
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A723C - the emitter manager's set-params hook (delegate slot 2).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A723C(EfEmitterManager* self, void* pm) {
#line 237
    NW4R_POINTER_ASSERT(lbl_80592C48, pm, lbl_80592C14);
    self->delegate->setParams(self, pm);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A7378 - the table's create-emitter slot.
 * ------------------------------------------------------------------------------------------------- */

extern "C" EfEmitterObj* fn_800A7378(EfEmitterObj* self, void* em, const EfEmitterParam* params,
                                  EfParticleRec* pm, s32 life_bonus, void* host) {
#line 307
    NW4R_POINTER_ASSERT(lbl_80592850, em, lbl_80592954);
    if ((EfGetWork(em)->flags & 2) == 0) {
        nw4r::db::Warning(lbl_80592850, 0x137, lbl_80592A14, fn_800A485C(em));
        return NULL;
    }
    EfEmitterObj* e = (EfEmitterObj*)fn_800A4654(self->managerEF, em, params->field_0x05, 0);
    if (e == NULL) {
        return NULL;
    }
    e->field_0x0E8 += (u16)life_bonus;
    e->parent = self;
    fn_800A337C(e->parent);
    e->flags3 = 0;
    if ((self->flags & 0x80) != 0) {
        e->flags3 |= 1;
    }
    if ((self->flags & 0x100) != 0) {
        e->flags3 |= 2;
    }
    e->alpha = self->work->mode;
    if (e->alpha == 0) {
        e->alpha = 100;
    }
    if ((self->flags & 0x800) != 0) {
        e->flags3 |= 4;
    }
    {
        EfVec v;
        nw4r::math::MTX34 m1, m2;
        assignVec3(&v, &e->position);
        e->position.x = lbl_80796004;
        e->position.y = lbl_80796004;
        e->position.z = lbl_80796004;
        fn_800A95D8(e);
        MTX34_ctor(&m1);
        MTX34_ctor(&m2);
        mtx34_identity(&m1);
        fn_8009CA30(&m1, e->rotation.x, e->rotation.y, e->rotation.z);
        fn_8009CBA0(&m1, &m1, &e->vec_0x9C);
        fn_800A94A4(e, &m2);
        mtx34_inverse(&m2, &m2);
        fn_800710BC(&m1, &m1, &m2);
        if (pm != NULL) {
            fn_800AE360((void*)pm->manager, &m2);
            fn_800710BC(&m1, &m1, &m2);
        }
        if (host == NULL) {
            if (pm != NULL) {
                fn_80501390(&m1, &m1, &pm->offset);
            }
        } else {
            fn_80501390(&m1, &m1, host);
        }
        fn_8009CA30(&m2, e->rotation.x, e->rotation.y, e->rotation.z);
        fn_8009CBA0(&m2, &m2, &e->vec_0x9C);
        mtx34_inverse(&m2, &m2);
        fn_800710BC(&m1, &m1, &m2);
        e->position.x = v.x + m1.m[0][3];
        e->position.y = v.y + m1.m[1][3];
        e->position.z = v.z + m1.m[2][3];
        fn_800A95D8(e);
    }
    if (params->field_0x00 != 0 || params->field_0x02 != 0 || params->field_0x03 != 0 ||
        params->field_0x04 != 0 || (params->field_0x07 & 2) != 0) {
        e->orig = pm;
        fn_800A337C(pm);
        fn_800A3390(&e->param, params);
    }
    return e;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A7750 - the frame's "create from the effect's emitter table" path.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A7750(EfEmitterObj* self, void* eh, const EfEmitterParam* params, EfParticleRec* pm,
                           s32 life_bonus, void* host) {
#line 432
    NW4R_POINTER_ASSERT(lbl_80592850, eh, lbl_80592954);
    if ((EfGetWork(eh)->flags & 2) == 0) {
        nw4r::db::Warning(lbl_80592850, 0x1B4, lbl_80592A14, fn_800A485C(eh));
        return 0;
    }
    {
        EfEmitterObj e;
        EfVec v;
        nw4r::math::MTX34 m1, m2;

        fn_800A6258((EfSysResourceObj*)&e);
        fn_800A4864(eh);
        fn_800A6A04(&e, eh, self->managerEF);
        e.field_0x0E8 += (u16)life_bonus;
        e.managerEF = self->managerEF;
        e.parent = self;
        e.flags3 = 0;
        if ((self->flags & 0x1000) != 0) {
            e.flags3 |= 1;
        }
        if ((self->flags & 0x2000) != 0) {
            e.flags3 |= 2;
        }
        e.alpha = self->work->alpha;
        if ((s8)e.alpha != 0) {
            e.alpha = 100;
        }
        if ((self->flags & 0x800) != 0) {
            e.flags3 |= 4;
        }
        if ((params->field_0x07 & 1) != 0 && params->field_0x00 != 0) {
            VEC3_ctor(&v);
            MTX34_ctor(&m1);
            MTX34_ctor(&m2);
            fn_800AE360((void*)pm->manager, &m2);
            m2.m[0][3] = lbl_80796004;
            m2.m[1][3] = lbl_80796004;
            m2.m[2][3] = lbl_80796004;
            fn_800A7F00(pm, &v);
            fn_800514FC(&v, &m2, &v);
            if (fn_8009C484(&v, &v) != 0) {
                if (params->field_0x00 < 0) {
                    fn_80051424(&v, &v, lbl_80796018);
                }
                fn_8009B448(&m1, &v);
                fn_8009CA30(&m2, e.rotation.x, e.rotation.y, e.rotation.z);
                fn_800710BC(&m1, &m1, &m2);
                fn_8009BCB4(&m1, &e.rotation);
            }
        }
        {
            EfVec pos;
            assignVec3(&pos, &e.position);
            e.position.x = lbl_80796004;
            e.position.y = lbl_80796004;
            e.position.z = lbl_80796004;
            fn_800A95D8(&e);
            MTX34_ctor(&m1);
            MTX34_ctor(&m2);
            mtx34_identity(&m1);
            fn_8009CA30(&m1, e.rotation.x, e.rotation.y, e.rotation.z);
            fn_8009CBA0(&m1, &m1, &e.vec_0x9C);
            fn_800A94A4(&e, &m2);
            mtx34_inverse(&m2, &m2);
            fn_800710BC(&m1, &m1, &m2);
            if (pm != NULL) {
                fn_800AE360((void*)pm->manager, &m2);
                fn_800710BC(&m1, &m1, &m2);
            }
            if (host == NULL) {
                if (pm != NULL) {
                    fn_80501390(&m1, &m1, &pm->offset);
                }
            } else {
                fn_80501390(&m1, &m1, host);
            }
            fn_8009CA30(&m2, e.rotation.x, e.rotation.y, e.rotation.z);
            fn_8009CBA0(&m2, &m2, &e.vec_0x9C);
            mtx34_inverse(&m2, &m2);
            fn_800710BC(&m1, &m1, &m2);
            e.position.x = pos.x + m1.m[0][3];
            e.position.y = pos.y + m1.m[1][3];
            e.position.z = pos.z + m1.m[2][3];
            fn_800A95D8(&e);
        }
        fn_800A8A5C(&e);
        fn_800A8F18(&e);
        {
            s32 use_a = 0;
            s32 use_b = 0;
            s32 use_c = 1;
            s8 scale = 0;
            EfEmitterObj* created;
            if ((self->flags & 0x1000) != 0 && (EfGetWork(eh)->flags & 0x20) != 0) {
                use_a = 1;
            }
            if ((self->flags & 0x2000) != 0 && (EfGetWork(eh)->flags & 0x40) != 0) {
                use_b = 1;
            }
            if ((self->flags & 0x800) == 0 && (EfGetWork(eh)->flags & 0x400) == 0) {
                use_c = 0;
            }
            {
                s8 a = *(s8*)((u8*)EfGetWork(self->work) + 0x0D);
                s8 b = *(s8*)((u8*)EfGetWork(eh) + 0x1A);
                if (a != 0 && b != 0) {
                    if (a == 100) {
                        scale = b;
                    } else if (b == 100) {
                        scale = a;
                    } else {
                        scale = (s8)((s32)a * (s32)b / 100);
                    }
                }
            }
            created = (EfEmitterObj*)fn_800A8040(self, eh, (u32)use_a, (u32)use_b, scale, (u32)use_c,
                                              params->field_0x05);
            if (created == NULL) {
                EfHost* ch = (EfHost*)fn_800A4420(self->managerEF->effect);
                created = ((EfEmitterObj* (*)(EfHost*, EfEmitterObj*, void*))ch->vtable->slots[4])(
                    ch, self, eh);
                if (created == NULL) {
                    return 0;
                }
                fn_800A43E8(&self->particles, created);
                fn_800A66A4((EfEmitterObj*)created, 1);
                created->scale_0x028.scale = lbl_80796004;
                if (use_a != 0) {
                    created->scale_0x028.scale_flags |= 1;
                }
                if (use_b != 0) {
                    created->scale_0x028.scale_flags |= 2;
                }
                ((EfPmView*)created)->field_0x88 = scale;
                if (use_c != 0) {
                    created->scale_0x028.scale_flags |= 4;
                }
                ((EfPmView*)created)->field_0x89 = params->field_0x05;
                fn_800A723C(self->managerEF, created);
            }
            if (params->field_0x00 != 0 || params->field_0x02 != 0 || params->field_0x03 != 0 ||
                params->field_0x04 != 0 || (params->field_0x07 & 2) != 0) {
                created->orig = pm;
                fn_800A337C(pm);
                fn_800A3390(&created->param, params);
            } else {
                created->orig = NULL;
            }
            {
                nw4r::math::MTX34 m3, m4;
                MTX34_ctor(&m3);
                MTX34_ctor(&m4);
                fn_800AE360((void*)created, &m4);
                mtx34_inverse(&m4, &m4);
                fn_800A94A4(&e, &m3);
                fn_800710BC(&m3, &m4, &m3);
                fn_800A834C(&e, (EfParticleRec*)created, &m3);
            }
            e.parent = NULL;
            e.managerEF = NULL;
            if (created->orig != NULL) {
                fn_800A3800(pm);
                created->orig = NULL;
            }
            if (self->state != 1 && fn_800A5248(created) == 1) {
                fn_800A67E8(self, (EfParticleRec*)created);
            }
        }
    }
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A7F00 - a particle's global position.
 * ------------------------------------------------------------------------------------------------- */

extern "C" EfVec* fn_800A7F00(EfParticleRec* self, EfVec* result) {
#line 300
    NW4R_POINTER_ASSERT(lbl_80592C08, result, lbl_80592BD0);
    {
        EfVec v;
        subVec3(&v, &self->offset, &self->position);
        copyVec3(result, &v);
    }
    return result;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8040 - sweep this object's managers for the one matching a set of flag bytes.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A8040(EfEmitterObj* self, void* target, u32 a, u32 b, s8 c, u32 d, u8 e) {
#line 700
    NW4R_POINTER_ASSERT(lbl_80592850, target, lbl_8059291C);
    void* node = fn_800A5250(&self->particles);
    while (node != NULL) {
        EfPmView* pm = (EfPmView*)node;
        if (pm->param == target) {
            u32 flags = pm->flags;
            if (a == (flags & 1) && b == (flags & 2) && pm->field_0x88 == c &&
                d == (flags & 4) && pm->field_0x89 == e) {
                return node;
            }
        }
        node = fn_80501C60(&self->particles, node);
    }
    return NULL;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8220 - the distance fade factor.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 fn_800A8220(void* a, void* b, f32 p1, f32 p2, f32 p3, f32 p4) {
    f32 range = p1 - p2;
    f32 d = sqrt_f32(fn_800A8300(b, a));
    f32 x = d - p2;
    f32 lo = range * p4;
    f32 hi = range * p3;
    if (lo > x) {
        return lbl_8079601C;
    }
    if (hi < x) {
        return lbl_80796004;
    }
    if (hi - lo == lbl_80796004) {
        return lbl_80796004;
    }
    return lbl_8079601C - (x - lo) / (hi - lo);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8300 - squared distance between two packed position records.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 fn_800A8300(void* a, void* b) {
    void* va = fn_800508AC(a);
    void* vb = fn_800508AC(b);
    return PSVECSquareDistance(va, vb);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A834C - the per-frame particle spawner.
 * ------------------------------------------------------------------------------------------------- */

#pragma fp_contract off
extern "C" void fn_800A834C(EfEmitterObj* self, EfParticleRec* pm, void* mtx) {
#line 768
    NW4R_POINTER_ASSERT(lbl_80592850, pm, lbl_80592A38);
    if (self->field_0x0E0 != 0) {
        self->field_0x0E0--;
        return;
    }
    self->field_0x0E0 = self->interval;
    if (self->rate != lbl_80796004) {
        f32 r = fn_800A8A08(&self->random.state);
        self->field_0x0E0 +=
            (u16)(s32)(fn_800A8A04(((f32)self->interval * self->rate) - lbl_8079601C) * r);
    }
    if ((self->flags2 & 0x2000) != 0) {
        self->life = (f32)self->life_frames;
    } else {
        f32 step;
        if (self->scale_step == lbl_80796004) {
            step = self->scale_0x028.scale;
        } else {
            step = self->scale_0x028.scale +
                   self->scale_0x028.scale * self->scale_step *
                       (lbl_80796020 * fn_800A8A08(&self->random.state) - lbl_8079601C);
        }
        if ((self->flags2 & 0x100) != 0) {
            EfEffectData* effect = self->managerEF->effect;
            EfVec v;
            VEC3_ctor(&v);
            fn_800514FC(&v, fn_800A94A4(self, NULL), &self->position);
            step *= self->color_b +
                    (lbl_8079601C - self->color_b) *
                        fn_800A8220(&v, &effect->ref_pos, effect->range_a, effect->range_b,
                                    self->color_g, self->color_r);
        }
        self->life += step;
        if (self->transform_dirty != 0 && self->scale_0x028.scale != lbl_80796004 &&
            self->life < lbl_8079601C) {
            self->life = lbl_8079601C;
        }
    }
    if (self->life >= lbl_8079601C) {
        EfEmitterWork* work = EfGetWork(self->work);
        fn_800B2878();
        if (self->field_0x0F0 != NULL) {
            if (self->managerEF->field_0x48 != NULL) {
                s32 life_i = (s32)self->life;
                u32 f2 = self->flags2;
                f32 anim[6];
                u16 frames = work->field_0x0A;
                f32 scale = (f32)work->field_0x0C / lbl_80796000;
                nw4r::math::MTX34 mc;
                anim[0] = self->animate_0x4C[0];
                anim[1] = self->animate_0x4C[1];
                anim[2] = self->animate_0x4C[2];
                anim[3] = self->animate_0x58[0];
                anim[4] = self->animate_0x58[1];
                anim[5] = self->animate_0x58[2];
                fn_800A89A0(&mc, mtx);
                ((void (*)(EfEmitterObj*, EfParticleRec*, s32*, u32*, f32*, u16*, f32*,
                           nw4r::math::MTX34*))self->managerEF->field_0x48)(self, pm, &life_i, &f2,
                                                                        anim, &frames, &scale, &mc);
                ((void (*)(void*, EfEmitterObj*, EfParticleRec*, s32, u32, f32*, u16, f32,
                           nw4r::math::MTX34*))((void**)self->field_0x0F0)[2])(
                    self->field_0x0F0, self, pm, life_i, f2, anim, frames, scale, &mc);
            } else {
                ((void (*)(void*, EfEmitterObj*, EfParticleRec*, s32, u32, f32*, u16, f32, void*))(
                    (void**)self->field_0x0F0)[2])(self->field_0x0F0, self, pm, (s32)self->life,
                                                   self->flags2, self->animate_0x4C,
                                                   work->field_0x0A,
                                                   (f32)work->field_0x0C / lbl_80796000,
                                                   mtx);
            }
            {
                s32 a = 0;
                s32 b = 0;
                fn_800A8998(&a, 0);
                fn_800AEE0C(&b, pm);
                fn_800A898C(&a, &b);
                if ((*(u8*)((u8*)fn_800A8944(&a) + 3) & 0x20) != 0) {
                    fn_8035B998(self->managerEF->field_0x44);
                }
            }
        }
        self->life -= (f32)(s32)self->life;
    }
    if (self->transform_dirty != 0) {
        self->transform_dirty = 0;
    }
}

#pragma fp_contract reset

/* -------------------------------------------------------------------------------------------------
 * 0x800A8944 / 0x800A8968 - the pooled-effect accessors.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A8944(void* self) {
    return fn_800A8968((EfEmitterObj*)fn_800A5484(self));
}

extern "C" void* fn_800A8968(EfEmitterObj* self) {
    return (u8*)fn_800A4864(self) + 0x8C;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A898C / 0x800A8998 / 0x800A89A0 - small pooled-record helpers.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A898C(void* dst, const void* src) {
    *(u32*)dst = *(const u32*)src;
    return dst;
}

extern "C" void* fn_800A8998(void* dst, s32 v) {
    *(u32*)dst = (u32)v;
    return dst;
}

extern "C" void* fn_800A89A0(void* dst, const void* src) {
    u32* d = (u32*)dst;
    const u32* s = (const u32*)src;
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
    d[8] = s[8];
    d[9] = s[9];
    d[10] = s[10];
    d[11] = s[11];
    return dst;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A04 - the spawner's truncation (a one-argument runtime call).
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 fn_800A8A04(f32 x) {
    return fn_80463E2C(x);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A08 - the random block stepped and normalised to 0..1.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 fn_800A8A08(u32* random) {
    fn_800A6EA4(random);
    return (f32)(*random >> 16) / lbl_80796024;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A5C - the frame's relocation pass over this object's particle managers.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A8A5C(EfEmitterObj* self) {
    if (fn_800A8D18(self) != 0) {
        return;
    }
    if (self->state != 1) {
        return;
    }
    if (self->field_0x0B4 != 0) {
        return;
    }
    if (self->field_0x0DE != 0) {
        return;
    }
    {
        u32 flags = self->flags & 4;
        if (flags == 0) {
            if (self->field_0x0E4 >= self->life_max) {
                fn_800A486C(self->managerEF, self);
                return;
            }
        } else {
            if ((u32)(self->field_0x0E4 + 1) == 0x10000) {
                fn_800A486C(self->managerEF, self);
                return;
            }
        }
        s32 range = flags != 0 ? -1 : (s32)self->life_max;
        s32 found = 0;
        u16 start = self->field_0x0E4 != 0 ? fn_800A8CE8(self->work) : 0;
        u16 count = fn_800A8BC8(self->work);
        for (u16 i = start; i < count; i++) {
            EfTrack* rec = (EfTrack*)fn_800A8C34(self->work, i);
            if ((rec->flags & 8) != 0) {
                continue;
            }
            if ((u8)(rec->field_0x00 + 0x55) > 1) {
                continue;
            }
            u8 off = rec->offset;
            fn_8009F85C(rec, (u8*)self + off + 0x20, self->field_0x0E4, self->seed, range);
            if (off >= 0x70) {
                found = 1;
            }
        }
        if (found != 0) {
            fn_800A95D8(self);
        }
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8BC8 / 0x800A8BF8 / 0x800A8C24 / 0x800A8C34 / 0x800A8CB8 / 0x800A8CE8 - the
 * emitter-resource track walkers.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u16 fn_800A8BC8(void* res) {
    u8* p = (u8*)fn_800A8BF8(res);
    return *(u16*)(p + (*(u16*)p) * 8 + 4);
}

extern "C" void* fn_800A8BF8(void* res) {
    EfChainHeader* h = (EfChainHeader*)fn_800A8C24(res);
    return (u8*)h + h->offset + sizeof(EfChainHeader);
}

extern "C" void* fn_800A8C24(void* res) {
    EfResHeader* h = (EfResHeader*)res;
    return (u8*)h + h->offset + sizeof(EfResHeader);
}

extern "C" void* fn_800A8C34(void* res, u16 index) {
    void* array = fn_800A8CB8(res);
#line 736
    if ((u32)index >= (u32)fn_800A8BC8(res)) {
        nw4r::db::Panic(lbl_80592CC0, __LINE__, lbl_80592C94);
    }
    return *(void**)((u8*)array + (u32)index * 4);
}

extern "C" void* fn_800A8CB8(void* res) {
    u8* p = (u8*)fn_800A8BF8(res);
    return p + (*(u16*)p) * 8 + 8;
}

extern "C" u16 fn_800A8CE8(void* res) {
    u8* p = (u8*)fn_800A8BF8(res);
    return *(u16*)(p + (*(u16*)p) * 8 + 6);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8D18 - this object's "freed" flag.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800A8D18(EfEmitterObj* self) {
    return (self->flags & 0x200) != 0;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8D30 - the table's sweep slot: call slot 6 of every manager on the active list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A8D30(EfEmitterObj* self) {
    if (fn_800A8D18(self) != 0) {
        return;
    }
#line 973
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, lbl_80592860);
    u16 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        EfHost* pm = (EfHost*)list[i];
        ((void (*)(EfHost*))pm->vtable->slots[6])(pm);
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8DF8 - the table's per-frame manager tick.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A8DF8(EfEmitterObj* self) {
    if (fn_800A8D18(self) != 0) {
        return;
    }
    if (self->field_0x0B4 != 0) {
        return;
    }
    self->field_0x0B4 = 1;
    if (self->field_0x0E8 != 0) {
        fn_800A5114(self->managerEF, 1);
    }
    if (self->state == 1) {
        EfParticleRec* pm = (EfParticleRec*)fn_800A5250(&self->particles);
        nw4r::math::MTX34 m1, m2;
        MTX34_ctor(&m1);
        MTX34_ctor(&m2);
        fn_800AE360((void*)pm->manager, &m2);
        mtx34_inverse(&m2, &m2);
        fn_800A94A4(self, &m1);
        fn_800710BC(&m1, &m2, &m1);
        if (self->field_0x0DE != 0) {
            self->field_0x0DE--;
            return;
        }
        fn_800A834C(self, pm, &m1);
        self->field_0x0E4++;
    } else {
        if (self->field_0x0DE != 0) {
            self->field_0x0DE--;
            return;
        }
        self->field_0x0E4++;
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8F18 - the table's rotation-from-parent slot.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A8F18(EfEmitterObj* self) {
    if (self->state != 1 && self->state != 2) {
        return;
    }
    if ((self->flags2 & 0x10000) == 0 && (self->flags2 & 0x8000) == 0) {
        return;
    }
    {
        nw4r::math::MTX34 m1, m2, m3, m4;
        MTX34_ctor(&m1);
        self->rotation.x = lbl_80796004;
        self->rotation.y = lbl_80796004;
        self->rotation.z = lbl_80796004;
        self->transform_dirty = 1;
        fn_800A94A4(self, &m1);
        MTX34_ctor(&m2);
        fn_800710BC(&m2, &self->managerEF->effect->ref_mtx, &m1);
        fn_800A89A0(&m3, &m2);
        m3.m[0][0] = fn_8009CD64((const f32*)&m3, 0);
        m3.m[2][0] = lbl_80796004;
        m3.m[1][0] = lbl_80796004;
        m3.m[1][1] = fn_8009CD64((const f32*)&m3, 1);
        m3.m[2][1] = lbl_80796004;
        m3.m[0][1] = lbl_80796004;
        m3.m[2][2] = fn_8009CD64((const f32*)&m3, 2);
        m3.m[1][2] = lbl_80796004;
        m3.m[0][2] = lbl_80796004;
        if ((self->flags2 & 0x10000) != 0) {
            MTX34_ctor(&m4);
            fn_8009CA30(&m4, lbl_80796028, lbl_80796004, lbl_80796004);
            fn_800710BC(&m3, &m3, &m4);
        }
        mtx34_inverse(&m2, &m2);
        fn_800710BC(&m2, &m2, &m3);
        fn_8009CCAC(&m2, &m2, &self->rotation);
        fn_8009CC20(&m2, &self->vec_0x9C, &m2);
        fn_8009BCB4(&m2, &self->rotation);
        fn_800A95D8(self);
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A90AC - the transform combine the parent chain uses.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A90AC(void* dst, void* orig, u32 a, u32 b, s8 c, u32 d) {
#line 1124
    NW4R_POINTER_ASSERT(lbl_80592850, dst, lbl_80592AA8);
#line 1125
    NW4R_POINTER_ASSERT(lbl_80592850, orig, lbl_80592AE0);
    if (dst == orig) {
        nw4r::db::Panic(lbl_80592850, 0x466, lbl_80592B14);
    }
    if (a != 0 && b != 0 && c == 100) {
        fn_800532DC((Mtx34*)dst, (Mtx34*)orig);
        return dst;
    }
    if (a == 0 && b == 0 && c == 0) {
        mtx34_identity(dst);
        return dst;
    }
    mtx34_identity(dst);
    {
        EfVec offset;
        EfVec pt;
        nw4r::math::MTX34 m;
        VEC3_ctor(&offset);
        fn_8009BF08(orig, &offset);
        if (c == 100) {
            fn_80501390(dst, dst, &offset);
        } else if (c != 0) {
            VEC3_ctor(&pt);
            fn_80051424(&pt, &offset, (f32)c / lbl_80796000);
            fn_80501390(dst, dst, &pt);
        }
        if (b != 0 || a != 0) {
            if (d != 0 && c != 100) {
                fn_80501390(dst, dst, &offset);
            }
            if (b != 0) {
                MTX34_ctor(&m);
                fn_8009B650(orig, &m);
                fn_800710BC(dst, dst, &m);
            }
            if (a != 0) {
                VEC3_ctor(&pt);
                fn_8009C040(orig, &pt);
                fn_8009CBA0(dst, dst, &pt);
            }
            if (d != 0 && c != 100) {
                VEC3_ctor(&pt);
                fn_80051424(&pt, &offset, lbl_80796018);
                fn_80501390(dst, dst, &pt);
            }
        }
    }
    return dst;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A94A4 - this object's transform, built on demand.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A94A4(EfEmitterObj* self, void* out) {
    if (self->transform_dirty != 0) {
        nw4r::math::MTX34 m;
        MTX34_ctor(&m);
        if (self->parent == NULL) {
            fn_8007100C(&self->matrix, fn_800A60C0(self->managerEF));
        } else {
            nw4r::math::MTX34 p;
            MTX34_ctor(&p);
            fn_800A94A4(self->parent, &p);
            fn_800A90AC(&self->matrix, &p, self->flags3 & 1, self->flags3 & 2, (s8)self->alpha,
                        self->flags3 & 4);
        }
        fn_80501390(&self->matrix, &self->matrix, &self->position);
        fn_8009CA30(&m, self->rotation.x, self->rotation.y, self->rotation.z);
        fn_800710BC(&self->matrix, &self->matrix, &m);
        fn_8009CBA0(&self->matrix, &self->matrix, &self->vec_0x9C);
        self->transform_dirty = 0;
    }
    if (out != NULL) {
        fn_800532DC((Mtx34*)out, &self->matrix);
        return out;
    }
    return &self->matrix;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A95D8 - mark this object and its descendants transform-dirty.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A95D8(EfEmitterObj* self) {
    self->transform_dirty = 1;
    {
        void* node = NULL;
        while ((node = fn_80501C60(&self->particles, node)) != NULL) {
            fn_800A96C0((EfPmView*)node);
        }
    }
    if (self->managerEF != NULL) {
        EfEmitterObj* e = NULL;
        while ((e = (EfEmitterObj*)fn_80501C60(&self->managerEF->emitters, e)) != NULL) {
            if (e->transform_dirty != 0) {
                continue;
            }
            EfEmitterObj* p = e->parent;
            while (p != NULL) {
                if (p == self) {
                    e->transform_dirty = 1;
                    {
                        void* node = NULL;
                        while ((node = fn_80501C60(&e->particles, node)) != NULL) {
                            fn_800A96C0((EfPmView*)node);
                        }
                    }
                }
                p = p->parent;
            }
        }
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A96C0 - mark a manager's transform dirty.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A96C0(EfPmView* pm) {
    pm->dirty = 1;
    if ((s32)fn_800A9700(pm) == lbl_80794920) {
        lbl_80794920 = -1;
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A9700 / 0x800A970C / 0x800A9714 / 0x800A9784 / 0x800A9790 / 0x800A98D4 - the accessor block.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u16 fn_800A9700(EfPmView* pm) {
    return (u16)pm->field_0x08;
}

extern "C" u16 fn_800A970C(EfEmitterObj* self) {
    return fn_800A4AF0(&self->particles);
}

extern "C" void* fn_800A9714(EfEmitterObj* self, u16 index) {
#line 1284
    if ((u32)index >= (u32)fn_800A970C(self)) {
        nw4r::db::Panic(lbl_80592850, __LINE__, lbl_80592B3C);
    }
    return fn_80501C9C(&self->particles, index);
}

extern "C" void fn_800A9784(EfEmitterObj* self, void* cb, void* arg, s32 flag) {
    fn_800A52E4(self->managerEF, cb, arg, flag, self);
}

extern "C" void fn_800A9790(EfEmitterObj* self, EfWalkCtx* arg) {
#line 1305
    NW4R_POINTER_ASSERT(lbl_80592850, self, lbl_80592B70);
    arg->count += fn_800A98D4(self, arg->cb, arg->arg, arg->flag, 0);
}

extern "C" s32 fn_800A98D4(EfEmitterObj* self, void* cb, void* arg, s32 flag, s32 recurse) {
    s32 count = 0;
    void* node = fn_800A5250(&self->particles);
    while (node != NULL) {
        void* next = fn_80501C60(&self->particles, node);
        if (flag != 0 || fn_800A5248(node) == 1) {
            ((void (*)(void*, void*))cb)(node, arg);
            count++;
        }
        node = next;
    }
    if (recurse != 0) {
        EfWalkCtx ctx;
        ctx.count = 0;
        ctx.cb = cb;
        ctx.arg = arg;
        ctx.flag = (u8)flag;
        fn_800A9784(self, (void*)&fn_800A9790, &ctx, 1);
        count += ctx.count;
    }
    return count;
}
