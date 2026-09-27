/* ai/fn_802C474C.cpp - the AI-NPC band's "aimed action" ladder: eleven per-motion dispatchers of the
 * AI work record (`_AINPC_W`), each a small state machine that arms a motion through the band's shared
 * helpers, walks the motion's 10- or 14-byte table row, and drives the effect/SE pass.
 *
 * `.text` 0x802C474C..0x802C5D10 (0x15C4 B, 11 functions), the run between the landed `light/light.cpp`
 * and `ai/fn_802CC794.cpp`.  The registration's right edge is the attribution plan's `--max-bytes` cap,
 * not a measured seam (`attribute.py`: "capped at --max-bytes, seam is a guess"), so the extab run
 * 0x8001442C..0x80014484 and the `.data` run 0x805D4784..0x805D4964 are this unit's but the extent past
 * 0x802C5D10 settles as the band's functions match (brief section 2).
 *
 * MODULE AND NAME.  Class 3 of the brief's evidence order: no `__FILE__` string is referenced by any of
 * the eleven (the nearest, `.data` 0x805CDFC8 = "menu_item.cpp", belongs to the landed
 * `menu/menu_item.cpp` and is emitted by 0x802A22A4, 0x59000 B below this range), and Dolphin's map has
 * only `zz_02c474c_`-style placeholders for all eleven.  What decides it is the record: every function
 * takes `_AINPC_W*`, the type the neighbouring `ai/fn_802CC794.cpp` reconstructs from `ai_skill_ck__FP8_AINPC_WUc`,
 * so the module is `ai` and the file keeps the map's stem.  The `.cpp` extension is the band's: the same
 * run is C++ (mangled callees `rotVecY__FPQ34nw4r4math4VEC3Ul`).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked: `symedit.py range`
 * over 0x802C4700..0x802C5E00 lists eleven `fn_` stems and nothing else, and `dumpmap.py lookup` on all
 * eleven returns `zz_<address>_` placeholders, i.e. unnamed rather than absent).
 *
 * LOAD-BEARING SOURCE SHAPES (each measured; the band's flags are the `ai` lib's `cflags_main` plus the
 * file-scope `#pragma peephole off` below):
 *   * the four motion tables are `u16` word arrays, not row structs.  `arg * 5` / `arg * 7`
 *     strength-reduce to `slwi`+`add` and then take the u16 scale, which is retail's index math; a
 *     10/14-byte struct index folds into one `mulli`, measured (playbook 39's unfused-form rule).
 *   * `self->state++` / `self->sub_step--`, never `x = x + 1` - the assignment form emits a `clrlwi`
 *     retail does not have.
 *   * the two `arg == 0 || arg == 2` class tests are `switch (arg)` with shared case labels, not
 *     `if`/`else if`: an if-chain gets a nested test/body layout, the switch the flat compare chain
 *     retail has (playbook 34/37).
 *   * `fn_802C4EA8` declares one output vector *per case* (three slots, MWCC hands the first-declared
 *     the highest offset) and `fn_802C58DC` writes the ground result with one chained assignment, which
 *     is what keeps retail's single `lfs` for its two stores.
 *
 * RESIDUAL.
 *   * the four tables are declared, never defined (playbook 29): `.data` 0x805D4784..0x805D4964 is this
 *     band's own run (`leak 0`, four labels) and belongs to the measured data pass, so they are
 *     `extern`-declared here and used as load operands.  `shell_set_func_ptr` (`.sbss` 0x80794B60) and
 *     the `.sdata2` constants are the same case.
 *   * the record's tail fields (+0x3C2..+0x498) are new in `include/ai/ainpc.h`; its owner
 *     `src/ai/fn_802CC794.cpp` still carries its own inline copy of the union and has to include the
 *     header instead (rule 1).
 *   * five rows are not byte-identical; what still differs, first divergence first:
 *     `fn_802C5428` (90.4) and `fn_802C5624` (90.9) - the scheduler hoists the SE word's base+offset
 *     ahead of the `arg` compares in retail and leaves it inside the arm here (same bytes, different
 *     order, 508/508 and 700/696 B).  `fn_802C4EA8` (94.0) - retail's `arg == 5` test is a signed
 *     `cmpwi` where a `u8` parameter gives `cmplwi` (one instruction), and its `case 1` state increment
 *     schedules three slots later.  `fn_802C5ACC` (94.5) and `fn_802C4CD4` (98.3) - the
 *     `shell_set_func_ptr` load is scheduled three instructions early in ours (4 x 1 instruction).
 *     `fn_802C4908` (99.4) - one `lhz`-vs-`lha` load of `_PLW` +0x650 and the r29/r30 pairing of `self`
 *     and the argument.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "pl.h"                /* `_PLW` - the record's +0x650/+0x652 the motion switch reads */
#include "Pl/fn_8027D684.h"    /* `fn_8027D76C` (rule 2: the owner's header) */
#include "Pl/fn_8028F66C.h"    /* `copyVec3`, `fn_8012A624`, `fn_80291B08` */
#include "fn_8004CAD8.h"       /* `rotVecY`, `fn_80050CA0`, `fn_80051378` */
#include "sound/fn_800D7F54.h" /* `fn_800DCC24` */
#include "ai/ainpc.h"

/* This band keeps the unfused forms retail has: the tables' index math is `clrlwi` + `slwi` separate
 * (the peephole folds them into one `clrlslwi`), so the pass is off for the whole file. */
#pragma peephole off

/* The four motion tables the band's rows live in, `.data` 0x805D4784..0x805D4964: the ladder's
 * 5-word rows (0x805D4810, 0x805D4874) and the wider 7-word rows (0x805D4784, 0x805D48D8).  Their owner
 * is the band's unclaimed `.data` run, so they are `extern`-declared and never defined (playbook 29).
 *
 * They are declared as `u16` word arrays, not as a row struct: a 10/14-byte struct's index scale folds
 * into one `mulli`, and retail's index math is the strength-reduced `arg * 5` / `arg * 7` followed by
 * the u16 scale (`slwi`+`add`+`slwi`), i.e. the source carried the word stride itself.  Word offsets,
 * ascending - the layout a struct would have stated, kept here because the tables are word arrays:
 *
 *   5-word rows                                7-word rows
 *   word +0 (byte +0x00) anim_id   s16         word +0 (byte +0x00) anim_id   s16
 *   word +1 (byte +0x02) motion    u16         word +1 (byte +0x02) motion    u16
 *   word +2 (byte +0x04) param     s16         word +2 (byte +0x04) param     s16
 *   word +3 (byte +0x06) variant   s16         word +3 (byte +0x06) variant   s16
 *   word +4 (byte +0x08) se_id     u16         word +4 (byte +0x08) motion2   u16
 *                                              word +5 (byte +0x0A) param2    s16
 *                                              word +6 (byte +0x0C) se_id     u16
 */
enum {
    ROW5_ANIM = 0, ROW5_MOTION = 1, ROW5_PARAM = 2, ROW5_VARIANT = 3, ROW5_SE = 4, ROW5_WORDS = 5,
};
enum {
    ROW7_ANIM = 0, ROW7_MOTION = 1, ROW7_PARAM = 2, ROW7_VARIANT = 3, ROW7_MOTION2 = 4,
    ROW7_PARAM2 = 5, ROW7_SE = 6, ROW7_WORDS = 7,
};

extern "C" {

extern u16 lbl_805D4784[];   /* 10 x 7 words: the flight's own ladder */
extern u16 lbl_805D4810[];   /* 10 x 5 words */
extern u16 lbl_805D4874[];   /* 10 x 5 words */
extern u16 lbl_805D48D8[];   /* 10 x 7 words */

/* The `.sdata2` constants the state machines compare against and arm (playbook 29: declared, never
 * defined - the pool is the split's). */
extern f32 lbl_8079A670;   /* 0.0f */
extern f32 lbl_8079A68C;
extern f32 lbl_8079A69C;
extern f32 lbl_8079A6A0;
extern f32 lbl_8079A6A4;
extern f32 lbl_8079A6A8;
extern f32 lbl_8079A6AC;
extern f32 lbl_8079A6B0;
extern f32 lbl_8079A6B4;
extern f32 lbl_8079A6B8;

/* `shell_set_func_ptr` (`.sbss` 0x80794B60) - the pointer the band's two outbound effect calls go
 * through.  Slots +0x24 and +0x38 are the only ones evidenced here, and the two take a different
 * argument list, so the table is a record of typed function pointers reached field by field (rule 6:
 * no pointer arithmetic; rule 10: a table outside our ranges is referenced, not written).
 * size: 0x3C */
struct AINPCShellFuncs {
    /* +0x00 */ u8 unused_0x00[0x24];
    /* +0x24 */ void (*set_target)(struct _AINPC_W* self, struct AINPCShellFuncs* shell);
    /* +0x28 */ u8 unused_0x28[0x38 - 0x28];
    /* +0x38 */ void (*request)(struct _AINPC_W* self, s32 id, struct AINPCShellFuncs* shell);
};

extern struct AINPCShellFuncs* shell_set_func_ptr;

/* The band's shared helpers: all unowned (their address band interleaves modules, so rule 2 leaves
 * them here rather than guessing a module header).  Signatures are the call sites' own argument
 * shapes. */
void fn_802D2904(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
void fn_802D2910(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
u32 fn_802D2984(struct _AINPC_W* self);
u32 fn_802D2990(struct _AINPC_W* self, u32 flag, f32 low, f32 high);
void fn_802D29F8(struct _AINPC_W* self, u32 variant);
void fn_802D2ABC(struct _AINPC_W* self, u32 flag, u16 a, u16 se);
void fn_802D2AD4(struct _AINPC_W* self, u32 state);
u32 fn_802D2B78(struct _AINPC_W* self, u32 flag);
void fn_802D2B88(struct _AINPC_W* self, u32 flag);
u32 fn_802D30F8(u32 step, u32 value, u32 limit);
void fn_802D31FC(struct _AINPC_W* self);
void fn_802D3210(struct _AINPC_W* self, u32* value);
void fn_802D3A2C(struct _AINPC_W* self, u32 flag);
void fn_802D3CCC(struct _AINPC_W* self, u32 flag);
void fn_802D3CD4(struct _AINPC_W* self, u32 flag);
void fn_802D40A4(struct _AINPC_W* self, u16 motion);
void fn_802D40EC(struct _AINPC_W* self, u16 motion, s16 param);
void fn_802D84D0(struct _AINPC_W* self, u32 flag);
void fn_802D9A44(struct _AINPC_W* self);
void fn_802D9D30(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);

} /* extern "C" */

/* The aimed action's first stage: arms the swing, then hands the motion's own ladder to the effect
 * pass once `fn_802D2990` reports the arm window open. */
extern "C" void fn_802C474C(struct _AINPC_W* self, u8 arg) {
    u8 state;
    if (fn_802D2B78(self, 0x10) != 0) {
        fn_802D3A2C(self, 2);
    }
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 3;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x17, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x3f, 4, 0);
        }
        break;
    case 1:
        if ((arg == 0 && fn_802D2990(self, 1, lbl_8079A69C, lbl_8079A670) != 0) ||
            (arg == 1 && fn_802D2990(self, 1, lbl_8079A6A0, lbl_8079A670) != 0)) {
            if (self->sub_step == 0) {
                self->state++;
            } else {
                self->sub_step--;
                if (arg == 0) {
                    fn_802D2904(self, 0x17, 0, 0x1c);
                } else {
                    fn_802D2904(self, 0x3f, 0, 0x30);
                }
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The ranged action's arm: reads the enemy's own motion out of the player work it targets and leaves
 * the record in the matching motion before the target's frame pass runs. */
extern "C" void fn_802C4908(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        struct _PLW* plw;
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 6, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x39, 4, 0);
        }
        plw = self->plw_0x16C;
        switch (plw->field_0x650) {
        case 0x1c:
            self->field_0x3C2 = 0x1c;
            break;
        case 0x1b:
        case 0x2d:
        case 0xef:
            self->field_0x3C2 = 0x1b;
            break;
        case 0x1a:
        case 0xf0:
            self->field_0x3C2 = 0x1a;
            break;
        case 0x1e:
            self->field_0x3C2 = 0x1e;
            break;
        case 0x1f:
            self->field_0x3C2 = 0x1f;
            break;
        case 0x20:
            self->field_0x3C2 = 0x20;
            break;
        default:
            self->field_0x3C2 = 0;
            fn_802D40EC(self, plw->field_0x650, plw->field_0x652);
            if (self->field_0x420 == 3) {
                self->field_0x422 = 0x708;
                self->field_0x42C = plw->field_0x650;
                self->field_0x427 = 0;
                self->field_0x428 = 0;
                self->field_0x429 = 0;
                self->field_0x42A = 0;
                self->field_0x42B = 0;
            }
            break;
        }
        fn_8027D76C(plw);
        break;
    }
    case 1: {
        struct _PLW* plw = self->plw_0x16C;
        if (fn_802D2984(self) != 1) {
            break;
        }
        if (arg == 0) {
            if (self->field_0x3C2 != 0) {
                fn_802D2ABC(self, 1, 0x14, 0);
            } else {
                fn_802D2ABC(self, 1, 0x17, 0);
                fn_802D9D30(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
            }
        } else {
            if (self->field_0x3C2 != 0) {
                fn_802D40A4(self, plw->field_0x650);
            }
            fn_802D2ABC(self, 1, 0x36, 0);
            fn_802D9D30(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
        }
        break;
    }
    }
}

/* The arm-out: cancels the armed motion's tail and returns the record to the variant it started the
 * action with. */
extern "C" void fn_802C4B68(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x25, 8, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            fn_802D2AD4(self, self->variant);
        }
        break;
    }
}

/* The two-stage aimed action's first half. */
extern "C" void fn_802C4BF4(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x12, 4, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x42, 4, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The four-stage aimed action: the swing, its hold, the settle (which hands the target to the shell
 * table's own state change) and the arm-out. */
extern "C" void fn_802C4CD4(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x2d, 2, 0);
        } else {
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x51, 2, 0);
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 4, 2, 0);
            } else {
                fn_802D2904(self, 0x37, 2, 0);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0x31, 0, 0);
            } else {
                fn_802D2904(self, 0x53, 6, 0);
            }
            shell_set_func_ptr->set_target(self, shell_set_func_ptr);
            fn_802D9A44(self);
            self->field_0x447 = 0;
            self->field_0x446 = 1;
            fn_802D9D30(self, (s8)self->field_0x482, 0xa, 0, 2);
        }
        break;
    case 3:
        if (fn_802D2984(self) == 1) {
            if (arg == 0) {
                fn_802D2AD4(self, 0);
            } else {
                fn_802D2AD4(self, 2);
            }
        }
        break;
    }
}

/* The arrow's flight: the row keys the shot, `rotVecY` builds the shot vector out of the record's own
 * yaw, and the per-frame pass integrates the effect vector and the stage timer. */
extern "C" void fn_802C4EA8(struct _AINPC_W* self, u8 arg) {
    nw4r::math::VEC3 shot;
    VEC3_ctor(&shot);
    shot.x = lbl_8079A670;
    shot.y = lbl_8079A670;
    shot.z = lbl_8079A6A4;
    switch (self->state) {
    case 0: {
        u16* row;
        nw4r::math::VEC3 out;
        self->state++;
        if (arg == 0 || arg == 5) {
            self->sub_step = 1;
        }
        row = &lbl_805D4784[arg * ROW7_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW7_ANIM]);
        fn_802D2904(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_PARAM2];
        }
        if (self->field_0x3F0 == 0) {
            fn_802D2B88(self, 1);
        }
        rotVecY(&shot, self->field_0x194);
        fn_80051378(&out, &self->vec_0x178, &shot);
        copyVec3(&self->vec_0x3E0, &out);
        self->field_0x3EC = (u16)self->field_0x194;
        self->field_0x3EE = 0;
        fn_800DCC24(self->sound_0x498, 0, 0);
        break;
    }
    case 1: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        fn_80050CA0(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            fn_802D2904(self, lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION], (s16)lbl_805D4784[arg * ROW7_WORDS + ROW7_PARAM], 0);
        } else if (arg < 5) {
            if (fn_802D2990(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE -= 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    case 2: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        fn_80050CA0(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (fn_802D2984(self) == 1) {
            if (self->sub_step == 0) {
                fn_802D2ABC(self, 1, lbl_805D4784[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
            } else {
                self->state = 1;
                self->sub_step--;
                fn_802D2904(self, lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION], (s16)lbl_805D4784[arg * ROW7_WORDS + ROW7_PARAM], 0);
                fn_800DCC24(self->sound_0x498, 0, 1);
            }
        } else if (arg < 5) {
            if (fn_802D2990(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE += 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    }
}

/* The three-row ladder's first rung (rows 0x805D4810): arms the row's motion, then re-arms either the
 * row's own settle SE or the next rung depending on how far into the ladder the caller is. */
extern "C" void fn_802C522C(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4810[arg * ROW5_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW5_ANIM]);
        fn_802D2904(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 1, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 1, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xd3, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xea, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The second rung (rows 0x805D4874), the same ladder one row group on. */
extern "C" void fn_802C5428(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4874[arg * ROW5_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW5_ANIM]);
        fn_802D2904(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 2, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 2, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xcb, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe2, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D4874[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4874[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The four-row ladder (rows 0x805D48D8): the widest of the three, one rung per motion of the volley. */
extern "C" void fn_802C5624(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D48D8[arg * ROW7_WORDS];
        fn_802D29F8(self, (u8)(s16)row[ROW7_ANIM]);
        fn_802D2904(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_VARIANT];
            fn_800DCC24(self->sound_0x498, 3, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 3, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (fn_802D2984(self) == 1) {
            self->state++;
            fn_802D2904(self, lbl_805D48D8[arg * ROW7_WORDS + ROW7_MOTION2], (s16)lbl_805D48D8[arg * ROW7_WORDS + ROW7_PARAM2], 0);
        }
        break;
    case 2:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xce, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe5, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
            }
        }
        break;
    case 3:
        if (fn_802D2984(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2904(self, 0xcf, 0, 0);
            } else if (arg == 5) {
                fn_802D2904(self, 0xe6, 0, 0);
            }
        }
        break;
    case 4:
        if (fn_802D2984(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The aimed shot's release: raises the effect handle, waits for the arm window, then queries the ground
 * under the shot vector and leaves the record in the landing state it reports. */
extern "C" void fn_802C58DC(struct _AINPC_W* self) {
    LandData land;
    f32 ground;
    fn_8012A624(&land);
    switch (self->state) {
    case 0:
        self->state++;
        fn_802D29F8(self, 2);
        fn_802D2910(self, 0xd, 8, 0);
        fn_802D31FC(self);
        self->field_0x1DC = lbl_8079A6AC;
        break;
    case 1:
        fn_802D3210(self, &self->field_0x190);
        self->field_0x190 = (u16)fn_802D30F8(0x4000, self->field_0x190 & 0xffff, 0x800);
        fn_802D3CD4(self, 2);
        if (self->vec_0x178.y > lbl_8079A6B0) {
            self->vec_0x178.y = lbl_8079A6B0;
        }
        if (self->field_0x19C - lbl_8079A6B4 >= self->vec_0x178.y) {
            self->state++;
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x3a, 8, 0);
        }
        break;
    case 2:
        self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x1000);
        if (fn_802D2984(self) == 1) {
            copyVec3(&self->vec_0x178, &self->vec_0x1B0);
            if (fn_80291B08((struct _PLW*)self, &self->vec_0x178, &land, &ground, -5) == 1) {
                /* One `lfs` and two stores in retail: the traveller's distance and the
                 * height it settles at are the same value. */
                self->vec_0x178.y = self->field_0x19C = ground;
            }
            self->field_0x3C6 = 0;
            fn_802D3CCC(self, 1);
            if ((land.field_0x00 & 3) != 0) {
                fn_802D2ABC(self, 1, 0x37, 0);
            } else {
                fn_802D2ABC(self, 1, 5, 0);
            }
        } else {
            fn_802D3CD4(self, 2);
            fn_802D84D0(self, 2);
        }
        break;
    }
}

/* The guard/parry action: the arm chooses the shell's own state table by the arg class, and the settle
 * asks the shell to play the class's SE. */
extern "C" void fn_802C5ACC(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
        case 2:
            fn_802D29F8(self, 0);
            fn_802D2904(self, 0x2e, 0, 0);
            break;
        case 1:
        case 3:
            fn_802D29F8(self, 2);
            fn_802D2904(self, 0x52, 0, 0);
            break;
        }
        break;
    case 1:
        if (fn_802D2984(self) == 1) {
            switch (arg) {
            case 0:
            case 2:
                fn_802D2AD4(self, 0);
                break;
            case 1:
            case 3:
                fn_802D2AD4(self, 2);
                break;
            }
        } else {
            switch (arg) {
            case 0:
            case 2:
                if (fn_802D2990(self, 0, lbl_8079A68C, lbl_8079A670) == 1) {
                    if (arg == 0) {
                        shell_set_func_ptr->request(self, 0xc, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xe, shell_set_func_ptr);
                        fn_802D9D30(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            case 1:
            case 3:
                if (fn_802D2990(self, 0, lbl_8079A6B8, lbl_8079A670) == 1) {
                    if (arg == 1) {
                        shell_set_func_ptr->request(self, 0xd, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xf, shell_set_func_ptr);
                        fn_802D9D30(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            }
        }
        break;
    }
}
