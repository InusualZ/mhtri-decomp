/* Pl/pl_act_step.cpp - the player action step set: the second motion -> SE frame dispatcher, the action/handler
 *   cluster, the per-act step handlers the act dispatcher tail-calls, the act state-machine band and the per-frame
 *   and main control clusters.
 * RANGE. .text 0x802430E8-0x802673A4 (286 functions); .ctors 0x8056F368, .data 0x805C4134-0x805C5E58, .bss
 *   0x806AB3E0-0x806AB410, .sdata 0x80792040-0x80792068, .sbss 0x80794B20-0x80794B28, .sdata2 0x80799E00-0x8079A000
 *   (the band's frame-window pool: `Pl/pl_frame_data.h` and `Pl/pl_act_data.h` declare it), extab, extabindex.
 *   `fn_802430E8` switches on `Get_motion_no` through the 261-entry `jumptable_805C4134` (0x805C4134-0x805C4548); 52 of
 *   its 59 arms are `Pl/fn_80241558.cpp`'s.  The act dispatcher 0x80251B88 indexes `jumptable_805C4AE0` with
 *   `_PLW`+0x00C and tail-calls one step handler per act id; each handler advances the act's step byte `_PLW`+0x005.
 * NAMES. The file name is a GUESS (the per-act step handlers).  GUESSes read from each body: `pl_act_step_<n>` is the
 *   handler the dispatcher table gives act `n` (`pl_act_step_84`, `_86`, `_89`, `_94`, `_121`, `_135`, `_151`, `_175`);
 *   `pl_act_step_attr_<n>` (`_112`, `_1007`, `_1056`, `_1018`) the three-step weapon-act handlers, after the attribute
 *   their arming step passes `Pl_chr_set_attr_default` (`fn_802564B0`'s `jumptable_805C4EB0` reaches the last three);
 *   `pl_act_step_offhand_gesture` (the act entered from an off-hand gesture), `pl_act_step_pitfall_arm`/`_hold` (the
 *   pitfall act's arming and follow-up steps), `pl_act_armed_motion_count` (how many of the three armed motion words
 *   agree), `pl_act_guard_timer_reset` (zeroes two guard timers, re-arms the third at 30 frames),
 *   `pl_act_guard_gauge_adjust` (steps the guard gauge `_PLW`+0x0AC and clamps it), `pl_act_arm_motion_and_flag`
 *   (shared by the eleven weapon-act handlers above it), `pl_act_charge_repeat_step` (one repeat of a six-part charge
 *   motion), `pl_act_countdown_step` (arms attribute 26 with a 40/80-frame countdown), `pl_act_step_timer_wait` (arms
 *   and counts down the act's 15-frame `+0x028` timer), `pl_act_gauge_gate_by_skill` (picks the guard gauge band from
 *   cat skill 20); the runtime dump answers placeholders for both.
 *   GUESS (from each body): pl_item_usable_ck (any of the 26 pouch slots holds a usable item), pl_item_slot_cycle (the
 *   next or previous such slot), pl_act_stagger_fill (raises the stagger flag and refills the stagger value),
 *   `pl_item_window_blocked` (an inline helper: the actor kinds and acts that keep the item window shut).
 *   From the callers: `pl_area_entry_set` (`enemy/em_pop.cpp` places the player at an area entry with it),
 *   `pl_act_step_arm_table` (the step table it arms, `Pl/pl_act_step_data.h`), `pl_act_net_hook_a`/`_b`/`_c` (the
 *   three receivers `hud/pl_frame_sync.cpp` replays, declared in `Pl/fn_802489D4.h`).
 *   GUESS (from each body): `pl_frame_post_step` (the per-frame tail: drift, wide-area effects, lobby events and the
 *   net report), `pl_condition_tick` (poison, the second damage timer and the hot/cold area drain),
 *   `pl_scan_row_input_ck` (a step-table row's input condition), `pl_item_use_act_pick` (the act a pouch item enters),
 *   `pl_hit_react` (applies the pending hit), `pl_scan_act_select` (enters the first step-table row that fires); the
 *   data `pl_shell_act_tbl_a`/`_b` (the key 252/251 act tables), `pl_float_0_7`, `pl_frame_window_999`; the `_PLW`
 *   names `hit_pending_0x39E`, `area_drain_timer_0x462`, `req_bits_b_0x0BC`, `window_start/lead/end_0x31C..0x320`,
 *   `buff_b_level_0x44D`, `buff_timer_0x46A`, `hold_timer_0x66A`, `frame_b_0x024`, `entry_hold_0x012`.
 * RESIDUALS. Every function is written.  `GetItemData` keeps the local `u8*` view: `menu/menu_item.h` and
 *   `ai/ai_npc.h` both define `_HIT_W`.  About 100 functions partial; the lowest are `pl_gather_item_take`,
 *   `pl_act_fishing`, `pl_act_net_hook_a`/`_b`/`_main`, `pl_act_item_throw`.  Recorded causes:
 *  - `pl_frame_post_step`: retail tests the master check with `bne` over a `b` to the epilogue; every `if`, `switch`
 *    and inline-helper spelling of the early return folds it into one `beq`;
 *  - `pl_condition_tick`: the act-kind 8/9 return keeps `bgt` over a `b` (the same fold);
 *  - `pl_item_use_act_pick`: items 380/384 and 583 share one tail (`li r24,1; b` / `li r24,5`); the `goto`-free
 *    spelling writes the tail twice (152 B);
 *  - `pl_act_status_step`: one extra `clrlwi` before the `+0x5C4` nibble store, and the cap/health compare takes
 *    r0/r3 the other way round;
 *  - `pl_scan_act_select`: rows 3/24 jump into row 16's body and keys 247/248 share one tail (the `goto`-free
 *    spellings repeat the test or re-test the key); the ground-probe calls load `&probe` before the two floats; the
 *    allocator colours the loop's saved registers differently (declaration order searched);
 *  - `pl_gather_item_take`, `pl_act_fishing`: retail places the shared tail between two switch bodies (the 0-2 and 3
 *    results; the 2/3 grade before the 4 grade); a switch's bodies are emitted in source order;
 *  - `pl_act_net_hook_main`: retail shares its arms' tails through jumps; a `goto`-free spelling repeats them;
 *  - `pl_guard_ck`: the allocator gives the scan-row pointer and the weapon kind other registers;
 *  - `pl_act_step_offhand_gesture`: `.text`, extab and extabindex are byte-identical; the target's extabindex
 *    `R_PPC_ADDR32` names dtk's `@etb_80011EFC` where MWCC writes its own local, which no source reaches;
 *  - `pl_act_step_pitfall_arm`: one extra `clrlwi r0,r0,24` on the case-0 store, the `u8` local's own conversion; the
 *    local is what gives retail's three-instruction range test (`addi r0,r3,-1; cmplwi r0,1; ble`);
 *  - `pl_act_step_pitfall_hold`: 8 B short - MWCC removes the loop counter retail keeps (`li r4,0`/`addi r4,r4,1`);
 *    the shape that keeps it (`u16*` plus `i`) re-bases the address and scores lower;
 *  - `fn_80259684`: the target materialises `fn_80277DAC`'s `0` argument after the two float arguments, ours before;
 *  - `fn_8025E0C8`: retail branches straight from each blocked test to one shared `field_0x308` clear; the inline
 *    `pl_item_window_blocked` returns a flag the caller tests (`li r0,1` + `cmpwi`), and an un-inlined chain repeats
 *    the clear at every test;
 *  - `fn_8025FF0C`: kind 3 tests `act == 27` with `beq` past a `b` to the 0xC0 arm, ours inverts it to `bne`; the
 *    `pl_move_work` scan keeps a dead loop counter and adds the row base before the entry offset;
 *  - `fn_8025FA00`: the surface clamp is one block that the `field_0x00A` switch reaches from case 0, case 1 and
 *    default; ours writes the clamp once per arm;
 *  - `pl_act_net_hook_a`: retail loads `shell_set_func_ptr` after the call arguments, ours before them;
 *  - `pl_act_net_hook_b`: retail places the 0x17F arm between case 2 and the 0x246/0x34 arm case 2 falls into, and
 *    loads `shell_set_func_ptr` after the call arguments; ours keeps the fallthrough adjacent and loads it first;
 *  - `fn_8025A7FC`: the target schedules `threshold = 15` between the `cmplwi` and the `bne` of the
 *    `field_0x128 == 2` test, ours before the load;
 *  - the other partial rows have no recorded cause.
 *  - flipcheck: the registry names this source spells resolve when the registry is exported.
 *  - flipcheck: the object emits no `.bss` (0x30 claimed), `.ctors` (0x4) or `.sdata` (0x28), one `.sbss` word
 *    (`pl_eft_func`; 0x8 claimed, the second word has no reader), and its own switch tables in `.data`; the own
 *    `.data`/`.sdata2` tables are still declared `extern`.
 * SHAPES. The per-part argument is `s32`: retail compares it with `cmpwi r4,0` and never masks it, which a `u8`
 *   declaration does (`clrlwi`); `pl_act_charge_repeat_step`'s range test is still `(u32)part <= 2` (`cmplwi r30,2`).
 *  - `++`/`--` on a byte field stores raw; `+= 1`, `= x + 1` and `x = x - 1` add a `clrlwi`/`extsb`; `*timer -= 10` on
 *    an `s16*` adds an `extsh` that `*timer -= (s16)10` does not;
 *  - an `s16` parameter that is only compared emits `extsh`+`cmpwi`, so it is `s32` where retail keeps `cmpwi`;
 *  - a `u16` field compared with an `(s16)` constant (`act_no < (s16)28`) keeps retail's signed `cmpwi`;
 *  - a redundant `(u8)` cast on the second of two byte stores is what makes MWCC narrow it;
 *  - `if (cond >= 1) { big }` rather than `if (cond < 1) { small }` puts the small block at the function's end;
 *  - a `switch`'s bodies are emitted in source order (a shared `case 1: case 2:` body included), so the cases are
 *    written in the order the target's bodies sit in `.text`;
 *  - the callees `fn_8025E298` and `cockpit_ticket_acquire` are called through per-call-site views (`<name>_viewN` cast macros,
 *    the same direct call), and the header declarations of `fn_8025E298`, `get_move_work_adrs` and
 *    `get_move_work_max` that disagree with a view are renamed away around their `#include`;
 *  - `pl_act_step_offhand_gesture` writes each follow-up arm out in full: the two constants each arm hands
 *    `Pl_frame_check` differ only per gesture;
 *  - `#pragma optimization_level 4` (playbook 108) brackets `pl_weapon_record_rearm`, `pl_act_knockback`,
 *    `pl_act_fishing`, `pl_act_paralysis`, `pl_scan_act_select`, `pl_condition_tick`, `pl_hit_react` and
 *    `pl_wide_area_effect_step`: their switches are retail's level-4 linear `subi`/`cmplwi` range chains;
 *  - `do { switch ... } while (0)` with `continue` (playbook 183) gives `pl_hit_react`'s cases that skip the hold
 *    notify, and `pl_act_status_step`'s area gate that skips the regeneration;
 *  - `(u8)(x + 250) > 2` is retail's `addi r0,r3,0xFA; clrlwi; cmplwi` range test (`(u8)(x - 6)` emits `subi`);
 *  - `fn_80265748` returns 0, then 2 below 0x8001, else 1: the one-expression return folds into `subfc`/`subfe`;
 *  - `pl_hit_react` reuses its `hp` local for the knockback motion, as retail's register reuse shows.
 */

#include "types.h"
#include "ef/get_move_work_adrs.h"
#include "pl.h"
#include "sound/se.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Network/network_pat_control.h"   /* isServerSelectState (owner header, rule 2) */
#include "ef/fn_800CDB2C.h"
#define get_move_work_adrs get_move_work_adrs_hidden_fn_8025F088_h
#define get_move_work_max get_move_work_max_hidden_fn_8025F088_h
#define fn_8025E298 fn_8025E298_hidden_fn_8025F088_h
#define fn_8025E298 fn_8025E298_hidden_fn_8025F088_h
#include "Pl/fn_8025F088.h"
#undef fn_8025E298
#undef fn_8025E298
#undef get_move_work_max
#undef get_move_work_adrs
#include "Pl/fn_802693C4.h"
#include "Pl/pl_act.h"
#include "Pl/pl_item_slot_item_get.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 frame-window pool (rule 2) */
#include "nw4r/math.h"
#include "Pl/fn_802693C4.h"   /* 0x802693C4-0x8026BA1C - the owner of the Pl_chr_set_attr_default/33C/644 group */
#define get_move_work_adrs get_move_work_adrs_hidden_ef_h
#define get_move_work_max get_move_work_max_hidden_ef_h
#include "unsplit/ef.h"
#undef get_move_work_max
#undef get_move_work_adrs
#include "ef/eft004.h"
#include "ef.h"
#include "g3d/g3d_calcworld.h"
#include "Pl/pl_act_step.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 float pool (rule 2) */
#include "Pl/fn_80273B14.h" /* the owner header of the act-motion setters (rule 2) */
#include "Pl/fn_80262940.h" /* the owner header of the model-state setter */
#include "ef/fn_800CDB2C.h"   /* the ef play-mode dispatcher the act-175 arm drives */
#include "Pl/pl_act_step.h"   /* 0x80257E70 - the owner of the three-timer reset this unit calls */
#include "Pl/fn_802693C4.h"   /* 0x802693C4-0x8026BA1C - the owner of the Pl_chr_set_attr_default/33C/644/3A8 group */
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "Pl/fn_802693C4.h"   /* `Get_motion_no` (rule 2) */
#include "Pl/pl_frame_data.h" /* this unit's .sdata2 pool, 0x80799E00-0x80799F98 */
#include "Pl/pl_act_data.h" /* the pool's second run, 0x80799F98-0x80799FDC */
#include "Pl/pl_coll.h" /* the owner of the `.bss` move-work table `pl_move_work` (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "ef/eft052.h" /* hud_item_msg_push (the owner's header, rule 2) */
#include "lobby/fn_8021E1EC.h"
#include "Pl/fn_802430E8.h" /* pl_hit_effect_spawn (this unit's leaf header) */
#include "Pl/pl_act_step_data.h" /* pl_act_step_table_247 (rule 2) */
#include "stage/shell_set_func_ptr.h" /* the shell-set job table (rule 2) */
#include "enemy/em_model.h" /* em_roster_area_hit_find (rule 2) */
#include "ef/cp_vector.h" /* `_CP_VECTOR`, the rotation `em_roster_area_hit_find` writes out (the type's shared header) */
#include "ai/ai_npc_player_in_reach_ck.h" /* the owner's leaf header: `ai/ai_npc.h` redefines `_HIT_W` */
#include "ai/ainpc_move.h" /* ai_npc_item_give (rule 2) */
#include "ef/eft019.h" /* ef_inst_spawn (rule 2) */
#include "stage/stage_zone_hit_ck.h"
#include "hud/Pl_act_frame_dispatch.h"
#include "ef/fn_8010D1A8.h"
#include "stage/stage_area_event_dispatch.h"
#include "enemy/em_kill_cut_target_get.h"
#include "Pl/pl_input_active_ck.h"
#include "ef/fn_8010BDE4.h"
#include "Pl/pl_hit_result_best_get.h"
#include "enemy/em_common.h"
#include "ai/ainpc_call_request.h"
#include "ai/ai_npc_beside_player_ck.h"
#include "sound/fn_800DD1F0.h"
#include "sound/se_req.h"
#include "Pl/pl_motion.h"
#include "camera/camera_main.h"
#include "hud/quest_slot_arm_all_b.h"
#include "ai/ainpc_area_buff_apply.h"
#include "quest/quest_item_slot.h"
#include "hud/cockpit.h"
#include "Pl/pl_act_class3.h"
#include "Pl/fn_80288CEC.h"
#include "hud/pl_flinch_steps_kind8.h"
#include "Pl/fn_802840DC.h"
#include "lobby/lobby_hunter_cards.h"
#include "Pl/pl_se_kind_dispatch.h"
#include "ef/eft021_pl_create.h"
#include "Pl/pl_equip_refresh.h"
#include "Pl/pl_se_frame_callback.h"
#include "lobby/fn_801E0ADC.h"
#include "ef/eft013_fx.h"
#include "enemy/eft_frame_pl_create.h"
#include "ef/ef_pl_effects_init.h"
#include "ef/eft_pl_model_slot_bind.h"
#include "stage/stg_w.h"
#include "menu/hit_attack_list_push.h"
#include "Pl/pl_skill_update.h"
#include "Pl/pl_skill_update2.h"
#include "Pl/pl_push_clear.h"
#include "Pl/pl_yure.h"
#include "lobby/lb_event_request.h"
#include "menu/get_pop_dat_ptr.h"
#include "mh3_pad/system_w.h" /* `system_w` (rule 2) */
#include "hud/Pl_net_send_hit_kind5.h"
#include "Pl/pl_camera_close_ck.h"
#include "enemy/em_pop.h"
#include "quest/quest_element_item_use.h"
#include "quest/quest_element_supply_state_get.h"
#include "hud/pl_gauge_level_get.h"
#include "ef/sys_quest_move.h"
#include "Pl/pl_model_blend_positive_ck.h"
#include "Pl/pl_armed_act_no_get.h"
#include "Pl/pl_call_answer_ck.h"
#include "Pl/pl_shell_class_get.h"
#include "Pl/pl_reload_needed_ck.h"
#include "Pl/pl_alt_form_free_ck.h"
#include "Pl/pl_daze_level_ck.h"
#include "Pl/pl_health_out_ck.h"
#include "Pl/pl_kind_item_slot_find.h"
#include "Pl/pl_kind_item_equip_find.h"
#include "ef/eft_model_effect_spawn.h"
#include "sound/se_slot0_pos_req.h"
#include "Pl/pl_health_damage_apply.h"
#include "Pl/pl_status_duration_map.h"
#include "Pl/pl_area_stamina_drain_step.h"
#include "Pl/pl_act_status_step.h"
#include "Pl/pl_motion_interruptible_ck.h"
#include "Pl/pl_item_use_free_ck.h"
#include "Pl/pl_spot_use_a_ck.h"
#include "Pl/pl_spot_use_b_ck.h"
#include "Pl/pl_stance_low_ck.h"
#include "ai/ai_npc_player_near_ck.h"
#include "Pl/pl_stick_window_gated_ck.h"
#include "Pl/pl_stick_window_level_ck.h"
#include "Pl/pl_stick_window_ck.h"
/* fn_802DE578_view1: the call sites disagree on this callee's parameters; each keeps its own view through a cast (same direct call). */
#define fn_802DE578_view1 ((void (*)(_PLW*, u16*))cockpit_ticket_acquire)
/* fn_8025E298_view3: the call sites disagree on this callee's parameters; each keeps its own view through a cast (same direct call). */
#define fn_8025E298_view3 ((u32 (*)(struct _PLW*, s32, s32))fn_8025E298)
/* fn_8025E298_view1: the call sites disagree on this callee's parameters; each keeps its own view through a cast (same direct call). */
#define fn_8025E298_view1 ((u32 (*)(struct _PLW*, s32, s32))fn_8025E298)

/* pl_se_entry_request: `se_entry_request` (`sound/fn_800D7F54.h`) as the player calls it - the owner declares the
 * enemy-work signature; the same direct call through a cast. */
#define pl_se_entry_request ((struct _se_w* (*)(s32, _PLW*, void (*)(_PLW*, s32)))se_entry_request)

extern "C" void fn_80247CC4(_PLW* self);

extern "C" {
/* This unit's own helpers, defined further down but called from the handlers above them. */
void fn_8025A7E0(_PLW* self, u8 a2);
void fn_80247C2C(_PLW* self);
void pl_act_net_hook_main(_PLW* self);
void pl_carry_latch_refresh(_PLW* self);
void pl_resist_item_apply(_PLW* self, _PLW* src, u16 item, u8 shared);
void pl_act_net_hook_a(_PLW* self);
void pl_act_net_hook_c(_PLW* self);
void fn_8024D3C8(_PLW* self);

s32 fn_8024D5AC(_PLW* self, _PLW* other);

s16 fn_8024CD8C(_PLW* self, s16 value);

void fn_8024CA50(_PLW* self, s32 a);

void fn_802F39DC(_PLW* self, s32 arg);
}

/* One 0x16-byte entry of the `_PLW::field_0x318` scan table `fn_8025E298` walks; `id_0x00` == 0xFF
 * terminates the run. size: 0x16 */
typedef struct PlScanEntry {
    /* +0x00 */ u8 id_0x00;
    /* +0x01 */ u8 pad_0x01[0x1];
    /* +0x02 */ u16 key_0x02;
    /* +0x04 */ u16 arg_0x04;         /* the act argument the row enters with */
    /* +0x06 */ u16 flags_0x06;       /* the low 14 bits pick the input condition `pl_scan_row_input_ck` tests */
    /* +0x08 */ u16 cond_a_0x08;      /* the condition's first operand (an input mask, a part flag, a window bound) */
    /* +0x0A */ u16 cond_b_0x0A;      /* its second operand */
    /* +0x0C */ u16 cond_c_0x0C;      /* its third operand */
    /* +0x0E */ u16 flags_0x0E;
    /* +0x10 */ s16 frame_start_0x10; /* the row's frame window start (999 and above: the motion end) */
    /* +0x12 */ s16 frame_end_0x12;   /* the window end */
    /* +0x14 */ s16 frame_lead_0x14;  /* the frames the window opens early (999 and above: no window test) */
} PlScanEntry; /* size: 0x16 */

/* One entry of `pl_move_work` is `PlMoveEntry`, declared by its owner's header (rule 1/2). */

/* One 2840-byte (0xB18) move-work record `get_move_work_adrs` hands back; the motion slot `fn_802607C4`
 * scores sits at +0x188 and the entry's own kind byte at +0x1E1. size: 0xB18 */
typedef struct PlMoveWork {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 pad_0x002[0x1];
    /* +0x003 */ u8 kind_0x003;       /* the enemy kind the grab act looks for (0x12, 4) */
    /* +0x004 */ u8 pad_0x004[0x16];
    /* +0x01A */ u8 sub_area_0x01A;   /* compared with the actor's `sub_area_0x3B6` */
    /* +0x01B */ u8 pad_0x01B[0x16D];
    /* +0x188 */ VEC3 pos_0x188;
    /* +0x194 */ u8 pad_0x194[0x4D];
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 pad_0x1E2[0x936];
} PlMoveWork; /* size: 0xB18 */

/* The player body sub-object `_PLW::physics_0x13C` points at: its `MHchar` block sits at +0x04 (the
 * layout `ef/fn_80114E34.cpp` owns as `_PLW_PHYSICS`).  A local view, because that owner keeps its
 * definition in its own `.cpp`; moving it to a shared header is open rule-1 work.
 * size: 0x144 (lower bound: the `MHchar` it holds is 0x140) */
typedef struct PlBodyWork {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr_0x04;
} PlBodyWork; /* size: 0x144 */

/* One item-data record `GetItemData` hands back (`menu/menu_item.h`'s `ItemDataRecord`, whose names the first and
 * third bytes keep); this unit reads the kind byte, the level byte and the category mask.
 * size: 0x03 (lower bound: +0x02 is the highest byte this unit names) */
typedef struct PlItemData {
    /* +0x00 */ u8 kind_0x00;    /* `1` marks an item that cannot be used from the pouch */
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x002;  /* the category mask; bit 3 marks a usable item */
} PlItemData; /* size: 0x03 */

/* One row of the item-use act table: the motion, the attribute and its part the use step arms, and the slot the
 * act returns to.  size: 0x8 */
typedef struct PlItemAct {
    /* +0x0 */ u16 motion_0x00;
    /* +0x2 */ u16 attr_0x02;
    /* +0x4 */ u16 part_0x04;
    /* +0x6 */ u16 slot_0x06;
} PlItemAct; /* size: 0x8 */

/* One row of the emote act table: the emote's kind (it picks the hit and step tables), the motion attribute with
 * its part and frame, and the follow-up attribute.  size: 0xA */
typedef struct PlEmote {
    /* +0x0 */ u16 kind_0x00;
    /* +0x2 */ u16 attr_0x02;
    /* +0x4 */ s16 part_0x04;
    /* +0x6 */ s16 frame_0x06;
    /* +0x8 */ u16 follow_0x08;
} PlEmote; /* size: 0xA */

/* One row of a gesture table: the motion attribute with its part and frame.  size: 0x6 */
typedef struct PlActAttr {
    /* +0x0 */ s16 attr_0x00;
    /* +0x2 */ s16 part_0x02;
    /* +0x4 */ s16 frame_0x04;
} PlActAttr; /* size: 0x6 */

/* One weapon-kind row of a `pl_scan_kind_remap` table: the scan row's mode and key as that weapon kind reads them.
 * size: 0x6 */
typedef struct PlScanRemap {
    /* +0x0 */ u16 mode_0x00;
    /* +0x2 */ u16 key_0x02;
    /* +0x4 */ u16 pad_0x04;
} PlScanRemap; /* size: 0x6 */

/* This unit's own `.data` tables, declared until the unit's `.data` is emitted from source. */
extern "C" {
extern s32 pl_hit_id_tables[6];
extern u8 pl_hit_table_main[0x108];
extern s16 pl_draw_attr_tbl[];
extern s16 pl_draw_attr_tbl_b[];
extern s16 pl_draw_attr_tbl_c[];
extern s16 pl_draw_attr_tbl_d[];
extern u32 pl_knockback_bow_steps[6];
extern s32 pl_knockback_bow_attr[6];
extern s16 pl_item_throw_tbl[];
extern u32 pl_evade_steps_left[];
extern u32 pl_evade_steps_right[];
extern u32 pl_evade_bow_steps[6];
extern u8 pl_gather_tool_limits[3];
extern s16 pl_weapon_follow_up_slots[];
extern s16 pl_knockback_flight_attr[];
extern s16 pl_knockback_getup_attr[];
extern s16 pl_knockback_land_attr[4];
extern u8 pl_stun_shake_needs[6];
extern u32 pl_sleep_steps[];
extern s32 pl_chunk_anchor_angle[];
extern u32 pl_kill_cut_steps[];
extern u16 pl_kill_cut_taunts[2];
extern u32 pl_kick_run_steps[];
extern PlEmote pl_emote_tbl[10];
extern s16 pl_emote_hit_kinds[4];
extern u32 pl_emote_steps[4];
extern u32 pl_emote_steps_drawn[4];
extern PlActAttr pl_kneel_gesture_tbl[6];
extern PlActAttr pl_kneel_gesture_tbl_kneel[6];
extern PlActAttr pl_gesture_once_tbl[3];
extern PlActAttr pl_gesture_once_tbl_kneel[3];
extern u32 pl_carry_steps[];
extern VEC3 pl_chunk_anchors[4];
extern u16 pl_bonus_item_weights[];
extern f32 pl_rest_spot_offset[];
extern u16 pl_companion_talk_odds[8];
extern u16* pl_scan_kind_remap[10];
}

/* The shell act tables: per shell class, the act number and the one the kind-3 actor enters instead. */
extern "C" {
extern u16 pl_shell_act_tbl_a[20];
extern u16 pl_shell_act_tbl_b[20];
}

/* pl_weapon_alt_form_ck_c1: the owner declares `pl_weapon_alt_form_ck` `void`, while its body returns the
 * alternate-form byte the draw/sheathe acts test; the same direct call through a cast. */
/* pl_act_held_ck_c1: the owner declares a second, unused parameter its callers here never load; the same direct call
 * through a cast. */
/* pl_hit_face_motion_c1: `fn_80278674` as the hit reaction calls it - the owner declares its motion `s16`, the caller
 * hands the unextended face angle; the same direct call through a cast. */
#define pl_hit_face_motion_c1 ((void (*)(_PLW*, s32, u8))fn_80278674)

/* pl_armed_act_no_get_c1: the owner declares the act number `u32`; the step-table walk stores it as the `u16` it is,
 * the same direct call through a cast. */
#define pl_armed_act_no_get_c1 ((u16 (*)(_PLW*, s16))pl_armed_act_no_get)
/* pl_act_dash_input_c1: the owner declares no parameter, the step-table walk hands it the actor; the same direct call
 * through a cast. */
#define pl_act_dash_input_c1 ((s32 (*)(_PLW*))pl_act_dash_input_ck)

#define pl_act_held_ck_c1 ((u32 (*)(_PLW*))pl_act_held_ck)

#define pl_weapon_alt_form_ck_c1 ((u8 (*)(_PLW*))pl_weapon_alt_form_ck)

/* The fishing SE requests of `sound/fn_800D7F54.cpp` as the fishing act calls them: the owner's bodies ignore the
 * position argument the callers pass, so it declares them without it; the same direct calls through casts. */
#define pl_fish_se_cast ((void (*)(VEC3*, u32))se_fishing_cast)
#define pl_fish_se_big ((void (*)(VEC3*))se_fishing_bite_big)
#define pl_fish_se_small ((void (*)(VEC3*))se_fishing_bite_small)
#define pl_fish_se_result ((void (*)(u8, VEC3*, u8))se_fishing_result)

/* pl_paralysis_spark: `ef/eft013_fx.cpp`'s spark spawner as the paralysis act calls it - the owner declares it with
 * the effect record and no scale; the same direct call through a cast. */
#define pl_paralysis_spark ((void (*)(_PLW*, f32))eft013_spark_spawn)

extern "C" {
/* this unit's own entry points (unmangled `fn_*` stems, so C linkage - playbook 42/48) */
void fn_80264274(struct _PLW* self, s16 value);
void fn_80264940(struct _PLW* self, u32 value);
u8 fn_8026495C(struct _PLW* self);
void pl_sub_gauge_level_cost(struct _PLW* self);
s32 fn_80264A84(struct _PLW* self);
s32 fn_80264EA4(struct _PLW* self);
s32 fn_80264EC0(struct _PLW* self);
s32 fn_80264ED4(struct _PLW* self);
s32 fn_80264EE8(struct _PLW* self);
s32 fn_80264F78(struct _PLW* self);
s32 fn_80264FF0(struct _PLW* self);
s32 fn_8026505C(struct _PLW* self);
s32 fn_802650EC(struct _PLW* self, u8 kind);
s32 fn_80265348(struct _PLW* self);

void fn_802656EC(struct _PLW* self);
s32 fn_802656FC(struct _PLW* self);
u32 fn_80265748(struct _PLW* self);
void fn_80265780(struct _PLW* self);
u32 fn_802657AC(struct _PLW* self);

void pl_model_state_set(struct _PLW* self, u32 action, s32 a, u16 b);
s32 fn_80264B4C(struct _PLW* self);
void fn_80266EB8(struct _PLW* self);
s32 pl_scan_act_select(struct _PLW* self);
s32 pl_hit_react(struct _PLW* self);
s32 pl_item_use_act_pick(struct _PLW* self, u8* kind, u16* act, u16* arg, s32* word, u16 slot, u16* point);
void pl_frame_post_step(struct _PLW* self);
void pl_condition_tick(struct _PLW* self);
u32 pl_scan_row_input_ck(struct _PLW* self, struct PlScanEntry* row);

/* The player effect hook `pl_frame_post_step` calls every frame (this unit's `.sbss`). */
void (*pl_eft_func)(struct _PLW* self);
}

s32 fn_80260198(_PLW* self);
void fn_8025F088(_PLW* self);


/* Steps the guard gauge by the two part flags (-1024 unless the menu owns the screen, +1024, else 768 back toward
 * zero) and clamps it to the caller's two limits in the field's own 16-bit wrap. */
extern "C" void pl_act_guard_gauge_adjust(_PLW* self, u16 neg_limit, u16 pos_limit, s32 flag_a,
                                          s32 flag_b)
{
    u32 gauge;

    if (pl_part_flag_ck(self, flag_a) == 1 && Pl_suimen_ck(self) == 0) {
        self->field_0x0AC -= 1024;
    } else if (pl_part_flag_ck(self, flag_b) == 1) {
        self->field_0x0AC += 1024;
    } else {
        gauge = self->field_0x0AC;
        if ((s16)gauge >= 0) {
            if (gauge >= 768) {
                self->field_0x0AC = gauge - 768;
            } else {
                self->field_0x0AC = 0;
            }
        } else {
            if (gauge >= 64768) {
                self->field_0x0AC = 0;
            } else {
                self->field_0x0AC = gauge + 768;
            }
        }
    }
    gauge = self->field_0x0AC;
    if ((s16)gauge < 0) {
        if ((u16)gauge <= neg_limit) {
            self->field_0x0AC = neg_limit;
        }
    } else if ((u16)gauge >= pos_limit) {
        self->field_0x0AC = pos_limit;
    }
}



/* Answers 1 while the actor's kind or act keeps the item window shut. */
inline BOOL pl_item_window_blocked(_PLW* self) {
    u8 kind = self->field_0x00A;
    if ((u32)(kind - 8) <= 1) {
        return TRUE;
    }
    switch (kind) {
    case 0:
        if (Pl_bari_ck(self, 1) == 1) {
            return TRUE;
        }
        break;
    case 6:
        switch (self->act_no) {
        case 27:
        case 35:
            return TRUE;
        }
        break;
    }
    return FALSE;
}


extern "C" void fn_802430E8(_PLW* work, u8 part) {
    u32 partHi;
    /* The three per-part `_se_w` work objects; the declaration order and the assignment order are
     * deliberately different - it is the shape that reproduces retail's colouring
     * (r31 = +0xAF4, r29 = +0xAF8, r30 = +0xAFC) together with its load order, exactly as in
     * `Pl/fn_80241558.cpp`. */
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 0x3EA:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4A, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x60, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set_bank(seWorkFrame, 4, (part << 0x18) | 4, 0x14, 2);
        return;
    case 0x3EB:
        se_req_frame_set_parts3(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set_bank(seWorkFrame, 0x66, part << 0x18, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x68, 3);
        return;
    case 0x3EC:
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x1E, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x3C, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 8, 3);
        se_req_frame_set_parts3(seWorkPart, 0x24, 3);
        return;
    case 0x3F1:
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x16, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x30, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x4A, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x66, partHi | 4, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 6, 3);
        se_req_frame_set_parts3(seWorkPart, 0x20, 3);
        se_req_frame_set_parts3(seWorkPart, 0x40, 3);
        se_req_frame_set_parts3(seWorkPart, 0x5A, 3);
        return;
    case 0x3F3:
        se_req_frame_set(seWorkPart, 6, SE_Code_Make(0, 3, 1, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0x29, 0xC, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xB, 0xE, 0x03000002);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 4, partHi | 3, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x2E, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x14, 3);
        se_req_frame_set_parts3(seWorkPart, 0x34, 3);
        return;
    case 0x408:
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x1E, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x3C, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 8, 3);
        se_req_frame_set_parts3(seWorkPart, 0x24, 3);
        return;
    case 0x409:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x14, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x2A, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x16, 3);
        se_req_frame_set_parts3(seWorkPart, 0x28, 3);
        return;
    case 0x40A:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x16, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x2C, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x18, 3);
        se_req_frame_set_parts3(seWorkPart, 0x2C, 3);
        return;
    case 0x40B:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set_bank(seWorkFrame, 0x14, (part << 0x18) | 4, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 0x1C, 3);
        return;
    case 0x40C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set_bank(seWorkFrame, 0x38, part << 0x18, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 0x3A, 3);
        return;
    case 0x40D:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x10, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x24, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x32, partHi | 4, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x14, 3);
        se_req_frame_set_parts3(seWorkPart, 0x34, 3);
        return;
    case 0x40E:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x2A, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x3A, partHi | 4, 0x11, 2);
        se_req_frame_set_part4(seWorkPart, 0x26, 0xB, 3);
        se_req_frame_set_parts3(seWorkPart, 0x3C, 3);
        return;
    case 0x40F:
        se_req_frame_set_parts3(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x14, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x36, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0xE, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x2A, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x42, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x5A, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x10, 3);
        se_req_frame_set_parts3(seWorkPart, 0x2C, 3);
        return;
    case 0x411:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x16, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x2E, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x18, 3);
        se_req_frame_set_parts3(seWorkPart, 0x30, 3);
        return;
    case 0x412:
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 2, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x1A, partHi, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 4, 3);
        se_req_frame_set_parts3(seWorkPart, 0x1C, 3);
        return;
    case 0x41B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3A, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x5C, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        return;
    case 0x41C:
        se_req_frame_set(seWorkMain, 0xA, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4A, 0xEA, 3, 0x03000003);
        return;
    case 0x41D:
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        return;
    case 0x421:
        se_req_frame_set(seWorkMain, 0xC, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1E, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xEA, 3, 0x03000003);
        return;
    case 0x426:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x03000003);
        return;
    case 0x428:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x14, 0x03000003);
        return;
    case 0x429:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x42A:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x11, 0x03000003);
        return;
    case 0x42B:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43A:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x43B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43C:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 6, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x22, 0xEE, 0x14, 0x03000003);
        return;
    case 0x424:
    case 0x425:
    case 0x42C:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x03000003);
        return;
    case 0x42D:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x11, 0x03000003);
        return;
    case 0x43D:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x44D:
    case 0x44E:
    case 0x44F:
        se_req_frame_set(seWorkPart, 8, 8, 0x11, 0x03000003);
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x5A, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x6C, partHi | 1, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x60, 3);
        return;
    case 0x450:
        se_req_frame_set(seWorkMain, 0x20, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        se_req_frame_set_bank(seWorkFrame, 0x18, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x78, partHi, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 0x1A, 3);
        se_req_frame_set_parts3(seWorkPart, 0x7A, 3);
        return;
    case 0x451:
    case 0x452:
    case 0x453:
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        return;
    case 0x454:
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x2C, partHi | 2, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x3E, partHi | 4, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x60, partHi | 1, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x7C, partHi, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 0x46, 3);
        se_req_frame_set_parts3(seWorkPart, 0x7E, 3);
        return;
    case 0x455:
    case 0x456:
        se_req_frame_set(seWorkMain, 0x24, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x68, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x84, partHi | 1, 0x11, 2);
        se_req_frame_set(seWorkPart, 6, 0x10, 0x11, 0x03000003);
        se_req_frame_set_parts3(seWorkPart, 0x6C, 3);
        return;
    case 0x458:
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x459:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(2, 3, 1, 3), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x42, partHi | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 0x27, 0x14, 0x03000003);
        se_req_frame_set_part4(seWorkPart, 0x36, 7, 3);
        return;
    case 0x45A:
    case 0x45B:
        se_req_frame_set(seWorkMain, 0x24, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2E, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3C, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        se_req_frame_set_bank(seWorkFrame, 0x18, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x78, partHi, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 4, 3);
        se_req_frame_set_parts3(seWorkPart, 0x7A, 3);
        return;
    case 0x45C:
    case 0x45D:
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x46A:
        se_req_frame_set(seWorkMain, 2, 1, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5C, 4, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0x12, partHi | 1, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x58, partHi | 4, 0x14, 2);
        return;
    case 0x46B:
        se_req_frame_set(seWorkMain, 2, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set_bank(seWorkFrame, 0x24, part << 0x18, 0x14, 2);
        se_req_frame_set_parts3(seWorkPart, 0x26, 3);
        return;
    case 0x47E:
    case 0x47F:
    case 0x480:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x14, 0x03000003);
        return;
    case 0x481:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x74, 0xED, 3, 0x03000003);
        return;
    case 0x482:
    case 0x483:
    case 0x484:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x11, 0x03000003);
        return;
    case 0x485:
    case 0x486:
    case 0x487:
        se_req_frame_set(seWorkMain, 0x24, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x36, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xEE, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x64, 0xED, 0x11, 0x03000003);
        return;
    case 0x489:
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x48A:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x14, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xE, 0x8B, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xED, 0x14, 0x03000003);
        return;
    case 0x48B:
    case 0x48C:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x68, 0xED, 0x14, 0x03000003);
        return;
    case 0x48D:
    case 0x48E:
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x49C:
        se_req_frame_set(seWorkMain, 4, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5A, 0xE, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x49D:
        se_req_frame_set(seWorkMain, 6, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x49E:
        se_req_frame_set(seWorkMain, 6, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x410:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0xA, partHi | 2, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        return;
    case 0x3F4:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 6, partHi, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x11, 0x03000003);
        partHi = partHi | 1;
        se_req_frame_set_bank(seWorkFrame, 0x1A, partHi, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x18, 3);
        return;
    case 0x3F5:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 6, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        se_req_frame_set_bank(seWorkFrame, 0x1A, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x18, 3);
        return;
    case 0x3F6:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 4, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        se_req_frame_set_bank(seWorkFrame, 0x18, partHi, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
        se_req_frame_set_parts3(seWorkPart, 0x18, 3);
        return;
    case 0x46C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        se_req_frame_set_bank(seWorkFrame, 0xC, partHi | 2, 0x14, 2);
        se_req_frame_set_bank(seWorkFrame, 0x1C, partHi | 3, 0x11, 2);
        se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x14, 2);
        return;
    case 0x4B1:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(4, 2, 4, 2), 0xC, 3);
        se_req_frame_set_bank(seWorkFrame, 2, part << 0x18, 0x14, 2);
        return;
    case 0x4BB:
        se_req_frame_set_bank(seWorkFrame, 0x3A, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 2, 0x10, 0x11, 0x03000003);
        se_req_frame_set_parts3(seWorkPart, 0x3C, 3);
        return;
    case 0x4BC:
        se_req_frame_set_bank(seWorkFrame, 0x72, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 8, 0x10, 0x11, 0x03000003);
        se_req_frame_set_parts3(seWorkPart, 0x74, 3);
        return;
    case 0x4E3:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x18, 1, 0x19, 1), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xC, 0xED, 0x11, 0x03000003);
        return;
    case 0x4ED:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1E, 0xED, 0x11, 0x03000003);
        return;
    case 0x4EE:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3A, 0xED, 0x11, 0x03000003);
        /* fallthrough */
    default:
        return;
    }
}

/* Arms the SE frames of the two long weapon motions 0x139 and 0x164 for the part's equipment kind. */
extern "C" void pl_se_long_motion_frames(struct _se_w** seWork, u32 part, u32 motion, u32 kind) {
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    u32 partHi;

    seWorkPart = seWork[0];
    seWorkFrame = seWork[1];
    seWorkMain = seWork[2];
    if ((u16)motion == 0x139) {
        switch ((u8)kind) {
        case 0:
            se_req_frame_set(seWorkMain, 4, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x13C, 4, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        case 1:
            se_req_frame_set(seWorkMain, 0x16, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x138, 3, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        case 2:
            se_req_frame_set(seWorkMain, 0x16, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x138, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        case 3:
            se_req_frame_set(seWorkMain, 0x12, 1, 7, 3);
            se_req_frame_set(seWorkMain, 0x14, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x136, 5, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        case 7:
            se_req_frame_set(seWorkMain, 0x16, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x1A, 4, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x138, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        case 8:
            se_req_frame_set(seWorkMain, 8, 2, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x14, 4, 0xB, 3);
            se_req_frame_set(seWorkMain, 0x24, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0xC, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x12C, 1, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x12E, 4, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x136, 6, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x64, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x92, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xC0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0xF0, 0x2C, 0xA, 0x03000003);
            se_req_frame_set(seWorkPart, 0x112, 0x26, 0xA, 0x03000003);
            partHi = (u8)part << 0x18;
            se_req_frame_set_bank(seWorkFrame, 0x32, partHi, 0x11, 2);
            se_req_frame_set_bank(seWorkFrame, 0x15A, partHi, 0x11, 2);
            se_req_frame_set_parts3(seWorkPart, 0x34, 3);
            se_req_frame_set_part4(seWorkPart, 0x64, 0xB, 3);
            se_req_frame_set_part4(seWorkPart, 0xC0, 0xB, 3);
            break;
        }
    }
    if ((u16)motion == 0x164) {
        switch ((u8)kind) {
        case 0:
            se_req_frame_set(seWorkMain, 0x16, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x130, 0xE, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            return;
        case 1:
            se_req_frame_set(seWorkMain, 0x16, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x130, 0xD, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            return;
        case 2:
            se_req_frame_set(seWorkMain, 0x16, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x138, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            return;
        case 3:
            se_req_frame_set(seWorkMain, 0x12, 0xB, 7, 3);
            se_req_frame_set(seWorkMain, 0x14, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x136, 0xF, 6, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            return;
        case 7:
            se_req_frame_set(seWorkMain, 0x16, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x1A, 0xE, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 0x130, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            return;
        case 8:
            se_req_frame_set(seWorkMain, 0x14, 5, 0xB, 3);
            se_req_frame_set(seWorkMain, 0x16, 0x15, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkMain, 2, 0x18, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x130, 0x18, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x136, 0x19, 0xB, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
            se_req_frame_set(seWorkMain, 0x130, 0x1A, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 3);
            se_req_frame_set(seWorkPart, 0x5A, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x86, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xBA, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0xE6, 0x8F, 0xB, 0x03000003);
            se_req_frame_set(seWorkPart, 0x10C, 0x8A, 0xB, 0x03000003);
            break;
        }
    }
}

/* Hands `pl_hit_attack_build` the actor's hit block with the hit table and the flags the equipment kind picks. */
extern "C" void pl_hit_table_arm(_PLW* self, u16 idx, u8 kind) {
    switch (kind) {
    case 0:
        pl_hit_attack_build(self, self->hit_0x484, pl_hit_table_main, idx, pl_hit_id_tables, 7);
        return;
    case 1:
        pl_hit_attack_build(self, self->hit_0x484, pl_hit_table_main, idx, pl_hit_id_tables, 4);
        return;
    case 2:
        pl_hit_attack_build(self, self->hit_0x484, pl_hit_table_main, idx, pl_hit_id_tables, 0x24);
        return;
    case 3:
        pl_hit_attack_build(self, self->hit_0x484, pl_hit_table_main, idx, pl_hit_id_tables, 0x44);
        return;
    }
}

/* Clears the act's timer/latch block on entry and, when the alternate path is taken on a live master,
 * drops the 0x300 pair from the act bitfield. */
extern "C" void pl_act_clear_wait(_PLW* self, u8 a) {
    if (Pl_master_ck(self) == 1 && a == 0) {
        self->field_0x3D8 &= ~0x300;
    }
    if (a == 0) {
        self->field_0x418 = 0;
        self->field_0x41E = 0;
        self->field_0x444 = 0;
    }
    self->field_0x416 = 0;
    self->field_0x41C = 0;
}

/* Reports the input-hold latch, defaulting to "held" while the pad layer has nothing to say. */
extern "C" s32 fn_80245E20(_PLW* self) {
    if (isServerSelectState() == 0) {
        return 1;
    }
    return self->field_0x645 != 0;
}

/* Drops the actor onto the ground of its area's zone; a dead master rides down, any other actor turns visible. */
extern "C" s32 pl_zone_ground_drop(_PLW* self) {
    VEC3 ground;
    u32 zone;
    f32 height;
    u8 area;

    em_land_data_init(&ground);
    area = self->area_0x16;
    if (area == 0xFF) {
        return 0;
    }
    if (stage_zone_hit_ck(stage_group_word_get(self->kind_0x015, area), &zone, &self->vec_0x03C) == 1 &&
        pl_ground_check(self, &self->vec_0x03C, &ground, &height, 0x4000) == 1) {
        pl_motion_clear(self);
        if (Pl_master_ck(self) == 1 && (s16)self->health <= 0) {
            pl_ride_down_set(self);
        } else {
            pl_model_visible_set(self, 10);
            pl_model_set_state(self, 10);
        }
        return 1;
    }
    return 0;
}

/* Re-arms the weapon record: its level from the second equipment for the blade kinds, a fixed 150 for the
 * gunner kinds, then the attack values and the affinity. */
#pragma optimization_level 4
extern "C" void pl_weapon_record_rearm(_PLW* self) {
    switch (self->field_0x002) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
        pl_weapon_record_read(self, &self->equipB);
        break;
    case 4:
    case 5:
    case 6:
        self->field_0x56A = 0;
        self->field_0x56E = 150;
        break;
    }
    self->field_0x56C = self->field_0x56E;
    self->field_0x56B = ((u8 (*)(_PLW*))pl_attack_range_tier_get)(self);
    pl_shell_ids_set(self, 0xFF, 0xFF);
    pl_weapon_class_refresh(self);
}
#pragma optimization_level reset

/* Resets the actor's act state: the latches, both attack entries, the hit-id list and the skill state. */
extern "C" void pl_act_state_reset(_PLW* self) {
    s32 i;

    self->field_0x18 = 0;
    self->field_0x5C6 = 0;
    if (self->field_0x002 == 8) {
        self->field_0x5C6 = 0;
    }
    self->field_0x266 = 0;
    self->field_0x369 = 0;
    self->field_0x65C = 0;
    self->field_0x65D = 0;
    self->field_0x382 = 0;
    self->blend_frames_0x0B4 = 0;
    self->field_0x5C4 = 0;
    self->field_0x585 = 0;
    if (option_cfg_eq_ck(self->chunk_ofs, 7, 1) == 1) {
        self->held_attack_0x5C5 = 1;
    } else {
        self->held_attack_0x5C5 = 0;
    }
    self->field_0x315 = 0;
    self->field_0x36A = 0;
    for (i = 0; i < 2; i++) {
        ((PlHitSlot*)self->hit_0x484)[i].active_0x05 = 0;
        ((PlHitSlot*)self->hit_0x484)[i].result_0x0E = 0xFF;
        hit_source_set((_HIT_W*)&((PlHitSlot*)self->hit_0x484)[i], 0, self);
        hit_owner_set((_HIT_W*)&((PlHitSlot*)self->hit_0x484)[i], 0, self);
    }
    hit_id_list_clear(self->hit_ids_0x53C, &self->hit_id_count_0x564);
    self->field_0x474 = 0;
    self->field_0x3E4 = 0;
    self->field_0x3E5 = 0;
    self->field_0x3E6 = 0;
    self->field_0x3E7 = 0;
    self->field_0x3E8 = 0;
    self->field_0x3E9 = 0;
    self->field_0x3A2 = 0;
    self->field_0x3A3 = 0;
    fn_8025A7E0(self, 0);
    self->field_0x5C8 = 0;
    pl_skill_update(self, 0, 0);
    pl_skill_update2(self, 0);
    self->field_0x120 = 0;
    self->field_0x121 = 0;
    self->field_0x122 = 0;
    self->field_0x123 = 0;
    self->field_0x124 = 0;
    self->field_0x125 = 0;
    self->field_0x126 = 0;
    self->field_0x127 = 0;
    pl_camera_lock_clear(self);
    pl_act_clear_flag5bb(self);
}

/* Steps the actor's chase one frame along the chunk's own rotation-offset pair. */
extern "C" void fn_80246158(_PLW* self) {
    VEC3 vec;
    VEC3 scratch;
    VEC3_ctor(&vec);
    VEC3_ctor(&scratch);
    vec.x = lbl_805C4898[self->chunk_ofs * 2];
    vec.y = pl_float_zero;
    vec.z = lbl_805C4898[self->chunk_ofs * 2 + 1];
    rotVecY(&vec, self->field_0x058);
    self->motion_pos_0x3C = self->motion_pos_0x3C + vec.x;
    self->motion_pos_0x44 = self->motion_pos_0x44 + vec.z;
}

/* Places the actor at `pos` in map `kind`, area `area`, facing `angle`; a position at or below the -50000 "no
 * height" mark keeps the current height, and the ground of the area lifts it. */
extern "C" void pl_pos_place(_PLW* self, VEC3* pos, u16 angle, u8 kind, u8 area, u8 keepChase) {
    f32 ground;

    self->kind_0x015 = kind;
    self->area_0x16 = area;
    self->rot_0x54.x = 0;
    self->rot_0x54.y = angle;
    self->rot_0x54.z = 0;
    self->field_0x0AC = 0;
    self->field_0x0A8 = angle;
    if (pos->y <= pl_float_neg50000) {
        self->vec_0x03C.x = pos->x;
        self->vec_0x03C.z = pos->z;
    } else {
        self->vec_0x03C.x = pos->x;
        self->vec_0x03C.y = pos->y;
        self->vec_0x03C.z = pos->z;
    }
    if (keepChase == 0) {
        fn_80246158(self);
    }
    copyVec3(&self->vec_0x048, &self->vec_0x03C);
    ground = GetGroundHit(&self->vec_0x03C, -5, self->area_0x16);
    if (self->vec_0x03C.y < ground) {
        self->vec_0x03C.y = ground;
    } else if ((u32)(pl_act_stage_get(self) - 2) <= 2) {
        self->vec_0x03C.y = ground;
    }
    pl_model_pos_sync(self);
}

/* 0x802462FC: Places the actor at an area entry and resets its act state; a downed or carted actor re-enters
 * its rest act, any other the act its latches pick.  The master's arrival raises the area's lobby events. */
extern "C" void pl_hit_effect_spawn(_PLW* self, VEC3* pos, u16 angle, u8 kind, u8 area, u8 keepChase) {
    PlBodyWork* body;
    u8 stage;
    u8 latch;

    body = (PlBodyWork*)self->physics_0x13C;
    pl_pos_place(self, pos, angle, kind, area, keepChase);
    self->field_0x352 = 10;
    pl_act_set_gauge_arm(self, 30);
    self->field_0x474 = 0;
    self->field_0x0B0 = 0;
    self->field_0x0B2 = 0;
    self->field_0x65C |= 1;
    self->field_0x65D = 0;
    self->field_0x382 = 0;
    self->blend_frames_0x0B4 = 0;
    self->field_0x3E4 = 0;
    self->field_0x3E5 = 0;
    self->field_0x3E6 = 0;
    self->field_0x3E7 = 0;
    self->field_0x3E8 = 0;
    self->field_0x3A2 = 0;
    self->field_0x3A3 = 0;
    fn_8025A7E0(self, 0);
    self->field_0x585 = 0;
    self->field_0x662 = 0;
    self->field_0x315 = 0;
    self->field_0x36A = 0;
    self->field_0x3AC = 0;
    self->field_0x3B0 = 0;
    self->field_0x65F = 0;
    self->field_0x389 = 0;
    self->field_0x5BC = 0;
    self->take_menu_open_0x5BE = 0;
    self->quest_menu_open_0x5BD = 0;
    self->field_0x268 = 0;
    fn_8027D4F0(self);
    pl_motion_scratch_clear(self);
    pl_push_clear(self);
    stage = pl_act_stage_get(self);
    if ((u32)(stage - 2) <= 1 || (stage == 4 && (s16)self->health <= 0)) {
        pl_skill_update(self, 1, 0);
        pl_skill_update2(self, 1);
        if (kind == 6 || kind == 0x11) {
            pl_act_reset_down(self, self->kind_0x09, 0, 0);
        } else if (PlayMode_ck() == 2) {
            self->kind_0x09 = 0;
            pl_act_enter(self, 9, 0, 14);
        } else {
            pl_act_enter_raw(self, 9, 0, 14);
        }
        pl_act_set_gauge_arm(self, 300);
    } else if (fn_8025F588(self) == 0) {
        latch = self->field_0x5C4;
        if ((latch & 0xF) != 0) {
            self->field_0x5C4 = (latch & 0xF0) | 5;
            pl_act_enter_raw(self, 12, 0, 14);
        } else if (fn_80278C7C(self) == 1) {
            pl_act_enter_raw(self, 6, 0x2C, 0);
        } else if (fn_80278CD0(self) == 1) {
            pl_act_enter_raw(self, 6, 0x40, 0);
        } else {
            pl_act_reset_idle(self, self->kind_0x09, 0, 0);
        }
    }
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(&body->chr_0x04);
    yure_player_clear_flags((YurePlayerWork*)self);
    if (Pl_master_ck(self) == 1) {
        if (stage != 0) {
            lb_event_request(0x11);
            if (userdata_progress_flag_ck(0x13) != 0) {
                lb_event_request(8);
            }
        }
        if (self->kind_0x015 == 1 || self->kind_0x015 == 0xC) {
            if (self->area_0x16 == 0) {
                lb_event_request(0x1A);
            }
            if (self->area_0x16 == 1) {
                lb_event_request(0x12);
            }
            if (self->area_0x16 == 3) {
                lb_event_request(2);
            }
            if (self->area_0x16 == 0xB) {
                lb_event_request(3);
            }
            if (self->area_0x16 == 0xC) {
                lb_event_request(0x1B);
            }
        }
    }
    pl_act_stage_latch_set(self, 0);
}

/* 0x80246654 (0x70): Spawns the area-entry effect at `pos` on the local player's work record. */
extern "C" void pl_area_entry_set(nw4r::math::VEC3* pos, u16 angle, u8 map, u8 area) {
    _PLW* players = (_PLW*)get_move_work_adrs(2);
    pl_hit_effect_spawn(&players[(s8)my_player_no()], pos, angle, map, area, 0);
}

/* Scans the equipment slots for the first crafted item and records its index. */
extern "C" void fn_802466C4(_PLW* self) {
    s32 i;
    self->field_0x304 = 0;
    for (i = 0; i < 0x1A; i++) {
        u16 item_id = self->slot_id[i].item_id;
        if (self->slot_id[i].value > 0) {
            u8* item = GetItemData(item_id);
            if (item_id != 0 && (item[2] & 8) != 0 && item[0] != 1) {
                self->field_0x304 = (u8)i;
                break;
            }
        }
    }
}

/* Builds the actor's first frame: places it at the current area's entry, resets every act and effect state,
 * binds its models, effects and SE work, and loads the master's decoration skills and hunter id. */
extern "C" void fn_8024676C(_PLW* self) {
    MHchar* chr;
    u8 map;
    u8 area;
    u16 angle;
    u16 slot;
    s32 i;

    chr = &((PlBodyWork*)self->physics_0x13C)->chr_0x04;
    self->field_0x004++;
    self->field_0x267 = 1;
    if (Pl_master_ck(self) == 1) {
        self->field_0x001 = 1;
    } else {
        self->field_0x001 = 0;
    }
    self->field_0x01E = 0;
    self->field_0x354 = pl_frame_window_1;
    self->drawn_0x01A = 0;
    map = get_now_mapno();
    area = get_now_areano();
    angle = stage_entry_angle_get(map, area);
    pl_pos_place(self, stage_entry_pos_get(map, area), angle, map, area, 0);
    pl_act_stage_latch_set(self, 0);
    self->field_0x017 = self->area_0x16;
    pl_act_state_reset(self);
    pl_weapon_record_rearm(self);
    self->field_0x0B0 = 0;
    self->field_0x0B2 = 0;
    self->field_0x266 = 0;
    self->field_0x369 = 0;
    self->field_0x077 = 0;
    self->kind_0x09 = 0;
    fn_8025F478(self);
    pl_act_reset_idle(self, self->kind_0x09, 0, 0);
    pl_act_set_step_time(self, 0);
    self->field_0x45A = 0;
    self->field_0x45C = 0;
    self->field_0x412 = 0;
    self->field_0x352 = 0;
    pl_act_set_gauge_arm(self, 30);
    self->field_0x38A = 0;
    self->field_0x074 = 0;
    self->field_0x075 = 0;
    self->field_0x076 = 0;
    self->field_0x654 = 0;
    self->field_0x660 = 0;
    self->field_0x65F = 0;
    self->field_0x442 = 0;
    self->field_0x470 = 0;
    self->field_0x453 = 0;
    slot = pl_item_slot_next_get(self, 0, 2);
    self->item_slot_sel = slot;
    self->magazine_0x270 = pl_item_slot_count_get(self, slot);
    pl_held_item_refresh(self);
    self->field_0x30C = 0;
    self->field_0x30D = 0;
    self->flag_0x30E = 0;
    self->field_0x30F = 0;
    fn_802466C4(self);
    self->field_0x658 = 0;
    self->field_0x268 = 0;
    self->field_0x47B = 0;
    self->field_0x47E = 0;
    self->field_0x481 = 0;
    self->field_0x47C = 0;
    self->field_0x47F = 0;
    self->field_0x482 = 0;
    self->field_0x47D = 0;
    self->field_0x480 = 0;
    self->field_0x483 = 0;
    self->talk_left_0x669 = 0;
    self->talk_wait_0x668 = 0;
    self->field_0x66A = 0;
    self->field_0x445 = 0;
    self->field_0x446 = 0;
    self->field_0x447 = 0;
    self->field_0x584 = 0;
    self->field_0x256 = 0;
    self->field_0x659 = 0;
    self->field_0x65A = 0;
    self->field_0x662 = 0;
    eft_pl_model_slot_bind(self);
    ef_pl_effects_init(self);
    eft_frame_pl_create(self, pl_frame_window_1);
    eft021_pl_create(self, 0, 0);
    eft013_set(self, 6);
    eft013_set(self, 7);
    eft013_set_pl_model((struct _EFT013_PL*)self, 0xF);
    eft033_set_gun(self);
    yure_player_init((YurePlayerWork*)self);
    self->field_0xAF4 = pl_se_entry_request(1, self, pl_se_frame_callback);
    self->field_0xAF8 = pl_se_entry_request(2, self, NULL);
    self->field_0xAFC = pl_se_entry_request(3, self, pl_se_kind_dispatch);
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(chr);
    self->field_0x352 = 200;
    pl_equip_refresh(self);
    for (i = 0; i < 4; i++) {
        if (Pl_master_ck(self) == 1) {
            self->deco_skill_id[i] = lb_param_w.slots_0x16[i];
        } else {
            self->deco_skill_id[i] = 0;
        }
    }
    if (Pl_master_ck(self) == 1 && isServerSelectState() == 1) {
        strcpy(self->hunter_0x5CA.id_0x5DB, lobby_hunter_cards[0].id_0x03);
    }
}

/* Reads the ground height under the actor into `*height` and returns the probe result (0 when neither probe hits,
 * 2 when only the upper one does): the lower ground wins when both hit, and the actor's own height stands in when
 * neither does. */
extern "C" s32 pl_ground_height_probe(_PLW* self, f32* height) {
    VEC3 land;
    f32 below;
    s32 hit;

    em_land_data_init(&land);
    if (pl_ground_check(self, &self->vec_0x03C, &land, &below, 4) != 0) {
        if (pl_ground_check(self, &self->vec_0x03C, &land, height, -6) == 0) {
            *height = below;
            hit = 2;
        } else {
            hit = 1;
        }
    } else {
        hit = pl_ground_check(self, &self->vec_0x03C, &land, height, -6) != 0;
    }
    if (hit == 0) {
        *height = self->vec_0x03C.y;
    }
    return hit;
}

/* The rest act: arms the idle motion and its step table, then after 30 frames plays the sit-down variant, and
 * steps the follow-up motions as each one ends. */
extern "C" void pl_act_rest(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8000, 0, 0);
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        } else {
            fn_802771A0(self, 0);
        }
        self->field_0x28 = 0;
        /* fallthrough */
    case 1:
        self->field_0x28++;
        if (self->field_0x28 > 30 && Get_motion_no(self) == 1) {
            if (self->se_name_set == 0) {
                Pl_chr_set_attr_default(self, 0x42, 6, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x3F, 6, 0);
            }
        }
        if (Pl_master_ck(self) == 1 && pl_item_page2_ck(self) == 1 && self->field_0x18 == 1) {
            pl_motion_stick_move(self, 0x3E9, 0);
        }
        switch (Get_motion_no(self)) {
        case 0x15:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 5, 4, 0x90);
                return;
            }
            return;
        case 0x3F:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x41, 2, 0);
                return;
            }
            break;
        case 0x42:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x43, 2, 0);
                return;
            }
            break;
        default:
            if (self->field_0x18 != 1 && (s16)self->field_0x37A <= 150) {
                Pl_chr_setX(self, 0x12F, 4, 0);
            }
            break;
        }
        break;
    }
}

/* The sheathe act for `variant` 0-3: arms the sheathe motion of the weapon kind (and its alternate form) on the
 * first step, then switches to the idle slot when the motion ends. */
extern "C" void pl_act_sheathe(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 1;
        self->field_0x5C6 = 0;
        if (self->field_0x002 != 1) {
            pl_act_clear_flag5bb(self);
        }
        switch (variant) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (pl_weapon_alt_form_ck_c1(self) == 1) {
                    Pl_chr_set_attr_default(self, 0x40B, 4, 0);
                } else if (self->field_0x002 == 3) {
                    Pl_chr_set_attr_default(self, 0x3EA, 2, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x3EA, 4, 0);
                }
            } else {
                Pl_chr_set_attr_default(self, 0x3EA, 4, 0);
            }
            break;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (pl_weapon_alt_form_ck_c1(self) == 1) {
                    Pl_chr_set_attr_default(self, 0x43A, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x41B, 4, 0);
                }
            } else {
                Pl_chr_set_attr_default(self, 0x41B, 4, 0);
            }
            break;
        case 2:
            Pl_act_set_motion(self, 0, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (self->field_0x002 == 6) {
                    if (pl_weapon_alt_form_ck_c1(self) == 1) {
                        Pl_chr_set_attr_default(self, 0x410, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x3EA, 4, 0);
                    }
                } else {
                    if (pl_weapon_alt_form_ck_c1(self) == 1) {
                        Pl_chr_set_attr_default(self, 0x40B, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x3EA, 4, 0);
                    }
                }
            } else {
                Pl_chr_set_attr_default(self, 0x3EA, 4, 0);
            }
            break;
        case 3:
            Pl_act_set_motion(self, 3, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (self->field_0x002 == 6) {
                    if (pl_weapon_alt_form_ck_c1(self) == 1) {
                        Pl_chr_set_attr_default(self, 0x43D, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x41B, 4, 0);
                    }
                } else {
                    Pl_chr_set_attr_default(self, 0x43D, 4, 0);
                }
            }
            break;
        }
        pl_act_clear_mode5c4(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (variant == 1 || variant == 3) {
                Pl_act_set_motion_slot(self, 3, 4, 0);
            } else {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            }
        }
        break;
    }
}

/* The draw act for `variant` 0-3: arms the draw motion of the weapon kind from the per-kind attribute tables on
 * the first step, then switches to the kind's armed slot when the motion ends. */
extern "C" void pl_act_draw(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        switch (variant) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            if (pl_item_page2_ck(self) == 1 && pl_weapon_alt_form_ck_c1(self) == 1) {
                Pl_chr_set_attr_default(self, 0x40C, pl_draw_attr_tbl[self->field_0x002 * 2 + 0], 0);
            } else {
                Pl_chr_set_attr_default(self, 0x3EB, pl_draw_attr_tbl[self->field_0x002 * 2 + 0], 0);
            }
            break;
        case 1:
            Pl_act_set_motion(self, 0, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (((s16 (*)(_PLW*))pl_act_kind_column_get)(self) == 1) {
                    if (pl_weapon_alt_form_ck_c1(self) == 0) {
                        Pl_chr_set_attr_default(self, 0x40F, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x40D, 4, 0);
                    }
                } else {
                    if (pl_weapon_alt_form_ck_c1(self) == 0) {
                        Pl_chr_set_attr_default(self, 0x3F1, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x40E, 4, 0);
                    }
                }
            } else {
                Pl_chr_set_attr_default(self, pl_draw_attr_tbl_b[self->field_0x002 * 3 + 0], pl_draw_attr_tbl_b[self->field_0x002 * 3 + 1], 0);
            }
            break;
        case 2:
            Pl_act_set_motion(self, 3, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (pl_weapon_alt_form_ck_c1(self) == 1) {
                    Pl_chr_set_attr_default(self, 0x43B, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x41C, 4, 0);
                }
            } else {
                Pl_chr_set_attr_default(self, 0x41C, pl_draw_attr_tbl_c[self->field_0x002 * 2 + 0], 0);
            }
            break;
        case 3:
            Pl_act_set_motion(self, 3, 0, 0);
            if (pl_item_page2_ck(self) == 1) {
                if (pl_weapon_alt_form_ck_c1(self) == 1) {
                    Pl_chr_set_attr_default(self, 0x43C, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x421, 4, 0);
                }
            } else {
                Pl_chr_set_attr_default(self, 0x421, pl_draw_attr_tbl_d[self->field_0x002 * 2 + 0], 0);
            }
            break;
        }
        if (Pl_master_ck(self) == 1 && pl_item_page2_ck(self) == 1) {
            pl_model_set_value(self, 0, pl_frame_window_1);
            pl_model_set_value(self, 1, pl_float_zero);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            switch (variant) {
            case 0:
                Pl_act_set_motion_slot(self, 0, pl_draw_attr_tbl[self->field_0x002 * 2 + 1], 0);
                break;
            case 1:
                Pl_act_set_motion_slot(self, 0, pl_draw_attr_tbl_b[self->field_0x002 * 3 + 2], 0);
                break;
            case 2:
                Pl_act_set_motion_slot(self, 3, pl_draw_attr_tbl_c[self->field_0x002 * 2 + 1], 0);
                break;
            case 3:
                Pl_act_set_motion_slot(self, 3, pl_draw_attr_tbl_d[self->field_0x002 * 2 + 1], 0);
                break;
            }
        }
        break;
    }
}

/* The step-down act: arms the motion and moves the actor one step ahead onto the ground there, then switches to
 * the idle slot when the motion ends. */
extern "C" void pl_act_step_down(_PLW* self, s32 variant) {
    VEC3 step;
    VEC3 probe;
    VEC3 ahead;

    VEC3_ctor(&step);
    VEC3_ctor(&probe);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_set_frame_timer(self);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x12, 0, 0x58);
        } else {
            Pl_chr_set_attr_default(self, 0x1A7, 0, 0);
        }
        self->field_0x0A8 = self->rot_0x54.y;
        Pl_act_set_motion(self, 0, 0, 0);
        step.x = pl_float_zero;
        step.y = pl_frame_window_180;
        step.z = pl_frame_window_55;
        rotVecY(&step, self->rot_0x54.y);
        addVec3(&ahead, &self->vec_0x03C, &step);
        copyVec3(&probe, &ahead);
        self->vec_0x03C.y = GetGroundHit(&probe, -6, self->area_0x16);
        self->vec_0x03C.x += step.x;
        self->vec_0x03C.z += step.z;
        se_step_pos_set(&self->vec_0x03C);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The step-up act: moves the actor one step ahead onto the ground there and holds a 30-frame guard; the master
 * leaves it on a dash input or a raised part flag, and re-enters the guard on a held button once it has run out. */
extern "C" void pl_act_step_up(_PLW* self) {
    VEC3 step;
    VEC3 probe;
    VEC3 ahead;

    VEC3_ctor(&step);
    VEC3_ctor(&probe);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x0A8 = self->rot_0x54.y;
        Pl_chr_set_attr_default(self, 0x11, 0, 0);
        pl_act_set_frame_timer(self);
        self->field_0x28 = 30;
        step.x = pl_float_zero;
        step.y = pl_frame_window_210;
        step.z = pl_frame_window_55;
        rotVecY(&step, self->rot_0x54.y);
        addVec3(&ahead, &self->vec_0x03C, &step);
        copyVec3(&probe, &ahead);
        self->vec_0x03C.y = GetGroundHit(&probe, -6, self->area_0x16);
        self->vec_0x03C.x += step.x;
        self->vec_0x03C.z += step.z;
        pl_act_clear_wait(self, 0);
        se_step_pos_set(&self->vec_0x03C);
        if (self->prev_act_kind == 2 && self->prev_act_no == 1) {
            self->field_0x5C7++;
        }
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
            if (self->field_0x28 <= 0) {
                self->field_0x28 = 0;
            }
        }
        if (Pl_master_ck(self) != 0) {
            if (((s32 (*)(_PLW*))pl_act_dash_input_ck)(self) == 1 || pl_part_flag_ck(self, 6) == 1 ||
                (self->field_0x0C8 & 0x2000) != 0) {
                pl_act_enter(self, 0, 7, 0);
            } else if ((self->field_0x0CC & 0x1000) != 0 && self->field_0x28 == 0) {
                pl_act_enter(self, 2, 6, 0);
            }
        }
        break;
    }
}

/* Drives the act's first two steps: arm the motion and hold, then wait for it. */
extern "C" void fn_802478A4(_PLW* self) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x12, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The guard-break act for `variant` 0-2: resets the act's latches and arms the stagger motion (or the drawn
 * weapon's step table), then steps the follow-up motion; with skill 0xB6 the actor slides ten units back. */
extern "C" void pl_act_flinch(_PLW* self, s32 variant) {
    VEC3 slide;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->field_0x28 = 0;
        if (variant == 2) {
            fn_80277BC4(self, 1);
        } else {
            fn_80277BC4(self, 0);
        }
        pl_act_set_motion_gate(self, 0);
        pl_sub_gauge_skill_add(self, -150);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        pl_stun_tick(self);
        pl_guard_latch_set(self);
        pl_camera_lock_clear(self);
        pl_act_clear_mode5c4(self);
        if (variant == 0 || variant == 2) {
            self->act_step_0x05 = 2;
            Pl_act_set_motion(self, 0, 0, 0);
            if ((self->field_0x002 == 2 || pl_item_page2_ck(self) == 1) && self->field_0x18 == 1) {
                Pl_chr_set_attr_default(self, 0x3F3, 2, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x1C, 2, 0);
            }
            if (self->field_0x18 == 1) {
                switch (self->field_0x002) {
                case 1:
                    Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind1, 0);
                    break;
                case 2:
                    Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind2, 0);
                    break;
                case 8:
                    if (pl_mode8_free_ck() == 0) {
                        Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind8, 0);
                    }
                    break;
                }
            }
        } else {
            self->act_step_0x05++;
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x16, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x1C, 0, 6);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 8, 0);
        } else if (Pl_Skill_ck(self, 0xB6) == 1 &&
                   Pl_frame_check(self, 3, pl_frame_window_10, pl_frame_window_40) == 1) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_frame_window_10;
            rotVecY(&slide, self->rot_0x54.y);
            self->vec_0x03C.x += slide.x;
            self->vec_0x03C.z += slide.z;
        }
        break;
    }
}

/* Halves (or thirds) the act's damage figure from the equip skills and hands it to the health change
 * helper. */
extern "C" void fn_80247C2C(_PLW* self) {
    s16 amount;
    if (Pl_Skill_ck(self, 0xA8) == 1) {
        amount = -0x10;
    } else if (Pl_Skill_ck(self, 0xA9) == 1) {
        amount = -0x24;
    } else {
        amount = -0x1E;
    }
    if (Pl_cat_skill_ck(self, 0x21) == 1) {
        amount = (s16)amount / 2;
    }
    pl_sub_gauge_add(self, amount);
}

/* 0x80247CC4 (0xB0): Steps the actor forward along its facing and picks the ground/air kind from the height. */
extern "C" void fn_80247CC4(_PLW* self) {
    VEC3 work;
    VEC3 vec;
    VEC3_ctor(&vec);
    em_land_data_init(&work);
    vec.x = pl_float_zero;
    vec.y = pl_float_zero;
    vec.z = pl_frame_window_3;
    rotVecY(&vec, self->field_0x058);
    self->motion_pos_0x3C = self->motion_pos_0x3C + vec.x;
    self->motion_pos_0x44 = self->motion_pos_0x44 + vec.z;
    if (self->motion_pos_0x40 <= self->field_0x064 && (self->field_0x5A4 & 1) != 0) {
        self->kind_0x09 = 3;
    } else {
        self->kind_0x09 = 2;
    }
}

/* The swim/ladder input check: without the hold latch the actor drops (or holds on, below its ground mark);
 * with a direction input it enters the stroke act `variant` and the held part flag pick.  Returns 1 when an act
 * was entered. */
extern "C" s32 pl_act_stroke_input_ck(_PLW* self, s32 variant) {
    u16 actHeld;
    u16 actFree;

    if (self->field_0x369 == 0) {
        if (variant == 0) {
            pl_act_enter(self, 2, 5, 0);
        } else if (self->vec_0x03C.y > self->field_0x064) {
            pl_act_enter(self, 2, 5, 0);
        } else {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        return 1;
    }
    if ((self->field_0x0C8 & 0x1000) != 0) {
        if (variant == 0) {
            actHeld = 0x67;
            actFree = 0xD;
        } else {
            actHeld = 0x69;
            actFree = 0x4D;
        }
    } else if ((self->field_0x0C8 & 0x2000) != 0) {
        if (variant == 0) {
            actHeld = 0x66;
            actFree = 0xC;
        } else {
            actHeld = 0x68;
            actFree = 0x4C;
        }
    } else {
        return 0;
    }
    if (pl_part_flag_ck(self, 0) == 1) {
        fn_80247C2C(self);
        if (Pl_act_ck(self, 0, actHeld) == 0) {
            pl_act_enter(self, 0, actHeld, 0);
        }
    } else if (Pl_act_ck(self, 0, actFree) == 0) {
        pl_act_enter(self, 0, actFree, 0);
    }
    return 1;
}

/* Runs the first two act steps of the alternate chase and then the shared chase update. */
extern "C" void fn_80247EF0(_PLW* self, u32 a) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            Pl_act_set_motion(self, 2, 0, 0);
            Pl_chr_set_attr_default(self, 0x22, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x8B, 4, 0);
        }
        self->field_0x585 = 0;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_90, pl_float_zero) == 1) {
            if (a == 0) {
                pl_act_enter(self, 0, 0xB, 0);
            } else {
                pl_act_enter(self, 0, 0x4B, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The stroke-hold act: arms the hold motion with the frame window of the motion it follows, then lets the master
 * leave it on input or a raised part flag. */
extern "C" void pl_act_stroke_hold(_PLW* self, s32 variant) {
    pl_act_set_step_time(self, 2);
    pl_model_visible_set(self, 1);
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_act_set_motion(self, 0x8002, 0, 0);
        } else {
            Pl_act_set_motion(self, 0x8003, 0, 0);
        }
        switch (Get_motion_no(self)) {
        case 0x22:
            if (Pl_frame_check(self, 0, pl_frame_window_48, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x22, 0, 0x30);
            } else {
                Pl_chr_set_attr_default(self, 0x22, 0, 0x5A);
            }
            break;
        case 0x8B:
            if (Pl_frame_check(self, 0, pl_frame_window_48, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x8B, 0, 0x30);
            } else {
                Pl_chr_set_attr_default(self, 0x8B, 0, 0x5A);
            }
            break;
        case 0x23:
            if (Pl_frame_check(self, 0, pl_frame_window_2, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x23, 0, 2);
            } else {
                Pl_chr_set_attr_default(self, 0x23, 0, 0x2C);
            }
            break;
        case 0x8C:
            if (Pl_frame_check(self, 0, pl_frame_window_2, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x8C, 0, 2);
            } else {
                Pl_chr_set_attr_default(self, 0x8C, 0, 0x2C);
            }
            break;
        }
    } else if (Pl_master_ck(self) == 1 && pl_act_stroke_input_ck(self, variant) == 0 && pl_part_flag_ck(self, 7) == 1) {
        if (variant == 0) {
            pl_act_enter(self, 2, 5, 0);
        } else if (self->vec_0x03C.y > self->field_0x064) {
            pl_act_enter(self, 2, 5, 0);
        } else {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        return;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The forward stroke act (`fast` doubles as the dash): arms the stroke motion, re-enters the hold act at the
 * stroke's frame windows, drops the actor when its stamina runs out and climbs out at a ledge ahead. */
extern "C" void pl_act_stroke_forward(_PLW* self, s32 variant, s32 fast) {
    VEC3 ledge;

    pl_act_set_step_time(self, 2);
    if (fast == 0) {
        self->field_0x354 = pl_float_0_8;
    } else {
        self->field_0x354 = pl_float_1_25;
        pl_act_arm_flags(self, 1);
    }
    pl_step_se_kind_get(self);
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_act_set_motion(self, 2, 0, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
        }
        self->field_0x585 = 0;
        switch (Get_motion_no(self)) {
        case 0x23:
            if (Pl_frame_check(self, 1, pl_frame_window_40, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x22, 0, 0x5A);
            } else {
                Pl_chr_set_attr_default(self, 0x22, 0, 0x30);
            }
            break;
        case 0x8C:
            if (Pl_frame_check(self, 1, pl_frame_window_40, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x8B, 0, 0x5A);
            } else {
                Pl_chr_set_attr_default(self, 0x8B, 0, 0x30);
            }
            break;
        default:
            if (variant == 0) {
                Pl_chr_setX(self, 0x22, 4, 0);
            } else {
                Pl_chr_setX(self, 0x8B, 4, 0);
            }
            break;
        }
    } else if ((Pl_frame_check(self, 0, pl_frame_window_48, pl_float_zero) == 1 ||
                Pl_frame_check(self, 0, pl_frame_window_90, pl_float_zero) == 1) &&
               Pl_master_ck(self) == 1 && pl_act_stroke_input_ck(self, variant) == 0) {
        if (variant == 0) {
            pl_act_enter(self, 0, 0xB, 0);
        } else {
            pl_act_enter(self, 0, 0x4B, 0);
        }
    }
    if ((s16)self->field_0x378 <= 0) {
        if (variant == 0) {
            pl_act_enter(self, 2, 6, 0);
        } else if (self->vec_0x03C.y > self->field_0x064) {
            pl_act_enter(self, 2, 6, 0);
        } else {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        return;
    }
    if (pl_ground_probe_ahead(self, &ledge, pl_frame_window_180, pl_frame_window_30) == 1) {
        self->field_0x0A8 = self->rot_0x54.y;
        pl_act_enter(self, 0, 6, 0);
        return;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The backward stroke act: as the forward one, but it climbs out when the actor sinks to its ground mark. */
extern "C" void pl_act_stroke_back(_PLW* self, s32 variant, s32 fast) {
    pl_act_set_step_time(self, 2);
    if (fast == 0) {
        self->field_0x354 = pl_float_0_8;
    } else {
        self->field_0x354 = pl_float_1_25;
        pl_act_arm_flags(self, 1);
    }
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_act_set_motion(self, 2, 0, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
        }
        self->field_0x585 = 0;
        switch (Get_motion_no(self)) {
        case 0x22:
            if (Pl_frame_check(self, 1, pl_frame_window_86, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x23, 0, 0x2C);
            } else {
                Pl_chr_set_attr_default(self, 0x23, 0, 2);
            }
            break;
        case 0x8B:
            if (Pl_frame_check(self, 1, pl_frame_window_86, pl_float_zero) == 1) {
                Pl_chr_set_attr_default(self, 0x8C, 0, 0x2C);
            } else {
                Pl_chr_set_attr_default(self, 0x8C, 0, 2);
            }
            break;
        default:
            if (variant == 0) {
                Pl_chr_setX(self, 0x23, 4, 2);
            } else {
                Pl_chr_setX(self, 0x8C, 4, 2);
            }
            break;
        }
    } else if ((Pl_frame_check(self, 0, pl_frame_window_2, pl_float_zero) == 1 ||
                Pl_frame_check(self, 0, pl_frame_window_44, pl_float_zero) == 1) &&
               Pl_master_ck(self) == 1 && pl_act_stroke_input_ck(self, variant) == 0) {
        if (variant == 0) {
            pl_act_enter(self, 0, 0xB, 0);
        } else {
            pl_act_enter(self, 0, 0x4B, 0);
        }
    }
    if ((s16)self->field_0x378 <= 0) {
        if (variant == 0) {
            pl_act_enter(self, 2, 6, 0);
        } else if (self->vec_0x03C.y > self->field_0x064) {
            pl_act_enter(self, 2, 6, 0);
        } else {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        return;
    }
    if (self->vec_0x03C.y <= self->ground_y_0x060) {
        if (variant == 0) {
            pl_act_enter(self, 0, 0xE, 0);
        } else {
            pl_act_enter(self, 0, 0x4E, 0);
        }
        return;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The close-quarters variant of the same two-step chase. */
extern "C" void fn_802488D4(_PLW* self, u32 a) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x24, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x8D, 6, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (a == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The knockback act for `variant` 0-4 (`level` picks the bow kind's attribute pair, `alt` the bow's alternate step
 * table): resets the act's latches and arms the knockback motion and step table, then steps the motion and the
 * per-kind slides the 0xB6 skill grants. */
#pragma optimization_level 4
extern "C" void pl_act_knockback(_PLW* self, s32 variant, s32 level, u8 alt) {
    VEC3 slide;
    s32 part;
    s32 frame;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (variant == 4) {
            fn_80277BC4(self, 1);
        } else {
            fn_80277BC4(self, 0);
        }
        Pl_act_set_motion(self, 0, 0, 0);
        pl_camera_lock_clear(self);
        pl_act_set_motion_gate(self, 0);
        pl_sub_gauge_skill_add(self, -150);
        pl_guard_latch_set(self);
        if (self->field_0x002 <= 2 || self->field_0x002 == 5 || self->field_0x002 == 7) {
            pl_stun_tick(self);
        }
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        if (level == 0) {
            part = 2;
            frame = 0;
        } else if (self->field_0x002 != 6) {
            part = 2;
            frame = 0;
        } else {
            part = pl_knockback_bow_attr[level * 2];
            frame = pl_knockback_bow_attr[level * 2 + 1];
        }
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0x3F5, part, frame);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x3F4, part, frame);
            break;
        case 2:
        case 4:
            Pl_chr_set_attr_default(self, 0x3F6, part, frame);
            break;
        case 3:
            Pl_chr_set_attr_default(self, 0x46F, part, frame);
            break;
        }
        pl_act_set_frame_timer(self);
        if (self->field_0x18 == 1 && variant != 3) {
            switch (self->field_0x002) {
            case 1:
                Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind1, 0);
                break;
            case 2:
                Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind2, 0);
                break;
            case 3:
                if (level <= 1) {
                    if (alt == 0) {
                        Pl_act_set_step_table(self, pl_knockback_bow_steps[level + variant * 2], 0);
                    } else {
                        Pl_act_set_step_table(self, (u32)pl_knockback_steps_bow_alt, 0);
                    }
                }
                break;
            }
        }
        break;
    case 1:
        if (variant != 3) {
            self->act_step_0x05++;
        } else {
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x3F6, 0, 0xA);
            }
            break;
        }
        /* fallthrough */
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 8, 0);
            break;
        }
        if (alt != 0 && self->field_0x002 == 3 && Pl_frame_check(self, 1, pl_frame_window_28, pl_float_zero) == 0) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_float_neg8;
            rotVecXYZ(&slide, &self->rot_0x54);
            addVec3To(&self->vec_0x03C, &slide);
        }
        if ((u32)variant <= 1 && self->field_0x002 == 8 &&
            Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
            slide.x = pl_frame_window_10;
            if (variant == 0) {
                slide.x = pl_frame_window_10 * pl_float_neg1;
            }
            slide.y = pl_float_zero;
            slide.z = pl_float_zero;
            rotVecY(&slide, self->rot_0x54.y);
            self->vec_0x03C.x += slide.x;
            self->vec_0x03C.z += slide.z;
        }
        if (Pl_Skill_ck(self, 0xB6) == 1) {
            if ((u32)(variant - 2) > 2) {
                if ((u32)variant <= 1) {
                    switch (self->field_0x002) {
                    case 3:
                    case 8:
                        if (Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
                            slide.x = pl_frame_window_8;
                            if (variant == 0) {
                                slide.x = pl_frame_window_8 * pl_float_neg1;
                            }
                            slide.y = pl_float_zero;
                            slide.z = pl_float_zero;
                            rotVecY(&slide, self->rot_0x54.y);
                            self->vec_0x03C.x += slide.x;
                            self->vec_0x03C.z += slide.z;
                        }
                        break;
                    case 4:
                    case 5:
                    case 6:
                        if (Pl_frame_check(self, 3, pl_frame_window_10, pl_frame_window_28) == 1) {
                            slide.x = pl_frame_window_10;
                            if (variant == 0) {
                                slide.x = pl_frame_window_10 * pl_float_neg1;
                            }
                            slide.y = pl_float_zero;
                            slide.z = pl_float_zero;
                            rotVecY(&slide, self->rot_0x54.y);
                            self->vec_0x03C.x += slide.x;
                            self->vec_0x03C.z += slide.z;
                        }
                        break;
                    case 0:
                    case 1:
                    case 2:
                    case 7:
                        if (Pl_frame_check(self, 3, pl_frame_window_10, pl_frame_window_40) == 1) {
                            slide.x = pl_frame_window_8;
                            if (variant == 0) {
                                slide.x = pl_frame_window_8 * pl_float_neg1;
                            }
                            slide.y = pl_float_zero;
                            slide.z = pl_float_zero;
                            rotVecY(&slide, self->rot_0x54.y);
                            self->vec_0x03C.x += slide.x;
                            self->vec_0x03C.z += slide.z;
                        }
                        break;
                    }
                }
            } else if (Pl_frame_check(self, 2, pl_frame_window_28, pl_float_zero) == 1) {
                slide.x = pl_float_zero;
                slide.y = pl_float_zero;
                slide.z = pl_float_neg10;
                rotVecY(&slide, self->rot_0x54.y);
                self->vec_0x03C.x += slide.x;
                self->vec_0x03C.z += slide.z;
            }
        }
        break;
    }
}
#pragma optimization_level reset

/* The item-use act for `kind` 0-5: arms the use motion from the item act table, fires the item's shell at the
 * use frame (spending one unless the infinite-item cheat input is held) and, for the return items, warps the
 * master to the camp entry when the motion ends. */
extern "C" void pl_act_item_throw(_PLW* self, s32 kind) {
    VEC3 camp;
    s16* act;
    f32 useFrame;

    setVec3(&camp, pl_float_zero, pl_float_zero, pl_float_zero);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        act = &pl_item_throw_tbl[kind * 4];
        Pl_act_set_motion(self, act[0], 0, 0);
        Pl_chr_set_attr_default(self, act[1], act[2], 0);
        break;
    case 1:
        if (kind == 0) {
            useFrame = pl_frame_window_26;
        } else if (kind == 3) {
            useFrame = pl_frame_window_22;
        } else {
            useFrame = pl_frame_window_20;
        }
        if (Pl_frame_check(self, 0, useFrame, pl_float_zero) == 1) {
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
            switch (self->field_0x306) {
            case 0x60:
                shell_set_func_ptr->method_0x00(self, 8, 2, shell_set_func_ptr);
                break;
            case 0x61:
                shell_set_func_ptr->method_0x00(self, 0, 2, shell_set_func_ptr);
                break;
            case 0x32:
            case 0x5:
                shell_set_func_ptr->method_0x00(self, 2, 2, shell_set_func_ptr);
                break;
            case 0x21:
                shell_set_func_ptr->method_0x00(self, 1, 2, shell_set_func_ptr);
                break;
            case 0x7:
                shell_set_func_ptr->method_0x00(self, 0xA, 2, shell_set_func_ptr);
                break;
            case 0x159:
                shell_set_func_ptr->method_0x00(self, 0xB, 2, shell_set_func_ptr);
                break;
            case 0x15A:
                shell_set_func_ptr->method_0x00(self, 0xE, 2, shell_set_func_ptr);
                break;
            case 0x15B:
                shell_set_func_ptr->method_0x00(self, 0xD, 2, shell_set_func_ptr);
                break;
            case 0x16E:
                shell_set_func_ptr->method_0x00(self, 0xC, 2, shell_set_func_ptr);
                break;
            case 0x8:
                shell_set_func_ptr->method_0x00(self, 3, 2, shell_set_func_ptr);
                break;
            case 0x9:
                shell_set_func_ptr->method_0x00(self, 4, 2, shell_set_func_ptr);
                break;
            case 0xB:
                shell_set_func_ptr->method_0x00(self, 6, 2, shell_set_func_ptr);
                break;
            case 0xA:
                shell_set_func_ptr->method_0x00(self, 5, 2, shell_set_func_ptr);
                break;
            case 0xC:
                shell_set_func_ptr->method_0x00(self, 7, 2, shell_set_func_ptr);
                break;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            if (kind != 2 && kind != 5) {
                Pl_act_set_motion_slot(self, self->kind_0x09, pl_item_throw_tbl[kind * 4 + 3], 0);
            } else if (Pl_master_ck(self) == 1) {
                copyVec3(&camp, stage_entry_pos_get(self->kind_0x015, 0));
                pl_act_stage_latch_set(self, 1);
                pl_warp_start(0, &camp, 0);
                pl_act_set_gauge_arm(self, 2);
                Pl_act_set_motion_slot(self, self->kind_0x09, pl_item_throw_tbl[kind * 4 + 3], 0);
            } else {
                Pl_act_set_motion_slot(self, self->kind_0x09, pl_item_throw_tbl[kind * 4 + 3], 0);
            }
        }
        break;
    }
}

/* 0x80249424 - the "down/knock-down" act handler: arms the down motion, then ends the act once the
 * master gate and the hit check agree. */
extern "C" void fn_80249424(_PLW* self, s32 arg1) {
    pl_camera_lock_clear(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3C, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x87, 4, 0);
        }
        self->act_end_request = 0;
        eft004_hold_item_spawn(self);
        return;
    case 1:
        if (Pl_master_ck(self) != 0 && (pl_part_flag_ck(self, 4) == 1 || self->act_end_request != 0)) {
            if (arg1 == 0) {
                pl_act_enter(self, 0, 0x15, 0);
                return;
            }
            pl_act_enter(self, 0, 0x9F, 0);
        }
        return;
    }
}

/* 0x80249558 - the get-up handler: arms the get-up motion, then hands the actor's kind to the motion
 * table once the pose check passes. */
extern "C" void fn_80249558(_PLW* self, s32 arg1) {
    pl_camera_lock_clear(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3D, 4, 0);
            return;
        }
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x88, 4, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        return;
    }
}

/* 0x8024963C - the get-up handler with an item-recovery arm: on `arg2 == 1` it reads the pending
 * recovery item and feeds it to `pl_item_add`. */
extern "C" void fn_8024963C(_PLW* self, s32 arg1, s32 arg2) {
    u16 item;
    s16 value;

    pl_camera_lock_clear(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x200, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x214, 4, 0);
        }
        if (arg2 == 1) {
            ai_npc_item_give(&item, &value);
            if (Pl_motion_input_ck(1) == 0 && value > 0) {
                pl_item_add(self, item, value);
            }
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        /* the last case has no branch: retail falls straight into the epilogue */
    }
}

/* 0x80249768 - the "hold/struggle" handler: arms the struggle motion and gives up after 600 frames
 * or when the act-end signal arrives. */
extern "C" void fn_80249768(_PLW* self, s32 arg1) {
    s32 timer;

    self->act_handler_entered = 1;
    cockpit_ticket_acquire(self, &self->field_0x598);
    pl_camera_lock_clear(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_setX(self, 1, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_setX(self, 0x64, 6, 0);
        }
        pl_act_set_frame_timer(self);
        if (isServerSelectState() == 1) {
            self->act_end_request = 0;
        }
        self->field_0x28 = 0;
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        timer = self->field_0x28 + 1;
        self->field_0x28 = timer;
        if (timer > 0x258 || self->act_end_request != 0) {
            if (self->act_end_request == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x49, 0);
                    return;
                }
                pl_act_enter(self, 0, 0xA0, 0);
                return;
            }
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
            return;
        }
        return;
    }
}

/* 0x802498E0 - the "stagger/recover" handler: it either arms a fixed motion table or hands the choice
 * to `fn_802771A0`, then picks the follow-up motion from the actor's state. */
extern "C" void fn_802498E0(_PLW* self) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8003, 0, 0);
        self->field_0x28 = 0x1E;
        if (self->field_0x018 == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805BE824, 0);
            return;
        }
        fn_802771A0(self, 0);
        return;
    case 1:
        if (self->field_0x28 > 0 || pl_target_lock_ck(self) == 0) {
            timer = self->field_0x28 - 1;
            self->field_0x28 = timer;
            if (timer <= 0) {
                self->field_0x0AC = 0;
            }
        }
        if (Pl_master_ck(self) == 1 && pl_item_page2_ck(self) == 1 && self->field_0x018 == 1) {
            pl_motion_stick_move(self, 0x41A, 0);
        }
        if (self->field_0x018 != 1) {
            if (fn_80276800(self, 0) == 1) {
                Pl_chr_setX(self, 0x79, 4, 0);
                return;
            }
            if (self->field_0x37A <= 0x96) {
                Pl_chr_setX(self, 0x168, 4, 0);
                return;
            }
            if (Pl_suimen_ck(self) == 1) {
                Pl_chr_setX(self, 0x76, 4, 0);
                return;
            }
            Pl_chr_setX(self, 0x64, 4, 0);
            return;
        }
        return;
    }
}

/* The evade act for `variant` 0-8 (`level`/`alt` pick the bow kind's step table): arms the roll or hop motion and
 * the drawn weapon's step table, slides the actor along the roll and, with skill 0xB6, a little further, then
 * chains into the follow-up act when the motion ends. */
extern "C" void pl_act_evade(_PLW* self, s32 variant, s32 level, u8 alt) {
    VEC3 slide;
    f32 swing;
    u8 dir;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_set_motion_gate(self, 0);
        if (variant != 4 && variant != 5) {
            fn_80277BC4(self, 0);
            pl_sub_gauge_skill_add(self, -150);
        }
        pl_camera_lock_clear(self);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        Pl_act_set_motion(self, 3, 0, 0);
        self->field_0x006 = 0;
        self->field_0x0A8 = self->rot_0x54.y;
        if (self->field_0x18 == 0) {
            if ((u32)variant <= 1) {
                Pl_chr_set_attr_default(self, 0x7B, 4, 0x12);
            } else {
                switch (variant) {
                case 2:
                case 4:
                    self->field_0x006 = 0xFF;
                    Pl_act_set_step_table(self, (u32)pl_evade_steps_left, 0);
                    if (variant == 2) {
                        Pl_chr_set_attr_default(self, 0x67, 4, 0);
                    } else {
                        Pl_chr_setX(self, 0x89, 8, 0);
                    }
                    break;
                case 3:
                case 5:
                    self->field_0x006 = 0xFF;
                    Pl_act_set_step_table(self, (u32)pl_evade_steps_right, 0);
                    if (variant == 3) {
                        Pl_chr_set_attr_default(self, 0x68, 4, 0);
                    } else {
                        Pl_chr_setX(self, 0x8A, 8, 0);
                    }
                    break;
                }
            }
        } else {
            switch (variant) {
            case 0:
                self->field_0x006 = 1;
                Pl_chr_set_attr_default(self, 0x425, 4, 0);
                self->field_0x0A8 = self->rot_0x54.y;
                break;
            case 1:
                self->field_0x006 = 2;
                Pl_chr_set_attr_default(self, 0x424, 4, 0);
                self->field_0x0A8 = self->rot_0x54.y;
                break;
            case 2:
            case 4:
                self->field_0x006 = 0xFF;
                Pl_chr_setX(self, 0x428, 4, 0);
                self->field_0x0AC = self->rot_0x54.x;
                Pl_act_set_step_table(self, (u32)pl_evade_steps_left, 4);
                break;
            case 3:
            case 5:
                self->field_0x006 = 0xFF;
                Pl_chr_setX(self, 0x42A, 4, 0);
                self->field_0x0AC = self->rot_0x54.x;
                Pl_act_set_step_table(self, (u32)pl_evade_steps_right, 4);
                break;
            case 6:
                self->field_0x006 = 0xFF;
                Pl_chr_set_attr_default(self, 0x42C, 2, 0);
                break;
            case 7:
                self->field_0x006 = 0xFF;
                Pl_chr_set_attr_default(self, 0x42D, 2, 0);
                break;
            case 8:
                self->field_0x006 = 3;
                Pl_chr_set_attr_default(self, 0x426, 4, 0);
                self->field_0x0A8 = self->rot_0x54.y;
                break;
            }
            switch (self->field_0x002) {
            case 0:
                if ((u32)variant <= 1) {
                    fn_802771A0(self, 0x26);
                }
                break;
            case 1:
                if (variant == 0 || variant == 1 || (u32)(variant - 6) <= 1) {
                    Pl_act_set_step_table(self, (u32)lbl_805C9118, 0);
                }
                break;
            case 2:
                if (variant == 0 || variant == 1 || (u32)(variant - 6) <= 1) {
                    Pl_act_set_step_table(self, (u32)lbl_805CAD74, 0);
                }
                break;
            case 3:
                if ((variant == 0 || variant == 1 || variant == 8) && level <= 1) {
                    if (alt == 0) {
                        dir = 2;
                        if (variant != 8) {
                            dir = variant;
                        }
                        Pl_act_set_step_table(self, pl_evade_bow_steps[level + dir * 2], 0);
                    } else {
                        Pl_act_set_step_table(self, (u32)pl_evade_steps_bow_alt, 0);
                    }
                }
                break;
            }
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            switch (variant) {
            case 2:
            case 4:
                pl_act_enter(self, 0, 0x59, 0);
                break;
            case 3:
            case 5:
                pl_act_enter(self, 0, 0x5A, 0);
                break;
            case 0:
            case 1:
            case 6:
            case 7:
            case 8:
                Pl_act_set_motion_slot(self, 3, 8, 0);
                break;
            }
            break;
        }
        if (alt != 0 && self->field_0x002 == 3 && Pl_frame_check(self, 1, pl_frame_window_28, pl_float_zero) == 0) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_float_neg8;
            rotVecXYZ(&slide, &self->rot_0x54);
            addVec3To(&self->vec_0x03C, &slide);
        }
        if (Pl_frame_check(self, 1, pl_frame_window_18, pl_float_zero) == 1) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_float_zero;
            if (Pl_frame_check(self, 2, pl_frame_window_44, pl_float_zero) == 1) {
                switch (self->field_0x006) {
                case 0:
                    slide.z = pl_frame_window_20;
                    break;
                case 1:
                    slide.x = pl_float_neg20;
                    break;
                case 2:
                    slide.x = pl_frame_window_20;
                    break;
                }
            } else if (Pl_frame_check(self, 2, pl_frame_window_76, pl_float_zero) == 1) {
                if (self->field_0x006 == 0) {
                    slide.z = pl_frame_window_20 *
                              cosf((pl_float_6_28319 *
                                    (pl_frame_window_90 *
                                     ((pl_rig_get_motion_frame(self) - pl_frame_window_44) * pl_float_0_03125))) /
                                   pl_frame_window_360);
                } else if (self->field_0x006 != 3) {
                    swing = pl_float_neg20 *
                            cosf((pl_float_6_28319 *
                                  (pl_frame_window_90 *
                                   ((pl_rig_get_motion_frame(self) - pl_frame_window_44) * pl_float_0_03125))) /
                                 pl_frame_window_360);
                    slide.x = swing;
                    if (self->field_0x006 == 2) {
                        slide.x = swing * pl_float_neg1;
                    }
                }
            } else {
                slide.x = pl_float_zero;
                slide.z = pl_float_zero;
            }
            if (self->field_0x006 == 0) {
                rotVecXYZ(&slide, &self->rot_0x54);
                addVec3To(&self->vec_0x03C, &slide);
            }
        }
        if (Pl_Skill_ck(self, 0xB6) == 1 && Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_float_zero;
            switch (variant) {
            case 0:
                slide.x = pl_float_neg8;
                break;
            case 1:
                slide.x = pl_frame_window_8;
                break;
            case 6:
                slide.y = pl_float_zero;
                break;
            case 7:
                slide.y = pl_float_zero;
                break;
            case 8:
                slide.z = pl_float_neg4;
                break;
            }
            rotVecXYZ(&slide, &self->rot_0x54);
            addVec3To(&self->vec_0x03C, &slide);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 8, 0);
        }
        break;
    }
}

/* 0x8024A248 - the "ride/mount" handler: arms one of three motion sets from the actor's `+0x2` kind, then ends the
 * act through `pl_act_enter` or pushes the rider along the mount's rotation. */
extern "C" void fn_8024A248(_PLW* self, s32 arg1) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        if ((u32)arg1 <= 1U) {
            fn_80277BC4(self, 0);
        } else {
            fn_80277BC4(self, 1);
        }
        pl_act_set_motion_gate(self, 0);
        pl_sub_gauge_skill_add(self, -0x96);
        pl_camera_lock_clear(self);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        if (self->field_0x018 == 0) {
            Pl_chr_set_attr_default(self, 0x7B, 4, 0);
            return;
        }
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 0x426, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x427, 4, 0);
        }
        switch (self->field_0x002) {
        case 1:
            Pl_act_set_step_table(self, (u32)lbl_805C9118, 0);
            return;
        case 2:
            Pl_act_set_step_table(self, (u32)lbl_805CAD74, 0);
            return;
        case 8:
            if (pl_mode8_free_ck() == 0) {
                Pl_act_set_step_table(self, (u32)pl_flinch_steps_kind8_guard, 0);
                return;
            }
            return;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 0 || self->field_0x018 == 1) {
                Pl_act_set_motion_slot(self, 3, 8, 0);
                return;
            }
            if (pl_act_param_tier_ck(self, 0) >= 2U) {
                if (pl_part_flag_ck(self, 0x25) == 1) {
                    pl_act_enter(self, 1, 9, 0x80);
                    return;
                }
                pl_act_enter(self, 1, 7, 0x80);
                return;
            }
            pl_act_enter(self, 1, 8, 0x80);
            return;
        }
        if (Pl_Skill_ck(self, 0xB6) == 1 && Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
            vec.x = pl_float_zero;
            vec.y = pl_float_zero;
            if (arg1 == 0 || arg1 == 2) {
                vec.z = pl_frame_window_8;
            } else {
                vec.z = pl_float_neg8;
            }
            /* +0x054 is the actor's rotation; other units read its first word as a scalar, so the
             * `_CP_VECTOR` view is taken through the named field rather than a byte offset. */
            rotVecXYZ(&vec, (_CP_VECTOR*)&self->param_0x54);
            addVec3To(&self->field_0x03C, &vec);
        }
        break;
    }
}

/* 0x8024A51C - the "sit/rest" handler: one arming step, then a two-way exit through `pl_act_enter`. */
extern "C" void fn_8024A51C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 7, 2, 0);
        pl_act_clear_mode5c4(self);
        pl_act_set_frame_timer(self);
        if (arg1 == 0) {
            pl_act_clear_flag5bb(self);
            return;
        }
        fn_8027AC00(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x1F, 0);
                    return;
                }
                pl_act_enter(self, 0xA, 0xE, 0);
                return;
            }
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 8, 6, 0);
            return;
        }
        return;
    }
}

/* 0x8024A640 - the one-step "equip/ready" handler. */
extern "C" void fn_8024A640(_PLW* self) {
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8001, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_setX(self, 8, 4, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_step_table(self, (u32)lbl_805BE5B8, 0);
    }
}

/* 0x8024A6CC - the two-step "sheathe" handler. */
extern "C" void fn_8024A6CC(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 9, 0, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8024A75C - the "stagger" handler: arms a 150-frame motion, then hands the actor's `+0x37A` value
 * to the motion table. */
extern "C" void fn_8024A75C(_PLW* self, s32 arg1) {
    s32 timer;

    pl_sub_gauge_add(self, 4);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        if (arg1 == 0 || arg1 == 2) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 5, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x7A, 6, 0);
        }
        self->field_0x28 = 0x96;
        return;
    case 1:
        timer = self->field_0x28 - 1;
        self->field_0x28 = timer;
        if (timer > 0) {
            return;
        }
        self->act_step_0x05++;
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 6, 4, 0);
            return;
        }
        Pl_chr_set_attr_default(self, 0x7C, 4, 0);
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_sub_gauge_add(self, self->field_0x37A);
            if (arg1 == 0 || arg1 == 2) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
                return;
            }
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8024A8EC - the "item use" handler: arms one of three motion/SE rows selected by `arg1`, then
 * ends the act through the matching `pl_act_enter` code. */
extern "C" void fn_8024A8EC(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A54[arg1 * 2], lbl_805C4A54[arg1 * 2 + 1], 0);
        self->field_0x018 = 0;
        if (arg1 == 0) {
            self->field_0x5B8 = 0;
            self->field_0x5BA = 0;
            camera_follow_master_turn(self);
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            switch (arg1) {
            case 0:
                pl_act_enter(self, 0, 0x23, 2);
                return;
            case 1:
                pl_act_enter(self, 0, 0x78, 2);
                return;
            case 2:
                pl_act_enter(self, 0, 0x77, 2);
                break;
            }
        }
        break;
    }
}

/* 0x8024AA04 - maps a motion id to the small class the act tables use: 0 for the idle pair, 1 for
 * 0x28, 2 for the two heavy ids. */
extern "C" s32 fn_8024AA04(u16 motion) {
    s32 result = 0;

    switch (motion) {
    case 0x2F:
        result = 0;
        break;
    case 0x28:
        result = 1;
        break;
    case 0x109:
        result = 2;
        break;
    case 0x1B6:
        result = 2;
        break;
    }
    return result;
}

/* The fishing act for `variant` 0-4: casts (or re-casts) the line and starts its SE, then counts the frames the
 * bait sits and grades the pull against the bite window of the row `pl_act_name_row_get` picks; a pull (or the
 * time limit) lands the catch the grade gives and chains into the result act. */
#pragma optimization_level 4
extern "C" void pl_act_fishing(_PLW* self, s32 variant) {
    u8* row;
    s16 count;
    s16 bite;
    s16 hold;
    s16 limit;
    u32 item;
    u8 big;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x18 = 0;
        switch (variant) {
        case 0:
            self->field_0x006 = 0;
            self->field_0x007 = 0;
            if (Pl_master_ck(self) == 1) {
                if (fn_8024AA04(self->field_0x306) == 2) {
                    pl_fish_se_big(&self->vec_0x03C);
                } else {
                    pl_fish_se_small(&self->vec_0x03C);
                }
            }
            self->field_0x28 = 0x7C;
            self->field_0x5B8 = 0;
            self->field_0x5BA = 0;
            Pl_chr_set_attr_default(self, 0x150, 0, 0);
            self->count_0x034 = 1;
            self->field_0x036 = 0;
            break;
        case 1:
        case 2:
            self->field_0x006 = 1;
            self->field_0x007 = 0;
            if (Pl_master_ck(self) == 1) {
                pl_fish_se_cast(&self->vec_0x03C, 0);
            }
            self->field_0x28 = 0x2A;
            self->field_0x5B8 = 0;
            self->field_0x5BA = 0;
            Pl_chr_set_attr_default(self, 0x150, 4, 0);
            self->count_0x034 = 1;
            self->field_0x036 = 0;
            break;
        case 3:
        case 4:
            self->field_0x006 = 2;
            self->field_0x007 = 1;
            if (Pl_master_ck(self) == 1) {
                pl_fish_se_cast(&self->vec_0x03C, 0);
            }
            self->field_0x354 = pl_frame_window_1 + (f32)((self->field_0x0B6 % 100) - 50) / pl_frame_window_100;
            Pl_chr_set_attr_default(self, 0x150, 0, 0x16);
            self->count_0x034 = 1;
            self->field_0x036 = 0;
            break;
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        switch (self->field_0x006) {
        case 0:
            if (--self->field_0x28 <= 0) {
                self->field_0x006++;
                if (Pl_master_ck(self) == 1 && fn_8024AA04(self->field_0x306) == 2) {
                    pl_fish_se_cast(&self->vec_0x03C, 0);
                }
            }
            break;
        case 1:
            if (Pl_frame_check(self, 0, pl_frame_window_80, pl_float_zero) == 1) {
                if (--self->count_0x034 <= 0) {
                    self->field_0x006++;
                    if (fn_8024AA04(self->field_0x306) == 2 && pl_part_flag_ck(self, 0x15) == 1 &&
                        Pl_item_timer_get(self, 0x1D) > 1) {
                        switch (variant) {
                        case 0:
                        case 1:
                            pl_act_enter(self, 0, 0x6F, 0);
                            break;
                        case 2:
                            pl_act_enter(self, 0, 0x70, 0);
                            break;
                        }
                        return;
                    }
                    if (Pl_master_ck(self) == 1 && fn_8024AA04(self->field_0x306) == 2) {
                        pl_fish_se_cast(&self->vec_0x03C, 1);
                    }
                    self->field_0x354 = pl_frame_window_1 + (f32)((self->field_0x0B6 % 100) - 50) / pl_frame_window_100;
                    self->field_0x036 = 1;
                }
            }
            break;
        }
        row = pl_act_name_row_get(self, variant, fn_8024AA04(self->field_0x306));
        count = ++self->field_0x5B8;
        bite = ((s16*)row)[1];
        hold = ((s16*)row)[3];
        if (count >= bite + ((s16*)row)[2]) {
            self->field_0x5BA = 4;
        } else if (count >= bite) {
            if (hold > 0 && count >= bite + hold && count < ((s16*)row)[4] + (bite + hold)) {
                self->field_0x5BA = 3;
            } else {
                self->field_0x5BA = 2;
            }
        } else if (count >= ((s16*)row)[0]) {
            self->field_0x5BA = 1;
        } else {
            self->field_0x5BA = 0;
        }
        if (variant == 0) {
            limit = 0x14A;
        } else {
            limit = 0x96;
            if (self->field_0x5B8 >= 0xA0) {
                limit = 0x14A;
            }
        }
        if (Pl_master_ck(self) != 1) {
            break;
        }
        if (Pl_motion_input_ck(1) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
            break;
        }
        if (pl_part_flag_ck(self, 9) != 1 && self->field_0x5B8 < limit) {
            break;
        }
        switch (self->field_0x5BA) {
        case 0:
            pl_model_state_set(self, 1, 0xD, 0);
            pl_fish_se_result(self->field_0x5BA, &self->vec_0x03C, 0);
            if (self->field_0x007 == 0) {
                pl_act_enter(self, 0, 0x25, 0);
            } else if (variant == 1 || variant == 3) {
                pl_act_enter(self, 0, 0x75, 0);
            } else {
                pl_act_enter(self, 0, 0x76, 0);
            }
            return;
        case 1:
            pl_item_add(self, 0x1D, -1);
            if (Pl_Skill_ck(self, 0xB5) == 1) {
                self->field_0x5BA = 2;
                item = 0x1A;
            } else {
                item = 0x1B;
            }
            if ((u32)item_get_with_msg(self, item, self->field_0x598, 9, 1) > 1) {
                if (self->field_0x007 != 0) {
                    self->field_0x007++;
                }
            }
            if (item == 0x1A) {
                break;
            }
            pl_fish_se_result(self->field_0x5BA, &self->vec_0x03C, 0);
            if (self->field_0x007 == 0) {
                pl_act_enter(self, 0, 0x25, 0);
            } else if (variant == 1 || variant == 3) {
                if (self->field_0x007 == 1) {
                    pl_act_enter(self, 0, 0x75, 0);
                } else {
                    pl_act_enter(self, 0, 0xA4, 0);
                }
            } else if (self->field_0x007 == 1) {
                pl_act_enter(self, 0, 0x76, 0);
            } else {
                pl_act_enter(self, 0, 0xA5, 0);
            }
            return;
        case 2:
        case 3:
            pl_item_add(self, 0x1D, -1);
            if (Pl_master_ck(self) == 1) {
                lb_event_request(7);
            }
            if ((u32)item_get_with_msg(self, 0x1A, self->field_0x598, 9, 1) > 1) {
                if (self->field_0x007 != 0) {
                    self->field_0x007++;
                }
            }
            break;
        case 4:
            pl_item_add(self, 0x1D, -1);
            pl_fish_se_result(self->field_0x5BA, &self->vec_0x03C, 0);
            if ((u32)item_get_with_msg(self, 0x1C, self->field_0x598, 0x11, 1) > 1) {
                if (self->field_0x007 != 0) {
                    self->field_0x007++;
                }
            }
            if (self->field_0x007 == 0) {
                pl_act_enter(self, 0, 0x25, 0);
            } else if (variant == 1 || variant == 3) {
                if (self->field_0x007 == 1) {
                    pl_act_enter(self, 0, 0x75, 0);
                } else {
                    pl_act_enter(self, 0, 0xA4, 0);
                }
            } else if (self->field_0x007 == 1) {
                pl_act_enter(self, 0, 0x76, 0);
            } else {
                pl_act_enter(self, 0, 0xA5, 0);
            }
            return;
        default:
            return;
        }
        big = fn_8024AA04(self->field_0x306) == 2;
        if (self->field_0x007 == 0) {
            pl_fish_se_result(self->field_0x5BA | 0x80, &self->vec_0x03C, big);
            pl_act_enter(self, 0, 0x24, 0);
        } else {
            pl_fish_se_result(self->field_0x5BA, &self->vec_0x03C, big);
            if (variant == 1 || variant == 3) {
                if (self->field_0x007 == 1) {
                    pl_act_enter(self, 0, 0x73, 0);
                } else {
                    pl_act_enter(self, 0, 0xA2, 0);
                }
            } else if (self->field_0x007 == 1) {
                pl_act_enter(self, 0, 0x74, 0);
            } else {
                pl_act_enter(self, 0, 0xA3, 0);
            }
        }
        break;
    }
}
#pragma optimization_level reset

/* 0x8024B35C - the two-step "fall" handler: arms the motion row for `arg1`, then ends the act
 * through the `Pl_act_set_motion_slot` or `pl_act_enter` code that `arg1` selects. */
extern "C" void fn_8024B35C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A60[arg1], 0, 0);
        return;
    case 1:
        switch (arg1) {
        case 0:
        case 3:
        case 4:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            }
            break;
        case 1:
        case 2:
            if (Pl_motion_end_ck(self) == 1) {
                if (arg1 == 1) {
                    pl_act_enter(self, 0, 0x71, 2);
                } else {
                    pl_act_enter(self, 0, 0x72, 2);
                }
            }
            break;
        }
        break;
    }
}

/* 0x8024B46C - the sibling "fall" handler whose 0x72 exit is gated on a frame check. */
extern "C" void fn_8024B46C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A6C[arg1], 0, 0);
        return;
    case 1:
        if ((u32)(arg1 - 3) > 1U && arg1 != 0) {
            switch (arg1) {
            case 1:
                if (Pl_motion_end_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x71, 2);
                    return;
                }
                break;
            case 2:
                if (Pl_frame_check(self, 1, pl_frame_window_80, pl_float_zero) == 1) {
                    pl_act_enter(self, 0, 0x72, 2);
                }
                break;
            }
        } else {
            if (Pl_motion_end_ck(self) == 1) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
                return;
            }
            return;
        }
        return;
    }
}

/* 0x8024B594 - the "swim/dive" handler: arms one of two motion sets, then hands the actor's kind to
 * the motion table. */
extern "C" void fn_8024B594(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0x8000, 0, 0);
            Pl_chr_set_attr_default(self, 0x29, 2, 0);
        } else {
            Pl_act_set_motion(self, 0x8003, 0, 0);
            Pl_chr_set_attr_default(self, 0x7E, 2, 0);
        }
        pl_act_set_frame_timer(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 8, 0);
        }
        return;
    }
}

/* 0x8024B66C - the "evade" handler: one of three arming motions, then the frame-checked roll-out. */
extern "C" void fn_8024B66C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 7, 2, 0);
            return;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 4, 0);
            return;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 4, 0);
            return;
        }
        break;
    case 1:
        switch (arg1) {
        case 0:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x29, 0xC);
                    return;
                }
                Pl_chr_set_attr_default(self, 8, 6, 0);
                return;
            }
            return;
        case 1:
            if (Pl_frame_check(self, 0, pl_frame_window_30, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x99, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
                return;
            }
            break;
        case 2:
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x9C, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
            }
            break;
        }
        break;
    }
}

/* 0x8024B868 - the "get-up from evade" handler: arms the motion, feeds the pending item back, then
 * waits 14 frames before handing over to `Pl_act_set_motion_slot`. */
extern "C" void fn_8024B868(_PLW* self, s32 arg1) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 8, 6, 0);
            break;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 0, 0x1E);
            break;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 0, 0x14);
            break;
        }
        self->field_0x28 = 0;
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_a(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
                return;
            }
        }
        return;
    case 1:
        if ((u32)(arg1 - 1) > 1U) {
            if (arg1 == 0) {
                timer = self->field_0x28 + 1;
                self->field_0x28 = timer;
                if (timer >= 0xE) {
                    self->act_step_0x05++;
                    Pl_chr_set_attr_default(self, 9, 0, 0);
                    return;
                }
            }
            return;
        }
        self->act_step_0x05++;
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
        }
        break;
    }
}

/* Wears the gathering tool in use (pickaxes 0x22-0x24, bug nets 0x25-0x27, the 0xC2/0x184 pair): past its grade's
 * use threshold the tool breaks with odds the gathering skills shift, and is taken from the pouch. */
extern "C" void pl_gather_tool_wear(_PLW* self) {
    u8* uses;
    s32 tool;
    s32 grade;
    u32 odds;
    u8 limit;

    switch (self->field_0x306) {
    case 0x22:
        tool = 0;
        grade = 0;
        odds = 3;
        break;
    case 0x23:
        tool = 0;
        grade = 1;
        odds = 10;
        break;
    case 0x24:
        tool = 0;
        grade = 2;
        odds = 15;
        break;
    case 0x25:
        tool = 1;
        grade = 0;
        odds = 3;
        break;
    case 0x26:
        tool = 1;
        grade = 1;
        odds = 10;
        break;
    case 0x27:
        tool = 1;
        grade = 2;
        odds = 15;
        break;
    case 0xC2:
        tool = 2;
        grade = 1;
        odds = 10;
        break;
    case 0x184:
        tool = 2;
        grade = 2;
        odds = 15;
        break;
    default:
        return;
    }
    switch (tool) {
    case 0:
        uses = &self->field_0x47B + grade;
        break;
    case 1:
        uses = &self->field_0x47E + grade;
        break;
    case 2:
        uses = &self->field_0x481 + grade;
        break;
    }
    (*uses)++;
    limit = pl_gather_tool_limits[grade];
    if (*uses > limit) {
        *uses = limit + 1;
        if (Pl_Skill_ck(self, 0x8F) == 1 && (ran_suu(1) & 3) == 0) {
            odds = (odds >> 1) + 1;
        }
        if (Pl_Skill_ck(self, 0x90) == 1 && (ran_suu(1) & 1) == 0) {
            odds = (odds >> 1) + 1;
        }
        if (ran_suu(1) % odds == 0 && (Pl_Skill_ck(self, 0x8D) != 1 || (ran_suu(1) & 3) != 0) &&
            (Pl_Skill_ck(self, 0x8E) != 1 || (ran_suu(1) & 1) != 0) &&
            (Pl_cat_skill_ck(self, 0xA) != 1 || (ran_suu(1) & 3) != 0)) {
            pl_model_state_set(self, 2, 0x16, self->field_0x306);
            pl_item_add(self, self->field_0x306, -1);
            *uses = 0;
        }
    }
}

/* Hands the gathered `item` to the pouch (the third argument is unread) (wearing the tool for the mining and netting `variant`s): a full pouch
 * plays the failure SE, a taken item its pickup SE, and a bonus item raises the bonus latch and its SE. */
extern "C" void pl_gather_item_take(_PLW* self, s32 variant, s32 unused, u16 item) {
    s16 result;

    if (item != 0) {
        if ((u32)(variant - 2) <= 1 || variant == 8) {
            pl_gather_tool_wear(self);
        }
        result = item_get_with_msg(self, item, self->field_0x598, 3, 1);
        if ((u32)result > 2) {
            switch (result) {
            case 3:
                snd_item_fail_play();
                break;
            case 5:
                if (item_bonus_class_get(item) != 0) {
                    snd_item_fail_play();
                    return;
                }
                /* fallthrough */
            default:
                se_item_pickup_play(item);
                return;
            }
        } else {
            se_item_pickup_play(item);
        }
        if (item_bonus_class_get(item) != 0 && Pl_item_timer_get(self, item) > 0) {
            self->field_0x007 = 1;
            se_slot_req(1);
            if (item == 0x17E) {
                pl_model_state_set(self, 2, 0x2E, item);
            }
        }
    } else {
        pl_model_state_set(self, 1, 9, 0);
    }
}

/* The gather act for `variant` 0-9 (mining, netting, carving, the quest-item pickup): arms the gather motion
 * (faster with skill 0x8C) and, once its countdown runs out, takes the gathering point's item; when the motion
 * ends it returns to the idle slot, or, with the bonus latch raised, enters the bonus act. */
extern "C" void pl_act_gather(_PLW* self, s32 variant) {
    u8 step;
    u8 taken;
    u16 item;

    fn_802DE578_view1(self, &self->field_0x598);
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        Pl_act_set_motion(self, 0, 0, 0);
        step = self->act_step_0x05 + 1;
        self->act_step_0x05 = step;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        switch (variant) {
        case 0:
        case 9:
            self->act_step_0x05 = step + 1;
            Pl_chr_set_attr_default(self, 0x131, -0xE, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_3;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            if (variant == 9 && Pl_master_ck(self) == 1) {
                pl_model_state_set(self, 1, 0x1A, 0);
            }
            break;
        case 1:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x137, -6, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_3;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 2:
            self->act_step_0x05 = step + 1;
            Pl_chr_set_attr_default(self, 0x13B, 0, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_4;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 3:
            self->act_step_0x05 = step + 1;
            Pl_chr_set_attr_default(self, 0x13E, 0, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_4;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 4:
            self->act_step_0x05 = step + 1;
            Pl_chr_set_attr_default(self, 0x13F, -6, 0);
            break;
        case 5:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x142, -6, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_3;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 6:
            self->act_step_0x05 = step + 1;
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x166, -0xE, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_3;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 7:
            self->act_step_0x05 = step + 1;
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x15F, -4, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_3;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        case 8:
            self->act_step_0x05 = step + 1;
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x160, -4, 0);
            if (Pl_Skill_ck(self, 0x8C) == 1) {
                self->field_0x354 = pl_float_1_4;
            } else {
                self->field_0x354 = pl_float_1_1;
            }
            break;
        }
        self->field_0x28 = 60;
        if (Pl_Skill_ck(self, 0x8C) == 1) {
            self->field_0x28 -= self->field_0x28 / 4;
        }
        break;
    case 1:
        if (self->field_0x28 > 1) {
            self->field_0x28--;
        }
        switch (variant) {
        case 1:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x131, -4, 0x24);
                if (Pl_Skill_ck(self, 0x8C) == 1) {
                    self->field_0x354 = pl_float_1_3;
                } else {
                    self->field_0x354 = pl_float_1_1;
                }
            }
            break;
        case 5:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x13F, 0, 0x44);
                if (Pl_Skill_ck(self, 0x8C) == 1) {
                    self->field_0x354 = pl_float_1_3;
                } else {
                    self->field_0x354 = pl_float_1_1;
                }
            }
            break;
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && self->field_0x28 > 0) {
            if (--self->field_0x28 == 0 && Pl_motion_input_ck(1) == 0) {
                if (variant == 9) {
                    quest_item_slots_use(self);
                } else if (self->field_0x478 == 0x10) {
                    pl_gather_item_take(self, variant, 1, 0x2B);
                } else {
                    taken = em_roster_area_gather_take(self->area_0x16, &self->vec_0x03C, self->field_0x474);
                    if ((u32)(taken - 1) <= 1) {
                        item = em_weight_table_pick((const EmWeightEntry*)self->field_0x474);
                        if (Pl_master_ck(self) == 1) {
                            switch (item) {
                            case 0x7B:
                                lb_event_request(5);
                                break;
                            case 0x63:
                                lb_event_request(6);
                                break;
                            case 0x90:
                                lb_event_request(0x19);
                                break;
                            }
                        }
                        pl_gather_item_take(self, variant, taken, item);
                    } else {
                        switch (taken) {
                        case 0xFF:
                            pl_model_state_set(self, 1, 1, 0);
                            break;
                        case 0:
                            pl_model_state_set(self, 1, 9, 0);
                            break;
                        }
                    }
                }
            }
        }
        if (self->field_0x006 == 0) {
            switch (variant) {
            case 1:
                if (Pl_frame_check(self, 1, pl_frame_window_200, pl_float_zero) == 1) {
                    self->field_0x006++;
                    Pl_chr_set_attr_default(self, 0x138, -4, 0);
                    if (Pl_Skill_ck(self, 0x8C) == 1) {
                        self->field_0x354 = pl_float_1_3;
                    } else {
                        self->field_0x354 = pl_float_1_1;
                    }
                }
                break;
            case 5:
                if (Pl_frame_check(self, 1, pl_frame_window_294, pl_float_zero) == 1) {
                    self->field_0x006++;
                    Pl_chr_set_attr_default(self, 0x143, -2, 0);
                    if (Pl_Skill_ck(self, 0x8C) == 1) {
                        self->field_0x354 = pl_float_1_3;
                    } else {
                        self->field_0x354 = pl_float_1_1;
                    }
                }
                break;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            if (self->field_0x007 == 0) {
                if ((u32)(variant - 6) <= 2) {
                    Pl_act_set_motion_slot(self, 3, 4, 0);
                } else {
                    switch (variant) {
                    case 0:
                    case 2:
                    case 9:
                        Pl_act_set_motion_slot(self, 0, 0xA, 0);
                        break;
                    case 1:
                    case 5:
                        Pl_act_set_motion_slot(self, 1, 6, 0);
                        break;
                    default:
                        Pl_act_set_motion_slot(self, 0, 4, 0);
                        break;
                    }
                }
            } else {
                pl_carry_latch_refresh(self);
                pl_act_enter(self, 0xC, 9, 0);
            }
        } else if (variant == 4 && Pl_Skill_ck(self, 0x8C) == 1 &&
                   Pl_frame_check(self, 0, pl_frame_window_100, pl_float_zero) == 1) {
            Pl_chr_set_attr_default(self, 0x13F, -8, 0xE0);
        }
        break;
    }
}

/* The carve-finish act: arms the motion (doubled speed with skill 0xB8), steps into the follow-up motion at frame
 * 130 and, at frame 60 of it, enters the master's next act. */
extern "C" void pl_act_carve_finish(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x131, -0xE, 0);
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
        }
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_130, pl_float_zero) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x136, 4, 0);
        }
        break;
    case 2:
        if (Pl_frame_check(self, 1, pl_frame_window_60, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1) {
                pl_act_enter(self, 0, 0x30, 0xC);
            } else {
                self->act_step_0x05++;
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0xA, 0);
        }
        break;
    }
}

/* 0x8024C75C - the "drink/item" handler: arms the item motion, refunds the pending item, and arms
 * the recovery scale when the skill is set. */
extern "C" void fn_8024C75C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x136, 0, 0x3C);
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_c(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x131, -4, 0xCE);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0xA, 0);
        }
    }
}

/* 0x8024C878 - the sibling "drink" handler whose 0x7E exit is gated on a frame check. */
extern "C" void fn_8024C878(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, -8, 0);
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_208, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1) {
                pl_act_enter(self, 0, 0x7E, 0xC);
                return;
            }
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
    }
}

/* 0x8024C96C - the "drink, standing" variant of 0x8024C75C. */
extern "C" void fn_8024C96C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_c(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 0xA, 0);
        }
    }
}

/* 0x8024CA50 - arms the recovery scale for the two "eat/drink" skills. */
extern "C" void fn_8024CA50(_PLW* self, s32 arg1) {
    if (arg1 == 0) {
        if (Pl_Skill_ck(self, 0xB0) == 1) {
            self->field_0x354 = pl_float_1_7;
        }
    } else if (Pl_Skill_ck(self, 0xAF) == 1 || Pl_Skill_ck(self, 0xB0) == 1) {
        self->field_0x354 = pl_float_1_8;
    }
}

/* The gather act for `variant` 0-3: acquires the gathering ticket, arms the gather motion, and, for the carve
 * variants, repeats the carve (skill 0xB1) and speeds it up (skills 0xAF/0xB0) inside the carve window; the
 * master chains into the result act when the motion ends. */
extern "C" void pl_act_carve(_PLW* self, s32 variant) {
    fn_802DE578_view1(self, &self->field_0x598);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        switch (variant) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x132, 4, 0);
            fn_8024CA50(self, 0);
            break;
        case 1:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x13C, 4, 0);
            break;
        case 2:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x161, 4, 0);
            fn_8024CA50(self, 0);
            break;
        case 3:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x13C, 4, 0);
            break;
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 1) {
                if (self->field_0x306 == 0x177) {
                    pl_act_net_hook_main(self);
                    Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
                } else if (self->kind_0x09 == 3) {
                    pl_act_enter(self, 0, 0x3A, 0);
                } else {
                    pl_act_enter(self, 0, 0x39, 0);
                }
            } else {
                Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
            }
            break;
        }
        if (variant == 1 || variant == 3) {
            if (Pl_Skill_ck(self, 0xB1) == 1 && Pl_frame_check(self, 0, pl_frame_window_62, pl_float_zero) == 1) {
                if (self->field_0x006 < 4) {
                    self->field_0x006++;
                    self->field_0x007 = 1;
                    Pl_chr_set_attr_default(self, 0x13C, 4, 0x34);
                }
            }
            if (Pl_frame_check(self, 3, pl_frame_window_38, pl_frame_window_180) == 1 &&
                (Pl_Skill_ck(self, 0xAF) == 1 || Pl_Skill_ck(self, 0xB0) == 1)) {
                self->field_0x354 = pl_float_1_8;
            } else {
                self->field_0x354 = pl_frame_window_1;
            }
        }
        break;
    }
}

/* 0x8024CD8C - scales a `s16` amount by the skill's multiplier (0xA6 first, then 0xA7). */
extern "C" s16 fn_8024CD8C(_PLW* self, s16 value) {
    s16 scaled = value;

    if (Pl_Skill_ck(self, 0xA6) == 1) {
        scaled = (s16)(pl_float_1_5 * (f32)scaled);
    } else if (Pl_Skill_ck(self, 0xA7) == 1) {
        scaled = (s16)(pl_float_0_66 * (f32)scaled);
    }
    return scaled;
}

/* 0x8024CE54 (0x240): Replays the shell-set job (slot 0x34) the current effect id asks for, picking the variant from
 * the cat skill and from whether the actor is in act 80. */
extern "C" void pl_act_net_hook_b(_PLW* self) {
    BOOL in_act = Pl_act_ck(self, 0, 0x50) == 1;
    switch (self->field_0x306) {
    case 0x2:
        if (Pl_cat_skill_ck(self, 5) != 1) {
            if (!in_act) {
                shell_set_func_ptr->method_0x34(self, 2, shell_set_func_ptr);
            } else {
                shell_set_func_ptr->method_0x34(self, 5, shell_set_func_ptr);
            }
            break;
        }
        /* fallthrough */
    case 0x246:
    case 0x34:
        if (!in_act) {
            shell_set_func_ptr->method_0x34(self, 3, shell_set_func_ptr);
        } else {
            shell_set_func_ptr->method_0x34(self, 6, shell_set_func_ptr);
        }
        break;
    case 0x17F:
        shell_set_func_ptr->method_0x34(self, 0x10, shell_set_func_ptr);
        break;
    case 0x1:
        if (Pl_cat_skill_ck(self, 5) != 1) {
            if (in_act == 1) {
                shell_set_func_ptr->method_0x34(self, 4, shell_set_func_ptr);
            }
            break;
        }
        /* fallthrough */
    case 0x237:
        if (in_act == 1) {
            shell_set_func_ptr->method_0x34(self, 0x12, shell_set_func_ptr);
        }
        break;
    case 0x258:
        if (Pl_cat_skill_ck(self, 5) != 1) {
            if (in_act == 1) {
                shell_set_func_ptr->method_0x34(self, 0x15, shell_set_func_ptr);
            }
            break;
        }
        /* fallthrough */
    case 0x259:
        if (in_act == 1) {
            shell_set_func_ptr->method_0x34(self, 0x16, shell_set_func_ptr);
        }
        break;
    case 0x108:
        if (in_act == 1) {
            shell_set_func_ptr->method_0x34(self, 0xB, shell_set_func_ptr);
        }
        break;
    }
}

/* Re-arms the quest slots of the actor's chunk after an item use and fires the item's shell: the small barrel
 * bomb (item 3) unless the actor is in act 0x7E, the large one for items 4 and 49. */
extern "C" void pl_act_net_hook_c(_PLW* self) {
    s32 inAct;

    inAct = Pl_act_ck(self, 0, 0x7E) == 1;
    quest_slot_arm_all_b((s8)self->chunk_ofs);
    switch (self->field_0x306) {
    case 3:
        if (inAct == 0) {
            shell_set_func_ptr->method_0x1C(self, 0, shell_set_func_ptr);
        }
        break;
    case 4:
    case 49:
        shell_set_func_ptr->method_0x20(self, 1, shell_set_func_ptr);
        break;
    }
}

/* 0x8024D144 (0x284): Replays the shell-set job (slot 0x34) the current effect id asks for, picking the variant from
 * the cat skill and from whether the actor is in act 153. */
extern "C" void pl_act_net_hook_a(_PLW* self) {
    BOOL in_act = Pl_act_ck(self, 0, 0x99) == 1;
    switch (self->field_0x306) {
    case 0x1:
        if (Pl_cat_skill_ck(self, 5) != 1) {
            if (!in_act) {
                shell_set_func_ptr->method_0x34(self, 1, shell_set_func_ptr);
            } else {
                shell_set_func_ptr->method_0x34(self, 4, shell_set_func_ptr);
            }
            break;
        }
        /* fallthrough */
    case 0x237:
        if (!in_act) {
            shell_set_func_ptr->method_0x34(self, 0x11, shell_set_func_ptr);
        } else {
            shell_set_func_ptr->method_0x34(self, 0x12, shell_set_func_ptr);
        }
        break;
    case 0x258:
        if (Pl_cat_skill_ck(self, 5) != 1) {
            if (!in_act) {
                shell_set_func_ptr->method_0x34(self, 0x13, shell_set_func_ptr);
            } else {
                shell_set_func_ptr->method_0x34(self, 0x15, shell_set_func_ptr);
            }
            break;
        }
        /* fallthrough */
    case 0x259:
        if (!in_act) {
            shell_set_func_ptr->method_0x34(self, 0x14, shell_set_func_ptr);
        } else {
            shell_set_func_ptr->method_0x34(self, 0x16, shell_set_func_ptr);
        }
        break;
    case 0x108:
        if (!in_act) {
            shell_set_func_ptr->method_0x34(self, 0xA, shell_set_func_ptr);
        } else {
            shell_set_func_ptr->method_0x34(self, 0xB, shell_set_func_ptr);
        }
        break;
    case 0x1D:
        shell_set_func_ptr->method_0x34(self, 0, shell_set_func_ptr);
        break;
    case 0x1E:
        shell_set_func_ptr->method_0x34(self, 7, shell_set_func_ptr);
        break;
    case 0x1F:
        shell_set_func_ptr->method_0x34(self, 8, shell_set_func_ptr);
        break;
    case 0x20:
        shell_set_func_ptr->method_0x34(self, 9, shell_set_func_ptr);
        break;
    }
}

/* 0x8024D3C8 - one-in-three chance the actor is staggered by `fn_80276CE8` when the skill is set. */
extern "C" void fn_8024D3C8(_PLW* self) {
    if (Pl_master_ck(self) == 1 && Pl_Skill_ck(self, 0x49) == 1 && ran_suu(1) % 3 == 0) {
        fn_80276CE8(self, 0x96);
    }
}

/* Applies the resistance item `item` (0x187-0x18A, 0x170) to the actor: arms its timer (the shared variant from
 * `src`'s skill at half length), clears the matching ailment counters and its two flag bits. */
extern "C" void pl_resist_item_apply(_PLW* self, _PLW* src, u16 item, u8 shared) {
    s16 time;

    if (shared == 0) {
        time = fn_8024CD8C(self, 9000);
    } else {
        time = fn_8024CD8C(src, 4500);
    }
    switch (item) {
    case 0x187:
        self->field_0x438 = time;
        self->field_0x42E = 0;
        self->field_0x404 = 0;
        self->field_0x3DC &= ~0x3;
        ef_inst_spawn(self, 9);
        break;
    case 0x188:
        self->field_0x43A = time;
        self->field_0x430 = 0;
        self->field_0x406 = 0;
        self->field_0x3DC &= ~0xC;
        ef_inst_spawn(self, 9);
        break;
    case 0x189:
        self->field_0x43E = time;
        self->field_0x434 = 0;
        self->field_0x40A = 0;
        self->field_0x3DC &= ~0xC0;
        ef_inst_spawn(self, 9);
        break;
    case 0x18A:
        self->field_0x43C = time;
        self->field_0x432 = 0;
        self->field_0x408 = 0;
        self->field_0x3DC &= ~0x30;
        ef_inst_spawn(self, 9);
        break;
    case 0x170:
        self->field_0x440 = time;
        self->field_0x436 = 0;
        self->field_0x40C = 0;
        self->field_0x3DC &= ~0x300;
        ef_inst_spawn(self, 9);
        break;
    }
}

/* 0x8024D5AC - the lobby-mode actor-match test: only in play mode, and only when the two actors
 * share their `+0x016` area byte. */
extern "C" s32 fn_8024D5AC(_PLW* self, _PLW* other) {
    if (PlayMode_ck() != 2) {
        return 0;
    }
    if (other == NULL) {
        return 0;
    }
    return self->area_0x16 == other->area_0x16;
}

/* Applies the item the actor just used (`_PLW`+0x306): spends it, then runs the item's effect - healing, buffs,
 * the party-wide variants a cat skill shares with the partner or the local player, the bombs and traps - and
 * arms the matching use motion. */
extern "C" void pl_act_net_hook_main(_PLW* self) {
    _PLW* partner;
    _PLW* me;
    s16 level;
    u16 item;
    u16* entry;
    u16 roll;
    s16 result;

    me = my_player_work_get();
    self->field_0x65F = 0xF;
    level = 0;
    if (self->field_0x306 != 0xD6 && Pl_motion_input_ck(1) == 0) {
        pl_item_add(self, self->field_0x306, -1);
    }
    if (PlayMode_ck() == 2) {
        partner = (_PLW*)pl_partner_work_get(self);
    }
    switch (self->field_0x306) {
    case 0x168:
        pl_health_recover(self, 0x32);
        fn_8024D3C8(self);
        ainpc_area_buff_apply(5);
        if (fn_8024D5AC(self, partner) == 1) {
            pl_health_recover(partner, 0x32);
            ef_inst_spawn(partner, 0);
        } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
            pl_health_recover(me, 0x32);
            ef_inst_spawn(me, 0);
        }
        ef_inst_spawn(self, 0);
        break;
    case 0x177:
        pl_act_clear_wait(self, 0);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 5);
        break;
    case 0xD:
        pl_health_recover(self, 0x1E);
        fn_8024D3C8(self);
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                ainpc_area_buff_apply(4);
            } else {
                ainpc_area_buff_apply(5);
            }
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                level = 0xF;
            }
            if (Pl_Skill_ck(self, 0x5C) == 1) {
                level = 0x1E;
            }
            if (fn_8024D5AC(self, partner) == 1) {
                ef_inst_spawn(partner, 0);
                pl_health_recover(partner, level);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                ef_inst_spawn(me, 0);
                pl_health_recover(me, level);
            }
        }
        ef_inst_spawn(self, 0);
        break;
    case 0x2C:
        pl_health_recover(self, 0x1E);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 0);
        break;
    case 0xE:
        pl_health_recover(self, 0x32);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 0);
        break;
    case 0x7B:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                ainpc_area_buff_apply(0);
            } else {
                ainpc_area_buff_apply(4);
            }
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                level = 0xA;
            }
            if (Pl_Skill_ck(self, 0x5C) == 1) {
                level = 0x14;
            }
            if (fn_8024D5AC(self, partner) == 1) {
                ef_inst_spawn(partner, 0);
                pl_health_recover(partner, level);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                ef_inst_spawn(me, 0);
                pl_health_recover(me, level);
            }
        }
        pl_health_recover(self, 0x14);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 0);
        break;
    case 0x111:
        pl_health_recover(self, 0xA);
        /* fallthrough */
    case 0x11:
        fn_8024D3C8(self);
        self->field_0x3EA = 0;
        ef_inst_spawn(self, 4);
        if (Pl_Skill_slot_item_get(self, 0x26) != 0 && self->field_0x306 == 0x11) {
            ainpc_area_buff_apply(3);
            if (fn_8024D5AC(self, partner) == 1) {
                partner->field_0x3EA = 0;
                ef_inst_spawn(partner, 4);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                me->field_0x3EA = 0;
                ef_inst_spawn(me, 4);
            }
        }
        break;
    case 0x74:
        pl_act_add_hold_gauge(self, -0xA);
        if ((s16)self->health <= 0) {
            self->health = 1;
        }
        /* fallthrough */
    case 0x7C:
        if ((self->field_0x0B6 & 1) != 0 || Pl_cat_skill_ck(self, 3) == 1) {
            self->field_0x3EA = 0;
            if (self->kind_0x09 == 3) {
                Pl_chr_set_attr_default(self, 0x162, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x133, 4, 0);
            }
            fn_8024CA50(self, 0);
            ef_inst_spawn(self, 4);
            fn_8024D3C8(self);
        } else if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x163, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x134, 4, 0);
        }
        return;
    case 0x187:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (fn_8024D5AC(self, partner) == 1) {
                pl_resist_item_apply(partner, self, 0x187, 1);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                pl_resist_item_apply(me, self, 0x187, 1);
            }
        }
        pl_resist_item_apply(self, self, 0x187, 0);
        fn_8024D3C8(self);
        break;
    case 0x188:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (fn_8024D5AC(self, partner) == 1) {
                pl_resist_item_apply(partner, self, 0x188, 1);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                pl_resist_item_apply(me, self, 0x188, 1);
            }
        }
        pl_resist_item_apply(self, self, 0x188, 0);
        fn_8024D3C8(self);
        break;
    case 0x189:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (fn_8024D5AC(self, partner) == 1) {
                pl_resist_item_apply(partner, self, 0x189, 1);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                pl_resist_item_apply(me, self, 0x189, 1);
            }
        }
        pl_resist_item_apply(self, self, 0x189, 0);
        fn_8024D3C8(self);
        break;
    case 0x18A:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (fn_8024D5AC(self, partner) == 1) {
                pl_resist_item_apply(partner, self, 0x18A, 1);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                pl_resist_item_apply(me, self, 0x18A, 1);
            }
        }
        pl_resist_item_apply(self, self, 0x18A, 0);
        fn_8024D3C8(self);
        break;
    case 0x170:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (fn_8024D5AC(self, partner) == 1) {
                pl_resist_item_apply(partner, self, 0x170, 1);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                pl_resist_item_apply(me, self, 0x170, 1);
            }
        }
        pl_resist_item_apply(self, self, 0x170, 0);
        fn_8024D3C8(self);
        break;
    case 0x17A:
        self->field_0x460 = fn_8024CD8C(self, 0x384);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 8);
        break;
    case 0x179:
        fn_80276CE8(self, 0x384);
        /* fallthrough */
    case 0x33:
        pl_gauge_ceiling_grow(self, 0x96, 0);
        pl_health_recover(self, 0x96);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 0);
        break;
    case 0x8A:
        pl_act_add_stagger(self, 0x32);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 3);
        break;
    case 0x6:
    case 0x2E:
        pl_act_add_stagger(self, 0x96);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 3);
        break;
    case 0x18E:
        pl_stagger_budget_add(self, 0x19);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 3);
        break;
    case 0x18F:
        pl_stagger_budget_add(self, 0x96);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 3);
        break;
    case 0x12:
    case 0x13:
        if (Pl_master_ck(self) == 1) {
            if (self->field_0x306 == 0x12) {
                pl_attack_up_set(self, 0, 3);
            } else {
                pl_attack_up_set(self, 0, 5);
            }
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 1);
        break;
    case 0x86:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                ainpc_area_buff_apply(1);
            } else {
                ainpc_area_buff_apply(6);
            }
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                level = 5;
            }
            if (Pl_Skill_ck(self, 0x5C) == 1) {
                level = 0xA;
            }
            if (fn_8024D5AC(self, partner) == 1) {
                ef_inst_spawn(partner, 1);
                partner->field_0x44A = fn_8024CD8C(self, 0x1518);
                pl_attack_up_set(partner, 1, level);
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                ef_inst_spawn(me, 1);
                me->field_0x44A = fn_8024CD8C(self, 0x1518);
                pl_attack_up_set(me, 1, level);
            }
        }
        fn_8024D3C8(self);
        if (Pl_master_ck(self) == 1) {
            pl_attack_up_set(self, 1, 0xA);
            self->field_0x44A = fn_8024CD8C(self, 0x1518);
        }
        ef_inst_spawn(self, 1);
        break;
    case 0x87:
        if (Pl_Skill_slot_item_get(self, 0x26) != 0) {
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                ainpc_area_buff_apply(2);
            } else {
                ainpc_area_buff_apply(7);
            }
            if (Pl_Skill_ck(self, 0x5B) == 1) {
                level = 0xA;
            }
            if (Pl_Skill_ck(self, 0x5C) == 1) {
                level = 0x14;
            }
            if (fn_8024D5AC(self, partner) == 1) {
                ef_inst_spawn(partner, 2);
                if (pl_defense_up_set(partner, 1, level) == 1) {
                    partner->field_0x44E = fn_8024CD8C(self, 0x1518);
                }
            } else if (Pl_master_ck(self) == 0 && self->area_0x16 == me->area_0x16) {
                ef_inst_spawn(me, 2);
                if (pl_defense_up_set(me, 1, level) == 1) {
                    me->field_0x44E = fn_8024CD8C(self, 0x1518);
                }
            }
        }
        fn_8024D3C8(self);
        if (Pl_master_ck(self) == 1 && pl_defense_up_set(self, 1, 0x14) == 1) {
            self->field_0x44E = fn_8024CD8C(self, 0x1518);
        }
        ef_inst_spawn(self, 2);
        break;
    case 0x14E:
        self->field_0x45E = fn_8024CD8C(self, 0x2328);
        ef_inst_spawn(self, 5);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x20, 0);
            fn_8024D3C8(self);
        }
        break;
    case 0x15:
    case 0x16:
        if (Pl_master_ck(self) == 1) {
            if (self->field_0x306 == 0x15) {
                pl_defense_up_set(self, 0, 7);
            } else {
                pl_defense_up_set(self, 0, 0xF);
            }
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 2);
        break;
    case 0x14:
        if (Pl_master_ck(self) == 1) {
            pl_attack_up_set(self, 1, 0x19);
            self->field_0x44A = fn_8024CD8C(self, 0x276);
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 1);
        self->field_0x354 = pl_float_1_5;
        break;
    case 0x17:
        if (Pl_master_ck(self) == 1) {
            if (pl_defense_up_set(self, 1, 0x19) == 1) {
                self->field_0x44E = fn_8024CD8C(self, 0x276);
            }
            self->field_0x45C = self->field_0x44E;
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 2);
        self->field_0x354 = pl_float_1_5;
        break;
    case 0x19:
        self->field_0x466 = fn_8024CD8C(self, 0x4650);
        ef_inst_spawn(self, 7);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x22, 0);
            fn_8024D3C8(self);
        }
        break;
    case 0x18:
        self->field_0x464 = fn_8024CD8C(self, 0x4650);
        ef_inst_spawn(self, 6);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x21, 0);
            fn_8024D3C8(self);
        }
        break;
    case 0x14B:
        pl_gauge_ceiling_grow(self, 0xA, 0);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 5);
        break;
    case 0x14C:
        pl_gauge_ceiling_grow(self, 0x14, 0);
        fn_8024D3C8(self);
        ef_inst_spawn(self, 5);
        break;
    case 0x176:
        fn_80276CE8(self, 0x96);
        fn_8024D3C8(self);
        self->field_0x3F4 = 0;
        ef_inst_spawn(self, 5);
        break;
    case 0x2D:
        pl_stamina_add(self, 0x96);
        ef_inst_spawn(self, 5);
        break;
    case 0x1B:
        pl_stamina_add(self, 0x96);
        ef_inst_spawn(self, 5);
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x12E, 4, 0);
        }
        return;
    case 0xA7:
        self->field_0x466 = fn_8024CD8C(self, 0x4650);
        ef_inst_spawn(self, 7);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x22, 0);
        }
        pl_stamina_add(self, 0x12C);
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x12E, 4, 0);
        }
        return;
    case 0xA6:
        self->field_0x464 = fn_8024CD8C(self, 0x4650);
        ef_inst_spawn(self, 6);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x21, 0);
        }
        pl_stamina_add(self, 0x12C);
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x12E, 4, 0);
        }
        return;
    case 0x1A:
        ef_inst_spawn(self, 5);
        pl_stamina_add(self, 0x12C);
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x12E, 4, 0);
        }
        return;
    case 0x1C:
        if ((self->field_0x0B6 & 1) != 0) {
            pl_stamina_add(self, 0x96);
            ef_inst_spawn(self, 5);
            if (self->kind_0x09 == 3) {
                Pl_chr_set_attr_default(self, 0x162, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x12E, 4, 0);
            }
        } else {
            fn_80276CE8(self, -0x96);
            if (self->kind_0x09 == 3) {
                Pl_chr_set_attr_default(self, 0x163, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x134, 4, 0);
            }
        }
        return;
    case 0xF:
        fn_80276CE8(self, 0x96);
        self->field_0x412 = fn_8024CD8C(self, 0x1518);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x17, 0);
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 5);
        break;
    case 0x10:
        fn_80276CE8(self, 0x12C);
        self->field_0x412 = fn_8024CD8C(self, 0x2A30);
        if (Pl_master_ck(self) == 1) {
            pl_model_state_set(self, 1, 0x17, 0);
            fn_8024D3C8(self);
        }
        ef_inst_spawn(self, 5);
        break;
    case 0xD0:
        pl_health_recover(self, 0x14);
        ef_inst_spawn(self, 0);
        fn_8024D3C8(self);
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x12E, 4, 0);
        }
        return;
    case 0xD6:
        if (Pl_master_ck(self) == 1) {
            pl_item_add(self, self->field_0x306, -1);
        }
        if ((self->field_0x0B6 & 0x8000) == 0) {
            if (Pl_master_ck(self) == 1) {
                item = 0xC3;
                entry = pl_bonus_item_weights;
                roll = self->field_0x0B6 % 100;
                while (entry[0] != 0xFFFF) {
                    if (roll < entry[0]) {
                        item = entry[1];
                        break;
                    }
                    roll -= entry[0];
                    entry += 2;
                }
                result = item_get_with_msg(self, item, self->field_0x598, 3, 1);
                if ((u32)result > 2) {
                    switch (result) {
                    case 3:
                        snd_item_fail_play();
                        break;
                    case 5:
                        snd_item_fail_play();
                        break;
                    default:
                        se_item_pickup_play(item);
                        break;
                    }
                } else {
                    se_item_pickup_play(item);
                }
            }
            Pl_chr_set_attr_default(self, 0x134, 4, 0);
        } else {
            pl_health_recover(self, 0x14);
            ef_inst_spawn(self, 0);
            fn_8024D3C8(self);
            if (self->kind_0x09 == 3) {
                Pl_chr_set_attr_default(self, 0x162, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x12E, 4, 0);
            }
        }
        return;
    case 0x91:
        roll = self->field_0x0B6 % 100;
        if (roll < 30) {
            if (roll < 15) {
                if (roll < 1) {
                    if (self->kind_0x09 == 3) {
                        pl_act_enter_raw(self, 6, 0x21, 0xC);
                    } else {
                        pl_act_enter_raw(self, 6, 0x1F, 0xC);
                    }
                    if (self->kind_0x09 == 3) {
                        Pl_chr_set_attr_default(self, 0x162, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x12E, 4, 0);
                    }
                } else {
                    fn_80276CE8(self, -0x96);
                    if (self->kind_0x09 == 3) {
                        Pl_chr_set_attr_default(self, 0x163, 4, 0);
                    } else {
                        Pl_chr_set_attr_default(self, 0x134, 4, 0);
                    }
                }
                break;
            }
            if (roll < 20) {
                pl_gauge_ceiling_grow(self, 0x96, 0);
                pl_health_recover(self, 0x96);
                fn_8024D3C8(self);
                ef_inst_spawn(self, 0);
            } else {
                fn_80276CE8(self, 0x96);
                self->field_0x412 = fn_8024CD8C(self, 0x1518);
                if (Pl_master_ck(self) == 1) {
                    pl_model_state_set(self, 1, 0x17, 0);
                    fn_8024D3C8(self);
                }
                ef_inst_spawn(self, 5);
            }
        } else if (roll < 70) {
            if (roll < 50) {
                if (pl_defense_up_set(self, 1, 0x28) == 1) {
                    self->field_0x44E = fn_8024CD8C(self, 0x1518);
                }
                fn_8024D3C8(self);
                ef_inst_spawn(self, 2);
            } else {
                pl_attack_up_set(self, 1, 0xA);
                self->field_0x44A = fn_8024CD8C(self, 0x1518);
                fn_8024D3C8(self);
                ef_inst_spawn(self, 1);
            }
        } else {
            pl_health_recover(self, 0x28);
            fn_8024D3C8(self);
            ef_inst_spawn(self, 0);
        }
        break;
    case 0x158:
        fn_8024D3C8(self);
        fn_80264274(self, 0xA);
        if ((self->field_0x0B6 & 1) != 0) {
            pl_gauge_ceiling_grow(self, 0xA, 0);
        }
        if (self->kind_0x09 == 3) {
            Pl_chr_set_attr_default(self, 0x163, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x134, 4, 0);
        }
        return;
    }
    if (self->kind_0x09 == 3) {
        Pl_chr_set_attr_default(self, 0x162, 4, 0);
    } else {
        Pl_chr_set_attr_default(self, 0x133, 4, 0);
    }
    fn_8024CA50(self, 0);
}

/* Applies a party item (0x16A-0x16D) another player used: the local player, the AI partner and, for a remote
 * user, its own work each get the item's effect, and the AI companion its area buff. */
extern "C" void pl_act_net_hook_d(_PLW* self) {
    _PLW* me;
    _PLW* partner;

    me = my_player_work_get();
    self->field_0x65F = 0xF;
    if (Pl_master_ck(self) == 0 && self->area_0x16 != me->area_0x16) {
        return;
    }
    if (PlayMode_ck() == 2 && (partner = (_PLW*)pl_partner_work_get(self)) == NULL) {
        return;
    }
    switch (self->field_0x306) {
    case 0x16A:
        pl_health_recover(me, 0x14);
        ef_inst_spawn(me, 0);
        if (fn_8024D5AC(self, partner) == 1) {
            ef_inst_spawn(partner, 0);
            pl_health_recover(partner, 0x14);
        } else if (Pl_master_ck(self) == 0) {
            ef_inst_spawn(self, 0);
        }
        ainpc_area_buff_apply(4);
        break;
    case 0x16B:
        me->field_0x3EA = 0;
        ef_inst_spawn(me, 4);
        if (fn_8024D5AC(self, partner) == 1) {
            partner->field_0x3EA = 0;
            ef_inst_spawn(partner, 4);
        } else if (Pl_master_ck(self) == 0) {
            ef_inst_spawn(self, 4);
        }
        ainpc_area_buff_apply(3);
        break;
    case 0x16C:
        me->field_0x449 = 0xA;
        me->field_0x44A = 0x1518;
        ef_inst_spawn(me, 1);
        if (fn_8024D5AC(self, partner) == 1) {
            partner->field_0x449 = 0xA;
            partner->field_0x44A = 0x1518;
            ef_inst_spawn(partner, 1);
        } else if (Pl_master_ck(self) == 0) {
            ef_inst_spawn(self, 1);
        }
        ainpc_area_buff_apply(1);
        break;
    case 0x16D:
        if (pl_defense_up_set(me, 1, 0x14) == 1) {
            me->field_0x44E = 0x1518;
        }
        ef_inst_spawn(me, 2);
        if (fn_8024D5AC(self, partner) == 1) {
            if (pl_defense_up_set(partner, 1, 0x14) == 1) {
                partner->field_0x44E = 0x1518;
            }
            ef_inst_spawn(partner, 2);
        } else if (Pl_master_ck(self) == 0) {
            ef_inst_spawn(self, 2);
        }
        ainpc_area_buff_apply(2);
        break;
    }
}

/* The item-use act: acquires the item ticket, arms the use motion and (for the master) applies the item at once,
 * then returns to the kind's slot when the motion ends. */
extern "C" void pl_act_item_use(_PLW* self, s32 variant) {
    fn_802DE578_view1(self, &self->field_0x598);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
        }
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_main(self);
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The eat act: arms the eating motion (doubled with skill 0xB8) and, at frame 70, the master enters the
 * swallow act; the others loop the chew. */
extern "C" void pl_act_eat(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x14C, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x165, 6, 0);
        }
        self->field_0x28 = 0;
        pl_act_set_frame_timer(self);
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
        }
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_70, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1) {
                if (variant == 0) {
                    pl_act_enter(self, 0, 0x3C, 0xC);
                } else {
                    pl_act_enter(self, 0, 0x50, 0xC);
                }
            } else {
                self->act_step_0x05++;
            }
        }
        break;
    case 2:
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x14C, 0, 0x46);
        } else {
            Pl_chr_set_attr_default(self, 0x165, 0, 0x46);
        }
        break;
    }
}

/* The swallow act: arms the swallow motion, applies the eaten item for the master (spending it unless the cheat
 * input is held), then returns to the kind's slot when the motion ends. */
extern "C" void pl_act_swallow(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x14C, 0, 0x46);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x165, 0, 0x46);
        }
        pl_act_set_frame_timer(self);
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_b(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 8, 0);
        }
        break;
    }
}

/* Advances one step of the act entered from an off-hand gesture: the step byte picks the arming step or one of
 * the per-skill follow-up steps. */
extern "C" void pl_act_step_offhand_gesture(_PLW* self, s32 part)
{
    s32 motion;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (part == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 313, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 356, 4, 0);
        }
        self->field_0x018 = 0;
        pl_act_set_frame_timer(self);
        self->field_0x007 = 0;
        if (Pl_cat_skill_ck(self, 50) == 1) {
            s32 level = self->field_0x0B6 & 7;

            if (level == 0) {
                self->field_0x007 = 1;
            } else if (level <= 2) {
                self->field_0x007 = 2;
            } else {
                self->field_0x007 = 3;
            }
        }
        if (Pl_Skill_ck(self, 32) == 1) {
            self->field_0x007++;
        }
        break;
    case 1:
        if (Pl_Skill_ck(self, 31) == 1 || self->field_0x007 == 1) {
            if (Pl_frame_check(self, 0, pl_frame_window_96, pl_float_zero) == 1) {
                if (part == 0) {
                    Pl_chr_set_attr_default(self, 313, 0, 238);
                } else {
                    Pl_chr_set_attr_default(self, 356, 0, 238);
                }
            }
        } else if (self->field_0x006 == 0) {
            switch (self->field_0x007) {
            case 2:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 3:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 0:
                if (Pl_Skill_ck(self, 32) == 1) {
                    if (part == 0) {
                        if (Pl_frame_check(self, 0, pl_frame_window_246, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 313, 0, 200);
                        }
                    } else {
                        if (Pl_frame_check(self, 0, pl_frame_window_238, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 356, 0, 198);
                        }
                    }
                }
                break;
            }
        }
        if (Pl_frame_check(self, 0, pl_frame_window_260, pl_float_zero) == 1) {
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
                switch (self->field_0x306) {
                case 98:
                case 207:
                    motion = 150;
                    break;
                case 48:
                    motion = 100;
                    break;
                }
                fn_80278674(self, motion, 0);
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The launched act: arms the launch motion, then throws the actor up and forward on the motion's end, lands it on
 * the ground (a quake and the landing motion) and, when that ends, enters the get-up act. */
extern "C" void pl_act_launched(_PLW* self) {
    VEC3 launch;
    f32 ground;

    VEC3_ctor(&launch);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x2E, 4, 0);
        Pl_act_set_motion(self, 2, 0, 0);
        pl_act_set_motion_gate(self, 0);
        pl_sub_gauge_add(self, -150);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        pl_act_set_gauge_arm(self, 2);
        break;
    case 1:
        pl_act_set_gauge_arm(self, 2);
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x2F, 0, 0);
            self->vec_0x03C.y += pl_frame_window_100;
            launch.x = pl_float_zero;
            launch.y = pl_float_zero;
            launch.z = pl_frame_window_16;
            rotVecY(&launch, self->rot_0x54.y);
            self->motion_vel_0x78 = launch.x;
            self->motion_vel_0x7C = pl_frame_window_3;
            self->motion_vel_0x80 = launch.z;
            pl_motion_acc_clear(self);
            pl_jump_gravity_set(self, 8);
        }
        break;
    case 2:
        pl_motion_integrate(self);
        pl_act_set_gauge_arm(self, 2);
        if (pl_zone_ground_drop(self) != 1 && self->motion_vel_0x7C <= pl_float_zero) {
            ground = self->ground_y_0x060;
            if (self->vec_0x03C.y <= ground) {
                self->act_step_0x05++;
                self->vec_0x03C.y = ground;
                self->field_0x01B = 1;
                self->kind_0x09 = 0;
                Pl_chr_set_attr_default(self, 0x30, 0, 0);
                pl_act_hold_notify(self, 1);
                camera_pl_quake_req(self, 0);
                pl_stun_tick(self);
                pl_guard_latch_set(self);
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_set_gauge_arm(self, 0);
            if (pl_part_flag_ck(self, 0) == 1) {
                pl_act_enter(self, 1, 0x11, 0);
            } else {
                pl_act_enter(self, 1, 3, 0);
            }
        } else if (Pl_frame_check(self, 2, pl_frame_window_30, pl_float_zero) == 1) {
            pl_act_set_gauge_arm(self, 2);
        }
        break;
    }
}

/* Advances the pitfall act: arms the three motion words from the work record's frame data, then waits for the
 * step's frame window (or the master action's end) before handing the act to `fn_802F39DC`. */
extern "C" void pl_act_step_pitfall_arm(_PLW* self)
{
    u8 step;

    fn_802DE578_view1(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    step = self->act_step_0x05;
    switch (step) {
    case 0:
        self->act_step_0x05 = step + 1;
        Pl_chr_set_attr_default(self, 338, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
        self->field_0x018 = 0;
        self->field_0x596 = 1;
        self->field_0x59A = 0;
        self->field_0x597 = 0;
        self->field_0x59C[0] = 0;
        self->field_0x59C[1] = 0;
        self->field_0x59C[2] = 0;
        break;
    case 1:
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 64, 0);
        } else if (Pl_frame_check(self, 0, pl_frame_window_104, pl_float_zero) == 1) {
            if (self->act_step_0x05 == 1) {
                self->act_step_0x05++;
                fn_802F39DC(self, 0);
            }
        }
        break;
    default:
        break;
    }
}

/* Advances the same pitfall act's follow-up: it hands the act back to the action the three armed
 * motion words select, and stops for the frame window that the work record's own data names. */
extern "C" void pl_act_step_pitfall_hold(_PLW* self)
{
    s32 i;

    fn_802DE578_view1(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 339, 4, 0);
        break;
    case 1:
        if (self->field_0x59A != 0) {
            for (i = 0; i < 3; i++) {
                if ((self->field_0x59C[i] & 0x8000) != 0) {
                    pl_act_enter(self, 0, 154, 0);
                    return;
                }
            }
        }
        if (pl_part_flag_ck(self, 9) == 1) {
            if (self->field_0x59A != 0) {
                pl_act_enter(self, 0, 65, 0);
            } else if (self->field_0x596 == 0) {
                pl_act_enter(self, 0, 66, 0);
            } else {
                pl_act_enter(self, 0, 67, 0);
            }
        }
        break;
    }
}

/* Counts how many of the three motion words the work record still has armed: 1, or 2 when the first
 * two agree, plus one more when the third matches the first. */
extern "C" s32 pl_act_armed_motion_count(_PLW* self)
{
    s16 count = 1;

    if (self->field_0x59C[0] == self->field_0x59C[1]) {
        count = 2;
    }
    if (self->field_0x59C[0] == self->field_0x59C[2]) {
        count = count + 1;
    }
    return count;
}

/* The combine act (`variant` 1 repeats the mixing twice): mixes for one motion, then at frame 120 of the result
 * motion the master spends the item and takes the combined one. */
extern "C" void pl_act_combine(_PLW* self, s32 variant) {
    fn_802DE578_view1(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x800);
        self->field_0x18 = 0;
        Pl_chr_set_attr_default(self, 0x154, 2, 0);
        break;
    case 1:
        if (variant == 0) {
            if (Pl_frame_check(self, 0, pl_frame_window_118, pl_float_zero) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x155, 2, 0);
            }
        } else if (Pl_motion_end_ck(self) == 1) {
            if (++self->field_0x007 >= 2) {
                self->act_step_0x05 = 3;
                Pl_chr_set_attr_default(self, 0x157, 4, 0);
                if (Pl_motion_input_ck(1) == 0) {
                    pl_item_add(self, self->field_0x306, -1);
                }
            }
        }
        break;
    case 2:
        if (Pl_frame_check(self, 0, pl_frame_window_120, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1 && Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
                item_get_with_msg(self, self->armed_motion_0x59C, self->field_0x598, 7,
                                  pl_act_armed_motion_count(self));
            }
        } else if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD8, 2, 0x24);
        }
        break;
    case 4:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The book act: reads (and, for `variant` 0, spends) the item at frame 120 and raises its model state, then
 * returns to the idle slot. */
extern "C" void pl_act_book_read(_PLW* self, s32 variant) {
    fn_802DE578_view1(self, &self->field_0x598);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x800);
        self->field_0x18 = 0;
        Pl_chr_set_attr_default(self, 0x156, 2, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 0, pl_frame_window_120, pl_float_zero) != 0 && Pl_master_ck(self) == 1 &&
            Pl_motion_input_ck(1) == 0) {
            if (variant == 0) {
                pl_item_add(self, self->field_0x306, -1);
                pl_model_state_set(self, 1, 0xB, 0);
            } else {
                pl_model_state_set(self, 1, 0xF, 0);
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    }
}

/* The sit act for `variant` 0-2: sits the actor down with the camera locked, then the master stands up again on a
 * move input or a raised part flag once the 20-frame hold has run out. */
extern "C" void pl_act_sit(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 1, 0, 0);
        self->field_0x5E6 = 1;
        self->camera_lock_0x5E5 = 4;
        self->field_0x18 = 0;
        self->field_0x28 = 20;
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 7, 2, 0);
            break;
        case 1:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 8, 2, 0);
            break;
        case 2:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x76, 2, 0);
            Pl_act_set_motion(self, 3, 0, 0);
            break;
        }
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 8, 6, 0);
            pl_motion_stick_move(self, 8, 1);
        } else {
            pl_motion_stick_move(self, 7, 1);
        }
        break;
    case 2:
        if (Pl_master_ck(self) != 0) {
            if (variant == 2) {
                pl_motion_stick_move(self, 0x76, 1);
            } else {
                pl_motion_stick_move(self, 8, 1);
            }
            if (self->field_0x28 > 0) {
                self->field_0x28--;
            } else if (pl_part_flag_ck(self, 7) == 1 || pl_part_flag_ck(self, 1) == 1 || Pl_motion_input_ck(1) == 1) {
                pl_camera_lock_clear(self);
                if (variant == 2) {
                    Pl_act_set_motion_slot(self, 3, 4, 0);
                } else {
                    Pl_act_set_motion_slot(self, 1, 4, 0);
                }
            }
        }
        break;
    }
}

/* The rest-at-a-spot act: idles (with the sit-down variants after 30 frames) and, for the master, looks for the
 * area's rest spot when the wait has run out - walks to the bench in front of it, or hands over to the online
 * rest handler. */
extern "C" void pl_act_rest_spot(_PLW* self) {
    VEC3 spot;
    VEC3 ahead;
    _CP_VECTOR rot;
    s32 index;
    s16 kind;

    VEC3_ctor(&spot);
    VEC3_ctor(&ahead);
    if (--self->field_0x28 <= 0) {
        self->field_0x28 = 0;
    }
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 1, 4, 0);
        if (PlayMode_ck() == 2) {
            self->quest_menu_open_0x5BD = 0;
            self->field_0x28 = 0;
        } else {
            self->quest_menu_open_0x5BD = 0xFF;
            self->field_0x28 = 30;
        }
        self->cockpit_menu_row_0x5BF = 0;
        self->field_0x5C2 = 30;
        self->cockpit_menu_0x5C0 = 0;
        break;
    case 1:
        switch (Get_motion_no(self)) {
        case 0x1:
            if (self->field_0x28 <= 0) {
                if (self->se_name_set == 0) {
                    Pl_chr_set_attr_default(self, 0x42, 6, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x3F, 6, 0);
                }
            }
            break;
        case 0x3F:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x41, 2, 0);
            }
            break;
        case 0x42:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x43, 2, 0);
            }
            break;
        }
        if (Pl_master_ck(self) != 1 || self->field_0x28 > 0) {
            break;
        }
        if (Pl_motion_input_ck(1) == 1 || (PlayMode_ck() != 2 && self->quest_menu_open_0x5BD == 0)) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
            break;
        }
        if (PlayMode_ck() == 2 || self->quest_menu_open_0x5BD == 1) {
            switch (self->cockpit_menu_row_0x5BF) {
            case 0:
                if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &kind, &spot, &rot,
                                            &index) != -1) {
                    if (kind == 8) {
                        self->field_0x0A8 = rot.y;
                        ahead.x = pl_frame_window_120 * pl_rest_spot_offset[self->chunk_ofs];
                        ahead.y = pl_float_zero;
                        ahead.z = pl_float_zero;
                        rotVecY(&ahead, rot.y);
                        self->target_pos_0x090.x = spot.x + ahead.x;
                        self->target_pos_0x090.y = spot.y + ahead.y;
                        self->target_pos_0x090.z = spot.z + ahead.z;
                        pl_pos_blend_start(self, 20);
                        pl_act_enter(self, 0, 0x54, 0);
                        self->quest_menu_open_0x5BD = 0;
                    }
                } else {
                    Pl_act_set_motion_slot(self, 0, 4, 0);
                    self->quest_menu_open_0x5BD = 0;
                }
                break;
            case 1:
                pl_net_move_handover();
                Pl_act_set_motion_slot(self, 0, 4, 0);
                self->quest_menu_open_0x5BD = 0;
                break;
            }
        }
        break;
    }
}

/* Advances act 84: restarts the move work and re-arms the act's timer block (stagger timer, stamina/guard pair,
 * two four-word runs, act bitfield), then hands the act to 85 once the motion finishes. */
extern "C" void pl_act_step_84(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 326, -6, 0);
        self->field_0x3EA = 0;
        pl_act_clear_wait(self, 0);
        self->field_0x3FC = 0;
        self->field_0x42E = 0;
        self->field_0x430 = 0;
        self->field_0x432 = 0;
        self->field_0x434 = 0;
        self->field_0x436 = 0;
        self->field_0x422 = 0;
        self->field_0x404 = 0;
        self->field_0x406 = 0;
        self->field_0x408 = 0;
        self->field_0x40A = 0;
        self->field_0x40C = 0;
        self->field_0x3DC = 0;
        self->field_0x410 = 0;
        self->field_0x3F4 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 85, 0);
        }
        break;
    default:
        break;
    }
}

/* The resting act: plays the rest motion for 120 frames (with a random stretch), refilling the hold gauge, and
 * the master leaves it into act 0x56. */
extern "C" void pl_act_resting(_PLW* self) {
    VEC3 rise;

    VEC3_ctor(&rise);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x28 = 120;
        Pl_chr_set_attr_default(self, 0x147, 0, 0);
        ef_inst_spawn(self, 0);
        break;
    case 1:
        if (--self->field_0x28 <= 0 && Pl_master_ck(self) == 1) {
            pl_act_enter(self, 0, 0x56, 0);
            break;
        }
        if (Get_motion_no(self) == 0x147 && (ran_suu(1) & 0x7F) == 0) {
            Pl_chr_set_attr_default(self, 0x148, 4, 0);
        } else if (Get_motion_no(self) == 0x148 && Pl_motion_end_ck(self) == 1) {
            Pl_chr_set_attr_default(self, 0x147, 4, 0);
        }
        rise.x = pl_float_zero;
        rise.y = pl_frame_window_20;
        rise.z = pl_float_zero;
        pl_act_add_hold_gauge(self, 2);
        break;
    }
}

/* Advances act 86: the arming step turns the +0x058/+0x0A8 angle pair by half a turn (a `u16` wrap) and arms
 * motion 329; the follow-up hands the motion on once the model reports it finished. */
extern "C" void pl_act_step_86(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x058 = (u16)(self->field_0x058 + 0x8000);
        self->field_0x0A8 = self->field_0x058;
        Pl_chr_set_attr_default(self, 329, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The bound act (`variant` 0 the short, 1 the long one): raises the bind latch and its 10/30-frame timer, then
 * leaves to the idle slot once the latch drops or the master breaks free. */
extern "C" void pl_act_bound(_PLW* self, s32 variant) {
    u8 bound;

    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_setX(self, 1, 6, 0);
        if (variant == 0) {
            self->field_0x5BC = 1;
        } else {
            self->take_menu_open_0x5BE = 1;
            self->cockpit_menu_row_0x5BF = 0;
            self->cockpit_menu_0x5C0 = 0;
        }
        if (variant == 0) {
            self->field_0x5C2 = 10;
        } else {
            self->field_0x5C2 = 30;
        }
        Pl_act_set_motion(self, 0, 0, 0);
        if (Pl_master_ck(self) == 1) {
            sysSE_req(7);
        }
        break;
    case 1:
        if (variant == 0) {
            bound = self->field_0x5BC;
        } else {
            bound = self->take_menu_open_0x5BE;
        }
        if (bound == 0 || Pl_motion_input_ck(1) == 1) {
            pl_act_set_gauge_arm(self, 0);
            self->field_0x315 = 10;
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* Advances acts 89/90: the arming step sets the flag bit, arms motion 3, clears the rotation words and picks the
 * attribute per part and actor mode (102/117, 1065/1067); the follow-up hands the motion on when it finishes. */
extern "C" void pl_act_step_89(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_arm_flags(self, 1);
        Pl_act_set_motion(self, 3, 0, 0);
        self->param_0x54 = 0;
        self->field_0x0AC = 0;
        if (self->field_0x018 == 0) {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 102, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 117, 0, 0);
            }
        } else {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 1065, 8, 0);
            } else {
                Pl_chr_set_attr_default(self, 1067, 8, 0);
            }
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 8, 0);
        }
        break;
    }
}

/* The net-swing act: swings the bug net and, the first time it catches something, wears the net. */
extern "C" void pl_act_net_swing(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x167, 6, 0);
        pl_hit_table_arm(self, 5, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else if (pl_hit_result_best_get(self) != 0xFF && self->field_0x006 == 0) {
            self->field_0x006 = 1;
            pl_gather_tool_wear(self);
        }
        break;
    }
}

/* Advances act 94: the arming step arms motion 360 with attribute 4; the second step waits out the master gate
 * and the 20-frame window, then bumps the follow-up stage and hands the motion on. */
extern "C" void pl_act_step_94(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 360, 4, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && self->field_0x006 == 0) {
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->field_0x006++;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* The idle act: arms the idle motion and its step table (or the drawn weapon's), and keeps the master's alternate
 * weapon form motion running. */
extern "C" void pl_act_idle(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8000, 0, 0);
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        } else {
            fn_802771A0(self, 0);
        }
        /* fallthrough */
    case 1:
        if (Pl_master_ck(self) == 1 && pl_item_page2_ck(self) == 1 && self->field_0x18 == 1) {
            pl_motion_stick_move(self, 0x3E9, 0);
        }
        break;
    }
}

/* Advances act 121: arm motion 318 (attribute -4) with a 60-frame `+0x028` countdown decremented every call, wait
 * the 46-frame window and arm motion 345, then wait for the master gate and the countdown and hand the motion on. */
extern "C" void pl_act_step_121(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    if (self->field_0x28 != 0) {
        self->field_0x28--;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 318, -4, 0);
        self->field_0x28 = 60;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_46, pl_float_zero) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 345, 0, 0);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && self->field_0x28 == 0 && self->field_0x006 == 0) {
            self->field_0x006++;
            pl_model_state_set(self, 1, 25, 0);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    default:
        break;
    }
}

/* Advances acts 135/137: arm attribute 320 with the flag word (324 alone for the second part), then wait out the
 * 44-frame window and the part flag before re-entering the act; the second part counts the window up. */
extern "C" void pl_act_step_135(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        if (part == 0) {
            Pl_chr_set_attr_default(self, 320, 4, 0);
            pl_act_set_flag(self, 512);
        } else {
            Pl_chr_set_attr_default(self, 324, 4, 0);
        }
        break;
    case 1:
        switch (part) {
        case 0:
            if (self->field_0x006 >= 3 ||
                (Pl_frame_check(self, 1, pl_frame_window_44, pl_float_zero) == 1 &&
                 pl_part_flag_ck(self, 4) == 1)) {
                pl_act_enter(self, 0, 136, 0);
            } else if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                self->field_0x006++;
            }
            break;
        case 1:
            if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                if (++self->field_0x006 >= 2) {
                    pl_act_enter(self, 0, 138, 0);
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* The gather-by-hand act (`variant` 1 the party items 0x16A-0x16D): arms the motion and, for the master, wears the
 * item with the gathering skills' odds; then returns to the idle slot. */
extern "C" void pl_act_gather_hand(_PLW* self, s32 variant) {
    u16 odds;

    odds = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        if (variant == 0) {
            odds = 5;
            Pl_chr_set_attr_default(self, 0x141, 2, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x145, 4, 0);
            if (Pl_master_ck(self) == 1) {
                pl_act_net_hook_d(self);
            }
            switch (self->field_0x306) {
            case 0x16A:
                odds = 0xC;
                break;
            case 0x16B:
                odds = 0xC;
                break;
            case 0x16C:
                odds = 8;
                break;
            case 0x16D:
                odds = 8;
                break;
            }
        }
        if (Pl_master_ck(self) == 1 && Pl_motion_input_ck(1) == 0) {
            if (Pl_Skill_ck(self, 0x8F) == 1 && (ran_suu(1) & 3) == 0) {
                odds = (odds >> 1) + 1;
            }
            if (Pl_Skill_ck(self, 0x90) == 1 && (ran_suu(1) & 1) == 0) {
                odds = (odds >> 1) + 1;
            }
            if (ran_suu(1) % odds == 0 && (Pl_Skill_ck(self, 0x8D) != 1 || (ran_suu(1) & 3) != 0) &&
                (Pl_Skill_ck(self, 0x8E) != 1 || (ran_suu(1) & 1) != 0) &&
                (Pl_cat_skill_ck(self, 9) != 1 || (ran_suu(1) & 1) != 0)) {
                pl_model_state_set(self, 2, 0x16, self->field_0x306);
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* Advances act 151: the arming step clears the actor mode, arms motion 113 with attribute 2 and drops the two
 * latch flags; the follow-up hands the motion on once the model reports it finished. */
extern "C" void pl_act_step_151(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 113, 2, 0);
        pl_act_clear_flag5bb(self);
        pl_act_clear_mode5c4(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* The place act for `variant` 0-2 (`kneel` picks the kneeling motion): places the barrel bomb, the trap or the
 * item, then returns to the kind's slot when the motion ends. */
extern "C" void pl_act_place(_PLW* self, s32 variant, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x13B, -2, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x160, -4, 0);
        }
        pl_act_clear_flag5bb(self);
        switch (variant) {
        case 0:
            stg_barrel_bomb_place(self);
            break;
        case 1:
            stg_trap_place(self);
            break;
        case 2:
            pl_item_add(self, self->field_0x306, -1);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 6, 0);
        }
        break;
    }
}

/* The aim act (`variant` 1 the flare, `kneel` the kneeling motion): raises the slinger and, for the master,
 * fires the loaded shot on input, or puts the slinger away on a move input or after 60 frames. */
extern "C" void pl_act_slinger_aim(_PLW* self, s32 variant, s32 kneel) {
    u16 attr;

    pl_act_set_step_time(self, 2);
    attr = 0x16D;
    if (kneel == 0) {
        attr = 0x13A;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
        }
        if (variant == 0) {
            Pl_chr_set_attr_default(self, attr, -4, 0);
            self->aim_pitch_0x5E8 = 0;
            self->shell_ang_0x583 = 0;
            self->field_0x038 = self->field_0x0A8;
        } else {
            if (Pl_master_ck(self) == 1) {
                map_se_req(0xD, &self->vec_0x03C);
            }
            Pl_chr_set_attr_default(self, attr, 0, 0x42);
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 60;
        self->field_0x006 = 30;
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (self->field_0x006 != 0) {
            self->field_0x006--;
        }
        if (Pl_master_ck(self) != 1) {
            break;
        }
        if (Pl_motion_input_ck(1) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 6, 0);
            break;
        }
        if (self->field_0x006 == 0) {
            pl_motion_stick_move(self, attr, 2);
            if (pl_part_flag_ck(self, 0x4C) == 1 && Pl_item_timer_get(self, self->field_0x306) >= 1) {
                if (kneel == 0) {
                    pl_act_enter(self, 0, 0xAB, 0x6C);
                } else {
                    pl_act_enter(self, 0, 0xAE, 0x6C);
                }
                break;
            }
        }
        if (self->field_0x28 == 0 && pl_part_flag_ck(self, 7) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 6, 0);
        }
        break;
    }
}

/* The slinger-fire act (`kneel` the kneeling motion): fires the loaded shell (spending it unless the cheat input
 * is held) and, 60 frames on, re-enters the aim act while ammunition is left. */
extern "C" void pl_act_slinger_fire(_PLW* self, s32 kneel) {
    u16 item;

    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x13A, 0, 0x42);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16D, 0, 0x42);
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 60;
        if (Pl_master_ck(self) == 1 && Pl_item_timer_get(self, self->field_0x306) <= 0) {
            break;
        }
        if (Pl_motion_input_ck(1) == 0) {
            item = self->field_0x306;
            if (item != 0x180) {
                pl_item_add(self, item, -1);
            }
        }
        camera_pl_quake_req(self, 0);
        switch (self->field_0x306) {
        case 0x17C:
            shell_set_func_ptr->method_0x78(self, 3, shell_set_func_ptr);
            break;
        case 0x247:
            shell_set_func_ptr->method_0x78(self, 5, shell_set_func_ptr);
            break;
        case 0x180:
            stg_slinger_special_fire(self);
            break;
        }
        break;
    case 1:
        if (--self->field_0x28 == 0) {
            if (Pl_item_timer_get(self, self->field_0x306) >= 1 && self->field_0x306 != 0x180) {
                if (kneel == 0) {
                    pl_act_enter(self, 0, 0xAA, 0x2C);
                } else {
                    pl_act_enter(self, 0, 0xAD, 0x2C);
                }
            } else {
                Pl_act_set_motion_slot(self, self->kind_0x09, 6, 0);
            }
        }
        break;
    }
}

/* Advances act 175: the arming step clears the actor mode, restarts the move work and arms motion 307 with
 * attribute 4; the follow-up hands the act motion 1 once the model reports the current one finished. */
extern "C" void pl_act_step_175(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 307, 4, 0);
        ef_move_state_dispatch(0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 1, 4, 0);
        }
        break;
    }
}

/* The call-the-companion act: whistles for the AI companion and waits up to 120 frames; when it arrives beside the
 * player the actor enters the hand-over act. */
extern "C" void pl_act_companion_call(_PLW* self) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_setX(self, 1, 4, 0);
        self->field_0x28 = 120;
        ainpc_call_request();
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else if (ai_npc_beside_player_ck() == 1) {
            pl_act_enter(self, 0, 0x22, 0);
        }
        break;
    }
}

/* The talk act (`variant` 0 to the companion, 1 to an NPC): waits out the talk timer; the master's companion talk
 * counts down the remaining talks with the falling odds of the talk table (cat skill 0xB keeps them). */
extern "C" void pl_act_talk(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        Pl_chr_setX(self, 1, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_clear_flag5bb(self);
        self->field_0x28 = 60;
        self->talk_wait_0x668 = 60;
        if (Pl_master_ck(self) != 1) {
            break;
        }
        if (variant == 0) {
            if (self->talk_left_0x669 <= 6) {
                self->talk_left_0x669++;
                if ((s32)(ran_suu(1) % 100) >= pl_companion_talk_odds[self->talk_left_0x669] && Pl_cat_skill_ck(self, 0xB) == 0) {
                    self->talk_left_0x669 = 7;
                }
            } else {
                self->talk_left_0x669 = 7;
            }
        } else if (self->talk_left_0x669 < 3) {
            self->talk_left_0x669++;
        } else {
            self->talk_left_0x669 = 3;
        }
        break;
    case 1:
        if (self->talk_wait_0x668 == 0) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The act dispatcher: runs the step handler of the actor's act number; an unknown act falls back to the
 * idle slot. */
extern "C" void pl_act_group0_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0x0:
        pl_act_rest(self);
        break;
    case 0x1:
        pl_act_sheathe(self, 0);
        break;
    case 0x2:
        pl_act_sheathe(self, 1);
        break;
    case 0x3:
        pl_act_draw(self, 0);
        break;
    case 0x4:
        pl_act_draw(self, 1);
        break;
    case 0x5:
        pl_act_step_down(self, 0);
        break;
    case 0x6:
        pl_act_step_up(self);
        break;
    case 0x7:
        fn_802478A4(self);
        break;
    case 0x8:
        pl_act_flinch(self, 0);
        break;
    case 0x9:
        pl_act_flinch(self, 1);
        break;
    case 0xA:
        fn_80247EF0(self, 0);
        break;
    case 0xB:
        pl_act_stroke_hold(self, 0);
        break;
    case 0xC:
        pl_act_stroke_forward(self, 0, 0);
        break;
    case 0xD:
        pl_act_stroke_back(self, 0, 0);
        break;
    case 0xE:
        fn_802488D4(self, 0);
        break;
    case 0xF:
        pl_act_knockback(self, 0, 0, 0);
        break;
    case 0x10:
        pl_act_knockback(self, 1, 0, 0);
        break;
    case 0x11:
        pl_act_item_throw(self, 0);
        break;
    case 0x12:
        pl_act_item_throw(self, 1);
        break;
    case 0x13:
        pl_act_item_throw(self, 2);
        break;
    case 0x14:
        fn_80249424(self, 0);
        break;
    case 0x15:
        fn_80249558(self, 0);
        break;
    case 0x16:
        fn_802498E0(self);
        break;
    case 0x17:
        pl_act_evade(self, 2, 0, 0);
        break;
    case 0x18:
        pl_act_evade(self, 3, 0, 0);
        break;
    case 0x19:
        pl_act_evade(self, 0, 0, 0);
        break;
    case 0x1A:
        pl_act_evade(self, 1, 0, 0);
        break;
    case 0x1B:
        fn_8024A248(self, 0);
        break;
    case 0x1C:
        pl_act_sheathe(self, 2);
        break;
    case 0x1E:
        fn_8024A51C(self, 0);
        break;
    case 0x1F:
        fn_8024A640(self);
        break;
    case 0x20:
        fn_8024A6CC(self);
        break;
    case 0x21:
        fn_8024A75C(self, 0);
        break;
    case 0x22:
        fn_8024A8EC(self, 0);
        break;
    case 0x23:
        pl_act_fishing(self, 0);
        break;
    case 0x24:
        fn_8024B35C(self, 0);
        break;
    case 0x25:
        fn_8024B46C(self, 0);
        break;
    case 0x26:
        fn_8024B594(self, 0);
        break;
    case 0x28:
        fn_8024B66C(self, 0);
        break;
    case 0x29:
        fn_8024B868(self, 0);
        break;
    case 0x2C:
        pl_act_gather(self, 0);
        break;
    case 0x2D:
        pl_act_gather(self, 1);
        break;
    case 0x2E:
        pl_act_draw(self, 2);
        break;
    case 0x2F:
        pl_act_carve_finish(self);
        break;
    case 0x30:
        fn_8024C75C(self);
        break;
    case 0x31:
        pl_act_gather(self, 2);
        break;
    case 0x32:
        pl_act_gather(self, 3);
        break;
    case 0x33:
        pl_act_gather(self, 4);
        break;
    case 0x34:
        pl_act_gather(self, 5);
        break;
    case 0x35:
        pl_act_carve(self, 0);
        break;
    case 0x36:
        pl_act_carve(self, 1);
        break;
    case 0x37:
        pl_act_carve(self, 2);
        break;
    case 0x38:
        pl_act_carve(self, 3);
        break;
    case 0x39:
        pl_act_item_use(self, 0);
        break;
    case 0x3A:
        pl_act_item_use(self, 1);
        break;
    case 0x3B:
        pl_act_eat(self, 0);
        break;
    case 0x3C:
        pl_act_swallow(self, 0);
        break;
    case 0x3D:
        pl_act_step_offhand_gesture(self, 0);
        break;
    case 0x3E:
        pl_act_launched(self);
        break;
    case 0x3F:
        pl_act_step_pitfall_arm(self);
        break;
    case 0x40:
        pl_act_step_pitfall_hold(self);
        break;
    case 0x41:
        pl_act_combine(self, 0);
        break;
    case 0x42:
        pl_act_book_read(self, 0);
        break;
    case 0x43:
        pl_act_book_read(self, 1);
        break;
    case 0x44:
        pl_act_sit(self, 0);
        break;
    case 0x45:
        pl_act_sit(self, 1);
        break;
    case 0x46:
        pl_act_rest_spot(self);
        break;
    case 0x47:
        pl_act_sit(self, 2);
        break;
    case 0x48:
        pl_act_step_offhand_gesture(self, 1);
        break;
    case 0x49:
        fn_8024963C(self, 0, 0);
        break;
    case 0x4A:
        fn_80247EF0(self, 1);
        break;
    case 0x4B:
        pl_act_stroke_hold(self, 1);
        break;
    case 0x4C:
        pl_act_stroke_forward(self, 1, 0);
        break;
    case 0x4D:
        pl_act_stroke_back(self, 1, 0);
        break;
    case 0x4E:
        fn_802488D4(self, 1);
        break;
    case 0x4F:
        pl_act_eat(self, 1);
        break;
    case 0x50:
        pl_act_swallow(self, 1);
        break;
    case 0x51:
        pl_act_evade(self, 4, 0, 0);
        break;
    case 0x52:
        pl_act_evade(self, 5, 0, 0);
        break;
    case 0x53:
        pl_act_draw(self, 3);
        break;
    case 0x54:
        pl_act_step_84(self);
        break;
    case 0x55:
        pl_act_resting(self);
        break;
    case 0x56:
        pl_act_step_86(self);
        break;
    case 0x57:
        pl_act_bound(self, 0);
        break;
    case 0x58:
        pl_act_bound(self, 1);
        break;
    case 0x59:
        pl_act_step_89(self, 0);
        break;
    case 0x5A:
        pl_act_step_89(self, 1);
        break;
    case 0x5B:
        fn_80249768(self, 0);
        break;
    case 0x5D:
        pl_act_net_swing(self);
        break;
    case 0x5E:
        pl_act_step_94(self);
        break;
    case 0x60:
        fn_8024B594(self, 1);
        break;
    case 0x61:
        pl_act_gather(self, 6);
        break;
    case 0x62:
        pl_act_gather(self, 7);
        break;
    case 0x63:
        pl_act_gather(self, 8);
        break;
    case 0x64:
        pl_act_evade(self, 6, 0, 0);
        break;
    case 0x65:
        pl_act_evade(self, 7, 0, 0);
        break;
    case 0x66:
        pl_act_stroke_forward(self, 0, 1);
        break;
    case 0x67:
        pl_act_stroke_back(self, 0, 1);
        break;
    case 0x68:
        pl_act_stroke_forward(self, 1, 1);
        break;
    case 0x69:
        pl_act_stroke_back(self, 1, 1);
        break;
    case 0x6A:
        pl_act_idle(self);
        break;
    case 0x6C:
        pl_act_item_throw(self, 3);
        break;
    case 0x6D:
        pl_act_item_throw(self, 4);
        break;
    case 0x6E:
        pl_act_item_throw(self, 5);
        break;
    case 0x6F:
        fn_8024A8EC(self, 1);
        break;
    case 0x70:
        fn_8024A8EC(self, 2);
        break;
    case 0x71:
        pl_act_fishing(self, 1);
        break;
    case 0x72:
        pl_act_fishing(self, 2);
        break;
    case 0x73:
        fn_8024B35C(self, 1);
        break;
    case 0x74:
        fn_8024B35C(self, 2);
        break;
    case 0x75:
        fn_8024B46C(self, 1);
        break;
    case 0x76:
        fn_8024B46C(self, 2);
        break;
    case 0x77:
        pl_act_fishing(self, 3);
        break;
    case 0x78:
        pl_act_fishing(self, 4);
        break;
    case 0x79:
        pl_act_step_121(self);
        break;
    case 0x7A:
        pl_act_sheathe(self, 3);
        break;
    case 0x7B:
        pl_act_knockback(self, 2, 0, 0);
        break;
    case 0x7C:
        fn_8024A248(self, 1);
        break;
    case 0x7D:
        fn_8024C878(self);
        break;
    case 0x7E:
        fn_8024C96C(self);
        break;
    case 0x7F:
        pl_act_gather(self, 9);
        break;
    case 0x80:
        pl_act_knockback(self, 0, 1, 0);
        break;
    case 0x81:
        pl_act_knockback(self, 1, 1, 0);
        break;
    case 0x82:
        pl_act_knockback(self, 2, 1, 0);
        break;
    case 0x83:
        pl_act_knockback(self, 0, 2, 0);
        break;
    case 0x84:
        pl_act_knockback(self, 1, 2, 0);
        break;
    case 0x85:
        pl_act_knockback(self, 2, 2, 0);
        break;
    case 0x86:
        pl_act_knockback(self, 3, 0, 0);
        break;
    case 0x87:
        pl_act_step_135(self, 0);
        break;
    case 0x88:
        pl_act_gather_hand(self, 0);
        break;
    case 0x89:
        pl_act_step_135(self, 1);
        break;
    case 0x8A:
        pl_act_gather_hand(self, 1);
        break;
    case 0x8B:
        pl_act_knockback(self, 2, 0, 1);
        break;
    case 0x8C:
        pl_act_evade(self, 0, 0, 0);
        break;
    case 0x8D:
        pl_act_evade(self, 1, 0, 0);
        break;
    case 0x8E:
        pl_act_evade(self, 8, 0, 0);
        break;
    case 0x8F:
        pl_act_evade(self, 0, 1, 0);
        break;
    case 0x90:
        pl_act_evade(self, 1, 1, 0);
        break;
    case 0x91:
        pl_act_evade(self, 8, 1, 0);
        break;
    case 0x92:
        pl_act_evade(self, 0, 2, 0);
        break;
    case 0x93:
        pl_act_evade(self, 1, 2, 0);
        break;
    case 0x94:
        pl_act_evade(self, 8, 2, 0);
        break;
    case 0x95:
        pl_act_evade(self, 2, 0, 1);
        break;
    case 0x96:
        fn_8024A75C(self, 1);
        break;
    case 0x97:
        pl_act_step_151(self);
        break;
    case 0x98:
        fn_8024B66C(self, 1);
        break;
    case 0x99:
        fn_8024B868(self, 1);
        break;
    case 0x9A:
        pl_act_combine(self, 1);
        break;
    case 0x9B:
        fn_8024B66C(self, 2);
        break;
    case 0x9C:
        fn_8024B868(self, 2);
        break;
    case 0x9D:
        fn_8024A75C(self, 2);
        break;
    case 0x9E:
        fn_80249424(self, 1);
        break;
    case 0x9F:
        fn_80249558(self, 1);
        break;
    case 0xA0:
        fn_8024963C(self, 1, 0);
        break;
    case 0xA1:
        fn_80249768(self, 1);
        break;
    case 0xA2:
        fn_8024B35C(self, 3);
        break;
    case 0xA3:
        fn_8024B35C(self, 4);
        break;
    case 0xA4:
        fn_8024B46C(self, 3);
        break;
    case 0xA5:
        fn_8024B46C(self, 4);
        break;
    case 0xA6:
        pl_act_talk(self, 1);
        break;
    case 0xA7:
        pl_act_place(self, 0, 0);
        break;
    case 0xA8:
        pl_act_place(self, 1, 0);
        break;
    case 0xA9:
        pl_act_slinger_aim(self, 0, 0);
        break;
    case 0xAA:
        pl_act_slinger_aim(self, 1, 0);
        break;
    case 0xAB:
        pl_act_slinger_fire(self, 0);
        break;
    case 0xAC:
        pl_act_slinger_aim(self, 0, 1);
        break;
    case 0xAD:
        pl_act_slinger_aim(self, 1, 1);
        break;
    case 0xAE:
        pl_act_slinger_fire(self, 1);
        break;
    case 0xAF:
        pl_act_step_175(self);
        break;
    case 0xB0:
        pl_act_companion_call(self);
        break;
    case 0xB1:
        pl_act_talk(self, 0);
        break;
    case 0xB2:
        pl_act_place(self, 0, 1);
        break;
    case 0xB3:
        fn_8024963C(self, 0, 1);
        break;
    case 0xB4:
        fn_8024963C(self, 1, 1);
        break;
    case 0xB5:
        pl_act_flinch(self, 2);
        break;
    case 0xB6:
        pl_act_knockback(self, 4, 0, 0);
        break;
    case 0xB7:
        fn_8024A248(self, 2);
        break;
    case 0xB8:
        fn_8024A248(self, 3);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The guard act: raises the guard (the drawn weapon's guard motion per kind and alternate form), then keeps it
 * while the master holds the guard input and drops it to the idle slot once the guard timer has run out. */
extern "C" void pl_act_guard(_PLW* self) {
    VEC3 push;

    VEC3_ctor(&push);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (self->field_0x18 == 1) {
            switch (self->field_0x002) {
            case 8:
                if (pl_mode8_free_ck() == 1) {
                    Pl_chr_set_attr_default(self, 0x3EC, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x400, 4, 0);
                }
                break;
            case 4:
                if (pl_weapon_alt_form_ck_c1(self) == 0) {
                    Pl_chr_set_attr_default(self, 0x3EC, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x408, 4, 0);
                }
                break;
            case 5:
                if (pl_weapon_alt_form_ck_c1(self) == 0) {
                    Pl_chr_set_attr_default(self, 0x412, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x411, 4, 0);
                }
                self->field_0x354 = pl_float_0_95;
                break;
            case 6:
                if (pl_weapon_alt_form_ck_c1(self) == 0) {
                    Pl_chr_set_attr_default(self, 0x409, 4, 0);
                } else {
                    Pl_chr_set_attr_default(self, 0x40A, 4, 0);
                }
                self->field_0x354 = pl_float_0_9;
                break;
            default:
                Pl_chr_set_attr_default(self, 0x3EC, 4, 0);
                break;
            }
        } else {
            Pl_chr_set_attr_default(self, 2, 4, 0);
        }
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x40);
        self->field_0x266 = 10;
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        } else {
            fn_802771A0(self, 0);
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x266 <= 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            }
        }
        break;
    }
}

/* The crouch act for `variant` 0-2: crouches (the hide motion for variant 2) and, for the master, stays while the
 * hold input lasts, then stands back up into the walk act. */
extern "C" void pl_act_crouch(_PLW* self, s32 variant) {
    VEC3 push;
    u8 limit;

    VEC3_ctor(&push);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x80);
        self->field_0x266 = 10;
        switch (variant) {
        case 0:
        case 1:
            if (variant == 0 && Get_motion_no(self) != 0x26) {
                Pl_chr_setX(self, 3, 6, 0);
            } else {
                Pl_chr_set_attr_default(self, 3, 0, 0x26);
            }
            Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
            break;
        case 2:
            Pl_act_set_step_table(self, (u32)pl_steps_crouch, 0);
            Pl_chr_setX(self, 0x19D, 4, 0);
            break;
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            limit = (variant == 2) ? 1 : 2;
            if (pl_act_param_tier_ck(self, 0) >= limit) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x266 <= 0 && pl_act_param_tier_ck(self, 0) == 0) {
                if (variant == 2) {
                    pl_act_enter(self, 1, 0x19, 0);
                } else {
                    pl_act_enter(self, 1, 3, 0);
                }
            } else if (pl_act_param_tier_ck(self, 0) != 0) {
                pl_act_set_cam_ang(self);
            }
        }
        break;
    }
}

/* The sneak act: creeps forward while the hold input lasts, falls back to the walk act when the stamina runs out,
 * enters the spotted act when an enemy watches the actor, and drops off a ledge it walks onto. */
extern "C" void pl_act_sneak(_PLW* self, s32 variant) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x100);
        if (variant == 0) {
            Pl_chr_setX(self, 3, 4, 0);
        } else {
            Pl_chr_setX(self, 3, 4, 0x1E);
        }
        self->field_0x354 = pl_float_1_25;
        self->field_0x266 = 10;
        Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        self->field_0x314 = 30;
        pl_act_arm_flags(self, 1);
        self->field_0x0A8 = self->rot_0x54.y;
        break;
    case 1:
        pl_act_gauge_gate(self, -1);
        if (Pl_master_ck(self) != 1) {
            break;
        }
        if (pl_stagger_budget_ck(self, 150) == 0) {
            pl_act_enter(self, 1, 0xF, 0);
        } else if (em_player_watched_ck(self->chunk_ofs) == 1) {
            pl_act_enter(self, 1, 0x10, 0);
            pl_act_arm_flags(self, 1);
        } else if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, 0) == 1) {
            pl_act_set_cam_ang(self);
            if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) == 1 && self->field_0x585 == 0) {
                self->field_0x0A8 = self->rot_0x54.y;
                pl_act_enter(self, 2, 0, 0xC);
            } else {
                step.x = pl_float_zero;
                step.y = pl_float_zero;
                step.z = pl_frame_window_5;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
            }
        } else if (self->field_0x266 <= 0 && pl_act_param_tier_ck(self, 0) == 0) {
            pl_act_enter(self, 1, 3, 0);
        } else if (pl_act_param_tier_ck(self, 0) != 0) {
            pl_act_set_cam_ang(self);
        }
        break;
    }
}

/* The turn act (`variant` 1 the hidden turn): turns the actor on the spot, then returns to the idle slot. */
extern "C" void pl_act_turn(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0xC0);
        self->field_0x0A8 = self->rot_0x54.y;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x1E, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_steps_turn, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x19E, 4, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The quick-turn act: snaps the facing to the stored one, plays the turn and enters the dash act when it ends. */
extern "C" void pl_act_quick_turn(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0xC0);
        self->rot_0x54.y = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 0x2A, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_steps_turn, 0);
        self->field_0x31C = 0;
        self->field_0x320 = 999;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 1, 5, 0);
        }
        break;
    }
}

/* The dash act (`variant` 1 continues a running dash): runs forward at the weapon kind's speed (faster with skill
 * 0xC5); the master keeps dashing while the tier holds, stepping the guard gauge, and stops into the walk act or the
 * drawn weapon's slot once the dash timer runs out. */
extern "C" void pl_act_dash(_PLW* self, s32 variant) {
    VEC3 run;

    VEC3_ctor(&run);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0xC0);
        self->field_0x266 = 10;
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805BE824, 0);
            if (variant == 0) {
                if (Get_motion_no(self) != 0x65) {
                    Pl_chr_set_attr_default(self, 0x6D, 4, 0);
                }
            } else {
                Pl_chr_set_attr(self, 0x65, 6, 0, 1);
            }
        } else {
            Pl_chr_set_attr_default(self, 0x41D, 4, 0);
            fn_802771A0(self, 0);
            switch (self->field_0x002) {
            case 5:
                self->field_0x354 = pl_float_1_15;
                break;
            case 6:
                self->field_0x354 = pl_float_1_25;
                break;
            }
        }
        self->field_0x314 = self->field_0x266;
        self->field_0x646 = 0;
        self->field_0x648 = 0;
        if (Pl_Skill_ck(self, 0xC5) == 1) {
            self->field_0x354 *= pl_float_1_05;
        }
        break;
    case 1:
        run.x = pl_float_zero;
        run.y = pl_float_zero;
        if (Get_motion_no(self) == 0x41D) {
            run.z = pl_float_zero;
            switch (self->field_0x002) {
            case 5:
                run.z = pl_frame_window_3;
                break;
            case 6:
                run.z = pl_frame_window_5;
                break;
            case 8:
                if (pl_mode8_free_ck() == 1) {
                    run.z = pl_frame_window_3;
                }
                break;
            }
        } else if (Pl_Skill_ck(self, 0xC5) == 1) {
            run.z = pl_float_10_5;
        } else {
            run.z = pl_frame_window_10;
        }
        if (Get_motion_no(self) == 0x6D && Pl_motion_end_ck(self) == 1) {
            Pl_chr_set_attr(self, 0x65, 0xA, 0, 1);
        }
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 2) {
                pl_camera_heading_update(self);
                pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
            } else if (self->field_0x266 <= 0) {
                if (self->field_0x18 == 0) {
                    pl_act_enter(self, 1, 8, 0x80);
                } else {
                    Pl_act_set_motion_slot(self, 3, 0xA, 0);
                }
            }
        }
        rotVecXYZ(&run, &self->rot_0x54);
        addVec3To(&self->vec_0x03C, &run);
        break;
    }
}

/* 0x80252F24 (0xB0): Arms the act's motion and step table, then holds the motion until it ends. */
extern "C" void pl_act_step_arm_table(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0x40);
        Pl_chr_set_attr_default(self, 0x6E, 4, 0);
        Pl_act_set_step_table(self, (u32)pl_act_step_table_247, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* The walk act: walks forward (the drawn weapon's walk, or the sheathed walk timed to the previous run, faster or
 * slower with skills 0xC5/0xC6); the master falls back to the tired walk when the stamina runs out, enters the
 * spotted act when watched, steps the guard gauge while the walk input holds, and stops when the input ends. */
extern "C" void pl_act_walk(_PLW* self, s32 unused) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0xC0);
        if (self->field_0x18 == 1) {
            Pl_chr_setX(self, 0x41D, 4, 0);
            self->field_0x006 = 0;
        } else {
            if (Get_motion_no(self) == 0x73) {
                Pl_chr_set_attr(self, 0x77, 4, (s32)(pl_rig_get_motion_frame(self) * pl_float_0_5), 1);
            } else {
                Pl_chr_setX(self, 0x77, 4, 0);
            }
            pl_act_arm_flags(self, 1);
            self->field_0x006 = 1;
            if (Pl_Skill_ck(self, 0xC5) == 1) {
                self->field_0x354 *= pl_float_1_1;
            } else if (Pl_Skill_ck(self, 0xC6) == 1) {
                self->field_0x354 *= pl_float_0_9;
            }
        }
        self->field_0x266 = 10;
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_walk, 0);
        } else {
            fn_802771A0(self, 0);
        }
        self->field_0x28 = 40;
        self->field_0x007 = 15;
        self->field_0x646 = 0;
        self->field_0x648 = 0;
        break;
    case 1:
        if (self->field_0x006 != 0) {
            pl_act_gauge_gate(self, -1);
        }
        if (self->field_0x007 != 0) {
            self->field_0x007--;
        }
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_master_ck(self) == 1) {
            if (self->field_0x006 != 0) {
                if (pl_stagger_budget_ck(self, 150) == 0) {
                    pl_act_enter(self, 1, 0x24, 1);
                    break;
                }
                if (em_player_watched_ck(self->chunk_ofs) == 1) {
                    pl_act_enter(self, 1, 0x1B, 1);
                    break;
                }
            }
            if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, 0x25) == 1) {
                if (self->field_0x007 == 0) {
                    pl_camera_heading_update(self);
                    pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
                }
            } else {
                if (self->field_0x28 <= 0) {
                    if (pl_act_param_tier_ck(self, 0) >= 2) {
                        pl_act_enter(self, 1, 7, 0);
                    } else {
                        pl_act_enter(self, 1, 8, 0);
                    }
                    break;
                }
                if (self->field_0x007 == 0 && pl_act_param_tier_ck(self, 0) >= 1) {
                    pl_camera_heading_update(self);
                }
            }
        }
        step.x = pl_float_zero;
        step.y = pl_float_zero;
        step.z = pl_frame_window_2;
        rotVecXYZ(&step, &self->rot_0x54);
        self->vec_0x03C.x += step.x;
        self->vec_0x03C.y += step.y;
        self->vec_0x03C.z += step.z;
        break;
    }
}

/* The stop act for `variant` 0-3: plays the stop (or the stumble) and its step table, then the master returns to
 * the idle slot, or the walk/stagger acts the variant picks. */
extern "C" void pl_act_stop(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 1, 0, 0);
        pl_act_set_flag(self, 0x40);
        self->field_0x18 = 0;
        switch (variant) {
        case 0:
            self->field_0x266 = 0;
            Pl_chr_set_attr_default(self, 0x25, 6, 0);
            Pl_act_set_step_table(self, (u32)lbl_805BE5B8, 0);
            break;
        case 1:
            self->field_0x266 = 10;
            Pl_chr_set_attr_default(self, 0xA, 6, 0);
            Pl_act_set_step_table(self, (u32)lbl_805BE5B8, 0);
            break;
        case 2:
            self->field_0x266 = 10;
            Pl_chr_set_attr_default(self, 0xA, 6, 0);
            break;
        case 3:
            self->field_0x266 = 0;
            Pl_chr_set_attr_default(self, 0x25, 6, 0);
            Pl_act_set_step_table(self, (u32)pl_steps_stumble, self->field_0x266);
            break;
        }
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if ((u32)(variant - 1) > 1) {
                if (variant != 3) {
                    pl_act_enter(self, 1, 0xC, 0);
                } else {
                    pl_act_enter(self, 1, 0x26, 0);
                }
            } else if (self->field_0x266 <= 0) {
                Pl_act_set_motion_slot(self, 1, 4, 0);
            }
        }
        break;
    }
}

/* Arms the 15-frame `+0x028` timer with motion 1 and attribute 8/10, counts it down while the master gate holds
 * and hands the motion on at zero (tail-called by `pl_act_step_arm_hold`). */
extern "C" void pl_act_step_timer_wait(_PLW* self, s32 unused)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 15;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 8, 10, 0);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (--self->field_0x28 <= 0) {
                Pl_act_set_motion_slot(self, 1, 4, 0);
            }
        }
        break;
    }
}

/* The look-around act: plays the look motion and its step table, then the master returns to the idle slot. */
extern "C" void pl_act_look_around(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x40);
        self->field_0x266 = 10;
        Pl_chr_set_attr_default(self, 0x31, 6, 0);
        Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x266 <= 0) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            }
        }
        break;
    }
}

/* The tired-walk act (`variant` 1 the hidden one): trudges forward; the master drops off a ledge it walks onto,
 * collapses when the stamina is gone and returns to the walk act when the input ends. */
extern "C" void pl_act_tired_walk(_PLW* self, s32 variant) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x180);
        pl_act_arm_flags(self, 1);
        Pl_chr_set_attr_default(self, 0x27, 0, 0);
        pl_act_set_frame_timer(self);
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x266 = 10;
        if (variant == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        } else {
            Pl_act_set_step_table(self, (u32)pl_steps_crouch, 0);
        }
        self->field_0x314 = self->field_0x266;
        break;
    case 1:
        pl_act_gauge_gate(self, -1);
        step.x = pl_float_zero;
        step.y = pl_float_zero;
        step.z = pl_float_1_5;
        rotVecY(&step, self->rot_0x54.y);
        self->vec_0x03C.x += step.x;
        self->vec_0x03C.z += step.z;
        if (Pl_master_ck(self) == 0) {
            break;
        }
        if (pl_act_param_tier_ck(self, 0) >= 1) {
            pl_act_set_cam_ang(self);
            if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) == 1 && self->field_0x585 == 0) {
                self->field_0x0A8 = self->rot_0x54.y;
                pl_act_enter(self, 2, 0, 0xC);
                break;
            }
            if (pl_part_flag_ck(self, 0) != 0) {
                break;
            }
        }
        if ((s16)self->field_0x378 <= 0) {
            if (variant == 0) {
                pl_act_enter(self, 0, 0x21, 0);
            } else {
                pl_act_enter(self, 0, 0x9D, 0);
            }
        } else if (self->field_0x266 <= 0) {
            if (variant == 0) {
                pl_act_enter(self, 1, 3, 0);
            } else {
                pl_act_enter(self, 1, 0x19, 0);
            }
        }
        break;
    }
}

/* Picks the guard gauge band the cat-skill arm selects: skill 20 narrows it by one step, its absence
 * by two. */
extern "C" void pl_act_gauge_gate_by_skill(_PLW* self)
{
    if (Pl_cat_skill_ck(self, 20) == 1) {
        pl_act_gauge_gate(self, -1);
    } else {
        pl_act_gauge_gate(self, -2);
    }
}

/* The tiptoe act for `variant` 0-1: sneaks forward with the stealth gauge the skills grant; the master falls back
 * to the tired walk, re-enters the walk when nothing watches, jumps on a part flag and drops off a ledge ahead. */
extern "C" void pl_act_tiptoe(_PLW* self, s32 variant) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x180);
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0x26, 0, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x26, 4, 0);
            break;
        }
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x266 = 10;
        Pl_act_set_step_table(self, (u32)pl_steps_idle, 0);
        self->field_0x314 = self->field_0x266;
        pl_act_arm_flags(self, 1);
        pl_act_gauge_gate_by_skill(self);
        break;
    case 1:
        if (Pl_master_ck(self) == 0) {
            break;
        }
        pl_act_gauge_gate_by_skill(self);
        if (pl_stagger_budget_ck(self, 150) == 0) {
            pl_act_enter(self, 1, 0xF, 0);
        } else if (em_player_watched_ck(self->chunk_ofs) == 0 && self->field_0x266 <= 0) {
            pl_act_enter(self, 1, 0xE, 0);
            pl_act_arm_flags(self, 1);
        } else if (pl_part_flag_ck(self, 4) == 1 && (s16)self->field_0x378 >= 150) {
            self->field_0x0A8 = self->rot_0x54.y;
            pl_act_enter(self, 0, 0x3E, 4);
        } else if (pl_act_param_tier_ck(self, 0) >= 2) {
            pl_act_set_cam_ang(self);
            if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) == 1 && self->field_0x585 == 0) {
                self->field_0x0A8 = self->rot_0x54.y;
                pl_act_enter(self, 2, 0, 0xC);
            } else {
                step.x = pl_float_zero;
                step.y = pl_float_zero;
                step.z = pl_frame_window_5;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
            }
        } else if (self->field_0x266 <= 0) {
            pl_act_enter(self, 1, 3, 0);
        }
        break;
    }
}

/* The dash-start act: arms the dash motion, clamps the stored tilt to +-60 degrees and burns 4 stamina a frame
 * for 25 frames, then chains into the dash or the run when the motion ends. */
extern "C" void pl_act_dash_start(_PLW* self, s32 unused) {
    u32 tilt;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0xC0);
        Pl_chr_set_attr_default(self, 0x69, 4, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_step_table(self, (u32)pl_steps_dash, 0);
        pl_act_arm_flags(self, 1);
        self->field_0x28 = 25;
        tilt = self->field_0x0AC;
        if ((s16)tilt < 0) {
            if ((u16)tilt <= 0xD556) {
                self->field_0x0AC = 0xD556;
            }
        } else if ((u16)tilt >= 0x2AAB) {
            self->field_0x0AC = 0x2AAB;
        }
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
            pl_act_gauge_gate(self, -4);
        }
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 0) {
                pl_act_enter(self, 1, 9, 0x80);
            } else if (pl_act_param_tier_ck(self, 0) >= 1) {
                if (pl_part_flag_ck(self, 0x25) == 1) {
                    pl_act_enter(self, 1, 9, 0x80);
                } else {
                    pl_act_enter(self, 1, 7, 0x80);
                }
            } else {
                pl_act_enter(self, 1, 8, 0);
            }
        }
        break;
    }
}

/* The dash-turn act: clears the act latches, plays the turn and re-enters the run when it ends. */
extern "C" void pl_act_dash_turn(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0x40);
        pl_act_clear_flag5bb(self);
        pl_act_clear_mode5c4(self);
        self->field_0x310 = 0;
        Pl_chr_set_attr_default(self, 0x6B, 4, 0);
        self->field_0x0A8 = self->rot_0x54.y;
        if (self->field_0x18 == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_dash, 0);
        } else {
            fn_802771A0(self, 0);
        }
        self->field_0x313 = 30;
        pl_camera_lock_clear(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 1, 7, 0);
        }
        break;
    }
}

/* The dash-stop act: levels the tilt and slides the actor eight units a frame until the stop motion ends. */
extern "C" void pl_act_dash_stop(_PLW* self) {
    VEC3 slide;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0x40);
        self->field_0x310 = 0;
        Pl_chr_set_attr_default(self, 0x6C, 4, 0);
        self->rot_0x54.x = 0;
        self->field_0x0AC = 0;
        pl_camera_lock_clear(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_frame_window_8;
            rotVecY(&slide, self->rot_0x54.y);
            self->vec_0x03C.x += slide.x;
            self->vec_0x03C.z += slide.z;
        }
        break;
    }
}

/* Advances the weapon act armed with attribute 112: arm motion 3 with flag 64 and copy the second counter into
 * the first; then enter act 8 out of the 56-frame window at tier 0, else act 7 once the motion finishes. */
extern "C" void pl_act_step_attr_112(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 64);
        self->field_0x058 = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 112, 0, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1 &&
                pl_act_param_tier_ck(self, 0) == 0) {
                pl_act_enter(self, 1, 8, 0);
            } else if (Pl_motion_end_ck(self) == 1) {
                pl_act_enter(self, 1, 7, 0);
            }
        } else if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1) {
            pl_act_enter(self, 1, 8, 0);
        }
        break;
    default:
        break;
    }
}

/* The run act: runs on from the walk (timed to its frame), faster or slower with skills 0xC5/0xC6; the master steps
 * the guard gauge while the run input holds, falls back to the tired run or re-enters the walk when nothing watches,
 * and slows to the walk or the stop when the input ends. */
extern "C" void pl_act_run(_PLW* self) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0xC0);
        pl_act_arm_flags(self, 1);
        self->field_0x266 = 10;
        if (Get_motion_no(self) == 0x77) {
            if ((self->field_0x0B6 & 1) != 0) {
                Pl_chr_set_attr(self, 0x73, 4, (s32)pl_rig_get_motion_frame(self), 1);
            } else {
                Pl_chr_set_attr(self, 0x73, 4, (s32)(pl_frame_window_2 * pl_rig_get_motion_frame(self)), 1);
            }
        } else {
            Pl_chr_setX(self, 0x73, 4, 0);
        }
        self->field_0x354 = pl_float_1_6;
        if (Pl_Skill_ck(self, 0xC5) == 1) {
            self->field_0x354 *= pl_float_1_1;
        } else if (Pl_Skill_ck(self, 0xC6) == 1) {
            self->field_0x354 *= pl_float_0_9;
        }
        Pl_act_set_step_table(self, (u32)pl_steps_walk, self->field_0x266);
        pl_act_gauge_gate_by_skill(self);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            pl_act_gauge_gate_by_skill(self);
            if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, 0x25) == 1) {
                pl_camera_heading_update(self);
                pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
                if (pl_stagger_budget_ck(self, 150) == 0) {
                    pl_act_enter(self, 1, 0x24, 1);
                } else if (em_player_watched_ck(self->chunk_ofs) == 0) {
                    pl_act_enter(self, 1, 9, 1);
                }
            } else if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_enter(self, 1, 7, 0);
            } else {
                pl_act_enter(self, 1, 8, 0);
            }
        }
        break;
    }
}

/* The run-turn act: turns while running; the master keeps running (after a 10-frame turn), steps the guard gauge
 * on a held tier, or stops into the run slot when the timer runs out. */
extern "C" void pl_act_run_turn(_PLW* self) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 10;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0x40);
        self->field_0x266 = 10;
        Pl_chr_set_attr_default(self, 0x6A, 6, 0);
        Pl_act_set_step_table(self, (u32)lbl_805BE824, 0);
        self->field_0x314 = self->field_0x266;
        break;
    case 1:
        if (Pl_master_ck(self) == 0) {
            break;
        }
        if (self->field_0x006 != 0) {
            self->field_0x006--;
        }
        if (pl_act_param_tier_ck(self, 0) >= 2) {
            if (self->field_0x006 == 0) {
                pl_act_enter(self, 1, 7, 0);
            } else {
                pl_act_enter(self, 1, 6, 0);
            }
            break;
        }
        if (pl_act_param_tier_ck(self, 0) >= 1) {
            pl_camera_heading_update(self);
            pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
        } else if (self->field_0x266 <= 0) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
            break;
        }
        step.x = pl_float_zero;
        step.y = pl_float_zero;
        step.z = pl_frame_window_3;
        rotVecXYZ(&step, &self->rot_0x54);
        self->vec_0x03C.x += step.x;
        self->vec_0x03C.y += step.y;
        self->vec_0x03C.z += step.z;
        break;
    }
}

/* The hidden walk act: walks crouched; the master falls back to the tired walk when the stamina runs out, drops off
 * a ledge ahead, or crouches again when the input ends. */
extern "C" void pl_act_hidden_walk(_PLW* self) {
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x80);
        self->field_0x266 = 10;
        Pl_act_set_step_table(self, (u32)pl_steps_crouch, 0);
        pl_act_arm_flags(self, 1);
        Pl_chr_setX(self, 0x19D, 4, 0);
        self->field_0x354 = pl_float_1_25;
        break;
    case 1:
        pl_act_gauge_gate(self, -1);
        if (Pl_master_ck(self) == 1) {
            if (pl_stagger_budget_ck(self, 150) == 0) {
                pl_act_enter(self, 1, 0x27, 0);
            } else if (pl_act_param_tier_ck(self, 0) >= 2) {
                pl_act_set_cam_ang(self);
                if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) == 1 && self->field_0x585 == 0) {
                    self->field_0x0A8 = self->rot_0x54.y;
                    pl_act_enter(self, 2, 0, 0xC);
                }
            } else if (self->field_0x266 <= 0) {
                pl_act_enter(self, 1, 0x19, 0);
            }
        }
        break;
    }
}

/* The tired-run act: runs on for 20 frames, stepping the guard gauge on the run input; the master collapses when
 * the stamina is gone or slows to the walk. */
extern "C" void pl_act_tired_run(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 0xC0);
        Pl_chr_setX(self, 0x78, 4, 0);
        pl_act_arm_flags(self, 1);
        self->field_0x266 = 10;
        Pl_act_set_step_table(self, (u32)pl_steps_walk, 0);
        self->field_0x28 = 20;
        self->field_0x646 = 0;
        self->field_0x648 = 0;
        break;
    case 1:
        pl_act_gauge_gate(self, -1);
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, 0x25) == 1) {
                pl_camera_heading_update(self);
                pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
            } else if (self->field_0x28 <= 0) {
                if ((s16)self->field_0x378 <= 0) {
                    pl_act_enter(self, 0, 0x96, 0);
                } else {
                    pl_act_enter(self, 1, 8, 0);
                }
            }
        }
        break;
    }
}

/* The move-act dispatcher: runs the step handler of the actor's move act number; an unknown act falls back
 * to the idle slot. */
extern "C" void pl_act_step_arm_hold(_PLW* self) {
    switch (self->act_no) {
    case 0:
        pl_act_guard(self);
        break;
    case 1:
        pl_act_crouch(self, 0);
        break;
    case 2:
        pl_act_sneak(self, 0);
        break;
    case 3:
        pl_act_turn(self, 0);
        break;
    case 4:
        pl_act_quick_turn(self);
        break;
    case 5:
        pl_act_crouch(self, 1);
        break;
    case 6:
        pl_act_dash(self, 0);
        break;
    case 7:
        pl_act_dash(self, 1);
        break;
    case 8:
        pl_act_step_arm_table(self);
        break;
    case 9:
        pl_act_walk(self, 0);
        break;
    case 10:
        pl_act_stop(self, 0);
        break;
    case 11:
        pl_act_stop(self, 1);
        break;
    case 12:
        pl_act_step_timer_wait(self, 0);
        break;
    case 13:
        pl_act_look_around(self);
        break;
    case 14:
        pl_act_sneak(self, 1);
        break;
    case 15:
        pl_act_tired_walk(self, 0);
        break;
    case 16:
        pl_act_tiptoe(self, 0);
        break;
    case 17:
        pl_act_tiptoe(self, 1);
        break;
    case 18:
        pl_act_dash_start(self, 0);
        break;
    case 20:
        pl_act_dash_turn(self);
        break;
    case 21:
        pl_act_dash_stop(self);
        break;
    case 23:
        pl_act_step_attr_112(self);
        break;
    case 24:
        pl_act_crouch(self, 2);
        break;
    case 25:
        pl_act_turn(self, 1);
        break;
    case 27:
        pl_act_run(self);
        break;
    case 28:
        pl_act_stop(self, 2);
        break;
    case 29:
        pl_act_run_turn(self);
        break;
    case 35:
        pl_act_hidden_walk(self);
        break;
    case 36:
        pl_act_tired_run(self);
        break;
    case 37:
        pl_act_stop(self, 3);
        break;
    case 38:
        pl_act_step_timer_wait(self, 1);
        break;
    case 39:
        pl_act_tired_walk(self, 1);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The jump act (`variant` 1 the drop from a ledge): launches the actor on the jump's velocity and gravity, grabs a
 * ledge ahead on the way down, and on landing rides down when dead, lands hard after a long fall, or rolls into the
 * next act. */
extern "C" void pl_act_jump(_PLW* self, s32 variant) {
    VEC3 move;
    f32 ledge;
    u8 hit;

    VEC3_ctor(&move);
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 2, 0, 0);
        pl_camera_lock_clear(self);
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0xF, 0, 0);
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x7C = pl_frame_window_18;
            self->motion_vel_0x80 = pl_frame_window_20;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x8C = pl_float_zero;
            pl_jump_gravity_set(self, 0x13);
            break;
        case 1:
            self->field_0x585 = 0;
            Pl_chr_set_attr_default(self, 0xF, 8, 0x20);
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x7C = pl_float_neg20;
            self->motion_vel_0x80 = pl_frame_window_8;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x88 = pl_float_neg0_72727;
            self->motion_acc_0x8C = pl_float_zero;
            break;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        self->field_0x007 = 0;
        break;
    case 1:
        self->motion_vel_0x78 += self->motion_acc_0x84;
        self->motion_vel_0x7C += self->motion_acc_0x88;
        self->motion_vel_0x80 += self->motion_acc_0x8C;
        if (self->motion_vel_0x78 < pl_float_zero) {
            self->motion_vel_0x78 = pl_float_zero;
        }
        if (self->motion_vel_0x80 < pl_float_zero) {
            self->motion_vel_0x80 = pl_float_zero;
        }
        move.x = self->motion_vel_0x78;
        move.y = self->motion_vel_0x7C;
        move.z = self->motion_vel_0x80;
        rotVecY(&move, self->rot_0x54.y);
        self->vec_0x03C.x += move.x;
        self->vec_0x03C.y += move.y;
        self->vec_0x03C.z += move.z;
        if (pl_zone_ground_drop(self) == 1 || self->motion_vel_0x7C >= pl_float_zero) {
            break;
        }
        if (pl_ground_probe_ahead(self, (VEC3*)&ledge, pl_frame_window_180, pl_frame_window_30) != 0 &&
            pl_hold_gauge_ck(self, 1) == 1 && self->field_0x5C7 <= 20) {
            self->vec_0x03C.y = ledge;
            self->ground_y_0x060 = ledge;
            self->field_0x01B = 1;
            self->field_0x0A8 = self->rot_0x54.y;
            pl_act_enter(self, 0, 6, 0);
            break;
        }
        ledge = GetGroundHit2(&self->vec_0x03C, -6, self->area_0x16, &hit);
        if (ledge >= self->vec_0x03C.y && hit != 0) {
            pl_act_kind_set(self, 0);
            self->vec_0x03C.y = ledge;
            self->field_0x585 = 0;
            if ((s16)self->health <= 0 && Pl_master_ck(self) == 1) {
                pl_ride_down_set(self);
            } else if (self->field_0x28 >= 30 && Pl_cat_skill_ck(self, 0x24) == 0) {
                pl_act_enter(self, 2, 2, 0);
            } else if (Pl_master_ck(self) == 1) {
                if (pl_item_menu_open_ck(self) == 0 && self->field_0x18 == 0 && pl_part_flag_ck(self, 0) == 1 &&
                    pl_act_param_tier_ck(self, 0) >= 1) {
                    self->act_step_0x05++;
                    Pl_chr_set_attr_default(self, 0x10, 0, 0);
                } else {
                    pl_act_enter(self, 2, 3, 0);
                }
            } else {
                pl_act_enter(self, 2, 3, 0);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 1, 0xE, 0);
        }
        break;
    }
}

/* The landing act (`variant` 1 the soft landing): plays the landing, raises the hold notice, then returns to the
 * slot the variant picks. */
extern "C" void pl_act_land(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x585 = 0;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x13, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x3E, 0, 0);
        }
        pl_act_hold_notify(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (variant == 0) {
                Pl_act_set_motion_slot(self, 0, 8, 0);
            } else {
                Pl_act_set_motion_slot(self, 0, 4, 0);
            }
        }
        break;
    }
}

/* The climb-down act for `variant` 0-2: crouches and hops down (or, for 1 and 2, drops off the ledge at once - 2
 * from the ledge's own position), falls under gravity, grabs a ledge ahead, and lands hard after a long fall. */
extern "C" void pl_act_climb_down(_PLW* self, s32 variant) {
    VEC3 back;
    f32 ground;
    u8 hit;
    f32 scale;
    f32 speed;

    VEC3_ctor(&back);
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        pl_motion_clear(self);
        switch (variant) {
        default:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xB, 2, 0);
            Pl_act_set_motion(self, 0, 0, 0);
            break;
        case 2:
            self->vec_0x03C.y -= pl_frame_window_195;
            back.x = pl_float_zero;
            back.y = pl_float_zero;
            back.z = pl_float_neg30;
            rotVecY(&back, self->rot_0x54.y);
            self->vec_0x03C.x += back.x;
            self->vec_0x03C.z += back.z;
            /* fallthrough */
        case 1:
            self->field_0x585 = 0;
            self->act_step_0x05 = 2;
            Pl_act_set_motion(self, 2, 0, 0);
            pl_motion_acc_clear(self);
            self->motion_vel_0x7C = pl_float_neg18;
            self->motion_acc_0x88 = pl_float_neg0_72727;
            if (variant == 1) {
                Pl_chr_set_attr_default(self, 0xC, 4, 0x16);
            } else {
                Pl_chr_set_attr_default(self, 0xC, 0, 0x16);
            }
            break;
        }
        pl_camera_lock_clear(self);
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_motion_acc_clear(self);
            if (self->field_0x18 == 1) {
                scale = pl_float_0_8;
            } else {
                scale = pl_frame_window_1;
            }
            speed = pl_frame_window_15 * scale;
            self->motion_vel_0x7C = pl_frame_window_2 * speed;
            pl_jump_gravity_set(self, 0x14);
            Pl_chr_set_attr_default(self, 0xC, 2, 0);
            pl_act_kind_set(self, 2);
            pl_act_set_frame_timer(self);
            self->field_0x28 = 0;
        }
        break;
    case 2:
        pl_motion_integrate(self);
        if (pl_zone_ground_drop(self) != 1 && self->motion_vel_0x7C < pl_float_zero) {
            if (variant != 2 && pl_ground_probe_ahead(self, (VEC3*)&ground, pl_frame_window_180, pl_frame_window_30) != 0 &&
                pl_hold_gauge_ck(self, 1) == 1) {
                self->field_0x0A8 = self->rot_0x54.y;
                self->vec_0x03C.y = ground;
                self->ground_y_0x060 = ground;
                self->field_0x01B = 1;
                pl_act_enter(self, 0, 6, 0);
                break;
            }
            ground = GetGroundHit2(&self->vec_0x03C, -6, self->area_0x16, &hit);
            if (ground >= self->vec_0x03C.y && hit != 0) {
                self->vec_0x03C.y = ground;
                pl_act_kind_set(self, 0);
                if ((s16)self->health <= 0 && Pl_master_ck(self) == 1) {
                    pl_ride_down_set(self);
                } else if (self->field_0x28 >= 30 && Pl_cat_skill_ck(self, 0x24) == 0) {
                    pl_act_enter(self, 2, 2, 0);
                } else {
                    self->act_step_0x05++;
                    Pl_chr_set_attr_default(self, 0xD, 0, 0);
                }
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The fall-act dispatcher: runs the jump, landing and climb-down step handlers by the actor's act number;
 * an unknown act falls back to the idle slot. */
extern "C" void pl_act_group2_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0:
        pl_act_jump(self, 0);
        break;
    case 1:
        pl_act_jump(self, 1);
        break;
    case 2:
        pl_act_land(self, 0);
        break;
    case 3:
        pl_act_land(self, 1);
        break;
    case 4:
        pl_act_climb_down(self, 0);
        break;
    case 5:
        pl_act_climb_down(self, 1);
        break;
    case 6:
        pl_act_climb_down(self, 2);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* Arms one motion of the caller's part, sets the low or high act flag and the actor mode, and re-arms the act's
 * flag set unless told otherwise; shared by the weapon-act band above 0x80255388. */
extern "C" void pl_act_arm_motion_and_flag(_PLW* self, u16 motion, u8 high_flag, u8 skip_arm)
{
    Pl_act_set_motion(self, motion, 0, 0);
    if (high_flag == 0) {
        pl_act_set_flag(self, 16);
    } else {
        pl_act_set_flag(self, 80);
    }
    self->field_0x018 = 1;
    if (skip_arm == 0) {
        pl_act_arm_flags(self, 1);
    }
}

/* The weapon guard act (`variant` 1 the guard already raised): raises the guard of the weapon kind and its step
 * table; the master drops it once the guard has lasted 16 frames without the guard input. */
extern "C" void pl_act_weapon_guard(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            if (self->field_0x002 == 3) {
                Pl_chr_set_attr_default(self, 0x3ED, 2, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x3ED, 0, 0);
            }
        } else if (self->field_0x002 == 3) {
            Pl_chr_set_attr_default(self, 0x3EE, 2, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x3EE, 2, 0x28);
        }
        switch (self->field_0x002) {
        case 0:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_kind0, 10);
            break;
        case 1:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_kind1, 10);
            break;
        case 3:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_kind3, 10);
            break;
        }
        self->field_0x264 = 0;
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && (s16)self->field_0x264 > 16 && pl_part_flag_ck(self, 0x11) == 0) {
            pl_act_enter(self, 5, 2, 0);
        } else if (Get_motion_no(self) == 0x3ED && Pl_motion_end_ck(self) == 1) {
            Pl_chr_set_attr_default(self, 0x3EE, 2, 0);
        }
        break;
    }
}

/* Advances the weapon act armed with attribute 1007: arms it, hands the motion on through the shared part armer,
 * then hands motion slot 2 on once the motion finishes. */
extern "C" void pl_act_step_attr_1007(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1007, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* The weapon sheathe-attack act: plays the attack, then returns to the drawn slot (the alternate form) or the
 * guard act. */
extern "C" void pl_act_weapon_sheathe_attack(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x4BB, 0, 0);
        pl_act_set_frame_timer(self);
        pl_sub_gauge_level_cost(self);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (pl_item_page2_ck(self) == 1) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                pl_act_enter(self, 5, 1, 0);
            }
        }
        break;
    }
}

/* The weapon thrust act: lunges (the ranged kinds 3-6 swing back on a cosine) and steps the follow-up of the kind:
 * the guard chain at frame 42, the bow's double draw, the gunner kinds' recoil. */
extern "C" void pl_act_weapon_thrust(_PLW* self) {
    VEC3 lunge;

    VEC3_ctor(&lunge);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        pl_sub_gauge_level_cost(self);
        if ((u32)(self->field_0x002 - 3) <= 3) {
            Pl_chr_set_attr_default(self, 0x4BC, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x4BD, 0, 0);
        }
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        lunge.z = pl_float_zero;
        if (Pl_frame_check(self, 2, pl_frame_window_30, pl_float_zero) == 1) {
            lunge.x = pl_float_zero;
            lunge.y = pl_float_zero;
            if ((u32)(self->field_0x002 - 3) <= 3) {
                lunge.z = pl_float_neg15 *
                          cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) * pl_float_0_03125))) /
                               pl_frame_window_360);
            }
            rotVecY(&lunge, self->rot_0x54.y);
            self->vec_0x03C.x += lunge.x;
            self->vec_0x03C.z += lunge.z;
        }
        if ((u32)(self->field_0x002 - 4) > 2) {
            switch (self->field_0x002) {
            case 0:
                if (Pl_frame_check(self, 1, pl_frame_window_42, pl_float_zero) == 1) {
                    if (Pl_master_ck(self) == 1) {
                        if (pl_part_flag_ck(self, 0x11) == 1) {
                            pl_act_enter(self, 5, 1, 0);
                        } else {
                            pl_act_enter(self, 5, 2, 0);
                        }
                    } else {
                        pl_act_enter(self, 5, 1, 0);
                    }
                }
                break;
            default:
                if (Pl_motion_end_ck(self) == 1) {
                    Pl_act_set_motion_slot(self, 0, 4, 0);
                }
                break;
            case 3:
                if (Pl_motion_end_ck(self) == 1) {
                    pl_act_enter(self, 5, 1, 0);
                    break;
                }
                if (self->field_0x006 == 0 && Pl_frame_check(self, 0, pl_frame_window_34, pl_float_zero) == 1) {
                    self->field_0x006++;
                    Pl_chr_set_attr_default(self, 0x4BC, 8, 0x32);
                }
                if (self->field_0x007 == 0 && Pl_frame_check(self, 0, pl_frame_window_60, pl_float_zero) == 1) {
                    self->field_0x007++;
                    Pl_chr_set_attr_default(self, 0x4BC, 8, 0x54);
                }
                break;
            }
        } else if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0xA, 0);
        } else if (self->field_0x006 == 0 && Pl_frame_check(self, 0, pl_frame_window_46, pl_float_zero) == 1) {
            self->field_0x006++;
            Pl_chr_set_attr_default(self, 0x4BC, 0xA, 0x56);
        }
        break;
    }
}

/* The weapon swing act (`variant` 1 the upswing): steps the swing's forward lunge on the kind's cosine curve and,
 * when the motion ends, returns to the kind's slot or the guard act. */
extern "C" void pl_act_weapon_swing(_PLW* self, s32 variant) {
    VEC3 lunge;

    VEC3_ctor(&lunge);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_sub_gauge_level_cost(self);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x4BC, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x465, 0, 0);
        }
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        lunge.z = pl_float_zero;
        lunge.x = pl_float_zero;
        lunge.y = pl_float_zero;
        if ((u32)(self->field_0x002 - 3) > 3) {
            switch (self->field_0x002) {
            case 0:
                if (Pl_frame_check(self, 2, pl_frame_window_104, pl_float_zero) == 1) {
                    lunge.z = pl_float_neg22 *
                              cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_104))) /
                                   pl_frame_window_360);
                }
                break;
            case 1:
                if (Pl_frame_check(self, 2, pl_frame_window_94, pl_float_zero) == 1) {
                    lunge.z = pl_float_neg18 *
                              cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_94))) /
                                   pl_frame_window_360);
                }
                break;
            }
        } else if (Pl_frame_check(self, 2, pl_frame_window_90, pl_float_zero) == 1) {
            lunge.z = pl_float_neg10 *
                      cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_104))) /
                           pl_frame_window_360);
        }
        rotVecY(&lunge, self->rot_0x54.y);
        self->vec_0x03C.x += lunge.x;
        self->vec_0x03C.z += lunge.z;
        if (Pl_motion_end_ck(self) == 1) {
            switch (self->field_0x002) {
            case 3:
                pl_act_enter(self, 5, 1, 0);
                break;
            case 1:
                Pl_act_set_motion_slot(self, 0, 6, 0);
                break;
            default:
                Pl_act_set_motion_slot(self, 0, 0xA, 0);
                break;
            }
        }
        break;
    }
}

/* The guard-walk act for `variant` 0-2: walks with the guard raised (the kind's step table) and, for the master,
 * leaves into the walk once the guard has lasted 16 frames without the guard input. */
extern "C" void pl_act_guard_walk(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0x41E, 2, 0);
            break;
        case 1:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x41F, 2, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x42E, 2, 0);
            break;
        }
        switch (self->field_0x002) {
        case 0:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_walk_kind0, 10);
            break;
        case 1:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_walk_kind1, 10);
            break;
        case 3:
            Pl_act_set_step_table(self, (u32)pl_steps_guard_walk_kind3, 10);
            break;
        }
        pl_act_set_frame_timer(self);
        pl_act_arm_motion_and_flag(self, 3, 0, 0);
        self->field_0x264 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x41F, 2, 0);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && (s16)self->field_0x264 > 16 && pl_part_flag_ck(self, 0x11) == 0) {
            pl_act_enter(self, 5, 8, 0x80);
        }
        break;
    }
}

/* The same three steps for the weapon act whose arming step carries attribute 1056, which arms
 * motion 3 instead of motion 0. */
extern "C" void pl_act_step_attr_1056(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1056, 2, 0);
        pl_act_arm_motion_and_flag(self, 3, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* The weapon follow-up act for `variant` 0-3: plays the follow-up of the kind (the ranged kinds lunge on a cosine),
 * then switches to the slot the per-kind follow-up table gives or, for none, the guard-walk act. */
extern "C" void pl_act_weapon_follow_up(_PLW* self, s32 variant) {
    VEC3 lunge;
    s16 slot;

    VEC3_ctor(&lunge);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0x4ED, 0, 0);
            break;
        case 1:
            if ((u32)(self->field_0x002 - 3) <= 3) {
                Pl_chr_set_attr_default(self, 0x4EE, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x4EF, 0, 0);
            }
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x4EE, 0, 0);
            break;
        case 3:
            Pl_chr_set_attr_default(self, 0x4EE, 0, 0);
            break;
        }
        pl_sub_gauge_level_cost(self);
        pl_act_arm_motion_and_flag(self, 3, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            slot = pl_weapon_follow_up_slots[self->field_0x002 * 4 + variant];
            if (slot == -1) {
                pl_act_enter(self, 5, 7, 0x80);
            } else {
                Pl_act_set_motion_slot(self, 3, slot, 0);
            }
            break;
        }
        lunge.x = pl_float_zero;
        lunge.y = pl_float_zero;
        lunge.z = pl_float_zero;
        if ((u32)(self->field_0x002 - 3) <= 3 && Pl_frame_check(self, 2, pl_frame_window_90, pl_float_zero) == 1) {
            lunge.z = pl_float_neg10 *
                      cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_104))) /
                           pl_frame_window_360);
        }
        if ((u32)(variant - 1) <= 2) {
            if (variant == 1) {
                lunge.z *= pl_float_0_75;
            }
            rotVecXYZ(&lunge, &self->rot_0x54);
            self->vec_0x03C.x += lunge.x;
            self->vec_0x03C.z += lunge.z;
        }
        if (variant == 1) {
            if ((u32)(self->field_0x002 - 4) > 2) {
                if (self->field_0x002 == 3) {
                    if (self->field_0x006 == 0 && Pl_frame_check(self, 0, pl_frame_window_34, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 0x4EE, 8, 0x32);
                    }
                    if (self->field_0x007 == 0 && Pl_frame_check(self, 0, pl_frame_window_60, pl_float_zero) == 1) {
                        self->field_0x007++;
                        Pl_chr_set_attr_default(self, 0x4EE, 8, 0x54);
                    }
                }
            } else if (self->field_0x006 == 0 && Pl_frame_check(self, 0, pl_frame_window_84, pl_float_zero) == 1) {
                self->field_0x006++;
                Pl_chr_set_attr_default(self, 0x4EE, 4, 0x68);
            }
        }
        break;
    }
}

/* The weapon aim act: raises the weapon to aim (the bow kind's aim table) and, for the master, keeps aiming while
 * the tier holds, or returns to the guard act. */
extern "C" void pl_act_weapon_aim(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x3F3, 4, 0);
        pl_act_arm_motion_and_flag(self, 0, 1, 1);
        self->field_0x266 = 10;
        if (self->field_0x002 == 3) {
            Pl_act_set_step_table(self, (u32)pl_steps_aim_kind3, 0);
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                if (pl_part_flag_ck(self, 0x11) == 0) {
                    pl_act_enter(self, 5, 1, 0);
                }
            } else if (self->field_0x266 <= 0) {
                pl_act_enter(self, 5, 1, 0);
            }
        }
        break;
    }
}

/* And for the weapon act whose arming step carries attribute 1018: the follow-up does not arm the
 * act's own motion but enters the child act 5 instead. */
extern "C" void pl_act_step_attr_1018(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1018, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 5, 1, 0);
        }
        break;
    default:
        break;
    }
}

/* The weapon aim-walk act: walks aiming (the bow kind's table), stepping the guard gauge while the tier holds,
 * and returns to the guard-walk act. */
extern "C" void pl_act_weapon_aim_walk(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x423, 4, 0);
        pl_act_arm_motion_and_flag(self, 3, 1, 1);
        self->field_0x266 = 10;
        if (self->field_0x002 == 3) {
            Pl_act_set_step_table(self, (u32)pl_steps_aim_walk_kind3, 0);
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_camera_heading_update(self);
                pl_act_guard_gauge_adjust(self, 0xC16D, 0x3E94, 0x2B, 0x2C);
                if (pl_part_flag_ck(self, 0x11) == 0) {
                    pl_act_enter(self, 5, 7, 0x80);
                }
            } else if (self->field_0x266 <= 0) {
                pl_act_enter(self, 5, 7, 0x80);
            }
        }
        break;
    }
}

/* The weapon-act dispatcher: clears the latch byte for a drawn melee weapon and runs the weapon act's step
 * handler; an unknown act falls back to the idle slot. */
extern "C" void pl_act_group5_dispatch(_PLW* self) {
    if (self->kind_0x09 != 3 && self->field_0x002 != 1) {
        pl_act_clear_flag5bb(self);
    }
    switch (self->act_no) {
    case 0:
        pl_act_weapon_guard(self, 0);
        break;
    case 1:
        pl_act_weapon_guard(self, 1);
        break;
    case 2:
        pl_act_step_attr_1007(self);
        break;
    case 3:
        pl_act_weapon_sheathe_attack(self);
        break;
    case 4:
        pl_act_weapon_thrust(self);
        break;
    case 5:
        pl_act_weapon_swing(self, 0);
        break;
    case 6:
        pl_act_guard_walk(self, 0);
        break;
    case 7:
        pl_act_guard_walk(self, 1);
        break;
    case 8:
        pl_act_step_attr_1056(self);
        break;
    case 9:
        pl_act_weapon_follow_up(self, 0);
        break;
    case 10:
        pl_act_weapon_follow_up(self, 1);
        break;
    case 11:
        pl_act_weapon_follow_up(self, 2);
        break;
    case 12:
        pl_act_guard_walk(self, 2);
        break;
    case 13:
        pl_act_weapon_aim(self);
        break;
    case 14:
        pl_act_step_attr_1018(self);
        break;
    case 15:
        pl_act_weapon_aim_walk(self);
        break;
    case 16:
        pl_act_weapon_swing(self, 1);
        break;
    case 17:
        pl_act_weapon_follow_up(self, 3);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The side-step act (`right` picks the direction): steps the actor four units a frame sideways for the motion,
 * marking the landing frame, then re-enters the idle slot. */
extern "C" void pl_act_side_step(_PLW* self, s32 right) {
    f32 side;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 1);
        if (self->field_0x18 == 1) {
            if (self->field_0x002 == 8 && pl_mode8_free_ck() == 0) {
                Pl_chr_set_attr_default(self, 0x4B2, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x4B1, 0, 0);
            }
        } else {
            Pl_chr_set_attr_default(self, 0xC9, 0, 0);
        }
        self->motion_vel_0x78 = pl_float_zero;
        self->motion_vel_0x7C = pl_float_zero;
        if (right != 0) {
            side = pl_frame_window_4;
        } else {
            side = pl_float_neg4;
        }
        self->motion_vel_0x80 = side;
        rotVecY((VEC3*)&self->motion_vel_0x78, self->rot_0x54.y);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
        } else if (Pl_frame_check(self, 2, pl_frame_window_16, pl_float_zero) == 1) {
            pl_motion_integrate_stop(self);
        }
        break;
    }
}

/* Arms the hit-reaction step table for the actor's state: sheathed, the bow kind, or another drawn kind (`heavy`
 * picks the heavy reaction). */
extern "C" void pl_hit_reaction_steps_arm(_PLW* self, u8 heavy) {
    if (self->field_0x18 == 0) {
        if (heavy == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_hit_light, 0);
        } else {
            Pl_act_set_step_table(self, (u32)pl_steps_hit_heavy, 0);
        }
    } else if (self->field_0x002 == 3) {
        if (heavy == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_hit_light_bow, 0);
        } else {
            Pl_act_set_step_table(self, (u32)pl_steps_hit_heavy_bow, 0);
        }
    } else if (heavy == 0) {
        Pl_act_set_step_table(self, (u32)pl_steps_hit_light_drawn, 0);
    } else {
        Pl_act_set_step_table(self, (u32)pl_steps_hit_heavy_drawn, 0);
    }
}

/* Arms the knock-down step table for the actor's state, as the hit reaction's twin. */
extern "C" void pl_knockdown_steps_arm(_PLW* self, u8 heavy) {
    if (self->field_0x18 == 0) {
        if (heavy == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_knockdown, 0);
        } else {
            Pl_act_set_step_table(self, (u32)pl_steps_knockdown_heavy, 0);
        }
    } else if (self->field_0x002 == 3) {
        if (heavy == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_knockdown_bow, 0);
        } else {
            Pl_act_set_step_table(self, (u32)pl_steps_knockdown_heavy_bow, 0);
        }
    } else if (heavy == 0) {
        Pl_act_set_step_table(self, (u32)pl_steps_knockdown_drawn, 0);
    } else {
        Pl_act_set_step_table(self, (u32)pl_steps_knockdown_heavy_drawn, 0);
    }
}

/* The light hit act (`variant` 1 from behind): plays the flinch and its recovery with the hit-reaction table,
 * then re-enters the idle slot and re-arms the gauge. */
extern "C" void pl_act_hit_light(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 1);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0xCA, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0xCC, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (variant == 0) {
                Pl_chr_set_attr_default(self, 0xD8, 6, 0x22);
            } else {
                Pl_chr_set_attr_default(self, 0xCF, 6, 0x3A);
            }
            pl_hit_reaction_steps_arm(self, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* The knock-back flight act for `variant` 0-3: throws the actor back on the variant's arc, lands it (or tumbles it
 * down a slope), then gets it up - or, at no health, carts the master. */
extern "C" void pl_act_knockback_flight(_PLW* self, s32 variant) {
    VEC3 fly;
    f32 ground;
    s32 hit;
    u32 done;

    VEC3_ctor(&fly);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 2, 0, 1);
        fn_8027D4F0(self);
        self->field_0x585 = 0;
        pl_get_joint_wpos(self, 0xE, &fly);
        if (self->vec_0x03C.y < fly.y) {
            self->vec_0x03C.y = fly.y;
        }
        pl_motion_acc_clear(self);
        fly.x = pl_float_zero;
        fly.y = pl_float_zero;
        Pl_chr_set_attr_default(self, pl_knockback_flight_attr[variant * 2], 0, pl_knockback_flight_attr[variant * 2 + 1]);
        switch (variant) {
        case 0:
            fly.z = pl_float_neg22_34;
            self->motion_acc_0x88 = pl_float_neg0_33333;
            break;
        case 2:
            fly.z = pl_float_neg19_04;
            self->motion_acc_0x88 = pl_float_neg0_36;
            break;
        case 1:
        case 3:
            fly.z = pl_frame_window_23;
            self->motion_acc_0x88 = pl_float_neg0_25;
            break;
        }
        rotVecY(&fly, self->rot_0x54.y);
        self->motion_vel_0x78 = fly.x;
        self->motion_vel_0x7C = pl_float_zero;
        self->motion_vel_0x80 = fly.z;
        self->field_0x28 = 16;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Get_motion_no(self) == 0xCD) {
                Pl_chr_set_attr_default(self, 0xD6, 0, 0);
            }
            if (Get_motion_no(self) == 0xD0) {
                Pl_chr_set_attr_default(self, 0xDC, 0, 0);
            }
        }
        pl_motion_integrate(self);
        if (pl_zone_ground_drop(self) == 1) {
            break;
        }
        fn_8025FF0C(self, 0);
        hit = pl_ground_height_probe(self, &ground);
        if (self->field_0x28 > 0) {
            self->field_0x28--;
            if (self->vec_0x03C.y <= pl_frame_window_63 + ground && hit != 0) {
                self->vec_0x03C.y = pl_frame_window_63 + ground;
            }
            break;
        }
        switch (hit) {
        case 1:
            if (self->motion_vel_0x7C <= pl_float_zero && self->vec_0x03C.y <= pl_frame_window_63 + ground) {
                self->act_step_0x05++;
                self->vec_0x03C.y = ground;
                self->field_0x01B = 1;
                Pl_chr_set_attr_default(self, pl_knockback_land_attr[variant], 0, 0);
                pl_act_kind_set(self, 4);
            }
            break;
        case 2:
            if (self->motion_vel_0x7C <= pl_float_zero && self->vec_0x03C.y <= ground) {
                self->vec_0x03C.y = ground;
                self->field_0x01B = 1;
                self->kind_0x09 = 3;
                if (Pl_master_ck(self) == 1 && (s16)self->health <= 0) {
                    pl_ride_down_set(self);
                } else {
                    pl_act_enter(self, 0, 0x97, 0xC);
                }
            }
            break;
        }
        break;
    case 2:
        done = 0;
        if (variant == 2) {
            if (Pl_frame_check(self, 1, pl_frame_window_58, pl_float_zero) == 1) {
                done = 1;
            }
        } else if (Pl_motion_end_ck(self) == 1) {
            done = 1;
        }
        if (done == 1) {
            if ((s16)self->health > 0) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, pl_knockback_getup_attr[variant * 2], pl_knockback_getup_attr[variant * 2 + 1], 0);
                pl_knockdown_steps_arm(self, 0);
            } else if (Pl_master_ck(self) == 1) {
                if (variant == 0) {
                    pl_dodge_motion_start(self, 2, 0);
                } else {
                    pl_dodge_motion_start(self, 3, 0);
                }
            } else {
                self->act_step_0x05 = 4;
                pl_model_set_state(self, 2);
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 6, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    case 4:
        pl_model_set_state(self, 2);
        break;
    }
}

/* The blown-away act (`variant` 1 forward): launches the actor up and back, lands it (or tumbles it down a slope),
 * turns it around on the get-up and re-enters the idle slot. */
extern "C" void pl_act_blown_away(_PLW* self, s32 variant) {
    VEC3 fly;
    f32 ground;
    f32 back;
    u16 motion;

    VEC3_ctor(&fly);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0xD5, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0xDB, 0, 0);
        }
        pl_get_joint_wpos(self, 0xE, &fly);
        if (self->vec_0x03C.y < fly.y) {
            self->vec_0x03C.y = fly.y;
        }
        Pl_act_set_motion(self, 2, 0, 1);
        fn_8027D4F0(self);
        self->field_0x585 = 0;
        pl_motion_acc_clear(self);
        fly.x = pl_float_zero;
        fly.y = pl_float_zero;
        if (variant != 0) {
            back = pl_frame_window_7;
        } else {
            back = pl_float_neg7;
        }
        fly.z = back;
        rotVecY(&fly, self->rot_0x54.y);
        self->motion_vel_0x78 = pl_frame_window_2 * fly.x;
        self->motion_vel_0x80 = pl_frame_window_2 * fly.z;
        self->motion_vel_0x7C = pl_frame_window_20;
        self->motion_acc_0x88 = pl_float_neg0_72727;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1 && ((motion = Get_motion_no(self)) == 0xD5 || motion == 0xDB)) {
            if (variant == 0) {
                Pl_chr_set_attr_default(self, 0xD6, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 0xDC, 0, 0);
            }
        }
        pl_motion_integrate(self);
        if (pl_zone_ground_drop(self) == 1) {
            break;
        }
        fn_8025FF0C(self, 0);
        switch (pl_ground_height_probe(self, &ground)) {
        case 1:
            if (self->motion_vel_0x7C <= pl_float_zero && self->vec_0x03C.y <= pl_frame_window_63 + ground) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0xD7, 0, 0);
                self->vec_0x03C.y = ground;
                self->field_0x01B = 1;
                pl_act_kind_set(self, 4);
            }
            break;
        case 2:
            if (self->motion_vel_0x7C <= pl_float_zero && self->vec_0x03C.y <= ground) {
                self->vec_0x03C.y = ground;
                self->field_0x01B = 1;
                self->kind_0x09 = 3;
                if (Pl_master_ck(self) == 1 && (s16)self->health <= 0) {
                    pl_ride_down_set(self);
                } else {
                    pl_act_enter(self, 0, 0x97, 0xC);
                }
            }
            break;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            motion = self->rot_0x54.y + 0x8000;
            self->rot_0x54.y = motion;
            self->field_0x0A8 = motion;
            Pl_chr_set_attr_default(self, 0xD8, 0, 0);
            pl_knockdown_steps_arm(self, 0);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 8, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* The heavy flinch act (`variant` 1 from behind): plays the flinch with the heavy hit-reaction table, then
 * re-enters the kind's slot and re-arms the gauge. */
extern "C" void pl_act_flinch_heavy(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 1);
        fn_8027D4F0(self);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0xFB, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0xFC, 0, 0);
        }
        pl_hit_reaction_steps_arm(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 6, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* The small flinch act: plays the sheathed (or drawn) flinch, then re-enters the kind's slot. */
extern "C" void pl_act_flinch_small(_PLW* self, s32 unused) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 1);
        fn_8027D4F0(self);
        if (self->field_0x18 == 0) {
            Pl_chr_set_attr_default(self, 0xFA, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x4E3, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 6, 0);
        }
        break;
    }
}

/* The stagger act (`variant` 1 from behind): staggers, recovers with the heavy knock-down table, then re-enters the
 * kind's slot and re-arms the gauge. */
extern "C" void pl_act_stagger(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 1);
        fn_8027D4F0(self);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0xFD, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0xFF, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x102, 4, 0);
            pl_knockdown_steps_arm(self, 1);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 6, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* Runs one repeat of the charge act the part selects: arm motion 0/3 and attribute 211/263 with the part's repeat
 * count at `+0x007`, wait that many motion-end reports, re-enter the act, then hand its motion slot on. */
extern "C" void pl_act_charge_repeat_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        switch (part) {
        case 0:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 1:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 2:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 3:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 4:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 5:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        default:
            break;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (++self->field_0x006 >= self->field_0x007) {
                self->act_step_0x05++;
                if ((u32)part <= 2) {
                    Pl_chr_set_attr_default(self, 212, 2, 0);
                } else {
                    Pl_chr_set_attr_default(self, 264, 2, 0);
                }
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, self->kind_0x09, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* The mud-stuck act (`variant` 1 the deep mud): wades back on a cosine with the mud splash every eight frames, and
 * frees the actor at frame 28 (variant 0) or after two struggle frames, re-entering the guard slot. */
extern "C" void pl_act_mud_stuck(_PLW* self, s32 variant) {
    VEC3 wade;
    VEC3 unused;
    u8 count;

    VEC3_ctor(&wade);
    VEC3_ctor(&unused);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_chr_set_attr_default(self, 0xDA, 4, 0);
        Pl_act_set_motion(self, 0, 0, 1);
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_28, pl_float_zero) == 1 && variant == 0) {
            self->act_step_0x05 = 3;
            Pl_chr_set_attr_default(self, 0xE2, 6, 0x14);
            break;
        }
        if (Pl_frame_check(self, 2, pl_frame_window_40, pl_float_zero) == 1) {
            wade.x = pl_float_zero;
            wade.y = pl_float_zero;
            wade.z = pl_float_neg18 *
                     cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_42))) /
                          pl_frame_window_360);
            rotVecXYZ(&wade, &self->rot_0x54);
            self->vec_0x03C.x += wade.x;
            self->vec_0x03C.z += wade.z;
            if ((self->field_0x264 & 7) == 0) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 3, 2, pl_float_1_4, pl_float_zero, pl_float_zero, 0x8000);
                } else {
                    eft004_set_pl2(self, 0xA, 0x11, pl_float_1_5, pl_float_zero, pl_float_zero, 0x8000);
                    eft004_set_pl2(self, 0xA, 0x14, pl_float_1_5, pl_float_zero, pl_float_zero, 0x8000);
                }
            }
            break;
        }
        self->act_step_0x05++;
        break;
    case 2:
        if (Pl_frame_check(self, 1, pl_frame_window_44, pl_float_zero) == 1) {
            count = ++self->field_0x006;
            if (variant == 0 || (variant == 1 && count >= 2)) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0xE2, 2, 0);
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 0xC, 0);
        }
        break;
    }
}

/* The mud-stand act: struggles up with the mud splash every eight frames, then re-enters the idle slot. */
extern "C" void pl_act_mud_stand(_PLW* self) {
    VEC3 unused;

    VEC3_ctor(&unused);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_chr_set_attr_default(self, 0xE3, 0, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
            break;
        }
        if (Pl_frame_check(self, 2, pl_frame_window_40, pl_float_zero) == 1 && (self->field_0x264 & 7) == 0) {
            if ((self->field_0x5A4 & 6) != 0) {
                eft004_set_pl2(self, 2, 2, pl_frame_window_1, pl_float_zero, pl_float_zero, 0x8000);
            } else {
                eft004_set_pl2(self, 0xA, 0x11, pl_float_1_2, pl_float_zero, pl_float_zero, 0x8000);
                eft004_set_pl2(self, 0xA, 0x14, pl_float_1_2, pl_float_zero, pl_float_zero, 0x8000);
            }
        }
        if (Pl_frame_check(self, 2, pl_frame_window_64, pl_float_zero) == 1) {
            pl_act_set_gauge_arm(self, 2);
        }
        break;
    }
}

/* The deep-mud walk act (`variant` 1 the long struggle): wades back on a cosine and frees the actor at frame 44
 * (variant 0) or after two struggle frames, re-entering the guard slot. */
extern "C" void pl_act_mud_walk(_PLW* self, s32 variant) {
    VEC3 wade;
    u8 count;

    VEC3_ctor(&wade);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_chr_set_attr_default(self, 0x109, 4, 0);
        Pl_act_set_motion(self, 3, 0, 1);
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_44, pl_float_zero) == 1 && variant == 0) {
            self->act_step_0x05 = 3;
            Pl_chr_set_attr(self, 0x10A, 4, 0, 1);
            break;
        }
        if (Pl_frame_check(self, 2, pl_frame_window_40, pl_float_zero) == 1) {
            wade.x = pl_float_zero;
            wade.y = pl_float_zero;
            wade.z = pl_float_neg18 *
                     cosf((pl_float_6_28319 * (pl_frame_window_90 * (pl_rig_get_motion_frame(self) / pl_frame_window_42))) /
                          pl_frame_window_360);
            rotVecXYZ(&wade, &self->rot_0x54);
            self->vec_0x03C.x += wade.x;
            self->vec_0x03C.z += wade.z;
        } else if (variant == 1) {
            self->act_step_0x05++;
        }
        break;
    case 2:
        if (Pl_frame_check(self, 1, pl_frame_window_88, pl_float_zero) == 1) {
            count = ++self->field_0x006;
            if (variant == 0 || (variant == 1 && count >= 2)) {
                self->act_step_0x05++;
                Pl_chr_set_attr(self, 0x10A, 4, 0, 1);
            }
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 0xC, 0);
        }
        break;
    }
}

/* Arms a countdown motion: clears the actor mode, arms attribute 26 with its frame timer and a 40-frame (first
 * part) or 80-frame countdown; the follow-up hands motion slot 12 on when it runs out. */
extern "C" void pl_act_countdown_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 26, 6, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        if (part == 0) {
            self->field_0x28 = 40;
        } else {
            self->field_0x28 = 80;
        }
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            Pl_act_set_motion_slot(self, 0, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* Resets the three guard timers the act cluster counts down: the two running timers to zero and the
 * third to its 30-frame window. */
extern "C" void pl_act_guard_timer_reset(_PLW* self)
{
    self->field_0x400 = 0;
    self->field_0x3FC = 0;
    self->field_0x402 = 30;
}

/* The stun act (`variant` 1 the drawn stun): arms the stun motion and its 300-frame windows; each window the input
 * shake count of the stun table (doubled with skill 9) shortens it, and the actor recovers after five. */
extern "C" void pl_act_stun(_PLW* self, s32 variant) {
    u8 need;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        pl_act_set_motion_gate(self, 0);
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 0x1A, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 0x106, 4, 0);
        }
        pl_act_set_frame_timer(self);
        pl_act_clear_flag5bb(self);
        self->field_0x28 = 300;
        self->field_0x006 = 0;
        self->field_0x007 = 5;
        self->field_0x400 = 0;
        self->field_0x3FC = 0;
        pl_act_clear_wait(self, 0);
        self->field_0x0A8 = (u16)self->rot_0x54.y;
        eft_pl_hit_light_spawn(self);
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            self->field_0x006 = 0;
            self->field_0x007--;
            self->field_0x28 = 300;
        } else if (pl_input_active_ck(self) == 1) {
            self->field_0x006++;
            need = pl_stun_shake_needs[self->field_0x007];
            if (Pl_Skill_ck(self, 9) == 1) {
                need = (s8)need * 2;
            }
            if (self->field_0x006 >= (s8)need) {
                self->field_0x006 = 0;
                self->field_0x007--;
                self->field_0x28 = 300;
            }
        }
        if (self->field_0x007 == 0) {
            pl_act_guard_timer_reset(self);
            self->field_0x315 = 20;
            pl_act_reenter(self, self->kind_0x09, 6, 0);
        }
        break;
    }
}

/* Spawns the actor's sleep bubble effect every 50 frames. */
extern "C" void pl_sleep_bubble_spawn(_PLW* self) {
    if ((s32)self->frame_0x020 % 50 == 0) {
        eft013_set(self, 0xA);
    }
}

/* The sleep act: falls asleep (the sheathed idle table), snoozes with the bubble effect and wakes up after 200
 * frames into the wake-up act. */
extern "C" void pl_act_sleep(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 1);
        pl_act_set_flag(self, 0x180);
        Pl_chr_set_attr_default(self, 0x15, 0, 0);
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x18 = 0;
        Pl_act_set_step_table(self, (u32)pl_sleep_steps, 10);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (Get_motion_no(self) == 0x15) {
                if (Pl_motion_end_ck(self) == 1) {
                    Pl_chr_set_attr_default(self, 5, 0, 0x92);
                }
            } else {
                pl_sleep_bubble_spawn(self);
            }
            if (++self->field_0x3F4 >= 200) {
                pl_act_enter(self, 6, 0x1E, 0xC);
            }
        }
        break;
    }
}

/* The drowsy walk act: staggers forward with the sleep bubbles; the master falls asleep after 200 frames, turns with
 * the camera while the tier holds, and stops into the drowsy stand after ten frames. */
extern "C" void pl_act_drowsy_walk(_PLW* self) {
    VEC3 unused;

    VEC3_ctor(&unused);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 1);
        pl_act_set_flag(self, 0x180);
        Pl_chr_set_attr_default(self, 0x27, 0, 0);
        pl_act_set_frame_timer(self);
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x18 = 0;
        Pl_act_set_step_table(self, (u32)pl_sleep_steps, 0);
        self->field_0x28 = 10;
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        pl_sleep_bubble_spawn(self);
        if (Pl_master_ck(self) != 0) {
            if (self->field_0x28 > 0) {
                self->field_0x28--;
            }
            if (++self->field_0x3F4 >= 200) {
                pl_act_enter(self, 6, 0x1E, 0xC);
            } else if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x28 <= 0) {
                pl_act_enter(self, 6, 0x1C, 1);
            }
        }
        break;
    }
}

/* The sleep-fall act: drops asleep where the actor stands for 240 frames (480 with skill 6), each input shake
 * cutting a frame, then wakes into the idle slot. */
extern "C" void pl_act_sleep_fall(_PLW* self) {
    VEC3 unused;

    VEC3_ctor(&unused);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 0, 0, 1);
        self->field_0x18 = 0;
        self->field_0x3F0 = 0;
        self->field_0x3F4 = 0;
        pl_act_guard_timer_reset(self);
        pl_act_clear_wait(self, 0);
        Pl_chr_set_attr_default(self, 0xE1, 4, 0);
        self->field_0x18 = 0;
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        pl_sleep_bubble_spawn(self);
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            self->field_0x28 = 240;
            if (Pl_Skill_ck(self, 6) == 1) {
                self->field_0x28 *= 2;
            }
            Pl_chr_set_attr_default(self, 0xD2, 0, 0x9A);
        }
        break;
    case 2:
        if (self->field_0x007 != 0) {
            self->field_0x007--;
        }
        if (pl_input_active_ck(self) == 1 && self->field_0x007 == 0) {
            self->field_0x007 = 3;
            self->field_0x28--;
        }
        if (--self->field_0x28 <= 0) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xCF, 0, 0);
        } else {
            pl_sleep_bubble_spawn(self);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
        }
        break;
    }
}

/* The paralysis act for `variant` 0-3 (2 and 3 the drawn ones; 1 and 3 continue a running paralysis): trembles
 * with the paralysis spark every 32 frames for 150 frames (300 with skill 4), each input shake cutting a frame,
 * then recovers into the kind's slot. */
#pragma optimization_level 4
extern "C" void pl_act_paralysis(_PLW* self, u32 variant) {
    self->field_0x006++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        if (variant <= 1) {
            Pl_act_set_motion(self, 0, 0, 1);
        } else {
            Pl_act_set_motion(self, 3, 0, 1);
        }
        self->field_0x18 = 0;
        self->field_0x3F6 = 0;
        self->field_0x3F0 = 0;
        pl_act_guard_timer_reset(self);
        pl_act_clear_wait(self, 0);
        pl_act_clear_flag5bb(self);
        switch (variant) {
        case 0:
        case 2:
            if (variant == 0) {
                Pl_chr_set_attr_default(self, 0xE0, 4, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x104, 4, 0);
            }
            self->field_0x006 = 0;
            pl_paralysis_spark(self, pl_float_0_75);
            self->field_0x3FA = 150;
            if (Pl_Skill_ck(self, 4) == 1) {
                self->field_0x3FA *= 2;
            }
            pl_act_set_gauge_arm(self, 2);
            break;
        case 1:
        case 3:
            self->act_step_0x05 = 2;
            self->field_0x006 = 0x10;
            if (self->field_0x3FA <= 0) {
                self->field_0x3FA = 0;
            }
            if (variant == 1) {
                Pl_chr_set_attr_default(self, 0xD2, 2, 0x96);
            } else {
                Pl_chr_set_attr_default(self, 0x103, 2, 0x112);
            }
            break;
        }
        pl_act_set_frame_timer(self);
        break;
    case 1:
        pl_act_set_gauge_arm(self, 2);
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if (variant == 0) {
                Pl_chr_set_attr_default(self, 0xD2, 2, 0x96);
            } else {
                Pl_chr_set_attr_default(self, 0x103, 2, 0x112);
            }
            pl_act_set_gauge_arm(self, 0);
        }
        if (self->act_step_0x05 <= 2 && (self->field_0x006 & 0x1F) == 0) {
            pl_paralysis_spark(self, pl_float_0_75);
        }
        break;
    case 2:
        if (self->field_0x007 != 0) {
            self->field_0x007--;
        }
        if (pl_input_active_ck(self) == 1 && self->field_0x007 == 0) {
            self->field_0x007 = 3;
            self->field_0x3FA--;
        }
        if (--self->field_0x3FA <= 0) {
            self->act_step_0x05++;
            if (variant <= 1) {
                Pl_chr_set_attr_default(self, 0xCF, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x102, 0, 0);
            }
            pl_act_set_gauge_arm(self, 2);
            break;
        }
        switch (Get_motion_no(self)) {
        case 0xD2:
            if (Pl_frame_check(self, 0, pl_frame_window_170, pl_float_zero) == 1 && (ran_suu(1) & 0x3F) == 0) {
                Pl_chr_set_attr_default(self, 0xDF, 0, 0);
            }
            break;
        case 0xDF:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0xD2, 4, 0xAA);
            }
            break;
        case 0x103:
            if (Pl_frame_check(self, 0, pl_frame_window_274, pl_float_zero) == 1 && (ran_suu(1) & 0x3F) == 0) {
                Pl_chr_set_attr_default(self, 0x105, 0, 0);
            }
            break;
        case 0x105:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0x103, 4, 0x112);
            }
            break;
        }
        if ((self->field_0x006 & 0x1F) == 0) {
            pl_paralysis_spark(self, pl_float_0_75);
        }
        break;
    case 3:
        pl_act_set_gauge_arm(self, 2);
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, self->kind_0x09, 4, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}
#pragma optimization_level reset

/* The heavy flinch act of the drawn weapon (`variant` 1 from behind): plays the flinch, then re-enters the kind's
 * slot and re-arms the gauge. */
extern "C" void pl_act_flinch_drawn(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 1);
        fn_8027D4F0(self);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x100, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x101, 0, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 6, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* The grabbed act (`variant` 0 by the kind-0x12 enemy, 1 by the kind-4 one): finds the grabbing enemy in the area
 * and flies the actor toward its joint (or, with none, knocks it back), then lands it, turns it around and
 * re-enters the slot the variant picks. */
extern "C" void pl_act_grabbed(_PLW* self, s32 variant) {
    VEC3 target;
    VEC3 dir;
    VEC3 diff;
    PlMoveWork* work;
    s32 found;
    s32 i;
    u16 count;
    u16 angle;

    VEC3_ctor(&target);
    VEC3_ctor(&dir);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_set_gauge_arm(self, 10);
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x10F, 0, 0);
            Pl_act_set_motion(self, 3, 0, 1);
            self->field_0x28 = 65;
        } else {
            Pl_chr_set_attr_default(self, 0xDB, 0, 0);
            Pl_act_set_motion(self, 2, 0, 1);
            self->field_0x28 = 90;
        }
        fn_8027D4F0(self);
        pl_get_joint_wpos(self, 0xE, &dir);
        if (self->vec_0x03C.y < dir.y) {
            self->vec_0x03C.y = dir.y;
        }
        self->field_0x585 = 0;
        pl_motion_clear(self);
        pl_act_clear_flag5bb(self);
        found = 0;
        work = (PlMoveWork*)get_move_work_adrs(3);
        count = get_move_work_max(3);
        for (i = 0; i < count; i++, work++) {
            if (work->field_0x000 != 0 && work->field_0x001 != 0 && work->field_0x1E1 == get_now_areano()) {
                if (variant == 0 && work->kind_0x003 == 0x12 && work->sub_area_0x01A == self->sub_area_0x3B6) {
                    found = 1;
                    get_joint_wpos_em((struct _ENEMY_WORK*)work, 0xF, &target);
                    subVec3(&diff, &target, &self->vec_0x03C);
                    copyVec3(&dir, &diff);
                    calcVecAngXY(&dir, (u32*)&self->rot_0x54.x, (u32*)&self->rot_0x54.y);
                    self->field_0x0AC = self->rot_0x54.x;
                    self->field_0x0A8 = self->rot_0x54.y;
                    break;
                }
                if (variant == 1 && work->kind_0x003 == 4 && work->sub_area_0x01A == self->sub_area_0x3B6) {
                    found = 1;
                    copyVec3(&target, &work->pos_0x188);
                    self->rot_0x54.y = calcVecAng2(&self->vec_0x03C, &target);
                    self->field_0x0A8 = self->rot_0x54.y;
                    break;
                }
            }
        }
        if (found == 0) {
            dir.x = pl_float_zero;
            dir.y = pl_float_zero;
            dir.z = pl_float_neg7;
            rotVecY(&dir, self->rot_0x54.y);
            self->motion_vel_0x78 = pl_frame_window_2 * dir.x;
            self->motion_vel_0x80 = pl_frame_window_2 * dir.z;
            self->motion_vel_0x7C = pl_frame_window_20;
            self->motion_acc_0x88 = pl_float_neg0_72727;
            break;
        }
        if (variant == 0) {
            dir.x = (target.x - self->vec_0x03C.x) / pl_frame_window_60;
            dir.y = (target.y - self->vec_0x03C.y) / pl_frame_window_60;
            dir.z = (target.z - self->vec_0x03C.z) / pl_frame_window_60;
        } else {
            dir.x = (target.x - self->vec_0x03C.x) / pl_frame_window_50;
            dir.y = (target.y - self->vec_0x03C.y) / pl_frame_window_50;
            dir.z = (target.z - self->vec_0x03C.z) / pl_frame_window_50;
        }
        self->motion_vel_0x78 = dir.x;
        self->motion_vel_0x80 = dir.z;
        if (variant == 0) {
            self->motion_vel_0x7C = dir.y;
            break;
        }
        if (dir.y <= pl_frame_window_15) {
            dir.y = pl_frame_window_15;
        }
        self->motion_vel_0x7C = dir.y;
        pl_jump_gravity_set(self, 0x32);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1 && Get_motion_no(self) == 0xDB) {
            Pl_chr_set_attr_default(self, 0xDC, 0, 0);
        }
        pl_motion_integrate(self);
        if (pl_zone_ground_drop(self) == 1) {
            break;
        }
        if (variant == 0) {
            if (--self->field_0x28 <= 0) {
                self->act_step_0x05 = 3;
                Pl_chr_set_attr_default(self, 0x111, 4, 0);
            }
        } else if (self->motion_vel_0x7C <= pl_float_zero &&
                   self->vec_0x03C.y <= pl_frame_window_63 + self->ground_y_0x060) {
            self->act_step_0x05++;
            pl_act_set_gauge_arm(self, 10);
            Pl_chr_set_attr_default(self, 0xD7, 0, 0);
            self->vec_0x03C.y = self->ground_y_0x060;
            self->field_0x01B = 1;
            self->kind_0x09 = 4;
            if (variant == 1) {
                fn_8025A7E0(self, 0);
                pl_act_enter(self, 7, 1, 2);
            }
        }
        break;
    case 2:
        pl_act_set_gauge_arm(self, 10);
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            angle = self->rot_0x54.y + 0x8000;
            self->rot_0x54.y = angle;
            self->field_0x0A8 = angle;
            Pl_chr_set_attr_default(self, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            if (variant == 0) {
                pl_act_reenter(self, 3, 8, 0);
            } else {
                pl_act_reenter(self, 0, 8, 0);
            }
            pl_act_set_gauge_arm(self, 10);
        }
        break;
    }
}

/* Drives one tick of the player's act state: it ages the shell timer the act belongs to, and
 * picks the next action code from the motion/act predicates when the timer runs out. */
extern "C" void fn_80258FCC(_PLW* self, s16 a2, s16 a3, u8 a4)
{
    s16* timer;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (a3 == 0) {
        timer = &self->field_0x418;
    } else {
        timer = &self->field_0x41E;
    }
    if (a4 == 0) {
        if ((s8)self->field_0x444 == 0) {
            if (pl_input_active_ck(self) == 1) {
                if (*timer > 0) {
                    *timer -= (s16)10;
                    if (*timer < 0) {
                        *timer = 0;
                    }
                    self->field_0x444 = 1;
                }
            }
        } else {
            self->field_0x444--;
        }
    }
    if (*timer <= 0) {
        pl_act_clear_wait(self, 0);
        if (a3 == 0) {
            self->field_0x3D8 &= ~0x300;
            pl_act_enter(self, 6, 45, 0);
        } else {
            self->field_0x3D8 &= ~0x300;
            pl_act_enter(self, 6, 65, 0);
        }
        return;
    }
    if (a4 != 0) {
        return;
    }
    if (pl_part_flag_ck(self, 4) == 1) {
        if (self->field_0x378 >= 150) {
            self->field_0x0A8 = self->field_0x058;
            if (a3 == 0) {
                pl_act_enter(self, 6, 73, 4);
            } else {
                pl_act_enter(self, 6, 72, 4);
            }
            return;
        }
    }
    if (pl_act_param_tier_ck(self, 0) >= 1) {
        pl_act_set_cam_ang(self);
        if (pl_part_flag_ck(self, 0) == 1 && (s8)self->field_0x266 == 0) {
            if (a3 == 0) {
                if (Pl_act_ck(self, 6, 47) != 0) {
                    return;
                }
                pl_act_enter(self, 6, 47, 0);
            } else {
                if (Pl_act_ck(self, 6, 67) != 0) {
                    return;
                }
                pl_act_enter(self, 6, 67, 0);
            }
            return;
        }
        if (a3 == 0) {
            if (Pl_act_ck(self, 6, 46) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            pl_act_enter(self, 6, 46, 0);
        } else {
            if (Pl_act_ck(self, 6, 66) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            pl_act_enter(self, 6, 66, 0);
        }
        return;
    }
    if (pl_status_bit_ck((_ENEMY_WORK*)self, 0x1C0) == 0) {
        return;
    }
    if (a2 == 1 && self->field_0x28 != 0) {
        return;
    }
    if (a3 == 0) {
        pl_act_enter(self, 6, 44, 0);
    } else {
        pl_act_enter(self, 6, 64, 0);
    }
}

/* Advances the act's own three-step state and hands the tick to `fn_80258FCC` once the timer the
 * caller passed has been picked. */
extern "C" void fn_80259310(_PLW* self, s32 a2, s32 a3)
{
    u8 flag;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        flag = 0;
        self->field_0x18 = 0;
        pl_act_clear_flag5bb(self);
        Pl_chr_setX(self, 51, 4, 0);
        if (a2 == 0) {
            self->field_0x3F6 = 0;
            self->field_0x3F0 = 0;
            pl_act_guard_timer_reset(self);
            pl_act_clear_wait(self, 0);
            if (a3 == 0) {
                self->field_0x418 = 900;
            } else {
                self->field_0x41E = 450;
            }
            self->field_0x45A = 0;
            self->field_0x45C = 0;
            self->field_0x28 = 4;
        } else {
            pl_act_clear_wait(self, 1);
            self->field_0x28 = 4;
        }
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
    case 1:
        self->field_0x28 = self->field_0x28 - 1;
        if (self->field_0x28 <= 0) {
            self->act_step_0x05++;
        }
        flag = 1;
        break;
    case 2:
        flag = 0;
        break;
    }
    if (Pl_master_ck(self) == 1) {
        fn_80258FCC(self, 0, a3, flag);
    }
}

/* Two-step act: arms the weapon pose and clears the act flag, then runs the tail predicate. */
extern "C" void fn_8025948C(_PLW* self, s32 unused)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        Pl_chr_set_attr_default(self, 307, 6, 0);
        pl_act_clear_flag5bb(self);
        self->field_0x354 = pl_float_1_7;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
        }
        break;
    }
}

/* Two-step act: arms the timed pose, then per tick nudges the actor along the heading and counts the
 * timer down while `fn_80258FCC` runs. */
extern "C" void fn_8025953C(_PLW* self, s32 a2)
{
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        Pl_chr_set_attr_default(self, 52, 4, 0);
        pl_act_clear_flag5bb(self);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        pl_act_set_flag(self, 0x40);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        v.x = pl_float_zero;
        v.y = pl_float_zero;
        v.z = pl_frame_window_2;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* Two-step act: arms the pose picked by the actor's SE name set, then per tick either steps to the
 * next action or nudges the actor along the heading while `fn_80258FCC` runs. */
extern "C" void fn_80259684(_PLW* self, s32 a2)
{
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 53, 4, 0);
        } else {
            Pl_chr_setX(self, 59, 4, 0);
        }
        pl_act_clear_flag5bb(self);
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        pl_act_set_flag(self, 0x180);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) != 0) {
            self->field_0x0A8 = (u16)self->field_0x058;
            if (a2 == 0) {
                pl_act_enter(self, 6, 48, 0);
                return;
            }
            pl_act_enter(self, 6, 68, 0);
            return;
        }
        v.x = pl_float_zero;
        v.y = pl_float_zero;
        v.z = pl_frame_window_5;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* The launched-by-wind act (`variant` 1 the short hop; `drawn` picks the drawn landing): throws the actor up and
 * along its facing, and on landing picks the hard landing after 50 frames of flight, the roll after 30, or the
 * soft landing. */
extern "C" void pl_act_wind_launched(_PLW* self, s32 variant, s32 drawn) {
    VEC3 move;
    s32 frames;

    VEC3_ctor(&move);
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 2, 0, 1);
        fn_8027D4F0(self);
        pl_act_clear_flag5bb(self);
        if (variant != 1) {
            Pl_chr_set_attr_default(self, 0x38, 0, 0);
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x80 = pl_frame_window_20;
            self->motion_vel_0x7C = pl_frame_window_18;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x8C = -(pl_frame_window_20 / pl_frame_window_80);
            pl_jump_gravity_set(self, 0x13);
        } else {
            Pl_chr_set_attr_default(self, 0x38, 8, 0x20);
            self->field_0x585 = 0;
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x80 = pl_frame_window_8;
            self->motion_vel_0x7C = pl_float_neg10;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x8C = pl_float_zero;
            self->motion_acc_0x88 = pl_float_neg0_72727;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        self->field_0x007 = 0;
        break;
    case 1:
        self->motion_vel_0x78 += self->motion_acc_0x84;
        self->motion_vel_0x7C += self->motion_acc_0x88;
        self->motion_vel_0x80 += self->motion_acc_0x8C;
        if (self->motion_vel_0x78 < pl_float_zero) {
            self->motion_vel_0x78 = pl_float_zero;
        }
        if (self->motion_vel_0x80 < pl_float_zero) {
            self->motion_vel_0x80 = pl_float_zero;
        }
        move.x = self->motion_vel_0x78;
        move.y = self->motion_vel_0x7C;
        move.z = self->motion_vel_0x80;
        rotVecY(&move, self->rot_0x54.y);
        self->vec_0x03C.x += move.x;
        self->vec_0x03C.y += move.y;
        self->vec_0x03C.z += move.z;
        if (pl_zone_ground_drop(self) == 1 || self->motion_vel_0x7C >= pl_float_zero) {
            break;
        }
        frames = ++self->field_0x28;
        if (self->vec_0x03C.y < self->ground_y_0x060) {
            self->vec_0x03C.y = self->ground_y_0x060;
            self->field_0x01B = 1;
            self->kind_0x09 = 0;
            self->field_0x585 = 0;
            if (frames >= 50) {
                pl_act_clear_wait(self, 0);
                pl_act_enter(self, 2, 2, 0);
            } else if (frames >= 30) {
                if (drawn == 0) {
                    pl_act_enter(self, 6, 0x32, 0);
                } else {
                    pl_act_enter(self, 6, 0x46, 0);
                }
            } else if (drawn == 0) {
                pl_act_enter(self, 6, 0x33, 0);
            } else {
                pl_act_enter(self, 6, 0x47, 0);
            }
        }
        break;
    }
}

/* The wind-stagger act (`variant` 1 from behind; `drawn` picks the drawn follow-up): plays the stagger, then enters
 * the follow-up act. */
extern "C" void pl_act_wind_stagger(_PLW* self, s32 variant, s32 drawn) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x39, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x3A, 0, 0);
        }
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (drawn == 0) {
                pl_act_enter(self, 6, 0x2C, 2);
            } else {
                pl_act_enter(self, 6, 0x40, 2);
            }
        }
        break;
    }
}

/* The swept-away act: turns the actor to face the current and drags it ten units a frame while its sink decays to
 * -250, then after 150 frames enters the washed-up act. */
extern "C" void pl_act_swept_away(_PLW* self) {
    VEC3 unused;
    s32 frames;

    VEC3_ctor(&unused);
    frames = ++self->field_0x28;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 1);
        Pl_chr_set_attr_default(self, 0xCE, 0, 0);
        pl_act_set_frame_timer(self);
        fn_8027D4F0(self);
        pl_act_guard_timer_reset(self);
        pl_act_clear_wait(self, 0);
        self->field_0x28 = 0;
        self->rot_0x54.y = 0x8000;
        self->field_0x0A8 = 0x8000;
        self->field_0x18 = 0;
        break;
    case 1:
        self->field_0x28 = frames + 1;
        if (frames + 1 >= 150) {
            pl_act_enter(self, 6, 0x3E, 0x2000);
            break;
        }
        self->field_0x06C -= pl_frame_window_4;
        if (self->field_0x06C < pl_float_neg250) {
            self->field_0x06C = pl_float_neg250;
        } else {
            self->vec_0x03C.z += pl_frame_window_10;
        }
        copyVec3(&self->vec_0x048, &self->vec_0x03C);
        break;
    }
}

/* Places the actor at its chunk's anchor plus its rotated motion offset (the drop-in acts' position). */
extern "C" void pl_chunk_anchor_place(_PLW* self) {
    VEC3 offset;

    VEC3_ctor(&offset);
    copyVec3(&offset, (VEC3*)&self->motion_vel_0x78);
    rotVecY(&offset, self->field_0x058);
    copyVec3(&self->vec_0x03C, &pl_chunk_anchors[self->chunk_ofs]);
    addVec3To(&self->vec_0x03C, &offset);
}

/* The drop-in act: falls from the chunk's anchor with a growing drop speed, landing pose at 30, keeping the actor
 * at the anchor's offset. */
extern "C" void pl_act_drop_in(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 1);
        Pl_chr_setX(self, 0x1A6, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_steps_drop_in, 30);
        pl_act_set_frame_timer(self);
        fn_8027D4F0(self);
        self->rot_0x54.y = pl_chunk_anchor_angle[self->chunk_ofs];
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x18 = 0;
        pl_chunk_anchor_place(self);
        self->field_0x354 = pl_frame_window_1;
        break;
    case 1:
        self->motion_vel_0x80 += pl_frame_window_4;
        if (self->motion_vel_0x80 > pl_frame_window_30) {
            self->motion_vel_0x80 = pl_frame_window_30;
            if (self->field_0x006 == 0) {
                self->field_0x006++;
                Pl_chr_setX(self, 0x1A5, 0, 0);
            }
        }
        pl_chunk_anchor_place(self);
        break;
    }
}

/* The drop-out act: rises out of the chunk's anchor from 2000 units below, then enters the arrival act. */
extern "C" void pl_act_drop_out(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 1);
        Pl_chr_set_attr_default(self, 0x1A4, 0, 0);
        fn_8027D4F0(self);
        self->rot_0x54.y = pl_chunk_anchor_angle[self->chunk_ofs];
        self->field_0x0A8 = self->rot_0x54.y;
        self->field_0x18 = 0;
        self->motion_vel_0x78 = pl_float_zero;
        self->motion_vel_0x7C = pl_float_zero;
        self->motion_vel_0x80 = pl_float_neg2000;
        self->field_0x06C = pl_float_zero;
        pl_chunk_anchor_place(self);
        copyVec3(&self->vec_0x048, &self->vec_0x03C);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 6, 0x3D, 0);
        }
        pl_chunk_anchor_place(self);
        break;
    }
}

/* The drawn knock-down act (`variant` 1 from behind, `heavy` the heavy one): plays the knock-down with the heavy
 * knock-down table, then re-enters the kind's slot and re-arms the gauge. */
extern "C" void pl_act_knockdown_drawn(_PLW* self, s32 variant, s32 heavy) {
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 1);
        fn_8027D4F0(self);
        if (variant == 0) {
            if (heavy == 0) {
                Pl_chr_set_attr_default(self, 0x10B, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 0x10D, 0, 0);
            }
        } else if (heavy == 0) {
            Pl_chr_set_attr_default(self, 0x10C, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x10E, 0, 0);
        }
        pl_knockdown_steps_arm(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 3, 6, 0);
            pl_act_set_gauge_arm_skilled(self, 6);
        }
        break;
    }
}

/* The roar-flinch act (`drawn` picks the drawn follow-up): resets the act latches and covers the ears; with skill
 * 0xB6 the actor slides back, and it enters the follow-up act when the motion ends. */
extern "C" void pl_act_roar_flinch(_PLW* self, s32 drawn) {
    VEC3 slide;

    VEC3_ctor(&slide);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0;
        fn_80277BC4(self, 0);
        pl_act_set_motion_gate(self, 0);
        pl_sub_gauge_skill_add(self, -150);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        pl_stun_tick(self);
        pl_guard_latch_set(self);
        pl_camera_lock_clear(self);
        pl_act_clear_mode5c4(self);
        Pl_act_set_motion(self, 0, 0, 1);
        Pl_chr_set_attr_default(self, 0x1C, 2, 0);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (drawn == 0) {
                pl_act_enter(self, 6, 0x2C, 2);
            } else {
                pl_act_enter(self, 6, 0x40, 2);
            }
        } else if (Pl_Skill_ck(self, 0xB6) == 1 &&
                   Pl_frame_check(self, 3, pl_frame_window_10, pl_frame_window_40) == 1) {
            slide.x = pl_float_zero;
            slide.y = pl_float_zero;
            slide.z = pl_frame_window_10;
            rotVecY(&slide, self->rot_0x54.y);
            self->vec_0x03C.x += slide.x;
            self->vec_0x03C.z += slide.z;
        }
        break;
    }
}

/* The hit-act dispatcher: drops the act latches and runs the hit, knock-back, ailment and environment act's step
 * handler; an unknown act falls back to the idle slot. */
extern "C" void pl_act_group6_dispatch(_PLW* self) {
    pl_camera_lock_clear(self);
    pl_act_clear_mode5c4(self);
    pl_act_set_motion_gate(self, 0);
    switch (self->act_no) {
    case 0x0:
        pl_act_side_step(self, 0);
        break;
    case 0x1:
        pl_act_side_step(self, 1);
        break;
    case 0x2:
        pl_act_hit_light(self, 0);
        break;
    case 0x3:
        pl_act_hit_light(self, 1);
        break;
    case 0x4:
        pl_act_knockback_flight(self, 0);
        break;
    case 0x5:
        pl_act_knockback_flight(self, 1);
        break;
    case 0x6:
        pl_act_blown_away(self, 0);
        break;
    case 0x7:
        pl_act_blown_away(self, 1);
        break;
    case 0x8:
        pl_act_flinch_heavy(self, 0);
        break;
    case 0x9:
        pl_act_flinch_heavy(self, 1);
        break;
    case 0xA:
        pl_act_flinch_small(self, 0);
        break;
    case 0xB:
        pl_act_flinch_small(self, 1);
        break;
    case 0xC:
        pl_act_stagger(self, 0);
        break;
    case 0xD:
        pl_act_stagger(self, 1);
        break;
    case 0xE:
        pl_act_charge_repeat_step(self, 0);
        break;
    case 0xF:
        pl_act_charge_repeat_step(self, 2);
        break;
    case 0x10:
        pl_act_mud_stuck(self, 0);
        break;
    case 0x11:
        pl_act_mud_stuck(self, 1);
        break;
    case 0x12:
        pl_act_mud_stand(self);
        break;
    case 0x13:
        pl_act_mud_walk(self, 0);
        break;
    case 0x14:
        pl_act_mud_walk(self, 1);
        break;
    case 0x15:
        pl_act_countdown_step(self, 0);
        break;
    case 0x16:
        pl_act_countdown_step(self, 1);
        break;
    case 0x1A:
        pl_act_knockback_flight(self, 2);
        break;
    case 0x1B:
        pl_act_stun(self, 0);
        break;
    case 0x1C:
        pl_act_sleep(self);
        break;
    case 0x1D:
        pl_act_drowsy_walk(self);
        break;
    case 0x1E:
        pl_act_sleep_fall(self);
        break;
    case 0x1F:
        pl_act_paralysis(self, 0);
        break;
    case 0x20:
        pl_act_paralysis(self, 1);
        break;
    case 0x21:
        pl_act_paralysis(self, 2);
        break;
    case 0x22:
        pl_act_paralysis(self, 3);
        break;
    case 0x23:
        pl_act_stun(self, 1);
        break;
    case 0x24:
        pl_act_charge_repeat_step(self, 3);
        break;
    case 0x25:
        pl_act_charge_repeat_step(self, 5);
        break;
    case 0x26:
        pl_act_knockback_flight(self, 3);
        break;
    case 0x27:
        pl_act_flinch_drawn(self, 0);
        break;
    case 0x28:
        pl_act_flinch_drawn(self, 1);
        break;
    case 0x29:
        pl_act_grabbed(self, 0);
        break;
    case 0x2A:
        pl_act_grabbed(self, 1);
        break;
    case 0x2B:
        fn_80259310(self, 0, 0);
        break;
    case 0x2C:
        fn_80259310(self, 1, 0);
        break;
    case 0x2D:
        fn_8025948C(self, 0);
        break;
    case 0x2E:
        fn_8025953C(self, 0);
        break;
    case 0x2F:
        fn_80259684(self, 0);
        break;
    case 0x30:
        pl_act_wind_launched(self, 0, 0);
        break;
    case 0x31:
        pl_act_wind_launched(self, 1, 0);
        break;
    case 0x32:
        pl_act_wind_stagger(self, 0, 0);
        break;
    case 0x33:
        pl_act_wind_stagger(self, 1, 0);
        break;
    case 0x34:
        pl_act_swept_away(self);
        break;
    case 0x36:
        pl_act_step_down(self, 1);
        break;
    case 0x37:
        pl_act_knockdown_drawn(self, 0, 0);
        break;
    case 0x38:
        pl_act_knockdown_drawn(self, 0, 1);
        break;
    case 0x39:
        pl_act_knockdown_drawn(self, 1, 0);
        break;
    case 0x3A:
        pl_act_knockdown_drawn(self, 1, 1);
        break;
    case 0x3B:
        pl_act_charge_repeat_step(self, 1);
        break;
    case 0x3C:
        pl_act_charge_repeat_step(self, 4);
        break;
    case 0x3D:
        pl_act_drop_in(self);
        break;
    case 0x3E:
        pl_act_drop_out(self);
        break;
    case 0x3F:
        fn_80259310(self, 0, 1);
        break;
    case 0x40:
        fn_80259310(self, 1, 1);
        break;
    case 0x41:
        fn_8025948C(self, 1);
        break;
    case 0x42:
        fn_8025953C(self, 1);
        break;
    case 0x43:
        fn_80259684(self, 1);
        break;
    case 0x44:
        pl_act_wind_launched(self, 0, 1);
        break;
    case 0x45:
        pl_act_wind_launched(self, 1, 1);
        break;
    case 0x46:
        pl_act_wind_stagger(self, 0, 1);
        break;
    case 0x47:
        pl_act_wind_stagger(self, 1, 1);
        break;
    case 0x49:
        pl_act_roar_flinch(self, 0);
        break;
    case 0x48:
        pl_act_roar_flinch(self, 1);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* Re-arms the act's 300-frame cooldown. */
extern "C" void fn_8025A7D4(_PLW* self)
{
    self->field_0x3A8 = 300;
}

/* Loads the act's strike budget: `a2` steps remain before the cooldown re-arms. */
extern "C" void fn_8025A7E0(_PLW* self, u8 a2)
{
    self->field_0x3A5 = a2;
    self->field_0x3A4 = (u8)a2;
    self->field_0x3A6 = 0;
    self->field_0x3A7 = 0;
    fn_8025A7D4(self);
}

/* Counts the act's strike window down and re-arms the cooldown once the per-skill strike budget is
 * spent; `field_0x309`'s 0x40 bit is the "struck" status the shell layer reads. */
extern "C" void fn_8025A7FC(_PLW* self)
{
    u8 threshold;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s8)self->field_0x3A7 > 0) {
        self->field_0x3A7--;
    }
    if (Pl_cat_skill_ck(self, 48) == 1) {
        if (self->field_0x128 == 2) {
            threshold = 6;
        } else {
            threshold = 8;
        }
    } else {
        threshold = 15;
        if (self->field_0x128 == 2) {
            threshold = 12;
        }
    }
    if (self->field_0x3A8 > 0) {
        if (self->field_0x378 <= 0) {
            return;
        }
        if (pl_input_active_ck(self) != 1) {
            return;
        }
        if ((s8)self->field_0x3A7 != 0) {
            return;
        }
        self->field_0x3A6++;
        self->field_0x3A7 = 1;
        if (self->field_0x3A6 < threshold) {
            return;
        }
        self->field_0x3A6 = 0;
        self->field_0x309 |= 0x40;
        if (self->field_0x3A4 != 0) {
            self->field_0x3A4 = self->field_0x3A4 - 1;
        }
        fn_8025A7D4(self);
    } else {
        self->field_0x3A6 = 0;
        fn_8025A7D4(self);
    }
}

/* The kill-cut start act: plays the cut-in pose, then the master enters the kill-cut wait act; the others fall into
 * the waiting pose. */
extern "C" void pl_act_kill_cut_start(_PLW* self) {
    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0xF0, 0, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        fn_8025A7E0(self, 0);
        self->field_0x18 = 0;
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (Pl_motion_end_ck(self) == 1) {
                pl_act_enter(self, 7, 1, 0xC);
            }
        } else {
            self->act_step_0x05++;
            Pl_chr_setX(self, 0xF1, 2, 0);
        }
        break;
    }
}

/* The kill-cut wait act (`variant` 1 the timed one): waits for the kill-cut target; on one it arms the cut kind the
 * target's species picks (plus a random 0-3) and enters the cut act, and it gives up after 300 frames or when the
 * cut ends. */
extern "C" void pl_act_kill_cut_wait(_PLW* self, s32 variant) {
    s8 status;
    struct _ENEMY_WORK* target;
    s32 cut;
    s32 frames;

    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_setX(self, 0xF1, 2, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        self->field_0x18 = 0;
        if (variant == 0) {
            self->field_0x28 = 300;
        }
        self->field_0x006 = 30;
        self->field_0x007 = 3;
        if (variant == 1) {
            Pl_act_set_step_table(self, (u32)pl_kill_cut_steps, 0);
        }
        break;
    case 1:
        if (Pl_master_ck(self) != 1) {
            break;
        }
        if (self->field_0x006 != 0) {
            self->field_0x006--;
        }
        switch (variant) {
        case 0:
            target = em_kill_cut_target_get(0, self, (s32*)&status);
            if (target != NULL) {
                switch (((PlMoveWork*)target)->kind_0x003) {
                case 1:
                    cut = 0xF;
                    break;
                case 2:
                    cut = 0x11;
                    break;
                case 4:
                    cut = 0xE;
                    break;
                case 7:
                    cut = 0x16;
                    break;
                default:
                    cut = 0xF;
                    break;
                }
                fn_8025A7E0(self, cut + (ran_suu(1) & 3));
                pl_act_enter(self, 7, 2, 0);
                break;
            }
            frames = self->field_0x28;
            self->field_0x28 = frames - 1;
            if (frames <= 0 || (u8)status == 1) {
                pl_act_enter(self, 7, 3, 0);
            }
            break;
        case 1:
            if (Pl_master_ck(self) == 1) {
                fn_8025A7FC(self);
                if (self->field_0x3A4 == 0 || em_search_record_ck(1, self, &status) == 0) {
                    pl_act_enter(self, 7, 3, 0);
                }
            }
            break;
        }
        break;
    }
}

/* The kill-cut end act (`variant` 1 the failed cut): plays the closing pose, then re-enters the idle slot. */
extern "C" void pl_act_kill_cut_end(_PLW* self, s32 variant) {
    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0xF8, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0xF7, 0, 0);
        }
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        self->field_0x18 = 0;
        pl_act_arm_flags(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_set_gauge_arm(self, 2);
            pl_act_set_step_time(self, 2);
            pl_act_reenter(self, 0, 6, 0);
        }
        break;
    }
}

/* The kill-cut throw act: throws the cut's item at frame 18 (spending it unless the cheat input is held), then
 * enters the cut act. */
extern "C" void pl_act_kill_cut_throw(_PLW* self) {
    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0xF6, 0, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        self->field_0x18 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 7, 2, 0);
        } else if (Pl_frame_check(self, 0, pl_frame_window_18, pl_float_zero) == 1) {
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
            shell_set_func_ptr->method_0x00(self, 0x10, 2, shell_set_func_ptr);
        }
        break;
    }
}

/* The kill-cut taunt act: plays one of the two taunts, then enters the cut act, or the closing act once the cut
 * target is gone. */
extern "C" void pl_act_kill_cut_taunt(_PLW* self) {
    s8 status;

    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, pl_kill_cut_taunts[self->field_0x0B6 & 1], 0, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        self->field_0x18 = 0;
        Pl_act_set_step_table(self, (u32)pl_kill_cut_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 7, 2, 0);
        } else if (Pl_master_ck(self) == 1) {
            fn_8025A7FC(self);
            if (self->field_0x3A4 == 0 || em_search_record_ck(1, self, &status) == 0) {
                pl_act_enter(self, 7, 3, 0);
            }
        }
        break;
    }
}

/* The kill-cut dispatcher: drops the act latches and runs the kill-cut act's step handler; an unknown act falls
 * back to the idle slot. */
extern "C" void pl_act_group7_dispatch(_PLW* self) {
    pl_camera_lock_clear(self);
    pl_act_clear_mode5c4(self);
    switch (self->act_no) {
    case 0:
        pl_act_kill_cut_start(self);
        break;
    case 1:
        pl_act_kill_cut_wait(self, 0);
        break;
    case 2:
        pl_act_kill_cut_wait(self, 1);
        break;
    case 3:
        pl_act_kill_cut_end(self, 0);
        break;
    case 4:
        pl_act_kill_cut_end(self, 1);
        break;
    case 5:
        pl_act_kill_cut_throw(self);
        break;
    case 6:
        pl_act_kill_cut_taunt(self);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* Ages the strike counter and saturates it at ten. */
extern "C" void fn_8025B0D4(_PLW* self)
{
    self->field_0x446++;
    if (self->field_0x446 >= 10) {
        self->field_0x446 = 10;
    }
}

/* The carted act for `variant` 0-2: collapses (sleeping, fainting or the drawn faint) with the lobby latches
 * cleared, lets the master's camera and the quest know, and after 300 frames hands the actor to the quest's warp. */
extern "C" void pl_act_carted(_PLW* self, s32 variant) {
    s32 frames;

    pl_act_set_gauge_arm(self, 2);
    pl_act_set_step_time(self, 10);
    pl_camera_lock_clear(self);
    pl_act_clear_wait(self, 0);
    self->field_0x5BC = 0;
    self->take_menu_open_0x5BE = 0;
    self->quest_menu_open_0x5BD = 0;
    self->talk_wait_0x668 = 0;
    pl_cond_flags_clear(self, 0x800);
    if (variant == 2) {
        pl_model_set_state(self, 2);
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_clear_flag5bb(self);
        self->field_0x28 = 0;
        switch (variant) {
        case 0:
            Pl_chr_set_attr_default(self, 0xD2, 0, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0xD2, 0, 0xAA);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0xD1, 0, 0x78);
            break;
        }
        pl_act_hold_notify(self, 3);
        pl_act_set_frame_timer(self);
        fn_8025B0D4(self);
        Pl_act_set_motion(self, 0, 0, 1);
        pl_cond_flags_clear(self, 4);
        pl_cond_flags_clear(self, 0x10);
        self->field_0x18 = 0;
        if (Pl_master_ck(self) == 1) {
            camera_mode2_req();
            quest_player_enter_notify(self->chunk_ofs);
        }
        break;
    case 1:
    case 2:
        switch (Get_motion_no(self)) {
        case 0xD2:
            if (Pl_frame_check(self, 0, pl_frame_window_164, pl_float_zero) == 1 && (ran_suu(1) & 0x3F) == 0) {
                Pl_chr_set_attr_default(self, 0xDF, 0, 0);
            }
            break;
        case 0xDF:
            if (Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 0xD2, 0, 0xA4);
            }
            break;
        }
        if (Pl_master_ck(self) == 1 && self->act_step_0x05 == 1) {
            frames = ++self->field_0x28;
            if (frames >= 300) {
                self->act_step_0x05++;
                quest_warp_by_state(self);
            }
        }
        break;
    }
}

/* The drawn carted act: collapses with the drawn faint, lets the master's camera and the quest know, and after 300
 * frames hands the actor to the quest's warp. */
extern "C" void pl_act_carted_drawn(_PLW* self) {
    s32 frames;

    pl_act_set_gauge_arm(self, 2);
    pl_act_clear_wait(self, 0);
    pl_act_set_step_time(self, 10);
    pl_camera_lock_clear(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_clear_flag5bb(self);
        Pl_chr_set_attr_default(self, 0x103, 0, 0);
        pl_act_hold_notify(self, 3);
        pl_act_set_frame_timer(self);
        fn_8025B0D4(self);
        Pl_act_set_motion(self, 3, 0, 1);
        pl_cond_flags_clear(self, 4);
        pl_cond_flags_clear(self, 0x10);
        self->field_0x18 = 0;
        self->field_0x28 = 0;
        if (Pl_master_ck(self) == 1) {
            camera_mode2_req();
            quest_player_enter_notify(self->chunk_ofs);
        }
        break;
    case 1:
    case 2:
        if (Pl_master_ck(self) == 1 && self->act_step_0x05 == 1) {
            frames = ++self->field_0x28;
            if (frames >= 300) {
                self->act_step_0x05++;
                quest_warp_by_state(self);
            }
        }
        break;
    }
}

/* The carted-act dispatcher: drops the act latches and runs the carted act's step handler; an unknown act falls
 * back to the idle slot. */
extern "C" void pl_act_group8_dispatch(_PLW* self) {
    pl_act_clear_mode5c4(self);
    pl_camera_lock_clear(self);
    switch (self->act_no) {
    case 0:
        pl_act_carted(self, 0);
        break;
    case 1:
        pl_act_carted_drawn(self);
        break;
    case 2:
        pl_act_carted(self, 1);
        break;
    case 3:
        pl_act_carted(self, 2);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The kick act: kicks with the kick's hit table, then returns to the idle slot. */
extern "C" void pl_act_kick(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x32, 4, 0);
        pl_hit_table_arm(self, 1, 2);
        Pl_act_set_motion(self, 0, 1, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The running kick act: kicks on the run with its step table, then returns to the run slot. */
extern "C" void pl_act_kick_run(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x8E, 4, 0);
        pl_hit_table_arm(self, 2, 1);
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_act_set_step_table(self, (u32)pl_kick_run_steps, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        }
        break;
    }
}

/* The kick-act dispatcher (acts 0x64 and 0x65); an unknown act falls back to the idle slot. */
extern "C" void pl_act_kick_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0x64:
        pl_act_kick(self);
        break;
    case 0x65:
        pl_act_kick_run(self);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The gesture-wait act (`variant` 1 the long blend): arms the waiting gesture and its step table. */
extern "C" void pl_act_gesture_wait(_PLW* self, s32 variant) {
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        fn_8027AC00(self);
        if (variant == 0) {
            Pl_chr_setX(self, 0x190, 4, 0);
        } else {
            Pl_chr_setX(self, 0x190, 0x14, 0);
        }
        Pl_act_set_step_table(self, (u32)pl_steps_crouch, 0);
    }
}

/* The gesture act (`variant` 1 the emote with its stored id): plays the gesture, then returns to the idle slot. */
extern "C" void pl_act_gesture(_PLW* self, s32 variant) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        fn_8027AC00(self);
        Pl_chr_set_attr_default(self, 0x191, 4, 0);
        if (variant == 1) {
            stage_area_event_dispatch(self, self->field_0x47A);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The bow gesture act: plays the bow, then returns to the idle slot. */
extern "C" void pl_act_gesture_bow(_PLW* self, s32 unused) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_clear_flag5bb(self);
        Pl_chr_set_attr_default(self, 0x192, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The emote act for emote `index` 0-9: plays the emote record's motion (blended for the long ones) with its hit and
 * step tables, its follow-up motion, then returns to the idle slot. */
extern "C" void pl_act_emote(_PLW* self, s32 index) {
    PlEmote* emote;

    emote = &pl_emote_tbl[index];
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        fn_8027AC00(self);
        if ((u32)(index - 1) <= 1 || (u32)(index - 6) <= 1 || index == 4 || index == 9) {
            Pl_chr_set_attr(self, emote->attr_0x02, emote->part_0x04, emote->frame_0x06, 1);
        } else {
            Pl_chr_set_attr_default(self, emote->attr_0x02, emote->part_0x04, emote->frame_0x06);
        }
        pl_hit_table_arm(self, pl_emote_hit_kinds[emote->kind_0x00], 1);
        if (self->field_0x002 == 1 && self->field_0x18 == 1) {
            Pl_act_set_step_table(self, pl_emote_steps_drawn[emote->kind_0x00], 0);
        } else {
            Pl_act_set_step_table(self, pl_emote_steps[emote->kind_0x00], 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, emote->follow_0x08, 0, 0);
        }
        break;
    case 2:
        if (self->field_0x006 == 0 && Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
            self->field_0x006++;
            fn_8027D4F0(self);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The sit-down gesture act: sits the actor down with the stumble table. */
extern "C" void pl_act_gesture_sit(_PLW* self, s32 unused) {
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8001, 0, 0);
        self->field_0x18 = 0;
        fn_8027AC00(self);
        Pl_chr_setX(self, 8, 4, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_step_table(self, (u32)pl_steps_stumble, 0);
    }
}

/* The gesture-act dispatcher: runs the gesture and emote act's step handler; an unknown act falls back to the idle
 * slot. */
extern "C" void pl_act_group10_dispatch(_PLW* self) {
    switch (self->act_no) {
    case 0:
        pl_act_gesture_wait(self, 0);
        break;
    case 1:
        pl_act_gesture(self, 0);
        break;
    case 2:
        pl_act_gesture_bow(self, 0);
        break;
    case 4:
        pl_act_emote(self, 0);
        break;
    case 9:
        pl_act_emote(self, 5);
        break;
    case 3:
        pl_act_gesture(self, 1);
        break;
    case 5:
        pl_act_emote(self, 1);
        break;
    case 6:
        pl_act_emote(self, 2);
        break;
    case 7:
        pl_act_emote(self, 3);
        break;
    case 8:
        pl_act_emote(self, 4);
        break;
    case 10:
        pl_act_emote(self, 6);
        break;
    case 11:
        pl_act_emote(self, 7);
        break;
    case 12:
        pl_act_emote(self, 8);
        break;
    case 13:
        pl_act_emote(self, 9);
        break;
    case 14:
        pl_act_gesture_sit(self, 0);
        break;
    case 15:
        fn_8024A51C(self, 1);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The kneel-gesture act for row `row` of the standing (or, `kneel` set, the kneeling) gesture table: plays the
 * gesture for 60 frames, then the master leaves it on input or re-arms the kneel. */
extern "C" void pl_act_kneel_gesture(_PLW* self, s32 row, s32 kneel) {
    PlActAttr* attr;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            attr = &pl_kneel_gesture_tbl[row];
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            attr = &pl_kneel_gesture_tbl_kneel[row];
        }
        Pl_chr_set_attr_default(self, attr->attr_0x00, attr->part_0x02, attr->frame_0x04);
        self->field_0x28 = 60;
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_master_ck(self) == 1 && self->field_0x28 <= 0) {
            if (pl_act_param_tier_ck(self, 0) != 0 || Pl_motion_input_ck(1) == 1) {
                Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
            } else {
                pl_motion_request_latch(self, kneel);
            }
        }
        break;
    }
}

/* The one-shot gesture act for row `row` of the standing (or kneeling) gesture table: plays it, then returns to the
 * kind's slot. */
extern "C" void pl_act_gesture_once(_PLW* self, s32 row, s32 kneel) {
    PlActAttr* attr;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            attr = &pl_gesture_once_tbl[row];
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            attr = &pl_gesture_once_tbl_kneel[row];
        }
        Pl_chr_set_attr_default(self, attr->attr_0x00, attr->part_0x02, attr->frame_0x04);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The sit-on-the-ground act (`kneel` the kneeling variant): sits down and, from frame 90, the master stands up into
 * the stand-up act on input or re-arms the kneel. */
extern "C" void pl_act_ground_sit(_PLW* self, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x1F7, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x20B, 4, 0);
        }
        self->field_0x28 = 60;
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && Pl_frame_check(self, 1, pl_frame_window_90, pl_float_zero) == 1) {
            if (pl_act_param_tier_ck(self, 0) != 0 || Pl_motion_input_ck(1) == 1) {
                if (kneel == 0) {
                    pl_act_enter(self, 0xB, 0xC, 0);
                } else {
                    pl_act_enter(self, 0xB, 0x1A, 0);
                }
            } else {
                pl_motion_request_latch(self, kneel);
            }
        }
        break;
    }
}

/* The stand-up act (`kneel` from the kneeling pose): stands up, the standing variant shaking off its numbness,
 * then returns to the kind's slot. */
extern "C" void pl_act_stand_up(_PLW* self, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x1F8, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x20C, 4, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (kneel == 1) {
                Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
            } else {
                self->act_step_0x05++;
                pl_model_set_state(self, 2);
                Pl_chr_set_attr_default(self, 0xCF, -4, 0x3C);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The stretch act (`kneel` the kneeling variant): stretches with the step sounds at frames 48, 68, 168 and 190,
 * then the master leaves it on input after 60 frames or re-arms the kneel. */
extern "C" void pl_act_stretch(_PLW* self, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x1FB, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x20F, 4, 0);
        }
        self->field_0x28 = 60;
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_frame_check(self, 0, pl_frame_window_48, pl_float_zero) == 1) {
            pl_hit_table_arm(self, 8, 3);
        }
        if (Pl_frame_check(self, 0, pl_frame_window_68, pl_float_zero) == 1) {
            pl_hit_table_arm(self, 9, 3);
        }
        if (Pl_frame_check(self, 0, pl_frame_window_168, pl_float_zero) == 1) {
            pl_hit_table_arm(self, 9, 3);
        }
        if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
            pl_hit_table_arm(self, 8, 3);
        }
        if (Pl_master_ck(self) == 1 && self->field_0x28 <= 0) {
            if (pl_act_param_tier_ck(self, 0) != 0 || Pl_motion_input_ck(1) == 1) {
                Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
            } else {
                pl_motion_request_latch(self, kneel);
            }
        }
        break;
    }
}

/* The meditate act (`kneel` the kneeling variant): rests and, with cat skill 0x25, grants the defence buff after
 * 150 frames; from frame 150 the master stands up on input or re-arms the kneel. */
extern "C" void pl_act_meditate(_PLW* self, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x201, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x215, 4, 0);
        }
        self->field_0x28 = 60;
        self->count_0x034 = 0;
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && self->field_0x006 == 0 && Pl_cat_skill_ck(self, 0x25) == 1) {
            if (++self->count_0x034 >= 150) {
                self->field_0x006++;
                pl_attack_up_set(self, 1, 0xF);
                self->field_0x44A = 900;
            }
        }
        if (Pl_master_ck(self) == 1 && Pl_frame_check(self, 1, pl_frame_window_150, pl_float_zero) == 1) {
            if (pl_act_param_tier_ck(self, 0) != 0 || Pl_motion_input_ck(1) == 1) {
                if (kneel == 0) {
                    pl_act_enter(self, 0xB, 0xD, 0);
                } else {
                    pl_act_enter(self, 0xB, 0x1B, 0);
                }
            } else {
                pl_motion_request_latch(self, kneel);
            }
        }
        break;
    }
}

/* The meditate-end act (`kneel` from the kneeling pose): rises, the kneeling variant through its recovery motion,
 * then returns to the kind's slot. */
extern "C" void pl_act_meditate_end(_PLW* self, s32 kneel) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (kneel == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x202, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x216, 4, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (kneel == 0) {
                Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
            } else {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x102, 0, 0x1E);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The lobby-gesture dispatcher: drops the act latches and runs the gesture act's step handler (acts 14-27 the
 * kneeling twins of 0-13); an unknown act falls back to the idle slot. */
extern "C" void pl_act_group11_dispatch(_PLW* self) {
    pl_camera_lock_clear(self);
    switch (self->act_no) {
    case 0:
        pl_act_kneel_gesture(self, 0, 0);
        break;
    case 1:
        pl_act_kneel_gesture(self, 1, 0);
        break;
    case 2:
        pl_act_kneel_gesture(self, 2, 0);
        break;
    case 3:
        pl_act_kneel_gesture(self, 3, 0);
        break;
    case 4:
        pl_act_gesture_once(self, 0, 0);
        break;
    case 5:
        pl_act_ground_sit(self, 0);
        break;
    case 12:
        pl_act_stand_up(self, 0);
        break;
    case 6:
        pl_act_kneel_gesture(self, 4, 0);
        break;
    case 7:
        pl_act_stretch(self, 0);
        break;
    case 8:
        pl_act_gesture_once(self, 1, 0);
        break;
    case 9:
        pl_act_kneel_gesture(self, 5, 0);
        break;
    case 10:
        pl_act_gesture_once(self, 2, 0);
        break;
    case 11:
        pl_act_meditate(self, 0);
        break;
    case 13:
        pl_act_meditate_end(self, 0);
        break;
    case 14:
        pl_act_kneel_gesture(self, 0, 1);
        break;
    case 15:
        pl_act_kneel_gesture(self, 1, 1);
        break;
    case 16:
        pl_act_kneel_gesture(self, 2, 1);
        break;
    case 17:
        pl_act_kneel_gesture(self, 3, 1);
        break;
    case 18:
        pl_act_gesture_once(self, 0, 1);
        break;
    case 19:
        pl_act_ground_sit(self, 1);
        break;
    case 26:
        pl_act_stand_up(self, 1);
        break;
    case 20:
        pl_act_kneel_gesture(self, 4, 1);
        break;
    case 21:
        pl_act_stretch(self, 1);
        break;
    case 22:
        pl_act_gesture_once(self, 1, 1);
        break;
    case 23:
        pl_act_kneel_gesture(self, 5, 1);
        break;
    case 24:
        pl_act_gesture_once(self, 2, 1);
        break;
    case 25:
        pl_act_meditate(self, 1);
        break;
    case 27:
        pl_act_meditate_end(self, 1);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* Refreshes the carry latch `_PLW`+0x5C4: for the master outside the drop acts, its high nibble takes the carried
 * item's bonus class (cleared when nothing is carried); its low nibble is always the carry mode 4. */
extern "C" void pl_carry_latch_refresh(_PLW* self) {
    u16 item;

    if (Pl_master_ck(self) == 1 && Pl_act_ck(self, 0xC, 0xA) == 0 && Pl_act_ck(self, 0xC, 0xB) == 0) {
        item = pl_carry_item_get(self);
        if (item != 0xFFFF) {
            self->field_0x5C4 = item_bonus_class_get(item) << 4;
        } else if (Pl_motion_input_ck(1) == 1) {
            self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 4;
        } else {
            self->field_0x5C4 = 0;
        }
    }
    self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 4;
}

/* The carry-idle act (`variant` 1 the blended start, 2 the move-state hand-over): arms the carry idle with its step
 * table. */
extern "C" void pl_act_carry_idle(_PLW* self, s32 variant) {
    pl_carry_latch_refresh(self);
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        switch (variant) {
        case 0:
            Pl_act_set_step_table(self, (u32)pl_carry_steps, 0);
            break;
        case 1:
            Pl_act_set_step_table(self, (u32)pl_carry_steps, 30);
            break;
        case 2:
            ef_move_state_dispatch(0);
            break;
        }
        self->field_0x266 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x33, 4, 0);
        pl_act_clear_flag5bb(self);
    }
}

/* The carry-walk act: walks with the carried item two units a frame while the master holds the walk input, and
 * returns to the carry idle when it ends. */
extern "C" void pl_act_carry_walk(_PLW* self) {
    VEC3 step;

    VEC3_ctor(&step);
    pl_carry_latch_refresh(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x266 = 30;
        Pl_chr_set_attr_default(self, 0x34, 4, 0);
        pl_act_set_frame_timer(self);
        pl_act_set_flag(self, 0x40);
        Pl_act_set_step_table(self, (u32)pl_carry_steps, 20);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                if (pl_carry_light_ck(self) == 1) {
                    step.z = pl_frame_window_2;
                    step.x = pl_float_zero;
                    step.y = pl_float_zero;
                    rotVecY(&step, self->rot_0x54.y);
                    self->vec_0x03C.x += step.x;
                    self->vec_0x03C.z += step.z;
                }
            } else {
                pl_act_enter(self, 0xC, 0, 0);
            }
        }
        break;
    }
}

/* The carry-run act: runs with the carried item five units a frame; the master jumps off a ledge ahead, drops the
 * item when the stamina is gone, and returns to the carry idle when the input ends. */
extern "C" void pl_act_carry_run(_PLW* self) {
    VEC3 step;

    VEC3_ctor(&step);
    pl_carry_latch_refresh(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 0x35, 4, 0);
        } else {
            Pl_chr_setX(self, 0x3B, 4, 0);
        }
        self->field_0x266 = 30;
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 0x180);
        pl_act_arm_flags(self, 1);
        Pl_act_set_step_table(self, (u32)pl_carry_steps, 20);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_master_ck(self) == 0) {
            break;
        }
        if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) != 0) {
            self->field_0x0A8 = (u16)self->rot_0x54.y;
            pl_act_enter(self, 0xC, 5, 0);
            break;
        }
        pl_guard_break_apply(self);
        if ((s16)self->field_0x378 <= 0) {
            pl_act_enter(self, 0xC, 4, 0);
        } else if (pl_act_param_tier_ck(self, 0) >= 1) {
            pl_act_set_cam_ang(self);
            if (pl_carry_light_ck(self) == 1) {
                step.z = pl_frame_window_5;
                step.x = pl_float_zero;
                step.y = pl_float_zero;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.z += step.z;
            }
        } else {
            pl_act_enter(self, 0xC, 0, 0);
        }
        break;
    }
}

/* The carry-drop act for `variant` 0-2 (1 the quick drop, 2 with the gauge held): sets the item down at the motion's
 * drop frame, clearing the carry latch and, for the master's delivery, crediting the carried item. */
extern "C" void pl_act_carry_drop(_PLW* self, s32 variant) {
    f32 frame;
    u16 item;

    if (variant == 2) {
        pl_act_set_gauge_arm(self, 2);
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 1) {
            Pl_chr_set_attr_default(self, 0x37, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x36, 4, 0);
        }
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (variant == 1) {
            frame = pl_frame_window_4;
        } else {
            frame = pl_frame_window_114;
        }
        if (Pl_frame_check(self, 0, frame, pl_float_zero) == 1) {
            self->act_step_0x05++;
            self->field_0x5C4 &= 0xF0;
            if (variant != 2) {
                eft_pl_weapon_follow_spawn((struct Plw*)self);
            }
            if (variant == 0 && Pl_master_ck(self) == 1) {
                item = pl_carry_item_get(self);
                if (item != 0xFFFF) {
                    pl_model_state_set(self, 2, 0x15, item);
                    pl_equip_slot_count_get(self, item);
                }
            }
        } else {
            pl_carry_latch_refresh(self);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            if (variant == 2) {
                pl_act_set_gauge_arm(self, 0);
            }
            Pl_act_set_motion_slot(self, 0, 8, 0);
        }
        break;
    }
}

/* The carry-jump act (`variant` 1 the drop off a ledge): leaps with the carried item and, on landing, breaks it after
 * a long fall (unless cat skill 0x24 cushions it) or lands with the carry kept. */
extern "C" void pl_act_carry_jump(_PLW* self, s32 variant) {
    VEC3 move;
    s32 breakAt;
    s32 rollAt;

    VEC3_ctor(&move);
    pl_carry_latch_refresh(self);
    self->field_0x28++;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 2, 0, 0);
        if (variant != 1) {
            Pl_chr_set_attr_default(self, 0x38, 0, 0);
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x80 = pl_frame_window_20;
            self->motion_vel_0x7C = pl_frame_window_18;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x8C = -(pl_frame_window_20 / pl_frame_window_80);
            pl_jump_gravity_set(self, 0x13);
        } else {
            Pl_chr_set_attr_default(self, 0x38, 8, 0x20);
            self->field_0x585 = 0;
            self->motion_vel_0x78 = pl_float_zero;
            self->motion_vel_0x80 = pl_frame_window_8;
            self->motion_vel_0x7C = pl_float_neg10;
            self->motion_acc_0x84 = pl_float_zero;
            self->motion_acc_0x8C = pl_float_zero;
            self->motion_acc_0x88 = pl_float_neg0_72727;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        self->field_0x007 = 0;
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        self->motion_vel_0x78 += self->motion_acc_0x84;
        self->motion_vel_0x7C += self->motion_acc_0x88;
        self->motion_vel_0x80 += self->motion_acc_0x8C;
        if (self->motion_vel_0x78 < pl_float_zero) {
            self->motion_vel_0x78 = pl_float_zero;
        }
        if (self->motion_vel_0x80 < pl_float_zero) {
            self->motion_vel_0x80 = pl_float_zero;
        }
        move.x = self->motion_vel_0x78;
        move.y = self->motion_vel_0x7C;
        move.z = self->motion_vel_0x80;
        rotVecY(&move, self->rot_0x54.y);
        self->vec_0x03C.x += move.x;
        self->vec_0x03C.y += move.y;
        self->vec_0x03C.z += move.z;
        if (pl_zone_ground_drop(self) == 1 || self->motion_vel_0x7C >= pl_float_zero) {
            break;
        }
        self->field_0x28++;
        if (self->vec_0x03C.y < self->ground_y_0x060) {
            self->vec_0x03C.y = self->ground_y_0x060;
            self->field_0x01B = 1;
            self->kind_0x09 = 0;
            self->field_0x585 = 0;
            if (Pl_cat_skill_ck(self, 0x24) == 1) {
                breakAt = 50;
                rollAt = 30;
            } else if (pl_carry_light_ck(self) == 1) {
                breakAt = 60;
                rollAt = 40;
            } else {
                breakAt = 50;
                rollAt = 30;
            }
            if (self->field_0x28 >= breakAt && Pl_cat_skill_ck(self, 0x24) == 0) {
                self->field_0x5C4 &= 0xF0;
                eft_pl_weapon_follow_spawn((struct Plw*)self);
                pl_act_enter(self, 2, 2, 0);
            } else if (self->field_0x28 >= rollAt) {
                pl_act_enter(self, 0xC, 7, 2);
            } else {
                pl_act_enter(self, 0xC, 8, 2);
            }
        }
        break;
    }
}

/* The carry-stagger act (`variant` 1 from behind): staggers with the carried item kept and the hold notice raised,
 * then returns to the carry idle. */
extern "C" void pl_act_carry_stagger(_PLW* self, s32 variant) {
    pl_carry_latch_refresh(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (variant == 0) {
            Pl_chr_set_attr_default(self, 0x39, 0, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x3A, 0, 0);
        }
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_hold_notify(self, 1);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0xC, 0, 2);
        }
        break;
    }
}

/* The carry-hand-over act: holds the item up for the talk timer until it is taken, sets it down, clears the carry
 * latch, then waits out the talk before returning to the idle slot. */
extern "C" void pl_act_carry_hand_over(_PLW* self) {
    pl_act_set_gauge_arm(self, 3);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_carry_latch_refresh(self);
        Pl_chr_setX(self, 0x33, 4, 0);
        self->talk_wait_0x668 = 60;
        self->field_0x28 = 60;
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        pl_carry_latch_refresh(self);
        if (pl_carry_item_get(self) == 0xFFFF) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x36, 4, 0);
        }
        break;
    case 2:
        if (Pl_frame_check(self, 0, pl_frame_window_114, pl_float_zero) != 0) {
            self->act_step_0x05++;
            self->field_0x5C4 &= 0xF0;
        } else {
            pl_carry_latch_refresh(self);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 1, 8, 0);
        }
        break;
    case 4:
        if (self->talk_wait_0x668 == 0) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    }
}

/* The carry-break act: drops and breaks the carried item (crediting the broken item 0x17D, a quake and the burst
 * shell), then returns to the idle slot. */
extern "C" void pl_act_carry_break(_PLW* self) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0xD3, 4, 0);
        pl_act_set_frame_timer(self);
        self->field_0x5C4 &= 0xF0;
        pl_equip_slot_count_get(self, 0x17D);
        pl_act_hold_notify(self, 3);
        camera_pl_quake_req(self, 0);
        shell_set_func_ptr->method_0x78(self, 2, shell_set_func_ptr);
        pl_act_clear_flag5bb(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD4, 2, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* The carry-act dispatcher: sheathes and runs the carry act's step handler; an unknown act falls back to the idle
 * slot. */
extern "C" void pl_act_group12_dispatch(_PLW* self) {
    self->field_0x18 = 0;
    switch (self->act_no) {
    case 0:
        pl_act_carry_idle(self, 0);
        break;
    case 1:
        pl_act_carry_walk(self);
        break;
    case 2:
        pl_act_carry_run(self);
        break;
    case 3:
        pl_act_carry_drop(self, 0);
        break;
    case 4:
        pl_act_carry_drop(self, 1);
        break;
    case 5:
        pl_act_carry_jump(self, 0);
        break;
    case 6:
        pl_act_carry_jump(self, 1);
        break;
    case 7:
        pl_act_carry_stagger(self, 0);
        break;
    case 8:
        pl_act_carry_stagger(self, 1);
        break;
    case 9:
        pl_act_carry_idle(self, 1);
        break;
    case 10:
        pl_act_carry_drop(self, 2);
        break;
    case 11:
        pl_act_carry_hand_over(self);
        break;
    case 12:
        pl_act_carry_break(self);
        break;
    case 13:
        pl_act_carry_idle(self, 2);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The cart-away act: lifts the fainted actor onto the cart (the master's camera follows) and drags it ten units a
 * frame for 90 frames, then enters the cart-arrival act. */
extern "C" void pl_act_cart_away(_PLW* self) {
    VEC3 drag;

    VEC3_ctor(&drag);
    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    self->field_0x001 = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_chr_set_attr_default(self, 0xDD, 0, 0);
        self->field_0x28 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        if (Pl_master_ck(self) == 1) {
            camera_mode3_req();
        }
        self->field_0x06C = pl_frame_window_40;
        break;
    case 1:
        if (++self->field_0x28 > 90) {
            pl_act_enter(self, 9, 1, 2);
            break;
        }
        drag.x = pl_float_zero;
        drag.y = pl_float_zero;
        drag.z = pl_frame_window_10;
        rotVecY(&drag, self->rot_0x54.y);
        addVec3To(&self->vec_0x03C, &drag);
        break;
    }
}

/* The cart-arrival act: drops the actor from the cart onto the camp ground, plays the wake-up and returns to the idle
 * slot. */
extern "C" void pl_act_cart_arrive(_PLW* self) {
    f32 ground;

    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0xDE, 0, 0);
        Pl_act_set_motion(self, 2, 0, 0);
        self->vec_0x03C.y = pl_frame_window_20 + self->ground_y_0x060;
        self->field_0x06C = pl_float_zero;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_28, pl_float_zero) != 0) {
            self->vec_0x03C.y -= pl_frame_window_4;
        }
        ground = self->ground_y_0x060;
        if (self->vec_0x03C.y <= ground) {
            self->vec_0x03C.y = ground;
        }
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD1, 0, 0x46);
            Pl_act_set_motion(self, 0, 0, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD8, 0, 0);
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* The respawn-pose act (`variant` 1 the drawn pose): sheathes into the pose with the respawn latch raised, then
 * returns to the kind's slot. */
extern "C" void pl_act_respawn_pose(_PLW* self, s32 variant) {
    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    pl_act_clear_flag5bb(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x077 = 1;
        self->field_0x18 = 0;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x133, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x162, 4, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* The camp-rest act (`variant` 1 kneeling): sits down at camp with the respawn latch raised and the master's camera
 * reset. */
extern "C" void pl_act_camp_rest(_PLW* self, s32 variant) {
    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    pl_act_clear_flag5bb(self);
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        self->field_0x077 = 1;
        self->field_0x18 = 0;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x1F7, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x20B, 4, 0);
        }
        if (Pl_master_ck(self) == 1) {
            camera_mode2_req();
        }
    }
}

/* The leave-sleeping act (`variant` 1 the drawn one): falls asleep with the lobby latches cleared and, for a remote
 * player, drops the slot after 300 frames. */
extern "C" void pl_act_leave_asleep(_PLW* self, s32 variant) {
    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    pl_camera_lock_clear(self);
    pl_act_clear_flag5bb(self);
    self->field_0x5BC = 0;
    self->quest_menu_open_0x5BD = 0;
    self->talk_wait_0x668 = 0;
    self->take_menu_open_0x5BE = 0;
    pl_cond_flags_clear(self, 0x800);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x077 = 1;
        self->field_0x28 = 300;
        self->field_0x18 = 0;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x15, 4, 0);
            self->vec_0x03C.y = self->ground_y_0x060;
        } else {
            self->act_step_0x05 = 2;
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x7A, 0, 0x56);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 5, 0, 0x92);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 0 && --self->field_0x28 < 0) {
            self->act_step_0x05++;
        }
        break;
    case 3:
        self->slot_active = 0;
        self->field_0x001 = 0;
        break;
    }
}

/* The leave-fainted act (`variant` 1 the drawn one): collapses with the lobby latches cleared and, for a remote
 * player, drops the slot after 300 frames. */
extern "C" void pl_act_leave_fainted(_PLW* self, s32 variant) {
    pl_act_set_step_time(self, 10);
    pl_act_set_gauge_arm(self, 10);
    pl_camera_lock_clear(self);
    pl_act_clear_flag5bb(self);
    self->field_0x5BC = 0;
    self->quest_menu_open_0x5BD = 0;
    self->talk_wait_0x668 = 0;
    self->take_menu_open_0x5BE = 0;
    pl_cond_flags_clear(self, 0x800);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x077 = 1;
        self->field_0x28 = 300;
        self->field_0x18 = 0;
        if (variant == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0xD2, 4, 0);
            self->vec_0x03C.y = self->ground_y_0x060;
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x103, 4, 0);
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 0 && --self->field_0x28 < 0) {
            self->act_step_0x05++;
        }
        break;
    case 2:
        self->slot_active = 0;
        self->field_0x001 = 0;
        break;
    }
}

/* The cart and leave dispatcher: drops the carry latch and runs the cart, respawn and leave act's step handler; an
 * unknown act falls back to the idle slot. */
extern "C" void pl_act_group9_dispatch(_PLW* self) {
    pl_act_clear_mode5c4(self);
    switch (self->act_no) {
    case 0:
        pl_act_cart_away(self);
        break;
    case 1:
        pl_act_cart_arrive(self);
        break;
    case 2:
        pl_act_respawn_pose(self, 0);
        break;
    case 3:
        pl_act_camp_rest(self, 0);
        break;
    case 4:
        pl_act_leave_asleep(self, 0);
        break;
    case 5:
        pl_act_leave_fainted(self, 0);
        break;
    case 6:
        pl_act_respawn_pose(self, 1);
        break;
    case 7:
        pl_act_camp_rest(self, 1);
        break;
    case 8:
        pl_act_leave_asleep(self, 1);
        break;
    case 9:
        pl_act_leave_fainted(self, 1);
        break;
    default:
        Pl_act_set_motion_slot(self, 0, 4, 0);
        break;
    }
}

/* The act-class dispatcher: clears the entry latch and runs the dispatcher of the actor's act class `_PLW`+0x00A;
 * class 4 (the weapon acts) picks the weapon kind's own dispatcher, or the kick acts from act 100 on. */
extern "C" void fn_8025DE38(_PLW* self) {
    self->field_0x00E = 0;
    switch (self->field_0x00A) {
    case 0:
        pl_act_group0_dispatch(self);
        break;
    case 1:
        pl_act_step_arm_hold(self);
        break;
    case 2:
        pl_act_group2_dispatch(self);
        break;
    case 4:
        if (self->act_no >= 100) {
            pl_act_kick_dispatch(self);
            break;
        }
        switch (self->field_0x002) {
        case 0:
            pl_weapon0_act_dispatch(self);
            break;
        case 1:
            pl_weapon1_act_dispatch(self);
            break;
        case 2:
            pl_weapon2_act_dispatch(self);
            break;
        case 3:
            pl_weapon3_act_dispatch(self);
            break;
        case 4:
            pl_weapon456_act_dispatch(self);
            break;
        case 5:
            pl_weapon456_act_dispatch(self);
            break;
        case 6:
            pl_weapon456_act_dispatch(self);
            break;
        case 7:
            pl_act_class7_dispatch(self);
            break;
        case 8:
            Pl_act_frame_dispatch(self);
            break;
        }
        break;
    case 5:
        pl_act_group5_dispatch(self);
        break;
    case 6:
        pl_act_group6_dispatch(self);
        break;
    case 7:
        pl_act_group7_dispatch(self);
        break;
    case 8:
        pl_act_group8_dispatch(self);
        break;
    case 10:
        pl_act_group10_dispatch(self);
        break;
    case 11:
        pl_act_group11_dispatch(self);
        break;
    case 9:
        pl_act_group9_dispatch(self);
        break;
    case 12:
        pl_act_group12_dispatch(self);
        break;
    }
}

/* 0x8025DEEC (0x8C): Answers 1 when any of the 26 pouch slots holds a usable item. */
extern "C" u32 pl_item_usable_ck(_PLW* self) {
    s32 i;
    for (i = 0; i < 26; i++) {
        PlItemData* item = (PlItemData*)GetItemData(self->slot_id[i].item_id);
        if (self->slot_id[i].item_id != 0 && self->slot_id[i].value > 0 && (item->field_0x002 & 8) != 0 &&
            item->kind_0x00 != 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8025DF78 (0x150): Returns the next (or, with `backward`, the previous) pouch slot after `cur` whose item is
 * usable, or 0 when none is. */
extern "C" u16 pl_item_slot_cycle(_PLW* self, u16 cur, u8 backward) {
    BOOL found = FALSE;
    u32 n;
    u16 slot;
    u16 item_id;
    PlItemData* item;

    if (backward == 0) {
        for (n = 0, slot = cur; n < 26; n++) {
            slot++;
            if (slot >= 26) {
                slot = 0;
            }
            if (self->slot_id[slot].value > 0) {
                item_id = self->slot_id[slot].item_id;
                item = (PlItemData*)GetItemData(item_id);
                if (item_id != 0 && (item->field_0x002 & 8) != 0 && item->kind_0x00 != 1) {
                    cur = slot;
                    found = TRUE;
                    break;
                }
            }
        }
    } else {
        for (n = 0, slot = cur; n < 26; n++) {
            if (slot == 0) {
                slot = 25;
            } else {
                slot--;
            }
            if (self->slot_id[slot].value > 0) {
                item_id = self->slot_id[slot].item_id;
                item = (PlItemData*)GetItemData(item_id);
                if (item_id != 0 && (item->field_0x002 & 8) != 0 && item->kind_0x00 != 1) {
                    cur = slot;
                    found = TRUE;
                    break;
                }
            }
        }
    }
    if (!found) {
        cur = 0;
    }
    return cur;
}

/* 0x8025E0C8 (0x17C): Runs the pouch-select input: opens the item window on id bit 11 and steps the selected slot
 * forward or back on bits 15 and 14; closes the window while the actor cannot use items. */
extern "C" void fn_8025E0C8(_PLW* self) {
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (Pl_motion_input_ck(1) != 1 && !pl_item_window_blocked(self)) {
        if ((u32)pl_id_bit_ck(self, 11) == 1) {
            if (self->field_0x308 == 0) {
                sysSE_req(5);
            }
            self->field_0x308 = 1;
            if (pl_item_usable_ck(self) != 0) {
                if ((u32)pl_id_bit_ck(self, 15) == 1) {
                    sysSE_req(6);
                    self->field_0x304 = pl_item_slot_cycle(self, self->field_0x304, 0);
                    self->field_0x309 |= 2;
                } else if ((u32)pl_id_bit_ck(self, 14) == 1) {
                    sysSE_req(6);
                    self->field_0x304 = pl_item_slot_cycle(self, self->field_0x304, 1);
                    self->field_0x309 |= 4;
                }
            }
        } else if (self->field_0x308 != 0) {
            sysSE_req(1);
            self->field_0x308 = 0;
        }
    } else {
        self->field_0x308 = 0;
    }
}

/* Tests the actor's action number against the handful of actions the held-item window accepts. */
extern "C" u32 fn_8025E244(_PLW* self)
{
    u16 action;

    if (self->field_0x00A != 0) {
        return 0;
    }
    action = self->act_no;
    if ((u32)(action - 0x31) <= 3 || (u32)(action - 0x61) <= 2 || (u32)(action - 0x2C) <= 1 ||
        (s32)action == 0x7F) {
        return 1;
    }
    return 0;
}

/* Scans the `field_0x318` table for the entry whose id/key pair the caller asks for, honouring the
 * entry's two suppression flags. */
extern "C" u32 fn_8025E298(_PLW* self, u16 a2, u16 a3)
{
    PlScanEntry* entry;

    entry = (PlScanEntry*)self->field_0x318;
    if (entry == NULL || (s8)self->field_0x313 > 0) {
        return 0;
    }
    while (entry->id_0x00 != 0xFF) {
        if ((((entry->flags_0x06 & 0x8000) == 0) || ((entry->flags_0x0E & 0x4000) == 0) ||
             (s8)self->field_0x314 <= 0) &&
            entry->id_0x00 == a2 && entry->key_0x02 == a3) {
            return 1;
        }
        entry++;
    }
    return 0;
}

/* Arms the held-item stance when the actor stands still and the item pair is in the scan table. */
extern "C" u32 fn_8025E32C(_PLW* self)
{
    if (self->field_0x002 == 1) {
        if (self->field_0x308 != 0) {
            return 0;
        }
    } else if (self->field_0x18 == 0 && self->field_0x308 != 0) {
        return 0;
    }
    if ((fn_8025E298(self, 0, 0xB1) == 1 || fn_8025E298(self, 0xC, 0xB) == 1) &&
        fn_8027D8A0(self, 1) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    if (fn_8025E298(self, 0, 0xA6) == 1 && fn_8027D8A0(self, 0) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    return 0;
}

/* Picks the master's action prompt (`_PLW`+0x3E4/+0x3E5): none inside the hit and carted act classes, the held
 * item's prompt while one is held, otherwise the prompt of what the actor faces - a gathering spot, a carried egg's
 * goal, a ledge, the roster area kind it stands in - delayed six frames when it changes to a gather prompt. */
extern "C" void fn_8025E448(_PLW* self) {
    VEC3 spot;
    VEC3 ledge;
    s32 index;
    s32 found;
    s16 kind;
    _CP_VECTOR rot;
    s32 near;
    u32 prompt;
    u8 icon;

    VEC3_ctor(&spot);
    if (Pl_master_ck(self) == 0) {
        self->field_0x3E4 = 0;
        self->field_0x3E7 = 0;
        return;
    }
    if ((s8)self->field_0x3E7 > 0) {
        self->field_0x3E7--;
    }
    self->field_0x3E6 = self->field_0x3E5;
    icon = 0;
    prompt = 0;
    self->field_0x3E8 = 0;
    self->field_0x3E9 = 0;
    if ((u32)(self->field_0x00A - 5) <= 4 || self->field_0x00A == 11) {
        if (self->field_0x308 != 0) {
            self->field_0x3E5 = 7;
            self->field_0x3E4 = 7;
            self->field_0x3E7 = 0;
            self->field_0x3E8 = 0;
            return;
        }
        self->field_0x3E4 = 0;
        self->field_0x3E7 = 0;
        return;
    }
    if (self->field_0x18 == 1) {
        if (self->field_0x002 != 1) {
            self->field_0x3E4 = 0;
            self->field_0x3E7 = 0;
            fn_8025E32C(self);
            return;
        }
        if (fn_8025E32C(self) == 1) {
            return;
        }
        self->field_0x3E8 = 1;
        if (self->field_0x308 != 0) {
            self->field_0x3E5 = 7;
            self->field_0x3E4 = 7;
            self->field_0x3E7 = 0;
            self->field_0x3E8 = 0;
            return;
        }
    } else {
        if (pl_act_no_68_71_ck(self) == 1) {
            self->field_0x3E4 = 0;
            self->field_0x3E7 = 0;
            return;
        }
        if (self->field_0x002 <= 1) {
            if (fn_8025E32C(self) == 1) {
                return;
            }
            if (self->field_0x128 == 1 && self->field_0x110 <= pl_float_neg70 && self->field_0x308 == 0) {
                self->field_0x3E9 = 1;
            }
        }
    }
    if (self->field_0x3E8 != 0) {
        self->field_0x3E4 = 0;
        self->field_0x3E7 = 0;
        return;
    }
    if (fn_8025E32C(self) == 1) {
        return;
    }
    if (quest_move_sub_flag_get() != 0 && (fn_8025E298(self, 0, 0xAF) == 1 || fn_8025E298(self, 0xC, 0xD) == 1)) {
        prompt = 0xE;
        icon = 0xE;
    }
    near = pl_stg_weapon_near_find(self, &spot, &found, &index);
    switch (near) {
    case 2:
    case 4:
        if (fn_8025E298(self, 0x80, 0xF6) == 1) {
            self->field_0x3E9 = 0;
            if (near == 2) {
                prompt = 0xF;
                icon = 0xF;
            } else {
                prompt = 0x11;
                icon = 0x11;
            }
        }
        break;
    case 3:
        if (fn_8025E298(self, 0xC, 0xC) == 1 && pl_carry_item_get(self) == 0x17D) {
            prompt = 0x12;
            icon = 0x12;
        }
        break;
    }
    if (self->field_0x369 != 0 &&
        (self->field_0x00A != 0 ||
         ((u32)(self->act_no - 0xA) > 4 && (u32)(self->act_no - 0x4A) > 4 && (self->act_no < 0x66 || self->act_no > 0x69))) &&
        (fn_8025E298(self, 0, 0xA) == 1 || fn_8025E298(self, 0, 0x4A) == 1)) {
        prompt = 6;
        icon = 6;
    }
    if (pl_ground_probe_ahead(self, &ledge, pl_frame_window_60, pl_float_240) == 1 &&
        (fn_8025E298(self, 0, 5) == 1 || fn_8025E298(self, 0, 6) == 1 || fn_8025E298(self, 2, 4) == 1)) {
        prompt = 6;
        icon = 6;
    }
    if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &kind, &spot, &rot, &index) != 0xFFFF) {
        switch (kind) {
        case 3:
            prompt = 2;
            if (fn_8025E244(self) == 1 || fn_8025E298(self, 0x80, 0xFE) == 1 || fn_8025E298(self, 0x80, 0xFD) == 1) {
                icon = 2;
            }
            break;
        case 2:
            prompt = 0xA;
            if (fn_8025E244(self) == 1 || fn_8025E298(self, 0x80, 0xFE) == 1) {
                icon = 0xA;
            }
            break;
        case 4:
            prompt = 3;
            if (fn_8025E244(self) == 1 || fn_8025E298(self, 0x80, 0xFE) == 1 || fn_8025E298(self, 0x80, 0xFD) == 1) {
                icon = 3;
            }
            break;
        case 0:
            prompt = 1;
            if (fn_8025E244(self) == 1 || fn_8025E298(self, 0x80, 0xFE) == 1) {
                icon = 1;
            }
            break;
        case 5:
            if (self->kind_0x09 == 3) {
                prompt = 0;
                icon = 0;
            } else if (fn_8025E298(self, 0x80, 0xFE) == 1 || fn_8025E298(self, 0x80, 0xFD) == 1) {
                prompt = 4;
                icon = 4;
            }
            break;
        case 8:
            if (pl_item_menu_open_ck(self) != 1 &&
                (self->field_0x00A != 0 || ((u32)(self->act_no - 0x54) > 2 && self->act_no != 0x46))) {
                prompt = 8;
                if (fn_8025E298(self, 0x80, 0xFE) == 1) {
                    icon = 8;
                }
            } else {
                prompt = 0;
                icon = 0;
            }
            break;
        case 6:
            if (PlayMode_ck() != 2 && pl_item_menu_open_ck(self) != 1 && Pl_act_ck(self, 0, 0x57) != 1) {
                prompt = 5;
                if (fn_8025E298(self, 0x80, 0xFE) == 1) {
                    icon = 5;
                }
            } else {
                prompt = 0;
                icon = 0;
            }
            break;
        case 10:
            if (pl_item_menu_open_ck(self) != 1 && Pl_motion_input_ck(0) != 1 && Pl_act_ck(self, 0, 0x58) != 1 &&
                Pl_act_ck(self, 0xC, 0xA) != 1) {
                prompt = 9;
                if (fn_8025E298(self, 0x80, 0xFE) == 1) {
                    icon = 9;
                }
                if (fn_8025E298(self, 0xC, 0xA) == 1) {
                    icon = 9;
                }
            } else {
                prompt = 0;
                icon = 0;
            }
            break;
        case 16:
            prompt = 0xB;
            icon = 0xB;
            break;
        case 17:
            prompt = 0xC;
            icon = 0xC;
            break;
        case 7:
            prompt = 0xD;
            icon = 0xD;
            break;
        case 1:
        case 9:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        default:
            prompt = 0;
            icon = 0;
            break;
        }
    }
    if (self->field_0x308 != 0) {
        self->field_0x3E5 = 7;
        self->field_0x3E4 = 7;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return;
    }
    self->field_0x3E5 = icon;
    if (self->field_0x3E4 != prompt) {
        if (self->field_0x3E6 != icon) {
            if ((u32)(icon - 1) > 3) {
                self->field_0x3E7 = 0;
            } else {
                self->field_0x3E7 = 6;
            }
        }
        if (self->field_0x3E4 == 0 && icon != 0) {
            if ((s8)self->field_0x3E7 == 0) {
                self->field_0x3E4 = self->field_0x3E5;
            }
        } else if (self->field_0x3E4 != icon && (s8)self->field_0x3E7 == 0) {
            self->field_0x3E4 = self->field_0x3E5;
        }
    }
}

/* 0x8025EBF0 (0x68): Raises the stagger flag, spawns the stagger effect below budget and refills the stagger. */
extern "C" void pl_act_stagger_fill(_PLW* self) {
    self->field_0x65D |= 1;
    if (self->field_0x37E < self->field_0x380 && (self->field_0x65C & 1) == 0) {
        ef_inst_spawn(self, 3);
    }
    pl_act_add_stagger(self, 150);
}

/* 0x8025EC58 (0x98): Staggers the actor when it stands in a roster area of kind 1, in water or within an NPC's reach. */
extern "C" void fn_8025EC58(_PLW* self) {
    s16 kind;
    s32 word;
    VEC3 pos;
    struct _CP_VECTOR rot;
    VEC3_ctor(&pos);
    if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &kind, &pos, &rot, &word) != 0xFFFF &&
        kind == 1) {
        pl_act_stagger_fill(self);
    } else if (Pl_suimen_ck(self) == 1 || (u32)ai_npc_player_in_reach_ck() == 1) {
        pl_act_stagger_fill(self);
    }
}

/* Toggles the act's boolean latch. */
extern "C" void fn_8025ECF0(_PLW* self)
{
    self->field_0x5E6 = self->field_0x5E6 ^ 1;
}

/* Keeps the master's alternate-form latch `_PLW`+0x5E6 in step with the drawn weapon and the act (outside act
 * classes 6-8), and drops the camera lock with it. */
extern "C" void fn_8025ED00(_PLW* self) {
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((u8)(self->field_0x00A + 250) > 2) {
        if (self->field_0x18 == 1 && pl_item_page2_ck(self) != 0 && pl_part_flag2_ck(self, 7) == 1) {
            if (self->field_0x00A == 0) {
                switch (self->act_no) {
                case 6:
                case 7:
                    self->field_0x5E6 = 0;
                    break;
                default:
                    fn_8025ECF0(self);
                    break;
                case 0x1:
                case 0x2:
                case 0x8:
                case 0xF:
                case 0x10:
                case 0x14:
                case 0x15:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x1C:
                case 0x49:
                case 0x51:
                case 0x52:
                case 0x5B:
                case 0x64:
                case 0x65:
                case 0x7A:
                case 0x7B:
                case 0x86:
                case 0x8C:
                case 0x8D:
                case 0x8E:
                case 0x9E:
                case 0x9F:
                case 0xA0:
                case 0xA1:
                case 0xB5:
                case 0xB6:
                case 0xB7:
                case 0xB8:
                    break;
                }
            } else {
                switch (self->field_0x00A) {
                case 11:
                    break;
                case 4:
                    if ((u32)(self->act_no - 0x10) > 2 && (u32)(self->act_no - 2) > 1 && self->act_no != 0) {
                        fn_8025ECF0(self);
                    }
                    break;
                case 1:
                    if ((u32)(self->act_no - 0x14) <= 1) {
                        self->field_0x5E6 = 0;
                    } else {
                        fn_8025ECF0(self);
                    }
                    break;
                default:
                    fn_8025ECF0(self);
                    break;
                }
            }
            if (self->kind_0x09 == 2) {
                self->field_0x5E6 = 0;
            }
            if (pl_act_true_ck(self) == 0) {
                pl_camera_lock_clear(self);
            }
        } else if (self->field_0x18 == 0) {
            if (Pl_bari_ck(self, 1) == 1) {
                if (pl_part_flag_ck(self, 0x36) == 1) {
                    fn_8025ECF0(self);
                }
            } else if (pl_act_no_68_71_ck(self) == 1) {
                self->field_0x5E6 = 1;
            } else {
                pl_camera_lock_clear(self);
            }
        } else if (pl_item_page2_ck(self) == 0) {
            if (pl_act_no_68_71_ck(self) == 1) {
                self->field_0x5E6 = 1;
            } else {
                pl_camera_lock_clear(self);
            }
        }
    }
    if (self->field_0x5E6 == 0) {
        self->camera_lock_0x5E5 = 0;
    }
}

/* Whether the camera sits within 100 units of the actor's head while the act shows no effect. */
extern "C" u32 pl_camera_near_head_ck(_PLW* self) {
    VEC3 camera;
    VEC3 head;

    VEC3_ctor(&camera);
    VEC3_ctor(&head);
    if (pl_act_no_eft_ck(self, 0) == 1) {
        const VEC3& pos = get_camera_pos();
        copyVec3(&camera, &pos);
        pl_get_joint_wpos(self, 3, &head);
        if (vec3_dist_sq(&camera, &head) < pl_float_10000) {
            return 1;
        }
    }
    return 0;
}

/* 0x8025F088 - the actor's per-frame reset/update: latches the state bytes, runs the sub-updates and
 * pushes the body transform into the model layer. */
void fn_8025F088(_PLW* self) {
    MHchar* chr = &((PlBodyWork*)self->physics_0x13C)->chr_0x04;

    get_move_work_adrs(0);
    copyVec3(&self->vec_0x048, &self->vec_0x03C);

    self->field_0x65C = self->field_0x65D;
    if (self->field_0x017 != self->area_0x16) {
        self->field_0x65C |= 1;
    }
    self->field_0x65D = 0;
    self->field_0x01B = 0;
    self->field_0x017 = self->area_0x16;
    self->field_0x076 = self->field_0x075;
    self->field_0x276 = 0;
    self->field_0x3DC &= ~0x100000;

    pl_act_status_step(self);
    self->field_0x309 = 0;
    self->field_0x657 = 0;
    self->field_0x30A = 0;
    if (Pl_master_ck(self) == 1 && fn_8027E1E4(self) == 0) {
        fn_8025E0C8(self);
        fn_80279C20(self);
    }
    self->field_0x46C = self->field_0x46D;
    if (Pl_master_ck(self) == 1) {
        if (em_player_spotted_set(self->chunk_ofs) == 1) {
            self->field_0x46C = 1;
        } else {
            self->field_0x46C = 0;
            self->field_0x46E = 0;
        }
        if (Pl_act_ck(self, 2, 1) == 0 && Pl_act_ck(self, 0, 6) == 0) {
            self->field_0x5C7 = 0;
        }
    } else {
        self->field_0x46C = 0;
        self->field_0x46D = 0;
        self->field_0x46E = 0;
        self->field_0x5C7 = 0;
    }
    fn_8025EC58(self);
    if (pl_item_page2_ck(self) == 1) {
        pl_gun_guard_row_latch(self);
    }
    self->field_0x3B4 = 0;
    self->field_0x3B5 = 0;
    self->field_0x5C9 = 0;
    if (pl_hit_react(self) == 0) {
        pl_scan_act_select(self);
    }
    pl_gauge_step_a(self);
    pl_gauge_step_b(self);
    pl_condition_step(self);
    pl_condition_tick(self);
    self->field_0x3B0 = self->field_0x3AC;
    self->field_0x3AC = 0;
    self->act_state_0x00E[0] = 0;
    self->field_0x654 = 0;
    self->field_0x268 = 0;
    fn_8025DE38(self);
    if (self->act_state_0x00E[0] != 0) {
        self->act_state_0x00E[0] = 0;
        fn_8025DE38(self);
    }
    fn_8025E448(self);
    if (self->field_0x30A == 0xFF) {
        self->field_0x30A = 0;
    } else if (pl_item_menu_open_ck(self) == 0 &&
               (fn_8025E298_view3(self, 128, 255) == 1 || fn_8025E298_view3(self, 137, 248) == 1 ||
                fn_8025E298_view3(self, 137, 247) == 1) &&
               self->field_0x00A != 7 && self->field_0x00A != 12) {
        self->field_0x30A = 1;
    }
    self->field_0x5C8 = 0;
    fn_80278D1C(self);
    fn_80277EC0(self);
    fn_802602A0(self);
    fn_8027AF34(self);
    fn_802607C4(self);
    pl_model_pos_sync(self);
    fn_8025ED00(self);
    pl_rig_model_move(self->physics_0x13C);
    copyVec3(&self->vec_0x03C, &chr->pos_0x04);
    switch (self->kind_0x09) {
    case 2:
        break;
    case 3:
        if (self->vec_0x03C.y < pl_float_110 + self->ground_y_0x060) {
            vec3_move_damp(pl_float_0_6, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    default:
        if (self->field_0x01B == 0) {
            vec3_move_damp(pl_float_0_3, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    }
    fn_8025FF0C(self, 0);
    fn_8025FA00(self);
    if (Pl_act_ck(self, 0, 20) == 1 || Pl_act_ck(self, 0, 158) == 1) {
        self->field_0x268 = 1;
    }
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(chr);
    if (pl_now_area_ck(self) == 0) {
        fn_8027D4F0(self);
    }
    hit_attack_list_push((_HIT_W*)&self->field_0x484);
    hit_attack_list_push((_HIT_W*)&self->field_0x4E0);
}

/* 0x8025F478 - enters the water-surface (swim) state: reads three motion slots and latches the
 * resulting depth, state byte and the two water flags. */
void fn_8025F478(_PLW* self) {
    VEC3 work;
    f32 value;

    em_land_data_init(&work);
    if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 0;
        return;
    }
    if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 3;
        if (pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
            self->field_0x064 = value;
            self->field_0x074 = 1;
            if (self->vec_0x03C.y >= value - pl_frame_window_30) {
                self->vec_0x03C.y = value - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        } else {
            self->field_0x074 = 0;
        }
    }
}

/* 0x8025F588 - the player's water-surface state machine: reads the swim motion slots and drives the
 * actor between the standing / swimming / diving kinds. */
s32 fn_8025F588(_PLW* self) {
    VEC3 work;
    f32 value;

    em_land_data_init(&work);
    if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if ((self->field_0x5C4 & 0xF) != 0) {
                self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                pl_act_enter_raw(self, 12, 6, 0);
                return 1;
            }
            if (fn_80278C7C(self) == 1) {
                pl_act_enter_raw(self, 6, 49, 0);
                return 1;
            }
            if (fn_80278CD0(self) == 1) {
                pl_act_enter_raw(self, 6, 69, 0);
                return 1;
            }
            self->kind_0x09 = 0;
            break;
        case 3:
            self->kind_0x09 = 0;
            break;
        default:
            if (self->vec_0x03C.y - value > pl_frame_window_30) {
                if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        pl_act_enter_raw(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        pl_act_enter_raw(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        pl_act_enter_raw(self, 6, 69, 0);
                        return 1;
                    }
                    pl_act_enter(self, 2, 1, 0);
                    return 1;
                }
                if (self->field_0x00A == 8) {
                    self->vec_0x03C.y = value;
                }
                pl_act_enter(self, 2, 1, 0);
            }
            break;
        }
    } else if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                if (self->vec_0x03C.y <= value) {
                    self->kind_0x09 = 3;
                } else {
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        pl_act_enter_raw(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        pl_act_enter_raw(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        pl_act_enter_raw(self, 6, 69, 0);
                        return 1;
                    }
                }
            }
            break;
        default:
            self->kind_0x09 = 3;
            break;
        case 3:
            break;
        }
    }
    if ((u8)(self->kind_0x09 - 2) <= 1 &&
        pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        self->field_0x074 = 1;
        if (self->vec_0x03C.y >= pl_frame_window_200 + value) {
            self->field_0x075 = 0;
            self->kind_0x09 = 2;
            if ((self->field_0x5C4 & 0xF) != 0) {
                pl_act_enter(self, 12, 6, 0);
            } else if (fn_80278C7C(self) == 1) {
                pl_act_enter_raw(self, 6, 49, 0);
            } else if (fn_80278CD0(self) == 1) {
                pl_act_enter_raw(self, 6, 69, 0);
            } else {
                pl_act_enter(self, 2, 1, 0);
            }
            return 1;
        }
        if (self->vec_0x03C.y >= value - pl_frame_window_30) {
            self->vec_0x03C.y = value - pl_frame_window_30;
        }
        if (Pl_suimen_ck(self) == 1) {
            self->field_0x075 = 1;
        } else {
            self->field_0x075 = 0;
        }
    } else if ((u8)(self->kind_0x09 - 2) <= 1) {
        self->field_0x074 = 0;
    }
    return 0;
}

/* 0x8025FA00 - the second half of the player's water-surface state machine (the entry/exit side). */
void fn_8025FA00(_PLW* self) {
    VEC3 work;
    f32 value;

    em_land_data_init(&work);
    if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_master_ck(self) == 1 && self->field_0x00A != 8) {
                pl_act_enter(self, 1, 21, 12);
            }
            break;
        default:
            if (self->field_0x585 != 0) {
                break;
            }
            if (self->vec_0x03C.y - value <= pl_frame_window_30) {
                self->vec_0x03C.y = value;
                self->field_0x585 = 0;
                if (fn_8027DCA8(self) == 1 || (self->field_0x5A4 & 0x1000) == 0) {
                    break;
                }
                if (self->field_0x00A != 8) {
                    if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                        pl_ride_down_set(self);
                        return;
                    }
                    pl_act_enter(self, 6, 52, 0);
                } else {
                    self->field_0x06C -= pl_frame_window_2;
                    if (self->field_0x06C < pl_float_neg250) {
                        self->field_0x06C = pl_float_neg250;
                    } else {
                        self->vec_0x03C.z += pl_frame_window_10;
                    }
                }
                break;
            }
            if (self->field_0x370 > 0) {
                self->kind_0x09 = 2;
                if (self->field_0x00A != 12) {
                    pl_act_enter(self, 2, 1, 0);
                } else {
                    pl_act_enter(self, 12, 6, 0);
                }
            } else if (self->field_0x00A == 8) {
                self->vec_0x03C.y = value;
            }
            break;
        }
    } else if (pl_ground_check(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (Pl_master_ck(self) != 0 &&
                pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value && pl_act_no_eft_ck(self, 0) == 0) {
                    self->vec_0x03C.y = self->field_0x064;
                    self->kind_0x09 = 3;
                    if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                        pl_ride_down_set(self);
                    } else {
                        pl_act_enter(self, 0, 151, 12);
                    }
                }
            }
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_act_ck(self, 0, 151) == 0 &&
                       self->vec_0x03C.y < pl_frame_window_100 + self->ground_y_0x060) {
                self->vec_0x03C.y = pl_frame_window_100 + self->ground_y_0x060;
            }
            break;
        default:
            if (Pl_master_ck(self) != 0 &&
                pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value) {
                    if (self->field_0x00A != 8) {
                        self->kind_0x09 = 3;
                        pl_act_enter(self, 1, 20, 12);
                    }
                } else if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if (self->field_0x00A != 12) {
                        pl_act_enter(self, 2, 1, 0);
                    } else {
                        pl_act_enter(self, 12, 6, 0);
                    }
                }
            }
            break;
        }
    }
    if (pl_ground_check(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        if (self->kind_0x09 == 3) {
            self->field_0x074 = 1;
            if (self->field_0x00A == 0) {
                if ((u32)(self->act_no - 74) > 3 && (self->act_no < 104 || self->act_no > 105) &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->field_0x00A == 1) {
                if (self->act_no != 21 &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        }
    } else if (self->kind_0x09 == 3) {
        self->field_0x074 = 0;
    }
}

/* 0x8025FF0C - computes the action flag word for the current state, hands it to the action layer and returns 0. */
s32 fn_8025FF0C(_PLW* self, u8 mode) {
    u16 flags = 0x102;

    switch (self->kind_0x09) {
    default:
        if (pl_status_bit_ck((_ENEMY_WORK*)self, 4544) != 0) {
            if (self->field_0x00A == 0) {
                s32 act = self->act_no;
                if ((u32)(act - 128) <= 6 || (u32)(act - 15) <= 1 || act == 123 || act == 139 || act == 182) {
                    flags |= 0x40;
                } else {
                    flags |= 0xC0;
                }
            } else {
                flags |= 0xC0;
            }
        } else {
            flags |= 0x40;
        }
        break;
    case 2:
        if (self->field_0x00A == 6) {
            flags |= 0x40;
        } else if (Pl_act_ck(self, 0, 62) == 1) {
            flags |= 0x40;
        } else {
            flags |= 0xC0;
        }
        break;
    case 3:
        if (pl_status_bit_ck((_ENEMY_WORK*)self, 4544) != 0) {
            if (self->field_0x00A == 0) {
                s32 act = self->act_no;
                if ((u32)(act - 140) <= 9 || (u32)(act - 25) <= 1 || act == 124) {
                    flags |= 0x80;
                } else {
                    switch (act) {
                    case 27:
                        if (self->field_0x002 == 3 && self->field_0x18 == 1) {
                            flags |= 0x80;
                        } else {
                            flags |= 0xC0;
                        }
                        break;
                    default:
                        flags |= 0xC0;
                        break;
                    }
                }
            } else {
                flags |= 0xC0;
            }
        } else {
            flags |= 0x80;
        }
        break;
    }
    if (self->field_0x00A == 0 &&
        ((u32)(self->act_no - 10) <= 4 || (u32)(self->act_no - 74) <= 4 ||
         (u32)(self->act_no - 102) <= 3)) {
        flags &= 0xFEFF;
    }
    pl_coll_shape_set(self, mode, flags);

    self->field_0x369 = 0;
    if (self->field_0x364 != 0) {
        s32 i;

        for (i = 0; i < 10; i++) {
            u16 kind = pl_move_work[self->chunk_ofs][i].kind;

            if (kind == 0) {
                break;
            }
            if ((kind & 2) != 0 && (self->field_0x364 & 0xE0000007) != 0) {
                self->field_0x369 = 1;
            }
        }
    }
    return 0;
}

/* 0x80260198 - looks for an armed entry in the actor's own chunk of the move-work table and adopts
 * its position pair as the two damage counters. */
s32 fn_80260198(_PLW* self) {
    s32 i;

    if ((self->field_0x364 & 0xE0000007) != 0) {
        for (i = 0; i < 10; i++) {
            PlMoveEntry* entry = &pl_move_work[self->chunk_ofs][i];
            u16 kind = entry->kind;
            u32 value;

            if (kind == 0) {
                break;
            }
            value = calc_xz_angle(entry->x_0x08, entry->z_0x10, pl_float_zero, pl_float_zero);
            if ((kind & 2) != 0) {
                self->field_0x058 = value;
                self->field_0x0A8 = value;
                return 1;
            }
        }
    }
    return 0;
}

/* 0x80260248 - advances the actor's signed 16-bit lean value by `delta`, clamps it to +/-100 and
 * zeroes it when `mode` 1 asks for the sign to be cancelled. */
void fn_80260248(_PLW* self, s32 mode, s16 delta) {
    s16 old = self->field_0x648;

    self->field_0x648 += delta;
    if (self->field_0x648 <= -100) {
        self->field_0x648 = -100;
    } else if (self->field_0x648 >= 100) {
        self->field_0x648 = 100;
    }
    if (mode != 1) {
        return;
    }
    if ((s32)old * (s32)self->field_0x648 < 0) {
        self->field_0x648 = 0;
    }
}

/* Turns the actor toward its stored facing `_PLW`+0x0A8 and pitch +0x0AC at the turn rate of its act (slower while
 * guarding or locked on), leaning the body (`_PLW`+0x0B0) into the turn, and blends the run motions' lean pose. */
extern "C" void fn_802602A0(_PLW* self) {
    f32 lean;
    s32 yawRate;
    s32 pitchRate;
    s32 bank;
    u16 diff;
    u16 motion;
    u16 body;
    u16 pitch;
    u16 pose;

    yawRate = 0xFA4;
    pitchRate = 0x71C;
    if (self->field_0x18 == 1 && self->field_0x00A == 1 && self->act_no <= 1) {
        yawRate = 0x9F4;
    } else if (Pl_act_ck(self, 1, 1) == 1 || Pl_act_ck(self, 1, 5) == 1) {
        yawRate = 0x71C;
    } else if (Pl_act_ck(self, 1, 2) == 1 || Pl_act_ck(self, 1, 0xE) == 1) {
        yawRate = 0x5B0;
    } else if (self->kind_0x09 == 3) {
        if (pl_status_bit_ck((struct _ENEMY_WORK*)self, 0xC0) != 0) {
            yawRate = 0x444;
            if (self->field_0x18 == 1) {
                yawRate = 0x71C;
            }
            pitchRate = 0x16C;
        } else {
            yawRate = 0x71C;
        }
    } else if (Pl_act_ck(self, 1, 4) == 1) {
        yawRate = 0;
    }
    if (Pl_master_ck(self) == 1 && Pl_act_ck(self, 0, 0x16) == 1 && pl_target_lock_ck(self) == 1) {
        pitchRate = 0;
    }
    diff = self->field_0x0A8 - self->rot_0x54.y;
    motion = Get_motion_no(self);
    switch (motion) {
    case 3:
        bank = 0x80;
        break;
    case 4:
        bank = 0xA0;
        break;
    default:
        bank = 0;
        break;
    }
    if ((u16)(diff + yawRate) < (u32)(yawRate * 2)) {
        self->rot_0x54.y = self->field_0x0A8;
        body = self->field_0x0B0;
        if ((s32)(body + 0x300) <= 0x600) {
            self->field_0x0B0 = 0;
        } else if (body < 0x8000) {
            self->field_0x0B0 = body - 0x300;
        } else {
            self->field_0x0B0 = body + 0x300;
        }
        if ((u16)(diff + 0x100) < 0x200) {
            if (self->field_0x648 > 0) {
                fn_80260248(self, 1, -5);
            } else if (self->field_0x648 < 0) {
                fn_80260248(self, 1, 5);
            }
        } else if (diff < 0x8000) {
            fn_80260248(self, 0, -10);
        } else {
            fn_80260248(self, 0, 10);
        }
    } else if (diff < 0x8000) {
        self->rot_0x54.y = (u16)(self->rot_0x54.y + yawRate);
        self->field_0x0B0 -= bank;
        fn_80260248(self, 0, -10);
    } else {
        self->rot_0x54.y = (u16)(self->rot_0x54.y - yawRate);
        self->field_0x0B0 += bank;
        fn_80260248(self, 0, 10);
    }
    body = self->field_0x0B0;
    if ((u16)(body + 0xF600) <= 0xEC00) {
        if (body >= 0x8000) {
            self->field_0x0B0 = 0xF600;
        } else {
            self->field_0x0B0 = 0xA00;
        }
    }
    pitch = self->field_0x0AC - self->rot_0x54.x;
    if ((u16)(pitch + pitchRate) < (u32)(pitchRate * 2)) {
        self->rot_0x54.x = self->field_0x0AC;
    } else if (pitch < 0x8000) {
        self->rot_0x54.x = (u16)(self->rot_0x54.x + pitchRate);
    } else {
        self->rot_0x54.x = (u16)(self->rot_0x54.x - pitchRate);
    }
    if ((u32)(motion - 0x77) <= 1 || motion == 0x65 || motion == 0x73) {
        if (self->field_0x648 == 0) {
            pl_model_set_value(self, 0, pl_frame_window_1);
            pl_model_set_value(self, 1, pl_float_zero);
            return;
        }
        lean = pl_float_0_01 * (f32)self->field_0x648;
        if (self->field_0x648 > 0) {
            switch (motion) {
            case 0x65:
                pose = 0x6F;
                break;
            case 0x73:
                pose = 0x7F;
                break;
            case 0x77:
                pose = 0x85;
                break;
            case 0x78:
                pose = 0x81;
                break;
            }
        } else {
            switch (motion) {
            case 0x65:
                pose = 0x7D;
                break;
            case 0x73:
                pose = 0x80;
                break;
            case 0x77:
                pose = 0x86;
                break;
            case 0x78:
                pose = 0x82;
                break;
            }
            lean *= pl_float_neg1;
        }
        pl_chr_set_motion_default(self, 1, pose, 0, (s16)pl_rig_get_motion_frame(self));
        pl_model_set_value(self, 0, pl_frame_window_1 - lean);
        pl_model_set_value(self, 1, lean);
    }
}

/* 0x8026077C - whether the two state bytes disagree while the game is outside play mode 3. */
s32 fn_8026077C(u8 a, u8 b) {
    if (a == b || (u8)PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* 0x802607C4 - steers the actor's stored angle toward the nearest armed move-work slot's bearing. */
void fn_802607C4(_PLW* self) {
    PlMoveWork* best_work;
    s32 found = 0;

    if ((u16)Get_motion_no(self) == 38) {
        f32 best = pl_float_2250000;
        PlMoveWork* work = (PlMoveWork*)get_move_work_adrs(3);
        u16 count = (u16)get_move_work_max(3);
        s32 i;

        for (i = 0; i < count; i++) {
            if (work->field_0x000 != 0 && work->field_0x001 != 0 &&
                fn_8026077C(self->area_0x16, work->field_0x1E1) != 0) {
                f32 distance = vec3_dist_sq(&self->vec_0x03C, &work->pos_0x188);

                if (best >= distance) {
                    best = distance;
                    best_work = work;
                    found = 1;
                }
            }
            work++;
        }
    }
    if (found == 0) {
        u16 value = self->field_0x0B2;

        if (value == 0) {
            return;
        }
        if ((u32)(value + 1024) <= 2048) {
            self->field_0x0B2 = 0;
        } else if ((s16)value >= 0) {
            self->field_0x0B2 = value - 1024;
        } else {
            self->field_0x0B2 = value + 1024;
        }
        return;
    }
    {
        s32 diff = ((s32)(u16)calcVecAng2(&self->vec_0x03C, &best_work->pos_0x188) + 0x10000) -
                   (s32)self->field_0x058;
        u16 current = self->field_0x0B2;

        if ((u16)diff >= 32768) {
            s32 target;

            if ((u16)diff < 54614) {
                diff = 54614;
            }
            target = current - 2048;
            if (current >= 32768) {
                if (target > (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        } else {
            s32 target;

            if ((u16)diff > 10923) {
                diff = 10923;
            }
            target = current + 2048;
            if (current < 32768) {
                if (target < (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        }
    }
}

/* 0x8026099C - runs the actor's per-frame body for the current mode byte, when the slot is live. */
void fn_8026099C(_PLW* self) {
    if (self->slot_active == 0) {
        return;
    }
    switch (self->field_0x004) {
    default:
        break;
    case 0:
        ((void (*)(void))fn_8024676C)();
        break;
    case 1:
        fn_8026F7B4();
        if ((u8)event_demo_running_ck() == 0 && event_demo_ck() == 0) {
            fn_8025F088(self);
        }
        break;
    }
}

/* 0x80260A18 - whether the actor's skill 382 is active. */
s16 fn_80260A18(_PLW* self) {
    s16 active = 0;

    if (Pl_item_timer_get(self, 382) > 0) {
        active = 1;
    }
    return active;
}

/* 0x80260A58 - the gate that keeps the actor out of the skill-382 action. */
s32 fn_80260A58(_PLW* self) {
    if (self->field_0x3EA > 0) {
        return 0;
    }
    if (fn_80278578(self, 2) == 0) {
        return 0;
    }
    if (fn_8027D40C(self) > 0) {
        return 0;
    }
    if ((self->field_0x5A4 & 0x200) != 0 && Pl_cat_skill_ck(self, 47) == 0) {
        return 0;
    }
    if (self->field_0x404 > 0) {
        return 0;
    }
    if (fn_80276800(self, 0) == 1) {
        return 0;
    }
    return fn_80260A18(self) <= 0;
}

/* 0x80260B38 (0xC38): the actor's per-frame status step - runs down every act, item and status timer, and for the
 * master the stamina, condition, buff and regeneration timers with their expiry messages. */
void pl_act_status_step(_PLW* self) {
    s32 period;
    s32 step;
    u32 carry;
    u8 latch;
    u8 count;
    s16 diff;

    self->frame_0x020++;
    self->frame_b_0x024++;
    if (++self->field_0x030 > 1000) {
        self->field_0x030 = 1000;
    }
    if (self->field_0x19 > 0) {
        self->field_0x19--;
    }
    if (self->field_0x580 > 0) {
        self->field_0x580--;
    }
    if (self->field_0x3A2 > 0) {
        self->field_0x3A2--;
    } else if (self->field_0x3A3 > 0) {
        self->field_0x3A3--;
    }
    if (self->field_0x266 > 0) {
        self->field_0x266--;
    }
    if (self->field_0x396 > 0) {
        self->field_0x396--;
    }
    if (self->field_0x45A > 0) {
        self->field_0x45A--;
    }
    if (self->field_0x45C > 0) {
        self->field_0x45C--;
    }
    if (self->field_0x264 < 30000) {
        self->field_0x264 += (s16)((s16)self->field_0x354 * 2);
        if (self->field_0x264 >= 30000) {
            self->field_0x264 = 30000;
        }
    }
    if (self->field_0x662 > 0) {
        self->field_0x662--;
    }
    if (self->wide_area_hold_0x660 > 0) {
        self->wide_area_hold_0x660--;
    }
    if ((s16)self->field_0x442 > 0) {
        self->field_0x442--;
    }
    if (Pl_master_ck(self) == 0) {
        if (self->field_0x584 != 0 && --self->field_0x256 <= 0) {
            self->slot_active = 0;
            self->field_0x001 = 0;
        }
        return;
    }
    if (self->daze_timer_0x468 > 0) {
        self->daze_timer_0x468--;
    }
    if (self->entry_hold_0x012 > 0) {
        self->entry_hold_0x012--;
    }
    if (self->field_0x313 > 0) {
        self->field_0x313--;
    }
    if ((s8)self->field_0x314 > 0) {
        self->field_0x314--;
    }
    if (self->field_0x658 != 0) {
        self->field_0x658--;
    }
    if ((s8)self->field_0x36A > 0) {
        self->field_0x36A--;
    }
    if (self->field_0x352 > 0) {
        self->field_0x352--;
    }
    if (self->field_0x59A != 0) {
        self->field_0x59A--;
    }
    if (self->talk_wait_0x668 != 0) {
        self->talk_wait_0x668--;
    }
    if (self->hold_timer_0x66A > 0) {
        self->hold_timer_0x66A--;
    }
    if (self->field_0x470 > 0) {
        self->field_0x470--;
    }
    if (self->field_0x460 > 0) {
        self->field_0x460--;
    }
    if (pl_mode8_alive_ck(self) == 1 && --self->guard_timer_0x36E <= 0) {
        self->guard_timer_0x36E = 150;
        pl_act_add_charge(self, 5);
    }
    latch = self->field_0x5C4;
    count = latch & 0xF;
    if (count != 0 && self->field_0x00A == 12) {
        self->field_0x5C4 = (u8)(count - 1) | (u8)(latch & 0xF0);
    } else if (self->field_0x00A != 0 ||
               ((u32)(self->act_no - 49) > 3 && (u32)(self->act_no - 97) > 2 &&
                (self->act_no < (s16)44 || self->act_no > (s16)45))) {
        carry = pl_carry_item_get(self);
        if ((u16)carry != 0xFFFF) {
            if (Pl_master_ck(self) == 1 && Pl_motion_input_ck(1) == 0) {
                pl_model_state_set(self, 2, 18, (u16)carry);
            }
            pl_equip_slot_count_get(self, (u16)carry);
        }
    }
    if (fn_80278578(self, 1) == 1 || Pl_act_state_ck(self) == 1) {
        if (Pl_dm_condition_ck(self, 12) == 1) {
            if (Pl_act_state_ck(self) == 1) {
                if (self->field_0x412 > 1) {
                    self->field_0x412--;
                }
                pl_sub_gauge_add(self, 4);
            } else {
                pl_sub_gauge_add(self, 1);
            }
        } else {
            pl_sub_gauge_add(self, 4);
        }
    }
    if (self->field_0x412 > 0 && --self->field_0x412 == 0 && Pl_motion_input_ck(1) == 0) {
        pl_model_state_set(self, 1, 24, 0);
    }
    if (self->kind_0x09 == 3 && self->field_0x00A != 8) {
        if (Pl_suimen_ck(self) == 0) {
            if (self->field_0x382 >= 90) {
                period = 128;
                if (Pl_Skill_ck(self, 193) == 1) {
                    period = 256;
                } else if (Pl_Skill_ck(self, 194) == 1) {
                    period = 103;
                }
                if ((s32)self->frame_0x020 % period == 0) {
                    pl_act_add_stagger(self, -1);
                    if (self->field_0x37E == 25) {
                        eft013_set(self, 3);
                    }
                }
            } else {
                self->field_0x382++;
            }
        }
        if (fn_80276800(self, 0) == 1) {
            self->field_0x3DC |= 0x100000;
            if ((self->frame_0x020 & 0xF) == 0) {
                pl_act_gauge_step_unlatched(self, -1, NULL);
            }
        }
    } else if (self->field_0x00A != 8) {
        pl_act_add_stagger(self, 150);
    }
    if (self->field_0x44A > 0 && --self->field_0x44A == 0) {
        self->field_0x449 = 0;
    }
    if (self->field_0x44E > 0 && --self->field_0x44E == 0) {
        self->buff_b_level_0x44D = 0;
    }
    if ((s16)self->field_0x406 > 0 && (s16)--self->field_0x406 <= 0) {
        self->field_0x3DC &= ~0xC;
    }
    if ((s16)self->field_0x408 > 0 && (s16)--self->field_0x408 <= 0) {
        self->field_0x3DC &= ~0x30;
    }
    if ((s16)self->field_0x40A > 0 && (s16)--self->field_0x40A <= 0) {
        self->field_0x3DC &= ~0xC0;
    }
    if ((s16)self->field_0x40C > 0 && (s16)--self->field_0x40C <= 0) {
        self->field_0x3DC &= ~0x300;
    }
    if (self->field_0x418 > 0) {
        self->field_0x418--;
    }
    if (self->field_0x41E > 0) {
        self->field_0x41E--;
    }
    if (self->field_0x454 > 0 && --self->field_0x454 <= 0) {
        self->field_0x454 = 0;
        self->field_0x450 = 0;
    }
    if (self->field_0x456 > 0 && --self->field_0x456 <= 0) {
        self->field_0x456 = 0;
        self->field_0x451 = 0;
    }
    if (self->field_0x458 > 0 && --self->field_0x458 <= 0) {
        self->field_0x458 = 0;
        self->field_0x452 = 0;
    }
    if (self->field_0x002 == 7) {
        if (self->daze_timer_0x468 == 0 && self->field_0x384 > 0 && (s32)self->frame_0x020 % 10 == 0) {
            self->field_0x384--;
        }
        if ((u8)pl_stagger_level_get(self) >= 3 && self->buff_timer_0x46A > 0 && --self->buff_timer_0x46A <= 0) {
            self->field_0x386 = 0;
        }
    }
    if (self->field_0x45E > 0 && --self->field_0x45E == 0 && Pl_master_ck(self) == 1 &&
        Pl_motion_input_ck(1) == 0) {
        pl_model_state_set(self, 1, 31, 0);
    }
    if (self->field_0x464 != 0) {
        if (pl_act_kind_get(self->area_0x16) == 3) {
            if (self->frame_0x020 & 1) {
                self->field_0x464 -= (s16)2;
            } else {
                self->field_0x464--;
            }
            if (self->field_0x464 < 0) {
                self->field_0x464 = 0;
            }
        } else {
            self->field_0x464--;
        }
        if (self->field_0x464 == 0 && Pl_master_ck(self) == 1 && Pl_motion_input_ck(1) == 0) {
            pl_model_state_set(self, 1, 35, 0);
        }
    }
    if (self->field_0x466 != 0) {
        if (pl_act_kind_get(self->area_0x16) == 4) {
            if (self->frame_0x020 & 1) {
                self->field_0x466 -= (s16)2;
            } else {
                self->field_0x466--;
            }
            if (self->field_0x466 < 0) {
                self->field_0x466 = 0;
            }
        } else {
            self->field_0x466--;
        }
        if (self->field_0x466 == 0 && Pl_master_ck(self) == 1 && Pl_motion_input_ck(1) == 0) {
            pl_model_state_set(self, 1, 36, 0);
        }
    }
    if ((s16)self->field_0x410 != 0) {
        self->field_0x410--;
    }
    if ((s16)self->status_timer_0x42E[0] > 0) {
        self->status_timer_0x42E[0]--;
    }
    if (self->field_0x438 > 0) {
        self->field_0x438--;
    }
    if ((s16)self->field_0x430 > 0) {
        self->field_0x430--;
    }
    if (self->field_0x43A > 0) {
        self->field_0x43A--;
    }
    if ((s16)self->field_0x432 > 0) {
        self->field_0x432--;
    }
    if (self->field_0x43C > 0) {
        self->field_0x43C--;
    }
    if ((s16)self->field_0x434 > 0) {
        self->field_0x434--;
    }
    if (self->field_0x43E > 0) {
        self->field_0x43E--;
    }
    if ((s16)self->field_0x436 > 0) {
        self->field_0x436--;
    }
    if (self->field_0x440 > 0) {
        self->field_0x440--;
    }
    if ((s16)self->field_0x422 > 0) {
        self->field_0x422--;
    }
    pl_area_stamina_drain_step(self);
    if ((u8)(self->field_0x00A + 250) > 2) {
        do {
            if (self->field_0x464 == 0) {
                switch (pl_act_kind_get(self->area_0x16)) {
                case 1:
                    if (Pl_Skill_ck(self, 124) == 0 && Pl_Skill_ck(self, 125) == 0) {
                        continue;
                    }
                    break;
                case 3:
                    if (Pl_Skill_ck(self, 125) == 0) {
                        continue;
                    }
                    break;
                }
            }
            if ((u32)fn_80260A58(self) != 1) {
                continue;
            }
            period = 60;
            if (Pl_Skill_ck(self, 17) == 1) {
                period = 30;
            }
            if (Pl_Skill_ck(self, 18) == 1) {
                period = 15;
            }
            if (Pl_Skill_ck(self, 19) == 1) {
                period = 120;
            }
            if (Pl_Skill_ck(self, 20) == 1) {
                period = 240;
            }
            step = 1;
            if (self->field_0x45E != 0) {
                step = 2;
            }
            if ((s32)self->frame_0x020 % period == 0 && self->field_0x376 > self->health) {
                diff = self->field_0x376 - self->health;
                if (diff >= step) {
                    pl_act_add_hold_gauge(self, step);
                } else {
                    pl_act_add_hold_gauge(self, diff);
                }
            }
        } while (0);
        if (self->field_0x400 > 0) {
            self->field_0x400--;
        }
        if (self->field_0x400 <= 0) {
            self->field_0x400 = 0;
            if (self->field_0x3FC > 0) {
                self->field_0x3FC--;
            }
        }
    }
}

/* 0x80261770 (0xA40): picks the act the item in pouch slot `slot` enters - its kind, act number and argument, and the
 * aim word or gathering point some items hand back; returns whether the item can be used here. */
s32 pl_item_use_act_pick(_PLW* self, u8* kind, u16* act, u16* arg, s32* word, u16 slot, u16* point) {
    VEC3 pos;
    _CP_VECTOR rot;
    s32 near_id;
    s32 near_word;
    s16 hit_kind;
    s32 want;

    VEC3_ctor(&pos);
    *kind = 0xFF;
    *act = 0xFF;
    self->field_0x309 |= 1;
    if (slot & 0x80) {
        return 0;
    }
    if (pl_item_now_usable_ck(self, self->slot_id[slot].item_id) == 0) {
        *kind = 0xFF;
        *act = 0xFF;
        return 0;
    }
    switch (self->slot_id[slot].item_id) {
    case 361:
        *kind = 0;
        *act = 135;
        break;
    case 362:
    case 363:
    case 364:
    case 365:
        *kind = 0;
        *act = 137;
        break;
    case 347:
        if (self->field_0x00A == 7) {
            *kind = 7;
            *act = 5;
            *arg = 0;
            break;
        }
    case 5:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 33:
    case 50:
    case 96:
    case 97:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 17;
        } else {
            *act = 108;
            *arg = 140;
        }
        break;
    case 366:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 19;
        } else {
            *act = 110;
            *arg = 12;
        }
        break;
    case 345:
    case 346:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 18;
        } else {
            *act = 109;
            *arg = 140;
        }
        break;
    case 1:
    case 264:
    case 567:
    case 600:
    case 601:
        if ((u32)pl_item_use_free_ck(self) == 1) {
            *arg = 12;
            switch (self->kind_0x09) {
            case 0:
                *kind = 0;
                *act = 40;
                break;
            case 1:
                *kind = 0;
                *act = 155;
                break;
            case 3:
                *kind = 0;
                *act = 152;
                if ((u32)(self->slot_id[slot].item_id - 600) <= 1) {
                    *arg |= 0x80;
                }
                break;
            }
        }
        break;
    case 29:
    case 30:
    case 31:
    case 32:
        if ((u32)pl_stance_low_ck(self) == 1) {
            switch (self->kind_0x09) {
            case 0:
                *kind = 0;
                *act = 40;
                break;
            case 1:
                *kind = 0;
                *act = 155;
                break;
            }
        }
        break;
    case 383:
        if (self->field_0x585 != 0) {
            *kind = 0;
            *act = 59;
        }
        break;
    case 2:
    case 52:
    case 582:
        if ((u32)pl_motion_interruptible_ck(self) == 1) {
            *kind = 0;
            if (self->kind_0x09 != 3) {
                *act = 59;
            } else {
                *act = 79;
            }
        }
        break;
    case 43:
        if (self->kind_0x09 != 3) {
            *kind = 10;
            *act = 1;
        }
        break;
    case 48:
    case 98:
    case 207:
        if (pl_item_page2_ck(self) == 0) {
            *kind = 0;
            if (self->kind_0x09 != 3) {
                *act = 61;
            } else {
                *act = 72;
            }
        }
        break;
    case 3:
        if ((u32)pl_spot_use_a_ck(self) == 1 && self->kind_0x09 != 3) {
            *kind = 0;
            *act = 47;
        }
        break;
    case 4:
    case 49:
        if ((u32)pl_spot_use_b_ck(self) == 1) {
            if (self->kind_0x09 != 3) {
                *kind = 0;
                *act = 47;
            } else {
                *kind = 0;
                *act = 125;
            }
        }
        break;
    case 6:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 44:
    case 45:
    case 46:
    case 51:
    case 116:
    case 123:
    case 124:
    case 134:
    case 135:
    case 138:
    case 145:
    case 273:
    case 331:
    case 332:
    case 334:
    case 344:
    case 360:
    case 368:
    case 374:
    case 377:
    case 378:
    case 391:
    case 392:
    case 393:
    case 394:
    case 398:
    case 399:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 53;
        } else {
            *act = 55;
        }
        break;
    case 375:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 57;
        } else {
            *act = 58;
        }
        break;
    case 26:
    case 27:
    case 28:
    case 166:
    case 167:
    case 208:
    case 214:
        *kind = 0;
        if (self->kind_0x09 != 3) {
            *act = 54;
        } else {
            *act = 56;
        }
        break;
    case 40:
    case 47:
    case 265:
        if (Pl_item_timer_get(self, 29) > 0 && self->kind_0x09 != 3) {
            *kind = 0;
            *act = 34;
        }
        break;
    case 438:
        if (Pl_item_timer_get(self, 29) > 0 && (u32)ai_npc_player_near_ck(4, pl_float_1000) == 1 &&
            self->kind_0x09 != 3) {
            *kind = 0;
            *act = 176;
        }
        break;
    case 34:
    case 35:
    case 36:
        if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                    (s32*)&self->field_0x474) != 0xFFFF &&
            hit_kind == 3) {
            *word = (u16)calcVecAng2(&self->vec_0x03C, &pos);
            *kind = 0;
            self->field_0x478 = hit_kind;
            if (self->kind_0x09 != 3) {
                *act = 49;
            } else {
                *act = 99;
            }
        }
        break;
    case 37:
    case 38:
    case 39:
        if (self->kind_0x09 != 3 &&
            em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                    (s32*)&self->field_0x474) != 0xFFFF &&
            hit_kind == 4) {
            *kind = 0;
            *act = 50;
            self->field_0x478 = hit_kind;
        }
        break;
    case 202:
    case 203:
    case 204:
    case 205:
    case 206:
    case 351:
    case 353:
    case 355:
    case 389:
    case 390:
        if (self->kind_0x09 != 3 &&
            em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                    (s32*)&self->field_0x474) != 0xFFFF &&
            hit_kind == 5) {
            *kind = 0;
            *act = 63;
            *word = rot.y;
            self->field_0x478 = hit_kind;
        }
        break;
    case 95:
        if ((u32)pl_act_true_ck(self) == 1) {
            *kind = 0;
            if (self->kind_0x09 == 3) {
                *act = 71;
            } else if (self->kind_0x09 == 1) {
                *act = 69;
            } else {
                *act = 68;
            }
        }
        break;
    case 194:
    case 388:
        if (self->kind_0x09 == 3) {
            *kind = 0;
            *act = 93;
            *arg = 140;
        }
        break;
    case 380:
    case 384:
        want = 1;
        if (want == pl_stg_weapon_near_find(self, &pos, &near_id, &near_word)) {
            *word = near_id;
            self->target_pos_0x090 = pos;
            pl_pos_blend_start(self, 20);
            if (self->kind_0x09 == 3) {
                *kind = 0;
                *act = 172;
                *arg = 12;
                *point = near_word;
            } else {
                *kind = 0;
                *act = 169;
                *arg = 12;
                *point = near_word;
            }
        }
        break;
    case 583:
        want = 5;
        if (want == pl_stg_weapon_near_find(self, &pos, &near_id, &near_word)) {
            *word = near_id;
            self->target_pos_0x090 = pos;
            pl_pos_blend_start(self, 20);
            if (self->kind_0x09 == 3) {
                *kind = 0;
                *act = 172;
                *arg = 12;
                *point = near_word;
            } else {
                *kind = 0;
                *act = 169;
                *arg = 12;
                *point = near_word;
            }
        }
        break;
    case 352:
    case 367:
    case 376:
    case 379:
    default:
        *kind = 0xFF;
        *act = 0xFF;
        break;
    }
    return *kind != 0xFF;
}

/* 0x802621B0 (0x4D8): tests the input condition a step-table row names (its `+0x06` low bits) against the actor's
 * request words, part flags, attack levels and stick windows. */
u32 pl_scan_row_input_ck(_PLW* self, PlScanEntry* row) {
    switch (row->flags_0x06 & 0x3FFF) {
    case 0:
        return 1;
    case 31:
        return 0;
    case 1:
        if (pl_stick_window_gated_ck(self, 1, row->cond_a_0x08, row->cond_b_0x0A) == 1) {
            return 1;
        }
        break;
    case 19:
        if (pl_stick_window_gated_ck(self, 1, row->cond_a_0x08, row->cond_b_0x0A) == 1 &&
            (self->field_0x0C8 & row->cond_c_0x0C) != 0) {
            return 1;
        }
        break;
    case 2:
        if (pl_stick_window_gated_ck(self, 0, row->cond_a_0x08, row->cond_b_0x0A) == 1) {
            return 1;
        }
        break;
    case 3:
        if ((self->req_bits_b_0x0BC & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 18:
        if ((self->field_0x0B8 & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 4:
        if (pl_act_param_tier_ck(self, 0) >= 1 && (self->req_bits_b_0x0BC & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 5:
        if ((self->field_0x0CC & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 6:
        if (pl_act_param_tier_ck(self, 0) == 0 && (self->req_bits_b_0x0BC & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 7:
        if (pl_act_param_tier_ck(self, 0) >= 1 && (self->field_0x0B8 & row->cond_a_0x08) != 0) {
            return 1;
        }
        break;
    case 8:
        if (pl_act_param_tier_ck(self, 0) >= 1) {
            return 1;
        }
        break;
    case 9:
        if (pl_act_param_tier_ck(self, 0) >= 3) {
            return 1;
        }
        break;
    case 20:
        if (pl_act_param_tier_ck(self, 1) >= 1) {
            return 1;
        }
        break;
    case 21:
        if (pl_act_param_tier_ck(self, 1) >= 3) {
            return 1;
        }
        break;
    case 10:
        if ((self->field_0x0B8 & row->cond_a_0x08) != 0 && (self->req_bits_b_0x0BC & row->cond_b_0x0A) != 0) {
            return 1;
        }
        break;
    case 11:
        if (pl_stick_window_level_ck(self, 0, row->cond_a_0x08, row->cond_b_0x0A) == 1 &&
            (self->req_bits_b_0x0BC & row->cond_c_0x0C) != 0) {
            return 1;
        }
        break;
    case 12:
        if (pl_act_param_tier_ck(self, 0) == 0) {
            return 1;
        }
        break;
    case 13:
        if (pl_act_param_tier_ck(self, 1) == 0) {
            return 1;
        }
        break;
    case 14:
        if (pl_stick_window_level_ck(self, 1, row->cond_a_0x08, row->cond_b_0x0A) == 1) {
            return 1;
        }
        break;
    case 15:
        if (pl_stick_window_level_ck(self, 0, row->cond_a_0x08, row->cond_b_0x0A) == 1) {
            return 1;
        }
        break;
    case 16:
        if (pl_stick_window_level_ck(self, 1, row->cond_a_0x08, row->cond_b_0x0A) == 0) {
            return 1;
        }
        break;
    case 17:
        if (pl_stick_window_level_ck(self, 0, row->cond_a_0x08, row->cond_b_0x0A) == 0) {
            return 1;
        }
        break;
    case 22:
        if (pl_part_flag_ck(self, row->cond_a_0x08) == 1) {
            return 1;
        }
        break;
    case 23:
        if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, row->cond_a_0x08) == 1) {
            return 1;
        }
        break;
    case 24:
        if (pl_act_param_tier_ck(self, 0) >= 1 && pl_part_flag_ck(self, row->cond_a_0x08) == 0) {
            return 1;
        }
        break;
    case 25:
        if ((u32)pl_part_flag2_ck(self, row->cond_a_0x08) == 1) {
            return 1;
        }
        break;
    case 26:
        if ((u32)pl_part_flag2_ck(self, row->cond_a_0x08) == 1 && pl_stick_dir_ck(self, row->cond_c_0x0C) == 1) {
            return 1;
        }
        break;
    case 29:
        if (pl_act_param_tier_ck(self, 0) >= 1 && (u32)pl_part_flag2_ck(self, row->cond_a_0x08) == 1) {
            return 1;
        }
        break;
    case 27:
        if (pl_part_flag_ck(self, row->cond_a_0x08) == 1 &&
            pl_stick_window_level_ck(self, 0, row->cond_b_0x0A, row->cond_c_0x0C) == 1) {
            return 1;
        }
        break;
    case 30:
        if (pl_part_flag_ck(self, row->cond_a_0x08) == 1 &&
            pl_stick_window_ck(self, 0, row->cond_b_0x0A, row->cond_c_0x0C) == 1) {
            return 1;
        }
        break;
    case 28:
        if (pl_part_flag_ck(self, row->cond_a_0x08) == 1 && pl_act_param_tier_ck(self, 0) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80262688 - adopts the nearest live player work as the actor's held item when the item-data kind
 * allows it, and reports whether one was adopted. */
s32 fn_80262688(_PLW* self) {
    _PLW* work;
    u16 count;
    s32 i;

    if (self->field_0x658 != 0) {
        return 0;
    }
    work = (_PLW*)get_move_work_adrs(2);
    count = (u16)get_move_work_max(2);
    for (i = 0; i < count; i++, work++) {
        if (work->slot_active != 0 &&
            (isServerSelectState() == 1 ? Pl_master_ck(work) != 1 : work->chunk_ofs != self->chunk_ofs) &&
            (work->field_0x655 == self->chunk_ofs || work->field_0x655 == 0xFF) &&
            (Pl_act_ck(work, 0, 20) != 0 || Pl_act_ck(work, 0, 158) != 0) &&
            self->area_0x16 == work->area_0x16 &&
            vec3_distance(&self->vec_0x03C, &work->vec_0x03C) >= pl_float_300 &&
            ((PlItemData*)GetItemData(work->field_0x650))->field_0x01 < 3) {
            self->field_0x658 = 90;
            if (pl_item_room_get(self, work->field_0x650, work->field_0x652) < work->field_0x652) {
                if ((u16)pl_item_slot_find(self, work->field_0x650) == 0xFFFF) {
                    pl_model_state_set(self, 1, 0, 0);
                } else {
                    pl_model_state_set(self, 2, 30, work->field_0x650);
                }
                return 0;
            }
            self->field_0x650 = work->field_0x650;
            self->field_0x652 = work->field_0x652;
            if (self->kind_0x09 == 3) {
                pl_act_enter(self, 0, 161, 0);
            } else {
                pl_act_enter(self, 0, 91, 0);
            }
            if (isServerSelectState() == 1) {
                if (Pl_master_ck(self) == 1) {
                    Pl_net_send(self, 7, (u16)work->chunk_ofs);
                }
            } else {
                pl_item_add(work, work->field_0x650, (s16)(-work->field_0x652));
                work->field_0x656 = 0xFF;
                pl_item_add(self, self->field_0x650, self->field_0x652);
                self->field_0x656 = 1;
                pl_model_state_set(self, 2, 29, self->field_0x650);
                if ((u16)item_se_ck(self->field_0x650) == 1) {
                    se_slot_req(4);
                }
            }
            return 1;
        }
    }
    return 0;
}

/* 0x80262940 (0x1934): walks the actor's step table and enters the act of the first row whose frame window, input
 * condition and row gate hold; returns 1 once a row fired. */
#pragma optimization_level 4
s32 pl_scan_act_select(_PLW* self) {
    VEC3 pos;
    _CP_VECTOR rot;
    f32 probe;
    s32 aim;
    s32 near_word;
    s32 near_id;
    u16 pair[2];
    u16 act;
    u16 arg;
    u16 point;
    s16 hit_kind;
    u8 kind;
    SysQuestMove* quest;
    PlScanEntry* row;
    u16* remap;
    s32 shell;
    u8* fired;
    s32 j;
    s32 at_end;
    s32 no_act;
    s32 i;
    u16 slot_item;
    u16 slot;
    f32 end;
    f32 start;
    f32 stop;
    f32 lead;

    VEC3_ctor(&pos);
    quest = (SysQuestMove*)get_move_work_adrs(0);
    if (self->field_0x318 == 0) {
        return 0;
    }
    if (self->field_0x313 > 0) {
        return 0;
    }
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    row = (PlScanEntry*)self->field_0x318;
    end = pl_rig_get_float_a4(self);
    aim = self->field_0x0A8;
    for (i = 0; row->id_0x00 != 0xFF; i++, row++) {
        if (i >= 48) {
            break;
        }
        at_end = 0;
        point = 0;
        if (self->window_start_0x31C != 0) {
            start = self->window_start_0x31C;
        } else if (row->frame_start_0x10 >= 999) {
            if (pl_model_blend_positive_ck(self) == 1) {
                start = end;
            } else {
                start = end - pl_frame_window_1;
            }
            at_end = 1;
        } else if (row->frame_start_0x10 < 0) {
            start = end + row->frame_start_0x10;
        } else {
            start = row->frame_start_0x10;
        }
        if (self->window_end_0x320 != 0) {
            stop = self->window_end_0x320;
            if (stop > end) {
                stop = end;
            }
        } else if (row->frame_end_0x12 >= 999) {
            if (pl_model_blend_positive_ck(self) == 0) {
                stop = end - pl_frame_window_1;
            } else {
                stop = end;
            }
        } else if (row->frame_end_0x12 < 0) {
            stop = end + row->frame_end_0x12;
        } else {
            stop = row->frame_end_0x12;
        }
        if (self->window_lead_0x31E != 0) {
            lead = self->window_lead_0x31E;
        } else {
            lead = row->frame_lead_0x14;
        }
        if (lead < pl_frame_window_999) {
            if (stop < pl_frame_window_1) {
                if (at_end == 0 || Pl_motion_end_ck(self) != 1) {
                    if (Pl_frame_check(self, 6, start - lead, stop) == 0) {
                        continue;
                    }
                }
            } else if (Pl_frame_check(self, 7, start - lead, stop) == 0) {
                continue;
            }
        }
        no_act = 0;
        slot_item = 0xFFFF;
        if (Pl_master_ck(self) == 1) {
            if (lead < pl_frame_window_1) {
                self->field_0x322[i] = 0;
            }
            if (pl_scan_row_input_ck(self, row) == 1) {
                self->field_0x322[i] = 1;
            }
        }
        fired = &self->field_0x322[i];
        if (*fired == 0) {
            continue;
        }
        if (row->flags_0x06 & 0x8000) {
            if ((row->flags_0x0E & 0x8000) &&
                ((u32)fn_80264EA4(self) == 1 || (u32)fn_80264EC0(self) == 1 || (u32)fn_80264ED4(self) == 1)) {
                *fired = 0;
                continue;
            }
            if ((row->flags_0x06 & 0x8000) && (row->flags_0x0E & 0x4000) && (s8)self->field_0x314 > 0) {
                *fired = 0;
                continue;
            }
            switch (row->flags_0x0E & 0xFFF) {
            default:
                *fired = 0;
                continue;
            case 1:
                if (pl_stagger_budget_ck(self, 150) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 22:
                if (Pl_suimen_ck(self) == 1) {
                    *fired = 0;
                    continue;
                }
                break;
            case 23:
                if (pl_stagger_budget_ck(self, 150) == 0 || Pl_suimen_ck(self) == 1) {
                    *fired = 0;
                    continue;
                }
                break;
            case 2:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (Pl_act_ck(self, row->id_0x00, row->key_0x02) == 1) {
                    no_act = 1;
                }
                break;
            case 8:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (self->field_0x00A == 1) {
                    switch (self->act_no) {
                    case 24:
                    case 25:
                    case 1:
                    case 5:
                    case 15:
                    case 39:
                    case 3:
                        no_act = 1;
                        break;
                    }
                }
                if (self->field_0x00A == 12 && self->act_no == (s16)1) {
                    no_act = 1;
                }
                break;
            case 9:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                switch (self->field_0x00A) {
                case 1:
                    switch (self->act_no) {
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 2:
                    case 3:
                    case 39:
                    case 35:
                    case 27:
                        no_act = 1;
                        break;
                    }
                    break;
                case 12:
                    if (self->act_no == (s16)2) {
                        no_act = 1;
                    }
                    break;
                }
                break;
            case 28:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (self->field_0x00A == 1) {
                    switch (self->act_no) {
                    case 6:
                    case 7:
                    case 9:
                    case 27:
                    case 29:
                    case 33:
                    case 34:
                    case 36:
                        no_act = 1;
                        break;
                    }
                }
                if (pl_alt_form_free_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 27:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (self->field_0x00A == 1) {
                    switch (self->act_no) {
                    case 33:
                    case 34:
                    case 6:
                    case 7:
                    case 9:
                    case 27:
                    case 36:
                        no_act = 1;
                        break;
                    }
                }
                break;
            case 11:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (self->field_0x00A == 1) {
                    switch (self->act_no) {
                    case 1:
                    case 2:
                    case 3:
                    case 5:
                    case 10:
                    case 11:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 37:
                    case 39:
                        no_act = 1;
                        break;
                    }
                }
                break;
            case 31:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (self->field_0x00A == 1) {
                    switch (self->act_no) {
                    case 6:
                    case 7:
                    case 9:
                    case 27:
                    case 29:
                    case 33:
                    case 34:
                    case 36:
                        no_act = 1;
                        break;
                    }
                }
                break;
            case 3:
            case 24:
                if (pl_status_bit_ck((_ENEMY_WORK*)self, 0x180) == 0) {
                    *fired = 0;
                    continue;
                }
                if (row->flags_0x0E == 24 && pl_item_page2_ck(self) != 0 && pl_reload_needed_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 37:
                if (pl_status_bit_ck((_ENEMY_WORK*)self, 0x180) != 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 4:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (pl_stagger_budget_ck(self, 150) == 0) {
                    *fired = 0;
                    continue;
                }
                if (Pl_act_ck(self, row->id_0x00, row->key_0x02) == 1) {
                    no_act = 1;
                }
                break;
            case 5:
                if (pl_ground_probe_ahead(self, (VEC3*)&probe, pl_frame_window_60, pl_frame_window_120) == 0) {
                    continue;
                }
                break;
            case 6:
                if (pl_ground_probe_ahead(self, (VEC3*)&probe, pl_frame_window_180, pl_frame_window_30) == 0) {
                    continue;
                }
                break;
            case 7:
                if (pl_ground_probe_ahead(self, (VEC3*)&probe, pl_frame_window_210, pl_frame_window_90) == 0) {
                    continue;
                }
                break;
            case 10:
                if (self->field_0x369 != 1) {
                    continue;
                }
                break;
            case 12:
                if (pl_call_answer_ck(self) == 0) {
                    continue;
                }
                break;
            case 13:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (pl_stagger_budget_ck(self, 150) == 0) {
                    *fired = 0;
                    continue;
                }
                if (self->field_0x00A == 1) {
                    if ((u32)(self->act_no - 16) <= 1) {
                        break;
                    }
                    *fired = 0;
                    continue;
                }
                *fired = 0;
                continue;
            case 14:
            case 26:
                if (self->field_0x269 == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 15:
                if (self->field_0x269 != 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 16:
                if (pl_item_page2_ck(self) != 0 && pl_reload_needed_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 17:
                if (row->id_0x00 & 0x80) {
                    continue;
                }
                if (Pl_act_ck(self, row->id_0x00, row->key_0x02) == 1) {
                    no_act = 1;
                }
                if (pl_alt_form_free_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 18:
                if (fn_80264EA4(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 42:
                if (fn_80264EC0(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 43:
                if (fn_80264ED4(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 25:
                if ((u16)pl_carry_item_get(self) != 0xFFFF) {
                    *fired = 0;
                    continue;
                }
                break;
            case 29:
                *fired = 0;
                continue;
            case 30:
                if (Pl_item_timer_get(self, 43) <= 0) {
                    *fired = 0;
                    continue;
                }
                if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                            (s32*)&self->field_0x474) != 0xFFFF) {
                    if (hit_kind != 17) {
                        *fired = 0;
                        continue;
                    }
                    self->field_0x47A = em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C,
                                                                &hit_kind, &pos, &rot, NULL);
                    self->field_0x478 = hit_kind;
                    break;
                }
                *fired = 0;
                continue;
            case 32:
                if (pl_daze_level_ck(self, 15) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 33:
                if (pl_daze_level_ck(self, 20) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 34:
                if (pl_daze_level_ck(self, 25) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 35:
                if (pl_daze_level_ck(self, 10) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 36:
                if (pl_camera_close_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 38:
                if (pl_item_menu_open_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 39:
                if (self->field_0x654 == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 40:
                if (fn_80262688(self) == 0) {
                    *fired = 0;
                    continue;
                }
                return 1;
            case 55:
                if (ai_npc_hand_reach_ck() == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 44:
                if (pl_health_out_ck(self) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 45:
                if ((u32)pl_health_out_ck(self) == 1) {
                    *fired = 0;
                    continue;
                }
                break;
            case 46:
                self->field_0x5C9 = 1;
                if (self->field_0x5C8 == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 47:
                if (Pl_motion_input_ck(1) == 0 || self->field_0x077 != 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 48:
                if (pl_camera_close_ck(self) == 1) {
                    *fired = 0;
                    continue;
                }
                break;
            case 49:
                if (fn_8027D8A0(self, 0) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 50:
                if (fn_8027D8A0(self, 1) == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 51:
                if (pl_item_menu_open_ck(self) == 0 || Pl_item_timer_get(self, 43) > 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 52:
                if ((u16)pl_carry_item_get(self) != 381 || pl_stg_weapon_near_find(self, &pos, &near_id, &near_word) != 3) {
                    *fired = 0;
                    continue;
                }
                point = near_word;
                break;
            case 56:
                if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                            (s32*)&self->field_0x474) != 0xFFFF &&
                    Pl_motion_input_ck(0) == 0 && hit_kind == 10 && Pl_motion_input_ck(0) != 1 && quest != NULL &&
                    quest->quest_0x0DC != NULL) {
                    for (j = 0; j < 3; j++) {
                        em_work_slot_pair_get(j, (s16*)pair);
                        if (pair[0] != 0 && quest_element_supply_state_get((s16)j) == 2 &&
                            pair[0] == (u16)pl_carry_item_get(self)) {
                            quest_element_item_use(self, pair[0], 1);
                        }
                    }
                    if ((u16)pl_carry_item_get(self) == 0xFFFF) {
                        break;
                    }
                }
                *fired = 0;
                continue;
            case 53:
                if (pl_gauge_level_get(self) <= 1) {
                    *fired = 0;
                    continue;
                }
                break;
            case 54:
                if (quest_move_sub_flag_get() == 0) {
                    *fired = 0;
                    continue;
                }
                break;
            case 57:
                if (self->field_0x565 != 0) {
                    *fired = 0;
                    continue;
                }
                switch (pl_hit_result_best_get(self)) {
                case 0:
                case 1:
                case 5:
                    break;
                default:
                    *fired = 0;
                    continue;
                }
                break;
            case 0:
                break;
            }
        }
        if (*fired == 0) {
            continue;
        }
        if (row->id_0x00 & 0x80) {
            switch (row->key_0x02) {
            case 255:
                arg = row->arg_0x04;
                point = 0xFFFF;
                if (pl_item_use_act_pick(self, &kind, &act, &arg, &aim, self->field_0x304, &point) == 0) {
                    *fired = 0;
                    continue;
                }
                slot_item = self->slot_id[self->field_0x304].item_id;
                if (point != 0xFFFF) {
                    arg |= 0x20;
                }
                self->field_0x309 |= 0x20;
                break;
            case 246:
                switch (pl_stg_weapon_near_find(self, &pos, &near_id, &near_word)) {
                case 0:
                case 1:
                case 3:
                case 5:
                    *fired = 0;
                    continue;
                case 2:
                    if ((u32)pl_work_flag0_clear_ck() == 1) {
                        kind = 0;
                        act = self->kind_0x09 == 3 ? 178 : 167;
                        arg = row->arg_0x04;
                        aim = (u16)calcVecAng2(&self->vec_0x03C, &pos);
                        break;
                    }
                    *fired = 0;
                    continue;
                case 4:
                    if ((u32)pl_work_flag8_clear_ck() == 1) {
                        kind = 0;
                        act = 168;
                        arg = row->arg_0x04;
                        aim = (u16)calcVecAng2(&self->vec_0x03C, &pos);
                        break;
                    }
                    *fired = 0;
                    continue;
                default:
                    *fired = 0;
                    continue;
                }
                break;
            case 254:
                if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                            (s32*)&self->field_0x474) != 0xFFFF) {
                self->field_0x478 = hit_kind;
                switch (hit_kind) {
                case 0:
                case 16:
                    kind = 0;
                    switch (self->kind_0x09) {
                    case 1:
                        act = 45;
                        break;
                    case 3:
                        act = 98;
                        break;
                    default:
                        act = 44;
                        break;
                    }
                    arg = row->arg_0x04;
                    break;
                case 7:
                    if (self->kind_0x09 != 3) {
                        kind = 0;
                        act = 127;
                        arg = row->arg_0x04;
                        break;
                    }
                    *fired = 0;
                    continue;
                case 2:
                    kind = 0;
                    switch (self->kind_0x09) {
                    case 1:
                        act = 52;
                        break;
                    case 3:
                        act = 97;
                        break;
                    default:
                        act = 51;
                        break;
                    }
                    arg = row->arg_0x04;
                    break;
                case 8:
                    if ((u32)(u32)pl_item_menu_open_ck(self) == 1) {
                        *fired = 0;
                        continue;
                    }
                    kind = 0;
                    act = 70;
                    arg = row->arg_0x04;
                    break;
                case 6:
                    if (PlayMode_ck() == 2 || (u32)pl_item_menu_open_ck(self) == 1) {
                        *fired = 0;
                        continue;
                    }
                    kind = 0;
                    act = 87;
                    arg = row->arg_0x04;
                    break;
                case 10:
                    if ((u32)pl_item_menu_open_ck(self) == 1 || Pl_motion_input_ck(0) == 1) {
                        *fired = 0;
                        continue;
                    }
                    kind = 0;
                    act = 88;
                    arg = row->arg_0x04;
                    break;
                default:
                    *fired = 0;
                    continue;
                }
                self->field_0x309 |= 0x20;
                break;
                }
                *fired = 0;
                continue;
            case 253:
                if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &hit_kind, &pos, &rot,
                                            (s32*)&self->field_0x474) != 0xFFFF) {
                slot = 0xFFFF;
                self->field_0x478 = hit_kind;
                switch (hit_kind) {
                case 3:
                    slot = pl_kind_item_equip_find(self, 0);
                    if (slot == 0xFFFF) {
                        slot = pl_kind_item_slot_find(self, 0);
                    }
                    break;
                case 4:
                    slot = pl_kind_item_equip_find(self, 1);
                    if (slot == 0xFFFF) {
                        slot = pl_kind_item_slot_find(self, 1);
                    }
                    break;
                case 5:
                    slot = pl_kind_item_equip_find(self, 2);
                    if (slot == 0xFFFF) {
                        slot = pl_kind_item_slot_find(self, 2);
                    }
                    break;
                }
                if (slot != 0xFFFF && !(slot & 0x80)) {
                    arg = row->arg_0x04;
                    if (pl_item_use_act_pick(self, &kind, &act, &arg, &aim, slot, &point) == 0) {
                        *fired = 0;
                        continue;
                    }
                    slot_item = self->slot_id[slot].item_id;
                    self->field_0x309 |= 0x20;
                    break;
                }
                *fired = 0;
                continue;
                }
                *fired = 0;
                continue;
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                remap = pl_scan_kind_remap[row->key_0x02];
                remap += self->field_0x002 * 3;
                if (remap[0] == 0 && (remap[1] == 0 || remap[1] == 22)) {
                    *fired = 0;
                    continue;
                }
                kind = remap[0];
                act = remap[1];
                arg = remap[2];
                switch (self->field_0x002) {
                case 7:
                    if (kind == 4) {
                        switch (act) {
                        case 20:
                            if (pl_daze_level_ck(self, 15) == 0) {
                                act = 21;
                            }
                            break;
                        case 37:
                            if (pl_daze_level_ck(self, 15) == 0) {
                                act = 38;
                            }
                            break;
                        }
                    }
                    break;
                case 8:
                    if (kind == 4) {
                        switch (act) {
                        case 15:
                        case 80:
                            if (pl_gauge_level_get(self) >= 2) {
                                act = 70;
                            }
                            break;
                        case 60:
                        case 81:
                            if (pl_gauge_level_get(self) >= 2) {
                                act = 71;
                            }
                            break;
                        }
                    }
                    break;
                }
                break;
            case 252:
                kind = 4;
                shell = pl_shell_class_get(self, self->held_item_kind_0x26C);
                if (self->kind_0x09 == 3) {
                    act = pl_shell_act_tbl_a[(s16)shell * 2 + 1];
                } else {
                    act = pl_shell_act_tbl_a[(s16)shell * 2];
                }
                arg = row->arg_0x04;
                break;
            case 251:
                kind = 4;
                shell = pl_shell_class_get(self, self->held_item_kind_0x26C);
                if (self->kind_0x09 == 3) {
                    act = pl_shell_act_tbl_b[(s16)shell * 2 + 1];
                } else {
                    act = pl_shell_act_tbl_b[(s16)shell * 2];
                }
                arg = row->arg_0x04;
                break;
            case 249:
            case 250:
                if (self->field_0x5C8 != 0) {
                    kind = 11;
                    if (row->key_0x02 == 250) {
                        act = pl_armed_act_no_get_c1(self, 0);
                    } else {
                        act = pl_armed_act_no_get_c1(self, 1);
                    }
                    arg = row->arg_0x04;
                } else {
                    *fired = 0;
                }
                break;
            case 248:
            case 247:
                if (row->key_0x02 == 248) {
                    probe = pl_float_zero;
                } else {
                    probe = pl_frame_window_1;
                }
                if (quest == NULL) {
                    *fired = 0;
                } else {
                    kind = 9;
                    arg = row->arg_0x04;
                    switch (quest->result_0x0FA) {
                    case 5:
                    case 6:
                        if (pl_float_zero == probe) {
                            act = 3;
                        } else {
                            act = 7;
                        }
                        break;
                    case 4:
                        if (pl_float_zero == probe) {
                            act = 2;
                        } else {
                            act = 6;
                        }
                        break;
                    case 7:
                        if (self->field_0x585 == 0) {
                            if (pl_float_zero == probe) {
                                act = 4;
                            } else {
                                act = 8;
                            }
                        } else {
                            *fired = 0;
                        }
                        break;
                    case 8:
                        if (pl_float_zero == probe) {
                            act = 5;
                        } else {
                            act = 9;
                        }
                        break;
                    }
                }
                break;
            default:
                *fired = 0;
                continue;
            }
        } else {
            kind = row->id_0x00 & 0x7F;
            act = row->key_0x02;
            arg = row->arg_0x04;
        }
        if (stop < pl_frame_window_1) {
            if (at_end != 0) {
                if (*fired == 0 || Pl_motion_end_ck(self) == 0) {
                    continue;
                }
            } else if (*fired == 0 || Pl_frame_check(self, 6, start, stop) == 0) {
                continue;
            }
        } else if (*fired == 0 || Pl_frame_check(self, 7, start, stop) == 0) {
            continue;
        }
        if (no_act == 0) {
            if (kind == 0 && act == 0) {
                Pl_act_set_motion_slot(self, 0, 10, 0);
            } else {
                if (kind == 1) {
                    switch (act) {
                    case 1:
                    case 2:
                    case 14:
                        if (pl_act_param_tier_ck(self, 0) >= 1) {
                            pl_act_set_cam_ang(self);
                            if ((u32)((u16)(self->field_0x0A8 + 0x10000 - self->field_0x058) - 0x6000) <= 0x4000) {
                                kind = 1;
                                act = 4;
                                arg = 0;
                            }
                        }
                        break;
                    case 0:
                    case 10:
                    case 11:
                    case 13:
                    case 16:
                    case 17:
                    case 24:
                    case 28:
                    case 29:
                    case 35:
                    case 37:
                    case 39:
                        if (pl_act_param_tier_ck(self, 0) >= 1) {
                            pl_act_set_cam_ang(self);
                        }
                        break;
                    case 6:
                    case 7:
                        if (pl_act_dash_input_c1(self) == 0 && self->field_0x018 == 0 &&
                            pl_act_param_tier_ck(self, 0) >= 1) {
                            pl_act_set_cam_ang(self);
                            if ((u32)((u16)(self->field_0x0A8 + 0x10000 - self->field_0x058) - 0x6000) <= 0x4000) {
                                kind = 1;
                                act = 23;
                                arg = 0;
                            }
                        }
                        break;
                    }
                }
                if (slot_item != 0xFFFF) {
                    self->field_0x0A8 = aim;
                    self->field_0x306 = slot_item;
                } else if (kind == 0 && (u32)(act - 167) <= 1) {
                    self->field_0x0A8 = aim;
                }
                if (row->flags_0x06 & 0x4000) {
                    if (pl_act_param_tier_ck(self, 0) >= 1) {
                        pl_act_set_cam_ang(self);
                    }
                } else if (kind == 5) {
                    switch (act) {
                    case 0:
                    case 6:
                    case 12:
                        if (pl_act_param_tier_ck(self, 0) >= 1) {
                            pl_act_set_cam_ang(self);
                        }
                        break;
                    }
                }
                if ((kind == 0 && act == 20) || (kind == 0 && act == 158)) {
                    self->act_handler_entered = 1;
                }
                if (arg & 0x20) {
                    self->field_0x0B6 = point;
                }
                pl_act_enter_raw(self, kind, act, arg);
            }
        }
        return 1;
    }
    return 0;
}
#pragma optimization_level reset

/* 0x80264274 - adds `value` to the actor's hit-stop timer unless the skill overrides it. */
void fn_80264274(_PLW* self, s16 value) {
    if (Pl_Skill_ck(self, 1) == 0) {
        self->field_0x3EA += value;
        return;
    }
    self->field_0x3EA = 0;
}

/* 0x802642D0 (0x670): ticks the master actor's poison, the second damage-over-time timer and the hot/cold area
 * drain, each stepping the health gauge on its own period. */
#pragma optimization_level 4
void pl_condition_tick(_PLW* self) {
    s16 delta;
    s32 drain;
    s16 kind;
    u8 act_kind;

    if (self->field_0x00A == 8) {
        return;
    }
    self->field_0x389 = 0;
    if (Pl_master_ck(self) == 1) {
        if (self->field_0x3EA > 0 && --self->field_0x3EE <= 0) {
            self->field_0x3EA--;
            delta = -1;
            self->field_0x3DC |= 0x100000;
            if (Pl_Skill_ck(self, 2) == 1) {
                delta = -2;
            }
            self->field_0x3EE = 30;
            eft013_set(self, 2);
            if ((u32)pl_act_gauge_step_unlatched(self, delta, NULL) == 1) {
                return;
            }
        }
        if (self->kind_0x09 == 3) {
            if (self->field_0x404 > 1) {
                self->field_0x404 = 1;
            }
        } else if ((self->field_0x5A4 & 6) != 0) {
            switch (self->field_0x00A) {
            case 0:
                switch (self->act_no) {
                case 15:
                case 16:
                    switch (self->field_0x002) {
                    case 3:
                    case 6:
                    case 8:
                        break;
                    default:
                        if (self->field_0x404 > 1) {
                            self->field_0x404 = 1;
                        }
                        break;
                    }
                    break;
                case 8:
                case 9:
                case 62:
                case 181:
                    if (self->field_0x404 > 1) {
                        self->field_0x404 = 1;
                    }
                    break;
                }
                break;
            case 6:
                if ((u32)(self->act_no - 72) <= 1 && self->field_0x404 > 1) {
                    self->field_0x404 = 1;
                }
                break;
            }
        }
        if (self->field_0x404 > 0) {
            self->field_0x3DC |= 0x100000;
            if (pl_act_held_ck_c1(self) == 0) {
                self->field_0x404--;
                delta = Pl_dm_condition_ck(self, 2) == 1 ? -4 : -2;
                if (self->field_0x404 <= 0) {
                    self->field_0x3DC &= ~3;
                }
                if (self->field_0x404 % 90 == 0 && fn_8027E1E4(self) == 0 &&
                    (u32)pl_act_gauge_step_signed(self, delta, NULL) == 1) {
                    return;
                }
            }
        }
    } else if (Pl_condition_ck(self, 1) == 1 && (s32)self->frame_0x020 % 30 == 0) {
        eft013_set(self, 2);
    }
    if (Pl_master_ck(self) == 1) {
        s32 heat = fn_8027D40C(self);

        if (heat > 0) {
            self->field_0x3DC |= 0x100000;
            if ((s32)self->frame_0x020 % 32 == 0 && (u32)pl_act_gauge_step_unlatched(self, -heat, NULL) == 1) {
                return;
            }
        }
        drain = fn_80260A18(self);
        if (drain > 0) {
            self->field_0x3DC |= 0x100000;
            if ((s32)self->frame_0x020 % 60 == 0 && (u32)pl_act_gauge_step_unlatched(self, -drain, NULL) == 1) {
                return;
            }
        }
    }
    if (Pl_master_ck(self) != 1) {
        return;
    }
    kind = pl_act_kind_get(self->area_0x16);
    if ((kind == 1 || kind == 3) && self->field_0x464 == 0 &&
        (kind != 1 || (Pl_Skill_ck(self, 124) != 1 && Pl_Skill_ck(self, 125) != 1)) &&
        (kind != 3 || Pl_Skill_ck(self, 125) != 1)) {
        self->field_0x3DC |= 0x100000;
        if (--self->area_drain_timer_0x462 <= 0) {
            if (kind == 1) {
                self->area_drain_timer_0x462 = 90;
                if (Pl_Skill_ck(self, 126) == 1) {
                    self->area_drain_timer_0x462 = 60;
                } else if (Pl_Skill_ck(self, 127) == 1) {
                    self->area_drain_timer_0x462 = 45;
                }
            } else {
                self->area_drain_timer_0x462 = 60;
                if (Pl_Skill_ck(self, 126) == 1) {
                    self->area_drain_timer_0x462 = 40;
                } else if (Pl_Skill_ck(self, 127) == 1) {
                    self->area_drain_timer_0x462 = 30;
                } else if (Pl_Skill_ck(self, 124) == 1) {
                    self->area_drain_timer_0x462 = 180;
                }
            }
            if ((u32)pl_act_gauge_step_unlatched(self, -1, NULL) == 1) {
                return;
            }
        }
    }
    if (Pl_Skill_ck(self, 125) == 1) {
        return;
    }
    if ((self->field_0x5A4 & 0x200) != 0) {
        if (Pl_cat_skill_ck(self, 47) == 0) {
            self->field_0x3DC |= 0x100000;
        }
        act_kind = self->field_0x00A;
        if (act_kind != 6) {
            if ((u8)(act_kind + 248) <= 1) {
                return;
            }
            if (self->kind_0x09 != 2 && pl_act_held_ck_c1(self) != 1) {
                self->field_0x389 = 1;
                if ((s32)self->frame_0x020 % 8 == 0 && Pl_cat_skill_ck(self, 47) == 0 &&
                    (u32)pl_act_gauge_step_unlatched(self, -1, NULL) == 1) {
                    return;
                }
                if (self->frame_0x020 % 16 == 0) {
                    eft004_set_pl2(self, 39, 255, pl_frame_window_1, pl_float_zero, pl_float_zero, 0);
                }
            }
        }
    } else if ((self->field_0x5A4 & 0x400) != 0 && Pl_Skill_ck(self, 1) == 0) {
        self->field_0x389 = 1;
        if (self->field_0x3EA < 10) {
            self->field_0x3EA = 10;
        }
    }
}

#pragma optimization_level reset

/* 0x80264940 - accumulates a u16 into the two damage counters it feeds. */
void fn_80264940(_PLW* self, u32 value) {
    u16 sum = self->field_0x398 + (u16)value;

    self->field_0x058 = sum;
    self->field_0x0A8 = sum;
}

/* 0x8026495C - the actor's ball-spin charge level after the skill adjustments. */
u8 fn_8026495C(_PLW* self) {
    u8 level = self->field_0x3A1;

    if (Pl_Skill_ck(self, 0x22) == 1) {
        if (level > 0x15) {
            level -= 0x14;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x21) == 1 || pl_status_bit_ck((_ENEMY_WORK*)self, 0x2000) != 0) {
        if (level > 0xB) {
            level -= 0xA;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x23) == 1) {
        level = (level < 0xF5) ? level + 0xA : 0xFF;
    }
    return level;
}

/* 0x80264A28 - converts the ball-spin charge level into the motion's knock-back distance. */
void pl_sub_gauge_level_cost(_PLW* self) {
    u8 level = fn_8026495C(self);
    s16 distance = -0x96;

    if (level >= 0x28) {
        distance = -0x168;
    } else if (level >= 0xF) {
        distance = -0xC8;
    }
    pl_sub_gauge_skill_add(self, distance);
}

/* 0x80264A84 - the motion's charge tier for the actor's current state. */
s32 fn_80264A84(_PLW* self) {
    u8 level = fn_8026495C(self);
    s32 tier = 0;

    switch ((u32)(self->field_0x002 - 4) <= 2 ? 1 : self->field_0x002) {
    case 0:
        if (level >= 0x28) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 1:
        if (level >= 0x15) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 3:
        if (level >= 0x32) {
            tier = 2;
        } else if (level >= 0x28) {
            tier = 1;
        }
        break;
    }
    return tier;
}

/* 0x80264B4C - the motion belongs to the "uncontrollable" set for the actor's current state. */
s32 fn_80264B4C(_PLW* self) {
    switch (self->field_0x00A) {
    case 0:
        switch (self->act_no) {
        case 23: case 24: case 25: case 26:
        case 81: case 82:
        case 100: case 101:
        case 140: case 141: case 142: case 143: case 144: case 145: case 146: case 147: case 148: case 149:
            return 1;
        }
        break;
    case 1:
        switch (self->act_no) {
        case 20:
            return 1;
        }
        break;
    case 6:
        switch (self->act_no) {
        case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        case 12: case 13:
        case 26:
        case 38:
        case 55: case 56: case 57: case 58:
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80264C3C (0x268): whether the actor guards a hit from `angle`: the guard state, the weapon kind, the stagger
 * budget and the facing window must all allow it, then the guard-up status or an auto-guard scan row decides. */
extern "C" s32 pl_guard_ck(_PLW* self, s32 angle)
{
    s32 guarded;
    s32 auto_guard;
    u32 rows;
    PlScanEntry* row;
    u16* remap;
    u32 mode;
    s32 key;
    s32 kind;
    u8 weapon;

    guarded = 0;
    auto_guard = 0;
    if (self->field_0x38A < 0) {
        return 0;
    }
    if ((u32)(self->field_0x39F - 27) <= 4) {
        return 0;
    }
    if ((self->field_0x3A0 & 0x20) != 0 && Pl_Skill_ck(self, 37) == 0) {
        return 0;
    }
    if (pl_weapon_guard_ck(self) == 0) {
        return 0;
    }
    if (pl_stagger_budget_ck(self, 150) == 0) {
        return 0;
    }
    if ((self->field_0x3A0 & 0x40) == 0) {
        kind = self->field_0x002;
        if ((u32)(kind - 4) > 2 && kind != 1) {
            if ((u16)(angle + 0x71C6) <= 0x638C) {
                return 0;
            }
        } else if ((u16)(angle + 0xF1C7) > 0x638E) {
            return 0;
        }
    }
    if (pl_status_bit_ck((_ENEMY_WORK*)self, 0x10) != 0) {
        guarded = 1;
    }
    if (Pl_Skill_ck(self, 38) == 1) {
        auto_guard = 1;
    }
    if ((u32)fn_80264B4C(self) != 1) {
        weapon = self->field_0x002;
        if ((u32)(weapon - 4) <= 2) {
            rows = self->field_0x318;
            if (rows == (u32)pl_scan_rows_guard_a || rows == (u32)pl_scan_rows_guard_b) {
                return 1;
            }
        }
        row = (PlScanEntry*)self->field_0x318;
        if (row != NULL && auto_guard != 0) {
            while ((mode = row->id_0x00) != 0xFF) {
                if ((mode & 0x80) != 0 && (row->key_0x02 == 2 || row->key_0x02 == 4)) {
                    remap = pl_scan_kind_remap[row->key_0x02] + self->field_0x002 * 3;
                    mode = (u8)remap[0];
                    key = (u8)remap[1];
                } else {
                    key = row->key_0x02;
                }
                if (((u8)mode & 0x7F) == 5) {
                    switch ((u16)key) {
                    case 0:
                    case 6:
                    case 12:
                        guarded = 1;
                        break;
                    }
                }
                row++;
            }
        }
    }
    return guarded != 0;
}

/* 0x80264EA4 - the actor's stamina/guard timer has not run out yet. */
s32 fn_80264EA4(_PLW* self) {
    return self->field_0x3FC >= 50;
}

/* 0x80264EC0 - the actor's poison timer is running. */
s32 fn_80264EC0(_PLW* self) {
    return self->field_0x416 > 0;
}

/* 0x80264ED4 - the actor's bleed timer is running. */
s32 fn_80264ED4(_PLW* self) {
    return self->field_0x41C > 0;
}

/* 0x80264EE8 - the "konchu ball" rolling state predicate. */
s32 fn_80264EE8(_PLW* self) {
    u16 motion;

    if (Pl_Skill_ck(self, 0xB2) != 1 && Pl_Skill_ck(self, 0xB3) != 1 && Pl_cat_skill_ck(self, 0x23) != 1) {
        return 0;
    }
    if (self->field_0x00A != 0) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 0x33) <= 1 || motion == 0x61) {
        return 1;
    }
    return 0;
}

/* 0x80264F78 - the "rolled up into a ball" predicate. */
s32 fn_80264F78(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x21) == 1 || Pl_cat_skill_ck(self, 0x22) == 1) && pl_act_no_eft_ck(self, 0) == 1 &&
        self->field_0x370 > 0) {
        return 1;
    }
    return 0;
}

/* 0x80264FF0 - the sleeping/paralysed state predicate. */
s32 fn_80264FF0(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x17) == 1 || Pl_cat_skill_ck(self, 0x18) == 1) && pl_carry_item_get(self) != 0xFFFF) {
        return 1;
    }
    return 0;
}

/* 0x8026505C - the actor is held/knocked down by any of the four status sources. */
s32 fn_8026505C(_PLW* self) {
    if (self->field_0x45A > 0) {
        return 1;
    }
    if (self->field_0x45C > 0) {
        return 1;
    }
    if (fn_80264EE8(self) == 1) {
        return 1;
    }
    if (fn_80264F78(self) == 1) {
        return 1;
    }
    return fn_80264FF0(self) == 1;
}

/* 0x802650EC - maps a motion id onto "the actor is locked in this motion". */
s32 fn_802650EC(_PLW* self, u8 motion) {
    switch (motion) {
    case 6:
    case 7:
    case 8:
        if (fn_8026505C(self) == 0) {
            return 0;
        }
        if (fn_80264EE8(self) == 1 && self->field_0x45A <= 0 && self->field_0x45C <= 0) {
            return 0;
        }
        return 1;
    case 1:
    case 17:
    case 18:
    case 24:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        return fn_8026505C(self) != 0;
    default:
        return 0;
    }
}

/* Enters the knock-back act `front` or `back` of act class 6 (class 7 for `side` 4) by the hit's `angle`, turning
 * the actor for the sided variants; `drawn` picks the act flags. */
extern "C" void pl_act_knockback_enter(_PLW* self, u16 angle, u16 front, u16 back, u8 side, u8 drawn) {
    u16 flags;

    flags = 0x8C;
    if (drawn == 0) {
        flags = 0xC;
    }
    switch (side) {
    case 0:
        if (angle <= 0x8000) {
            pl_act_enter(self, 6, front, flags);
        } else {
            pl_act_enter(self, 6, back, flags);
        }
        break;
    case 1:
    case 5:
        if (angle <= 0x8000) {
            fn_80264940(self, 0);
            pl_act_enter(self, 6, front, flags);
        } else {
            fn_80264940(self, 0x8000);
            pl_act_enter(self, 6, back, flags);
        }
        break;
    case 2:
        fn_80264940(self, 0);
        pl_act_enter(self, 6, front, flags);
        break;
    case 3:
        fn_80264940(self, 0x8000);
        pl_act_enter(self, 6, back, flags);
        break;
    case 4:
        fn_80264940(self, 0);
        pl_act_enter(self, 7, front, flags);
        break;
    }
}

/* 0x80265348 - the actor is in the "konchu ball bounce" motion pair. */
s32 fn_80265348(_PLW* self) {
    if (self->field_0x00A == 6 && (u32)(self->act_no - 0x1F) <= 3) {
        return 1;
    }
    return 0;
}

/* 0x80265374 (0x378): applies the pending status build-ups to their timers and enters the stun or faint act
 * they trigger; returns 1 when it entered an act. */
extern "C" s32 pl_status_pending_apply(_PLW* self)
{
    s16 amount;
    s32 i;

    if ((s16)self->health <= 0) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (self->status_pending_0x424[i] > 0) {
            if (Pl_cat_skill_ck(self, 46) == 1) {
                self->status_timer_0x42E[i] = 1800;
            } else {
                self->status_timer_0x42E[i] = 3600;
            }
        }
    }
    if (self->field_0x420 > 0) {
        if (Pl_cat_skill_ck(self, 46) == 1) {
            self->field_0x422 = 2700;
        } else {
            self->field_0x422 = 5400;
        }
    }
    if (self->field_0x3FE > 0) {
        if (Pl_Skill_ck(self, 8) == 1 || Pl_act_ck(self, 6, 27) == 1 || Pl_act_ck(self, 6, 35) == 1) {
            self->field_0x3FE = 0;
            pl_act_guard_timer_reset(self);
        } else {
            amount = self->field_0x3FE;
            if ((Pl_dm_condition_ck(self, 0x20) == 1 || Pl_dm_condition_ck(self, 0x10) == 1) &&
                (pl_dm_condition_ex_ck(self, 0x20) == 1 || pl_dm_condition_ex_ck(self, 0x10) == 1)) {
                amount += (s16)(amount / 4);
            }
            self->field_0x3FC += amount;
            if (self->kind_0x09 == 3) {
                self->field_0x400 = 300;
            } else {
                self->field_0x400 = 150;
            }
            if ((u32)fn_80264EA4(self) == 1 && Pl_Skill_ck(self, 7) == 1 && ((u16)ran_suu(1) & 1) != 0) {
                self->field_0x3FE = 0;
                pl_act_guard_timer_reset(self);
            }
        }
    }
    if (self->field_0x3EC > 0 && Pl_Skill_ck(self, 1) == 0) {
        fn_80264274(self, self->field_0x3EC);
        self->field_0x3EE = 30;
    }
    if (self->field_0x40E > 0) {
        self->field_0x410 = 5400;
    }
    amount = self->field_0x3F2;
    if (amount > 0 && self->kind_0x09 != 3 && Pl_Skill_ck(self, 5) == 0) {
        self->field_0x3F0 += amount;
        self->field_0x3F4 += amount;
        if (self->field_0x00A != 6 || self->act_no < (s16)28 || self->act_no > (s16)29) {
            pl_act_guard_timer_reset(self);
            if (self->field_0x3F4 >= 200) {
                pl_act_enter(self, 6, 28, 0);
            } else {
                pl_act_enter(self, 6, 29, 0);
            }
        }
        return 1;
    }
    amount = self->field_0x3F8;
    if (amount > 0 && Pl_Skill_ck(self, 3) == 0) {
        self->field_0x3F6 += amount;
        if (fn_80265348(self) == 0) {
            if (self->kind_0x09 == 3) {
                pl_act_enter(self, 6, 33, 12);
            } else {
                pl_act_enter(self, 6, 31, 12);
            }
            return 1;
        }
    }
    return 0;
}

/* 0x802656EC - clears the two shell timers. */
void fn_802656EC(_PLW* self) {
    self->field_0x418 = 0;
    self->field_0x41E = 0;
}

/* 0x802656FC - the actor is in one of the "charge up a shot" motions. */
s32 fn_802656FC(_PLW* self) {
    u16 motion;

    if (self->field_0x00A != 5) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 3) <= 2 || (u32)(motion - 9) <= 2 || (u32)(motion - 16) <= 1) {
        return 1;
    }
    return 0;
}

/* 0x80265748 - the shell/element gauge is at or past its first charge step. */
u32 fn_80265748(_PLW* self) {
    u16 gauge = self->field_0x39C;

    if ((u16)(gauge + 0x1555) <= 0x2AAB) {
        return 0;
    }
    if (gauge < 0x8001) {
        return 2;
    }
    return 1;
}

/* 0x80265780 - fires the "draw the weapon" action request. */
void fn_80265780(_PLW* self) {
    if (self->kind_0x09 == 3) {
        pl_act_enter(self, 6, 0x22, 0);
        return;
    }
    pl_act_enter(self, 6, 0x20, 0);
}

/* 0x802657AC - the actor is aiming a shot. */
u32 fn_802657AC(_PLW* self) {
    if (self->kind_0x09 != 3) {
        return 0;
    }
    return Pl_act_ck(self, 1, 0x15) != 0;
}

/* 0x802657F8 (0x1300): applies the actor's pending hit - the guard reaction, the damage with its critical-hit
 * skills and the red health, the status the hit carries, and the knockback act its kind selects. */
#pragma optimization_level 4
s32 pl_hit_react(_PLW* self) {
    VEC3 pos;
    u8 guts;
    s16 dmg;
    s16 hp;
    u8 power;
    u8 kind;
    s32 pending;
    u16 guard_face;
    s32 crit;
    s32 mod;
    s32 face;
    s32 no_se;
    s16 now;

    no_se = 0;
    VEC3_ctor(&pos);
    if (self->hit_pending_0x39E == 0) {
        return 0;
    }
    if (self->field_0x00A == 8) {
        return 0;
    }
    pending = 0;
    kind = self->field_0x39F;
    dmg = self->field_0x38A;
    power = self->field_0x3A1;
    hp = self->health;
    guts = 0;
    face = (u16)(self->field_0x058 - self->field_0x398 + 0x4000);
    guard_face = self->field_0x0A8 - self->field_0x398 + 0x4000;
    do {
        if (pl_status_bit_ck((_ENEMY_WORK*)self, 0x10) != 0 && kind == 6) {
            continue;
        }
        switch (self->field_0x39F) {
        case 6:
        case 7:
        case 8:
        case 9:
        case 18:
        case 19:
            if ((u32)fn_802656FC(self) == 1) {
                continue;
            }
            break;
        }
        if ((u32)pl_guard_ck(self, guard_face) == 1 && Pl_master_ck(self) == 1) {
            switch ((u8)fn_80264A84(self)) {
            case 0:
                face = -1;
                if (fn_802657AC(self) == 1) {
                    if (self->field_0x002 == 3 && Pl_act_ck(self, 4, 44) == 1) {
                        pl_act_enter(self, 4, 47, 140);
                    } else if (self->field_0x002 != 3 || Pl_act_ck(self, 4, 49) != 1) {
                        pl_act_enter(self, 5, 9, 128);
                    }
                } else if (self->field_0x002 == 3 && Pl_act_ck(self, 4, 34) == 1) {
                    pl_act_enter(self, 4, 37, 12);
                } else if (self->field_0x002 != 3 || Pl_act_ck(self, 4, 39) != 1) {
                    pl_act_enter(self, 5, 3, 0);
                }
                break;
            case 1:
                face = -2;
                if (fn_802657AC(self) == 1) {
                    if (self->field_0x002 == 3) {
                        if (Pl_act_ck(self, 4, 44) == 1) {
                            pl_act_enter(self, 4, 48, 140);
                        } else {
                            pl_act_enter(self, 5, 10, 128);
                        }
                    } else {
                        pl_act_enter(self, 5, 10, 128);
                    }
                } else if (self->field_0x002 == 3) {
                    if (Pl_act_ck(self, 4, 34) == 1) {
                        pl_act_enter(self, 4, 38, 12);
                    } else {
                        pl_act_enter(self, 5, 4, 0);
                    }
                } else {
                    pl_act_enter(self, 5, 4, 0);
                }
                break;
            case 2:
                face = -10;
                if (self->field_0x3A0 & 0x20) {
                    dmg = dmg / 2;
                } else {
                    dmg = dmg / 4;
                }
                if ((u32)pl_health_damage_apply(self, -dmg, (s8*)&guts) == 1) {
                    pl_ride_down_set(self);
                    return 1;
                }
                now = self->health;
                if (now < hp) {
                    if (guts != 0) {
                        self->field_0x376 = 1;
                    } else {
                        self->field_0x376 = now + (hp - now) / 2;
                    }
                } else {
                    self->field_0x376 = now;
                }
                if (fn_802657AC(self) == 1) {
                    if (self->field_0x002 == 3 && Pl_act_ck(self, 4, 44) == 1) {
                        pl_act_enter(self, 5, 17, 128);
                    } else {
                        pl_act_enter(self, 5, 11, 128);
                    }
                } else if (self->field_0x002 == 3 && Pl_act_ck(self, 4, 34) == 1) {
                    pl_act_enter(self, 5, 16, 0);
                } else {
                    pl_act_enter(self, 5, 5, 0);
                }
                break;
            }
            if (self->field_0x002 == 0 && dmg > 0) {
                pl_hit_face_motion_c1(self, face, 0);
            }
            continue;
        }
        if (Pl_master_ck(self) != 1) {
            continue;
        }
        pl_status_duration_map(self);
        if (dmg != 0) {
            if (dmg > 0 && self->health > 0) {
                crit = 0;
                if ((ran_suu(1) & 3) == 0) {
                    if (Pl_Skill_ck(self, 86) == 1) {
                        dmg = pl_float_0_7 * dmg;
                        if (dmg == 0) {
                            dmg = 1;
                        }
                        crit |= 1;
                    } else if (Pl_Skill_ck(self, 87) == 1) {
                        dmg = pl_float_1_3 * dmg;
                        crit |= 2;
                    }
                }
                mod = 0;
                if (Pl_cat_skill_ck(self, 7) == 1) {
                    mod = 4;
                } else if (Pl_cat_skill_ck(self, 8) == 1) {
                    mod = 8;
                }
                if (mod != 0 && ran_suu(1) % mod == 0) {
                    dmg = pl_float_0_7 * dmg;
                    if (dmg == 0) {
                        dmg = 1;
                    }
                    crit |= 4;
                }
                if (((u8)crit & 1) || ((u8)crit & 4)) {
                    eft013_set(self, 32);
                } else if ((u8)crit & 2) {
                    ((PlBodyWork*)self->physics_0x13C)->chr_0x04.get_joint_wpos(3, &pos);
                    eft_model_effect_spawn(&pos, 0, 0, self->area_0x16, &self->rot_0x54, pl_frame_window_1);
                }
            }
            if ((u32)pl_health_damage_apply(self, -dmg, (s8*)&guts) == 1) {
                return 1;
            }
            now = self->health;
            if (now < hp) {
                if (guts != 0) {
                    self->field_0x376 = 1;
                } else {
                    self->field_0x376 = now + (hp - now) / 2;
                }
            } else if (self->field_0x376 < now) {
                self->field_0x376 = now;
            }
        }
        if ((u32)pl_status_pending_apply(self) == 1) {
            pending = 1;
        }
        switch (kind) {
        case 23:
            if (fn_802657AC(self) == 0 && Pl_Skill_ck(self, 10) == 0) {
                self->field_0x416 = 10;
            }
            break;
        case 24:
            if (fn_802657AC(self) == 0 && Pl_Skill_ck(self, 10) == 0) {
                self->field_0x41C = 10;
            }
            break;
        case 17:
            fn_8027D4F0(self);
            break;
        }
        if (pending != 0) {
            continue;
        }
        if ((u32)fn_802650EC(self, kind) == 1) {
            no_se = 1;
            if ((u32)fn_80264EA4(self) == 1) {
                no_se = 0;
                if (fn_802657AC(self) == 1) {
                    pl_act_enter(self, 6, 35, 0);
                } else {
                    pl_act_enter(self, 6, 27, 0);
                }
            } else {
                if ((u32)fn_80264EC0(self) == 1 && fn_802657AC(self) == 0) {
                    no_se = 0;
                    pl_act_enter(self, 6, 43, 0);
                }
                if ((u32)fn_80264ED4(self) == 1 && fn_802657AC(self) == 0) {
                    no_se = 0;
                    pl_act_enter(self, 6, 63, 0);
                }
            }
            continue;
        }
        now = self->health;
        if (now > 0 || self->kind_0x09 == 2) {
            switch (self->kind_0x09) {
            case 2:
                switch (kind) {
                case 27:
                case 28:
                case 29:
                case 30:
                case 31:
                case 2:
                case 15:
                    break;
                case 32:
                case 33:
                case 3:
                    power = 40;
                    kind = 2;
                    break;
                default:
                    power = 40;
                    kind = 2;
                    break;
                }
                break;
            }
            if (now <= 0) {
                power = 40;
                kind = 2;
            }
            switch (kind) {
            default:
            case 0:
            case 1:
            case 10:
            case 16:
            case 17:
            case 22:
            case 23:
            case 24:
            case 34:
            case 35:
                if ((u32)fn_80265348(self) == 1) {
                    fn_80265780(self);
                } else {
                    fn_802656EC(self);
                    if (fn_802657AC(self) == 1) {
                        fn_80265748(self);
                        if (power > 10) {
                            pl_act_knockback_enter(self, face, 8, 9, 1, 1);
                        } else {
                            pl_act_knockback_enter(self, face, 10, 11, 1, 1);
                        }
                    } else {
                        if (kind == 24) {
                            self->field_0x41E = 450;
                        } else if (kind == 23) {
                            self->field_0x418 = 900;
                        }
                        if (power > 10) {
                            pl_act_knockback_enter(self, face, 2, 3, 1, 0);
                        } else {
                            pl_act_knockback_enter(self, face, 0, 1, 1, 0);
                        }
                    }
                }
                break;
            case 2:
            case 15:
            case 33:
                fn_802656EC(self);
                if (fn_802657AC(self) == 1) {
                    if (power > 30 && kind != 33) {
                        switch ((u8)fn_80265748(self)) {
                        case 0:
                            pl_act_knockback_enter(self, face, 12, 13, 5, 1);
                            break;
                        case 1:
                            pl_act_knockback_enter(self, face, 55, 57, 5, 1);
                            break;
                        case 2:
                            pl_act_knockback_enter(self, face, 56, 58, 5, 1);
                            break;
                        }
                    } else {
                        fn_80265748(self);
                        pl_act_knockback_enter(self, face, 39, 40, 5, 1);
                    }
                } else {
                    if (power > 30 && kind != 33) {
                        pl_act_knockback_enter(self, face, 4, 5, 1, 0);
                    } else {
                        pl_act_knockback_enter(self, face, 26, 38, 1, 0);
                    }
                    self->field_0x585 = 0;
                }
                break;
            case 3:
            case 32:
                fn_802656EC(self);
                if (fn_802657AC(self) == 1) {
                    switch ((u8)fn_80265748(self)) {
                    case 0:
                        pl_act_knockback_enter(self, face, 12, 13, 5, 1);
                        break;
                    case 1:
                        pl_act_knockback_enter(self, face, 55, 57, 5, 1);
                        break;
                    case 2:
                        pl_act_knockback_enter(self, face, 56, 58, 5, 1);
                        break;
                    }
                } else {
                    pl_act_knockback_enter(self, face, 6, 7, 1, 0);
                }
                self->field_0x585 = 0;
                break;
            case 4:
            case 5:
            case 36:
                if ((u32)fn_80265348(self) == 1) {
                    fn_80265780(self);
                    break;
                }
                switch (kind) {
                case 4:
                    if (Pl_Skill_ck(self, 88) == 1 || Pl_Skill_ck(self, 89) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (fn_802657AC(self) == 1) {
                        hp = 36;
                    } else {
                        hp = 14;
                    }
                    break;
                case 36:
                    if (Pl_Skill_ck(self, 89) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (fn_802657AC(self) == 1) {
                        hp = 60;
                    } else {
                        hp = 59;
                    }
                    break;
                case 5:
                    if (Pl_Skill_ck(self, 89) == 1) {
                        no_se = 1;
                        continue;
                    }
                    hp = fn_802657AC(self) == 1 ? 37 : 15;
                    break;
                }
                fn_802656EC(self);
                pl_act_knockback_enter(self, face, hp, hp, 0, 0);
                break;
            case 6:
            case 7:
            case 8:
            case 9:
                if (fn_802657AC(self) == 1) {
                    no_se = 1;
                    continue;
                }
                if ((u32)fn_80265348(self) == 1) {
                    fn_80265780(self);
                    break;
                }
                switch (kind) {
                case 8:
                    if (Pl_Skill_ck(self, 133) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (self->field_0x450 >= 2) {
                        no_se = 1;
                        continue;
                    }
                    break;
                case 6:
                case 7:
                    if (Pl_Skill_ck(self, 133) == 1 || Pl_Skill_ck(self, 132) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (self->field_0x450 >= 1) {
                        no_se = 1;
                        continue;
                    }
                    break;
                }
                switch (self->field_0x00A) {
                case 0:
                case 1:
                case 4:
                    if (kind != 9) {
                        if (self->kind_0x09 == 1) {
                            no_se = 1;
                            continue;
                        }
                        switch (self->field_0x002) {
                        case 2:
                            if (self->charge_gauge_0x310 > 0) {
                                no_se = 1;
                                continue;
                            }
                            break;
                        }
                    }
                    break;
                case 5:
                    if (kind == 6) {
                        no_se = 1;
                        continue;
                    }
                    if (kind != 9 && (u32)fn_802656FC(self) == 1) {
                        no_se = 1;
                        continue;
                    }
                    break;
                case 6:
                    if ((u32)(self->act_no - 16) <= 2 || (u32)(self->act_no - 2) <= 1) {
                        no_se = 1;
                        continue;
                    }
                    break;
                case 8:
                case 9:
                    no_se = 1;
                    continue;
                }
                fn_802656EC(self);
                switch (kind) {
                default:
                    pl_act_knockback_enter(self, face, 2, 3, 2, 0);
                    break;
                case 7:
                    pl_act_knockback_enter(self, face, 16, 16, 2, 0);
                    break;
                case 8:
                    pl_act_knockback_enter(self, face, 17, 17, 2, 0);
                    break;
                case 9:
                    if (Pl_Skill_ck(self, 133) == 1) {
                        pl_act_knockback_enter(self, face, 17, 17, 2, 0);
                    } else {
                        pl_act_knockback_enter(self, face, 18, 18, 2, 0);
                    }
                    break;
                }
                break;
            case 11:
            case 12:
                fn_802656EC(self);
                if (fn_802657AC(self) == 1) {
                    pl_act_knockback_enter(self, face, 10, 11, 0, 0);
                } else {
                    pl_act_knockback_enter(self, face, 0, 1, 0, 0);
                }
                break;
            case 13:
                fn_802656EC(self);
                if (fn_802657AC(self) == 1) {
                    pl_act_knockback_enter(self, face, 10, 11, 0, 0);
                } else {
                    fn_8025A7E0(self, 0);
                    pl_act_knockback_enter(self, face, 0, 0, 4, 0);
                }
                break;
            case 14:
                fn_802656EC(self);
                pl_act_enter(self, 7, 6, 12);
                break;
            case 18:
            case 19:
                if (fn_802657AC(self) == 0) {
                    no_se = 1;
                    continue;
                }
                if ((u32)fn_80265348(self) == 1) {
                    fn_80265780(self);
                    break;
                }
                switch (kind) {
                case 19:
                    if (Pl_Skill_ck(self, 196) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (self->field_0x451 >= 2) {
                        no_se = 1;
                        continue;
                    }
                    break;
                case 18:
                    if (Pl_Skill_ck(self, 196) == 1 || Pl_Skill_ck(self, 195) == 1) {
                        no_se = 1;
                        continue;
                    }
                    if (self->field_0x451 >= 1) {
                        no_se = 1;
                        continue;
                    }
                    break;
                }
                switch (self->field_0x00A) {
                case 0:
                case 1:
                case 4:
                    if (kind == 18) {
                        switch (self->field_0x002) {
                        case 2:
                            if (self->charge_gauge_0x310 > 0) {
                                no_se = 1;
                                continue;
                            }
                            break;
                        }
                    }
                    break;
                case 5:
                case 6:
                case 8:
                case 9:
                    no_se = 1;
                    continue;
                }
                fn_802656EC(self);
                fn_80265748(self);
                switch (kind) {
                default:
                    pl_act_knockback_enter(self, face, 19, 19, 2, 1);
                    break;
                case 19:
                    pl_act_knockback_enter(self, face, 20, 20, 2, 1);
                    break;
                }
                break;
            case 20:
            case 21:
                if (fn_802657AC(self) == 1) {
                    no_se = 1;
                    continue;
                }
                if ((u32)fn_80265348(self) == 1) {
                    fn_80265780(self);
                    break;
                }
                if (Pl_Skill_ck(self, 180) == 1) {
                    no_se = 1;
                    continue;
                }
                fn_802656EC(self);
                if (kind == 21) {
                    pl_act_knockback_enter(self, face, 22, 22, 0, 0);
                } else {
                    pl_act_knockback_enter(self, face, 21, 21, 0, 0);
                }
                break;
            case 25:
                pl_act_knockback_enter(self, face, 41, 41, 2, 1);
                break;
            case 26:
                pl_act_knockback_enter(self, face, 42, 42, 2, 0);
                break;
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
                no_se = 1;
                continue;
            }
            pl_act_hold_notify(self, 2);
        } else {
            pl_ride_down_set(self);
        }
    } while (0);
    if (no_se == 0) {
        se_slot0_pos_req(&self->vec_0x03C);
    }
    return 1;
}
#pragma optimization_level reset

/* 0x80266AF8 (0x3C0): takes the wide-area item effects the other players in the area aimed at this player
 * (their +0x65F bit for this player) and spawns the matching effect, at most once per 60 frames. */
#pragma optimization_level 4
extern "C" void pl_wide_area_effect_step(_PLW* self)
{
    s32 i;
    u16 count;
    _PLW* other;
    s32 bit;
    u16 act;

    if (PlayMode_ck() == 2) {
        if (self->field_0x00A == 8) {
            return;
        }
    } else if (Pl_master_ck(self) == 1 || self->field_0x00A == 8) {
        return;
    }
    other = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; i++, other++) {
        if (i == self->chunk_ofs || other->area_0x16 != self->area_0x16) {
            continue;
        }
        bit = 1 << self->chunk_ofs;
        if ((other->field_0x65F & bit) == 0 || other->field_0x00A != 0) {
            continue;
        }
        other->field_0x65F &= (u8)~bit;
        act = other->act_no;
        switch (act) {
        case 57:
        case 58:
            switch (other->field_0x306) {
            case 360:
                if (self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 0);
                }
                break;
            case 13:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 0);
                }
                break;
            case 123:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 0);
                }
                break;
            case 17:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 4);
                }
                break;
            case 134:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 1);
                }
                break;
            case 135:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 2);
                }
                break;
            case 368:
            case 391:
            case 392:
            case 393:
            case 394:
                if ((Pl_Skill_ck(other, 91) == 1 || Pl_Skill_ck(other, 92) == 1) && self->wide_area_hold_0x660 == 0) {
                    self->wide_area_hold_0x660 = 60;
                    ef_inst_spawn(self, 9);
                }
                break;
            }
            break;
        case 138:
            if (self->wide_area_hold_0x660 == 0) {
                self->wide_area_hold_0x660 = 60;
                switch (other->field_0x306) {
                case 362:
                    ef_inst_spawn(self, 0);
                    break;
                case 363:
                    ef_inst_spawn(self, 4);
                    break;
                case 364:
                    ef_inst_spawn(self, 1);
                    break;
                case 365:
                    ef_inst_spawn(self, 2);
                    break;
                }
            }
            break;
        }
    }
}
#pragma optimization_level reset

/* 0x80266EB8 - tells the SE layer the actor's action has finished (or was superseded). */
void fn_80266EB8(_PLW* self) {
    s32 settled = 0;
    u16 motion;

    if (Pl_master_ck(self) != 0) {
        if (self->field_0x00A == 0) {
            motion = self->act_no;
            if ((u32)(motion - 0x6F) <= 9 || (u32)(motion - 0x22) <= 3) {
                settled = 1;
            }
        }
        if (settled == 0) {
            fn_800DB2DC();
        }
    }
}

/* 0x80266F30 (0x340): runs the actor's per-frame tail: the drift, the wide-area effects, the lobby events the
 * area and roster hits raise, and the network position report. */
void pl_frame_post_step(_PLW* self) {
    MHchar* chr = &((PlBodyWork*)self->physics_0x13C)->chr_0x04;
    VEC3 pos;
    _CP_VECTOR rot;
    s32 word;
    s16 kind;
    u8 act_kind;

    VEC3_ctor(&pos);
    self->field_0x268 = 0;
    if (event_demo_ck() != 1) {
        if (self->drift_on_0x582 != 0) {
            addVec3To(&self->vec_0x03C, (VEC3*)self->drift_vel_0x574);
            fn_8025FF0C(self, 1);
            fn_8025FA00(self);
            pl_model_pos_sync(self);
            mhchar_model_mtx_update(chr);
        }
        if ((u32)pl_item_menu_open_ck(self) == 1 && pl_now_area_ck(self) == 1) {
            shell_se_req(self->field_0xAF4, &self->vec_0x03C, 4, (u32)self);
        }
        if (Pl_act_ck(self, 0, 20) == 1 || Pl_act_ck(self, 0, 158) == 1) {
            self->field_0x268 = 1;
        }
        if (self->field_0x018 == 0) {
            self->charge_gauge_0x310 = 0;
        } else {
            act_kind = self->field_0x00A;
            if (act_kind > 1 && (u32)(act_kind - 4) > 1) {
                self->charge_gauge_0x310 = 0;
            }
        }
        self->dm_condition_prev_0x3E0 = self->field_0x3DC;
        pl_wide_area_effect_step(self);
        pl_eft_func(self);
        fn_80266EB8(self);
        if (Pl_master_ck(self) == 1) {
            if ((self->kind_0x015 == 1 || self->kind_0x015 == 12) && (self->area_0x16 == 7 || self->area_0x16 == 12) &&
                self->kind_0x09 == 3) {
                lb_event_request(9);
            }
            if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, &self->vec_0x03C, &kind, &pos, &rot, &word) !=
                0xFFFF) {
                switch (kind) {
                case 8:
                    lb_event_request(12);
                    break;
                case 6:
                    lb_event_request(10);
                    break;
                case 10:
                    lb_event_request(11);
                    break;
                case 3:
                    lb_event_request(23);
                    break;
                case 16:
                    lb_event_request(22);
                    break;
                }
            }
            if (self->field_0x380 - self->field_0x37E >= self->field_0x380 / 20) {
                lb_event_request(24);
            }
            if (quest_move_sub_flag_get() != 0) {
                lb_event_request(16);
            }
        }
        if (isServerSelectState()) {
            if (Pl_master_ck(self) == 0) {
                return;
            }
            if ((u32)fn_80245E20(self) == 1) {
                if (self->field_0x00A != 8 && self->field_0x00A != 9 && pl_act_stage_get(self) == 0 &&
                    (s16)self->field_0x012 == 0 && system_w.field_0x0c % 15 == 0) {
                    Pl_net_send(self, 2, 0);
                }
            } else {
                Pl_net_send_hit_kind5(self->area_0x16, &self->vec_0x03C, self->field_0x058);
            }
        }
    }
}

/* 0x80267270 - forwards an action request to the actor's G3D work. */
void pl_model_state_set(_PLW* self, u32 action, s32 a, u16 b) {
    if (Pl_master_ck(self) != 0 || action == 3) {
        switch (action) {
        case 2:
            hud_item_msg_push(2, a, b);
            return;
        case 1:
            hud_item_msg_push(1, a, b);
            return;
        case 3:
            hud_item_msg_push(3, a, self->chunk_ofs);
            break;
        }
    }
}

/* Seeds the four chunk anchors the drop-in acts start from. */
extern "C" void pl_chunk_anchors_init(void) {
    setVec3(&pl_chunk_anchors[0], pl_float_neg152, pl_float_zero, pl_frame_window_2048);
    setVec3(&pl_chunk_anchors[1], pl_frame_window_181, pl_float_zero, pl_frame_window_2051);
    setVec3(&pl_chunk_anchors[2], pl_float_neg152, pl_float_zero, pl_frame_window_2048);
    setVec3(&pl_chunk_anchors[3], pl_frame_window_181, pl_float_zero, pl_frame_window_2051);
}
