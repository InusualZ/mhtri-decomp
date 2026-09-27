/* enemy/fn_80387844.cpp - the enemy monster-AI action band, `.text` 0x80387844..0x8038E8E8.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup`: every address here resolves to a `zz_XXXXXXXX_` dump
 * name and a bare `.text` entry in config/RMHE08/symbols.txt, so no real function name survives).
 *
 * WHAT IT IS.  The per-action state machines of an enemy monster's AI: every body takes the shared
 * `_ENEMY_WORK` record and drives its `state` (+0x005) through a small machine whose steps call the
 * motion API (`fn_8012F5B8`, `fn_80130478`, `fn_8012F93C`), test frames (`em_frame_check`), spawn
 * effects (`eft009_set_pos`, `fn_801048B4`, `fn_80304508`, `fn_8010562C`) and rotate the work
 * (`rotVecY`).  Four dispatch tables pick the machine by the work's `action` (+0x1E5) or
 * `state_sub` (+0x1E6): `fn_80388144`/`fn_80388B98`/`fn_80389634`/`fn_8038BA34` (state_sub),
 * `fn_8038CBD4` (action).  The band is the enemy module's action-program block; its `.data` holds
 * the compiler-emitted switch tables and the `lbl_805EFCE4`/`lbl_805F05xx` record tables the
 * machines index.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lis`/`addi` pair in the range resolves to the `.sdata2` float pool, a switch table or one of the
 * band's own record tables, never to a source-file-name literal.  2. `dumpmap.py lookup` answers only
 * `zz_XXXXXXXX_` placeholders.  3. The code places the unit in `enemy`: every body drives
 * `_ENEMY_WORK` and calls `em_frame_check__FP11_ENEMY_WORKUsff`, `em_magma_check`,
 * `get_joint_wpos_em`, and the `.data` switch tables sit in the band the enemy `em0XX_prog_tbl`
 * program tables bracket.  The file therefore keeps the map's `fn_80387844` stem (brief option 4);
 * no name was invented and no module was guessed.
 *
 * LANGUAGE AND SECTIONS.  C++: the range reaches genuinely mangled callees (`setVector3`,
 * `mulVecMatAddTrans`, `rotVecY`, `calcVecAng2`, `eft009_set_pos`) through their real signatures
 * (rule 9).  Every plain `fn_` definition is `extern "C"` so it keeps the map's name (playbook 42).
 * Sections: `.text` 0x80387844..0x8038E8E8, plus the extab/extabindex runs its framed functions
 * carry.  The `.data` switch tables are not yet claimed (residual - see the outbox).
 *
 * STATUS / RESIDUALS.  See the outbox `config_requests` and the batch note; the band was written
 * from the target object's own disassembly and the m2c shape oracle.
 */

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h"
#include "ef/eft009.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "enemy/fn_80387844.h"
#include "mh3_pad.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"

/* The shell-callback table `shell_set_func_ptr` points at: this band calls its +0x18 and +0x2C
 * slots (the `ai` band's view names +0x80/+0x88).  A local view with its own name (the shared
 * `ShellSetFuncs` in `include/stage/fn_802B2AA0.h` carries the other view). */
struct EmShellSet;
typedef void (*EmShellFn18)(_ENEMY_WORK*, nw4r::math::VEC3*, void*, u32, u32, u32, EmShellSet*);
typedef void (*EmShellFn2C)(_ENEMY_WORK*, u32, u32, nw4r::math::VEC3*, f32, u32, EmShellSet*);
/* size: 0x8C */
struct EmShellSet {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ EmShellFn18 method_0x18;
    /* +0x1C */ u8 pad_0x1C[0x2C - 0x1C];
    /* +0x2C */ EmShellFn2C method_0x2C;
    /* +0x30 */ u8 pad_0x30[0x8C - 0x30];
};
extern "C" EmShellSet* shell_set_func_ptr;

/* The band's pooled `.sdata2` constants (declared, never defined: the pool belongs to the data pass,
 * playbook 29). */
extern f32 lbl_8079BFF8; extern f32 lbl_8079BFFC;
extern f32 lbl_8079C000; extern f32 lbl_8079C004; extern f32 lbl_8079C008; extern f32 lbl_8079C00C;
extern f32 lbl_8079C010; extern f32 lbl_8079C014; extern f32 lbl_8079C01C; extern f32 lbl_8079C020;
extern f32 lbl_8079C024; extern f32 lbl_8079C028; extern f32 lbl_8079C02C; extern f32 lbl_8079C030;
extern f32 lbl_8079C034; extern f32 lbl_8079C038; extern f32 lbl_8079C03C; extern f32 lbl_8079C040;
extern f32 lbl_8079C044; extern f32 lbl_8079C048; extern f32 lbl_8079C04C; extern f32 lbl_8079C050;
extern f32 lbl_8079C054; extern f32 lbl_8079C058; extern f32 lbl_8079C05C; extern f32 lbl_8079C060;
extern f32 lbl_8079C064; extern f32 lbl_8079C068; extern f32 lbl_8079C06C; extern f32 lbl_8079C070;
extern f32 lbl_8079C074; extern f32 lbl_8079C078; extern f32 lbl_8079C07C; extern f32 lbl_8079C080;
extern f32 lbl_8079C084; extern f32 lbl_8079C088; extern f32 lbl_8079C08C; extern f32 lbl_8079C090;
extern f32 lbl_8079C094; extern f32 lbl_8079C098; extern f32 lbl_8079C09C; extern f32 lbl_8079C0A0;
extern f32 lbl_8079C0A4; extern f32 lbl_8079C0A8; extern f32 lbl_8079C0AC; extern f32 lbl_8079C0B0;
extern f32 lbl_8079C0B4; extern f32 lbl_8079C0B8; extern f32 lbl_8079C0BC; extern f32 lbl_8079C0C0;
extern f32 lbl_8079C0C4; extern f32 lbl_8079C0C8; extern f32 lbl_8079C0CC; extern f32 lbl_8079C0D0;
extern f32 lbl_8079C0D4; extern f32 lbl_8079C0D8; extern f32 lbl_8079C0DC; extern f32 lbl_8079C0E0;
extern f32 lbl_8079C0E4; extern f32 lbl_8079C0F0; extern f32 lbl_8079C0F4; extern f32 lbl_8079C0F8;
extern f32 lbl_8079C0FC; extern f32 lbl_8079C100; extern f32 lbl_8079C104; extern f32 lbl_8079C108;
extern f32 lbl_8079C10C; extern f32 lbl_8079C110; extern f32 lbl_8079C114; extern f32 lbl_8079C118;
extern f32 lbl_8079C11C; extern f32 lbl_8079C120; extern f32 lbl_8079C124; extern f32 lbl_8079C128;
extern f32 lbl_8079C12C; extern f32 lbl_8079C130; extern f32 lbl_8079C134; extern f32 lbl_8079C138;
extern f32 lbl_8079C13C; extern f32 lbl_8079C140; extern f32 lbl_8079C144; extern f32 lbl_8079C148;
extern f32 lbl_8079C14C; extern f32 lbl_8079C150; extern f32 lbl_8079C154; extern f32 lbl_8079C158;
extern f32 lbl_8079C15C; extern f32 lbl_8079C160; extern f32 lbl_8079C164; extern f32 lbl_8079C168;
extern f32 lbl_8079C16C; extern f32 lbl_8079C170; extern f32 lbl_8079C174; extern f32 lbl_8079C178;
extern f32 lbl_8079C17C; extern f32 lbl_8079C180; extern f32 lbl_8079C184; extern f32 lbl_8079C188;
extern f32 lbl_8079C18C; extern f32 lbl_8079C190; extern f32 lbl_8079C194; extern f32 lbl_8079C198;
extern f32 lbl_8079C19C; extern f32 lbl_8079C1A0; extern f32 lbl_8079C1A4; extern f32 lbl_8079C1A8;
extern f32 lbl_8079C1AC; extern f32 lbl_8079C1B0; extern f32 lbl_8079C1B4; extern f32 lbl_8079C1B8;
extern f32 lbl_8079C1BC; extern f32 lbl_8079C1C0; extern f32 lbl_8079C1C4; extern f32 lbl_8079C1C8;
extern f32 lbl_8079C1CC; extern f32 lbl_8079C1D0; extern f32 lbl_8079C1D4; extern f32 lbl_8079C1D8;
extern f32 lbl_8079C1DC; extern f32 lbl_8079C1E0; extern f32 lbl_8079C1E4; extern f32 lbl_8079C1E8;
extern f32 lbl_8079C1EC; extern f32 lbl_8079C1F0; extern f32 lbl_8079C1F4; extern f32 lbl_8079C1F8;
extern f32 lbl_8079C1FC; extern f32 lbl_8079C200; extern f32 lbl_8079C204; extern f32 lbl_8079C208;
extern f32 lbl_8079C20C; extern f32 lbl_8079C210; extern f32 lbl_8079C214; extern f32 lbl_8079C218;
extern f32 lbl_8079C21C; extern f32 lbl_8079C220; extern f32 lbl_8079C224; extern f32 lbl_8079C228;
extern f32 lbl_8079C22C; extern f32 lbl_8079C230; extern f32 lbl_8079C234; extern f32 lbl_8079C238;
extern f32 lbl_8079C23C; extern f32 lbl_8079C240; extern f32 lbl_8079C244; extern f32 lbl_8079C248;
extern f32 lbl_8079C24C; extern f32 lbl_8079C250; extern f32 lbl_8079C254; extern f32 lbl_8079C258;
extern f32 lbl_8079C25C; extern f32 lbl_8079C260; extern f32 lbl_8079C264; extern f32 lbl_8079C268;
extern f32 lbl_8079C26C; extern f32 lbl_8079C270; extern f32 lbl_8079C274; extern f32 lbl_8079C278;
extern f32 lbl_8079C27C; extern f32 lbl_8079C280; extern f32 lbl_8079C284; extern f32 lbl_8079C288;
extern f32 lbl_8079C28C; extern f32 lbl_8079C290;

/* The band's own `.data`/`.rodata` tables (declared, never defined). */
extern u8 lbl_80570B20[]; extern u8 lbl_80570B60[];
extern u8 lbl_805EFA00[]; extern u8 lbl_805EFCE4[];
extern u8 lbl_805EFF68[]; extern u8 lbl_805EFF90[]; extern u8 lbl_805EFFB8[];
extern u8 lbl_805F0018[]; extern u8 lbl_805F0088[]; extern u8 lbl_805F00C8[];
extern u8 lbl_805F00F0[]; extern u8 lbl_805F0118[]; extern u8 lbl_805F0140[];
extern u8 lbl_805F0168[]; extern u8 lbl_805F0190[]; extern u8 lbl_805F01B8[];
extern u8 lbl_805F0210[]; extern u8 lbl_805F0278[]; extern u8 lbl_805F02B0[];
extern u8 lbl_805F02D8[]; extern u8 lbl_805F0300[]; extern u8 lbl_805F0328[];
extern u8 lbl_805F0350[]; extern u8 lbl_805F0378[]; extern u8 lbl_805F03A0[];
extern u8 lbl_805F03C8[]; extern u8 lbl_805F0408[]; extern u8 lbl_805F0478[];
extern u8 lbl_805F04C0[]; extern u8 lbl_805F04F0[]; extern u8 lbl_805F0518[];
extern u8 lbl_805F0558[]; extern u8 lbl_805F0570[]; extern u8 lbl_805F0620[];
extern u8 lbl_805F0790[]; extern u8 lbl_805F07B8[];

/* forward declarations of the larger state machines, defined further down in address order. */
extern "C" void fn_8038A198(_ENEMY_WORK* self, u8 a);
extern "C" void fn_8038A7B8(_ENEMY_WORK* self, u8 a, u8 b);
extern "C" void fn_8038C124(_ENEMY_WORK* self);
extern "C" void fn_8038CCA4(_ENEMY_WORK* self, u8 a, u8 b, u32 c, s32 d, f32 e);
extern "C" void fn_8038D0A4(_ENEMY_WORK* self);

static _ENEMY_WORK* w(void* p) { return (_ENEMY_WORK*)p; }

/* =================================================================================================
 * 0x80387844
 * ================================================================================================= */
extern "C" void fn_80387844(_ENEMY_WORK* self)
{
    u32 found;
    u16 count;
    u16 i;
    u8* move;

    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x28, 2, 0);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        return;
    case 2:
        fn_8013221C(self, lbl_8079C024, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            found = 1;
        } else if (fn_8012D1A0(self) == 1) {
            count = get_move_work_max(2);
            move = (u8*)get_move_work_adrs(2);
            found = 0;
            for (i = 0; i < count; i++) {
                if (move[0] != 0 && fn_8012D1A8(move[8]) == 0 && fn_8012D0B4(self, move) == 1) {
                    found = 1;
                }
                move += 0xB20;
            }
        }
        if (found == 1) {
            fn_8012B380(self, 5, 8, 4);
            fn_80128A14(self, 1, 6);
        }
        return;
    }
}

/* =================================================================================================
 * 0x803879EC
 * ================================================================================================= */
extern "C" void fn_803879EC(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC8, 4, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_8079C028, lbl_8079BFF8) == 1) {
            fn_80306A98(self, 0x10);
        }
        if (em_frame_check(self, 1, lbl_8079C02C, lbl_8079C030) == 1
            && (system_w.field_0x0c & 7) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079C034, lbl_8079C038);
            fn_80304508(self, 0x90, 0x15, &v, lbl_8079C00C);
        }
        if (em_after_frame_check(self, 0, lbl_8079C03C, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C040, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C044, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C048, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C04C, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C050, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C054, lbl_8079BFF8) == 1
            || em_after_frame_check(self, 0, lbl_8079C058, lbl_8079BFF8) == 1) {
            setVector3(&v, lbl_8079BFF8, lbl_8079C034, lbl_8079C038);
            fn_80304508(self, 0x8F, 0x15, &v, lbl_8079C00C);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80128A14(self, 1, 5);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80387C28
 * ================================================================================================= */
extern "C" void fn_80387C28(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_8012F5B8(self, 0xCA, 4, 0);
        fn_80130CDC(self, 0x3E8);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80387CA4
 * ================================================================================================= */
extern "C" void fn_80387CA4(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_8012F62C(self, 0x6E, 0, 0);
        self->timer_0x020 = 0x96;
        fn_80132224(self);
        return;
    case 1:
        fn_8013221C(self, lbl_8079C024, 1, 3);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            fn_8012F5B8(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x26, 4, 0);
        }
        return;
    case 3:
        if (em_frame_check(self, 1, lbl_8079C05C, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 4);
            fn_80134964(self, lbl_80570B20, 0, 0, 0);
            fn_8013032C(self);
            fn_801303EC(self, lbl_8079BFF8);
        }
        return;
    case 4:
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            self->state++;
            fn_80130478(self, 4);
            fn_8012F5B8(self, 0xA, 0, 0);
            fn_80134004(self, 0, lbl_8079BFF8);
            fn_8013032C(self);
            fn_801303EC(self, lbl_8079BFF8);
        }
        return;
    case 5:
        fn_80131D84(self);
        if (fn_80134114(self, 0, 0x100) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x27, 0, 0);
            fn_801303EC(self, lbl_8079BFF8);
            fn_80136D14(self);
        }
        return;
    case 6:
        if (em_frame_check(self, 2, lbl_8079C060, lbl_8079BFF8) == 1) {
            fn_8012CF20(self);
            fn_80131E74(self);
        }
        if (em_frame_check(self, 3, lbl_8079C064, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F62C(self, 1, 6, 0);
        }
        return;
    case 7:
        if (fn_8012F948(self) == 1) {
            fn_8012B380(self, 0, 0, 0);
            fn_80128A14(self, 1, 7);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80387FF4
 * ================================================================================================= */
extern "C" void fn_80387FF4(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_80570B20, 0, 1, 0);
        return;
    case 1:
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F62C(self, 2, 6, 0);
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x803880C8
 * ================================================================================================= */
extern "C" void fn_803880C8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xE, 6, 0);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388144 - dispatch by `state_sub`
 * ================================================================================================= */
extern "C" void fn_80388144(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: fn_80387528(); return;
    case 1: fn_803875A4(); return;
    case 2: fn_80387620(); return;
    case 3: fn_80387844(self); return;
    case 4: fn_803879EC(self); return;
    case 5: fn_80387C28(self); return;
    case 6: fn_80387CA4(self); return;
    case 7: fn_80387FF4(self); return;
    case 8: fn_803880C8(self); return;
    }
}

/* =================================================================================================
 * 0x80388190
 * ================================================================================================= */
extern "C" void fn_80388190(_ENEMY_WORK* self, u8 a, u8 b)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xA, 4, 0);
        fn_8012933C(self, 0, 0xC, 2);
        switch (a) {
        case 0: fn_80134004(self, 0, lbl_8079BFF8); return;
        case 1: fn_80134004(self, 0, lbl_8079C068); return;
        case 2: fn_80134004(self, 0, lbl_8079C06C); return;
        case 3: fn_80134004(self, 0, lbl_8079C070); return;
        case 4: fn_80134004(self, 0, lbl_8079BFF8); return;
        }
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            switch (b) {
            case 0:
                self->state++;
                fn_8012F5B8(self, 0xB, 6, 0);
                return;
            case 1:
                fn_80127F48(self);
                return;
            }
        }
        return;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038833C
 * ================================================================================================= */
extern "C" void fn_8038833C(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_80570B20, 0, 1, 0);
        return;
    case 1:
        if (a == 1 && fn_8012F93C(self) == 1) {
            fn_80127F48(self);
            return;
        }
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x803883F0
 * ================================================================================================= */
extern "C" void fn_803883F0(_ENEMY_WORK* self)
{
    f32 ratio;

    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x21, 4, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079BFF8, lbl_8079C074) == 1) {
            ratio = (lbl_8079C07C * (lbl_8079C080 * fn_8012F8E4(self))) / lbl_8079C03C;
            fn_80133C50(self, (u16)(s32)(lbl_8079C078 + ratio));
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x803884C8
 * ================================================================================================= */
extern "C" void fn_803884C8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x10, 0xA, 0);
        fn_80134004(self, 0, lbl_8079BFF8);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038855C
 * ================================================================================================= */
extern "C" void fn_8038855C(_ENEMY_WORK* self)
{
    f32 scale;

    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_80134964(self, lbl_80570B60, 0, 1, 0);
        fn_801353F8(self);
        scale = get_em_chg_scale(self);
        self->field_0x318 = lbl_8079C084 * fn_8012F8E4(self) * scale;
        self->field_0x324 = lbl_8079BFF8;
        rotVecY(&self->offset_0x30C.vec_0x310, calcVecAng2(&self->pos, &self->vec_0x36C));
        rotVecY(&self->vec_0x31C, calcVecAng2(&self->pos, &self->vec_0x36C));
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079BFF8, lbl_8079C088) == 1) {
            CancelFade(self);
        }
        if (fn_80134B0C(self, lbl_80570B60) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388684
 * ================================================================================================= */
extern "C" void fn_80388684(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, a == 0 ? 0x1E : 0x1F, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079C08C, lbl_8079C090) == 1) {
            fn_80133C50(self, 0x100);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038874C
 * ================================================================================================= */
extern "C" void fn_8038874C(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;
    f32 s1;
    f32 s2;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x17, 6, 0);
        fn_80134004(self, 0x10, lbl_8079BFF8);
        fn_801353F8(self);
        self->offset_0x30C.vec_0x310.x = lbl_8079BFF8;
        self->field_0x314 = lbl_8079BFF8;
        s1 = get_em_chg_scale(self);
        self->field_0x318 = lbl_8079C094 * fn_8012F8E4(self) * s1;
        self->vec_0x31C.x = lbl_8079BFF8;
        self->field_0x320 = lbl_8079BFF8;
        s2 = get_em_chg_scale(self);
        self->field_0x324 = fn_8012F8E4(self) * s2;
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079C098, lbl_8079BFF8) == 1
            && (system_w.field_0x0c & 3) == 0) {
            if (em_magma_check(self) == 1) {
                copyVec3(&v, &self->pos);
                v.y = lbl_8079C09C + self->field_0x214;
                eft009_set_pos(0x8B, &v, (struct _CP_VECTOR*)&self->field_0x1BC, lbl_8079C00C, self->area_no);
            } else {
                setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C028);
                fn_80304508(self, 0x44, 1, &v, lbl_8079C00C);
                fn_80304508(self, 0x82, 1, &v, lbl_8079C00C);
            }
        }
        if (em_frame_check(self, 0, lbl_8079C0A0, lbl_8079BFF8) == 1) {
            fn_8012933C(self, 0, 6, 0xA);
        }
        if (em_frame_check(self, 1, lbl_8079C0A4, lbl_8079BFF8) == 1) {
            fn_801355C8(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_8079C0A8) {
                self->field_0x318 = lbl_8079C0A8;
            } else {
                fn_8012F8C8(self, lbl_8079C0AC + self->field_0x7C4);
            }
        }
        if (fn_80134114(self, 0, 0x80) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x18, 0, 0);
            self->field_0x318 *= lbl_8079C0B0;
            self->vec_0x31C.x = lbl_8079BFF8;
            self->field_0x320 = lbl_8079BFF8;
            self->field_0x324 = lbl_8079C0B4;
        }
        return;
    case 2:
        if (em_frame_check(self, 1, lbl_8079BFFC, lbl_8079C0B8) == 1
            && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (struct _CP_VECTOR*)&self->field_0x1BC, lbl_8079C00C, self->area_no);
            } else {
                setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079BFF8);
                fn_80304508(self, 0x45, 0x23, &v, lbl_8079C00C);
            }
        }
        if (em_frame_check(self, 1, lbl_8079C0BC, lbl_8079C0C0) == 1
            && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (struct _CP_VECTOR*)&self->field_0x1BC, lbl_8079C00C, self->area_no);
            } else {
                setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079BFF8);
                fn_80304508(self, 0x45, 0x23, &v, lbl_8079C00C);
            }
        }
        if (em_frame_check(self, 0, lbl_8079C0C4, lbl_8079BFF8) == 1) {
            fn_80129724(self, 0);
        }
        fn_801355C8(self, &self->field_0x1BC);
        if (self->field_0x318 < lbl_8079BFF8) {
            self->field_0x318 = lbl_8079BFF8;
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388B98 - dispatch by `state_sub`
 * ================================================================================================= */
extern "C" void fn_80388B98(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: fn_80388190(self, 1, 0); return;
    case 1: fn_80388190(self, 0, 1); return;
    case 2: fn_80388190(self, 1, 1); return;
    case 3: fn_80388190(self, 2, 1); return;
    case 4: fn_80388190(self, 3, 1); return;
    case 5: fn_80388190(self, 4, 1); return;
    case 6: fn_8038833C(self, 0); return;
    case 7: fn_803883F0(self); return;
    case 8: fn_8038833C(self, 1); return;
    case 9: fn_803884C8(self); return;
    case 10: fn_8038855C(self); return;
    case 11: fn_80388684(self, 0); return;
    case 12: fn_80388684(self, 1); return;
    case 13: fn_8038874C(self); return;
    }
}

/* =================================================================================================
 * 0x80388C38
 * ================================================================================================= */
extern "C" void fn_80388C38(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x26, 4, 0);
        fn_80129668(self, 0, 0x10);
        fn_80129668(self, 1, 0x11);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079C05C, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            fn_8013032C(self);
            fn_801303EC(self, lbl_8079BFF8);
            fn_801280F4(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388D1C
 * ================================================================================================= */
extern "C" void fn_80388D1C(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F62C(self, 1, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self, lbl_8079BFF8);
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        if (--self->timer_0x020 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388DC8
 * ================================================================================================= */
extern "C" void fn_80388DC8(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 6, 0, 0);
        fn_80134004(self, 0, lbl_8079C0C8);
        fn_8013032C(self);
        fn_801303EC(self, lbl_8079BFF8);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x100) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388E88
 * ================================================================================================= */
extern "C" void fn_80388E88(_ENEMY_WORK* self, u8 a)
{
    f32 end = lbl_8079BFF8;

    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F5B8(self, 0xA, 0, 0);
        fn_8012F8C8(self, lbl_8079C0CC);
        if (a == 0) {
            end = lbl_8079C0D0;
        }
        fn_80134004(self, 0, end);
        fn_8013032C(self);
        fn_801303EC(self, lbl_8079BFF8);
        return;
    case 1:
        if (fn_80134114(self, 0, 0x100) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80388F88
 * ================================================================================================= */
extern "C" void fn_80388F88(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_80134964(self, lbl_80570B20, 0, 0, 0);
        fn_8013032C(self);
        fn_801303EC(self, lbl_8079BFF8);
        return;
    case 1:
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            fn_801280F4(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80389038
 * ================================================================================================= */
extern "C" void fn_80389038(_ENEMY_WORK* self)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x27, 0, 0);
        fn_801303EC(self, lbl_8079BFF8);
        fn_80129668(self, 0, 0x13);
        fn_80129668(self, 1, 0x14);
        fn_80136D14(self);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079C064, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80389120
 * ================================================================================================= */
extern "C" void fn_80389120(_ENEMY_WORK* self, u8 a)
{
    fn_8012CF20(self);
    fn_80131E74(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x26, 4, 0);
        fn_80129668(self, 0, 0x10);
        fn_80129668(self, 1, 0x11);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079C05C, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            if ((u32)(a - 3) > 6) {
                if (a > 2) {
                    if ((u32)(a - 0xA) <= 2) {
                        fn_8012B380(self, 5, 8, 9);
                    }
                } else {
                    fn_8012B380(self, 5, 8, 7);
                }
            } else {
                fn_8012B380(self, 5, 8, 8);
            }
            fn_80128BF8(self, 0);
            fn_80130478(self, 4);
            fn_80134964(self, lbl_80570B20, 0, 0, 0);
            fn_8013032C(self);
            fn_801303EC(self, lbl_8079BFF8);
        }
        return;
    case 2:
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            self->state++;
            fn_8012F5B8(self, 0xA, 0, 0);
            fn_8012F8C8(self, lbl_8079C0CC);
            fn_80134004(self, 0, lbl_8079BFF8);
            fn_8013032C(self);
            fn_801303EC(self, lbl_8079BFF8);
            fn_80131D84(self);
        }
        return;
    case 3:
        fn_80131D84(self);
        if (fn_80134114(self, 0, 0x100) == 1) {
            if ((u32)(a - 3) > 6) {
                if (a <= 2 || (u32)(a - 0xA) <= 2) {
                    self->state++;
                    fn_8012B380(self, 5, 8, 2);
                    fn_80128BF8(self, 0);
                    fn_80134964(self, lbl_80570B20, 0, 0, 0);
                    fn_8013032C(self);
                    fn_801303EC(self, lbl_8079BFF8);
                    return;
                }
            } else {
                self->state = 5;
                fn_80130478(self, 0);
                fn_8012F5B8(self, 0x27, 0, 0);
                self->field_0x1BC = 0;
                self->field_0x1C0 = 0xDC00;
                self->field_0x1C4 = 0;
                fn_801303EC(self, lbl_8079BFF8);
                fn_80129668(self, 0, 0x13);
                fn_80129668(self, 1, 0x14);
                fn_80136D14(self);
                return;
            }
        }
        return;
    case 4:
        if (fn_80134B0C(self, lbl_80570B20) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x27, 0, 0);
            fn_801303EC(self, lbl_8079BFF8);
            fn_80129668(self, 0, 0x13);
            fn_80129668(self, 1, 0x14);
            fn_80136D14(self);
        }
        return;
    case 5:
        if (em_frame_check(self, 3, lbl_8079C064, lbl_8079BFF8) == 1) {
            fn_80136D14(self);
        }
        if (fn_8012F93C(self) == 1) {
            switch (a) {
            case 0: fn_80128A14(self, 7, 0x22); return;
            case 1: fn_80128A14(self, 7, 0x2B); return;
            case 2: fn_80128A14(self, 7, 0x2D); return;
            case 3: fn_80128A14(self, 7, 0x2F); return;
            case 4: fn_80128A14(self, 7, 0x31); return;
            case 5: fn_80128A14(self, 7, 0x33); return;
            case 6: fn_80128A14(self, 7, 0x35); return;
            case 7: fn_80128A14(self, 7, 0x37); return;
            case 8: fn_80128A14(self, 7, 0x39); return;
            case 9: fn_80128A14(self, 7, 0x3B); return;
            case 10: fn_80128A14(self, 7, 0x3D); return;
            case 11: fn_80128A14(self, 7, 0x3F); return;
            case 12: fn_80128A14(self, 7, 0x41); return;
            }
        }
        return;
    }
}

/* =================================================================================================
 * 0x80389634 - dispatch by `state_sub`
 * ================================================================================================= */
extern "C" void fn_80389634(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: fn_80388C38(self); return;
    case 1: fn_80388D1C(self); return;
    case 2: fn_80388DC8(self); return;
    case 3: fn_80388E88(self, 0); return;
    case 4: fn_80388F88(self); return;
    case 5: fn_80389038(self); return;
    case 6: fn_80388E88(self, 1); return;
    case 10: fn_80389120(self, 0); return;
    case 11: fn_80389120(self, 1); return;
    case 12: fn_80389120(self, 2); return;
    case 13: fn_80389120(self, 3); return;
    case 14: fn_80389120(self, 4); return;
    case 15: fn_80389120(self, 5); return;
    case 16: fn_80389120(self, 6); return;
    case 17: fn_80389120(self, 7); return;
    case 18: fn_80389120(self, 8); return;
    case 19: fn_80389120(self, 9); return;
    case 20: fn_80389120(self, 0xA); return;
    case 21: fn_80389120(self, 0xB); return;
    case 22: fn_80389120(self, 0xC); return;
    }
}

/* =================================================================================================
 * 0x803896E8
 * ================================================================================================= */
extern "C" void fn_803896E8(_ENEMY_WORK* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v;
    f32 f;
    f32 g;
    u16 ang;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x12, 4, 0);
        fn_8012933C(self, 0, 1, 8);
        fn_80130CDC(self, -0x1E);
        ang = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (ang >= 0x8000) {
            ang = 0x10000 - ang;
        }
        self->timer_0x020 = (s16)ang;
        if ((s16)ang > 0x3000) {
            self->timer_0x020 = 0x3000;
        }
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_8079C0A8, lbl_8079C008) == 1) {
            f = lbl_8079C03C * (f32)self->timer_0x020;
            g = f / lbl_8079C07C;
            f = (lbl_8079C07C * ((g / lbl_8079C0A8) * fn_8012F8E4(self))) / lbl_8079C03C;
            fn_80133C50(self, (u16)(s32)(lbl_8079C078 + f));
        }
        if (em_frame_check(self, 0, lbl_8079C010, lbl_8079BFF8) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0D4, lbl_8079C060);
            mulVecMatAddTrans(&v, &mtx);
            shell_set_func_ptr->method_0x18(self, &v, &self->field_0x1BC, 0x2F, self->area_no, self->field_0xAEA, shell_set_func_ptr);
        }
        if (em_frame_check(self, 0, lbl_8079C0D8, lbl_8079BFF8) == 1) {
            fn_80136B50(self, 0x15, 6);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0DC, lbl_8079C0E0);
            shell_set_func_ptr->method_0x2C(self, 9, 0x15, &v, lbl_8079C0E4, self->field_0xAEA, shell_set_func_ptr);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038995C
 * ================================================================================================= */
extern "C" void fn_8038995C(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::VEC3 v;
    f32 s;
    f32 t;
    s32 idx;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x15, 4, 0);
        fn_801353F8(self);
        self->angle_0x328 = self->field_0x1C0;
        fn_8012933C(self, 0, 2, 8);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_8079C0F0, lbl_8079BFF8) == 1) {
            fn_8012F8C8(self, lbl_8079C0F4);
        } else {
            fn_8012F8C8(self, lbl_8079C00C);
        }
        fn_80133E3C(self, 0x4000, lbl_8079C0F8, lbl_8079C0FC);
        fn_80133E3C(self, 0x4000, lbl_8079C0FC, lbl_8079C0D8);
        fn_80133E3C(self, 0x4000, lbl_8079C0D8, lbl_8079C100);
        fn_80133E3C(self, 0x4000, lbl_8079C100, lbl_8079C104);
        fn_80133E3C(self, 0x4000, lbl_8079C104, lbl_8079C108);
        self->offset_0x30C.vec_0x310.x = lbl_8079BFF8;
        self->field_0x314 = lbl_8079BFF8;
        s = fn_8012F8E4(self);
        t = fn_80135644(self, lbl_805EFA00) * s;
        s = get_em_chg_scale(self) * t;
        self->field_0x318 = lbl_8079C10C * s;
        rotVecY(&self->offset_0x30C.vec_0x310, self->angle_0x328);
        fn_80135418(self);
        if (a > 1) {
            if (a == 2 && fn_8012F93C(self) == 1) {
                fn_80127F48(self);
            }
            return;
        }
        if (em_frame_check(self, 0, lbl_8079C110, lbl_8079BFF8) == 1) {
            idx = self->bits_0x1EC % 3;
            if (a == 0) {
                switch (idx) {
                case 0: fn_80128A14(self, 7, 0x24); return;
                case 1: fn_80128A14(self, 7, 0x25); return;
                case 2: fn_80128A14(self, 7, 0x26); return;
                }
            } else {
                switch (idx) {
                case 0: fn_80128A14(self, 7, 0x28); return;
                case 1: fn_80128A14(self, 7, 0x29); return;
                case 2: fn_80128A14(self, 7, 0x2A); return;
                }
            }
        }
        return;
    }
}

/* =================================================================================================
 * 0x80389C50
 * ================================================================================================= */
extern "C" void fn_80389C50(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;
    f32 s;
    f32 t;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0x15, 0, 0);
        fn_801353F8(self);
        if (self->field_0xAE9 == 1) {
            fn_8012933C(self, 0, 2, 0x10);
        } else {
            fn_80129668(self, 0, 2);
        }
        /* fallthrough */
    case 1:
        if (em_frame_check(self, 3, lbl_8079C0F0, lbl_8079BFF8) == 1) {
            fn_8012F8C8(self, lbl_8079C0F4);
        } else {
            fn_8012F8C8(self, lbl_8079C00C);
        }
        fn_80133E3C(self, 0x4000, lbl_8079C0F8, lbl_8079C0FC);
        fn_80133E3C(self, 0x4000, lbl_8079C0FC, lbl_8079C0D8);
        fn_80133E3C(self, 0x4000, lbl_8079C0D8, lbl_8079C100);
        fn_80133E3C(self, 0x4000, lbl_8079C100, lbl_8079C104);
        fn_80133E3C(self, 0x4000, lbl_8079C104, lbl_8079C108);
        self->offset_0x30C.vec_0x310.x = lbl_8079BFF8;
        self->field_0x314 = lbl_8079BFF8;
        s = fn_8012F8E4(self);
        t = fn_80135644(self, lbl_805EFA00) * s;
        s = get_em_chg_scale(self) * t;
        self->field_0x318 = lbl_8079C10C * s;
        rotVecY(&self->offset_0x30C.vec_0x310, self->angle_0x328);
        fn_80135418(self);
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x80389E1C
 * ================================================================================================= */
extern "C" void fn_80389E1C(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::VEC3 v;
    u32 id1;
    u32 id2;
    u32 kind;

    VEC3_ctor(&v);
    if (a == 1) {
        id1 = 0xCB;
        id2 = 0xCA;
        kind = 0xF;
    } else {
        id1 = 0x7F;
        id2 = 0x4C;
        kind = 3;
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x13, 4, 0);
        fn_8012F8C8(self, lbl_8079C114);
        fn_80130CDC(self, -0xA);
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        return;
    case 1:
        if (em_after_frame_check(self, 3, lbl_8079C000, lbl_8079C118) == 1
            && (system_w.field_0x0c & 3) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079BFF8);
            fn_80304508(self, id1, 3, &v, lbl_8079C00C);
        }
        if (em_after_frame_check(self, 3, lbl_8079C11C, lbl_8079C120) == 1
            && (system_w.field_0x0c & 7) == 0) {
            if (self->state_0x007 == 0) {
                self->state_0x007 = 1;
                setVector3(&v, lbl_8079BFF8, lbl_8079C0C8, lbl_8079BFF8);
                fn_80304510(self, id2, 3, &v, 0, lbl_8079C00C);
            } else {
                self->state_0x007 = 0;
                setVector3(&v, lbl_8079BFF8, lbl_8079C0C8, lbl_8079BFF8);
                fn_80304510(self, id2, 3, &v, 0x2000, lbl_8079C00C);
            }
        }
        if (em_frame_check(self, 0, lbl_8079C100, lbl_8079BFF8) == 1) {
            self->state++;
            fn_8012933C(self, 0, kind, 2);
        }
        return;
    case 2:
        if (em_after_frame_check(self, 3, lbl_8079C11C, lbl_8079C120) == 1
            && (system_w.field_0x0c & 7) == 0) {
            if (self->state_0x007 == 0) {
                self->state_0x007 = 1;
                setVector3(&v, lbl_8079BFF8, lbl_8079C0C8, lbl_8079BFF8);
                fn_80304510(self, id2, 3, &v, 0, lbl_8079C00C);
            } else {
                self->state_0x007 = 0;
                setVector3(&v, lbl_8079BFF8, lbl_8079C0C8, lbl_8079BFF8);
                fn_80304510(self, id2, 3, &v, 0x2000, lbl_8079C00C);
            }
        }
        if (fn_8012F93C(self) == 1) {
            self->state_0x006++;
            if (a == 0 && self->state_0x006 == 3) {
                fn_8012933C(self, 0, kind, 2);
            }
        }
        if (self->state_0x006 >= 4) {
            self->state++;
            fn_8012F5B8(self, 0x14, 4, 0);
            fn_80129744(self);
        }
        return;
    case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038B888
 * ================================================================================================= */
extern "C" void fn_8038B888(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x1A, 4, 0);
        fn_80129668(self, 0, 5);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038B914
 * ================================================================================================= */
extern "C" void fn_8038B914(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        if (a == 0) {
            fn_8012F5B8(self, 0x1D, 4, 0);
            fn_80129668(self, 0, 7);
            fn_80129668(self, 1, 8);
            return;
        }
        fn_8012F5B8(self, 0x1C, 4, 0);
        fn_80129668(self, 0, 9);
        fn_80129668(self, 1, 0xA);
        return;
    case 1:
        if (a == 0) {
            fn_80133E3C(self, 0x4000, lbl_8079BFF8, lbl_8079C0A8);
        } else {
            fn_80133E3C(self, -0x4000, lbl_8079BFF8, lbl_8079C0A8);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038BD28
 * ================================================================================================= */
extern "C" void fn_8038BD28(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x17: fn_801251D0(self, lbl_805EFF68, 0, 0x17); return;
    case 0x18: fn_801251D0(self, lbl_805EFF90, 0, 0x18); return;
    case 0x19: fn_801251D0(self, lbl_805EFFB8, 0, 0x19); return;
    case 0x1A: fn_801251D0(self, lbl_805EFF90, 0, 0x1A); return;
    case 0x1B: fn_801251D0(self, lbl_805F0018, 0, 0x1B); return;
    case 0x1C: fn_801251D0(self, lbl_805F0088, 0, 0x1C); return;
    case 0x1D: fn_801251D0(self, lbl_805F00C8, 0, 0x1D); return;
    case 0x7A: fn_801251D0(self, lbl_805F00F0, 0, 0x7A); return;
    case 0x7B: fn_801251D0(self, lbl_805F0118, 0, 0x7B); return;
    case 0x9F: fn_801251D0(self, lbl_805F0140, 0, 0x9F); return;
    case 0xA0: fn_801251D0(self, lbl_805F0140, 0, 0xA0); return;
    case 0x7C: fn_801251D0(self, lbl_805F0168, 0, 0x7C); return;
    case 0x8D: fn_801251D0(self, lbl_805F0190, 0, 0x8D); return;
    case 0x78: fn_801251D0(self, lbl_805F01B8, 0, 0x78); return;
    case 0xA8: fn_801251D0(self, lbl_805F02D8, 0, 0xA8); return;
    case 0x7E: fn_801251D0(self, lbl_805F0210, 0, 0x7E); return;
    case 0x7F: fn_801251D0(self, lbl_805F0278, 0, 0x7F); return;
    case 0x8E: fn_801251D0(self, lbl_805F02B0, 0, 0x8E); return;
    case 0xB6: fn_801251D0(self, lbl_805F0300, 0, 0xB6); return;
    case 0xB7: fn_801251D0(self, lbl_805F0328, 0, 0xB7); return;
    case 0xB8: fn_801251D0(self, lbl_805F0350, 0, 0xB8); return;
    case 0xB9: fn_801251D0(self, lbl_805F0378, 0, 0xB9); return;
    case 0xBA: fn_801251D0(self, lbl_805F03A0, 0, 0xBA); return;
    case 0xBB: fn_801251D0(self, lbl_805F03C8, 0, 0xBB); return;
    case 0xBC: fn_801251D0(self, lbl_805F03C8, 0, 0xBC); return;
    case 0xBF: fn_801251D0(self, lbl_805F0408, 0, 0xBF); return;
    case 0xC1: fn_801251D0(self, lbl_805F0190, 0, 0xC1); return;
    case 0xC9: fn_801251D0(self, lbl_805F0478, 0, 0xC9); return;
    default: fn_80127F48(self); return;
    }
}

/* =================================================================================================
 * 0x8038C080
 * ================================================================================================= */
extern "C" void fn_8038C080(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: fn_801251D8(self, lbl_805F04C0, 0, 0); return;
    case 15: fn_801251D8(self, lbl_805F04F0, 0, 0xF); return;
    case 26: fn_801251D8(self, lbl_805F0558, 0, 0x1A); return;
    case 28: fn_801251D8(self, lbl_805F0518, 0, 0x1C); return;
    default: fn_801251D8(self, lbl_805F04C0, 0, 0); return;
    }
}

/* =================================================================================================
 * 0x8038C10C
 * ================================================================================================= */
extern "C" void fn_8038C10C(_ENEMY_WORK* self)
{
    fn_80389038(self);
}

/* =================================================================================================
 * 0x8038C110
 * ================================================================================================= */
extern "C" void fn_8038C110(_ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        fn_8038C10C(self);
    }
}

/* =================================================================================================
 * 0x8038BA34 - dispatch by `state_sub`
 * ================================================================================================= */
extern "C" void fn_8038BA34(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x0: fn_803896E8(self); return;
    case 0x1: fn_8038995C(self, 0); return;
    case 0x2: fn_80389E1C(self, 0); return;
    case 0x3: fn_8038A198(self, 0); return;
    case 0x4: fn_80389E1C(self, 1); return;
    case 0x5: fn_8038A7B8(self, 0, 0); return;
    case 0x6: fn_8038A7B8(self, 1, 0); return;
    case 0x7: fn_8038A7B8(self, 1, 1); return;
    case 0x8: fn_8038B888(self); return;
    case 0x9: fn_8038B914(self, 0); return;
    case 0xA: fn_8038B914(self, 1); return;
    case 0xB: fn_8038A198(self, 1); return;
    case 0xC: fn_8038A198(self, 2); return;
    case 0xD: fn_8038A7B8(self, 2, 0); return;
    case 0xE: fn_8038A7B8(self, 3, 0); return;
    case 0xF: fn_8038A7B8(self, 2, 1); return;
    case 0x10: fn_8038A7B8(self, 3, 1); return;
    case 0x11: fn_8038A7B8(self, 4, 0); return;
    case 0x12: fn_8038A7B8(self, 5, 0); return;
    case 0x13: fn_8038A7B8(self, 4, 1); return;
    case 0x14: fn_8038A7B8(self, 5, 1); return;
    case 0x15: fn_8038A7B8(self, 6, 0); return;
    case 0x16: fn_8038A7B8(self, 7, 0); return;
    case 0x17: fn_8038A7B8(self, 6, 1); return;
    case 0x18: fn_8038A7B8(self, 7, 1); return;
    case 0x19: fn_8038A7B8(self, 8, 0); return;
    case 0x1A: fn_8038A7B8(self, 9, 0); return;
    case 0x1B: fn_8038A7B8(self, 8, 1); return;
    case 0x1C: fn_8038A7B8(self, 9, 1); return;
    case 0x1D: fn_8038A198(self, 3); return;
    case 0x1E: fn_8038A198(self, 4); return;
    case 0x1F: fn_8038A198(self, 5); return;
    case 0x20: fn_8038A198(self, 6); return;
    case 0x21: fn_8038A198(self, 7); return;
    case 0x22: fn_8038A7B8(self, 0xA, 0); return;
    case 0x23: fn_8038A7B8(self, 0xA, 1); return;
    case 0x24: fn_80389C50(self); return;
    case 0x25: fn_80389C50(self); return;
    case 0x26: fn_80389C50(self); return;
    case 0x27: fn_8038995C(self, 1); return;
    case 0x28: fn_80389C50(self); return;
    case 0x29: fn_80389C50(self); return;
    case 0x2A: fn_80389C50(self); return;
    case 0x2B: fn_8038A7B8(self, 0xB, 0); return;
    case 0x2C: fn_8038A7B8(self, 0xB, 1); return;
    case 0x2D: fn_8038A7B8(self, 0xC, 0); return;
    case 0x2E: fn_8038A7B8(self, 0xC, 1); return;
    case 0x2F: fn_8038A7B8(self, 0xD, 0); return;
    case 0x30: fn_8038A7B8(self, 0xD, 1); return;
    case 0x31: fn_8038A7B8(self, 0xE, 0); return;
    case 0x32: fn_8038A7B8(self, 0xE, 1); return;
    case 0x33: fn_8038A7B8(self, 0xF, 0); return;
    case 0x34: fn_8038A7B8(self, 0xF, 1); return;
    case 0x35: fn_8038A7B8(self, 0x10, 0); return;
    case 0x36: fn_8038A7B8(self, 0x10, 1); return;
    case 0x37: fn_8038A7B8(self, 0x11, 0); return;
    case 0x38: fn_8038A7B8(self, 0x11, 1); return;
    case 0x39: fn_8038A7B8(self, 0x12, 0); return;
    case 0x3A: fn_8038A7B8(self, 0x12, 1); return;
    case 0x3B: fn_8038A7B8(self, 0x13, 0); return;
    case 0x3C: fn_8038A7B8(self, 0x13, 1); return;
    case 0x3D: fn_8038A7B8(self, 0x14, 0); return;
    case 0x3E: fn_8038A7B8(self, 0x14, 1); return;
    case 0x3F: fn_8038A7B8(self, 0x15, 0); return;
    case 0x40: fn_8038A7B8(self, 0x15, 1); return;
    case 0x41: fn_8038A7B8(self, 0x16, 0); return;
    case 0x42: fn_8038A7B8(self, 0x16, 1); return;
    case 0x43: fn_8038995C(self, 2); return;
    case 0x44: fn_8038A7B8(self, 0x17, 0); return;
    case 0x45: fn_8038A7B8(self, 0x17, 1); return;
    }
}

/* =================================================================================================
 * 0x8038CB0C
 * ================================================================================================= */
extern "C" void fn_8038CB0C(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 2, 0, 0);
        fn_80146058(self, lbl_8079C1F8, lbl_8079C1A8, lbl_8079C1FC);
        fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        return;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038CBB0
 * ================================================================================================= */
extern "C" void fn_8038CBB0(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: fn_8038C124(self); return;
    case 1: fn_8038CB0C(self); return;
    }
}

/* =================================================================================================
 * 0x8038CBD4 - dispatch by `action`
 * ================================================================================================= */
extern "C" void fn_8038CBD4(_ENEMY_WORK* self)
{
    switch (self->action) {
    case 0: fn_803874C8(self); return;
    case 1: fn_80388144(self); return;
    case 2: fn_80388B98(self); return;
    case 6: fn_80389634(self); return;
    case 7: fn_8038BA34(self); return;
    case 10: fn_8038BD28(self); return;
    case 11: fn_8038C080(self); return;
    case 12: fn_8038C110(self); return;
    case 13: fn_8038CBB0(self); return;
    }
}

/* =================================================================================================
 * 0x8038CC20
 * ================================================================================================= */
extern "C" void fn_8038CC20(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    if (fn_8012EC60(self) == 1 && (system_w.field_0x0c & 0x1F) == 0) {
        setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C000);
        fn_8010562C(self, 0x18, 0x14, &v, lbl_8079C00C);
    }
}

/* =================================================================================================
 * 0x8038A198
 * ================================================================================================= */
extern "C" void fn_8038A198(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v;
    f32 f;
    f32 g;
    f32 h;
    u16 ang;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x16, 6, 0);
        fn_8012F8C8(self, lbl_8079C124);
        fn_80130CDC(self, -0xA);
        if ((u32)(a - 3) > 2) {
            switch (a) {
            default: self->state_0x007 = 0; break;
            case 1: case 6: self->state_0x007 = 1; break;
            case 2: case 7: self->state_0x007 = 2; break;
            }
        } else {
            self->state_0x007 = self->bits_0x1EC % 3;
        }
        ang = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (ang >= 0x8000) {
            ang = 0x10000 - ang;
        }
        self->timer_0x020 = (s16)ang;
        if ((s16)ang > 0x3000) {
            self->timer_0x020 = 0x3000;
        }
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_8079C000, lbl_8079BFF8) == 1) {
            fn_8012933C(self, 0, 4, 0xA);
        }
        if (em_frame_check(self, 0, lbl_8079C128, lbl_8079BFF8) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079C12C, lbl_8079BFF8) == 1) {
            fn_8012933C(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, lbl_8079C130, lbl_8079BFF8) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079C134, lbl_8079BFF8) == 1) {
            fn_8012933C(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, lbl_8079C138, lbl_8079BFF8) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079C13C, lbl_8079BFF8) == 1) {
            fn_8012933C(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, lbl_8079C140, lbl_8079BFF8) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079C128, lbl_8079BFF8) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0D4, lbl_8079C060);
            mulVecMatAddTrans(&v, &mtx);
            shell_set_func_ptr->method_0x18(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA, shell_set_func_ptr);
            fn_80136B50(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, lbl_8079C130, lbl_8079BFF8) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0D4, lbl_8079C060);
            mulVecMatAddTrans(&v, &mtx);
            shell_set_func_ptr->method_0x18(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA, shell_set_func_ptr);
            fn_80136B50(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, lbl_8079C138, lbl_8079BFF8) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0D4, lbl_8079C060);
            mulVecMatAddTrans(&v, &mtx);
            shell_set_func_ptr->method_0x18(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA, shell_set_func_ptr);
            fn_80136B50(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, lbl_8079C140, lbl_8079BFF8) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, lbl_8079BFF8, lbl_8079C0D4, lbl_8079C060);
            mulVecMatAddTrans(&v, &mtx);
            shell_set_func_ptr->method_0x18(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA, shell_set_func_ptr);
            fn_80136B50(self, 0x15, 9);
        }
        switch (a) {
        default:
            if (em_frame_check(self, 1, lbl_8079C144, lbl_8079C148) == 1) {
                f = lbl_8079C03C * (f32)self->timer_0x020;
                g = f / lbl_8079C07C;
                h = (lbl_8079C07C * ((g / lbl_8079C14C) * fn_8012F8E4(self))) / lbl_8079C03C;
                fn_80133C50(self, (u16)(s32)(lbl_8079C078 + h));
            }
            break;
        case 4: case 6:
            fn_80133E3C(self, 0x3000, lbl_8079C144, lbl_8079C148);
            break;
        case 5: case 7:
            fn_80133E3C(self, -0x4000, lbl_8079C144, lbl_8079C148);
            break;
        }
        switch (self->state_0x007) {
        case 1: fn_80133E3C(self, 0x3000, lbl_8079C014, lbl_8079C150); break;
        case 2: fn_80133E3C(self, -0x4000, lbl_8079C014, lbl_8079C150); break;
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038C124
 * ================================================================================================= */
extern "C" void fn_8038C124(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    self->timer_0x020++;
    fn_80131D84(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 1, 0, 0);
        fn_8014616C(self, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        if (fn_80146008(0x8C) == 1) {
            self->state++;
            fn_8014619C(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0x17, 0, 0xA4);
            fn_801461A8(self, (s16)self->timer_0x020, lbl_805F0570, (void*)0);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C198, lbl_8079BFF8);
        }
        return;
    case 2:
        if (em_frame_check(self, 1, lbl_8079C098, lbl_8079BFF8) == 1 && (fn_80145FE4() & 3) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C028);
            fn_80304508(self, 0x44, 1, &v, lbl_8079C00C);
        }
        fn_801461A8(self, (s16)self->timer_0x020, lbl_805F0570, (void*)0);
        if (fn_80146008(0xF2) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C19C, lbl_8079BFF8);
            fn_801462A4(self, (s16)self->timer_0x020, lbl_805F0620, lbl_805F0790, 7, 2);
        }
        return;
    case 3:
        if (em_frame_check(self, 1, lbl_8079C098, lbl_8079BFF8) == 1 && (fn_80145FE4() & 3) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C028);
            fn_80304508(self, 0x44, 1, &v, lbl_8079C00C);
        }
        if (fn_80145FE4() == 0x8F) {
            fn_801048B4(self, 1, 6, 0, lbl_8079C00C);
        }
        if (fn_80145FE4() == 0xC1 || fn_80145FE4() == 0xE5) {
            copyVec3(&v, &self->pos);
            v.y = lbl_8079C09C + self->field_0x20C;
            fn_8010D2B0(&v, self->area_no, 1, 0, lbl_8079C1A0);
        }
        fn_801462A4(self, (s16)self->timer_0x020, lbl_805F0620, lbl_805F0790, 7, 2);
        if (fn_80146008(0x1DE) == 1) {
            self->state++;
            fn_8012F8C8(self, lbl_8079C0F4);
        }
        return;
    case 4:
        if (em_frame_check(self, 1, lbl_8079C098, lbl_8079BFF8) == 1 && (fn_80145FE4() & 3) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C028);
            fn_80304508(self, 0x44, 1, &v, lbl_8079C00C);
        }
        fn_801462A4(self, (s16)self->timer_0x020, lbl_805F0620, lbl_805F0790, 7, 2);
        if (fn_80146008(0x26C) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            fn_8012F5B8(self, 0x17, 0, 0xD6);
            fn_80146058(self, lbl_8079C1A4, lbl_8079C1A8, lbl_8079C1AC);
            fn_801462A4(self, (s16)self->timer_0x020, lbl_805F07B8, NULL, 5, 0);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1B0, lbl_8079BFF8);
        }
        return;
    case 5:
        if (em_frame_check(self, 1, lbl_8079C1B4, lbl_8079C1B8) == 1 && (fn_80145FE4() & 3) == 0) {
            setVector3(&v, lbl_8079BFF8, lbl_8079BFF8, lbl_8079C028);
            fn_80304508(self, 0x44, 1, &v, lbl_8079C00C);
        }
        fn_801462A4(self, (s16)self->timer_0x020, lbl_805F07B8, NULL, 5, 0);
        if (fn_80146008(0x280) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x18, 0xA, 0xA);
        }
        return;
    case 6:
        if (em_frame_check(self, 0, lbl_8079BFFC, lbl_8079BFF8) == 1) {
            get_joint_wpos_em(self, 3, &v);
            v.y = lbl_8079C09C + self->field_0x20C;
            fn_8010D2B0(&v, self->area_no, 4, 0, lbl_8079C00C);
        }
        if (em_frame_check(self, 1, lbl_8079BFFC, lbl_8079C0B8) == 1 && (fn_80145FE4() % 3) == 0) {
            fn_80304508(self, 0x45, 0x23, NULL, lbl_8079C00C);
        }
        if (em_frame_check(self, 1, lbl_8079C144, lbl_8079C038) == 1 && (fn_80145FE4() % 3) == 1) {
            fn_80304508(self, 0x45, 0x1C, NULL, lbl_8079C00C);
        }
        if (em_frame_check(self, 1, lbl_8079C0BC, lbl_8079C0C0) == 1 && (fn_80145FE4() % 3) == 0) {
            fn_80304508(self, 0x45, 0x23, NULL, lbl_8079C00C);
        }
        fn_801462A4(self, (s16)self->timer_0x020, lbl_805F07B8, NULL, 5, 0);
        if (fn_80146008(0x2BE) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x18, 0, 0x48);
            fn_80146058(self, lbl_8079C1BC, lbl_8079C1C0, lbl_8079C1C4);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1C8, lbl_8079BFF8);
        }
        return;
    case 7:
        if (em_frame_check(self, 1, lbl_8079C0BC, lbl_8079C0C0) == 1 && (fn_80145FE4() % 3) == 0) {
            fn_80304508(self, 0x45, 0x23, NULL, lbl_8079C00C);
        }
        if (em_frame_check(self, 0, lbl_8079C0BC, lbl_8079BFF8) == 1) {
            get_joint_wpos_em(self, 3, &v);
            v.y = lbl_8079C09C + self->field_0x20C;
            fn_8010D2B0(&v, self->area_no, 4, 0, lbl_8079C00C);
        }
        if (fn_80146008(0x34C) == 1) {
            self->state++;
            fn_8012F5B8(self, 2, 6, 0);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1C8, lbl_8079BFF8);
        }
        return;
    case 8:
        if (fn_80146008(0x428) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x12, 0, 0);
            fn_80146058(self, lbl_8079C1CC, lbl_8079C1C0, lbl_8079C1D0);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        }
        return;
    case 9:
        if (fn_80146008(0x4B6) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x12, 0, 0x6E);
            fn_80146058(self, lbl_8079C1D8, lbl_8079C1A8, lbl_8079C1DC);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        }
        return;
    case 10:
        if (fn_80146008(0x4F2) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x12, 0, 0xA0);
            fn_80146058(self, lbl_8079C1E0, lbl_8079C1A8, lbl_8079C1E4);
        }
        return;
    case 11:
        if (fn_80146008(0x526) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x12, 0, 0xA6);
            fn_80146058(self, lbl_8079C1E8, lbl_8079C1A8, lbl_8079C1EC);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        }
        return;
    case 12:
        if (fn_80146008(0x5CE) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x12, 0, 0x164);
            fn_80146058(self, lbl_8079C1F0, lbl_8079C1A8, lbl_8079C1F4);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        }
        return;
    case 13:
        if (fn_80146008(0x624) == 1) {
            self->state++;
            fn_8012F5B8(self, 2, 6, 0);
            fn_80146058(self, lbl_8079C1F8, lbl_8079C1A8, lbl_8079C1FC);
            fn_8014610C(self, lbl_8079BFF8, lbl_8079C1D4, lbl_8079BFF8);
        }
        return;
    }
}

/* =================================================================================================
 * 0x8038CCA4
 * ================================================================================================= */
extern "C" void fn_8038CCA4(_ENEMY_WORK* self, u8 a, u8 b, u32 c, s32 d, f32 scale)
{
    nw4r::math::VEC3 v;
    u8 kind;

    kind = b;
    VEC3_ctor(&v);
    switch (a) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (kind) {
            case 0: kind = 0xD; break;
            case 2: kind = 0xC; break;
            case 4: kind = 0x13; break;
            default: return;
            }
            if (c == 0xFF) {
                v.x = self->pos.x;
                v.y = lbl_8079C09C + self->field_0x210;
                v.z = self->pos.z;
            }
        } else {
            if ((u32)(kind - 0xC) > 0xD) {
                if (kind == 0x26) return;
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = lbl_8079C09C + self->field_0x20C;
                    v.z = self->pos.z;
                }
            } else {
                return;
            }
        }
        if (c == 0xFF) {
            eft009_set_pos(kind, &v, (struct _CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            fn_801048B4(self, c, kind, d, scale);
        }
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            switch (kind) {
            case 0: case 5:
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = lbl_8079C09C + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(0x11, &v, (struct _CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, c, 0x11, d, scale);
                }
                return;
            case 1:
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = lbl_8079C09C + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(0x10, &v, (struct _CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    fn_801048B4(self, c, 0x10, d, scale);
                }
                return;
            default: return;
            }
        }
        if (c == 0xFF) {
            copyVec3(&v, &self->pos);
        } else {
            get_joint_wpos_em(self, c, &v);
        }
        v.y = self->field_0x20C;
        fn_8010D2B0(&v, self->area_no, kind, d, scale * get_em_chg_scale(self));
        return;
    case 2:
        if (c == 0xFF) {
            copyVec3(&v, &self->pos);
        } else {
            get_joint_wpos_em(self, c, &v);
        }
        v.y = self->field_0x20C;
        fn_80106694(self, &v, kind, scale * get_em_chg_scale(self));
        return;
    }
}
