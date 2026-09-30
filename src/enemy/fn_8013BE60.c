/* auto/8013BE60_fn_8013BE60.c - the enemy parameter interpreter and its handler table,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * .text 0x8013BE60..0x8013F764 (78 functions).
 *
 * What it is.  `fn_8013BE60` is the generic "apply one parameter record" interpreter: it takes the
 * enemy work record, a byte stream, an action id, a sub-command and a signed value, and dispatches on
 * the stream's command byte and the sub-command.  `fn_8013C244` is its two-argument tail
 * (`fn_8013BE60(self, in, id, 0, 0)`).  The other 76 functions are the per-action entries:
 *   * the two-argument entries - `if (*in == 0) fn_8013BE60(self, in, ID, SUB, VALUE); else
 *     fn_8013C244(self, in, ID);` - about thirty of them, all identical in shape;
 *   * the stream readers (`switch (*in)` over 0 / 2 / 255) that advance the stream through
 *     `fn_801406E0`/`fn_8013BDE4` and report a `s16` back through a stack slot;
 *   * the entries that read a field of the `_ENEMY_DATA` record `enemy_data_find` returns.
 *
 * `self` is the game's enemy work record (`_ENEMY_WORK`, the map's `11_ENEMY_WORK` mangling):
 * `get_enemy_data`, `em_sleep_ck` and `get_move_work_adrs` take it directly, and +0x188/+0x36C are its
 * own and its target position.  The field names are offset-derived, because the record is a large
 * opaque engine struct whose fields are only visible one at a time through the handlers; a field is
 * renamed when its use makes the meaning clear.
 *
 * Flags.  Two scoped pragmas, each measured on this unit:
 *   * `#pragma peephole off` - retail keeps MWCC's unfused forms where the -O3 peephole folds them:
 *     `(x & 8) == 0` stays `rlwinm`+`cmpwi` (fn_8013DD48) and `((x & 0xF) << 8) | y` stays
 *     `clrlwi`+`slwi`+`or` instead of one `rlwimi` (fn_8013D310).  With the pragma:
 *     fn_8013DD48 97.81 -> 100.00, fn_8013D310 75.67 -> 100.00, fn_8013DA7C 99.98 -> 100.00,
 *     fn_8013E388 99.93 -> 100.00; no other symbol moves.
 *   * `#pragma fp_contract off` - the -fp_contract-on default fuses `dx*dx + dz*dz` into `fmadds` in
 *     fn_8013E2B0 (99.81) where retail keeps `fmuls`+`fmuls`+`fadds` (100.00).
 *
 * Residual (65 of 78 functions byte-identical; unit fuzzy_match_percent 72.19, matched_code 8584 of
 * 14596):
 *   * not reconstructed yet: fn_8013C57C (536 B), fn_8013E528 (460 B), fn_8013BE60 (996 B),
 *     fn_8013EFD4 (1508 B), fn_8013F5B8 (428 B);
 *   * fn_8013D1D0 47.50 - a 16-byte tail call where retail computes `in[0]` into a scratch register
 *     and `in + 1` from the unmodified pointer, while our source loads `in[0]` into r4 first;
 *   * fn_8013D588 79.51 - the 16-bit angular-difference idiom: retail emits
 *     `clrlwi`+`cmplwi 0x8000`+`subf 0x10000` (i.e. `abs((s16)diff)`) where our plain
 *     `(s16)diff > tolerance` gives `extsh`+`cmpw`;
 *   * fn_8013C7E0 91.45, fn_8013CAA0 91.04, fn_8013D8C8 98.00 - nested-loop return shapes (retail
 *     carries a second unconditional branch to the shared `return 0` that our source does not emit);
 *   * fn_8013C988 99.69, fn_8013D054 99.66, fn_8013EBC8 99.82 - one allocator or compare instruction
 *     each.
 *
 * The bodies are grouped by shape rather than written in address order; objdiff pairs by symbol name,
 * so the per-symbol score is unaffected.
 *
 * Range and inventory: `python tools/units/ledger.py unit auto/8013BE60_fn_8013BE60.c`.
 * Attribute pass evidence: `.pi/attribution-batch-4.patch.md`, `.pi/notes/attribution-batch-4.md`.
 * Data runs this unit's split records but does not yet claim (playbook 23, docs/plan.md 8.4):
 *   extabindex 0x80027D20..0x80027F60 (48)   .data 0x805A19D0..0x805A1A7C (3)
 *   .bss       0x806BD360..0x806BD808 (1)    .sdata2 0x80796D90..0x80796DBC (9)
 * `extab`/`extabindex` travel with the code unit (`dataqueue.py`'s `FRAGMENT_SECTIONS`); the rest
 * waits for the measured data pass (`tools/units/dataclaim.py`).
 *
 * Types.  `_ENEMY_WORK`, `_ENEMY_DATA`, `_ENEMY_TABLE`/`_ENEMY_LIST_*`/`_ENEMY_QUEUE_ENTRY` are
 * reconstructed here.  `Vec3` comes from `nw4r/math.h` (the same three `f32` as `nw4r::math::VEC3`,
 * now includable from C as well as C++).
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/fn_80138074.h"
#include "unsplit/enemy.h"
#include "ef/eft_slot.h"     /* eft_slot_effect_key, enemy_data_find, enemy_data_grp */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma peephole off
#pragma fp_contract off

/* --------------------------------------------------------------------------------------------- */
/* The enemy work record.  Fields are listed at their real offsets; `pad_0xNNN` is only what the
 * handlers never touch.  size: at least 0xA04 (the largest field read is +0xA00). */
/* --------------------------------------------------------------------------------------------- */

/* size: 0xC
 * `Vec3` is the C spelling from `nw4r/math.h` (layout x/y/z at +0x00/+0x04/+0x08). */

/* The enemy-data table chain: `get_enemy_data`'s +0x18 word points at the head record, whose +0x00 word
 * is the first 8-byte action entry; an entry's +0x04 word is its own list of 8-byte item entries. */
/* size: 0x8 */
typedef struct _ENEMY_LIST_HEAD {
    /* +0x00 */ struct _ENEMY_ACTION *first;
    /* +0x04 */ u32 pad_0x04;
} _ENEMY_LIST_HEAD;

/* size: 0x8 */
typedef struct _ENEMY_ACTION {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ struct _ENEMY_ITEM *items;
} _ENEMY_ACTION;

/* size: 0x8 */
typedef struct _ENEMY_ITEM {
    /* +0x00 */ u8 id;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u8 *entries; /* 8-byte entries */
} _ENEMY_ITEM;

/* The per-action entry table `fn_8013C254` indexes with the record at +0x9A4. */
/* size: 0xC */
typedef struct _ENEMY_TABLE_ENTRY {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ f32 value_0x04;
    /* +0x08 */ u8 value_0x08;
    /* +0x09 */ u8 pad_0x09[0x3];
} _ENEMY_TABLE_ENTRY;

/* size: 0x8 */
typedef struct _ENEMY_TABLE {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ _ENEMY_TABLE_ENTRY *entries;
} _ENEMY_TABLE;

/* One 8-byte slot of the action queue at +0x9AC. */
/* size: 0x8 */
typedef struct _ENEMY_QUEUE_ENTRY {
    /* +0x00 */ u8 a;
    /* +0x01 */ u8 b;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u32 value;
} _ENEMY_QUEUE_ENTRY;

/* size: 0xB18 (the per-kind allocation stride `create_move_work__Fl` uses for kind 3; the highest field
 * this unit reads is +0xA00). */
typedef struct _ENEMY_WORK {
    /* +0x000 */ u8 pad_0x000[0x3];
    /* +0x003 */ u8 id_0x03;
    /* +0x004 */ u8 pad_0x004[0x6];
    /* +0x00A */ u8 id_0x0A;
    /* +0x00B */ u8 pad_0x00B[0x1];
    /* +0x00C */ u8 state_0x00C;
    /* +0x00D */ u8 pad_0x00D[0x17B];
    /* +0x188 */ Vec3 pos;
    /* +0x194 */ Vec3 prev_pos;
    /* +0x1A0 */ u8 pad_0x1A0;
    /* +0x1A1 */ u8 pad_0x1A1[0xF];
    /* +0x1B0 */ Vec3 aim;
    /* +0x1BC */ u32 param_0x1BC;
    /* +0x1C0 */ u32 param_0x1C0;
    /* +0x1C4 */ u32 param_0x1C4;
    /* +0x1C8 */ u32 flags_0x1C8;
    /* +0x1CC */ u8 pad_0x1CC[0x14];
    /* +0x1E0 */ u8 enemy_id;
    /* +0x1E1 */ u8 act_id;
    /* +0x1E2 */ u8 state_0x1E2;
    /* +0x1E3 */ u8 state_0x1E3;
    /* +0x1E4 */ u8 pad_0x1E4[0x1];
    /* +0x1E5 */ u8 state_0x1E5;
    /* +0x1E6 */ u8 pad_0x1E6[0x1];
    /* +0x1E7 */ u8 state_0x1E7;
    /* +0x1E8 */ u8 pad_0x1E8[0x4];
    /* +0x1EC */ u16 bits_0x1EC;
    /* +0x1EE */ u8 pad_0x1EE[0x4];
    /* +0x1F2 */ u16 counter_0x1F2;
    /* +0x1F4 */ u8 pad_0x1F4[0x5];
    /* +0x1F9 */ u8 state_0x1F9;
    /* +0x1FA */ u8 state_0x1FA;
    /* +0x1FB */ u8 pad_0x1FB[0x1];
    /* +0x1FC */ u8 state_0x1FC;
    /* +0x1FD */ u8 pad_0x1FD[0x1];
    /* +0x1FE */ u8 state_0x1FE;
    /* +0x1FF */ u8 state_0x1FF;
    /* +0x200 */ u8 pad_0x200[0x16C];
    /* +0x36C */ Vec3 target;
    /* +0x378 */ u8 pad_0x378[0x8];
    /* +0x380 */ u8 state_0x380;
    /* +0x381 */ u8 state_0x381;
    /* +0x382 */ u8 state_0x382;
    /* +0x383 */ u8 state_0x383;
    /* +0x384 */ f32 value_0x384;
    /* +0x388 */ u8 pad_0x388[0x9C];
    /* +0x424 */ u32 timer_0x424;
    /* +0x428 */ u8 pad_0x428[0x13];
    /* +0x43B */ u8 state_0x43B;
    /* +0x43C */ u8 state_0x43C;
    /* +0x43D */ u8 state_0x43D;
    /* +0x43E */ u8 pad_0x43E[0xE];
    /* +0x44C */ s16 value_0x44C;
    /* +0x44E */ s16 value_0x44E;
    /* +0x450 */ u8 pad_0x450[0x2];
    /* +0x452 */ s16 value_0x452;
    /* +0x454 */ u8 pad_0x454[0x18];
    /* +0x46C */ u8 enemy_data_id;
    /* +0x46D */ u8 state_0x46D;
    /* +0x46E */ u8 state_0x46E;
    /* +0x46F */ u8 state_0x46F;
    /* +0x470 */ u8 pad_0x470[0x432];
    /* +0x8A2 */ u16 counter_0x8A2;
    /* +0x8A4 */ u8 pad_0x8A4[0x72];
    /* +0x916 */ s16 value_0x916;
    /* +0x918 */ u8 pad_0x918[0x40];
    /* +0x958 */ u32 ptr_0x958;
    /* +0x95C */ u8 state_0x95C;
    /* +0x95D */ u8 state_0x95D;
    /* +0x95E */ u8 pad_0x95E[0x1];
    /* +0x95F */ u8 state_0x95F;
    /* +0x960 */ u8 state_0x960;
    /* +0x961 */ u8 pad_0x961[0x1];
    /* +0x962 */ u8 state_0x962;
    /* +0x963 */ u8 bytes_0x963[0xA];
    /* +0x96D */ u8 pad_0x96D[0x3];
    /* +0x970 */ u32 arr_0x970[0xA];
    /* +0x998 */ u8 state_0x998;
    /* +0x999 */ u8 state_0x999;
    /* +0x99A */ u8 state_0x99A;
    /* +0x99B */ u8 pad_0x99B[0x1];
    /* +0x99C */ u8 state_0x99C;
    /* +0x99D */ u8 state_0x99D;
    /* +0x99E */ u8 pad_0x99E[0x2];
    /* +0x9A0 */ u32 ptr_0x9A0;
    /* +0x9A4 */ u32 ptr_0x9A4;
    /* +0x9A8 */ u8 state_0x9A8;
    /* +0x9A9 */ u8 pad_0x9A9[0x3];
    /* +0x9AC */ _ENEMY_QUEUE_ENTRY queue_0x9AC[6];
    /* +0x9DC */ u8 pad_0x9DC[0xC];
    /* +0x9E8 */ u8 state_0x9E8;
    /* +0x9E9 */ u8 state_0x9E9;
    /* +0x9EA */ u8 pad_0x9EA[0x2];
    /* +0x9EC */ u32 ptr_0x9EC;
    /* +0x9F0 */ u32 ptr_0x9F0;
    /* +0x9F4 */ u8 state_0x9F4;
    /* +0x9F5 */ u8 state_0x9F5;
    /* +0x9F6 */ u8 state_0x9F6;
    /* +0x9F7 */ u8 state_0x9F7;
    /* +0x9F8 */ u8 state_0x9F8;
    /* +0x9F9 */ u8 state_0x9F9;
    /* +0x9FA */ u8 pad_0x9FA[0x1];
    /* +0x9FB */ u8 state_0x9FB;
    /* +0x9FC */ u8 state_0x9FC;
    /* +0x9FD */ u8 state_0x9FD;
    /* +0x9FE */ u8 state_0x9FE;
    /* +0x9FF */ u8 state_0x9FF;
    /* +0xA00 */ u32 ptr_0xA00;
} _ENEMY_WORK;

/* Callees owned by other translation units.  Addresses and the mangled spellings are the map's. */
/* `enemy_data_grp` (0x803439D4) is declared in its owner's header, `include/ef/eft_slot.h` (rule 2). */

/* --------------------------------------------------------------------------------------------- */
/* The generic parameter interpreter and its two-argument tail. */
/* --------------------------------------------------------------------------------------------- */

s16 fn_8013BE60(_ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s16 value);
void fn_8013C36C(_ENEMY_WORK *self, u8 value, u32 param);

/* The two-argument tail: sub-command 0 and a value of 0. */
void fn_8013C244(_ENEMY_WORK *self, u8 *in, u32 id) {
    fn_8013BE60(self, in, (u8)id, 0, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* Byte-set entries: a one-byte record copied straight into the work record. */
/* --------------------------------------------------------------------------------------------- */

void fn_8013C97C(_ENEMY_WORK *self, u8 *in) {
    self->state_0x9F9 = in[0];
}

void fn_8013E05C(_ENEMY_WORK *self, u8 *in) {
    self->state_0x95F = in[0];
}

void fn_8013E9C8(_ENEMY_WORK *self, u8 *in) {
    self->state_0x960 = in[0];
}

void fn_8013D684(_ENEMY_WORK *self, u8 *in) {
    self->timer_0x424 = in[0] * 100;
}

void fn_8013E6F4(_ENEMY_WORK *self) {
    self->value_0x452 = 0;
}

void fn_8013DED4(_ENEMY_WORK *self, u8 *in) {
    self->param_0x1BC = in[0] << 8;
    self->param_0x1C0 = in[1] << 8;
    self->param_0x1C4 = in[2] << 8;
}

/* --------------------------------------------------------------------------------------------- */
/* Two-argument parameter entries: `in[0] == 0` applies the value, anything else re-sends the id. */
/* --------------------------------------------------------------------------------------------- */

void fn_8013CBE4(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 7, 0, self->act_id);
    } else {
        fn_8013C244(self, in, 7);
    }
}

void fn_8013CC0C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 8, 0, self->state_0x43B);
    } else {
        fn_8013C244(self, in, 8);
    }
}

void fn_8013CC34(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 9, 0, self->state_0x43C);
    } else {
        fn_8013C244(self, in, 9);
    }
}

void fn_8013CC5C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 10, 0, self->state_0x43D);
    } else {
        fn_8013C244(self, in, 10);
    }
}

void fn_8013CC84(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 11, 4, 0);
    } else {
        fn_8013C244(self, in, 11);
    }
}

void fn_8013CCA8(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 12, 0, self->state_0x1E2);
    } else {
        fn_8013C244(self, in, 12);
    }
}

void fn_8013D2C8(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 22, 5, 0);
    } else {
        fn_8013C244(self, in, 22);
    }
}

void fn_8013D2EC(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 23, 6, 0);
    } else {
        fn_8013C244(self, in, 23);
    }
}

void fn_8013D310(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 24, 7, (s16)(u16)(((self->act_id & 0xF) << 8) | in[1]));
    } else {
        fn_8013C244(self, in, 24);
    }
}

void fn_8013D990(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 35, 0, self->state_0x00C);
    } else {
        fn_8013C244(self, in, 35);
    }
}

void fn_8013DE08(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 41, 0, self->id_0x0A);
    } else {
        fn_8013C244(self, in, 41);
    }
}

void fn_8013DE30(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 68, 9, self->id_0x0A);
    } else {
        fn_8013C244(self, in, 68);
    }
}

void fn_8013DE58(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 42, 15, self->enemy_id);
    } else {
        fn_8013C244(self, in, 42);
    }
}

void fn_8013E9D4(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 61, 0, self->state_0x960);
    } else {
        fn_8013C244(self, in, 61);
    }
}

void fn_8013EBA0(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 64, 0, self->state_0x1FE);
    } else {
        fn_8013C244(self, in, 64);
    }
}

void fn_8013EF14(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 70, 0, self->id_0x03);
    } else {
        fn_8013C244(self, in, 70);
    }
}

void fn_8013EF3C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 71, 0, self->state_0x1E3);
    } else {
        fn_8013C244(self, in, 71);
    }
}

void fn_8013EF64(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 73, 0, self->state_0x9F7);
    } else {
        fn_8013C244(self, in, 73);
    }
}

void fn_8013EF8C(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 74, 1, self->counter_0x1F2 % 100);
    } else {
        fn_8013C244(self, in, 74);
    }
}

/* A three-argument entry: the caller supplies a value whose low 16 bits are reduced modulo 100. */
void fn_8013C794(_ENEMY_WORK *self, u8 *in, u32 value) {
    if (in[0] == 0) {
        fn_8013BE60(self, in, 2, 1, (u16)value % 100);
    } else {
        fn_8013C244(self, in, 2);
    }
}

/* Forwards the stream one byte in and the first byte as the value. */
void fn_8013D1D0(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];

    fn_8013C36C(self, value, (u32)(in + 1));
}

/* --------------------------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------------------------- */
/* The enemy-data record `enemy_data_find` returns: only the offsets this unit reads are named. */
/* --------------------------------------------------------------------------------------------- */

/* size: 0x40 (lower bound: the highest field this unit reads is +0x39) */
typedef struct _ENEMY_DATA {
    /* +0x000 */ u8 pad_0x000[0x08];
    /* +0x008 */ u8 field_0x08;
    /* +0x009 */ u8 pad_0x009[0x2];
    /* +0x00B */ u8 field_0x0B;
    /* +0x00C */ u8 pad_0x00C;
    /* +0x00D */ u8 field_0x0D;
    /* +0x00E */ u8 field_0x0E;
    /* +0x00F */ u8 mode_0x0F;
    /* +0x010 */ u8 value_0x10;
    /* +0x011 */ u8 pad_0x011[0x3];
    /* +0x014 */ u8 mode_0x14;
    /* +0x015 */ u8 mode_0x15;
    /* +0x016 */ u8 mode_0x16;
    /* +0x017 */ u8 mode_0x17;
    /* +0x018 */ u32 ptr_0x18;
    /* +0x01C */ u8 pad_0x01C[0x1A];
    /* +0x036 */ u8 field_0x36;
    /* +0x037 */ u8 field_0x37;
    /* +0x038 */ u8 pad_0x038;
    /* +0x039 */ u8 field_0x39;
    /* +0x03A */ u8 pad_0x03A[0x6];
} _ENEMY_DATA;


/* The decoded parameter record another unit owns; its 0x10-byte entries are what the case-2 path of
 * every reader parses. */
typedef struct _ENEMY_PARAM _ENEMY_PARAM;

extern _ENEMY_DATA *get_enemy_data__FP11_ENEMY_WORK(_ENEMY_WORK *self);
/* `enemy_data_find` (0x803438E4): its owner's header, `include/ef/eft_slot.h` (rule 2). */
extern u32 fn_80345A6C(void *a, u8 b, Vec3 *c, f32 d);
extern u32 fn_80131BD4(void);
extern u32 PlayMode_ck__Fv(void);
extern f32 lbl_80796DA0; /* 0.0f */
extern f32 lbl_80796DA4; /* 0.5f */
extern f32 lbl_80796DA8; /* 65536.0f */
extern f32 lbl_80796DAC; /* 6.2831855f */
extern f32 calcVecDistXZ(Vec3 *a, Vec3 *b);
extern f32 sqrt_f32(f32 x);
extern f32 atan2f(f32 y, f32 x);
extern u32 fn_80125FF0(u8 a, u8 b);
extern void em_act_step_arm(_ENEMY_WORK *self, u32 a, u32 b, u32 c);
extern void fn_80126278(_ENEMY_WORK *self, u16 a, Vec3 *out);
extern void fn_8012B380(_ENEMY_WORK *self, u32 a, u32 b, u32 c);
extern void em_target_pos_set(_ENEMY_WORK *self, u32 a);
extern u32 fn_80126DAC(_ENEMY_WORK *self, u32 a, u32 b);
extern u32 fn_80126F80(_ENEMY_WORK *self, u32 a, u32 b);
extern u32 fn_801272C4(_ENEMY_WORK *self, Vec3 *pos, u32 a);
/* `eft_slot_effect_key` (0x8034539C): its owner's header, `include/ef/eft_slot.h` (rule 2). */
extern void fn_8013C57C(_ENEMY_WORK *self, u8 *in, u32 flag);
extern u32 fn_801275F0(_ENEMY_WORK *self, u32 a);
extern u32 fn_80127A7C(_ENEMY_WORK *self, u32 a);
extern void em_area_change(_ENEMY_WORK *self, u32 a);
extern void fn_802B01AC(Vec3 *out, Vec3 *in, u32 idx);
extern f32 fn_802B0430(u32 idx);
extern void get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(Vec3 *out, Vec3 *pos, u8 act);

/* A neighbouring record `fn_80130DF8` returns; only the two bytes this unit reads are named. */
/* size: 0x8 (lower bound: the unit reads +0x05 and +0x06) */
typedef struct _ENEMY_OTHER {
    /* +0x000 */ u8 pad_0x00[0x5];
    /* +0x005 */ u8 field_0x05;
    /* +0x006 */ u8 field_0x06;
    /* +0x007 */ u8 pad_0x07;
} _ENEMY_OTHER;

/* --------------------------------------------------------------------------------------------- */
/* The stream readers: `switch (in[0])` over the command byte, each case advancing the stream
 * through `fn_801406E0`/`fn_8013BDE4` and reporting a `s16` back through the stack slot. */
/* --------------------------------------------------------------------------------------------- */

s16 fn_8013CCD0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A), self->enemy_data_id) == 0) {
            in += (u8)fn_801406E0(14, in[0]);
            fn_8013BDE4(&in, 14, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(14, in[0]);
        result = fn_80140778(in, 14, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013CEB0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->state_0x382 == 255) {
            in += (u8)fn_801406E0(17, in[0]);
            fn_8013BDE4(&in, 17, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(17, in[0]);
        result = fn_80140778(in, 17, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D4C4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (em_alt_mode_ck(self) == 0) {
            in += (u8)fn_801406E0(28, in[0]);
            fn_8013BDE4(&in, 28, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(28, in[0]);
        result = fn_80140778(in, 28, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D694(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x44C > 0) {
            in += (u8)fn_801406E0(31, in[0]);
            fn_8013BDE4(&in, 31, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(31, in[0]);
        result = fn_80140778(in, 31, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D80C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x916 <= 0) {
            in += (u8)fn_801406E0(33, in[0]);
            fn_8013BDE4(&in, 33, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(33, in[0]);
        result = fn_80140778(in, 33, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013D9B8(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_8012EC3C(self) == 0) {
            in += (u8)fn_801406E0(36, in[0]);
            fn_8013BDE4(&in, 36, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(36, in[0]);
        result = fn_80140778(in, 36, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DA7C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A), self->enemy_data_id);
        if (data == 0 || data->mode_0x14 != self->act_id) {
            in += (u8)fn_801406E0(37, in[0]);
            fn_8013BDE4(&in, 37, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0(37, in[0]);
        result = fn_80140778(in, 37, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DC58(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A), self->enemy_data_id);
        if (data == 0 || data->mode_0x17 == 255) {
            in += (u8)fn_801406E0(39, in[0]);
            fn_8013BDE4(&in, 39, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0(39, in[0]);
        result = fn_80140778(in, 39, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DD48(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((self->flags_0x1C8 & 0x8) == 0) {
            in += (u8)fn_801406E0(40, in[0]);
            fn_8013BDE4(&in, 40, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(40, in[0]);
        result = fn_80140778(in, 40, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013DF78(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((u16)(calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(&self->pos, &self->target) -
                  self->param_0x1C0) >= 0x8000) {
            in += (u8)fn_801406E0(46, in[0]);
            fn_8013BDE4(&in, 46, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(46, in[0]);
        result = fn_80140778(in, 46, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E180(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A), self->enemy_data_id);
        in += (u8)fn_801406E0(50, in[0]);
        if (data == 0) {
            fn_80140AF8(self, 8, 50);
            fn_8013BDE4(&in, 50, &result);
        } else if (fn_80345A6C(data, data->field_0x36, &self->pos, lbl_80796DA0) == 0) {
            fn_8013BDE4(&in, 50, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0(50, in[0]);
        result = fn_80140778(in, 50, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E388(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x452 < (s16)(in[2] * 30 + in[1] * 1800)) {
            in += (u8)fn_801406E0(52, in[0]);
            fn_8013BDE4(&in, 52, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(52, in[0]);
        result = fn_80140778(in, 52, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E464(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_8012ECF0() == 0) {
            in += (u8)fn_801406E0(53, in[0]);
            fn_8013BDE4(&in, 53, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(53, in[0]);
        result = fn_80140778(in, 53, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013E700(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->value_0x452 < self->value_0x44E) {
            in += (u8)fn_801406E0(56, in[0]);
            fn_8013BDE4(&in, 56, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(56, in[0]);
        result = fn_80140778(in, 56, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

/* --------------------------------------------------------------------------------------------- */
/* The stream readers: `switch (in[0])` over the command byte, each case advancing the stream
 * through `fn_801406E0`/`fn_8013BDE4` and reporting a `s16` back through the stack slot. */
/* --------------------------------------------------------------------------------------------- */

s16 fn_8013E900(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((u8)PlayMode_ck__Fv() != 2) {
            in += (u8)fn_801406E0(59, in[0]);
            fn_8013BDE4(&in, 59, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(59, in[0]);
        result = fn_80140778(in, 59, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013EAE4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->state_0x1FC == 0) {
            in += (u8)fn_801406E0(63, in[0]);
            fn_8013BDE4(&in, 63, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(63, in[0]);
        result = fn_80140778(in, 63, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013EBC8(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (self->counter_0x8A2 > in[1] * 10 && fn_8012EC3C(self) == 0) {
            in += (u8)fn_801406E0(65, in[0]);
            fn_8013BDE4(&in, 65, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(65, in[0]);
        result = fn_80140778(in, 65, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013ECA4(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_80132184() == 0) {
            in += (u8)fn_801406E0(66, in[0]);
            fn_8013BDE4(&in, 66, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(66, in[0]);
        result = fn_80140778(in, 66, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

s16 fn_8013ED68(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if (fn_80131BD4() == 0) {
            in += (u8)fn_801406E0(67, in[0]);
            fn_8013BDE4(&in, 67, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(67, in[0]);
        result = fn_80140778(in, 67, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

/* Forwards its arguments to the second parameter interpreter. */
void fn_8013E068(_ENEMY_WORK *self, u8 *in, u32 id, u32 sub, s32 value) {
    fn_801409C8(self, in, id, sub, value);
}

/* --------------------------------------------------------------------------------------------- */
/* The record-based entries. */
/* --------------------------------------------------------------------------------------------- */

/* True when the angle between the record's position and its target is at or below the per-action
 * threshold indexed by the sub-state. */
u32 fn_8013C254(_ENEMY_WORK *self) {
    f32 angle = calcVecDistXZ(&self->pos, &self->target);
    _ENEMY_TABLE *table = (_ENEMY_TABLE *)self->ptr_0x9A4;

    return angle <= table->entries[self->state_0x382].value_0x04;
}

/* Finds the action entry matching the current action and returns the item entry the sub-state
 * selects, or 0 when the enemy has no data or either list runs out. */
u32 fn_8013C2B0(_ENEMY_WORK *self) {
    if (self->enemy_id != 255) {
        _ENEMY_DATA *data = get_enemy_data__FP11_ENEMY_WORK(self);
        _ENEMY_LIST_HEAD *head = (_ENEMY_LIST_HEAD *)data->ptr_0x18;

        if (head != 0 && head->first != 0) {
            _ENEMY_ACTION *act = head->first;

            for (; act->id != 255; act++) {
                if (fn_80125FF0(self->enemy_id, act->id) == 1) {
                    _ENEMY_ITEM *item = act->items;

                    for (; item->id != 255; item++) {
                        if (item->id == self->act_id) {
                            return (u32)&item->entries[self->state_0x9FB * 8];
                        }
                    }
                    break;
                }
            }
        }
    }
    return 0;
}

/* Pushes an 8-byte action slot (the current action pair plus `param`) onto the queue at +0x9AC and
 * re-seeds the current action pair when the push does not take. */
void fn_8013C36C(_ENEMY_WORK *self, u8 value, u32 param) {
    if (self->state_0x9A8 >= 6) {
        fn_80140AF8(self, 15, 0);
        self->state_0x9A8--;
    }
    self->queue_0x9AC[self->state_0x9A8].a = self->state_0x95C;
    self->queue_0x9AC[self->state_0x9A8].b = self->state_0x95D;
    self->queue_0x9AC[self->state_0x9A8].value = param;
    if (fn_8013AB74(self, 1, value) == 0) {
        fn_80140AF8(self, 3, self->state_0x95C);
        self->state_0x95C = self->queue_0x9AC[self->state_0x9A8].a;
        self->state_0x95D = self->queue_0x9AC[self->state_0x9A8].b;
    }
    self->state_0x9A8++;
}

/* Chooses the sub-command the current combat state maps to and applies it. */
void fn_8013C458(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];
    u8 sub = in[1];

    if (value == 0 && sub == 255) {
        switch (self->state_0x1E2) {
        case 2:
            if (fn_8012EC3C(self) == 1 && (self->flags_0x1C8 & 0x40) != 0) {
                sub = 5;
            } else {
                sub = 4;
            }
            break;
        case 1:
            switch (self->state_0x1E3) {
            case 2:
                sub = 8;
                break;
            case 1:
                sub = 9;
                break;
            default:
                sub = 3;
                break;
            }
            break;
        case 3:
            sub = 6;
            break;
        case 4:
            sub = 7;
            break;
        default:
            if (fn_8012EC3C(self) == 1 && (self->flags_0x1C8 & 0x40) != 0) {
                sub = 2;
            } else {
                sub = (self->state_0x43D == 1);
            }
            break;
        }
    }
    em_act_step_arm(self, value, sub, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* The remaining action entries. */
/* --------------------------------------------------------------------------------------------- */

/* Recomputes the current action's parameter ids from the type byte and the preset byte. */
void fn_8013C7E0(_ENEMY_WORK *self, u8 *in) {
    switch (in[0]) {
    default:
        self->state_0x9F8 = 255;
        self->state_0x9F7 = 255;
        break;
    case 1:
        self->state_0x9F8 = in[1];
        if (self->state_0x9F8 == 253) {
            self->state_0x9F8 = self->state_0x1FF;
        }
        self->state_0x9F7 = fn_80126DAC(self, self->act_id, self->state_0x9F8);
        break;
    case 2: {
        u8 preset = in[1];
        u32 current;

        if (preset == 253) {
            preset = self->state_0x1FF;
        }
        current = fn_80126DAC(self, self->act_id, preset);
        self->state_0x9F7 = current;
        self->state_0x9F8 = preset;
        while ((u8)current != preset) {
            if (fn_80126F80(self, self->enemy_id, (u8)current) == 1) {
                self->state_0x9F8 = current;
                break;
            }
            current = fn_80126DAC(self, (u8)current, preset);
        }
        break;
    }
    case 4: {
        u32 id;
        _ENEMY_DATA *entry = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                            self->enemy_data_id);

        if (entry != 0) {
            id = eft_slot_effect_key((struct EftSlot *)entry);
            self->state_0x9F8 = id;
            self->state_0x9F7 = fn_80126DAC(self, self->act_id, (u8)id);
        } else {
            id = fn_801272C4(self, &self->pos, self->act_id);
            self->state_0x9F7 = id;
            self->state_0x9F8 = id;
        }
        break;
    }
    case 3: {
        u32 id = fn_801272C4(self, &self->pos, self->act_id);

        self->state_0x9F7 = id;
        self->state_0x9F8 = id;
        break;
    }
    case 5:
        self->state_0x9F7 = 254;
        self->state_0x9F8 = 254;
        break;
    }
}

/* Copies the position the record aims at into the aim vector. */
void fn_8013D41C(_ENEMY_WORK *self, u8 *in) {
    switch (in[0]) {
    case 0:
        copyVec3(&self->aim, &self->pos);
        break;
    case 1:
        copyVec3(&self->aim, &self->target);
        break;
    default:
        fn_80140AF8(self, 9, 0);
        break;
    }
}

/* Sets the aim timer and the motion blend from a parameter record, then re-runs the movement entry. */
void fn_8013D45C(_ENEMY_WORK *self, u8 *in, u32 flag) {
    self->state_0x383 = 1;
    self->value_0x384 = (f32)(in[3] * 100);
    if (in[4] == 1) {
        self->state_0x383 |= 0x80;
    }
    fn_8013C57C(self, in, (u8)flag);
}

/* True while the difference between the stored facing and the vector to the target is greater than the
 * tolerance the record carries. */
void fn_8013D588(_ENEMY_WORK *self, u8 *in, u32 unused) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        if ((s16)(calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(&self->pos, &self->target) -
                  self->param_0x1C0) > in[1] * 256) {
            in += (u8)fn_801406E0(29, in[0]);
            fn_8013BDE4(&in, 29, &result);
        }
        break;
    case 2:
        in += (u8)fn_801406E0(29, in[0]);
        result = fn_80140778(in, 29, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
}

/* Queues a sub-action pointer for the movement entry, or re-seeds the current action when the queue
 * is already occupied. */
void fn_8013D750(_ENEMY_WORK *self, u8 *in) {
    u8 value = in[0];
    u8 *next = in + 1;

    if (self->state_0x95C != 0) {
        fn_80140AF8(self, 10, self->state_0x95C);
        self->ptr_0x958 = (u32)next;
    } else {
        if (self->state_0x962 >= 10) {
            fn_80140AF8(self, 11, 0);
            self->state_0x962--;
        }
        self->bytes_0x963[self->state_0x962] = self->state_0x95D;
        self->arr_0x970[self->state_0x962] = (u32)next;
        self->state_0x962++;
        fn_8013AB74(self, 0, value);
    }
}

/* Sends the parameter's remainder modulo the record's step count. */
void fn_8013D8C8(_ENEMY_WORK *self, u8 *in, u32 param) {
    if (in[0] == 0) {
        if (fn_8013BE60(self, in, 34, 11, 1) >= 0) {
            return;
        }
        {
            u8 *next = in + (u8)fn_801406E0(34, in[0]);
            s16 step = fn_80140778(next, 34, 2);

            fn_8013BE60(self, in, 34, 10, (s16)((u16)param % step));
        }
        return;
    }
    fn_8013C244(self, in, 34);
}

/* Moves the converted target position into the record's position and remembers the old one. */
void fn_8013DE80(_ENEMY_WORK *self, u8 *in) {
    fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | in[0]), &self->pos);
    copyVec3(&self->prev_pos, &self->pos);
}

/* Rebuilds the aim vector from a parameter record and stores the facing angle and zero offsets. */
void fn_8013DEFC(_ENEMY_WORK *self, u8 *in) {
    Vec3 v;

    VEC3_ctor(&v);
    fn_80126278(self, (u16)(((self->act_id & 0xF) << 8) | in[0]), &v);
    self->param_0x1BC = 0;
    self->param_0x1C0 = calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3(&self->pos, &v);
    self->param_0x1C4 = 0;
}

/* Sends the horizontal bearing to the target as a byte-scaled angle. */
void fn_8013E2B0(_ENEMY_WORK *self, u8 *in) {
    if (in[0] == 0) {
        f32 dx = self->target.x - self->pos.x;
        f32 dz = self->target.z - self->pos.z;
        f32 dist = sqrt_f32(dx * dx + dz * dz);
        f32 angle = atan2f(-(self->target.y - self->pos.y), dist);
        u16 raw = (u16)(s32)(lbl_80796DA8 * angle / lbl_80796DAC + lbl_80796DA4);

        fn_8013BE60(self, in, 51, 8, (s16)(u8)(((s32)raw >> 8) + 64));
    } else {
        fn_8013C244(self, in, 51);
    }
}

/* Starts, advances and retires a repeated parameter record; returns -1 while one is still active. */
s16 fn_8013E06C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0:
        self->state_0x9F4 = in[1];
        self->state_0x9F5 = in[2];
        self->ptr_0x9F0 = (u32)(in + 3);
        break;
    case 3:
        self->state_0x9F4 = 255;
        in += (u8)fn_801406E0(49, in[0]);
        result = fn_80140778(in, 49, 1);
        break;
    case 255:
        switch (self->state_0x9F4) {
        default:
            fn_80140AF8(self, 14, self->state_0x9F4);
            break;
        case 0:
            result = -1;
            self->ptr_0x958 = self->ptr_0x9F0;
            break;
        case 1:
            self->state_0x9F5--;
            if (self->state_0x9F5 != 0) {
                result = -1;
                self->ptr_0x958 = self->ptr_0x9F0;
            }
            break;
        case 255:
            break;
        }
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}

/* Reads the data record's action field or, when there is none, the record's own parsed value. */
s16 fn_8013E9FC(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 62);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(62, in[0]);
            result = fn_80140778(in, 62, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 62, 0, data->field_0x0B);
    } else {
        fn_8013C244(self, in, 62);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 69 and the data record's +0x0D field. */
s16 fn_8013EE2C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 18);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(69, in[0]);
            result = fn_80140778(in, 69, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 69, 2, data->field_0x0D);
    } else {
        fn_8013C244(self, in, 69);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 38 and the data record's +0x08 field. */
s16 fn_8013DB70(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 38);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(38, in[0]);
            result = fn_80140778(in, 38, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 38, 0, data->field_0x08);
    } else {
        fn_8013C244(self, in, 38);
    }
}

/* Same reader as `fn_8013E9FC`, for the action id 21 and the data record's +0x0F field. */
s16 fn_8013D1E0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 21);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(21, in[0]);
            result = fn_80140778(in, 21, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 21, 0, data->mode_0x0F);
    } else {
        fn_8013C244(self, in, 21);
    }
}

/* Selects the combat sub-command the data record's mode byte names. */
void fn_8013D34C(_ENEMY_WORK *self) {
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 25);
        return;
    }
    switch (data->mode_0x0F) {
    case 0:
        fn_8012B380(self, 1, 2, data->value_0x10);
        break;
    case 1:
        fn_8012B380(self, 2, 2, data->value_0x10);
        break;
    case 2:
        fn_8012B380(self, 3, 2, data->value_0x10);
        break;
    default:
        break;
    }
    em_target_pos_set(self, 0);
}

/* --------------------------------------------------------------------------------------------- */
/* The aim and action-queue entries. */
/* --------------------------------------------------------------------------------------------- */

/* Re-projects the position of the aim target and either applies it or sends the failure code. */
void fn_8013C988(_ENEMY_WORK *self, u8 *in) {
    Vec3 a;
    Vec3 b;
    Vec3 c;
    Vec3 d;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (in[0]) {
    case 0:
        if (self->state_0x9F7 != 255) {
            self->state_0x1F9 = 1;
            self->state_0x1FA = 1;
            fn_8012B380(self, 9, self->state_0x9F7, 255);
            em_target_pos_set(self, 0);
        }
        break;
    case 255:
        if (self->state_0x9F7 != self->act_id && self->state_0x9F7 != 255) {
            get_worldworld_pos__FPQ34nw4r4math4VEC3Uc(&c, &self->pos, self->act_id);
            copyVec3(&a, &c);
            fn_802B01AC(&d, &a, self->state_0x9F7);
            copyVec3(&self->pos, &d);
            self->pos.y = fn_802B0430(self->state_0x9F7);
            em_area_change(self, 0);
            em_target_pos_set(self, 0);
        }
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
}

/* Starts a repeated parameter record and restores the action pair when it cannot be served. */
void fn_8013CAA0(_ENEMY_WORK *self, u8 *in) {
    u32 value;

    self->state_0x999 = 0;
    self->ptr_0x9A4 = 0;
    self->state_0x998 = in[0];
    self->state_0x99A = in[1];
    self->state_0x99C = self->state_0x95C;
    self->state_0x99D = self->state_0x95D;
    self->ptr_0x9A0 = (u32)(in + 2);
    if (fn_8013A884(self, self->state_0x95C) == 1) {
        fn_8012B380(self, 8, self->state_0x998, 255);
        if (self->ptr_0x9A4 != 0) {
            value = ((_ENEMY_TABLE *)self->ptr_0x9A4)->entries[self->state_0x382].value_0x08;
        }
        if (fn_8013AB74(self, 2, (u8)value) == 1) {
            em_target_pos_set(self, 0);
            if (self->ptr_0x9A4 != 0 &&
                (u32)((*(u8 *)self->ptr_0x9A4) - 252) > 3) {
                self->state_0x1F9 = 1;
                self->state_0x1FA = 1;
            }
            return;
        }
    }
    fn_80140AF8(self, 3, self->state_0x95C);
    fn_80140B10(self, 0, self->id_0x03);
    if (self->state_0x9F7 != 255) {
        em_area_change(self, 0);
    }
    self->state_0x95C = self->state_0x99C;
    self->state_0x95D = self->state_0x99D;
    self->ptr_0x958 = self->ptr_0x9A0;
}

/* Applies a motion blend record and asks the movement entry for the blended piece. */
void fn_8013CDB4(_ENEMY_WORK *self, u8 *in) {
    u32 piece;

    self->state_0x1E7 = in[0];
    if (in[1] != 0) {
        self->state_0x383 = 1;
        self->value_0x384 = (f32)(in[1] * 100);
        if (in[2] == 1) {
            self->state_0x383 |= 0x80;
        }
    }
    piece = fn_801275F0(self, 1);
    if ((u8)fn_80127A7C(self, (u8)piece) == 0) {
        fn_8012B380(self, 10, 1, (u8)piece);
    } else {
        fn_8012B380(self, 10, 2, (u8)piece);
    }
    em_target_pos_set(self, 0);
    self->state_0x1E7 = in[0];
}

/* Same reader as `fn_8013D1E0`, for the action id 18 and the data record's +0x0E field. */
s16 fn_8013CF6C(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 18);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(18, in[0]);
            result = fn_80140778(in, 18, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        fn_8013BE60(self, in, 18, 2, data->field_0x0E);
    } else {
        fn_8013C244(self, in, 18);
    }
}

/* Sends the combat sub-command the data record's aim fields select. */
s16 fn_8013D054(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;
    _ENEMY_DATA *data = (_ENEMY_DATA *)enemy_data_find((u8)enemy_data_grp(self->id_0x03, self->id_0x0A),
                                                   self->enemy_data_id);

    if (data == 0) {
        fn_80140AF8(self, 8, 19);
        if (in[0] <= 2) {
            in += (u8)fn_801406E0(19, in[0]);
            result = fn_80140778(in, 19, 1);
        }
        return result;
    }
    if (in[0] == 0) {
        u32 sub;
        s16 value;

        switch (in[1]) {
        case 0:
            value = self->state_0x46D;
            sub = 3;
            break;
        case 1:
            value = self->state_0x46E;
            sub = 3;
            break;
        case 2:
            if (self->state_0x46F == 255 ||
                self->state_0x46E > data->field_0x39 - 1) {
                if (self->state_0x46E > data->field_0x37) {
                    self->state_0x46F = 255;
                } else {
                    self->state_0x46F = 0;
                }
            }
            value = self->state_0x46F;
            sub = 2;
            break;
        default:
            fn_80140AF8(self, 12, 0);
            value = 0;
            sub = 3;
            break;
        }
        fn_8013BE60(self, in, 19, sub, value);
    } else {
        fn_8013C244(self, in, 19);
    }
}

/* Starts or advances a repeated parameter record against the current enemy data. */
s16 fn_8013E7C0(_ENEMY_WORK *self, u8 *in) {
    s16 result = 0;

    switch (in[0]) {
    case 0: {
        _ENEMY_OTHER *other = (_ENEMY_OTHER *)fn_80130DF8();

        if (other != 0) {
            if (fn_8013A884(self, 10) == 1) {
                self->state_0x9E8 = self->state_0x95C;
                self->state_0x9E9 = self->state_0x95D;
                self->ptr_0x9EC = (u32)(in + 1);
                fn_8013AB74(self, 10, 0);
            }
            fn_8012B380(self, 7, other->field_0x05, other->field_0x06);
            em_target_pos_set(self, 0);
        } else {
            in += (u8)fn_801406E0(58, in[0]);
            fn_8013BDE4(&in, 58, &result);
        }
        break;
    }
    case 2:
        in += (u8)fn_801406E0(58, in[0]);
        result = fn_80140778(in, 58, 1);
        break;
    case 255:
        break;
    default:
        fn_80140AF8(self, 0, 0);
        break;
    }
    return result;
}
