/* ef/eft_slot.cpp - the `_EFT` family's slot pool at `.text` 0x803432B4..0x80349DD8 (92 functions).
 * The range's still-unwritten entries keep the map's `fn_` stems (`fn_803432B4`, the head, is the one
 * `ef/eft050.cpp`'s state dispatcher calls); every symbol this file DEFINES is named from its own body
 * (NAMES below, and the evidence next to each declaration).  The escape below records what is left.
 *
 * WHAT IT IS.  Every body drives the 0x48-byte effect instance `_EFT` (`include/ef.h`): the
 * range reads `flag_0x01`, `state_0x05`, `field_0x06`, `timer_0x0C`, `pos_0x18`, `rot_0x24`,
 * `work_0x38` and `area_0x44`, spawns and releases models through `ef/eft_res.cpp`'s
 * `res_eft_model_create`/`res_eft_UV_model_create`, and gates on the effect manager's
 * `eft_control`.  The `.data` band at 0x805E7F80..0x805E9200 belongs to the same code band: its
 * records hold this range's function pointers (the five three-entry sets
 * 0x805E7FB0/0x805E87F0/0x805E8B48/0x805E9118/0x805E9140 are one `{init, step, exit}` triple each)
 * and its `jumptable_805E918C` is `enemy_data_grp`'s own switch table.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: the only `lis`/`addi` pairs that do not resolve to a call are the `.sdata2` pool words
 * 0x8079B320..0x8079B360, and no `.rodata` string is referenced at all.  2. `dumpmap.py lookup`
 * answers only `zz_` placeholders (see the rule-7 line above).  3. The module is `ef` from the code:
 * the range's `self` is `_EFT` field for field (`+0x01`/`+0x05`/`+0x06`/`+0x0C`/`+0x18`/`+0x38`/
 * `+0x44`, the `include/ef.h` layout the sibling `ef/eft035.cpp` measured), it calls
 * `res_eft_model_create`/`res_eft_UV_model_create` (`ef/eft_res.cpp`) and the effect manager
 * `eft_control`; 4. nothing names the range - `dumpmap.py lookup` answers the dump's `zz_XXXXXXXX_`
 * placeholder for its entry points (`fn_803432B4`, `enemy_data_grp`, `eft_slot_spawn_targets`,
 * `fn_80344658`, `fn_80347B54`, `fn_80349398`, `fn_80349928` all checked) and no `__FILE__` string is
 * reachable, so every name this file defines is DERIVED from its own body and is a guess a later pass
 * may refine.
 *
 * NAMES.  The file is `eft_slot` because the range is the `eft` family's 10-entry slot pool
 * (`lbl_806BF0A0`, one 0x3C-byte `EftSlot` per entry - the record `include/enemy/ENEMY_DATA.h` also
 * views as the enemy per-entry data record) plus the enemy-record scan that drives it, and the family
 * dispatches into it from the `.data` tables above.  The 34 definitions, by body:
 *
 *   * the pool: `eft_slot_clear`/`eft_slot_pool_clear` (one / all entries cleared, the +0x14 byte
 *     stamped 255), `eft_slot_find_free` (first entry with a zero key, 255 = full), `eft_slot_find`
 *     (the (key_0x00, key_0x01) lookup), `eft_slot_spawn` (find-or-allocate and seed the whole state
 *     block), `eft_slot_spawn_targets` (one spawn per live enemy work record and per live target
 *     record);
 *   * `enemy_data_find`/`enemy_data_grp` are named for the ENEMY band's use, not this one's: its
 *     20+ call sites (`enemy/fn_8013BE60.c`, `enemy/fn_80165FC8.cpp`, `enemy/fn_80170600.cpp`,
 *     `enemy/em_action.cpp`) read the returned record as `_ENEMY_DATA` (its one home is
 *     `include/enemy/ENEMY_DATA.h`) and call `enemy_data_grp(team, id)` for the group the table is
 *     keyed on.  This unit owns both addresses, so the names have to serve those call sites;
 *   * the definition table (`lbl_805E9168`, the `EftDef` records): `eft_def_get`, `eft_def_flags`,
 *     `eft_def_handler`, `eft_def_model_block`;
 *   * the predicates: `eft_slot_armed_ck`, `eft_slot_live_ck` (a definition flag bit AND a non-zero
 *     +0x18, the entry's live enemy work), `eft_slot_persist_ck`, `eft_slot_area_ck`;
 *   * the per-frame work: `eft_slot_work_bind`, `eft_slot_work_update`, `eft_slot_counters_step`,
 *     `eft_slot_kind_set`, `eft_slot_effect_key`, `eft_slot_state_set`, `eft_slot_state_request`;
 *   * the record match tests: `eft_work_match_ck`/`eft_target_match_ck` (the exact key) and
 *     `eft_work_wide_ck`/`eft_target_wide_ck` (the wider one), `eft_slot_match_count`;
 *   * the (mode, value) pair the record keeps at +0x0F/+0x10: `eft_slot_mode_set`,
 *     `eft_slot_mode_set_imm`/`eft_slot_mode_set_defer` (the two wrappers), `eft_slot_mode_set_map`
 *     (mode/kind/value mapped onto the pair) and `eft_slot_mode_set_work` (the work record rewrites
 *     the triple first);
 *   * `eft_state_advance`/`eft_instance_release`: the two instance arms, `ef/eft050.cpp`'s state
 *     dispatcher cases 2 and 3, named for what they do to the `_EFT`.
 *
 * A later pass may refine any of them; only the bodies support them.  The record's own type is still
 * this file's `EftSlot` view (its fields are the ones these bodies read) - folding it onto
 * `_ENEMY_DATA` is a shared-file change, recorded in this unit's outbox rather than smuggled in here.
 *
 * Naming note: the 34 symbols this file defines are named above, so the escape covers only the
 * names it REFERENCES - this range's own still-unwritten entries (`fn_803432B4`, `fn_80345A2C`) and
 * eight callees other units own (`fn_800F886C`, `fn_80125FF0`, `fn_8012A9E8`, `fn_8012D1A8`,
 * `fn_80143BF8`, `fn_803386C4`, `fn_8042CB9C`, `fn_8042CC20`).
 *
 * SEAM (UNPROVEN - not settled; the merger round re-checked it from the DOL and left it that way).  The
 * range is an `attribute.py` `--max-bytes` cut, and the code band above it (0x8033F270..0x803432B4) is
 * the same `_EFT` family - `fn_80343130`, which ends where this range starts, already takes an `_EFT*`
 * and calls `res_eft_UV_model_create`.  Falsification tests, all negative (so neither edge is FALSE):
 *   * `__FILE__` copies: 117 single-copy file-name strings in the DOL; the only two in the whole region
 *     0x8030121C..0x8034C0C4 are `menu_infomation.cpp` @0x805DCCDC (referrers all in
 *     0x8030A328..0x8031A244) and `menu_note.cpp` @0x805E91F8 (sole referrer fn_8034C0C4, above the
 *     right edge).  Neither is cited on both sides of either edge, and no string covers this band.
 *   * data straddle: of every `.data`/`.sdata`/`.sdata2`/`.rodata` label with a code referrer, **0** are
 *     referenced from both sides of either edge.
 *   * `extab`/`extabindex`: the section is globally sorted by function address (11,210 entries, only 4
 *     non-ascending, all in the trailing pad), so the entry ORDER carries no object-chunk information;
 *     this band's 70-entry run names exactly this range's functions, ascending, its last entry
 *     (fn_80349C9C + 0x13C) ends exactly on 0x80349DD8, and its extab pointers are exactly the
 *     contiguous block 0x80016DB4..0x80016FE4 - self-consistent tiling, but the run boundary is derived
 *     from the registration, so it cannot pin the edge.
 *   * alignment: no gap at either edge (fn_80343130 + 0x184 = 0x803432B4, fn_80349C9C + 0x13C =
 *     0x80349DD8) and only 9 `gap_` symbols exist repo-wide, so the `-func_align` padding oracle is empty.
 * What the reliable `.sdata2` class does say is only a BRACKET: this band's own pool run is exactly
 * 0x8079B320..0x8079B368, and the next owner above it is fn_8034CDDC, so the object containing the band
 * ends at a function start in (0x8034782C, 0x8034CDDC] - 0x80349DD8 is one of ~24 candidates.  The left
 * hand-off (0x8079B31C owned by fn_80343130 -> 0x8079B320 owned by fn_803432B4) is adjacent+ascending,
 * which is NOT evidence: the PROVEN boundary at `menu_note.cpp`'s TU has the identical shape
 * (lbl_805E91E8/fn_8034A448 -> lbl_805E91F8/fn_8034C0C4).  The `.data` fragment edge at 0x805E91E8 is a
 * candidate only (~7 % noise floor).  The Dolphin map is no help either: 92 entries for this range's
 * `.text`, only 3 non-`zz_` (`J3DColorBlockLightOff::getColorChanNum(void)` @0x80343AE8, `DBClose`
 * @0x803489F0, `FUN_80348a14` @0x80348A14 - the last is a Ghidra auto-name), and its `_<hex>` prefix is
 * NOT the owner (measured here: `_8034239cswitchdataD_805e918c` names a function in the band BELOW,
 * while the DOL's only referrer of that table is enemy_data_grp - the same correction playbook 54 records
 * for the `__FILE__` strings, extended to the `switchdataD_` entries).  So the range is registered whole
 * with both edges where the proposal put them; matching is the arbiter, and the land-or-delete call is
 * the orchestrator's.
 *
 * LANGUAGE AND SECTIONS.  C++ (the range reaches genuinely mangled callees - `move__6MHcharFUs`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `get_now_areano__Fv`, `em_work_die_ck__FP11_ENEMY_WORK` -
 * through their real signatures, rule 9).  Every definition whose map name is plain is `extern "C"`
 * so its emitted name stays the map's stem (playbook 42).  The lib is `ef` (`cflags_main`).  dtk's
 * own split gave the range `extab` 0x80016DB4..0x80016FE4, `extabindex` 0x800362F4..0x8003663C and
 * one `.ctors` word at 0x8056F3A4 from the functions the claim covers.
 *
 * FLAGS.  The whole file builds with `#pragma peephole off`.  Retail keeps the *unfused* form of
 * every fold this band's code triggers: `eft_slot_live_ck` keeps `clrlwi r0,r3,24` + `clrlwi r0,r0,31` +
 * `cmpwi` where the pass emits one record-form `clrlwi.`, and `eft_slot_persist_ck` keeps
 * `clrlwi`/`rlwinm 0,30,30`/`cntlzw`/`srwi` where the pass emits `rlwinm 31,31,31` + `xori`.  Measured
 * per symbol with `recompile.py --measure` (before -> after): `eft_slot_live_ck` 90.0 -> 100.0,
 * `eft_slot_persist_ck` 73.33 -> 100.0, `enemy_data_find` 70.25 -> 98.33, `eft_slot_work_bind` 78.74 -> 82.94,
 * `eft_slot_spawn_targets` 88.29 -> 91.24, `eft_slot_spawn` 87.83 -> 91.20, `eft_slot_clear` 28.5 -> 57.5; only
 * `eft_def_handler` moved the other way (92.29 -> 91.25).  No function is below its peephole-on score.
 *
 * DATA.  The unit's own `.data` is claimed in full (0x805E9168..0x805E91E8, 128 B) and defined here: the
 * 9-entry definition table `lbl_805E9168` (36 B, index 0 NULL, values read from the DOL) and the switch
 * table `jumptable_805E918C` (23 x 4 B) MWCC emits for `enemy_data_grp`.  The table is this unit's by the
 * sole-referencer rule (playbook 58): the only object that names it is this one - 6 relocs, 2 per caller
 * (`eft_def_get`, `eft_def_flags`, `eft_def_handler`), no referrer outside the band - and it is contiguous
 * with the compiler-emitted table, so the two are one object's `.data` chunk.  Both sides now carry
 * `.data` 128 B and `.rela.data` 372 B, `.data` is 100 % fuzzy and `matched_data` is 128 of 1496 B.  The
 * remaining `.data`/`.sdata`/`.sdata2` words (0x8079B320..0x8079B360, `lbl_806BF0A0`, `lbl_806A54E0`)
 * are `extern`-declared by their map names and never defined (playbook 29); the EftDef records the table
 * points at (0x805E7FC0..0x805E9150, and `lbl_806BF2F8` in `.bss`) are still unclaimed and are named
 * only as the table's targets - claiming the 0x805E7F80..0x805E9168 band is a separate, unproven step.
 *
 * STATUS / RESIDUALS (measured with `recompile.py --measure`, official report metric).  33 of the
 * range's 92 functions are reconstructed; 16 are byte-identical and every one but `eft_def_model_block` is
 * at or above the 80 % bar:
 *
 *   byte-identical  eft_state_advance, eft_instance_release, eft_slot_pool_clear, eft_slot_find_free, eft_def_get, eft_def_flags,
 *                   eft_slot_armed_ck, eft_slot_live_ck, eft_slot_persist_ck, eft_work_match_ck, eft_target_match_ck, eft_slot_state_set,
 *                   eft_slot_effect_key, eft_slot_mode_set_imm, eft_slot_mode_set_defer, eft_slot_mode_set_map
 *   eft_work_wide_ck     99.56   eft_target_wide_ck 99.54   eft_slot_state_request 98.28   enemy_data_find 98.33
 *   eft_slot_mode_set     98.17   eft_slot_kind_set 97.67   eft_slot_area_ck 97.05   eft_slot_match_count 96.30
 *   eft_slot_mode_set_work     94.98   eft_slot_work_update 90.24   enemy_data_grp 91.69   eft_def_handler 91.25
 *   eft_slot_spawn_targets     91.24   eft_slot_spawn 91.20   eft_slot_counters_step 90.29   eft_slot_clear 88.0
 *   eft_slot_work_bind     82.94
 *
 *   * `enemy_data_grp` 91.69 - the instruction stream is the target's exactly (128 B) and the only
 *     difference is the switch table's relocation name: retail's `.data` symbol is the map's
 *     `jumptable_805E918C`, ours is MWCC's anonymous `@499`.  No source shape can name a
 *     compiler-emitted table (playbook 53's residual), and the data claim above only makes the
 *     target object carry the same bytes, not the same reloc name.
 *   * `eft_def_model_block` 77.64 - the three-level table walk.  Our 248 B against the target's 244 B: the
 *     `fn_80125FF0(slot->field_0x13, index)` call's second argument is *live* in r4 at the target's
 *     call site (the source passed only one argument through a wider declaration), so our version
 *     materialises it.  The sibling walkers `fn_803457A8`/`fn_80345894` were left unwritten for the
 *     same reason: their call passes no second argument at all (r4 is dead), which the owner header's
 *     two-parameter declaration cannot spell without a materialised argument.
 *   * `eft_def_handler` 91.25, `eft_slot_work_bind` 82.94, `eft_slot_spawn` 91.20, `eft_slot_spawn_targets` 91.24 - each is
 *     4 bytes off with the same instructions in a different order (MWCC's block placement around the
 *     `== NULL` early return and the slot-pool pointer arithmetic); the source shapes tried are the
 *     ones the sibling units record for this class.
 *   * `eft_slot_work_update` 90.24 (632 B target / 644 B ours) - the merge round measured this row at 90.24 in
 *     BOTH the pre-merge and the merged tree (those objects are byte-identical), so the outbox's 92.87
 *     is stale: the target object `recompile.py --measure` used for it was regenerated in MAIN at 19:52,
 *     after that worker's session.  The three real differences: the target's two calls relocate to the
 *     map's mangled `get_move_work_adrs__FUc` / `get_move_work_max__FUc` while ours call the plain names
 *     (this unit's closure carries no declaration of them - an implicit call, which is also why retail
 *     has no argument mask where ours emits `clrlwi`/`extsb`), and one stride constant reads 0xB20 in
 *     the target against 0xB18 in ours.
 *
 * The 58 functions still unwritten, in address order (size): fn_803432B4 (0x490), fn_80344658 (0x74C),
 * fn_80344E9C (0x288), fn_80345124 (0xEC), fn_803455F0 (0xC4), fn_803457A8 (0xEC), fn_80345894
 * (0x100), fn_80345994 (0x98), fn_80345A2C (0x40), fn_80345A6C (0x184), fn_80345BF0 (0xD0),
 * fn_80345CC0 (0xCC), fn_80345D8C (0x158), fn_80345EE4 (0x68), fn_80345F4C (0x124),
 * fn_80346070 (0xCC), fn_8034613C (0xB0), fn_803461EC (0x7C), fn_80346268 (0x4C),
 * fn_803462B4 (0xAC), fn_80346360 (0x124), fn_80346484 (0x50), fn_803464D4 (0xDC),
 * fn_803465B0 (0x428), fn_803469D8 (0x54), fn_80346A2C (0x4), fn_80346A30 (0x5C),
 * fn_80346A8C (0x57C), fn_80347008 (0xDC), fn_803470E4 (0x124), fn_80347208 (0x50),
 * fn_80347258 (0x104), fn_8034735C (0x4D0), fn_8034782C (0x2D8), fn_80347B04 (0x50),
 * fn_80347B54 (0x780), fn_803482D4 (0x1BC), fn_80348490 (0x5C), fn_803484EC (0x504),
 * fn_803489F0 (0x4), fn_803489F4 (0x20), fn_80348A14 (0x34), fn_80348A48 (0x30),
 * fn_80348A78 (0x22C), fn_80348CA4 (0x50), fn_80348CF4 (0x2E0), fn_80348FD4 (0xAC),
 * fn_80349080 (0x104), fn_80349184 (0xA4), fn_80349228 (0x104), fn_8034932C (0x6C),
 * fn_80349398 (0x57C), fn_80349914 (0x14), fn_80349928 (0x16C), fn_80349A94 (0x70),
 * fn_80349B04 (0x140), fn_80349C44 (0x58), fn_80349C9C (0x13C).  The `.data` records
 * 0x805E7FB0/0x805E87F0/0x805E8B48/0x805E9118/0x805E9140 list the entry points of the five
 * three-function programs those bodies implement (`fn_80346484`/`fn_803465B0`/`fn_803469D8`,
 * `fn_80346A2C`/`fn_80346A8C`/`fn_80347008`, `fn_80347208`/`fn_8034735C`/`fn_8034782C`,
 * `fn_80347B04`/`fn_80347B54`/`fn_803482D4`, `fn_80348490`/`fn_803484EC`/`fn_803489F0`), which is
 * where the next pass should start - `fn_80344658` is the shared distance sort they call.
 */

#pragma peephole off

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "fn_8004CAD8.h" /* the vector/geometry helpers the range calls */
#include "Network/network_pat_control.h" /* fn_8042CC20 */
#include "camera/camera.h" /* get_camera_pos / get_camera_direction / fn_802BE088 */
#include "ef/fn_800CDB2C.h" /* my_player_no (`fn_800CF384` before the hud/move_work_update landing named it) */
#include "ef/eft_slot.h" /* fn_803386C4 - its owner (`hud/fn_80334568.cpp`) owns the address, its header is unreachable here */
#include "enemy/ENEMY_WORK.h" /* the move-work records `eft_slot_spawn_targets`/`eft_slot_work_bind` walk */
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck, fn_8012D1A8 */
#include "enemy/fn_801251D0.h" /* fn_80125FF0, fn_8012A9E8 */
#include "enemy/enemy_control.h" /* fn_80143BF8 */

/* The 0x3C-byte slot pool `lbl_806BF0A0` (10 entries) the family's spawn/reset pair walks: `key_0x00`
 * is the definition-table index (`eft_def_get`..`eft_def_handler`), `field_0x14` is stamped 255 by the
 * pool reset, and the +0x18 block holds the slot's per-instance state. size: 0x3C */
struct EftSlot {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 key_0x01;
    /* +0x02 */ u8 armed_0x02;  /* set by `eft_slot_work_bind` when the slot has no move work yet */
    /* +0x03 */ s8 work_0x03;   /* the move-work index, -1 = none */
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ u8 field_0x08;  /* the state byte `eft_slot_state_set` writes */
    /* +0x09 */ u8 field_0x09;  /* the pending state `eft_slot_state_request` records when the slot is idle */
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;   /* the work index the previous tick settled on */
    /* +0x0D */ u8 field_0x0D;
    /* +0x0E */ u8 field_0x0E;
    /* +0x0F */ u8 field_0x0F;  /* the mode `eft_slot_effect_key` selects the +0x14 byte on */
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 field_0x11;
    /* +0x12 */ u8 field_0x12;
    /* +0x13 */ u8 field_0x13;   /* the map number `get_now_mapno` seeds */
    /* +0x14 */ u8 field_0x14;   /* the "slot live" byte the reset stamps 255 */
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
    /* +0x18 */ u32 field_0x18;  /* `eft_slot_live_ck` gates on it being non-zero */
    /* +0x1C */ s16 field_0x1C;
    /* +0x1E */ u16 field_0x1E;
    /* +0x20 */ u16 field_0x20;
    /* +0x22 */ u16 field_0x22;
    /* +0x24 */ f32 field_0x24;
    /* +0x28 */ f32 field_0x28;
    /* +0x2C */ f32 field_0x2C;
    /* +0x30 */ f32 field_0x30;
    /* +0x34 */ s16 field_0x34;
    /* +0x36 */ u8 field_0x36;
    /* +0x37 */ u8 field_0x37;
    /* +0x38 */ u8 field_0x38;
    /* +0x39 */ u8 field_0x39;
    /* +0x3A */ u8 field_0x3A;
    /* +0x3B */ u8 field_0x3B;
};

struct EftEntry;
/* One record of the definition table `lbl_805E9168`, indexed by `EftSlot::key_0x00`.  `kind_0x00` is
 * the byte `eft_def_flags` returns (its two low bits select the spawn path), `field_0x14` points at the
 * three per-kind entry points `eft_def_handler` selects between. size: 0x18 (to the next named member) */
struct EftDef {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x04 */ EftEntry* entries_0x04;   /* the per-motion table `eft_def_model_block` walks */
    /* +0x08 */ u8 unused_0x08[0x10 - 0x08];
    /* +0x10 */ EftEntry* entries_0x10;   /* the second table `fn_803457A8` walks */
    /* +0x14 */ void (**handlers_0x14)(_EFT*);
};

/* One 8-byte entry of a definition's sub-table: `key_0x00` is 255 at the end of the run, and
 * `sub_0x04` is the next level down (the model table, or the 2-byte pair table). size: 0x08 */
struct EftEntry {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x04 */ EftEntry* sub_0x04;
};

/* One 2-byte {key, value} pair of the innermost table `fn_803457A8` walks. size: 0x02 */
struct EftPair {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 value_0x01;
};

/* The per-model work block `_EFT::work_0x38` points at for this family: the model handle array the
 * spawn entry walks, the single extra model at +0x14 with its created scene handle at +0x18, and the
 * scale the step writes. size: 0x2C */
struct EftModelWork {
    /* +0x00 */ u32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[4];
    /* +0x14 */ MHchar* model_0x14;
    /* +0x18 */ void* created_0x18;
    /* +0x1C */ u8 unused_0x1C[0x28 - 0x1C];
    /* +0x28 */ f32 scale_0x28;
};

/* One 0x44-byte record of the 128-entry array `lbl_806A54E0` the live-slot scan walks: the same
 * (team, area, action) key an enemy work record carries, plus the key bytes the slot pool matches
 * against.  size: 0x44 */
struct EftTargetRecord {
    /* +0x00 */ u8 live_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 team_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 state_0x08;  /* 1 and 2 are skipped by the live-slot scan */
    /* +0x09 */ u8 key_0x09;    /* the slot key the record matches, 255 = none */
    /* +0x0A */ u8 index_0x0A;  /* the slot's own index argument */
    /* +0x0B */ u8 unused_0x0B[0x44 - 0x0B];
};

/* The 10-slot pool `eft_slot_pool_clear` clears, `eft_slot_find_free` scans for a free entry and `enemy_data_find`
 * looks a key up in. */
extern "C" EftSlot lbl_806BF0A0[10];
/* The definition table the slot's `key_0x00` indexes (36 B, 9 entries; index 0 is the "no definition"
 * slot).  This unit owns it: its only referrers are this range's own functions (`eft_def_get`,
 * `eft_def_flags`, `eft_def_handler` - 6 relocs, 2 per caller, and no object outside the band names it), it
 * is contiguous with the switch table MWCC emits for `enemy_data_grp` below, and the `.data` claim covers
 * 0x805E9168..0x805E91E8 so the object is the target object.  The values and their order are the DOL's
 * words at 0x805E9168..0x805E918C; the eight non-null entries point at the 0x18-byte `EftDef` records
 * the band's `.data` band holds (playbook 58: a unit's sole-referencer data is claimed, not extern'd). */
extern "C" EftDef lbl_806BF2F8;   /* .bss  0x806BF2F8 */
extern "C" EftDef lbl_805E7FC0;   /* .data 0x805E7FC0 */
extern "C" EftDef lbl_805E7FD8;   /* .data 0x805E7FD8 */
extern "C" EftDef lbl_805E8800;   /* .data 0x805E8800 */
extern "C" EftDef lbl_805E8828;   /* .data 0x805E8828 */
extern "C" EftDef lbl_805E8B58;   /* .data 0x805E8B58 */
extern "C" EftDef lbl_805E9128;   /* .data 0x805E9128 */
extern "C" EftDef lbl_805E9150;   /* .data 0x805E9150 */
extern "C" EftDef* lbl_805E9168[9] = {
    NULL,
    &lbl_806BF2F8,
    &lbl_805E7FC0,
    &lbl_805E8800,
    &lbl_805E8828,
    &lbl_805E8B58,
    &lbl_805E9128,
    &lbl_805E9150,
    &lbl_805E7FD8,
};
/* The 128-entry record array `eft_slot_spawn_targets`/`eft_slot_match_count` walk. */
extern "C" EftTargetRecord lbl_806A54E0[128];

/* The `.sdata2` pool words the range reads; declared, never defined (playbook 29). */
extern "C" f32 lbl_8079B320;
extern "C" f32 lbl_8079B324;
extern "C" f32 lbl_8079B328;
extern "C" f32 lbl_8079B32C;
extern "C" f32 lbl_8079B330;
extern "C" f32 lbl_8079B334;
extern "C" f32 lbl_8079B338;
extern "C" f32 lbl_8079B33C;
extern "C" f32 lbl_8079B350;

/* The effect manager's control block (`ef/effect.cpp`). */
extern "C" _EFT* eft_control_0x04;
extern "C" u8 lbl_806A20F0[];

/* The range's definitions, each named from its own body (see NAMES above; the note after each one is
 * the evidence, so a later pass can re-derive or refine the name without re-reading the whole file). */
extern "C" void fn_803432B4(_EFT* self);  /* the range's still-unwritten head; `ef/eft050.cpp`'s dispatcher calls it */
extern "C" void eft_state_advance(_EFT* self);  /* `_EFT::state_0x05++` - the dispatcher's advance arm */
extern "C" void eft_instance_release(_EFT* self);  /* hands the instance to `fn_800F886C`, the shared release path */
extern "C" void eft_slot_clear(u8 index);  /* zeroes one pool entry's key byte and stamps its +0x14 byte 255 */
extern "C" void eft_slot_pool_clear(void);  /* the 10-iteration loop over `eft_slot_clear` */
extern "C" u8 eft_slot_find_free(void);  /* first entry whose `key_0x00` is zero, 255 when the pool is full */
extern "C" EftDef* eft_def_get(EftSlot* slot);  /* `lbl_805E9168[slot->key_0x00]` */
extern "C" u8 eft_def_flags(EftSlot* slot);  /* the definition's first byte; callers test bits 0, 1, 2 and 3 */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*);  /* `def->handlers_0x14[kind]`, NULL when absent */
extern "C" u8 eft_slot_armed_ck(EftSlot* slot);  /* returns the entry's `armed_0x02` byte */
extern "C" u32 eft_slot_live_ck(EftSlot* slot);  /* a definition flag bit set AND `field_0x18` (the live work) non-zero */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot);  /* the definition's bit 1 clear = the instance survives its work */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index);  /* find-or-allocate, then seed the whole 0x3C block */
extern "C" void eft_slot_spawn_targets(void);  /* one spawn per live enemy work record and per live target record */
extern "C" void eft_slot_work_bind(EftSlot* slot);  /* settles `work_0x03`/`armed_0x02` against `my_player_no()` */
extern "C" void eft_slot_work_update(EftSlot* slot);  /* the per-frame work pick; signals states 5, 7 and 8 */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work);  /* does the slot already track this work record? */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record);  /* the same test on the 128-entry array */
extern "C" void eft_slot_match_count(EftSlot* slot);  /* counts the matches into the entry's +0x0D byte */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work);  /* wider test: the key, or a second slot sharing +0x17 */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record);  /* the same wider test, 128-entry array */
extern "C" u32 eft_slot_area_ck(EftSlot* slot);  /* tracked work -> 0; a matching record of the current area -> 1 */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force);  /* re-points +0x14, the previous value kept in +0x15 */
extern "C" void eft_slot_counters_step(EftSlot* slot);  /* the three per-frame counters, each saturating at its ceiling */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* writes +0x08/+0x09, calls handler 2 */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* live -> switch now, else record it */
extern "C" u8 eft_slot_effect_key(EftSlot* slot);  /* +0x10 in mode 3, else +0x14 - the key the family's spawner stamps */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate);  /* (mode, value) into +0x0F/+0x10 */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 1 */
extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 0 */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate);  /* mode/kind/value onto the pair */
extern "C" void fn_80345A2C(EftSlot* slot);  /* this range's still-unwritten entry `eft_slot_kind_set` calls */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate);  /* the work record rewrites the triple first */
extern "C" u8* eft_def_model_block(EftSlot* slot, u8 index);  /* the per-motion walk down to the 0x20-byte block */

/* ---------------------------------------------------------------------------------------------------
 * the range's own records and data
 * ------------------------------------------------------------------------------------------------- */



/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Advances the family's step counter. */
extern "C" void eft_state_advance(_EFT* self) {
    self->state_0x05++;
}

/* Hands the instance to the shared effect release path. */
extern "C" void eft_instance_release(_EFT* self) {
    fn_800F886C(self);
}

/* Resets one slot of the 0x3C-byte slot pool: cleared, with its +0x14 byte stamped 255. */
extern "C" void eft_slot_clear(u8 index) {
    u32 offset = (u32)index * 0x3C;

    /* the pool as bytes: neither store names a field (rule 6's byte-offset case) */
    ((u8*)lbl_806BF0A0)[offset] = 0;
    ((u8*)lbl_806BF0A0)[offset + 0x14] = 255;
}

/* Clears the whole 10-slot pool. */
extern "C" void eft_slot_pool_clear(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        eft_slot_clear(i);
    }
}

/* Returns the first free slot's index, or 255 when the pool is full. */
extern "C" u8 eft_slot_find_free(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        if (lbl_806BF0A0[i].key_0x00 == 0) {
            return i;
        }
    }
    return 255;
}

/* Looks the 10-entry table up by its group index and the enemy-data id. */
extern "C" void* enemy_data_find(u8 key0, u8 key1) {
    u8 i;

    for (i = 0; i < 10; i++) {
        EftSlot* slot = &lbl_806BF0A0[i];

        if (slot->key_0x00 == key0 && slot->key_0x01 == key1) {
            return slot;
        }
    }
    return NULL;
}

/* The group index the 10-entry table is keyed on, from an enemy's kind byte and its variant. */
extern "C" u8 enemy_data_grp(u8 kind, u8 flag) {
    switch (kind) {
    default:
        return 1;
    case 10:
    case 11:
    case 12:
        return 2;
    case 13:
    case 14:
        return 8;
    case 23:
        return 5;
    case 27:
    case 28:
        return 3;
    case 32:
        return flag == 2 ? 4 : 1;
    case 22:
        return flag == 0 ? 7 : 6;
    }
}

/* Returns the definition record a slot's key byte selects. */
extern "C" EftDef* eft_def_get(EftSlot* slot) {
    return lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's kind byte. */
extern "C" u8 eft_def_flags(EftSlot* slot) {
    return *(u8*)lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's per-kind entry point, or NULL when the definition has none. */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*) {
    EftDef* def = lbl_805E9168[slot->key_0x00];

    if (def->handlers_0x14 == NULL) {
        return NULL;
    }
    switch (kind) {
    case 0:
        return def->handlers_0x14[0];
    case 1:
        return def->handlers_0x14[1];
    case 2:
        return def->handlers_0x14[2];
    }
    return NULL;
}

/* Returns the slot's own kind byte. */
extern "C" u8 eft_slot_armed_ck(EftSlot* slot) {
    return slot->armed_0x02;
}

/* Tests whether the slot's definition asks for the camera-facing variant and the slot is live. */
extern "C" u32 eft_slot_live_ck(EftSlot* slot) {
    if ((eft_def_flags(slot) & 1) != 0 && slot->field_0x18 != 0) {
        return 1;
    }
    return 0;
}

/* Tests whether the slot's definition leaves the instance in the world after its work ends. */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot) {
    return (eft_def_flags(slot) & 2) == 0;
}

/* Spawns (or finds) the slot for one (key, kind) pair and seeds its whole state block. */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index) {
    EftSlot* slot = (EftSlot*)enemy_data_find(key1, key0);
    u8 free_index;

    if (slot != NULL) {
        return slot;
    }
    free_index = eft_slot_find_free();
    if (free_index == 255) {
        return NULL;
    }
    slot = &lbl_806BF0A0[free_index];
    slot->key_0x01 = key0;
    slot->field_0x08 = 0;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x13 = get_now_mapno();
    slot->field_0x14 = index;
    slot->field_0x11 = 255;
    slot->field_0x12 = 255;
    eft_slot_mode_set_defer(slot, 0, 0);
    slot->field_0x1C = 0;
    slot->field_0x1E = (u16)-1;
    slot->field_0x20 = 0;
    slot->field_0x22 = 0;
    slot->field_0x34 = 0;
    slot->field_0x0D = 0;
    slot->field_0x0E = 0;
    slot->field_0x38 = 0;
    slot->field_0x39 = 255;
    slot->field_0x17 = 255;
    slot->field_0x18 = 0;
    slot->field_0x24 = lbl_8079B350;
    slot->field_0x28 = lbl_8079B350;
    slot->field_0x2C = lbl_8079B350;
    slot->field_0x30 = lbl_8079B350;
    slot->field_0x36 = 255;
    slot->field_0x15 = 255;
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
    slot->field_0x16 = 0;
    slot->key_0x00 = key1;
    {
        void (*entry)(_EFT*) = eft_def_handler(slot, 0);

        if (entry != NULL) {
            entry((_EFT*)slot);
        }
    }
    slot->field_0x37 = slot->field_0x39 - 1;
    slot->field_0x06 = 0;
    eft_slot_work_bind(slot);
    slot->field_0x04 = 0;
    slot->field_0x05 = 0;
    return slot;
}

/* Spawns the live slots of every enemy work record, then of the 128-entry record array. */
extern "C" void eft_slot_spawn_targets(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    EftTargetRecord* record = lbl_806A54E0;
    u16 count = get_move_work_max(3);
    s32 i;

    for (i = 0; i < (s32)count; i++, work++) {
        if (em_work_die_ck(work) != 1 && work->field_0x46C != 255) {
            eft_slot_spawn(work->field_0x46C, enemy_data_grp(work->team, work->field_0x00A),
                        work->area_no);
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (record->live_0x00 != 0 && record->state_0x08 != 1 && record->state_0x08 != 2 &&
            record->key_0x09 != 255) {
            eft_slot_spawn(record->key_0x09, enemy_data_grp(record->team_0x02, record->field_0x03),
                        record->index_0x0A);
        }
    }
}

/* Settles the slot's move-work index and the two bytes that follow it. */
extern "C" void eft_slot_work_bind(EftSlot* slot) {
    s8 current = (s8)my_player_no();

    if ((eft_def_flags(slot) & 2) != 0) {
        slot->armed_0x02 = 1;
        slot->work_0x03 = current;
        return;
    }
    slot->work_0x03 = -1;
    {
        u8* work = (u8*)get_move_work_adrs(2);
        u16 count = get_move_work_max(2);
        s8 i;

        for (i = 0; i < (s8)count; i++, work += 0xB20) {
            if (work[0] != 0 && fn_8012D1A8(work[8]) == 0) {
                slot->work_0x03 = i;
                break;
            }
        }
    }
    if (slot->work_0x03 == current) {
        slot->armed_0x02 = 0;
        slot->field_0x0C = slot->work_0x03;
    } else if (slot->work_0x03 == -1) {
        slot->armed_0x02 = 1;
    }
}

/* Picks the move work the slot follows: the free-slot scan, then the forward search for the record
 * whose area the slot was seeded with. */
extern "C" void eft_slot_work_update(EftSlot* slot) {
    s8 current = (s8)my_player_no();
    _ENEMY_WORK* work;
    u16 count;

    if ((eft_def_flags(slot) & 2) != 0) {
        return;
    }
    if (fn_8042CB9C() != 1) {
        return;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    if (eft_slot_armed_ck(slot) == 0) {
        if (fn_8042CC20() == 1) {
            u32 found = 0;

            if (work[(s8)slot->work_0x03].active == 0 ||
                fn_8012D1A8(slot->work_0x03) != 0) {
                u16 i;

                for (i = 0; i < count; i++) {
                    if (work[i].active != 0 && fn_8012D1A8((u8)i) == 0) {
                        slot->work_0x03 = (s8)i;
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 1) {
                if ((s8)slot->work_0x03 == current) {
                    slot->armed_0x02 = 1;
                    fn_803386C4(slot, 5, 0);
                    return;
                }
                fn_803386C4(slot, 7, slot->work_0x03);
                return;
            }
            if (fn_80143BF8() == 1) {
                fn_803386C4(slot, 7, slot->work_0x03);
            }
        } else {
            s16 timer = slot->field_0x06;

            if (timer < 900) {
                slot->field_0x06 = timer + 1;
                return;
            }
            fn_803386C4(slot, 8, 0);
        }
    } else {
        if (fn_80143BF8() == 1) {
            slot->armed_0x02 = 1;
            fn_803386C4(slot, 5, 0);
            return;
        }
        {
            u8 index = slot->work_0x03;

            if (slot->field_0x14 != *((u8*)work + (s8)index * 0xB20 + 0x16)) {
                u8 next = index + 1;
                u16 i;

                for (i = 0; i < (u16)(count - 1); i++, next++) {
                    _ENEMY_WORK* record;

                    if (next >= count) {
                        next = 0;
                    }
                    record = (_ENEMY_WORK*)((u8*)work + next * 0xB20);
                    if (record->active != 0 && fn_8012D1A8(next) == 0 &&
                        slot->field_0x14 == record->field_0x016) {
                        slot->work_0x03 = (s8)next;
                        slot->armed_0x02 = 0;
                        fn_803386C4(slot, 5, 0);
                        return;
                    }
                }
            }
        }
    }
}

/* Tests whether the slot is already tracking the enemy work record the scan handed over. */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0) {
        if (work->action != 0xB && work->action != 0xC &&
            slot->key_0x00 == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 key = work->field_0x46C;

            if (key != 0xFF && key == slot->key_0x01) {
                return 1;
            }
        }
    }
    return 0;
}

/* The same test against one record of the 128-entry array. */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0 &&
        slot->key_0x00 == enemy_data_grp(record->team_0x02, record->field_0x03)) {
        u8 key = record->key_0x09;

        if (key != 0xFF && key == slot->key_0x01) {
            return 1;
        }
    }
    return 0;
}

/* Counts the work records and array entries the slot matches, into the slot's +0x0D byte. */
extern "C" void eft_slot_match_count(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    EftTargetRecord* record = lbl_806A54E0;
    u8 i;

    slot->field_0x0D = 0;
    for (i = 0; i < (u8)count; i++, work++) {
        if (eft_work_match_ck(slot, work) == 1) {
            slot->field_0x0D++;
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (eft_target_match_ck(slot, record) == 1) {
            slot->field_0x0D++;
        }
    }
}

/* The wider match test: the work record's own key, or another live slot that shares this slot's
 * +0x17 byte (or, for the definitions that ask for it, its +0x14 byte). */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0 && work->action != 0xB && work->action != 0xC) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 other_key = work->field_0x46C;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, work->field_0x46C);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The same wider test against one record of the 128-entry array. */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(record->team_0x02, record->field_0x03)) {
            u8 other_key = record->key_0x09;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, record->key_0x09);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Whether a record of the current area still matches the slot: any enemy work record it already
 * tracks means "not spawned", any array entry whose area byte is the current area means "spawned". */
extern "C" u32 eft_slot_area_ck(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    s32 i;

    if (eft_slot_armed_ck(slot) == 0) {
        return 0;
    }
    for (i = 0; i < (s32)count; i++, work++) {
        if (eft_work_wide_ck(slot, work) == 1) {
            return 0;
        }
    }
    {
        EftTargetRecord* record = lbl_806A54E0;
        s32 j;

        for (j = 0; j < 128; j++, record++) {
            if (eft_target_wide_ck(slot, record) == 1 && record->index_0x0A == get_now_areano()) {
                return 1;
            }
        }
    }
    return 0;
}

/* Re-points the slot at another effect kind (the +0x14 byte the family's spawner stamps). */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force) {
    if (force != 0 || eft_slot_armed_ck(slot) != 0) {
        u8 previous = slot->field_0x14;

        if (previous != key) {
            slot->field_0x15 = previous;
            slot->field_0x14 = key;
            slot->field_0x34 = 0;
            fn_80345A2C(slot);
            if (force == 0 && (eft_def_flags(slot) & 4) == 0) {
                fn_803386C4(slot, 3, 0);
            }
        }
    }
}

/* Steps the slot's three per-frame counters, each saturating at its own ceiling. */
extern "C" void eft_slot_counters_step(EftSlot* slot) {
    s16 timer;

    if (slot->field_0x34 < 0x7FFF) {
        slot->field_0x34 += 1;
    }
    if (slot->field_0x1C < 0x7FFF) {
        slot->field_0x1C += 1;
    }
    if ((u16)slot->field_0x1E <= 0x7FFE) {
        slot->field_0x1E += 1;
    }
}

/* Switches the slot to another state byte and hands the previous one to the definition's state-2
 * entry point. */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    u8 previous = slot->field_0x08;
    void (*entry)(EftSlot*, u8, _ENEMY_WORK*);

    slot->field_0x08 = state;
    slot->field_0x09 = state;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x22 = 0;
    slot->field_0x20 = 0;
    slot->field_0x05 = index;
    entry = (void (*)(EftSlot*, u8, _ENEMY_WORK*))eft_def_handler(slot, 2);
    if (entry != NULL) {
        entry(slot, previous, work);
    }
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
}

/* The external state setter: the live slots switch straight away, the others only record it. */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    if (eft_slot_armed_ck(slot) == 1 || eft_slot_live_ck(slot) == 1 || (eft_def_flags(slot) & 4) != 0) {
        if ((u32)(state - 4) > 1) {
            eft_slot_mode_set_defer(slot, 0xFF, 0);
        }
        eft_slot_state_set(slot, state, work, index);
        if (eft_slot_live_ck(slot) == 0 || (eft_def_flags(slot) & 4) == 0) {
            fn_803386C4(slot, 1, 0);
        }
    } else {
        slot->field_0x09 = state;
    }
}

/* The byte the family's spawner stamps as the slot's effect key: the +0x10 byte in mode 3, the +0x14
 * byte otherwise. */
extern "C" u8 eft_slot_effect_key(EftSlot* slot) {
    if (slot->field_0x0F == 3) {
        return slot->field_0x10;
    }
    return slot->field_0x14;
}

/* Records a new (mode, value) pair on the slot; the previous pair is kept in the two bytes above. */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate) {
    if (immediate != 0 || eft_slot_live_ck(slot) == 1) {
        slot->field_0x11 = slot->field_0x0F;
        slot->field_0x12 = slot->field_0x10;
        slot->field_0x0F = mode;
        slot->field_0x10 = value;
        slot->field_0x1C = 0;
        return;
    }
    if (eft_slot_armed_ck(slot) == 1) {
        u8 previous = slot->field_0x0F;

        if (previous != mode || slot->field_0x10 != value) {
            slot->field_0x11 = previous;
            slot->field_0x12 = slot->field_0x10;
            slot->field_0x0F = mode;
            slot->field_0x10 = value;
            slot->field_0x1C = 0;
            if ((eft_def_flags(slot) & 4) == 0) {
                fn_803386C4(slot, 2, 0);
            }
        }
    }
}

/* The two thin wrappers the family's spawner uses to stamp a pair with (1 = immediate). */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 1);
}

extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 0);
}

/* Maps a (mode, kind, value) triple onto the pair the slot records, then stamps it. */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate) {
    u8 pair_mode = 255;
    u8 pair_value = 0;

    switch (mode) {
    case 1:
        if (kind == 2) {
            pair_mode = 0;
            pair_value = value;
        }
        break;
    case 2:
        pair_mode = 1;
        pair_value = value;
        break;
    case 3:
        if (kind == 2) {
            pair_mode = 2;
            pair_value = value;
        }
        break;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, pair_mode, pair_value);
        return;
    }
    eft_slot_mode_set_defer(slot, pair_mode, pair_value);
}

/* Stamps a (kind, a, b) triple on the slot, after the enemy work's record rewrites it. */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate) {
    u8 first = kind;
    u8 second = a;
    u8 third = b;

    if (kind <= 4) {
        fn_8012A9E8(work, &first, &second, &third);
        eft_slot_mode_set_map(slot, first, second, third, immediate);
        return;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, 0, 0xFF);
        return;
    }
    eft_slot_mode_set_defer(slot, 0, 0xFF);
}

/* Walks the definition's per-motion table down to the 0x20-byte block the slot's effect key selects. */
extern "C" u8* eft_def_model_block(EftSlot* slot, u8 index) {
    EftDef* def;
    EftEntry* entry;
    EftEntry* model;

    if (index == 0xFF) {
        return NULL;
    }
    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x04;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF && fn_80125FF0(slot->field_0x13, index) != 1) {
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    model = entry->sub_0x04;
    while (model->key_0x00 != 0xFF && model->key_0x00 != slot->field_0x14) {
        model++;
    }
    if (model->key_0x00 == 0xFF) {
        return NULL;
    }
    return (u8*)model->sub_0x04 + ((u32)index << 5);
}

