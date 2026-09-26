/*
 * ai/fn_802D0F34.cpp - the 0x802D0F34-0x802D44F4 AI-NPC motion band (90 functions, 13760 B).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/symedit.py: every name this file defines is a bare `.text` row in
 * config/RMHE08/symbols.txt, no `__FILE__` string covers the range - the nearest one,
 * `lbl_805CDFC8` ("menu_item.cpp"), is referenced only by 0x802A5444/0x802A579C/0x802A64B0, eight
 * bands below - and the runtime dump answers `zz_` for every row).
 *
 * Home and type, from evidence: the range sits directly above `ai/fn_802D0DCC.c` and its `self` is
 * the AI-NPC work record - the three names the map does spell out are `ai_area_ck__FP8_AINPC_W`,
 * `ai_get_motion_no__FP8_AINPC_W` and
 * `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`, and the band forwards `self` to the `ai`
 * band below (`fn_802CD770`, `fn_802D0C9C`, `ai_skill_ck`, `fn_802D0DCC`) and to
 * `get_move_work_adrs(3)`.  So the module is `ai`; the file keeps the map's stem (brief section 2,
 * class 4 - nothing in the object names the original source file).  The record and the attack entry
 * embedded in it live in `include/ai/ai_npc.h`; the cross-unit declarations are in
 * `include/ai/fn_802D0F34.h`.
 *
 * Sections this unit owns: .text 0x802D0F34..0x802D44F4, extab 0x80014844..0x8001496C (37 8-byte
 * records), extabindex 0x80032ACC..0x80032C88, .data 0x805D4E80..0x805D5000 (the four
 * `scope:local` jump tables `fn_802D0F34`, `fn_802D2264`, `fn_802D4248` and the one at 0x805D5000
 * that belongs to 0x802D44F4).
 *
 * Flags: `cflags_main` (`Wii/1.3`, `-O3 -inline noauto`, `-Cpp_exceptions on`) - the neighbour `ai`
 * lib's setting, and `-Cpp_exceptions on` is what emits the target's extab.
 *
 * Rule-1 debt: `_HIT_W` is defined here (through `include/ai/ai_npc.h`) as well as in
 * `include/menu/menu_item.h` and `include/Pl/fn_80295EF4.h`, which carry their own partial views of
 * the same record; none of them names the offsets this band reads.  `_AINPC_W` likewise also lives in
 * `include/ai/ainpc.h` (the `fn_802CC794.cpp`/`fn_802C474C.cpp` view) - `include/ai/ai_npc.h` carries
 * the note and the two must be folded by the next `ai` worker.
 *
 * Score at this commit: 90.96 % fuzzy (13760/13760 B of `.text`, 52 of 90 functions byte-identical,
 * 85 of 90 at or above the 80 % bar) - `ninja build/RMHE08/report.json`, unit `main/ai/fn_802D0F34`.
 *
 * Residuals (measured, `tools/objdiff/symdiff.py -u main/ai/fn_802D0F34 <symbol>`):
 *   * `fn_802D2984` 60.0 %, `fn_802D2990` 50.0 %, `fn_802D29A0` 50.0 % - the address of the model
 *     (`addi r3,r3,8`) is scheduled *before* the constant argument (`li r4/r5`) in retail; every
 *     source spelling tried (a `MHchar*` local, the member form) keeps MWCC's constant-first order.
 *     Same instruction set, so this is an emission-order residual, not a source shape.
 *   * `fn_802D3F1C` 73.3 % - retail keeps `li r0,0xFF; cmplwi r0,0xFF; beq` where the first
 *     `highest == 0xFF` test is provably true; our build folds it away (the operand-order spelling
 *     `highest < value` was chosen to keep the second compare's shape).
 *   * `fn_802D3984` 76.1 % - retail widens `reverse` with `clrlwi r0,r6,24` and sign-extends both
 *     bounds; ours narrows only inside the arithmetic (a `u8`/`s16` local spelling costs the
 *     `clrlwi`), best variant kept.
 *   * `fn_802D2264` 81.7 % - the counter-attack gate; the control flow is reproduced with the
 *     rule-8 shapes (`for (;;) { ... break; }` where retail branches to a shared tail), so the
 *     residual is register numbering and the `(s16)`-narrowed `-window` argument.
 *   * `fn_802D282C` 82.3 %, `fn_802D30F8` 84.7 %, `fn_802D15DC` 84.8 %, `fn_802D3184` 86.0 %,
 *     `fn_802D2F7C` 83.5 % - register-allocation/argument-width residuals (first divergence is an
 *     `ARG` row, the instruction set is equal).
 *   * our object carries an 8-byte `.sdata2` section the target object does not: the double
 *     constant MWCC materialises for the `(f32)` conversion in `fn_802D287C`.  Harmless while the
 *     unit is `NonMatching` (the linker pools `.sdata2` per program), but it is the first thing to
 *     look at if this unit is ever flipped.
 *   * the three `scope:local` jump tables (`jumptable_805D4E80` 0x28, `jumptable_805D4EA8` 0x94,
 *     `jumptable_805D4F3C` 0xC4) *are* emitted - byte-identical bytes and identical
 *     `R_PPC_ADDR32 fn_802D0F34+0x...` relocations against the target's - but objdiff pairs `.data`
 *     by symbol name and MWCC emits them anonymously, so the rows read 0 %; the target names come from
 *     `dol split`, not from the object (playbook 23).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "ai/ai_npc.h"
#include "ai/fn_802D0F34.h"

/* One entry of the motion table `lbl_805D3AD8` (`fn_802D2F7C` walks it with a 0x1A stride): the two
 * s16 distances and the u8 motion row `lbl_80792508` is indexed with. size: 0x1A */
struct MotionEntry {
    /* +0x00 */ s16 approach_0x00;
    /* +0x02 */ s16 retreat_0x02;
    /* +0x04 */ u8 unused_0x04[0x0F - 0x04];
    /* +0x0F */ u8 motion_id_0x0F;
    /* +0x10 */ u8 unused_0x10[0x1A - 0x10];
};

/* =================================================================================================
 * 0x802D0F34 .. 0x802D27E0
 * ============================================================================================== */

extern "C" {

/* 0x802D0F34 - the attack driver: the counter-attack countdown, the "blocked" arm and the whole
 * attack-plan dispatch on +0x1CC. */
void fn_802D0F34(struct _AINPC_W* self)
{
    u8* enemy_work = get_move_work_adrs(3);

    if (self->motion != 7 &&
        (fn_802D948C(self) != 1 || self->area == self->plw_0x16C->area_0x16 ||
         self->field_0x485 == 1)) {
        if (self->field_0x1D1 != 0) {
            self->field_0x35C = 0;
            self->field_0x358 = 0;
            fn_802D3A20(self);
            if (self->field_0x1D1 == 1) {
                fn_802D4200(self);
                self->field_0x389 = 1;
                fn_802D41C8(self);
            }
            self->field_0x1D1 = 0;
        }
        if (fn_802D3DE8(self) != 1) {
            if (self->variant == 2) {
                self->field_0x1C0 = fn_80050EF4(&self->pos_0x1B0, &self->pos_0x178);
            } else {
                self->field_0x1C0 = fn_80050F80(&self->pos_0x1B0, &self->pos_0x178);
            }
            if (self->field_0x1CD != 0) {
                if (self->field_0x1CF != 0) {
                    fn_802D4238(self);
                    switch (self->field_0x1CC) {
                    case 0:
                        if (self->field_0x354 == 1) {
                            fn_802D41E0(self, 2, 0);
                            self->field_0x354 = 2;
                        } else if (self->field_0x354 == 3) {
                            fn_802D41E0(self, 0x33, 1);
                        } else if (self->variant == 2) {
                            if (self->field_0x460 == 1) {
                                fn_802D41E0(self, 0x29, 1);
                            } else if (self->field_0x461 == 1) {
                                fn_802D41E0(self, 0x2A, 1);
                            } else if (self->field_0x468 != 0) {
                                fn_802D41E0(self, 0x2C, 1);
                            } else if (self->field_0x34D == 1) {
                                fn_802D41E0(self, 5, 1);
                            } else {
                                fn_802D41E0(self, 0x0A, 0);
                            }
                        } else if (self->field_0x460 == 1) {
                            fn_802D41E0(self, 0x27, 1);
                        } else if (self->field_0x461 == 1) {
                            fn_802D41E0(self, 0x28, 1);
                        } else if (self->field_0x468 != 0) {
                            fn_802D41E0(self, 0x2B, 1);
                        } else if (self->field_0x440 != 0) {
                            fn_802D41E0(self, 0x13, 0);
                        } else if (self->field_0x34D == 1) {
                            fn_802D41E0(self, 4, 1);
                        } else {
                            fn_802D41E0(self, 3, 0);
                        }
                        break;
                    case 1:
                        switch (self->field_0x354) {
                        case 1:
                            self->field_0x1CE = 2;
                            self->field_0x354 = 2;
                            break;
                        case 2:
                            fn_802D0DCC(self);
                            break;
                        case 3:
                            fn_802D41E0(self, 0x33, 1);
                            break;
                        default:
                            if (self->variant == 2) {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[1]);
                                } else if (self->field_0x420 == 5) {
                                    fn_802D41E0(self, 0x2E, 1);
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x24, 1);
                                } else {
                                    fn_802D41E0(self, 0x0D, 1);
                                }
                            } else {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[0]);
                                } else if (self->field_0x420 == 5) {
                                    u8* enemy = enemy_work;

                                    if (self->enemy_index != 0xFF) {
                                        enemy += self->enemy_index * 0xB18;
                                    }
                                    if ((u8)(enemy[0x1E5] + 0xF8) <= 1) {
                                        fn_802D41E0(self, 0x19, 1);
                                    } else {
                                        fn_802D41E0(self, 0x2D, 1);
                                    }
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x23, 1);
                                } else {
                                    fn_802D41E0(self, 1, 1);
                                }
                            }
                            break;
                        }
                        break;
                    case 2:
                    case 3:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x11, 1);
                        } else {
                            fn_802D41E0(self, 6, 1);
                        }
                        break;
                    case 4:
                        if (self->variant == 2) {
                            if (self->field_0x447 == 2) {
                                self->field_0x447 = 1;
                                fn_802D41E0(self, 0x26, 1);
                            } else if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else if (self->field_0x34F == 1) {
                                fn_802D41E0(self, 0x20, 1);
                            } else {
                                fn_802D41E0(self, 0x0C, 1);
                            }
                        } else if (self->field_0x447 == 2) {
                            self->field_0x447 = 1;
                            fn_802D41E0(self, 0x25, 1);
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else if (self->field_0x34F == 1) {
                            fn_802D41E0(self, 0x1F, 1);
                        } else {
                            fn_802D41E0(self, 8, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 5:
                        if (self->variant == 2) {
                            if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else {
                                fn_802D41E0(self, 0x0B, 1);
                            }
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else {
                            fn_802D41E0(self, 7, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 6:
                        self->field_0x1CE = 0x10;
                        break;
                    case 7:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x22, 1);
                        } else {
                            fn_802D41E0(self, 0x21, 1);
                        }
                        break;
                    case 8:
                        fn_802D41E0(self, 0x2F, 1);
                        break;
                    case 9:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x32, 1);
                        } else {
                            fn_802D41E0(self, 0x31, 1);
                        }
  
                        break;
                    }
                }
                fn_802D0C9C(self);
                fn_802D41D4(self);
            }
        }
    }
}

/* 0x802D15DC - the move solver: fills the two probe vectors, walks the ~0x20-radius band and hands
 * the result to the collision query. */
void fn_802D15DC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 probe;
    nw4r::math::VEC3 current;
    u16 flags = 0x20;
    s32 blocked = 0;

    fn_80043EA8(&probe);
    fn_80043EA8(&current);
    if (self->field_0x3CA > 0) {
        self->field_0x3C8 = 0;
        return;
    }
    if (fn_802D2B38(self, 2, 9) == 1 || fn_802D2B38(self, 2, 0x13) == 1) {
        blocked = 1;
    }
    if (fn_802D7B24(self) == 1) {
        blocked = 1;
    }
    if ((u32)(self->motion - 4) > 1 && blocked == 0) {
        flags = 0x20 | 0xC0;
    }
    fn_80041E40(&probe, &self->pos_0x184);
    probe.y = probe.y + lbl_8079A7A8;
    fn_80041E40(&current, &self->pos_0x178);
    current.y = current.y + lbl_8079A7A8;
    self->field_0x3C8 = (s8)fn_8029208C(&probe, &current, &self->pos_0x178, 0xFFFF, flags, 1,
                                        self->area, lbl_8079A7A8);
}

/* 0x802D1710 - the step-height/ground probe: walks the two forward probes and re-seeds the motion
 * when the NPC has to step up or down. */
void fn_802D1710(struct _AINPC_W* self)
{
    nw4r::math::VEC3 base;
    f32 value;

    fn_8012A624(&base);
    self->field_0x3CE = 0;
    if (self->field_0x3D0 <= 0 && fn_802D2B78(self, 2) == 0) {
        if (fn_80291B08(self, &self->pos_0x178, &self->field_0x324, &value, -6) == 1) {
            self->field_0x19C = value;
            if (self->variant == 0) {
                if (self->pos_0x178.y - value <= lbl_8079A6A8) {
                    self->pos_0x178.y = value;
                } else {
                    fn_802D2A00(self, 6, 0, 0);
                }
            } else if (self->variant == 2) {
                if (fn_802D2B38(self, 2, 0x0D) != 1) {
                    fn_802D2A00(self, 2, 0x0D, 0);
                    fn_802D29F8(self, 0);
                    fn_802D4200(self);
                }
            }
        } else if (fn_80291B08(self, &self->pos_0x178, &self->field_0x324, &value, 1) == 1) {
            self->field_0x19C = value;
            if (self->variant == 2) {
                if (self->pos_0x178.y > lbl_8079A6B0) {
                    self->pos_0x178.y = lbl_8079A6B0;
                    self->field_0x3CE = 1;
                } else if (self->pos_0x178.y < lbl_8079A754 + value) {
                    self->pos_0x178.y = lbl_8079A754 + value;
                    self->field_0x3CE = 1;
                }
            } else {
                fn_802D2A00(self, 2, 0x0C, 0);
                fn_802D29F8(self, 2);
                fn_802D4200(self);
            }
        }
        if (fn_80291B08(self, &self->pos_0x178, &base, &value, 4) == 1) {
            self->field_0x1A0 = value;
        }
    }
}

/* 0x802D18E8 - mirrors the model's visibility flag onto the engine model when it moved. */
void fn_802D18E8(struct _AINPC_W* self)
{
    u8 visible = self->field_0x417;

    if (self->field_0x418 != visible) {
        if (visible == 1) {
            self->model.setVisibility(0x15, false);
        } else {
            self->model.setVisibility(0x15, true);
        }
    }
    self->field_0x418 = self->field_0x417;
}

/* 0x802D1954 - re-seeds the NPC from the player work it targets: area, position, motion and the
 * whole attack bookkeeping. */
void fn_802D1954(struct _AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 position;

    fn_80043EA8(&offset);
    if (fn_802D948C(self) != 1) {
        if (fn_802D94A0(self) == 1 && plw->area_0x16 == 0) {
            self->field_0x486 = 0x5A;
        }
        self->area = plw->area_0x16;
        fn_800FC0D4(&self->vec_0x190, (_CP_VECTOR*)&plw->param_0x54);
        setVector3(&offset, lbl_8079A7AC, lbl_8079A670, lbl_8079A7B0);
        rotVecY(&offset, self->vec_0x190.y);
        fn_80051378(&position, &plw->vec_0x03C, &offset);
        fn_80041E40(&self->pos_0x184, &position);
        fn_80041E40(&self->pos_0x178, &self->pos_0x184);
        if (plw->kind_0x09 != 3) {
            fn_802D29F8(self, 0);
        } else {
            fn_802D29F8(self, 2);
        }
        if (self->motion == 5) {
            if ((u32)(self->motion_step - 5) <= 1) {
                fn_802D2A00(self, 0, 0, 0);
            }
        } else {
            fn_802D2AD4(self, self->variant);
        }
        fn_802D1710(self);
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        fn_802D4200(self);
        self->field_0x389 = 1;
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x383 = 1;
        self->field_0x447 = fn_802D8550(self);
        self->field_0x448 = 0;
        self->field_0x38A = 0;
        self->field_0x440 = 0;
        self->model.scale_0x1C.x = lbl_8079A674;
        self->model.scale_0x1C.y = lbl_8079A674;
        self->model.scale_0x1C.z = lbl_8079A674;
        fn_802D1FFC(self);
        fn_800E0914(&self->model);
    }
}

/* 0x802D1AFC - the weak-point roll: after the counter-attack window it picks one of the five
 * weak-point rates, bumps it and books the two hit effects. */
void fn_802D1AFC(struct _AINPC_W* self)
{
    u16 id;
    s16 count;

    if (self->field_0x420 == 3) {
        if (fn_802D86D4(&id, &count) == 1) {
            if (self->field_0x422 <= 0 && self->field_0x461 == 0) {
                u8 slot;

                self->field_0x422 = 0x384;
                slot = ran_suu(1) % 5;
                self->field_0x426 = slot;
                if (ran_suu(1) % 100 < self->weak_point_0x427[slot]) {
                    slot = ran_suu(1) % 5;
                    self->field_0x426 = slot;
                }
                self->weak_point_0x427[slot] = self->weak_point_0x427[slot] + 0x0A;
                if (self->weak_point_0x427[slot] >= 0x64) {
                    self->weak_point_0x427[slot] = 0x64;
                }
                fn_802D40EC(self, id, -count);
                if (self->field_0x42C != 0) {
                    fn_802D40EC(self, fn_802D89D4(self), count);
                }
            }
        } else {
            self->field_0x422 = 0;
            self->field_0x426 = 0;
        }
    }
}

/* 0x802D1C90 - the aim timer: bands the target's distance and runs the two aim-build limits. */
void fn_802D1C90(struct _AINPC_W* self)
{
    if (fn_802CB900(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A6F0) != 0) {
        if (fn_802CB900(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A774) != 0) {
            self->field_0x416 = 2;
        } else {
            self->field_0x416 = 1;
        }
    } else {
        self->field_0x416 = 0;
    }
    if (self->field_0x416 == 0) {
        self->field_0x38C = self->field_0x38C + 1;
        if (self->field_0x38C >= 0x12C) {
            self->field_0x38C = 0x12C;
            self->field_0x38E = 1;
        }
    } else if (self->field_0x416 == 1) {
        self->field_0x38C = self->field_0x38C + 1;
        if (self->field_0x38C >= 0x96) {
            self->field_0x38C = 0x96;
            self->field_0x38E = 1;
        }
    } else {
        self->field_0x38C = 0;
        self->field_0x38E = 0;
    }
}

/* 0x802D1D90 - the band's per-frame driver: runs the whole chain of sub-steps and then the model
 * transform. */
void fn_802D1D90(struct _AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    u8 i;

    fn_80041E40(&self->pos_0x184, &self->pos_0x178);
    self->field_0x3C4 = 0;
    self->field_0x417 = 0;
    self->field_0x450 = 0;
    self->field_0x46C = 0;
    if (self->area != plw->area_0x16) {
        fn_802D1954(self);
    }
    fn_802D36B8(self);
    fn_802D1AFC(self);
    fn_802D1C90(self);
    fn_802D8478(self);
    fn_802CD770(self);
    if (fn_802D94A0(self) == 1) {
        fn_802D9A40(self);
    }
    if (fn_802D948C(self) == 1) {
        fn_802D93B8(self);
    } else {
        fn_802CDB10(self);
    }
    fn_802CDA1C(self);
    if (fn_802D2264(self) == 0 && self->motion != 4 && self->motion != 5) {
        fn_802D0F34(self);
    }
    fn_802D6D4C(self);
    fn_802D6DF4(self);
    fn_802D214C(self);
    fn_802CB858(self);
    if (self->field_0x174 != 0) {
        fn_802CB858(self);
    }
    if (fn_802D2B78(self, 2) != 0 || self->area != get_now_areano()) {
        self->in_area = 0;
    } else {
        self->in_area = 1;
    }
    self->model.field_0x34 = self->in_area;
    if (self->field_0x002 == 0) {
        self->model.field_0x34 = self->field_0x002;
    }
    fn_802D18E8(self);
    fn_802D6888(self);
    fn_802D1FFC(self);
    self->model.move(0);
    fn_80041E40(&self->pos_0x178, &self->model.pos_0x04);
    if (self->variant != 1) {
        if (self->variant != 2) {
            fn_800524C0(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, &self->pos_0x178,
                         lbl_8079A7B4);
        } else if (self->pos_0x184.y < lbl_8079A688 + self->field_0x19C) {
            fn_800524C0(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, &self->pos_0x178,
                         lbl_8079A73C);
        }
    }
    fn_802D15DC(self);
    fn_802D1710(self);
    fn_802D1FFC(self);
    fn_800E0914(&self->model);
    for (i = 0; i < 2; i++) {
        fn_8029EFDC(&self->hit[i]);
    }
    fn_8007F0CC(pRoot, self->model.field_0x118);
}

/* 0x802D1FFC - copies the model-transform words into the engine model and the position into its
 * own +0x04. */
void fn_802D1FFC(struct _AINPC_W* self)
{
    self->model.field_0x28 = self->vec_0x190.x;
    self->model.field_0x2C = self->vec_0x190.y;
    self->model.field_0x30 = self->vec_0x190.z;
    fn_80041E40(&self->model.pos_0x04, &self->pos_0x178);
}

/* 0x802D2024 - advances the four counter-attack counter blocks. */
u32 fn_802D2024(struct _AINPC_W* self)
{
    if (self->field_0x21E > 0 && ai_skill_ck(self, 0x1A) == 0) {
        self->field_0x21C = self->field_0x21C + self->field_0x21E;
        self->field_0x220 = 0x1E;
    }
    if (self->field_0x230 > 0) {
        self->field_0x22E = self->field_0x22E + self->field_0x230;
    }
    if (self->field_0x22A > 0) {
        self->field_0x228 = self->field_0x228 + self->field_0x22A;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x13, 0);
        } else {
            fn_802D2ABC(self, 4, 8, 0);
        }
        return 1;
    }
    if (self->field_0x224 > 0) {
        self->field_0x222 = self->field_0x222 + self->field_0x224;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x15, 0);
        } else {
            fn_802D2ABC(self, 4, 0x0A, 0);
        }
        return 1;
    }
    return 0;
}

/* 0x802D214C - runs down the aim-build timer and books the effect when it expires. */
void fn_802D214C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 offset;

    fn_80043EA8(&offset);
    setVector3(&offset, lbl_8079A670, lbl_8079A68C, lbl_8079A670);
    if (self->motion != 5) {
        s16 count = self->field_0x21C;

        if (count > 0) {
            self->field_0x220 = self->field_0x220 - 1;
            if (self->field_0x220 <= 0) {
                self->field_0x21C = count - 1;
                self->field_0x220 = 0x1E;
                fn_801075AC(self, &offset, 0x0E, 2, lbl_8079A674);
                fn_802D35EC(self, -1);
            }
        }
    }
}

/* 0x802D21F8 - the attack selectors that need no counter-attack support. */
s32 fn_802D21F8(u8 selector)
{
    if ((u32)(selector - 4) <= 5 || (u32)(selector - 0x1B) <= 4 || (u32)(selector - 0x12) <= 1 ||
        (s32)selector == 0x24) {
        return 1;
    }
    return 0;
}

/* 0x802D2238 - starts the "raise guard" motion. */
void fn_802D2238(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2ABC(self, 4, 0x16, 0);
    } else {
        fn_802D2ABC(self, 4, 0x0B, 0);
    }
}

/* 0x802D2264 - the counter-attack gate: rolls the guard, drains the window and dispatches the
 * support attack on +0x21A. */
s32 fn_802D2264(struct _AINPC_W* self)
{
    s16 window;
    u8 selector;
    s32 step = 0;
    s32 move;
    s32 commit;
    s32 near;
    s32 on_target;

    if (self->field_0x216 == 0) {
        return 0;
    }
    selector = self->field_0x21A;
    window = self->field_0x212;
    move = 0;
    commit = 1;
    near = 0;
    on_target = 0;
    if (fn_802D7B24(self) == 1) {
        near = 1;
    }
    if (fn_802D3F08(self) == 1) {
        on_target = 1;
        move = -1;
        if (fn_802D21F8(selector) == 0) {
            fn_802D2238(self);
        }
    } else {
        for (;;) {
        if (fn_802D2024(self) == 1) {
            step = 1;
        }
        if (window != 0) {
            fn_802D3A34(self, window);
            if (fn_802D35EC(self, (s16)-window) == 1) {
                move = 1;
                break;
            }
        }
        if (self->variant == 1 && fn_802D21F8(selector) == 0) {
            fn_802D2A00(self, 4, 2, 0);
        } else if (step == 0) {
            switch (selector) {
            case 1:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 2:
            case 3:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0D, 0);
                } else {
                    fn_802D2A00(self, 4, 1, 0);
                }
                move = 1;
                break;
            case 4:
                if (ai_skill_ck(self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0E, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 3, 0);
                }
                break;
            case 36:
                if (ai_skill_ck(self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x18, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 0x17, 0);
                }
                break;
            case 5:
                if (ai_skill_ck(self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0F, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 4, 0);
                }
                break;
            case 6:
            case 7:
            case 18:
                if (ai_skill_ck(self, 0x19) == 1 || self->field_0x23C >= 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x11, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 6, 0);
                }
                break;
            default:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
                move = -1;
                commit = -1;
                break;
            }
            }
            break;
        }
    }
    if (move >= 0) {
        fn_800DCB74(self->sound_handle_0x498, move, &self->pos_0x178);
    }
    if (commit >= 0) {
        fn_802D4200(self);
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x389 = 1;
        if (near != 0) {
            fn_800DCC24(self->sound_handle_0x498, 7, 0);
            fn_802D9D30(self, (s8)self->field_0x482, 9, 0, 2);
        }
        return 1;
    }
    return 0;
}
}

/* -------------------------------------------------------------------------------------------------
 * 0x802D27E0 .. 0x802D44F4
 * ---------------------------------------------------------------------------------------------- */

extern "C" {

/* 0x802D27E0 - the address of the band's shared `.bss` work block. */
u8* fn_802D27E0(void)
{
    return lbl_806BD360;
}

}  /* the three mangled `ai_*` symbols are C++: nodefaults keeps their real spelling */

/* 0x802D27EC - whether the NPC is in the area the game is currently in. */
s32 ai_area_ck(struct _AINPC_W* self)
{
    return self->area == get_now_areano();
}

extern "C" {

/* 0x802D282C - the two-level motion-table walk: row `index / 100` of `lbl_805D3FB8` holds that
 * row's 100-entry column and the entry is `column[index % 100]`. */
s32 fn_802D282C(s32 index)
{
    u16 value = index;
    s32* row = (s32*)lbl_805D3FB8[value / 100];

    if (row == NULL) {
        return 0;
    }
    return row[value % 100];
}

/* 0x802D287C - plays `motion` on the model with the row `fn_802D282C` resolves. */
void fn_802D287C(struct _AINPC_W* self, s32 motion, s32 a, s32 b, s32 c)
{
    fn_800E11C0(&self->model, 3, 0, (u16)motion, a, fn_802D282C((u16)motion), b, (f32)c,
                lbl_8079A678);
}

/* 0x802D2904 - the same with the default row. */
void fn_802D2904(struct _AINPC_W* self, s32 motion, s32 a, s32 b)
{
    fn_802D287C(self, (u16)motion, a, b, 0);
}

/* 0x802D2910 - plays the motion only when the model is not already on it. */
void fn_802D2910(struct _AINPC_W* self, s32 motion, s32 a, s32 b)
{
    if ((u16)motion != ai_get_motion_no(self)) {
        fn_802D287C(self, (u16)motion, a, b, 0);
    }
}

/* 0x802D2984 - resets the model's motion layer. */
void fn_802D2984(struct _AINPC_W* self)
{
    MHchar* model = &self->model;

    fn_800E2198(model, 0);
}

/* 0x802D2990 - plays `motion` as a loop. */
void fn_802D2990(struct _AINPC_W* self, s32 motion)
{
    MHchar* model = &self->model;

    fn_800E16DC(model, (u16)motion, 0);
}

/* 0x802D29A0 - plays `motion` as a one-shot. */
void fn_802D29A0(struct _AINPC_W* self, s32 motion)
{
    MHchar* model = &self->model;

    fn_800E16DC(model, (u16)motion, 1);
}

}  /* `ai_get_motion_no__FP8_AINPC_W` is C++ */

/* 0x802D29B0 - the motion number the model is playing. */
u16 ai_get_motion_no(struct _AINPC_W* self)
{
    return self->model.motion_no_0x50;
}

extern "C" {

/* 0x802D29B8 - arms the model's second state flag. */
void fn_802D29B8(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF2 = value;
}

/* 0x802D29C0 - clears it. */
void fn_802D29C0(struct _AINPC_W* self)
{
    self->model.field_0xF2 = 0;
}

/* 0x802D29CC - sets the model's speed to the record's +0x44C times `scale`. */
void fn_802D29CC(struct _AINPC_W* self, f32 scale)
{
    fn_800E1640(&self->model, self->field_0x44C * scale);
}

/* 0x802D29DC - the model rotation component the scale setters read back. */
f32 fn_802D29DC(struct _AINPC_W* self)
{
    return self->model.field_0x5C;
}

/* 0x802D29E4 - arms the model's first state flag. */
void fn_802D29E4(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF1 = value;
}

/* 0x802D29EC - clears it. */
void fn_802D29EC(struct _AINPC_W* self)
{
    self->model.field_0xF1 = 0;
}

/* 0x802D29F8 - selects the locomotion arm. */
void fn_802D29F8(struct _AINPC_W* self, s8 variant)
{
    self->variant = variant;
}

/* 0x802D2A00 - the motion state setter: latches the gauge word, saves the outgoing motion and
 * sub-step, clears the three follow-up bytes, re-arms the model flags and sets the speed. */
void fn_802D2A00(struct _AINPC_W* self, u8 motion, u16 step, s16 gauge)
{
    self->field_0x1EC = gauge;
    self->field_0x004 = 0;
    self->field_0x005 = 0;
    self->field_0x006 = 0;
    self->prev_motion = self->motion;
    self->prev_motion_step = self->motion_step;
    self->motion = motion;
    self->motion_step = step;
    fn_802D29C0(self);
    fn_802D29EC(self);
    self->field_0x002 = 1;
    self->field_0x44C = lbl_8079A678;
    if (ai_skill_ck(self, 0x11) == 1 && motion == 2 && self->variant != 2) {
        self->field_0x44C = lbl_8079A6F8;
    }
    fn_802D29CC(self, lbl_8079A678);
}

/* 0x802D2ABC - the same setter through the "pending" byte the drivers test. */
void fn_802D2ABC(struct _AINPC_W* self, s32 motion, s32 step, s32 gauge)
{
    self->field_0x174 = 1;
    fn_802D2A00(self, motion, step, gauge);
}

/* 0x802D2AD4 - switches the locomotion arm and re-seeds the motion. */
void fn_802D2AD4(struct _AINPC_W* self, u8 variant)
{
    self->variant = variant;
    if (variant != 2) {
        fn_802D2ABC(self, 0, 0, 0);
    } else {
        fn_802D2ABC(self, 0, 1, 0);
    }
    fn_802D41C8(self);
}

/* 0x802D2B38 - whether the record is already on the given motion and sub-step. */
s32 fn_802D2B38(struct _AINPC_W* self, s32 motion, s32 step)
{
    if (self->motion == (u8)motion && self->motion_step == (u16)step) {
        return 1;
    }
    return 0;
}

/* 0x802D2B68 - resets the model's joint layer. */
void fn_802D2B68(struct _AINPC_W* self)
{
    fn_800E0A14(&self->model);
}

}  /* `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3` is C++ */

/* 0x802D2B70 - the model's joint world position. */
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out)
{
    self->model.get_joint_wpos(joint, out);
}

extern "C" {

/* 0x802D2B78 - tests the record's flag word. */
s32 fn_802D2B78(struct _AINPC_W* self, s32 mask)
{
    return self->field_0x1EC & (u16)mask;
}

/* 0x802D2B88 - sets flags in it. */
void fn_802D2B88(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC |= flags;
}

/* 0x802D2B98 - clears flags in it. */
void fn_802D2B98(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC &= ~flags;
}

/* 0x802D2BB0 - fills hit entry `hit` for the attack the NPC is running: the selector bytes, then
 * one OR per skill the NPC owns, the random weak-point roll, and the "blocked" case that zeroes the
 * entry again. */
void fn_802D2BB0(struct _AINPC_W* self, struct _HIT_W* hit)
{
    u8 applied = 0;

    hit->field_0x31 = 0x0A;
    hit->field_0x32 = 4;
    if (ai_skill_ck(self, 2) == 1) {
        hit->field_0x48 |= 0x10;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (ai_skill_ck(self, 3) == 1) {
        hit->field_0x48 |= 0x20;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (ai_skill_ck(self, 4) == 1) {
        hit->field_0x48 |= 0x200;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (ai_skill_ck(self, 5) == 1) {
        hit->field_0x48 |= 0x40;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (ai_skill_ck(self, 6) == 1) {
        hit->field_0x48 |= 0x80;
        hit->field_0x4A = self->field_0x210;
        applied = 1;
    }
    if (ran_suu(1) % 3 == 0) {
        if (ai_skill_ck(self, 8) == 1) {
            hit->field_0x48 |= 2;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
        if (ai_skill_ck(self, 9) == 1) {
            hit->field_0x48 |= 4;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
        if (ai_skill_ck(self, 0x0A) == 1) {
            hit->field_0x48 |= 1;
            hit->field_0x4A = self->field_0x210;
            applied |= 2;
        }
    }
    if (ai_skill_ck(self, 7) == 1 && (applied & 1) != 0) {
        hit->field_0x4A = (hit->field_0x4A * 0x82) / 100;
    }
    if (ai_skill_ck(self, 0x0B) == 1 && (applied & 2) != 0) {
        hit->field_0x4A = (hit->field_0x4A * 0x82) / 100;
    }
    if (self->field_0x420 == 1 && self->field_0x422 > 0) {
        hit->field_0x42 = 0x11;
    }
    if (ai_skill_ck(self, 0x0D) == 1 && ran_suu(1) % 100 < 0x14) {
        hit->field_0x40 = (hit->field_0x40 * 0x7D) / 100;
        hit->field_0x4C |= 0x80;
    }
    if (ai_skill_ck(self, 0x0C) == 1) {
        hit->field_0x4E = (s8)(hit->field_0x4E * 0x78) / 100;
    }
    if (fn_802D2B78(self, 4) != 0) {
        hit->field_0x31 = 1;
        hit->field_0x40 = 0;
        if (self->field_0x34E == 1) {
            hit->field_0x42 = 2;
        }
        hit->field_0x4E = 0;
        hit->field_0x48 = 0;
        hit->field_0x4A = 0;
    }
    if ((applied & 3) != 0) {
        hit_flag_set(hit, 0x800);
    }
}

/* 0x802D2F7C - fills hit entry `index` from motion-table entry `row` and hands it to the attack
 * filler. */
void fn_802D2F7C(struct _AINPC_W* self, u8 index, s32 row)
{
    struct _HIT_W* hit = &self->hit[index];
    struct MotionEntry* entry = (struct MotionEntry*)(lbl_805D3AD8 + row * 0x1A);
    f32 limit;

    hit->field_0x08 = lbl_80792508[entry->motion_id_0x0F];
    fn_8029F538(hit);
    hit_flag_set(hit, 0x320);
    hit->motion_no_0x18 = ai_get_motion_no(self);
    hit->field_0x1A = 0;
    hit->field_0x1C = 0;
    hit->field_0x1E = 0;
    limit = self->model.field_0x44 - lbl_8079A730;
    if (limit < lbl_8079A670 || (self->model.field_0x4C & 1) != 0) {
        limit = lbl_8079A670;
    }
    hit->offset_0x38 = entry->approach_0x00;
    hit->offset_0x3C = entry->retreat_0x02;
    if (entry->approach_0x00 != 0) {
        hit->offset_0x38 = hit->offset_0x38 + (limit - self->model.field_0x74);
        if (hit->offset_0x38 < lbl_8079A670) {
            hit->offset_0x3C = hit->offset_0x3C + hit->offset_0x38;
            hit->offset_0x38 = lbl_8079A670;
            if (hit->offset_0x3C == lbl_8079A670) {
                hit->field_0x05 = 0;
                return;
            }
        }
    }
    fn_8029F204(hit, entry, lbl_8079A670);
    fn_802D2BB0(self, hit);
}

/* 0x802D30F8 - clamps the angle difference to `limit` degrees, taking the short way round. */
u16 fn_802D30F8(u16 angle, s32 target, u16 limit)
{
    u16 diff = angle - target;
    u16 result;

    if (fn_8045AB38((s16)diff) < limit) {
        result = angle;
    } else {
        result = target - limit;
        if (diff < -0x8000) {
            result = target + limit;
        }
    }
    return result;
}

/* 0x802D3184 - whether the model's facing is outside `limit` degrees of the record's +0x194. */
s32 fn_802D3184(struct _AINPC_W* self, u32 limit)
{
    u16 diff = self->vec_0x190.y - calcVecAng2(&self->pos_0x178, &self->pos_0x1B0);

    if (diff <= (u16)(limit * -1) && diff >= (u16)limit) {
        return 0;
    }
    return 1;
}

/* 0x802D31FC - zeroes the model offset vector at +0x1D4. */
void fn_802D31FC(struct _AINPC_W* self)
{
    self->pos_0x1D4.x = lbl_8079A670;
    self->pos_0x1D4.y = lbl_8079A670;
    self->pos_0x1D4.z = lbl_8079A670;
}

/* 0x802D3210 - builds the offset vector at +0x1D4 from the two angles at `angles[0]`/`angles[1]`
 * and adds it to the position. */
void fn_802D3210(struct _AINPC_W* self, u32* angles)
{
    nw4r::math::VEC3 offset;

    fn_80043EA8(&offset);
    fn_80041E40(&offset, &self->pos_0x1D4);
    rotVecX(&offset, angles[0]);
    rotVecY(&offset, angles[1]);
    fn_80073F68(&self->pos_0x178, &offset);
}

/* 0x802D327C - the stored variant, then the same offset walk from +0x1D4. */
void fn_802D327C(struct _AINPC_W* self, u32* angles)
{
    fn_802D3210(self, angles);
    fn_80073F68(&self->pos_0x1D4, &self->pos_0x1E0);
}

/* 0x802D32B4 - rolls one of the six-byte attack rows the skill table points at, weighted by the
 * rows' first word, and starts the motion the row names. */
s32 fn_802D32B4(struct _AINPC_W* self, u16* row)
{
    u16 total = 0;
    u16 roll;
    u16 walked = 0;
    u16* walk = row;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 3;
    }
    if (total == 0) {
        return 0;
    }
    roll = ran_suu(1) % total;
    for (; *row != 0xFFFF; row += 3) {
        walked += *row;
        if (roll < walked) {
            fn_802D2A00(self, (u8)row[1], row[2], 0);
            return 1;
        }
    }
    return 0;
}

/* 0x802D3398 - the same for the four-byte rows the dispatchers use. */
s32 fn_802D3398(struct _AINPC_W* self, u16* row)
{
    u16 total = 0;
    u16 roll;
    u16 walked = 0;
    u16* walk = row;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 2;
    }
    if (total == 0) {
        return 0;
    }
    roll = ran_suu(1) % total;
    for (; *row != 0xFFFF; row += 2) {
        walked += *row;
        if (roll < walked) {
            fn_802D41E0(self, (u8)row[1], 1);
            return 1;
        }
    }
    return 0;
}

/* 0x802D3474 - the same motion setter for the "blocked" arm. */
void fn_802D3474(struct _AINPC_W* self, s32 motion, s32 step)
{
    fn_802D2ABC(self, 5, (u16)motion, (u16)step);
}

/* 0x802D348C - drops the aim/attack state: clears the gauge, resets the attack bookkeeping and
 * re-seeds the motion (the recorder's counter bumps below 7 while the skill is held). */
void fn_802D348C(struct _AINPC_W* self)
{
    if (self->motion != 5) {
        self->gauge_0x1F0 = 0;
        fn_802D3AD8(self);
        fn_802D3CFC(self, 0);
        fn_802D3D84(self);
        fn_802D4200(self);
        self->field_0x389 = 1;
        if (fn_802D948C(self) == 1) {
            fn_802D3474(self, 9, 0);
            return;
        }
        if (self->variant != 2) {
            fn_802D3474(self, 0, 0);
        } else {
            fn_802D3474(self, 4, 0);
        }
        if (ai_skill_ck(self, 0x15) == 1) {
            if (self->field_0x43C < 7) {
                self->field_0x43C = self->field_0x43C + 1;
                self->field_0x1FE = (self->field_0x1FE * 0x6A) / 100;
                self->field_0x200 = (self->field_0x200 * 0x6A) / 100;
            }
        }
    }
}

/* 0x802D35B4 - adjusts the motion gauge and clamps it to [0, +0x1F4]. */
void fn_802D35B4(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_0x1F0 + (s16)delta;

    self->gauge_0x1F0 = value;
    if (value <= 0) {
        self->gauge_0x1F0 = 0;
    }
    if (self->gauge_0x1F0 >= self->gauge_max_0x1F4) {
        self->gauge_0x1F0 = self->gauge_max_0x1F4;
    }
}

/* 0x802D35EC - drains the gauge (zeroed while the player is down) and reports whether it ran out. */
u32 fn_802D35EC(struct _AINPC_W* self, s32 delta)
{
    if (fn_8027BC48(0) == 1) {
        delta = 0;
    }
    fn_802D35B4(self, delta);
    if (fn_802D948C(self) == 1) {
        fn_802D92F8(self);
    }
    if (self->gauge_0x1F0 <= 0 && self->variant != 1) {
        fn_802D348C(self);
        return 1;
    }
    return 0;
}

/* 0x802D3684 - re-seeds the gauge from the +0x1F8 value. */
void fn_802D3684(struct _AINPC_W* self)
{
    self->gauge_max_0x1F4 = self->gauge_refill_0x1F8;
    self->gauge_0x1F0 = self->gauge_refill_0x1F8;
}

/* 0x802D3694 - raises the gauge cap by `delta`, clamped at 150. */
void fn_802D3694(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_max_0x1F4 + (s16)delta;

    self->gauge_max_0x1F4 = value;
    if (value >= 0x96) {
        self->gauge_max_0x1F4 = 0x96;
    }
}

/* 0x802D36B8 - the frame step: runs down every one of the band's countdowns. */
void fn_802D36B8(struct _AINPC_W* self)
{
    if (self->field_0x214 > 0) {
        self->field_0x214 = self->field_0x214 - 1;
    }
    if (self->field_0x35A > 0) {
        self->field_0x35A = self->field_0x35A - 1;
    }
    if (self->field_0x35E > 0) {
        self->field_0x35E = self->field_0x35E - 1;
    }
    if (self->field_0x352 > 0) {
        self->field_0x352 = self->field_0x352 - 1;
        if (self->field_0x352 == 1) {
            self->field_0x354 = 3;
        }
    }
    if (self->field_0x34A != 0) {
        self->field_0x34A = self->field_0x34A - 1;
    }
    if (self->field_0x3CA > 0) {
        self->field_0x3CA = self->field_0x3CA - 1;
    }
    if (self->field_0x3D0 > 0) {
        self->field_0x3D0 = self->field_0x3D0 - 1;
    }
    if (self->field_0x43E > 0) {
        self->field_0x43E = self->field_0x43E - 1;
    }
    if (self->field_0x422 > 0) {
        self->field_0x422 = self->field_0x422 - 1;
        if (self->field_0x420 == 1 && (self->field_0x422 == 0 || self->variant == 2)) {
            self->field_0x422 = 0;
            fn_800DCCF8(self->sound_handle_0x498, &self->pos_0x178, 1);
        }
    }
    if (self->field_0x424 > 0) {
        self->field_0x424 = self->field_0x424 - 1;
    }
    if (self->field_0x374 > 0) {
        self->field_0x374 = self->field_0x374 - 1;
    }
    if (self->field_0x236 > 0) {
        self->field_0x236 = self->field_0x236 - 1;
        if (self->field_0x236 == 0) {
            self->field_0x235 = 0;
        }
    }
    if (self->field_0x23A > 0) {
        self->field_0x23A = self->field_0x23A - 1;
        if (self->field_0x23A == 0) {
            self->field_0x239 = 0;
        }
    }
    if (self->field_0x3D4 < self->field_0x3D6) {
        self->field_0x3DC = self->field_0x3DC - 1;
        if (self->field_0x3DC <= 0) {
            fn_802D6B2C(self, 1);
        }
    }
    if (self->field_0x23E > 0) {
        self->field_0x23E = self->field_0x23E - 1;
        if (self->field_0x23E <= 0) {
            self->field_0x23E = 0;
            self->field_0x23C = 0;
        }
    }
    if (self->field_0x408 > 0) {
        self->field_0x408 = self->field_0x408 - 1;
    }
    if (self->field_0x40A > 0) {
        self->field_0x40A = self->field_0x40A - 1;
    }
    if (self->field_0x40C > 0) {
        self->field_0x40C = self->field_0x40C - 1;
    }
    if (self->field_0x40E > 0) {
        self->field_0x40E = self->field_0x40E - 1;
    }
    if (self->field_0x410 > 0) {
        self->field_0x410 = self->field_0x410 - 1;
    }
    if (self->field_0x412 > 0) {
        self->field_0x412 = self->field_0x412 - 1;
    }
    if (self->field_0x47C > 0) {
        self->field_0x47C = self->field_0x47C - 1;
    }
    if (self->field_0x46A > 0) {
        self->field_0x46A = self->field_0x46A - 1;
    }
    if (self->field_0x38A > 0) {
        self->field_0x38A = self->field_0x38A - 1;
    }
    if (self->field_0x486 > 0) {
        self->field_0x486 = self->field_0x486 - 1;
    }
}

/* 0x802D3984 - the 0..0x14 progress interpolation `high` -> `low`. */
s16 fn_802D3984(struct _AINPC_W* self, s32 high, s32 low, u32 reverse)
{
    s16 hi = high;
    s16 lo = low;
    s16 progress;

    if ((u8)reverse == 0) {
        progress = self->field_0x339;
    } else {
        progress = 0x14 - self->field_0x339;
    }
    return lo + (progress * (hi - lo)) / 20;
}

/* 0x802D39DC - seeds the entry timer from that interpolation. */
void fn_802D39DC(struct _AINPC_W* self)
{
    self->field_0x35A = fn_802D3984(self, 0x1E, 0x0A, 1) * 0x1E;
}

/* 0x802D3A20 - clears the +0x34F counter. */
void fn_802D3A20(struct _AINPC_W* self)
{
    self->field_0x34F = 0;
}

/* 0x802D3A2C - arms the +0x214 countdown. */
void fn_802D3A2C(struct _AINPC_W* self, s16 value)
{
    self->field_0x214 = value;
}

/* 0x802D3A34 - adds to the +0x350 counter (halved without the skill) and clamps it to the
 * model's limit. */
void fn_802D3A34(struct _AINPC_W* self, s16 delta)
{
    s16 limit;

    if (ai_skill_ck(self, 0x17) == 1) {
        self->field_0x350 = self->field_0x350 + delta;
    } else {
        self->field_0x350 = self->field_0x350 + (s16)(delta / 2);
    }
    limit = fn_802D9D14(self);
    if (self->field_0x350 > limit) {
        self->field_0x350 = limit;
    }
    if (self->field_0x350 < 0) {
        self->field_0x350 = 0;
    }
}

/* 0x802D3AD8 - clears the three +0x350..+0x354 words. */
void fn_802D3AD8(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        self->field_0x350 = 0;
        self->field_0x354 = 0;
        self->field_0x352 = 0;
    }
}

/* 0x802D3AF8 - clears the four +0x348..+0x34B bytes. */
void fn_802D3AF8(struct _AINPC_W* self)
{
    self->field_0x348 = 0;
    self->field_0x349 = 0;
    self->field_0x34A = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B10 - clears +0x34D and arms +0x34E. */
void fn_802D3B10(struct _AINPC_W* self)
{
    self->field_0x34D = 0;
    self->field_0x34E = 0xFF;
}

/* 0x802D3B24 - clears +0x382 and +0x34B. */
void fn_802D3B24(struct _AINPC_W* self)
{
    self->field_0x382 = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B34 - advances the three +0x3C6/+0x3CC/+0x480 timers and reports whether the move the
 * caller handed in is a repeat of the last accepted one. */
s32 fn_802D3B34(struct _AINPC_W* self, u16 limit, u8 copy)
{
    u32 moved = 0;

    if (self->field_0x3C8 > 0) {
        self->field_0x3C6 = self->field_0x3C6 + 1;
        if (self->field_0x3C6 >= (s32)limit) {
            self->field_0x3C6 = limit;
            moved = 1;
        }
    } else {
        self->field_0x3C6 = 0;
    }
    if (self->field_0x3CE > 0) {
        self->field_0x3CC = self->field_0x3CC + 1;
        if (self->field_0x3CC >= (s32)limit) {
            self->field_0x3CC = limit;
            moved = 1;
        }
    } else {
        self->field_0x3CC = 0;
    }
    if (self->field_0x47E != 0) {
        self->field_0x480 = self->field_0x480 + 1;
        if (self->field_0x480 >= (s32)limit) {
            self->field_0x480 = limit;
            moved = 1;
            self->field_0x389 = 0;
        }
    } else {
        self->field_0x480 = 0;
    }
    if (moved == 1) {
        if (copy == 1) {
            fn_80041E40(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        if ((self->field_0x358 != 0 || self->field_0x35C != 0) &&
            calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0) < lbl_8079A6E0) {
            fn_80041E40(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x802D3CB8 - clears the three move timers. */
void fn_802D3CB8(struct _AINPC_W* self)
{
    self->field_0x3C6 = 0;
    self->field_0x3CC = 0;
    self->field_0x480 = 0;
}

/* 0x802D3CCC - arms the +0x3CA countdown. */
void fn_802D3CCC(struct _AINPC_W* self, s16 value)
{
    self->field_0x3CA = value;
}

/* 0x802D3CD4 - arms the +0x3D0 countdown. */
void fn_802D3CD4(struct _AINPC_W* self, s16 value)
{
    self->field_0x3D0 = value;
}

/* 0x802D3CDC - clears the two +0x22E block words. */
void fn_802D3CDC(struct _AINPC_W* self)
{
    self->field_0x22E = 0;
    self->field_0x232 = 0;
}

/* 0x802D3CEC - clears the two +0x222 block words. */
void fn_802D3CEC(struct _AINPC_W* self)
{
    self->field_0x222 = 0;
    self->field_0x226 = 0;
}

/* 0x802D3CFC - clears the whole motion-gauge block; `full` also drops the +0x234..+0x23E run. */
void fn_802D3CFC(struct _AINPC_W* self, u8 full)
{
    fn_802D3CDC(self);
    fn_802D3CEC(self);
    self->field_0x21C = 0;
    self->field_0x220 = 0;
    self->field_0x228 = 0;
    self->field_0x22C = 0;
    if (full == 0) {
        self->field_0x234 = 0;
        self->field_0x235 = 0;
        self->field_0x238 = 0;
        self->field_0x239 = 0;
        self->field_0x236 = 0;
        self->field_0x23A = 0;
        self->field_0x23C = 0;
        self->field_0x23E = 0;
    }
    fn_802D6888(self);
}

/* 0x802D3D84 - resets the band's whole attack/aim bookkeeping. */
void fn_802D3D84(struct _AINPC_W* self)
{
    self->field_0x35C = 0;
    self->field_0x358 = 0;
    self->field_0x3DE = 0;
    self->field_0x461 = 0;
    self->field_0x451 = 0;
    fn_802D3AF8(self);
    fn_802D3B10(self);
    fn_802D3B24(self);
    fn_802D3A20(self);
    fn_802D4238(self);
}

/* 0x802D3DE8 - reports (and starts the follow-up motion) when the +0x22E counter has run out. */
s32 fn_802D3DE8(struct _AINPC_W* self)
{
    if (self->field_0x22E >= 0x96) {
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x14, 0);
        } else {
            fn_802D2ABC(self, 4, 9, 0);
        }
        return 1;
    }
    return 0;
}

/* 0x802D3E4C - the counter-attack roll. */
s32 fn_802D3E4C(struct _AINPC_W* self)
{
    s32 chance = 0x0A;

    self->field_0x321 = 0;
    if ((u32)(self->field_0x21A - 0x1B) <= 4) {
        return 0;
    }
    if (ai_skill_ck(self, 0x10) == 1) {
        chance = 0x28;
    }
    if (self->variant != 1 && ran_suu(1) % 100 < chance) {
        self->field_0x321 = 1;
        return 1;
    }
    return 0;
}

/* 0x802D3F08 - whether the counter-attack roll latched. */
u32 fn_802D3F08(struct _AINPC_W* self)
{
    return self->field_0x321 != 0;
}

/* 0x802D3F1C - the highest weak-point rate of the two hit entries. */
u8 fn_802D3F1C(struct _AINPC_W* self)
{
    u8 highest = 0xFF;
    u8 value = self->hit[0].field_0x0E;

    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    value = self->hit[1].field_0x0E;
    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    return highest;
}

/* 0x802D3F70 - the percentage roll the +0x339 progress interpolates between `high` and `low`. */
s32 fn_802D3F70(struct _AINPC_W* self, u8 high, u8 low)
{
    return ran_suu(1) % 100 < low + ((self->field_0x339 * (high - low)) / 20);
}

/* 0x802D4020 - the formation's attack roll, raised by the skill. */
s32 fn_802D4020(struct _AINPC_W* self)
{
    u8 chance = self->formation_0x41C->attack_rate;

    if (ai_skill_ck(self, 0x14) == 1) {
        chance += 0x14;
    }
    if (ran_suu(1) % 100 < chance) {
        return 1;
    }
    return 0;
}

/* 0x802D40A4 - appends an entry to the 8-slot ring at +0x390. */
void fn_802D40A4(struct _AINPC_W* self, s16 id)
{
    self->slots[self->entry_index].id = id;
    self->slots[self->entry_index].count = 1;
    self->entry_index = self->entry_index + 1;
    if (self->entry_index >= 8) {
        self->entry_index = 0;
    }
}

/* 0x802D40EC - adds `delta` to the ring/pending entry `id` carries; a new id goes to the first free
 * pending slot. */
void fn_802D40EC(struct _AINPC_W* self, s32 id, s32 delta)
{
    u8 i;

    for (i = 8; i < 12; i++) {
        if ((u16)id == self->slots[i].id) {
            self->slots[i].count = self->slots[i].count + (s16)delta;
            if (self->slots[i].count <= 0) {
                self->slots[i].count = 0;
                self->slots[i].id = 0;
            }
            return;
        }
    }
    if (self->slots[8].id == 0 && delta > 0) {
        self->slots[8].id = id;
        self->slots[8].count = delta;
        return;
    }
    if (self->slots[9].id == 0 && delta > 0) {
        self->slots[9].id = id;
        self->slots[9].count = delta;
        return;
    }
    if (self->slots[10].id == 0 && delta > 0) {
        self->slots[10].id = id;
        self->slots[10].count = delta;
        return;
    }
    if (self->slots[11].id == 0 && delta > 0) {
        self->slots[11].id = id;
        self->slots[11].count = delta;
        return;
    }
}

/* 0x802D41C8 - latches +0x1CD. */
void fn_802D41C8(struct _AINPC_W* self)
{
    self->field_0x1CD = 1;
}

/* 0x802D41D4 - clears it. */
void fn_802D41D4(struct _AINPC_W* self)
{
    self->field_0x1CD = 0;
}

/* 0x802D41E0 - latches the attack selector; `reset` clears the two follow-up bytes. */
void fn_802D41E0(struct _AINPC_W* self, s8 selector, s32 reset)
{
    self->field_0x1CE = selector;
    if ((u8)reset == 1) {
        self->field_0x1CF = 0;
        self->field_0x1D0 = 0;
    }
}

/* 0x802D4200 - re-arms the attack step. */
void fn_802D4200(struct _AINPC_W* self)
{
    self->field_0x1D0 = 0;
    self->field_0x1CF = 1;
    self->field_0x389 = 1;
}

/* 0x802D4218 - selects the first follow-up. */
void fn_802D4218(struct _AINPC_W* self)
{
    self->field_0x1D1 = 1;
}

/* 0x802D4224 - selects the second. */
void fn_802D4224(struct _AINPC_W* self)
{
    self->field_0x1D1 = 2;
}

/* 0x802D4230 - stores the model offset factor. */
void fn_802D4230(struct _AINPC_W* self, f32 value)
{
    self->field_0x1BC = value;
}

/* 0x802D4238 - clears the two +0x376 bytes. */
void fn_802D4238(struct _AINPC_W* self)
{
    self->field_0x376 = 0;
    self->field_0x378 = 0;
}

/* 0x802D4248 - spawns the attack's effect: takes the joint's world position (or the record's own),
 * drops it by the entry's offset and maps the attack kind onto the effect id the emitter takes. */
void fn_802D4248(struct _AINPC_W* self, u8 a1, u8 kind, u32 joint, s32 effect, f32 scale)
{
    nw4r::math::VEC3 pos;
    u8 code = kind;

    fn_80043EA8(&pos);
    if ((self->field_0x324 & 0x4000) != 0 || a1 != 0) {
        return;
    }
    if (joint != 0xFF) {
        self->model.get_joint_wpos(joint, &pos);
    } else {
        fn_80041E40(&pos, &self->pos_0x178);
    }
    if ((self->field_0x324 & 6) != 0) {
        pos.y = lbl_8079A7B8 + self->field_0x1A0;
        switch (code) {
        case 0:
        case 17:
            code = 1;
            fn_801006A0(2, &pos, effect, self->area, 2, scale);
            break;
        case 6:
        case 7:
        case 41:
            code = 0x23;
            break;
        case 18:
            code = 1;
            fn_801006A0(2, &pos, effect, self->area, 2, scale);
            break;
        case 40:
            code = 0x0E;
            break;
        case 42:
        case 43:
        case 44:
            fn_801006A0(0x23, &pos, effect, self->area, 3, scale);
            return;
        case 45:
            fn_801006A0(0x23, &pos, effect, self->area, 1, scale);
            return;
        case 46:
            fn_801006A0(2, &pos, effect, self->area, 1, scale);
            return;
        case 47:
            fn_801006A0(1, &pos, effect, self->area, 1, scale);
            fn_801006A0(2, &pos, effect, self->area, 1, scale);
            return;
        case 48:
            fn_801006A0(1, &pos, effect, self->area, 1, scale);
            return;
        }
        fn_801006A0(code, &pos, effect, self->area, 2, scale);
    } else {
        pos.y = lbl_8079A7B8 + self->field_0x19C;
        switch (code) {
        case 2:
        case 42:
        case 46:
            return;
        case 40:
        case 43:
        case 45:
            code = 6;
            break;
        case 41:
            code = 0;
            break;
        case 44:
            code = 7;
            break;
        case 47:
            code = 0x12;
            break;
        case 48:
            code = 1;
            break;
        }
        fn_801006A0(code, &pos, effect, self->area, 2, scale);
    }
}
}
