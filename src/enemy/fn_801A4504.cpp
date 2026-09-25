/* enemy/fn_801A4504.cpp - the enemy per-motion action dispatcher, .text 0x801A4504..0x801A9540
 * (5 functions / 0x503C bytes: `fn_801A4504` 0x4D0C, `fn_801A9210` 0x174, `fn_801A9384` 0x13C,
 * `fn_801A94C0` 0x4, `fn_801A94C4` 0x7C), plus the extab run 0x8000F304..0x8000F324 and the
 * extabindex run 0x8002AB70..0x8002ABA0.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` - every address answers `zz_XXXXXXXX_`/`FUN_XXXXXXXX`,
 * i.e. the runtime dump has no name, and no `__FILE__` string is referenced anywhere in the range:
 * the only pooled data it reaches is `.sdata2` float constants).
 *
 * Registered once at its final home (docs/plan.md 12).  Module `enemy`: `self` is dispatched by
 * `em_get_mot_no(_ENEMY_WORK*)` and every callee is `em_*`/`get_joint_*_em`/`get_enemy_data`
 * (`_ENEMY_WORK`), the two bracketing registered units (below: `enemy/fn_80191598.cpp`, above across
 * the unclaimed run: the `enemy` band) are the enemy module, and `include/enemy/ENEMY_WORK.h` is the
 * band.  Name: evidence class 4 - no `__FILE__` string, no real runtime name, so the map's own
 * `fn_` stem is kept and the file takes its first symbol (the sibling units use the same scheme).
 * C++ because the range reaches mangled callees (`get_joint_wpos_em`, `em_after_frame_check`,
 * `get_joint_wmat_em`, `get_enemy_data`) through their real signatures (rule 9); every plain
 * `fn_XXXXXXXX` definition here is `extern "C"` so its map name is emitted.
 *
 * `fn_801A4504` is one 0x4D0C-byte switch on `em_get_mot_no(self)` (a 0x73-entry table at
 * 0x805B04D0): the per-motion action bodies, followed by a per-slot joint-effect pass over
 * `_ENEMY_WORK::handles_0x328`/`field_0x338`.  `fn_801A9210` updates the enemy model's effect matrix
 * and material colour, `fn_801A9384` answers a per-kind predicate, `fn_801A94C4` resets the position
 * and two rotation words, and `fn_801A94C0` is the 4-byte tail thunk into `fn_8019EA04`.
 *
 * Script-discovered correction (docs/plan.md 6.5, settled from the callee's body):
 * `MHchar::getMatColor` returns the material status in r3 (its body writes 0 on the no-resource and
 * no-material paths and the inner reader's value otherwise), so `include/sound/mhchar.h` declares it
 * `u32`, not `void`, and `fn_801A9210` compares the result against 1.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"
#include "nw4r/g3d/g3d_resmat.h"
#include "unsplit/g3d.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"

/* The pooled `.sdata2` floats this range reads (each is a bare marker symbol in the map; the values
 * drive the comparisons/floats below).  Declared, never defined here: the pool belongs to the data
 * pass. */
extern const f32 lbl_8079851C;
extern const f32 lbl_80798528;
extern const f32 lbl_80798538;
extern const f32 lbl_80798544;
extern const f32 lbl_80798548;
extern const f32 lbl_80798560;
extern const f32 lbl_80798564;
extern const f32 lbl_80798588;
extern const f32 lbl_8079858C;
extern const f32 lbl_80798590;
extern const f32 lbl_807985A4;
extern const f32 lbl_807985A8;
extern const f32 lbl_807985BC;
extern const f32 lbl_807985EC;
extern const f32 lbl_807985F0;
extern const f32 lbl_80798600;
extern const f32 lbl_80798610;
extern const f32 lbl_80798614;
extern const f32 lbl_80798640;
extern const f32 lbl_80798644;
extern const f32 lbl_80798654;
extern const f32 lbl_80798658;
extern const f32 lbl_80798664;
extern const f32 lbl_80798668;
extern const f32 lbl_8079866C;
extern const f32 lbl_80798678;
extern const f32 lbl_8079867C;
extern const f32 lbl_80798680;
extern const f32 lbl_8079868C;
extern const f32 lbl_80798690;
extern const f32 lbl_807986A4;
extern const f32 lbl_807986A8;
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

/* 0x80304508 - unowned (the bracketing registered units name different modules, `ai`/`ef`) and its
 * two existing consumers disagree on the parameter spellings (`s32`/`nw4r::math::VEC3*` in
 * `ef/fn_801173AC.cpp`, `u32`/`void*` in `enemy/fn_80147CE0.cpp`), so it cannot share one band
 * header declaration (docs/plan.md 6.5 rule 2's named gap).  This unit keeps the `void*` form. */
extern "C" void fn_80304508(struct _ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);

/* One 0x10-byte record of the per-enemy effect table `get_enemy_data(self)+0x2C` points at: a joint
 * id and the joint-local offset the effect is placed at (the target reads +0x00 as `lwz` and copies
 * a `VEC3` from +0x04).  size: 0x10 */
struct EmDataEntry {
    /* +0x00 */ s32 joint;
    /* +0x04 */ nw4r::math::VEC3 vec;
};

/* The `get_enemy_data` record's unnamed +0x2C field (the effect-table pointer); the canonical
 * `EnemyData` leaves +0x28..+0x30 as padding, so this unit carries the one offset it walks, exactly
 * as `ef/eft019.cpp`/`ef/fn_80105314.cpp` do for the offsets they reach.  size: 0x30 */
struct _ENEMY_DATA_VIEW {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ EmDataEntry* table_0x2C;
};

extern "C" {


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

    fn_8005050C(&mtx);
    fn_80043EA8(&v2C);
    fn_80043EA8(&v20);
    fn_80043EA8(&v14);
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
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x2:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
        }
        if ((em_after_frame_check(self, 3, lbl_807987C0, lbl_80798734) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798680);
            fn_801A42F4(self, 0x14, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                fn_80304510(self, 0x74, 0x14, &v14, 0, lbl_80798528);
                fn_8019E960(self, 0);
            }
        }
        setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
        fn_801A42F4(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 3, &v20);
        temp_f1 = self->field_0x20C;
        if ((v2C.y < temp_f1) && (v20.y > temp_f1) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_807987B8);
            fn_80304510(self, 0x70, 0x19, &v14, 0xF4A0, lbl_80798528);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_807987B8);
            fn_80304510(self, 0x70, 0x19, &v14, 0xB61, lbl_80798528);
        }
        get_joint_wpos_em(self, 5, &v2C);
        get_joint_wpos_em(self, 0x25, &v20);
        temp_f1_2 = self->field_0x20C;
        if ((v2C.y < temp_f1_2) && (v20.y > temp_f1_2) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_8079866C);
            fn_80304510(self, 0x70, 3, &v14, 0xF4A0, lbl_80798528);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_8079866C);
            fn_80304510(self, 0x70, 3, &v14, 0xB61, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_80798610, lbl_80798790) == 1U) {
            if (fn_8019E9AC(self, 1) == 0) {
                get_joint_wpos_em(self, 0x25, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    fn_80304510(self, 0x74, 0x25, &v14, 0, lbl_80798528);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                get_joint_wpos_em(self, 0x29, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    fn_80304510(self, 0x74, 0x29, &v14, 0, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807987B8, lbl_80798790) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304510(self, 0x7E, 0x2D, &v14, 0, lbl_80798528);
                fn_8019E960(self, 3);
            }
        }
        break;
    case 0x3:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x4:
        if (em_after_frame_check(self, 0, lbl_807987C4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 0xF, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807987C0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
            fn_80304510(self, 0x9F, 0x19, &v14, 0, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987C8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 9, &v14, 0x8000, lbl_80798528);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x5:
        if ((em_after_frame_check(self, 0, lbl_807987CC, lbl_80798538) == 1U) || (em_after_frame_check(self, 0, lbl_807986A4, lbl_80798538) == 1U)) {
            setVector3(&v14, lbl_80798538, lbl_80798600, lbl_80798538);
            fn_80304508(self, 0x84, 0x12, &v14, lbl_80798528);
        }
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x6:
        get_joint_wpos_em(self, 5, &v2C);
        if (v2C.y > self->field_0x20C) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                fn_80304510(self, 0x5B, 4, &v14, 0xF8E5, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                fn_80304510(self, 0x5B, 4, &v14, 0x71C, lbl_80798528);
            }
        }
        break;
    case 0x7:
        if (em_after_frame_check(self, 0, lbl_807987D0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x7E, 0x2D, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_807987D4, lbl_807987D8) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x25, &v20);
            temp_f1_3 = self->field_0x20C;
            if ((v2C.y > temp_f1_3) && (v20.y < temp_f1_3) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_807987DC, lbl_8079866C, lbl_80798778);
                fn_80304508(self, 0x71, 0x19, &v14, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 3, lbl_807987D4, lbl_80798614) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            get_joint_wpos_em(self, 0x26, &v20);
            temp_f1_4 = self->field_0x20C;
            if ((v2C.y > temp_f1_4) && (v20.y < temp_f1_4) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_807987E0, lbl_807986B0, lbl_807987B8);
                fn_80304508(self, 0x71, 0x19, &v14, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807987E4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x19, &v14, 0xEAAC, lbl_80798528);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x19, &v14, 0x1555, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807986E8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x13, &v14, 0xEAAC, lbl_80798528);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x13, &v14, 0x1555, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807986EC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x19, &v14, 0xEAAC, lbl_80798528);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 0x19, &v14, 0x1555, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807987EC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 5, &v14, 0xEAAC, lbl_80798528);
            setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0xA3, 5, &v14, 0x1555, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807987F0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798734);
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079858C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 2, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987F4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            fn_80304508(self, 0xA2, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 4, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798680);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_807987D4, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_807985EC);
            fn_802BE638(self, 9, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798654, lbl_80798538) == 1U) {
            fn_80136B50(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 0, lbl_80798658, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807987FC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x7E, 0x2D, &v14, lbl_80798528);
        }
        break;
    case 0x8:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c % 9) == 0) {
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        break;
    case 0x9:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 3, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 3, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_80798690, lbl_80798778) == 1U) {
            if (fn_8019E9AC(self, 0) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x25, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x25, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 0);
                }
            }
            if (fn_8019E9AC(self, 1) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x24, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x24, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x24, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 3, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 3, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
            if (fn_8019E9AC(self, 3) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 4, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 4, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 4, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 3);
                }
            }
            if (fn_8019E9AC(self, 4) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 5, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 5, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 4);
                }
            }
            if (fn_8019E9AC(self, 5) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x12, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x12, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x12, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 5);
                }
            }
            if (fn_8019E9AC(self, 6) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798560, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x19, &v14, 0xF334, lbl_80798528);
                    setVector3(&v14, lbl_80798744, lbl_80798538, lbl_80798680);
                    fn_80304510(self, 0x70, 0x19, &v14, 0xCCD, lbl_80798528);
                    fn_8019E960(self, 6);
                }
            }
            if (fn_8019E9AC(self, 7) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807987B8);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x8A, 0x19, &v14, 0, lbl_80798528);
                    fn_8019E960(self, 7);
                }
            }
        }
        break;
    case 0xA:
        if (em_after_frame_check(self, 0, lbl_80798800, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 9, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807986E8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798778, lbl_80798538, lbl_80798734);
            fn_80304510(self, 0x9E, 4, &v14, 0x471C, lbl_80798528);
            setVector3(&v2C, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_802BE638(self, 6, &v2C);
        }
        if ((em_after_frame_check(self, 3, lbl_807986A8, lbl_80798690) == 0) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        if ((em_after_frame_check(self, 3, lbl_80798804, lbl_80798808) == 0) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 3, lbl_8079880C, lbl_80798750) == 0) {
            setVector3(&v14, lbl_80798538, lbl_807987E8, lbl_80798538);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                fn_80306A98(self, 8);
            }
        }
        if ((s32) (system_w.field_0x0c & 7) == 0) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798810, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798814, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x9D, 0xF, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807986C0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798818, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x9D, 9, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079881C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x9D, 0xF, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798820, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798560);
            fn_80304508(self, 0xA1, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798824, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798610);
            fn_80304510(self, 0xA1, 0x19, &v14, 0, lbl_80798528);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 2, lbl_80798544, lbl_80798538) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                fn_80306A98(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            temp_f1_5 = self->field_0x20C;
            if ((v2C.y < temp_f1_5) && (v20.y > temp_f1_5) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798828);
                fn_80304510(self, 0x70, 3, &v14, 0xF4A0, lbl_80798528);
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798828);
                fn_80304510(self, 0x70, 3, &v14, 0xB61, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807986F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079882C);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798830, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798690, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798748, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079866C);
            fn_80304508(self, 0x7E, 0x2A, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079877C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798834, lbl_807987B8);
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
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
            fn_80304508(self, 0x8A, 0x25, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079878C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x74, 0x2D, &v14, 0, lbl_80798528);
        }
        break;
    case 0xE:
        if (em_after_frame_check(self, 0, lbl_80798840, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798590);
            fn_80304510(self, 0xAD, 0x14, &v14, 0, lbl_80798528);
        }
        break;
    case 0x11:
        setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
        fn_801A42F4(self, 0x19, &v2C, &v14);
        get_joint_wpos_em(self, 0x25, &v20);
        temp_f1_6 = self->field_0x20C;
        if ((v2C.y > temp_f1_6) && (v20.y < temp_f1_6) && ((s32) (system_w.field_0x0c % 10) == 0)) {
            setVector3(&v14, lbl_80798610, lbl_80798538, lbl_80798844);
            fn_80304510(self, 0x70, 3, &v14, 0xF556, lbl_80798528);
            setVector3(&v14, lbl_80798848, lbl_80798538, lbl_80798844);
            fn_80304510(self, 0x70, 3, &v14, 0xAAB, lbl_80798528);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y < self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_807986C0, lbl_80798538, lbl_807987B8);
            fn_80304510(self, 0x71, 0x19, &v14, 0, lbl_80798528);
            setVector3(&v14, lbl_8079884C, lbl_80798538, lbl_807987B8);
            fn_80304510(self, 0x71, 0x19, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798820, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0x12:
        get_joint_wpos_em(self, 0x12, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        get_joint_wpos_em(self, 3, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x7E, 0x14, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x8A, 0x25, &v14, lbl_80798528);
                    fn_8019E960(self, 4);
                }
            }
        }
        if (em_after_frame_check(self, 0, lbl_8079866C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0xA0, 0x14, &v14, lbl_80798528);
            fn_80136B50(self, -1, 0xC6);
        }
        if (em_after_frame_check(self, 3, lbl_807987B8, lbl_80798590) == 1U) {
            if (fn_8019E9AC(self, 5) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x25, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 0x25, &v14, 0xF8E5, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 0x25, &v14, 0x71C, lbl_80798528);
                    fn_8019E960(self, 5);
                }
            }
            if (fn_8019E9AC(self, 6) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 3, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 3, &v14, 0xF8E5, lbl_80798528);
                    setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 3, &v14, 0x71C, lbl_80798528);
                    fn_8019E960(self, 6);
                }
            }
            if (fn_8019E9AC(self, 7) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 5, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798548, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 5, &v14, 0xF8E5, lbl_80798528);
                    setVector3(&v14, lbl_807987E8, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 5, &v14, 0x71C, lbl_80798528);
                    fn_8019E960(self, 7);
                }
            }
            if (fn_8019E9AC(self, 8) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x13, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798610, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 0x13, &v14, 0xF8E5, lbl_80798528);
                    setVector3(&v14, lbl_80798848, lbl_80798538, lbl_80798548);
                    fn_80304510(self, 0x70, 0x13, &v14, 0x71C, lbl_80798528);
                    fn_8019E960(self, 8);
                }
            }
            if (fn_8019E9AC(self, 9) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
                    fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
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
            fn_80304510(self, 0x9F, 2, &v14, 0, lbl_80798528);
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
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798678, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798734, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 0x19, &v14, 0xF1C8, lbl_80798528);
            setVector3(&v14, lbl_80798834, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 0x19, &v14, 0xE39, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798668, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_8079868C, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 0x13, &v14, 0xF1C8, lbl_80798528);
            setVector3(&v14, lbl_80798894, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 0x13, &v14, 0xE39, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798898, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 5, &v14, 0xF1C8, lbl_80798528);
            setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_807986D0);
            fn_80304510(self, 0x70, 5, &v14, 0xE39, lbl_80798528);
        }
        get_joint_wpos_em(self, 0x14, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
            fn_80306A98(self, 8);
        }
        get_joint_wpos_em(self, 4, &v2C);
        if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
            fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
        }
        break;
    case 0x1E:
        if (em_act_ck(self, 0xD, 3) == 0) {
            if (em_after_frame_check(self, 0, lbl_807987D4, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798704, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798710, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798610, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 9, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807987B8, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xA3, 0xF, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_8079889C, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                fn_80304508(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798880, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798734);
                fn_80304508(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_80798564, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0xB8, 0x14, &v14, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807988A0, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304510(self, 0x74, 3, &v14, 0, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807988A4, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304510(self, 0x74, 0x24, &v14, 0, lbl_80798528);
            }
            if (em_after_frame_check(self, 0, lbl_807988A8, lbl_80798538) == 1U) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0x7E, 0x2D, &v14, lbl_80798528);
            }
        }
        break;
    case 0x1F:
        if (em_after_frame_check(self, 2, lbl_80798544, lbl_80798538) == 1U) {
            get_joint_wpos_em(self, 0x12, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c % 9) == 0)) {
                fn_80306A98(self, 8);
            }
            get_joint_wpos_em(self, 3, &v2C);
            if ((v2C.y > self->field_0x20C) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            get_joint_wpos_em(self, 3, &v2C);
            get_joint_wpos_em(self, 0x13, &v20);
            temp_f1_7 = self->field_0x20C;
            if ((v2C.y < temp_f1_7) && (v20.y > temp_f1_7) && ((s32) (system_w.field_0x0c % 10) == 0)) {
                setVector3(&v14, lbl_807987B8, lbl_80798538, lbl_80798828);
                fn_80304510(self, 0x70, 3, &v14, 0xF4A0, lbl_80798528);
                setVector3(&v14, lbl_807986B0, lbl_80798538, lbl_80798828);
                fn_80304510(self, 0x70, 3, &v14, 0xB61, lbl_80798528);
            }
        }
        if (em_after_frame_check(self, 0, lbl_807986F8, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_807986D0);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798560, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079882C);
            fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798830, lbl_80798538) == 1U) {
            setVector3(&v2C, lbl_80798714, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 6, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_80798690, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_80798748, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_8079866C);
            fn_80304508(self, 0x7E, 0x2A, &v14, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079877C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798834, lbl_807987B8);
            fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
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
            fn_80304508(self, 0x8A, 0x25, &v14, lbl_80798528);
            setVector3(&v2C, lbl_80798600, lbl_80798600, lbl_80798538);
            fn_802BE638(self, 0x46, &v2C);
        }
        if (em_after_frame_check(self, 0, lbl_8079878C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x74, 0x2D, &v14, 0, lbl_80798528);
        }
        break;
    case 0x65:
        get_joint_wpos_em(self, 2, &v2C);
        if (v2C.y > (self->field_0x20C - lbl_80798548)) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x9D, 9, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x9D, 0xF, &v14, lbl_80798528);
                    fn_8019E960(self, 4);
                }
            }
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, lbl_807988AC, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 9, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_8079867C, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 0xF, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807988B0, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 9, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 0, lbl_807988B4, lbl_80798538) == 1U) {
            setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
            fn_80304510(self, 0x9D, 0xF, &v14, 0, lbl_80798528);
        }
        if (em_after_frame_check(self, 3, lbl_807987C0, lbl_80798880) == 1U) {
            if (fn_8019E9AC(self, 0) == 0) {
                setVector3(&v14, lbl_80798538, lbl_80798834, lbl_80798538);
                fn_801A42F4(self, 0x19, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    fn_80304508(self, 0x8A, 0x19, &v14, lbl_80798528);
                    fn_8019E960(self, 0);
                    setVector3(&v2C, lbl_80798734, lbl_80798538, lbl_807986D4);
                    fn_802BE638(self, 5, &v2C);
                }
            }
            if (fn_8019E9AC(self, 1) == 0) {
                get_joint_wpos_em(self, 4, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    fn_80304508(self, 0x8A, 4, &v14, lbl_80798528);
                    fn_8019E960(self, 1);
                }
            }
            if (fn_8019E9AC(self, 2) == 0) {
                get_joint_wpos_em(self, 0x26, &v2C);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                    fn_80304508(self, 0x8A, 0x26, &v14, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
        }
        if ((em_after_frame_check(self, 3, lbl_8079887C, lbl_80798880) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            get_joint_wpos_em(self, 0x2D, &v2C);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798538);
                fn_80304508(self, 0x7E, 0x2D, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x9D, 9, &v14, lbl_80798528);
                    fn_8019E960(self, 2);
                }
            }
            if ((em_after_frame_check(self, 3, lbl_807987C0, lbl_8079866C) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
                setVector3(&v14, lbl_80798734, lbl_80798538, lbl_80798538);
                fn_801A42F4(self, 0x14, &v2C, &v14);
                if (v2C.y < self->field_0x20C) {
                    setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798560);
                    fn_80304508(self, 0x8A, 0x14, &v14, lbl_80798528);
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
                    fn_80304508(self, 0x7E, 0x2D, &v14, lbl_80798528);
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
            fn_80129668(self, 0, 0x24);
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
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807985F0, lbl_807988D4) == 1U) && (fn_8019E9AC(self, 0) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y > self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                fn_80304508(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 0);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_80798644, lbl_80798640) == 1U) && (fn_8019E9AC(self, 1) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                fn_80304508(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 1);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_807987D4, lbl_807986F4) == 1U) && (fn_8019E9AC(self, 2) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                fn_80304508(self, 0xA1, 0x19, &v14, lbl_80798528);
                fn_8019E960(self, 2);
            }
        }
        if ((em_after_frame_check(self, 3, lbl_80798740, lbl_80798748) == 1U) && (fn_8019E9AC(self, 3) == 0)) {
            setVector3(&v14, lbl_80798538, lbl_807988D8, lbl_807987B8);
            fn_801A42F4(self, 0x19, &v2C, &v14);
            if (v2C.y < self->field_0x20C) {
                setVector3(&v14, lbl_80798538, lbl_80798538, lbl_80798548);
                fn_80304508(self, 0xA1, 0x19, &v14, lbl_80798528);
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
                fn_80306A98(self, 8);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_80798590, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
            }
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&v14, lbl_807987BC, lbl_80798538, lbl_807987B8);
                fn_80304508(self, 0x5B, 4, &v14, lbl_80798528);
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
                    fn_80041E40(&v2C, fn_80143174(&rec8, &e->vec, 0));
                    mulVecMatAddTrans(&v2C, &mtx);
                    fn_803B993C(self->handles_0x328[i], &v2C, self->area_no);
                }
            }
            i++;
        } while (i < 4);
    }
}

/* The model-effect refresh: when the map lookup says 6 and the enemy is in area 1, copy the scene
 * model's effect matrix (wrapped into [0,1) on the Y row), then push the enemy's material colour
 * (alpha scaled by `field_0x1D4`). */
void fn_801A9210(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    fn_8005050C(&mtx);
    if (fn_802B0668(self->field_0x1E0) == 6 && self->area_no == 1) {
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

/* The per-kind predicate the action code polls.  `kind` selects the byte/float test; the return is
 * 0/1 for kinds 0/1/4/5 and a 0..3 band for kinds 2/3, so the return is `int` (the target's arms
 * have no trailing `clrlwi`), and the switch narrows the wider argument once (`clrlwi r4,24`).
 * Residual 83.3 %: the target's kinds 0/1/5 keep the redundant bool conversion and kind 3 keeps the
 * `extrwi`+`clrlwi` the value forms below let MWCC fold away. */
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
        return fn_803B50A8() == 1;
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

/* Clear the two bytes and reset the position and the three rotation words. */
void fn_801A94C4(_ENEMY_WORK* self, u8* a, u8* b) {
    fn_80130478(self, 0);
    *a = 0;
    *b = 0;
    setVector3(&self->pos, lbl_80798538, lbl_80798538, lbl_80798538);
    self->field_0x1BC = 0;
    self->field_0x1C0 = 0;
    self->field_0x1C4 = 0;
}

}  /* extern "C" */
