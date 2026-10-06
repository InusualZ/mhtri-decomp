/*
 * enemy/em011_prog.cpp - enemy 011's program: the per-action `_ENEMY_WORK` state steps, their dispatchers and
 *   the static initializer.
 * RANGE. .text 0x8016D1C4-0x80170600 (56 functions); extab 0x8000E3DC-0x8000E52C, extabindex
 *   0x800294B4-0x800296AC, .ctors 0x8056F330-0x8056F334, .rodata 0x8056FCD0-0x8056FD10, .data 0x805A7CE8-0x805A8370
 *   (`em011_prog_tbl` first), .bss 0x806A7970-0x806A79A0, .sdata2 0x807977D8-0x80797910.
 * FLAGS. `cflags_main`, no `#pragma`.
 * NAMES. The file name follows the runtime dump's `em011_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.
 *   The `.bss` record names (`vec_pair_801679B0_9`/`_10`) are GUESSes.
 * RESIDUALS. 28 rows unwritten: 0x8016D1C4-0x8016D22C, 0x8016D234-0x8016D384, 0x8016D9EC-0x8016DBE8,
 *   0x8016DC3C-0x8016E3A0, 0x8016E444-0x8016E544, 0x8016E6EC-0x8016E7D4, 0x8016E824-0x8016EE00,
 *   0x8016EEE8-0x8016F044, 0x8016F0A0-0x8016F9AC, 0x8016FA48-0x801704AC, 0x801704B0-0x801704CC,
 *   0x8017054C-0x80170600.
 *   3 partial rows:
 *  - `fn_8016E610`, `fn_8016EE00`: retail re-masks the `u8` argument (`clrlwi r0,r4,24`) before the compares, ours
 *    compares r4, and ours emits one more `b`;
 *  - `fn_8016E544`: ours branches to a shared `em_mot_end_ck` call where retail calls it in place.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata2` claimed, not emitted; `.data` 0x9C against 0x688; `.text` (0xBC8 of
 *   0x343C), extab (0x88 of 0x150) and extabindex (0xCC of 0x1F8) short of the claim and differing.
 */

#include "enemy/fn_80128AAC.h" /* fn_80128AAC (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "unsplit/enemy.h"
#include "enemy/fn_8012BDF4.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80128AAC_c1 ((void (*)(_ENEMY_WORK *, s32, s32))fn_80128AAC)
#define em_hit_window_set_default_c1 ((void (*)(_ENEMY_WORK *, s32, s32))em_hit_window_set_default)

/* The one mangled callee, declared by its real signature so the front-end emits the map's
 * `em_frame_check__FP11_ENEMY_WORKUsff` (rule 9). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {

extern void fn_8016D414(_ENEMY_WORK *self);
extern void fn_8016D490(_ENEMY_WORK *self);

extern u8 stage_map_kind_get(u8 a);

extern void fn_8016DC3C(_ENEMY_WORK *self, s32 a, s32 b);
extern void fn_8016DE40(_ENEMY_WORK *self, s32 a);
extern void fn_8016DEF8(_ENEMY_WORK *self);
extern void fn_8016E088(_ENEMY_WORK *self, s32 a);
extern void fn_8016E2C8(_ENEMY_WORK *self);

extern void fn_8016D53C(_ENEMY_WORK *self);
extern void fn_8016D5B8(_ENEMY_WORK *self);
extern void fn_8016D63C(_ENEMY_WORK *self);
extern void fn_8016D6C0(_ENEMY_WORK *self);
extern void fn_8016D744(_ENEMY_WORK *self);
extern void fn_8016D7C8(_ENEMY_WORK *self);
extern void fn_8016D84C(_ENEMY_WORK *self);
extern void fn_8016D8D8(_ENEMY_WORK *self);
extern void fn_8016D95C(_ENEMY_WORK *self);
extern void fn_8016D9EC(_ENEMY_WORK *self);
extern void fn_8016DB20(_ENEMY_WORK *self);
extern void fn_8016DBE8(_ENEMY_WORK *self);
extern void fn_8016E3A0(_ENEMY_WORK *self);
extern void fn_8016E444(_ENEMY_WORK *self);
extern void fn_8016E544(_ENEMY_WORK *self);
extern void fn_8016E610(_ENEMY_WORK *self, u8 a);
extern void fn_8016E6EC(_ENEMY_WORK *self);
extern void fn_8016E7D4(_ENEMY_WORK *self);
extern void fn_8016E824(_ENEMY_WORK *self);
extern void fn_8016EB4C(_ENEMY_WORK *self);
extern void fn_8016EBF4(_ENEMY_WORK *self, s32 a);
extern void fn_8016EE00(_ENEMY_WORK *self, u8 a);
extern void fn_8016EEE8(_ENEMY_WORK *self, s32 a);
extern void fn_8016F044(_ENEMY_WORK *self);
extern void fn_8016F180(_ENEMY_WORK *self);
extern void fn_8016F3FC(_ENEMY_WORK *self);
extern void fn_8016F4B0(_ENEMY_WORK *self);
extern void fn_8016F630(_ENEMY_WORK *self);
extern void fn_8016F6E4(_ENEMY_WORK *self);
extern void fn_8016F8F8(_ENEMY_WORK *self);
extern void fn_8016F9AC(_ENEMY_WORK *self);
extern void fn_8016FA00(_ENEMY_WORK *self);

extern f32 lbl_807977DC;
extern f32 lbl_807977E0;
extern f32 lbl_8079785C;
extern f32 lbl_80797860;
}

extern "C" s32 fn_801704CC(_ENEMY_WORK *self) {
    return self->field_0x1E2 == 0;
}

extern "C" void fn_8016D22C(void) {}

extern "C" void fn_8016D230(void) {}

extern "C" void fn_801704AC(void) {}

extern "C" void fn_8016D50C(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016D414(self);
        return;
    case 1:
        fn_8016D490(self);
        return;
    case 2:
        fn_8016D414(self);
        return;
    }
}

extern "C" void fn_8016D384(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    fn_80128AAC_c1(self, 1, 3);
    fn_80133BB4(self);
}

extern "C" void fn_8016D3CC(_ENEMY_WORK *self) {
    em_move_mode_set(self, 0);
    fn_80128AAC_c1(self, 1, 2);
    fn_80133BB4(self);
}

extern "C" void fn_8016D414(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D490(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D53C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016E7D4(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016E444(self);
        return;
    case 2:
        fn_8016E544(self);
        return;
    case 3:
        fn_8016E610(self, 0);
        return;
    case 4:
        fn_8016E6EC(self);
        return;
    case 5:
        fn_8016E610(self, 1);
        return;
    }
}

extern "C" void fn_8016F044(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016EBF4(self, 0);
        return;
    case 1:
        fn_8016EE00(self, 0);
        return;
    case 2:
        fn_8016EBF4(self, 1);
        return;
    case 3:
        fn_8016EEE8(self, 1);
        return;
    case 4:
        fn_8016EE00(self, 1);
        return;
    }
}

extern "C" void fn_8016F9AC(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016F180(self);
        return;
    case 1:
        fn_8016F3FC(self);
        return;
    case 2:
        fn_8016F4B0(self);
        return;
    case 3:
        fn_8016F630(self);
        return;
    case 4:
        fn_8016F6E4(self);
        return;
    case 5:
        fn_8016F8F8(self);
        return;
    }
}

extern "C" void fn_8016DBE8(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 1:
        fn_8016D53C(self);
        return;
    case 2:
        fn_8016D5B8(self);
        return;
    case 3:
        fn_8016D63C(self);
        return;
    case 4:
        fn_8016D6C0(self);
        return;
    case 5:
        fn_8016D744(self);
        return;
    case 6:
        fn_8016D7C8(self);
        return;
    case 7:
        fn_8016D84C(self);
        return;
    case 8:
        fn_8016D8D8(self);
        return;
    case 9:
        fn_8016D95C(self);
        return;
    case 10:
        fn_8016D9EC(self);
        return;
    case 11:
        fn_8016DB20(self);
        return;
    }
}

extern "C" void fn_8016FA00(_ENEMY_WORK *self) {
    switch (self->action) {
    case 0:
        fn_8016D50C(self);
        return;
    case 1:
        fn_8016DBE8(self);
        return;
    case 2:
        fn_8016E3A0(self);
        return;
    case 7:
        fn_8016E7D4(self);
        return;
    case 10:
        fn_8016E824(self);
        return;
    case 11:
        fn_8016EB4C(self);
        return;
    case 12:
        fn_8016F044(self);
        return;
    case 13:
        fn_8016F9AC(self);
        return;
    }
}

extern "C" s32 fn_801704DC(_ENEMY_WORK *self) {
    if ((u32)(stage_map_kind_get(self->field_0x1E0) - 1) <= 1U && self->field_0x011 != 0) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 7;
        self->field_0x1FF = 0;
        return 1;
    }
    return 0;
}

extern "C" void fn_8016D5B8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x18, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D3CC(self);
        }
        return;
    }
}

extern "C" void fn_8016D63C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 8, 0x5E);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D6C0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D744(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D7C8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D3CC(self);
        }
        return;
    }
}

extern "C" void fn_8016D8D8(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1F, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016D84C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1A, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801321D0(self);
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016D95C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x19, 4, 0);
        fn_801321C4(self);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807977E0, lbl_807977DC) == 1) {
            fn_8016D384(self);
        }
        return;
    }
}

extern "C" void fn_8016E3A0(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8016DC3C(self, 0, 0);
        return;
    case 1:
        fn_8016DC3C(self, 1, 1);
        return;
    case 2:
        fn_8016DC3C(self, 2, 1);
        return;
    case 3:
        fn_8016DC3C(self, 3, 1);
        return;
    case 4:
        fn_8016DC3C(self, 4, 1);
        return;
    case 5:
        fn_8016DE40(self, 0);
        return;
    case 6:
        fn_8016DEF8(self);
        return;
    case 7:
        fn_8016E088(self, 0);
        return;
    case 8:
        fn_8016E2C8(self);
        return;
    case 9:
        fn_8016E088(self, 1);
        return;
    case 10:
        fn_8016DC3C(self, 5, 1);
        return;
    case 11:
        fn_8016DE40(self, 1);
        return;
    case 12:
        fn_8016DC3C(self, 6, 1);
        return;
    }
}

extern "C" void fn_8016E544(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_hit_window_set_default_c1(self, 0, 1);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079785C, lbl_807977DC) == 1) {
            self->state += 1;
            em_hit_window_set_default_c1(self, 0, 2);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_8016E610(_ENEMY_WORK *self, u8 a) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_hit_window_set_default_c1(self, 0, 3);
        return;
    case 1:
        switch (a) {
        case 0:
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
                return;
            }
            return;
        case 1:
            if (em_frame_check(self, 1, lbl_80797860, lbl_807977DC) == 1) {
                self->state += 1;
                em_state_set(self, 7, 4);
            }
            break;
        }
        break;
    }
}

extern "C" void fn_8016EE00(_ENEMY_WORK *self, u8 a) {
    if (a == 1) {
        em_frame_flag_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xA, 8, 0);
        em_approach_start(self, lbl_807977DC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state += 1;
            em_mot_set(self, 0xB, 2, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The unit's `.bss`: the two two-vector records `fn_8017054C` seeds.  Names are GUESSes. */
VEC3 vec_pair_801679B0_9[2];  /* +0x806A7970 */
VEC3 vec_pair_801679B0_10[2];  /* +0x806A7988 */
