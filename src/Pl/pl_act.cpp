/*
 * Pl/pl_act.cpp - the player actor: the part/motion cluster, the master, skill and item layers, the act-entry front
 *   end, the action state machines and the equipment helpers.
 * RANGE. .text 0x802693C4-0x802840DC (459 functions); .rodata 0x805706C0-0x80570700, .data 0x805C5F30-0x805C80A8,
 *   .bss 0x806AB810-0x806AB830, .sdata 0x80792140-0x80792188, .sbss 0x80794B30-0x80794B38, .sdata2
 *   0x8079A000-0x8079A1D8, extab, extabindex.  Seven sections in address order, each banner-marked: part/motion
 *   0x802693C4, master 0x8026BA1C, health gate 0x8026FFBC, skill 0x80270018, act entry 0x80273B14, action 0x80276B58,
 *   equipment 0x8027D684.  Each section keeps its own callee declarations in a namespace (some shared callees are
 *   declared with different signatures); `extern "C"` names stay unmangled, and the C++-linkage callees and the types
 *   the manglings name stay at global scope.  Uniting the declarations is open work.
 *   The left seam is unproven by extabindex: 0x8002F6DC -> 0x8026910C is `Pl/player_control.cpp`'s last record and
 *   0x8002F6E8 -> 0x802695A4 this unit's first; `fn_80269394` and `fn_802693C4`..`fn_80269508` are frameless, so
 *   extab cannot place the 0x802693C4 seam.
 * FLAGS. `cflags_pl` (docs/pl.md).  The skill section is under `#pragma optimization_level 4` (docs/pl.md: level 3
 *   rematerialises `fn_8027350C`'s base); the file is under `#pragma peephole off` from the act-entry section on.  The
 *   master section's `#pragma peephole off` runs from `fn_8026BE94` through `fn_8026CBE4` (including `fn_8026BF98`),
 *   and `off`/`reset` pairs scope `fn_8026F888`, `fn_8026F9A4` and `Pl_act_ck`..`fn_8026FEF0`: recorded as
 *   load-bearing in six places (they turn the fused record form back into retail's `clrlwi`/`rlwinm`/`and` + `cmpwi`
 *   pair and keep the byte/short truncations), measured with a flag set without `-opt nopeephole` and not re-measured
 *   under the lib flag.
 * NAMES. GUESSes read from each body: `Pl_chr_set_attr_default` 0x8026A224 (`Pl_chr_set_attr` with its last argument
 *   0), `Pl_motion_end_ck` 0x8026A33C (the model layer's "motion finished" predicate), `Pl_act_set_motion` 0x80275B04
 *   (arms the act's three status bits from a packed mask), `Pl_act_set_motion_slot` 0x802761B8 (the same hand-off with
 *   two trailing zeros), `Pl_act_set_step_table` 0x802770E8 (installs the per-act `.data` record at +0x318),
 *   `Pl_item_id_usable_ck` 0x802752C8 and its parameter `mode` (dead in retail; the call sites pass 0/1/2 against the
 *   body's 1/2/0x10 masks).
 *   GUESS (from each body and the NPC swap callers): pl_carry_item_get, pl_item_room_get
 *   GUESS (from each body and its callers): pl_act_name_row_get
 * RESIDUALS. 122 functions unwritten (objdiff scores them 0) in 18 runs: 0x802695A4-0x80269998, 0x80269AC4-0x8026A00C,
 *   0x8026A0D4-0x8026A224, 0x8026A248-0x8026A2BC, 0x8026A3A8-0x8026A4E4, 0x8026A5A4-0x8026A618, 0x8026A738-0x8026BA04,
 *   0x80274B20-0x80274B5C, 0x8027D7EC-0x8027D874, 0x8027D8A0-0x8027DC64, 0x8027E120-0x8027E1B8, 0x8027E404-0x8027E918,
 *   0x8027E98C-0x8027EC50, 0x8027ED28-0x8027EE08, 0x8027EE24-0x8027EFB4, 0x8027F008-0x8027FC70, 0x8027FC84-0x8027FF20,
 *   0x8027FF30-0x802840DC.  Known blockers among them: the rig at 0x80269AC4-0x80269F88 (`new` of 0x1560 bytes) needs
 *   its 0x90/0x110-byte element types (vtables 0x805BAB58/0x805BAB74); `Pl_chr_set_attr`/0x8026A178/`Pl_chr_setX`
 *   funnel into `fn_800E12CC`, whose argument order is open; `fn_8027D8A0`, `fn_8027D968`, `fn_8027E5E4` and
 *   `fn_8027F3CC` carry paired-single / `cror` instructions the front end cannot emit.
 *  - flipcheck's undefined reference `get_ControlType` (the map's `get_ControlType__Fl` needs C++ scope): a linkage
 *    item for a fixer.
 *  - 90 functions partial in 51 runs (`sweepcomments.py --unit Pl/pl_act` lists them), including:
 *  - `fn_80270F50`: the implicit int->float conversion constant rows (compiler-synthesised, playbook 29);
 *  - `fn_80271BD4`/`fn_80271E0C`: MWCC's `switch` decision tree against retail's three linear `subi`/`cmplwi` range
 *    tests, and in `fn_80271E0C` the `deco_count` bound retail re-masks from r0; the label-chain shape that rule 8
 *    removed reproduced more rows (docs/pl.md);
 *  - `Pl_cat_skill_ck`: MWCC reassociates `(plw + 4) + 0x612` into `plw + 0x616` for every walk tried;
 *  - `fn_8027350C`: the three call-spanning values are coloured base/`pend`/mask in r24/r25/r26 by retail, mask/base/
 *    `pend` by ours;
 *  - `fn_802751B4`: retail saves `f31` through a `psq_st`/`vmrghb` pair (the compiler's own float-save form);
 *  - `fn_80278144`/`fn_80278310`: the early `(u32)((u8)m - 6) <= 1` exit inlines `li r3,0; b epilogue` (+8 B each)
 *    where retail shares one block through a label (rule 8; docs/pl.md);
 *  - `Pl_motion_input_ck`: the case-3/5 failures return inline where retail branches to a shared `return 0`;
 *  - `fn_8027A57C`: the conformant if/else-if dispatch interleaves the case tests retail lays out `switch`-shaped;
 *  - `fn_8027885C`: retail's `mr r0,r3; mr r3,self; extsh r4,r0` says `fn_802753E4` returns `s16`; the declaration
 *    stays wider (`fn_802789EC`/`fn_80278D1C` depend on it) and each of the five `(s16)` call sites costs one row;
 *  - `fn_8027B0BC`: six `clrlwi r0,r0,24` before the `stb`s into `q + 0x5E1` (a peephole-level residual);
 *  - `pl_carry_item_get`: a `v`/`id` callee-saved swap; `fn_8027D0D4`: retail gives the jump table r23 and the loop bound
 *    r30, ours the mirror (playbook 22); `fn_8027C208`: the frame is 0x30 against retail's 0x40 (unused locals);
 *  - `fn_8027A340`: an extra `li r31,0`/`b` pair where retail shares case 2's block with the guard's edge;
 *    `Pl_bari_ck`: retail lays the first arm out of line and the second inline, the natural chain is the mirror;
 *  - `fn_8027DCA8` (`return a || b` shares the `li r3,1` tail), `fn_8027DCE0` (retail merges every `return 0`),
 *    `fn_8027DE88`/`fn_8027DF38` (retail lays the id-scan loop head after the body), `fn_8027DFE4` (the `case 12`
 *    equality if-converts to the branchless bool form), `fn_8027DDC4` (written as the negated/nested if-chain; retail
 *    compares the id signed against 0x41/0x9A where ours is unsigned, and its arm order differs by one `b`);
 *  - `fn_8027CB1C`: retail re-tests the second byte's range (`li r0,0xff; cmplwi; beq; cmplw; bge`) where ours
 *    folds it (20 B short); `fn_8027D40C`: the 32-bit scan's two induction variables sit in r5/r6 swapped;
 *  - `fn_8027CFC0`: two extra `extsh` before its clamp stores (+8 B);
 *  - `fn_80277FF8`: the `t`-vs-`arg1` test polarity - about a dozen rows around an inlined `li r3,0; blr` (+12 B);
 *  - the other 63 partial rows have no recorded cause.
 *  - `fn_8026CC7C` is byte-identical; its switch table relocation goes through MWCC's local `@NNNN` where the target
 *    names `jumptable_805C5FA0` (same address).  The extab record names agree (`@etb_800126CC` is in both objects).
 *  - the action and equipment sections are not in `.text` order (`Pl_attack_set_sub` and `fn_80277974`.. sit after
 *    `fn_8027B0BC`); the object's function order is source order, so the file has to
 *    be sorted before the unit can link.
 *  - `.data`: retail has a zero word at 0x805C60A4 between the act-entry section's second switch table and its pick
 *    table that MWCC does not emit (it puts a zero-initialised variable in `.bss`/`.sdata`), so the pick table sits 4
 *    bytes early.
 *  - flipcheck: the object emits no `.bss` (0x20 claimed), `.rodata` (0x40), `.sbss` (0x8) or `.sdata` (0x48);
 *    `.text` 0x12E5C, `.data` 0x174, `.sdata2` 0x10, extab 0x548 and extabindex 0x7EC against the claims 0x1AD18,
 *    0x2178, 0x1D8, 0x928 and 0xD50; every compared section differs in bytes.
 * SHAPES. A `switch` on an unsigned value puts the case blocks out of line with a forward `b` to the default
 *   (`fn_8027176C`, `fn_80271674`, `fn_80271A18`); `switch (u8)` promotes to a signed `cmpwi` chain.
 *  - the pool floats are `extern` declarations used as load operands, never definitions (playbook 29), and a multiply
 *    is written `lbl * value`, the target's `fmuls` operand order;
 *  - `pl_act_param_tier_ck`: the dead copy chain `classCopy0..2` of `self->field_0x002` plus a separate `weaponClass`
 *    load fixes MWCC's web order so `value` lands in r4; the load stays `(s16)self->part_tbl_b_0xD4[idx]`, above the
 *    early-out (docs/pl.md);
 *  - `fn_8026BF98`/`fn_8026CC7C` take the action state as a local `st` (retail keeps the block base in a callee-saved
 *    register); `fn_8026BF98`'s `Vec3` loop is `do { } while (p < &vec[16])` (a `for`/`while` adds a guard), it reads
 *    `st->unk7D` through a `switch` with `default:` first (`cmpwi`, fall-through layout), and it tests the inner
 *    request bits through `self->unkBC` but the enclosing ones through `st->flagsBC` (the base register shows);
 *  - `fn_8026CC7C` is the m2c reconstruction: its `1U`/`s32` spellings and `st->flagsB8`/`self->unkCC` base choices
 *    are load-bearing; `fn_8026CBE4` keeps two unused trailing parameters (its callers pass four);
 *  - `fn_80271BD4`/`fn_80271E0C` dispatch with `switch ((u32)kind)`: cases 1-5, case 6, then 7-15 as `default`, which
 *    lays the bodies out in retail's B, C, A order;
 *  - the record walks index the typed arrays (`rec->skill_id[i]`); `fn_80273044`/`pl_item_room_get` scan `plw->slot_id`
 *    with a flat `for (i = 0; i < 24; i++)` (unrolled eight-wide under `mtctr 3`), and `pl_item_room_get`'s spare-slot scan
 *    is a `u16*` walk (`p[i * 2]`); `Pl_cat_skill_ck` is a two-iteration loop over a `u16*` walk (`p += 2`);
 *  - the valid-bit updates in `fn_80273998`/`fn_8027373C`/`fn_802738E8` are compound (`plw->set_valid &= (u16)~mask`);
 *    the plain assignment adds a `clrlwi` before the `sth`;
 *  - narrow returns decide where MWCC re-converts: `item_take` and `pl_item_add` return `s16`, `fn_802736A0`'s
 *    `Get_pl_type__FP6_EQUIPP6_EQUIP` returns `u8`, `fn_802724E8`/`fn_8027252C` take signed-byte `s8*`/`s8` levels;
 *    `fn_80272D5C` restates `(s16)v` at its second store and compare;
 *  - `fn_8027252C` declares `tv, tb, ta` in that order (stack slots 0x20/0x18/0x10 and the register order), nests the
 *    value test `if (v != 0) { if (v > 0) ... } else { zeros }` and swaps tv/tb before ta; `fn_80272B10` indexes
 *    `plw->slot_id` with `?:` and calls `GetItemData` in each arm; `pl_item_add` skips its second lookup through a
 *    single-iteration `for (;;)` with `break`, and its `case 4` compares `plw->unk26E` against the first lookup;
 *  - `fn_80273B14` and `fn_80274B5C` write their case bodies in the order their jump tables point at, not case order;
 *  - `Pl_net_send` is called through its two-parameter view `PlNetSend2`: retail's call sites pass two arguments to
 *    the owner's three-parameter function, and a third argument would emit `li r5,X`;
 *  - an `s16`/`s8` parameter with a compound assignment stores the raw sum and sign-extends only at the compare
 *    (`fn_80279154`, `fn_80279194`, `fn_8027C89C`, `fn_8027A044`, `fn_80276CE8`, `fn_8027CA48`, `fn_80276E08`,
 *    `fn_80278674`, `fn_8027D5A4`; playbook 38); `field = field + value` and a temporary extend at the store, and
 *    `field--` stores raw where `field = field - 1` extends (`fn_8027B0BC`);
 *  - a 32-bit accumulator that retail sign-extends only at each compare is an `s32` with `b = (s16)(b + N)` on the
 *    arms that convert and `a -= b` for the difference (`fn_80279490`, `fn_802791FC`, `pl_carry_item_get`);
 *  - equality on a `u16`/`u8` value pairs as `cmpwi` through a signed local or cast (`s32 id = self->act_no`,
 *    `(s32)(u8)x`, `(s32)arg0 == N`) and `(u32)` on an `s32` helper gives `cmplwi` (`fn_8027C8B4`, `fn_802789EC`);
 *  - `x <= N ? 1 : 2` gives retail's `xoris/subfic/addc/subfe/addi` and `x == N ? a : b` its `addi/subfic/nor/srawi`
 *    (`Get_Shell_bure_type`, `fn_802791FC`, `fn_80279490`); a constant before the index (`lbl + k + idx * 4`) makes
 *    MWCC form the base first;
 *  - `extern u8 lbl_80792150;` taken by address gives retail's `@sda21` form where an unsized array gives `lis/addi`;
 *  - `fn_802784B8`'s first case falls through into a `default:` written between the cases; `fn_8027A57C`'s shared
 *    tail is the `switch`'s `default:` after the last case; `fn_8027C064` writes its `default:` first;
 *  - `fn_8027C208` ends in `return 0` with an explicit `default: return 1`, and every refused path is
 *    `if (!c) return 1; break;` (the `if (c) return 0; return 1;` form if-converts; playbook 34);
 *  - `fn_8027A340` writes its cases in body address order (0, 4, 2) with the range guard negated and its arms swapped;
 *  - `fn_8027AF34` keeps the pre-decrement value in an `s16` local and an unreachable `if (v < 0) return;` after the
 *    store, which reproduces retail's `blelr ... sth ... bltlr`;
 *  - `pl_pos_blend_start` writes `(f32)(s16)arg1` inline at each division (retail recomputes the conversion);
 *  - `fn_80278590` writes `* 14` as `(x * 7) << 1` (strength reduction kept); `fn_8027993C` casts its `u16` index to
 *    `s16` each iteration; `fn_8027C064`'s local declaration order fixes its stack slots; `fn_80277C94` spells its
 *    constant `-0x1006`; `fn_80279414`/`Get_Shell_rate_adj` read `((u8*)&self->equipC)[0]` before forming the pointer;
 *  - `fn_8027D0D4`'s 15-case `switch` is a jump table and one arm assigns the loop counter (`i = 5`).
 */

/* ==== 0x802693C4-0x8026BA1C: the part/motion cluster ==== */

#include "types.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80224AC4.h"
#include "Pl/pl_skill.h"
#include "sound/fn_800DD1F0.h"
#include "Pl/pl_act_step_data.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_equip_page.h"
#include "menu/hit_attack_list_push.h"
#include "unsplit/Pl.h"

/* The joint holder `_PLW::physics_0x13C` points at: the `MHchar` the player's joints are read from
 * sits at `+0x04`, the same layout `ef/eft001.cpp` names `joint_0x004`. size: 0x4 + sizeof(MHchar) */
typedef struct PlJointHolder {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ MHchar chr_0x04;
} PlJointHolder;

/* The six symbols the map spells mangled are defined at C++ scope so the front-end reproduces the
 * map's names (rules 9 and 50); the rest are the map's unmangled `fn_` stems and are `extern "C"`. */

void set_com_motion_type(u8 type)
{
    if (lbl_80794B28 != NULL) {
        lbl_80794B28->com_motion_type = type;
    }
}

u16 Get_motion_no(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.motion_no_0x50;
}

/* The two flags are the callers' (`frame` is masked to 16 bits); the body passes a constant 0/1 to
 * the model layer in their place, which is why the two floats are unused here. */
u32 Pl_frame_check(_PLW* self, u32 frame, f32 a, f32 b)
{
    return fn_800E16DC(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, (u16)frame, 0);
}

void pl_get_joint_wpos(_PLW* self, u32 joint, nw4r::math::VEC3* out)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.get_joint_wpos(joint, out);
}
namespace s_802693C4 {


extern "C" {

/* Arms one part of one player's object record: 0 when the part is already armed (its state is 1) or
 * was consumed (> 1), 1 after the state moves to 2 and the part's gate is cleared. */
u32 fn_802693C4(s32 player, u8 part, s32 value)
{
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    _PLOBJ* obj;
    _PLWORK* work;

    obj = &lbl_80794B28->table[player];
    if (obj->state[part] > 1) {
        return 0;
    }
    if (obj->group[3][part] == value) {
        obj->state[part] = 1;
        return 0;
    }
    obj->part_flag_0x0B[part] = 0;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    work = &lbl_80794B28->objects[player];
    if (part < 7) {
        ((PlSeRig*)work->model_0x13C)->field_0x9C0[part] = 0;
    } else {
        ((PlSeRig*)work->model_0x13C)->field_0x9C7[part - 7] = 0;
    }
    return 1;
}

/* Consumes one part: 0 when the part already carries this value, 1 after the flag is set, the value
 * stored and the state moved to 2; a consumed part (> 1) is reported as 0. */
u32 fn_80269474(s32 player, u8 part, s32 value)
{
    _PLGLOBAL* g = lbl_80794B28;
    _PLOBJ* obj;

    if (g == NULL) {
        return 0;
    }
    obj = &g->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] > 1) {
        return 0;
    }
    obj->part_flag_0x0B[part] = 1;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    ((PlSeRig*)lbl_80794B28->objects[player].model_0x13C)->field_0x9C0[part] = 0;
    return 1;
}

/* The value stored for one part of the player the work record names, or -1 with no global or no
 * work record. */
s32 fn_80269508(_PLW* self, s32 part)
{
    if (self == NULL) {
        return -1;
    }
    if (lbl_80794B28 == NULL) {
        return -1;
    }
    return lbl_80794B28->table[self->chunk_ofs].group[3][part];
}

/* 0 when the part already carries this value, 1 while it is still free, -1 once it is consumed. */
s32 fn_8026954C(s32 player, u8 part, s32 value)
{
    if (lbl_80794B28 == NULL) {
        return -1;
    }
    _PLOBJ* obj = &lbl_80794B28->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] <= 1) {
        return 1;
    }
    return -1;
}

/* Returns 0 with no global, otherwise the global's common motion type. */
u8 fn_802699AC(void)
{
    if (lbl_80794B28 == NULL) {
        return 0;
    }
    return lbl_80794B28->com_motion_type;
}

/* Clears one player's object record: a part mid-load (state 4 or 5) drops its load request, then the state and
 * the six value words go to -1 and the player's slot byte to 0. */
void fn_802699C8(u8 player)
{
    _PLGLOBAL* g = lbl_80794B28;
    _PLOBJ* obj;
    s32 i;

    if (g == NULL) {
        return;
    }
    obj = &g->table[player];
    for (i = 0; i < 7; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    for (; i < 11; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    g->slot_state[player] = 0;
}

/* The motion-number table lookup for the 0-999 range: row `id / 100`, entry `id % 100`. */
s16 fn_8026A00C(u16 id)
{
    if (id >= 1000) {
        return 0;
    }
    return lbl_805C0C98[id / 100][id % 100];
}

/* The motion-number table lookup for the 1000+ range: two levels, the first keyed by the work
 * record's own byte, the second by `(id - 1000) / 100`. */
s16 fn_8026A068(_PLW* self, u16 id)
{
    u16 index = (u16)(id - 1000);
    u8 kind = self->field_0x002;
    s16* row = lbl_805C1584[kind][index / 100];

    if (row == NULL) {
        return 0;
    }
    return row[index % 100];
}

/* The character attribute setter's argument shuffle: the motion word is narrowed and the fifth
 * argument is left 0. */
void Pl_chr_set_attr_default(_PLW* self, u16 motion, s32 a, s32 b)
{
    Pl_chr_set_attr(self, motion, a, b, 0);
}

/* The character motion setter's argument shuffle (`fn_8026A178` takes a sixth, always-0 argument). */
void fn_8026A230(_PLW* self, s32 a, u16 motion, s32 b, s32 c)
{
    fn_8026A178(self, a, motion, b, c, 0);
}

/* The model block's per-index value setter. */
void fn_8026A23C(_PLW* self, s32 index, f32 value)
{
    fn_800E26B4(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, index, value);
}

/* Hands the model layer the scale the work record carries. */
void fn_8026A2BC(_PLW* self)
{
    fn_800E1640(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, self->field_0x354);
}

void fn_8026A2D0(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = value;
}

void fn_8026A2DC(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = 0;
}

void pl_model_set_state(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = value;
}

void fn_8026A2F8(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = 0;
}

/* The same frame check as `Pl_frame_check`, with the model layer's second flag set. */
u32 fn_8026A328(_PLW* self, u16 frame, f32 a, f32 b)
{
    return fn_800E16DC(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, frame, 1);
}

/* The model layer's frame reset. */
u32 Pl_motion_end_ck(_PLW* self)
{
    return fn_800E2198(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, 0);
}

f32 fn_8026A34C(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x74;
}

f32 pl_rig_get_float_a4(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xA4;
}

/* Whether the model's blend value is above zero. */
u32 fn_8026A364(_PLW* self)
{
    return !(((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xBC < lbl_8079A000);
}

/* The named joint's world position, through the model. */
void fn_8026A394(_PLW* self, s32 joint, nw4r::math::MTX34* out)
{
    mhchar_joint_mtx_get(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, joint, out);
}

/* The work record's actor-mode byte. */
u8 fn_8026A3A0(_PLW* self)
{
    return self->field_0x18;
}

/* The three motion integrators: position += velocity, velocity += acceleration, and both zeroed. */
void fn_8026A4E4(_PLW* self)
{
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void fn_8026A518(_PLW* self)
{
    self->motion_vel_0x78 = self->motion_vel_0x78 + self->motion_acc_0x84;
    self->motion_vel_0x7C = self->motion_vel_0x7C + self->motion_acc_0x88;
    self->motion_vel_0x80 = self->motion_vel_0x80 + self->motion_acc_0x8C;
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void fn_8026A570(_PLW* self)
{
    self->motion_acc_0x8C = lbl_8079A000;
    self->motion_acc_0x88 = lbl_8079A000;
    self->motion_acc_0x84 = lbl_8079A000;
    self->motion_vel_0x80 = lbl_8079A000;
    self->motion_vel_0x7C = lbl_8079A000;
    self->motion_vel_0x78 = lbl_8079A000;
}

void fn_8026A590(_PLW* self)
{
    self->motion_acc_0x8C = lbl_8079A000;
    self->motion_acc_0x88 = lbl_8079A000;
    self->motion_acc_0x84 = lbl_8079A000;
}

/* The three per-id flag words (128 ids each at +0xE0 and +0xF0, one word at +0xDC): set, test and
 * clear. */
void fn_8026A618(_PLW* self, s32 id)
{
    self->id_flags_0xE0[id / 32] |= 1 << (id & 31);
}

u32 pl_part_flag_ck(_PLW* self, s32 id)
{
    return (self->id_flags_0xE0[id / 32] & (1 << (id & 31))) != 0;
}

void fn_8026A678(_PLW* self, s32 id)
{
    self->id_flags_0xF0[id / 32] |= 1 << (id & 31);
}

s32 fn_8026A6A4(_PLW* self, s32 id)
{
    return (self->id_flags_0xF0[id / 32] & (1 << (id & 31))) != 0;
}

void fn_8026A6D8(_PLW* self, s32 id)
{
    self->id_flags_0xDC |= 1 << (id & 31);
}

s32 fn_8026A6F4(_PLW* self, s32 id)
{
    return (self->id_flags_0xDC & (1 << (id & 31))) != 0;
}

void fn_8026A718(_PLW* self, s32 id)
{
    self->id_flags_0xDC &= ~(1 << (id & 31));
}

/* Whether the work record's second flag bit is set. */
u32 fn_8026BA04(_PLW* self)
{
    return (self->field_0x134 & 2) != 0;
}

} /* extern "C" */

} /* namespace s_802693C4 */

/* ==== 0x8026BA1C-0x8026FFBC: the master layer (the actor's action-state block) ==== */

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

typedef nw4r::math::VEC3 Vec3;

/* The actor's 132-byte action-state block, `self + 0xB8`: `fn_8026F7B4`/`fn_8026F828` clear it, the two
 * big dispatchers (`fn_8026BF98`, `fn_8026CC7C`) drive it, and `fn_8026BE94` reads the request word and the
 * per-part selector out of it, so one type has to carry both views. Retail materialises the base pointer
 * once and keeps it in a callee-saved register - hence the local `st` in the dispatchers below. */
typedef struct ActState {
    u16 flagsB8;         /* 0x00 - self->field_0x0B8: request bit word A (the "kind" mask fn_8026BE94 tests) */
    u16 unk02;           /* 0x02 */
    u16 flagsBC;         /* 0x04 - self->unkBC: request bit word B */
    u8 pad06[0x0A];      /* 0x06 */
    u16 unk10;           /* 0x10 - the word fn_8026BE94 tests 0x2000/0x1000/0x800/0x400 against */
    u16 unk12;           /* 0x12 */
    u16 flagsCC;         /* 0x14 - self->unkCC: per-part behaviour bits */
    u8 pad16[0x3E];      /* 0x16 */
    f32 unk54;           /* 0x54 - self->unk10C: the charge/axis value fn_8026CC7C bands */
    f32 unk58;           /* 0x58 - self->unk110: the weapon/model axis length */
    u8 pad5C[0xC];       /* 0x5C */
    u8 part[8];          /* 0x68 - per-part selector table (fn_8026BE94 indexes it, the dispatchers read part[1]) */
    u8 mode;             /* 0x70 - self->unk128: the dispatchers' switch value */
    u8 unk71;            /* 0x71 - self->field_0x129: the "an action is running" latch */
    u8 pad72[6];         /* 0x72 */
    s32 unk78;           /* 0x78 - self->unk130 */
    u8 unk7C;            /* 0x7C - self->unk134: low two bits are a per-part flag */
    u8 unk7D;            /* 0x7D - self->unk135 */
    u8 unk7E;            /* 0x7E - self->unk136: the deferred-command bits */
    u8 unk7F;            /* 0x7F - self->unk137: the hold delay */
    u16 unk80;           /* 0x80 - self->unk138: flags latched for the hold */
    u16 unk82;           /* 0x82 - self->unk13A: flags latched for the hold */
} ActState;
namespace s_8026BA1C {


/* Unmangled map names: `extern "C"` so the compiler emits the map's spelling. */
extern "C" {
void fn_8026A618(_PLW* self, u32 id);
u32 pl_part_flag_ck(_PLW* self, u32 id);
void fn_8026A678(_PLW* self, u32 id);
u32 fn_8026A6F4(_PLW* self, u32 id);
u32 fn_8026B99C(_PLW* self);
u32 fn_8026B934(_PLW* self);
u32 fn_8026BA04(_PLW* self);
u32 fn_803BECC8(u8 value, u32 low, u32 high);
void fn_8026AF08(_PLW* self, u32 value);
void fn_800E09D0(void* dst, void* src);
u8 GameMode_ck(void);
s8 my_player_no(void);
u32 lobby_input_locked_ck(void);
u16 fn_802BE038(void);
void* memset(void* dst, int value, u32 size);
}

/* Pool literals owned by a neighbouring Pl unit: 60.0f and the two charge bands `fn_8026CC7C` compares
 * `st->unk54` against. Declared, not defined - defining them would rebuild the section (playbook 29). */
extern "C" { extern f32 lbl_8079A020; }
extern "C" { extern f32 lbl_8079A024; }
extern "C" { extern f32 lbl_8079A028; }
} /* namespace s_8026BA1C */


/* Mangled map names: written by their source name so the compiler emits the map's spelling. */
u8 PlayMode_ck(void);
u8 get_now_areano(void);
u32 Pl_Skill_ck(_PLW* self, u16 skill);
namespace s_8026BA1C {


/* This unit's own order: the file has to emit the functions in the map's address order. */
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx);
extern "C" void fn_8026CC7C(_PLW* self);
extern "C" void fn_8026BA1C(_PLW* self);
} /* namespace s_8026BA1C */

u32 Pl_master_ck(_PLW* self);
namespace s_8026BA1C {

extern "C" u8 pl_act_param_tier_ck(_PLW* self, u32 idx);
extern "C" u32 fn_8026F9A4(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FB20(_PLW* self, s32 kind);
extern "C" u32 fn_8026FC40(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3);

/* 0x8026BA1C: the master action dispatcher - picks the actor's action id from the angle window the
 * target lies in, per weapon class. */
extern "C" void fn_8026BA1C(_PLW* self)
{
    fn_8026A618(self, 4);
    fn_8026A618(self, 25);
    if (self->field_0x129 == 0) {
        if (fn_8026FA6C(self, 0, -9102, 9102) == 1) {
            fn_8026A618(self, 62);
            fn_8026A618(self, 68);
        } else if (fn_8026FA6C(self, 0, 23666, -23666) == 1) {
            fn_8026A618(self, 63);
            fn_8026A618(self, 69);
        } else if (fn_8026FA6C(self, 0, -23666, -9102) == 1) {
            fn_8026A618(self, 61);
            fn_8026A618(self, 67);
            fn_8026A618(self, 65);
        } else if (fn_8026FA6C(self, 0, 9102, 23666) == 1) {
            fn_8026A618(self, 60);
            fn_8026A618(self, 66);
            fn_8026A618(self, 64);
        }
    } else {
        if (self->part_tbl_b_0xD4[0] < 50 && self->unk128 == 1) {
            if (self->unk110 <= lbl_8079A020) {
                fn_8026A618(self, 64);
            } else {
                fn_8026A618(self, 65);
            }
        }
        switch (self->field_0x002) {
        case 6:
            if (fn_8026FA6C(self, 0, -8192, 8192) == 1) {
                fn_8026A618(self, 62);
            } else if (fn_8026FA6C(self, 0, 24576, -24576) == 1) {
                fn_8026A618(self, 63);
            } else if (fn_8026FA6C(self, 0, -24576, -8192) == 1) {
                fn_8026A618(self, 61);
            } else if (fn_8026FA6C(self, 0, 8192, 24576) == 1) {
                fn_8026A618(self, 60);
            }
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 66);
            }
            break;
        case 3:
            if (fn_8026FA6C(self, 0, -5461, 16384) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 16384, -27307) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, -27307, -5461) == 1) {
                fn_8026A618(self, 61);
                fn_8026A618(self, 67);
            }
            break;
        default:
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 60);
                fn_8026A618(self, 66);
            }
            break;
        }
    }
}

#pragma peephole off

/* 0x8026BE94: raises the requests an action's own kind mask and state word imply. */
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx)
{
    if ((req->flagsB8 & mask) == 0) {
        u8 part = req->part[idx];
        if ((u8)(part - 1) <= 7) {
            fn_8026A618(self, 54);
        }
    }
    if (req->unk10 & 0x2000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 56);
    } else if (req->unk10 & 0x1000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 57);
    }
    if (req->unk10 & 0x800) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 58);
    } else if (req->unk10 & 0x400) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 59);
    }
}

/* 0x8026BF98: the action dispatcher. Switches on the action state's mode byte and raises the command set
 * the actor's request words and hold state imply. */
extern "C" void fn_8026BF98(_PLW* self)
{
    ActState* st = (ActState*)&self->field_0x0B8;
    Vec3 vec[16];
    Vec3* p = vec;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < &vec[16]);

    switch (st->mode) {
    case 2:
        switch (st->unk7D) {
        default:
                if ((st->flagsBC & 0x120) == 0x120) {
                    st->unk7E |= 8;
                } else {
                    st->unk7E = 0;
                    if (st->flagsBC & 0x320) {
                        st->unk7D++;
                        st->unk7F = 2;
                        st->unk80 = st->flagsBC & 0x320;
                        st->unk82 = st->flagsBC & 0x100;
                    } else {
                        st->unk80 = 0;
                        st->unk82 = 0;
                    }
                }
            break;
        case 1:
                if (--st->unk7F == 0 || (st->flagsBC & 0x320) != 0) {
                    if (st->flagsBC & 0x320) {
                        if (st->flagsBC & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->flagsBC & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->flagsBC & 0x20) {
                            st->unk7E |= 2;
                        }
                        if (((u16)(st->flagsBC | st->unk80) & 0x120) == 0x120) {
                            st->unk7E |= 8;
                        }
                    } else {
                        if (st->unk80 & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->unk80 & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->unk80 & 0x20) {
                            st->unk7E |= 2;
                        }
                    }
                    st->unk7D = 0;
                } else if ((st->flagsB8 & 0x320) == 0) {
                    if (st->unk80 & 0x200) {
                        st->unk7E |= 4;
                    }
                    if (st->unk80 & 0x100) {
                        st->unk7E |= 1;
                    }
                    if (st->unk80 & 0x20) {
                        st->unk7E |= 2;
                    }
                    st->unk7D = 0;
                }
            break;
        }
        fn_8026BE94(self, st, 0x80, 2);
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 34);
            fn_8026A618(self, 37);
            fn_8026A618(self, 50);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 22);
        }
        if (self->held_attack_0x5C5 != 0) {
            if (st->flagsBC & 0x8000) {
                fn_8026A618(self, 8);
            }
            if (st->unk7E & 1) {
                fn_8026A618(self, 10);
                fn_8026A618(self, 11);
            }
            if (st->unk7E & 2) {
                if (self->field_0x308 == 0) {
                    if (pl_act_param_tier_ck(self, 0) >= 1) {
                        fn_8026A618(self, 27);
                    } else {
                        fn_8026A618(self, 26);
                    }
                }
                fn_8026A618(self, 76);
            }
        } else {
            if (st->unk7E & 1) {
                fn_8026A618(self, 8);
                fn_8026A618(self, 76);
            }
            if ((st->flagsCC & 0x3C) != 0 && self->field_0x308 == 0) {
                if (st->flagsCC & 0x10) {
                    fn_8026A618(self, 12);
                    fn_8026A618(self, 24);
                }
                fn_8026A618(self, 11);
                fn_8026A618(self, 10);
            }
            if (st->flagsB8 & 0x100) {
                if (pl_act_param_tier_ck(self, 0) >= 1) {
                    fn_8026A618(self, 27);
                } else {
                    fn_8026A618(self, 26);
                }
            }
            if (st->flagsBC & 0x20) {
                fn_8026A618(self, 13);
                fn_8026A618(self, 14);
            }
        }
        if (st->flagsB8 & 0x80) {
            if (st->unk7E & 8) {
                fn_8026A618(self, 18);
                fn_8026A618(self, 19);
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            } else if (st->unk7E & 1) {
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            }
            fn_8026A618(self, 28);
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 17);
        }
        if (fn_803BECC8(self->chunk_ofs, 10, 1) == 1) {
            if (st->flagsB8 & 4) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x10) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 0x10) {
            fn_8026A618(self, 39);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 38);
        }
        if (st->flagsBC & 0x10) {
            fn_8026A618(self, 33);
            fn_8026A618(self, 49);
        }
        if (st->flagsBC & 4) {
            fn_8026A618(self, 32);
            fn_8026A618(self, 48);
        }
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            fn_8026A618(self, 47);
            fn_8026BA1C(self);
        } else if (st->unk71 != 0) {
            if (self->unkBC & 4) {
                fn_8026A618(self, 64);
            } else if (self->unkBC & 0x10) {
                fn_8026A618(self, 65);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 5);
            fn_8026A618(self, 6);
            fn_8026A618(self, 9);
            fn_8026A618(self, 53);
            if (self->field_0x308 == 0) {
                fn_8026A618(self, 2);
                fn_8026A618(self, 52);
                fn_8026A618(self, 77);
            }
        }
        if ((st->flagsB8 & 0x80) != 0 && (st->flagsBC & 0x20) != 0) {
            fn_8026A618(self, 71);
            fn_8026A618(self, 73);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 70);
                fn_8026A618(self, 72);
            }
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 21);
        }
        break;
    case 1:
        if (fn_803BECC8(self->chunk_ofs, 10, 1) == 1) {
            if (st->flagsB8 & 0x2000) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x1000) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 50);
            fn_8026A618(self, 37);
        }
        if (st->flagsBC & 0x80) {
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x20) != 0 || ((st->flagsBC & 0x200) != 0 && self->field_0x308 == 1)) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
        }
        if ((st->flagsB8 & 0x100) == 0 && (u8)(st->part[1] - 1) <= 7) {
            fn_8026A618(self, 47);
        }
        if (st->flagsBC & 0x100) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            if (st->unk78 <= 24) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 32);
                } else {
                    fn_8026A618(self, 33);
                }
            }
            fn_8026BA1C(self);
        }
        if (st->flagsB8 & 0x100) {
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 38);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 48);
                }
            } else {
                fn_8026A618(self, 39);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 49);
                }
            }
        }
        fn_8026BE94(self, st, 4, 7);
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 8);
        }
        if ((st->flagsBC & 0x200) != 0 || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 6);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 52);
                fn_8026A618(self, 2);
                fn_8026A618(self, 5);
            }
        }
        if (((st->flagsB8 & 4) != 0 && (st->flagsBC & 0x200) != 0) || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 71);
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 73);
            }
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 70);
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 72);
                }
            }
        }
        if ((st->flagsBC & 0x220) != 0 || (st->unk7C & 4) != 0) {
            fn_8026A618(self, 9);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 21);
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 20);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 76);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 53);
                fn_8026A618(self, 77);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 22);
        }
        if (fn_8026B934(self) == 1) {
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x200) != 0 && self->field_0x308 == 0) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                fn_8026A618(self, 27);
            } else {
                fn_8026A618(self, 26);
            }
        }
        break;
    }
}

#pragma peephole off

/* 0x8026CBE4 */
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3)
{
    if (self->unk18 == 1) {
        u16 flags = which == 0 ? self->field_0x0B8 : self->unkBC;
        if ((flags & 4) != 0) {
            return 1;
        }
    } else if (which == 0) {
        if ((self->field_0x0B8 & 0x44) == 0x44) {
            return 1;
        }
    } else if ((self->unkBC & 0x44) != 0 && (self->field_0x0B8 & 0x44) == 0x44) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026CC70: the retail body compares a byte of its second argument's structure and returns the first. */
extern "C" u32 fn_8026CC70(u32 value, u8* data)
{
    switch ((u32)data[0x70]) {
    case 1:
        return value;
    }
    return value;
}


/* 0x8026CC7C: the per-weapon-class action dispatcher - a nine-entry jump table on the actor's weapon
 * class, each arm a further switch on the action state's mode byte. */
extern "C" void fn_8026CC7C(_PLW* self)
{
    ActState* st = (ActState*)&self->field_0x0B8;
    fn_8026CC70((u32)self, (u8*)st);
    switch (self->field_0x002) {
    case 0:
        switch (st->mode) {
        case 2:
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xC);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0x10);
                    } else {
                        fn_8026A678(self, 0x12);
                        fn_8026A678(self, 0x13);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->flagsB8 & 0x100) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0x11);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0xC);
                } else if (fn_8026F9A4(self, 1, 0xE000, 0x2000) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                } else if (fn_8026F9A4(self, 1, 0x6000, 0xA000) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                } else if (fn_8026F9A4(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 4);
                }
                if (fn_8026FC40(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 5);
                }
                if (fn_8026FC40(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0x11);
                    return;
                }
            }
            break;
        case 1:
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                    if (pl_part_flag_ck(self, 1) == 0) {
                        fn_8026A618(self, 0xC);
                        fn_8026A618(self, 0x18);
                    }
                }
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((s32) (st->flagsB8 & 0x200) != 0) {
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 9);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 8);
                    } else {
                        fn_8026A678(self, 7);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x10);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 0xA);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0xF);
            }
            if ((s32) (st->unk7C & 1) != 0) {
                fn_8026A678(self, 0xD);
                return;
            }
            break;
        }
        break;
    case 4:
    case 5:
    case 6:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                if (((s8) st->part[2] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
            } else if ((u8) (st->part[2] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if (((s32) (st->unk7E & 8) != 0) && ((s32) self->unk5E6 == 0)) {
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 4);
                }
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->unk7E & 8) != 0)) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0x18);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 4);
                }
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 6);
                    return;
                }
                if (((s32) self->field_0x308 == 0) && ((s32) (self->unkCC & 0x10) != 0)) {
                    fn_8026A678(self, 5);
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0x18);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && ((s32) (st->flagsB8 & 0x100) != 0)) {
                fn_8026A678(self, 8);
            }
            if ((s32) (st->flagsB8 & 4) != 0) {
                if (((s8) st->part[7] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
                if ((s32) (st->flagsBC & 0x40) != 0) {
                    fn_8026A618(self, 0x18);
                }
            } else if ((u8) (st->part[7] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 5);
                if ((s32) (st->flagsB8 & 4) != 0) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0xB);
                }
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((fn_8026B99C(self) == 1U) && ((s32) self->unk5E6 == 0)) {
                fn_8026A678(self, 6);
                return;
            }
            break;
        }
        break;
    case 1:
        switch (st->mode) {
        case 2:
            if ((s32) self->held_attack_0x5C5 != 0) {
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xB);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 1);
                        fn_8026A618(self, 0x4B);
                        return;
                    }
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0x4A);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x200) != 0) {
                    fn_8026A678(self, 0x15);
                }
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xC);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A618(self, 0x4B);
                    } else {
                        fn_8026A618(self, 0x4A);
                    }
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 9);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (pl_part_flag_ck(self, 1) == 0)) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((fn_8026A6F4(self, 0xB) == 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 7);
                }
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 6);
                    }
                }
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 3);
                }
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 6);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 9);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        if (pl_act_param_tier_ck(self, 0) >= 1U) {
                            fn_8026A618(self, 0x4B);
                        } else {
                            fn_8026A618(self, 0x4A);
                        }
                    } else {
                        fn_8026A678(self, 0xD);
                        fn_8026A678(self, 0xE);
                    }
                }
                if (fn_8026FA6C(self, 0, 0xA000, 0xE000) == 1U) {
                    fn_8026A678(self, 0xF);
                }
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 5);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 3);
            }
            if (((s32) (st->flagsBC & 0x20) != 0) || (((s32) (st->flagsBC & 0x200) != 0) && ((u8) self->field_0x308 == 1))) {
                fn_8026A678(self, 0x15);
                return;
            }
            break;
        }
        break;
    case 3:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x12);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->flagsB8 & 0x80) != 0) {
                    if ((s32) (st->unk7E & 2) != 0) {
                        fn_8026A678(self, 0xA);
                        fn_8026A678(self, 0x10);
                        return;
                    }
                } else {
                    fn_8026A678(self, 0xB);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0xF);
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 9);
                    fn_8026A678(self, 0xA);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if (fn_8026FC40(self, 1, 0x6AAB, 0xAAAB) == 0) {
                    fn_8026A678(self, 0xB);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 8);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (pl_part_flag_ck(self, 1) == 0)) {
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            }
            if (((s32) (st->flagsB8 & 0x200) == 0) || ((s32) (st->unk7C & 1) != 0)) {
                fn_8026A678(self, 0xB);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 2);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (fn_8026A6F4(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 8);
                return;
            }
            break;
        }
        break;
    case 2:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                fn_8026A618(self, 0x11);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 9) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0xEAAB, 0x9555) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    if ((s32) self->field_0x308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (fn_8026F9A4(self, 1, 0x9555, 0xEAAB) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 0, 1, 4) == 1U) {
                fn_8026A618(self, 0x11);
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 6);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 2);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 3);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 3);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 5);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 1);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 0);
                fn_8026A678(self, 2);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 3);
                return;
            }
            break;
        }
        break;
    case 7:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xD);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (fn_8026FB20(self, 0x800) == 1U) {
                        fn_8026A678(self, 0x12);
                    } else if (fn_8026FB20(self, 0x400) == 1U) {
                        fn_8026A678(self, 0x11);
                    }
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                    fn_8026A678(self, 0xC);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xC);
                    return;
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xC);
                    if ((s32) self->field_0x308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (fn_8026F9A4(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                } else if (fn_8026F9A4(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                } else if (fn_8026F9A4(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 7);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0xE);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 5);
                        fn_8026A678(self, 0xA);
                    } else {
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0xB);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 7);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 2);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 3);
                fn_8026A678(self, 8);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xB);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 4);
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                    return;
                }
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                    return;
                }
            }
            break;
        }
        break;
    case 8:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 1);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1B);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x17);
                    fn_8026A678(self, 0x18);
                    fn_8026A678(self, 0x14);
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0x13);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                        return;
                    }
                }
            } else {
                if (fn_8026F9A4(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0x13);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (fn_8026FB20(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                } else {
                    if ((fn_8026F9A4(self, 1, 0xE000, 0x2000) == 1U) || (fn_8026F9A4(self, 1, 0x6000, 0xA000) == 1U)) {
                        fn_8026A678(self, 2);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0x10);
                        fn_8026A678(self, 0x19);
                        fn_8026A678(self, 0x1C);
                    } else if (fn_8026F9A4(self, 1, 0xA000, 0x2000) == 1U) {
                        fn_8026A678(self, 0x1B);
                        fn_8026A678(self, 0x1C);
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 0x13);
                        fn_8026A678(self, 0x17);
                        fn_8026A678(self, 0x14);
                        fn_8026A678(self, 0x15);
                        fn_8026A678(self, 0x18);
                        if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                            fn_8026A678(self, 0xC);
                        }
                    }
                    if ((s32) (st->flagsBC & 0x100) != 0) {
                        fn_8026A678(self, 6);
                        fn_8026A678(self, 7);
                        if (fn_8026FB20(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                    if ((s32) (self->unkCC & 0x3C) != 0) {
                        fn_8026A678(self, 7);
                        if (fn_8026FB20(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                }
                if ((s32) (self->unkCC & 0x3C) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0x15);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 7);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 0x13);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 0x13);
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xF);
                }
                fn_8026A678(self, 0x1C);
                fn_8026A678(self, 7);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 0x16);
                fn_8026A678(self, 0x1D);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x18);
                        fn_8026A678(self, 0x1A);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xE);
                }
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 3);
                    return;
                }
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0x17);
                    return;
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 0x17);
                fn_8026A678(self, 0x18);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 0x1C);
                if (fn_8026FB20(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
            }
            break;
        }
        break;
    }
}

/* 0x8026F7B4: resets the action-state region, or starts a new action from the current area/state. */
extern "C" void fn_8026F7B4(_PLW* self)
{
    u32 value = 0;

    if (Pl_master_ck(self) == 0) {
        memset((u8*)self + 184, 0, 132);
    } else {
        if (PlayMode_ck() == 2) {
            value = self->chunk_ofs;
        }
        fn_8026AF08(self, value);
    }
}

/* 0x8026F828 */
extern "C" void fn_8026F828(_PLW* self)
{
    if (Pl_master_ck(self) == 0 || lobby_input_locked_ck() == 1) {
        memset((u8*)self + 184, 0, 132);
    } else {
        fn_8026AF08(self, 0);
    }
}

#pragma peephole off

/* 0x8026F888: true while the actor has an action-state flag set. */
extern "C" u32 fn_8026F888(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if ((self->unkBC & 0x3FF) != 0) {
        return 1;
    }
    if ((self->unkCC & 0x3C3C) != 0) {
        return 1;
    }
    return (self->unk134 & 3) != 0;
}

#pragma peephole reset

/* 0x8026F908: maps a part's motion value to an attack level (3/1/0), 0 for the "gun" weapon classes. */
extern "C" u8 pl_act_param_tier_ck(_PLW* self, u32 idx)
{
    u32 level = 0;
    s16 value = (s16)self->part_tbl_b_0xD4[idx];
    /* Dead copies, load-bearing for the allocator: retail's colouring needs the class web to be born
     * after a chain of copies of it (see the unit header). */
    u32 classCopy0 = self->field_0x002;
    u32 classCopy1 = classCopy0;
    u32 classCopy2 = classCopy1;
    u32 weaponClass = self->field_0x002;
    s32 high;
    s32 mid;
    s32 low;

    if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1 && self->unk5E6 == 1) {
        return 0;
    }
    if (self->unk128 == 2) {
        high = 110;
        mid = 90;
        low = 50;
    } else {
        high = 100;
        mid = 80;
        low = 40;
    }
    if (value >= high) {
        level = 3;
    } else if (value >= mid) {
        level = 1;
    } else if (value >= low) {
        level = 1;
    }
    return level;
}

#pragma peephole off

/* 0x8026F9A4: an attack-level gate plus the part's angle window. */
extern "C" u32 fn_8026F9A4(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 flags = self->unkCC;

    if (idx == 0) {
        if ((flags & 0x3C00) == 0) {
            return 0;
        }
    } else {
        if ((flags & 0x3C) == 0) {
            return 0;
        }
    }
    if (pl_act_param_tier_ck(self, idx) >= 1 && (u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026FA6C: the same window test, relative to the action's start frame. */
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 offset = 0;

    if (self->field_0x129 != 0) {
        offset = (u16)(self->field_0x0A8 - fn_802BE038());
    }
    if (self->part_tbl_b_0xD4[idx] >= 50) {
        u16 value = (u16)(self->part_tbl_a_0xD0[idx] - offset);
        if ((u16)(value - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FB20: picks the angle window for a direction code. */
extern "C" u32 fn_8026FB20(_PLW* self, s32 kind)
{
    switch (kind) {
    case 8192:
        return fn_8026FA6C(self, 0, 8192, 24576);
    case 1024:
        return fn_8026FA6C(self, 0, -8192, 8192);
    case 2048:
        return fn_8026FA6C(self, 0, 24576, -24576);
    case 4096:
        return fn_8026FA6C(self, 0, -24576, -8192);
    case 32:
        return fn_8026FA6C(self, 1, 8192, 24576);
    case 4:
        return fn_8026FA6C(self, 1, -8192, 8192);
    case 8:
        return fn_8026FA6C(self, 1, 24576, -24576);
    case 16:
        return fn_8026FA6C(self, 1, -24576, -8192);
    }
    return 0;
}

/* 0x8026FC40 */
extern "C" u32 fn_8026FC40(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (pl_act_param_tier_ck(self, idx) >= 1 && (u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

/* 0x8026FCCC: the angle window test without the attack-level gate. */
extern "C" u32 fn_8026FCCC(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (self->part_tbl_b_0xD4[idx] >= 50) {
        if ((u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FD0C: hands the actor's transform to its physics sub-object. */
extern "C" void fn_8026FD0C(_PLW* self)
{
    Vec3 vec;
    u32* p;

    VEC3_ctor(&vec);
    p = (u32*)&((PlJointHolder*)self->physics_0x13C)->chr_0x04;
    p[10] = self->param_0x54;
    p[11] = self->field_0x058;
    p[12] = self->rot_z_0x5C;
    copyVec3((nw4r::math::VEC3*)&p[1], (const nw4r::math::VEC3*)((u8*)self + 60));
    vec.x = self->field_0x068;
    vec.y = self->unk6C;
    vec.z = self->unk70;
    fn_800E09D0(p, &vec);
}

/* 0x8026FD94: true when the actor is in the area the player is currently in. */
extern "C" u32 fn_8026FD94(_PLW* self)
{
    return get_now_areano() == self->area_0x16;
}
} /* namespace s_8026BA1C */


/* 0x8026FDD4: master gate - true while the actor's Pl_master is the one in charge. */
u32 Pl_master_ck(_PLW* self)
{
    using s_8026BA1C::GameMode_ck; using s_8026BA1C::my_player_no;
    if (PlayMode_ck() == 2) {
        return 1;
    }
    if (GameMode_ck() == 3) {
        return 1;
    }
    return my_player_no() == self->chunk_ofs;
}
namespace s_8026BA1C {


/* 0x8026FE44: true for the "gun" weapon classes. */
extern "C" u32 fn_8026FE44(_PLW* self)
{
    return (u32)(self->field_0x002 - 4) <= 2;
}

#pragma peephole off
} /* namespace s_8026BA1C */


/* 0x8026FE68 */
u32 Pl_act_ck(_PLW* self, u8 action, u16 step)
{
    if (self->field_0x00A == action && self->act_no == step) {
        return 1;
    }
    return 0;
}
namespace s_8026BA1C {


/* 0x8026FE98: tests one bit of the two status words; the mask's top bit picks the word. */
extern "C" u32 fn_8026FE98(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        return self->field_0x35C & mask;
    }
    return self->unk360 & (mask & 0x7FFFFFFF);
}

/* 0x8026FEC0 */
extern "C" void pl_act_set_flag(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->field_0x35C |= mask;
    } else {
        self->unk360 |= mask & 0x7FFFFFFF;
    }
}

/* 0x8026FEF0 */
extern "C" void fn_8026FEF0(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->field_0x35C &= ~mask;
    } else {
        self->unk360 &= ~(mask & 0x7FFFFFFF);
    }
}

#pragma peephole reset

/* 0x8026FF20: the attack bonus the actor's active skills grant. */
extern "C" s32 fn_8026FF20(_PLW* self)
{
    if (Pl_Skill_ck(self, 13) == 1) {
        return 20;
    }
    if (Pl_Skill_ck(self, 14) == 1) {
        return 50;
    }
    if (Pl_Skill_ck(self, 15) == 1) {
        return -10;
    }
    return Pl_Skill_ck(self, 16) == 1 ? -30 : 0;
}

} /* namespace s_8026BA1C */

/* ==== 0x8026FFBC-0x80270018: the low-health gate ==== */

#include "types.h"
#include "pl.h"
namespace s_8026FFBC {


extern "C" {
extern f32 lbl_8079A02C;
}

/* Whether the actor's health is at or below the 0.4 threshold (the low-health gate the skill code
 * arms its attack modifiers with). */
extern "C" BOOL fn_8026FFBC(_PLW* self)
{
    return (f32)self->health / (f32)self->health_max <= lbl_8079A02C;
}

} /* namespace s_8026FFBC */

/* ==== 0x80270018-0x80273B14: the skill and item layer (optimization level 4) ==== */
#include "Pl/pl_act.h"
#pragma optimization_level 4

#include "types.h"
namespace s_80270018 {


extern "C" {
extern f32 lbl_8079A000;
extern f32 lbl_8079A004;
extern f32 lbl_8079A010;
extern f32 lbl_8079A030;
extern f32 lbl_8079A034;
extern f32 lbl_8079A038;
extern f32 lbl_8079A03C;
extern f32 lbl_8079A040;
extern f32 lbl_8079A044;
extern f32 lbl_8079A050;
extern f32 lbl_8079A054;
extern f32 lbl_8079A058;
extern f32 lbl_8079A05C;
extern f32 lbl_8079A060;
extern f32 lbl_8079A064;
extern f32 lbl_8079A068;
extern f32 lbl_8079A06C;
extern f32 lbl_8079A070;
extern f32 lbl_8079A074;
extern f32 lbl_8079A078;
extern f32 lbl_8079A07C;
}


extern "C" {
void equip_record_copy(_EQUIP*, _EQUIP*);
u32 fn_8026FFBC(_PLW*);
u32 fn_80273ED8(_PLW*, int, s8);

int fn_802753E4(_PLW*, u16);
void fn_80272A08(_PLW*);
u32 fn_8026FE44(_PLW*);
void fn_8027885C(_PLW*, int, int);
void fn_802789EC(_PLW*, int);
extern u32 lbl_805C5FC8[];
extern u32 lbl_805C5FE0[];
u8 GameMode_ck(void);
u32 fn_80363A2C(void);
int fn_80274AB8(int);
u32 fn_8027E29C(u8);
int Pl_item_timer_get(_PLW*, u16);
u32 fn_8029F6B4(u16);
u32 fn_8027E290(u8);
void fn_8027E98C(u8*);
s32 fn_8027EBA8(_PLW*, u8*);
u32 fn_80274B20(u16);
void fn_8027252C(_EQUIP*, u16*, s8*, u8*);
void* memset(void*, int, u32);
u8 Get_pl_type__FP6_EQUIPP6_EQUIP(_EQUIP*, _EQUIP*);
u16 fn_8027993C(_PLW*, u16, int);
void fn_80279B84(_PLW*);
u8* GetItemData__FUs(u16);
u16 fn_80273044(_PLW*, u16);
s32 fn_8004BD30(s16*);

void fn_8027350C(_PLW*, s32);


void* fn_8027E344(void);

u32 fn_80274AEC(u8*, u32, u8);

s16 item_take(u16, s16, void*, int, int, int);
u16 fn_8025DF78(_PLW*, u16, int);
u32 fn_80269394(void*);
u32 fn_802693C4(u8, int, s32);
u32 fn_80269474(u8, int, s32);
void fn_802695A4(u8, void*, void*);
void fn_80223258(_PLW*, u8);
void fn_80272B10(_PLW*, s32);
s8 fn_8027234C(u8*, u8);
void fn_802736A0(_PLW*);


extern u8 lbl_80792140;
extern u8 lbl_80792148;
}
} /* namespace s_80270018 */


/* One record of the skill-id table at `lbl_805C01C8`. */
/* size: 0x10 */
struct _SKILLREC {
    /* 0x0 */ u16 id;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 unkD;
    /* 0xE */ u8 unkE;
    /* 0xF */ u8 unkF;
};

/* One equipment/decoration record; only the skill-id/skill-level pairs are named. */
/* size: 0xC */
struct _SKILLITEM {
    /* 0x0 */ u8 unk0[8];
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
};

u32 Pl_cat_skill_ck(_PLW*, u16);
namespace s_80270018 {


/* Sums the player's skill deltas into a display value and writes the rank flag to `out`. */
extern "C" u16 fn_80270018(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (Pl_item_timer_get(plw, 596) > 0) {
            value += 6;
            *out = 1;
        }
        if (Pl_item_timer_get(plw, 597) > 0) {
            value += 9;
            *out = 1;
        }
        value += plw->unk448;
        value += plw->unk449;
        if (Pl_Skill_ck(plw, 74) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 75) == 1) {
            value += 15;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 76) == 1) {
            value += 20;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 77) == 1) {
            value += -5;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 78) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 79) == 1) {
            value += -15;
            *out = 2;
        }
        if (plw->field_0x370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A030 * value);
            *out = 1;
        } else if (fn_8026FFBC(plw) == 1) {
            if (Pl_Skill_ck(plw, 165) == 1) {
                value = (int)(lbl_8079A034 * value);
                *out = 2;
            } else if (Pl_Skill_ck(plw, 164) == 1) {
                value = (int)(lbl_8079A038 * value);
                *out = 1;
            }
        }
        if (Pl_Skill_ck(plw, 202) == 1) {
            if (plw->unk446 >= 2) {
                value = (int)(lbl_8079A03C * value);
            } else if (plw->unk446 >= 1) {
                value = (int)(lbl_8079A040 * value);
            }
        }
        if (value <= 0) {
            value = 1;
        }
        if (*out != 0) {
            if ((u16)value >= (u16)param) {
                *out = 1;
            } else {
                *out = 2;
            }
        }
    }
    return (u16)value;
}

/* Recomputes the player's skill point total and the fraction of its 700 point cap. */
extern "C" void fn_8027035C(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 6, 0);

        plw->skill_point_0x3B8 = fn_80270018(plw, (u16)v, &flag);
    }
    if (plw->skill_point_0x3B8 >= 700) {
        plw->skill_point_0x3B8 = 700;
    }
    plw->unk3BC = (f32)plw->skill_point_0x3B8 / lbl_8079A044;
}

/* Recomputes the second skill set's point total and its rank. */
extern "C" u16 fn_802703F4(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (Pl_item_timer_get(plw, 598) > 0) {
            value += 8;
            *out = 1;
        }
        if (Pl_item_timer_get(plw, 599) > 0) {
            value += 12;
            *out = 1;
        }
        value += plw->unk44C;
        value += plw->unk44D;
        if (Pl_Skill_ck(plw, 80) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 81) == 1) {
            value = (int)(lbl_8079A050 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 82) == 1) {
            value = (int)(lbl_8079A040 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 83) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 84) == 1) {
            value = (int)(lbl_8079A054 * value);
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 85) == 1) {
            value = (int)(lbl_8079A058 * value);
            value += -10;
            *out = 2;
        }
        if (fn_8026FFBC(plw) == 1) {
            *out = 1;
            if (Pl_Skill_ck(plw, 165) == 1) {
                value += 21;
            } else if (Pl_Skill_ck(plw, 163) == 1 || Pl_Skill_ck(plw, 164) == 1) {
                value += 45;
            } else {
                value += 30;
            }
        }
        if (plw->field_0x370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A05C * value);
            *out = 1;
        }
        if (value <= 0) {
            value = 1;
        }
        if (plw->unk422 > 0) {
            value -= value / 5;
            if (value < 1) {
                value = 1;
            }
            *out = 2;
        }
    }
    return (u16)value;
}

/* Recomputes the second skill set's point meter, clamped to a minimum of ten points. */
extern "C" void fn_80270728(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 0, 0) + 1;

        plw->unk3BA = fn_802703F4(plw, (u16)v, &flag);
    }
    plw->unk3C0 = (f32)plw->unk3BA;
    if (plw->unk3C0 < lbl_8079A010) {
        plw->unk3C0 = lbl_8079A010;
    }
}

/*
 * Applies one weapon-type skill group to one attack-bonus field: the +20/+15/+10 levels of the group's
 * three positive skills, then the -15/-10 levels of its two negative ones.
 */
#define SKILL_GROUP(field, base)                                 if (Pl_Skill_ck(plw, base) == 1) {                               plw->field += lbl_8079A060;                                     } else if (Pl_Skill_ck(plw, base - 1) == 1) {                    plw->field += lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base - 2) == 1) {                    plw->field += lbl_8079A010;                                     }                                                            if (Pl_Skill_ck(plw, base + 2) == 1) {                           plw->field -= lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base + 1) == 1) {                    plw->field -= lbl_8079A010;                                     }

/* Recomputes the five weapon-type attack-bonus fields from the player's active skills. */
extern "C" void fn_802707B4(_PLW* plw) {
    SKILL_GROUP(unk3C4, 96)
    SKILL_GROUP(unk3C8, 102)
    SKILL_GROUP(unk3CC, 108)
    SKILL_GROUP(unk3D0, 120)
    SKILL_GROUP(unk3D4, 114)
}

/* Clamps the five attack-bonus fields to the +-99 range the display and the status code use. */
extern "C" void fn_80270B98(_PLW* plw) {
    if (plw->unk3C4 < lbl_8079A068) {
        plw->unk3C4 = lbl_8079A068;
    }
    if (plw->unk3C8 < lbl_8079A068) {
        plw->unk3C8 = lbl_8079A068;
    }
    if (plw->unk3CC < lbl_8079A068) {
        plw->unk3CC = lbl_8079A068;
    }
    if (plw->unk3D0 < lbl_8079A068) {
        plw->unk3D0 = lbl_8079A068;
    }
    if (plw->unk3D4 < lbl_8079A068) {
        plw->unk3D4 = lbl_8079A068;
    }
    if (plw->unk3C4 > lbl_8079A06C) {
        plw->unk3C4 = lbl_8079A06C;
    }
    if (plw->unk3C8 > lbl_8079A06C) {
        plw->unk3C8 = lbl_8079A06C;
    }
    if (plw->unk3CC > lbl_8079A06C) {
        plw->unk3CC = lbl_8079A06C;
    }
    if (plw->unk3D0 > lbl_8079A06C) {
        plw->unk3D0 = lbl_8079A06C;
    }
    if (plw->unk3D4 > lbl_8079A06C) {
        plw->unk3D4 = lbl_8079A06C;
    }
}

/* The defence delta of one armour-piece slot: -1 for a negative skill, +1 for a positive one. */
extern "C" f32 fn_80270C64(s16* table, s16 slot) {
    f32 delta = lbl_8079A000;

    if (((s16*)((u8*)table + 0x42E))[slot] > 0) {
        delta -= lbl_8079A070;
    }
    if (((s16*)((u8*)table + 0x438))[slot] > 0) {
        delta += lbl_8079A074;
    }
    return delta;
}

/* Rebuilds the five attack-bonus fields from the player's armour skills and the weapon's own bonuses. */
extern "C" void fn_80270CA4(_PLW* plw) {
    plw->unk3C4 = (f32)(s16)fn_80273ED8(plw, 1, 0) + fn_80270C64((s16*)plw, 0);
    plw->unk3C8 = (f32)(s16)fn_80273ED8(plw, 2, 0) + fn_80270C64((s16*)plw, 1);
    plw->unk3CC = (f32)(s16)fn_80273ED8(plw, 3, 0) + fn_80270C64((s16*)plw, 2);
    plw->unk3D0 = (f32)(s16)fn_80273ED8(plw, 4, 0) + fn_80270C64((s16*)plw, 4);
    plw->unk3D4 = (f32)(s16)fn_80273ED8(plw, 5, 0) + fn_80270C64((s16*)plw, 3);
    fn_802707B4(plw);
    plw->unk3C4 += (f32)(s16)fn_802753E4(plw, 6);
    plw->unk3C8 += (f32)(s16)fn_802753E4(plw, 7);
    plw->unk3CC += (f32)(s16)fn_802753E4(plw, 8);
    plw->unk3D0 += (f32)(s16)fn_802753E4(plw, 10);
    plw->unk3D4 += (f32)(s16)fn_802753E4(plw, 9);
    if (Pl_condition_ck(plw, 256) == 1) {
        plw->unk3D4 -= lbl_8079A070;
    }
    fn_80270B98(plw);
}

/* Swaps the player's nine equipment slots for another set, re-derives every skill field and fills the status
 * screen's skill summary; a null equipment set only fills the summary. */
extern "C" void fn_80270F50(_PLW* plw, _EQUIP* equip, u8* out) {
    _EQUIP saved[9];
    int i;

    if (equip != 0) {
        equip_record_copy(&saved[0], &plw->equipA[0]);
        equip_record_copy(&saved[1], &plw->equipA[1]);
        equip_record_copy(&saved[2], &plw->equipA[2]);
        equip_record_copy(&saved[3], &plw->equipA[3]);
        equip_record_copy(&saved[4], &plw->equipA[4]);
        equip_record_copy(&saved[5], &plw->equipA[5]);
        equip_record_copy(&saved[6], &plw->equipB);
        equip_record_copy(&saved[7], &plw->equipC);
        equip_record_copy(&saved[8], &plw->equipD);
        equip_record_copy(&plw->equipA[0], &equip[0]);
        equip_record_copy(&plw->equipA[1], &equip[1]);
        equip_record_copy(&plw->equipA[2], &equip[2]);
        equip_record_copy(&plw->equipA[3], &equip[3]);
        equip_record_copy(&plw->equipA[4], &equip[4]);
        equip_record_copy(&plw->equipA[5], &equip[5]);
        equip_record_copy(&plw->equipB, &equip[6]);
        equip_record_copy(&plw->equipC, &equip[7]);
        equip_record_copy(&plw->equipD, &equip[8]);
        fn_80272A08(plw);
        for (int i = 0; i < 8; i++) {
            plw->unk61A[i] = plw->unk5F2[i];
        }
        fn_8027885C(plw, 0, 0);
        fn_802789EC(plw, 0);
        fn_8027035C(plw);
        fn_80270728(plw);
        fn_80270CA4(plw);
    }
    out[10] = 0;
    *(u16*)(out + 12) = 0;
    for (i = 1; i < 6; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FC8[i], 0);

        if (v > 0) {
            out[10] = (u8)i;
            *(s16*)(out + 12) = v;
            break;
        }
    }
    out[11] = 0;
    *(u16*)(out + 14) = 0;
    for (i = 0; i < 3; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FE0[i], 0);

        if (v > 0) {
            out[11] = (u8)(i + 12);
            *(s16*)(out + 14) = v;
            break;
        }
    }
    *(s16*)(out + 0) = plw->unk372;
    *(s16*)(out + 2) = plw->unk37A;
    *(s16*)(out + 4) = plw->unk380;
    *(u16*)(out + 6) = plw->skill_point_0x3B8;
    *(u16*)(out + 8) = plw->unk3BA;
    *(s16*)(out + 16) = (s16)plw->unk3C4;
    *(s16*)(out + 18) = (s16)plw->unk3C8;
    *(s16*)(out + 20) = (s16)plw->unk3CC;
    *(s16*)(out + 24) = (s16)plw->unk3D0;
    *(s16*)(out + 22) = (s16)plw->unk3D4;
    for (i = 0; i < 7; i++) {
        out[26 + i] = 0;
    }
    if (equip != 0) {
        equip_record_copy(&plw->equipA[0], &saved[0]);
        equip_record_copy(&plw->equipA[1], &saved[1]);
        equip_record_copy(&plw->equipA[2], &saved[2]);
        equip_record_copy(&plw->equipA[3], &saved[3]);
        equip_record_copy(&plw->equipA[4], &saved[4]);
        equip_record_copy(&plw->equipA[5], &saved[5]);
        equip_record_copy(&plw->equipB, &saved[6]);
        equip_record_copy(&plw->equipC, &saved[7]);
        equip_record_copy(&plw->equipD, &saved[8]);
        fn_80272A08(plw);
        for (int i = 0; i < 8; i++) {
            plw->unk61A[i] = 0;
        }
        fn_8027885C(plw, 0, 0);
        fn_802789EC(plw, 0);
        fn_8027035C(plw);
        fn_80270728(plw);
        fn_80270CA4(plw);
    }
}
} /* namespace s_80270018 */


/* Whether the player's active skill set contains `skill`. */
u32 Pl_Skill_ck(_PLW* plw, u16 skill) {
    using s_80270018::GameMode_ck; using s_80270018::fn_80363A2C; 
    bool ok = false;
    int i;

    if (GameMode_ck() == 2) {
        if (lobby_w.state_0x000 == 6) {
            ok = true;
        } else if (lobby_w.state_0x000 == 15 && fn_80363A2C() == 1) {
            ok = true;
        }
    }
    if (ok == 1) {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->unk61A[i]) {
                return true;
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->unk5F2[i]) {
                return true;
            }
        }
    }
    return false;
}
namespace s_80270018 {


/* Whether `skill` is one of the player's eight base skill ids. */
extern "C" u32 fn_802714F0(_PLW* plw, u16 skill) {
    int i;

    for (i = 0; i < 8; i++) {
        if (skill == plw->unk5F2[i]) {
            return 1;
        }
    }
    return 0;
}

/* The level of the skill in one slot: the live set in the lobby menu, the base set otherwise. */
extern "C" u32 Pl_Skill_slot_item_get(_PLW* plw, u32 slot) {
    int i;

    if (GameMode_ck() == 2 && lobby_w.state_0x000 == 6) {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk62A[i]) {
                return (u8)plw->unk61A[i];
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->unk602[i]) {
                return (u8)plw->unk5F2[i];
            }
        }
    }
    return 0;
}

/* Whether the player currently provides `kind` of skill: kind 15-17 ask the equipment, 74 the player
 * type, and anything else the two skill-id tables. */
extern "C" u32 fn_80271674(_PLW* plw, u8 kind) {
    u16 mode = (fn_8026FE44(plw) == 1);

    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (u32)(fn_80274AB8((int)plw) == 1);
    case 74: {
        int type = plw->field_0x002;

        if ((u32)(type - 7) <= 1 || type == 0 || type == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Whether the player's active skill set satisfies one skill query: the equipment-provided kinds
 * 7-10/14-15, the level query kinds 11-13, and the 74 player-type special case. */
extern "C" u32 fn_8027176C(_PLW* plw, u8* p, u32 a, u8 kind) {
    u32 lvl;
    u32 mode;

    if (p == 0) {
        return 0;
    }
    {
        u8 v = *p;

        switch (v) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            lvl = fn_8027E29C(v);
            mode = 0;
            break;
        case 11:
        case 12:
        case 13:
            lvl = 6;
            mode = 1;
            break;
        default:
            return fn_80271674(plw, kind);
        }
    }
    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (fn_80274AEC(p, a, lvl) - 1) == 0;
    case 74: {
        u8 l = (u8)lvl;

        if ((u32)(l - 7) <= 1 || (s32)l == 0 || (s32)l == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Maps a display skill id to its internal id through the 16-byte record table, or 0 when absent. */
extern "C" u8 fn_802718C8(u16 skill) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;

    if (skill == 0) {
        return 0;
    }
    while (rec->id != 0xFFFF) {
        if (rec->unk3 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk5 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk7 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk9 == skill) {
            return (u8)rec->id;
        }
        if (rec->unkB == skill) {
            return (u8)rec->id;
        }
        if (rec->unkD == skill) {
            return (u8)rec->id;
        }
        rec++;
    }
    return 0;
}

/* The one-argument form of fn_80271674: the skill is resolved through the id table first. */
extern "C" void fn_80271978(_PLW* plw, u16 skill) {
    fn_80271674(plw, fn_802718C8(skill));
}

/* The three-argument form of fn_8027176C: the skill is resolved through the id table first. */
extern "C" void fn_802719B8(_PLW* plw, u8* p, u32 a, u16 skill) {
    fn_8027176C(plw, p, a, fn_802718C8(skill));
}

/* Whether a skill is available in the current quest context: three kinds are gated on the
 * quest mode, one on the player type, everything else is always available. */
extern "C" u32 fn_80271A18(_PLW* plw, u16 skill) {
    u16 mode = (fn_8026FE44(plw) == 1);

    switch (skill) {
    case 15:
    case 30:
        if (mode == 0) {
            return 0;
        }
        break;
    case 19:
    case 38:
    case 50:
        if (mode == 1) {
            return 0;
        }
        break;
    case 40:
        if ((u32)(plw->field_0x002 - 7) <= 1) {
            return 0;
        }
        break;
    default:
        break;
    }
    return 1;
}

/* The skill level an equipment record contributes for `skill`, or 0. */
extern "C" s8 fn_80271AD8(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)fn_8029F6B4(item);
    s8 lvl = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            lvl = (s8)rec->unk9;
        }
        if (rec->unkA == skill) {
            lvl += rec->unkB;
        }
    }
    return lvl;
}

/* Whether an equipment record provides `skill` at a non-zero level. */
extern "C" u32 fn_80271B4C(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)fn_8029F6B4(item);
    u32 ok = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            if ((s8)rec->unk9 != 0) {
                ok = 1;
            }
        }
        if (rec->unkA == skill) {
            if ((s8)rec->unkB != 0) {
                ok = 1;
            }
        }
    }
    return ok;
}

/* The 4-byte slot-table entry at 0x278: an item id and a signed value. */
#define SLOT_IDX(slot) (((slot) & 0x80) ? (((slot) & 0x7F) + 26) : (slot))

/* Appends one (value, a, b) triple to a bounded parallel-array set and bumps the count. */
extern "C" void fn_802724E8(u16* table, s8* a, u8* b, u8* count, u16 v, u8 bval, s8 aval) {
    if (v == 0) {
        return;
    }
    if (*count >= 8) {
        return;
    }
    table[*count] = v;
    a[*count] = aval;
    b[*count] = bval;
    *count += 1;
}

/* The saved-skill set behind fn_80272A08: looks each skill id up in the 16-byte record table, keeps the
 * rank the player's equipment reaches, converts it to a display value and sorts the eight entries. */
extern "C" void fn_8027252C(_EQUIP* equips, u16* table, s8* a, u8* b) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;
    s16 tv[8];
    u8 tb[8];
    u8 ta[8];
    u8 count = 0;
    s8 level;
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        table[i] = 0;
        b[i] = 0;
        a[i] = 0;
    }
    while (rec->id != 0xFFFF) {
        level = fn_8027234C((u8*)equips, (u8)rec->id);
        if (level <= (s8)rec->unk2) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk3, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk4) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk5, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk6) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk7, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkC) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkD, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkA) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkB, (u8)rec->id, level);
        } else if (level >= (s8)rec->unk8) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk9, (u8)rec->id, level);
        }
        rec++;
    }
    for (i = 0; i < 8; i++) {
        s8 v = (s8)a[i];

        if (v != 0) {
            if (v > 0) {
                tv[i] = (s16)(v + 200);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            } else {
                tv[i] = (s16)((s16)(-v) + 100);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            }
        } else {
            tv[i] = 0;
            tb[i] = 0;
            ta[i] = 0;
        }
    }
    for (i = 0; i < 7; i++) {
        for (j = i + 1; j < 8; j++) {
            if (tv[i] < tv[j] || (tv[i] == tv[j] && tb[i] < tb[j])) {
                s16 s = tv[i];
                u8 t = tb[i];

                tv[i] = tv[j];
                tb[i] = tb[j];
                tv[j] = s;
                tb[j] = t;
                {
                    u8 u = ta[i];

                    ta[i] = ta[j];
                    ta[j] = u;
                }
            }
        }
    }
    for (i = 0; i < 8; i++) {
        a[i] = (s8)tv[i];
        b[i] = tb[i];
        table[i] = ta[i];
    }
}
} /* namespace s_80270018 */


/* Whether `skill` is one of the four decoration-slot skill ids. */
u32 Pl_cat_skill_ck(_PLW* plw, u16 skill) {
    int i;
    u16* p = plw->deco_skill_id;

    for (i = 0; i < 2; i++) {
        if (skill == p[0]) {
            return 1;
        }
        if (skill == p[1]) {
            return 1;
        }
        p += 2;
    }
    return 0;
}
namespace s_80270018 {


/* The item id in one equipment slot, or 0 for the empty slot. */
extern "C" u16 fn_80272C80(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (slot & 0x80) {
        return *(u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    }
    return *(u16*)((u8*)plw + slot * 4 + 0x278);
}

/* The signed skill value of one equipment slot, or 20 for the fixed-value item. */
extern "C" s16 fn_80272CC8(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (fn_80272C80(plw, slot) == 0x35) {
        return 20;
    }
    if (slot & 0x80) {
        return *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
    }
    return *(s16*)((u8*)plw + slot * 4 + 0x27A);
}

/* Recomputes the current slot's skill value and lowers the running minimum. */
extern "C" void fn_80272D5C(_PLW* plw) {
    s16 v = fn_80272CC8(plw, (u8)plw->unk26E);

    plw->unk270 = v;
    plw->unk272 = (s16)v;
    if ((s16)v < plw->unk269) {
        plw->unk269 = (u8)(s16)v;
        plw->unk274 = (u8)(s16)v;
    }
}

/* The weapon-type slot count for one equipment slot, 2 for the empty slot. */
extern "C" s32 fn_80272DB4(_PLW* plw, u16 slot) {
    u16 id = fn_80273044(plw, slot);

    if (id != 0xFFFF) {
        if (id & 0x80) {
            return fn_8004BD30((s16*)&plw->slot_id[(id & 0x7F) + 26]);
        }
        return fn_8004BD30((s16*)&plw->slot_id[id]);
    }
    return 2;
}

/* Clears the four per-set skill summary words. */
extern "C" void fn_8027346C(_PLW* plw) {
    plw->unk634 = 0;
    plw->unk638 = 0;
    plw->unk63C = 0;
    plw->unk640 = 0;
}

/* Removes the equipment set selected by `idx` and clears its valid bit. */
extern "C" void fn_8027373C(_PLW* plw, u8 idx) {
    u32 mask = 1 << idx;

    if ((plw->equip_valid & mask) == 0) {
        return;
    }
    equip_record_copy((_EQUIP*)((u8*)plw + idx * 12 + 0x140), (_EQUIP*)((u8*)plw + idx * 12 + 0x188));
    plw->equip_valid &= (u16)~mask;
}

/* One byte of the seven-entry decoration skill-id table. */
extern "C" u8 fn_802738B8(u8 idx) {
    if (idx >= 7) {
        return 0;
    }
    return (&lbl_80792140)[idx];
}

/* One byte of the second seven-entry decoration table. */
extern "C" u8 fn_802738D8(u8 idx) {
    return (&lbl_80792148)[idx];
}

/* Stores one skill value into a set slot and updates its valid bit. */
extern "C" void fn_80273998(_PLW* plw, u8 idx, s32 val) {
    if (val == plw->set_applied[idx]) {
        plw->set_valid &= (u16)~(1 << idx);
        return;
    }
    plw->set_pending[idx] = val;
    plw->set_valid |= (u16)(1 << idx);
}

/* Copies the player's nine equipment slots into a local save area and rebuilds the skill set from it. */
extern "C" void fn_80272A08(_PLW* plw) {
    _EQUIP saved[9];

    equip_record_copy(&saved[0], &plw->equipA[0]);
    equip_record_copy(&saved[1], &plw->equipA[1]);
    equip_record_copy(&saved[2], &plw->equipA[2]);
    equip_record_copy(&saved[3], &plw->equipA[3]);
    equip_record_copy(&saved[4], &plw->equipA[4]);
    equip_record_copy(&saved[5], &plw->equipA[5]);
    equip_record_copy(&saved[6], &plw->equipB);
    equip_record_copy(&saved[7], &plw->equipC);
    equip_record_copy(&saved[8], &plw->equipD);
    fn_8027252C(&saved[0], plw->unk5F2, (s8*)plw->unk60A, plw->unk602);
}

/* Re-selects the skill slot: keeps the current one when it still holds the same skill id, otherwise
 * falls back through the resolved-slot table and drops it when the item is not a valid one. */
extern "C" void fn_80272B10(_PLW* plw, s32 slot) {
    u16* ent;
    u32 v;

    if (slot & 0x80) {
        ent = (u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    } else {
        ent = (u16*)((u8*)plw + slot * 4 + 0x278);
    }
    v = fn_80274B20(*ent);
    if (slot != plw->unk26E) {
        if (plw->held_item_kind_0x26C != (u8)v) {
            return;
        }
        plw->unk26E = (u16)slot;
    }
    if (slot == plw->unk26E) {
        plw->unk26E = fn_8027993C(plw, (u16)slot, 2);
        fn_80279B84(plw);
        plw->unk272 = plw->unk270;
        plw->unk275 = plw->unk26A;
        plw->unk274 = 0;
        return;
    }
    if (fn_8026FE44(plw) != 0) {
        u16 cur = plw->unk26E;

        if (cur == 0xFF) {
            plw->unk26E = fn_8027993C(plw, 0, 2);
            fn_80279B84(plw);
            return;
        }
        u8* item;

        if (cur & 0x80) {
            item = GetItemData__FUs(plw->slot_id[(cur & 0x7F) + 26].item_id);
        } else {
            item = GetItemData__FUs(plw->slot_id[cur].item_id);
        }
        if (item[0] != 1) {
            plw->unk26E = fn_8027993C(plw, plw->unk26E, 0);
            fn_80279B84(plw);
        }
    }
}

/* Resolves the equipment slot for a changed slot and re-selects the skill to display. */
extern "C" s16 pl_item_add(_PLW* plw, u16 item, s16 value) {
    u8* data = GetItemData__FUs(item);
    u16 cur = fn_80273044(plw, item);
    s16 v;
    u16 slot;

    for (;;) {
        if (data[0] == 1 && fn_8026FE44(plw) == 1 && (cur == 0xFFFF || (cur & 0x80) != 0)) {
            v = item_take(item, value, plw->spare_slot_id, 8, 1, 0);
            slot = fn_80273044(plw, item);
            if ((u32)v <= 4) {
                break;
            }
        }
        v = item_take(item, value, plw->slot_id, 24, 1, 0);
        slot = fn_80273044(plw, item);
        break;
    }
    switch (v) {
    case 0:
        {
            u8* d = GetItemData__FUs(plw->slot_id[plw->field_0x304].item_id);

            if ((d[2] & 8) == 0 || d[0] == 1) {
                plw->field_0x304 = fn_8025DF78(plw, plw->field_0x304, 0);
            }
        }
        fn_80272B10(plw, slot);
        break;
    case 1:
    case 2:
    case 3:
        if (data[0] == 1 && plw->unk26E == slot) {
            fn_80272D5C(plw);
        }
        break;
    case 4:
        if (data[0] == 1) {
            if (fn_8027993C(plw, plw->unk26E, 1) == 0xFF) {
                plw->unk26E = 0xFF;
                plw->unk270 = 0;
                plw->unk26A = 0;
                plw->unk269 = 0;
            } else if (plw->unk26E == cur) {
                plw->unk270 = 0;
                plw->unk26A = 0;
                plw->unk269 = 0;
            }
            if (plw->unk26E == cur) {
                fn_80272D5C(plw);
            }
        }
        break;
    }
    return v;
}

/* The value one equipment slot contributes to `value`, clamped to the item's own limit. */
extern "C" s32 pl_item_room_get(_PLW* plw, u16 item, s16 value) {
    u16 slot = fn_80273044(plw, item);
    u8* data = GetItemData__FUs(item);
    s16 ent;

    if (slot != 0xFFFF) {
        if (slot & 0x80) {
            ent = *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
            if ((s16)value + ent >= data[3]) {
                return data[3] - ent;
            }
            return value;
        }
        ent = *(s16*)((u8*)plw + slot * 4 + 0x27A);
        if ((s16)value + ent >= data[3]) {
            return data[3] - ent;
        }
        return value;
    }
    if (data[0] == 1 && fn_8026FE44(plw) == 1) {
        u16* p = (u16*)plw->spare_slot_id;

        for (int i = 0; i < 8; i++) {
            if (p[i * 2] == 0) {
                return value;
            }
        }
    }
    for (int i = 0; i < 24; i++) {
        if (plw->slot_id[i].item_id == 0) {
            return value;
        }
    }
    return 0;
}

/* Flushes the skill sets whose cached values no longer resolve, and the decoration slots. */
extern "C" void fn_8027350C(_PLW* plw, s32 arg) {
    u8 i;
    u8 id;
    u32 mask;
    u32 val;
    u32 ok;

    if (!(plw->set_valid != 0 || plw->equip_valid != 0 || plw->deco_dirty != 0)) {
        return;
    }
    ok = fn_80269394(lbl_80794B28->table + plw->chunk_ofs);
    for (i = 0; i < 7; i++) {
        id = fn_802738B8(i);
        mask = 1 << i;
        if (plw->set_valid & mask) {
            if (ok == 1) {
                if (*(u32*)((u8*)plw->physics_0x13C + i * 0x164 + 0x11C) == 0) {
                    val = fn_802693C4(plw->chunk_ofs, i, plw->set_pending[i]);
                } else {
                    val = fn_80269474(plw->chunk_ofs, i, plw->set_pending[i]);
                }
                if (val == 1) {
                    plw->set_valid &= (u16)~mask;
                    plw->set_applied[i] = plw->set_pending[i];
                    if (id != 0xFF) {
                        fn_8027373C(plw, id);
                    }
                }
            }
        } else {
            plw->set_valid &= (u16)~mask;
            if (id != 0xFF) {
                fn_8027373C(plw, id);
                fn_80223258(plw, id);
            }
        }
    }
    if (plw->deco_dirty != 0 && ok == 1) {
        fn_802695A4(plw->chunk_ofs, (u8*)plw + 0x1DC, (u8*)plw + 0x200);
        fn_802736A0(plw);
    }
}

/* Clears every derived skill array and the three valid-bit words. */
extern "C" void fn_80273484(_PLW* plw) {
    memset((u8*)plw + 0x188, 0, 72);
    memset((u8*)plw + 0x1DC, 0, 12);
    memset((u8*)plw + 0x200, 0, 24);
    memset((u8*)plw + 0x218, -1, 28);
    memset((u8*)plw + 0x234, -1, 28);
    plw->equip_valid = 0;
    plw->set_valid = 0;
    plw->deco_dirty = 0;
}

/* Re-derives the player type after restoring the two decoration equipment slots. */
extern "C" void fn_802736A0(_PLW* plw) {
    int i;

    if (plw->deco_dirty == 0) {
        return;
    }
    equip_record_copy(&plw->equipB, &plw->equipB2);
    for (i = 0; i < 2; i++) {
        equip_record_copy((&plw->equipC) + i, &plw->equipE[i]);
    }
    plw->deco_dirty = 0;
    plw->field_0x002 = Get_pl_type__FP6_EQUIPP6_EQUIP(&plw->equipB, &plw->equipC);
}

/* The skill level one player equipment record contributes for `skill`: a per-kind record layout,
 * with the per-decoration levels summed for the kinds that carry them. */
extern "C" s8 fn_80271BD4(u8* raw, u8 skill) {
    _EQUIP* rec = (_EQUIP*)raw;
    s8 v = 0;
    int i;

    if (rec->item_id == 0 || skill == 0) {
        return 0;
    }
    {
        s32 kind = rec->kind;

        switch ((u32)kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            {
                u8* r = (u8*)fn_8027E344();

                if (r[14] == skill) {
                    v = (s8)r[15];
                }
                if (r[16] == skill) {
                    v += r[17];
                }
                if (r[18] == skill) {
                    v += r[19];
                }
                if (r[20] == skill) {
                    v += r[21];
                }
                if (r[22] == skill) {
                    v += r[23];
                }
                for (i = 0; i < 3; i++) {
                    if (rec->skill_id[i] != 0) {
                        v += fn_80271AD8(rec->skill_id[i], skill);
                    }
                }
            }
            break;
        case 6:
            if (rec->deco_count != 0) {
                for (i = 0; i < rec->deco_count; i++) {
                    if (rec->skill_id[i] != 0) {
                        v += fn_80271AD8(rec->skill_id[i], skill);
                    }
                }
            }
            if ((s32)rec->deco_count < 3) {
                if (rec->skill_id[rec->deco_count] == skill) {
                    v += (s8)((s8)(u8)rec->deco_level - 10);
                }
                if ((s32)rec->deco_count + 1 < 3 &&
                    rec->skill_id[rec->deco_count + 1] == skill) {
                    v += (s8)((s8)(u8)((rec->deco_level >> 8) & 0xFF) - 10);
                }
            }
            break;
        default:
            for (i = 0; i < 3; i++) {
                if (rec->skill_id[i] != 0) {
                    v += fn_80271AD8(rec->skill_id[i], skill);
                }
            }
            break;
        }
    }
    return v;
}

/* Scales the skill level of one record by how many of the nine player records carry it. */
extern "C" s8 fn_80272084(u8* rec, u8 skill) {
    s8 v = fn_80271BD4(rec, skill);

    if (v != 0) {
        s8 n = 1;

        if (fn_80271BD4(rec + 0x30, 1)) {
            n = 2;
        }
        if (fn_80271BD4(rec + 0x00, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x0C, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x18, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x24, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x3C, 1)) {
            n++;
        }
        if (fn_80271BD4(rec + 0x48, 1)) {
            n++;
        }
        if (rec[0x54] == 12 && fn_80271BD4(rec + 0x54, 1)) {
            n++;
        }
        if (rec[0x60] == 13 && fn_80271BD4(rec + 0x60, 1)) {
            n++;
        }
        v = v * n;
    }
    return v;
}

/* Whether one player equipment record provides `skill` at a non-zero level. */
extern "C" u32 fn_80271E0C(u8* raw, u8 skill) {
    _EQUIP* rec = (_EQUIP*)raw;
    u32 ok = 0;
    int i;

    if (rec->item_id == 0 || skill == 0) {
        return 0;
    }
    {
        s32 kind = rec->kind;

        switch ((u32)kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            {
                u8* r = (u8*)fn_8027E344();

                if (r[14] == skill && (s8)r[15] != 0) {
                    ok = 1;
                }
                if (r[16] == skill && (s8)r[17] != 0) {
                    ok = 1;
                }
                if (r[18] == skill && (s8)r[19] != 0) {
                    ok = 1;
                }
                if (r[20] == skill && (s8)r[21] != 0) {
                    ok = 1;
                }
                if (r[22] == skill && (s8)r[23] != 0) {
                    ok = 1;
                }
                for (i = 0; i < 3; i++) {
                    if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                        ok = 1;
                    }
                }
            }
            break;
        case 6:
            if (rec->deco_count != 0) {
                for (i = 0; i < rec->deco_count; i++) {
                    if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                        ok = 1;
                    }
                }
            }
            if ((s32)rec->deco_count < 3) {
                if (rec->skill_id[rec->deco_count] == skill &&
                    (s8)(u8)rec->deco_level - 10 != 0) {
                    ok = 1;
                }
                if ((s32)rec->deco_count + 1 < 3 &&
                    rec->skill_id[rec->deco_count + 1] == skill &&
                    (s8)(u8)((rec->deco_level >> 8) & 0xFF) - 10 != 0) {
                    ok = 1;
                }
            }
            break;
        default:
            for (i = 0; i < 3; i++) {
                if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                    ok = 1;
                }
            }
            break;
        }
    }
    return ok;
}

/* Whether any of the nine player records provides `skill`. */
extern "C" u32 fn_80272218(u8* rec, u8 skill) {
    u32 ok = 0;

    if (fn_80271E0C(rec + 0x30, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x0C, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x18, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x24, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x3C, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec + 0x48, skill) == 1) {
        ok = 1;
    }
    if (rec[0x54] == 12 && fn_80271E0C(rec + 0x54, skill) == 1) {
        ok = 1;
    }
    if (rec[0x60] == 13 && fn_80271E0C(rec + 0x60, skill) == 1) {
        ok = 1;
    }
    if (fn_80271E0C(rec, skill) == 1) {
        ok = 1;
    }
    return ok;
}

/* Sums the skill level of all nine player records, the main record scaled by its carrier count. */
extern "C" s8 fn_8027234C(u8* rec, u8 skill) {
    s8 v = fn_80271BD4(rec + 0x30, skill);

    v += fn_80271BD4(rec + 0x0C, skill);
    v += fn_80271BD4(rec + 0x18, skill);
    v += fn_80271BD4(rec + 0x24, skill);
    v += fn_80271BD4(rec + 0x3C, skill);
    v += fn_80271BD4(rec + 0x48, skill);
    if (rec[0x54] == 12) {
        v += fn_80271BD4(rec + 0x54, skill);
    }
    if (rec[0x60] == 13) {
        v += fn_80271BD4(rec + 0x60, skill);
    }
    v += fn_80272084(rec, skill);
    return v;
}

/* Sums the skill level of the nine saved equipment slots. */
extern "C" void fn_8027243C(_PLW* plw, u8 skill) {
    _EQUIP saved[9];

    equip_record_copy(&saved[0], &plw->equipA[0]);
    equip_record_copy(&saved[1], &plw->equipA[1]);
    equip_record_copy(&saved[2], &plw->equipA[2]);
    equip_record_copy(&saved[3], &plw->equipA[3]);
    equip_record_copy(&saved[4], &plw->equipA[4]);
    equip_record_copy(&saved[5], &plw->equipA[5]);
    equip_record_copy(&saved[6], &plw->equipB);
    equip_record_copy(&saved[7], &plw->equipC);
    equip_record_copy(&saved[8], &plw->equipD);
    fn_8027234C((u8*)&saved[0], skill);
}

/* The signed skill value of the equipment slot `slot`, or 0 for the empty slot. */
extern "C" int Pl_item_timer_get(_PLW* plw, u16 slot) {
    u16 id = fn_80273044(plw, slot);

    if (id != 0xFFFF) {
        if (id & 0x80) {
            return *(s16*)((u8*)plw + ((id & 0x7F) + 26) * 4 + 0x27A);
        }
        return *(s16*)((u8*)plw + id * 4 + 0x27A);
    }
    return 0;
}

/* Stores one equipment record into the saved set selected by its own slot id and marks it valid. */
extern "C" void fn_802738E8(_PLW* plw, u8* rec) {
    u8 idx = (u8)fn_8027E290(rec[0]);

    fn_8027E98C(rec);
    equip_record_copy((_EQUIP*)((u8*)plw + idx * 12 + 0x188), (_EQUIP*)rec);
    plw->equip_valid |= (u16)(1 << idx);
    idx = fn_802738D8(idx);
    if (idx != 0xFF) {
        fn_80273998(plw, idx, fn_8027EBA8(plw, rec));
    }
}

/* Scales one attack value by the attack-boost skills the player has and writes the rank flag. */
extern "C" s32 fn_802739F0(_PLW* plw, s16 value, s32 mode, s8* out) {
    f32 v = (f32)value;

    if (mode == 0) {
        if (Pl_Skill_ck(plw, 65) == 1) {
            v *= lbl_8079A03C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 66) == 1) {
            v *= lbl_8079A078;
            *out = 2;
        }
    } else {
        if (Pl_Skill_ck(plw, 63) == 1 || Pl_cat_skill_ck(plw, 6) == 1) {
            v *= lbl_8079A07C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 64) == 1) {
            v *= lbl_8079A058;
            *out = 2;
        }
    }
    return (s32)v;
}

/* Stores two decoration equipment records into the spare slots, clearing whichever is absent. */
extern "C" void fn_802737B0(_PLW* plw, u8* a2, u8* a3, u8* a4) {
    fn_8027E98C(a2);
    equip_record_copy((_EQUIP*)((u8*)plw + 0x1DC), (_EQUIP*)a2);
    if (a3 != 0) {
        if (a3[0] != 0) {
            fn_8027E98C(a3);
            equip_record_copy((_EQUIP*)((u8*)plw + 0x200), (_EQUIP*)a3);
        } else {
            memset((u8*)plw + 0x200, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x200, 0, 12);
    }
    if (a4 != 0) {
        if (a4[0] != 0) {
            fn_8027E98C(a4);
            equip_record_copy((_EQUIP*)((u8*)plw + 0x20C), (_EQUIP*)a4);
        } else {
            memset((u8*)plw + 0x20C, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x20C, 0, 12);
    }
    plw->deco_dirty = 1;
}

/* Maps a slot's item id to the display slot number, the spare-slot half being marked with 0x80. */
extern "C" u16 fn_80273044(_PLW* plw, u16 slot) {
    int i;

    if (GetItemData__FUs(slot)[0] == 1 && fn_8026FE44(plw) == 1) {
        for (i = 0; i < 8; i++) {
            if (slot == plw->spare_slot_id[i].item_id) {
                return (u16)(i | 0x80);
            }
        }
    }
    for (i = 0; i < 24; i++) {
        if (slot == plw->slot_id[i].item_id) {
            return (u16)i;
        }
    }
    return 0xFFFF;
}

} /* namespace s_80270018 */
#pragma optimization_level 3
#pragma peephole off

/* ==== 0x80273B14-0x80276B58: the act-entry front end ==== */
#include "Pl/pl_act_stage_latch_set.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/lb_param_w.h"
#include "types.h"
#include "hud/Pl_net_send.h"
#include "pl.h"
#include "mh3_pad/control.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_802693C4.h"   /* the part/motion section's `Pl_chr_set_attr_default`/`fn_8026A2DC`/
                          * `fn_8026A2F8`/`Pl_chr_setX` (rule 2) */
#include "ef/fn_800CDB2C.h"
#include "unsplit/unknown.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "menu/menu_item.h"   /* `item_category_ck` (owner: `menu/menu_item.cpp`, rule 2) */
#include "ef/eft001.h"
#include "stage/stg_w.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"   /* quest_flag_80_ck, em_work_slot_pair_get, quest_element_remaining_get (rule 2) */
#include "quest/quest_entry.h"   /* quest_element_supply_state_get, quest_element_item_use (rule 2) */

/* 0x8045F554 - the runtime string copy `fn_8027552C` uses to arm a hunter name (owner: `MSL_C/alloc.cpp`, rule 2). */
#include "MSL_C/alloc.h"

/* 0x80335CE8 - the act-message sender `pl_act_enter_raw`'s tail calls (owner `hud/net_char_sync.cpp`, leaf header
 * `hud/Pl_net_send.h`).  The owner takes a third `u16`; this band's call sites pass two arguments (`mr r3, r30;
 * li r4, X; bl`), so they call it through this two-parameter view. */
typedef void (*PlNetSend2)(struct _PLW* plw, s32 kind);

/* The three equipment-slot records `fn_8027ECAC`/`fn_8027ED18`/`fn_8027E344` hand back.  Only this
 * unit reads them, so they live here (rule 1); each offset/width is the one its readers narrow to.
 */

/* The skill record an equipment slot resolves to. size: 0x18 */
typedef struct PlSkillRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u8 level_a;  /* scaled by 0x32 and biased by 0x96 into `+0x56E` (`fn_802740F4`) */
    /* +0x09 */ u8 level_b;  /* the sibling level the same function scales */
    /* +0x0A */ u8 pad_0x0A[0x3];
    /* +0x0D */ s8 value;    /* the value `fn_802744A0` stores into `+0x570` */
    /* +0x0E */ u8 pad_0x0E[0xA];
} PlSkillRec; /* size: 0x18 */

/* The weapon/skill record an equipment slot resolves to (the 0x24-byte sibling table). size: 0x24 */
typedef struct PlWeaponRec {
    /* +0x00 */ u8 pad_0x00[0x7];
    /* +0x07 */ s8 bonus_0x07;   /* the points `fn_80274E6C` accumulates */
    /* +0x08 */ u8 pad_0x08[0x1];
    /* +0x09 */ s8 bonus_0x09;   /* the points `fn_80275014` accumulates */
    /* +0x0A */ s16 bonus_0x0A;  /* the bonus `fn_802751B4` scales by 1/100 */
    /* +0x0C */ u8 pad_0x0C[0x6];
    /* +0x12 */ s8 bonus_0x12;   /* the points `fn_80274370`/`fn_802744A0` add up */
    /* +0x13 */ u8 pad_0x13[0x11];
} PlWeaponRec; /* size: 0x24 */

/* The per-slot record `fn_8027EE08` hands back through its second argument; `fn_80273B14` reads one
 * act-kind delta per slot out of it.  It is a different table from the `fn_8027E344` record below
 * (its +0x0A is a 2-byte value where that one's is a signed byte). size: 0x14 */
typedef struct PlEquipSlot {
    /* +0x00 */ u8 pad_0x00[0xA];
    /* +0x0A */ s16 value_0x0A;   /* the act kind's 2-byte delta (case 6) */
    /* +0x0C */ u8 value_0x0C;    /* the kind-0 delta */
    /* +0x0D */ u8 pad_0x0D[0x1];
    /* +0x0E */ u8 kind_0x0E;     /* the skill-tier id the 7..11 arms compare against */
    /* +0x0F */ s8 value_0x0F;
    /* +0x10 */ u8 kind_0x10;     /* the skill-tier id the 12..14 arms compare against */
    /* +0x11 */ s8 value_0x11;
    /* +0x12 */ u8 pad_0x12[0x2];
} PlEquipSlot; /* size: 0x14 */

/* The per-slot record `fn_8027E344` hands back; `fn_80273ED8` sums one signed byte per act kind out
 * of it. size: 0x10 */
typedef struct PlSlotRec {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ s8 value_0x08;
    /* +0x09 */ s8 value_0x09;
    /* +0x0A */ s8 value_0x0A;
    /* +0x0B */ s8 value_0x0B;
    /* +0x0C */ s8 value_0x0C;
    /* +0x0D */ u8 pad_0x0D[0x3];
} PlSlotRec; /* size: 0x10 */

/* One 8-byte row of the act/motion pick table `Pl_decide_mot_get` walks (0x805C60A8, 11 rows: the
 * motion, its parameter and the two control words the row is gated on). size: 0x8 */
typedef struct PlMotRow {
    /* +0x0 */ u16 motion;
    /* +0x2 */ u16 param;
    /* +0x4 */ u16 control_a;
    /* +0x6 */ u16 control_b;
} PlMotRow; /* size: 0x8 */
namespace s_80273B14 {
using ::_PLW;
using ::_EQUIP;
#include "Pl/fn_80273B14.h"

 /* size: 0x8 */

/* --- the act-kind dispatchers ----------------------------------------------------------------- */

/* Sums the act-kind deltas of the three equipment slots; `mode` selects the delta column (0/6 and the 7..16 skill
 * tiers), `flag` the melee/ranged variant (`fn_80273ED8`/`fn_80274D98` wrap it). */
extern "C" s32 fn_80273B14(struct _PLW* plw, s32 mode, u8 flag, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s16 value = 0;

    *out = 0;
    if ((u32)(mode - 6) > 10U && mode != 0) {
        return 0;
    }
    if (equip0 != NULL) {
        void* slot;
        void* scratch;
        /* `fn_8027EE08` is defined `void*(_EQUIP*, u32, u32)`, so the two out-pointers are passed as their
         * addresses and the register result is read back as the `s32` the caller tests. */
        s32 kind = (s32)fn_8027EE08(equip0, (u32)&slot, (u32)&scratch);

        if (kind == 1) {
            if (mode == 6) {
                value = fn_8027E5E4(equip0);
                if (equip1 != NULL && equip1->kind == 12) {
                    value += fn_8027E5E4(equip1);
                }
            } else if (mode == 0) {
                value = fn_8027E708(equip0);
                if (equip2 != NULL && equip2->kind == 13) {
                    value += fn_8027E708(equip2);
                }
            }
            return value;
        }
        if (kind != 0) {
            return value;
        }
        if (mode <= 16) {
            PlEquipSlot* rec = (PlEquipSlot*)slot;

            switch (mode) {
            case 6:
                value = rec->value_0x0A;
                break;
            case 0:
                value = rec->value_0x0C;
                break;
            case 7:
                if (rec->kind_0x0E == 1) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 8:
                if (rec->kind_0x0E == 2) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 9:
                if (rec->kind_0x0E == 3) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 10:
                if (rec->kind_0x0E == 4) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 11:
                if (rec->kind_0x0E == 5) {
                    value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                }
                break;
            case 12:
                if (rec->kind_0x10 == 1) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 13:
                if (rec->kind_0x10 == 2) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 14:
                if (rec->kind_0x10 == 3) {
                    value = fn_802739F0(plw, rec->value_0x11, 1, out);
                }
                break;
            case 15:
                value = fn_802739F0(plw, rec->value_0x0F, 0, out);
                break;
            case 16:
                value = fn_802739F0(plw, rec->value_0x11, 1, out);
                break;
            default:
                break;
            }
        }
    } else {
        if (equip1 != NULL && mode == 6 && equip1->kind == 12) {
            value = fn_8027E5E4(equip1);
        }
        if (equip2 != NULL && mode == 0 && equip2->kind == 13) {
            value += fn_8027E708(equip2);
        }
    }
    if (value < 0) {
        if (Pl_Skill_ck(plw, 0xC9) == 1U) {
            *out = 1;
            value = value * 3;
            return value;
        }
        if (flag == 0) {
            return 0;
        }
        value = value * 3;
    }
    return value;
}

/* The single-slot wrapper of `fn_80273B14`: it hands the player's own melee/ranged slots on. */
extern "C" s32 fn_80273ED8(struct _PLW* plw, s32 kind, s32 flag)
{
    s32 value = 0;

    if ((u32)(kind - 6) > 10U) {
        if ((u32)(kind - 1) > 4U) {
            if (kind != 0) {
                return 0;
            }
            value = fn_80273B14(plw, kind, (u8)flag, &plw->equipB, &plw->equipC, &plw->equipD,
                                (s8*)&plw->field_0x00E);
        } else {
            s32 n = 0;
            struct _EQUIP* slot = plw->equipA;

            do {
                PlSlotRec* rec = (PlSlotRec*)fn_8027E344(slot);

                switch (kind) {
                case 0:
                    value += fn_8027E510(slot);
                    break;
                case 1:
                    value += rec->value_0x08;
                    break;
                case 2:
                    value += rec->value_0x09;
                    break;
                case 3:
                    value += rec->value_0x0A;
                    break;
                case 4:
                    value += rec->value_0x0C;
                    break;
                case 5:
                    value += rec->value_0x0B;
                    break;
                }
                slot++;
                n++;
            } while (n < 6);
        }
    } else {
        value = fn_80273B14(plw, kind, (u8)flag, &plw->equipB, &plw->equipC, &plw->equipD,
                            (s8*)&plw->field_0x00E);
    }
    return value;
}

/* Maps a weapon/skill id to the bonus class the pick tables use. */
extern "C" u8 fn_8027403C(u16 kind)
{
    switch (kind) {
    case 0x8B:
        return 1;
    case 0x17D:
        return 3;
    case 0x17E:
        return 4;
    case 0x18B:
        return 2;
    default:
        return 0;
    }
}

/* Maps the player's act-state byte to the act kind `fn_80273B14` selects its delta column by. */
extern "C" s32 fn_8027408C(struct _PLW* plw)
{
    switch (plw->field_0x002) {
    case 6:
        return 1;
    case 5:
        fn_802740EC(plw);
        return 2;
    case 4:
        fn_802740EC(plw);
        return 0;
    default:
        return 0;
    }
}

/* Clears the player's short-range equipment slot record. */
extern "C" void fn_802740EC(struct _PLW* plw)
{
    fn_8027ED28(&plw->equipB);
}

/* Re-reads the player's armed weapon record into the level/attack bytes at +0x56A/+0x56E. */
extern "C" void fn_802740F4(struct _PLW* plw, struct _EQUIP* equip)
{
    PlSkillRec* rec = (PlSkillRec*)fn_8027ECAC(equip);

    plw->field_0x56A = rec->level_a;
    plw->field_0x56E = (s16)(rec->level_b * 0x32 + 0x96);
    if (Pl_Skill_ck(plw, 0x17) == 1U) {
        plw->field_0x56E += 0x32;
    }
    if (plw->field_0x56E > 0x1C2) {
        plw->field_0x56E = 0x1C2;
    }
}

/* Adds the act-kind and skill-tier deltas on top of `base`, and records which way the value moved. */
extern "C" s16 fn_80274174(struct _PLW* plw, s16 base, u8 ranged, s8* out)
{
    s16 value = base;

    if (base > 0 && ranged == 0 && fn_8026FE44(plw) == 0) {
        switch (plw->field_0x56B) {
        case 4:
            value += 5;
            break;
        case 5:
            value += 10;
            break;
        case 6:
            value += 15;
            break;
        default:
            break;
        }
    }
    if (ranged == 0) {
        if (Pl_dm_condition_ck(plw, 0x100) == 1U) {
            value -= 0x32;
            *out = 2;
        } else if (Pl_dm_condition_ck(plw, 0x200) == 1U) {
            value -= 0x32;
            *out = 2;
        }
    }
    if (Pl_Skill_ck(plw, 0x19) == 1U) {
        value += 10;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1A) == 1U) {
        value += 20;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1B) == 1U) {
        value += 30;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1C) == 1U) {
        value -= 5;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1D) == 1U) {
        value -= 10;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1E) == 1U) {
        value -= 15;
        *out = 2;
    }
    return value;
}
} /* namespace s_80273B14 */


/* Re-computes the player's critical-hit flag from the current weapon slot. */
s32 Pl_critical_get(struct _PLW* plw)
{
    using s_80273B14::fn_80274174;
    s8 flag;

    return fn_80274174(plw, plw->field_0x570, 0, &flag);
}
namespace s_80273B14 {


/* Sums the points of the two weapon records the act is holding, then folds in the act-kind and
 * skill-tier deltas. */
extern "C" s32 fn_80274370(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s32 value;

    *out = 0;
    switch (equip0->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        value = ((PlSkillRec*)fn_8027ECAC(equip0))->value;
        break;
    case 12:
    case 13:
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x12;
        break;
    case 11:
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x12;
        if (equip1 != NULL && equip1->kind == 12) {
            value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x12;
        }
        if (equip2 != NULL && equip2->kind == 13) {
            value += ((PlWeaponRec*)fn_8027ED18(equip2))->bonus_0x12;
        }
        break;
    default:
        return 0;
    }
    if (plw != NULL) {
        value = fn_80274174(plw, (s16)value, 1, out);
    }
    return value;
}

/* Refreshes the player's armed weapon class and ranged attack value out of the melee slot. */
extern "C" void fn_802744A0(struct _PLW* plw)
{
    switch (plw->equipB.kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        plw->field_0x570 = ((PlSkillRec*)fn_8027ECAC(&plw->equipB))->value;
        break;
    case 11:
        plw->field_0x570 = ((PlWeaponRec*)fn_8027ED18(&plw->equipB))->bonus_0x12;
        if (plw->equipD.kind == 13) {
            plw->field_0x570 += ((PlWeaponRec*)fn_8027ED18(&plw->equipD))->bonus_0x12;
        }
        if (plw->equipC.kind == 12) {
            plw->field_0x570 += ((PlWeaponRec*)fn_8027ED18(&plw->equipC))->bonus_0x12;
        }
        break;
    default:
        plw->field_0x570 = 0;
        break;
    }
}

/* True while the player's act is armed with a weapon. */
extern "C" s32 fn_80274570(struct _PLW* plw)
{
    return plw->field_0x30A != 0;
}

/* Writes the armed byte of the player's own move work record. */
extern "C" void fn_80274584(s8 value)
{
    struct _PLW* work = (struct _PLW*)get_move_work_adrs(2);

    if (work != NULL) {
        work = (struct _PLW*)((u8*)work + (s8)my_player_no() * 0xB20);
        work->field_0x5C8 = value;
    }
}
} /* namespace s_80273B14 */


/* The act-number lists `fn_802745DC` (by armed slot) and `fn_80274624` (by act number) walk: the
 * melee family, then the ranged family; slot 0 is unused and 0xFFFF ends a list.  size: 0x1C each */
u16 pl_act_no_tbl_melee[14] = {0x0000, 0x000A, 0x0004, 0x0003, 0x0008, 0x0000, 0x0002,
                               0x0001, 0x0009, 0x0005, 0x0006, 0x000B, 0x0007, 0xFFFF};
u16 pl_act_no_tbl_ranged[14] = {0x000E, 0x0018, 0x0012, 0x0011, 0x0016, 0x000E, 0x0010,
                                0x000F, 0x0017, 0x0013, 0x0014, 0x0019, 0x0015, 0xFFFF};
namespace s_80273B14 {


/* Reads the armed slot's value table for the given kind. */
extern "C" u32 fn_802745DC(struct _PLW* plw, s16 kind)
{
    u8 slot = plw->field_0x5C8;

    if (slot == 0) {
        return 0;
    }
    if (kind == 0) {
        return pl_act_no_tbl_melee[slot];
    }
    return pl_act_no_tbl_ranged[slot];
}

/* Finds the index the player's act number occupies in one of the two act tables. */
extern "C" s32 fn_80274624(struct _PLW* plw)
{
    const u16* row;
    s32 index;

    if (plw->field_0x00A != 0xB) {
        return 0;
    }
    row = pl_act_no_tbl_melee;
    index = 1;
    while (row[1] != 0xFFFF) {
        switch (row[1]) {
        case 5:
            if (plw->act_no == 5 || plw->act_no == 12) {
                return index;
            }
            break;
        case 11:
            if (plw->act_no == 11 || plw->act_no == 13) {
                return index;
            }
            break;
        default:
            if (plw->act_no == row[1]) {
                return index;
            }
            break;
        }
        index++;
        row += 1;
    }
    row = pl_act_no_tbl_ranged;
    index = 1;
    while (row[1] != 0xFFFF) {
        switch (row[1]) {
        case 19:
            if (plw->act_no == 19 || plw->act_no == 26) {
                return index;
            }
            break;
        case 25:
            if (plw->act_no == 25 || plw->act_no == 27) {
                return index;
            }
            break;
        default:
            if (plw->act_no == row[1]) {
                return index;
            }
            break;
        }
        index++;
        row += 1;
    }
    return 0;
}

/* Latches the act's follow-up stage and arms its two-frame hold. */
extern "C" void pl_act_stage_latch_set(struct _PLW* plw, u8 stage)
{
    if (Pl_master_ck(plw) != 0) {
        plw->field_0x01F = stage;
        plw->field_0x396 = 2;
    }
}

/* True when the act's follow-up stage is open and the player is in a state that may use it. */
extern "C" s32 fn_80274794(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if ((u32)(plw->field_0x00A - 8) <= 1U) {
        return 0;
    }
    if (plw->field_0x370 <= 0) {
        return 0;
    }
    return plw->field_0x01F == 0;
}

/* The act's follow-up stage byte. */
extern "C" u8 pl_act_stage_get(struct _PLW* plw)
{
    return plw->field_0x01F;
}

/* The player's own move work record. */
extern "C" struct _PLW* my_player_work_get(void)
{
    return (struct _PLW*)((u8*)get_move_work_adrs(2) + (s8)my_player_no() * 0xB20);
}

/* The first move work record whose slot byte differs from this player's. */
extern "C" void* fn_80274850(struct _PLW* plw)
{
    u8* work = (u8*)get_move_work_adrs(2);
    u16 count = (u16)get_move_work_max(2);

    if ((s32)count > 0) {
        do {
            if (work[8] != plw->chunk_ofs) {
                return work;
            }
            work += 0xB20;
        } while (--count != 0);
    }
    return NULL;
}

/* Buckets the player's stagger timer into 0-3. */
extern "C" s32 fn_802748C8(void* self)
{
    struct _PLW* plw = (struct _PLW*)self;
    s32 value = 0;

    if (plw->field_0x386 > 0x50) {
        value = 3;
    } else if (plw->field_0x386 > 0x28) {
        value = 2;
    } else if (plw->field_0x386 > 0) {
        value = 1;
    }
    return value;
}

/* True once the armed-slot table has been read for this player. */
extern "C" s32 fn_80274904(struct _PLW* plw)
{
    return plw->field_0x5C9 != 0;
}

/* Picks the act-name table row for the player's current stance. */
extern "C" u8* pl_act_name_row_get(struct _PLW* plw, s32 force, s32 index)
{
    if (force == 0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x5B8 >= 0xA0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x036 == 0) {
        return (u8*)(lbl_805C5F50 + index * 0xA);
    }
    return (u8*)(lbl_805C5F70 + index * 0xA);
}

/* True when either of the player's weapon slots resolves to a bonus. */
extern "C" s32 fn_80274988(struct _PLW* plw)
{
    if (fn_8026FE44(plw) == 0) {
        return 0;
    }
    if (fn_8027FFFC(&plw->equipB) == 1U) {
        return 1;
    }
    if (plw->equipC.kind == 12 && fn_8027FFFC(&plw->equipC) == 1U) {
        return 1;
    }
    return 0;
}

/* The same test for a caller-supplied slot pair and act kind. */
extern "C" s32 fn_80274A04(struct _EQUIP* equip0, struct _EQUIP* equip1, u8 kind)
{
    if ((u32)(kind - 4) > 2U) {
        return 0;
    }
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            if (fn_8027FFFC(equip0) == 1U) {
                return 1;
            }
            if (equip1 != NULL && equip1->kind == 12 && fn_8027FFFC(equip1) == 1U) {
                return 1;
            }
        } else if (equip0->kind == 12 && fn_8027FFFC(equip0) == 1U) {
            return 1;
        }
    }
    return 0;
}

/* True when the player's act kind is one the melee/ranged family owns. */
extern "C" s32 fn_80274AB8(struct _PLW* plw)
{
    if ((u32)(plw->field_0x002 - 4) > 2U) {
        if (plw->field_0x002 <= 1U || plw->field_0x002 == 3) {
            return 1;
        }
        return 0;
    }
    return fn_80274988(plw);
}

/* The same test for a standalone act kind. */
extern "C" s32 fn_80274AEC(u8 kind)
{
    if ((u32)(kind - 4) > 2U) {
        if (kind <= 1U || kind == 3) {
            return 1;
        }
        return 0;
    }
    return fn_80274A04(NULL, NULL, kind);
}

/* Looks an id up in the 4-byte-stride id/value table at 0x805BF490. */
extern "C" u8 fn_80274B20(u16 id)
{
    const u16* row = lbl_805BF490;

    for (;;) {
        u16 key = row[0];

        if (key == 0) {
            return 0xFF;
        }
        if ((u16)id == key) {
            return (u8)row[1];
        }
        row += 2;
    }
}

/* Sums the weapon/skill bonus records of the player's three weapon slots for the given act kind. */
extern "C" s32 fn_80274B5C(struct _PLW* plw, u8 kind, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    u8 value;

    *out = 0;
    value = (u8)fn_8027F008(equip0, kind);
    if (equip1 != NULL && equip1->kind == 12) {
        value = (u8)(value + (u8)fn_8027F008(equip1, kind));
    }
    if (equip2 != NULL && equip2->kind == 13) {
        value = (u8)(value + (u8)fn_8027F008(equip2, kind));
    }
    switch (kind) {
    case 0:
    case 1:
    case 2:
        if (Pl_Skill_ck(plw, 0x36) == 1U) {
            value += 3;
            *out = 1;
        }
        break;
    case 3:
        if (Pl_Skill_ck(plw, 0x37) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 4:
    case 5:
        if (Pl_Skill_ck(plw, 0x38) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 6:
        if (Pl_Skill_ck(plw, 0x39) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 7:
    case 8:
        if (Pl_Skill_ck(plw, 0x3A) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 9:
        if (Pl_Skill_ck(plw, 0x3B) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 10:
    case 11:
        if (Pl_Skill_ck(plw, 0x3C) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 12:
        if (Pl_Skill_ck(plw, 0x3D) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 13:
    case 14:
        if (Pl_Skill_ck(plw, 0x3E) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    default:
        break;
    }
    if (value != 0) {
        if (Pl_Skill_ck(plw, 0xAA) == 1U) {
            value += 1;
            *out = 1;
        }
    }
    return value;
}

/* The player-bound wrapper of `fn_80274B5C`. */
extern "C" u8 fn_80274D98(struct _PLW* plw, u8 kind)
{
    return (u8)fn_80274B5C(plw, kind, &plw->equipB, &plw->equipC, &plw->equipD,
                            (s8*)&plw->field_0x00E);
}

/* True when the player-bound bonus sum is non-zero. */
extern "C" u8 fn_80274DCC(struct _PLW* plw, u8 kind)
{
    return fn_80274D98(plw, kind) != 0;
}

/* Gates the act's bonus sum on the act id being a live one. */
extern "C" s32 fn_80274E00(struct _PLW* plw, u16 id)
{
    u8 cls;

    if (fn_8026FE44(plw) == 0) {
        return 1;
    }
    cls = fn_80274B20(id);
    if (cls == 0xFF) {
        return 1;
    }
    return fn_80274DCC(plw, cls);
}

/* Sums the weapon records' `bonus_0x07` column and folds the skill tiers in, clamped to 0..6. */
extern "C" u8 fn_80274E6C(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value = 0;

    *out = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x07;
        if (value == 0) {
            value = 1;
        }
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x07;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x2D) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2E) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2F) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x30) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x31) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x32) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    if (value < 0) {
        value = 0;
    }
    if (value > 6) {
        value = 6;
    }
    return (u8)value;
}

/* Sums the weapon records' `bonus_0x09` column plus a fixed 3, clamped to 0..9. */
extern "C" u8 fn_80275014(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value = 0;

    *out = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = ((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x09;
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x09;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x27) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x28) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x29) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x2A) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2B) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2C) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    value += 3;
    if (value < 0) {
        value = 0;
    }
    if (value > 9) {
        value = 9;
    }
    return (u8)value;
}

/* The two weapon records' elemental bonus product, scaled by 1/100. */
extern "C" f32 fn_802751B4(struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    f32 value = lbl_8079A000;

    *out = 0;
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            value = (f32)((PlWeaponRec*)fn_8027ED18(equip0))->bonus_0x0A;
        }
        if (equip1 != NULL && equip1->kind == 12) {
            value = value * (f32)((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x0A / lbl_8079A044;
        }
    } else if (equip1 != NULL && equip1->kind == 12) {
        value = (f32)((PlWeaponRec*)fn_8027ED18(equip1))->bonus_0x0A;
    }
    return value / lbl_8079A044;
}

/* True when the item/act id is one the player may still use.  `mode` is dead in retail - every call
 * site sets r4, the body never reads it (see the owner's header, which marks the name a GUESS). */
extern "C" s32 Pl_item_id_usable_ck(u16 id, s32 mode)
{
    if (item_category_ck(id, 1) != 0) {
        return 0;
    }
    if (item_category_ck(id, 2) != 0) {
        return 0;
    }
    if (item_category_ck(id, 0x10) != 0) {
        return 0;
    }
    if (fn_8027403C(id) != 0) {
        return 0;
    }
    if ((u32)(id - 0x1B6) <= 1U || id == 0x2B || id == 0x35 || id == 0xDF) {
        return 0;
    }
    return 1;
}

/* True when the player's act/act-state pair is one of the two held combinations. */
extern "C" s32 fn_80275394(struct _PLW* plw)
{
    switch (plw->field_0x00A) {
    case 0:
        if (plw->act_no == 0xB1 || plw->act_no == 0xA6) {
            return 1;
        }
        break;
    case 12:
        if (plw->act_no == 0xB) {
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* The per-act bonus the weapon slots contribute, out of the lobby parameter block's three rows. */
extern "C" s16 fn_802753E4(struct _PLW* plw, u16 id)
{
    s16 value = 0;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if (plw->field_0x446 != 0 && Pl_cat_skill_ck(plw, 0x27) == 0) {
        return 0;
    }
    if (id == lb_param_w.flag_0x0C[0]) {
        value = lb_param_w.value_0x10[0];
    }
    if (id == lb_param_w.flag_0x0C[1]) {
        value += lb_param_w.value_0x10[1];
    }
    if (id == lb_param_w.flag_0x0C[2]) {
        value += lb_param_w.value_0x10[2];
    }
    return value;
}

/* Marks the player as gone. */
extern "C" void fn_802754B4(struct _PLW* plw)
{
    plw->field_0x19 = 1;
}

/* Starts the mode's act for the player's current state. */
extern "C" void fn_802754C0(struct _PLW* plw, u8 flag)
{
    u8 kind = 0;

    if (Pl_master_ck(plw) == 1U) {
        if (PlayMode_ck() == 2) {
            kind = plw->chunk_ofs;
        }
        fn_80044B14(kind, flag);
    }
}

/* Copies (or clears) the player's hunter name in the move work record. */
extern "C" void fn_8027552C(struct _PLW* plw, const char* src)
{
    if (src == NULL) {
        memset(plw->name_0xB05, 0, 10);
        return;
    }
    strcpy((char*)plw->name_0xB05, src);
}
} /* namespace s_80273B14 */


/* The 11 act/motion pick rows `Pl_decide_mot_get` walks: motion, parameter and the two control
 * words each row is gated on. size: 0x58 */
PlMotRow pl_mot_pick_tbl[11] = {
    {0x02D1, 0x02BD, 0x2000, 0x2000}, {0x02D3, 0x02BF, 0x0800, 0x0800},
    {0x02D5, 0x02C3, 0x0400, 0x0400}, {0x02D7, 0x02C4, 0x1000, 0x1000},
    {0x02DA, 0x02C7, 0x2000, 0x2000}, {0x02D2, 0x02BE, 0x0800, 0x0800},
    {0x02D4, 0x02C0, 0x0400, 0x0400}, {0x02D6, 0x02C5, 0x1000, 0x1000},
    {0x02D8, 0x02C6, 0x0040, 0x4000}, {0x02D9, 0x02C1, 0x0080, 0x8000},
    {0x02DB, 0x02C2, 0x0030, 0x0014},
};

/* Picks the motion/parameter pair the player's current control state maps to. */
void Pl_decide_mot_get(u16* motion, u16* param)
{
    u16 index = (u16)(ran_suu(1) % 5);
    PlMotRow* row = pl_mot_pick_tbl;
    s32 i;

    for (i = 0; i < 11; i++) {
        switch (get_ControlType(0)) {
        case 1:
            if (i < 4) {
                if ((Psw[0].control_0x104 & row[i].control_a) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x104 & row[i].control_a) != 0 && (Psw[0].control_0x0FC & 4) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0FC & row[i].control_a) != 0) {
                index = (u16)i;
            }
            break;
        case 2:
            if (i < 4) {
                if ((Psw[0].control_0x0DE & row[i].control_b) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x0DE & row[i].control_b) != 0 && (Psw[0].control_0x0D6 & 0x88) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0D6 & row[i].control_b) != 0) {
                index = (u16)i;
            }
            break;
        default:
            break;
        }
    }
    *motion = row[index].motion;
    *param = row[index].param;
}
namespace s_80273B14 {


/* Resets every per-act field, then arms the new act's kind/number from its flag word `mask`. */
extern "C" void pl_act_enter_raw(struct _PLW* plw, u8 kind, u16 no, u16 mask)
{
    u32 i;

    plw->field_0x35C = 0;
    plw->field_0x360 = 0;
    plw->act_step_0x05 = 0;
    plw->field_0x006 = 0;
    plw->field_0x007 = 0;
    plw->prev_act_kind = plw->field_0x00A;
    plw->prev_act_no = plw->act_no;
    plw->field_0x00A = (u8)kind;
    plw->act_no = (u16)no;
    plw->field_0x264 = 0;
    plw->field_0x030 = 0;
    if ((mask & 0x20) != 0) {
        plw->field_0x0B6 = (u16)((ran_suu(1) & 0xFF00) | (plw->field_0x0B6 & 0xFF));
    } else {
        plw->field_0x0B6 = (u16)ran_suu(1);
    }
    plw->field_0x354 = lbl_8079A080;
    plw->field_0x318 = 0;
    plw->field_0x313 = 0;
    plw->field_0x314 = 0;
    plw->field_0x31C = 0;
    plw->field_0x320 = 0;
    plw->field_0x31E = 0;
    plw->field_0x36A = 0;
    plw->field_0x388 = 0;
    for (i = 0; i < 0x30; i++) {
        plw->field_0x322[i] = 0;
    }
    if ((mask & 0x400) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 - 0x2000);
    }
    if ((mask & 0x800) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 + 0x2000);
    }
    if (Pl_master_ck(plw) == 1U && (mask & 0x1000) != 0) {
        s32 step;

        mask &= 0xEFFF;
        switch (plw->field_0x002) {
        case 2:
            step = 0x1000;
            break;
        case 7:
            step = 0x1000;
            break;
        case 8:
            step = 0x1000;
            break;
        case 3:
            step = 0x1000;
            break;
        default:
            step = 0;
            break;
        }
        if (Pl_master_ck(plw) == 1U) {
            if (fn_8026FB20(plw, 0x800) == 1U) {
                plw->field_0x0A8 += step;
            } else if (fn_8026FB20(plw, 0x400) == 1U) {
                plw->field_0x0A8 -= step;
            }
        }
    }
    if ((mask & 0x4000) == 0) {
        plw->field_0x445 = 0;
    }
    if ((mask & 0x80) == 0) {
        plw->field_0x0AC = 0;
    }
    if ((mask & 0x100) == 0) {
        plw->field_0x567 = (u8)(plw->field_0x567 & 0xFE);
    }
    if ((mask & 1) == 0) {
        plw->field_0x646 = 0;
        plw->field_0x648 = 0;
        plw->field_0x64A = 0;
        plw->field_0x64F = 0;
    }
    if ((mask & 0x2000) == 0) {
        plw->field_0x068 = lbl_8079A084;
        plw->field_0x06C = lbl_8079A084;
        plw->field_0x070 = lbl_8079A084;
    }
    fn_8026A2DC(plw);
    fn_8026A2F8(plw);
    fn_8027AC2C(plw, 0xFF, 0xFF);
    plw->field_0x662 = 0;
    plw->field_0x3A3 = 0;
    plw->field_0x3A2 = 0;
    plw->field_0x565 = 0;
    plw->field_0x566 = 0;
    if (Pl_master_ck(plw) == 1U && (mask & 2) == 0) {
        if ((mask & 0x10) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 6);
        } else if ((mask & 0x40) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 9);
        } else {
            ((PlNetSend2)Pl_net_send)(plw, 1);
        }
        plw->field_0x012 = 0xF;
    }
}

/* Sets the per-act marker byte and arms the act. */
extern "C" void pl_act_enter(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x00E = 1;
    pl_act_enter_raw(plw, a, b, c);
}

/* The "2" marker's act entry, with the +0x256 timer preset. */
extern "C" void fn_80275ADC(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x584 = 1;
    plw->field_0x256 = 0x1C2;
    pl_act_enter_raw(plw, a, b, c);
}

/* Writes the player's act kind byte. */
extern "C" void fn_80275AFC(struct _PLW* plw, s8 value)
{
    plw->kind_0x09 = value;
}

/* Arms the act's three status bits from a packed mask. */
extern "C" void Pl_act_set_motion(struct _PLW* plw, u16 a, u32 b, u32 c)
{
    switch ((u8)a) {
    case 1:
        fn_80275AFC(plw, 1);
        break;
    case 2:
        fn_80275AFC(plw, 2);
        break;
    case 3:
        fn_80275AFC(plw, 3);
        break;
    default:
        fn_80275AFC(plw, 0);
        break;
    }
    if ((a & 0x8000) != 0) {
        fn_8026FEF0(plw, 1);
    } else {
        pl_act_set_flag(plw, 1);
    }
    if (b == 0) {
        fn_8026FEF0(plw, 2);
    } else {
        pl_act_set_flag(plw, 2);
    }
    if (c == 0) {
        fn_8026FEF0(plw, 4);
        return;
    }
    pl_act_set_flag(plw, 4);
}

/* The act-state dispatcher's motion hand-off. */
extern "C" void fn_80275C18(struct _PLW* plw, u32 motion, u32 a, u32 b, u32 c)
{
    if ((u16)c == 0) {
        Pl_chr_setX(plw, (u16)motion, a, b);
    } else {
        Pl_chr_set_attr_default(plw, (u16)motion, a, b);
    }
}

/* Picks and arms the entry motion for the player's current act state. */
extern "C" void fn_80275C34(struct _PLW* plw, u32 kind, u32 a, u32 b, u32 c, u32 d)
{
    plw->field_0x5C4 = 0;
    plw->field_0x310 = 0;
    plw->kind_0x09 = (u8)kind;
    if (kind != 1) {
        if (kind != 3) {
            if (plw->field_0x585 != 0) {
                if (plw->field_0x018 == 1) {
                    if (plw->field_0x002 == 8) {
                        if (fn_80331104() == 1U) {
                            fn_80275C18(plw, 0x3E9, a, b, d);
                        } else {
                            fn_80275C18(plw, 0x3FC, a, b, d);
                        }
                    } else {
                        fn_80275C18(plw, 0x3E9, a, b, d);
                    }
                } else {
                    fn_80275C18(plw, 0x18, a, b, d);
                }
                pl_act_enter(plw, 0, 0x6A, (u16)c);
                return;
            }
            if (plw->field_0x018 == 1) {
                if (plw->field_0x002 == 8) {
                    if (fn_80331104() == 1U) {
                        fn_80275C18(plw, 0x3E9, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x3FC, a, b, d);
                    }
                } else {
                    fn_80275C18(plw, 0x3E9, a, b, d);
                }
                pl_act_enter(plw, 0, 0, (u16)c);
                return;
            }
            if (fn_8027AC18(plw) == 1U) {
                fn_80275C18(plw, 0x190, a, b, d);
                pl_act_enter(plw, 0xA, 0, (u16)c);
                return;
            }
            if (plw->field_0x37A <= 0x96) {
                fn_80275C18(plw, 0x12F, a, b, d);
            } else {
                switch (pl_act_kind_get(plw->area_0x16)) {
                case 1:
                    if (Pl_Skill_ck(plw, 0x7C) == 1U || Pl_Skill_ck(plw, 0x7D) == 1U
                        || Pl_condition_ck(plw, 0x400) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x15, a, b, d);
                    }
                    break;
                case 3:
                    if (Pl_Skill_ck(plw, 0x7D) == 1U || Pl_condition_ck(plw, 0x400) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x15, a, b, d);
                    }
                    break;
                case 2:
                    if (Pl_Skill_ck(plw, 0x80) == 1U || Pl_Skill_ck(plw, 0x81) == 1U
                        || Pl_condition_ck(plw, 0x800) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x14B, a, b, d);
                    }
                    break;
                case 4:
                    if (Pl_Skill_ck(plw, 0x81) == 1U || Pl_condition_ck(plw, 0x800) == 1U) {
                        fn_80275C18(plw, 1, a, b, d);
                    } else {
                        fn_80275C18(plw, 0x14B, a, b, d);
                    }
                    break;
                default:
                    fn_80275C18(plw, 1, a, b, d);
                    break;
                }
            }
            pl_act_enter(plw, 0, 0, (u16)c);
            return;
        }
        if (plw->field_0x018 == 0) {
            if (fn_80276800(plw, 0) == 1) {
                fn_80275C18(plw, 0x79, a, b, d);
            } else if (plw->field_0x37A <= 0x96) {
                fn_80275C18(plw, 0x168, a, b, d);
            } else if (Pl_suimen_ck(plw) == 1U) {
                fn_80275C18(plw, 0x76, a, b, d);
            } else {
                fn_80275C18(plw, 0x64, a, b, d);
            }
        } else if (plw->field_0x002 == 8) {
            if (fn_80331104() == 1U) {
                fn_80275C18(plw, 0x41A, a, b, d);
            } else {
                fn_80275C18(plw, 0x438, a, b, d);
            }
        } else {
            fn_80275C18(plw, 0x41A, a, b, d);
        }
        pl_act_enter(plw, 0, 0x16, (u16)(c | 0x80));
        return;
    }
    if (plw->field_0x585 != 0) {
        fn_80275C18(plw, 0x18, a, b, d);
        pl_act_enter(plw, 0, 0x6A, (u16)c);
        return;
    }
    fn_80275C18(plw, 8, a, b, d);
    if (fn_8027AC18(plw) == 1U) {
        pl_act_enter(plw, 0xA, 0xE, (u16)c);
        return;
    }
    pl_act_enter(plw, 0, 0x1F, (u16)c);
}

/* The three no-argument act-state entries. */
extern "C" void Pl_act_set_motion_slot(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 0, 0);
}

extern "C" void fn_802761C4(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 12, 1);
}

extern "C" void fn_802761D0(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 2, 1);
}

/* The motion-parameter act-state entry. */
extern "C" void fn_802761DC(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 1, 0);
}

/* Clears the act's hold latch and re-enters the act state. */
extern "C" void pl_act_reenter(struct _PLW* self, s32 a, s32 b, s32 c)
{
    self->field_0x442 = 0;
    self->field_0x30A = 0xFF;
    fn_80275C34(self, a, b, c, 0, 0);
}

/* True when the given motion value fits inside the act's stagger budget. */
extern "C" s32 fn_80276254(struct _PLW* self, s32 value)
{
    return (s16)value <= self->field_0x378;
}

/* True when the given motion value fits inside the act's hold gauge. */
extern "C" s32 fn_80276270(struct _PLW* plw, s32 value)
{
    return (s16)value <= plw->field_0x370;
}

/* Adds `delta` to the act's hold gauge, clamped to 0..`+0x372`. */
extern "C" void pl_act_add_hold_gauge(struct _PLW* plw, s16 delta)
{
    if (Pl_master_ck(plw) != 0 && (Pl_motion_input_ck(0) != 1U || delta >= 0) && plw->field_0x00A != 8) {
        s16 value;

        plw->field_0x370 += delta;
        value = plw->field_0x370;
        if (value <= 0) {
            plw->field_0x370 = 0;
        }
        if (plw->field_0x370 >= plw->field_0x372) {
            plw->field_0x370 = plw->field_0x372;
        }
        if (plw->field_0x376 < plw->field_0x370) {
            plw->field_0x376 = plw->field_0x370;
        }
    }
}

/* The act's frame step: runs the hold gauge down and either enters the act's next motion or dumps
 * the player out of it.  Returns 1 when the act ended this frame. */
extern "C" s32 fn_8027633C(struct _PLW* plw, s16 delta, s8* out)
{
    nw4r::math::VEC3 pos;
    s16 before;

    VEC3_ctor(&pos);
    if (out != NULL) {
        *out = 0;
    }
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    before = plw->field_0x370;
    pl_act_add_hold_gauge(plw, delta);
    if (plw->field_0x370 <= 0) {
        if (Pl_cat_skill_ck(plw, 0x2C) == 1U) {
            if (plw->field_0x447 == 0 && before >= 0x40) {
                plw->field_0x447++;
                plw->field_0x370 = 1;
                plw->field_0x376 = 1;
                ((PlJointHolder*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(3, &pos);
                fn_800FC0F0(&pos, 0, 0, plw->area_0x16, (_CP_VECTOR*)&plw->param_0x54, lbl_8079A080);
                if (out != NULL) {
                    *out = 1;
                }
                return 0;
            }
        }
        plw->field_0x376 = 0;
        if (plw->kind_0x09 != 2) {
            fn_80278BE4(plw);
            return 1;
        }
        if (plw->field_0x00A != 6 && Pl_act_ck(plw, 2, 1) == 0) {
            pl_act_enter_raw(plw, 2, 1, 0);
        }
    }
    return 0;
}

/* The same step, suppressed while the shell/ammo latch is latched. */
extern "C" s32 fn_802764B0(struct _PLW* plw, s16 delta, s8* out)
{
    if (fn_8027E1E4(plw) == 1U) {
        return 0;
    }
    return fn_8027633C(plw, delta, out);
}

/* The same step for a signed gauge delta (a negative one also moves the stagger gauge). */
extern "C" s32 fn_80276514(struct _PLW* plw, s16 delta, s8* out)
{
    s16 value;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    value = 0;
    if (delta < 0) {
        plw->field_0x376 += delta;
        if (plw->field_0x376 < plw->field_0x370) {
            value = plw->field_0x376 - plw->field_0x370;
        }
    } else {
        value = delta;
    }
    return fn_8027633C(plw, value, out);
}

/* Applies the skill-tier damage bonus and steps the hold gauge. */
extern "C" void fn_802765B4(struct _PLW* plw, s16 amount)
{
    s16 value = amount;

    if (amount > 0) {
        if (Pl_cat_skill_ck(plw, 3) == 1U) {
            value += amount / 10;
        }
        if (Pl_Skill_ck(plw, 0x98) == 1U) {
            value += value / 4;
        } else if (Pl_Skill_ck(plw, 0x99) == 1U) {
            value -= value / 4;
        }
    }
    pl_act_add_hold_gauge(plw, value);
}

/* Grows the act's gauge ceilings (and, when `a` is 0, the soft cap) by `amount`. */
extern "C" void fn_80276690(struct _PLW* plw, s32 amount, u8 a)
{
    if (Pl_master_ck(plw) != 0) {
        s16 value;

        plw->field_0x372 += (s16)amount;
        if (plw->field_0x372 <= 1) {
            plw->field_0x372 = 1;
        }
        if (plw->field_0x372 >= 0x96) {
            plw->field_0x372 = 0x96;
        }
        value = plw->field_0x372;
        if (plw->field_0x370 >= value) {
            plw->field_0x370 = value;
        }
        if (plw->field_0x376 >= value) {
            plw->field_0x376 = value;
        }
        if (a == 0) {
            plw->field_0x374 += (s16)amount;
            if (plw->field_0x374 <= 1) {
                plw->field_0x374 = 1;
            }
            if (plw->field_0x374 >= 0x96) {
                plw->field_0x374 = 0x96;
            }
        }
    }
}

/* Adds to the act's stagger budget up to its 150-point ceiling. */
extern "C" void fn_80276778(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x380 += amount;
    if (plw->field_0x380 >= 0x96) {
        plw->field_0x380 = 0x96;
    }
    fn_802767B4(plw, 0x96);
}

/* Adds to the act's current stagger value, clamped to 0..`+0x380`. */
extern "C" void fn_802767B4(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x37E += amount;
    if (plw->field_0x37E >= plw->field_0x380) {
        plw->field_0x37E = plw->field_0x380;
        return;
    }
    if (plw->field_0x37E < 0) {
        plw->field_0x37E = 0;
    }
}

/* True while the act's current stagger value is inside `v`. */
extern "C" u32 fn_80276800(struct _PLW* self, s32 v)
{
    return self->field_0x37E <= v;
}

/* True while the act's hold-gauge re-arm word is set. */
extern "C" u32 Pl_act_state_ck(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    return plw->field_0x412 > 0;
}

/* Steps the act's stagger budget by `amount`, clamped to 0..`+0x37A`. */
extern "C" void fn_80276868(struct _PLW* plw, s16 amount)
{
    if (Pl_master_ck(plw) != 0 && (amount >= 0 || Pl_act_state_ck(plw) != 1)) {
        s16 value;

        plw->field_0x378 += amount;
        value = plw->field_0x378;
        if (value <= 0) {
            plw->field_0x378 = 0;
        }
        if (plw->field_0x378 >= plw->field_0x37A) {
            plw->field_0x378 = plw->field_0x37A;
        }
    }
}

/* The same step with the defence-skill and combo modifiers folded in. */
extern "C" void pl_act_gauge_gate(struct _PLW* plw, s16 amount)
{
    s16 value = amount;

    if (Pl_master_ck(plw) == 0) {
        return;
    }
    if (value < 0) {
        if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
            value = value * 4 - value;
        } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
            value = value * 4 - value;
        }
        if (value == -1) {
            if (Pl_Skill_ck(plw, 0xA8) == 1U && (plw->frame_0x020 & 1) != 0) {
                return;
            }
        } else if (Pl_Skill_ck(plw, 0xA8) == 1U) {
            value = (s16)(((u32)value >> 0x1FU) + value >> 1);
        }
        if (Pl_Skill_ck(plw, 0xA9) == 1U && plw->frame_0x020 % 5 == 0) {
            value = value * 2;
        }
    }
    fn_80276868(plw, value);
}

/* The act's guard-break entry: picks the stagger step from the defence skills and applies it. */
extern "C" void fn_80276A3C(struct _PLW* plw)
{
    s16 step = -1;
    s32 mask = 0;

    if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
        step = -3;
    } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
        step = -3;
    }
    if (Pl_Skill_ck(plw, 0xA8) == 1U) {
        mask = 1;
    }
    if (Pl_cat_skill_ck(plw, 0x17) == 1U) {
        s32 had = mask;

        mask = 3;
        if (had == 0) {
            mask = 1;
        }
    }
    if ((u16)mask == 0 || (plw->frame_0x020 & (u16)mask) == 0) {
        if (Pl_Skill_ck(plw, 0xA9) == 1U && plw->frame_0x020 % 5 == 0) {
            step *= 2;
        }
        fn_80276868(plw, step);
    }
}

} /* namespace s_80273B14 */

/* ==== 0x80276B58-0x8027D684: the action state machines ==== */

#include "types.h"
#include "enemy/enemy_control.h"
#include "ef/fn_800CDB2C.h"   /* my_player_no (rule 2: the owner is `ef/system_core.cpp`) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Pl/pl_coll.h" /* the owner of the `.bss` move-work table `pl_move_work` (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "quest/quest_item_slot.h" /* quest_item_work_notify (rule 2: its owner) */

/* The actors this unit calls into; the mangling of the source names reproduces the map's spellings. */
u32 Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);
namespace s_80276B58 {


extern "C" u32 Pl_act_state_ck(_PLW*);
extern "C" void fn_80276868(_PLW*, s16);
extern "C" void fn_80276CE8(_PLW*, s16);
extern "C" u8 pl_act_kind_get(u8);
extern "C" s32 fn_80331104(void);
extern "C" void fn_8010D688(_PLW*);
extern "C" u32 fn_8026FE44(_PLW*);
extern "C" void fn_800E1640(u8*, f32);
extern "C" u32 fn_802B0688(void*);
extern "C" void fn_8026FEF0(_PLW*, s32);
extern "C" void pl_act_enter(_PLW*, s32, u16, u16);

/* In-unit callees that the appended bodies reference before their own definition. */
extern "C" void fn_802789EC(_PLW*, s32);
extern "C" void fn_8027A17C(_PLW*);
extern "C" u32 fn_80278144(u8, u8*, u8);
extern "C" u32 fn_80278310(u8, u8*, u8);
extern "C" void fn_80278B58(_PLW*, s32, s32);
} /* namespace s_80276B58 */


/* condition-bit tests over the actor's flag words, one per flag set. */
u32 Pl_dm_condition_ck(_PLW*, u32);


u32 Pl_act_ck(_PLW*, u8, u16);
u32 Pl_bari_ck(_PLW*, s32);
void Pl_get_gunner_vec(_PLW*, _CP_VECTOR*);
void Pl_get_gunner_pos(_PLW*, nw4r::math::VEC3*, s32);
void cpSetRotMatrixZXY(_CP_VECTOR*, nw4r::math::MTX34*);
void rotVecXYZ(nw4r::math::VEC3*, _CP_VECTOR*);
namespace s_80276B58 {


extern "C" void eft_rot_vec_copy(_CP_VECTOR*, void*);
extern "C" f32 fn_80050EF4(void*, void*);
extern "C" u32 ef_inst_spawn(_PLW*, s32);
extern "C" u32 stage_map_kind_get(u8);
extern "C" u32 fn_802753E4(_PLW*, s32);
extern "C" void fn_80278D1C(_PLW*);
} /* namespace s_80276B58 */

u8 get_now_mapno(void);
u32 Pl_frame_check(_PLW*, u32, f32, f32);
f32 GetGroundHit2(nw4r::math::VEC3*, u32, u8, u8*);
void rotVecY(nw4r::math::VEC3*, u32);
namespace s_80276B58 {


extern "C" s32 fn_802E5CFC(s32);
extern "C" u32 isServerSelectState(s32);
/* 0x80338E04 - `lobby/lb_companion_ui.cpp`'s.  Its header declares `Pl_cat_skill_ck` returning `void` against this
 * file's `u32` (`(10505) illegal overloading`), so the declaration is this unit's view in a linkage block; the
 * three arguments are this unit's call site (the callee reads r3/r4/r5). */
extern "C" {
void lb_entry_handover_send(s32, u8, u8);
}
extern "C" void pl_item_add(_PLW*, u16, s16);
extern "C" void fn_802E5D68(u16);

extern "C" s32 pl_part_flag_ck(_PLW*, s32);
extern "C" u8 fn_802748C8(_PLW*);

extern "C" u32 pl_carry_item_get(_PLW*);
extern "C" s32 fn_80272C80(_PLW*, u8);
extern "C" u8 fn_80274B20(u16);
extern "C" s16 fn_80272CC8(_PLW*, u8);
extern "C" u8 fn_80274D98(_PLW*, u8);
extern "C" u32 fn_8028732C(_PLW*);
extern "C" u8* fn_8027ED18(void*);
extern "C" u8* fn_80279360(u8*, u8);

extern "C" { extern const f32 lbl_8079A088; }
extern "C" { extern const f32 lbl_8079A08C; }
extern "C" { extern const f32 lbl_8079A090; }
extern "C" { extern const f32 lbl_8079A094; }
extern "C" { extern const f32 lbl_8079A098; }
extern "C" { extern const f32 lbl_8079A09C; }
extern "C" { extern const f64 lbl_8079A0A0; }
extern "C" { extern const f32 lbl_8079A0A8; }
extern "C" { extern const f32 lbl_8079A0AC; }
extern "C" { extern const f32 lbl_8079A0B0; }
extern "C" { extern const f32 lbl_8079A0B4; }
extern "C" { extern const f32 lbl_8079A084; }
extern "C" { extern const f32 lbl_8079A0C8; }
extern "C" { extern const f32 lbl_8079A0CC; }
extern "C" { extern const f32 lbl_8079A0D0; }
extern "C" { extern const f32 lbl_8079A0DC; }
extern "C" { extern const f32 lbl_8079A0E0; }
extern "C" { extern const f32 lbl_8079A0E4; }
extern "C" { extern const f32 lbl_8079A0E8; }
extern "C" { extern const f32 lbl_8079A0FC; }
extern "C" { extern const f32 lbl_8079A0F4; }
extern "C" { extern const f32 lbl_8079A100; }
extern "C" { extern const f32 lbl_8079A104; }
extern "C" { extern const f32 lbl_8079A108; }
extern "C" { extern const f32 lbl_8079A10C; }
extern "C" { extern const f32 lbl_8079A0C4; }
extern "C" { extern const f32 lbl_8079A0EC; }
extern "C" { extern const f32 lbl_8079A110; }

extern "C" { extern u32 lbl_805E2248[]; }
extern "C" { extern u32 lbl_805E25D0[]; }

/* 0x80276B58: the attack-scale multiplier the actor's active skills grant for a signed modifier. */
extern "C" void fn_80276B58(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) != 0) {
        f32 f = (f32)(s16)arg1;
        if ((s16)arg1 < 0) {
            if (Pl_Skill_ck(self, 0xB9) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A088;
                } else {
                    f *= lbl_8079A08C;
                }
            } else if (Pl_Skill_ck(self, 0xBA) == 1) {
                f *= lbl_8079A088;
            } else if (Pl_Skill_ck(self, 0xBB) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A090;
                } else {
                    f *= lbl_8079A094;
                }
            } else if (Pl_Skill_ck(self, 0xBC) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A098;
                } else {
                    f *= lbl_8079A09C;
                }
            } else if (Pl_cat_skill_ck(self, 4) == 1) {
                f *= lbl_8079A08C;
            }
        }
        fn_80276868(self, (s16)f);
    }
}

/* 0x80276CE8: adds a signed amount to the actor's stamina pool and clamps it, with a lower bound reset. */
extern "C" void fn_80276CE8(_PLW* self, s16 arg1)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 < 0 && Pl_act_state_ck(self) == 1) {
        return;
    }
    {
        self->unk37A += arg1;
        if (self->unk37A <= 0x96) {
            self->unk37A = 0x96;
        } else if (self->unk37A > 0x384) {
            self->unk37A = 0x384;
            self->unk37C = 0x2A30;
        }
        s16 v = self->unk37A;
        if (self->unk378 > v) {
            self->unk378 = v;
        }
    }
}

/* 0x80276D94: applies the stamina bonus the two armour skills grant, then the signed amount. */
extern "C" void fn_80276D94(_PLW* self, s32 arg1)
{
    if ((s16)arg1 > 0 && (Pl_Skill_ck(self, 0x48) == 1 || Pl_Skill_ck(self, 0x49) == 1)) {
        arg1 += 150;
    }
    fn_80276CE8(self, (s16)arg1);
}

/* 0x80276E08: recomputes the actor's attack-range/level modifier from its weapon class and skills. */
extern "C" void fn_80276E08(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        s16 cls = pl_act_kind_get(self->area_0x16);
        s32 v = 0;
        if (cls == 2 || cls == 4) {
            if (self->unk466 == 0 && (cls != 2 || (Pl_Skill_ck(self, 0x80) != 1 && Pl_Skill_ck(self, 0x81) != 1))
                && (cls != 4 || Pl_Skill_ck(self, 0x81) != 1)) {
                if (cls == 2) {
                    if (Pl_Skill_ck(self, 0x82) == 1) {
                        v = 3;
                    } else if (Pl_Skill_ck(self, 0x83) == 1) {
                        v = 4;
                    } else {
                        v = 2;
                    }
                } else if (Pl_Skill_ck(self, 0x80) == 1) {
                    v = 1;
                } else if (Pl_Skill_ck(self, 0x82) == 1) {
                    v = 4;
                } else if (Pl_Skill_ck(self, 0x83) == 1) {
                    v = 6;
                } else {
                    v = 3;
                }
            }
        } else {
            v = 0;
        }
        if (Pl_Skill_ck(self, 0x45) == 0) {
            if (self->kind_0x09 == 3) {
                if ((self->unk020 & 1) == 0 && (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 3) != 0)) {
                    v += 1;
                    if (Pl_Skill_ck(self, 0x47) == 1) {
                        v += 1;
                    } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 3) == 0) {
                        v += 1;
                    }
                }
            } else if (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 1) != 0) {
                v += 1;
                if (Pl_Skill_ck(self, 0x47) == 1) {
                    v += 1;
                } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 1) == 0) {
                    v = (s16)(v + 1);
                }
            }
        }
        if ((s16)v > 0) {
            self->unk37C -= (s16)v;
            if ((s16)self->unk37C <= 0) {
                if (Pl_act_state_ck(self) == 1) {
                    self->unk37C = 1;
                    return;
                }
                self->unk37C = 0x2A30;
                fn_80276CE8(self, -0x96);
            }
        }
    }
}

/* 0x802770E0 */
extern "C" s32 fn_802770E0(void)
{
    return 0;
}

/* 0x802770E8: initialises the actor's attack-range state - the id/count words, the zeroed tail fields and
 * the 16-byte per-range table. */
extern "C" void Pl_act_set_step_table(_PLW* self, u32 table, s32 arg2)
{
    self->unk318 = table;
    self->unk313 = (s8)arg2;
    self->unk314 = 0;
    self->unk31C = 0;
    self->unk320 = 0;
    self->unk31E = 0;
    for (s16 i = 0; i < 48; i++) {
        self->unk322[i] = 0;
    }
}

/* 0x802771A0: picks the attack-range table for the actor's weapon class and initialises it. */
extern "C" void fn_802771A0(_PLW* self, s32 arg1)
{
    if (self->kind_0x09 != 3) {
        if (self->field_0x002 == 8 && fn_80331104() == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805E2248, (s16)arg1);
        } else {
            Pl_act_set_step_table(self, lbl_805BF448[self->field_0x002], (s16)arg1);
        }
    } else {
        if (self->field_0x002 == 8 && fn_80331104() == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805E25D0, (s16)arg1);
        } else {
            Pl_act_set_step_table(self, lbl_805BF46C[self->field_0x002], (s16)arg1);
        }
    }
}

/* 0x80277B44 */
extern "C" void pl_act_set_gauge_arm(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
}

/* 0x80277B4C: scales the actor's attack-range modifier by the two armour skills. */
extern "C" void pl_act_set_gauge_arm_skilled(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
    if (Pl_cat_skill_ck(self, 28) == 1) {
        self->unk396 = (s16)(self->unk396 * 3);
    } else if (Pl_cat_skill_ck(self, 29) == 1) {
        self->unk396 = (s16)(self->unk396 * 2);
    }
}

/* 0x80277C48 */
extern "C" void pl_act_set_step_time(_PLW* self, s16 arg1)
{
    self->unk580 = arg1;
}

/* 0x80277C50 */
extern "C" void fn_80277C50(_PLW* self, s16 arg1)
{
    self->field_0x45A = arg1;
}

/* 0x80277FE0 */
extern "C" s32 fn_80277FE0(_PLW* self)
{
    return 1;
}

/* 0x80277FE8 */
extern "C" u32 fn_80277FE8(_PLW* self)
{
    return (u32)(self->unk30C - 1) >> 31;
}

/* 0x802784A8 */
extern "C" s32 fn_802784A8(_PLW* self)
{
    return (u32)(self->unk30D - 3) >> 31;
}

/* 0x80278564 */
extern "C" void pl_act_arm_flags(_PLW* self, u32 arg1)
{
    self->unk388 |= (u8)arg1;
}

/* 0x80278578 */
extern "C" s32 fn_80278578(_PLW* self, u32 arg1)
{
    return (self->unk388 & (u16)arg1) == 0;
}
} /* namespace s_80276B58 */


/* 0x80276E08 sibling: whether the actor is inside the invulnerability window the suimen skill
 * grants (a +/- one-unit band around its current height). */
u32 Pl_suimen_ck(_PLW* self)
{
    using s_80276B58::lbl_8079A0CC;
    if (self->unk074 != 0) {
        if (self->motion_pos_0x40 >= self->unk064 - lbl_8079A0CC && self->motion_pos_0x40 <= lbl_8079A0CC + self->unk064) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790B4: whether any of the given condition bits is set in the actor's condition word. */
u32 Pl_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3D8 & mask) != 0;
}

/* 0x802790CC: whether any of the given bits is set in the actor's damage-condition word. */
u32 Pl_dm_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3DC & mask) != 0;
}
namespace s_80276B58 {


/* 0x802790E4 */
extern "C" u32 fn_802790E4(_PLW* self, u32 mask)
{
    return (self->unk3E0 & mask) != 0;
}

/* 0x80279154 */
extern "C" void fn_80279154(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        if (self->unk448 < arg2) {
            self->unk448 = arg2;
        }
    } else {
        if (self->unk449 < arg2) {
            self->unk449 = arg2;
        }
    }
}

/* 0x802798FC */
extern "C" s32 fn_802798FC(_PLW* self)
{
    if ((self->unk269 == 0 || (self->unk269 < self->unk26A && self->unk269 < self->unk270))
        && self->unk270 > 0) {
        return 1;
    }
    return 0;
}

/* 0x8027A000: adds a signed amount to the actor's charge counter and clamps it to +/-100. */
extern "C" void fn_8027A000(_PLW* self, s32 arg1)
{
    self->gauge_0x64F += (s8)arg1;
    if (self->gauge_0x64F >= 100) {
        self->gauge_0x64F = 100;
    }
    if (self->gauge_0x64F <= -100) {
        self->gauge_0x64F = -100;
    }
}

/* 0x8027A044: adds a signed amount to the actor's aim angle and clamps it to +/-90 degrees. */
extern "C" void fn_8027A044(_PLW* self, s16 arg1)
{
    self->unk5E8 += arg1;
    if (arg1 >= 0) {
        if (self->unk5E8 >= 8192) {
            self->unk5E8 = 8192;
        }
    } else {
        if (self->unk5E8 <= -8192) {
            self->unk5E8 = -8192;
        }
    }
}

/* 0x8027A17C */
extern "C" void fn_8027A17C(_PLW* self)
{
    self->unk5E6 = 0;
    self->unk5E5 = 0;
    self->unk5E8 = 0;
}

/* 0x8027A190 */
extern "C" s32 fn_8027A190(_PLW* self)
{
    return 1;
}

/* 0x8027A198 */
extern "C" s32 fn_8027A198(_PLW* self)
{
    if (self->field_0x00A == 0) {
        s32 id = self->act_no;
        if ((u32)(id - 68) <= 1 || id == 71) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027A554 */
extern "C" s32 fn_8027A554(_PLW* self)
{
    if (self->field_0x18 == 1 && self->unk5E6 != 0) {
        return 0;
    }
    return 1;
}

/* 0x8027AC00 */
extern "C" void fn_8027AC00(_PLW* self)
{
    self->unk5BB = 1;
}

/* 0x8027AC0C */
extern "C" void pl_act_clear_flag5bb(_PLW* self)
{
    self->unk5BB = 0;
}

/* 0x8027AF34: ticks the actor's residual-velocity timer and integrates its position deltas. */
extern "C" void fn_8027AF34(_PLW* self)
{
    s16 v = self->unk0B4;
    if (v > 0) {
        self->unk0B4--;
        /* unreachable - it makes the compiler reuse the first test's condition register */
        if (v < 0) {
            return;
        }
        self->motion_pos_0x3C += self->unk09C;
        self->motion_pos_0x40 += self->unk0A0;
        self->motion_pos_0x44 += self->unk0A4;
    }
}

/* 0x8027AF80 */
extern "C" s32 fn_8027AF80(u8 dir)
{
    return 1;
}

/* 0x8027C030 */
extern "C" s32 fn_8027C030(_PLW* self)
{
    if (self->field_0x00A == 7) {
        s32 id = self->act_no;
        if ((u32)(id - 5) <= 1 || id == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027C89C: raises the actor's stored action id to the given one. */
extern "C" void fn_8027C89C(_PLW* self, s16 arg1)
{
    if (self->unk468 < arg1) {
        self->unk468 = arg1;
    }
}

/* 0x8027CA04 */
extern "C" s32 fn_8027CA04(_PLW* self, s32 arg1)
{
    if (self->field_0x002 != 7) {
        return 0;
    }
    if (self->unk468 > 0) {
        return 1;
    }
    return self->field_0x384 >= (s16)arg1;
}

/* 0x8027CC2C */
extern "C" u32 fn_8027CC2C(_PLW* self)
{
    return (self->field_0x5A4 & 0x40) != 0;
}

/* 0x8027CF24 */
extern "C" s32 fn_8027CF24(_PLW* self)
{
    if (fn_8026FE44(self) == 1 && self->unk276 != 0) {
        return 1;
    }
    return 0;
}

/* 0x8027CF70 */
extern "C" s32 fn_8027CF70(_PLW* self)
{
    return Pl_condition_ck(self, 0x20000);
}
} /* namespace s_80276B58 */


/* 0x8027CF78: starts a zanzo (afterimage) trail on the actor's current motion. */
void Pl_zanzo_set(_PLW* self, s32 arg1, u8 arg2)
{
    self->field_0x662 = (s16)arg1;
    self->field_0x65E = arg2;
    self->field_0x664 = Get_motion_no(self);
    self->field_0x666 = 0xFFFF;
}
namespace s_80276B58 {


/* 0x8027D3F0 */
extern "C" void fn_8027D3F0(_PLW* self, u8 arg1)
{
    self->unk3AC |= 1 << arg1;
}

/* 0x8027D4F0 */
extern "C" void fn_8027D4F0(_PLW* self)
{
    self->unk3B4 = 1;
}

/* 0x8027D4FC */
extern "C" s32 fn_8027D4FC(_PLW* self)
{
    return self->unk3B4 != 0;
}

/* 0x8027D510 */
extern "C" void fn_8027D510(_PLW* self)
{
    self->unk3B5 = 1;
}

/* 0x8027D51C */
extern "C" s32 fn_8027D51C(_PLW* self)
{
    return self->unk3B5 != 0;
}

/* 0x8027D584 */
extern "C" void fn_8027D584(_PLW* self, u8* arg1)
{
    self->unk59C = arg1[3] | 0x8000;
    self->unk59E = 0;
    self->unk5A0 = 0;
}

/* 0x80277C58: writes the actor's remaining vertical range from its motion frame data. */
extern "C" void pl_act_set_frame_timer(_PLW* self)
{
    u8* p = ((u8*)self->physics_0x13C);
    f32 v = *(f32*)(p + 72);
    if (v < lbl_8079A084) {
        v = lbl_8079A084;
    }
    self->unk264 = (s16)(*(f32*)(p + 120) - v);
}

/* 0x80277BC4: the attack-range modifier the actor's two armour skills set. */
extern "C" void fn_80277BC4(_PLW* self, u8 arg1)
{
    s32 v;
    if (arg1 == 0) {
        v = 6;
        if (Pl_Skill_ck(self, 0xA1) == 1) {
            v = 10;
        } else if (Pl_Skill_ck(self, 0xA2) == 1) {
            v = 12;
        }
    } else {
        v = 12;
    }
    pl_act_set_gauge_arm(self, v);
}

/* 0x80277EC0: hands the actor's vertical speed to its motion frame and caches the result. */
extern "C" void fn_80277EC0(_PLW* self)
{
    if (self->unk3A2 > 0) {
        fn_800E1640(((u8*)self->physics_0x13C) + 4, lbl_8079A084);
    } else if (self->unk3A3 > 0) {
        fn_800E1640(((u8*)self->physics_0x13C) + 4, lbl_8079A0C8 * self->unk354);
    } else {
        fn_800E1640(((u8*)self->physics_0x13C) + 4, self->unk354);
    }
    self->unk358 = *(f32*)(((u8*)self->physics_0x13C) + 0x60);
}

/* 0x80277F54: whether the actor's current motion is still free to be interrupted. */
extern "C" s32 fn_80277F54(_PLW* self)
{
    if (self->flag_0x30E >= 2) {
        return 0;
    }
    s32 kind = self->kind_0x015;
    if (kind != 9) {
        u8* p = (u8*)get_move_work_adrs(0);
        if (p != 0 && self->area_0x16 == p[0xF6]) {
            return 0;
        }
    } else {
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802782B8 */
extern "C" s32 fn_802782B8(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278144(self->area_0x16, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278450 */
extern "C" s32 fn_80278450(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278310(self->area_0x16, (u8*)self + 60, self->unk5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278994: clears the actor's stored per-motion scratch values. */
extern "C" void fn_80278994(_PLW* self)
{
    self->unk39E = 0;
    self->unk38A = 0;
    self->unk3EC = 0;
    self->unk3F8 = 0;
    self->unk3FE = 0;
    self->unk3F2 = 0;
    self->unk40E = 0;
    self->unk414 = 0;
    self->unk41A = 0;
    self->unk420 = 0;
    self->unk38C = 0;
    self->unk424 = 0;
    self->unk38E = 0;
    self->unk426 = 0;
    self->unk390 = 0;
    self->unk428 = 0;
    self->unk392 = 0;
    self->unk42A = 0;
    self->unk394 = 0;
    self->unk42C = 0;
}

/* 0x80278B58: starts the actor's dodge/step motion. */
extern "C" void fn_80278B58(_PLW* self, s32 arg1, s32 arg2)
{
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->health = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    pl_act_enter(self, 8, (u16)arg1, (u16)(arg2 | 32));
}

/* 0x80278BE4 */
extern "C" void fn_80278BE4(_PLW* self)
{
    if (self->field_0x00A == 8) {
        return;
    }
    fn_8026FEF0(self, 4);
    fn_8026FEF0(self, 16);
    self->health = 0;
    self->unk376 = 0;
    fn_802789EC(self, 1);
    fn_8027A17C(self);
    if (self->kind_0x09 == 3) {
        fn_80278B58(self, 1, 0);
    } else {
        fn_80278B58(self, 0, 0);
    }
}

/* 0x80278C7C */
extern "C" u32 fn_80278C7C(_PLW* self)
{
    if (self->field_0x416 > 0) {
        return 1;
    }
    if (self->field_0x00A == 6) {
        s32 id = self->act_no;
        if ((u32)(id - 46) <= 5 || (u32)(id - 43) <= 1 || id == 73) {
            return 1;
        }
    }
    return 0;
}

/* 0x80278CD0 */
extern "C" u32 fn_80278CD0(_PLW* self)
{
    if (self->field_0x41C > 0) {
        return 1;
    }
    if (self->field_0x00A == 6) {
        s32 id = self->act_no;
        if ((u32)(id - 66) <= 6 || (u32)(id - 63) <= 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790FC */
extern "C" s32 fn_802790FC(_PLW* self)
{
    if (self->unk46C == 1 && self->unk46C != self->unk46D && self->unk46E == 0) {
        self->unk46E = 1;
        if (self->unk470 == 0) {
            self->unk470 = 1800;
            return 1;
        }
        return 0;
    }
    return 0;
}

/* 0x80279194 */
extern "C" s32 fn_80279194(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        self->unk422 = 0;
        if (self->unk44C < arg2) {
            self->unk44C = arg2;
        }
    } else {
        if (self->unk422 > 0) {
            self->unk422 = 0;
            return 0;
        }
        if (self->unk44D < arg2) {
            self->unk44D = arg2;
        }
    }
    return 1;
}

/* 0x8027A08C: applies a charge delta to the actor's aim and converts the counter to an angle. */
extern "C" void fn_8027A08C(_PLW* self, s8 arg1)
{
    f32 v;
    self->shell_ang_s_0x583 += arg1;
    if (self->shell_ang_s_0x583 > 100) {
        self->shell_ang_s_0x583 = 100;
    }
    if (self->shell_ang_s_0x583 < -100) {
        self->shell_ang_s_0x583 = -100;
    }
    v = (f32)self->shell_ang_s_0x583 / lbl_8079A0D0;
    if (self->shell_ang_s_0x583 >= 0) {
        v = lbl_8079A0E0 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    } else {
        v = lbl_8079A0E8 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    }
}

/* 0x8027BE2C */
extern "C" void pl_act_clear_mode5c4(_PLW* self)
{
    if ((self->unk5C4 & 0xF) != 0) {
        self->unk5C4 &= 0xF0;
        fn_8010D688(self);
    }
}

/* 0x8027AC18 */
extern "C" s32 fn_8027AC18(_PLW* self)
{
    return self->unk5BB != 0;
}

/* 0x8027BDD4 */
extern "C" s32 fn_8027BDD4(_PLW* self)
{
    if (Pl_Skill_ck(self, 93) == 1) {
        return 1;
    }
    return (u16)pl_carry_item_get(self) == 395;
}

/* 0x8027CBC8: the highest carve-slot value the actor has available. */
extern "C" u8 fn_8027CBC8(_PLW* self)
{
    u8 v = 0;
    if (self->unk489 != 0 && self->unk492 != 255) {
        if (v < self->unk4DC) {
            v = self->unk4DC;
        }
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255) {
        if (v < self->unk538) {
            v = self->unk538;
        }
    }
    return v;
}

/* 0x8027D530 */
extern "C" s32 fn_8027D530(_PLW* self)
{
    if (Pl_act_ck(self, 0, 64) == 1 && self->unk306 == 206) {
        return 1;
    }
    return 0;
}

/* 0x8027A4EC */
extern "C" s32 fn_8027A4EC(_PLW* self)
{
    s32 v = 0;
    if (self->field_0x00A == 4) {
        s32 id = self->act_no;
        if ((u32)(id - 19) <= 30 || (u32)(id - 5) <= 8) {
            v = 1;
        }
    }
    if (fn_8028732C(self) == 1) {
        v = 1;
    }
    return v;
}

/* 0x80279414: looks the given id up in the actor's two reaction tables. */
extern "C" u8* fn_80279414(_PLW* self, u8 arg1)
{
    u8* p = fn_80279360(fn_8027ED18((u8*)self + 464), arg1);
    if (p != 0) {
        return p;
    }
    u8 n = ((u8*)&self->equipC)[0];
    if (n == 12) {
        u8* q = fn_80279360(fn_8027ED18((&self->equipC)), arg1);
        if (q != 0) {
            return q;
        }
    }
    return 0;
}

/* 0x80279360: finds the reaction-table entry the given motion id maps to. */
extern "C" u8* fn_80279360(u8* self, u8 arg1)
{
    for (s32 i = 0; i < 4; i++) {
        u8 idx = self[0x18 + i];
        if (idx == 0) {
            break;
        }
        u8* p = lbl_805BF5E0 + idx * 4;
        if (p[0] == arg1) {
            return p;
        }
    }
    return 0;
}

/* 0x80279B84: refreshes the actor's held-item state from its item id. */
extern "C" void fn_80279B84(_PLW* self)
{
    if (fn_8026FE44(self) != 0 && self->unk26E != 255) {
        (void)GetItemData((u16)fn_80272C80(self, (u8)self->unk26E));
        self->held_item_kind_0x26C = fn_80274B20((u16)fn_80272C80(self, (u8)self->unk26E));
        self->unk270 = fn_80272CC8(self, (u8)self->unk26E);
        self->unk26A = fn_80274D98(self, self->held_item_kind_0x26C);
        self->unk269 = 0;
    }
}

/* 0x8027A2A0: whether the actor may still act - master/rage state, the bari timer, the stun flag
 * and the held-item lock all have to agree. */
extern "C" s32 fn_8027A2A0(_PLW* self, s32 arg1)
{
    s32 v = 1;
    if (Pl_master_ck(self) == 0) {
        v = 0;
    }
    if (Pl_bari_ck(self, 1) == 0) {
        v = 0;
    }
    if (self->unk5E6 != 0) {
        v = 0;
    }
    if (pl_part_flag_ck(self, 55) == 0 && (u8)arg1 == 0) {
        v = 0;
    }
    return v;
}

/* 0x8027BC48: whether the current motion still takes directional input. */
extern "C" s32 Pl_motion_input_ck(s32 arg1)
{
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return 1;
    }
    s32 x = p[0xFA];
    /* `break` = "this motion still takes directional input"; the loop is the shared `return 1` the
     * retail source reached through a label (the other half, the shared `return 0`, is inlined). */
    do {
        if ((u32)(x - 6) <= 2) {
            break;
        }
        switch (x) {
        case 3:
            if (arg1 != 0) {
                return 0;
            }
            break;
        case 5:
            if (arg1 == 2) {
                return 0;
            }
            break;
        case 4:
            break;
        default:
            return 0;
        }
    } while (0);
    return 1;
}

/* 0x8027CFC0: ticks down the actor's stun timer and clears the stun condition at zero. */
extern "C" void fn_8027CFC0(_PLW* self)
{
    if (Pl_master_ck(self) != 0 && self->unk404 > 0) {
        if (Pl_dm_condition_ck(self, 2) == 1) {
            self->unk404 = (s16)(self->unk404 - 90);
        } else {
            self->unk404 = (s16)(self->unk404 - 120);
        }
        if (self->unk404 <= 0) {
            self->unk404 = 0;
            self->unk3DC &= 0xFFFFFFFC;
        }
    }
}

/* 0x8027D050: the actor's stored carve value, scanned out of the item table. */
extern "C" u8 fn_8027D050(_PLW* self)
{
    u8 v = 0;
    if (self->unk369 == 0) {
        return 0;
    }
    if ((self->unk364 & 0xE0000007) != 0) {
        for (s32 i = 0; i < 10; i++) {
            if (pl_move_work[self->chunk_ofs][i].carve_0x14 != 0) {
                v = pl_move_work[self->chunk_ofs][i].carve_0x14;
                break;
            }
        }
    }
    return v;
}

/* 0x8027D5A4: applies a signed charge delta to the actor's stored charge, scaled by the two
 * charge skills, and clamps it to 0..100. */
extern "C" void pl_act_add_charge(_PLW* self, s32 arg1)
{
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->field_0x36C += (s16)f;
    if (self->field_0x36C < 0) {
        self->field_0x36C = 0;
    } else if (self->field_0x36C > 100) {
        self->field_0x36C = 100;
    }
}
} /* namespace s_80276B58 */


/* 0x8027CDF0: the gunner's world-space origin, biased by the charge counter. */
void Pl_get_gunner_vec(_PLW* self, _CP_VECTOR* out)
{
    using s_80276B58::lbl_8079A088; using s_80276B58::lbl_8079A0D0; using s_80276B58::lbl_8079A0DC; using s_80276B58::lbl_8079A0E4; using s_80276B58::lbl_8079A110;
    u32 x = self->param_0x54;
    f32 t = (f32)self->gauge_0x64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    out->x = x + (u16)t;
    out->y = self->field_0x058;
    out->z = 0;
}
namespace s_80276B58 {


/* 0x8027CE74: whether the actor's current motion is one of the gunner charge motions. */
extern "C" s32 fn_8027CE74(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
    switch ((u16)Get_motion_no(self)) {
    case 1104:
    case 1114:
    case 1115:
    case 1130:
    case 1131:
    case 1132:
    case 1153:
    case 1163:
    case 1164:
    case 1180:
    case 1181:
    case 1182:
        return 1;
    }
    }
    return 0;
}

/* 0x8027CA48: applies a charge delta to the weapon's charge timer. */
extern "C" void fn_8027CA48(_PLW* self, s16 arg1)
{
    if (self->field_0x002 != 7) {
        return;
    }
    self->unk386 += arg1;
    if (arg1 >= 0) {
        self->unk46A = 1800;
        if (self->unk386 >= 100) {
            self->unk386 = 100;
        }
        switch ((u8)fn_802748C8(self)) {
        case 1:
            fn_8027C89C(self, 900);
            break;
        case 2:
            fn_8027C89C(self, 900);
            break;
        case 3:
            fn_8027C89C(self, 900);
            break;
        }
    } else {
        if (self->unk386 < 0) {
            self->unk386 = 0;
        }
    }
}

/* 0x8027CB1C: the carve slot to select, sentinel 255 meaning "none". */
extern "C" u8 fn_8027CB1C(_PLW* self)
{
    u8 v = 255;
    if (self->unk489 != 0 && self->unk492 != 255 && (v == 255 || v < self->unk492)) {
        v = self->unk492;
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255 && (v == 255 || v < self->unk4EE)) {
        v = self->unk4EE;
    }
    if (Pl_Skill_ck(self, 24) == 1 && v == 0) {
        v = 2;
    }
    return v;
}

/* 0x8027D40C: the number of set bits in the actor's action-lock word. */
extern "C" s32 fn_8027D40C(_PLW* self)
{
    s32 n = 0;
    for (s32 i = 0; i < 32; i++) {
        if (self->unk3AC & (1 << i)) {
            n++;
        }
    }
    return n;
}

/* 0x8027CD0C: builds the gunner's aim matrix from its rotation and gun position. */
extern "C" void fn_8027CD0C(_PLW* self, nw4r::math::MTX34* mtx)
{
    _CP_VECTOR pos;
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    eft_rot_vec_copy(&pos, (u8*)self + 84);
    f32 t = (f32)self->gauge_0x64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    pos.x = pos.x + (u16)t;
    cpSetRotMatrixZXY(&pos, mtx);
    Pl_get_gunner_pos(self, &v, 0);
    mtx->m[0][3] = v.x;
    mtx->m[1][3] = v.y;
    mtx->m[2][3] = v.z;
}
} /* namespace s_80276B58 */


/* 0x8027CC44: the gunner's gun position in actor-local space, biased by the charge counter. */
void Pl_get_gunner_pos(_PLW* self, nw4r::math::VEC3* out, s32 arg2)
{
    using s_80276B58::lbl_8079A0CC; using s_80276B58::lbl_8079A100; using s_80276B58::lbl_8079A104; using s_80276B58::lbl_8079A108; using s_80276B58::lbl_8079A10C;
    if (self->kind_0x09 == 3) {
        out->x = lbl_8079A100;
        out->y = lbl_8079A104;
        out->z = lbl_8079A108;
    } else {
        out->x = lbl_8079A100;
        out->y = lbl_8079A10C;
        out->z = lbl_8079A108;
    }
    if (arg2 != 0) {
        out->z = out->z + lbl_8079A0CC;
    }
    rotVecXYZ(out, (_CP_VECTOR*)((u8*)self + 84));
    out->x = out->x + self->motion_pos_0x3C;
    out->y = out->y + self->motion_pos_0x40;
    out->z = out->z + self->motion_pos_0x44;
}
namespace s_80276B58 {


/* 0x8027AE28: sets the actor's residual-velocity timer from a target position over the given
 * number of frames. */
extern "C" void pl_pos_blend_start(_PLW* self, s32 arg1)
{
    if (fn_80050EF4((u8*)self + 60, (u8*)self + 144) >= lbl_8079A0F4 || (s16)arg1 == 0) {
        self->unk0B4 = 0;
        self->motion_pos_0x3C = self->target_pos_0x090.x;
        self->motion_pos_0x40 = self->target_pos_0x090.y;
        self->motion_pos_0x44 = self->target_pos_0x090.z;
        self->unk09C = lbl_8079A084;
        self->unk0A0 = lbl_8079A084;
        self->unk0A4 = lbl_8079A084;
    } else {
        self->unk0B4 = arg1;
        self->unk09C = (self->target_pos_0x090.x - self->motion_pos_0x3C) / (f32)(s16)arg1;
        self->unk0A0 = (self->target_pos_0x090.y - self->motion_pos_0x40) / (f32)(s16)arg1;
        self->unk0A4 = (self->target_pos_0x090.z - self->motion_pos_0x44) / (f32)(s16)arg1;
    }
}

/* 0x8027C8B4: applies a signed charge delta to the actor's attack-range counter, with the two
 * charge skills scaling a positive delta. */
extern "C" void fn_8027C8B4(_PLW* self, s32 arg1)
{
    if (self->field_0x002 != 7) {
        return;
    }
    if ((s16)arg1 < 0 && self->unk468 > 0) {
        return;
    }
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->field_0x384 += (s16)f;
    if (f >= lbl_8079A084) {
        if (self->field_0x384 >= 100) {
            self->field_0x384 = 100;
            if ((u32)Pl_master_ck(self) == 1 && self->unk468 == 0) {
                ef_inst_spawn(self, 1);
            }
            fn_8027C89C(self, 900);
        }
    } else {
        if (self->field_0x384 < 0) {
            self->field_0x384 = 0;
        }
    }
}
} /* namespace s_80276B58 */


/* 0x8027A2A0 sibling: whether the actor's bari (rage) state covers the given action class. */
u32 Pl_bari_ck(_PLW* self, s32 arg1)
{
    using s_80276B58::lbl_8079A0EC;
    s32 v = 0;
    if (self->field_0x00A == 0) {
        s32 id = self->act_no;
        if ((u32)(id - 169) <= 1 || (u32)(id - 172) <= 1) {
            if (arg1 != 2) {
                v = 1;
            } else {
                s32 m = (u16)Get_motion_no(self);
                if ((m == 314 || m == 365)
                    && Pl_frame_check(self, 1, lbl_8079A0EC, lbl_8079A084) == 1) {
                    v = 1;
                }
            }
        } else if ((id == 171 || id == 174) && (u32)(arg1 - 1) <= 1) {
            v = 1;
        }
    }
    return v;
}
namespace s_80276B58 {


/* 0x802784B8 */
extern "C" s32 fn_802784B8(_PLW* self)
{
    s32 m = (u8)stage_map_kind_get((u8)get_now_mapno());
    switch (m) {
    case 6:
    case 17:
        if (self->area_0x16 == 2) {
            return 0;
        }
        /* falls through */
    default: {
        u8* p = (u8*)get_move_work_adrs(0);
        if (p != 0 && self->area_0x16 == p[0xF6]) {
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688((u8*)self + 60) == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278590: the actor's attack-range tier for the current weapon class. */
extern "C" s32 fn_80278590(_PLW* self)
{
    if (fn_8026FE44(self) == 1) {
        return 0;
    }
    s16* t = (s16*)(lbl_805BFFA8[self->field_0x002]
                    + ((*((u8*)self + 0x56A) * 7) << 1));
    s16 v = *((s16*)self + 0x2B6);
    if (v <= t[0]) {
        return 0;
    }
    if (v <= t[1]) {
        return 1;
    }
    if (v <= t[2]) {
        return 2;
    }
    if (v <= t[3]) {
        return 3;
    }
    if (v <= t[4]) {
        return 4;
    }
    return v <= t[5] ? 5 : 6;
}

/* 0x802789EC: resets the actor's whole action state and re-applies the armour skill values. */
extern "C" void fn_802789EC(_PLW* self, s32 arg1)
{
    fn_80278994(self);
    *(u32*)((u8*)self + 988) = 0;
    *(s16*)((u8*)self + 1042) = 0;
    *(s16*)((u8*)self + 1002) = 0;
    *(s16*)((u8*)self + 1006) = 0;
    *(s16*)((u8*)self + 1024) = 0;
    *(s16*)((u8*)self + 1020) = 0;
    *(s16*)((u8*)self + 1008) = 0;
    *(s16*)((u8*)self + 1012) = 0;
    *(s16*)((u8*)self + 1014) = 0;
    *(s16*)((u8*)self + 1018) = 0;
    *(s16*)((u8*)self + 1026) = 0;
    *(s16*)((u8*)self + 900) = 0;
    *(s16*)((u8*)self + 902) = 0;
    *(s16*)((u8*)self + 1040) = 0;
    *(s16*)((u8*)self + 1048) = 0;
    *(s16*)((u8*)self + 1054) = 0;
    *(s16*)((u8*)self + 1046) = 0;
    *(s16*)((u8*)self + 1052) = 0;
    *(s16*)((u8*)self + 1118) = 0;
    *(s16*)((u8*)self + 1120) = 0;
    *(s16*)((u8*)self + 1122) = 0;
    *(s16*)((u8*)self + 1124) = 0;
    *(s16*)((u8*)self + 1126) = 0;
    *(u8*)((u8*)self + 1096) = 0;
    *(u8*)((u8*)self + 1100) = 0;
    *(u8*)((u8*)self + 1097) = 0;
    *(u8*)((u8*)self + 1101) = 0;
    *(s16*)((u8*)self + 1098) = 0;
    *(s16*)((u8*)self + 1102) = 0;
    *(s16*)((u8*)self + 1028) = 0;
    *(s16*)((u8*)self + 1030) = 0;
    *(s16*)((u8*)self + 1032) = 0;
    *(s16*)((u8*)self + 1034) = 0;
    *(s16*)((u8*)self + 1036) = 0;
    *(s16*)((u8*)self + 1108) = 0;
    *(u8*)((u8*)self + 1104) = 0;
    *(s16*)((u8*)self + 1112) = 0;
    *(u8*)((u8*)self + 1106) = 0;
    *(s16*)((u8*)self + 1128) = 0;
    *(s16*)((u8*)self + 1130) = 0;
    *(s16*)((u8*)self + 1070) = 0;
    *(s16*)((u8*)self + 1080) = 0;
    *(s16*)((u8*)self + 1072) = 0;
    *(s16*)((u8*)self + 1082) = 0;
    *(s16*)((u8*)self + 1074) = 0;
    *(s16*)((u8*)self + 1084) = 0;
    *(s16*)((u8*)self + 1076) = 0;
    *(s16*)((u8*)self + 1086) = 0;
    *(s16*)((u8*)self + 1078) = 0;
    *(s16*)((u8*)self + 1088) = 0;
    *(s16*)((u8*)self + 1058) = 0;
    *(s16*)((u8*)self + 1114) = 0;
    *(s16*)((u8*)self + 1116) = 0;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 39) == 1)) {
        self->unk448 = (s8)fn_802753E4(self, 4);
        self->unk44C = (s8)fn_802753E4(self, 5);
    }
    fn_80278D1C(self);
}

/* 0x8027BCE0: the highest of the actor's 24 stored item ids that is in the carve set. */
extern "C" u32 pl_carry_item_get(_PLW* self)
{
    s32 id;
    u16 v = 0xFFFF;
    for (s32 i = 0; i < 24; i++) {
        if (self->slot_id[i].value > 0) {
            id = self->slot_id[i].item_id;
            if ((u32)(id - 381) <= 1 || id == 139 || id == 395) {
                v = (u16)id;
            }
        }
    }
    return v;
}
} /* namespace s_80276B58 */


/* 0x80279670: the shell multiplier adjustment the two shell tables give. */
f32 Get_Shell_rate_adj(_PLW* self, u8 arg1)
{
    using s_80276B58::fn_8027ED18; using s_80276B58::lbl_8079A0D0;
    f32 v = (f32)*(s16*)(fn_8027ED18((u8*)self + 464) + 10);
    if (((u8*)&self->equipC)[0] == 12) {
        v = v * (f32)*(s16*)(fn_8027ED18((&self->equipC)) + 10) / lbl_8079A0D0;
    }
    return v / lbl_8079A0D0;
}
namespace s_80276B58 {


/* 0x80277C94: ground-height probe along the actor's facing, returning whether the probe hit
 * inside the requested band. */
extern "C" s32 fn_80277C94(_PLW* self, nw4r::math::VEC3* out, f32 arg2, f32 arg3)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    out->x = lbl_8079A084;
    v.x = lbl_8079A084;
    v.y = arg2 + arg3;
    v.z = lbl_8079A0C4;
    rotVecY(&v, self->field_0x058);
    v.x = v.x + self->motion_pos_0x3C;
    v.y = v.y + self->motion_pos_0x40;
    v.z = v.z + self->motion_pos_0x44;
    f32 h = GetGroundHit2(&v, -0x1006, self->area_0x16, &hit);
    f32 c = self->motion_pos_0x40 + arg2;
    if (h >= c && h <= arg3 + c && hit != 0) {
        out->x = h;
        return 1;
    }
    return 0;
}

/* 0x8027AF88: refreshes the actor's shell/clutch state from its move work. */
extern "C" void fn_8027AF88(_PLW* self)
{
    *(s16*)((u8*)self + 1474) = 30;
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (isServerSelectState(fn_802E5CFC(*(s8*)(q + 1505))) == 1) {
        *(s16*)((u8*)self + 1626) = 900;
        *((u8*)self + 1625) = 1;
        lb_entry_handover_send(1, (u8)my_player_no(), *(u8*)(q + 1505));
        return;
    }
    *(s16*)((u8*)self + 1626) = 0;
    *((u8*)self + 1625) = 0;
    pl_item_add(self, *(u16*)(q + 1506 + *(s8*)(q + 1505) * 4),
                *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4));
    *(u32*)(q + 1668 + (*(s8*)(q + 1505) >> 5) * 4) |= 1 << (*(s8*)(q + 1505) & 31);
    fn_802E5D68(*(u16*)(q + 1506 + *(s8*)(q + 1505) * 4));
    *(s16*)(q + 1506 + *(s8*)(q + 1505) * 4) = 0;
    *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4) = 0;
}

/* 0x80278D1C: rebuilds the actor's two condition word sets from its stored state, its current action id
 * and the two related-player flags the small helpers report. */
extern "C" void fn_80278D1C(_PLW* self)
{
    s32 bits = 0;
    if (Pl_master_ck(self) != 0) {
        if (*(s16*)((u8*)self + 1002) != 0) {
            bits |= 1;
        }
        if (self->unk448 + self->unk449 > 0) {
            bits |= 0x10;
        }
        if (*(s16*)((u8*)self + 900) >= 100) {
            bits |= 0x80000000;
        }
        if (self->unk44C + self->unk44D > 0) {
            bits |= 0x40;
        }
        if (self->field_0x00A == 6) {
            s32 id = self->act_no;
            if ((u32)(id - 0x1F) > 3) {
                if ((u32)(id - 0x1C) > 2) {
                    if (id != 0x1B && id != 0x23) {
                    } else {
                        bits |= 8;
                    }
                } else {
                    bits |= 2;
                }
            } else {
                bits |= 4;
            }
        }
        if (*(s16*)((u8*)self + 1124) > 0) {
            bits |= 0x400;
        }
        if (*(s16*)((u8*)self + 1126) > 0) {
            bits |= 0x800;
        }
        if (*(s16*)((u8*)self + 1118) > 0) {
            bits |= 0x4000;
        }
        if (fn_80278C7C(self) == 1) {
            bits |= 0x100;
        }
        if (fn_80278CD0(self) == 1) {
            bits |= 0x200;
        }
        if (*(s16*)((u8*)self + 1058) > 0) {
            bits |= 0x80;
        }
        if (*(s16*)((u8*)self + 1040) > 0) {
            bits |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1112) > 0) {
            bits |= 0x10000;
        }
        if (*(u32*)((u8*)self + 944) != 0) {
            bits |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1108) > 0) {
            bits |= 0x2000;
        }
        self->unk3D8 = bits;
        self->unk3DC &= 0xFFF003FF;
        if (*(s16*)((u8*)self + 1070) > 0 || (s16)fn_802753E4(self, 6) < 0) {
            self->unk3DC |= 0x400;
        }
        if (*(s16*)((u8*)self + 1072) > 0 || (s16)fn_802753E4(self, 7) < 0) {
            self->unk3DC |= 0x800;
        }
        if (*(s16*)((u8*)self + 1074) > 0 || (s16)fn_802753E4(self, 8) < 0) {
            self->unk3DC |= 0x1000;
        }
        if (*(s16*)((u8*)self + 1076) > 0 || (s16)fn_802753E4(self, 9) < 0 || fn_80278C7C(self) == 1) {
            self->unk3DC |= 0x2000;
        }
        if (*(s16*)((u8*)self + 1078) > 0 || (s16)fn_802753E4(self, 10) < 0) {
            self->unk3DC |= 0x4000;
        }
        if (*(s16*)((u8*)self + 1080) > 0 || (s16)fn_802753E4(self, 6) > 0) {
            self->unk3DC |= 0x8000;
        }
        if (*(s16*)((u8*)self + 1082) > 0 || (s16)fn_802753E4(self, 7) > 0) {
            self->unk3DC |= 0x10000;
        }
        if (*(s16*)((u8*)self + 1084) > 0 || (s16)fn_802753E4(self, 8) > 0) {
            self->unk3DC |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1086) > 0 || (s16)fn_802753E4(self, 9) > 0) {
            self->unk3DC |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1088) > 0 || (s16)fn_802753E4(self, 10) > 0) {
            self->unk3DC |= 0x80000;
        }
    }
}

extern "C" s32 fn_8026A6F4(_PLW*, s32);
} /* namespace s_80276B58 */

void sysSE_req(s32);
namespace s_80276B58 {

/* 0x8027B918: steps the actor's clutch state machine from the pad edge events the move work reports. */
extern "C" void fn_8027B918(_PLW* self)
{
    u8* p;
    u8* q;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BD) == 0) {
        return;
    }
    p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        sysSE_req(5);
        *(s16*)((u8*)self + 0x5C2) = 30;
        (*(u8*)((u8*)self + 0x5C0))++;
        return;
    case 1: {
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            (*(s16*)((u8*)self + 0x5C2))--;
        }
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            break;
        }
        if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5BD) = 0;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            switch (*(u8*)((u8*)self + 0x5BF)) {
            case 0:
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5BD) = 1;
                return;
            case 1:
                if ((u32)Pl_motion_input_ck(0) != 1) {
                    if ((u32)quest_flag_80_ck(NULL) == 1) {
                        if (quest_item_work_notify(1) == 1 || quest_item_work_notify(2) == 1) {
                            (*(u8*)((u8*)self + 0x5C0))++;
                            *(u8*)(q + 0x68D) = 1;
                            sysSE_req(0);
                            return;
                        }
                        sysSE_req(2);
                        return;
                    }
                    sysSE_req(2);
                    return;
                }
                sysSE_req(2);
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x12) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) == 0) {
                    *(u8*)((u8*)self + 0x5BF) = 1;
                } else {
                    (*(u8*)((u8*)self + 0x5BF))--;
                }
            }
            if ((u32)fn_8026A6F4(self, 0x13) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) >= 1) {
                    *(u8*)((u8*)self + 0x5BF) = 0;
                    return;
                }
                (*(u8*)((u8*)self + 0x5BF))++;
                return;
            }
        }
        break;
    }
    case 2:
        if (*(s8*)(q + 0x68D) == 0) {
            if ((u32)fn_8026A6F4(self, 0x17) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                if ((s32)*(u8*)((u8*)self + 0x5BF) == 1 && (u32)Pl_motion_input_ck(0) != 1) {
                    sysSE_req(0);
                    *(u8*)((u8*)self + 0x5BD) = 1;
                    return;
                }
            } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(3);
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x16) == 1 || (u32)fn_8026A6F4(self, 0x17) == 1) {
                *(u8*)((u8*)self + 0x5C0) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x14) == 1) {
                *(u8*)(q + 0x68D) = 0;
                sysSE_req(3);
            }
        }
        break;
    }
}

extern "C" s32 Pl_item_timer_get(_PLW*, u16);

/* 0x8027B358: steps the shell-selection state machine - the clutch index the move work stores plus the
 * three charge levels - from the pad edge events. */
extern "C" void fn_8027B358(_PLW* self)
{
    u16 sp8;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BE) == 0) {
        return;
    }
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if ((u32)Pl_motion_input_ck(0) == 1) {
        sysSE_req(1);
        *(u8*)((u8*)self + 0x5BE) = 0;
        return;
    }
    em_work_slot_pair_get((u16)*(s8*)(q + 0x68C), (s16*)&sp8);
    s32 r28 = quest_element_remaining_get((u16)*(s8*)(q + 0x68C));
    *(u16*)(q + 0x696) = 0;
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        *(u8*)(q + 0x68E) = 1;
        *(u8*)((u8*)self + 0x5C0) = 1;
        return;
    case 1:
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            (*(u8*)(q + 0x68C))++;
            if (*(s8*)(q + 0x68C) >= 3) {
                *(u8*)(q + 0x68C) = 2;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            (*(u8*)(q + 0x68C))--;
            if (*(s8*)(q + 0x68C) < 0) {
                *(u8*)(q + 0x68C) = 0;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            switch (quest_element_supply_state_get(*(s8*)(q + 0x68C))) {
            case 0:
                sysSE_req(2);
                return;
            case 1:
                sysSE_req(2);
                return;
            case 2:
                if (Pl_item_timer_get(self, sp8) < 1) {
                    sysSE_req(2);
                    return;
                }
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68E) = 1;
                sysSE_req(0);
                return;
            }
        } else if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            *(u8*)((u8*)self + 0x5BE) = 0;
            return;
        }
        break;
    case 2:
        if (quest_element_supply_state_get(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            sysSE_req(6);
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68E) > 1) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 2;
                (*(u8*)(q + 0x68E))--;
                return;
            }
            *(u8*)(q + 0x68E) = 1;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            u8 t31 = *(u8*)(q + 0x68E);
            if ((s8)t31 < Pl_item_timer_get(self, sp8)) {
                if ((s8)t31 < r28) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 1;
                    (*(u8*)(q + 0x68E))++;
                    return;
                }
                *(s8*)(q + 0x68E) = r28;
                return;
            }
        } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
            u8 t31b = *(u8*)(q + 0x68E);
            if (r28 < Pl_item_timer_get(self, sp8)) {
                *(s8*)(q + 0x68E) = r28;
            } else {
                *(s8*)(q + 0x68E) = Pl_item_timer_get(self, sp8);
            }
            if ((s8)t31b != *(s8*)(q + 0x68E)) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 1;
                return;
            }
        } else {
            if ((u32)fn_8026A6F4(self, 0x14) == 1) {
                if (*(s8*)(q + 0x68E) != 1) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 2;
                }
                *(u8*)(q + 0x68E) = 1;
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5C0) = 3;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            if ((u32)fn_8026A6F4(self, 0x17) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 1;
                return;
            }
        }
        break;
    case 3:
        if (quest_element_supply_state_get(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68D) < 1) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))++;
                return;
            }
            *(u8*)(q + 0x68D) = 1;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x12) == 1) {
            if (*(s8*)(q + 0x68D) > 0) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))--;
                return;
            }
            *(u8*)(q + 0x68D) = 0;
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x16) == 1) {
            if (*(s8*)(q + 0x68D) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            sysSE_req(8);
            *(u8*)((u8*)self + 0x5C0) = 1;
            quest_element_item_use(self, sp8, *(s8*)(q + 0x68E));
            return;
        }
        if ((u32)fn_8026A6F4(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 2;
            return;
        }
        break;
    default:
        *(u8*)((u8*)self + 0x5BE) = 0;
        break;
    }
}

extern "C" s32 pl_item_room_get(_PLW*, u16, s32);

/* 0x8027B0BC: steps the handling-direction state machine - an 8-way direction on a five-row grid of
 * stored action entries - from the pad edge events. */
extern "C" void fn_8027B0BC(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s32)*(u8*)((u8*)self + 0x5BC) == 0) {
        return;
    }
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        (*(s16*)((u8*)self + 0x5C2))--;
    }
    if (*(s16*)((u8*)self + 0x65A) != 0) {
        (*(s16*)((u8*)self + 0x65A))--;
        if (*(s16*)((u8*)self + 0x65A) == 0) {
            *(u8*)((u8*)self + 0x659) = 0;
        }
    }
    if ((s32)*(u8*)((u8*)self + 0x659) != 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        return;
    }
    if ((u32)fn_8026A6F4(self, 0x17) == 1) {
        *(u8*)((u8*)self + 0x5BC) = 0;
        sysSE_req(1);
        return;
    }
    if ((u32)fn_8026A6F4(self, 0x14) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 0) {
            (*(u8*)(q + 0x5E1))--;
        } else {
            (*(u8*)(q + 0x5E1)) += 7;
        }
    } else if ((u32)fn_8026A6F4(self, 0x15) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 7) {
            (*(u8*)(q + 0x5E1))++;
        } else {
            (*(u8*)(q + 0x5E1)) -= 7;
        }
    }
    if ((u32)fn_8026A6F4(self, 0x13) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) += 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w >= 40) {
            (*(u8*)(q + 0x5E1)) -= 40;
        }
    } else if ((u32)fn_8026A6F4(self, 0x12) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) -= 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w < 0) {
            (*(u8*)(q + 0x5E1)) += 40;
        }
    }
    u8 dir = *(u8*)(q + 0x5E1);
    if (((1 << ((s8)dir & 0x1F)) & *(u32*)(q + 0x684 + ((s8)dir >> 5) * 4)) == 0) {
        if ((u32)fn_8027AF80(*(u8*)(q + 0x5E1)) == 1) {
            if ((u32)fn_8026A6F4(self, 0x16) == 1) {
                u16 item = *(u16*)(q + 0x5E2 + (s8)*(u8*)(q + 0x5E1) * 4);
                if ((s32)item != 0) {
                    s16 val = *(s16*)(q + 0x5E4 + (s8)*(u8*)(q + 0x5E1) * 4);
                    if (val > 0) {
                        if (pl_item_room_get(self, item, val) >= val) {
                            sysSE_req(8);
                            fn_8027AF88(self);
                            return;
                        }
                        sysSE_req(2);
                    }
                }
            }
        }
    }
}
} /* namespace s_80276B58 */


/* The two side structures the attack-subsystem setters fill in: `_HIT_W` is the attack entry the
 * caller uses, `_HIT_DATA` the hit record it came from. Only the offsets this unit touches are named. */
struct _HIT_DATA {
    u8 unk00[0x12];
    s8 unk12;
};
namespace s_80276B58 {


extern "C" u8 fn_80331210(_PLW*);
extern "C" s16 fn_80273ED8(_PLW*, s32, s32);
} /* namespace s_80276B58 */

s32 Pl_critical_get(_PLW*);

/* 0x80277284: fills in the attack entry an incoming hit produces - the skill-granted hit flags, the
 * hit rate multipliers and the critical roll - from the attack flags and the actor's skills. */
void Pl_attack_set_sub(_PLW* self, _HIT_DATA* hit, _HIT_W* data, u16 flags)
{
    using s_80276B58::fn_80273ED8; using s_80276B58::fn_802748C8; using s_80276B58::fn_80331210; using s_80276B58::lbl_8079A08C; using s_80276B58::lbl_8079A0A8; using s_80276B58::lbl_8079A0AC; using s_80276B58::lbl_8079A0B0; using s_80276B58::lbl_8079A0B4;
    s32 f8;
    s32 f10;
    data->attack_flags_0x31 = 0xF;
    data->attack_kind_0x32 = 1;
    f10 = flags & 0x10;
    if (f10 != 0 && Pl_Skill_ck(self, 0xC7) == 1) {
        s8 v = hit->unk12;
        if ((s32)v < 0) {
            data->field_0x04E = (s8)-v;
        }
    }
    if ((s8)data->field_0x04E > 0 && Pl_cat_skill_ck(self, 0x28) == 1) {
        data->field_0x04E = (s8)(lbl_8079A0A8 * (f32)(s8)data->field_0x04E);
    }
    if ((s32)(flags & 2) != 0) {
        s32 sel;
        s16 t;
        f8 = flags & 8;
        if (f8 != 0 && fn_80331210(self) == 1) {
            sel = 1;
        } else {
            sel = 0;
            if (Pl_Skill_ck(self, 0xC9) == 1) {
                sel = 1;
            }
        }
        t = fn_80273ED8(self, 7, sel);
        if (t > 0) {
            data->field_0x048 |= 0x10;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 8, sel);
        if (t > 0) {
            data->field_0x048 |= 0x20;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 9, sel);
        if (t > 0) {
            data->field_0x048 |= 0x40;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 0xA, sel);
        if (t > 0) {
            data->field_0x048 |= 0x80;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 0xB, sel);
        if (t > 0) {
            data->field_0x048 |= 0x200;
            data->field_0x04A = (u8)t;
        }
        if (f8 != 0 && fn_80331210(self) == 3) {
            data->field_0x048 = 0x80;
            t = fn_80273ED8(self, 0xF, 1);
            if (t <= 0) {
                data->field_0x04A = 1;
            } else {
                data->field_0x04A = (u8)t;
            }
        }
        if ((s32)(*(u16*)((u8*)self + 0xB6) % 3) == 0) {
            t = fn_80273ED8(self, 0xC, sel);
            if (t > 0) {
                data->field_0x048 |= 2;
                data->field_0x04A = (u8)t;
            }
            t = fn_80273ED8(self, 0xD, sel);
            if (t > 0) {
                data->field_0x048 |= 4;
                data->field_0x04A = (u8)t;
            }
            t = fn_80273ED8(self, 0xE, sel);
            if (t > 0) {
                data->field_0x048 |= 1;
                data->field_0x04A = (u8)t;
            }
            if (f8 != 0 && fn_80331210(self) == 2) {
                data->field_0x048 = (u16)((data->field_0x048 & 0xFFFC) | 4);
                t = fn_80273ED8(self, 0x10, 1);
                if (t <= 0) {
                    data->field_0x04A = 1;
                } else {
                    data->field_0x04A = (u8)t;
                }
            }
        }
        if ((s32)data->field_0x048 != 0) {
            data->field_0x05A = 0xA;
        }
    }
    if (self->field_0x002 == 7 && (flags & 1) != 0) {
        switch (fn_802748C8(self)) {
        case 1:
            data->life_0x40 = (s16)(lbl_8079A0AC * (f32)data->life_0x40);
            break;
        case 2:
            data->life_0x40 = (s16)(lbl_8079A0A8 * (f32)data->life_0x40);
            break;
        case 3:
            data->life_0x40 = (s16)(lbl_8079A0B0 * (f32)data->life_0x40);
            break;
        }
    }
    if ((flags & 8) != 0) {
        switch ((u8)fn_80331210(self)) {
        case 0:
            data->life_0x40 = (s16)(lbl_8079A0B4 * (f32)data->life_0x40);
            break;
        case 1:
            if ((s32)data->field_0x048 != 0) {
                u8 c = data->field_0x04A;
                if ((s32)c != 0) {
                    data->field_0x04A = (u8)(s32)(lbl_8079A0B4 * (f32)c);
                }
            }
            break;
        }
    }
    if ((flags & 0x20) != 0 && Pl_cat_skill_ck(self, 0x1F) == 1) {
        data->life_0x40 = (s16)(data->life_0x40 * 5);
    }
    if ((flags & 0x40) != 0 && Pl_cat_skill_ck(self, 0x20) == 1) {
        data->life_0x40 = (s16)(data->life_0x40 * 5);
    }
    if ((flags & 0x14) != 0) {
        s32 crit = Pl_critical_get(self);
        s32 sc;
        if (f10 != 0 && Pl_Skill_ck(self, 0xB7) == 1) {
            crit = 100;
        }
        sc = (s16)crit;
        if (sc != 0 && (u32)*(u8*)((u8*)self + 0x56B) >= 1) {
            if (sc > 0) {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < sc) {
                    data->life_0x40 = (s16)(lbl_8079A0B4 * (f32)data->life_0x40);
                    data->field_0x04C |= 0x80;
                }
            } else {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < -sc) {
                    if (data->life_0x40 > 0) {
                        data->life_0x40 = (s16)(lbl_8079A08C * (f32)data->life_0x40);
                        if (data->life_0x40 == 0) {
                            data->life_0x40 = 1;
                        }
                        data->field_0x04C |= 0x40;
                    }
                }
            }
        }
    }
}

/* The move-work records `get_move_work_adrs(2)` hands back: an array of actor-shaped entries on a
 * 0xB20 stride. Only the offsets this unit touches are named. */
struct _MOVE_WORK {
    u8 unk000;
    u8 unk001[0x16 - 0x01];
    u8 unk016;
    u8 unk017[0x3DC - 0x17];
    u32 unk3DC;
    u8 unk3E0[0x3EA - 0x3E0];
    s16 unk3EA;
    u8 unk3EC[0x404 - 0x3EC];
    s16 unk404;
    s16 unk406;
    s16 unk408;
    s16 unk40A;
    s16 unk40C;
    u8 unk40E[0x412 - 0x40E];
    s16 unk412;
    u8 unk414[0x42E - 0x414];
    s16 unk42E;
    s16 unk430;
    s16 unk432;
    s16 unk434;
    s16 unk436;
    s16 unk438;
    s16 unk43A;
    s16 unk43C;
    s16 unk43E;
    s16 unk440;
    u8 unk442[0x44A - 0x442];
    s16 unk44A;
    u8 unk44C[0x44E - 0x44C];
    s16 unk44E;
    u8 unk450[0x464 - 0x450];
    s16 unk464;
    s16 unk466;
};
namespace s_80276B58 {


extern "C" void pl_act_add_hold_gauge(_MOVE_WORK*, s32);
extern "C" void fn_8027D6A4(_MOVE_WORK*, s32, s32);
extern "C" void fn_8027D6C0(_MOVE_WORK*, s32, s32);

/* 0x8027D0D4: applies one scripted action to every move-work record of type 2 whose kind byte matches
 * `arg0` - the per-action clamps and the state-helper calls of the attack/critical chain. */
extern "C" void fn_8027D0D4(u8 arg0, u8 arg1)
{
    s32 i;
    u8* r29;
    s32 max;
    r29 = (u8*)get_move_work_adrs(2);
    max = (u16)get_move_work_max(2);
    for (i = 0; i < max; i++) {
        _MOVE_WORK* p = (_MOVE_WORK*)r29;
        if (p->unk000 != 0 && p->unk016 == arg0) {
            switch (arg1) {
            case 0:
                pl_act_add_hold_gauge(p, 0x32);
                ef_inst_spawn((_PLW*)p, 0);
                break;
            case 1:
                fn_80279154((_PLW*)p, 1, 0xA);
                p->unk44A = 0x1518;
                ef_inst_spawn((_PLW*)p, 1);
                break;
            case 2:
                if ((u32)fn_80279194((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 2);
                break;
            case 3:
            case 6:
                if (arg1 == 3) {
                    pl_act_add_hold_gauge(p, 0x14);
                } else {
                    pl_act_add_hold_gauge(p, 0x32);
                }
                ef_inst_spawn((_PLW*)p, 0);
                break;
            case 4:
            case 7:
                if (arg1 == 4) {
                    fn_80279154((_PLW*)p, 1, 3);
                } else {
                    fn_80279154((_PLW*)p, 1, 5);
                }
                p->unk44A = 0x1518;
                ef_inst_spawn((_PLW*)p, 1);
                break;
            case 5:
            case 8:
                if (arg1 == 5) {
                    if ((u32)fn_80279194((_PLW*)p, 1, 0xA) == 1) {
                        p->unk44E = 0x1518;
                    }
                } else if ((u32)fn_80279194((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 2);
                break;
            case 9:
                p->unk3EA = 0;
                ef_inst_spawn((_PLW*)p, 4);
                break;
            case 10:
                if (p->unk466 < 0x2328) {
                    p->unk466 = 0x2328;
                }
                ef_inst_spawn((_PLW*)p, 7);
                break;
            case 11:
                if (p->unk464 < 0x2328) {
                    p->unk464 = 0x2328;
                }
                ef_inst_spawn((_PLW*)p, 6);
                break;
            case 12:
                if (p->unk412 < 0x1518) {
                    p->unk412 = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 5);
                break;
            case 13:
                fn_8027D6A4(p, 1, 0x1518);
                fn_8027D6C0(p, 1, 0x1518);
                ef_inst_spawn((_PLW*)p, 9);
                break;
            case 14:
                p->unk42E = 0;
                if (p->unk438 < 0x1518) {
                    p->unk438 = 0x1518;
                }
                p->unk430 = 0;
                if (p->unk43A < 0x1518) {
                    p->unk43A = 0x1518;
                }
                p->unk432 = 0;
                if (p->unk43C < 0x1518) {
                    p->unk43C = 0x1518;
                }
                p->unk434 = 0;
                if (p->unk43E < 0x1518) {
                    p->unk43E = 0x1518;
                }
                p->unk436 = 0;
                if (p->unk440 < 0x1518) {
                    p->unk440 = 0x1518;
                }
                i = 5;
                p->unk404 = 0;
                p->unk408 = 0;
                p->unk406 = 0;
                p->unk40A = 0;
                p->unk40C = 0;
                p->unk3DC &= 0xFFFFFC00;
                ef_inst_spawn((_PLW*)p, 9);
                break;
            }
        }
        r29 += 0xB20;
    }
}

/* 0x8027BE4C: maps the actor's five stored status durations onto the condition word and the shell
 * timers it exposes. */
extern "C" void fn_8027BE4C(_PLW* self)
{
    s16 t;
    u32 bits;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (Pl_Skill_ck(self, 0xC8) == 1) {
        return;
    }
    t = self->unk38C;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFFC;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 2;
            self->unk404 = 0x1C2;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 1;
            self->unk404 = 0x1C2;
        }
    }
    t = self->unk38E;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFF3;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 8;
            *(s16*)((u8*)self + 0x406) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 4;
            *(s16*)((u8*)self + 0x406) = 0x384;
        }
    }
    t = self->unk390;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFCF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x20;
            *(s16*)((u8*)self + 0x408) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x10;
            *(s16*)((u8*)self + 0x408) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
    t = self->unk392;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFF3F;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x80;
            *(s16*)((u8*)self + 0x40A) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x40;
            *(s16*)((u8*)self + 0x40A) = 0x384;
        }
    }
    t = self->unk394;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFCFF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x200;
            *(s16*)((u8*)self + 0x40C) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
            return;
        }
        if (t >= 0x1F) {
            self->unk3DC = bits | 0x100;
            *(s16*)((u8*)self + 0x40C) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
}

extern "C" void fn_80276690(_PLW*, s16, s32);
extern "C" void fn_80276778(_PLW*, s16);
extern "C" s16 fn_8026FF20(_PLW*);

/* 0x8027885C: resets the actor's shell/ammo timers and, when the relevant skill is on, reseeds the
 * three shell tables from the actor's current state. */
extern "C" void fn_8027885C(_PLW* self, s32 arg1, s32 arg2)
{
    *(s16*)((u8*)self + 0x37A) = 0x258;
    *(s16*)((u8*)self + 0x372) = 0x64;
    *(s16*)((u8*)self + 0x380) = 0x64;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 0x27) == 1)) {
        fn_80276690(self, (s16)fn_802753E4(self, 1), 0);
        fn_80276CE8(self, (s16)fn_802753E4(self, 2));
        fn_80276778(self, (s16)fn_802753E4(self, 3));
    }
    fn_80276690(self, fn_8026FF20(self), 0);
    *(s16*)((u8*)self + 0x36C) = 0x32;
    *(s16*)((u8*)self + 0x36E) = 0x96;
    if (arg2 == 0) {
        s16 v;
        *(s16*)((u8*)self + 0x378) = *(s16*)((u8*)self + 0x37A);
        *(s16*)((u8*)self + 0x37C) = 0x2A30;
        *(u8*)((u8*)self + 0x39E) = 0;
        *(s16*)((u8*)self + 0x38A) = 0;
        v = *(s16*)((u8*)self + 0x372);
        *(s16*)((u8*)self + 0x370) = v;
        *(s16*)((u8*)self + 0x376) = v;
        *(s16*)((u8*)self + 0x37E) = *(s16*)((u8*)self + 0x380);
    }
}

/* 0x802791FC: the shell-level cap one equipment slot contributes, before the actor's skill modifiers
 * are applied. */
extern "C" s32 fn_802791FC(_PLW* self, u8 arg1)
{
    if ((s32)fn_8026FE44(self) == 0) {
        return 1;
    }
    s32 a = *(u8*)(lbl_805BF538 + 1 + arg1 * 4);
    s32 b = *(u8*)(fn_8027ED18(&self->equipB) + 9);
    if (((u8*)&self->equipD)[0] == 0xD) {
        b += *(u8*)(fn_8027ED18(&self->equipD) + 9);
    }
    if (Pl_Skill_ck(self, 0x27) == 1) {
        b = (s16)(b + 2);
    }
    if (Pl_Skill_ck(self, 0x28) == 1) {
        b = (s16)(b + 3);
    }
    if (Pl_Skill_ck(self, 0x29) == 1) {
        b = (s16)(b + 4);
    }
    if (Pl_Skill_ck(self, 0x2A) == 1) {
        b = (s16)(b - 1);
    }
    if (Pl_Skill_ck(self, 0x2B) == 1) {
        b = (s16)(b - 2);
    }
    if (Pl_Skill_ck(self, 0x2C) == 1) {
        b = (s16)(b - 3);
    }
    a -= b;
    if ((s16)a <= 4) {
        return 0;
    }
    return ((s16)a <= 7) ? 1 : 2;
}

extern "C" { extern u8 lbl_805C6100[]; }
extern "C" { extern u8 lbl_80792150; }
} /* namespace s_80276B58 */


/* 0x80279720: the shell "bure" type (0/1/2) the actor's equipped shell and its skills resolve to. */
u32 Get_Shell_bure_type(_PLW* self, u8 arg1)
{
    using s_80276B58::fn_8027ED18; using s_80276B58::lbl_805C6100; using s_80276B58::lbl_80792150;
    u8* t31 = lbl_805BFCD8 + *(u8*)(fn_8027ED18(&self->equipB) + 0x10) * 2;
    s32 v29 = t31[1];
    v29 += *(s8*)(lbl_805BF538 + 3 + arg1 * 4);
    s32 r29;
    if (((u8*)&self->equipC)[0] == 0xC) {
        v29 -= (&lbl_80792150)[*(u8*)(fn_8027ED18(&self->equipC) + 0x10)];
    }
    if ((s8)v29 <= 5) {
        r29 = 0;
    } else {
        r29 = ((s8)v29 <= 15) ? 1 : 2;
    }
    if (Pl_Skill_ck(self, 0xAC) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            if ((u8)r29 != 0) {
                r29 -= 1;
            }
        } else {
            r29 = 0;
        }
    } else if (Pl_Skill_ck(self, 0xAB) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 0 && (u8)r29 != 0) {
            r29 -= 1;
        }
    } else if (Pl_Skill_ck(self, 0xAD) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            r29 = 2;
        } else if ((u8)r29 < 2) {
            r29 += 1;
        }
    } else if (Pl_cat_skill_ck(self, 0x1E) == 1 && (u8)r29 < 2) {
        r29 = (u8)((u8)r29 + 1);
    }
    u8 t3 = t31[0];
    return lbl_805C6100[(u8)r29 + (t3 * 4 - t3)];
}
namespace s_80276B58 {


/* 0x80279490: the shell capacity one equipment slot contributes after the actor's skill modifiers,
 * mapped onto the 0-3 capacity class the caller uses. */
extern "C" s32 fn_80279490(_PLW* self, u8 arg1)
{
    if ((s32)fn_8026FE44(self) == 0) {
        return 2;
    }
    s32 a = *(u8*)(lbl_805BF538 + arg1 * 4);
    u8* p = fn_80279414(self, arg1);
    if (p != 0) {
        return (s16)(*(u8*)(p + 3) + 3);
    }
    s32 b = 0;
    if (fn_80279414(self, arg1) == 0) {
        b = *(u8*)(fn_8027ED18(&self->equipB) + 7);
        if (((u8*)&self->equipD)[0] == 0xD) {
            b += *(u8*)(fn_8027ED18(&self->equipD) + 7);
        }
        if (Pl_Skill_ck(self, 0x2D) == 1) {
            b += 2;
        } else if (Pl_Skill_ck(self, 0x2E) == 1) {
            b += 3;
        } else if (Pl_Skill_ck(self, 0x2F) == 1) {
            b = (s16)(b + 4);
        }
        if (Pl_Skill_ck(self, 0x30) == 1) {
            b -= 1;
        } else if (Pl_Skill_ck(self, 0x31) == 1) {
            b -= 2;
        } else if (Pl_Skill_ck(self, 0x32) == 1) {
            b = (s16)(b - 3);
        }
    }
    a -= b;
    s32 r3 = 2;
    if ((s16)a <= 0) {
        a = 0;
    }
    if ((s16)a <= 8) {
        r3 = 0;
    } else if ((s16)a <= 0xA) {
        r3 = 1;
    }
    if ((u32)(arg1 - 0x27) <= 2) {
        if (r3 == 0) {
            return 3;
        }
        return (r3 == 1) ? 8 : 9;
    }
    return r3;
}

extern "C" s32 fn_8026A6A4(_PLW*, s32);
extern "C" void fn_80279EBC(_PLW*, u16, u16);
extern "C" s32 fn_8027A340(_PLW*);
extern "C" u32 fn_80287244(_PLW*, s32);
extern "C" void fn_802B9740(_PLW*, u8*, u8*, f32*);
extern "C" u8 get_cfg(u8, s32);
extern "C" { extern u8 lbl_805C6118[]; }
extern "C" { extern const f32 lbl_8079A080; }
extern "C" { extern const f32 lbl_8079A0F0; }

/* 0x8027A57C: resolves one motion's stick input into movement - the four pad-direction flags, the direction and
 * magnitude the move table interpolates for the current speed, and the per-motion write-back. */
extern "C" void fn_8027A57C(_PLW* self, u16 arg1, u8 arg2)
{
    u8 sp9;
    u8 sp8;
    f32 spC;
    s32* t;
    s32 mag, dir, speed, step;
    u8 f27, f26, f25, f24;
    f27 = 0;
    f26 = 0;
    f25 = 0;
    f24 = 0;
    mag = 0;
    dir = 0;
    step = 0;
    speed = *(s32*)((u8*)self + 0xA8);

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 != (u16)Get_motion_no(self)) {
        return;
    }
    if ((s8)self->unk36A != 0) {
        return;
    }
    if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
        self->unk5E5 = 0xA;
    }
    for (;;) {
        if ((s32)self->unk5E5 == 0 && (s32)self->unk5E6 == 0) {
            if ((u32)fn_8026A6F4(self, 0xB) == 1) {
                break;
            }
            if (arg2 == 0) {
                if (!(self->field_0x00A == 4 || fn_8027A340(self) != 0)) {
                    break;
                }
            } else if (arg2 == 2) {
                if (pl_part_flag_ck(self, 0x37) == 0) {
                    break;
                }
            }
        }
        switch (arg2) {
        case 0:
            if (fn_8027A340(self) != 0 ||
                ((s32)self->unk5E6 == 0 && (u32)fn_80287244(self, 0) == 1)) {
                u8 c;
                u8 r;
                if (fn_8026A6A4(self, 0) != 0) {
                    f25 = 1;
                } else if (fn_8026A6A4(self, 1) != 0) {
                    f24 = 1;
                }
                if (fn_8026A6A4(self, 2) != 0) {
                    f27 = 1;
                } else if (fn_8026A6A4(self, 3) != 0) {
                    f26 = 1;
                }
                r = get_cfg(self->chunk_ofs, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x180;
                }
            }
            break;
        case 2:
            if ((s32)self->unk5E6 == 0) {
                u8 r;
                u8 c;
                if (pl_part_flag_ck(self, 0x38) != 0) {
                    f25 = 1;
                } else if (pl_part_flag_ck(self, 0x39) != 0) {
                    f24 = 1;
                }
                if (pl_part_flag_ck(self, 0x3A) != 0) {
                    f27 = 1;
                } else if (pl_part_flag_ck(self, 0x3B) != 0) {
                    f26 = 1;
                }
                r = get_cfg(self->chunk_ofs, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x200;
                }
            }
            break;
        default:
            break;
        }
        if (mag == 0) {
            /* `mag` is still 0 only when no case arm fired: that is this unit's rule-8 replacement for the
             * `block_53` edge, and the pad scan below is what retail's label reached. */
            if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
                u16 w = *(u16*)((u8*)self + 0xC8);
                u8 r;
                u8 c;
                if ((s32)(w & 0x3C00) != 0 && (u16)*(u16*)((u8*)self + 0xD4) >= 0x32) {
                    if ((s32)(w & 0x2000) != 0) {
                        f25 = 1;
                    } else if ((s32)(w & 0x1000) != 0) {
                        f24 = 1;
                    }
                    if ((s32)(w & 0x800) != 0) {
                        f27 = 1;
                    } else if ((s32)(w & 0x400) != 0) {
                        f26 = 1;
                    }
                    r = get_cfg(self->chunk_ofs, 9);
                    if (r == 1 || r == 3) {
                        c = f25;
                        f25 = f24;
                        f24 = c;
                    }
                    if ((u8)(r + 0xFE) <= 1) {
                        c = f27;
                        f27 = f26;
                        f26 = c;
                    }
                    spC = lbl_8079A080;
                    fn_802B9740(self, &sp9, &sp8, &spC);
                    mag = 1;
                    step = 0x4C;
                    t = (s32*)lbl_805C6118;
                    while (t[0] != -1) {
                        if ((s32)*(u16*)((u8*)self + 0xD4) >= t[0]) {
                            f32 a = (f32)t[1];
                            f32 m = spC * ((f32)t[2] - a);
                            mag = (s32)(a + m);
                            f32 b = (f32)t[3];
                            f32 s = spC * ((f32)t[4] - b);
                            step = (s32)(b + s);
                            break;
                        }
                        t += 5;
                    }
                }
            }
        {
            s32 c = f25 + f24;
                if (c != 0 && (s32)(f27 + f26) != 0) {
                    step = (s32)(lbl_8079A0F0 * (f32)step);
                    mag = (s32)(lbl_8079A0F0 * (f32)mag);
                    if (mag < 1) {
                        mag = 1;
                    }
                }
                if (c != 0) {
                    dir = mag;
                    if ((s32)f25 == 0) {
                        dir = -mag;
                    }
                    self->unk5E5 = 5;
                }
                if ((s32)f27 != 0) {
                    self->unk5E5 = 0xA;
                    speed += step;
                }
                if ((s32)f26 != 0) {
                    self->unk5E5 = 0xA;
                    speed -= step;
                }
            }
        }
        break;
    }
    switch (arg2) {
    case 0:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A000(self, dir);
        switch (arg1) {
        case 0x3E9:
            fn_80279EBC(self, 0x406, 0x407);
            return;
        case 0x41A:
            fn_80279EBC(self, 0x438, 0x439);
            return;
        }
        break;
    case 1:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A044(self, (s16)(dir << 6));
        return;
    case 2: {
        *(s32*)((u8*)self + 0xA8) = speed;
        u16 m = *(u16*)((u8*)self + 0x38);
        u16 d = speed - m;
        if ((u32)(d - 0x4001) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m + 0x4000;
        } else if ((u32)(d - 0x8000) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m - 0x4000;
        }
        fn_8027A08C(self, (s8)dir);
        break;
    }
    }
}

/* The callees of the bodies below, under the map's spellings. */
extern "C" u16 fn_803BA9B0(u8, u8, u8*, s16*, void*, void*, void*);
extern "C" void hit_flag_set__FP6_HIT_WUl(void*, u32);
extern "C" void hit_flags_clear(void*);
extern "C" void hud_item_msg_push(s32, s32, s32);
extern "C" void lb_event_request(s32);
extern "C" u16 ran_suu__Fl(s32);
extern "C" void item_pair_copy(void*, void*);
extern "C" u32 fn_80274DCC(_PLW*, u8);
extern "C" s32 fn_8027D968(_PLW*, void*, void*, void*);
extern "C" s32 fn_8027DC90(void);
extern "C" u32 fn_802D7804(s32, f32);
extern "C" void mtx34_identity(void*);
extern "C" void fn_8008C484(void*, f32, f32, f32);
extern "C" void mtx34_concat_assign(void*, void*);
extern "C" u8 fn_80224E28(_PLW*, u8);
extern "C" void fn_8026A394(_PLW*, s32, void*);
extern "C" void fn_8026A230(_PLW*, s32, u16, s32, s32);
extern "C" void fn_8026A23C(_PLW*, s32, f32);
extern "C" f32 fn_8026A34C(_PLW*);
} /* namespace s_80276B58 */


void mulVecMat(nw4r::math::VEC3*, nw4r::math::MTX34*);
void setVector3(nw4r::math::VEC3*, f32, f32, f32);
namespace s_80276B58 {


extern "C" { extern const f32 lbl_8079A0C0; }
extern "C" { extern const f32 lbl_8079A0D4; }
extern "C" { extern const f32 lbl_8079A0D8; }
extern "C" { extern const f32 lbl_8079A0F8; }
extern "C" { extern u8 lbl_805C6168[]; }
extern "C" { extern u8 lbl_805C61D4[]; }

/* 0x80277DAC: whether a ground-height probe along the actor's fractional facing clears the band. */
extern "C" s32 fn_80277DAC(_PLW* self, s32 arg1, f32 arg2, f32 arg3)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    v.x = lbl_8079A084;
    v.y = lbl_8079A084;
    v.z = arg2;
    rotVecY(&v, self->field_0x058);
    v.x = v.x + self->motion_pos_0x3C;
    v.y = v.y + self->motion_pos_0x40;
    v.z = v.z + self->motion_pos_0x44;
    f32 h = GetGroundHit2(&v, (u32)-2, self->area_0x16, &hit);
    if ((s32)hit == 0) {
        return 0;
    }
    f32 c = self->motion_pos_0x40 + arg3;
    if (arg1 == 0) {
        if (h < c) {
            return 1;
        }
    } else {
        if (h > c) {
            return 1;
        }
    }
    return 0;
}

/* 0x80277FF8: whether the given action may start on the current map/weapon combination. */
extern "C" s32 fn_80277FF8(u8 arg0, u8 arg1, s32 arg2)
{
    s32 t = arg2 & 0x7F;
    switch (arg0) {
    case 1:
        if (((u32)(arg1 - 2) <= 4U || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 2:
        if (((s32)(u8)arg1 == 6 || (s32)(u8)arg1 == 9 || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 3:
        if ((u32)(arg1 - 5) <= 1U) {
            if (t != 0) {
                return 0;
            }
        } else if ((s32)(u8)arg1 == 1 || (s32)(u8)arg1 == 8) {
            if ((u32)t > 1U) {
                return 0;
            }
        } else if ((s32)(u8)arg1 == 9) {
            if (t != 0) {
                return 0;
            }
        }
        break;
    case 4:
        if ((u32)(arg1 - 1) <= 5U && t != 0) {
            return 0;
        }
        break;
    case 5:
        if (((s32)(u8)arg1 == 3 || (s32)(u8)arg1 == 5 || (s32)(u8)arg1 == 8 || (s32)(u8)arg1 == 10) &&
            t != 0) {
            return 0;
        }
        break;
    case 8:
    case 9:
    case 10:
        if (t != 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278144: whether the given action may start in the player's current map/move-work state. */
extern "C" u32 fn_80278144(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = stage_map_kind_get(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = (u8*)get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 7 || (s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 3:
            if ((s32)arg0 == 2 || (s32)arg0 == 4 || (s32)arg0 == 8) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278310: the same map/move-work gate as 80278144, for the smaller action set. */
extern "C" u32 fn_80278310(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = stage_map_kind_get(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = (u8*)get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (fn_802B0688(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80277974: builds the attack entry an incoming hit produces and runs the shared attack set-up. */
extern "C" void fn_80277974(_PLW* self, _HIT_W* hit, u8* base, u16 idx, s32* ids, u16 flags)
{
    u8* p = (u8*)((u8*)self->physics_0x13C);
    u8* d = base + idx * 0x1A;
    *(s32*)((u8*)hit + 0x08) = ids[*(u8*)(d + 0xF)];
    hit_flags_clear(hit);
    if ((flags & 1) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x728);
    } else {
        hit_flag_set__FP6_HIT_WUl(hit, 0x708);
    }
    if (self->field_0x002 == 1) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x800);
    }
    if ((flags & 0x80) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x1000);
    }
    if ((flags & 0x100) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x2000);
    }
    *(u16*)((u8*)hit + 0x18) = Get_motion_no(self);
    *(s16*)((u8*)hit + 0x1A) = 0;
    *(s16*)((u8*)hit + 0x1C) = 0;
    *(s16*)((u8*)hit + 0x1E) = 0;
    f32 f = *(f32*)(p + 0x48) - lbl_8079A0C0;
    if (f < lbl_8079A084 || (*(u32*)(p + 0x50) & 1) != 0) {
        f = lbl_8079A084;
    }
    f32 g = *(f32*)(p + 0x78);
    f32 t = (f32)*(s16*)(d + 0);
    f32 u = (f32)*(s16*)(d + 2);
    *(f32*)((u8*)hit + 0x38) = t;
    *(f32*)((u8*)hit + 0x3C) = u;
    if (*(s16*)(d + 0) != 0) {
        f32 x = t + (f - g);
        *(f32*)((u8*)hit + 0x38) = x;
        if (x < lbl_8079A084) {
            f32 y = u + x;
            *(f32*)((u8*)hit + 0x3C) = y;
            *(f32*)((u8*)hit + 0x38) = lbl_8079A084;
            if (y <= lbl_8079A084) {
                *(u8*)((u8*)hit + 0x05) = 0;
                return;
            }
        }
    }
    hit_data_apply((_HIT_W*)hit, (::_HIT_DATA*)d);
    Pl_attack_set_sub(self, (_HIT_DATA*)d, hit, flags);
}

/* 0x80278674: adds a signed amount to the actor's stamina pool, with the two armour-skill modifiers. */
extern "C" void fn_80278674(_PLW* self, s16 arg1, u8 arg2)
{
    s16 v = arg1;
    s32 changed = 0;
    if (Pl_master_ck(self) != 0 && fn_8026FE44(self) != 1U) {
        if (v < 0 && arg2 == 0) {
            if (Pl_Skill_ck(self, 0x15) == 1U) {
                if (v == -1 && (ran_suu__Fl(1) & 1) != 0) {
                    return;
                }
                v = (s16)((((s32)((u32)v >> 31)) + v) >> 1);
                if (v == 0) {
                    v = -1;
                }
            } else if (Pl_Skill_ck(self, 0x16) == 1U) {
                v = (s16)(v * 2);
            }
        }
        *(s16*)((u8*)self + 0x56C) += v;
        if (*(s16*)((u8*)self + 0x56C) <= 0) {
            *(s16*)((u8*)self + 0x56C) = 0;
        }
        s16 lim = *(s16*)((u8*)self + 0x56E);
        if (*(s16*)((u8*)self + 0x56C) > lim) {
            *(s16*)((u8*)self + 0x56C) = lim;
            changed = 1;
        }
        s32 m = fn_80278590(self);
        u8 old = *(u8*)((u8*)self + 0x56B);
        if ((u8)m != old) {
            if (old > (u8)m) {
                hud_item_msg_push(1, 4, 0);
                lb_event_request(0x15);
            } else {
                hud_item_msg_push(1, 5, 0);
            }
            *(u8*)((u8*)self + 0x56B) = m;
        }
        if (changed != 0) {
            hud_item_msg_push(1, 6, 0);
        }
    }
}

/* 0x8027993C: the first item slot the actor may use, scanned round the 33-entry shell ring. */
extern "C" u16 fn_8027993C(_PLW* self, u16 arg1, u8 arg2)
{
    u16 slot;
    s32 i;
    if (fn_8026FE44(self) == 0) {
        return 0xFF;
    }
    u8 ring[33 * 4];
    for (i = 0; i < 0x18; i++) {
        item_pair_copy(ring + i * 4, (u8*)self + 0x278 + i * 4);
    }
    for (i = 0; i < 9; i++) {
        s16 j = (s16)i;
        item_pair_copy(ring + (j + 0x18) * 4, (u8*)self + 0x278 + (j + 0x1A) * 4);
    }
    slot = arg1;
    if ((slot & 0x80) != 0) {
        slot = (u16)((slot & 0x7F) + 0x18);
    }
    if ((s32)slot >= 0x21) {
        slot = 0;
    }
    switch (arg2) {
    case 0:
        slot = (u16)((slot + 1) % 33);
        break;
    case 1:
        slot = (u16)((slot == 0) ? 32 : slot - 1);
        break;
    }
    for (i = 0; i < 0x21; i++) {
        u8* e = ring + slot * 4;
        if (*(u16*)(e + 0) != 0 && *(s16*)(e + 2) > 0) {
            u8* it = (u8*)GetItemData(*(u16*)(e + 0));
            if ((it[2] & 8) != 0 && it[0] == 1 &&
                fn_80274DCC(self, (u8)fn_80274B20(*(u16*)(e + 0))) == 1U) {
                if ((u32)slot >= 0x18U) {
                    slot = (u16)((slot - 0x18) | 0x80);
                }
                return slot;
            }
        }
        switch (arg2) {
        case 0:
        case 2:
            slot = (u16)((slot + 1) % 33);
            break;
        case 1:
        case 3:
            slot = (u16)((slot == 0) ? 32 : slot - 1);
            break;
        }
    }
    return 0xFF;
}

/* 0x80279C20: refreshes the actor's held-shell state each frame - the ring position, the shell search,
 * and the save/restore of the fields it rewrites. */
extern "C" void fn_80279C20(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (fn_8026FE44(self) == 0) {
        return;
    }
    if (Pl_motion_input_ck(1) == 1U) {
        return;
    }
    if (Pl_bari_ck(self, 1) == 1U) {
        return;
    }
    u8 a = self->field_0x00A;
    if ((u32)(a - 8) <= 1U) {
        return;
    }
    switch (a) {
    case 6: {
        s32 id = self->act_no;
        if (id == 0x1B) {
            return;
        }
        if (id == 0x23) {
            return;
        }
        break;
    }
    case 4: {
        s32 id = self->act_no;
        if (id >= 0x17) {
            if (id >= 0x32) {
                break;
            }
            if (id >= 0x1A) {
                return;
            }
        } else {
            if (id >= 0x0E) {
                if (id >= 0x13) {
                    return;
                }
                break;
            }
            if (id < 5) {
                break;
            }
        }
        if (*(u8*)((u8*)self + 5) <= 2) {
            return;
        }
        break;
    }
    }
    if (*(u8*)((u8*)self + 0x26B) == 0) {
        *(u8*)((u8*)self + 0x26B) = 1;
        *(u8*)((u8*)self + 0x26D) = self->held_item_kind_0x26C;
        *(s16*)((u8*)self + 0x272) = *(s16*)((u8*)self + 0x270);
        *(u8*)((u8*)self + 0x275) = *(u8*)((u8*)self + 0x26A);
        *(u8*)((u8*)self + 0x274) = self->unk269;
    }
    if (fn_8026A6F4(self, 0xB) == 1U) {
        if (fn_8026A6F4(self, 0xD) == 1U) {
            if (self->unk26E == 0xFF) {
                self->unk26E = fn_8027993C(self, 0, 3);
            } else {
                self->unk26E = fn_8027993C(self, 1, 0);
            }
            fn_80279B84(self);
            if (self->unk26E != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 8);
                sysSE_req(6);
            }
        } else if (fn_8026A6F4(self, 0xC) == 1U) {
            if (self->unk26E == 0xFF) {
                self->unk26E = fn_8027993C(self, 0, 2);
            } else {
                self->unk26E = fn_8027993C(self, 0, 0);
            }
            fn_80279B84(self);
            if (self->unk26E != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 0x10);
                sysSE_req(6);
            }
        }
    } else if (self->unk26B != 0) {
        self->unk26B = 0;
        if (self->unk26D != self->held_item_kind_0x26C) {
            fn_80279B84(self);
        } else {
            *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
            *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
            self->unk269 = *(u8*)((u8*)self + 0x274);
        }
    }
    if (self->unk26D == self->held_item_kind_0x26C) {
        *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
        *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
        self->unk269 = *(u8*)((u8*)self + 0x274);
    }
}

/* 0x80279EBC: starts the shell-swap motion, scaled by the actor's stored shell count. */
extern "C" void fn_80279EBC(_PLW* self, u16 arg1, u16 arg2)
{
    s32 v = *(s8*)((u8*)self + 0x64F);
    if (v == 0) {
        fn_8026A23C(self, 0, lbl_8079A080);
        fn_8026A23C(self, 1, lbl_8079A084);
        return;
    }
    f32 f = (f32)v;
    *(u8*)((u8*)self + 0x64E) = 1;
    u16 m;
    if (v > 0) {
        if (arg1 == 0x581) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg1;
        }
    } else {
        if (arg2 == 0x582) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg2;
        }
        f = f * lbl_8079A0D4;
    }
    fn_8026A230(self, 1, m, 0, (s32)(lbl_8079A0C0 + fn_8026A34C(self)));
    f32 t = f * lbl_8079A0D8;
    fn_8026A23C(self, 0, lbl_8079A080 - t);
    fn_8026A23C(self, 1, t);
}

/* 0x8027A340: whether the actor may act at all this frame - master/bari state, the stun flag and the
 * per-action-class exceptions. */
extern "C" s32 fn_8027A340(_PLW* self)
{
    s32 ok = 1;
    if (Pl_master_ck(self) == 0) {
        ok = 0;
    }
    if (fn_8026FE44(self) == 0) {
        ok = 0;
    }
    if (self->unk5E6 != 0) {
        ok = 0;
    }
    if (pl_part_flag_ck(self, 0x1D) == 0) {
        if (self->field_0x00A == 4) {
            s32 id = self->act_no;
            if (id != 0x13 && id != 0x2A && id != 0x2E && id != 0x15 && id != 0x2C && id != 0x30) {
                ok = 0;
            }
        } else {
            ok = 0;
        }
    }
    u8 a = *(u8*)((u8*)self + 0x00A);
    if ((u32)(a - 5) > 6U) {
        switch (a) {
        case 0: {
            s32 id = self->act_no;
            switch (id) {
            case 1:
            case 2:
            case 5:
            case 6:
            case 7:
            case 20:
            case 21:
            case 28:
            case 73:
            case 91:
            case 122:
            case 158:
            case 159:
            case 160:
            case 161:
                ok = 0;
                break;
            }
            break;
        }
        case 4: {
            s32 id = self->act_no;
            if ((u32)(id - 2) <= 1U || (u32)(id - 0x10) <= 1U) {
                ok = 0;
            }
            break;
        }
        case 2:
            ok = 0;
            break;
        }
    } else {
        ok = 0;
    }
    return ok;
}

/* 0x8027AC2C: sets the two stored shell ids (or their per-weapon-class defaults) and scales them by
 * the actor's ammo skill. */
extern "C" void fn_8027AC2C(_PLW* self, u8 arg1, u8 arg2)
{
    if (arg1 == 0xFF) {
        u8 t = self->field_0x002;
        *(u8*)((u8*)self + 0x568) = lbl_805BFFCC[t * 2];
        if (t == 7 && Pl_condition_ck(self, 0x80000000) == 1U) {
            *(u8*)((u8*)self + 0x568) = 0x71;
        }
    } else {
        *(u8*)((u8*)self + 0x568) = arg1;
    }
    if (arg2 == 0xFF) {
        u8 t = self->field_0x002;
        *(u8*)((u8*)self + 0x569) = lbl_805BFFCC[t * 2 + 1];
    } else {
        *(u8*)((u8*)self + 0x569) = arg2;
    }
    f32 f = (f32)((s8)*(u8*)((u8*)self + 0x452) + 100) / lbl_8079A0D0;
    if (Pl_cat_skill_ck(self, 0x26) == 1U) {
        s32 n = *(u8*)((u8*)self + 0x445);
        if (n != 0) {
            s16 i = 0;
            if (n > 0) {
                for (; i < n; i++) {
                    f = f * lbl_8079A0AC;
                }
            }
        }
    }
    *(u8*)((u8*)self + 0x568) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x568)) * f);
    *(u8*)((u8*)self + 0x569) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x569)) * f);
}

/* 0x8027C064: builds the impact vector an attacking part starts from - the per-weapon-class motion, the
 * hit matrix and the actor's world offset. */
extern "C" void fn_8027C064(_PLW* self, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 v;
    u8 buf[12];
    nw4r::math::MTX34 m;
    nw4r::math::MTX34 m2;
    s32 part;
    MTX34_ctor(&m);
    MTX34_ctor(&m2);
    VEC3_ctor(&v);
    u8 t = fn_80224E28(self, lbl_805BAA90[self->field_0x002]);
    out->x = lbl_8079A084;
    out->y = lbl_8079A084;
    setVector3(&v, lbl_8079A084, lbl_8079A084, lbl_8079A084);
    switch ((s32)t) {
    default:
        part = *(s32*)(lbl_805C6168 + (self->field_0x002 * 3 + 2) * 4);
        out->z = lbl_8079A084;
        break;
    case 1:
        if (self->field_0x002 == 3) {
            copyVec3(&v, (const nw4r::math::VEC3*)fn_80143174(buf, lbl_805BAC98 + self->field_0x002 * 0x18 + 0xC,
                                                              self->field_0x002 * 0x18));
        }
        /* fall through */
    case 0:
        part = *(s32*)(lbl_805C6168 + (t + self->field_0x002 * 3) * 4);
        out->z = *(f32*)(lbl_805C61D4 + self->field_0x002 * 4);
        break;
    }
    fn_8026A394(self, part, &m);
    mtx34_identity(&m2);
    fn_8008C484(&m2, v.x, v.y, v.z);
    mtx34_concat_assign(&m, &m2);
    mulVecMat(out, &m);
    out->x = out->x + m.m[0][3];
    out->y = out->y + m.m[1][3];
    out->z = out->z + m.m[2][3];
}

/* 0x8027C208: the action-class gate - whether the actor may start the given action id. */
extern "C" s32 fn_8027C208(_PLW* self, u16 arg1)
{
    nw4r::math::VEC3 sp24;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s16 sp8;
    VEC3_ctor(&sp24);
    if (self->field_0x00A == 7) {
        if (arg1 == 0x15B) {
            if ((u32)self->act_no <= 1U) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    if ((Pl_act_ck(self, 6, 0x1C) == 1U || Pl_act_ck(self, 6, 0x1D) == 1U) && arg1 != 0x176) {
        return 0;
    }
    if ((fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) && arg1 != 0x177) {
        return 0;
    }
    switch ((s32)arg1) {
    case 0x177:
        if (fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) {
            return 1;
        }
        break;
    case 3:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_802782B8(self);
    case 4:
    case 0x31:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_80278450(self);
    case 0x22:
    case 0x23:
    case 0x24:
        if (fn_803BA9B0(self->chunk_ofs, self->area_0x16, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 3) {
            return 1;
        }
        break;
    case 0x25:
    case 0x26:
    case 0x27:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (fn_803BA9B0(self->chunk_ofs, self->area_0x16, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 4) {
            return 1;
        }
        break;
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0x15F:
    case 0x161:
    case 0x163:
    case 0x185:
    case 0x186:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (fn_803BA9B0(self->chunk_ofs, self->area_0x16, (u8*)self + 0x3C, &sp8, &sp24, &sp18, &sp14) != 0xFFFFU &&
            sp8 == 5) {
            return 1;
        }
        break;
    case 0x159:
    case 0x15A:
    case 0x15B:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0xA6:
    case 0xA7:
    case 0xD0:
    case 0xD6:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->kind_0x09 == 3) {
            return 0;
        }
        return fn_802784A8(self);
    case 0x16E:
        return fn_802784B8(self);
    case 6:
    case 0x2E:
    case 0x8A:
    case 0xC2:
    case 0x184:
        if (self->kind_0x09 == 3) {
            return 1;
        }
        break;
    case 0x28:
    case 0x2F:
    case 0x109:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (fn_8027CC2C(self) == 1U) {
            return 0;
        }
        if (Pl_item_timer_get(self, 0x1D) > 0 && self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1B6:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (fn_8027CC2C(self) == 1U) {
            return 0;
        }
        if (Pl_item_timer_get(self, 0x1D) > 0 && fn_802D7804(4, lbl_8079A0F8) == 1U) {
            return 1;
        }
        break;
    case 0x169:
    case 0x16A:
    case 0x16B:
    case 0x16C:
    case 0x16D:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x2B:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x17F:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 1;
        }
        break;
    case 2:
    case 0x34:
    case 0x246:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return fn_80277F54(self);
    case 0x108:
        if (self->kind_0x09 == 3) {
            return 0;
        }
    case 1:
    case 0x237:
    case 0x258:
    case 0x259:
        if (*(u8*)((u8*)self + 0x585) == 0) {
            return 1;
        }
        break;
    case 0x30:
    case 0x62:
    case 0xCF:
        if (fn_8026FE44(self) == 0) {
            return 1;
        }
        break;
    case 0x180:
        if (fn_8027DC90() == 0) {
            return 0;
        }
    case 0x17C:
        if (fn_8027D968(self, &sp24, &sp10, &spC) == 1) {
            return 1;
        }
        break;
    case 0x247:
        if (fn_8027D968(self, &sp24, &sp10, &spC) == 5) {
            return 1;
        }
        break;
    default:
        return 1;
    }
    return 0;
}

} /* namespace s_80276B58 */

/* ==== 0x8027D684-0x802840DC: the equipment helpers ==== */

#include "types.h"
#include "pl.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
namespace s_8027D684 {



extern "C" s32 equip_kind_table_class(u8 kind);
extern "C" u8 fn_8027E290(u8 kind);
extern "C" u8 fn_8027E29C(s32 kind);

/* The 1-based kind's 0x18-byte equipment row (the row stride is the `mulli r0,r0,24` in retail). */
extern "C" void* fn_8027E2A8(u8 kind, u16 index) {
    u8** base = lbl_806AB810[0];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = fn_8027E290(kind);
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    return base[slot] + index * 0x18;
}

/* The sibling 0x1C-byte row of the second table. */
extern "C" void* fn_8027E354(u8 kind, u16 index) {
    u8** base = lbl_806AB810[1];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = fn_8027E290(kind);
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    return base[slot] + index * 0x1C;
}

/* The 7-based kind's 0x18-byte row of the fifth table (no kind guard, unlike the 1-based pair). */
extern "C" void* fn_8027EC50(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[fn_8027E29C(kind)] + index * 0x18;
}

/* The sibling 0x24-byte row of the same table. */
extern "C" void* fn_8027ECBC(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[fn_8027E29C(kind)] + index * 0x24;
}

/* In-unit callees referenced before their own definition (this TU is their owner). */
extern "C" void* fn_8027EC50(u8 kind, u16 item_id);
extern "C" void* fn_8027ECBC(u8 kind, u16 item_id);
extern "C" void* fn_8027ED6C(u8 kind, u16 item_id, u32 a, u32 b);
extern "C" void* fn_8027FB84(u8 kind, u16 item_id, u8 deco_count);
extern "C" void* fn_8027FE50(u8 kind, u16 item_id);
extern "C" void* fn_8027E2A8(u8 kind, u16 index);
extern "C" void* fn_8027E354(u8 kind, u16 index);

/* True while the player's health is at or below zero. */
extern "C" s32 fn_8027D684(_PLW* self) {
    return self->field_0x36C <= 0;
}

/* Stores the raw pair the +0x452/+0x458 group holds. */
extern "C" void fn_8027D698(_PLW* self, s8 arg1, s16 arg2) {
    self->field_0x452 = arg1;
    self->field_0x458 = arg2;
}

/* Stores the byte and raises the +0x454 limit to the argument. */
extern "C" void fn_8027D6A4(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x450 = arg1;
    if (self->field_0x454 < arg2) {
        self->field_0x454 = arg2;
    }
}

/* Stores the byte and raises the +0x456 limit to the argument. */
extern "C" void fn_8027D6C0(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x451 = arg1;
    if (self->field_0x456 < arg2) {
        self->field_0x456 = arg2;
    }
}

/* Latches the motion-request byte, then sends the motion through the +0x5C8 gate. */
extern "C" void fn_8027D6DC(_PLW* self, s16 value) {
    self->field_0x5C9 = 1;
    if (self->field_0x5C8 != 0) {
        u32 motion = fn_802745DC(self, value);
        pl_act_enter_raw(self, 0xB, (u16)motion, 0);
    }
}

/* Reports the byte at +0x268 as a boolean; `u32`, because the item menu's caller compares the result
 * unsigned (`cmplwi r3,0x1` at 0x802A008C), as `Pl/fn_8027D684.h` declares it. */
extern "C" u32 fn_8027D738(_PLW* self) {
    return self->field_0x268 != 0;
}

/* The +0x655 high-bit gate in front of the +0x268 boolean. */
extern "C" s32 fn_8027D74C(_PLW* self) {
    if ((self->field_0x655 & 0x80) == 0) {
        return 0;
    }
    return fn_8027D738(self);
}

/* Ends the charged action once its timer has run out. */
extern "C" void fn_8027D76C(_PLW* self) {
    if (Pl_master_ck(self) != 0 && (self->field_0x655 & 0x80) != 0) {
        self->act_end_request = 1;
        s16 timer = self->field_0x652;
        if (Pl_item_timer_get(self, self->field_0x650) >= timer) {
            pl_item_add(self, self->field_0x650, -timer);
        }
    }
}

/* The act range 0x54-0x56 of the idle kind. */
extern "C" s32 fn_8027D874(_PLW* self) {
    if (self->field_0x00A == 0 && (u32)(self->act_no - 0x54) <= 2U) {
        return 1;
    }
    return 0;
}

/* Reports the first of the three per-slot gate bytes at 0x806BB7A0 as empty. */
extern "C" s32 fn_8027DC64(void) {
    return lbl_806BB7A0[0].flag_0x00 == 0;
}

/* Reports the second gate byte as empty. */
extern "C" s32 fn_8027DC78(void) {
    return lbl_806BB7A0[1].flag_0x00 == 0;
}

/* Reports the third gate byte as empty. */
extern "C" s32 fn_8027DC90(void) {
    return lbl_806BB7A0[2].flag_0x00 == 0;
}

/* The +0x460 timer, as a boolean. */
extern "C" s32 Pl_timer_0x460_ck(_PLW* self) {
    return self->field_0x460 > 0;
}

/* Advances the 0-100 counter at +0x445 and saturates it. */
extern "C" void fn_8027E048(_PLW* self) {
    if (++self->field_0x445 >= 0x64U) {
        self->field_0x445 = 0x64U;
    }
}


/* The act-number ranges of the held (kind 6) actor. */
extern "C" s32 fn_8027DCA8(_PLW* self) {
    if (self->field_0x00A == 6) {
        return (u32)(self->act_no - 0x34) <= 2U || (u32)(self->act_no - 0x3D) <= 1U;
    }
    return 0;
}

/* The act-number gate the held-state handler asks with the part index. */
extern "C" u32 fn_8027DCE0(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A != 0) {
        return 0;
    }
    switch (self->act_no) {
    default:
        return 0;
    case 34:
    case 35:
        return 1;
    case 36:
        return arg1 != 2;
    case 37:
        return arg1 != 1;
    case 111:
    case 112:
    case 113:
    case 114:
        return 1;
    case 115:
    case 116:
        return arg1 != 1;
    case 117:
    case 118:
        return arg1 != 2;
    case 119:
    case 120:
        return 1;
    }
}

/* The second act-number gate of the same handler family. */
extern "C" u32 fn_8027DDC4(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A == 0) {
        if ((u32)(self->act_no - 0x3F) > 1U) {
            if ((u32)(self->act_no - 0x42) > 1U) {
                if (self->act_no != 0x41 && self->act_no != 0x9A) {
                    return 0;
                }
                if (arg1 == 2) {
                    return 0;
                }
                return 1;
            }
            return arg1 != 1;
        }
        return 1;
    }
    return 0;
}

/* Scans the per-kind item-id table for the first id the slot lookup accepts. */
extern "C" u16 fn_8027DE88(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = lbl_80792030;
        break;
    case 1:
        table = lbl_80792038;
        break;
    case 2:
        table = lbl_805BFFE0;
        break;
    default:
        return 0xFFFF;
    }
    for (;;) {
        u16 id = *table;
        if (id == 0xFFFF) {
            return 0xFFFF;
        }
        u16 result = fn_80273044(self, id);
        if (result != 0xFFFF) {
            return result;
        }
        table++;
    }
}

/* Scans the same id table for the equipped slot that carries the id. */
extern "C" u16 fn_8027DF38(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = lbl_80792030;
        break;
    case 1:
        table = lbl_80792038;
        break;
    case 2:
        table = lbl_805BFFE0;
        break;
    default:
        return 0xFFFF;
    }
    for (;;) {
        u16 id = *table;
        if (id == 0xFFFF) {
            return 0xFFFF;
        }
        u16 slot = self->field_0x304;
        if (id == self->slot_id[slot].item_id && self->slot_id[slot].value > 0) {
            return slot;
        }
        table++;
    }
}

/* The act-number class of the held actor, as the handler's return code. */
extern "C" s32 fn_8027DFE4(_PLW* self) {
    switch (self->field_0x00A) {
    case 0:
        switch (self->act_no) {
        case 0xAB:
        case 0xAE:
            return 1;
        case 0xA7:
            return 3;
        }
        return 0;
    case 12:
        if (self->act_no == 0xC) {
            return 2;
        }
        return 0;
    }
    return 0;
}

/* The per-part act-number gate of the held actor. */
extern "C" s32 fn_8027E06C(_PLW* self, s32 arg1) {
    if (self->field_0x00A != 6) {
        return 0;
    }
    switch (arg1) {
    case 0:
        if ((u16)(self->act_no - 0x15) <= 1U) {
            return 1;
        }
        return 0;
    case 1:
        if ((u32)(self->act_no - 0xE) <= 1U || (u32)(self->act_no - 0x24) <= 1U) {
            return 1;
        }
        return 0;
    case 2:
        if ((u32)(self->act_no - 4) <= 3U || (u32)(self->act_no - 0x1E) <= 2U ||
            (u32)(self->act_no - 0x1A) <= 1U || self->act_no == 0x26) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* ORs the act-flag bits into the byte at +0x567. */
extern "C" void fn_8027E1B8(_PLW* self, u8 flags) {
    self->atk_act_flag |= flags;
}
} /* namespace s_8027D684 */


/* Tests the act-flag byte at +0x567 against a mask. */
s32 Pl_atk_act_flag_ck(_PLW* self, u8 mask) {
    return (self->atk_act_flag & mask) != 0;
}
namespace s_8027D684 {


/* The three-way "is this actor held" test: the +0x352 timer, the system flag or the +0x46F byte. */
extern "C" s32 fn_8027E1E4(_PLW* self) {
    if (self->field_0x352 > 0 || system_w.field_0x2a != 0 || self->field_0x46F != 0) {
        return 1;
    }
    return 0;
}

/* Reports the +0x396 timer, or the live player work through the hold test. */
extern "C" u32 fn_8027E220(_PLW* self, s32 unused) {
    if (self->field_0x396 > 0) {
        return 1;
    }
    if (Pl_master_ck(self) == 1U && fn_8027E1E4(self) == 1U) {
        return 1;
    }
    return 0;
}

/* The equipment kind the data tables are indexed by, one-based. */
extern "C" u8 fn_8027E290(u8 kind) {
    return kind - 1;
}

/* The same one-based kind, for the seven-kind table. */
extern "C" u8 fn_8027E29C(s32 kind) {
    return kind - 7;
}

/* Looks up the 0x18-byte equipment row an `_EQUIP` record names. */
extern "C" void* fn_8027E344(_EQUIP* equip) {
    return fn_8027E2A8(equip->kind, equip->item_id);
}

/* Looks up the 0x1C-byte sibling row the same record names. */
extern "C" void* fn_8027E3F4(_EQUIP* equip) {
    return fn_8027E354(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> data-row lookup the table helper and the sound unit share. */
extern "C" void* fn_8027ECAC(_EQUIP* equip) {
    return fn_8027EC50(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> item lookup the four Pl consumers share. */
extern "C" void* fn_8027ED18(void* equip) {
    _EQUIP* e = (_EQUIP*)equip;
    return fn_8027ECBC(e->kind, e->item_id);
}

/* The three-argument sibling of `fn_8027ED18`. */
extern "C" void* fn_8027EE08(_EQUIP* equip, u32 arg2, u32 arg3) {
    return fn_8027ED6C(equip->kind, equip->item_id, arg2, arg3);
}

/* The `_EQUIP` -> row lookup that also passes the decoration count. */
extern "C" void* fn_8027FC70(_EQUIP* equip) {
    return fn_8027FB84(equip->kind, equip->item_id, equip->deco_count);
}

/* The last `_EQUIP` -> row lookup of the family. */
extern "C" void* fn_8027FF20(_EQUIP* equip) {
    return fn_8027FE50(equip->kind, equip->item_id);
}


/* The equipment kind's row-table class: 0 for kinds 1-6, 1 for 7-11 and 14-15, 2 for 12-13, 0xFF for
 * anything else. */
extern "C" s32 equip_kind_table_class(u8 kind) {
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
        return 1;
    case 12:
    case 13:
        return 2;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return 0;
    }
    return 0xFF;
}

/* The per-kind equipment row count the kind's own table class selects. */
extern "C" s32 fn_8027E918(u8 kind) {
    s32 count = 0;
    if ((u32)(kind - 7) > 8U) {
        if ((u32)(kind - 1) <= 5U) {
            count = lbl_805706C0[fn_8027E290(kind)];
        }
    } else {
        count = lbl_805706D8[fn_8027E29C(kind)];
    }
    return count;
}
} /* namespace s_8027D684 */


/* The address of the eight per-kind equipment table pointers at 0x806AB810. */
u8*** get_eq_data_ptr(void) {
    return lbl_806AB810;
}
