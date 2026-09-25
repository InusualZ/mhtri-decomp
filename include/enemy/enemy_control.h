/* The records `enemy/enemy_control.cpp` reads out of the enemy-control work blob `emc_work`
 * (0x806A4590, size 0xF50).  Kept in a header because more than one of the unit's functions uses
 * them (docs/plan.md 6.5 rule 1).
 *
 * NOTE (rule-1 follow-up): `emc_work`'s per-enemy slot is the same 0x18-byte record
 * `enemy/fn_8013F764.cpp` calls `EmcWork` privately in its `src/` file; the two definitions should
 * be folded into this one when that unit is touched.  This header names the slot `EmcSlot` to keep
 * the two spellings visibly distinct until then.
 */
#ifndef MHTRI_ENEMY_ENEMY_CONTROL_H
#define MHTRI_ENEMY_ENEMY_CONTROL_H

#include "types.h"

/* one 0x18-byte per-enemy slot of `emc_work` (the array `fn_801413D0` scans and `fn_80141358`
 * clears; `enemy/fn_8013F764.cpp` calls the same record `EmcWork`).
 * size: 0x18 */
struct EmcSlot {
    /* +0x00 */ u8 id;         /* the slot index `fn_80141358` stores, returned by `fn_80141470` */
    /* +0x01 */ u8 state;      /* 0 idle, 1 claimed, 2 released */
    /* +0x02 */ u8 kind;       /* the file kind `fn_80141470` claims the slot for */
    /* +0x03 */ u8 flags;
    /* +0x04 */ u8 field_0x04; /* cleared by `fn_801415A8` on release */
    /* +0x05 */ u8 unused_0x05[3];
    /* +0x08 */ s32 handle_0x08;
    /* +0x0C */ s32 handle_0x0C;
    /* +0x10 */ s32 handle_0x10;
    /* +0x14 */ s32 handle_0x14;
};

/* one 0x14-byte sparkle ("senko") record: the array at `emc_work + 0x90` (`fn_801416EC` returns its
 * first free element, `senko_set` fills it, `fn_80141690` frees an element).
 * size: 0x14 */
struct SenkoRec {
    /* +0x00 */ u8 unused_0x00[0x0C];
    /* +0x0C */ f32 value_0x0C;  /* `senko_set`'s float */
    /* +0x10 */ s16 countdown;   /* `senko_set`'s third value / the tick `fn_801417FC` steps */
    /* +0x12 */ u8 value_0x12;
    /* +0x13 */ u8 state;        /* 2 = free (set by `fn_80141690`), else in use */
};

/* one 0x24-byte record of the second marker array at `emc_work + 0x1F8` (the ten entries
 * `fn_80141B2C` clears the +0x09 byte of).
 * size: 0x24 */
struct Marker2Rec {
    /* +0x00 */ u8 unused_0x00[0x09];
    /* +0x09 */ u8 field_0x09;
    /* +0x0A */ u8 unused_0x0A[0x24 - 0x0A];
};

/* the enemy-control work blob `emc_work`, as far as this unit's reconstructed functions read it.
 * Unnamed runs keep their offsets (rules 4/5); fields are added as the bodies need them.
 * size: 0xF50 */
struct EmcWork {
    /* +0x000 */ EmcSlot slot_0x000[6];
    /* +0x090 */ SenkoRec senko_0x090[8];
    /* +0x130 */ SenkoRec marker_0x130[10];
    /* +0x1F8 */ struct Marker2Rec marker2_0x1F8[10];
    /* +0x360 */ u32 field_0x360;
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ s16 field_0x368;
    /* +0x36A */ u8 field_0x36A;
    /* +0x36B */ u8 field_0x36B;
    /* +0x36C */ u8 unused_0x36C[0xF48 - 0x36C];
    /* +0xF48 */ u32* field_0xF48;
    /* +0xF4C */ u32 field_0xF4C;
};

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the entry points this unit publishes to its consumers (docs/plan.md 6.5 rule 2).  `fn_80143174`
 * is `src/Pl/pl_act.cpp`'s - it is registered but not yet reconstructed (see the unit source's
 * Status). */
void* fn_80143174(void* a, void* b, s32 c);
/* The action band's arming helpers that live inside this unit's range, called by
 * `enemy/fn_80178378.cpp` (docs/plan.md 6.5 rule 2: an extern lives with the TU that owns it).
 * The bodies come with this unit's follow-up queue; the signatures are the call sites'. */
void fn_80146058(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_8014610C(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);


/* 0x801421E4 - r3 is narrowed with `clrlwi r3,r3,16` (a u16 id, 0xFFFF = the "no record" arm) and r4
 * is the out record `fn_80125F54` prepared; returns a word the enemy program functions compare with 1.
 * Added with `enemy/fn_801B7020.cpp` (rule 2: this unit owns the address). */
u32 fn_801421E4(u32 id, void* out);

#ifdef __cplusplus
}
#endif

#endif
