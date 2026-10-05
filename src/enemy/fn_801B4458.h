/* enemy/fn_801B4458.h - the enemy seat/effect-action band 0x801B4458..0x801B7020 (48 functions).
 *
 * The unit owns the `.text` range 0x801B4458..0x801B7020, the extab run 0x8000F6B4..0x8000F7EC
 * (39 records), the extabindex run 0x8002B0F8..0x8002B2CC (39 records) and one `.ctors` word at
 * 0x8056F348 (the static initializer `fn_801B6FB0`).  It is the unclaimed gap between
 * `enemy/fn_801B0010.cpp` (the em030 program unit, ending at 0x801B4458) and
 * `enemy/fn_801B7020.cpp` (starting at 0x801B7020).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` on every address of the 48-function inventory: every one
 * answers `zz_<addr>_`, so the dump is not evidence; `config/RMHE08/symbols.txt` carries nothing but
 * the bare `fn_XXXXXXXX` entries and the range references no `__FILE__`/class-name string).
 *
 * The 0x20-byte seat record the em030 unit's `fn_801B4398` hands back (through its third argument)
 * and this band walks record by record.  `code` is the seat's kind (1 = a two-point seat whose
 * second point is `vec_0x10`, anything else a single point), `vec_0x04`/`vec_0x10` are engine
 * vectors (`vec_to_mh_vec3` converts them) and `value_0x1C` is the seat's own float parameter.
 * size: 0x20 */
struct EmSeatRec {
    /* +0x00 */ u8 code;
    /* +0x01 */ u8 unused_0x01[3];
    /* +0x04 */ f32 vec_0x04[3];
    /* +0x10 */ f32 vec_0x10[3];
    /* +0x1C */ f32 value_0x1C;
};

#ifndef MHTRI_ENEMY_FN_801B4458_H
#define MHTRI_ENEMY_FN_801B4458_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* The engine 3-float vector `vec_to_mh_vec3` converts (only forward-declared here: the definition
 * lives in `ef.h`, and a `Vec*` is not a `nw4r::math::VEC3*`). */
struct Vec;

#ifdef __cplusplus
extern "C" {
#endif

/* The band's own flat symbols, in address order. */
u8 fn_801B4458(struct _ENEMY_WORK* self, u8 kind);
s32 fn_801B45B0(void* point, void* seat, f32 radius);
void fn_801B4694(struct _ENEMY_WORK* self, u8 seat);
void fn_801B47A4(struct _ENEMY_WORK* self);
s32 fn_801B4C54(u16 id);
void fn_801B4D14(struct _ENEMY_WORK* self, u8 kind);
void fn_801B4E38(void);
void fn_801B4E3C(struct _ENEMY_WORK* self);
void fn_801B4EA8(struct _ENEMY_WORK* self);
void fn_801B4F08(struct _ENEMY_WORK* self);
void fn_801B4F84(struct _ENEMY_WORK* self);
void fn_801B5000(struct _ENEMY_WORK* self);
void fn_801B5030(struct _ENEMY_WORK* self);
void fn_801B5108(struct _ENEMY_WORK* self);
void fn_801B5184(struct _ENEMY_WORK* self);
void fn_801B5200(struct _ENEMY_WORK* self);
void fn_801B529C(struct _ENEMY_WORK* self);
void fn_801B5338(struct _ENEMY_WORK* self);
void fn_801B53E8(struct _ENEMY_WORK* self);
void fn_801B5464(struct _ENEMY_WORK* self);
void fn_801B54E0(struct _ENEMY_WORK* self);
void fn_801B555C(struct _ENEMY_WORK* self);
void fn_801B55A8(struct _ENEMY_WORK* self);
void fn_801B563C(struct _ENEMY_WORK* self, u8 kind);
void fn_801B5774(struct _ENEMY_WORK* self, u8 kind, u8 flag);
void fn_801B5908(struct _ENEMY_WORK* self, u8 kind, u8 count);
void fn_801B5A08(struct _ENEMY_WORK* self, u8 flag);
void fn_801B5AE4(struct _ENEMY_WORK* self);
void fn_801B5B60(struct _ENEMY_WORK* self);
void fn_801B5BEC(struct _ENEMY_WORK* self);
void fn_801B5D24(struct _ENEMY_WORK* self, u8 flag);
void fn_801B5E20(struct _ENEMY_WORK* self);
void fn_801B5ED4(struct _ENEMY_WORK* self, u8 flag);
void fn_801B6010(struct _ENEMY_WORK* self);
void fn_801B60D4(struct _ENEMY_WORK* self);
void fn_801B610C(struct _ENEMY_WORK* self);
void fn_801B6308(struct _ENEMY_WORK* self);
void fn_801B63E8(struct _ENEMY_WORK* self);
void fn_801B64E8(struct _ENEMY_WORK* self);
void fn_801B64FC(struct _ENEMY_WORK* self);
void fn_801B65B8(struct _ENEMY_WORK* self, u8 kind, u8 id, s32 joint, s32 arg4, f32 scale);
void fn_801B670C(struct _ENEMY_WORK* self);
void fn_801B6B94(struct _ENEMY_WORK* self);
s32 fn_801B6C38(struct _ENEMY_WORK* self, u8 flag);
void fn_801B6C84(struct _ENEMY_WORK* self, s8* out_state, s8* out_flag);
void fn_801B6EF4(struct _ENEMY_WORK* self, u8 kind);
void fn_801B6FB0(void);
u32 fn_801B701C(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801B4458_H */
