/* quest/quest_entry.cpp - the quest entry/init band's record counters and item-slot copies.
 *
 * `.text` 0x803AA4A4..0x803B0F98 (27380 B, 70 functions), `extab` 0x80018A6C..0x80018C54 (60 records),
 * `extabindex` 0x80038E08..0x800390E4 (60 x 12 B) - each run is exactly the gap between the bracketing
 * registrations (`menu/multi_result.cpp` below, `Network/fn_803D3CE8.cpp` above).  Registered from
 * `proposal/803AA4A4_fn_803AA4A4.cpp`; the seam is UNPROVEN (the discovery `--max-bytes` cap) and the run
 * is plainly a *sequence* of objects, so the edges are hints, not proven TU boundaries.
 *
 * NAME - which evidence class decided it.  Class 1 (`__FILE__` string) FAILS: the range references no
 * `.cpp`/`.c` string at all (its `.data` references are item-pair tables, jump tables and pool floats -
 * checked by relocating every `.text` data reference of the 70 auto objects).  Class 2 (runtime-dump name)
 * fails for the range itself (`dumpmap.py lookup 0x803AA4A4` answers `zz_03aa4a4_` for 69 of the 70
 * addresses), but it *does* name the band's own globals: `q_result_msg_adrs` 0x806C5528,
 * `quest_ex_condition_tbl` 0x80794C48, `em_bui_tbl`/`em_bui_rem_l,h`/`em_hokaku_rem_l,h` 0x80794C28-0x80794C38,
 * `q_npc_snd_func` 0x80794C00 - the `q_`/`quest_`/`em_` family the module is named from, together with the
 * range's one real function name, `quest_init(unsigned char)` at 0x803AD47C.  Class 3 therefore decides the
 * MODULE (`quest`) and the file name: the range drives the quest entry/start flow (`quest_init`, the
 * entry state machine that sends `lb_entry_start_send`, `lb_quest_work_init`) and owns its counters.
 * MARKED GUESS: the file name itself - the original source file name is not evidenced, so
 * `quest_entry.cpp` is derived from what the band does.
 *
 * The rename's reference half was swept with it: `fn_803B0CD4` -> `quest_entry_active_ck` in
 * `src/enemy/fn_8012EC74.cpp` (2 sites), the only reference site outside this file.
 *
 * DATA / RULE 12 - the `.bss` claim (this pass).  This unit owns `.bss` 0x806C5558..0x806C5858 in
 * `config/RMHE08/splits.txt`: the band's four 0xC0-byte picked-pair tables `quest_item_pair_tbl_a/b/c/d`
 * (0x806C5558/0x806C5618/0x806C56D8/0x806C5798, 48 four-byte pairs each).  Evidence: the four rows tile
 * the run exactly, `q_result_msg_adrs` (0x806C5528, read by `menu/menu_result.cpp`) ends at its start and
 * `quest_work` (0x806C5858) begins at its end, and every address-taken site of all four is in this file
 * (`callers.py`: 16/8/8/8 sites, in `quest_item_pair_tbl_copy`, `quest_pair_roll_all`/`_first`/
 * `_last`/`fn_803ABE44`/`fn_803AC6B0`, `quest_monster_setup` and `quest_element_clear`).  The two
 * rows still spelled `lbl_806C56D8`/`lbl_806C5798` were renamed `quest_item_pair_tbl_c`/`_d` in the same
 * change and all four are DEFINED in this file (rule 12: an `extern` for data the unit owns would leave
 * `matched_data` at zero), so `datagap.py --unit quest/quest_entry` has no `target-extra` for the run.
 *
 * The picked candidates that are still NOT written, and the one thing each waits for:
 *   * `quest_pair_apply` (0x803AE990, 376 B) needs `lbl_805F76A0` (`.data` 0x805F76A0, 0xA4) - unowned
 *     but NOT this unit's to claim: `menu/arena_result.cpp`'s `fn_803B0F98`/`fn_803B177C` and
 *     `enemy/enemy_control.cpp`'s `fn_8014246C` also take its address (playbook 58: claimable only while
 *     the unit is the sole referencer), so it waits for an owner header.
 *
 * RESIDUAL / KNOWN DEBT.
 *   * `quest_item_pair_copy_cell` / `_block` / `_row` stop at 89-94 %: the residual is the callee-saved
 *     register assignment only (the target colours `rec`=r29, `dst`/`flagged`=r30, `kind`=r31; ours is a
 *     cyclic shift of that), plus `quest_item_slot_find`'s mask/load scheduling order.  No source shape
 *     tried moved them further; both were kept at the best measured form.
 *   * `quest_item_slot_add` 92.50 %: the residual is the `extsh` temp (`extsh r0,r3` in the target
 *     against our in-place `extsh r3,r3`), the target's unsigned `cmplwi r3,5` against our signed
 *     `cmpwi`, and the target's preserved loop counter (`addi r5,r5,1`; ours is dead-code eliminated).
 *     Three shapes moved it and were kept: the `Q_ItemCount* slot = slots;` declaration belongs *inside*
 *     the branch (+3 points), `Q_ItemCount::num` is `s8` and not `u8` - the target's `stb` stores the
 *     `extsb`'d value raw, where a `u8` member costs a `clrlwi` (+3.8 points, and it took
 *     `quest_item_slot_find` 90.15 -> 91.67 with it) - and the `slots[i]` indexed spelling is *worse*
 *     (84.92, measured), so the moving pointer stays.
 *   * `quest_element_build` is 100 %: its `total`/`entry` assignment order and their declaration order
 *     are load-bearing (the target's accumulator is the lower register), recorded here because both
 *     were needed.
 *   * `quest_element_copy` 85.52 %: instruction-identical, and the 6 trailing word copies are load/store
 *     paired in the target (`lwz r5,8; lwz r0,0xc; stw r5,8; stw r0,0xc; ...`) where ours does one word at
 *     a time.  Same source shape, scheduler placement - recorded, not chased.
 *   * `quest_lot_pick_first` 97.32 / `quest_lot_pick_last` 97.20 / `quest_lot_pick` 97.02: the residual is
 *     the callee-saved colouring (the target maps `chance`->r25, `table`->r26, `out`->r27, `count`->r28,
 *     `picked`->r29, `i`->r30, `entry`->r31; ours keeps the same values in a rotated set) plus, in
 *     `quest_lot_pick_first` rows 40/41 and `quest_lot_pick_last` rows 37/38, the weight-sum compare's
 *     operand order and branch polarity - the target's `cmplw r4,r0; bge` against our `cmplw r0,r4; ble`
 *     - and in all three the two narrow-load REPLACEs (target `cmplwi r6,1` / `extsh r0,r0`, ours
 *     `cmpwi` / `extsb`).
 *     Declaring `chance` as the walked pointer (`u8*`, incremented in the `for`) rather than a separate
 *     `const u8* ch` copy was worth +4 points - the target increments the parameter's own register.
 *   * Foreign callees whose bands are still unregistered keep names derived from their own bodies and
 *     renamed in the map (`move_work_state_ck`, `move_work_item_work_get`, `quest_slot_progress_get` -
 *     the rename swept their 12 reference sites in the five units that already called them).  Each name
 *     is a GUESS; a pass with the runtime dump's own names can sharpen them.
 *   * `quest_item_work_notify` (0x803AB190, 96 B) was unwritten; it is written now (the map's 0x58 B
 *     row is 0x60 on the report and the unit's earlier note had it 0x58).  Its declaration moved here
 *     from `include/unsplit/menu.h`, whose band no longer owns the address (`Pl/pl_act.cpp` is its one
 *     consumer and keeps a local `extern`).
 *   * `get_move_work_adrs` is declared in this unit's own header although its owner is
 *     `ef/fn_800CDB2C.cpp`: that owner's header cannot carry it, because `include/hud/cockpit_quest.h`
 *     (`u8*`), `include/lobby/lb_companion_ui.h` (`LbMoveWork*`) and `include/unsplit/ef.h` (`void*`, and
 *     at C scope, i.e. the wrong mangling) each spell the same mangling with a different return type, so a
 *     fourth spelling in the owner header breaks the ten units that include it.  Declared at C++ scope so
 *     the front-end reproduces the map's mangling `get_move_work_adrs__FUc` (a C-scope spelling is
 *     `undefined` at flip time).  The declaration carries a rule-11 marker and the file's rule-2 set is
 *     left at its previous size by moving `move_work_state_ck` to that owner's header, where it already
 *     lived.
 *   * `quest_record_state_get` (0x803ADF48) is renamed but unwritten: its callee `quest_record_get` is
 *     declared only inside `src/menu/arena_result.cpp`, and that unit is another lane's this wave, so the
 *     declaration waits for its owner header.
 *   * Small functions still blocked by a *foreign* name in another lane's registered range (writing them
 *     would add a rule-7 finding to this file): `quest_monster_setup`, `quest_list_load_hunt`/`_arena`,
 *     `quest_grade_set`.  (`quest_element_build` was one until it was written; `quest_pair_apply`'s own
 *     blocker is the unowned-and-shared `lbl_805F76A0`, below.)
 *   * The `Q_UserData`/`Q_ItemWork`/`Q_MoveWork` views are partial: only the offsets this unit reads are
 *     named, everything between them is padding.  All three SIZES are measured, not approximate -
 *     `Q_UserData` 0x6000 (its own pass), `Q_ItemWork` 0x6AB8 and `Q_MoveWork` 0x22E8 (this pass; the
 *     evidence is in each record's comment in `include/quest/quest_entry.h`).
 *   * `get_userdata` is declared at C++ scope now, not inside the header's `extern "C"` block: the
 *     map's row is the mangling `get_userdata__Fv`, so the C-scope spelling relocated an unmangled
 *     `get_userdata` that nothing defines - `undefrefs.py quest/quest_entry` answered NOT READY before
 *     the fix and is clean after it.  No row moved (2724 B and 25 of 70 both sides).
 *   * `menu/menu_item_page.h` and `lobby/lb_companion_ui.h` still declare this unit's symbols themselves
 *     instead of including this header (rule 2's debt; both are headers, so the lint does not fire).  The
 *     `lobby` one also reads `lb_act_best_keep`'s argument as a u16 (`LbActSel::half_0x00`, added with
 *     this change) where it used to read a byte - the target loads `lhz` there, and the fix took that
 *     function 94.03 -> 99.59 %.
 *   * `include/menu/menu_item.h`'s `ItemDataRecord` byte at +0x003 was `unused_0x003` until this pass;
 *     `quest_item_slot_add` reads it as the signed cap a slot's count is clamped to, so it is named
 *     `max_num_0x003` now.
 *   * `quest_pl_skill_slot_set` (0x803AB438, 476 B) is still unwritten although every callee it needs
 *     (`Pl_Skill_ck`, `Pl_cat_skill_ck`, `ran_suu`) is declared: its 16-element byte fix-up loop is
 *     unrolled by 8 and the target keeps a live counter the source shape for is not obvious
 *     (`addi r4,r4,7` per iteration, so the counter is not the element index).
 *   * The `.ctors` word (0x8056F3B0, 4 B) this unit's block claims has no emitter here - pre-existing
 *     (the register records its target as `fn_803AB1F0`'s static initializer), and the fix rides that
 *     body's pass.
 *
 * THE `.bss` PASS wrote two bodies and moved the four tables' definitions to the foot of the file:
 *   * `quest_item_pair_tbl_copy` (0x803AB3BC) 100 %, `quest_element_clear` (0x803ACE90) 100 % - unit
 *     12.31 -> 14.14 %, 14 -> 16 functions at 100 %.
 *   * WHERE the tables are DEFINED is load-bearing: with the definitions above the bodies MWCC folds
 *     the four `.bss` addresses of `quest_element_clear`'s four `memset`s into one section base plus
 *     three `addi` displacements (372 B, 91.20 %), where the target - and our object, with the
 *     definitions at the foot - emits a `lis`/`addi` pair per symbol (376 B, 100 %).  The target's own
 *     `.rela.text` carries per-symbol `R_PPC_ADDR16_HA/LO` for all four, so its compiler saw them as
 *     `extern` at the use sites too: the bytes are this unit's object's (rule 12) but their definitions
 *     sit where the original's codegen says they did.
 *   * `quest_item_pair_tbl_copy`'s own shape is that same lever one level down: its counter's `li` is
 *     emitted first while MWCC colours locals in declaration order, so `a`/`b` are declared (empty)
 *     before `i` and assigned after it.
 *
 * THE STATE-PROBE PASS wrote nine bodies and the fix-up pass took the tenth: 14.14 -> 17.34 %, code
 * 1848 -> 2724 B, 16 -> 25 functions at 100 %.
 *   * `quest_move_state_valid_ck` (0x803AB028, 72 B, 100 %) was WRONG when that pass landed it, and the
 *     residual it recorded misread the target: `lbz r0,0x22d4(r3)` + `rlwinm r3,r0,0,24,24` has
 *     MB=ME=24, which selects bit 24 of the loaded value - the mask is **0x00000080**, not a widening
 *     of the byte - so the body is `(state_0x22D4 & 0x80) != 0`, and `neg`/`or`/`srwi 31` booleanizes
 *     the MASKED value.  The twelve spellings the old residual listed were all `!= 0`-shaped, and a
 *     plain nonzero test folds to a direct `lbz`, so none of them could ever reach the mask - the
 *     shape was never missing, the question was the wrong one.  The bit's meaning is corroborated: the
 *     sibling `quest_move_state_get` masks the SAME byte with `0x7F`, and `fn_803B849C`/`fn_803AEED0`
 *     read bit 7 and then hand the low seven bits to `fn_800EF270` - bit 7 is "a state code follows",
 *     the low seven bits are the code.  Peephole ON is enough - no pragma: with the mask in the source
 *     the pair stays `rlwinm` + `srwi` and the row is byte-identical to the target.
 *   * SHAPES that were load-bearing here (each one was the difference between 74-82 % and 100 %):
 *     `quest_item_work_merge`'s `item_value_0x10E` must be **`s16`** - a `u16` member masks the stored
 *     `s16` into `clrlwi`+`sth` where the target stores it raw; `quest_select_ready_ck` wants the two
 *     option-block tests joined by `&&` inside one `if` (an `if (...) return 0;` chain is if-converted
 *     into a boolean AND); `quest_work_busy_ck` wants three flat `if (...) return 1;` arms with the
 *     LAST arm written `if (record != 0) return ...;  return 1;` (a nested `if` collapses all three
 *     into one shared return block); `quest_id_head_ck`/`_tail_ck` compare `(u16)(quest_id_get() +
 *     0xFFFF)` (a `- 1` spelling loses the `addis`, and the comparison constant is `<= 2` / `> 3`, not
 *     the `< 2` / `< 3` the ranges suggest); `quest_element_find` wants `i` declared BEFORE the element
 *     pointer (register colouring, playbook 63) and both sides of the id test cast to `u32` for the
 *     target's `cmplw`.
 *   * RENAMES this pass made, each with its referrer half swept in the same change (all were map rows
 *     whose owners are other registered units, so the sweep is the rename's other half - rule 7 has no
 *     deferral): `fn_803A8D4C` -> `quest_id_get` (owner `lobby/lb_quest_screen.cpp`; its declaration
 *     now sits in that unit's own header, swept in `src/enemy/fn_80176C58.cpp`), `fn_80125F9C` ->
 *     `enemy_kind_same_ck` (owner `enemy/fn_801251D0.cpp`, 2 sites in its own source + header), and
 *     this unit's own `fn_803AAB80`/`fn_803AB028`/`fn_803AB070`/`fn_803AAEC0`/`fn_803AAF3C`/
 *     `fn_803AAF88`/`fn_803AAFE0`/`fn_803ADF84` (their 22 reference sites across `ai`, `ef`, `menu`,
 *     `stage`, `enemy` were swept with the map edit).  The fix-up pass renamed `quest_move_state_ck`
 *     -> `quest_move_state_valid_ck`: the old name came from the wrong `!= 0` body, and the map row
 *     plus its 11 code sites in `ai`/`menu` and this unit moved together.
 *
 * THE ROLL PASS (this pass) wrote the three pair rolls and the notify, claimed their two data runs and
 * named the band they call: 17.34 -> 22.54 %, code 2724 -> 4148 B, 25 -> 29 of 70 rows.  All four rows
 * are 100 %.  What it cost and what it left:
 *   * the `.data` run 0x805F7AB0..0x805F7AF8 is CLAIMED and emitted (four tables, bytes identical);
 *     `quest_pair_roll_all` also needed the 4-byte `.sdata` run 0x80793600..0x80793604, claimed and
 *     emitted.  With both claims in `splits.txt` the split still links and the DOL is unchanged.
 *   * NAMING: `fn_803AA060` -> `quest_element_pick_ck` - its declaration moved OUT of
 *     `include/lobby/lb_companion_ui.h` into its owner's header `include/lobby/lb_quest_screen.h`, and
 *     the two `lobby/lb_companion_ui.cpp` call sites pass their own view (`(QuestWork*)companion`).
 *     `lb_companion_ui.h` cannot include the owner's header: the owner declares `fmt_803AA41C(s32,
 *     f32)` where that header's own list has the one-argument `fn_803AA41C(s32)`, so the pair trips
 *     `illegal function overloading` (measured) - the re-declaration is the same signature as the
 *     owner's, so both spellings agree.  `fn_803B5E2C` -> `quest_element_state_ck` (owner
 *     `enemy/em_pop.h`) and this unit's `fn_803ABB74`/`fn_803ABCDC`/`fn_803AB914` ->
 *     `quest_pair_roll_first`/`_last`/`_all`.  Names remain GUESSes where the runtime dump has none.
 *   * SHAPES: the rolls are playbook 63 - the declaration order `item; u16 total; Q_LotEntry* entry;`
 *     is the difference between 99.5 and 100 % (the other order colours the first walk's pointer and
 *     accumulator the other way); and `quest_element_state_ck` must return **`u32`**, not `s32`, or its
 *     `== 1` compares `cmpwi` where the target has `cmplwi`.
 *   * FILED, not half-done: `fn_8042CB9C` (`Network/network_pat_control.cpp`) - **43 reference sites in
 *     16 files + 5 headers** - blocks `fn_803AB0FC` and through it the whole entry/loader group, and
 *     the `Network/` tree is another lane's this wave; `fn_80125F54` (21 sites/9 files),
 *     `fn_80137604` (13/11), `fn_80141B88` (11/6), `fn_80272E30` (29/14) block `fn_803AE030` (1012 B),
 *     `fn_803AE424` (1296 B) and `fn_803AAC0C` (624 B).
 *   * still blocked by an unowned, shared symbol: `lbl_805F76A0` (`.data` 0x805F76A0, 0xA4) - read by
 *     `quest_pair_apply`, `fn_803AE030`, `menu/arena_result.cpp` and `enemy/enemy_control.cpp`, so not
 *     this unit's to claim (playbook 58).
 *   * still unwritten from the FIRST `.data` run this pass claimed: `fn_803ABE44` (2156 B, whose only
 *     external is `quest_reward_group_tbl`, now named) and `quest_monster_setup` (384 B) - the latter
 *     needs the item work's +0x6980/+0x6984 words AND the indirect call through the function-pointer
 *     table at 0x806DC410 (no map row at all: it sits inside `auto_08_806D2AF8_bss`, so it needs a new
 *     symbol row and a `.bss` claim before a body can name it).
 *   * the second `.data` run 0x805F7B50..0x805F7B78 is NOT this unit's: the 88 B between the two runs
 *     holds `arena_time_table` (0x805F7AF8, 0x30) and `multi_arena_clr_time` (0x805F7B28, 0x28), read
 *     by three other units (`menu/menu_result.cpp` 1 site, `menu/arena_result.cpp` 2 + 2,
 *     `quest/arenatask.cpp` 1), and playbook 53 would make this unit own those bytes between two
 *     claimed runs of one section.  `fn_803AC6B0` references none of the span.
 *   * `Q_ItemWork` now carries `record_0x3C` and the `Q_ResultRow` prefix view (the +0x310 word the
 *     roll gate reads).  `menu/arena_result.cpp`'s `QuestRecord` in `include/unsplit/menu.h` is the
 *     full view of the same row and that header is not this lane's to extend, so the prefix is
 *     duplicated here - merging the views is a follow-up, as is folding `LbCompanionWork` and
 *     `Q_ItemWork` into one type.
 */

#include "quest/quest_entry.h"
#include "ef/fn_800CDB2C.h"    /* `move_work_state_ck` - owned by ef/fn_800CDB2C.cpp (rule 2) */
#include "unsplit/menu.h"       /* `quest_work_ptr` - the band data no registered unit claims */
#include "unsplit/lobby.h"      /* `lb_param_w` - the option block no registered unit claims */
#include "unsplit/unknown.h"    /* `system_w` - the system block no registered unit claims */
#include "enemy/em_pop.h"       /* `quest_flag_*_ck` - owned by enemy/em_pop.cpp (rule 2) */
#include "enemy/fn_801251D0.h"  /* `enemy_kind_same_ck` - owned by enemy/fn_801251D0.cpp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"  /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "Runtime.PPCEABI.H/memcpy.h"  /* memcpy (owner: the Runtime.PPCEABI.H lib) */

/* The band's quest-work pointer in this unit's own view of the record.  `quest_work_ptr` itself is
 * `.sbss` band data `include/unsplit/menu.h` declares, and that header cannot take this unit's
 * offsets, so the two views are cast rather than merged. */
#define QUEST_WORK ((Q_ItemWork*)quest_work_ptr)

/* ---- the item-slot family (0x803AA9D0) ---- */

/* Adds `count` of `id` to the five-slot list.  An id already in the list is merged into its slot and
 * clamped to the item record's own cap; a new id takes the first free slot, or the round-robin slot
 * the caller's rotation byte points at. */
s32 quest_item_slot_add(Q_ItemCount* slots, u8* rot, u16 id, s16 count) {
    ItemDataRecord* rec = GetItemData(id);
    s32 result;
    s32 i;

    if (rec == NULL) {
        return 1;
    }
    result = quest_item_slot_find(slots, id);
    if (result == 0) {
        result = 5;
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == 0 && count > 0) {
                slot->id = id;
                slot->num = (s8)count;
                result = 0;
                break;
            }
        }
        if (result == 5) {
            slot = &slots[(s8)*rot];
            slot->id = id;
            slot->num = (s8)count;
            *rot = *rot + 1;
            if ((s8)*rot >= 5) {
                *rot = 0;
            }
        }
    } else {
        Q_ItemCount* slot = slots;

        for (i = 0; i < 5; i++, slot++) {
            if (slot->id == id) {
                s8 cap = (s8)rec->max_num_0x003;

                if (count > 0 && (s8)slot->num >= cap) {
                    slot->num = cap;
                    result = 3;
                    break;
                }
                slot->num = slot->num + (s8)count;
                if ((s8)slot->num <= 0) {
                    slot->id = 0;
                    slot->num = 0;
                    result = 4;
                    break;
                }
                if ((s8)slot->num > cap) {
                    slot->num = cap;
                    result = 2;
                    break;
                }
                result = 1;
                break;
            }
        }
    }
    return result;
}

/* Records the item a caller hands over in the local slot's move work, then merges it into the item
 * work's own five-slot list with the negated value (the callers pass -1, so the merge adds one).
 * `owner` is the player work record `src/enemy/fn_801B0010.cpp` passes and the target never reads. */
void quest_item_work_merge(_PLW* owner, u16 id, s16 value) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return;
    }
    work->item_id_0x10C = id;
    work->item_value_0x10E = value;
    work->item_flag_0x110 = 1;
    item = work->item_work;
    if (item == NULL) {
        return;
    }
    quest_item_slot_add(item->slots_0x40, &item->rot_0x54, id, (s16)-value);
    item->flag_0x55 = 1;
}

/* ---- the picked-pair table copy (0x803AB3BC) ---- */

/* Copies both of the band's first two picked-pair tables into a result record's own two adjacent
 * 0xC0 buffers, one four-byte pair at a time. */
void quest_item_pair_tbl_copy(Q_ItemPair* dst_a, Q_ItemPair* dst_b) {
    const Q_ItemPair* a;
    const Q_ItemPair* b;
    s32 i = 0;

    a = quest_item_pair_tbl_a;
    b = quest_item_pair_tbl_b;
    for (; i < 0x30; i++) {
        item_pair_copy(dst_a++, a);
        item_pair_copy(dst_b++, b);
        a++;
        b++;
    }
}

/* ---- the band's small state getters ---- */

/* The current quest id when it is a low-table key (below 0x64), and 0 otherwise. */
u32 quest_id_low_get(void) {
    u32 id = quest_id_get();

    return quest_item_id_low_ck((u16)id) == 1 ? (u8)id : 0;
}

/* Whether the current quest id is one of the three at the head of the low list, and the local slot has
 * a quest selected at all (`(u16)(id + 0xFFFF) <= 2`, i.e. id is 1, 2 or 3). */
u32 quest_id_head_ck(void) {
    if (quest_select_ready_ck() == 0) {
        return 0;
    }
    return (u16)(quest_id_get() + 0xFFFF) <= 2;
}

/* The complementary probe for a slot whose move work is not yet in its quest state: the current quest
 * id is past the head of the low list (`(u16)(id + 0xFFFF) > 3`, i.e. id is 5 or above). */
u32 quest_id_tail_ck(void) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    return (u16)(quest_id_get() + 0xFFFF) > 3;
}

/* Whether the local slot's move work carries a state: bit 7 of the +0x22D4 byte (`rlwinm` MB=ME=24
 * selects 0x00000080).  `fn_803B849C`/`fn_803AEED0` read the same bit before handing the low seven
 * bits to `fn_800EF270`, so it is the "there is a state code here" flag, not a mere nonzero test. */
u32 quest_move_state_valid_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return (work->state_0x22D4 & 0x80) != 0;
}

/* The state code itself: the low seven bits of that same byte, or 0xFF when the slot has no move work
 * or no state. */
u32 quest_move_state_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0xFF;
    }
    state = work->state_0x22D4;
    if (state == 0) {
        return 0xFF;
    }
    return state & 0x7F;
}

/* Whether the local slot has a quest selected: the player's move work must be live and the option
 * block must carry both the selected flag and a nonzero quest id. */
u32 quest_select_ready_ck(void) {
    if (move_work_state_ck() != 1) {
        return 0;
    }
    if (lb_param_w.flag_0x0b == 1 && lb_param_w.field_0x00 != 0) {
        return 1;
    }
    return 0;
}

/* Whether the local slot's move work has its +0x22DC flag byte set. */
u32 quest_move_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->flag_0x22DC != 0;
}

/* Whether an item id belongs to the low pair table (every id below 0x64); id 0 is the empty slot. */
u32 quest_item_id_low_ck(u16 id) {
    if (id == 0) {
        return 0;
    }
    return id < 100;
}

/* The quest phase the whole game switches on while no entry is running - 1 unless the slot is in its
 * entry state, in which case the pause gate decides. */
u32 quest_play_state_ck(void) {
    if (move_work_state_ck() == 0) {
        return quest_work_busy_ck();
    }
    return 1;
}

/* The local slot's quest phase byte (+0xE9 of the move work). */
u8 quest_phase_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->phase_0xE9;
}

/* Whether the local slot's item work has its arena item count set. */
u32 quest_item_work_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return 0;
    }
    item = work->item_work;
    if (item == NULL) {
        return 0;
    }
    return (s8)item->count_0x6A2A != 0;
}

/* The index of the quest-work element `kind` names: the first of the three whose +0x00 gate bit is set
 * and whose +0x04 id either equals `kind` or maps to the same enemy kind, or -1 when none does. */
s32 quest_element_find(u8 kind) {
    s32 i;
    QuestElement* e = quest_work.elements_0x0094;

    if (e == NULL) {
        return -1;
    }
    for (i = 0; i < 3; i++, e++) {
        if (e->flags & 1) {
            if ((u32)e->id == (u32)kind) {
                return i;
            }
            if (enemy_kind_same_ck((u8)e->id, kind) == 1) {
                return i;
            }
        }
    }
    return -1;
}

/* Whether the quest work is busy: either of the system block's two pre-quest flags, or a live work
 * whose record is missing or whose +0x2E8 counter is set. */
u32 quest_work_busy_ck(void) {
    if (system_w.field_0x7d2 == 1) {
        return 1;
    }
    if (system_w.field_0x7d1 == 1) {
        return 1;
    }
    if (quest_work_ptr->record_0x03C != 0) {
        return quest_work_ptr->field_0x2E8 > 0;
    }
    return 1;
}

/* Whether the local slot's sub-state byte is the entry pair (6 or 7), or 4. */
u32 quest_move_sub_state_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0;
    }
    state = work->sub_0xFA;
    if (state == 4) {
        return 1;
    }
    return (u8)(state + 250) <= 1;
}

/* ---- the two entry predicates (0x803B0CD4, 0x803B0CFC) ---- */

/* Whether the quest work's entry-send header has been started. */
u32 quest_entry_active_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    return QUEST_WORK->entry_send_0x6A3A[0] != 0;
}

/* Whether the entry may start: the header must be live and the quest's 0x800000 condition satisfied. */
u32 quest_entry_ready_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    if (QUEST_WORK->entry_send_0x6A3A[0] == 0) {
        return 0;
    }
    if (quest_flag_800000_ck(NULL) == 0) {
        return 0;
    }
    return quest_entry_active_ck();
}

/* ---- the two block copies (0x803AEB7C, 0x803AEB08) ---- */

/* Copies the 8-byte slot pair a result row is built from. */
void quest_pair_copy(Q_SlotPair* dst, const Q_SlotPair* src) {
    *dst = *src;
}

/* Copies the 0x20-byte key block one of those rows carries. */
void quest_element_copy(Q_ElementBlock* dst, const Q_ElementBlock* src) {
    *dst = *src;
}

/* ---- the weighted lot-table picks (0x803AB614, 0x803AB728, 0x803AB80C) ---- */

/* Walks `count` rows of `chance`, and for each row whose chance byte admits it rolls a `total`-wide
 * number into the weighted table and appends the entry the roll lands on.  The first row always
 * rolls 0, so it always takes the table's first non-empty entry. */
s32 quest_lot_pick_first(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        if (i == 0) {
            roll = 0;
        }
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk without the first-roll override: every row rolls its own number. */
s32 quest_lot_pick(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;
    const Q_LotEntry* entry;

    for (i = 0; i < count; i++) {
        u16 roll;
        u16 acc;
        s32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* ---- the arena pair rolls (0x803AB914, 0x803ABB74, 0x803ABCDC) ---- */

/* Hands the local slot's item work to the pick gate for slot `idx`, or 0 when the slot has no move
 * work (or no item work) to hand over. */
u32 quest_item_work_notify(s32 idx) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    if (work->item_work == NULL) {
        return 0;
    }
    return quest_element_pick_ck((QuestWork*)work->item_work, (u8)idx, 1);
}

/* The four-group roll: once the result row's live bit, the element state probe or the first pick gate
 * lets it through, it fills the first two 8-pair groups of `quest_item_pair_tbl_a` from the item
 * work's first two element tables, then - with the four-byte chance table copied over the buffer's
 * head - the two remaining four-pair groups from its third and fourth tables. */
void quest_pair_roll_all(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_a, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if ((item->record_0x3C->flags_0x310 & 0x00800000) != 0 || quest_element_state_ck() == 1 ||
        quest_element_pick_ck((QuestWork*)item, 0, 1) != 0) {
        entry = item->lot_e0a_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
        }
        entry = item->lot_e0b_0xC8;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
        }
    }
    memcpy(chance, quest_pair_chance_tbl_d, sizeof(quest_pair_chance_tbl_d));
    if (quest_element_pick_ck((QuestWork*)item, 1, 1) != 0) {
        entry = item->lot_e1_0xFC;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e1_0xFC, quest_item_pair_tbl_a + 0x20, 4, total);
        }
    }
    if (quest_element_pick_ck((QuestWork*)item, 2, 1) != 0) {
        entry = item->lot_e2_0x15C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e2_0x15C, quest_item_pair_tbl_a + 0x24, 4, total);
        }
    }
}

/* The two-group roll that picks both groups with `quest_lot_pick_first` and sets the pl skill slots;
 * the clause `quest_element_pick_ck` gates walks the item work's first two element tables. */
void quest_pair_roll_first(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 1);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* The same two-group roll with the pl skill slots cleared and the second group picked with
 * `quest_lot_pick_last`. */
void quest_pair_roll_last(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* ---- the arena element builder (0x803AD008) ---- */

/* Clears the element's payload, then fills it from the item work's own weighted lot table: a table of
 * 3 or 5 rows, chosen by the element's own sub-flag, and only when the caller passed the arena kind. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_build(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[6];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(element->payload_0x3F4, 0, sizeof(element->payload_0x3F4));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        }
        if ((u8)kind != 4) {
            return;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, element->payload_0x3F4, rows, total);
    }
}

/* ---- the persisted pair-table builder (0x803ACE90) ---- */

/* The same build for the band's persisted pair tables: zeroes all four, then refills the first from
 * the item work's own weighted lot table - 3, 5 or 8 rows, chosen by the element's sub-flag. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_clear(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[8];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else if (element->flag_0x434 == 1) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        } else {
            chance[0] = 32;
            chance[1] = 32;
            chance[2] = 22;
            chance[3] = 22;
            chance[4] = 22;
            chance[5] = 12;
            chance[6] = 12;
            chance[7] = 12;
            rows = 8;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, quest_item_pair_tbl_a, rows, total);
    }
}

/* The same walk for the band that gates on the row's own chance byte only. */
s32 quest_lot_pick_last(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The recorded count of `kind` in the first record set, plus the live item work's copy of it.  The index
 * spelling 0x24..0x27 sums the set's four group totals instead of reading a count. */
s32 quest_record_a_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_a.total[0] + user->set_a.total[1];
        sum += user->set_a.total[2];
        sum += user->set_a.total[3];
        recorded = sum;
    } else {
        recorded = user->set_a.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_c.total[0] + work->set_c.total[1];
        sum += work->set_c.total[2];
        sum += work->set_c.total[3];
        live = sum;
    } else {
        live = work->set_c.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_a_count_get_wide(s32 kind) {
    return quest_record_a_count_get((u8)kind);
}

/* The recorded count of `kind` in the second record set, plus the live item work's copy of it. */
s32 quest_record_b_count_get(u32 kind) {
    Q_UserData* user = get_userdata();
    s32 recorded;

    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = user->set_b.total[0] + user->set_b.total[1];
        sum += user->set_b.total[2];
        sum += user->set_b.total[3];
        recorded = sum;
    } else {
        recorded = user->set_b.count[(u8)kind];
    }
    if (move_work_state_ck() == 1) {
        return recorded;
    }
    Q_ItemWork* work = move_work_item_work_get();
    if (work == NULL) {
        return recorded;
    }
    s32 live;
    if ((u8)(kind + 0xDC) <= 3) {
        s32 sum = work->set_d.total[0] + work->set_d.total[1];
        sum += work->set_d.total[2];
        sum += work->set_d.total[3];
        live = sum;
    } else {
        live = work->set_d.count[(u8)kind];
    }
    return (u16)(recorded + live);
}

/* The same accessor for a caller that passes the index wider than a byte. */
s32 quest_record_b_count_get_wide(s32 kind) {
    return quest_record_b_count_get((u8)kind);
}

/* Zeroes the first slot, then copies the item record's leading slice into it: 0x10 slots while the
 * caller's `kind` is 2 or the slot's own progress flag is set, 0x18 otherwise. */
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind) {
    dst[0].id = 0;
    dst[0].num = 0;
    if (id != 0xFFFF) {
        Q_ItemPair* rec;
        s32 flagged = 0;

        if (id >= 0x64) {
            rec = q_item_pair_tbl_high[id - 0x64];
        } else {
            rec = q_item_pair_tbl_low[id];
            if (quest_slot_progress_get(0) == 1) {
                flagged = 1;
            }
        }
        if (kind == 2) {
            flagged = 1;
        }
        if (rec != 0) {
            s32 n = 0x10;
            if (flagged == 0) {
                n = 0x18;
            }
            for (s32 i = 0; i < n; i++) {
                item_pair_copy(dst, rec);
                dst++;
                rec++;
            }
        }
    }
}

/* Copies one 8-slot row of an item record into the caller's buffer, when the row's `kind` admits it. */
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind) {
    if (row < 1) {
        return;
    }
    if (id == 0xFFFF) {
        return;
    }
    if (kind == 1) {
        if (quest_slot_progress_get(0) != 1) {
            return;
        }
    } else if (kind != 2) {
        return;
    }
    if (kind != 2 && id >= 0x64) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    rec += (row - 1) * 8 + 0x10;
    dst += (row - 1) * 8 + 0x10;
    for (s32 i = 0; i < 8; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* Copies one 4-slot cell of an item record into the caller's buffer at the cell's own offset. */
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col) {
    if (id == 0xFFFF) {
        return;
    }
    Q_ItemPair* rec;
    if (id >= 0x64) {
        rec = q_item_pair_tbl_high[id - 0x64];
    } else {
        rec = q_item_pair_tbl_low[id];
    }
    if (rec == 0) {
        return;
    }
    dst += col * 4 + 0x20;
    rec += col * 4 + 0x20;
    for (s32 i = 0; i < 4; i++) {
        item_pair_copy(dst, rec);
        dst++;
        rec++;
    }
}

/* The count of the five-slot list's entry for `id`, or 0 when the list has none. */
s16 quest_item_slot_find(Q_ItemCount* slots, u16 id) {
    if (slots[0].id == id) {
        return (s8)slots[0].num;
    }
    if (slots[1].id == id) {
        return (s8)slots[1].num;
    }
    if (slots[2].id == id) {
        return (s8)slots[2].num;
    }
    if (slots[3].id == id) {
        return (s8)slots[3].num;
    }
    if (slots[4].id == id) {
        return (s8)slots[4].num;
    }
    return 0;
}

/* This unit's own `.bss` (`config/RMHE08/splits.txt`, `.bss 0x806C5558..0x806C5858`): the band's four
 * picked-pair tables, 48 four-byte pairs each.  The lot picks write them and the result rows are built
 * from them - `quest_item_pair_tbl_copy` moves `_a`/`_b` into two adjacent 0xC0 buffers of a result
 * record, `quest_element_clear` zeroes all four and refills `_a` from the item work's own lot table. */
Q_ItemPair quest_item_pair_tbl_a[0x30];
Q_ItemPair quest_item_pair_tbl_b[0x30];
Q_ItemPair quest_item_pair_tbl_c[0x30];
Q_ItemPair quest_item_pair_tbl_d[0x30];

/* This unit's own `.data` (`config/RMHE08/splits.txt`, `.data 0x805F7AB0..0x805F7AF8`, the four rows
 * the readers of each table are the only referrers of): the band's reward roll tables.  Each chance
 * table is one byte per pick slot - 32 or 22 - in the shape `quest_element_clear` builds in-line, and
 * the four runs tile the claim exactly (0x10 + 0x18 + 0x10 + 0x10). */
u8 quest_pair_chance_tbl_a[0x10] = {
    32, 32, 32, 22, 22, 22, 22, 22, 32, 22, 22, 22, 22, 22, 22, 22,
};
Q_RewardGroup quest_reward_group_tbl[6] = {
    { 0, 0, 0 }, { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 1 }, { 4, 2, 1 }, { 5, 3, 0 },
};
u8 quest_pair_chance_tbl_b[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};
u8 quest_pair_chance_tbl_c[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};

/* This unit's own `.sdata` (`splits.txt` `.sdata 0x80793600..0x80793604`): the four-byte chance
 * table `quest_pair_roll_all` copies over the head of its 16-byte roll buffer before the last two
 * picks.  The row's sole referrer is that function. */
u8 quest_pair_chance_tbl_d[0x4] = { 32, 22, 22, 22 };
