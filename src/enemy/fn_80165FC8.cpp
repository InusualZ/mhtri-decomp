/* enemy/fn_80165FC8.cpp - the enemy per-area seat/action unit, 0x80165FC8..0x801679B0 (21 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup` on every one of the 21 addresses: the shared runtime dump answers
 * only `zz_<addr>_`, and `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX`
 * entries - no `__FILE__`/class string names a file here).
 *
 * What it is.  The range sits between the two registered enemy action/state units
 * (`enemy/fn_8015E854.cpp` ends exactly at 0x80165FC8, `enemy/fn_801679B0.cpp` starts exactly at
 * 0x801679B0) and continues the same family: it drives the shared `_ENEMY_WORK` record.
 *   * `fn_80165FC8` and `fn_80166DF8` are the per-area seat/entry selectors: they switch on
 *     `stage_map_kind_get(self->field_0x1E0)` (the map lookup) and on `self->area_no` (+0x1E1) and the
 *     entry state `self->field_0x9F6`, then call the motion setter `fn_80126324` with the per-area
 *     motion ids.
 *   * `fn_801663E4` is the big nested dispatch that maps (map, area, entry-state) to the same
 *     `fn_80126324` ids and reports whether the entry is still running.
 *   * `fn_801671AC` is the per-tick seat hook; the `fn_8016730C`..`fn_801678A0` set is the
 *     per-motion state step (`switch (self->state)` -> set a motion through `em_mot_set`/
 *     `em_mot_set_ck` and wait on `em_mot_end_ck`/`em_frame_check`), dispatched by `fn_80167404` /
 *     `fn_80167968` on `self->state_sub` (+0x1E6).
 *   * `fn_80166330` is the `.ctors` initializer that seeds the two global float vectors
 *     (`lbl_806A7868`, `lbl_806A7880`); the range owns `.ctors 0x8056F328..0x8056F32C`.
 *   * `fn_801661BC`, `fn_801661FC` and `fn_801662D4` are three methods of the `ResUserDataAc`
 *     user-data accessor (their addresses fall in this range, so they are emitted here): the
 *     `lbl_805A6D28` vtable slots +0x18, +0x2C and +0x08.  The table itself is another TU's data
 *     (docs/plan.md 6.5 rule 10: a table outside our ranges is referenced, not emitted), so these
 *     stay flat `extern "C"` functions and no class with virtuals is declared here.
 *
 * The seam is unproven (docs/plan.md 8.3).  It is exactly the unclaimed gap between the two
 * registered units, so both edges are their `.text` edges; the range is not contiguous with
 * `enemy/fn_8015E854.cpp`'s own unwritten follow-up queue and does not reuse its jump tables.
 *
 * Language: C++.  Every callee the range reaches through a mangling is declared at its real
 * signature (`em_frame_check`, `em_die_ck`, `em_act_ck`, `get_move_work_adrs`, `get_move_work_max`,
 * `ran_suu`, `rotMatrixX`, `rotMatrixZ`, `copyMat33`) - rule 9 never spells the mangling.  The
 * range's own flat symbols stay C-linkage through the `extern "C"` block in
 * `include/enemy/fn_80165FC8.h`.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy/ENEMY_WORK.h` (the one shared home; the fields
 * this range names were added there - +0x00C, +0x320, +0x608/+0x610, +0x834/+0x835).  The
 * `enemy_data_find` entry and the `ResUserDataAc` accessor are in this unit's own header.
 *
 * Flags: no deviation - the `enemy` lib's `cflags_main` measured every body below.  No `#pragma`.
 *
 * State of the reconstruction.  All 21 bodies are written; the per-symbol measurements are in the
 * outbox (`flags_probed` empty - no flag was needed).  The residual differences are recorded with
 * the function that carries them.
 */

#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_80165FC8.h"
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "stage/stg_w.h"

/* -------------------------------------------------------------------------------------------------
 * the range's functions, in address order
 * ------------------------------------------------------------------------------------------------- */

u32 fn_80165FC8(_ENEMY_WORK* self, u32 arg1) {
    s16* seat = &self->field_0x328;
    u8 entry;
    u32 sel;

    get_move_work_adrs(3);
    get_move_work_max(3);
    entry = stage_map_kind_get(self->field_0x1E0);
    if ((s32)entry != 2) {
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    sel = 0xFF;
    if ((s32)entry == 2) {
        sel = 3;
    }
    if (sel != 0xFF && (s32)fn_80129DB8(self) == 2) {
        return 1;
    }
    if (fn_8012ECF0(self) == 0 && self->value_0x452 >= 0x384) {
        u32 motion;
        if ((s32)self->area_no != 6) {
            if (fn_802B0998(3) == 1) {
                motion = 6;
            } else {
                motion = 4;
            }
        } else {
            motion = 6;
        }
        if (fn_8012A014(self, 0, motion, (u16)arg1, 0, lbl_805A5DB4) == 1) {
            return 1;
        }
    }
    if (fn_8012ECF0(self) == 0 && self->value_0x452 >= 0x384 && fn_8015D934(self) <= 2 && (s32)entry == 2
        && self->area_no != 3 && fn_80129A1C(self, (u16)arg1, 0x1E, seat) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 3;
        return 1;
    }
    if (fn_80129A70(self, (u16)arg1) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

void fn_801661BC(ResUserDataAc* self) {
    u8 pad[0x1C];

    fn_8005D1AC(pad, 0);
    fn_8013A654(self, 7);
}

void fn_801661FC(ResUserDataAc* self, MTX34* mtx, void* cursor, s32 arg3) {
    u32 head[1];
    s32 idx;
    MTX34 local;
    MTX34 out;

    (void)arg3;
    fn_8005D1AC(head, 0);
    MTX34_ctor(&out);
    MTX34_ctor(&local);
    idx = (s32)(u32)fn_80097EB0(cursor, 0x18);
    fn_8005D0CC(head, &idx);
    {
        _ENEMY_WORK* work = self->work;
        fn_800532DC(&local, &mtx[fn_8006FDCC(head)]);
        fn_800504D4(&out);
        rotMatrixX(work->field_0x608, &out);
        rotMatrixZ(work->field_0x610, &out);
        fn_80051574(&local, &out);
        copyMat33(&mtx[fn_8006FDCC(head)], &local);
    }
}

ResUserDataAc* fn_801662D4(ResUserDataAc* self, s32 flags) {
    if (self != 0) {
        fn_8013918C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

void fn_80166330(void) {
    VEC3 v0;
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    /* `setVec3` returns its first argument (the retail call site keeps it in r4 across the `bl`), so
     * the cast back to the callee's `VEC3*` costs no instruction. */
    fn_80051490(&lbl_806A7868[0], (VEC3*)setVec3(&v0, lbl_80797330, lbl_80797338, lbl_80797330));
    fn_80051490(&lbl_806A7868[1], (VEC3*)setVec3(&v1, lbl_80797330, lbl_807974EC, lbl_80797330));
    fn_80051490(&lbl_806A7880[0], (VEC3*)setVec3(&v2, lbl_80797330, lbl_80797330, lbl_80797330));
    fn_80051490(&lbl_806A7880[1], (VEC3*)setVec3(&v3, lbl_80797330, lbl_807974EC, lbl_80797330));
}

u32 fn_801663E4(_ENEMY_WORK* self) {
    u32 stale = 0;
    u8 map = stage_map_kind_get(self->field_0x1E0);

    switch (map) {
    case 1:
        switch (self->area_no) {
        case 1:
            fn_80126324(self, 2, 5, lbl_807974F0);
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x15, 0xA, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 9, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 9, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 0x11, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 0x11, lbl_807974F0);
                break;
            }
            break;
        case 10:
            fn_80126324(self, 4, 5, lbl_807974F0);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x13, 0x14, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 4, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 0x14, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 0x13, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 0x14, lbl_807974F0);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x24, 0x27, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 0x25, 0x26, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 6, lbl_807974F0);
                break;
            case 7:
                fn_80126324(self, 0x18, 7, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 6, lbl_807974F0);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 7:
                fn_80126324(self, 3, 4, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 7:
            switch (self->field_0x9F6) {
            case 5:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 10:
            fn_80126324(self, 1, 3, lbl_807974F4);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 4, 6, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 5, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            case 2:
                fn_80126324(self, 4, 6, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 5, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 7:
            fn_80126324(self, 0, 2, lbl_807974F4);
            return 1;
        case 9:
            fn_80126324(self, 0, 2, lbl_807974F4);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 8:
        if (self->area_no == 1) {
            fn_80126324(self, 0, 1, lbl_807974F8);
            return 1;
        }
        stale = 1;
        break;
    case 9:
        if (self->area_no == 0) {
            fn_80126324(self, 0, 1, lbl_807974F8);
            return 1;
        }
        stale = 1;
        break;
    default:
        stale = 1;
        break;
    }
    if (stale == 1) {
        self->pos.z = lbl_807974FC;
        self->pos.y = lbl_807974FC;
        self->pos.x = lbl_807974FC;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        fn_8012B380(self, 5, 2, 0xA);
    }
    return 0;
}

void fn_80166DF8(_ENEMY_WORK* self, u32 kind) {
    VEC3 origin;
    u8 a;
    u8 b;
    u32 running;
    u8 map;

    setVec3(&origin, lbl_807974FC, lbl_807974FC, lbl_807974FC);
    self->field_0x835 = 1;
    switch ((u8)kind) {
    case 0:
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->area_no == data->field_0x14) {
                    fn_80127308(self, &data->vec_0x24, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, lbl_807974F0, 0, (u16)ran_suu(0));
                } else {
                    fn_80127308(self, &origin, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, lbl_807974F0, 0, (u16)ran_suu(0));
                }
                self->field_0x1C4 = 0;
                self->field_0x1BC = 0;
                self->field_0x1C0 = (u16)ran_suu(1);
            }
        }
        break;
    case 2:
        fn_8016C674(self, &a, &b);
        fn_80128A8C(self, a, b);
        fn_80133BC0(self);
        break;
    case 3:
        map = stage_map_kind_get(self->field_0x1E0);
        running = 0;
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0 && fn_80176AA8(data->work_0x18) == 1) {
                switch (map) {
                case 1:
                    if (self->area_no == 2 && self->field_0x9F6 == 1) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 2:
                    if (self->area_no == 2 && self->field_0x9F6 == 1) {
                        running = 0;
                    } else if (self->area_no == 4
                               && (self->field_0x9F6 == 1 || self->field_0x9F6 == 6)) {
                        running = 0;
                    } else if (self->area_no == 5 && self->field_0x9F6 == 9) {
                        running = 0;
                    } else if (self->area_no == 6
                               && (self->field_0x9F6 == 4 || self->field_0x9F6 == 7)) {
                        running = 0;
                    } else if (self->area_no == 7 && self->field_0x9F6 == 6) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 4:
                    running = 1;
                    break;
                }
            }
        }
        if (running == 0) {
            if (fn_801663E4(self) == 0) {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 2, 0);
            } else {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 0xC, 1);
            }
        } else if (map == 2 && self->area_no == 4) {
            em_move_mode_set(self, 0);
            fn_80126324(self, 0x13, 0x14, lbl_80797500);
            fn_80128A8C(self, 0xC, 1);
        } else if (map == 2 && self->area_no == 5) {
            em_move_mode_set(self, 0);
            fn_80126324(self, 0x13, 0x15, lbl_807974F8);
            fn_80128A8C(self, 0xC, 1);
        } else {
            fn_8016C674(self, &a, &b);
            fn_80128A8C(self, a, b);
        }
        fn_80133BC0(self);
        break;
    }
}

void fn_801671A8(void) {
}

void fn_801671AC(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (self->field_0x834 == 1 || fn_801337FC(self) == 1) {
            fn_8012E664(self);
            fn_8013072C(self, 1, 0);
        }
        {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->field_0x46C == data->field_0x17) {
                    copyVec3(&self->aim, &data->vec_0x24);
                }
                if (self->field_0x011 == 0) {
                    if (data->work_0x18 != 0 && data->work_0x18->area_no == self->area_no
                        && em_act_ck(data->work_0x18, 1, 6) == 1
                        && em_frame_check(data->work_0x18, 0, lbl_80797504, lbl_807974FC) == 1
                        && self->field_0x916 <= 0) {
                        fn_8012CEB4(self, (s16)(ran_suu(0) & 0x1F), 0);
                    }
                }
                if (data->field_0x08 == 0xFF) {
                    self->field_0x46C = 0xFF;
                    fn_8013AAC4(self);
                    fn_8012B380(self, 6, 0xFF, 0);
                }
            }
        }
    }
}

void fn_8016730C(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167388(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xB, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167404(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        return fn_8016730C(self);
    case 1:
        return fn_80167388(self);
    case 2:
        return fn_8016730C(self);
    case 3:
        return fn_8016730C(self);
    case 4:
        return fn_8016730C(self);
    case 6:
        return fn_8016730C(self);
    }
}

void fn_80167458(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801674D4(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167550(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801675CC(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167648(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801676C4(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        fn_8012FCC4(self, 0x1A, lbl_807974FC);
        break;
    case 1:
        if (self->field_0x1D4 <= lbl_807974FC) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_80167770(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80797508, lbl_807974FC) == 1) {
            self->state = self->state + 1;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->field_0x314 = lbl_8079750C;
            self->field_0x318 = lbl_80797510;
            self->field_0x320 = lbl_80797514;
            fn_8012FCC4(self, 0x10, lbl_807974FC);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x1D4 <= lbl_807974FC || self->pos.y <= self->field_0x20C) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_801678A0(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 8, 0);
        fn_8012FCC4(self, 0x1E, lbl_807974FC);
        self->timer_0x020 = 0x20;
        break;
    case 1:
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= lbl_807974FC) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_80167968(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        return fn_80167458(self);
    case 2:
        return fn_801674D4(self);
    case 3:
        return fn_80167550(self);
    case 4:
        return fn_801675CC(self);
    case 5:
        return fn_80167648(self);
    case 6:
        return fn_801676C4(self);
    case 7:
        return fn_80167770(self);
    case 8:
        return fn_801678A0(self);
    }
}
