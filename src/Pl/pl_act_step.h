/* Declarations owned by `Pl/pl_act_step.cpp` (rule 2). */
#ifndef MHTRI_PL_ACT_STEP_H
#define MHTRI_PL_ACT_STEP_H

#include "types.h"

struct _PLW;
struct _se_w;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80257E70 - the three-timer reset of the guard act (GUESS name: the map had only the stem and the runtime dump a
 * placeholder); `extern "C"` because the map row is unmangled. */
void pl_act_guard_timer_reset(struct _PLW* self);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/Pl.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80245DA0 - the act tail's wait clear, which the act state-machine band calls.  `void`: the body ends without
 * setting r3 and the three call sites drop the result. */
void pl_act_clear_wait(struct _PLW* self, u8 a);

/* 0x80246654 - places the local player at an area entry (GUESS name). */
void pl_area_entry_set(nw4r::math::VEC3* pos, u16 angle, u8 map, u8 area);

/* 0x8025DF78 - the next (`backward` 0) or previous pouch slot after `cur` whose item is usable, or 0 when none is
 * (GUESS name).  `u8 backward`: the body narrows it with `clrlwi r0,r5,24`. */
u16 pl_item_slot_cycle(struct _PLW* self, u16 cur, u8 backward);

/* 0x80244E88 - arms the SE frames of the long weapon motions 0x139/0x164 (GUESS name); `Pl/fn_80229ECC.cpp` hands it
 * `&self->field_0xAF4`, the actor's three SE works. */
void pl_se_long_motion_frames(struct _se_w** seWork, u32 part, u32 motion, u32 kind);

#ifdef __cplusplus
}
#endif


/* The interior words of this unit's `.sdata2` pool its own bodies read (the rest are declared by
 * `Pl/pl_frame_data.h` and `Pl/pl_act_data.h`); declared, never defined (playbook 29). */
#ifdef __cplusplus
extern "C" {
#endif

extern const f32 pl_float_neg50000; /* 0x80799E04: -50000 */
extern const f32 pl_frame_window_55; /* 0x80799E10: 55 */
extern const f32 pl_frame_window_210; /* 0x80799E14: 210 */
extern const f32 pl_frame_window_48; /* 0x80799E28: 48 */
extern const f32 pl_frame_window_86; /* 0x80799E3C: 86 */
extern const f32 pl_frame_window_26; /* 0x80799E5C: 26 */
extern const f32 pl_frame_window_22; /* 0x80799E60: 22 */
extern const f32 pl_frame_window_76; /* 0x80799E6C: 76 */
extern const f32 pl_float_1_3; /* 0x80799E90: 1.3 */
extern const f32 pl_frame_window_294; /* 0x80799EA0: 294 */
extern const f32 pl_frame_window_130; /* 0x80799EA4: 130 */
extern const f32 pl_frame_window_62; /* 0x80799EB8: 62 */
extern const f32 pl_frame_window_38; /* 0x80799EBC: 38 */
extern const f32 pl_frame_window_70; /* 0x80799EC8: 70 */
extern const f32 pl_frame_window_999; /* 0x80799FDC: 999 */
extern const f32 pl_float_0_7; /* 0x80799FE8: 0.7 */
extern const f32 pl_float_neg152; /* 0x80799FEC: -152 */
extern const f32 pl_frame_window_2048; /* 0x80799FF0: 2048 */
extern const f32 pl_frame_window_181; /* 0x80799FF4: 181 */
extern const f32 pl_frame_window_2051; /* 0x80799FF8: 2051 */

#ifdef __cplusplus
}
#endif

/* The player's two attack entries at `_PLW`+0x484 and +0x4E0 sit 0x5C apart: each is the head of a `_HIT_W`
 * (`Pl/hit_w.h`) whose last four bytes the next entry overlaps.  This view names the two bytes the act reset
 * writes in each.  size: 0x5C */
typedef struct PlHitSlot {
    /* +0x00 */ u8 pad_0x00[0x5];
    /* +0x05 */ u8 active_0x05;    /* `_HIT_W::active_0x05` */
    /* +0x06 */ u8 pad_0x06[0x8];
    /* +0x0E */ u8 result_0x0E;    /* `_HIT_W::result`; 0xFF is "no result" */
    /* +0x0F */ u8 pad_0x0F[0x4D];
} PlHitSlot; /* size: 0x5C */

#endif /* MHTRI_PL_ACT_STEP_H */
