/*
 * lobby/lb_npc.cpp - the lobby NPC group: the NPC tables and motion lookups, the NPC work state machines, the NPC/character
 * action layer and the player-character control band.
 *
 * `.text` 0x801FBF78..0x80212810 (389 functions), `.bss` 0x2C00 B, `.data` 0xA78 B, `.sbss` 0x58 B, `.sdata` 0x70 B, `.sdata2` 0x398 B,
 * `.ctors` 4 B, extab 0x958 B and extabindex 0xE04 B.  Phase 4 fold (docs/splits/phase4): the four registered units
 * `lobby/lb_npc`, `lobby/fn_802029B4`, `lobby/fn_802076D4` and `lobby/fn_8020C588` are one TU of the candidate (the `.sdata2`
 * pool and the `lb_npc` `.bss` tables run on across their old edges); their bodies are kept below in text order, each under
 * its own former header.
 *
 * Scopes: the four former units were written against different views of the same lobby globals (`lobby_w`,
 * `lobby_world_block`) and declare some unsplit callees with different signatures, so each section keeps its own declarations in a
 * namespace (`extern "C"` names stay unmangled; the C++-linkage callees and the types the manglings name stay at global scope,
 * where they are made).  Uniting the views into one declaration set is the open work of this unit.
 * Name: the survivor's own, `lb_npc` (the range owns `lb_npc`, `npc_data_town`, `npc_model_*`).
 * Flags: `cflags_lobby` for all four former units; the pragmas the third band used (`dont_inline on`, `peephole off`) are
 * scoped to its own section below.
 * Data: no `.data`/`.sdata`/`.sdata2` is defined here beyond what the former sources defined; the claimed ranges keep the
 * original bytes (`NonMatching`).
 */

/* ==== survivor: lobby/lb_npc.cpp (0x801FBF78..0x802027CC) ==== */
/* lobby/lb_npc.cpp - the lobby NPC / world-update group.
 *
 * `.text` 0x801FBF78..0x802029B4 (127 functions, 27196 B), extab 0x80010ADC..0x80010DBC,
 * extabindex 0x8002CF34..0x8002D384 (92 x 12 B), .ctors 0x8056F35C..0x8056F360 (one word, fn_801FF700).
 * Registered once, at its final home (docs/plan.md 12), from proposal/801FBF78_fn_801FBF78.cpp.
 *
 * Module `lobby`: the range owns the `lb_npc` NPC work array (.bss 0x806A7BA0, 0x12 x 0x268), the npc
 * data tables (`npc_data_town`, `npc_data_village`, `npc_lp_tbl`, `npc_model_*`, `npc_sub_data`,
 * `npc_event_set_data`), defines `lb_npc_Get_motion_no`, `lb_npc_area_ck` and `get_talk_npc_data_ptr`,
 * and its predecessors/successors in splits.txt are `lobby/lobby_scene.c` and `Pl/pl_master.cpp`
 * (the lobby band).  Language C++: `get_talk_npc_data_ptr__Fv` / `lb_npc_*__FP7_LB_NPC` are defined
 * mangled and the range calls `LbCheckKujiraEvent__Fv`, `get_move_work_adrs__FUc`, `ran_suu__Fl`, ...
 *
 * Name.  Nothing in the range emits a `__FILE__` string (the .rodata/.data pools were scanned for a
 * bare source-file name; none sits in this range) and `dumpmap.py lookup 0x801FBF78` answers only
 * `zz_01fbf78_`.  The file name is taken from what the code is (brief section 2, class 3): the
 * subsystem's own global is `lb_npc` and its two named functions are `lb_npc_*`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
 * on the range inventory: 124 of the 127 .text entries are bare fn_XXXXXXXX/LbCheckKujiraEvent-style
 * map placeholders and the runtime dump answers only zz_XXXXXXXX_ for them)
 *
 * State: 35 of the 127 rows are written (0x801FBF78..0x801FBF78's first 6 rows, 0x801FC268..0x801FCB68,
 * 0x801FD0F8, 0x801FD174, 0x801FDD9C..0x801FE1F8); 22 are byte-identical and 13 more are above 80 %.
 * Unit `report generate`: 13.366672 % fuzzy, 1668 / 27196 matched bytes, 22/127 matched functions.
 * The unwritten rows are the 0x801FC318..0x801FCBC8 block and 0x801FD338..0x802029B4 - they are absent,
 * not stubbed, so the next session continues at 0x801FC318 in address order.
 *
 * Shapes that earned their score:
 *   - `__declspec(noinline)` on the small helpers the target keeps out of line (`fn_801FDE3C`,
 *     `fn_801FE11C`, `fn_801FE130`, `fn_801FCAC8`): `-inline auto` (cflags_lobby) otherwise inlines
 *     them into `fn_801FDEE4`/`fn_801FE13C`/`fn_801FCA80` (91.1 / 84.4 / 69.7 -> 100.0 / 99.6 / 98.9).
 *     A scoped `#pragma inline off` does not stop MWCC's auto-inliner; `-inline noauto` is a lib flag
 *     and would move the two already-landed lobby units, so the attribute is per function.
 *   - `#pragma peephole off` scoped to `fn_801FC268`: the target keeps `clrlwi r0,r3,24` + `cmpwi`
 *     unfused (96.9 -> 100.0); every other row is unaffected.
 *   - `if (a <= 0) {clear} else {tick}` for `fn_801FDE74`: the target lays the clear block first and
 *     branches `bgt` over it (33.1 -> 100.0); the natural `> 0` order emits `ble` the other way.
 *   - a full-width id parameter with `(u16)` at the use for `fn_801FE1AC`/`fn_801FE1CC`/`fn_801FE1DC`:
 *     a `u16` parameter drops the target's `clrlwi r4,r4,16`.
 *   - `(u16)id - field == 0` for `fn_801FE1AC`: the subtraction's operand order is the whole diff (65.0 ->
 *     100.0); `==` alone emits `subf` the other way round.
 *   - the value computed before the range test for `fn_801FCB68` (75.4 -> 86.25).
 *
 * Residuals (measurements in .pi/outbox/801fbf78-fn-801fbf78-814b.json):
 *   - `fn_801FDFA4` 14.3 %: the target's first store is `rlwinm r0,r4,0,24,24` (a mask of bit 7) and the
 *     `setVector3` pointer is materialised before the constant load; `(u8)(flag & 0x80)` matches the
 *     mask but MWCC schedules the `lfs` ahead of the `addi`.
 *   - `fn_801FE1CC`/`fn_801FE1DC` 50 %, `fn_801FE1EC` 60 %: the tail call's setup order is
 *     `addi r3,r3,0x58` first in the target and `li r4/r5` first here - a scheduling delta on a 3-row row.
 *   - `fn_801FC8C8` 83.3 %: the target has `lbz; extsb; stb`; MWCC forwards the byte store and drops the
 *     `extsb` for every source spelling tried (`s8`, `u8`, an `s32` temporary).
 *   - `fn_801FC810` 91.2 %: the loop's `p += 2` / `i += 1` order - the target increments the pointer
 *     before the counter, MWCC schedules the counter first for both the indexed and the pointer loop.
 */
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "lobby/lobby_w.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
namespace s_801FBF78 {


/* ---------------------------------------------------------------------------------------------------
 * Data
 */
extern "C" LbNpcUserData* lobby_world_block;  /* .sbss pointer */
extern "C" _LB_NPC lb_npc[0x12];        /* .bss 0x806A7BA0 */
extern "C" f32 lbl_80799880;
extern "C" f32 lbl_80799884;
extern "C" f64 lbl_807998D0;
extern "C" f32 lbl_807998D8;
extern "C" LbNpcSystemWork system_w;   /* .bss 0x806585E0 */
extern "C" u8 lobby_state_block[];
extern "C" u8 lbl_806AA6F0[];
extern "C" u8 lbl_80794AB0;
extern "C" u8* lbl_80794B18;
extern "C" void* npc_data_town[];
extern "C" void* npc_data_village[];
extern "C" void* npc_lp_tbl[];
extern "C" LbResId lbl_80582A68[];

/* ---------------------------------------------------------------------------------------------------
 * Declarations: mangled map names are C++ linkage (rule 9), plain fn_/lbl_ names are C.
 */
extern "C" {
s32 fn_8004D27C(s32);
u32 fn_8004D70C(s32);
void fn_802FAFC8(u16);
u16 fn_802FB54C(s32);
u8 fn_802FB948(void);
void fn_80220078(void);
void fn_801E9AA8(u8);
s32 fn_800E11C0(void* self, u8 kind, s32 a3, u16 id, s32 a5, s32 motion, s32 a7, f32 c, f32 k);
void fn_800E16DC(void* self, u16 a, s32 b);
void fn_800E2198(void* self, s32 a);
void fn_800E1640(void* self);
s32 fn_8026A00C(u16 id);
s32 fn_802D282C(u16 id);
s32 fn_8045AB38(s32 a);
}
} /* namespace s_801FBF78 */


void wii_sysmsg_gen(s32 id, s8* buf, s32 flag);
void put_message_sys(s8* buf, u32 a, u32 b, u32 c);
void put_message_sys_ok_button(void);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
u16 lb_npc_Get_motion_no(_LB_NPC* self);
namespace s_801FBF78 {


/* ---------------------------------------------------------------------------------------------------
 * Bodies
 */
extern "C" {

/* 0x801FBF78 - the lobby "world update" state dispatcher: each state runs its own quest-flag gate.
 * `fn_802FB54C(1)` returns the current mode; the switch's 10 arms are states 0x1D..0x26. */
void fn_801FBF78(void)
{
    switch (fn_802FB54C(1)) {
    case 0x1D:
        if (fn_8004D27C(0x3C) == 0 && fn_8004D70C(0x2711) == 1 && fn_8004D70C(0x2714) == 1 &&
            fn_8004D70C(0x2715) == 1 && fn_8004D70C(0x2718) == 1 && fn_8004D70C(0x271D) == 1) {
            fn_802FAFC8(0x3C);
        }
        break;
    case 0x1E:
        if (fn_8004D70C(0x36B0) == 1) {
            fn_802FAFC8(0x3D);
        }
        break;
    case 0x1F:
        if (fn_8004D27C(0x3E) == 0 && fn_8004D70C(0x2743) == 1 && fn_8004D70C(0x274A) == 1 &&
            fn_8004D70C(0x274B) == 1 && fn_8004D70C(0x274D) == 1 && fn_8004D70C(0x2750) == 1) {
            fn_802FAFC8(0x3E);
        }
        break;
    case 0x20:
        if (fn_8004D70C(0x36B1) == 1) {
            fn_802FAFC8(0x3F);
        }
        break;
    case 0x21:
        if (fn_8004D27C(0x40) == 0 && fn_8004D70C(0x2774) == 1 && fn_8004D70C(0x2775) == 1 &&
            fn_8004D70C(0x277A) == 1 && fn_8004D70C(0x277C) == 1 && fn_8004D70C(0x2780) == 1) {
            fn_802FAFC8(0x40);
        }
        break;
    case 0x22:
        if (fn_8004D70C(0x36B2) == 1) {
            fn_802FAFC8(0x41);
        }
        break;
    case 0x23:
        if (fn_8004D27C(0x42) == 0 && fn_8004D70C(0x3A9B) == 1 && fn_8004D70C(0x3A9E) == 1 &&
            fn_8004D70C(0x3A9F) == 1 && fn_8004D70C(0x3AA1) == 1 && fn_8004D70C(0x3AA6) == 1) {
            fn_802FAFC8(0x42);
        }
        break;
    case 0x24:
        if (fn_8004D70C(0x4A38) == 1) {
            fn_802FAFC8(0x43);
        }
        break;
    case 0x25:
        if (fn_8004D27C(0x44) == 0 && fn_8004D70C(0x3ACB) == 1 && fn_8004D70C(0x3ACD) == 1 &&
            fn_8004D70C(0x3AD0) == 1 && fn_8004D70C(0x3AD4) == 1 && fn_8004D70C(0x3AD6) == 1) {
            fn_802FAFC8(0x44);
        }
        break;
    case 0x26:
        if (fn_8004D70C(0x4A39) == 1) {
            fn_802FAFC8(0x45);
        }
        break;
    }
}

/* 0x801FC268 - refresh the user-data "talk" byte from the current mode. */
#pragma peephole off
void fn_801FC268(void)
{
    switch (fn_802FB54C(0)) {
    case 0x1A:
        if (fn_802FB948() == 0) {
            lobby_world_block->field_0x3E01 = 9;
        } else {
            lobby_world_block->field_0x3E01 = 0xFF;
        }
        break;
    case 0:
        lobby_world_block->field_0x3E01 = 0;
        break;
    default:
        lobby_world_block->field_0x3E01 = 0xFF;
        break;
    }
}
#pragma peephole on

/* 0x801FC2F0 - reset the menu layer and leave the menu. */
void fn_801FC2F0(void)
{
    fn_80220078();
    fn_801E9AA8(0);
}

/* 0x801FC6A0 - show the system message indexed by *value when it is positive. */
void fn_801FC6A0(u32 unused, s32* value)
{
    s8 buf[0x108];

    if (*value > 0) {
        wii_sysmsg_gen(*value, buf, 0);
        put_message_sys(buf, 0, 0, 0);
        put_message_sys_ok_button();
    }
}

/* 0x801FC810 - seed the lobby's 13-entry random slot table. */
void fn_801FC810(void)
{
    s32 i = 0;

    do {
        lobby_w.field_0x036[i] = (s16)ran_suu(1);
        i++;
    } while (i < 13);
    lobby_w.field_0x003 = 0xFF;
}

/* 0x801FC874 - pick one of the three lobby variants at random. */
void fn_801FC874(void)
{
    s32 value = (u16)ran_suu(1);

    lobby_w.field_0x080 = (u8)(value % 3);
}

/* 0x801FC8C8 - copy the lobby's signed state byte into the shadow byte. */
void fn_801FC8C8(void)
{
    s32 value = lobby_w.field_0x002_s;

    lobby_w.field_0x079 = (s8)value;
}

}
} /* namespace s_801FBF78 */
 /* extern "C" */

/* 0x801FC8E0 - the talk-NPC data block. */
u8* get_talk_npc_data_ptr(void)
{
    return lobby_w.talk_0x0C0;
}
namespace s_801FBF78 {


extern "C" {

/* 0x801FCAC8 - copy the 8-byte sprite record `src` into `dst`. */
__declspec(noinline) void fn_801FCAC8(u32* dst, const u32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

} /* extern "C" */

/* =================================================================================================== */

/* This unit's own functions, forward-declared in address order (the bodies below call later rows). */
extern "C" {
s32 fn_801FCB68(u32 motion_id);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
void fn_801FDFE4(_LB_NPC* self, u32 motion_id, s32 a, s32 b);
void fn_801FE11C(_LB_NPC* self);
void fn_801FE130(_LB_NPC* self);
void fn_801FE1F8(_LB_NPC* self);
}

extern "C" {

/* 0x801FDD9C - aim the NPC at its target over `duration`: latch the tick count and the per-tick step. */
void fn_801FDD9C(_LB_NPC* self, f32 duration)
{
    self->field_0x1E2 = (s16)duration;
    self->vel_0x1F8.x = (self->target_0x1EC.x - self->pos_0x10.x) / duration;
    self->vel_0x1F8.y = (self->target_0x1EC.y - self->pos_0x10.y) / duration;
    self->vel_0x1F8.z = (self->target_0x1EC.z - self->pos_0x10.z) / duration;
}

/* 0x801FDDF4 - advance the timed move by one tick. */
void fn_801FDDF4(_LB_NPC* self)
{
    if (self->field_0x1E2 == 0) {
        return;
    }
    self->pos_0x10.x += self->vel_0x1F8.x;
    self->pos_0x10.y += self->vel_0x1F8.y;
    self->pos_0x10.z += self->vel_0x1F8.z;
    self->field_0x1E2--;
}

/* 0x801FDE3C - the NPC's heading minus the stored reference angle. */
__declspec(noinline) s32 fn_801FDE3C(_LB_NPC* self, VEC3* target)
{
    return calcVecAng2(&self->pos_0x10, target) - self->field_0x02C;
}

/* 0x801FDE74 - tick the scripted turn; when it runs out clear the turn flags. */
void fn_801FDE74(_LB_NPC* self)
{
    if (self->field_0x1E4 <= 0) {
        self->field_0x1E4 = 0;
        self->field_0x229 = 0;
        self->field_0x228 = 0;
    } else {
        self->field_0x1E4--;
        self->field_0x02C += (s16)self->field_0x1E8;
    }
}

/* 0x801FDEB4 - start a turn from the current angle to `to` over `ticks`. */
void fn_801FDEB4(_LB_NPC* self, s32 to, s32 ticks)
{
    self->field_0x1D4 = (s16)to - (s16)self->field_0x02C;
    self->field_0x1E8 = (u16)((s16)self->field_0x1D4 / (s16)ticks);
    self->field_0x1E4 = (s16)ticks;
}

/* 0x801FDEE4 - turn towards the motion record's first vector over `ticks`, latching the turn flag. */
void fn_801FDEE4(_LB_NPC* self, s32 ticks)
{
    s32 angle;

    self->field_0x1D4 = fn_801FDE3C(self, &self->field_0x214->vec_0x3C);
    angle = fn_8045AB38((s16)self->field_0x1D4);
    if (angle > 10 && self->field_0x229 == 0) {
        self->field_0x228 = 1;
        self->field_0x218 = self->field_0x02C;
    }
    self->field_0x1E8 = (u16)((s16)self->field_0x1D4 / (s16)ticks);
    self->field_0x1E4 = (s16)ticks;
}

/* 0x801FDF70 - start a turn back to the latched angle over `ticks`. */
void fn_801FDF70(_LB_NPC* self, s32 ticks)
{
    self->field_0x1D4 = (u16)(self->field_0x218 - self->field_0x02C);
    self->field_0x1E8 = (s16)self->field_0x1D4 / (s16)ticks;
    self->field_0x1E4 = (s16)ticks;
    self->field_0x229 = 1;
}

/* 0x801FDFA4 - flag the NPC and zero its effect vector. */
void fn_801FDFA4(_LB_NPC* self, u32 flag)
{
    self->field_0x264 = (u8)(flag & 0x80);
    setVector3(&self->field_0x258, lbl_80799880, lbl_80799880, lbl_80799880);
}

/* 0x801FDFC0 - set the NPC's action byte. */
void fn_801FDFC0(_LB_NPC* self, u8 value)
{
    self->field_0x225 = value;
}

/* 0x801FDFC8 - set the NPC's secondary action byte. */
void fn_801FDFC8(_LB_NPC* self, u8 value)
{
    self->field_0x256 = value;
}

/* 0x801FDFD0 - true while the lobby is in its idle state. */
s32 fn_801FDFD0(void)
{
    return lobby_w.field_0x000 == 0;
}

/* 0x801FDFE4 - spawn the NPC's motion resource and hand it to the model layer. */
void fn_801FDFE4(_LB_NPC* self, u32 motion_id, s32 a, s32 b)
{
    s32 motion;

    if (self->model_0x050->flag_0x006 == 0) {
        if ((u16)motion_id < 0x3E8) {
            motion = fn_8026A00C((u16)motion_id);
        } else {
            motion = fn_801FCB68((u16)motion_id);
        }
    } else {
        motion = fn_802D282C((u16)motion_id);
    }
    fn_800E11C0(self->body_0x058, self->field_0x210, 0, (u16)motion_id, a, motion, 0, (f32)b,
                 lbl_80799884);
}

/* 0x801FE0AC - switch the NPC's motion when the requested one differs from the current one. */
void fn_801FE0AC(_LB_NPC* self, u16 motion_id, s32 a, s32 b)
{
    if ((u16)motion_id != lb_npc_Get_motion_no(self)) {
        fn_801FDFE4(self, motion_id, a, b);
    }
}

/* 0x801FE11C - clear the NPC's first state byte. */
__declspec(noinline) void fn_801FE11C(_LB_NPC* self)
{
    self->field_0x149 = 0;
}

/* 0x801FE128 - set the NPC's second state byte. */
void fn_801FE128(_LB_NPC* self, u8 value)
{
    self->field_0x14A = value;
}

/* 0x801FE130 - clear the NPC's second state byte. */
__declspec(noinline) void fn_801FE130(_LB_NPC* self)
{
    self->field_0x14A = 0;
}

/* 0x801FE13C - restart the NPC's motion state machine. */
void fn_801FE13C(_LB_NPC* self, u16 motion_id)
{
    self->field_0x006 = 0;
    self->field_0x007 = 0;
    self->field_0x008 = 0;
    self->field_0x1D0 = self->field_0x1CC;
    self->field_0x1CC = motion_id;
    self->field_0x1D8 = (s16)ran_suu(1);
    self->field_0x1DC = lbl_80799884;
    fn_801FE1F8(self);
    fn_801FE11C(self);
    fn_801FE130(self);
}

/* 0x801FE1AC - true when `motion_id` is the NPC's current motion. */
s32 fn_801FE1AC(_LB_NPC* self, u32 motion_id)
{
    return (u16)motion_id - self->field_0x1CC == 0;
}

/* 0x801FE1CC / 0x801FE1DC - play the NPC's motion, once or looped. */
void fn_801FE1CC(_LB_NPC* self, u32 motion_id)
{
    fn_800E16DC(self->body_0x058, (u16)motion_id, 0);
}

void fn_801FE1DC(_LB_NPC* self, u32 motion_id)
{
    fn_800E16DC(self->body_0x058, (u16)motion_id, 1);
}

/* 0x801FE1EC - stop the NPC's motion. */
void fn_801FE1EC(_LB_NPC* self)
{
    fn_800E2198(self->body_0x058, 0);
}

/* 0x801FE1F8 - finalise the NPC's model. */
void fn_801FE1F8(_LB_NPC* self)
{
    fn_800E1640(self->body_0x058);
}

}
} /* namespace s_801FBF78 */
 /* extern "C" */

/* 0x801FE1C4 - the NPC's current motion number. */
u16 lb_npc_Get_motion_no(_LB_NPC* self)
{
    return self->motion_0x0A8;
}

/* ===================================================================================================
 * The per-frame world-update group (0x801FC6EC..0x801FCB68).
 */
void prim_init_all(void);
void player_control_move(void);
void light_move(void);
void push_g3d_wk(void* work);
s32 get_now_mapno(void);
void* ckResourceName(s8* name);
namespace s_801FBF78 {


extern "C" {
void fn_80212370(void);
void fn_8021261C(void);
void fn_803C3F6C(void);
u32 fn_804276D8(void);
void fn_80375D30(void);
void fn_802DB2B8(void);
void fn_80211E68(void);
void fn_802680C4(void);
void fn_802B58D4(void);
void fn_802ADB98(void);
void fn_800F8DA4(void);
void fn_8035AB50(void);
void ai_npc_reaction_forward(void);
void fn_802DB2EC(void);
void fn_802A07A0(void);
void fn_8021DC24(void);
void fn_80324274(void);
void fn_802DB2F0(void);
void fn_801FD0F8(void);
void fn_801FD174(_LB_NPC* self);
void fn_801FD338(_LB_NPC* self, u8* data);
u32 fn_802FB600(void);
void fn_801E0298(u8 index);
void fn_800E26C4(u8* self);
void res_file_ctor(void* out, s32 zero);
void res_file_assign(void* a, void* b);
void fn_800E3358(s32 a, u8* b, void* c);
void fn_800D5CAC(void* rec);
void* memset(void* dst, s32 c, u32 n);
}

extern "C" {

/* 0x801FC6EC - the lobby's per-frame world update: refresh the systems, then either arm the village
 * transition or run the normal frame's subsystem ticks. */
void fn_801FC6EC(void)
{
    prim_init_all();
    fn_80212370();
    fn_8021261C();
    fn_803C3F6C();
    lobby_w.field_0x027 = lobby_w.field_0x000;
    if (fn_804276D8() == 1) {
        fn_80375D30();
    }
    player_control_move();
    lbl_80794AB0 = 0;
    if (fn_804276D8() == 1 && lobby_state_block[3] == 0 && lobby_w.field_0x16F == 0 &&
        lobby_w.field_0x163 == 0) {
        lobby_w.field_0x12C = 1;
        lobby_w.field_0x172 = 10;
    } else {
        fn_802DB2B8();
        fn_801FD0F8();
        fn_80211E68();
        fn_802680C4();
        fn_802B58D4();
        fn_802ADB98();
        light_move();
        fn_800F8DA4();
        fn_8035AB50();
        ai_npc_reaction_forward();
        fn_802DB2EC();
        system_w.field_0x8C0 = 0;
        if (lobby_w.field_0x12C != 1 && (lobby_w.field_0x000 == 0 || lobby_w.field_0x000 == 0x24)) {
            fn_802A07A0();
        }
        fn_8021DC24();
        fn_80324274();
        fn_802DB2F0();
    }
}

/* 0x801FC8F0 - reset the 0x12 NPC slots and refill them from the current map's NPC table. */
void fn_801FC8F0(void)
{
    s32 i;
    s32 count;
    _LB_NPC* npc;
    u8* data;

    for (i = 0; i < 0x12; i++) {
        fn_801FD174(&lb_npc[i]);
    }
    if (lbl_80794B18[1] == 0x15) {
        data = (u8*)npc_data_town[lbl_80794B18[2]];
    } else {
        data = (u8*)npc_data_village[lbl_80794B18[2]];
    }
    npc = lb_npc;
    count = 0;
    while (data[0] != 0) {
        if (count >= 0x12) {
            break;
        }
        if (fn_802FB600() == 1) {
            fn_801FD338(npc, data);
            npc++;
            count++;
        }
        data += 8;
    }
    memset(lbl_806AA6F0, 0, 0x1C);
    if (get_now_mapno() == 0x16) {
        for (i = 0; i < 7; i++) {
            fn_801E0298((u8)i);
        }
    }
}

/* 0x801FCA00 - release every live NPC's model slot. */
void fn_801FCA00(void)
{
    s32 i;
    _LB_NPC* npc = lb_npc;

    for (i = 0; i < 0x12; i++) {
        if (npc->field_0x000 != 0) {
            if (npc->field_0x164 != 0) {
                push_g3d_wk(npc->field_0x164);
                npc->field_0x164 = 0;
                npc->field_0x170 = 0;
            }
            fn_800E26C4(npc->body_0x058);
        }
        npc++;
    }
}

/* 0x801FCA80 - queue the `index`-th row of the lobby resource table for loading. */
void fn_801FCA80(u32 index)
{
    LbResRec rec;

    fn_801FCAC8((u32*)&rec, (const u32*)&lbl_80582A68[index]);
    rec.c_0x08 = 0;
    fn_800D5CAC(&rec);
}

/* 0x801FCADC - load the `index`-th resource table entry and hand it to effect id 4. */
void fn_801FCADC(u8* self, u32 index)
{
    LbResRec dst;
    LbResRec src;

    res_file_ctor(&dst, 0);
    {
        LbResource* res = (LbResource*)ckResourceName((s8*)lbl_80582A68[index].b_0x04);
        if (res != 0) {
            res_file_ctor(&src, (s32)res->payload_0x44);
            res_file_assign(&dst, &src);
            fn_800E3358(4, self, &dst);
        }
    }
}

/* 0x801FCB68 - the NPC's friendship/level value for a scripted motion id >= 0x3E8. */
s32 fn_801FCB68(u32 id)
{
    u16 value = (u16)(id - 0x3E8);

    if ((u16)id < 0x3E8) {
        return 0;
    }
    return ((s16*)npc_lp_tbl[value / 100])[value % 100];
}

} /* extern "C" */

/* ===================================================================================================
 * The NPC slot update group (0x801FD0F8..0x801FDBD0).
 */
extern "C" {
void fn_80207B28(void);
void fn_801FD4BC(_LB_NPC* self);
void fn_801FD594(_LB_NPC* self);
void fn_801FDBD0(_LB_NPC* self);
void fn_801FF9E0(u8* sub, _LB_NPC* self);
}

extern "C" {

/* 0x801FD0F8 - tick every live NPC slot. */
void fn_801FD0F8(void)
{
    s32 i;
    _LB_NPC* npc = lb_npc;

    fn_80207B28();
    for (i = 0; i < 0x12; i++) {
        if (npc->field_0x000 != 0) {
            if (npc->field_0x005 == 0) {
                fn_801FD4BC(npc);
                fn_801FD594(npc);
            }
            fn_801FDBD0(npc);
        }
        npc++;
    }
}

/* 0x801FD174 - clear one NPC slot back to its reset state. */
void fn_801FD174(_LB_NPC* self)
{
    u16 last = 0xFFFF;

    self->field_0x000 = 0;
    self->field_0x001 = 0;
    self->field_0x002 = 0;
    self->field_0x003 = 0;
    self->field_0x004 = 0xFF;
    self->field_0x005 = 0;
    self->field_0x006 = 0;
    self->field_0x007 = 0;
    self->field_0x008 = 0;
    self->field_0x00C = 0;
    self->pos_0x10.x = lbl_80799880;
    self->pos_0x10.y = lbl_80799880;
    self->pos_0x10.z = lbl_80799880;
    copyVec3(&self->field_0x01C, &self->pos_0x10);
    self->field_0x028 = 0;
    self->field_0x02C = 0;
    self->field_0x030 = 0;
    self->field_0x034 = lbl_80799880;
    setVector3(&self->field_0x038, lbl_80799880, lbl_80799880, lbl_80799880);
    setVector3(&self->field_0x044, lbl_80799880, lbl_80799880, lbl_80799880);
    self->model_0x050 = 0;
    fn_801FF9E0(self->field_0x054, self);
    self->field_0x1CC = last;
    self->field_0x1CE = 0;
    self->field_0x1D0 = last;
    self->field_0x1D4 = 0;
    self->field_0x1D8 = 0;
    self->field_0x1DC = lbl_80799884;
    self->field_0x1E0 = 0;
    self->field_0x1E2 = 0;
    self->field_0x1E4 = 0;
    self->field_0x1E8 = 0;
    setVector3(&self->target_0x1EC, lbl_80799880, lbl_80799880, lbl_80799880);
    setVector3(&self->vel_0x1F8, lbl_80799880, lbl_80799880, lbl_80799880);
    self->field_0x204 = 0;
    self->field_0x208 = 0;
    self->field_0x20A = 0;
    self->field_0x20C = lbl_80799880;
    self->field_0x210 = 0xFF;
    self->field_0x214 = 0;
    self->field_0x218 = 0;
    self->field_0x21C = 0;
    self->field_0x222 = 0;
    self->field_0x21E = 0;
    self->field_0x223 = 0;
    self->field_0x220 = 0;
    self->field_0x224 = 0;
    self->field_0x225 = 0;
    self->field_0x226 = 0;
    self->field_0x227 = 0;
    self->field_0x228 = 0;
    self->field_0x229 = 0;
    self->field_0x22A = 0;
    self->field_0x22B = 0;
    self->field_0x22C = 0;
    self->field_0x22D = 0;
    self->field_0x22E = 0;
    self->field_0x230 = 0;
    memset(self->field_0x234, 0, 0x20);
    self->field_0x254 = 0;
    self->field_0x255 = 0xFF;
    self->field_0x256 = 0;
    setVector3(&self->field_0x258, lbl_80799880, lbl_80799880, lbl_80799880);
    self->field_0x264 = 0;
}

} /* extern "C" */

} /* namespace s_801FBF78 */

/* ==== absorbed from lobby/fn_802029B4.cpp (0x802029B4..0x80207698) ==== */
/* lobby/fn_802029B4.cpp - `.text` 0x802029B4..0x802076D4 (68 functions, 0x4D20 bytes), extab
 * 0x80010DBC..0x80010F7C (56 unwind records), extabindex 0x8002D384..0x8002D624 (56 x 12 B).
 * Registered once, at its final home (docs/plan.md 12), from proposal/802029B4_fn_802029B4.cpp.
 *
 * What it is.  The lobby NPC work band: every function takes the shared `_LB_NPC` record and runs a
 * byte state machine on `_LB_NPC::field_0x006` (0..3), arming one of the NPC's motions through the
 * `lobby/lb_npc.cpp` helper set (`fn_801FDFE4` = set motion + build the MHchar, `fn_801FDEE4`/
 * `fn_801FDF70` = play the motion over the model's own frame, `fn_801FE13C` = restart the motion the
 * NPC's motion table entry names) and waiting on `fn_801FDFD0` (the lobby's field_0x000 flag).  The
 * band also carries the NPC's own steering/position code (the `nw4r` math and `VEC3_ctor`/
 * `copyVec3` vector work) and three 0x14-stride switch tables in `.data`.
 *
 * Module and name (brief section 2, in evidence order).
 *   1. No `__FILE__` string covers the range: the only bare source name in the image's `.data` is
 *      `enemy_control.cpp` (0x805A1BB8) and it is referred to by the registered `enemy/enemy_control.cpp`
 *      unit far below, never from here (this range's `.data` references are the `jumptable_805B8F88`/
 *      `805B8FBC`/`805B9000`/`lbl_805B8E**` tables only).  `attribute.py`'s `source_owner` soft vote
 *      reports the same name for this proposal, but it reports it for the registered `lobby/lb_npc.cpp`
 *      band below and for 21 other unrelated ranges, so it is an artefact of the vote's width, not
 *      evidence.
 *   2. `dumpmap.py lookup` answers `zz_02029b4_` for the range (a placeholder is not evidence).
 *   3. The band is `lobby`: both bracketing registered units are `lobby` (`lobby/lb_npc.cpp` below at
 *      0x801FBF78..0x802029B4, `lobby/fn_80212810.cpp` above at 0x80212810..0x80219260), the code takes
 *      the `_LB_NPC` record `include/lobby/lb_npc.h` owns, calls its helpers, and reads the band's own
 *      `.bss` (`lb_npc_move_data`, `lobby_w`).  The file keeps the map's own stem, like its neighbours.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802029B4 0x802076D4`: all 68 rows are bare
 * `fn_XXXXXXXX = .text:0x...` entries, and `python tools/symbols/dumpmap.py lookup` answers
 * `zz_XXXXXXXX_` for them in the shared runtime dump).
 *
 * Language.  C++: the range's callees are C++ manglings (`lb_npc_Get_motion_no__FP7_LB_NPC`,
 * `ran_suu__Fl`, `get_move_work_adrs__FUc`, `get_now_mapno__Fv`, `LbCheckKujiraEvent__Fv`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `rotVecY__FPQ34nw4r4math4VEC3Ul`, `mulVecMat__FP...`,
 * `calcVecAng2__FP...`, `calcDistanceSqXZ__FP...`).  Rule 9: those are declared with the signature
 * their mangling encodes and called through it; every `fn_*` definition stays `extern "C"`.
 *
 * Sections.  The target object's extab is exactly the 56 framed functions' unwind records (the other
 * 12 rows are frameless), so the unit claims 0x80010DBC..0x80010F7C / 0x8002D384..0x8002D624 - pinned
 * by the split's own per-function objects, whose `extabindex` relocations run `@etb_80010DBC` (this
 * range's `fn_802029B4`) up to `@etb_80010F74` and then hand `@etb_80010F7C` to `fn_802076D4`, the
 * next proposal's first framed function.  `cflags_lobby`'s `-Cpp_exceptions on` (flags-audit
 * 2026-09-28, no longer a per-file pragma) is what emits them; see the residual note below.
 *
 * Seam.  Unproven, as the brief says.  Both edges are function boundaries at the registered units'
 * addresses; the left edge is `attribute.py`'s byte cap rather than evidence, and this band may be the
 * tail of `lobby/lb_npc.cpp`'s original file (they share the `_LB_NPC` type, the `.bss` and the helper
 * set) - the outbox records the merge question.
 *
 * Status.  63 of the 68 rows are written, 62 of them byte-identical (`report.json`: `.text` 81.05 %,
 * 15524/19744 B).  Still absent (not stubbed): fn_80204DA8, fn_8020623C, fn_80206AD0, fn_80207284,
 * fn_802074E8 - continue at fn_80204DA8 in address order.
 *
 * Residuals.
 *   - fn_802050AC 98.2377 %: the target schedules the accumulation's `lwz r4,44(r31)` between the
 *     `(s16)(u16)` mask and the signed `/ 5`, ours issues the load after the divide; `+=`, `a = a + b`
 *     and a named temporary all measure the same.
 *   - extab 63.19 %: all 56 records are emitted (the lib's `-Cpp_exceptions on`) and paired, and a
 *     few records' flag words differ from the target's.
 *   - extabindex 0 %: the 672 bytes are emitted at the target's exact size but objdiff pairs none of
 *     the entries (the target's carry relocations to functions the linker placed at their DOL
 *     addresses, ours resolve inside this object).
 *
 * Shapes that earned their score.
 *   - `__declspec(noinline)` on the eight rows `fn_80203114` dispatches to: with the lib's
 *     `-inline auto` MWCC inlines them into the dispatcher and it scores 54.81 instead of 99.09 (the
 *     outbox asks for `-inline noauto` on the lib, which reaches the same 99.09 without the
 *     attributes).
 *   - `#pragma peephole off` around `fn_80203114`: the target keeps `clrlwi`+`clrlwi`+`cmpwi` unfused
 *     for `(ran_suu(1) & 1) != 0` (99.38 -> 100.0).
 *   - callee return width is codegen: `cmplwi r3,1` means the callee returns `u32`; `u16 ran_suu`
 *     would drop the target's `clrlwi r0,r3,16`, so every `ran_suu` use narrows with `(u16)`.
 *   - the target materialises the negative angles `-8192`/`-7281`/`-1819`/`-909`/`-17202` as
 *     `lis r3,1` + `addi`/`subi` (the unsigned 16-bit value), so the source writes `(u16)-8192`.
 *   - a two-arm chain on a value is a `switch`, not `if/else if`, where the target keeps the compare
 *     chain at the top (`fn_80203EC4`, `fn_802047C8`); and the small arm goes last in the source
 *     (`fn_802069D0`), which is what puts the `bgt` on the target's side.
 *   - `i` declared and initialised at the top of `fn_802055D8` (not in the `for`) flips the allocator's
 *     colouring to the target's (self in r30, `i` in r31); `self->field_0x204[0]` vs
 *     `[self->field_0x208]` picks whether the target's `mulli 20` is kept or folded.
 */
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "unsplit/lobby.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
namespace s_802029B4 {


/* ---------------------------------------------------------------------------------------------------
 * Declarations.  The lobby band's helper set is defined in `lobby/lb_npc.cpp`; its header publishes the
 * `_LB_NPC` type this range needs but not these prototypes, so they are declared here and the header
 * corrections are a `shared-file` request in the worker's outbox.  Functions and scalars without a
 * mangling are C linkage; the mangled callees are declared at C++ scope (rule 9).
 */
extern "C" {
f32 fn_80050EF4(VEC3* a, VEC3* b);
void addVec3(VEC3* out, const VEC3* a, const VEC3* b);
void fn_801FDD9C(_LB_NPC* self, f32 duration);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
void fn_801FDEB4(_LB_NPC* self, s32 to, s32 ticks);
void fn_801FDEE4(_LB_NPC* self, s32 frames);
void fn_801FDF70(_LB_NPC* self, s32 frames);
void fn_801FDFC0(_LB_NPC* self, u8 value);
void fn_801FDFC8(_LB_NPC* self, u8 value);
u32 fn_801FDFD0(_LB_NPC* self);
void fn_801FDFE4(_LB_NPC* self, u16 id, s32 a3, s32 a4);
void fn_801FE0AC(_LB_NPC* self, u16 id, s32 a3, s32 a4);
void fn_801FE13C(_LB_NPC* self, u16 id);
u32 fn_801FE1CC(_LB_NPC* self, u16 id, f32 a, f32 b);
u32 fn_801FE1DC(_LB_NPC* self, u16 id, f32 a, f32 b);
u32 fn_801FE1EC(_LB_NPC* self);
void fn_801FE200(_LB_NPC* self, u8 visible, bool flag);
void fn_801FE128(_LB_NPC* self, u8 value);
u32 fn_801FE32C(_LB_NPC* self, u8 kind);
u32 fn_801FE348(_LB_NPC* self);
u32 fn_801FE39C(_LB_NPC* self);
u32 fn_801FE400(_LB_NPC* self);
u32 fn_801FE450(_LB_NPC* self);
u32 fn_801FE4A0(_LB_NPC* self);
u32 fn_801FE4B4(_LB_NPC* self);
u32 fn_801FE55C(_LB_NPC* self);
u32 fn_801FE8CC(_LB_NPC* self);
u8 fn_802FB5E4(void);
u8 fn_802FB948(void);
u8 fn_802FB8EC(s32 idx);
void fn_802BE4FC(s32 id);
void fn_80201AA0(_LB_NPC* self, s32 flag);
void fn_80201BE4(_LB_NPC* self);
void fn_80201C80(_LB_NPC* self);
void fn_80201F60(_LB_NPC* self);
void fn_802020FC(_LB_NPC* self);
void fn_80202394(_LB_NPC* self);
void fn_802024C4(_LB_NPC* self);
void fn_80202570(_LB_NPC* self);
void fn_80202728(_LB_NPC* self);
void fn_802027CC(_LB_NPC* self, s32 action);
void fn_801FFE44(_LB_NPC* self);
void fn_801E9A90(u8 a, u8 b);
s32 fn_80207EAC(_LB_NPC* self);
LbNpcMotionEntry* fn_80207DC4(_LB_NPC* self, u8* kind);
void fn_80207FD0(_LB_NPC* self, u8 kind, s32 flag);
s32 fn_802080C0(_LB_NPC* self);
void fn_80395D04(_LB_NPC* self, s32 a, s32 b, VEC3* v, f32 f);
}
} /* namespace s_802029B4 */


u16 lb_npc_Get_motion_no(_LB_NPC* self);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
u32 LbCheckKujiraEvent(void);
void* get_move_work_adrs(u8 idx);
namespace s_802029B4 {

extern "C" s32 my_player_no(void);
} /* namespace s_802029B4 */

s32 get_now_mapno(void);
namespace s_802029B4 {


/* ---------------------------------------------------------------------------------------------------
 * Bodies
 */
extern "C" {

/* 0x802029B4 - the NPC's `state` machine 0: arm motion 1147, wait for the 0x47C motion to run out and
 * select one of two follow-up arms, then restart the motion the motion table names. */
__declspec(noinline) void fn_802029B4(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1147, 4, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            if (fn_801FE1CC(self, 2, lbl_807999C4, lbl_807999A0) == 1) {
                fn_801FDFE4(self, 1148, 12, 50);
            } else {
                fn_801FDFE4(self, 1148, 8, 0);
            }
            self->field_0x006++;
        }
        break;
    case 2:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202AC4 - arm 1167 off motion 1151, or 1169 off motion 1168, then wait for the flag and restart
 * the table's motion. */
__declspec(noinline) void fn_80202AC4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (lb_npc_Get_motion_no(self) == 1151) {
            fn_801FDFE4(self, 1167, 4, 0);
        } else if (lb_npc_Get_motion_no(self) == 1168) {
            fn_801FDFE4(self, 1169, 4, 0);
        }
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202B88 - arm 1163 over 16 frames plus motion 10, then wait for the flag, advance motion 10 and
 * restart the table's motion. */
__declspec(noinline) void fn_80202B88(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1163, 16, 0);
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202C24 - play motion 10, arm 1130; on motion 1 arm 1129 over 16 frames and 24 frames of delay,
 * else step on once the flag is up; then wait for the flag, advance motion 10 and restart the table's. */
__declspec(noinline) void fn_80202C24(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1130, 4, 0);
        break;
    case 1:
        if (fn_801FE1CC(self, 1, lbl_807999C8, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 16, 24);
        } else if (fn_801FDFD0(self) == 1) {
            self->field_0x006++;
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202D24 - play motion 10 and arm 1131, then wait for the flag, advance motion 10 and restart the
 * table's motion. */
__declspec(noinline) void fn_80202D24(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1131, 8, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202DD0 - play motion 10 and arm 1129 over 8 frames, then wait for the flag, advance motion 10 and
 * restart the table's motion. */
__declspec(noinline) void fn_80202DD0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1129, 8, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202E6C - arm 1180 over 2 frames, then wait for the flag and restart the table's motion. */
__declspec(noinline) void fn_80202E6C(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1180, 2, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202F00 - the NPC's `state` 4-arm machine for the motion it is playing: arm 1092 (or 1188 when
 * motion 37 is up), wait for the motion to stop, then either walk the last leg of the motion (turning
 * 20 ticks onto the angle to the motion's own vector) or hand the motion off once the lobby is idle. */
__declspec(noinline) void fn_80202F00(_LB_NPC* self)
{
    VEC3 offset;
    VEC3 delta;

    VEC3_ctor(&offset);
    self->field_0x22D = 1;
    self->field_0x22C = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (self->field_0x1D0 == 37) {
            fn_801FDFE4(self, 1188, 4, 0);
        } else {
            fn_801FE0AC(self, 1092, 4, 0);
            self->field_0x006 = 2;
        }
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FE0AC(self, 1092, 4, 0);
        }
        break;
    case 2:
        if (self->field_0x1E4 <= 0) {
            self->field_0x006++;
            self->field_0x00C = 300;
        }
        break;
    case 3:
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->field_0x214->vec_0x3C) / 10;
        self->field_0x02C += self->field_0x1D4;
        if (fn_80050EF4(&self->pos_0x10, &self->field_0x214->vec_0x3C) < lbl_807999CC) {
            setVector3(&offset, lbl_807999A0, lbl_807999A0, lbl_807999CC);
            rotVecY(&offset, calcVecAng2(&self->field_0x214->vec_0x3C, &self->pos_0x10));
            addVec3(&delta, &self->field_0x214->vec_0x3C, &offset);
            copyVec3(&self->target_0x1EC, &delta);
            fn_801FDD9C(self, lbl_807999D0);
            fn_801FE13C(self, 44);
        } else if (fn_801FDFD0(self) == 1) {
            if (lobby_w.field_0x076 == 1) {
                fn_801FE13C(self, 55);
            } else {
                fn_801FE13C(self, 44);
            }
        }
        break;
    }
}

#pragma peephole off

/* 0x80203114 - the band's top-level per-frame dispatcher: it forces the action byte to 1 and then
 * routes on the model kind (and, for the kinds without a handler, arms one of the motions the kind
 * selects), which is how each per-kind row below is reached. */
void fn_80203114(_LB_NPC* self)
{
    fn_801FDFC0(self, 1);

    if (fn_801FE32C(self, 1) == 1) {
        fn_80201AA0(self, 1);
    } else if (fn_801FE32C(self, 14) == 1) {
        fn_80201BE4(self);
    } else if (fn_801FE32C(self, 11) == 1) {
        fn_80201C80(self);
    } else if (fn_801FE32C(self, 2) == 1) {
        fn_80201F60(self);
    } else if (fn_801FE32C(self, 13) == 1) {
        fn_802020FC(self);
    } else if (fn_801FE348(self) == 1) {
        fn_80202394(self);
    } else if (fn_801FE39C(self) == 1) {
        fn_802024C4(self);
    } else if (fn_801FE32C(self, 3) == 1) {
        if (LbCheckKujiraEvent() == 1) {
            fn_80202728(self);
        } else {
            fn_80202570(self);
        }
    } else if (fn_801FE32C(self, 7) == 1) {
        if (fn_802FB5E4() == 9) {
            fn_802027CC(self, 1);
        } else if (((u16)ran_suu(1) & 1) != 0) {
            fn_802027CC(self, 0);
        } else {
            fn_802027CC(self, 1);
        }
    } else if (fn_801FE400(self) == 1) {
        fn_802029B4(self);
    } else if (fn_801FE32C(self, 12) == 1) {
        fn_80202AC4(self);
    } else if (fn_801FE32C(self, 5) == 1) {
        fn_80202B88(self);
    } else if (fn_801FE4A0(self) == 1) {
        fn_80202E6C(self);
    } else if (fn_801FE8CC(self) == 1) {
        fn_80202F00(self);
    } else if (fn_801FE4B4(self) == 1) {
        fn_80202DD0(self);
    } else if (fn_801FE55C(self) == 1) {
        fn_80202D24(self);
    } else if (fn_801FE32C(self, 19) == 1 || fn_801FE32C(self, 20) == 1) {
        fn_80202C24(self);
    } else {
        switch (self->field_0x006) {
        case 0:
            if (fn_801FE32C(self, 10) == 1 || fn_801FE32C(self, 9) == 1) {
                fn_801FE0AC(self, 1047, 16, 0);
            } else if (fn_801FE32C(self, 8) == 1) {
                if (lb_npc_Get_motion_no(self) == 52 || lb_npc_Get_motion_no(self) == 51) {
                    fn_801FE0AC(self, 51, 4, 0);
                } else {
                    fn_801FE0AC(self, 1136, 4, 0);
                }
            } else if (fn_801FE32C(self, 17) == 1) {
                fn_801FE0AC(self, 1136, 4, 0);
            } else if (fn_801FE450(self) == 1) {
                fn_801FE0AC(self, 1144, 4, 0);
            } else if (fn_801FE32C(self, 16) == 1) {
                if (lb_npc_Get_motion_no(self) == 1137 || lb_npc_Get_motion_no(self) == 1138) {
                    fn_801FE0AC(self, 1139, 4, 0);
                } else {
                    fn_801FE0AC(self, 1136, 4, 0);
                }
            } else if (fn_801FE32C(self, 6) == 1) {
                fn_801FE0AC(self, 1111, 4, 0);
            } else if (fn_801FE32C(self, 4) == 1) {
                fn_801FE0AC(self, 1071, 4, 0);
            } else if (fn_801FE32C(self, 18) == 1) {
                fn_801FE0AC(self, 1098, 4, 0);
            } else {
                fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
                break;
            }
            self->field_0x006++;
            fn_801FDEE4(self, 10);
            break;
        case 1:
            if (fn_801FDFD0(self) == 1) {
                fn_801FDF70(self, 10);
                self->field_0x006++;
            }
            break;
        case 2:
            if (self->field_0x1E4 <= 0) {
                fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
            }
            break;
        }
    }
}


/* 0x80203664 - the NPC's 5-arm `state` cycle for the "look around" action: hold motion 1040 for a random
 * 100..355 frames, then either move on (state 1) or hand the action over (state 4); states 2..4 wait for
 * the motion to stop and pick the next hold or restart. */
void fn_80203664(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
        fn_801FDFE4(self, 1040, 16, 0);
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C > 0) {
            break;
        }
        if (((u16)ran_suu(1) & 1) == 0) {
            self->field_0x006++;
            self->field_0x00C = 1;
            fn_801FDFE4(self, 1041, 6, 0);
        } else {
            self->field_0x006 = 4;
            fn_801FDFE4(self, 1044, 8, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1042, 0, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 16, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 8, 0);
        }
        break;
    }
}

/* 0x80203834 - the NPC's "wait" opener: arm motion 1088 over 4 frames once. */
void fn_80203834(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1088, 4, 0);
}

/* 0x8020385C - the NPC's "sit down" opener: arm motion 1086 over 6 frames once. */
void fn_8020385C(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1086, 6, 0);
}

/* 0x80203884 - the NPC's idle `state` machine: play motion 10, arm 1081 when the turn runs out, wait for
 * the motion to stop and for the lobby flag to arm 1082, then either release the action byte and arm 1078
 * or release it and restart the motion the table names. */
void fn_80203884(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (self->field_0x1E4 == 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1081, 6, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            if (fn_801FDFD0(self) == 1) {
                fn_801FDFE4(self, 1082, 6, 0);
                self->field_0x006++;
            }
        }
        break;
    case 3:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1078, 6, 0);
            self->field_0x006++;
        }
        break;
    case 4:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802039D4 - pick the NPC's "sit" motion variant once, 1134 or 1135, off the kind byte. */
void fn_802039D4(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    switch (kind) {
    case 0:
        fn_801FE0AC(self, 1134, 6, 0);
        break;
    case 1:
        fn_801FE0AC(self, 1135, 6, 0);
        break;
    }
}

/* 0x80203A24 - `fn_80203664`'s sibling for the NPC that holds motion 1040 for a random 100..355 frames
 * and restarts it (rather than handing over) from state 4. */
void fn_80203A24(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
        fn_801FDFE4(self, 1040, 4, 0);
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C > 0) {
            break;
        }
        if (((u16)ran_suu(1) & 1) == 0) {
            self->field_0x006++;
            self->field_0x00C = 1;
            fn_801FDFE4(self, 1041, 6, 0);
        } else {
            self->field_0x006 = 4;
            fn_801FDFE4(self, 1044, 8, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1042, 0, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = 120;
            fn_801FDFE4(self, 1040, 4, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 8, 0);
        }
        break;
    }
}

/* 0x80203BE4 - the NPC's three-arm "idle fidget": hold motion 1142 for a random 0..7 frames, then hold
 * it again (restarting the counter) or pick 1143/1149 for the next two frames off a coin flip. */
void fn_80203BE4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1142, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                if (((u16)ran_suu(1) & 1) != 0) {
                    fn_801FDFE4(self, 1143, 2, 0);
                } else {
                    fn_801FDFE4(self, 1149, 4, 0);
                }
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1142, 4, 0);
        }
        break;
    }
}

/* 0x80203D10 - the NPC's four-arm "notice and greet" machine: hold motion 1063 with the shell visible,
 * then either walk one of two 24-frame motions (1064/1065, off a coin flip) or, if the player's motion
 * never arrived, flash the shell; states 2/3 restart motion 1063 or the table's motion once their own
 * motion is done. */
void fn_80203D10(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1063, 4, 0);
        fn_801FE200(self, 24, 1);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (((u16)ran_suu(1) & 1) != 0) {
                fn_801FDFE4(self, 1064, 24, 0);
                self->field_0x006 = 2;
            } else {
                fn_801FDFE4(self, 1065, 24, 0);
                self->field_0x006 = 3;
            }
        } else if (fn_801FE1CC(self, 0, lbl_807999D4, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 1);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, 0);
        } else if (fn_801FE1CC(self, 0, lbl_807999D8, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 0);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            fn_801FDFE4(self, 1063, 4, 0);
        } else if (fn_801FE1CC(self, 0, lbl_807999DC, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 0);
        }
        break;
    }
}

/* 0x80203EC4 - the NPC's four-arm "map greeting" machine: it picks motion 1055 or 1051 by whether the
 * current map is map 22, waits for the motion, counts 2 motions down, then arms 1056/1058 the same way
 * and finally restarts the table's motion. */
void fn_80203EC4(_LB_NPC* self)
{
    s32 other_map;

    fn_801FDFC0(self, 0);
    other_map = ((u8)get_now_mapno() != 22);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = 2;
        switch (other_map) {
        case 0:
            fn_801FDFE4(self, 1055, 16, 12);
            self->field_0x006 = 2;
            break;
        case 1:
            fn_801FDFE4(self, 1051, 2, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1057, 4, 0);
            self->field_0x006++;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                switch (other_map) {
                case 0:
                    fn_801FDFE4(self, 1056, 4, 0);
                    break;
                case 1:
                    fn_801FDFE4(self, 1058, 6, 0);
                    break;
                }
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80204070 - the NPC's three-arm hold: motion 1035 for a random 0..3 frames, then 1036 for the next
 * 6, then release the action byte. */
void fn_80204070(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FDFE4(self, 1035, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1036, 6, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, 0);
        }
        break;
    }
}

/* 0x80204318 - arm motion 1155 over 8 or 6 frames off the kind byte, then motion 2 with the 102-frame
 * delay and hand the NPC over to the band above. */
void fn_80204318(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        switch (kind) {
        case 0:
            fn_801FDFE4(self, 1155, 8, 12);
            break;
        case 1:
            fn_801FDFE4(self, 1155, 6, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 2, 6, 102);
            fn_801FFE44(self);
        }
        break;
    }
}

/* 0x802043D4 - arm motion 511 or 502 over 12 frames off the kind byte, then play the NPC's motion 0 at
 * the kind's own speed and, once it is running, flag the NPC's part slot and clear its flag byte. */
void fn_802043D4(_LB_NPC* self, u8 kind)
{
    f32 speed = lbl_807999A0;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        switch (kind) {
        case 0:
            fn_801FDFE4(self, 511, 12, 0);
            break;
        case 1:
            fn_801FDFE4(self, 502, 12, 0);
            break;
        }
        break;
    case 1:
        switch (kind) {
        case 0:
            speed = lbl_807999E4;
            break;
        case 1:
            speed = lbl_807999E8;
            break;
        }
        if (fn_801FE1CC(self, 0, speed, lbl_807999A0) == 1) {
            s32 slot = fn_802080C0(self);

            self->field_0x234[0] = 2;
            fn_801E9A90((u8)slot, 2);
            self->field_0x001 = 0;
        }
        break;
    }
}

/* 0x80204148 - the NPC's four-arm "wave the player over" machine: play motion 10, arm 1184 and set the
 * three shell flags, arm 1185 once the lobby flag is up (keeping the shell visible while the pad is
 * idle), answer 1184/1185's window by re-flagging the shell or handing over to the band above, then
 * restart the table's motion. */
void fn_80204148(_LB_NPC* self)
{
    fn_801FDFC0(self, 0);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (self->field_0x1E4 == 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1184, 4, 0);
            fn_801FE200(self, 24, 1);
            fn_801FE200(self, 22, 0);
            fn_801FE200(self, 23, 0);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1185, 4, 0);
            fn_801FE200(self, 25, 0);
            if (fn_802FB948() == 0) {
                fn_801FE200(self, 24, 1);
            }
        } else if (fn_801FE1CC(self, 0, lbl_807999C8, lbl_807999A0) != 0) {
            fn_801FE200(self, 24, 0);
            fn_801FE200(self, 25, 1);
        } else if (fn_801FE1CC(self, 0, lbl_807999E0, lbl_807999A0) != 0) {
            fn_802BE4FC(8);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802044C0 - arm one of the three "look" motions 1159/1160/1161 over 8 frames, once, off the kind
 * byte. */
void fn_802044C0(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    switch (kind) {
    case 0:
        fn_801FE0AC(self, 1159, 8, 0);
        break;
    case 1:
        fn_801FE0AC(self, 1160, 8, 0);
        break;
    case 2:
        fn_801FE0AC(self, 1161, 8, 0);
        break;
    }
}

/* 0x802046E4 - point the NPC at the lobby's move table and restart its first motion. */
void fn_802046E4(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    self->field_0x204 = lb_npc_move_data.table_0x0C;
    self->field_0x208 = 0;
    fn_801FE13C(self, self->field_0x204[0].motion_0x0C);
}

/* 0x8020471C - clear the action byte, flag the NPC's second state byte, arm 1074 over 4 frames and then
 * restart the table's motion. */
void fn_8020471C(_LB_NPC* self)
{
    fn_801FDFC0(self, 0);
    fn_801FE128(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1074, 4, 0);
        fn_801FDEE4(self, 4);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802047C8 - the NPC's four-arm "keep the player company" machine: hold 1174 for 8 frames or 0, then
 * 1173 for 3, then 1101; while 1101 plays it nudges the NPC onto a neighbouring move table half the
 * time, and it steps the NPC 910 units along the path while either of the two pad checks answers. */
void fn_802047C8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 8;
        fn_801FE0AC(self, 1174, 8, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                self->field_0x00C = 3;
                fn_801FDFE4(self, 1173, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1101, 4, 0);
            }
        } else if (fn_801FE1CC(self, 0, lbl_807999C4, lbl_807999A0) != 0) {
            if ((u16)ran_suu(1) % 100 < 50) {
                self->field_0x204 = lb_npc_move_data.table_0x10;
                self->field_0x208 = 0;
                fn_801FE13C(self, self->field_0x204[0].motion_0x0C);
            }
        } else if (fn_801FE1CC(self, 5, lbl_807999F8, lbl_8079999C) == 1 ||
                   fn_801FE1CC(self, 5, lbl_807999FC, lbl_80799A00) == 1) {
            self->field_0x02C -= 910;
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802049C4 - play motion 1181, then either hand the NPC over to the band above or restart motion 34
 * off the kind byte. */
void fn_802049C4(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1181, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            switch (kind) {
            case 0:
                fn_801FFE44(self);
                break;
            case 1:
                fn_801FE13C(self, 34);
                break;
            }
        }
        break;
    }
}

/* 0x80204A68 - arm motion 1182 for one motion, then restart motion 36. */
void fn_80204A68(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1182, 0, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, 36);
        }
        break;
    }
}

/* 0x80204ADC - play motion 1183 over 14 frames, arm 1095 next, then restart motion 33. */
void fn_80204ADC(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1183, 14, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1095, 4, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, 33);
        }
        break;
    }
}

/* 0x80204B88 - the NPC's three-arm "back off" machine: arm 1175, wait, then arm 1176 over 16 frames
 * (or 1191 over 24) off a coin flip and restart the countdown. */
void fn_80204B88(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = 0;
        fn_801FE0AC(self, 1175, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x00C = (u16)ran_suu(1) % 2;
                self->field_0x006++;
                fn_801FE0AC(self, 1176, 16, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x00C = (u16)ran_suu(1) % 2 + 2;
                self->field_0x006 = 1;
                fn_801FE0AC(self, 1191, 24, 0);
            }
        }
        break;
    }
}

/* 0x80204CB8 - play motion 1186, then arm 1187 over 2 frames or, while motion 0 has not started yet,
 * turn the NPC onto motion 26943/27853 over 17 ticks off the kind byte. */
void fn_80204CB8(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1186, 8, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FE0AC(self, 1187, 2, 0);
        } else if (fn_801FE1CC(self, 0, lbl_807999D0, lbl_807999A0) != 0) {
            switch (kind) {
            case 0:
                fn_801FDEB4(self, 26943, 17);
                break;
            case 1:
                fn_801FDEB4(self, 27853, 17);
                break;
            }
        }
        break;
    }
}

/* 0x8020505C - arm motion 1190 over 4 frames with the 74-frame delay, once. */
void fn_8020505C(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1190, 4, 74);
}

/* 0x80205084 - arm motion 1109 over 20 frames, once. */
void fn_80205084(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1109, 20, 0);
}

/* 0x80205294 - arm motion 1098 or 1095 over 4 frames, once, off the kind byte. */
void fn_80205294(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (kind == 0) {
        fn_801FDFE4(self, 1098, 4, 0);
    } else {
        fn_801FDFE4(self, 1095, 4, 0);
    }
}

/* 0x802052D8 - arm motion 1093 over 4 frames, or re-arm it over 0 frames with the 88-frame delay. */
void fn_802052D8(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (kind == 0) {
        fn_801FDFE4(self, 1093, 4, 0);
    } else {
        fn_801FE0AC(self, 1093, 0, 88);
    }
}

/* 0x8020531C - hold motion 1105 for 20 frames, then 1103 for 5, and finally clear the lobby's field and
 * restart motion 0's table entry (or motion 52 when the map is not map 21). */
void fn_8020531C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1105, 4, 38);
        self->field_0x00C = 20;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            self->field_0x00C = 5;
            fn_801FDFE4(self, 1103, 4, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            lobby_w.field_0x076 = 0;
            if ((u8)get_now_mapno() == 21) {
                self->field_0x208 = 0;
                fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
            } else {
                self->field_0x208 = 0;
                fn_801FE13C(self, 52);
            }
        }
        break;
    }
}

/* 0x80205424 - arm 1101, then either 1095 (and step on) or, while the NPC's looped motion 0 has not
 * started, spawn the effect at the offset vector and hand the NPC over. */
void fn_80205424(_LB_NPC* self)
{
    VEC3 offset;

    VEC3_ctor(&offset);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1101, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1095, 4, 0);
            self->field_0x006++;
        } else if (fn_801FE1DC(self, 0, lbl_807999D0, lbl_807999A0) == 1) {
            setVector3(&offset, lbl_807999A0, lbl_8079999C, lbl_807999A0);
            fn_80395D04(self, 3, 10, &offset, lbl_80799A14);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x208 = 0;
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80205544 - arm motion 1104 and set the NPC's secondary action byte; once the motion is done restart
 * motion 55 and set the lobby's field. */
void fn_80205544(_LB_NPC* self)
{
    fn_801FDFC8(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1104, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, 55);
            lobby_w.field_0x076 = 1;
        }
        break;
    }
}

/* 0x802055D8 - pick the NPC's move table off the current map and its part kind, then scan that map's
 * spot table for the first spot the NPC's position is inside of (past it in both x and z) and point the
 * NPC at the matching table entry; the spot scan's 10000 x terminator leaves the table base in place. */
void fn_802055D8(_LB_NPC* self)
{
    LbNpcMotionEntry** list;
    LbNpcMotionEntry* selected;
    LbNpcMoveSpot* current;
    LbNpcMoveSpot* spot;
    u8 i = 0;

    if (self->field_0x006 != 0) {
        return;
    }
    switch ((u8)get_now_mapno()) {
    case 22:
        switch (self->field_0x004) {
        case 0:
            list = lb_npc_move_data.list_0x18;
            spot = lbl_805B8EF0;
            break;
        case 1:
            list = lb_npc_move_data.list_0x1C;
            spot = lbl_805B8F28;
            break;
        case 2:
            list = lb_npc_move_data.list_0x20;
            spot = lbl_805B8F38;
            break;
        }
        break;
    case 21:
        switch (self->field_0x004) {
        case 5:
            list = lb_npc_move_data.list_0x24;
            spot = lbl_805B8F50;
            break;
        case 6:
            list = lb_npc_move_data.list_0x28;
            spot = lbl_805B8F60;
            break;
        case 7:
            list = lb_npc_move_data.list_0x2C;
            spot = lbl_805B8F70;
            break;
        }
        break;
    }

    /* The target's no-match path stores the table base itself as the entry pointer. */
    selected = (LbNpcMotionEntry*)list;
    for (current = spot; current->x < lbl_80799A18; i++, current++) {
        if (current->x > self->pos_0x10.x && current->z > self->pos_0x10.z) {
            selected = list[i];
            break;
        }
    }
    self->field_0x208 = 0;
    self->field_0x204 = selected;
    fn_801FE13C(self, selected[0].motion_0x0C);
}

/* 0x80205978 - set the NPC's secondary action byte and clear its two flag bytes, then set the lobby's
 * second field once. */
void fn_80205978(_LB_NPC* self)
{
    fn_801FDFC8(self, 1);
    self->field_0x001 = 0;
    self->field_0x003 = 0;

    if (self->field_0x006 == 0) {
        self->field_0x006++;
        lobby_w.field_0x077 = 1;
    }
}

/* 0x80205F1C - hand the NPC over to the band above with the kind its own query answers. */
void fn_80205F1C(_LB_NPC* self)
{
    s32 kind = fn_80207EAC(self);

    fn_80207FD0(self, (u8)kind, 1);
}

/* 0x80206044 - step the NPC's motion table onto its +0x10 branch and restart the first motion there. */
void fn_80206044(_LB_NPC* self)
{
    self->field_0x204 = self->field_0x204[self->field_0x208].field_0x10;
    self->field_0x208 = 0;
    fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
}

/* 0x80207698 - restart motion 67 when the NPC's own flag byte is set, otherwise re-arm motion 1 over
 * 4 frames. */
void fn_80207698(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (self->field_0x264 != 0) {
        fn_801FE13C(self, 67);
    } else {
        fn_801FE0AC(self, 1, 4, 0);
    }
}

/* 0x80205F5C - play motion 10 and arm 1131, then restart the table's motion. */
void fn_80205F5C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1131, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80205FEC - set the NPC's action byte and arm motion 54 over 4 frames, once. */
void fn_80205FEC(_LB_NPC* self)
{
    fn_801FDFC0(self, 1);

    if (self->field_0x006 == 0) {
        self->field_0x006++;
        fn_801FDFE4(self, 54, 4, 0);
    }
}

/* 0x80206074 - the NPC's three-arm "walk a set route" machine: it aims the NPC's position vector at the
 * route's first point with the heading latched for 95 frames, then at the second point for 80, then
 * restarts the route. */
void fn_80206074(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1115, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A24, lbl_80799A28, lbl_80799A2C);
        self->field_0x02C = (u16)-8192;
        self->field_0x00C = 95;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1121, 0, 0);
            setVector3(&self->pos_0x10, lbl_80799A30, lbl_80799A28, lbl_80799A34);
            self->field_0x00C = 80;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1115, 0, 0);
            setVector3(&self->pos_0x10, lbl_80799A24, lbl_80799A28, lbl_80799A2C);
        }
        break;
    }
}

/* 0x8020618C - aim the NPC's position at the third route point, latch its heading for 449 frames and
 * then arm motion 1178. */
void fn_8020618C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1177, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A38, lbl_80799A3C, lbl_80799A40);
        self->field_0x02C = (u16)-7281;
        self->field_0x00C = 449;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1178, 0, 0);
        }
        break;
    }
}

/* 0x802059D8 - the NPC's three-arm "pace on the spot" machine: hold motion 1177 for a random 0..8
 * frame wait, then 1178, then restart 1177 with the 92-frame delay. */
void fn_802059D8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 8;
        fn_801FE0AC(self, 1177, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1178, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 8;
            fn_801FDFE4(self, 1177, 4, 92);
        }
        break;
    }
}

/* 0x80205AD4 - the NPC's three-arm "pace" machine: hold motion 1123 for a random 0..3 frame wait, then
 * 1124, then restart 1123 with the 130-frame delay. */
void fn_80205AD4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FE0AC(self, 1123, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1124, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
            self->field_0x00C = (u16)ran_suu(1) & 3;
            fn_801FDFE4(self, 1123, 4, 130);
        }
        break;
    }
}

/* 0x80205BD0 - the NPC's five-arm "hand over" machine: hold motion 1197 for a random 0..3 frame wait,
 * then 1194 (same wait), then 1196, then 1195, then back to the start. */
void fn_80205BD0(_LB_NPC* self)
{
    fn_801FE128(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FDFE4(self, 1197, 20, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                self->field_0x00C = (u16)ran_suu(1) & 3;
                fn_801FDFE4(self, 1194, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1196, 4, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1195, 4, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
        }
        break;
    }
}

/* 0x80205D54 - the NPC's four-arm "settle in" machine: arm 1115 with a random 0..14 frame wait, then
 * 1193 over 8 frames with a coin-flip sign, then re-arm 1193 while the player's approach is not over,
 * then step the counter down and restart the band's arms. */
void fn_80205D54(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1115, 4, 0);
        self->field_0x00C = (u16)ran_suu(1) % 15;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1193, 8, 0);
            self->field_0x00C = (u16)ran_suu(1) % 2;
        }
        break;
    case 2:
        if (self->field_0x00C > 0) {
            if (fn_801FE1CC(self, 0, lbl_807999B0, lbl_807999A0) == 1) {
                fn_801FDFE4(self, 1193, 4, 0);
                self->field_0x00C--;
            }
        } else if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 0x9D;
            if (((u16)ran_suu(1) & 1) != 0) {
                fn_801FDFE4(self, 1121, 16, 0);
            } else {
                fn_801FDFE4(self, 1129, 8, 0);
            }
        }
        break;
    case 3:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006 = 1;
        }
        break;
    }
}

/* 0x80206DE8 - arm motion 1121 onto the route's second point with the heading latched, then motion 1129
 * once the player's approach is over. */
void fn_80206DE8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1121, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AAC, lbl_80799A28, lbl_80799AB0);
        self->field_0x02C = (u16)-1819;
        break;
    case 1:
        if (fn_801FE1CC(self, 0, lbl_80799AB4, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
        }
        break;
    }
}

/* 0x80206E9C - arm motion 1194 onto the route's third point, hold the heading for 210 frames, then arm
 * motion 1129. */
void fn_80206E9C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1194, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AB8, lbl_80799A28, lbl_80799ABC);
        self->field_0x02C = (u16)-909;
        self->field_0x00C = 210;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 8, 0);
        }
        break;
    }
}

/* 0x80206F4C - arm motion 1193 onto the route's fourth point, hold the heading for 215 frames, then arm
 * motion 1194. */
void fn_80206F4C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1193, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AC0, lbl_80799A28, lbl_80799AC4);
        self->field_0x02C = 5461;
        self->field_0x00C = 215;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
        }
        break;
    }
}

/* 0x80206FF8 - the route walk's turn-around: aim the NPC at the route's fifth point for 168 frames,
 * then motion 1121 for 72, then motion 1194. */
void fn_80206FF8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1194, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AC8, lbl_80799A28, lbl_80799ACC);
        self->field_0x02C = 16384;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1121, 4, 0);
            self->field_0x00C = 72;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
        }
        break;
    }
}

/* 0x802070E4 - the route walk's turn: aim the NPC at the route's sixth point for 168 frames, then
 * motion 1194 for 57, then motion 1129. */
void fn_802070E4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1121, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AD0, lbl_80799A28, lbl_80799AD4);
        self->field_0x02C = 14564;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
            self->field_0x00C = 57;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
        }
        break;
    }
}

/* 0x802071D0 - the route walk's last turn: aim the NPC at the route's seventh point for 168 frames and
 * then hold motion 1129 for 72. */
void fn_802071D0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1129, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AD8, lbl_80799A28, lbl_80799ADC);
        self->field_0x02C = 13653;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
            self->field_0x00C = 72;
        }
        break;
    }
}

/* 0x802069D0 - the NPC's three-arm "walk to the meeting point" machine: motion 1 onto the meeting
 * point for 470 frames, then 1046 over 18 frames (clearing its flag byte each frame) and 1045. */
void fn_802069D0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A88, lbl_80799A5C, lbl_80799A8C);
        self->field_0x02C = 12379;
        self->field_0x00C = 470;
        self->field_0x001 = 0;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1046, 18, 0);
            self->field_0x00C = 79;
        } else {
            self->field_0x001 = 0;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1045, 18, 0);
        }
        break;
    }
}

/* 0x80206CB4 - the NPC's four-arm walk to the meeting point: motion 1 onto the point for 540 frames
 * (clearing its flag byte), then 1084, then 1085, then 1077 with the 74-frame delay. */
void fn_80206CB4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AA4, lbl_80799A5C, lbl_80799AA8);
        self->field_0x02C = (u16)-17202;
        self->field_0x00C = 540;
        self->field_0x001 = 0;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1084, 0, 0);
        } else {
            self->field_0x001 = 0;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1085, 6, 0);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1077, 4, 74);
        }
        break;
    }
}

/* 0x80204528 - the NPC's five-arm "turn to face" machine: arm 1170, then 1171 (latching the turn for
 * one frame) or, while 1170 still plays, start a 15-tick turn to angle 0xC001 and latch it; then step
 * the motion counter down, play 1172, and finally either restart the move table or aim over 11 ticks. */
void fn_80204528(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1170, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1171, 0, 0);
            self->field_0x00C = 1;
        } else if (fn_801FE1CC(self, 0, lbl_807999EC, lbl_807999A0) != 0) {
            fn_801FDEB4(self, 0xC001, 15);
            self->field_0x218 = self->field_0x02C;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
            }
        }
        break;
    case 3:
        if (fn_801FE1CC(self, 1, lbl_807999F0, lbl_807999A0) != 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1172, 0, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDEB4(self, (u16)-909, 10);
            fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
        } else if (fn_801FE1CC(self, 0, lbl_807999F4, lbl_807999A0) != 0) {
            fn_801FDEE4(self, 11);
        }
        break;
    }
}

/* 0x80206824 - the NPC's six-arm "walk the meeting-point route" machine: motion 1012 onto the first
 * point for 123 frames, then 1012 over 24, 1012 over 16, 1013 over 40 with the 141-frame latch, 1009
 * with the 292-frame delay, and 1010 over 6. */
void fn_80206824(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1012, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A74, lbl_80799A78, lbl_80799A7C);
        self->field_0x02C = 0;
        self->field_0x00C = 123;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1012, 24, 0);
        }
        break;
    case 2:
        if (fn_801FE1CC(self, 0, lbl_80799A80, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1012, 16, 0);
        }
        break;
    case 3:
        if (fn_801FE1CC(self, 0, lbl_80799A84, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1013, 40, 6);
            self->field_0x00C = 141;
        }
        break;
    case 4:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1009, 0, 292);
        }
        break;
    case 5:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1010, 6, 0);
        }
        break;
    }
}

/* 0x80205764 - the NPC's "keep station on the move work's own record" machine: it copies the record's
 * vector into its target, marks its two flags, then walks between the 80/200/400-unit distance bands,
 * turning 10% of the heading error onto its own heading each tick. */
void fn_80205764(_LB_NPC* self)
{
    LbNpcMoveWorkEntry* work;
    f32 near;
    f32 far;
    f32 dist;

    work = (LbNpcMoveWorkEntry*)get_move_work_adrs(2);
    copyVec3(&self->target_0x1EC, &work[(s8)my_player_no()].vec_0x3C);
    self->field_0x22C = 1;
    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1091, 4, 0);
        break;
    case 1:
        near = lbl_80799A10;
        far = lbl_80799A1C;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1095, 4, 0);
        } else if (dist >= far * far) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 1092, 4, 0);
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    case 2:
        far = lbl_80799A20;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist <= far * far) {
            self->field_0x006 = 0;
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    case 3:
        far = lbl_80799A20;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist >= far * far) {
            self->field_0x006 = 0;
        }
        break;
    }
}

/* 0x802050AC - the NPC's "find a move table entry" machine: arm 1098 and mark its flag byte, then on
 * the lobby's flag either restart motion 55 or pick a table entry for the current part (with two map-22
 * kinds gated on the shop's own state), then aim the NPC 1/5 of the way onto its heading each frame. */
void fn_802050AC(_LB_NPC* self)
{
    LbNpcMotionEntry* entry;
    u8 kind = 0xFF;
    s32 skip = 0;
    s32 turn;

    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1098, 4, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            if (lobby_w.field_0x076 == 1) {
                fn_801FE13C(self, 55);
            } else if ((u8)get_now_mapno() == 22) {
                entry = fn_80207DC4(self, &kind);
                if ((u8)get_now_mapno() == 22 && self->field_0x004 == 2) {
                    switch ((u32)kind) {
                    case 1:
                        if (fn_802FB8EC(2) == 0) {
                            skip = 1;
                        }
                        break;
                    case 5:
                        if (fn_802FB8EC(3) == 0) {
                            skip = 1;
                        }
                        break;
                    }
                }
                if (entry != 0 && skip == 0) {
                    self->field_0x208 = 0;
                    self->field_0x204 = entry;
                    fn_801FE13C(self, entry[0].motion_0x0C);
                } else {
                    self->field_0x208 = 0;
                    self->field_0x204 = &lbl_805B8EC8;
                    fn_801FE13C(self, 48);
                }
            } else {
                self->field_0x208 = 0;
                fn_801FE13C(self, self->field_0x204[self->field_0x208].motion_0x0C);
            }
        } else {
            turn = (s16)(u16)fn_801FDE3C(self, &self->field_0x214->vec_0x3C) / 5;
            self->field_0x02C += turn;
        }
        break;
    }
}

#pragma peephole on

} /* extern "C" */

} /* namespace s_802029B4 */

/* ==== absorbed from lobby/fn_802076D4.cpp (0x802076D4..0x8020C4C8) ==== */
#define EF_FN_800CDB2C_NO_RAN_SUU 1
/* lobby/fn_802076D4.cpp - the lobby NPC/character act layer.
 *
 * `.text` 0x802076D4..0x8020C588 (82 functions, 20148 B), extab 0x80010F7C..0x80011184 and extabindex
 * 0x8002D624..0x8002D930 (both 65 records, abutting the bracketing registered units
 * `lobby/lb_npc.cpp` and `lobby/fn_80212810.cpp`).  Registered once, at its final home
 * (docs/plan.md 12), from proposal/802076D4_fn_802076D4.cpp.
 *
 * Module and language.  `lobby`: the range reads `lobby_w`/`lb_npc_move_data`/`lb_param_w`, calls the
 * neighbours' `fn_801FExxx`/`fn_801FDE3C` and the lobby UI helpers (`LbStr`, `get_lsp_data`), and both
 * bracketing registered units are `lobby`.  C++: the range's undefined set is full of manglings
 * (`Pl_chr_setX__FP4_PLWUsll`, `calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`, ...).
 *
 * Name.  No `__FILE__` string covers the range (the `.rodata`/`.data` pools were scanned for a bare
 * source-file name: the only one that could be argued for is `enemy_control.cpp` at 0x805A1BB8, and it
 * is never referenced anywhere in this band - the discovery seam note `one source file
 * (enemy_control.cpp)` comes from `attribute.py:source_owner`, which returns *the latest accepted
 * source name at or before the cut*, and that is the last `__FILE__` string before this band) and
 * `dumpmap.py lookup` answers only `zz_` placeholders, so the file keeps the map stem (brief section 2,
 * class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with tools/symbols/symedit.py at 0x802076D4 for the inventory and python tools/symbols/dumpmap.py lookup on all 82 rows - all of them answer zz_0207xxx_)
 *
 * Seam.  Unproven: the range edges are `dtk` pool-run cuts and the interior ones were joined by
 * `owner_merge` on the artefact above, so this may be more than one file.  `fn_80207698`
 * (0x80207698..0x802076D4) is tail-called from `fn_80207A44`'s act dispatch with the same four act
 * handlers this range defines, which is the strongest hint that the real seam is 0x80207698.
 *
 * Two record types.  The range's functions take one of two records and both are already reconstructed:
 *   - `_LB_NPC` (include/lobby/lb_npc.h, 0x268 B) for the act-state functions - `fn_802076D4` reads the
 *     VEC3 at +0x10 and the VEC3 at +0x1EC, and `_LB_NPC` is the only view with both.
 *   - `_PLW` (include/pl.h, 0xB20) for the ones that drive the player work and reach past +0x268
 *     (`fn_8020A3E4` walks +0x322, `fn_80208928` memcmps the name at +0xB05, most call
 *     `Pl_chr_setX`/`Pl_master_ck`, whose map parameter type is `_PLW*`).
 *
 * Residuals (measured with `tools/units/recompile.py lobby/fn_802076D4.cpp --measure <symbol>`):
 *   - the rows below the 80 % bar are listed in the campaign outbox, not here.
 *   - the unwritten rows are absent, not stubbed, so a re-measure reports them as 0 % (`objdiff` pairs
 *     by symbol name) - the next session continues at 0x80207B3C in address order.
 */
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "pl.h"

#include "ef/fn_800CDB2C.h"      /* my_player_no */
#include "unsplit/Pl.h"          /* Get_motion_no, Pl_chr_setX, Pl_frame_check */
#include "unsplit/lobby.h"       /* LbStr, lbl_806AA6F0, lbl_80799Bxx */
#include "Network/network_pat_control.h" /* checkOtherInvite (rule 2: its owner's header) */
#include "Pl/Pl_master_ck.h"        /* Pl_master_ck, Pl_act_ck, fn_8026FD0C */
#include "Pl/fn_802693C4.h"
#include "ef/eft004.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
namespace s_802076D4 {


extern "C" int memcmp(const void* a, const void* b, u32 n);
} /* namespace s_802076D4 */


/* ---------------------------------------------------------------------------------------------------
 * The callees this batch needs.  A `fn_XXXXXXXX` stem is the map's own placeholder (not a mangling), so
 * those are `extern "C"`; a name the map spells with an argument list is the real C++ declaration.
 */
void* get_move_work_adrs(u8 kind);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
namespace s_802076D4 {

extern "C" int memcmp(const void* a, const void* b, u32 n);

extern "C" {
__declspec(noinline) u32 fn_80208AB8(void);
__declspec(noinline) u32 fn_80208AC0(_PLW* self);
__declspec(noinline) u32 fn_80208AE8(_PLW* self);
void fn_8020A3E4(_PLW* self, u32 a, s32 b, s32 c);
__declspec(noinline) void fn_8020A5D4(_PLW* self, u32 a, s32 b, s32 c);
__declspec(noinline) void fn_8020A5EC(_PLW* self, u8 id);
void fn_8020A5F4(_PLW* self, u32 a, s32 b, s32 c, u16 d);
__declspec(noinline) void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c);

/* The act handlers of the unit before this range (the seam at 0x80207698 is unproven, so they are the
 * neighbour's). */
void fn_80206044(_LB_NPC* self);
void fn_8020623C(_LB_NPC* self);
void fn_80207284(_LB_NPC* self);
void fn_80207698(_LB_NPC* self);

/* The Pl-band helpers, and this group's own neighbours. */
u32 Pl_motion_end_ck(_PLW* self);
u32 game_ready_ck(void);
u32 fn_8021B8A8(u8 a, u8 b);
u32 fn_8021B8F4(u8 a, u8 b);
void fn_801FDEE4(_LB_NPC* self, u32 motion);
u32 fn_801FDFD0(_LB_NPC* self);
void fn_801FE0AC(_LB_NPC* self, u16 motion_id, s32 a, s32 b);
void fn_801FE13C(_LB_NPC* self, u16 motion_id);
u32 fn_801FE1EC(_LB_NPC* self);
u32 fn_801FE32C(_LB_NPC* self, u16 id);
void fn_801FE200(_LB_NPC* self, u16 a, u16 b);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
}

/* The map spells this range's symbols `fn_XXXXXXXX` (placeholders, not manglings), so every definition
 * below is `extern "C"` - a C++ definition would mangle and pair nothing (playbook 42/48). */
#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------------------
 * 0x802076D4 - the first act-state handler: face the move work's target vector, then run the close/far
 * distance ladder that picks the next state.
 */
void fn_802076D4(_LB_NPC* self)
{
    u8* work;

    work = (u8*)get_move_work_adrs(2);
    work += (s8)my_player_no() * 0xB20;
    if (lobby_w.field_0x12C == 1) {
        return;
    }
    copyVec3(&self->target_0x1EC, (VEC3*)(work + 0x3C));
    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0: {
        f32 near;

        self->field_0x006++;
        near = lbl_80799B18;
        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1, 4, 0);
        } else {
            fn_801FE0AC(self, 8, 4, 0);
        }
        break;
    }
    case 1: {
        f32 near = lbl_80799B18;
        f32 far = lbl_80799B1C;
        f32 d = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);

        if (d <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1, 4, 0);
        } else if (d >= far * far) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 11, 4, 0);
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    }
    case 2: {
        f32 far = lbl_80799B20;

        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) <= far * far) {
            self->field_0x006 = 0;
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    }
    case 3: {
        f32 far = lbl_80799B20;

        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) >= far * far) {
            self->field_0x006 = 0;
        }
        break;
    }
    }
}

/* 0x80207938 - the second act-state handler: zero a scratch vector, play motion 10, wait for the
 * motion/talk predicates, then clear the action and the +0x208 word. */
__declspec(noinline) void fn_80207938(_LB_NPC* self)
{
    VEC3 scratch;

    VEC3_ctor(&scratch);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006 = 1;
        fn_801FE0AC(self, 2, 4, 0);
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 1, 4, 0);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x208 = 0;
            fn_801FE13C(self, 0);
        }
        break;
    }
}

/* 0x80207A1C - the third act-state handler: arm motion 22 once and hand over. */
__declspec(noinline) void fn_80207A1C(_LB_NPC* self)
{
    if (self->field_0x006 == 0) {
        self->field_0x006++;
        fn_801FE0AC(self, 22, 4, 0);
    }
}

/* 0x80207A44 - dispatch the current action id (+0x1CC) onto its per-action handler. */
void fn_80207A44(_LB_NPC* self)
{
    switch (self->field_0x1CC) {
    case 0:
        fn_80207698(self);
        break;
    case 67:
        fn_802076D4(self);
        break;
    case 7:
        fn_80207938(self);
        break;
    case 68:
        fn_80207A1C(self);
        break;
    case 65:
        fn_80206044(self);
        break;
    case 71:
        fn_8020623C(self);
        break;
    case 82:
        fn_80207284(self);
        break;
    }
}

/* 0x80207AA4 - arm the lobby sub-scene latch block for a scene id. */
void fn_80207AA4(u8 scene)
{
    memset(lbl_806AA6F0, 0, 28);
    lbl_806AA6F0[16] = scene;
    lbl_806AA6F0[2] = 1;
}

/* 0x80207AF8 - whether the latch block is armed and its +3 byte is set. */
u32 fn_80207AF8(void)
{
    if (lbl_806AA6F0[2] == 0) {
        return 1;
    }
    return lbl_806AA6F0[3] != 0;
}

/* 0x80207B28 - release the latch block. */
void fn_80207B28(void)
{
    lbl_806AA6F0[17] = 0;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80207E9C and the two small id decoders.
 */
__declspec(noinline) void fn_80207E9C(_LB_NPC* self, u8 state)
{
    self->field_0x234[0] = 1;
    self->field_0x234[1] = state;
}

__declspec(noinline) u32 fn_80207EAC(_LB_NPC* self)
{
    u32 value = 0;

    switch (self->field_0x002) {
    case 20:
        value = 0;
        break;
    case 21:
        value = 1;
        break;
    case 22:
        value = 2;
        break;
    }
    return value;
}

__declspec(noinline) u32 fn_802080C0(_LB_NPC* self)
{
    u32 value = 0;

    switch (self->field_0x002) {
    case 10:
        value = 0;
        break;
    case 11:
        value = 1;
        break;
    case 12:
        value = 2;
        break;
    }
    return value;
}

/* ---------------------------------------------------------------------------------------------------
 * The 0x80208928..0x80208B48 block: the lobby-act latch (`_PLW+0xB00`) gate.
 */

/* 0x80208928 - the gate the lobby act handlers test before taking over the player work: same area as
 * the scene, no master act, the talk predicates clear, and the hunter name equal to the move work's. */
u32 fn_80208928(_PLW* self)
{
    u8* work;

    if (self->area_0x16 != lobby_w.area_0x002) {
        return 0;
    }
    if (Pl_master_ck(self) != 0) {
        return 1;
    }
    if (fn_8021B8F4(self->kind_0x015, self->area_0x16) == 1) {
        return 0;
    }
    if (fn_8021B8A8(self->kind_0x015, self->area_0x16) != 1) {
        return 1;
    }
    work = (u8*)get_move_work_adrs(2);
    work += (s8)my_player_no() * 0xB20;
    if (memcmp(self->name_0xB05, work + 0xB05, 10) != 0) {
        return 0;
    }
    return 1;
}

/* 0x802089F4 - the id range the lobby act layer scans. */
__declspec(noinline) u16 fn_802089F4(s32 id)
{
    return 0xFFFF;
}

/* 0x80208A00 - latch the act step when the master act is running. */
__declspec(noinline) void fn_80208A00(_PLW* self)
{
    if (Pl_master_ck(self) == 1) {
        self->field_0xB00 = 2;
    }
}

/* 0x80208A3C - clear the act latch. */
__declspec(noinline) void fn_80208A3C(_PLW* self)
{
    self->field_0xB00 = 0;
}

/* 0x80208A48 - clear the act latch when the decoder says the id is free. */
__declspec(noinline) void fn_80208A48(_PLW* self)
{
    if (fn_80208AC0(self) == 1) {
        self->field_0xB00 = 0;
    }
}

/* 0x80208A84 - whether the act layer is idle: an unknown id and a clear +0x30E byte. */
__declspec(noinline) u32 fn_80208A84(_PLW* self)
{
    int id = self->field_0x00A;

    if (!((u32)(id - 11) <= 1 || id == 9)) {
        return self->flag_0x30E != 0;
    }
    return 0;
}

/* 0x80208AB8 - always "no". */
__declspec(noinline) u32 fn_80208AB8(void)
{
    return 0;
}

/* 0x80208AC0 - whether the act latch holds the "armed" step for the lobby act id. */
__declspec(noinline) u32 fn_80208AC0(_PLW* self)
{
    if (self->field_0x00A != 12) {
        return 0;
    }
    return self->field_0xB00 == 2;
}

/* 0x80208AE8 - whether the act latch holds the "taken over" step. */
__declspec(noinline) u32 fn_80208AE8(_PLW* self)
{
    return self->field_0xB00 == 3;
}

/* ---------------------------------------------------------------------------------------------------
 * The 0x8020A5xx act-family entry points.
 */

/* 0x8020A5D4 - set the act latch byte at +0xE and re-enter the family's main handler. */
__declspec(noinline) void fn_8020A5D4(_PLW* self, u32 a, s32 b, s32 c)
{
    self->act_state_0x00E[0] = 1;
    fn_8020A3E4(self, (u8)a, (u16)b, (u16)c);
}

/* 0x8020A5EC - store the act id byte. */
__declspec(noinline) void fn_8020A5EC(_PLW* self, u8 id)
{
    self->kind_0x09 = id;
}

/* 0x8020A70C - the five-argument form of the family's main handler, with the trailing word clear. */
__declspec(noinline) void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c)
{
    fn_8020A5F4(self, a, b, c, 0);
}

/* 0x8020A5F4 - the act-family handler: arm the act id, set the +0x30E flag and select the motion
 * from the latch state. */
void fn_8020A5F4(_PLW* self, u32 id, s32 arg, s32 speed, u16 flags)
{
    if (speed > 0) {
        speed = -speed;
    }
    self->kind_0x09 = (u8)id;
    self->flag_0x30E = 1;
    if (fn_80208AB8() != 0 || fn_80208AC0(self) != 0) {
        Pl_chr_set_attr_default(self, 51, speed, arg);
        fn_8020A5D4(self, 12, 0, flags);
    } else if (fn_80208AE8(self) == 1) {
        Pl_chr_set_attr_default(self, 21, speed, arg);
        fn_8020A5D4(self, 0, 0, flags);
    } else {
        Pl_chr_set_attr_default(self, 1, speed, arg);
        fn_8020A5D4(self, 0, 0, flags);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The 0x8020AD14..0x8020B264 act state machines: wait for the frame predicate, then hand the motion to
 * the player character setter.
 */

/* 0x8020AD14 - */
void fn_8020AD14(_LB_NPC* self)
{
    switch (self->field_0x005) {
    case 0: {
        u8 index;

        self->field_0x005 = 1;
        fn_8020A5EC((_PLW*)self, 0);
        Pl_chr_setX((_PLW*)self, 610, 4, 0);
        if (Pl_master_ck((_PLW*)self) == 1 && game_ready_ck() == 1) {
            index = self->field_0x182;
            checkOtherInvite(index + 1, (s8*)&lobby_w.param_0x12F);
        }
        break;
    }
    case 1:
        if (Pl_motion_end_ck((_PLW*)self) == 1) {
            fn_8020A70C((_PLW*)self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020AEF0 - */
void fn_8020AEF0(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0: {
        u32 timer;

        self->act_step_0x05 = 1;
        fn_8020A5EC(self, 0);
        timer = self->field_0x058 + 0x18000;
        self->field_0x058 = timer & 0xFFFF;
        self->field_0x0A8 = timer & 0xFFFF;
        Pl_chr_setX(self, 329, 0, 0);
        break;
    }
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 6, 0);
        }
        break;
    }
}

/* 0x8020AF90 - */
void fn_8020AF90(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05 = 1;
        Pl_chr_setX(self, 602, 4, 0);
        fn_8020A5EC(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_chr_setX(self, 609, 6, 0);
            fn_8020A3E4(self, 0, 4, 32);
        }
        break;
    }
}

/* 0x8020B264 - */
void fn_8020B264(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05 = 1;
        Pl_chr_setX(self, 207, 2, 0);
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

#ifdef __cplusplus
}
#endif

} /* namespace s_802076D4 */

/* ==== absorbed from lobby/fn_8020C588.cpp (0x8020C588..0x80212760) ==== */
#include "menu/menu_message.h"
/* C++-linkage declarations of this section: they stay at global scope (their manglings are made there). */
void* LbStr(u8 kind, u16 idx);
s32 get_fade_stat(s32 slot);
/* lobby/fn_8020C588.cpp - the lobby player-character control band.
 *
 * `.text` 0x8020C588..0x80212810 (112 functions, 25224 B), extab 0x80011184..0x80011434 (86 records),
 * extabindex 0x8002D930..0x8002DD38 (86 x 12 B) and one `.data` jump table
 * `jumptable_805B9770` 0x805B9770..0x805B97BC (19 words, the arms of `fn_80212760`'s switch - the
 * table at 0x805B96D8 in front of it is `fn_80211E68`'s and is unclaimed while that body is not
 * written).  Registered from `proposal/8020C588_fn_8020C588.cpp`.
 *
 * Module `lobby`: both registered units bracketing the range in the address band are `lobby`
 * (`lobby/lb_npc.cpp` ends at 0x802029B4, `lobby/fn_80212810.cpp` starts at 0x80212810), and this
 * range both **defines** `LbStr__FUcUs` - the lobby string helper `include/unsplit/lobby.h` declares
 * and `lobby/fn_801E7530.cpp`/`fn_80212810.cpp` call - and reads `lobby_w` and the lobby UI tables.
 *
 * Name.  No `__FILE__` string is reachable from the range and the runtime dump answers only
 * `zz_XXXXXXXX_` placeholders, so the file keeps the map's `fn_8020C588` stem (brief section 2,
 * classes 3+4).  Siblings: `lobby/fn_80212810.cpp`, `lobby/fn_8021E1EC.cpp`.
 *
 * Seam.  Unproven (this is one maximal unclaimed run).  The left edge 0x8020C588 is a proposal
 * boundary, not a TU boundary; the range's extab/`extabindex` runs agree with the right edge exactly
 * (86 records, `fn_8020C588` first, `fn_8021261C` last, and the run ends where
 * `lobby/fn_80212810.cpp`'s extab begins at 0x80011434).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `dumpmap.py lookup` over the range's inventory: every name but `LbStr__FUcUs` and
 * `glplatTextureGetHeight` is a bare `.text` entry in config/RMHE08/symbols.txt and the runtime dump
 * has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Residuals (official report metric; the bar is 80 %):
 *  - 21 of the 112 functions are reconstructed and every one of them is 100.000000 (1660 / 25224
 *    `.text` bytes, 6.58 % of the unit; the `.data` table above is 100 % too).  Measured with
 *    `build/tools/objdiff-cli.exe diff -p . -u main/lobby/fn_8020C588` against this worktree's own
 *    split target object (MAIN has no `obj/lobby/fn_8020C588.o`, so `recompile.py --measure` falls
 *    back to the retired single-symbol objects and cannot pair these).
 *  - the other 91 functions are unwritten.  Every one of them takes the shared `_PLW` player record
 *    and drives it with `Pl_act_ck`/`Pl_master_ck`/`Pl_frame_check`, so the blocker is `include/pl.h`,
 *    which still spells the offsets this range reads `pad_*`/`unk*` (+0x004/+0x005/+0x006 state,
 *    +0x028 timer, +0x03C/+0x040/+0x044 floats, +0x0B4/+0x0B6, +0x30E, +0x313, +0x354,
 *    +0x656/+0x657, +0x265..0x267, +0x5C8).  Naming them in `pl.h` is the sanctioned ``wave 2'' work
 *    (the header says so) but it renames fields `src/Pl/pl_master.cpp` and
 *    `src/sound/fn_800EF7D8.cpp` already read, so it is a config_request, not this batch's edit.  The
 *    enforce-lint would otherwise flag every `unk` member access in this file (rule 7's unk half is
 *    not covered by the `Naming note` line above).
 *  - `fn_80212370` (352 B) reads the `Psw` pad record's 0x2C0..0x2DF bytes; the only `PlayerPad`
 *    definition lives in `src/mh3_pad.cpp` (rule 1: a shared type in one header), so naming them here
 *    would copy it.  Config_request: move `PlayerPad` into `include/mh3_pad.h`.
 *  - `fn_80211E68` (504 B, 0x80211E68) is the range's other jump-table switch and needs no `_PLW`
 *    field, only `Pl_act_ck(_PLW*, u8, u16)` + `my_player_work_get()` passed straight through.  Its arms are
 *    in the DOL's table order `0, 28|33|37, 27, 7, 17, 23, 24, 2, 21, 22, 3, 15, 4, 11, 5, 6, 8, 12,
 *    29, 13, 14, 16, 18, 19, 10, 20, 26, 30, 31, 32, 34, 25, 35` (read out of `jumptable_805B96D8`),
 *    and cases 1, 9 and 36 fall straight through to the end.  Left for the next session rather than
 *    half-written, because it needs 35 callee declarations that belong in other units' headers.
 *  - largest unwritten, largest first: fn_8020EE14 1840 B, fn_8020FE18 1428 B, fn_8020F880 764 B,
 *    fn_8020F544 680 B, fn_8020EBF4 544 B, fn_80211E68 504 B, fn_8020D1FC 488 B.
 *
 * Flags.  The unit needs the auto-inliner and the peephole pass off, and both are file-scoped pragmas
 * (neither setting is one the other `lobby` lib units want): with `-inline auto` MWCC inlined the
 * 220 B `fn_80212060` into each of its five callers (fn_8021213C came out 276 B against a 92 B
 * target, 0 %), and with the peephole on it folds fn_80211DCC's `clrlwi` + `slwi` into one `rlwinm`
 * (16 B against 20).  `#pragma inline off`, `#pragma inline_depth 0` and `#pragma dont_inline on` all
 * work; `dont_inline on` is the one kept.  Measured alternative: `-inline noauto` on `cflags_lobby`
 * fixes this unit too (2.71 -> 5.98 % before the bodies below existed) and moves four other lobby
 * units up (fn_801E7530 27.65 -> 31.76, lb_npc 13.37 -> 13.75, fn_8021E1EC 9.22 -> 9.66) with
 * `lobby_scene` unchanged at 100 - recorded as a config_request rather than applied, since it is a
 * lib-wide change.
 */
#include "types.h"


/* The range was built with the auto-inliner and the peephole pass off: retail keeps the `bl
 * fn_80212060` the accessors below make (with `-inline auto` MWCC inlines its 220-byte body into each
 * of them - fn_8021213C came out 276 B instead of 92) and keeps the unfused `clrlwi` + `slwi` of the
 * species-id index in fn_80211DCC (the peephole folds it into one `rlwinm`).  Both are file-scoped
 * pragmas because neither setting is something the rest of the `lobby` lib needs: `-inline noauto` for
 * the lib does fix this unit, but it also moves four other lobby units' numbers and the pragma keeps
 * the change inside this unit. */
#pragma dont_inline on
#pragma peephole off
namespace s_8020C588 {
#include "lobby/fn_8021213C.h"
#include "lobby/lb_npc_callees.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "lobby/lb_equip_page.h"



/* The four species-id string tables, indexed by the id byte (`fn_80211DCC`..`fn_80211E08`). */
extern "C" s32 fn_80211DCC(u32 id) {
    return lb_chacha_skill_str[(u8)id];
}

extern "C" s32 fn_80211DE0(u32 id) {
    return lb_chacha_mask_str[(u8)id];
}

extern "C" s32 fn_80211DF4(u32 id) {
    return lb_cat_name[(u8)id];
}

extern "C" s32 fn_80211E08(u32 id) {
    return lb_pig_name[(u8)id];
}
} /* namespace s_8020C588 */


/* Returns group `kind`'s string `idx`, or group 1's first string when either index is out of range. */
void* LbStr(u8 kind, u16 idx) {
    using s_8020C588::lb_str_tbl; using s_8020C588::lbl_805B9450;
    if ((u32)kind >= 5 || (s32)idx >= lbl_805B9450[kind]) {
        return (void*)lb_str_tbl[1][0];
    }
    return (void*)lb_str_tbl[kind][idx];
}
namespace s_8020C588 {


/* Nonzero while the lobby screen is owned by a transition or a fade. */
extern "C" u32 fn_80212060(void) {
    if (lobby_w.busy_0x172 != 0) {
        return 1;
    }
    if (get_fade_stat(0) == 1 || get_fade_stat(0) == 2) {
        return 1;
    }
    if (get_fade_stat(1) == 1 || get_fade_stat(1) == 2) {
        return 1;
    }
    if (get_fade_stat(2) == 1 || get_fade_stat(2) == 2) {
        return 1;
    }
    if (get_fade_stat(3) == 1 || get_fade_stat(3) == 2) {
        return 1;
    }
    return fn_803768F8();
}

/* The four player-0 command-mask testers; all report "clear" while the screen is not owned. */
extern "C" s32 fn_8021213C(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][0] & mask) != 0;
}

extern "C" s32 fn_80212198(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][1] & mask) != 0;
}

extern "C" s32 fn_802121F4(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][2] & mask) != 0;
}

extern "C" s32 fn_80212250(u16 mask) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][3] & mask) != 0;
}

/* The four player-0 command-mask getters. */
extern "C" u16 fn_802122AC(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][2];
}

extern "C" u16 fn_802122E8(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 glplatTextureGetHeight(void) {
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 fn_80212334(void) {
    if (fn_80212060() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][3];
}

/* Steps the caller's 4/8 sprite stepper over player 0's first mask. */
extern "C" void fn_80212540(void* self) {
    toggle_word_step_dpad(self, fn_802122E8(), 4, 8);
}

/* Steps the caller's 4/8 sprite stepper over the same mask, ignoring the screen guard. */
extern "C" void fn_80212584(void* self) {
    toggle_word_step_dpad(self, glplatTextureGetHeight(), 4, 8);
}

/* Toggles the item database's display byte and redraws the lobby when the pad loop is idle. */
extern "C" void fn_802125C8(void) {
    if (game_ready_ck() == 0) {
        lobby_world_block[0x3E00] ^= 1;
        if (fn_8021F238() == 1) {
            fn_801E9888();
        }
        fn_801E9C58();
        fn_802FF2C0();
        fn_80220114();
    }
}

/* Decays the lobby countdown and ramps the +-0x3C cursor slide, then ticks the frame counter. */
extern "C" void fn_8021261C(void) {
    if (lobby_w.countdown_0x028 != 0) {
        lobby_w.countdown_0x028--;
    }
    if (fn_8021F238() == 0) {
        if (lobby_w.slide_0x034 < 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 < 0x3C) {
            lobby_w.slide_0x034++;
        }
    } else {
        if (lobby_w.slide_0x034 > 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 > -0x3C) {
            lobby_w.slide_0x034--;
        }
    }
    lobby_w.counter_0x030++;
}

/* The index of the first `count` entries whose running total reaches `value`, over s16 weights. */
extern "C" s16 fn_802126E8(s16* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += *table;
        if (value < (u32)sum) {
            break;
        }
        table++;
        i++;
    } while (i < count);
    return i;
}

/* The same walk over byte weights. */
extern "C" s16 fn_80212724(u8* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += table[i];
        if (value < (u32)sum) {
            break;
        }
        i++;
    } while (i < count);
    return i;
}

/* Maps a lobby part kind to its string id and returns the string. */
extern "C" void fn_80212760(u32 kind) {
    u16 str_id;

    switch ((u8)kind) {
    default:
        str_id = 0xF0;
        break;
    case 1:
        str_id = 0xF1;
        break;
    case 2:
        str_id = 0xF2;
        break;
    case 3:
        str_id = 0xF3;
        break;
    case 4:
        str_id = 0xF4;
        break;
    case 5:
        str_id = 0xF5;
        break;
    case 6:
        str_id = 0xF6;
        break;
    case 7:
        str_id = 0xFA;
        break;
    case 8:
        str_id = 0xFB;
        break;
    case 9:
        str_id = 0xFC;
        break;
    case 10:
        str_id = 0xFD;
        break;
    case 11:
        str_id = 0xFE;
        break;
    case 12:
        str_id = 0xF7;
        break;
    case 13:
        str_id = 0xFF;
        break;
    case 14:
        str_id = 0x100;
        break;
    case 15:
        str_id = 0x101;
        break;
    case 18:
        str_id = 0xE8;
        break;
    }
    LbStr(0, str_id);
}

} /* namespace s_8020C588 */
#pragma dont_inline reset
#pragma peephole reset
