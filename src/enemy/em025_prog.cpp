/*
 * enemy/em025_prog.cpp - enemy 025's program: the `_ENEMY_WORK` motion and action state machines, the frame-window
 *   effect passes, the per-area and quest tests, and the closing deleting destructor `fn_801AA0F8`.
 * RANGE. .text 0x8019E670-0x801AA154 (93 functions); extab 0x8000F124-0x8000F374, extabindex
 *   0x8002A8A0-0x8002AC18, .rodata 0x80570150-0x805701A0, .data 0x805AE750-0x805B06D0 (`em025_prog_tbl` first),
 *   .sdata 0x80791A80-0x80791A88, .sdata2 0x80798518-0x80798910.  Left edge: `fn_8019E604`,
 *   `enemy/em018_prog.cpp`'s static initializer, ends there and the 0.0 pool entry repeats at `lbl_80798538` from
 *   here.  The right edge is a GUESS inside the pool window 0x801AA0F8-0x801AA154 (the 0.0 entry repeats at
 *   `lbl_8079893C`, first read by `fn_801AA154`): `fn_801AA0F8` is a deleting destructor like `fn_8019E5A8`, which
 *   closes its own TU.
 * FLAGS. `cflags_main`, no `#pragma`.
 * NAMES. The file name follows the runtime dump's `em025_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.  `stage_map_kind_get` is the owner's `u8` form (`stage/stg_w.h`),
 *   `quest_flag_200000_ck` takes a `QuestRecord*` (`enemy/em_pop.h`) and `em_roster_record_release` is
 *   `em_pop.h`'s `u32`.
 * RESIDUALS. 36 rows unwritten: 0x8019EC38-0x8019ECD4, 0x8019ED34-0x8019EF40, 0x8019EF44-0x8019F07C,
 *   0x8019F378-0x8019F56C, 0x801A0210-0x801A05B4, 0x801A0684-0x801A0FEC, 0x801A10AC-0x801A12C8,
 *   0x801A1894-0x801A2DAC, 0x801A2E30-0x801A30D4, 0x801A30E8-0x801A3DBC, 0x801A3E90-0x801A4218,
 *   0x801A437C-0x801A4504, 0x801A9748-0x801A9C6C, 0x801A9CAC-0x801A9DF4, 0x801A9E60-0x801AA0F8.
 *   22 partial rows, including:
 *  - `fn_8019F56C`, `fn_801A1384`, `fn_801A1454`, `fn_801A16D4`: retail stores the decremented `timer_0x020` and
 *    tests it with a separate `cmpwi`, MWCC folds every spelling tried into `addic.` (4 bytes shorter);
 *  - `fn_8019F07C`: the `lbl_805AF9CC`/`lbl_805AFC08` stores materialise the address in r4 where retail uses r0;
 *  - `fn_801A4218`: one register or `cror` pair in the `em_frame_check` window pass;
 *  - `fn_801A9384`: retail keeps the bool conversions (`neg; or; srwi`, the `>= 1` borrow sequence, the
 *    trailing `clrlwi`s) and tests the float with `fcmpo` + `cror`; ours folds them and uses `fcmpu`.
 *   The other 15 partial rows have no recorded cause.  Relocations (`relocdiff --by-owner`): `fn_8019EA04` and
 *   `fn_801A4504` save from r27 (`_savegpr_27`/`_restgpr_27`) where retail saves from r26, and `fn_801A16D4`
 *   calls `fn_8013032C` once where retail does not.
 *   flipcheck: `.rodata`/`.sdata`/`.sdata2` claimed, not emitted; `.data` 0x2AC against 0x1F80; `.text` (0x71F0
 *   of 0xBAE4), extab (0x138 of 0x250) and extabindex (0x1D4 of 0x378) short of the claim and differing.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck */
#include "enemy/fn_8012EC74.h" /* em_get_mot_no, em_after_frame_check, get_joint_wmat_em */
#include "enemy/fn_8019ED34.h"
#include "lobby/lb_quest_screen.h" /* quest_rand_next (the owner's header, rule 2) */
#include "fn_8004CAD8.h" /* MTX34_ctor, mulVecMatAddTrans */
#include "unsplit/enemy.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcmaterial.h" /* fn_8006F304 (rule 2) */
#include "enemy/enemy_control.h"
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "enemy/em_pop.h"
#include "enemy/em_model.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "enemy/fn_801B0010.h"
#include "enemy/fn_8011D448.h"
#include "ef/fn_800CDB2C.h"
#include "g3d/g3d_anmchr.h"
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "sys_mem.h"
#include "enemy/fn_80138074.h"

extern "C" {
/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet. */
extern f32 lbl_80798518;
extern f32 lbl_8079851C;
extern f32 lbl_80798520;
extern f32 lbl_80798524;
extern f32 lbl_80798528;
extern f32 lbl_8079852C;
extern f32 lbl_80798538;

extern f32 lbl_80798544;
extern f32 lbl_80798548;

extern f32 lbl_80798560;
extern f32 lbl_80798564;
extern f32 lbl_80798568;
extern f32 lbl_8079856C;
extern f32 lbl_80798570;
extern f32 lbl_80798574;
extern f32 lbl_80798578;
extern f32 lbl_8079857C;
extern f32 lbl_80798580;
extern f32 lbl_80798584;
extern f32 lbl_80798588;
extern f32 lbl_8079858C;
extern f32 lbl_80798590;
extern f32 lbl_80798594;
extern f32 lbl_80798598;
extern f32 lbl_8079859C;
extern f32 lbl_807985A0;
extern f32 lbl_807985A4;
extern f32 lbl_807985A8;
extern f32 lbl_807985D4;
extern f32 lbl_807985D8;
extern f32 lbl_807985DC;
extern f32 lbl_807985E0;
extern f32 lbl_807985E4;
extern f32 lbl_807985E8;
extern f32 lbl_807985EC;
extern f32 lbl_807985F0;
extern f32 lbl_807987B4;

/* The unit's `.data` tables, declared: `fn_8019F07C` stores two into the record's dispatch words, the rest
 * are the dispatcher's motion tables. */
extern u8 lbl_805AE7C0[];
extern u8 lbl_805AF268[];
extern u8 lbl_805AF9CC[];
extern u8 lbl_805AFC08[];
extern u8 lbl_805B0478[];
extern u8 lbl_80570150[];

/* `enemy/fn_8011D448.cpp`'s status-bit helpers (`bits_0x824`), declared with these call sites' spelling. */
void fn_8011E5EC(struct _ENEMY_WORK* self);
void em_status_bits_set(struct _ENEMY_WORK* self, u32 mask);
void fn_8011E630(struct _ENEMY_WORK* self, u32 mask);

/* `stage/stg_w.cpp`'s blend helper: r3 is the `_ENEMY_WORK` (set by the caller), f1 the scalar. */
void fn_802B08DC(f32 a);

void fn_8019F2C0(struct _ENEMY_WORK* self);
void fn_8019F768(struct _ENEMY_WORK* self);
void fn_8019F9BC(struct _ENEMY_WORK* self);
void fn_8019FC10(struct _ENEMY_WORK* self);
void fn_8019FE80(struct _ENEMY_WORK* self);
void fn_801A00F0(struct _ENEMY_WORK* self);
void fn_801A0210(struct _ENEMY_WORK* self, u32 a);
void fn_801A0684(struct _ENEMY_WORK* self, u32 a);
void fn_8019F378(struct _ENEMY_WORK* self, u32 a);
void fn_8019F56C(struct _ENEMY_WORK* self);
void fn_8019F5F4(struct _ENEMY_WORK* self);
void fn_8019F690(struct _ENEMY_WORK* self);
void fn_8019F70C(struct _ENEMY_WORK* self);
void fn_801A01C8(struct _ENEMY_WORK* self);
void fn_801A05B4(struct _ENEMY_WORK* self);
void fn_801A07E8(struct _ENEMY_WORK* self, u32 a);
void fn_801A08C8(struct _ENEMY_WORK* self);
void fn_801A0988(struct _ENEMY_WORK* self, u32 a);
void fn_801A0AD0(struct _ENEMY_WORK* self, u32 a);
void fn_801A0C48(struct _ENEMY_WORK* self, u32 a);
void fn_801A0FEC(struct _ENEMY_WORK* self);
void fn_801A10AC(struct _ENEMY_WORK* self);
void fn_801A11B0(struct _ENEMY_WORK* self);
void fn_801A12C8(struct _ENEMY_WORK* self);
void fn_801A1384(struct _ENEMY_WORK* self, u32 a);
void fn_801A1454(struct _ENEMY_WORK* self);
void fn_801A14FC(struct _ENEMY_WORK* self);
void fn_801A1624(struct _ENEMY_WORK* self);
void fn_801A16D4(struct _ENEMY_WORK* self, u32 a);
void fn_801A17B8(struct _ENEMY_WORK* self);
void fn_801A1894(struct _ENEMY_WORK* self);
void fn_801A1964(struct _ENEMY_WORK* self);
void fn_801A1A4C(struct _ENEMY_WORK* self, u32 a);
void fn_801A1BB8(struct _ENEMY_WORK* self);
void fn_801A1E60(struct _ENEMY_WORK* self);
void fn_801A1F54(struct _ENEMY_WORK* self);
void fn_801A2014(struct _ENEMY_WORK* self);
void fn_801A2350(struct _ENEMY_WORK* self);
void fn_801A246C(struct _ENEMY_WORK* self, u32 a);
void fn_801A26B4(struct _ENEMY_WORK* self, u32 a);
void fn_801A28C8(struct _ENEMY_WORK* self, u32 a);
void fn_801A2DAC(struct _ENEMY_WORK* self);
void fn_801A2E30(struct _ENEMY_WORK* self);
void fn_801A30D4(struct _ENEMY_WORK* self);
void fn_801A30E8(struct _ENEMY_WORK* self);
void fn_801A35CC(struct _ENEMY_WORK* self);
void fn_801A3C54(struct _ENEMY_WORK* self);
void fn_801A3DBC(struct _ENEMY_WORK* self);
}

/* ---- 0x801A4504..0x801A9540 ---- */

/* More of the unit's `.sdata2` floats, declared, not defined. */
extern const f32 lbl_807985BC;
extern const f32 lbl_80798600;
extern const f32 lbl_80798610;
extern const f32 lbl_80798614;
extern const f32 lbl_80798640;
extern const f32 lbl_80798644;
extern const f32 lbl_80798654;
extern const f32 lbl_80798658;
extern const f32 lbl_80798668;
extern const f32 lbl_8079866C;
extern const f32 lbl_80798678;
extern const f32 lbl_8079867C;
extern const f32 lbl_80798680;
extern const f32 lbl_8079868C;
extern const f32 lbl_80798690;
extern const f32 lbl_807986A4;
extern const f32 lbl_807986B0;
extern const f32 lbl_807986B4;
extern const f32 lbl_807986C0;
extern const f32 lbl_807986D0;
extern const f32 lbl_807986D4;
extern const f32 lbl_807986E0;
extern const f32 lbl_807986E8;
extern const f32 lbl_807986EC;
extern const f32 lbl_807986F4;
extern const f32 lbl_807986F8;
extern const f32 lbl_80798704;
extern const f32 lbl_80798708;
extern const f32 lbl_80798710;
extern const f32 lbl_80798714;
extern const f32 lbl_80798734;
extern const f32 lbl_80798740;
extern const f32 lbl_80798744;
extern const f32 lbl_80798748;
extern const f32 lbl_80798750;
extern const f32 lbl_80798778;
extern const f32 lbl_8079877C;
extern const f32 lbl_80798780;
extern const f32 lbl_8079878C;
extern const f32 lbl_80798790;
extern const f32 lbl_807987B0;
extern const f32 lbl_807987B8;
extern const f32 lbl_807987BC;
extern const f32 lbl_807987C0;
extern const f32 lbl_807987C4;
extern const f32 lbl_807987C8;
extern const f32 lbl_807987CC;
extern const f32 lbl_807987D0;
extern const f32 lbl_807987D4;
extern const f32 lbl_807987D8;
extern const f32 lbl_807987DC;
extern const f32 lbl_807987E0;
extern const f32 lbl_807987E4;
extern const f32 lbl_807987E8;
extern const f32 lbl_807987EC;
extern const f32 lbl_807987F0;
extern const f32 lbl_807987F4;
extern const f32 lbl_807987F8;
extern const f32 lbl_807987FC;
extern const f32 lbl_80798800;
extern const f32 lbl_80798804;
extern const f32 lbl_80798808;
extern const f32 lbl_8079880C;
extern const f32 lbl_80798810;
extern const f32 lbl_80798814;
extern const f32 lbl_80798818;
extern const f32 lbl_8079881C;
extern const f32 lbl_80798820;
extern const f32 lbl_80798824;
extern const f32 lbl_80798828;
extern const f32 lbl_8079882C;
extern const f32 lbl_80798830;
extern const f32 lbl_80798834;
extern const f32 lbl_80798838;
extern const f32 lbl_8079883C;
extern const f32 lbl_80798840;
extern const f32 lbl_80798844;
extern const f32 lbl_80798848;
extern const f32 lbl_8079884C;
extern const f32 lbl_80798850;
extern const f32 lbl_80798854;
extern const f32 lbl_80798858;
extern const f32 lbl_8079885C;
extern const f32 lbl_80798860;
extern const f32 lbl_80798864;
extern const f32 lbl_80798868;
extern const f32 lbl_8079886C;
extern const f32 lbl_80798870;
extern const f32 lbl_80798874;
extern const f32 lbl_80798878;
extern const f32 lbl_8079887C;
extern const f32 lbl_80798880;
extern const f32 lbl_80798884;
extern const f32 lbl_80798888;
extern const f32 lbl_8079888C;
extern const f32 lbl_80798890;
extern const f32 lbl_80798894;
extern const f32 lbl_80798898;
extern const f32 lbl_8079889C;
extern const f32 lbl_807988A0;
extern const f32 lbl_807988A4;
extern const f32 lbl_807988A8;
extern const f32 lbl_807988AC;
extern const f32 lbl_807988B0;
extern const f32 lbl_807988B4;
extern const f32 lbl_807988B8;
extern const f32 lbl_807988BC;
extern const f32 lbl_807988C0;
extern const f32 lbl_807988C4;
extern const f32 lbl_807988C8;
extern const f32 lbl_807988CC;
extern const f32 lbl_807988D0;
extern const f32 lbl_807988D4;
extern const f32 lbl_807988D8;
extern const f32 lbl_807988DC;
extern const f32 lbl_807988E0;
extern const f32 lbl_807988E4;
extern const f32 lbl_807986B8;
extern const f32 lbl_807988E8;
extern const f32 lbl_807988EC;
extern const f32 lbl_807988F0;

/* `lobby/fn_8030121C.cpp`'s `eft_em_spawn`, in the `void*` spelling these call sites use (its callers
 * disagree on the parameter types). */
extern "C" void eft_em_spawn(struct _ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);

/* One record of the effect table at `get_enemy_data(self)+0x2C`: a joint id and the joint-local offset.
 * size: 0x10 */
struct EmDataEntry {
    /* +0x00 */ s32 joint;
    /* +0x04 */ nw4r::math::VEC3 vec;
};

/* This unit's view of the `get_enemy_data` record's +0x2C effect-table pointer (`EnemyData` pads it).
 * size: 0x30 */
struct _ENEMY_DATA_VIEW {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ EmDataEntry* table_0x2C;
};

extern "C" {
/* ------------------------------------------------------------------------------------------------
 * This unit's own functions.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8019E670 - the band's range test on the record's byte at +0x961: mode 0 `==`, 1 `>=`, 2 `<=`,
 * 3 the inclusive window. */
u32 fn_8019E670(struct _ENEMY_WORK* self, u32 mode, u32 lo, u32 hi) {
    switch ((u8)mode) {
    case 0:
        if (self->stack_0x961[0] == (u8)lo) {
            return 1;
        }
        break;
    case 1:
        if (self->stack_0x961[0] >= (u8)lo) {
            return 1;
        }
        break;
    case 2:
        if (self->stack_0x961[0] <= (u8)lo) {
            return 1;
        }
        break;
    case 3:
        if (self->stack_0x961[0] >= (u8)lo && self->stack_0x961[0] <= (u8)hi) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019E70C - the "which sub-phase of the shore action" query: 255 when the record is not the one
 * this table describes, else the phase the byte at +0x961 falls into. */
u32 fn_8019E70C(struct _ENEMY_WORK* self) {
    if (self == 0) {
        return 255;
    }
    if (self->team != 25) {
        return 255;
    }
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1 && quest_flag_200000_ck(0) != 1) {
        if (fn_8019E670(self, 3, 0, 3)) {
            return 0;
        }
        if (fn_8019E670(self, 3, 4, 19)) {
            return 1;
        }
        if (fn_8019E670(self, 3, 20, 59)) {
            return 3;
        }
        if (fn_8019E670(self, 3, 60, 70)) {
            return 4;
        }
        if (fn_8019E670(self, 3, 71, 91)) {
            return 2;
        }
    }
    return 255;
}

/* 0x8019E840 - the matching yes/no form of the same table. */
u32 fn_8019E840(struct _ENEMY_WORK* self) {
    if (self != 0) {
        if (self->team == 25) {
            if (stage_map_kind_get(self->field_0x1E0) == 6) {
                if (self->area_no == 1) {
                    if (quest_flag_200000_ck(0) != 1) {
                        if (fn_8019E670(self, 3, 30, 59) == 1) {
                            return 1;
                        }
                        if (fn_8019E670(self, 3, 90, 92) == 1) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* 0x8019E908 - arms/steps the +0x1E4 countdown: mode 0 steps it while bit 0 is set, mode 1 sets bit 0. */
void fn_8019E908(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 0) {
        if ((self->field_0x1E4 & 1) == 0) {
            return;
        }
        self->field_0x1E4--;
    } else if (mode == 1) {
        self->field_0x1E4 |= 1;
    }
}

/* 0x8019E948 - clears the four-byte action bit set at +0x354 (the target's four `stb`s are the
 * compiler's inlined `memset`). */
void fn_8019E948(struct _ENEMY_WORK* self) {
    self->mask_0x354[0] = 0;
    self->mask_0x354[1] = 0;
    self->mask_0x354[2] = 0;
    self->mask_0x354[3] = 0;
}

/* 0x8019E960 - sets bit `id` of that set: slot `id / 8` clamped to the last of the four bytes. */
void fn_8019E960(struct _ENEMY_WORK* self, s32 id) {
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    self->mask_0x354[slot] |= 1 << (id % 8);
}

/* 0x8019E9AC - the matching test. */
s32 fn_8019E9AC(struct _ENEMY_WORK* self, s32 id) {
    s32 slot = id / 8;
    if (slot >= 4) {
        slot = 3;
    }
    if (self->mask_0x354[slot] & (1 << (id % 8))) {
        return 1;
    }
    return 0;
}

/* 0x8019EA04 - releases every live enemy-control handle and re-arms the slot states. */
void fn_8019EA04(struct _ENEMY_WORK* self) {
    for (u16 i = 0; i < 4; i++) {
        if (self->handles_0x328[i] != -1) {
            em_roster_record_release(self->handles_0x328[i]);
            self->handles_0x328[i] = -1;
        }
        self->states_0x338[i] = 255;
    }
    self->field_0x761 = 0;
}

/* 0x8019EA80 - picks one of the free slot indices at random: the low 8 bits of +0x761 mark the used
 * slots, the RSO random source is folded down to the free count. */
u32 fn_8019EA80(struct _ENEMY_WORK* self, u16 idx) {
    u8 avail[8];
    u8 n = 0;
    u8 mask = self->field_0x761;
    u8 i;
    for (i = 0; i < 8; i++) {
        if ((mask & (1 << i)) == 0) {
            avail[n] = i;
            n++;
        }
    }
    if (n <= 4) {
        return 255;
    }
    s32 value = ((quest_rand_next() & 0xFFFF) + 0x157E7) >> (idx & 0xFFFF);
    value = (value * 13) & 0xFFFF;
    return avail[value % (s32)n];
}

/* 0x8019EBAC - steps the rotation word at +0x1C0 by 4 (clamped to the 0x4000..0x4004 band) and keeps
 * the height at +0x190 inside its two pool bounds, snapping to the middle value otherwise. */
void fn_8019EBAC(struct _ENEMY_WORK* self) {
    u32 angle = self->field_0x1C0 & 0xFFFF;
    self->field_0x1C0 = angle;
    if (angle >= 16388) {
        self->field_0x1C0 = angle - 4;
    } else if (angle > 16380) {
        self->field_0x1C0 = 16384;
    } else {
        self->field_0x1C0 = angle + 4;
    }
    if (self->pos.z >= lbl_80798518) {
        self->pos.z -= lbl_8079851C;
        return;
    }
    if (self->pos.z <= lbl_80798520) {
        self->pos.z += lbl_8079851C;
        return;
    }
    self->pos.z = lbl_80798524;
}

/* 0x8019ECD4 - arms the four enemy-control slots at +0x328 and seeds the record's remaining state,
 * then clears the bit set. */
void fn_8019ECD4(struct _ENEMY_WORK* self) {
    self->handles_0x328[0] = -1;
    self->states_0x338[0] = 255;
    self->handles_0x328[1] = -1;
    self->states_0x338[1] = 255;
    self->handles_0x328[2] = -1;
    self->states_0x338[2] = 255;
    self->handles_0x328[3] = -1;
    self->states_0x338[3] = 255;
    self->field_0x33C = 0;
    self->field_0x340 = 0;
    self->field_0x344 = 0;
    self->field_0x348 = lbl_80798528;
    self->field_0x352 = 1;
    self->field_0x34C = lbl_8079852C;
    self->field_0x350 = 0;
    self->field_0x353 = 0;
    self->field_0x358 = 0;
    fn_8019E948(self);
}

/* 0x8019EF40 - an empty virtual body (the class's no-op slot). */
void fn_8019EF40(void) {}

/* 0x8019F12C - the band's motion arming tail shared by several states: arm mode 4, set the
 * `fn_80128AAC` window (6, 9) and refresh through `fn_80133BB4`. */
void fn_8019F12C(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 9);
    fn_80133BB4(self);
}

/* 0x8019F174 - the same tail arming mode 0 with the `fn_80128AAC` window (1, 0). */
void fn_8019F174(struct _ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 1, 0);
    fn_80133BB4(self);
}

/* 0x8019F1BC - the two-state motion step: state 0 arms `em_mot_set_ck(self, 13, 30, 0)` plus the
 * `lbl_80798538` timer, state 1 waits for `em_mot_end_ck` and then runs `em_action_finish`. */
void fn_8019F1BC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        fn_801303EC(self, lbl_80798538);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F244 - the same shape arming `em_mot_set_ck(self, 1, 30, 0)` and completing through
 * `fn_801280F4`. */
void fn_8019F244(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019F2C0 - the per-sub-state dispatcher of the motion above: sub-states 0/1/2 arm through
 * `fn_8019F1BC`, sub-state 7 runs `fn_8019F244`. */
void fn_8019F2C0(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F1BC(self);
        break;
    case 1:
        fn_8019F1BC(self);
        break;
    case 2:
        fn_8019F1BC(self);
        break;
    case 7:
        fn_8019F244(self);
        break;
    }
}

/* 0x8019F2FC - state 0 arms `em_mot_set_ck(self, 21, 30, 0)`, state 1 completes through
 * `fn_8019F174`. */
void fn_8019F2FC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 21, 30, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019F56C - state 0 arms mode 13 (window 30) and the 150-frame `timer_0x020`; state 1 runs the
 * timer down and completes through `em_action_finish`. */
void fn_8019F56C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 13, 30, 0);
        self->timer_0x020 = 150;
        break;
    case 1: {
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* 0x8019F5F4 - state 0 arms `em_mot_set(self, 26, 20, 0)` plus the two `em_hit_window_set_default` windows
 * (0,20)/(1,21), state 1 completes through `fn_8019F174`. */
void fn_8019F5F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        em_hit_window_set_default(self, 0, 20);
        em_hit_window_set_default(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019F690 - state 0 arms `em_mot_set(self, 27, 20, 0)`, state 1 completes through
 * `em_action_finish`. */
void fn_8019F690(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 27, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F70C - the per-sub-state dispatcher of the motion above. */
void fn_8019F70C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F2FC(self);
        break;
    case 1:
        fn_8019F378(self, 1);
        break;
    case 2:
        fn_8019F378(self, 0);
        break;
    case 3:
        fn_8019F56C(self);
        break;
    case 4:
        fn_8019F5F4(self);
        break;
    case 5:
        fn_8019F690(self);
        break;
    }
}

/* 0x8019F768 - the first `fn_801A01C8` motion step: arm `em_mot_set(self, 20, 100, 0)`, then a ten-window
 * hit-window pass ending in the `em_approach_step` completion. */
void fn_8019F768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        em_approach_start(self, lbl_80798538, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_8079856C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798570, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_80798574, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, lbl_80798578, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079857C, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, lbl_80798580, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_80798584, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_approach_step(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019FC10 - the same ten-window pass as `fn_8019F768`, completing through `em_mot_end_ck` first
 * and `em_approach_step` second. */
void fn_8019FC10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 100, 0);
        em_approach_start(self, lbl_80798538, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 14, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_8079856C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798570, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 15, 3);
        }
        if (em_frame_check(self, 0, lbl_80798574, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 12, 3);
        }
        if (em_frame_check(self, 0, lbl_80798578, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079857C, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_frame_check(self, 0, lbl_80798580, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_80798584, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 13, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (em_approach_step(self, 0, 1) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019F9BC - the second motion step: the same ten-window pass against the other pooled floats
 * (`lbl_80798588`..) arming mode 22, completing through `fn_8019F174`. */
void fn_8019F9BC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        em_approach_start(self, lbl_80798538, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798588, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798590, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_80798594, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, lbl_80798598, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079859C, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_807985A0, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_approach_step(self, 0, 1) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x8019FE80 - the second motion step with the `fn_8019FC10` completion pair. */
void fn_8019FE80(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 22, 100, 0);
        em_approach_start(self, lbl_80798538, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798588, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 18, 3);
        }
        if (em_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798568, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, lbl_80798590, lbl_80798538) == 1) {
            em_hit_window_set(self, 0, 19, 3);
        }
        if (em_frame_check(self, 0, lbl_80798594, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 16, 3);
        }
        if (em_frame_check(self, 0, lbl_80798598, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_8079859C, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1) {
            em_hit_window_clear(self, 1);
        }
        if (em_frame_check(self, 0, lbl_807985A0, lbl_80798538) == 1) {
            em_hit_window_set(self, 1, 17, 3);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_8019F174(self);
        } else if (em_approach_step(self, 0, 1) == 1) {
            fn_8019F174(self);
        }
        break;
    }
}

/* 0x801A00F0 - the fourth `fn_801A01C8` motion step: motion 26 with two hit windows, then motion 27 blended,
 * then `em_action_finish`. */
void fn_801A00F0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 26, 20, 0);
        em_hit_window_set_default(self, 0, 20);
        em_hit_window_set_default(self, 1, 21);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 27, 20, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801A01C8 - the five-way motion dispatcher of the `fn_801A3DEC` action 2. */
void fn_801A01C8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8019F768(self);
        break;
    case 1:
        fn_8019F9BC(self);
        break;
    case 2:
        fn_8019FC10(self);
        break;
    case 3:
        fn_8019FE80(self);
        break;
    case 4:
        fn_801A00F0(self);
        break;
    }
}

/* 0x801A05B4 - action 3's two-way sub-state selector into `fn_801A0210` (0/1). */
void fn_801A05B4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A0210(self, 0);
        break;
    case 1:
        fn_801A0210(self, 1);
        break;
    }
}

/* 0x801A05E0 - action 6's first motion: arm `fn_8012F5C4(self, 1, 30, 0, 1)`, then wait on
 * `em_approach_step(self, 0, 0)` and complete through `fn_801280F4`. */
void fn_801A05E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_8012F5C4(self, 1, 30, 0, 1);
        fn_801303EC(self, lbl_807985A4);
        em_approach_start(self, lbl_80798538, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019F07C - selects the record's two dispatch words (0x8C8/0x8CC) from the area/motion, then
 * asks `fn_8019E840` which of the two pooled alpha values `fn_802B08DC` should blend. */
void fn_8019F07C(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        self->field_0x8C8 = fn_801260BC(self);
        self->field_0x8CC = fn_801260E0(self);
    } else if (self->action == 11) {
        self->field_0x8C8 = (u32)lbl_805AF9CC;
        self->field_0x8CC = (u32)lbl_805AFC08;
    } else {
        self->field_0x8C8 = (u32)lbl_805AE7C0;
        self->field_0x8CC = (u32)lbl_805AF268;
    }
    if (fn_8019E840(self) == 1) {
        fn_802B08DC(lbl_8079852C);
    } else {
        fn_802B08DC(lbl_80798528);
    }
}

/* 0x801A2DAC - the state_sub dispatcher of the `state`-band's second half: 0..14, with the double
 * entries 2/12, 8/11, 9/13, 10/14 passing the 0/1 selector to the four two-argument motion steps. */
void fn_801A2DAC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A1894(self);
        break;
    case 1:
        fn_801A1964(self);
        break;
    case 2:
        fn_801A1A4C(self, 0);
        break;
    case 3:
        fn_801A1BB8(self);
        break;
    case 4:
        fn_801A1E60(self);
        break;
    case 5:
        fn_801A1F54(self);
        break;
    case 6:
        fn_801A2014(self);
        break;
    case 7:
        fn_801A2350(self);
        break;
    case 8:
        fn_801A246C(self, 0);
        break;
    case 9:
        fn_801A26B4(self, 0);
        break;
    case 10:
        fn_801A28C8(self, 0);
        break;
    case 11:
        fn_801A246C(self, 1);
        break;
    case 12:
        fn_801A1A4C(self, 1);
        break;
    case 13:
        fn_801A26B4(self, 1);
        break;
    case 14:
        fn_801A28C8(self, 1);
        break;
    }
}

/* 0x801A17B8 - action 6's `state_sub` dispatcher (27 entries): the first slot runs this band's
 * `fn_801A05E0`, the rest dispatch into the motion-mode table by (function, mode). */
void fn_801A17B8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801A05E0(self);
        break;
    case 1:
        fn_801A0684(self, 0);
        break;
    case 2:
        fn_801A0684(self, 1);
        break;
    case 3:
        fn_801A0684(self, 2);
        break;
    case 4:
        fn_801A0684(self, 0);
        break;
    case 5:
        fn_801A0684(self, 0);
        break;
    case 6:
        fn_801A07E8(self, 0);
        break;
    case 7:
        fn_801A08C8(self);
        break;
    case 8:
        fn_801A0988(self, 0);
        break;
    case 9:
        fn_801A0AD0(self, 0);
        break;
    case 10:
        fn_801A0C48(self, 0);
        break;
    case 11:
        fn_801A0FEC(self);
        break;
    case 12:
        fn_801A10AC(self);
        break;
    case 13:
        fn_801A11B0(self);
        break;
    case 14:
        fn_801A12C8(self);
        break;
    case 15:
        fn_801A1384(self, 0);
        break;
    case 16:
        fn_801A1454(self);
        break;
    case 17:
        fn_801A0988(self, 1);
        break;
    case 18:
        fn_801A14FC(self);
        break;
    case 19:
        fn_801A1624(self);
        break;
    case 20:
        fn_801A16D4(self, 0);
        break;
    case 21:
        fn_801A16D4(self, 1);
        break;
    case 22:
        fn_801A0AD0(self, 1);
        break;
    case 23:
        fn_801A1384(self, 1);
        break;
    case 24:
        fn_801A0AD0(self, 2);
        break;
    case 25:
        fn_801A07E8(self, 1);
        break;
    case 26:
        fn_801A0AD0(self, 3);
        break;
    }
}

/* 0x801A30D4 - the tail thunk into `em_se_tbl_play_alt(self, &lbl_805B0478, 0, 0)`. */
void fn_801A30D4(struct _ENEMY_WORK* self) {
    em_se_tbl_play_alt(self, lbl_805B0478, 0, 0);
}

/* 0x801A3DBC - the action dispatcher: 2 -> `fn_801A30E8`, 3 -> `fn_801A35CC`, 4 -> `fn_801A3C54`. */
void fn_801A3DBC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_801A30E8(self);
        break;
    case 3:
        fn_801A35CC(self);
        break;
    case 4:
        fn_801A3C54(self);
        break;
    }
}

/* 0x801A3DEC - the action-id dispatcher (0x1E5) plus the block's shared tail: the `field_0x1E2 == 4`
 * gate runs `em_shake_req_set`. */
void fn_801A3DEC(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_8019F2C0(self);
        break;
    case 1:
        fn_8019F70C(self);
        break;
    case 2:
        fn_801A01C8(self);
        break;
    case 3:
        fn_801A05B4(self);
        break;
    case 6:
        fn_801A17B8(self);
        break;
    case 7:
        fn_801A2DAC(self);
        break;
    case 10:
        fn_801A2E30(self);
        break;
    case 11:
        fn_801A30D4(self);
        break;
    case 13:
        fn_801A3DBC(self);
        break;
    }
    if (self->field_0x1E2 == 4) {
        em_shake_req_set(self);
    }
}

/* 0x801A0FEC - action 6's sub-state 11: the shared `em_busy_set`/`em_busy_timer_reset` gate then a
 * two-state timer whose state 1 completes through `fn_8019F12C`. */
void fn_801A0FEC(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_80570150, 0, 16, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570150) == 1) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A12C8 - action 6's sub-state 14: arm mode 8 and the scaled `field_0x318`, then step the move offset
 * and complete through `fn_801280F4`. */
void fn_801A12C8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 60, 0);
        em_mot_speed_set(self, lbl_807985D4);
        fn_801303EC(self, lbl_807985A4);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_807985D8 * get_em_base_scale(self);
        break;
    case 1:
        em_move_offset_step(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A1384 - action 6's sub-state 15/23: state 0 arms mode 1 and seeds `timer_0x020` from the
 * (mode) selector (240/150), state 1 runs the timer down and completes through `fn_801280F4`. */
void fn_801A1384(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, lbl_807985A4);
        switch ((u8)mode) {
        case 0:
            self->timer_0x020 = 240;
            break;
        case 1:
            self->timer_0x020 = 150;
            break;
        default:
            self->timer_0x020 = 150;
            break;
        }
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A1454 - action 6's sub-state 16: state 0 arms mode 1 and the 150-frame timer, state 1 runs
 * it down and completes through `fn_8019F12C`. */
void fn_801A1454(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 150;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A14FC - action 6's sub-state 18: the busy gate, mode 9 and the `field_0x318` scale, two frame
 * windows, then `fn_8019F12C`. */
void fn_801A14FC(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend(self, 9, 30, 0, 0);
        fn_801303EC(self, lbl_807985A4);
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_807985DC * get_em_base_scale(self);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_807985E0, lbl_80798538) == 1) {
            fn_801303FC(self, lbl_807985E4 * get_em_base_scale(self));
        }
        if (em_frame_check(self, 1, lbl_807985A8, lbl_80798538) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_801303EC(self, fn_8013032C(self));
            fn_8019F12C(self);
        }
        break;
    }
}

/* 0x801A1624 - action 6's sub-state 19: state 0 arms mode 1, state 1 waits for the 64-bit
 * `em_turn_to_target` predicate and advances, state 2 completes through `fn_801280F4`. */
void fn_801A1624(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 30, 0);
        fn_801303EC(self, lbl_807985A4);
        break;
    case 1:
        if (em_turn_to_target(self, 64) == 1) {
            self->state++;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x801A16D4 - action 6's sub-state 20/21: drive the `field_0x1AC` height against the mode's blend window,
 * then `fn_8019F12C` (mode 0) or `fn_801280F4`. */
void fn_801A16D4(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        fn_8012F5C4(self, 1, 30, 0, 0);
        em_move_vec2_clr(self);
        break;
    case 1:
        if ((u8)mode == 0) {
            fn_801303FC(self, lbl_807985E8);
            if (self->field_0x1AC <= lbl_807985EC) {
                fn_801303EC(self, fn_8013032C(self));
                fn_8019F12C(self);
            }
        } else {
            fn_801303FC(self, lbl_807985F0);
            if (self->field_0x1AC >= lbl_807985A4) {
                fn_801303EC(self, fn_8013032C(self));
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* 0x801A4218 - action 7's status-bit pass: the (7,6) and (7,7) windows re-arm bits 0x40000/0x80000,
 * otherwise `fn_8011E5EC` seeds and area 1 clears 0x40000. */
void fn_801A4218(struct _ENEMY_WORK* self) {
    if (em_act_ck(self, 7, 6) == 1) {
        fn_8011E630(self, -1);
        if ((u16)em_get_mot_no(self) == 7) {
            if (em_after_frame_check(self, 2, lbl_807987B4, lbl_80798538) == 1) {
                em_status_bits_set(self, 0x40000);
            }
        }
    } else if (em_act_ck(self, 7, 7) == 1) {
        fn_8011E630(self, -1);
        em_status_bits_set(self, 0x80000);
    } else {
        fn_8011E5EC(self);
        if (self->area_no == 1) {
            fn_8011E630(self, 0x40000);
        }
    }
}

/* 0x801A42F4 - transforms `tmp` through the joint's world matrix `joint`, then copies it into
 * `out`.  The four-argument call sites are the distance probes in `fn_801A28C8`. */
void fn_801A42F4(struct _ENEMY_WORK* self, u32 joint, Vec3* out, Vec3* tmp) {
    nw4r::math::MTX34 m;
    MTX34_ctor(&m);
    get_joint_wmat_em(self, joint, &m);
    mulVecMatAddTrans(tmp, &m);
    out->x = tmp->x;
    out->y = tmp->y;
    out->z = tmp->z;
}

/* The per-motion action dispatcher.  Runs the five per-frame setup steps, then dispatches on the
 * motion number (0x73 cases) and finally walks the four per-slot joint effects. */
void fn_801A4504(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v2C;
    nw4r::math::VEC3 v20;
    nw4r::math::VEC3 v14;
    nw4r::math::VEC3 rec8;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f1_3;
    f32 temp_f1_4;
    f32 temp_f1_5;
    f32 temp_f1_6;
    f32 temp_f1_7;
    u16 temp_r3;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v2C);
    VEC3_ctor(&v20);
    VEC3_ctor(&v14);
    fn_801A3E90(self);
    fn_801A3FD8(self);
    fn_801A4218(self);
    fn_801A9748(self);
    fn_801A98F8(self);
    temp_r3 = em_get_mot_no(self);
    switch (temp_r3) {
    case 0x1:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x2:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        if ((em_after_frame_check(self, 3, lbl_807987C0, lbl_80798734) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798680);
            fn_801A42F4(self, 0x14, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                eft_em_spawn_param(self, 0x74, 0x14, &v14, lbl_80798528, 0);
                fn_8019E960(self, 0);
            }
        }
        setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
        fn_801A42F4(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 3, &v20);
        temp_f1 = self->field_0x20C;
        if ((v2C.y < temp_f1) && (v20.y > temp_f1) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_807987B8);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xF4A0);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_807987B8);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xB61);
        }
        get_joint_wpos_em(self, 5, &v2C);
        get_joint_wpos_em(self, 0x25, &v20);
        temp_f1_2 = self->field_0x20C;
        if ((v2C.y < temp_f1_2) && (v20.y > temp_f1_2) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_8079866C);
            eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF4A0);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_8079866C);
            eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xB61);
        }
        if (em_after_frame_check(self, 3, lbl_80798610, lbl_80798790) == 1U) {
            if (fn_8019E9AC(self, 1) == 0) {
                get_joint_wpos_em(self, 0x25, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn_param(self, 0x74, 0x25, &v14, lbl_80798528, 0);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                get_joint_wpos_em(self, 0x29, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn_param(self, 0x74, 0x29, &v14, lbl_80798528, 0);
                    fn_8019E960(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807987B8, lbl_80798790) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn_param(self, 0x7E, 0x2D, &v14, lbl_80798528, 0);
                fn_8019E960(self, 3);
            }
        }
        break;
    case 0x3:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x4:
        if (em_after_frame_check(self, 0, lbl_807987C4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807987C0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
            eft_em_spawn_param(self, 0x9F, 0x19, &v14, lbl_80798528, 0);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987C8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 9, &v14, lbl_80798528, 0x8000);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x5:
        if ((em_after_frame_check(self, 0, lbl_807987CC, lbl_80798538) == 1U) || (em_after_frame_check(self, 0, lbl_807986A4, lbl_80798538) == 1U)) {
            setVector3(&v14, lbl_80798538, lbl_80798600, lbl_80798538);
            eft_em_spawn(self, 0x84, 0x12, &v14, lbl_80798528);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x6:
        get_joint_wpos_em(self, 5, &v2C);
        if (v2C.y > self->field_0x20C) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                eft_em_spawn_param(self, 0x5B, 4, &v14, lbl_80798528, 0xF8E5);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                eft_em_spawn_param(self, 0x5B, 4, &v14, lbl_80798528, 0x71C);
            }
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, lbl_807987D0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x7E, 0x2D, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_807987D4, lbl_807987D8) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x25, &v20);
            temp_f1_3 = self->field_0x20C;
            if ((v2C.y > temp_f1_3) && (v20.y < temp_f1_3) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_807987DC, lbl_8079866C, lbl_80798778);
                eft_em_spawn(self, 0x71, 0x19, &v14, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 3, lbl_807987D4, lbl_80798614) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x26, &v20);
            temp_f1_4 = self->field_0x20C;
            if ((v2C.y > temp_f1_4) && (v20.y < temp_f1_4) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_807987E0, lbl_807986B0, lbl_807987B8);
                eft_em_spawn(self, 0x71, 0x19, &v14, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807987E4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, lbl_80798528, 0xEAAC);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, lbl_80798528, 0x1555);
        }
        if (em_after_frame_check(self, 0, lbl_807986E8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x13, &v14, lbl_80798528, 0xEAAC);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x13, &v14, lbl_80798528, 0x1555);
        }
        if (em_after_frame_check(self, 0, lbl_807986EC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, lbl_80798528, 0xEAAC);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 0x19, &v14, lbl_80798528, 0x1555);
        }
        if (em_after_frame_check(self, 0, lbl_807987EC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 5, &v14, lbl_80798528, 0xEAAC);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0xA3, 5, &v14, lbl_80798528, 0x1555);
        }
        if (em_after_frame_check(self, 0, lbl_807987F0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798734);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 2, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987F4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            eft_em_spawn(self, 0xA2, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 4, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798680);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987D4, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798654, lbl_80798538) == 1U) {
            em_camera_req(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 0, lbl_80798658, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807987FC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x7E, 0x2D, &v14, lbl_80798528);
        }
        break;
    case 0x8:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x9:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 3, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 3, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_80798690, lbl_80798778) == 1U) {
            if (fn_8019E9AC(self, 0) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 0);
                }
            }
            if (fn_8019E9AC(self, 1) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x24, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x24, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x24, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 2);
                }
            }
            if (fn_8019E9AC(self, 3) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 4, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 4, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 4, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 3);
                }
            }
            if (fn_8019E9AC(self, 4) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 4);
                }
            }
            if (fn_8019E9AC(self, 5) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x12, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x12, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x12, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 5);
                }
            }
            if (fn_8019E9AC(self, 6) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798560, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xF334);
                    setVector3(&v14, lbl_80798744, lbl_80798538, lbl_80798680);
                    eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xCCD);
                    fn_8019E960(self, 6);
                }
            }
            if (fn_8019E9AC(self, 7) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x8A, 0x19, &v14, lbl_80798528, 0);
                    fn_8019E960(self, 7);
                }
            }
        }
        break;
    case 0xA:
        if (em_after_frame_check(self, 0, lbl_80798800, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 9, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807986E8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798778, lbl_80798538, lbl_80798734);
            eft_em_spawn_param(self, 0x9E, 4, &v14, lbl_80798528, 0x471C);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_802BE638(self, 6, &v2C);
        }
        if ((em_after_frame_check(self, 3, lbl_807986A8, lbl_80798690) == 0) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        if ((em_after_frame_check(self, 3, lbl_80798804, lbl_80798808) == 0) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 3, lbl_8079880C, lbl_80798750) == 0) {
            setVector3(&v14, lbl_80798538, lbl_807987E8, lbl_80798538);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
        }
        if ((s32) (system_w.field_0x0c & 7) == 0) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798810, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798814, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x9D, 0xF, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807986C0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798818, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079881C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x9D, 0xF, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798820, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798560);
            eft_em_spawn(self, 0xA1, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798824, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798610);
            eft_em_spawn_param(self, 0xA1, 0x19, &v14, lbl_80798528, 0);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 2, lbl_80798544, lbl_80798538) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            temp_f1_5 = self->field_0x20C;
            if ((v2C.y < temp_f1_5) && (v20.y > temp_f1_5) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798828);
                eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF4A0);
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798828);
                eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xB61);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807986F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079882C);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798830, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798690, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798748, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079866C);
            eft_em_spawn(self, 0x7E, 0x2A, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079877C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798834, lbl_807987B8);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798838, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x45, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079883C, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798548, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_807987E8);
            eft_em_spawn(self, 0x8A, 0x25, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079878C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x74, 0x2D, &v14, lbl_80798528, 0);
        }
        break;
    case 0xE:
        if (em_after_frame_check(self, 0, lbl_80798840, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798590);
            eft_em_spawn_param(self, 0xAD, 0x14, &v14, lbl_80798528, 0);
        }
        break;
    case 0x11:
        setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
        fn_801A42F4(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 0x25, &v20);
        temp_f1_6 = self->field_0x20C;
        if ((v2C.y > temp_f1_6) && (v20.y < temp_f1_6) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_80798610, lbl_80798538, lbl_80798844);
            eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF556);
            setVector3(&v14, lbl_80798848, lbl_80798538, lbl_80798844);
            eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xAAB);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_807986C0, lbl_80798538, lbl_807987B8);
            eft_em_spawn_param(self, 0x71, 0x19, &v14, lbl_80798528, 0);
            setVector3(&v14, lbl_8079884C, lbl_80798538, lbl_807987B8);
            eft_em_spawn_param(self, 0x71, 0x19, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798820, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0x12:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798644, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_802BE638(self, 9, &v2C);
        }
        break;
    case 0x13:
        if (em_after_frame_check(self, 3, lbl_80798850, lbl_807987C0) == 1U) {
            if (fn_8019E9AC(self, 0) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798854);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798828);
                    eft_em_spawn(self, 0x7E, 0x14, &v14, lbl_80798528);
                    fn_8019E960(self, 0);
                    setVector3(&v2C, lbl_807985EC, lbl_80798538, lbl_80798538);
                    fn_802BE638(self, 5, &v2C);
                }
            }
            if (fn_8019E9AC(self, 3) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                    eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
                    fn_8019E960(self, 3);
                    setVector3(&v2C, lbl_807985EC, lbl_80798538, lbl_80798538);
                    fn_802BE638(self, 5, &v2C);
                }
            }
            if (fn_8019E9AC(self, 4) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x25, &v2C, &v14);
                if (v2C.y > self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                    eft_em_spawn(self, 0x8A, 0x25, &v14, lbl_80798528);
                    fn_8019E960(self, 4);
                }
            }
        }
        if (em_after_frame_check(self, 0, lbl_8079866C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0xA0, 0x14, &v14, lbl_80798528);
            em_camera_req(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 3, lbl_807987B8, lbl_80798590) == 1U) {
            if (fn_8019E9AC(self, 5) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, lbl_80798528, 0xF8E5);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 0x25, &v14, lbl_80798528, 0x71C);
                    fn_8019E960(self, 5);
                }
            }
            if (fn_8019E9AC(self, 6) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF8E5);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0x71C);
                    fn_8019E960(self, 6);
                }
            }
            if (fn_8019E9AC(self, 7) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0xF8E5);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0x71C);
                    fn_8019E960(self, 7);
                }
            }
            if (fn_8019E9AC(self, 8) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x13, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798610, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 0x13, &v14, lbl_80798528, 0xF8E5);
                    setVector3(&v14, lbl_80798848, lbl_80798538, lbl_80798548);
                    eft_em_spawn_param(self, 0x70, 0x13, &v14, lbl_80798528, 0x71C);
                    fn_8019E960(self, 8);
                }
            }
            if (fn_8019E9AC(self, 9) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                    eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
                    fn_8019E960(self, 9);
                }
            }
        }
        break;
    case 0x14:
    case 0x16:
        if ((s32) (system_w.field_0x0c & 3) == 0) {
            if (em_after_frame_check(self, 3, lbl_807985BC, lbl_80798664) == 1U) {
                fn_8019EC38(self, 0x12, 7, 1);
            }
            if (em_after_frame_check(self, 3, lbl_80798858, lbl_807985A4) == 1U) {
                fn_8019EC38(self, 0x12, 7, 1);
            }
            if (em_after_frame_check(self, 3, lbl_8079885C, lbl_80798860) == 1U) {
                fn_8019EC38(self, 0x12, 7, 1);
            }
        }
        if (em_after_frame_check(self, 0, lbl_80798864, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 0xF, 0, lbl_80798528);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798868, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 9, 0, lbl_80798528);
            fn_8019EC38(self, 9, 7, 0);
        }
        break;
    case 0x17:
        if (em_after_frame_check(self, 0, lbl_8079886C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798714, lbl_80798870);
            eft_em_spawn_param(self, 0x9F, 2, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798708, lbl_80798538) == 1U) {
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798588, lbl_80798538) == 1U) {
            fn_8019EC38(self, 9, 7, 0);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, lbl_80798874, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 9, 0, lbl_80798878);
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_8079887C, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 0xF, 0, lbl_80798878);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798880, lbl_80798538) == 1U) {
            fn_8019EC38(self, 0x14, 5, 3);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 0, lbl_80798614, lbl_80798538) == 1U) {
            fn_8019EC38(self, 0x12, 9, 2);
        }
        break;
    case 0x1A:
        if ((s32) (system_w.field_0x0c & 3) == 0) {
            if (em_after_frame_check(self, 3, lbl_807985A8, lbl_80798884) == 1U) {
                fn_8019EC38(self, 9, 7, 0);
            }
            if (em_after_frame_check(self, 3, lbl_80798664, lbl_80798668) == 1U) {
                fn_8019EC38(self, 0xF, 7, 0);
            }
        }
        if (em_after_frame_check(self, 0, lbl_80798888, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 9, 0, lbl_80798528);
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798644, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 0xF, 0, lbl_80798528);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        break;
    case 0x1C:
        if (em_after_frame_check(self, 0, lbl_807987DC, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0x19, 0, lbl_8079888C);
        }
        break;
    case 0x1D:
        if (em_after_frame_check(self, 0, lbl_80798890, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798680);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798678, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798734, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xF1C8);
            setVector3(&v14, lbl_80798834, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 0x19, &v14, lbl_80798528, 0xE39);
        }
        if (em_after_frame_check(self, 0, lbl_80798668, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_8079868C, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 0x13, &v14, lbl_80798528, 0xF1C8);
            setVector3(&v14, lbl_80798894, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 0x13, &v14, lbl_80798528, 0xE39);
        }
        if (em_after_frame_check(self, 0, lbl_80798898, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0xF1C8);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_807986D0);
            eft_em_spawn_param(self, 0x70, 5, &v14, lbl_80798528, 0xE39);
        }
        get_joint_wpos_em(self, 0x14, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            eft_spawn_type_at_area(self, 8);
        }
        get_joint_wpos_em(self, 4, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0x1E:
        if (em_act_ck(self, 0xD, 3) == 0) {
            if (em_after_frame_check(self, 0, lbl_807987D4, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798704, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798710, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798610, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807987B8, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_8079889C, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                eft_em_spawn(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798880, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798734);
                eft_em_spawn(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807988A0, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn_param(self, 0x74, 3, &v14, lbl_80798528, 0);
            }
            if (em_after_frame_check(self, 0, lbl_807988A4, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn_param(self, 0x74, 0x24, &v14, lbl_80798528, 0);
            }
            if (em_after_frame_check(self, 0, lbl_807988A8, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0x7E, 0x2D, &v14, lbl_80798528);
            }
        }
        break;
    case 0x1F:
        if (em_after_frame_check(self, 2, lbl_80798544, lbl_80798538) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                eft_spawn_type_at_area(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            temp_f1_7 = self->field_0x20C;
            if ((v2C.y < temp_f1_7) && (v20.y > temp_f1_7) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798828);
                eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xF4A0);
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798828);
                eft_em_spawn_param(self, 0x70, 3, &v14, lbl_80798528, 0xB61);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807986F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079882C);
            eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798830, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 6, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798690, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798748, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079866C);
            eft_em_spawn(self, 0x7E, 0x2A, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079877C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798834, lbl_807987B8);
            eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798838, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x45, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079883C, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798548, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_807987E8);
            eft_em_spawn(self, 0x8A, 0x25, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079878C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x74, 0x2D, &v14, lbl_80798528, 0);
        }
        break;
    case 0x65:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 3, lbl_807985A8, lbl_8079866C) == 1U) {
            if (fn_8019E9AC(self, 1) == 0) {
                get_joint_wpos_em(self, 9, &v2C);
                if (v2C.y > self->field_0x20C) {
                    fn_8019E960(self, 1);
                }
            } else if (fn_8019E9AC(self, 2) == 0) {
                get_joint_wpos_em(self, 9, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x9D, 9, &v14, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
            if (fn_8019E9AC(self, 3) == 0) {
                get_joint_wpos_em(self, 0xF, &v2C);
                if (v2C.y > self->field_0x20C) {
                    fn_8019E960(self, 3);
                }
            } else if (fn_8019E9AC(self, 4) == 0) {
                get_joint_wpos_em(self, 0xF, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x9D, 0xF, &v14, lbl_80798528);
                    fn_8019E960(self, 4);
                }
            }
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, lbl_807988AC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 9, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_8079867C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988B0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 9, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988B4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            eft_em_spawn_param(self, 0x9D, 0xF, &v14, lbl_80798528, 0);
        }
        if (em_after_frame_check(self, 3, lbl_807987C0, lbl_80798880) == 1U) {
            if (fn_8019E9AC(self, 0) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798834, lbl_80798538);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x8A, 0x19, &v14, lbl_80798528);
                    fn_8019E960(self, 0);
                    setVector3(&v2C, lbl_80798734, lbl_80798538, lbl_807986D4);
                    fn_802BE638(self, 5, &v2C);
                }
            }
            if (fn_8019E9AC(self, 1) == 0) {
                get_joint_wpos_em(self, 4, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x8A, 4, &v14, lbl_80798528);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                get_joint_wpos_em(self, 0x26, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x8A, 0x26, &v14, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, lbl_8079887C, lbl_80798880) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                eft_em_spawn(self, 0x7E, 0x2D, &v14, lbl_80798528);
                fn_8019E960(self, 3);
            }
        }
        break;
    case 0x67:
        if (em_act_ck(self, 0xD, 2) == 0) {
            if ((em_after_frame_check(self, 3, lbl_80798600, lbl_807987DC) == 1U) && (fn_8019E9AC(self, 2) == 0)) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 9, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x9D, 9, &v14, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
            if ((em_after_frame_check(self, 3, lbl_807987C0, lbl_8079866C) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
                setVector3(&v14, lbl_80798734, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798560);
                    eft_em_spawn(self, 0x8A, 0x14, &v14, lbl_80798528);
                    fn_8019E960(self, 0);
                    setVector3(&v2C, lbl_807987E8, lbl_80798538, lbl_80798848);
                    fn_802BE638(self, 5, &v2C);
                }
            }
            if ((em_after_frame_check(self, 3, lbl_80798614, lbl_8079887C) == 1U) && (fn_8019E9AC(self, 1) == 0)) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x2D, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    eft_em_spawn(self, 0x7E, 0x2D, &v14, lbl_80798528);
                    fn_8019E960(self, 1);
                }
            }
        }
        break;
    case 0x68:
        if (em_after_frame_check(self, 0, lbl_807988B8, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 9, 0, lbl_80798878);
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988BC, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 0xF, 0, lbl_80798878);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        break;
    case 0x69:
        if (em_after_frame_check(self, 0, lbl_80798668, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0xF, 0, lbl_80798528);
            fn_8019EC38(self, 0xF, 1, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988C0, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0x14, 0, lbl_8079888C);
            fn_8019EC38(self, 0xF, 1, 3);
        }
        if (em_after_frame_check(self, 0, lbl_807988C4, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 9, 0, lbl_807986B4);
        }
        break;
    case 0x6A:
        if (em_after_frame_check(self, 0, lbl_807987B0, lbl_80798538) == 1U) {
            em_hit_window_set_default(self, 0, 0x24);
        }
        if ((em_after_frame_check(self, 3, lbl_807985A8, lbl_807986E0) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            fn_8019EC38(self, 0x19, 2, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988C8, lbl_80798538) == 1U) {
            fn_8019EC38(self, 0x12, 9, 3);
        }
        if (em_after_frame_check(self, 0, lbl_80798804, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 4, 9, 0, lbl_80798528);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, lbl_80798668, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0xF, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807988C0, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0x14, 0, lbl_8079888C);
            fn_8019EC38(self, 0x14, 5, 3);
        }
        if (em_after_frame_check(self, 0, lbl_807988C4, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 9, 0, lbl_807986B4);
        }
        break;
    case 0x6E:
        if (em_after_frame_check(self, 0, lbl_807988CC, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 0xF, 0, lbl_80798528);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_80798810, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 7, 9, 0, lbl_80798528);
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988D0, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 4, 0x19, 0, lbl_8079888C);
            fn_8019EC38(self, 0x19, 5, 3);
        }
        break;
    case 0x6F:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798734)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807985F0, lbl_807988D4) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y > self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                eft_em_spawn(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 0);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_80798644, lbl_80798640) == 1U) && (fn_8019E9AC(self, 1) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                eft_em_spawn(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 1);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807987D4, lbl_807986F4) == 1U) && (fn_8019E9AC(self, 2) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                eft_em_spawn(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 2);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_80798740, lbl_80798748) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                eft_em_spawn(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 3);
            }
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, lbl_807986A4, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 0xF, 0, lbl_80798878);
        }
        if (em_after_frame_check(self, 0, lbl_807988DC, lbl_80798538) == 1U) {
            fn_801A437C(self, 0, 2, 9, 0, lbl_80798878);
        }
        if (em_after_frame_check(self, 0, lbl_807988E0, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 9, 0, lbl_80798780);
            fn_8019EC38(self, 9, 7, 0);
        }
        if (em_after_frame_check(self, 0, lbl_807988E4, lbl_80798538) == 1U) {
            fn_801A437C(self, 1, 1, 0xF, 0, lbl_80798780);
            fn_8019EC38(self, 0xF, 7, 0);
        }
        break;
    case 0x72:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798734)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                eft_spawn_type_at_area(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                eft_em_spawn(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    }
    if ((u8) self->action != 0xB) {
        _ENEMY_DATA_VIEW* data = (_ENEMY_DATA_VIEW*) get_enemy_data(self);
        s32 i = 0;
        do {
            if (self->handles_0x328[i] != -1) {
                u8 idx = self->field_0x338[i];
                if (idx != 0xFF) {
                    EmDataEntry* e = &data->table_0x2C[idx];
                    get_joint_wmat_em(self, e->joint, &mtx);
                    copyVec3(&v2C, (const nw4r::math::VEC3*)fn_80143174(&rec8, &e->vec, 0));
                    mulVecMatAddTrans(&v2C, &mtx);
                    em_roster_record_pos_set(self->handles_0x328[i], &v2C, self->area_no);
                }
            }
            i++;
        } while (i < 4);
    }
}

/* The model-effect refresh: on map 6 in area 1 copy the scene model's effect matrix (Y row wrapped into
 * [0,1)), then push the material colour (alpha scaled by `field_0x1D4`). */
void fn_801A9210(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*) self->field_0x13C, 6);
        if (fn_800E2994(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            u32 handle = access.GetResTexSrt(false);
            fn_8006F304(&srt, handle);
            srt.GetEffectMtx(0, &mtx);
            mtx.m[0][3] -= lbl_8079851C;
            if (mtx.m[0][3] < lbl_80798538) {
                mtx.m[0][3] += lbl_80798528;
            }
            srt.SetEffectMtx(0, &mtx);
        }
        if (((MHchar*) &self->char_0x024)->getMatColor(6, GX_COLOR0A0, &color) == 1U) {
            color.a = (u8)(lbl_807988E8 * self->field_0x1D4);
            ((MHchar*) &self->char_0x024)->setMatColor(6, GX_COLOR0A0, color, false);
        }
    } else {
        if (((MHchar*) &self->char_0x024)->getMatColor(6, GX_COLOR0A0, &color) == 1U) {
            color.a = 0;
            ((MHchar*) &self->char_0x024)->setMatColor(6, GX_COLOR0A0, color, false);
        }
    }
}

/* The per-kind predicate the action code polls: 0/1 for kinds 0/1/4/5, a 0..3 band for kinds 2/3 (an `int`
 * return; the switch narrows the argument once). */
int fn_801A9384(_ENEMY_WORK* self, u32 kind) {
    u8 narrowed = (u8) kind;
    switch (narrowed) {
    case 0:
        return (self->field_0x1E4 & 1) != 0;
    case 1:
        return self->field_0x353 != 0;
    case 2:
        if (self->field_0x1AC == lbl_807985A4) {
            return 0;
        }
        if (self->field_0x1AC > lbl_80798538) {
            return 1;
        }
        if (self->field_0x1AC > lbl_807988EC) {
            return 2;
        }
        return 3;
    case 3:
        if (self->pos.x > lbl_807988F0) {
            return 2;
        }
        return self->pos.x > lbl_807986B8;
    case 4:
        return quest_flag_200000_ck(0) == 1;
    case 5:
        return self->field_0x358 != 0;
    default:
        return 0;
    }
}

/* The 4-byte thunk into the per-joint slot release; forwards `self` unchanged. */
void fn_801A94C0(_ENEMY_WORK* self) {
    fn_8019EA04(self);
}

/* Clears the two bytes and resets the position and the three rotation words. */
void fn_801A94C4(_ENEMY_WORK* self, u8* a, u8* b) {
    em_move_mode_set(self, 0);
    *a = 0;
    *b = 0;
    setVector3(&self->pos, lbl_80798538, lbl_80798538, lbl_80798538);
    self->field_0x1BC = 0;
    self->field_0x1C0 = 0;
    self->field_0x1C4 = 0;
}

/* r3 the work record; the per-action entry: resets the joint-effect slots on the request, ticks the
 * motion, and walks the two live slots through `em_roster_slot_effect_set`. */
void fn_801A9540(struct _ENEMY_WORK* self) {
    s32* slots = self->handles_0x328;

    if (em_busy_ck(self) == 1U) {
        lb_area_change_send((u8)my_player_no(), 0);
    }
    fn_8013A9F4(self);
    if (self->area_no == 2) {
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 0, 0);
        fn_801303EC(self, lbl_80798538);
        self->field_0x7B0 = lbl_807988F4;
    }
    fn_8019EA04(self);
    em_roster_slot_effect_set(self, 3, slots, 0, 0);
    if (slots[0] != -1) {
        self->states_0x338[0] = 8;
    }
}

/* r3 the work record, r4 the part kind (a byte); the "this part is attackable" predicate: the part
 * must be 2 or 3, its damage level must not be odd, and the per-part flag must be clear. */
s32 fn_801A960C(struct _ENEMY_WORK* self, u8 kind) {
    if ((u32)(kind - 2) <= 1U && (em_parts_damage_level_get(self, kind) & 1) == 0 &&
        self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

/* r3 the work record, r4 the part kind; raises the per-part damage flag the kind maps to.  Kinds 6
 * and 7 only raise theirs at damage level 2 or above. */
void fn_801A9670(struct _ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        self->flags_0x836 |= 0x1000;
        return;
    case 1:
        self->flags_0x836 |= 0x2000;
        return;
    case 6:
        if (em_parts_damage_level_get(self, 6) >= 2U) {
            self->flags_0x836 |= 0x8000;
            return;
        }
        return;
    case 7:
        if (em_parts_damage_level_get(self, 7) >= 2U) {
            self->flags_0x836 |= 0x4000;
        }
        return;
    default:
        return;
    }
}

/* r3 the work record; asks the area table for action 13's slot, 2 in area 1 and 3 otherwise. */
void fn_801A9724(struct _ENEMY_WORK* self) {
    if (self->area_no == 1) {
        fn_80128AEC(self, 13, 2);
        return;
    }
    fn_80128AEC(self, 13, 3);
}

/* r3 the work record; the kind-3 request: clears its slot word, arms the aim motion and takes the
 * 3-byte area-table entry. */
void fn_801A9C6C(struct _ENEMY_WORK* self) {
    nw4r::g3d::ResNode sp8((void*)0);
    fn_8013A654(self, 3);
}

/* r3 the work record; releases the joint-effect slots and re-arms the two it owns. */
void fn_801A9DF4(struct _ENEMY_WORK* self) {
    s32* slot = self->handles_0x328;
    u16 i;

    fn_8019EA04(self);
    for (i = 0; i < 2; i++) {
        em_roster_slot_effect_set(self, i, slot + i, 5, 0);
    }
}

/* r3 the record, r4 the sign-extended flag; releases the record's user data and frees it when the
 * flag is positive.  Returns r3 (the record). */
s32 fn_801AA0F8(s32 record, s16 free_it) {
    if (record != 0) {
        fn_8013918C((struct _ENEMY_WORK*)record, 0);
        if (free_it > 0) {
            operator delete((void*)record);
        }
    }
    return record;
}
}
