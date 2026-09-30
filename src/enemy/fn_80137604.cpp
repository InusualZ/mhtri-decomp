/* enemy/fn_80137604.cpp - an enemy's per-motion action/rotation update set, `.text` 0x80137604..0x80138074.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80137604 0x80138074`: all 20 symbols are bare
 * `fn_XXXXXXXX = .text:0x...` rows with no real name).
 *
 * What it is.  One enemy's per-motion update block, in three parts that share `_ENEMY_WORK`:
 *   * 0x80137604..0x801376B4 - the small state accessors: two field setters, the action/sub-state test
 *     (`fn_80137614`), the `field_0x43F` flag pair (`fn_8013763C`/`fn_80137648`), the sound/effect kick
 *     (`fn_8013765C`) and the `flags_0x8B3` bit helpers (`fn_801376B4`/`BC`/`DC`/`04`).
 *   * 0x80137720..0x801378A0 - the motion-mode hook (`fn_80137720`, clears or arms the motion timer
 *     through `fn_80126494`/`fn_801376B4`), the move-work slot picker (`fn_801377D0`) and the light
 *     table install (`fn_801378A0`).
 *   * 0x8013791C..0x80138074 - the per-frame driver `fn_8013791C` (an outer `field_0x004` phase switch
 *     around an inner `state_0x017` action switch, with the action-end block that clears the record's
 *     slots), its init (`fn_80137C20`), the action callbacks (`fn_80137C94`/`fn_80137C9C`), the angle
 *     wrap guard (`fn_80137DD0`), the slot-angle average (`fn_80137EE0`) and the `fn_80133DB0` angle
 *     step (`fn_80138024`).  `fn_80137C9C` takes the caller's callback in r4, so it is the seam that
 *     pulls the block into one unit.
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string is reachable from this range:
 * every `lis`/`addi`/SDA21 operand in the region resolves to the `.sdata2` float pool
 * (`lbl_80796C58`..`lbl_80796D3C`) or the `.bss` light table (`lbl_806A4560`), never to a bare
 * source-file name.  2. `python tools/symbols/dumpmap.py lookup` answers `zz_XXXXXXXX_` for 17 of the
 * 20, and the three "real" names it does carry (0x80137604/0x8013760C
 * `J3DColorBlockLightOff::setColorChanNum`, 0x8013763C `GoalOverlay::SceneCreated`) are dump noise:
 * each of those class names repeats at 90-odd unrelated addresses across the DOL, and each is
 * contradicted by the code here (an 8-byte `stb r4,13(r3)` is not a `J3DColorBlock` member, and a
 * `GoalOverlay::SceneCreated` does not write a `_ENEMY_WORK`'s +0x43F).  3. The code is enemy-band: it
 * defines no mangled symbol but calls `em_act_ck(_ENEMY_WORK*, u8, u8)`, `get_enemy_data(_ENEMY_WORK*)`
 * and `get_move_work_adrs`/`get_move_work_max`, and both bracketing registered units are `enemy`
 * (`enemy/fn_8012BDF4.cpp` below, `enemy/fn_80138074.c` above); the neighbours' scheme is the map's own
 * stem (`enemy/fn_8012BA00.c`, `enemy/fn_80138074.c`).  The file therefore keeps the map stem (brief
 * option 4); no name was invented.  Module `enemy`, which is what the brief's registration step needs.
 *
 * Language.  The unit calls mangled callees whose declarations must be the real signatures
 * (`em_act_ck__FP11_ENEMY_WORKUcUc`, `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`, `ran_suu__Fl`, ...) and
 * rule 9 forbids spelling a mangling as the callable identifier, so the file is C++ (docs/plan.md 6.5
 * rule 9; the same finding `enemy/fn_80177890.cpp` records).  Every definition keeps the map's plain
 * `fn_XXXXXXXX` name, so they are all `extern "C"`; the mangled callees are declared at C++ scope
 * before the linkage block, so this front-end mangles them back to the map's spelling.
 *
 * Types.  `_ENEMY_WORK` is the shared 0xB18 record in `include/enemy/ENEMY_WORK.h` (rule 1).  The
 * offsets these twenty functions read are named there (`field_0x00D`/`0x00E`/`0x012`/`0x016`/`018`,
 * `state_0x017`, `field_0x018`/`01A`, `field_0x1BC`/`1C0`/`1C4`/`1C8`/`1D0`/`1DE`/`1E7`,
 * `field_0x218`, `slots_0x244` and its `EmMotionSlot` records, `field_0x43F`, `field_0x7C8`..`0x94C`,
 * `field_0xAEE`); the bytes between them stay `unused_0xNN` padding so every measured offset of the
 * units that already include the header keeps its place.  `EnemyData`'s definition lives in
 * `include/enemy.h`, which defines `_ENEMY_WORK` a second time and so cannot be included beside
 * `enemy/ENEMY_WORK.h` (rule 1), so `fn_80137C20` reads the one word it needs with the owner's own
 * declaration.
 *
 * Rule 2 note.  The callees this unit calls are declared in its linkage block because their owning
 * units' headers do not carry them yet - the interim home `enemy/fn_80177890.cpp` uses for the same
 * band.  `fn_80130778` and `fn_80144584` are the two whose *spelling* differs between consumers (one
 * argument in `enemy/fn_8012BDF4.cpp`, two here); this unit writes the two-argument form its own call
 * sites show, and the reconciliation belongs to whichever unit registers their range.
 *
 * Sections.  Besides `.text` the unit owns `extab` 0x8000D284..0x8000D2D4, `extabindex`
 * 0x80027AB0..0x80027B28 and one `.ctors` word at 0x8056F318 (the target object's `.ctors` is
 * `R_PPC_ADDR32 fn_801378A0`, i.e. the light-table install is the TU's static initializer); the split
 * attributed all three when the unit was registered, and `extab`/`extabindex` come out byte-identical.
 * The `.ctors` word is not emitted by this file: no symbol name exists for it in the target object, the
 * report scores no unit's `.ctors` (85 units carry one, none with a percentage), and the linker takes
 * the word from the split object either way.
 *
 * Residuals.  Nineteen of the twenty bodies are byte-identical (100.00 % on the official report
 * metric); `fn_80137EE0` measures 89.07 %.  The residual is the *position* of one hoisted instruction:
 * the target keeps the narrowed `kind` parameter in a callee-saved register and emits the
 * `clrlwi r30,r4,24` at the end of the loop's preheader, while this build emits the same mask at the
 * top of the block, so its `cmplwi r30,1` pairs with `lwz r4,580(r29)` one instruction later.  Both
 * objects are 81 instructions with the same registers and branches; `u8 kind_lo = kind` (the ABI lets
 * the front-end skip the mask) and `u32 kind_lo = (u8)kind` (the form kept here) were measured, and
 * only the second reproduces the mask at all.  The unit's own object also reproduces the target's
 * `.text` (0xA70), `extab` (0x50) and `extabindex` (0x78) byte for byte.
 *
 * Source shape worth keeping: the unit needs `#pragma peephole off`.  Retail keeps the unfused
 * `clrlwi`/`rlwinm`/`and` + `cmpwi` pairs these bit tests are built from; with the peephole pass on the
 * fold fuses them into `and.`/`rlwinm.` and moves the branch.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/pRoot.h"

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ *
 * pooled data owned by other units: declared, never defined (playbook 29), so the load operands pair
 * with the target's pool relocations
 * ------------------------------------------------------------------------------------------------ */

extern f32 lbl_80796C58;
extern f32 lbl_80796D34;
extern f32 lbl_80796D38;
extern f32 lbl_80796D3C;

/* The four 0x0C-byte colour records `setVec3` rebuilds; the table's bytes belong to the data pass. */
extern nw4r::math::VEC3 lbl_806A4560[];

/* The scene root `g3d_root_model_bind` looks a model up in. */

/* ------------------------------------------------------------------------------------------------ *
 * the callees
 *
 * The three the map carries mangled are declared at C++ scope with the signature the mangling encodes
 * (`tools/units/mangle.py` proves each one), so no call site spells a mangling (rule 9).
 * ------------------------------------------------------------------------------------------------ */

/* `get_enemy_data__FP11_ENEMY_WORK` (owner `enemy/fn_801251D0.cpp`).  Only the record's leading flag
 * word is read here, and `EnemyData`'s definition lives in `include/enemy.h`, which cannot be included
 * beside `enemy/ENEMY_WORK.h` - both define `_ENEMY_WORK` - so the word is read at the offset the
 * target loads (`lwz r0,0(r3)`). */
struct EnemyData;
struct EnemyData* get_enemy_data(_ENEMY_WORK* work);

/* `em_act_ck__FP11_ENEMY_WORKUcUc` (owner `enemy/fn_8012BDF4.cpp`). */
u32 em_act_ck(_ENEMY_WORK* work, u8 action, u8 arg);

/* `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);

/* `get_move_work_adrs__FUc` / `get_move_work_max__FUc` (owner `ef/fn_800CDB2C.cpp`). */
void* get_move_work_adrs(u8 area);
u16 get_move_work_max(u8 area);

/* `ran_suu__Fl` (owner `ef/fn_800CDB2C.cpp`). */
s32 ran_suu(s32 range);

/* `fn_800E0914`'s parameter is a model base, not a `_ENEMY_WORK`; the record embeds one at +0x24. */
struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------
 * The plain `fn_XXXXXXXX` callees.  The map spells them as C symbols, so the whole set - the map's own
 * definitions included - is `extern "C"`: without it this C++ front-end would mangle them
 * (`fn_801376BC__FP11_ENEMY_WORK`) and objdiff would pair nothing (docs/matching.md row 42).
 * Their owning units' headers do not carry them yet, so they are declared here (rule 2's interim home
 * for the enemy band, the same one `enemy/fn_80177890.cpp` uses).
 * ------------------------------------------------------------------------------------------------ */

/* this unit's own entry points, used before their definitions below */
void fn_801376B4(_ENEMY_WORK* self);
void fn_801376BC(_ENEMY_WORK* self, u8 flag);
void fn_801376DC(_ENEMY_WORK* self, u8 flag);
void fn_80137C94(_ENEMY_WORK* self);

/* `enemy/fn_801251D0.cpp` */
void fn_8012555C(_ENEMY_WORK* self, u32 kind);
u32 fn_801260BC(_ENEMY_WORK* self);
u32 fn_801260E0(_ENEMY_WORK* self);
s16* fn_80126494(_ENEMY_WORK* self);
s16 fn_80126844(_ENEMY_WORK* self);
void fn_80127D94(_ENEMY_WORK* self);
void fn_8012820C(_ENEMY_WORK* self);
void fn_80128A30(_ENEMY_WORK* self, u32 a, u32 b);

/* `enemy/fn_8012BDF4.cpp` */
u32 fn_8012D1A0(_ENEMY_WORK* self);
void fn_8012E728(_ENEMY_WORK* self, u32 mode);

/* `enemy/fn_80138074.c` */
void fn_8013823C(_ENEMY_WORK* self);
void fn_80138E18(_ENEMY_WORK* self);
void fn_80138E28(_ENEMY_WORK* self);
void fn_80139024(_ENEMY_WORK* self);
void fn_8013A978(_ENEMY_WORK* self);

/* `mh3_pad.cpp`: rebuilds one 0x0C-byte record from three floats. */

/* `g3d/g3d_scnmdl.cpp`: finds a scene model by id. */
void g3d_root_model_bind(s32 root, u32 id);

/* `ef/fn_800CDB2C.cpp` */
u32 my_player_no(void);

/* `sound/fn_800DD1F0.cpp`: refreshes the model base at `_ENEMY_WORK::char_0x024`. */
void fn_800E0914(struct MHchar* model);

/* the enemy band's still-unregistered helpers (the 0x8011xxxx/0x8012xxxx/0x8013xxxx proposals) */
void fn_8011E5EC(_ENEMY_WORK* self);
void fn_8011E960(_ENEMY_WORK* self);
u32 fn_80130778(_ENEMY_WORK* self, s32 kind);
void fn_80130844(_ENEMY_WORK* self);
void fn_80130B28(_ENEMY_WORK* self);
void fn_80131DB4(_ENEMY_WORK* self);
void fn_80131DF4(_ENEMY_WORK* self);
void fn_80132064(_ENEMY_WORK* self);
void fn_801322B4(_ENEMY_WORK* self, u32 kind);
void fn_80133BC0(_ENEMY_WORK* self);
u32 fn_80133C48(void);
u16 fn_80133DB0(u16 a, u16 b, u16 c);
u8 fn_8014278C(u16 a, u16 b);
f32 fn_80142958(u16 a, u16 b);
void fn_80144584(_ENEMY_WORK* self, s32 mode);

/* the 0x803xxxxx helpers the band shares */
void fn_8033737C(_ENEMY_WORK* self, u32 a, u32 b);
void lb_sub0e_send(u8 a, u16 b, u8 c, u16 d);

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------------------------------ *
 * 0x80137604..0x801376B4 - the state accessors
 * ------------------------------------------------------------------------------------------------ */

/* Stores the record's byte +0x0D. */
extern "C" void fn_80137604(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* The second entry point that stores the same byte. */
extern "C" void fn_8013760C(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* Whether the record is on action 11 with the `fn_80137614` sub-state set. */
extern "C" u32 fn_80137614(_ENEMY_WORK* self)
{
    if (self->action == 11 && self->field_0x8D3 == 1) {
        return 1;
    }
    return 0;
}

/* Sets the field `fn_80137648` reads back. */
extern "C" void fn_8013763C(_ENEMY_WORK* self)
{
    self->field_0x43F = 1;
}

/* Whether the `fn_8013763C` field is set. */
extern "C" u32 fn_80137648(_ENEMY_WORK* self)
{
    return self->field_0x43F == 1;
}

/* Kicks `lb_sub0e_send` for the record when `fn_8012D1A0` says the action is armed. */
extern "C" void fn_8013765C(_ENEMY_WORK* self, u32 arg1)
{
    if (fn_8012D1A0(self) == 1) {
        lb_sub0e_send(my_player_no(), arg1, 1, self->field_0x01A);
    }
}

/* Sets the `flags_0x8B3` bit 1. */
extern "C" void fn_801376B4(_ENEMY_WORK* self)
{
    fn_801376DC(self, 1);
}

/* Adds `flag` to the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376BC(_ENEMY_WORK* self, u8 flag)
{
    if ((self->flags_0x8B3 & flag) == 0) {
        self->flags_0x8B3 |= flag;
    }
}

/* Removes `flag` from the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376DC(_ENEMY_WORK* self, u8 flag)
{
    if (self->flags_0x8B3 & flag) {
        self->flags_0x8B3 &= ~flag;
    }
}

/* Whether `flag` is set in the record's `flags_0x8B3` bitmap. */
extern "C" u32 fn_80137704(_ENEMY_WORK* self, u8 flag)
{
    return (self->flags_0x8B3 & flag) != 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x80137720..0x801378A0 - the motion-mode hook, the move-work picker and the light table
 * ------------------------------------------------------------------------------------------------ */

/* Latches the record's motion mode and, for mode 2, arms the timer `fn_80126494` returns. */
extern "C" void fn_80137720(_ENEMY_WORK* self, u8 mode)
{
    if ((self->field_0x1C8 & 1) == 0 || (self->field_0x1C8 & 0x80000) != 0) {
        self->field_0x89F = 0;
    }
    self->field_0x89F = mode;
    switch (mode) {
    case 2: {
        s16* timer = fn_80126494(self);

        fn_801376BC(self, 1);
        if (timer != NULL) {
            self->field_0x8A4 = *timer;
        } else {
            self->field_0x8A4 = 3600;
        }
        break;
    }
    case 0:
        fn_801376B4(self);
        break;
    }
}

/* The move-work record `mode` picks: the slot holding the first bit of `mask` when `mode` is 10, the
 * slot `mask` names otherwise. */
extern "C" u8* fn_801377D0(u8 mode, u8 mask)
{
    u8* work = (u8*)get_move_work_adrs(2);

    if (mask != 0xFF) {
        u16 max = get_move_work_max(2);
        u8 slot;

        if (mode == 10) {
            u8 i;

            slot = 0xFF;
            for (i = 0; i < max; i++) {
                if (mask & (1 << i)) {
                    slot = i;
                    break;
                }
            }
        } else {
            slot = mask;
        }
        if (slot < max) {
            work += slot * 2848;
        }
    }
    return work;
}

/* Rebuilds the record's four light records. */
extern "C" void fn_801378A0(void)
{
    setVec3(&lbl_806A4560[0], lbl_80796D34, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[1], lbl_80796C58, lbl_80796D38, lbl_80796D3C);
    setVec3(&lbl_806A4560[2], lbl_80796D3C, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[3], lbl_80796C58, lbl_80796D38, lbl_80796D34);
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8013791C..0x80138074 - the per-frame driver and its callbacks
 * ------------------------------------------------------------------------------------------------ */

/* One frame of the record's action state machine. */
extern "C" void fn_8013791C(_ENEMY_WORK* self)
{
    self->field_0x1DE = 0;
    switch (self->field_0x004) {
    case 0:
        self->field_0x004++;
        fn_80137C94(self);
        fn_80131DB4(self);
        fn_80131DF4(self);
        fn_800E0914((struct MHchar*)&self->char_0x024);
        break;

    case 1:
        if (self->field_0xAEE != 0) {
            if ((self->field_0x1C8 & 8) != 0 && (self->field_0x1C8 & 1) != 0) {
                fn_8033737C(self, 3, 0);
            }
            self->field_0xAEE = 0;
        }
        fn_8013823C(self);
        {
        u32 refresh = 0;

        switch (self->state_0x017) {
        case 0:
            if (self->field_0x011 == 2) {
                fn_8012E728(self, 1);
            } else {
                self->field_0x1DE = 1;
            }
            break;

        case 1:
            self->field_0x016++;
            self->field_0x012 = 0;
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
                rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
                fn_8033737C(self, 4, 0);
            } else {
                fn_80144584(self, 2);
            }
            fn_8012555C(self, 1);
            refresh = 1;
            break;

        case 2:
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                self->field_0x012++;
                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
                rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
                fn_8033737C(self, 4, 0);
                fn_8012555C(self, 1);
                refresh = 1;
            }
            break;

        case 3:
            fn_8012E728(self, 0);
            break;

        case 4:
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
            rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
            }
            fn_8033737C(self, 4, 1);
            fn_8012E728(self, 0);
            break;

        case 5:
            fn_8012E728(self, 2);
            break;
        }

        if (refresh == 1) {
            fn_80131DB4(self);
            fn_80131DF4(self);
            fn_800E0914((struct MHchar*)&self->char_0x024);
        }
        if (self->model_flag_0x058 != 0) {
            g3d_root_model_bind(pRoot, self->field_0x13C);
        }
        }
        break;

    case 2:
        fn_80138E18(self);
        break;

    case 3:
        fn_80138E28(self);
        break;
    }
}

/* Latches the record's static data and reseats the helpers `fn_8013791C` drives. */
extern "C" void fn_80137C20(_ENEMY_WORK* self)
{
    self->field_0x1C8 = *(u32*)get_enemy_data(self);
    self->field_0x8C8 = fn_801260BC(self);
    self->field_0x8CC = fn_801260E0(self);
    fn_8011E5EC(self);
    fn_80127D94(self);
    fn_80132064(self);
    fn_8011E960(self);
    fn_8013A978(self);
}

/* Hands the record's `field_0x00E` to `fn_8012555C`. */
extern "C" void fn_80137C94(_ENEMY_WORK* self)
{
    fn_8012555C(self, self->field_0x00E);
}

/* The action callback `fn_8013791C` runs each frame: the `fn_80139024` step, or the caller's own
 * callback while `em_act_ck` has not armed the action. */
extern "C" s32 fn_80137C9C(_ENEMY_WORK* self, void (*callback)(_ENEMY_WORK*))
{
    if (self->field_0x1E9 == 0) {
    if (em_act_ck(self, 12, 255) == 1) {
        fn_80139024(self);
    } else {
        callback(self);
    }
    if (self->field_0x011 == 2) {
        if ((self->field_0x1C8 & 8) != 0) {
            fn_80130778(self, 0);
            fn_80130844(self);
            fn_80130B28(self);
            fn_801322B4(self, 247);
            self->field_0x916 = 0;
            self->field_0x38E = 0;
            self->field_0x38F = 0;
            self->field_0x94C = 0;
            self->field_0x812 = 0;
            self->field_0x814 = 0;
            self->field_0x94A = 0;
            self->field_0x011 = 0;
            self->field_0x1E7 = 1;
            self->field_0x018 = fn_80126844(self);
            fn_80128A30(self, 12, 255);
            fn_80133BC0(self);
            return 1;
        }
        return 0;
    }
    return (u8)(self->state_0x017 + 253) > 1;
    }
    fn_8012820C(self);
    return 1;
}

/* Wraps the record's two angle fields back into their 16-bit range. */
extern "C" void fn_80137DD0(_ENEMY_WORK* self)
{
    if ((u32)(self->field_0x1E2 - 1) <= 3) {
    if (fn_80133C48() == 0) {
        u16 angle = (u16)self->field_0x1BC;

        if (angle < 32768) {
            if (angle < 256) {
                self->field_0x1BC = 0;
            } else {
                self->field_0x1BC -= 256;
            }
        } else if (angle > 65280) {
            self->field_0x1BC = 0;
        } else {
            self->field_0x1BC += 256;
        }
    }
    if (self->field_0x784 == 0) {
        if (self->field_0x1E2 != 1 || self->field_0x1E3 != 1) {
            u16 angle = (u16)self->field_0x1C4;

            if (angle < 32768) {
                if (angle < 256) {
                    self->field_0x1C4 = 0;
                } else {
                    self->field_0x1C4 -= 256;
                }
            } else if (angle > 65280) {
                self->field_0x1C4 = 0;
            } else {
                self->field_0x1C4 += 256;
            }
        }
    }
    } else {
        self->field_0x1BC = 0;
        self->field_0x1C4 = 0;
    }
}

/* The average of the angles `calcVecAngXY` derives from the record's motion slots. */
extern "C" s32 fn_80137EE0(_ENEMY_WORK* self, u32 kind)
{
    u32 angles[10];

    if (self->field_0x218 == 0) {
        return -1;
    }
    {
        u32 kind_lo = (u8)kind;
        u32 count = 0;
        s32 i;

        for (i = 0; i < 10; i++) {
            if (self->slots_0x244[i].flags == 0) {
                continue;
            }
            if (kind_lo == 1) {
                if ((u32)(self->slots_0x244[i].value - 24576) > 16384) {
                    continue;
                }
            }
            if ((self->slots_0x244[i].flags & 0x800) == 0) {
                continue;
            }
            {
                u32 angle_x;
                u32 angle_y;

                calcVecAngXY(&self->slots_0x244[i].vec, &angle_x, &angle_y);
                angles[count] = angle_y;
                count++;
            }
        }
        if (count == 0) {
            return -1;
        }
        if (count == 1) {
            return angles[0] & 0xFFFF;
        }
        {
            s32 sum = angles[0];
            s8 n;

            for (n = 1; (u32)n < count; n++) {
                s32 delta = angles[n] - sum;

                if (delta > 32768) {
                    delta -= 65536;
                } else if (delta < -32768) {
                    delta += 65536;
                }
                sum += delta / (n + 1);
            }
            return sum & 0xFFFF;
        }
    }
}

/* Steps the record's second angle field through `fn_80133DB0`. */
extern "C" void fn_80138024(_ENEMY_WORK* self, u32 a, u32 b)
{
    self->field_0x1C0 = fn_80133DB0((u16)(b + 0x8000), (u16)self->field_0x1C0, (u16)a);
}
