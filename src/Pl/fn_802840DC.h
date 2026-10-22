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

/* The per-act handlers `fn_802840DC` tail-calls out of its `act_no` switch (cases 0-34; the two
 * families are the ones carrying an index argument and the ones that take only the actor).  The
 * call site's own register width is all there is to go on: the index is `li`-materialised (0-4) and
 * the actor is the same `_PLW` every other Pl entry takes. */
void fn_80282CCC(struct _PLW* self, u32 arg);
void fn_80282ED8(struct _PLW* self, u32 arg);
void fn_80283010(struct _PLW* self, u32 arg);
void fn_802831EC(struct _PLW* self, u32 arg);
void fn_802833F4(struct _PLW* self, u32 arg);
void fn_8028350C(struct _PLW* self);
void fn_802835D8(struct _PLW* self);
void fn_802836A4(struct _PLW* self, u32 arg);
void fn_80283810(struct _PLW* self, u32 arg);
void fn_802839A8(struct _PLW* self, u32 arg);
void fn_80283B88(struct _PLW* self);
void fn_80283C60(struct _PLW* self, u32 arg);
void fn_80283DB0(struct _PLW* self);
void fn_80283E80(struct _PLW* self);
void fn_80283F24(struct _PLW* self);
void fn_80283FC8(struct _PLW* self, u32 arg);

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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_802840DC_H */
