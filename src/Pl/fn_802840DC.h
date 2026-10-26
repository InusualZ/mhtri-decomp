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

/* One 18-byte attack row of a weapon-2 row act: `mode` selects which half of the motion/parameter fields
 * applies. size: 0x12 (`mulli ...,<kind>,18` at every call site) */
typedef struct ShellAtkRow {
    /* +0x00 */ u16 mode;       /* the motion when `mode == 0xFFFF` picks the second field set, the
                                 * row's alternate-selector when the caller asks for kind 1 */
    /* +0x02 */ s16 value_0x02; /* second set: the motion's first argument */
    /* +0x04 */ s16 value_0x04; /* second set: the motion's second argument */
    /* +0x06 */ u16 add_0x06;   /* second set: frames added to the actor's own frame counters */
    /* +0x08 */ s16 gate_0x08;  /* > 0 arms the motion gate (`pl_act_set_motion_gate`) with this value */
    /* +0x0A */ u16 motion_0x0A;  /* first set: the motion `Pl_chr_set_attr_default` sets */
    /* +0x0C */ s16 param_0x0C;   /* first set: the motion's first argument */
    /* +0x0E */ s16 param_0x0E;   /* first set: the motion's second argument */
    /* +0x10 */ s16 attack_0x10;  /* first set: the attack value `pl_weapon2_attack_set` is run with */
} ShellAtkRow;

/* The weapon-2 band's own `.data` tables (`Pl/fn_802840DC.cpp`'s claim 0x805C9568-0x805CBFC0), declared, not yet
 * emitted: the hit table its attack entries walk and its `.sdata` id list, the two charge-stage act lists, the
 * `ShellAtkRow` tables its row acts index by kind, and the step tables `Pl_act_set_step_table` is handed. */
extern u8 pl_weapon2_hit_table[];
extern s32 pl_weapon2_hit_ids[2];
extern u16 pl_weapon2_charge_acts_a[6];
extern u16 pl_weapon2_charge_acts_b[6];
extern ShellAtkRow pl_weapon2_rows_a[];
extern ShellAtkRow pl_weapon2_rows_b[];
extern ShellAtkRow pl_weapon2_rows_c[];
extern ShellAtkRow pl_weapon2_alt_rows_a[];
extern ShellAtkRow pl_weapon2_alt_rows_b[];
extern ShellAtkRow pl_weapon2_alt_rows_c[];
extern ShellAtkRow pl_weapon2_alt_rows_lunge[];
extern ShellAtkRow pl_weapon2_alt_rows_d[];
extern ShellAtkRow pl_weapon2_alt_rows_e[];
extern u8 pl_weapon2_steps_rapid[];
extern u8 pl_weapon2_steps_rapid_loop[];
extern u8 pl_weapon2_alt_steps_rapid[];
extern u8 pl_weapon2_alt_steps_rapid_loop[];
extern u8 pl_weapon2_steps_row_a[];
extern u8 pl_weapon2_steps_row_b[];
extern u8 pl_weapon2_steps_row_b1[];
extern u8 pl_weapon2_steps_lead[];
extern u8 pl_weapon2_steps_lead_swing[];
extern u8 pl_weapon2_steps_row_c[];
extern u8 pl_weapon2_steps_row_c1[];
extern u8 pl_weapon2_steps_back_swing[];
extern u8 pl_weapon2_steps_swing_g[];
extern u8 pl_weapon2_steps_charge[];
extern u8 pl_weapon2_steps_lunge[];
extern u8 pl_weapon2_steps_lunge_timed[];
extern u8 pl_weapon2_steps_lunge_long[];
extern u8 pl_weapon2_steps_shell_fire[];
extern u8 pl_weapon2_steps_swing_f[];
extern u8 pl_weapon2_steps_step_motion[];
extern u8 pl_weapon2_alt_steps_row_a[];
extern u8 pl_weapon2_alt_steps_row_b[];
extern u8 pl_weapon2_alt_steps_row_b1[];
extern u8 pl_weapon2_alt_steps_lead_swing[];
extern u8 pl_weapon2_alt_steps_row_c[];
extern u8 pl_weapon2_alt_steps_row_c1[];
extern u8 pl_weapon2_alt_steps_step_motion[];
extern u8 pl_weapon2_alt_steps_back_swing[];
extern u8 pl_weapon2_alt_steps_row_d[];
extern u8 pl_weapon2_alt_steps_charge[];
extern u8 pl_weapon2_alt_steps_lunge[];
extern u8 pl_weapon2_alt_steps_lunge_timed[];
extern u8 pl_weapon2_alt_steps_lunge_b[];
extern u8 pl_weapon2_alt_steps_shell_fire[];
extern u8 pl_weapon2_alt_steps_row_e[];
/* The weapon-4/5/6 and class-7 attack entries' hit tables and `.sdata` id lists (a *sized* id list is what makes
 * MWCC address it through r13, the target's `@sda21` form). */
extern u8 pl_weapon456_hit_table[];
extern s32 pl_weapon456_hit_ids[1];
extern u8 pl_class7_hit_table[];
extern s32 pl_class7_hit_ids[2];

/* One fire-motion row of the weapon-4/5/6 fire act: the fire motion and its aim pair. size: 0x6 */
typedef struct PlGunFireMotion {
    /* +0x0 */ u16 motion;
    /* +0x2 */ u16 up_motion;
    /* +0x4 */ u16 down_motion;
} PlGunFireMotion;

/* The weapon-4/5/6 tables: the melee step tables per weapon class, the two forms' fire-motion rows and fire step
 * tables, the crouch fire's frame counts per ammo and its two follow-up act lists. */
extern u8 pl_weapon456_steps_melee_4[];
extern u8 pl_weapon456_steps_melee_5[];
extern u8 pl_weapon456_steps_melee_6[];
extern u8 pl_weapon456_alt_steps_melee_4[];
extern u8 pl_weapon456_alt_steps_melee_56[];
extern PlGunFireMotion pl_weapon456_fire_motions[];
extern PlGunFireMotion pl_weapon456_alt_fire_motions[];
extern u32 pl_weapon456_fire_steps_5[];
extern u32 pl_weapon456_fire_steps_6[];
extern u32 pl_weapon456_alt_fire_steps[];
extern s32 pl_weapon456_crouch_frames[];
extern u16 pl_weapon456_crouch_acts[3];
extern u16 pl_weapon456_alt_crouch_acts[3];

/* 0x802872E4 (0x48): latches +0x276 while a magazine is loaded and the scan rows hold the id-252 row. */
void pl_gun_guard_row_latch(struct _PLW* self);
/* 0x8028732C (0x60): 1 when the actor's scan rows (+0x318) hold a kind-4 row with id 252. */
u32 pl_scan_rows_id252_ck(struct _PLW* self);

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
