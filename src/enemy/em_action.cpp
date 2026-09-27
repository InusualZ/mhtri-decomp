/* Monster Hunter Tri (RMHE08) - the enemy `0x803253BC-0x8032C920` band (85 functions / 30052 B),
 * reconstructed from the split target object.
 *
 * Registration (brief section 2).  The range is proposal `803253BC_fn_803253BC` from discovery's
 * attribution queue (the map stem the range was claimed under; *Naming* below replaces it).
 * Seam (campaign rule, 2026-09-26), settled from the data - referrer sets taken
 * from every current target object's relocations, not from `tudiscover`'s hints (it reports only
 * weak signals at every candidate cut here): the range references no string at all, so the
 * `__FILE__` test is inapplicable (the DOL's only `enemy`-band file name is `enemy_control.cpp`, one
 * copy, another TU's), and the pool test is decisive instead.  Of the 188 `lbl_`/`jumptable_` labels
 * this object relocates against, **184 are referenced by no other target object in the tree** - the
 * whole `.sdata2` run 0x8079AF4C..0x8079B100 (107 labels), the whole `.data` run
 * 0x805DDA50..0x805DFC00 (76, jump tables and tables), and `.rodata` `lbl_80570840` - and the
 * remaining four are the shared `.bss` globals `lbl_806BE2C8`..`lbl_806BE310`, which the `.data`
 * initializer band also points at.  The `.sdata2` tiling is disjoint and ordered either side
 * (0x8079AF00..0x8079AF48 -> the 0x80324304-0x80324CC4 run, 0x8079AF4C..0x8079B100 -> this range,
 * 0x8079B108.. -> 0x8032C920+), so the range is **data-complete**: it can own everything it
 * references, and playbook 58's sole-referencer condition holds for all of it.  Residual: the pool
 * boundary bounds the left seam only to (0x80324CC4, 0x803253BC], so `fn_803250B0` (780 B, its own
 * extab record, no pool entry of its own) could in principle still belong to this TU; no datum is
 * cited on both sides of either edge, so no edge is *falsified*.
 *
 * Module `enemy` (evidence class 3): the range drives the shared `_ENEMY_WORK`
 * record (`em_frame_check`, `em_get_mot_no`, `em_die_ck`, `em_after_frame_check`, `get_em_scale`)
 * and calls into the registered enemy units `enemy/enemy_control.cpp`,
 * `enemy/fn_801251D0.cpp` and `enemy/fn_8012EC74.cpp` (fn_8012F5B8 x75, fn_80130478 x62,
 * fn_801251D0 x60, fn_801303EC x51, fn_8012F93C x44, fn_801280F4 x42).  Lib `enemy` (cflags_main),
 * the group its link neighbours use.
 *
 * Naming.  The map has no real name for any of the range's 85 addresses: `dumpmap.py join --kind
 * rename` reports **0** candidates over 65679 map symbols, every `lookup` answers `zz_<addr>_`, and
 * no `__FILE__` string covers the band.  The 14 symbols this file defines - 13 bodies plus the still
 * bodyless `em_act_move_step` - and the file name `em_action` are therefore **guesses derived from
 * the bodies** and the module's `em_*` sibling scheme (`em_frame_check`, `em_die_ck`,
 * `em_get_mot_no`, `em_after_frame_check`): `em_act_slots_clr` clears the four action halfwords,
 * `em_act_entry_start` starts the entry action for a mode and installs its state record once,
 * `em_act_frame_ck` is that action's per-step hook, `em_act_dispatch` switches on the action
 * sub-state, `em_act_hold`/`em_act_follow`/`em_act_mot21` are the three motions it dispatches,
 * `em_act_arm_m06s07` arms motion 6 sub-state 7, `em_act_aim` re-latches the aim angle and clamps it
 * into one of the two facing windows, `em_rot_reset` zeroes the three rotation angles,
 * `em_tut_cam_ck` is the tutorial-map/camera-distance gate, and `em_act_rec_init`/`em_act_noop` are
 * the 0xC-byte action record's constructor and its empty step.  Refine them when better evidence
 * turns up; only the bodies support them.
 *
 * Naming note: the file's own 14 symbols are named above; what the escape still covers is
 * precisely the names this file *references* in other units - 25 callee symbols in **42**
 * occurrences (fn_80130478 x5, fn_80126324 x4, fn_8012F93C x3, fn_801303EC x2, fn_8012F5C4 x2,
 * fn_8013072C x2, fn_80133BC0 x2, enemy_data_find x2, enemy_data_grp x2, ...; 21 of the 25 answer `zz_` in
 * the dump too).  Renaming those is a cross-unit rename batch in ~10 owner units, not this lane's
 * change, so the escape stays until that batch runs.
 *
 * Sections this unit owns: .text 0x803253BC-0x8032C920, extab 0x8001622C-0x80016464,
 * extabindex 0x800351A8-0x800354FC (both derived from the range's `@etb_` labels) and the one
 * `.ctors` word 0x8056F39C -> fn_8032C65C.
 *
 * The record is the shared `_ENEMY_WORK` (`include/enemy/ENEMY_WORK.h`, rule 1).
 *
 * Data state (brief section 2).  `python tools/units/datagap.py --unit em_action` reports **no
 * `ours-extra` row** and no data-section row in either direction: this object emits no
 * `.data`/`.sdata2`/`.sdata`/`.rodata` and neither does the target object, because the entries the
 * range loads live in data bands no `splits.txt` block claims.  They are declared, never defined
 * (`include/unsplit/enemy.h`, playbook 29) - a definition would emit a second copy.  The ranges
 * *are* claimable, but only once the bodies that use them are written: claiming them now, at 13 of
 * 85 functions, gives the target object bytes this one does not emit (`target-extra`), which playbook
 * 29 calls strictly worse than leaving the claim out.  The one claimed section this object does not
 * emit is the `.ctors` word 0x8056F39C -> fn_8032C65C (`target-extra` 4 B): it needs the static
 * object whose constructor `fn_8032C65C` is, and that body is unwritten.
 *
 * Reconstructed: the head of the band, 0x803253BC-0x80325E34 (13 of the 85 functions, 1744 of the
 * target's 30052 B; our object's `.text` is 1760 B); every other symbol in the range is still a
 * bodyless `fn_` in the map.  Per-symbol
 * numbers come from `python tools/units/recompile.py enemy/em_action.cpp --measure <symbol>`;
 * the drafts the bodies were written from are regenerated with `python tools/units/m2cinput.py
 * build/RMHE08/obj/enemy/em_action.o -o build/tmp/x.s` then `python tools/m2c/m2c.py -t
 * ppc-mwcc-c build/tmp/x.s`.
 *
 * Residuals of what is written:
 *   * em_tut_cam_ck 81.0 - the by-value `get_camera_pos()` result is copied through a temporary where
 *     retail hands the temporary straight to `copyVec3`, i.e. the owner's declaration is a
 *     `const VEC3&` (or the original took a temporary's address); the `const VEC3*` spelling this
 *     unit must use costs the 3-word copy.
 *   * em_act_hold 87.5, em_act_frame_ck 93.7, em_act_aim 93.9 - register colouring plus one duplicated
 *     branch tail (284-304 B against 312/144/144 B).
 *   * em_act_mot21 96.8 - retail keeps an `lfs f1, lbl_8079AF54` before the `fn_801305C4` call whose
 *     owner header declares `(self)` only; adding the second parameter there would change that
 *     header's other callers.
 *   * em_act_rec_init 99.3, em_rot_reset 97.6, em_act_entry_start 97.9, em_act_follow 97.3 - one
 *     instruction or one register colour away.
 *     one register colour away.
 *   * em_act_slots_clr, em_act_noop, em_act_arm_m06s07 and em_act_dispatch are byte-identical.
 *
 * Merged with `main` (2026-09-26).  The `_ENEMY_WORK` +0x194 field split had landed without its
 * trailing pad, which shrank the struct to 0xB0C and moved every member above +0x194 down by 0x0C -
 * invisible to this unit's own rows but worth 446 lower rows across the tree.  The two views are one
 * `union` now; an MWCC probe (array extents read back from the object's symtab) re-asserts
 * `sizeof(_ENEMY_WORK) == 0xB18`, `field_0x1AC` at 0x1AC and `aim` at 0x1B0.  Four of the thirteen
 * rows above gained from the repair.
 */
#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/enemy_control.h"
#include "fn_8004CAD8.h"
#include "ef.h"
#include "g3d/g3d_resanmchr.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"
#include "stage/stg_w.h"
#include "camera/camera.h"
#include "sys_mem.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */

extern "C" {

/* The unit's own entry points: the original file's definition order is not the address order, so the
 * callers below need these prototypes. */
void* em_act_rec_init(void* self);
void em_act_move_step(_ENEMY_WORK* self, u8 mode, f32 step);
void em_act_hold(_ENEMY_WORK* self);
void em_act_follow(_ENEMY_WORK* self);
void em_act_mot21(_ENEMY_WORK* self);

/* Zeroes a 3-vector, then reports whether the current map/area is the tutorial's and the camera is
 * past the given height. */
s32 em_tut_cam_ck(void) {
    nw4r::math::VEC3 zeroed;
    nw4r::math::VEC3 cam;
    VEC3_ctor(&zeroed);
    if (get_now_mapno() != 0x16) {
        return 1;
    }
    if (get_now_areano() == 2) {
        cam = get_camera_pos();
        copyVec3(&zeroed, &cam);
        if (zeroed.x >= lbl_8079AF4C) {
            return 0;
        }
    }
    return 1;
}

/* Resets the record's three rotation angles; re-arms the two "rotation handed over" angles when the
 * area is 7/8 and the sub-state matches. */
void em_rot_reset(_ENEMY_WORK* self) {
    if (fn_802B0668(self->field_0x1E0) == 2) {
        switch (self->area_no) {
        case 7:
            if (self->field_0x9F6 == 8) {
                fn_80126324(self, 1, 2, lbl_8079AF50);
                self->field_0x1C0 = 0x8000;
                return;
            }
            fn_80126324(self, 1, 2, lbl_8079AF50);
            return;
        case 8:
            if (self->field_0x9F6 == 7) {
                fn_80126324(self, 1, 2, lbl_8079AF50);
                self->field_0x1C0 = 0xF000;
                return;
            }
            fn_80126324(self, 1, 2, lbl_8079AF50);
            return;
        default:
            self->pos.z = lbl_8079AF54;
            self->pos.y = lbl_8079AF54;
            self->pos.x = lbl_8079AF54;
            self->field_0x1C4 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1BC = 0;
            return;
        }
    } else {
        self->pos.z = lbl_8079AF54;
        self->pos.y = lbl_8079AF54;
        self->pos.x = lbl_8079AF54;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
    }
}

/* Clears the band's four halfword action slots. */
void em_act_slots_clr(_ENEMY_WORK* self) {    self->band_0x328.arm_0x32A = 0;
    self->band_0x328.arm_0x32C = 0;
    self->band_0x328.count_0x334 = 0;
    self->band_0x328.limit_0x336 = 0;
}

/* Starts the band's entry action for the given mode and installs its state record once. */
void em_act_entry_start(_ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        fn_80130478(self, 4);
        fn_80128A8C(self, 6, 0x20);
        fn_80133BC0(self);
        break;
    case 3:
        fn_80130478(self, 4);
        fn_80128A8C(self, 6, 0x16);
        em_rot_reset(self);
        fn_80133BC0(self);
        break;
    }
    self->field_0x835 = 1;
    if (fn_801391E8(self) == 0) {
        void* record = operator new(0xC);
        if (record != NULL) {
            em_act_rec_init(record);
        }
        fn_801390FC(self, record);
    }
}

/* Initialises the 0xC-byte action record's shared vtable word. */
void* em_act_rec_init(void* self) {
    fn_80147E2C(self);
    *(void**)self = (void*)&lbl_805DFC00;
    return self;
}

void em_act_noop(void) {
}

/* Per-step hook of the band's entry action: arms the transition flag for the motion and state the
 * caller reports, and asks for the "leave" transition when the enemy data entry disagrees. */
void em_act_frame_ck(_ENEMY_WORK* self, u8 motion, u8 state) {
    u8 kind = self->field_0x00A;

    switch (kind) {
    case 0:
        self->band_0x328.arm_0x32A = 0;
        self->band_0x328.arm_0x32C = 0;
        switch (motion) {
        case 6:
            if ((u32)(state - 0x14) <= 1U) {
                self->band_0x328.arm_0x32C = 1;
            }
            return;
        case 11: {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL && entry->field_0x08 == 1) {
                fn_80346268(2, self->area_no);
            }
            return;
        }
        case 7:
            if (state == 5) {
                self->band_0x328.arm_0x32A = 1;
            }
            return;
        }
        return;
    case 1:
        if (self->field_0x43C != 0) {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL) {
                if (entry->field_0x0B != 1) {
                    fn_8013072C(self, 0, 0);
                }
            } else {
                fn_8013072C(self, 0, 0);
            }
        }
        return;
    }
}

/* Arms the band's fifth action: motion 6 sub-state 7. */
void em_act_arm_m06s07(_ENEMY_WORK* self) {
    fn_80130478(self, 4);
    fn_80128AAC(self, 6, 7);
    fn_80133BB4(self);
}

/* The band's 50-frame hold: steps its own state byte and hands the follow-up motion to the move
 * engine. */
void em_act_hold(_ENEMY_WORK* self) {
    u8 state = self->state;

    fn_801303EC(self, lbl_8079AF5C);
    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        fn_80130478(self, 0);
        fn_8012F5C4(self, 1, 6, 0, 3);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        return;
    }
}

/* The band's follow-up motion: resets the pitch to zero when the move engine reports done. */
void em_act_follow(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        fn_80130478(self, 4);
        fn_8012F5C4(self, 1, 6, 0, 3);
        fn_801303EC(self, lbl_8079AF54);
        return;
    case 1:
        em_act_move_step(NULL, 0, lbl_8079AF58);
        if (fn_8012F93C(self) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

/* The band's 21-step motion: hands it to the move engine, then reports done. */
void em_act_mot21(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x15, 2, 0);
        return;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127FE4(self);
        }
        return;
    }
}

/* Per-step dispatch of the band's four motions on the action sub-state. */
void em_act_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_act_hold(self);
        return;
    case 1:
        em_act_hold(self);
        return;
    case 2:
        em_act_hold(self);
        return;
    case 3:
        em_act_mot21(self);
        return;
    case 7:
        em_act_follow(self);
        return;
    }
}

/* Re-aims the record at its target: the angle to the target position is latched, and clamped into
 * one of the two facing windows when it falls outside them. */
void em_act_aim(_ENEMY_WORK* self) {
    if (self->field_0x314 == lbl_8079AF60) {
        s32 angle = fn_80050C40(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle + 0xFFFF5FFF) <= 0x3FFEU) {
            self->field_0x1BC = 0xE000;
        }
    } else {
        s32 angle = fn_80050C40(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle - 0x2001) <= 0x1FFEU) {
            self->field_0x1BC = 0x2000;
        }
    }
}

} /* extern "C" */