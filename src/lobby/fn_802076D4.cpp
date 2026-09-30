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
#include "Pl/pl_master.h"        /* Pl_master_ck, Pl_act_ck, fn_8026FD0C */
#include "Pl/fn_802693C4.h"
#include "ef/eft004.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

int memcmp(const void* a, const void* b, u32 n);

/* ---------------------------------------------------------------------------------------------------
 * The callees this batch needs.  A `fn_XXXXXXXX` stem is the map's own placeholder (not a mangling), so
 * those are `extern "C"`; a name the map spells with an argument list is the real C++ declaration.
 */
void* get_move_work_adrs(u8 kind);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
int memcmp(const void* a, const void* b, u32 n);

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
