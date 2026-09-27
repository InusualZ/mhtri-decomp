/* enemy/fn_8015D860.cpp - the em008 enemy's per-action state-step band,
 * `.text` 0x8015D860..0x8015E854 (28 functions).
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x8015D860`: the shared runtime dump answers only
 * `zz_015d860_` - no `__FILE__`/class string names a source file - and
 * `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX` entries for the range;
 * every data reference the range makes is a vtable word, a jump table or a `.sdata2` float).
 *
 * What it is.  One `enemy`-module actor's per-action state steps, addressed through the shared
 * `_ENEMY_WORK` record: each step is the same three-part shape - a `state` (+0x05) switch, the
 * motion hand-off (`fn_80130478`/`fn_8012F5B8`/`fn_8012F62C`/`fn_80128AAC`), then the wait
 * (`fn_8012F93C`) and the finish (`fn_80127F48`/`fn_801280F4`).  `fn_8015E05C` and `fn_8015E804`
 * are the two dispatchers over `state_sub` (+0x1E6): the first is the compare chain for the
 * sparse step set {0,1,2,3,4,6,7}, the second the jump table for the contiguous 0..9 set (the
 * `.data` table at `jumptable_805A5DC0`, which the split leaves in the auto band - this unit does
 * not claim it, as the two bracketing registered units do not claim their own tables either).
 * `fn_8015D860` seeds the `field_0x1E4` flag byte from `em_parts_damage_level_get(self, 2)` and
 * two `fn_80135748` mask probes; `fn_8015D8F0`/`fn_8015D908`/`fn_8015D934` are its predicates;
 * `fn_8015D9B8`/`fn_8015DD6C` own the two +0x328/+0x32A countdowns; `fn_8015D9C8` is the per-tick
 * entry that attaches the 0xC-byte vtable helper `fn_8015DAA8` constructs; `fn_8015DAE4` and
 * `fn_8015DB68` are the `(state, sub-state)` transition tables.
 *
 * The seam is unproven (docs/plan.md 8.3).  The bracketing registered units are
 * `enemy/fn_801550FC.cpp` (below, `.text` ends exactly at 0x8015D860) and `enemy/fn_8015E854.cpp`
 * (above, starts exactly at 0x8015E854), and both headers record their own boundary as
 * provisional: `enemy/fn_8015E854.cpp`'s dispatchers call this range's `fn_8015E05C`/`fn_8015E804`
 * and `fn_80162500` calls `fn_8015D860`, so the real translation unit plausibly spans all three.
 * The registration follows the proposal's own range.
 *
 * Language: C++ (`medium` in the brief, re-derived here from the range's own evidence: the callees
 * with an argument list are C++ manglings - `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`,
 * `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3` - and the 0xC-byte helper is allocated
 * with `operator new` (`__nw__FUl`), which is C++ only).  The flat `fn_*` symbols are `extern "C"`
 * so objdiff pairs them by name; the mangled callees are declared through their real signatures
 * (rule 9) and the front-end produces the map spelling.
 *
 * Registration.  Class 4 of the brief's evidence order: no `__FILE__` string in the region's data
 * (class 1), `dumpmap.py lookup` answers only `zz_015d860_` (class 2), and the siblings' naming
 * scheme is the map's own stem with the rule-7 deferral (`enemy/fn_801550FC.cpp`,
 * `enemy/fn_8015E854.cpp`), so the file keeps the `fn_8015D860` stem.  Module `enemy` from the
 * link band (both bracketing registered units are `enemy`) and from the code (`_ENEMY_WORK` and
 * the enemy-band helpers).  Sections claimed with the block: `.text`, the `extab`
 * (0x8000DFA4..0x8000E04C) and `extabindex` (0x80028E60..0x80028F5C) runs the range's 21 framed
 * functions carry - the auto units of the retired scaffolding bucket split them exactly there - and
 * the `.data` jump table `fn_8015E804` lowers to (0x805A5DC0..0x805A5DE8, the map's 0x28-byte
 * `jumptable_805A5DC0`): the object emits it and its ten relocations are the target's own, so the
 * range is claimed rather than left to the auto band (the two bracketing units have no jump-table
 * claim of their own; this one is measured byte-identical).
 *
 * Types.  `_ENEMY_WORK` is the shared record in `include/enemy/ENEMY_WORK.h` (the one home,
 * rule 1).  This unit's additions there: the byte at +0x491 (`fn_8015DB68` sets it) and the s16
 * view of +0x32A (`fn_8015D9B8`/`fn_8015DB68`/`fn_8015DD6C` count it down as a halfword, where
 * `enemy/fn_80170600.cpp` stores a `stb` over the same byte - a union member, not a re-typing).
 * The 0xC-byte helper `fn_8015DAA8` constructs is private to this unit (`Helper_8015DAA8`), the
 * same record `enemy/fn_80147CE0.cpp`/`enemy/fn_80176C58.cpp` carry privately until rule 1 folds
 * the three into one header.
 *
 * Flags.  `#pragma peephole off` is load-bearing for the whole unit: retail keeps the unfused
 * `clrlwi`/`rlwinm` + `cmpwi` pairs (playbook 39) that `-O3`'s peephole folds into `clrlwi.`.  Probed
 * by turning the pass back on and re-measuring every symbol (peephole on -> off): fn_8015D8F0
 * 80.83 -> 100.0, fn_8015D908 79.82 -> 98.91, fn_8015D934 80.91 -> 100.0, fn_8015DAA8 99.33 ->
 * 100.0, fn_8015DAE4 96.82 -> 100.0, fn_8015DB68 97.36 -> 100.0, fn_8015E1C4 98.87 -> 100.0,
 * fn_8015E338 97.75 -> 100.0, fn_8015E6E8 96.00 -> 100.0; the other nineteen symbols are identical
 * under both.  No lib flag is involved: the `enemy` lib's `cflags_main` (see the block's comment in
 * `configure.py`) measures every body below.
 *
 * Status (official report metric, per symbol, measured in the worktree with
 * `python tools/units/recompile.py enemy/fn_8015D860.cpp --measure <symbol>`): 27 of the 28
 * functions are exactly 100 %; `fn_8015D908` is 98.91 % (44 B, the target's own size).  The object's
 * `.text` is 0xFF4 - the target's size to the byte - and its `extab` (0xA8), `extabindex` (0xFC) and
 * `.data` (0x28, the `fn_8015E804` jump table: ten `fn_8015E804+0x24..+0x48` words, the same ten
 * relocations the split's table at 0x805A5DC0 carries) are byte-identical to the target object
 * (`objdump -s -j <section>` on the two objects diffs clean).
 *
 * Residual - `fn_8015D908` 98.91 %.  Same 11 instructions, same size, same branch polarities
 * (`beq`/`bne`); only the two return blocks' ORDER differs: the target lays out
 * `[test bit0][test bit1][li r3,1 / blr][li r3,0 / blr]` and this source's shape lays out
 * `[test bit0][test bit1][li r3,0 / blr][li r3,1 / blr]`, so the two branch displacements are 4 B
 * apart.  Shapes tried and measured: the nested `if (bit0) { if (bit1) return 0; } return 1;`
 * (98.91 %, this one), the same with an explicit `return 1` inside the outer arm (79.45 % - MWCC
 * if-converts the if/else-return pair to `cntlzw`/`srwi`), `if ((bit0) == 0) return 1;` first
 * (48.18 %), an explicit `else` on either arm (79.45 %), a `result` variable with `return result`
 * (74.45 %), the if/else-if chain (48.18 %) and the `cond`/`compound`/`stmt_order`/`ternary`/`switch`
 * shape search (`tools/flags/shapesearch.py`, depth 3, beam 4 - best semantics-preserving candidate
 * 98.91 %; its 99.36 % row is a condition-flipped variant, i.e. not this function).  The residual is
 * the compiler's block ordering, not comprehension: the predicate is "0 only when flag bits 0 and 1
 * are both set".
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_8015D860.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8015D860.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "ef.h"              /* VEC3_ctor */
#include "ef/fn_80105314.h"  /* fn_801057A4 */
#include "draw_shape.h"      /* fn_80056A54 */
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * the 0xC-byte vtable helper this unit allocates
 * ------------------------------------------------------------------------------------------------- */

/* The helper `fn_8015D9C8` allocates and `fn_8015DAA8` constructs: a 12-byte record whose +0x00
 * word is the address of a `.data` table (`lbl_805A6D28`, the same shape as
 * `enemy/fn_80147CE0.cpp`'s `Helper_80147CE0` and `enemy/fn_80176C58.cpp`'s `Helper_80176E50` -
 * all three constructors call this band's base `fn_80147E2C` first and then store their own
 * table).  Only the table slot is touched by this unit; the rest is padding.
 * size: 0xC (traced from the `operator new(0xC)` call in `fn_8015D9C8`). */
typedef struct Helper_8015DAA8 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_8015DAA8;

/* The helper's `.data` table (a label the split has not assigned to a unit; rule 10: reference it,
 * never define it - declaring the class here would make MWCC emit a table into this object). */
extern "C" u8 lbl_805A6D28[];

/* ---------------------------------------------------------------------------------------------------
 * callees
 * ------------------------------------------------------------------------------------------------- */

/* Declared in a shared header: `fn_80130478`, `fn_8012F5B8`, `fn_8012F62C`, `fn_8012F93C`,
 * `fn_80127F48`, `fn_80128A14`, `fn_8013032C`, `fn_801303EC`, `fn_80130CDC`, `fn_80131E74`,
 * `fn_80133BB4`, `fn_80133BC0`, `fn_80135C5C`, `fn_80136D14`, `fn_801376B4`, `fn_8013221C`,
 * `fn_80132224`, `fn_80132264`, `fn_80141B88`, `fn_8012EC3C` (`unsplit/enemy.h`);
 * `fn_80128A8C`, `fn_80128AAC`, `fn_8012933C`, `fn_801280F4` (`enemy/fn_801251D0.h`);
 * `fn_8012D1A0` (`enemy/fn_8012BDF4.h`); `fn_801391E8`, `fn_801390FC` (`enemy/fn_80138074.h`);
 * `fn_80147E2C` (`enemy/fn_80147CE0.h`); `VEC3_ctor` (`ef.h`); `fn_801057A4`
 * (`ef/fn_80105314.h`); `fn_80056A54` (`draw_shape.h`); `setVector3` (`nw4r/math.h`);
 * `system_w` (`unsplit/unknown.h`). */

/* The three C++ free functions, declared by their real signatures so the front-end mangles them to
 * the map spellings (rule 9: the mangled spelling is never written). */
u8 em_parts_damage_level_get(_ENEMY_WORK* self, u8 part);
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(_ENEMY_WORK* self, u32 joint, Vec3* out);

/* `operator new` is what `fn_8015D9C8`'s 0xC-byte allocation lowers to (`__nw__FUl`). */
void* operator new(unsigned long size);

/* This unit's own forward declarations (each is called above its definition). */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self);
extern "C" void fn_8015DE48(_ENEMY_WORK* self);
extern "C" void fn_8015DEC4(_ENEMY_WORK* self);
extern "C" void fn_8015DF40(_ENEMY_WORK* self);
extern "C" void fn_8015DFBC(_ENEMY_WORK* self);
extern "C" void fn_8015E0BC(_ENEMY_WORK* self);
extern "C" void fn_8015E148(_ENEMY_WORK* self);
extern "C" void fn_8015E1C4(_ENEMY_WORK* self);
extern "C" void fn_8015E338(_ENEMY_WORK* self);
extern "C" void fn_8015E454(_ENEMY_WORK* self);
extern "C" void fn_8015E4F8(_ENEMY_WORK* self);
extern "C" void fn_8015E568(_ENEMY_WORK* self);
extern "C" void fn_8015E62C(_ENEMY_WORK* self);
extern "C" void fn_8015E6E8(_ENEMY_WORK* self);
extern "C" void fn_8015E788(_ENEMY_WORK* self);

/* Declarations for addresses whose bracketing registered units name different modules, so rule 2's
 * band header has no sound home for them (the same gap and the same spellings as the landed
 * `enemy/fn_80147CE0.cpp`, whose band these addresses sit in). */
/* 0x80305924 - between `ai/fn_802D0DCC.c` and `ef/fn_803066F0.c`: the effect spawn
 * `(self, u32, u32, VEC3*, f32, s32)`. */
extern "C" void fn_80305924(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s, s32 id);
/* 0x80304508 - the same band: `(self, u32, u32, VEC3*, f32)`. */
extern "C" void fn_80304508(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);
/* 0x803B9BA0 - between `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`:
 * `(self, VEC3*, s32)`. */
extern "C" void fn_803B9BA0(_ENEMY_WORK* self, void* pos, s32 value);

/* The `.sdata2` pool this band's float work reads (values measured from the DOL's `.sdata2`;
 * pooled data owned by another unit - declared, never defined, playbook 29). */
extern f32 lbl_80797330; /* 0.0 */
extern f32 lbl_80797334; /* -20.0 */
extern f32 lbl_80797338; /* 100.0 */
extern f32 lbl_8079733C; /* 1.0 */
extern f32 lbl_80797340; /* 50.0 */
extern f32 lbl_80797344; /* 150.0 */
extern f32 lbl_80797348; /* 160.0 */
extern f32 lbl_8079734C; /* -50.0 */
extern f32 lbl_80797350; /* 164.0 */
extern f32 lbl_80797354; /* 264.0 */
extern f32 lbl_80797358; /* 0.6 */
extern f32 lbl_8079735C; /* 120.0 */

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x8015D860 - seed the action's flag byte: clear it, then set bit 2 when the enemy has at least
 * two damage levels on part 2, bit 0 when the mask-2 probe answers and bit 5 when the mask-1 probe
 * does. */
extern "C" void fn_8015D860(_ENEMY_WORK* self) {
    self->field_0x1E4 = 0;
    if (em_parts_damage_level_get(self, 2) >= 2) {
        self->field_0x1E4 |= 4;
    }
    if (fn_80135748(self, 2) == 1) {
        self->field_0x1E4 |= 1;
    }
    if (fn_80135748(self, 1) == 1) {
        self->field_0x1E4 |= 0x20;
    }
}

/* 0x8015D8F0 - is none of the mask's bits set in the flag byte? */
extern "C" u32 fn_8015D8F0(_ENEMY_WORK* self, u8 mask) {
    return (self->field_0x1E4 & mask) == 0;
}

/* 0x8015D908 - the two-bit gate: 0 only when bit 0 and bit 1 are both set. */
extern "C" u32 fn_8015D908(_ENEMY_WORK* self) {
    if (self->field_0x1E4 & 1) {
        if (self->field_0x1E4 & 2) {
            return 0;
        }
    }
    return 1;
}

/* 0x8015D934 - count the clear bits of the low six (bit 0 .. bit 5). */
extern "C" u32 fn_8015D934(_ENEMY_WORK* self) {
    u8 count = 0;

    if ((self->field_0x1E4 & 1) == 0) {
        count = 1;
    }
    if ((self->field_0x1E4 & 2) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 4) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 8) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x10) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x20) == 0) {
        count = count + 1;
    }
    return count;
}

/* 0x8015D9B8 - clear the two +0x328/+0x32A countdowns. */
extern "C" void fn_8015D9B8(_ENEMY_WORK* self) {
    self->field_0x328 = 0;
    self->timer_0x32A = 0;
}

/* 0x8015D9C8 - the per-tick entry: on the `arg == 2` arm hand the enemy its motion, re-seed the
 * flag byte, attach the vtable helper when the enemy has none, and - when the enemy is not in the
 * +0x009 gate - spawn the position effect. */
extern "C" void fn_8015D9C8(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_8015DAA8* helper;

    VEC3_ctor(&v);
    switch ((u8)arg) {
      case 2:
        fn_80130478(self, 4);
        fn_80128A8C(self, 6, 5);
        fn_80133BC0(self);
        break;
    }
    fn_8015D860(self);
    if (fn_801391E8(self) == 0) {
        helper = (Helper_8015DAA8*)operator new(0xC);
        if (helper != 0) {
            fn_8015DAA8(helper);
        }
        fn_801390FC(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, lbl_80797330, lbl_80797334, lbl_80797338);
        fn_801057A4(self, 0x14, &v, lbl_8079733C, 0);
    }
}

/* 0x8015DAA8 - the helper's constructor: base first, then this class's table. */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self) {
    fn_80147E2C(self);
    self->vtbl = lbl_805A6D28;
    return self;
}

/* 0x8015DAE4 - the (state, sub-state) transition table of one action pair: state 7 advances the
 * sub-state 7/0xD to 0x10 when `fn_8012EC3C` answers, and 0xB to 0x11 while the flag byte's bit 0
 * is clear. */
extern "C" void fn_8015DAE4(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
      case 7:
        switch (*sub) {
          case 7:
          case 0xD:
            if (fn_8012EC3C(self) == 1) {
                *sub = 0x10;
            }
            break;
          case 0xB:
            if ((self->field_0x1E4 & 1) == 0) {
                *sub = 0x11;
            }
            break;
        }
        break;
    }
}

/* 0x8015DB68 - the second transition table: action 1 arms the +0x32A countdown (sub 6), spawns the
 * joint effect when `fn_8012D1A0` answers (sub 8) and hands over to `fn_801376B4` (sub 9);
 * action 0xA's sub 0xC3/0xC8 arms the flag byte's bit 0/bit 5, stores the +0x491 byte, and both
 * arms spawn an effect at the enemy's position. */
extern "C" void fn_8015DB68(_ENEMY_WORK* self, u8 arg, u8 sub) {
    VEC3 v1;
    VEC3 v2;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    switch (arg) {
      case 1:
        switch (sub) {
          case 6:
            self->timer_0x32A = 0x384;
            break;
          case 8:
            if (fn_8012D1A0(self) == 1) {
                get_joint_wpos_em(self, 0x14, &v2);
                fn_80141B88(self->field_0x01A, 0x21, 0, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF,
                            &v2, 0);
            }
            break;
          case 9:
            fn_801376B4(self);
            break;
        }
        break;
      case 0xA:
        switch (sub) {
          case 0xC3:
            fn_80135C5C(self, 1, 0);
            if (fn_8015D8F0(self, 1) == 1) {
                self->field_0x1E4 |= 1;
                setVector3(&v1, lbl_80797330, lbl_80797340, lbl_80797344);
                fn_80305924(self, 1, 0x14, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
          case 0xC8:
            fn_80135C5C(self, 0, 0);
            self->field_0x491 = 1;
            if (fn_8015D8F0(self, 0x20) == 1) {
                self->field_0x1E4 |= 0x20;
                setVector3(&v1, lbl_80797330, lbl_80797330, lbl_80797330);
                fn_80305924(self, 1, 0x2D, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
        }
        break;
    }
}

/* 0x8015DD6C - the per-tick bookkeeping: mirror "state 4" into the +0x38B flag and run the two
 * +0x328/+0x32A countdowns down while they are positive. */
extern "C" void fn_8015DD6C(_ENEMY_WORK* self) {
    switch (self->field_0x1E2) {
      case 4:
        self->field_0x38B = 1;
        break;
      default:
        self->field_0x38B = 0;
        break;
    }
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (self->timer_0x32A > 0) {
        self->timer_0x32A--;
    }
}

/* 0x8015DDB8 - the state-0 step: motion 0, then motion set (7, 0x12). */
extern "C" void fn_8015DDB8(_ENEMY_WORK* self) {
    fn_80130478(self, 0);
    fn_80128AAC(self, 7, 0x12);
    fn_80133BB4(self);
}

/* 0x8015DE00 - the state-0 step: motion 4, then motion set (6, 0xC). */
extern "C" void fn_8015DE00(_ENEMY_WORK* self) {
    fn_80130478(self, 4);
    fn_80128AAC(self, 6, 0xC);
    fn_80133BB4(self);
}

/* 0x8015DE48 - the two-step state: start motion set (1, 0xA) on entry, finish on the wait. */
extern "C" void fn_8015DE48(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 1, 0xA, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015DEC4 - the same two-step shape with motion set (2, 6). */
extern "C" void fn_8015DEC4(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 2, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015DF40 - the same two-step shape with motion set (0xE, 6). */
extern "C" void fn_8015DF40(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xE, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015DFBC - the two-step state that also refreshes the scene: motion set (1, 0, 0) and the
 * height pair on entry, `fn_801280F4` on the wait. */
extern "C" void fn_8015DFBC(_ENEMY_WORK* self) {
    fn_80131E74(self);
    fn_80136D14(self);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 4);
        fn_8012F62C(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8015E05C - the sparse sub-state dispatcher: the compare chain for {0,1,2,3,4,6,7}. */
extern "C" void fn_8015E05C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015DE48(self);
        break;
      case 1:
        fn_8015DEC4(self);
        break;
      case 2:
        fn_8015DF40(self);
        break;
      case 3:
        fn_8015DE48(self);
        break;
      case 4:
        fn_8015DE48(self);
        break;
      case 6:
        fn_8015DE48(self);
        break;
      case 7:
        fn_8015DFBC(self);
        break;
    }
}

/* 0x8015E0BC - the two-step state with motion set (0xF, 6); the local vector is zeroed but not
 * used by either arm (retail does the same call). */
extern "C" void fn_8015E0BC(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xF, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E148 - the same two-step shape with motion set (9, 6). */
extern "C" void fn_8015E148(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 9, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E1C4 - the long step: motion set (0xF, 6) on entry, then three frame windows
 * (`em_frame_check`) that fire the part effect, the ground effect at (0, -50, 100) and the second
 * one at the same point while `system_w`'s +0x0C low three bits are clear. */
extern "C" void fn_8015E1C4(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            fn_8012933C(self, 1, 0x12, 5);
            fn_80056A54((u32)self, 0x14, 0xA);
        }
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
            fn_80304508(self, 0, 0x13, &v, lbl_8079733C);
        }
        if (em_frame_check(self, 3, lbl_80797350, lbl_80797354) == 1) {
            if ((system_w.field_0x0c & 7) == 0) {
                setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
                fn_80304508(self, 1, 0x13, &v, lbl_8079733C);
            }
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E338 - the four-step state: motion set (0x28, 2), then (0x6E, 4) with the +0x20 counter
 * seeded to 0x708, then the 0.6-ratio step (0x70, 4) once the counter runs out, then the wait. */
extern "C" void fn_8015E338(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x28, 2, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
      case 2:
        fn_8013221C(self, lbl_80797358, 1, 0xA);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            fn_8012F5B8(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        break;
      case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E454 - the two-step state: motion set (0xC8, 8) and the +0x20 counter at 3 plus the
 * `fn_80130CDC` arm on entry; the 120-frame window hands over to `fn_80128A14`. */
extern "C" void fn_8015E454(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xC8, 8, 0);
        self->timer_0x020 = 3;
        fn_80130CDC(self, 0x3E8);
        break;
      case 1:
        if (em_frame_check(self, 1, lbl_8079735C, lbl_80797330) == 1) {
            fn_80128A14(self, 1, 8);
        }
        break;
    }
}

/* 0x8015E4F8 - the two-step state with motion set (0xCA, 2). */
extern "C" void fn_8015E4F8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_8012F5B8(self, 0xCA, 2, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E568 - the two-step state that counts two motion waits: motion set (0x24, 6) plus the
 * `fn_8012933C` hand-off and the +0x20 counter cleared, then the second motion set (1, 7) once the
 * counter reaches 2. */
extern "C" void fn_8015E568(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x24, 6, 0);
        fn_8012933C(self, 1, 0xB, 2);
        self->timer_0x020 = 0;
        /* falls through to the wait arm */
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (self->timer_0x020 >= 2) {
            self->state++;
            fn_80128A14(self, 1, 7);
        }
        break;
    }
}

/* 0x8015E62C - the three-step state: motion set (0x29, 8) plus a flag re-seed on entry, then
 * (0x70, 2, 0x42), then the wait. */
extern "C" void fn_8015E62C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x29, 8, 0);
        fn_8015D860(self);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x70, 2, 0x42);
        }
        break;
      case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E6E8 - the two-step state: motion set (0xC8, 4) and the +0x20 counter at 3 on entry, then
 * the counter runs down and hands over to `fn_80128A14`. */
extern "C" void fn_8015E6E8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xC8, 4, 0);
        self->timer_0x020 = 3;
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            if (--self->timer_0x020 <= 0) {
                fn_80128A14(self, 1, 5);
            }
        }
        break;
    }
}

/* 0x8015E788 - the two-step state with motion set (0xE, 6). */
extern "C" void fn_8015E788(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xE, 6, 0);
        break;
      case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x8015E804 - the contiguous sub-state dispatcher (the jump table at `jumptable_805A5DC0`). */
extern "C" void fn_8015E804(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015E0BC(self);
        break;
      case 1:
        fn_8015E148(self);
        break;
      case 2:
        fn_8015E1C4(self);
        break;
      case 3:
        fn_8015E338(self);
        break;
      case 4:
        fn_8015E454(self);
        break;
      case 5:
        fn_8015E4F8(self);
        break;
      case 6:
        fn_8015E568(self);
        break;
      case 7:
        fn_8015E62C(self);
        break;
      case 8:
        fn_8015E6E8(self);
        break;
      case 9:
        fn_8015E788(self);
        break;
    }
}
