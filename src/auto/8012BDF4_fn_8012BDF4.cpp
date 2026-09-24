/* auto/8012BDF4_fn_8012BDF4.cpp - enemy action/status module, 71 function(s), 0x8012BDF4..0x8012E968.
 *
 * The unit is the enemy-side action bookkeeping: its public entry points (`ana_em_ck`, `shibire_em_ck`,
 * `em_act_ck`, `em_area_ck`, `em_die_ck`, `em_work_die_ck`) test an enemy's state, and the `fn_8012C*`
 * helpers walk the per-group work tables and set the flag byte at `_ENEMY_WORK::flags_0xA04` that a
 * later pass consumes.
 *
 * Types.  Two work records meet here, both reached through `get_move_work_adrs`:
 *   - `_PLW`  - the player-side record (`get_move_work_adrs(0)`/`(2)`), 0xB20 apart;
 *   - `_ENEMY_WORK` - the enemy-side record (`get_move_work_adrs(3)`), 0xB18 apart.  It is the same
 *     object the `em_*` entry points take, and `em_area_ck` confirms the area byte at +0x1E1.
 * Only the fields this file reads are named; everything else is `unused_0xNN` padding.
 *
 * Signature note.  The map's mangled names (`em_act_ck__FP11_ENEMY_WORKUcUc`, ...) are the symbol names
 * objdiff pairs on, but the `Uc`/`Ul` in them encode the *guess* the map was generated with, not the
 * parameters the original source used: a `u8` parameter is never masked by this compiler, while retail
 * masks these.  Every such symbol is therefore declared `extern "C"` under the map's own spelling,
 * which keeps the name and lets the parameter widths be chosen from the codegen.  Do not "tidy" them
 * into C++ declarations - that silently reintroduces the missing `clrlwi` and 0 % pairing.
 *
 * Flags and evidence: the unit's command line is the `auto` lib's (`configure.py`); the file carries a
 * scoped `#pragma peephole off` because retail keeps several folds this unit's `-O3` peephole fuses:
 * it keeps `clrlwi` + `cmpwi` separate where the peephole would fuse them into `clrlwi.`, which cost
 * `fn_8012C220` 5.7 points and `fn_8012C600` 3.9 with the peephole on (`fn_8012BDF4` 83.7 -> 85.2,
 * `fn_8012C0EC` 90.6 -> 96.7).
 *
 * Status.  64 of the 71 symbols are written; 60 of those measure >= 80 % and 42 are byte-identical
 * (`fuzzy_match_percent` 70.13, `matched_code` 3796 of 11124).  The 7 still unwritten are
 * `fn_8012C9AC` (1096 B), `fn_8012D23C` (420), `fn_8012D498` (384), `fn_8012D618` (484),
 * `fn_8012E708`, `fn_8012E718` and `fn_8012E728` - each needs a struct or a switch shape the written
 * ones did not settle.
 *
 * Residuals (the four written functions still under the bar):
 *   - `fn_8012E040` 54.8 %, `fn_8012E5D4` 29.5 %.  Both are dense-case dispatch trees: retail shares
 *     one tail per constant return and lowers the dense quartet 0x24..0x27 with `subi`/`cmplwi`,
 *     while every source shape tried emits either inline return blocks or a two-compare range test.
 *     Tried: if-chain in target order, `switch` over a `u8` local, `switch` over the cast expression,
 *     `(u32)`/`(u8)` casts on the range test, the two range tests merged with `||`.
 *   - `ana_em_ck__FUcPQ34nw4r4math4VEC3fUc`, `shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc` 78.5 %.
 *     Their loop bodies are right; the residual is the two `(u8)` argument masks: retail re-materialises
 *     `clrlwi` at each call site, this build hoists them out of the loop.  The sub-functions they call
 *     are 100 %, so the masks cannot move into the callee's prototype without losing those two.
 *
 * Pool literals (`lbl_805A0FF8`, `lbl_80796C90`, ...) belong to the data pass: they are declared, never
 * defined, so the load operands pair with the target's pooled constants (playbook 29).
 */

#include "types.h"

#include "nw4r/math.h"

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ *
 * The enemy-side work record (`get_move_work_adrs(3)`, 0xB18 apart).
 * ------------------------------------------------------------------------------------------------ */

/* size: 0xB18 */
struct _ENEMY_WORK {
    /* +0x000 */ u8 active;          /* nonzero while the record is in use */
    /* +0x001 */ u8 unused_0x001;
    /* +0x002 */ u8 group;           /* the enemy group/entry index, matched against the caller's */
    /* +0x003 */ u8 team;            /* matched against a player record's team byte */
    /* +0x004 */ u8 field_0x004;     /* `em_work_die_ck` treats values below 2 as still alive */
    /* +0x005 */ u8 unused_0x005[3];
    /* +0x008 */ u8 field_0x008;
    /* +0x009 */ u8 unused_0x009;
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 unused_0x00B[0x011 - 0x00B];
    /* +0x011 */ u8 field_0x011;     /* the attack timer  arms */
    /* +0x012 */ u8 unused_0x012[2];
    /* +0x014 */ u8 field_0x014;     /* the mode  maps to a two-state mask */
    /* +0x015 */ u8 unused_0x015[0x188 - 0x015];
    /* +0x188 */ nw4r::math::VEC3 pos;
    /* +0x194 */ u8 unused_0x194[0x1C8 - 0x194];
    /* +0x1C8 */ u32 field_0x1C8;    /* bit 0x80 suppresses `fn_8012C6F4` */
    /* +0x1CC */ u8 unused_0x1CC[0x1E0 - 0x1CC];
    /* +0x1E0 */ u8 field_0x1E0;     /* passed to `fn_802B0668` (map lookup) */
    /* +0x1E1 */ u8 area_no;         /* the area the enemy belongs to; `em_area_ck` compares it */
    /* +0x1E2 */ u8 field_0x1E2;
    /* +0x1E3 */ u8 field_0x1E3;
    /* +0x1E4 */ u8 unused_0x1E4;
    /* +0x1E5 */ u8 state;           /* activity state; `em_act_ck` matches the first parameter */
    /* +0x1E6 */ u8 state_sub;       /* `em_act_ck`'s second parameter */
    /* +0x1E7 */ u8 unused_0x1E7;
    /* +0x1E8 */ u8 field_0x1E8;
    /* +0x1E9 */ u8 field_0x1E9;
    /* +0x1EA */ u8 unused_0x1EA[0x1F5 - 0x1EA];
    /* +0x1F5 */ u8 field_0x1F5;
    /* +0x1F6 */ u8 unused_0x1F6[0x1F8 - 0x1F6];
    /* +0x1F8 */ u8 field_0x1F8;
    /* +0x1F9 */ u8 unused_0x1F9[0x38C - 0x1F9];
    /* +0x38C */ u8 field_0x38C;     /* bit 0 gates `fn_8012C870` */
    /* +0x38D */ u8 unused_0x38D[2];
    /* +0x38F */ u8 field_0x38F;     /* the per-attacker bit set `fn_8012C9AC` masks against */
    /* +0x390 */ s32 values_0x390[5]; /* index 4 is `fn_8012C204` */
    /* +0x3A4 */ s32 values_0x3A4[37]; /* the chosen attacker's damage, `fn_8012C20C` */
    /* +0x438 */ u8 unused_0x438[0x439 - 0x438];
    /* +0x439 */ u8 field_0x439;     /* the pending-attacker latch `fn_8012C9AC` clears */
    /* +0x43A */ u8 field_0x43A;     /* latched by `fn_8012C6DC` */
    /* +0x43B */ u8 unused_0x43B[2];
    /* +0x43D */ u8 field_0x43D;     /* 1 suppresses the latch */
    /* +0x43E */ u8 field_0x43E;     /* set when no attacker was chosen */
    /* +0x43F */ u8 unused_0x43F[0x784 - 0x43F];
    /* +0x784 */ u8 field_0x784;
    /* +0x785 */ u8 unused_0x785[0x794 - 0x785];
    /* +0x794 */ u16 field_0x794;    /* the per-attacker flag word */
    /* +0x796 */ s16 field_0x796;    /* the countdown `fn_8012CF2C` runs down */
    /* +0x798 */ u8 field_0x798;     /* the chosen attacker index, 0xFF when none */
    /* +0x799 */ u8 field_0x799;
    /* +0x79A */ u8 field_0x79A;
    /* +0x79B */ u8 field_0x79B;     /* "busy", cleared by `fn_8012C9AC` */
    /* +0x79C */ u8 unused_0x79C[0x916 - 0x79C];
    /* +0x916 */ s16 field_0x916;
    /* +0x918 */ u8 unused_0x918[0x954 - 0x918];
    /* +0x954 */ s32** field_0x954;  /* the state-name table `fn_8012CDF4` reads through */
    /* +0x958 */ s32 field_0x958;
    /* +0x95C */ u8 field_0x95C;
    /* +0x95D */ u8 field_0x95D;
    /* +0x95E */ u8 unused_0x95E[0x9DC - 0x95E];
    /* +0x9DC */ u8 field_0x9DC;     /* previous state, saved by `fn_8012CDF4` */
    /* +0x9DD */ u8 field_0x9DD;
    /* +0x9DE */ u8 unused_0x9DE[2];
    /* +0x9E0 */ s32 field_0x9E0;
    /* +0x9E4 */ u8 field_0x9E4;
    /* +0x9E5 */ u8 unused_0x9E5[0xA04 - 0x9E5];
    /* +0x0A04 */ u8 flags_0xA04;    /* bit 0 stun, bit 1 sleep, bit 2 the sleep result */
    /* +0x0A05 */ u8 field_0xA05;
    /* +0x0A06 */ u8 field_0xA06;
    /* +0x0A07 */ u8 field_0xA07;
    /* +0x0A08 */ u8 unused_0xA08[0xB18 - 0xA08];
};

/* ------------------------------------------------------------------------------------------------ *
 * The player-side work record (`get_move_work_adrs(0)`/`(2)`, 0xB20 apart).
 * ------------------------------------------------------------------------------------------------ */

/* size: 0xB20 */
struct _PLW {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 unused_0x001[0x008 - 0x001];
    /* +0x008 */ u8 field_0x008;     /* the slot `fn_8012D1A8` tests against player 0 */
    /* +0x009 */ u8 unused_0x009;
    /* +0x00A */ u8 field_0x00A;     /* 8 rejects the record in `fn_8012D0B4` */
    /* +0x00B */ u8 unused_0x00B[0x016 - 0x00B];
    /* +0x016 */ u8 field_0x016;     /* matched against the enemy's area in `fn_8012D0B4` */
    /* +0x017 */ u8 unused_0x017[0x1A4 - 0x017];
    /* +0x1A4 */ u8 field_0x1A4;     /* the second area byte `fn_8012D188` compares */
    /* +0x1A5 */ u8 unused_0x1A5[0xB20 - 0x1A5];
};

/* The root object `get_move_work_adrs(0)` returns - the per-player status bytes that
 * `fn_8012D1A8` reads.  It is not the 0xB20 record array; the index is a byte offset into this one
 * object.
 * size: 0x22FD (lower bound; the per-player array bound is approximate) */
struct _PLAYER_ROOT {
    /* +0x00000 */ u8 unused_0x00000[0x22DD];
    /* +0x022DD */ u8 player_state[32];
};

/* ------------------------------------------------------------------------------------------------ *
 * The enemy's own action tables.  `get_enemy_data(enemy)->field_0x0A0->field_0x024` points at a set of
 * three table pointers; each of the three is replaced by a built-in default when the enemy's data has
 * none (`lbl_805A0FF8`, `lbl_805A1034`, `lbl_805A106C`).
 * ------------------------------------------------------------------------------------------------ */

/* One action entry: two condition/value pairs, the value added to the attacker's damage when the
 * matching `Pl_*_condition_ck` passes.
 * size: 0x1C */
struct EnemyActionEntry {
    /* +0x000 */ u8 unused_0x000[0x00C];
    /* +0x00C */ s32 condition_0x00C;
    /* +0x010 */ s32 value_0x010;
    /* +0x014 */ s32 condition_0x014;
    /* +0x018 */ s32 value_0x018;
};

/* A whole action table; the entry the actor uses is at +0x08.
 * size: 0x0C */
struct EnemyActionTable {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ EnemyActionEntry* entry;
};

/* The three-table set the enemy carries.  It is written as an array so the built-in defaults
 * (`lbl_805A0FF8` and friends) share the type.
 * size: 0x0C */
struct EnemyActionSet {
    /* +0x000 */ EnemyActionTable* table_0x000;
    /* +0x004 */ EnemyActionTable* table_0x004;
    /* +0x008 */ EnemyActionTable* table_0x008;
};

/* The block at `get_enemy_data(enemy)->field_0x0A0`; only the action-set pointer is named.
 * size: 0x28 (lower bound) */
struct EnemyExtraData {
    /* +0x000 */ u8 unused_0x000[0x024];
    /* +0x024 */ EnemyActionSet* action_set;
};

/* The per-enemy data record `get_enemy_data` returns; only the two pointers this file reads are
 * named.
 * size: 0x0A4 (lower bound) */
struct EnemyData {
    /* +0x000 */ u8 unused_0x000[0x064];
    /* +0x064 */ u8* values;        /* the enemy's own value list (0-terminated) */
    /* +0x068 */ u8 unused_0x068[0x0A0 - 0x068];
    /* +0x0A0 */ EnemyExtraData* extra;
};

/* ------------------------------------------------------------------------------------------------ *
 * The global game-state block at 0x806BD360 (0x4A8 bytes in .bss); two bytes are read here.
 * ------------------------------------------------------------------------------------------------ */

/* size: 0x4A8 */
struct GameState {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 unused_0x001[0x171 - 0x001];
    /* +0x171 */ u8 field_0x171;
    /* +0x172 */ u8 unused_0x172[0x4A8 - 0x172];
};

/* ------------------------------------------------------------------------------------------------ *
 * Callees and pooled data.
 * ------------------------------------------------------------------------------------------------ */

extern "C" f32 fn_80050EAC(void* ref, nw4r::math::VEC3* pos);
extern "C" u8 fn_80133BCC(void);
extern "C" void fn_80133BB4(_ENEMY_WORK* enemy);
extern "C" u32 fn_801322CC(_ENEMY_WORK* enemy, s32 value);
extern "C" u32 fn_8013A884(_ENEMY_WORK* enemy, s32 value);
extern "C" u32 fn_8013A8B4(_ENEMY_WORK* enemy, s32 a, s32 b);
extern "C" u8 fn_8013A900(_ENEMY_WORK* enemy);
extern "C" s32 fn_8027AC18(void* arg);
extern "C" void* fn_8028EF7C(u16 id);
extern "C" void fn_80144584(s32 kind);
extern "C" u32 fn_8012B5C4(_ENEMY_WORK* enemy, u32 a, u32 b, u8 slot);
extern "C" f32 fn_80050EF4(void* ref, nw4r::math::VEC3* pos);
extern "C" f32 fn_80050F80(void* ref, nw4r::math::VEC3* pos);
extern "C" s32 fn_803A8858(void);
extern "C" s32 fn_803A87E0(void);
extern "C" u32 fn_8027BC48(s32 arg);
extern "C" u8 fn_8012B86C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind);
extern "C" u32 fn_8012D23C(_ENEMY_WORK* enemy, u32 kind, u32 slot);
extern "C" void* fn_8012D498(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 fn_8012D7FC(void* arg);
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 fn_8012D8D0(_ENEMY_WORK* enemy);
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" s32 fn_8012DB3C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DD58(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DE78(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DED8(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012DF68(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E040(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E0D8(u32 state, u32 action);
extern "C" s32 fn_8012E158(u32 state, u32 action);
extern "C" s32 fn_8012E21C(u32 state, u32 action);
extern "C" s32 fn_8012E2A8(u32 state, u32 sub);
extern "C" s32 fn_8012E2D4(u32 state, u32 action);
extern "C" s32 fn_8012E35C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E464(_ENEMY_WORK* enemy, u32 flag);
extern "C" s32 fn_8012E548(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E5A8(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags);
extern "C" s32 fn_8012E644(_ENEMY_WORK* record);
extern "C" s32 fn_8012E654(_ENEMY_WORK* record);
extern "C" void fn_8012E664(_ENEMY_WORK* enemy);
extern "C" void fn_8012E694(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E6A0(u32 kind, u16 id);
extern "C" u8 fn_8012E884(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E8C0(_ENEMY_WORK* enemy);
extern "C" void fn_8012E8DC(_ENEMY_WORK* record);
extern "C" s32 fn_8012E8F4(f32 seconds);
extern "C" void fn_8013AAC4(_ENEMY_WORK* enemy);
extern "C" void fn_80130A10(_ENEMY_WORK* enemy, s32 value);
extern "C" void fn_8012B8F8(_ENEMY_WORK* enemy);
extern "C" u8 fn_802B0668(u32 map_no);
extern "C" u32 fn_802B0688(void* pos);
extern "C" u8 fn_8028EF30(u32 value);
extern "C" u32 fn_8042CB9C(s32 value);
extern "C" s32 fn_8012B944(_ENEMY_WORK* enemy, u8 index, s32 value);
extern "C" void fn_8012B988(_ENEMY_WORK* enemy, s32 value);
extern "C" void fn_8012B9BC(_ENEMY_WORK* enemy, u8 index, s32 value);
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag);
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag);
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag);
extern "C" void fn_80130858(_ENEMY_WORK* enemy, s16 value);
extern "C" u32 fn_80130778(s32 kind);

/* Map-mangled callees, kept as the map spells them (see the signature note in the file header). */
extern "C" u8 get_now_areano__Fv(void);
extern "C" EnemyData* get_enemy_data__FP11_ENEMY_WORK(_ENEMY_WORK* enemy);
extern "C" _PLW* get_move_work_adrs__FUc(u8 kind);
extern "C" u16 get_move_work_max__FUc(u8 kind);
extern "C" u32 Pl_Skill_ck__FP4_PLWUs(_PLW* work, u16 skill);
extern "C" u32 Pl_condition_ck__FP4_PLWUl(_PLW* work, u32 condition);
extern "C" u32 Pl_dm_condition_ck__FP4_PLWUl(_PLW* work, u32 condition);

extern "C" GameState lbl_806BD360;
extern "C" EnemyActionTable* lbl_805A0FF8[4];
extern "C" EnemyActionTable* lbl_805A1034[3];
extern "C" EnemyActionTable* lbl_805A106C[3];

extern "C" f32 lbl_80796C58;
extern "C" f32 lbl_80796C90;
extern "C" f32 lbl_80796C94;

extern "C" s32 fn_8012BA00(_ENEMY_WORK* enemy, EnemyActionTable* table, void* work, s32 kind, u16 index);
extern "C" s32 fn_8012D0B4(_ENEMY_WORK* enemy, _PLW* work);
extern "C" s32 fn_8012D1A8(u32 index);
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work);
extern "C" u32 fn_8012D1A0(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012CF90(_ENEMY_WORK* enemy, u32 value);
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2);
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask);

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012BDF4
 * ------------------------------------------------------------------------------------------------ */

/* Scores every player-side work record against its action table and hands the result to
 * `fn_8012B944`; then, when the global state allows it, picks the enemy's target for `fn_8012B9BC`. */
extern "C" void fn_8012BDF4(_ENEMY_WORK* enemy)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    _PLW* work;
    _ENEMY_WORK* target;
    u16 max;
    u16 i;
    u16 enemy_max;
    u16 j;
    s32 value;

    set = get_enemy_data__FP11_ENEMY_WORK(enemy)->extra->action_set;
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    max = get_move_work_max__FUc(2);
    work = get_move_work_adrs__FUc(2);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && fn_8012D1A8(work->field_0x008) == 0) {
            value = fn_8012BA00(enemy, table, work, 1, i);
            if (value > 0) {
                if (Pl_Skill_ck__FP4_PLWUs(work, 0xC) == 1) {
                    value = (s32)((f32)value * lbl_80796C90);
                }
                if (Pl_Skill_ck__FP4_PLWUs(work, 0xB) == 1) {
                    value = (s32)((f32)value * lbl_80796C94);
                    if (value <= 0) {
                        value = 1;
                    }
                }
            }
            fn_8012B944(enemy, (u8)i, value);
        }
    }
    if (lbl_806BD360.field_0x000 != 0) {
        if (lbl_806BD360.field_0x171 != 5) {
            if (set == NULL || (table = set->table_0x004) == NULL) {
                table = lbl_805A1034[0];
            }
            value = fn_8012BA00(enemy, table, &lbl_806BD360, 2, 0);
        } else {
            value = -5;
        }
        fn_8012B988(enemy, value);
    }
    if (set == NULL || (table = set->table_0x008) == NULL) {
        table = lbl_805A106C[0];
    }
    enemy_max = get_move_work_max__FUc(3);
    target = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    for (j = 0; j < enemy_max; j++, target++) {
        if (enemy->group == j || target->active == 0 || enemy->team == target->team ||
            fn_8012CF90(enemy, target->team) == 1 || (u8)(target->state - 0x0B) <= 1) {
            enemy->values_0x3A4[j] = 0;
        } else {
            fn_8012B9BC(enemy, (u8)j, fn_8012BA00(enemy, table, target, 3, j));
        }
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C0EC
 * ------------------------------------------------------------------------------------------------ */

/* The attacker's damage against one player-side record: the stored value plus the two table
 * adjustments whose conditions the player passes. */
extern "C" s32 fn_8012C0EC(_ENEMY_WORK* enemy, u32 index)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    EnemyActionEntry* entry;
    _PLW* work;
    s32 value;

    work = get_move_work_adrs__FUc(2);
    set = get_enemy_data__FP11_ENEMY_WORK(enemy)->extra->action_set;
    value = enemy->values_0x390[(u8)index];
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    work += (u8)index;
    if (work->active != 0 && fn_8012D1A8(work->field_0x008) == 0 && fn_8012D0B4(enemy, work) == 1) {
        entry = table->entry;
        if (entry->condition_0x00C != 0 && Pl_condition_ck__FP4_PLWUl(work, entry->condition_0x00C) == 1) {
            value += entry->value_0x010;
        }
        if (entry->condition_0x014 != 0 && Pl_dm_condition_ck__FP4_PLWUl(work, entry->condition_0x014) == 1) {
            value += entry->value_0x018;
        }
    }
    if (value < 0) {
        value = 0;
    }
    return value;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C204, 0x8012C20C
 * ------------------------------------------------------------------------------------------------ */

/* The enemy's fourth per-attacker value. */
extern "C" s32 fn_8012C204(_ENEMY_WORK* enemy)
{
    return enemy->values_0x390[4];
}

/* The per-attacker value at `values_0x390[index + 5]`. */
extern "C" s32 fn_8012C20C(_ENEMY_WORK* enemy, u32 index)
{
    return enemy->values_0x3A4[(u8)index];
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C220
 * ------------------------------------------------------------------------------------------------ */

/* Claims the first enemy-side record of `team` that no other record's `state_sub` owns, marking it
 * with `flags_0xA04` bit 0 and returning 1; 0 when none is free. */
extern "C" s32 fn_8012C220(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->state != 0x0B && work->state != 0x0C && work->team == (u8)team &&
            work->area_no != (u8)state_sub) {
            if ((work->flags_0xA04 & 1) == 0) {
                work->flags_0xA04 |= 1;
                work->field_0xA05 = state_sub;
                work->field_0xA06 = 0;
            }
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C300
 * ------------------------------------------------------------------------------------------------ */

/* Sets `flags_0xA04` bit 1 on every enemy-side record of `team` whose area matches `state_sub` and
 * returns 1 when at least one was marked. */
extern "C" s32 fn_8012C300(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    found = 0;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->state != 0x0B && work->state != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            work->flags_0xA04 |= 2;
            found = 1;
        }
    }
    return found;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C3C8
 * ------------------------------------------------------------------------------------------------ */

/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`. */
extern "C" s32 fn_8012C3C8(u32 team, u32 state_sub, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    found = 0;
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->state != 0x0B && work->state != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                fn_80050EAC(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                found = 1;
            }
        }
    }
    return found;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C4E8
 * ------------------------------------------------------------------------------------------------ */

/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`, and hands each one to `fn_80130858`. */
extern "C" void fn_8012C4E8(u32 team, u32 state_sub, s16 arg3, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->state != 0x0B && work->state != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                fn_80050EAC(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                fn_80130858(work, arg3);
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C600
 * ------------------------------------------------------------------------------------------------ */

/* Runs the pending bits of `flags_0xA04` down: the sleep flag (bit 1) is cleared, and the stun flag
 * (bit 0) counts one frame before it is released. */
extern "C" void fn_8012C600(_ENEMY_WORK* enemy)
{
    if (enemy->flags_0xA04 != 0) {
        if ((enemy->flags_0xA04 & 2) != 0) {
            fn_80130778(1);
            enemy->flags_0xA04 &= 0xFD;
        }
        if ((enemy->flags_0xA04 & 1) != 0) {
            if (enemy->field_0xA06 == 0) {
                enemy->field_0xA06 = enemy->field_0xA06 + 1;
                fn_8012CE8C(enemy, 8);
            } else if (enemy->area_no == enemy->field_0xA05) {
                enemy->flags_0xA04 &= 0xFE;
                enemy->field_0xA05 = 0xFF;
                enemy->field_0xA06 = 0;
                fn_8012CE9C(enemy, 8);
            }
        }
        if ((enemy->flags_0xA04 & 4) != 0) {
            enemy->field_0xA07 = 1;
            enemy->flags_0xA04 &= 0xFB;
        }
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C6DC
 * ------------------------------------------------------------------------------------------------ */

/* Latches the "hit by" byte unless the enemy is already in the state that consumes it. */
extern "C" void fn_8012C6DC(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x43D != 1) {
        enemy->field_0x43A = 1;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C6F4
 * ------------------------------------------------------------------------------------------------ */

/* Marks the enemy as being hit for the given attacker: clears `field_0x798` and, when `mask` names
 * any live attacker, picks the last one whose bit is set, then runs the hit state machine. */
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2)
{
    _PLW* work;
    u16 max;
    u8 i;

    if ((enemy->field_0x1C8 & 0x80) != 0) {
        return 0;
    }
    if (enemy->field_0x95C == 5) {
        return 0;
    }
    fn_80130778(1);
    if (fn_8013A884(enemy, 5) == 1) {
        max = get_move_work_max__FUc(2);
        work = get_move_work_adrs__FUc(2);
        enemy->field_0x798 = 0xFF;
        if ((u8)mask != 0xFF) {
            for (i = 0; i < max; i++, work++) {
                if (work->active != 0 && fn_8012D1A8(work->field_0x008) == 0 &&
                    ((u8)mask & (1 << i)) != 0 && fn_8012D0B4(enemy, work) != 0) {
                    enemy->field_0x798 = i;
                    break;
                }
            }
        }
        if (arg2 == 0) {
            fn_8012CDF4(enemy, 5, 0);
        } else if (fn_8013A8B4(enemy, 5, 1) == 1) {
            fn_8012CDF4(enemy, 5, 1);
        } else {
            fn_8012CDF4(enemy, 5, 0);
        }
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012C870
 * ------------------------------------------------------------------------------------------------ */

/* The knock-down variant of `fn_8012C6F4`: gated on the shell timer, it picks the last live attacker
 * out of `mask` and enters the knock-down state. */
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask)
{
    _PLW* work;
    u16 max;
    u8 i;

    if (enemy->field_0x95C == 4) {
        return 0;
    }
    if (enemy->field_0x916 > 0) {
        return 0;
    }
    if ((enemy->field_0x38C & 1) == 0) {
        return 0;
    }
    fn_80130778(2);
    if (fn_8013A884(enemy, 4) == 1) {
        max = get_move_work_max__FUc(2);
        work = get_move_work_adrs__FUc(2);
        enemy->field_0x798 = 0xFF;
        for (i = 0; i < max; i++, work++) {
            if (work->active != 0 && fn_8012D1A8(work->field_0x008) == 0 &&
                ((u8)mask & (1 << i)) != 0 && fn_8012D0B4(enemy, work) != 0) {
                enemy->field_0x798 = i;
                break;
            }
        }
        fn_8012CDF4(enemy, 4, 0);
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012CDF4
 * ------------------------------------------------------------------------------------------------ */

/* Switches the enemy to the given state pair, saving the previous state and its label first. */
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    enemy->field_0x9DC = enemy->field_0x95C;
    enemy->field_0x9DD = enemy->field_0x95D;
    enemy->field_0x9E0 = enemy->field_0x958;
    enemy->field_0x9E4 = fn_80133BCC();
    enemy->field_0x95C = state;
    enemy->field_0x95D = sub;
    enemy->field_0x958 = enemy->field_0x954[(u8)state][(u8)sub];
    enemy->field_0x79A = 1;
    fn_80133BB4(enemy);
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012CE8C, 0x8012CE9C, 0x8012CEB4, 0x8012CF04, 0x8012CF20
 * ------------------------------------------------------------------------------------------------ */

/* Sets the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag)
{
    enemy->field_0x794 |= flag;
}

/* Clears the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag)
{
    enemy->field_0x794 &= ~flag;
}

/* Arms the "hit by" timer and remembers which attack it was, then marks the state word. */
extern "C" void fn_8012CEB4(_ENEMY_WORK* enemy, s16 timer, u8 index)
{
    fn_8012CE8C(enemy, 2);
    enemy->field_0x796 = timer;
    enemy->field_0x799 = index;
}

/* Whether any of the given bits is set in the per-attacker flag word. */
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag)
{
    return (enemy->field_0x794 & flag) != 0;
}

/* Marks the enemy busy for this frame. */
extern "C" void fn_8012CF20(_ENEMY_WORK* enemy)
{
    enemy->field_0x79B = 1;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012CF2C
 * ------------------------------------------------------------------------------------------------ */

/* Runs the "hit by" timer down and reports 1 on the frame it expires. */
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy)
{
    s16 timer;

    if (fn_8012CF04(enemy, 2) == 1) {
        timer = enemy->field_0x796 - 1;
        enemy->field_0x796 = timer;
        if (timer <= 0) {
            enemy->field_0x796 = 0;
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012CF90
 * ------------------------------------------------------------------------------------------------ */

/* Whether the enemy's own value list contains `value` (0x28 is always a hit). */
extern "C" s32 fn_8012CF90(_ENEMY_WORK* enemy, u32 value)
{
    u8* list = get_enemy_data__FP11_ENEMY_WORK(enemy)->values;

    if ((u8)value == 0x28) {
        return 1;
    }
    if (list != NULL) {
        while (*list != 0) {
            if (*list == (u8)value) {
                return 1;
            }
            list++;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012D004, 0x8012D02C, 0x8012D074, 0x8012D188, 0x8012D1A0, 0x8012D1A8, 0x8012D20C
 * ------------------------------------------------------------------------------------------------ */

/* Whether the enemy record is still alive: an inactive record, or one whose death flag has run past
 * 2, counts as dead. */
extern "C" s32 em_work_die_ck__FP11_ENEMY_WORK(_ENEMY_WORK* enemy)
{
    if (enemy->active != 0 && enemy->field_0x004 < 2) {
        return 0;
    }
    return 1;
}

/* Whether the enemy is in its death state. */
extern "C" s32 em_die_ck__FP11_ENEMY_WORK(_ENEMY_WORK* enemy)
{
    if (enemy->state == 0x0B || em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xC, 0xFF) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the area the game is currently in. */
extern "C" s32 em_area_ck__FP11_ENEMY_WORK(_ENEMY_WORK* enemy)
{
    return get_now_areano__Fv() == enemy->area_no;
}

/* Whether the enemy and the given work record share an area. */
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work)
{
    return work->field_0x1A4 == enemy->area_no;
}

/* The enemy's activity byte. */
extern "C" u32 fn_8012D1A0(_ENEMY_WORK* enemy)
{
    return enemy->field_0x1F5;
}

/* Whether the given player slot is in the "out of action" state; only player 0 is looked at. */
extern "C" s32 fn_8012D1A8(u32 index)
{
    _PLAYER_ROOT* root;

    if (fn_8042CB9C(index) == 1) {
        root = (_PLAYER_ROOT*)get_move_work_adrs__FUc(0);
        if (root != NULL && root->player_state[(u8)index] == 4) {
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the given state pair. */
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    if (enemy->state == (u8)state && enemy->state_sub == (u8)sub) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012D0B4
 * ------------------------------------------------------------------------------------------------ */

/* Whether the enemy may attack the given work record: same area, not already out, and past the
 * map-specific position gate. */
extern "C" s32 fn_8012D0B4(_ENEMY_WORK* enemy, _PLW* work)
{
    if (fn_8012D1A8(work->field_0x008) == 1) {
        return 0;
    }
    if (work->field_0x00A == 8) {
        return 0;
    }
    if (enemy->area_no == work->field_0x016) {
        if (fn_802B0668(enemy->field_0x1E0) == 9) {
            if (fn_8028EF30(work->field_0x008) == 0) {
                if (fn_802B0688(&enemy->pos) == 1) {
                    return 1;
                }
            } else if (fn_802B0688(&enemy->pos) == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012D23C is not written yet (420 bytes, a two-level switch over the state byte and the state_sub
 * timer).  Its per-slot predicate `fn_8012D3E0` and the search helpers below are.
 * ------------------------------------------------------------------------------------------------ */

/* The first attacker slot whose per-kind predicate `fn_8012D23C` accepts the enemy. */
extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind)
{
    u16 max;
    s32 i;

    max = get_move_work_max__FUc(2);
    for (i = 0; i < max; i++) {
        if (fn_8012D23C(enemy, kind, (u8)i) == 1) {
            return (u8)i;
        }
    }
    return 0xFF;
}

/* Whether the search helper finds any record. */
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag)
{
    return fn_8012D498((u8)selector, work, out_flag) != NULL;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012D7FC, 0x8012D850, 0x8012D878, 0x8012D8A4, 0x8012D8D0
 * ------------------------------------------------------------------------------------------------ */

/* Whether the actor has the "charge" skill, or the pair is on its last frame. */
extern "C" s32 fn_8012D7FC(void* arg)
{
    if (Pl_Skill_ck__FP4_PLWUs((_PLW*)arg, 0xCD) == 1) {
        return 1;
    }
    return (fn_8027AC18(arg) - 1) == 0;
}

/* Whether the enemy is in the low part of its action state. */
extern "C" s32 fn_8012D850(_ENEMY_WORK* enemy)
{
    if (enemy->state == 0x0A && enemy->state_sub <= 0x12) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the mid-range part of its action state. */
extern "C" s32 fn_8012D878(_ENEMY_WORK* enemy)
{
    if (enemy->state == 0x0A && (u32)(enemy->state_sub - 0x58) <= 0x1F) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the high part of its action state. */
extern "C" s32 fn_8012D8A4(_ENEMY_WORK* enemy)
{
    if (enemy->state == 0x0A && (u32)(enemy->state_sub - 0xC3) <= 0x0F) {
        return 1;
    }
    return 0;
}

/* The "analysable" action-state list `ana_em_ck` accepts. */
extern "C" s32 fn_8012D8D0(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBB) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x0F) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1C) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x24) == 1) {
        return 1;
    }
    return 0;
}

/* The "paralysed" action-state list. */
extern "C" s32 fn_8012DB3C(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xC0) == 1) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------ *
 * The "analysable enemy at position" and "paralysed enemy at position" searches.
 * ------------------------------------------------------------------------------------------------ */

/* Whether one record is an analysable enemy of the given area within `radius` of `pos`. */
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if ((u8)flag == 0 && enemy->state == 0x0B) {
        return 0;
    }
    if (fn_8012D8D0(enemy) == 0) {
        return 0;
    }
    return fn_80050F80(pos, &enemy->pos) <= radius;
}

/* The first analysable enemy of the given area within `radius` of `pos`. */
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    for (i = 0; i < max; i++, work++) {
        if (ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos, (u8)flag,
                                                                radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* Whether one record is a paralysed enemy of the given area within `radius` of `pos`. */
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if (fn_8012DB3C(enemy) == 0 &&
        ((u8)flag == 0 || (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1A) == 0 &&
                           em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1B) == 0))) {
        return 0;
    }
    if (enemy->field_0x1E2 == 2) {
        if (fn_80050EF4(pos, &enemy->pos) <= radius) {
            return 1;
        }
    } else if (fn_80050F80(pos, &enemy->pos) <= radius) {
        return 1;
    }
    return 0;
}

/* The first paralysed enemy of the given area within `radius` of `pos`. */
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs__FUc(3);
    max = get_move_work_max__FUc(3);
    for (i = 0; i < max; i++, work++) {
        if (shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos,
                                                                    (u8)flag, radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------------ *
 * The action-state list predicates.
 * ------------------------------------------------------------------------------------------------ */

/* The "sleeping" action-state list. */
extern "C" s32 fn_8012DD58(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE8) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEE) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xED) == 1) {
        return 1;
    }
    return 0;
}

/* The two-state action list shared with `fn_8012DE78`. */
extern "C" s32 fn_8012DE78(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF2) == 1) {
        return 1;
    }
    return 0;
}

/* The "waking up" action-state list. */
extern "C" s32 fn_8012DED8(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF5) == 1) {
        return 1;
    }
    return 0;
}

/* The "stunned" action-state list. */
extern "C" s32 fn_8012DF68(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xAF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB0) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB2) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB5) == 1) {
        return 1;
    }
    return 0;
}

/* Three-way action-state query: 0/1 for the two states it knows, 0xFF otherwise. */
extern "C" s32 fn_8012E040(_ENEMY_WORK* enemy)
{
    switch (enemy->state) {
    case 0x0A:
        if ((u32)(enemy->state_sub - 0xE6) <= 3) {
            return 0;
        }
        if ((u32)(enemy->state_sub - 0xEC) <= 3) {
            return 1;
        }
        if (enemy->state_sub == 0xE4) {
            return 0;
        }
        if (enemy->state_sub == 0xEA) {
            return 1;
        }
        return 0xFF;
    case 0x0B:
        if (enemy->state_sub == 0x34 || enemy->state_sub == 0x36) {
            return 0;
        }
        if (enemy->state_sub == 0x35 || enemy->state_sub == 0x37) {
            return 1;
        }
        return 0xFF;
    }
    return 0xFF;
}

/* ------------------------------------------------------------------------------------------------ *
 * The per-state action id tests.
 * ------------------------------------------------------------------------------------------------ */

/* Whether the action id is one of the "frontal" attacks. */
extern "C" s32 fn_8012E0D8(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7C:
        case 0x82:
        case 0x8B:
        case 0x95:
        case 0x9C:
        case 0xBB:
        case 0xE8:
        case 0xEE:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "rear" attacks. */
extern "C" s32 fn_8012E158(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7A:
        case 0x7B:
        case 0x80:
        case 0x81:
        case 0x89:
        case 0x8A:
        case 0x93:
        case 0x94:
        case 0x9A:
        case 0x9B:
        case 0xB9:
        case 0xBA:
        case 0xE6:
        case 0xE7:
        case 0xEC:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "side" attacks. */
extern "C" s32 fn_8012E21C(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7B:
        case 0x81:
        case 0x8A:
        case 0x94:
        case 0x9B:
        case 0xBA:
        case 0xE7:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the three-state part of the second action state. */
extern "C" s32 fn_8012E2A8(u32 state, u32 sub)
{
    if ((u8)state == 0xB && (u32)((u8)sub - 0x1A) <= 2) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "leap" attacks. */
extern "C" s32 fn_8012E2D4(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x83:
        case 0x8C:
        case 0x96:
        case 0x9D:
        case 0xBC:
        case 0xE9:
        case 0xEF:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "roar" attacks. */
extern "C" s32 fn_8012E35C(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x84) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x85) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x86) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x87) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x88) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x97) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x98) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x99) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x9E) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "ball" attacks of the given flavour. */
extern "C" s32 fn_8012E464(_ENEMY_WORK* enemy, u32 flag)
{
    if ((u8)flag == 0) {
        if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD6) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD7) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD8) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD9) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDA) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDB) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDC) == 1) {
            return 1;
        }
    }
    return 0;
}

/* The two-state action list around the "poison" attacks. */
extern "C" s32 fn_8012E548(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBD) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBE) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the four-state part of the second action state. */
extern "C" s32 fn_8012E5A8(_ENEMY_WORK* enemy)
{
    if (enemy->state == 0x0B && (u32)(enemy->state_sub - 0x10) <= 3) {
        return 1;
    }
    return 0;
}

/* Per-actor hit/guard flags: 0x22..0x27 want the "guard" bit, 0x1B wants exactly 3. */
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags)
{
    if ((u32)((u8)actor - 0x24) <= 3) {
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    }
    switch ((u8)actor) {
    case 0x22:
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    case 0x1B:
        if (flags == 3) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* The hit test for the actor/mode pair the record carries. */
extern "C" s32 fn_8012E644(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->team, record->field_0x00A);
}

/* The hit test for the record's secondary actor/mode pair. */
extern "C" s32 fn_8012E654(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->group, record->team);
}

/* Arms the attack timer and, when it was not already armed, tells the attack system about it. */
extern "C" void fn_8012E664(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x011 == 0) {
        enemy->field_0x011 = 1;
        if ((enemy->field_0x1C8 & 8) == 0) {
            fn_80144584(3);
        }
    }
}

/* Marks the attack timer as expired. */
extern "C" void fn_8012E694(_ENEMY_WORK* enemy)
{
    enemy->field_0x011 = 2;
}

/* Whether the given actor id has a live hit record. */
extern "C" s32 fn_8012E6A0(u32 kind, u16 id)
{
    u8* entry;

    if (fn_8027BC48(0) == 1) {
        return 0;
    }
    entry = (u8*)fn_8028EF7C(id);
    if (entry == NULL || (s16)((u16*)entry)[2] <= 0) {
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------ *
 * 0x8012E884, 0x8012E8C0, 0x8012E8DC, 0x8012E8F4
 * ------------------------------------------------------------------------------------------------ */

/* Whether the enemy is in the "charge" state pair. */
extern "C" u8 fn_8012E884(_ENEMY_WORK* enemy)
{
    return (u8)((fn_8012E2A8(enemy->state, enemy->state_sub) - 1) == 0);
}

/* The two-state mask the record's mode byte selects. */
extern "C" s32 fn_8012E8C0(_ENEMY_WORK* enemy)
{
    return (enemy->field_0x014 == 2) ? 2 : 0;
}

/* Clears the "attack finished" byte once it has been consumed. */
extern "C" void fn_8012E8DC(_ENEMY_WORK* record)
{
    if (record->field_0x008 == 5) {
        record->field_0x008 = 0;
    }
}

/* Whether more than `seconds` have passed since the last frame stamp. */
extern "C" s32 fn_8012E8F4(f32 seconds)
{
    s32 now = fn_803A8858();

    return (f32)(now - fn_803A87E0()) > seconds;
}
