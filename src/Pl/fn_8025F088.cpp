/*
 * Player per-frame control cluster (proposal 8025F088): `.text` 0x8025F088-0x80262940 (17 functions,
 * 0x38B8 B) with its own exception tables - extab 0x8001239C-0x8001241C (16 records) and extabindex
 * 0x8002F454-0x8002F514 (16 records).  The extab run starts exactly at fn_8025F088 and ends exactly
 * after fn_80262688, so both seams are real TU boundaries, not caps.
 *
 * Registration (docs/plan.md 12, evidence class 3 + 4):
 *   - no `__FILE__` string covers the range: the only strings its data touches are the `.sdata2`
 *     float pool 0x80799E00-0x80799FDC and `.bss` `pl_move_work`, and no source-name string is
 *     referenced from the range at all (class 1 out);
 *   - `dumpmap.py lookup` answers only `zz_` placeholders for all 17 symbols (class 2 out);
 *   - the code is the `Pl` module: every actor parameter is a `_PLW` (`Pl_master_ck`, `Pl_act_ck`,
 *     `Pl_Skill_ck`, `Pl_cat_skill_ck`, `Get_motion_no`), it reads the move work
 *     `get_move_work_adrs`/`get_move_work_max` and the `pl_move_work` chunk table, and its siblings
 *     are `Pl/fn_80241558.cpp` (before) and `Pl/fn_80262940.cpp` (after);
 *   - all 17 map entries carry only the `fn_XXXXXXXX` stem, so the file keeps the stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8025F088 0x80262940`; all 17 entries are the map's
 * placeholders and `dumpmap.py lookup` answers `zz_` for each).
 *
 * Flags: the unit uses `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * `mw_version Wii/1.0`), the Pl lib's measured flag set - the extab/extabindex pair above is what
 * `-Cpp_exceptions on` reproduces.
 *
 * Rule 2 debt (recorded, not hidden): `include/Pl/fn_8025F088.h` declares the callees this unit needs
 * whose owners' headers do not declare them (the `enemy/fn_80165FC8.h` precedent).  Moving each to
 * its owner's header is a later pass; `include/unsplit/Pl.h`'s five declarations that this
 * registration makes owned (`fn_8025FA00`, `fn_80260A18`, `fn_80261770`, `fn_802621B0`,
 * `fn_80262688`) were removed here.
 *
 * Residual: see `.pi/notes/8025f088-fn-8025f088-eedb.md` for the per-function measurements and the
 * functions still unwritten.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/fn_8025F088.h"
#include "Pl/fn_802693C4.h"   /* `Get_motion_no`, owned by Pl/fn_802693C4.cpp - the band header's
                          * copy is gone now that the owner's registration declares it (rule 2) */
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "unsplit/Pl.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 pool 0x80799E00-0x80799F98 (rule 2) */
#include "Pl/pl_act_data.h" /* the owner of the pool's second run, 0x80799F98-0x80799FDC (rule 2) */
#include "Pl/bss_pool.h" /* the owner of the `.bss` move-work table `pl_move_work` (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */

/* One entry of `pl_move_work` is `PlMoveEntry`, declared by its owner's header (rule 1/2). */

/* One 2840-byte (0xB18) move-work record `get_move_work_adrs` hands back; the motion slot `fn_802607C4`
 * scores sits at +0x188 and the entry's own kind byte at +0x1E1. size: 0xB18 */
typedef struct PlMoveWork {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 pad_0x002[0x186];
    /* +0x188 */ VEC3 pos_0x188;
    /* +0x194 */ u8 pad_0x194[0x4D];
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 pad_0x1E2[0x936];
} PlMoveWork; /* size: 0xB18 */

/* 0x8025F478 - enters the water-surface (swim) state: reads three motion slots and latches the
 * resulting depth, state byte and the two water flags. */
void fn_8025F478(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 0;
        return;
    }
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 3;
        if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
            self->field_0x064 = value;
            self->field_0x074 = 1;
            if (self->vec_0x03C.y >= value - pl_frame_window_30) {
                self->vec_0x03C.y = value - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        } else {
            self->field_0x074 = 0;
        }
    }
}

/* 0x80260198 - looks for an armed entry in the actor's own chunk of the move-work table and adopts
 * its position pair as the two damage counters. */
s32 fn_80260198(_PLW* self) {
    s32 i;

    if ((self->field_0x364 & 0xE0000007) != 0) {
        for (i = 0; i < 10; i++) {
            PlMoveEntry* entry = &pl_move_work[self->chunk_ofs][i];
            u16 kind = entry->kind;
            u32 value;

            if (kind == 0) {
                break;
            }
            value = fn_80050A40(entry->x_0x08, entry->z_0x10, pl_float_zero, pl_float_zero);
            if ((kind & 2) != 0) {
                self->field_0x058 = value;
                self->field_0x0A8 = value;
                return 1;
            }
        }
    }
    return 0;
}

/* 0x80260248 - advances the actor's signed 16-bit lean value by `delta`, clamps it to +/-100 and
 * zeroes it when `mode` 1 asks for the sign to be cancelled. */
void fn_80260248(_PLW* self, s32 mode, s16 delta) {
    s16 old = self->field_0x648;

    self->field_0x648 += delta;
    if (self->field_0x648 <= -100) {
        self->field_0x648 = -100;
    } else if (self->field_0x648 >= 100) {
        self->field_0x648 = 100;
    }
    if (mode != 1) {
        return;
    }
    if ((s32)old * (s32)self->field_0x648 < 0) {
        self->field_0x648 = 0;
    }
}

/* 0x8026077C - whether the two state bytes disagree while the game is outside play mode 3. */
s32 fn_8026077C(u8 a, u8 b) {
    if (a == b || (u8)PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* 0x8026099C - runs the actor's per-frame body for the current mode byte, when the slot is live. */
void fn_8026099C(_PLW* self) {
    if (self->slot_active == 0) {
        return;
    }
    switch (self->field_0x004) {
    default:
        break;
    case 0:
        fn_8024676C();
        break;
    case 1:
        fn_8026F7B4();
        if ((u8)fn_803C482C() == 0 && event_demo_ck() == 0) {
            fn_8025F088(self);
        }
        break;
    }
}

/* 0x80260A18 - whether the actor's skill 382 is active. */
s16 fn_80260A18(_PLW* self) {
    s16 active = 0;

    if (fn_802731B4(self, 382) > 0) {
        active = 1;
    }
    return active;
}

/* 0x80260A58 - the gate that keeps the actor out of the skill-382 action. */
s32 fn_80260A58(_PLW* self) {
    if (self->field_0x3EA > 0) {
        return 0;
    }
    if (fn_80278578(self, 2) == 0) {
        return 0;
    }
    if (fn_8027D40C(self) > 0) {
        return 0;
    }
    if ((self->field_0x5A4 & 0x200) != 0 && Pl_cat_skill_ck(self, 47) == 0) {
        return 0;
    }
    if (self->field_0x404 > 0) {
        return 0;
    }
    if (fn_80276800(self, 0) == 1) {
        return 0;
    }
    return fn_80260A18(self) <= 0;
}

/* 0x8025FF0C - computes the action flag word for the current state and hands it to the action layer. */
void fn_8025FF0C(_PLW* self, u8 mode) {
    u16 flags = 0x102;

    switch (self->kind_0x09) {
    default:
        if (fn_8026FE98((_ENEMY_WORK*)self, 4544) == 0) {
            flags |= 0x40;
        } else if (self->field_0x00A != 0) {
            flags |= 0xC0;
        } else if ((u32)(self->act_no - 128) <= 6 || (u32)(self->act_no - 15) <= 1 ||
                   self->act_no == 123 || self->act_no == 139 || self->act_no == 182) {
            flags |= 0x40;
        } else {
            flags |= 0xC0;
        }
        break;
    case 2:
        if (self->field_0x00A == 6) {
            flags |= 0x40;
        } else if (Pl_act_ck(self, 0, 62) == 1) {
            flags |= 0x40;
        } else {
            flags |= 0xC0;
        }
        break;
    case 3:
        if (fn_8026FE98((_ENEMY_WORK*)self, 4544) == 0) {
            flags |= 0x80;
        } else if (self->field_0x00A != 0) {
            flags |= 0xC0;
        } else if ((u32)(self->act_no - 140) <= 9 || (u32)(self->act_no - 25) <= 1 ||
                   self->act_no == 124) {
            flags |= 0x80;
        } else if (self->act_no == 27) {
            if (self->field_0x002 == 3 && self->field_0x18 == 1) {
                flags |= 0x80;
            } else {
                flags |= 0xC0;
            }
        } else {
            flags |= 0xC0;
        }
        break;
    }
    if (self->field_0x00A == 0 &&
        ((u32)(self->act_no - 10) <= 4 || (u32)(self->act_no - 74) <= 4 ||
         (u32)(self->act_no - 102) <= 3)) {
        flags &= ~0x100;
    }
    fn_802950D8(self, mode, flags);

    self->field_0x369 = 0;
    if (self->field_0x364 != 0) {
        s32 i;

        for (i = 0; i < 10; i++) {
            u16 kind = pl_move_work[self->chunk_ofs][i].kind;

            if (kind == 0) {
                break;
            }
            if ((kind & 2) != 0 && (self->field_0x364 & 0xE0000007) != 0) {
                self->field_0x369 = 1;
            }
        }
    }
}

/* 0x802607C4 - steers the actor's stored angle toward the nearest armed move-work slot's bearing. */
void fn_802607C4(_PLW* self) {
    PlMoveWork* best_work;
    s32 found = 0;

    if ((u16)Get_motion_no(self) == 38) {
        f32 best = pl_float_2250000;
        PlMoveWork* work = (PlMoveWork*)get_move_work_adrs(3);
        u16 count = (u16)get_move_work_max(3);
        s32 i;

        for (i = 0; i < count; i++) {
            if (work->field_0x000 != 0 && work->field_0x001 != 0 &&
                fn_8026077C(self->area_0x16, work->field_0x1E1) != 0) {
                f32 distance = fn_80050EAC(&self->vec_0x03C, &work->pos_0x188);

                if (best >= distance) {
                    best = distance;
                    best_work = work;
                    found = 1;
                }
            }
            work++;
        }
    }
    if (found == 0) {
        u16 value = self->field_0x0B2;

        if (value == 0) {
            return;
        }
        if ((u32)(value + 1024) <= 2048) {
            self->field_0x0B2 = 0;
        } else if ((s16)value >= 0) {
            self->field_0x0B2 = value - 1024;
        } else {
            self->field_0x0B2 = value + 1024;
        }
        return;
    }
    {
        s32 diff = ((s32)(u16)calcVecAng2(&self->vec_0x03C, &best_work->pos_0x188) + 0x10000) -
                   (s32)self->field_0x058;
        u16 current = self->field_0x0B2;

        if ((u16)diff >= 32768) {
            s32 target;

            if ((u16)diff < 54614) {
                diff = 54614;
            }
            target = current - 2048;
            if (current >= 32768) {
                if (target > (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        } else {
            s32 target;

            if ((u16)diff > 10923) {
                diff = 10923;
            }
            target = current + 2048;
            if (current < 32768) {
                if (target < (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        }
    }
}

/* 0x8025FA00 - the second half of the player's water-surface state machine (the entry/exit side). */
void fn_8025FA00(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_master_ck(self) == 1 && self->field_0x00A != 8) {
                pl_act_enter(self, 1, 21, 12);
            }
            break;
        default:
            if (self->field_0x585 != 0) {
                break;
            }
            if (self->vec_0x03C.y - value <= pl_frame_window_30) {
                self->vec_0x03C.y = value;
                self->field_0x585 = 0;
                if (fn_8027DCA8(self) == 1 || (self->field_0x5A4 & 0x1000) == 0) {
                    break;
                }
                if (self->field_0x00A == 8) {
                    self->field_0x06C -= pl_frame_window_2;
                    if (self->field_0x06C < pl_float_neg250) {
                        self->field_0x06C = pl_float_neg250;
                    } else {
                        self->vec_0x03C.z += pl_frame_window_10;
                    }
                } else if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                    fn_80278BE4(self);
                    return;
                } else {
                    pl_act_enter(self, 6, 52, 0);
                }
                break;
            }
            if (self->field_0x370 > 0) {
                self->kind_0x09 = 2;
                if (self->field_0x00A != 12) {
                    pl_act_enter(self, 2, 1, 0);
                } else {
                    pl_act_enter(self, 12, 6, 0);
                }
            } else if (self->field_0x00A == 8) {
                self->vec_0x03C.y = value;
            }
            break;
        }
    } else if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (Pl_master_ck(self) != 0 &&
                fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value && fn_8027D7EC(self, 0) == 0) {
                    self->vec_0x03C.y = self->field_0x064;
                    self->kind_0x09 = 3;
                    if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                        fn_80278BE4(self);
                    } else {
                        pl_act_enter(self, 0, 151, 12);
                    }
                }
            }
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_act_ck(self, 0, 151) == 0 &&
                       self->vec_0x03C.y < pl_frame_window_100 + self->ground_y_0x060) {
                self->vec_0x03C.y = pl_frame_window_100 + self->ground_y_0x060;
            }
            break;
        default:
            if (Pl_master_ck(self) != 0 &&
                fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value) {
                    if (self->field_0x00A != 8) {
                        self->kind_0x09 = 3;
                        pl_act_enter(self, 1, 20, 12);
                    }
                } else if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if (self->field_0x00A != 12) {
                        pl_act_enter(self, 2, 1, 0);
                    } else {
                        pl_act_enter(self, 12, 6, 0);
                    }
                }
            }
            break;
        }
    }
    if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        if (self->kind_0x09 == 3) {
            self->field_0x074 = 1;
            if (self->field_0x00A == 0) {
                if ((u32)(self->act_no - 74) > 3 && (self->act_no < 104 || self->act_no > 105) &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->field_0x00A == 1) {
                if (self->act_no != 21 &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        }
    } else if (self->kind_0x09 == 3) {
        self->field_0x074 = 0;
    }
}

/* 0x8025F588 - the player's water-surface state machine: reads the swim motion slots and drives the
 * actor between the standing / swimming / diving kinds. */
s32 fn_8025F588(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if ((self->field_0x5C4 & 0xF) != 0) {
                self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                fn_802756F0(self, 12, 6, 0);
                return 1;
            }
            if (fn_80278C7C(self) == 1) {
                fn_802756F0(self, 6, 49, 0);
                return 1;
            }
            if (fn_80278CD0(self) == 1) {
                fn_802756F0(self, 6, 69, 0);
                return 1;
            }
            self->kind_0x09 = 0;
            break;
        case 3:
            self->kind_0x09 = 0;
            break;
        default:
            if (self->vec_0x03C.y - value > pl_frame_window_30) {
                if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        fn_802756F0(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        fn_802756F0(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        fn_802756F0(self, 6, 69, 0);
                        return 1;
                    }
                    pl_act_enter(self, 2, 1, 0);
                    return 1;
                }
                if (self->field_0x00A == 8) {
                    self->vec_0x03C.y = value;
                }
                pl_act_enter(self, 2, 1, 0);
            }
            break;
        }
    } else if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                if (self->vec_0x03C.y <= value) {
                    self->kind_0x09 = 3;
                } else {
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        fn_802756F0(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        fn_802756F0(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        fn_802756F0(self, 6, 69, 0);
                        return 1;
                    }
                }
            }
            break;
        default:
            self->kind_0x09 = 3;
            break;
        case 3:
            break;
        }
    }
    if ((u8)(self->kind_0x09 - 2) <= 1 &&
        fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        self->field_0x074 = 1;
        if (self->vec_0x03C.y >= pl_frame_window_200 + value) {
            self->field_0x075 = 0;
            self->kind_0x09 = 2;
            if ((self->field_0x5C4 & 0xF) != 0) {
                pl_act_enter(self, 12, 6, 0);
            } else if (fn_80278C7C(self) == 1) {
                fn_802756F0(self, 6, 49, 0);
            } else if (fn_80278CD0(self) == 1) {
                fn_802756F0(self, 6, 69, 0);
            } else {
                pl_act_enter(self, 2, 1, 0);
            }
            return 1;
        }
        if (self->vec_0x03C.y >= value - pl_frame_window_30) {
            self->vec_0x03C.y = value - pl_frame_window_30;
        }
        if (Pl_suimen_ck(self) == 1) {
            self->field_0x075 = 1;
        } else {
            self->field_0x075 = 0;
        }
    } else if ((u8)(self->kind_0x09 - 2) <= 1) {
        self->field_0x074 = 0;
    }
    return 0;
}

/* The player body sub-object `_PLW::physics_0x13C` points at: its `MHchar` block sits at +0x04 (the
 * layout `ef/fn_80114E34.cpp` owns as `_PLW_PHYSICS`).  A local view, because that owner keeps its
 * definition in its own `.cpp`; moving it to a shared header is the rule-1 debt this unit records.
 * size: 0x144 (lower bound: the `MHchar` it holds is 0x140) */
typedef struct PlBodyWork {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr_0x04;
} PlBodyWork; /* size: 0x144 */

/* One item-data record `GetItemData` hands back; only the kind byte at +0x01 is read here.
 * size: 0x02 (lower bound: +0x01 is the highest byte this unit names) */
typedef struct PlItemData {
    /* +0x00 */ u8 pad_0x00[0x1];
    /* +0x01 */ u8 field_0x01;
} PlItemData; /* size: 0x02 */

/* 0x80262688 - adopts the nearest live player work as the actor's held item when the item-data kind
 * allows it, and reports whether one was adopted. */
s32 fn_80262688(_PLW* self) {
    _PLW* work;
    u16 count;
    s32 i;

    if (self->field_0x658 != 0) {
        return 0;
    }
    work = (_PLW*)get_move_work_adrs(2);
    count = (u16)get_move_work_max(2);
    for (i = 0; i < count; i++, work++) {
        if (work->slot_active != 0 &&
            (isServerSelectState() == 1 ? Pl_master_ck(work) != 1 : work->chunk_ofs != self->chunk_ofs) &&
            (work->field_0x655 == self->chunk_ofs || work->field_0x655 == 0xFF) &&
            (Pl_act_ck(work, 0, 20) != 0 || Pl_act_ck(work, 0, 158) != 0) &&
            self->area_0x16 == work->area_0x16 &&
            fn_80050EF4(&self->vec_0x03C, &work->vec_0x03C) >= pl_float_300 &&
            ((PlItemData*)GetItemData(work->field_0x650))->field_0x01 < 3) {
            self->field_0x658 = 90;
            if (fn_80273228(self, work->field_0x650, work->field_0x652) < work->field_0x652) {
                if ((u16)fn_80273044(self, work->field_0x650) == 0xFFFF) {
                    pl_model_state_set(self, 1, 0, 0);
                } else {
                    pl_model_state_set(self, 2, 30, work->field_0x650);
                }
                return 0;
            }
            self->field_0x650 = work->field_0x650;
            self->field_0x652 = work->field_0x652;
            if (self->kind_0x09 == 3) {
                pl_act_enter(self, 0, 161, 0);
            } else {
                pl_act_enter(self, 0, 91, 0);
            }
            if (isServerSelectState() == 1) {
                if (Pl_master_ck(self) == 1) {
                    fn_80335CE8(self, 7, (u16)work->chunk_ofs);
                }
            } else {
                fn_80272E30(work, work->field_0x650, (s16)(-work->field_0x652));
                work->field_0x656 = 0xFF;
                fn_80272E30(self, self->field_0x650, self->field_0x652);
                self->field_0x656 = 1;
                pl_model_state_set(self, 2, 29, self->field_0x650);
                if ((u16)fn_8004EB18(self->field_0x650) == 1) {
                    fn_800F16D4(4);
                }
            }
            return 1;
        }
    }
    return 0;
}

/* 0x8025F088 - the actor's per-frame reset/update: latches the state bytes, runs the sub-updates and
 * pushes the body transform into the model layer. */
void fn_8025F088(_PLW* self) {
    MHchar* chr = &((PlBodyWork*)self->physics_0x13C)->chr_0x04;

    get_move_work_adrs(0);
    copyVec3(&self->vec_0x048, &self->vec_0x03C);

    self->field_0x65C = self->field_0x65D;
    if (self->field_0x017 != self->area_0x16) {
        self->field_0x65C |= 1;
    }
    self->field_0x65D = 0;
    self->field_0x01B = 0;
    self->field_0x017 = self->area_0x16;
    self->field_0x076 = self->field_0x075;
    self->field_0x276 = 0;
    self->field_0x3DC &= ~0x100000;

    fn_80260B38(self);
    self->field_0x309 = 0;
    self->field_0x657 = 0;
    self->field_0x30A = 0;
    if (Pl_master_ck(self) == 1 && fn_8027E1E4(self) == 0) {
        fn_8025E0C8(self);
        fn_80279C20(self);
    }
    self->field_0x46C = self->field_0x46D;
    if (Pl_master_ck(self) == 1) {
        if (fn_80131934(self->chunk_ofs) == 1) {
            self->field_0x46C = 1;
        } else {
            self->field_0x46C = 0;
            self->field_0x46E = 0;
        }
        if (Pl_act_ck(self, 2, 1) == 0 && Pl_act_ck(self, 0, 6) == 0) {
            self->field_0x5C7 = 0;
        }
    } else {
        self->field_0x46C = 0;
        self->field_0x46D = 0;
        self->field_0x46E = 0;
        self->field_0x5C7 = 0;
    }
    fn_8025EC58(self);
    if (fn_8026FE44(self) == 1) {
        fn_802872E4(self);
    }
    self->field_0x3B4 = 0;
    self->field_0x3B5 = 0;
    self->field_0x5C9 = 0;
    if (fn_802657F8(self) == 0) {
        fn_80262940(self);
    }
    fn_8027035C(self);
    fn_80270728(self);
    fn_80270CA4(self);
    fn_802642D0(self);
    self->field_0x3B0 = self->field_0x3AC;
    self->field_0x3AC = 0;
    self->act_state_0x00E[0] = 0;
    self->field_0x654 = 0;
    self->field_0x268 = 0;
    fn_8025DE38(self);
    if (self->act_state_0x00E[0] != 0) {
        self->act_state_0x00E[0] = 0;
        fn_8025DE38(self);
    }
    fn_8025E448(self);
    if (self->field_0x30A == 0xFF) {
        self->field_0x30A = 0;
    } else if (fn_8027AC18(self) == 0 &&
               (fn_8025E298(self, 128, 255) == 1 || fn_8025E298(self, 137, 248) == 1 ||
                fn_8025E298(self, 137, 247) == 1) &&
               self->field_0x00A != 7 && self->field_0x00A != 12) {
        self->field_0x30A = 1;
    }
    self->field_0x5C8 = 0;
    fn_80278D1C(self);
    fn_80277EC0(self);
    fn_802602A0(self);
    fn_8027AF34(self);
    fn_802607C4(self);
    fn_8026FD0C(self);
    fn_8025ED00(self);
    fn_80224AC4(self->physics_0x13C);
    copyVec3(&self->vec_0x03C, &chr->pos_0x04);
    switch (self->kind_0x09) {
    case 2:
        break;
    case 3:
        if (self->vec_0x03C.y < pl_float_110 + self->ground_y_0x060) {
            fn_800524C0(pl_float_0_6, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    default:
        if (self->field_0x01B == 0) {
            fn_800524C0(pl_float_0_3, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    }
    fn_8025FF0C(self, 0);
    fn_8025FA00(self);
    if (Pl_act_ck(self, 0, 20) == 1 || Pl_act_ck(self, 0, 158) == 1) {
        self->field_0x268 = 1;
    }
    fn_8026FD0C(self);
    fn_800E0914(chr);
    if (fn_8026FD94(self) == 0) {
        fn_8027D4F0(self);
    }
    fn_8029EFDC(&self->field_0x484);
    fn_8029EFDC(&self->field_0x4E0);
}
