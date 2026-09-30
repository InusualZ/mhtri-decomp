/* auto/801251D0_fn_801251D0.cpp - the enemy control unit, 43 function(s) so far, `.text`
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * 0x801251D0..0x8012BA00 (26672 bytes, 145 symbols).
 *
 * What it is.  The enemy-side work block's taken/seated-state bookkeeping: the small accessors that
 * flip the `_ENEMY_WORK` flags around an action change (`fn_801251E0`/`fn_801252C0`), the per-part
 * helpers, and `get_enemy_data`, the accessor that turns an enemy's group + kind into its static data
 * record (`fn_80140C00`).  The large movers (`em_act_advance`, 2552 B; `em_target_pos_set`, 1860 B;
 * `fn_8012A9E8`, 2456 B) are the unit's update/state-machine bodies; the rest are the getters and
 * setters that surround them.  The .data pool the unit references (`lbl_805A1ADC`,
 * `lbl_807919D0`, the jump tables) belongs to the data pass and is declared, never defined
 * (playbook 29).
 *
 * Naming evidence (brief section 2, in order).  1. No `__FILE__` string is reachable from this range:
 * a scan of every `lis`+`addi` pair in 0x801251D0..0x8012BA00 resolves to no bare source-file name
 * (the nearest, `enemy_control.cpp` at 0x805A1BB8, is referenced only by 0x801411DC/0x80141258, i.e.
 * `enemy/fn_8013BE60.c` - not this range).  2. `dumpmap.py lookup` gives no real dump name for any
 * symbol here.  3. The neighbours' scheme is the map's own `fn_XXXXXXXX` stem
 * (`enemy/fn_8012BA00.c`, `enemy/fn_8012BDF4.cpp`), and the only real name, the mangled
 * `get_enemy_data__FP11_ENEMY_WORK` defined here, is honoured at C++ scope.  The file therefore keeps
 * the map stem (brief option 4); no name was invented.
 *
 * Language.  The object defines one mangled symbol (`get_enemy_data__FP11_ENEMY_WORK`) and references
 * mangled callees, so the TU is C++ (langcheck's `mangled-defined` evidence).  Every plain `fn_`
 * symbol is defined `extern "C"` so it keeps the map's name; `get_enemy_data` is defined at C++ scope
 * and reaches its mangling (rule 9).
 *
 * Status / residuals.  43 bodies written; 42 measure 100 % and `fn_8012B86C` measures 97.6 %
 * (a register-colouring difference: retail keeps the mask in r6 and the shift count in r5, this
 * build swaps the two; every instruction and branch is otherwise right).  The remaining 102 symbols
 * are unwritten and keep their original bytes in the target object; the large bodies
 * (`em_act_advance`, `em_target_pos_set`, `fn_8012A9E8`, ...) need the unit's full state-machine layout, which
 * the written accessors only partly pin.
 *
 * Source shape worth keeping: the unit needs `#pragma peephole off`.  With the peephole pass on,
 * the fused `clrlwi`+`slwi` and the `extsb`+`stb` pairs retail keeps are folded away (that is worth
 * `fn_80127CB0` 68 %, `fn_801269E8` 96.8 %, `fn_801252C0` 85 %); with the pass off every one of
 * those is byte-identical.  Two functions (`fn_80128590`, `fn_801285A0`) take `u8`/`u16` parameters
 * so their stores stay unmasked - with `u32` parameters the peephole-off build re-materialises the
 * `clrlwi` before each `stb`/`sth`.
 *
 * `_ENEMY_WORK` (0xB1C) is the shared record `include/enemy.h` owns; this unit adds the fields its
 * accessors read (`timer_0x18`, `field_0x60`, `field_0x1F4`, `state_0x9FA`) by splitting that header's
 * padding, never a second definition.
 */

#include "types.h"
#include "hud/em_net_send.h" /* the owner's leaf header (rule 2) */

#include "nw4r/math.h"
#include "enemy.h"
#include "enemy/fn_801251D0.h"
#include "unsplit/enemy.h"
#include "unsplit/enemy_pool.h" /* the band's unowned .data pools (rule 2) */

#pragma peephole off

/* ---------------------------------------------------------------------------------------------- *
 * Callees outside this unit.
 * ---------------------------------------------------------------------------------------------- */

/* `fn_80124C5C` (0x80124C5C, 0x574 B) sits in the unclaimed run 0x80119DEC..0x801251D0; its
 * bracketing *registered* units are `ef/fn_80119C44.c` and `enemy/fn_8012BA00.c`, different modules,
 * so rule 2 has no sound header to move it to and the declaration stays in the consumer
 * (docs/plan.md 6.5 rule 2, the named gap). */
extern "C" void fn_80124C5C(u32 a, u32 b, u8 c);

/* The unit's own next symbols, defined in this TU. */
extern "C" void em_hit_window_set(struct _ENEMY_WORK* self, u8 a, u32 b, u32 c);
extern "C" u32 stage_map_kind_get(u32 kind);

/* `em_net_send` (0x8033737C) sits in the unclaimed run 0x803250B0..0x8033737C+: its bracketing
 * *registered* units are `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`, different modules,
 * so rule 2 has no sound header (the named gap). */

/* Pool literals (declared, never defined - playbook 29). */
extern "C" u8 lbl_807919D0;

/* ---------------------------------------------------------------------------------------------- *
 * The written bodies, in address order.
 * ---------------------------------------------------------------------------------------------- */

extern "C" void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    fn_80124C5C((u32)self, (u32)tbl, (u8)a);
}

extern "C" void em_se_tbl_play_alt(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    fn_80124C5C((u32)self, (u32)tbl, (u8)a);
}

extern "C" void fn_801252C0(struct _ENEMY_WORK* self, u8 a)
{
    self->field_0x1F6 = (s8)a;
    self->field_0x1F5 = 0;
    em_net_send(self, 2, 0);
}

extern "C" void fn_8012554C(struct _ENEMY_WORK* self)
{
    self->state_0x9F9 = 0;
    self->state_0x9FA = 0;
}

extern "C" u8 fn_80125F88(u32 idx)
{
    return lbl_805A1ADC[(u8)idx];
}

extern "C" u32 enemy_kind_same_ck(u32 a, u32 b)
{
    return fn_80125F88((u8)a) == fn_80125F88((u8)b);
}

extern "C" u32 fn_80125FF0(u32 a, u32 b)
{
    return (u8)stage_map_kind_get((u8)a) == (u8)b;
}

extern "C" u8* fn_80126044(struct _ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    u8* result = fn_8014260C(data->field_0x0E);
    if (result != NULL) {
        return result + self->field_0x7c8 * 0x1C;
    }
    return lbl_805A1078;
}

extern "C" s32 fn_80126098(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x14;
}

extern "C" s32 fn_801260BC(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x1C;
}

extern "C" s32 fn_801260E0(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x20;
}

extern "C" s32 fn_80126104(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x24;
}

EnemyData* get_enemy_data(struct _ENEMY_WORK* work)
{
    return fn_80140C00(work->team, work->field_0x00A);
}

extern "C" void* fn_80126704(struct _ENEMY_WORK* self)
{
    if (self->field_0x60 != NULL) {
        return self->field_0x60;
    }
    return &lbl_807919D0;
}

extern "C" void fn_80126898(struct _ENEMY_WORK* self)
{
    if (self->timer_0x18 > 0) {
        self->timer_0x18--;
    }
}

/* A 4-byte table entry `fn_80127568` indexes: a value word with the payload at +0x2. */
struct _EM_REC4 {
    /* +0x0 */ u16 field_0x0;
    /* +0x2 */ u16 field_0x2;
}; /* size: 0x4 */

extern "C" u32 fn_801269E8(u32* table, u32 a, u32 b)
{
    if ((u8)a == (u8)b) {
        return 1;
    }
    if ((u8)a == 255 || (u8)b == 255) {
        return 0;
    }
    s8* row = (s8*)table[(u8)a];
    if (row == NULL) {
        return 0;
    }
    return row[(u8)b] != -1;
}

extern "C" u16 fn_80127568(void* unused, struct _EM_REC4* table, u32 idx)
{
    if (table == NULL) {
        return 3;
    }
    return table[(u8)idx].field_0x2;
}

extern "C" u32 fn_80127CB0(struct _ENEMY_WORK* self, u32 idx)
{
    return self->values_0x868[(u8)idx];
}

extern "C" void fn_80128590(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->field_0x1EE = a;
    self->field_0x1EF = b;
    self->field_0x1F0 = c;
}

extern "C" void fn_801285A0(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->state_0x05 = 0;
    self->phase_0x06 = 0;
    self->step_0x07 = 0;
    self->action_0x1E5 = a;
    self->state_sub = b;
    self->bits_0x1EC = c;
}

extern "C" void fn_801281EC(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 1;
}

extern "C" void fn_801281F8(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 0;
}

extern "C" u32 fn_80128204(struct _ENEMY_WORK* self)
{
    return self->field_0x1F4;
}

extern "C" void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_hit_window_set(self, (u8)a, b, 0);
}

extern "C" void fn_80129864(struct _ENEMY_WORK* self)
{
    if (self->value_0x452 < 27000) {
        self->value_0x452++;
    }
}

extern "C" void fn_80129984(struct _ENEMY_WORK* self)
{
    self->state_0x1FC = 0;
    self->state_0x1FE = 0;
    self->state_0x1FF = 255;
}

extern "C" u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c)
{
    if (self->special_type_0x380 == (u8)a) {
        if (self->state_0x381 == (u8)b) {
            if (self->special_part_0x382 == (u8)c) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void fn_8012B604(void)
{
}

/* ---------------------------------------------------------------------------------------------- *
 * The status/motion accessors (0x80128A14..0x8012B9FC).
 * ---------------------------------------------------------------------------------------------- */

extern "C" void fn_80128998(struct _ENEMY_WORK* self, u8 a, u8 b);
extern "C" void em_act_step_arm(struct _ENEMY_WORK* self, u8 a, u8 b, u32 c);

extern "C" void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    fn_80128998(self, (u8)a, (u8)b);
}

extern "C" void fn_80128A30(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_act_step_arm(self, (u8)a, (u8)b, 1);
    fn_801281EC(self);
}

extern "C" void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    fn_80128A30(self, (u8)a, (u8)b);
}

extern "C" void fn_80128A8C(struct _ENEMY_WORK* self, u8 a, u8 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    em_act_step_arm(self, a, b, 1);
}

extern "C" void fn_80128AAC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 2);
}

extern "C" void fn_80128ACC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 5);
}

extern "C" void fn_80128AEC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action_0x1E5 == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 0);
}

extern "C" void fn_801299D8(struct _ENEMY_WORK* self)
{
    if (--self->field_0x200 <= 0) {
        self->field_0x200 = 0;
    }
    if (--self->field_0x202 <= 0) {
        self->field_0x202 = 0;
    }
}

extern "C" u32 fn_80129A1C(void* unused, u32 a, u32 b, s16* timer)
{
    if (*timer <= 0) {
        if ((u16)a % 100 < (u16)b) {
            return 1;
        }
        *timer = 900;
    }
    return 0;
}

extern "C" u32 fn_8012A204(struct _ENEMY_WORK* self)
{
    if (self->value_0x44C <= 0
        || (self->timed_0x43D != 1 && self->value_0x452 > self->value_0x44E)) {
        self->state_0x1FC = 1;
        self->state_0x1FE = 2;
        self->state_0x1FF = 255;
        return 1;
    }
    return 0;
}

extern "C" u32 fn_8012B86C(struct _ENEMY_WORK* self)
{
    u8 mask = 0;
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x428[i] >= 20000) {
            mask |= 1 << i;
        }
    }
    return mask;
}

extern "C" void fn_8012B8F8(struct _ENEMY_WORK* self)
{
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x428[i] >= 20000) {
            self->values_0x428[i] = 0;
        }
    }
}

extern "C" void fn_8012B944(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x390[(u8)a] += b;
    v = self->values_0x390[(u8)a];
    if (v > 20000) {
        self->values_0x390[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[(u8)a] = 0;
    }
}

extern "C" void fn_8012B988(struct _ENEMY_WORK* self, s32 b)
{
    s32 v = self->values_0x390[4] + b;
    self->values_0x390[4] = v;
    if (v > 20000) {
        self->values_0x390[4] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[4] = 0;
    }
}

extern "C" void fn_8012B9BC(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x3A4[(u8)a] += b;
    v = self->values_0x3A4[(u8)a];
    if (v > 20000) {
        self->values_0x3A4[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x3A4[(u8)a] = 0;
    }
}
