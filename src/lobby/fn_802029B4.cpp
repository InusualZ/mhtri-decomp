/* lobby/fn_802029B4.cpp - `.text` 0x802029B4..0x802076D4 (68 functions, 0x4D20 bytes), extab
 * 0x80010DBC..0x80010F7C (56 unwind records), extabindex 0x8002D384..0x8002D624 (56 x 12 B).
 * Registered once, at its final home (docs/plan.md 12), from proposal/802029B4_fn_802029B4.cpp.
 *
 * What it is.  The lobby NPC work band: every function takes the shared `_LB_NPC` record and runs a
 * byte state machine on `_LB_NPC::field_0x006` (0..3), arming one of the NPC's motions through the
 * `lobby/lb_npc.cpp` helper set (`fn_801FDFE4` = set motion + build the MHchar, `fn_801FDEE4`/
 * `fn_801FDF70` = play the motion over the model's own frame, `fn_801FE13C` = restart the motion the
 * NPC's motion table entry names) and waiting on `fn_801FDFD0` (the lobby's field_0x000 flag).  The
 * band also carries the NPC's own steering/position code (the `nw4r` math and `fn_80043EA8`/
 * `fn_80041E40` vector work) and three 0x14-stride switch tables in `.data`.
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
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
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
 * next proposal's first framed function.  `#pragma exceptions on` (the lib's `cflags_lobby` turns
 * exceptions off, and every lobby target object carries extab anyway) is what emits them; see the
 * residual note below.
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
 *   - extab 63.19 %: all 56 records are emitted (the `#pragma exceptions on` above) and paired, and a
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

#pragma exceptions on

#include "lobby/lb_npc.h"
#include "unsplit/lobby.h"

/* ---------------------------------------------------------------------------------------------------
 * Declarations.  The lobby band's helper set is defined in `lobby/lb_npc.cpp`; its header publishes the
 * `_LB_NPC` type this range needs but not these prototypes, so they are declared here and the header
 * corrections are a `shared-file` request in the worker's outbox.  Functions and scalars without a
 * mangling are C linkage; the mangled callees are declared at C++ scope (rule 9).
 */
extern "C" {
void fn_80041E40(VEC3* dst, const VEC3* src);
void fn_80043EA8(VEC3* out);
f32 fn_80050EF4(VEC3* a, VEC3* b);
void fn_80051378(VEC3* out, const VEC3* a, const VEC3* b);
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

u16 lb_npc_Get_motion_no(_LB_NPC* self);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
u32 LbCheckKujiraEvent(void);
void* get_move_work_adrs(u8 idx);
s32 my_player_no(void);
s32 get_now_mapno(void);

/* ---------------------------------------------------------------------------------------------------
 * Bodies
 */
extern "C" {

/* 0x802029B4 - the NPC's `state` machine 0: arm motion 1147, wait for the 0x47C motion to run out and
 * select one of two follow-up arms, then restart the motion the motion table names. */
__declspec(noinline) void fn_802029B4(_LB_NPC* self)
{
    VEC3 unused;
    fn_80043EA8(&unused);

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
    fn_80043EA8(&unused);

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
    fn_80043EA8(&unused);

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

    fn_80043EA8(&offset);
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
            fn_80051378(&delta, &self->field_0x214->vec_0x3C, &offset);
            fn_80041E40(&self->target_0x1EC, &delta);
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

    fn_80043EA8(&offset);

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
    fn_80041E40(&self->target_0x1EC, &work[(s8)my_player_no()].vec_0x3C);
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
