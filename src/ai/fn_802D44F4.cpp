/*
 * ai/fn_802D44F4.cpp - the 0x802D44F4-0x802DDC04 band (165 functions, 38672 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup + the Dolphin dump map at
 * D:/WiiExperiment/DumpSymbols.zip: every defined symbol in the range is a bare `fn_`/`zz_`
 * entry, and no `__FILE__` string covers it), so the file keeps the map's stem.
 *
 * Seam, module and language:
 *   - the range is where a discovery run was cut by `--max-bytes`, so its edge is a size cap and
 *     not a translation-unit boundary (brief section 1).  `tudiscover at 0x802D44F4` finds no
 *     closure edge, no must-link anchor and no labelled data the range references, so the
 *     boundary is unconstrained and the range is worked as one unit; the registered neighbour
 *     below is `ai/fn_802D0DCC.c` and the next proposal starts at 0x802DDC04.
 *   - the range defines `ai_skill_ck__FP8_AINPC_WUc`, `ai_torch_ck__FP8_AINPC_W`,
 *     `ai_taru_move_ck__FP8_AINPC_W`, `ai_taru_range_ck__FP8_AINPC_W` and `ai_demo_stop_ck__Fv`,
 *     i.e. the AI-NPC state checks of the `ai` module the neighbouring unit also builds, so the
 *     module is `ai`.  Its tail also defines the UI helpers `PutPageArrow__F...` and
 *     `get_rare_color__FUc`, which look like a second original file; that seam could not be
 *     proven from any evidence kind (docs/plan.md 8.3) and is recorded here as a hint instead.
 *   - the language is C++ (every defined name in the range is mangled).  The `fn_XXXXXXXX`
 *     definitions are `extern "C"` (the map name is a placeholder stem, not a mangling); the
 *     `ai_*`/`get_rare_color`/`PutPageArrow` ones are C++ free functions at global scope so the
 *     front-end reproduces the map's own mangling (rule 9).
 *
 * Types: the record the whole AI band drives is `_AINPC_W`, whose shared home is
 * `include/ai/ainpc.h` (main already created it as the union of the `ai` band's accessors); this
 * unit added the offsets its own bodies name to that file - +0x000, +0x172, +0x20C..+0x23E,
 * +0x33A, +0x374, +0x3B0..+0x3BC, +0x3D4..+0x3DC, +0x3F4..+0x414, +0x424, +0x431..+0x438,
 * +0x450, +0x46C, +0x47C, +0x483 - rather than carrying a second copy (rule 1).  The four tuning
 * tables it indexes are declared in `include/unsplit/ai.h` (no registered unit owns them, rule 2).
 *
 * Flags: `cflags_main`, plus `#pragma peephole off` for the whole file - retail keeps the unfused
 * `clrlwi`+`slwi` / `clrlwi`+`cmpwi` forms that the peephole pass folds into `clrlslwi` and a
 * masked compare (playbook 39); turning it back on costs `fn_802D773C` 73.3 -> 100 and
 * `fn_802D7754` 85.3 -> 75.5 (measured).
 *
 * Sections: .text 0x802D44F4..0x802DDC04, extab 0x8001496C..0x80014C64 (95 unwind-only records),
 * extabindex 0x80032C88..0x800330FC, .ctors 0x8056F388 (the static constructor in the range), and
 * .data 0x805D5428..0x805D5460 - `jumptable_805D5428`, the 14-entry switch table MWCC emits for
 * `fn_802D77A0` (playbook 53/56: claim exactly the table's own range, never the band around it).
 * The table was recovered by reading the 14 slot values out of `main.elf` (0 -> 0x802D77C4 etc.),
 * which is what turned the switch's arm mapping into source.
 *
 * Registered NonMatching; the bodies below are the part reconstructed so far, in address order:
 * 48 of 165 functions, 23 byte-identical and 40 at or above the 80 % bar (2.62 % of the unit's
 * bytes; the extab/`.data` sections are still unmatched, so the unit's own percent is 9.91).
 *
 * Residuals:
 *   - `fn_802D7B5C`/`fn_802D7C04`/`fn_802D7C6C` (61.7 / 0 / 19.9): the target's search loop stays a
 *     loop, MWCC unrolls ours - the `for (i = 0; i < N; i++) if (x < table[i+1]) break;` shape is
 *     right (conditions and body match) but the constant trip count is unrolled in our build.
 *   - `fn_802D7B24` (11.4): the two range tests want the raw `subi` result in a 32-bit `cmplwi`;
 *     written that way the compiler still inserts the sign-splitting sequence (`subfic`/`orc`).
 *   - `fn_802D6690` (59.0): the `field_0x33A * 4` index is computed before the table base in retail,
 *     after it in ours.
 *   - `fn_802D6B2C` (74.1) / `fn_802D77DC` (71.7) / `fn_802D7A50` (76.7) / `fn_802D7CE4` (75.7):
 *     register-allocation and scheduling differences only - the instruction stream agrees, the
 *     web order does not.
 *   - `fn_802D7688` (89.3), `fn_802D6A00` (89.0), `fn_802D7464` (99.98): same, instruction counts
 *     equal.
 *   - the remaining 117 functions have no body yet; `fn_802D44F4` alone is 8256 B (21 % of the
 *     range) and is a 2064-instruction compare tree on `ai_get_motion_no()`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/fn_800CDB2C.h"
#include "pl.h"
#include "ai/ainpc.h"
#include "unsplit/ai.h"

/* Retail keeps the unfused `clrlwi`+`slwi` / `clrlwi`+`cmpwi` forms (the peephole pass fuses them
 * into `clrlslwi` and a masked compare), so the unit is compiled with the pass off (playbook 39). */
#pragma peephole off

/* The C++-spelled entry points of this range: their map names carry argument lists, so they are
 * declared at global C++ scope and the front-end reproduces the mangling (rule 9).  `ai_skill_ck`
 * is also called by `src/ai/fn_802CC794.cpp`, which declares it itself (that unit predates this
 * one); this declaration is the range's own. */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);
s32 fn_802D9CE0(struct _AINPC_W* self);
s32 ai_area_ck(struct _AINPC_W* self);

#ifdef __cplusplus
extern "C" {
#endif

/* The unregistered helpers and tables these bodies call or index.  Their addresses are outside this
 * unit's range and owned by nobody yet, so stylelint's rule 2 counts them as an address-band gap
 * (`tools/units/stylelint.py`, "the registered address bands interleave modules"); playbook 29: the
 * table is only ever *declared*, never defined here, and as a **complete** array so the base comes
 * out as `lis`/`addi` like the target's. */
extern u8 lbl_805D4190[0x10];
extern u8 lbl_805D41A0[0x10];
extern u16 lbl_805D41B0[0x10];
extern char* lbl_805D4390[4];
extern u32 lbl_805D4358[7];
extern u16 lbl_805D4560[6];
extern u32 lbl_805D5574[7];
extern f32 lbl_80794B78[2];        /* .sbss:0x80794B78 - its second word is the scale below */

/* The per-monster hold-slot table at 0x805D43A0: 31 records of four u16 (size 0xF8).  Field 0 of a
 * record is the id the scan compares, fields 1..3 the values `fn_802D7BA4`..`fn_802D7BE4` hand out;
 * the records are reached as `lbl_805D43A0[index * 4 + field]` because that is the addressing the
 * object itself uses (`slwi r3,r0,2` then `addi r0,r3,1; slwi r0,r0,1`). */
extern u16 lbl_805D43A0[0x7C];
/* The matching 0x805D44EC table: 21 records of two u16 (size 0x54). */
extern u16 lbl_805D44EC[0x2A];
extern u8 lb_param_w[];            /* .bss:0x806BF838, the lobby/option parameter block */

s32 quest_move_state_valid_ck(void);
f32 fn_80050EAC(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 fn_80050EF4(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_800DCCF8(void* state, nw4r::math::VEC3* pos, s32 enable);

/* The AI-NPC work record's own helpers, all inside this range and defined below. */
s32 fn_802D77A0(u8 value);
void fn_802D9544(void);
void fn_802D9EB4(struct _AINPC_W* self);
s32 fn_802D77DC(u8 slot);
void fn_802D9D30(struct _AINPC_W* self, s32 a, s32 b, s32 c, s32 d);
void fn_802D4218(struct _AINPC_W* self);
s32 fn_802D2B38(struct _AINPC_W* self, s32 a, s32 b);
void fn_802D2A00(struct _AINPC_W* self, s32 a, s32 b, s32 c);

/* 0x802D6534 - arms the "hold item" timer when the player is within the AI NPC's reach: the gate
 * byte at +0x43D is latched once, and the timer at +0x43E gets the long or the short value from
 * the distance test. */
void fn_802D6534(void)
{
    if (quest_move_state_valid_ck() == 0) {
        return;
    }
    if (lbl_806BD360.active == 0) {
        return;
    }
    if (lbl_806BD360.field_0x171 == 5) {
        return;
    }
    if (lbl_806BD360.field_0x43D != 0) {
        return;
    }
    lbl_806BD360.field_0x43D = 1;
    if (fn_80050EF4(&lbl_806BD360.vec_0x178,
                    (nw4r::math::VEC3*)&lbl_806BD360.plw_0x16C->motion_pos_0x3C) >= lbl_8079A770) {
        lbl_806BD360.field_0x43E = 0x258;
    } else {
        lbl_806BD360.field_0x43E = 0x12C;
    }
}

/* 0x802D65CC - the hold-item countdown: past 0x384 frames it fires the release animation and the
 * effect, otherwise it re-arms from the per-record tuning table. */
void fn_802D65CC(struct _AINPC_W* self)
{
    if (self->field_0x422 > 0x384) {
        self->field_0x422 = 0;
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 1);
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
    } else if (self->variant != 2) {
        self->field_0x422 = (s16)(lbl_805D5354[self->field_0x33A] * 0x1E);
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 0);
        fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
    }
}

/* 0x802D6690 - the hold countdown's frame budget, from the per-record tuning table's +0x3 byte. */
void fn_802D6690(struct _AINPC_W* self)
{
    u8 index = self->field_0x33A;
    s16 value = (s16)(lbl_805D5360[index].field_0x3 * 0x708);

    self->field_0x424 = value;
}

/* 0x802D66B8 - the weighted three-way choice: 1 while the roll is under the first threshold, 2 at
 * or past the sum of both, 0 in between. */
s32 fn_802D66B8(struct _AINPC_W* self)
{
    s16 roll = (s16)((u16)ran_suu(1) % 100);
    s16 first = (s16)lbl_805D5360[self->field_0x33A].field_0x0;

    if (roll < first) {
        return 1;
    }
    return (roll >= first + lbl_805D5360[self->field_0x33A].field_0x1) ? 2 : 0;
}

/* 0x802D675C - holds the sub-state while the frame budget is running, then re-arms both timers from
 * the second tuning table and starts the release animation. */
void fn_802D675C(struct _AINPC_W* self)
{
    if (self->variant != 2) {
        return;
    }
    if (self->field_0x424 > 0) {
        if (self->field_0x422 != 0) {
            return;
        }
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
        return;
    }
    s16 first = (s16)(lbl_805D5378[self->field_0x33A * 2] * 0x1E);
    self->field_0x422 = first;
    self->field_0x424 = (s16)(first + lbl_805D5378[self->field_0x33A * 2 + 1] * 0x1E);
    fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
}

/* 0x802D67F8 - the same hold/ re-arm pair as `fn_802D675C` but from the third tuning table. */
void fn_802D67F8(struct _AINPC_W* self)
{
    if (self->field_0x424 > 0) {
        if (self->field_0x422 != 0) {
            return;
        }
        fn_802D9D30(self, self->field_0x482, 0x11, 0, 2);
        return;
    }
    s16 first = (s16)(lbl_805D5390[self->field_0x33A * 2] * 0x1E);
    self->field_0x422 = first;
    self->field_0x424 = (s16)(first + lbl_805D5390[self->field_0x33A * 2 + 1] * 0x1E);
    fn_802D9D30(self, self->field_0x482, 0x12, 0, 2);
}

/* 0x802D6888 - rebuilds the behaviour flag word at +0x20C from the hold counters and the sub-state
 * of the +0x171 == 4 reaction state. */
void fn_802D6888(struct _AINPC_W* self)
{
    u16 flags = 0;

    if (self->field_0x21C != 0) {
        flags |= 1;
    }
    if (self->field_0x23C != 0) {
        flags |= 0x2000;
    }
    if ((s16)(self->field_0x234 + self->field_0x235) > 0) {
        flags |= 0x10;
    }
    if ((s16)(self->field_0x238 + self->field_0x239) > 0) {
        flags |= 0x40;
    }
    if (self->field_0x171 == 4) {
        switch (self->field_0x172) {
        case 8:
        case 0x13:
            flags |= 4;
            break;
        case 9:
        case 0x14:
            flags |= 8;
            break;
        case 0xA:
        case 0x15:
            flags |= 2;
            break;
        }
    }
    self->field_0x20C = flags;
}

/* 0x802D6964 - raises one of the two halves of the first held-item counter. */
void fn_802D6964(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x234 < value) {
            self->field_0x234 = value;
        }
    } else if (self->field_0x235 < value) {
        self->field_0x235 = value;
    }
}

/* 0x802D69A4 - raises one of the two halves of the second held-item counter. */
void fn_802D69A4(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x238 < value) {
            self->field_0x238 = value;
        }
    } else if (self->field_0x239 < value) {
        self->field_0x239 = value;
    }
}

/* 0x802D69E4 - latches the +0x23C flag and raises the +0x23E timer. */
void fn_802D69E4(struct _AINPC_W* self, u8 flag, s16 value)
{
    self->field_0x23C = flag;
    if (self->field_0x23E < value) {
        self->field_0x23E = value;
    }
}

/* 0x802D6A00 - re-rolls the four per-direction slot levels: the two 0/1 rolls at +0x3F6/+0x3F7
 * pick four entries of the shared level tables, and the best of them becomes the +0x414 timer. */
void fn_802D6A00(struct _AINPC_W* self)
{
    s16 best = 0;
    u16 w[4];
    u8 i;

    self->field_0x414 = 0;
    w[0] = (u16)(self->field_0x3F6 * 5);
    w[1] = (u16)(self->field_0x3F6 * 4 + self->field_0x3F7);
    w[2] = (u16)(self->field_0x3F7 * 5);
    w[3] = (u16)(self->field_0x3F7 * 4 + self->field_0x3F6);
    for (i = 0; i < 4; i++) {
        u16 v = w[i];
        s16 value;

        self->field_0x3F8[i] = lbl_805D4190[v];
        self->field_0x3FC[i] = lbl_805D41A0[v];
        self->field_0x400[i] = lbl_805D41B0[v];
        value = (u8)fn_802D77A0(self->field_0x3FC[i]);
        if (best <= value) {
            best = value;
        }
    }
    self->field_0x414 = fn_802D77DC((u8)best);
    self->field_0x3F4 = (u16)(self->field_0x400[0] | self->field_0x400[1] | self->field_0x400[2] |
                              self->field_0x400[3]);
}

/* 0x802D6B2C - moves the +0x3D4 gauge and clamps it into [0, +0x3D6], then mirrors +0x3DA. */
void fn_802D6B2C(struct _AINPC_W* self, s16 delta)
{
    s16 value = self->field_0x3D4 + delta;

    self->field_0x3D4 = value;
    if (self->field_0x3D6 < value) {
        self->field_0x3D4 = self->field_0x3D6;
    }
    if (self->field_0x3D4 < 0) {
        self->field_0x3D4 = 0;
    }
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D6B6C - raises the +0x3D6 cap by `delta`, clamped at 0x96. */
void fn_802D6B6C(struct _AINPC_W* self, s16 delta)
{
    if (self->field_0x3D6 < 0x96) {
        s16 value = self->field_0x3D6 + delta;

        self->field_0x3D6 = value;
        if (value >= 0x96) {
            self->field_0x3D6 = 0x96;
        }
    }
}

/* 0x802D6B98 - latches the +0x3D8 gauge into both the value and its cap, and mirrors +0x3DA. */
void fn_802D6B98(struct _AINPC_W* self)
{
    self->field_0x3D6 = self->field_0x3D8;
    self->field_0x3D4 = self->field_0x3D8;
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D7464 - the AI NPC's skill load-out: copies the three skill slots out of `lb_param_w`, bumps
 * the two skill counters for every unlocked skill, and scales the +0x3D4 gauge by the resulting
 * percentage. */
void fn_802D7464(struct _AINPC_W* self)
{
    s32 scale = 100;

    self->field_0x431 = 0;
    self->field_0x434 = 100;
    self->field_0x438 = 100;
    self->skill_0x42E[0] = lb_param_w[0x26];
    self->skill_0x42E[1] = lb_param_w[0x27];
    self->skill_0x42E[2] = lb_param_w[0x28];
    if (ai_skill_ck(self, 2) == 1 || ai_skill_ck(self, 3) == 1 || ai_skill_ck(self, 4) == 1 ||
        ai_skill_ck(self, 5) == 1 || ai_skill_ck(self, 6) == 1 || ai_skill_ck(self, 8) == 1 ||
        ai_skill_ck(self, 9) == 1 || ai_skill_ck(self, 0xA) == 1) {
        self->field_0x431 = 1;
    }
    if (ai_skill_ck(self, 0xE) == 1) {
        self->field_0x434 += 0x14;
    }
    if (ai_skill_ck(self, 0xF) == 1) {
        self->field_0x438 += 0x14;
    }
    if (ai_skill_ck(self, 0x13) == 1) {
        self->field_0x434 += 0x14;
        self->field_0x438 += 0x14;
        scale = 0x78;
    }
    if (ai_skill_ck(self, 0x16) == 1) {
        self->field_0x434 += 0x32;
        self->field_0x438 += 0x32;
        scale += 0x32;
    }
    self->field_0x3D4 = (s16)(self->field_0x3D4 * scale / 100);
    self->field_0x3D6 = self->field_0x3D8 = self->field_0x3D4;
}

/* 0x802D7688 - which of the four behaviour lists the given id belongs to (3 when in none). */
s32 fn_802D7688(u8 value)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        u8* p = (u8*)lbl_805D4390[i];

        while (*p != 0) {
            if (*p == value) {
                return i;
            }
            p++;
        }
    }
    return 3;
}

/* 0x802D773C - the behaviour-list record for the given index. */
u32 fn_802D773C(u8 index)
{
    return ((u32*)lbl_805D4358)[index];
}

/* 0x802D7754 - the four hold-countdown tuning values for the two 0/1 rolls at +0x3F6/+0x3F7. */
void fn_802D7754(u8 first, u8 second, u16* out)
{
    out[0] = lbl_805D41A0[first * 5];
    out[1] = lbl_805D41A0[first * 4 + second];
    out[2] = lbl_805D41A0[second * 5];
    out[3] = lbl_805D41A0[second * 4 + first];
}

/* 0x802D77A0 - the 14-entry level lookup: which of the three level values a slot level index maps
 * to (the table at 0x805D5428 is MWCC's own output for this switch). */
s32 fn_802D77A0(u8 value)
{
    switch (value) {
    default:
        return 0;
    case 1:
    case 3:
    case 6:
    case 9:
    case 12:
    case 13:
        return 1;
    case 2:
    case 4:
    case 7:
    case 10:
    case 11:
        return 2;
    }
}

/* 0x802D77DC - the +0x414 timer for the best slot level, in frames. */
s32 fn_802D77DC(u8 slot)
{
    if (slot == 1) {
        return 0x32;
    }
    s32 value = 0x1E;

    if (slot == 2) {
        value = 0x46;
    }
    return value;
}

/* 0x802D7A50 - raises the player-side AI NPC's "release" flag when the record is live, then
 * forwards to the shared dispatcher. */
void fn_802D7A50(void)
{
    if (lbl_806BD360.active == 0) {
        return;
    }
    lbl_806BD360.field_0x440 = 1;
    fn_802D4218(&lbl_806BD360);
}

/* 0x802D7A74 - whether one of the four hold slots is holding the live-chain item (0x1D) while the
 * AI NPC is in the +0x420 == 4 state. */
s32 fn_802D7A74(struct _AINPC_W* self)
{
    if (self->field_0x420 != 4) {
        return 0;
    }
    if (self->field_0x1CC != 0 && self->field_0x1CC != 5) {
        return 0;
    }
    if (self->hold_id_0x3B0 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3B4 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3B8 == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    if (self->hold_id_0x3BC == 0x1D && self->field_0x424 == 0) {
        return 1;
    }
    return 0;
}

/* 0x802D7B24 - whether the +0x171 == 1 state's sub-state is one of the two 0x20/0x3D.. ranges. */
s32 fn_802D7B24(struct _AINPC_W* self)
{
    u16 sub;

    if (self->field_0x171 != 1) {
        return 0;
    }
    sub = self->field_0x172;
    if ((u32)(sub - 0x20) <= 0x13) {
        return 1;
    }
    if ((u32)(sub - 0x3D) <= 0x13) {
        return 1;
    }
    return 0;
}

/* 0x802D7B5C - the hold-slot record index the item id falls in. */
s32 fn_802D7B5C(u16 value)
{
    u8 i;

    for (i = 1; i < 0x1E; i++) {
        if (value < lbl_805D43A0[(i + 1) * 4]) {
            break;
        }
    }
    return i;
}

/* 0x802D7BA4 - the second field of the hold-slot record. */
u16 fn_802D7BA4(u8 index)
{
    return lbl_805D43A0[index * 4 + 1];
}

/* 0x802D7BC4 - the third field of the hold-slot record. */
u16 fn_802D7BC4(u8 index)
{
    return lbl_805D43A0[index * 4 + 2];
}

/* 0x802D7BE4 - the fourth field of the hold-slot record. */
u16 fn_802D7BE4(u8 index)
{
    return lbl_805D43A0[index * 4 + 3];
}

/* 0x802D7C04 - the hold-range record index the item id falls in. */
s32 fn_802D7C04(u16 value)
{
    u8 i;

    for (i = 0; i < 0x14; i++) {
        if (value < lbl_805D44EC[(i + 1) * 2]) {
            break;
        }
    }
    return i;
}

/* 0x802D7C4C - the upper bound of the hold-range record. */
u16 fn_802D7C4C(u8 index)
{
    return lbl_805D44EC[index * 2 + 1];
}

/* 0x802D7C6C - the hold-range record index the item id falls in, over the 0x805D4560 table. */
s32 fn_802D7C6C(u16 value)
{
    u8 i;

    for (i = 0; i < 5; i++) {
        if (value < lbl_805D4560[i + 1]) {
            break;
        }
    }
    return i;
}

/* 0x802D7CB0 - the hang time the AI NPC's wake-up skill grants. */
s32 fn_802D7CB0(struct _AINPC_W* self)
{
    if (ai_skill_ck(self, 0x12) == 1) {
        return 5;
    }
    return 0xA;
}

/* 0x802D7CE4 - the "found the player" reaction: state 0x1A in a live area drops the AI NPC into
 * step 7. */
void fn_802D7CE4(u8 state)
{
    if (state != 0x1A) {
        return;
    }
    if (lbl_806BD360.active == 0) {
        return;
    }
    if (ai_area_ck(&lbl_806BD360) == 0) {
        return;
    }
    fn_802D2A00(&lbl_806BD360, 7, 0, 0);
}

/* 0x802D7D4C - the matching "lost the player" reaction for the +0x171 == 7 state. */
void fn_802D7D4C(void)
{
    if (lbl_806BD360.active == 0) {
        return;
    }
    if (ai_area_ck(&lbl_806BD360) == 0) {
        return;
    }
    if (lbl_806BD360.field_0x171 != 7) {
        return;
    }
    fn_802D2A00(&lbl_806BD360, 7, 1, 0);
}

/* 0x802D7DB4 - whether the +0x420 == 5 state's timer is running. */
s32 fn_802D7DB4(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    if (self->field_0x420 != 5) {
        return 0;
    }
    return self->field_0x422 != 0;
}

/* 0x802D7DF0 - sets the +0x374 gauge. */
void fn_802D7DF0(struct _AINPC_W* self, s16 value)
{
    self->field_0x374 = value;
}

/* 0x802D7DF8 - whether the +0x374 gauge is running. */
s32 fn_802D7DF8(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x374 != 0;
}

/* 0x802D7E20 - whether the AI NPC has arrived (its step counter is at 1). */
s32 fn_802D7E20(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return fn_802D2B38(self, 5, 3) == 1;
}

/* 0x802D7E68 - the same arrival test for the other motion slot. */
s32 fn_802D7E68(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return fn_802D2B38(self, 5, 2) == 1;
}

/* 0x802D7F10 - whether the id is one of the three slots of the list. */
s32 fn_802D7F10(u8* slots, u8 value)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (slots[i] == value) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D84D0 - sets the +0x47C gauge. */
void fn_802D84D0(struct _AINPC_W* self, s16 value)
{
    self->field_0x47C = value;
}

/* 0x802D948C - whether the item page at +0x483 is the first one. */
s32 fn_802D948C(struct _AINPC_W* self)
{
    return self->field_0x483 == 1;
}

/* 0x802D94A0 - whether the item page at +0x483 is the first or the third one. */
s32 fn_802D94A0(struct _AINPC_W* self)
{
    if (self->field_0x483 == 2) {
        return 1;
    }
    return self->field_0x483 == 3;
}

/* 0x802D9A40 - forwards to the page-arrow routine. */
void fn_802D9A40(void)
{
    fn_802D9544();
}

/* 0x802D9A44 - marks the AI NPC as having acknowledged the current item page. */
void fn_802D9A44(struct _AINPC_W* self)
{
    self->field_0x46C = 1;
}

/* 0x802D9A50 - whether the acknowledgement has already happened. */
s32 fn_802D9A50(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x46C == 1;
}

/* 0x802D9EA4 - forwards to the reaction dispatcher. */
void fn_802D9EA4(struct _AINPC_W* self)
{
    fn_802D9EB4(self);
}

/* 0x802D7804 - whether the AI NPC is close enough to the player to act on it. */
s32 fn_802D7804(u8 index, f32 distance)
{
    _PLW* plw;

    if (lbl_806BD360.active == 0) {
        return 0;
    }
    plw = lbl_806BD360.plw_0x16C;
    if (plw == 0) {
        return 0;
    }
    if (distance < lbl_8079A670) {
        return 1;
    }
    if (lbl_806BD360.field_0x1A4 != plw->area_0x16) {
        return 0;
    }
    if (lbl_806BD360.field_0x171 == 5) {
        return 0;
    }
    if (index == 4 && fn_802D9CE0(&lbl_806BD360) == 1) {
        return 0;
    }
    return fn_80050EAC(&lbl_806BD360.vec_0x178, (nw4r::math::VEC3*)&plw->motion_pos_0x3C) <
           distance * distance;
}

/* 0x802D78FC - whether the player is inside the AI NPC's current attack reach. */
s32 fn_802D78FC(void)
{
    _PLW* plw;

    if (lbl_806BD360.active == 0) {
        return 0;
    }
    if (lbl_806BD360.field_0x420 != 2) {
        return 0;
    }
    plw = lbl_806BD360.plw_0x16C;
    if (plw == 0) {
        return 0;
    }
    if (lbl_806BD360.field_0x1A4 != plw->area_0x16) {
        return 0;
    }
    if (lbl_806BD360.field_0x422 == 0) {
        return 0;
    }
    return fn_80050EAC(&lbl_806BD360.vec_0x178, (nw4r::math::VEC3*)&plw->motion_pos_0x3C) <
           lbl_8079A870;
}

#ifdef __cplusplus
}
#endif

/* 0x802D7640 - whether `skill` occupies any of the three skill slots copied out of `lb_param_w`. */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (skill == self->skill_0x42E[i]) {
            return 1;
        }
    }
    return 0;
}
