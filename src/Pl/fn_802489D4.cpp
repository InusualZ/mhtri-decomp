/*
 * The player "action/handler" cluster of the `Pl` module: `.text` 0x802489D4-0x8024F200 (42
 * functions, 0x6D2C B), with its own exception tables - extab 0x80011DB4-0x80011EFC (41 records)
 * and extabindex 0x8002EB78-0x8002ED64 (41 records).  Both runs are bracketed exactly by the
 * neighbouring functions: `fn_802488D4`'s extab record ends at 0x80011DB4 and `pl_act_step_offhand_gesture`'s
 * starts at 0x80011EFC, so the extent is pinned by the section runs, not by a byte cap.
 *
 * WHAT IT IS.  The band drives a `_PLW` (player work) actor through its act/step state machine and
 * its per-act handlers: every function takes `_PLW*` in r3 and the unit's outbound calls are the Pl
 * gates (`Pl_master_ck`, `Pl_Skill_ck`, `Pl_act_ck`, `Pl_cat_skill_ck`, `Pl_frame_check`,
 * `Pl_chr_setX`, `PlayMode_ck`) plus the 0x8026xxxx/0x8027xxxx Pl helpers.  Four of the 42 are
 * small `switch`-on-an-id classifiers shared by the rest (`fn_8024AA04`, `fn_8024CD8C`,
 * `fn_8024D094`, `fn_8024D3C8`), and `fn_8024D61C` (0x15E0 B) is the band's main dispatcher.
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   - Class 1 (`__FILE__` string): out.  The `.data` source-name pool has no name for this band
 *     between `enemy_control.cpp` @0x805A1BB8 and `menu_item.cpp` @0x805CDFC8 (the sibling Pl units
 *     `Pl/fn_80241558.cpp` and `Pl/fn_80262940.cpp` checked the same pool), and no string in the
 *     range's own data references is a bare source-file name.
 *   - Class 2 (runtime-dump name): out.  `dumpmap.py lookup` gives the `zz_XXXXXXXX_` placeholder
 *     for every address in the range, and the map has only `fn_XXXXXXXX`.
 *   - Class 3 (module): `Pl`.  Every sibling unit is `Pl/*.cpp`, the actor is `_PLW`, and the gates
 *     this band calls are the Pl ones listed above.
 *   - Class 4 (file stem): the map's `fn_802489D4`, the sibling class-4 pattern of
 *     `Pl/fn_80229ECC.cpp` / `Pl/fn_80241558.cpp` / `Pl/fn_80262940.cpp` / `Pl/fn_80288CEC.cpp`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802489D4 0x8024F200` - all 42 entries are the map's
 * `fn_XXXXXXXX` placeholders, and `python tools/symbols/dumpmap.py lookup` resolves every one of
 * them to a `zz_XXXXXXXX_` dump placeholder).
 *
 * Seam: unproven.  The range is one maximal unclaimed run cut at `attribute.py`'s `--max-bytes`
 * cap; discovery's "one source file (enemy_control.cpp)" warning is an `owner_merge` artefact (the
 * name it names is 0x10xxxx away and only "owns" the range because `source_owner` takes the latest
 * name at or before the index).  The section runs above are what actually pin the extent.
 *
 * Language: C++.  The map's undefined set carries eleven mangled names
 * (`Pl_Skill_ck__FP4_PLWUs`, `Pl_act_ck__FP4_PLWUcUs`, `Pl_chr_setX__FP4_PLWUsll`,
 * `Pl_frame_check__FP4_PLWUlff`, `rotVecXYZ__FPQ34nw4r4math4VEC3P10_CP_VECTOR`, ...), and every Pl
 * object carries an extab/extabindex pair, which is why the Pl lib sets `-Cpp_exceptions on`.
 *
 * Flags: the lib's `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * mw_version Wii/1.0), the Pl lib's measured flag set.
 *
 * Type work: the `_PLW` offsets this band touches that `include/pl.h` still spelled `unkNN` or hid
 * inside a `pad_` run were given names there in this batch (+0x018, +0x0AC, +0x37A, +0x5B8,
 * +0x5BA, +0x656, +0x657).  Where the context supports a meaning the field got one; where it only
 * fixes the offset the field follows the header's existing `field_0xNN` convention.
 *
 * Residual: work in progress - see `.pi/notes/802489d4-fn-802489d4-7ce7.md`.
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/fn_802693C4.h"   /* 0x802693C4-0x8026BA1C - the owner of the Pl_chr_set_attr_default/33C/644 group */
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Network/network_pat_control.h"   /* isServerSelectState (owner header, rule 2) */
#include "unsplit/ef.h"
#include "ef/eft004.h"
#include "ef.h"
#include "g3d/g3d_calcworld.h"
#include "ef/fn_800CDB2C.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 frame-window pool (rule 2) */

/* The unit's definitions are `extern "C"`: the map spells every one of them `fn_XXXXXXXX`
 * (unmangled), and a C++ definition would mangle and pair nothing (playbook rows 42/48). */
extern "C" {

/* This unit's own helpers, defined further down but called from the handlers above them. */
void fn_8024D144(_PLW* self);
void fn_8024D094(_PLW* self);
void fn_8024D3C8(_PLW* self);
void fn_8024D44C(_PLW* self, s32 a, s32 b, s32 c);
s32 fn_8024D5AC(_PLW* self, _PLW* other);
void fn_8024D61C(_PLW* self, s32 a);
s16 fn_8024CD8C(_PLW* self, s16 value);
void fn_8024CE54(_PLW* self);
void fn_8024CA50(_PLW* self, s32 a);
void fn_8024CAC8(_PLW* self);
void fn_8024BCB0(_PLW* self, s32 a, s32 b, s32 c);
void fn_8024B9FC(_PLW* self);
void fn_8024EBFC(_PLW* self);

/* 0x80249424 - the "down/knock-down" act handler: arms the down motion, then ends the act once the
 * master gate and the hit check agree. */
void fn_80249424(_PLW* self, s32 arg1) {
    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3C, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x87, 4, 0);
        }
        self->act_end_request = 0;
        fn_80101594(self);
        return;
    case 1:
        if (Pl_master_ck(self) != 0 && (pl_part_flag_ck(self, 4) == 1 || self->act_end_request != 0)) {
            if (arg1 == 0) {
                pl_act_enter(self, 0, 0x15, 0);
                return;
            }
            pl_act_enter(self, 0, 0x9F, 0);
        }
        return;
    }
}

/* 0x80249558 - the get-up handler: arms the get-up motion, then hands the actor's kind to the motion
 * table once the pose check passes. */
void fn_80249558(_PLW* self, s32 arg1) {
    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3D, 4, 0);
            return;
        }
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x88, 4, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        return;
    }
}

/* 0x8024963C - the get-up handler with an item-recovery arm: on `arg2 == 1` it reads the pending
 * recovery item and feeds it to `fn_80272E30`. */
void fn_8024963C(_PLW* self, s32 arg1, s32 arg2) {
    u16 item;
    s16 value;

    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x200, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x214, 4, 0);
        }
        if (arg2 == 1) {
            fn_802D884C(&item, &value);
            if (Pl_motion_input_ck(1) == 0 && value > 0) {
                fn_80272E30(self, item, value);
            }
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        /* the last case has no branch: retail falls straight into the epilogue */
    }
}

/* 0x80249768 - the "hold/struggle" handler: arms the struggle motion and gives up after 600 frames
 * or when the act-end signal arrives. */
void fn_80249768(_PLW* self, s32 arg1) {
    s32 timer;

    self->act_handler_entered = 1;
    fn_802DE578(self, &self->field_0x598);
    fn_8027A17C(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_setX(self, 1, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_setX(self, 0x64, 6, 0);
        }
        pl_act_set_frame_timer(self);
        if (isServerSelectState() == 1) {
            self->act_end_request = 0;
        }
        self->field_0x28 = 0;
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        timer = self->field_0x28 + 1;
        self->field_0x28 = timer;
        if (timer > 0x258 || self->act_end_request != 0) {
            if (self->act_end_request == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x49, 0);
                    return;
                }
                pl_act_enter(self, 0, 0xA0, 0);
                return;
            }
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
            return;
        }
        return;
    }
}

/* 0x802498E0 - the "stagger/recover" handler: it either arms a fixed motion table or hands the choice
 * to `fn_802771A0`, then picks the follow-up motion from the actor's state. */
void fn_802498E0(_PLW* self) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8003, 0, 0);
        self->field_0x28 = 0x1E;
        if (self->field_0x018 == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805BE824, 0);
            return;
        }
        fn_802771A0(self, 0);
        return;
    case 1:
        if (self->field_0x28 > 0 || fn_8027A340(self) == 0) {
            timer = self->field_0x28 - 1;
            self->field_0x28 = timer;
            if (timer <= 0) {
                self->field_0x0AC = 0;
            }
        }
        if (Pl_master_ck(self) == 1 && fn_8026FE44(self) == 1 && self->field_0x018 == 1) {
            fn_8027A57C(self, 0x41A, 0);
        }
        if (self->field_0x018 != 1) {
            if (fn_80276800(self, 0) == 1) {
                Pl_chr_setX(self, 0x79, 4, 0);
                return;
            }
            if (self->field_0x37A <= 0x96) {
                Pl_chr_setX(self, 0x168, 4, 0);
                return;
            }
            if (Pl_suimen_ck(self) == 1) {
                Pl_chr_setX(self, 0x76, 4, 0);
                return;
            }
            Pl_chr_setX(self, 0x64, 4, 0);
            return;
        }
        return;
    }
}

/* 0x8024A51C - the "sit/rest" handler: one arming step, then a two-way exit through `pl_act_enter`. */
void fn_8024A51C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 7, 2, 0);
        pl_act_clear_mode5c4(self);
        pl_act_set_frame_timer(self);
        if (arg1 == 0) {
            pl_act_clear_flag5bb(self);
            return;
        }
        fn_8027AC00(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x1F, 0);
                    return;
                }
                pl_act_enter(self, 0xA, 0xE, 0);
                return;
            }
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 8, 6, 0);
            return;
        }
        return;
    }
}

/* 0x8024A640 - the one-step "equip/ready" handler. */
void fn_8024A640(_PLW* self) {
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8001, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_setX(self, 8, 4, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_step_table(self, (u32)lbl_805BE5B8, 0);
    }
}

/* 0x8024A6CC - the two-step "sheathe" handler. */
void fn_8024A6CC(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 9, 0, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8024A75C - the "stagger" handler: arms a 150-frame motion, then hands the actor's `+0x37A` value
 * to the motion table. */
void fn_8024A75C(_PLW* self, s32 arg1) {
    s32 timer;

    fn_80276868(self, 4);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        if (arg1 == 0 || arg1 == 2) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 5, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x7A, 6, 0);
        }
        self->field_0x28 = 0x96;
        return;
    case 1:
        timer = self->field_0x28 - 1;
        self->field_0x28 = timer;
        if (timer > 0) {
            return;
        }
        self->act_step_0x05++;
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 6, 4, 0);
            return;
        }
        Pl_chr_set_attr_default(self, 0x7C, 4, 0);
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_80276868(self, self->field_0x37A);
            if (arg1 == 0 || arg1 == 2) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
                return;
            }
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8024A8EC - the "item use" handler: arms one of three motion/SE rows selected by `arg1`, then
 * ends the act through the matching `pl_act_enter` code. */
void fn_8024A8EC(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A54[arg1 * 2], lbl_805C4A54[arg1 * 2 + 1], 0);
        self->field_0x018 = 0;
        if (arg1 == 0) {
            self->field_0x5B8 = 0;
            self->field_0x5BA = 0;
            fn_802B8DF8(self);
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            switch (arg1) {
            case 0:
                pl_act_enter(self, 0, 0x23, 2);
                return;
            case 1:
                pl_act_enter(self, 0, 0x78, 2);
                return;
            case 2:
                pl_act_enter(self, 0, 0x77, 2);
                break;
            }
        }
        break;
    }
}

/* 0x8024AA04 - maps a motion id to the small class the act tables use: 0 for the idle pair, 1 for
 * 0x28, 2 for the two heavy ids. */
s32 fn_8024AA04(u16 motion) {
    s32 result = 0;

    switch (motion) {
    case 0x2F:
        result = 0;
        break;
    case 0x28:
        result = 1;
        break;
    case 0x109:
        result = 2;
        break;
    case 0x1B6:
        result = 2;
        break;
    }
    return result;
}

/* 0x8024A248 - the "ride/mount" handler: arms one of three motion sets from the actor's `+0x2`
 * kind, then either ends the act through `pl_act_enter` or pushes the rider along the mount's
 * rotation. */
void fn_8024A248(_PLW* self, s32 arg1) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        if ((u32)arg1 <= 1U) {
            fn_80277BC4(self, 0);
        } else {
            fn_80277BC4(self, 1);
        }
        fn_80277C50(self, 0);
        fn_80276B58(self, -0x96);
        fn_8027A17C(self);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        if (self->field_0x018 == 0) {
            Pl_chr_set_attr_default(self, 0x7B, 4, 0);
            return;
        }
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 0x426, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x427, 4, 0);
        }
        switch (self->field_0x002) {
        case 1:
            Pl_act_set_step_table(self, (u32)lbl_805C9118, 0);
            return;
        case 2:
            Pl_act_set_step_table(self, (u32)lbl_805CAD74, 0);
            return;
        case 8:
            if (fn_80331104() == 0) {
                Pl_act_set_step_table(self, (u32)lbl_805E2048, 0);
                return;
            }
            return;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 0 || self->field_0x018 == 1) {
                Pl_act_set_motion_slot(self, 3, 8, 0);
                return;
            }
            if (pl_act_param_tier_ck(self, 0) >= 2U) {
                if (pl_part_flag_ck(self, 0x25) == 1) {
                    pl_act_enter(self, 1, 9, 0x80);
                    return;
                }
                pl_act_enter(self, 1, 7, 0x80);
                return;
            }
            pl_act_enter(self, 1, 8, 0x80);
            return;
        }
        if (Pl_Skill_ck(self, 0xB6) == 1 && Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
            vec.x = pl_float_zero;
            vec.y = pl_float_zero;
            if (arg1 == 0 || arg1 == 2) {
                vec.z = pl_frame_window_8;
            } else {
                vec.z = pl_float_neg8;
            }
            /* +0x054 is the actor's rotation; other units read its first word as a scalar, so the
             * `_CP_VECTOR` view is taken through the named field rather than a byte offset. */
            rotVecXYZ(&vec, (_CP_VECTOR*)&self->param_0x54);
            addVec3To(&self->field_0x03C, &vec);
        }
        break;
    }
}

/* 0x8024B35C - the two-step "fall" handler: arms the motion row for `arg1`, then ends the act
 * through the `Pl_act_set_motion_slot` or `pl_act_enter` code that `arg1` selects. */
void fn_8024B35C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A60[arg1], 0, 0);
        return;
    case 1:
        if ((u32)(arg1 - 3) > 1U) {
            if ((u32)(arg1 - 1) > 1U) {
                if (arg1 != 0) {
                    return;
                }
            } else {
                if (Pl_motion_end_ck(self) == 1) {
                    if (arg1 == 1) {
                        pl_act_enter(self, 0, 0x71, 2);
                        return;
                    }
                    pl_act_enter(self, 0, 0x72, 2);
                }
                return;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8024B46C - the sibling "fall" handler whose 0x72 exit is gated on a frame check. */
void fn_8024B46C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A6C[arg1], 0, 0);
        return;
    case 1:
        if ((u32)(arg1 - 3) > 1U && arg1 != 0) {
            switch (arg1) {
            case 1:
                if (Pl_motion_end_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x71, 2);
                    return;
                }
                break;
            case 2:
                if (Pl_frame_check(self, 1, pl_frame_window_80, pl_float_zero) == 1) {
                    pl_act_enter(self, 0, 0x72, 2);
                }
                break;
            }
        } else {
            if (Pl_motion_end_ck(self) == 1) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
                return;
            }
            return;
        }
        return;
    }
}

/* 0x8024B594 - the "swim/dive" handler: arms one of two motion sets, then hands the actor's kind to
 * the motion table. */
void fn_8024B594(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0x8000, 0, 0);
            Pl_chr_set_attr_default(self, 0x29, 2, 0);
        } else {
            Pl_act_set_motion(self, 0x8003, 0, 0);
            Pl_chr_set_attr_default(self, 0x7E, 2, 0);
        }
        pl_act_set_frame_timer(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 8, 0);
        }
        return;
    }
}

/* 0x8024B66C - the "evade" handler: one of three arming motions, then the frame-checked roll-out. */
void fn_8024B66C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 7, 2, 0);
            return;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 4, 0);
            return;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 4, 0);
            return;
        }
        break;
    case 1:
        switch (arg1) {
        case 0:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x29, 0xC);
                    return;
                }
                Pl_chr_set_attr_default(self, 8, 6, 0);
                return;
            }
            return;
        case 1:
            if (Pl_frame_check(self, 0, pl_frame_window_30, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x99, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
                return;
            }
            break;
        case 2:
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x9C, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
            }
            break;
        }
        break;
    }
}

/* 0x8024B868 - the "get-up from evade" handler: arms the motion, feeds the pending item back, then
 * waits 14 frames before handing over to `Pl_act_set_motion_slot`. */
void fn_8024B868(_PLW* self, s32 arg1) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 8, 6, 0);
            break;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 0, 0x1E);
            break;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 0, 0x14);
            break;
        }
        self->field_0x28 = 0;
        if (Pl_master_ck(self) == 1) {
            fn_8024D144(self);
            if (Pl_motion_input_ck(1) == 0) {
                fn_80272E30(self, self->field_0x306, -1);
                return;
            }
        }
        return;
    case 1:
        if ((u32)(arg1 - 1) > 1U) {
            if (arg1 == 0) {
                timer = self->field_0x28 + 1;
                self->field_0x28 = timer;
                if (timer >= 0xE) {
                    self->act_step_0x05++;
                    Pl_chr_set_attr_default(self, 9, 0, 0);
                    return;
                }
            }
            return;
        }
        self->act_step_0x05++;
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
        }
        break;
    }
}

/* 0x8024C75C - the "drink/item" handler: arms the item motion, refunds the pending item, and arms
 * the recovery scale when the skill is set. */
void fn_8024C75C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x136, 0, 0x3C);
        if (Pl_master_ck(self) == 1) {
            fn_8024D094(self);
            if (Pl_motion_input_ck(1) == 0) {
                fn_80272E30(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x131, -4, 0xCE);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0xA, 0);
        }
    }
}

/* 0x8024C878 - the sibling "drink" handler whose 0x7E exit is gated on a frame check. */
void fn_8024C878(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, -8, 0);
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_208, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1) {
                pl_act_enter(self, 0, 0x7E, 0xC);
                return;
            }
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
    }
}

/* 0x8024C96C - the "drink, standing" variant of 0x8024C75C. */
void fn_8024C96C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
        if (Pl_master_ck(self) == 1) {
            fn_8024D094(self);
            if (Pl_motion_input_ck(1) == 0) {
                fn_80272E30(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 0xA, 0);
        }
    }
}

/* 0x8024CA50 - arms the recovery scale for the two "eat/drink" skills. */
void fn_8024CA50(_PLW* self, s32 arg1) {
    if (arg1 == 0) {
        if (Pl_Skill_ck(self, 0xB0) == 1) {
            self->field_0x354 = pl_float_1_7;
        }
    } else if (Pl_Skill_ck(self, 0xAF) == 1 || Pl_Skill_ck(self, 0xB0) == 1) {
        self->field_0x354 = pl_float_1_8;
    }
}

/* 0x8024CD8C - scales a `s16` amount by the skill's multiplier (0xA6 first, then 0xA7). */
s16 fn_8024CD8C(_PLW* self, s16 value) {
    s16 scaled = value;

    if (Pl_Skill_ck(self, 0xA6) == 1) {
        scaled = (s16)(pl_float_1_5 * (f32)scaled);
    } else if (Pl_Skill_ck(self, 0xA7) == 1) {
        scaled = (s16)(pl_float_0_66 * (f32)scaled);
    }
    return scaled;
}

/* 0x8024D3C8 - one-in-three chance the actor is staggered by `fn_80276CE8` when the skill is set. */
void fn_8024D3C8(_PLW* self) {
    if (Pl_master_ck(self) == 1 && Pl_Skill_ck(self, 0x49) == 1 && ran_suu(1) % 3 == 0) {
        fn_80276CE8(self, 0x96);
    }
}

/* 0x8024D5AC - the lobby-mode actor-match test: only in play mode, and only when the two actors
 * share their `+0x016` area byte. */
s32 fn_8024D5AC(_PLW* self, _PLW* other) {
    if (PlayMode_ck() != 2) {
        return 0;
    }
    if (other == NULL) {
        return 0;
    }
    return self->area_0x16 == other->area_0x16;
}

} /* extern "C" */
