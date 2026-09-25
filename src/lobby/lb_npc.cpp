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
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py lookup
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

/* ---------------------------------------------------------------------------------------------------
 * Data
 */
extern "C" LbNpcUserData* lbl_80794880;  /* .sbss pointer */
extern "C" LbNpcLobbyWork lobby_w;       /* .bss 0x806AAB44 */
extern "C" _LB_NPC lb_npc[0x12];        /* .bss 0x806A7BA0 */
extern "C" f32 lbl_80799880;
extern "C" f32 lbl_80799884;
extern "C" f64 lbl_807998D0;
extern "C" f32 lbl_807998D8;
extern "C" LbNpcSystemWork system_w;   /* .bss 0x806585E0 */
extern "C" u8 lbl_806BF530[];
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

void wii_sysmsg_gen(s32 id, s8* buf, s32 flag);
void put_message_sys(s8* buf, u32 a, u32 b, u32 c);
void put_message_sys_ok_button(void);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
u16 lb_npc_Get_motion_no(_LB_NPC* self);

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
            lbl_80794880->field_0x3E01 = 9;
        } else {
            lbl_80794880->field_0x3E01 = 0xFF;
        }
        break;
    case 0:
        lbl_80794880->field_0x3E01 = 0;
        break;
    default:
        lbl_80794880->field_0x3E01 = 0xFF;
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
    s32 value = lobby_w.field_0x002;

    lobby_w.field_0x079 = (s8)value;
}

} /* extern "C" */

/* 0x801FC8E0 - the talk-NPC data block. */
u8* get_talk_npc_data_ptr(void)
{
    return lobby_w.talk_0x0C0;
}

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

} /* extern "C" */

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
void fn_802D9EA4(void);
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
void fn_80054FE8(void* out, s32 zero);
void fn_80054FAC(void* a, void* b);
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
    if (fn_804276D8() == 1 && lbl_806BF530[3] == 0 && lobby_w.field_0x16F == 0 &&
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
        fn_802D9EA4();
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

    fn_80054FE8(&dst, 0);
    {
        LbResource* res = (LbResource*)ckResourceName((s8*)lbl_80582A68[index].b_0x04);
        if (res != 0) {
            fn_80054FE8(&src, (s32)res->payload_0x44);
            fn_80054FAC(&dst, &src);
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
void fn_80041E40(VEC3* dst, VEC3* src);
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
    fn_80041E40(&self->field_0x01C, &self->pos_0x10);
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
