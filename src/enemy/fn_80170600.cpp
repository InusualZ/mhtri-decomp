/* src/enemy/fn_80170600.cpp - one `_ENEMY_WORK` action family, `.text` 0x80170600..0x80170FA8 (18 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap: every
 * `dump` name in the range is `zz_XXXXXXXX_`, and the one real dump hit, `l2cu_release_rcb`, is flagged
 * `ambiguous` in dumpmap-join.json - the same name is proposed for two addresses - so it is not evidence).
 *
 * What it is.  The unit is the enemy AI action-state family that hangs off `_ENEMY_WORK`.
 *   * `fn_80170A00` is the dispatcher: it switches on `state_sub` (the action slot) and tail-calls one
 *     of the per-slot step functions (`fn_8017088C`/`fn_80170908`/`fn_80170984`).
 *   * `fn_8017088C`, `fn_80170908`, `fn_80170984`, `fn_80170A54`, `fn_80170AD0`, `fn_80170B4C`,
 *     `fn_80170C68`, `fn_80170D04`, `fn_80170D74`, `fn_80170DF0`, `fn_80170E78`, `fn_80170EF4` are the
 *     per-action step machines: they switch on `state`, advance it, arm the action through
 *     `fn_80130478`/`fn_8012F5B8`/`fn_8012F62C`, then wait on `fn_8012F93C`/`em_frame_check` and finish
 *     through `fn_80127F48`.
 *   * `fn_80170600` clears the two `field_0x328`/`field_0x32A` timers; `fn_80170610` is the slot-2 setup
 *     (reads the action byte pair through `fn_80176090`, then seats the position through `setVector3`);
 *     `fn_801706B8` arms a status effect through `fn_80141B88`; `fn_80170758` reacts to an event byte;
 *     `fn_80170804` runs the slot-0 idle tick.
 *
 * Language.  C++: `nw4r::math::setVector3` is declared for C++ only (`nw4r/math.h`), and the calls the
 * target makes to `em_die_ck__FP11_ENEMY_WORK`/`em_frame_check__FP11_ENEMY_WORKUsff` are the manglings of
 * the C++ functions `em_die_ck`/`em_frame_check` (mangle.py verified).  The unit's own symbols are plain
 * (`fn_80170600`, no mangling), so they carry `extern "C"`.
 *
 * Types.  `_ENEMY_WORK` lives in its one shared home, `enemy/ENEMY_WORK.h` (docs/plan.md 6.5 rule 1).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_80170600.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80171194.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "ef/eft_slot.h"     /* enemy_data_find / enemy_data_grp (their owner's header) */

#pragma peephole off

#ifdef __cplusplus
extern "C" {
#endif

/* The record `enemy_data_find` looks up; only the flag byte at +0x08 is read here.
 * size: 0x09 (approximate - only +0x08 is observed) */
typedef struct ENEMY_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u8 field_0x08;
} ENEMY_ENTRY;

/* ---- foreign callees the owner's header does not declare yet ---- */

extern void fn_80128A8C(_ENEMY_WORK *self, u8 a, u8 b);
/* `enemy_data_grp`/`enemy_data_find` (0x803439D4 / 0x803438E4) come from their owner's header,
 * `include/ef/eft_slot.h` (rule 2). */

/* ---- the range's functions ---- */


void fn_80170600(_ENEMY_WORK *self) {
    self->field_0x32A = 0;
    self->field_0x328 = 0;
}

void fn_80170610(_ENEMY_WORK *self, u8 a) {
    Vec3 v;
    u8 b, c;

    fn_80043EA8(&v);
    if ((a & 0xFF) == 2) {
        fn_80176090(self, &b, &c);
        fn_80128A8C(self, b, c);
        fn_80133BC0(self);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, 0.0f, 45.0f);
        fn_801057A4(self, 0x14, &v, 0.7f, 0x1C72);
    }
}

void fn_801706B8(_ENEMY_WORK *self);

void fn_801706B8(_ENEMY_WORK *self) {
    if (self->team == 0xC) {
        fn_80141B88(self->field_0x01A, 0xA, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    } else {
        fn_80141B88(self->field_0x01A, 0xD, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    }
}

void fn_80170758(_ENEMY_WORK *self, u8 a, u8 b) {
    if ((a & 0xFF) == 1) {
        switch (b) {
        case 5:
            fn_80130F74(self);
            break;
        case 0xA:
            self->field_0x32A = 0;
            break;
        case 0xC:
            if (fn_8012D1A0(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xD:
            if (fn_8012D1A0(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xE:
            fn_801376B4(self);
            break;
        }
    }
}

void fn_80170804(_ENEMY_WORK *self) {
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (em_die_ck(self) == 0) {
        u8 v = (u8)enemy_data_grp(self->team, self->field_0x00A);
        ENEMY_ENTRY *entry = (ENEMY_ENTRY *)enemy_data_find(v, self->field_0x46C);
        if (entry != 0 && entry->field_0x08 == 0xFF) {
            self->field_0x46C = 0xFF;
            fn_8013AAC4(self);
        }
    }
}

void fn_8017088C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 1, 0xA, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170908(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 2, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170984(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xE, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170A00(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8017088C(self);
        break;
    case 1:
        fn_80170908(self);
        break;
    case 2:
        fn_80170984(self);
        break;
    case 3:
        fn_8017088C(self);
        break;
    case 4:
        fn_8017088C(self);
        break;
    case 6:
        fn_8017088C(self);
        break;
    }
}

void fn_80170A54(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 9, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170AD0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xF, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170B4C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x18, 2, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        f32 t = 0.4f;
        fn_8013221C(self, t, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            fn_8012F5B8(self, 0x71, 4, 0);
            fn_80132264(self);
        }
    }
        break;
    case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170C68(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC8, 4, 0);
        self->timer_0x020 = 0x84;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        } else {
            self->state++;
            fn_80128A14(self, 1, 5);
        }
        break;
    }
}

void fn_80170D04(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_8012F5B8(self, 0xCA, 4, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170D74(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC9, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170DF0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 138.0f, 0.0f) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170E78(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0x71, 0, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

void fn_80170EF4(_ENEMY_WORK *self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC9, 6, 0);
        break;
    case 1:
        if ((u8)a == 1) {
            if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
                fn_80128A14(self, 1, 0xC);
            }
        } else {
            if (fn_8012F93C(self) == 1) {
                fn_80127F48(self);
            }
        }
        break;
    }
}

#ifdef __cplusplus
}
#endif
