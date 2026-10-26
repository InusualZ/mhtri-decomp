/* `Pl/fn_802840DC.cpp`'s view of the per-act handlers it tail-calls in `Pl/pl_act.cpp`'s equipment section
 * (0x8027D684-0x802840DC), with the widths its own call sites use; they belong in `Pl/pl_act.h` (rule 2).
 */
#ifndef MHTRI_PL_FN_802840DC_H
#define MHTRI_PL_FN_802840DC_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The per-act handlers `pl_weapon1_act_dispatch` tail-calls out of its `act_no` switch (cases 0-34; the two
 * families are the ones carrying an index argument and the ones that take only the actor).  The
 * call site's own register width is all there is to go on: the index is `li`-materialised (0-4) and
 * the actor is the same `_PLW` every other Pl entry takes. */
void pl_weapon1_act_combo(struct _PLW* self, u32 arg);
void pl_weapon1_act_shell_swing(struct _PLW* self, u32 arg);
void pl_weapon1_act_double(struct _PLW* self, u32 arg);
void pl_weapon1_act_turn_swing(struct _PLW* self, u32 arg);
void pl_weapon1_act_swing2(struct _PLW* self, u32 arg);
void pl_weapon1_act_chain(struct _PLW* self);
void pl_weapon1_act_single(struct _PLW* self);
void pl_weapon1_act_combo3(struct _PLW* self, u32 arg);
void pl_weapon1_act_rise(struct _PLW* self, u32 arg);
void pl_weapon1_act_double3(struct _PLW* self, u32 arg);
void pl_weapon1_act_single3(struct _PLW* self);
void pl_weapon1_act_finisher3(struct _PLW* self, u32 arg);
void pl_weapon1_act_chain3(struct _PLW* self);
void pl_weapon1_act_kick3(struct _PLW* self);
void pl_weapon1_act_kick(struct _PLW* self);
void pl_weapon1_act_turn3(struct _PLW* self, u32 arg);

/* One row of a class-7 handler's motion-row table (`pl_act_class7_handler_04_rows` and its siblings): a plain
 * motion with its two set-up values, then the motion the step table drives with its two values and the attack
 * entry it arms.  size: 0xE (the handlers index the table with `mulli r6,r31,14`; every field is 2-byte
 * aligned). */
typedef struct PlMotionRow {
    /* +0x0 */ u16 motion;       /* 0xFFFF: no plain motion, the row goes straight to its step half */
    /* +0x2 */ s16 motion_a;
    /* +0x4 */ s16 motion_b;
    /* +0x6 */ u16 step_motion;
    /* +0x8 */ s16 step_motion_a;
    /* +0xA */ s16 step_motion_b;
    /* +0xC */ s16 attack;       /* the attack value `pl_act_attack_set` arms (0: none) */
} PlMotionRow;

/* 0x80288B98 (0x94): builds the actor's own attack entry from a signed attack value; `kind` picks the flag pair. */
void pl_act_attack_set(struct _PLW* self, s16 value, u8 kind);
/* 0x80288C2C (0xC0): applies one motion row: its plain motion, or (`mode` 1, or a row without one) its step motion
 * with `step_table` and its attack entry; `mode` 0 on the step half also advances the act step byte (+0x05). */
void pl_act_motion_row_apply(struct _PLW* self, u8 mode, const PlMotionRow* row, u32 step_table);

/* 0x80284474 - the player's charge stage for the second weapon class (`ef/eft029_fx.cpp` reads it). */
u8 pl_charge_stage2_get(struct _PLW* plw);

/* 0x805CAD48: the step table the guard-break act hands Pl_act_set_step_table for weapon kind 2 */
extern u32 pl_flinch_steps_kind2[];
void pl_weapon1_act_dispatch(struct _PLW* self);
/* 0x80286DF8: the class-2 weapon act dispatcher */
void pl_weapon2_act_dispatch(struct _PLW* self);
/* 0x80288848: the class-4-6 weapon act dispatcher */
void pl_weapon456_act_dispatch(struct _PLW* self);
/* 0x805CB894 / 0x805CBA64: the two scan-row tables (`_PLW`+0x318) during which weapon kinds 4-6 guard any hit */
extern u8 pl_scan_rows_guard_a[];
extern u8 pl_scan_rows_guard_b[];
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_802840DC_H */
