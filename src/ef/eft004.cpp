/* auto/800FF8D4_fn_800FF8D4.cpp - the `eft004` effect cluster, 0x800FF8D4..0x80101DF4 (37 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * The game's `eft004` effect module: the per-frame handlers (`fn_800FF8D4`, `fn_800FFCAC`,
 * `fn_80100088`, ...), the effect-record state-machine dispatchers (`fn_80100A6C`, `fn_80101774`,
 * `fn_80101DB8`), the pool release helpers (`fn_80100A30`, `fn_80101D70`), the spawn helper
 * `fn_801007BC` and the three public setters `eft004_set` / `eft004_set_pl` / `eft004_set_pl2`.
 * 29 of the 37 symbols are recovered; the eight not yet written are the large handlers
 * (`fn_800FF8D4` 964 B, `fn_800FFCAC` 604 B, `fn_80100088` 632 B, `fn_801007BC` 628 B,
 * `fn_80100AA8` 1804 B, `fn_801011B4` 600 B, `fn_801017B0` 464 B, `fn_80101980` 736 B).
 *
 * Flags: the whole file is compiled with the peephole pass off (`#pragma peephole off`, playbook 39) -
 * the retail object keeps the unfused `rlwinm` + `cmpwi` where the pass emits a record-form `rlwinm.`,
 * and the function pointer through `r0`. It took `fn_80101594` 97.73 -> 100, `fn_80101670` 95.6 -> 100,
 * `eft004_set` 75.5 -> 92.86, `fn_801006A0` 80.1 -> 94.29 and `fn_8010072C` 86.1 -> 94.44.
 *
 * Residuals (measured with `python tools/units/recompile.py auto/800FF8D4_fn_800FF8D4 --measure <sym>`):
 *   * the eight unwritten handlers above are 0 %.
 *   * `fn_80101470` 81.84 % - the two nested type switches reproduce the target's jump tables and case
 *     bodies, but MWCC lays the state-0 block and the state-2/3 block out in the other order and folds
 *     the `state == 1` return into a `bnelr` (target: `cmpwi r5,1; beqlr; blr`).
 *   * `eft004_set` / `fn_801006A0` / `fn_8010072C` / `eft004_set_pl2` 92.86-96.97 % - the spawn
 *     parameter block's address (`addi r7,r1,8`) is scheduled three instructions before the call where
 *     retail has it after the `lfs`/`fmr` constant setup; the code is otherwise identical.
 *   * `fn_800FFF08` 93.75 % - `self->work_0x38` and `&eft_control` swap `r3`/`r5`.
 *   * `fn_80100330` 96.29 % - the `IsValidPointer` inline colours the tested value in `r4` where retail
 *     uses `r6`.
 *   * `fn_80101C74` 96.51 % - the loop keeps `pool->entries[i]` in a register where retail reloads it.
 *
 * Load-bearing source shapes:
 *   * `fn_80100A6C` / `fn_80101774` / `fn_80101DB8` are four-case dispatchers whose cases `return` (not
 *     `break`): a `break` emits a shared trailing branch retail does not have.
 *   * `fn_80100A30` / `fn_80101738` and `fn_80101D70` read the same `self->work_0x38` pool through two
 *     layouts (`EftEffectPool`, `EftHeapPool`); the pool is `void*` on `Eft004` and cast per family.
 *   * `fn_80101470`'s outer `switch (state)` lists `case 2` and `case 3` together and has no `case 1`.
 *   * the setter family passes the spawning owner (`_PLW*`) as `fn_801007BC`'s first argument in
 *     `eft004_set_pl`/`eft004_set_pl2` and `NULL` in `eft004_set`/`fn_801006A0`/`fn_8010072C`.
 *
 * Types: `Eft004`, `_PLW`, the three pool views, `EftEmitter`, `Eft004Owner` and `EftControl` are
 * reconstructed minimally (only the offsets this unit reads); `nw4r::math::MTX34` moved to
 * `include/nw4r/math.h`. `EftControl` is sized 0xC44 (from symbols.txt) so MWCC emits the far
 * `lis`/`addi` address retail has instead of an `@sda21` load.
 *
 * The unit owns no data section: its literals and jump tables live in the shared `.data`/`.sdata2` run
 * (`0x8059BF90..`, `0x807966A8..`), referenced by name only. Evidence for the attribution:
 * `.pi/attribution-batch-4.patch.md` and `.pi/notes/attribution-batch-4.md`; inventory:
 * `python tools/units/ledger.py unit auto/800FF8D4_fn_800FF8D4.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/fn_80101DF4.h"
#include "ef/eft007.h"
#include "sound/fn_800D7F54.h"

/* The retail object keeps the unfused forms of several peephole folds (a `rlwinm` + `cmpwi` where the
 * pass would emit a record-form `rlwinm.`, a function pointer through `r0`); the whole file is compiled
 * with the pass off (playbook 39). */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * nw4r declarations the map's mangled names encode
 * ------------------------------------------------------------------------------------------------- */

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)`. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace db

namespace nw4r {
namespace ef {

/* `nw4r::ef::Effect` comes from its owner `ef.h` (rule 1) and already carries
 * `RetireEmitterAll`. */

/* `nw4r::ef::EffectSystem` now lives in its owner `ef.h` (rule 1: one definition - the
 * resource manager and this per-frame handler both need it).  `RetireEffect` is direct and
 * `virtual_0x0C` is the fourth vtable slot reached through `fn_800A4420`. */

}  // namespace ef
}  // namespace ef

/* ---------------------------------------------------------------------------------------------------
 * the effect record and the player work the handlers take
 * ------------------------------------------------------------------------------------------------- */

/* The 0x48-byte game effect record. `work_0x38` is the family-specific pool block; each function casts
 * it to the pool layout it owns. size: 0x48 */
struct Eft004 {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 phase_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 flag_0x07;
    /* +0x08 */ u8 byte_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* owner_0x30;
    /* +0x34 */ void (*dispatch_0x34)(Eft004*);
    /* +0x38 */ void* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(Eft004*);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* The player work the public setters and the effect spawners take; only the offsets this unit reads are
 * named. size: 0x668 */
struct _PLW {
    /* +0x000 */ u8 unused_0x000[0x16];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 unused_0x017[0x3C - 0x17];
    /* +0x03C */ nw4r::math::VEC3 pos_0x3C;
    /* +0x048 */ u8 unused_0x048[0x54 - 0x48];
    /* +0x054 */ u32 param_0x54;
    /* +0x058 */ u32 param_0x58;
    /* +0x05C */ u32 param_0x5C;
    /* +0x060 */ u8 unused_0x060[0x1A4 - 0x60];
    /* +0x1A4 */ u8 effect_key_0x1A4;
    /* +0x1A5 */ u8 unused_0x1A5[0x5A4 - 0x1A5];
    /* +0x5A4 */ u16 field_0x5A4;
    /* +0x5A6 */ u8 unused_0x5A6[0x655 - 0x5A6];
    /* +0x655 */ u8 field_0x655;
    /* +0x656 */ u8 unused_0x656[0x668 - 0x656];
};

/* The pool block the `push_eft_effect_heap_num` release helpers walk: a count followed by the effect
 * handles. size: 0x08 - lower bound, an approximation (the pool continues past what this unit reads) */
struct EftEffectPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
};

/* The pool block `fn_80101670` seeds: the count and a scale. size: 0x0C - lower bound, an approximation */
struct EftScaledPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ f32 scale_0x08;
};

/* The pool block `fn_80101C74` fills: a count, twelve heap handles at +0x10, one more at +0x40 and a
 * field at +0x5C. size: 0x60 - lower bound, an approximation */
struct EftHeapPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x10 - 0x04];
    /* +0x10 */ void* entries[12];
    /* +0x40 */ void* handle_0x40;
    /* +0x44 */ u8 unused_0x44[0x5C - 0x44];
    /* +0x5C */ s32 field_0x5C;
};

/* The emitter `fn_800A51C8` answers for; only the +0x90 position this unit copies is named.
 * size: 0x9C - lower bound, an approximation */
struct EftEmitter {
    /* +0x00 */ u8 unused_0x00[0x90];
    /* +0x90 */ nw4r::math::VEC3 pos_0x90;
};

/* The effect's owner object; only the +0x498 sound handle this unit reads is named.
 * size: 0x49C - lower bound, an approximation */
struct Eft004Owner {
    /* +0x000 */ u8 unused_0x000[0x498];
    /* +0x498 */ s32 handle_0x498;
};

/* The global effect control block; only the system pointer this unit retires through is named. */
struct EftControl {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ nw4r::ef::EffectSystem* system_0x04;
    /* +0x008 */ u8 unused_0x008[0xC44 - 0x08];
};
/* size: 0xC44 */


/* ---------------------------------------------------------------------------------------------------
 * externs
 * ------------------------------------------------------------------------------------------------- */

extern "C" EftControl eft_control;

extern "C" void fn_800F886C(void* self);
extern "C" void fn_800F8A44(void* block, s32 count);
extern "C" void* fn_800F8914();
extern "C" Eft004* fn_800F8788(u32 pool_id);
extern "C" void fn_800F9DF4(Eft004* self, s32 a, s32 b);
extern "C" Eft004* fn_801007BC(void* owner, u32 arg1, u32 arg2, u32 arg3, s32* arg4,
                               f32 farg0, f32 farg1, f32 farg2);
extern "C" u32 fn_80100330(u32* p);
extern "C" void* fn_800A485C(u32 color);
extern "C" void fn_80041E40(void* dst, const void* src);
extern "C" void* fn_800A60C0(void* self);
extern "C" void fn_800A4AF8(nw4r::ef::Effect* effect);
extern "C" nw4r::ef::EffectSystem* fn_800A4420(nw4r::ef::EffectSystem* system);
extern "C" EftEmitter* fn_800A51C8(void* self);
extern "C" u8 fn_800CF208(Eft004* self);
extern "C" f32 lbl_807966B8; /* 0.0f  .sdata2 */
extern "C" void fn_80100024(EftEmitter* self, nw4r::math::VEC3* out);
extern "C" void fn_80100AA8(Eft004* self);
extern "C" void fn_801011B4(Eft004* self);
extern "C" void fn_8010145C(Eft004* self);
extern "C" void fn_8010146C(Eft004* self);
extern "C" void fn_801017B0(Eft004* self);
extern "C" void fn_80101980(Eft004* self);
extern "C" void fn_80101C60(Eft004* self);
extern "C" void fn_80101C70(Eft004* self);
/* fn_80101DF4 / fn_80101FA4 / fn_801025E8 / fn_801025F8 come from their owners' headers (rule 2). */

extern "C" u8 get_now_areano();

/* `push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count)` - the map's mangled spelling. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* `map_se_req(u8, nw4r::math::VEC3*)` - the map's mangled spelling. */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

/* The unit's own assert strings, referenced but not emitted (the pool lives in another unit). */
extern "C" const char lbl_8059C4D0[]; /* "emitter.h"                                            .data */
extern "C" const char lbl_8059C4AC[]; /* "NW4R:Failed assertion trans != NULL"                 .data */
extern "C" const char lbl_8059C510[]; /* "res_emitter_ac.h"                                    .data */
extern "C" const char lbl_8059C4DC[]; /* "NW4R:Pointer Error\nmData(=%p) is not valid pointer." .data */

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Bumps the effect's state index. */
extern "C" void fn_800FFC98(Eft004* self)
{
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_800FFCA8(Eft004* self)
{
    fn_800F886C(self);
}

/* Copies one 32-bit value. */
extern "C" void fn_80100300(u32* dst, const u32* src)
{
    *dst = *src;
}

/* Returns the emitter the color is resolved through. */
extern "C" void* fn_8010030C(void* p)
{
    return fn_800A485C(fn_80100330((u32*)p));
}

/* Asserts that the value is a valid console pointer and returns it. */
extern "C" u32 fn_80100330(u32* p)
{
    if (!IsValidPointer(*p)) {
        nw4r::db::Panic(lbl_8059C510, 102, lbl_8059C4DC);
    }
    return *p;
}

/* Copies a matrix's translation column into a vector. */
extern "C" void fn_8010140C(nw4r::math::MTX34* mtx, nw4r::math::VEC3* out)
{
    out->x = mtx->m[0][3];
    out->y = mtx->m[1][3];
    out->z = mtx->m[2][3];
}

/* Adds a vector to a matrix's translation column. */
extern "C" void fn_80101428(nw4r::math::MTX34* mtx, nw4r::math::VEC3* v)
{
    mtx->m[0][3] += v->x;
    mtx->m[1][3] += v->y;
    mtx->m[2][3] += v->z;
}

/* Bumps the effect's state index. */
extern "C" void fn_8010145C(Eft004* self)
{
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_8010146C(Eft004* self)
{
    fn_800F886C(self);
}

/* Bumps the effect's state index. */
extern "C" void fn_80101C60(Eft004* self)
{
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_80101C70(Eft004* self)
{
    fn_800F886C(self);
}

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80100A30(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80101738(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Releases the pool's twelve heap handles and its single extra handle, then clears its count. */
extern "C" void fn_80101D70(Eft004* self)
{
    EftHeapPool* pool = (EftHeapPool*)self->work_0x38;

    fn_800F8A44(pool->entries, pool->count);
    fn_800F8A44(&pool->handle_0x40, 1);
    pool->count = 0;
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80100A6C(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80100AA8(self);
        return;
    case 1:
        fn_801011B4(self);
        return;
    case 2:
        fn_8010145C(self);
        return;
    case 3:
        fn_8010146C(self);
        return;
    }
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80101774(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_801017B0(self);
        return;
    case 1:
        fn_80101980(self);
        return;
    case 2:
        fn_80101C60(self);
        return;
    case 3:
        fn_80101C70(self);
        return;
    }
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80101DB8(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80101DF4(self);
        return;
    case 1:
        fn_80101FA4(self);
        return;
    case 2:
        fn_801025E8(self);
        return;
    case 3:
        fn_801025F8(self);
        return;
    }
}

/* Copies the emitter's position out and, when the emitter is live, offsets it by the current view
 * matrix's translation. Answers whether the offset was applied. */
extern "C" s32 fn_800FFF98(void* arg0, nw4r::math::VEC3* out)
{
    EftEmitter* emitter = fn_800A51C8(arg0);

    if (emitter != NULL) {
        fn_80100024(emitter, out);
        nw4r::math::MTX34* mtx = (nw4r::math::MTX34*)fn_800A60C0(arg0);
        out->x += mtx->m[0][3];
        out->y += mtx->m[1][3];
        out->z += mtx->m[2][3];
        return 1;
    }
    return 0;
}

/* Asserts the destination vector, then copies the emitter's position into it. */
extern "C" void fn_80100024(EftEmitter* self, nw4r::math::VEC3* out)
{
    if (out == NULL) {
        nw4r::db::Panic(lbl_8059C4D0, 391, lbl_8059C4AC);
    }
    fn_80041E40(out, &self->pos_0x90);
}

/* Retires one live effect: runs the nw4r teardown, releases it from the system and clears its slot. */
extern "C" void fn_800FFF08(Eft004* self, u32 idx)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;
    nw4r::ef::EffectSystem* system = eft_control.system_0x04;
    nw4r::ef::Effect** slot = &pool->effects[idx];

    if (*slot != NULL) {
        fn_800A60C0(*slot);
        fn_800A4AF8(*slot);
        (*slot)->RetireEmitterAll();
        system->RetireEffect(*slot);
        fn_800A4420(system)->virtual_0x0C();
        *slot = NULL;
    }
}

/* Spawns an effect from a vector, then stamps its key byte. */
extern "C" void fn_801006A0(u8 type, nw4r::math::VEC3* pos, u32 param, u8 flag, u8 key, f32 scale)
{
    s32 params[3];
    Eft004* effect;

    params[0] = 0;
    params[1] = (s32)param;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, params, scale, lbl_807966B8, lbl_807966B8);
    if (effect != NULL) {
        fn_80041E40(&effect->pos_0x18, pos);
        effect->byte_0x08 = key;
    }
}

/* Spawns an effect the player owns from a vector, then stamps its key byte. */
extern "C" void fn_8010072C(_PLW* owner, u8 type, nw4r::math::VEC3* pos, u32 param, f32 scale)
{
    s32 params[3];
    Eft004* effect;

    params[0] = 0;
    params[1] = (s32)param;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, owner->effect_key_0x1A4, 255, params, scale,
                         lbl_807966B8, lbl_807966B8);
    if (effect != NULL) {
        effect->owner_0x30 = owner;
        fn_80041E40(&effect->pos_0x18, pos);
        effect->byte_0x08 = 2;
    }
}

/* Public setter: spawns an effect at a vector. */
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag)
{
    s32 params[3];
    Eft004* effect;
    u32 p = param;

    params[0] = 0;
    params[1] = (s32)p;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, params, scale, lbl_807966B8, lbl_807966B8);
    if (effect != NULL) {
        fn_80041E40(&effect->pos_0x18, pos);
    }
}

/* Public setter: spawns an effect the player owns, at the player's position plus an offset. */
void eft004_set_pl(_PLW* self, u8 type, f32 a, f32 b, f32 c, u32 param)
{
    s32 params[3];
    Eft004* effect;

    params[0] = self->param_0x54;
    params[1] = self->param_0x58 + param;
    params[2] = self->param_0x5C;
    effect = fn_801007BC(self, type, self->area_0x16, 255, params, a, b, c);
    if (effect != NULL) {
        fn_80041E40(&effect->pos_0x18, &self->pos_0x3C);
        if (type == 17 && (self->field_0x5A4 & 0x6) != 0) {
            eft004_set_pl(self, 2, a, b, c, param);
        }
    }
}

/* Public setter: spawns a player-owned effect with a caller-chosen parameter block. */
void eft004_set_pl2(_PLW* self, u8 type, u32 param, f32 a, f32 b, f32 c, u32 extra)
{
    nw4r::math::VEC3 dir;
    s32 params[3];
    Eft004* effect;

    fn_80043EA8(&dir);
    params[0] = self->param_0x54;
    params[1] = self->param_0x58 + extra;
    params[2] = self->param_0x5C;
    effect = fn_801007BC(self, type, self->area_0x16, param, params, a, b, c);
    if (effect != NULL) {
        fn_80041E40(&effect->pos_0x18, &self->pos_0x3C);
        if (type == 17 && (self->field_0x5A4 & 0x6) != 0) {
            eft004_set_pl2(self, 2, param, a, b, c, extra);
        }
    }
}

/* Creates a player-owned effect with a 1-effect pool, seeding its area and key byte. */
extern "C" void fn_80101594(_PLW* self)
{
    Eft004* effect = fn_800F8788(12);

    if (effect != NULL) {
        EftEffectPool* pool = (EftEffectPool*)effect->work_0x38;
        u8 key;

        pool->count = 1;
        effect->phase_0x03 = 5;
        effect->type_0x02 = 0;
        effect->timer_0x0C = 0;
        effect->field_0x10 = 20;
        effect->owner_0x30 = self;
        effect->area_0x44 = self->area_0x16;
        if (fn_800CF208(effect) == 1) {
            key = self->field_0x655;
            if ((key & 0x80) != 0) {
                effect->byte_0x08 = 0;
            } else {
                effect->byte_0x08 = key;
            }
        } else {
            effect->byte_0x08 = 0;
        }
        fn_800F9DF4(effect, 1, 0);
        effect->release_0x40 = fn_80101738;
        effect->dispatch_0x34 = fn_80101774;
    }
}

/* Creates a player-owned effect with a 1-effect pool, at a caller-chosen position and scale. */
extern "C" void fn_80101670(nw4r::math::VEC3* pos, u8 area, f32 scale)
{
    Eft004* effect = fn_800F8788(12);

    if (effect != NULL) {
        EftScaledPool* pool = (EftScaledPool*)effect->work_0x38;

        pool->count = 1;
        pool->scale_0x08 = scale;
        effect->phase_0x03 = 5;
        effect->type_0x02 = 1;
        effect->owner_0x30 = NULL;
        effect->area_0x44 = area;
        fn_80041E40(&effect->pos_0x18, pos);
        effect->timer_0x0C = 0;
        effect->field_0x10 = 0;
        fn_800F9DF4(effect, 1, 0);
        effect->release_0x40 = fn_80101738;
        effect->dispatch_0x34 = fn_80101774;
    }
}

/* Creates an effect with a twelve-entry heap pool and one extra handle; destroys it on any failure. */
extern "C" void fn_80101C74(u8 type)
{
    u8 area = get_now_areano();
    Eft004* effect = fn_800F8788(0x60);

    if (effect != NULL) {
        EftHeapPool* pool;
        s32 i;

        effect->type_0x02 = type;
        effect->release_0x40 = fn_80101D70;
        effect->dispatch_0x34 = fn_80101DB8;
        pool = (EftHeapPool*)effect->work_0x38;
        pool->count = 12;
        for (i = 0; i < pool->count; i++) {
            pool->entries[i] = fn_800F8914();
            if (pool->entries[i] == NULL) {
                fn_800F886C(effect);
                return;
            }
        }
        pool->field_0x5C = 0;
        pool->handle_0x40 = fn_800F8914();
        if (pool->handle_0x40 == NULL) {
            fn_800F886C(effect);
            return;
        }
        effect->phase_0x03 = 6;
        effect->area_0x44 = area;
        fn_800F9DF4(effect, 0, 0);
    }
}

/* Plays the footstep/voice sound the effect's state and type select. */
extern "C" void fn_80101470(Eft004* self)
{
    s32 state = self->byte_0x08;

    switch (state) {
    case 0:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            fn_800DA72C(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 32:
            fn_800DA72C(0, 20, &self->pos_0x18);
            break;
        case 35: case 46:
            fn_800DA72C(0, 37, &self->pos_0x18);
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        }
        break;
    case 2:
    case 3:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            fn_800DA72C(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 35:
            if (state == 2) {
                fn_800DA72C(0, 37, &self->pos_0x18);
            } else {
                fn_800DA72C(0, 19, &self->pos_0x18);
            }
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        case 42:
            fn_800DCF0C(((Eft004Owner*)self->owner_0x30)->handle_0x498, &self->pos_0x18);
            break;
        case 46:
            fn_800DA72C(0, 37, &self->pos_0x18);
            break;
        }
        break;
    }
}
