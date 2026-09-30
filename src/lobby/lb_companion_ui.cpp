/* lobby/lb_companion_ui.cpp - the lobby companion/status UI band, `.text` 0x80338808..0x8033F270
 *
 * NAMES.  The 79 functions this file defines are named from their own bodies (each declaration
 * carries its `Name:` evidence; the table is in `.pi/notes/80338808-named.md`).  Two classes were
 * not derivable and are marked as guesses where they stand: the link-gated senders whose caller is
 * unwritten are named after the protocol sub-command their body builds (`lb_sub0a_send`, ...), and
 * 0x8033A0BC follows the map row main's `hud/move_work_update` landing renamed (`hud_key_lookup`),
 * which is also what restored that row's pairing after the merge.  The runtime dump's own answer
 * for 0x8033C4D8/0x8033C570 (`homebutton::MotorCallback(OSAlarm...)`) is NOT adopted: their bodies
 * are two 0x24-byte refresh wrappers with no argument, so the dump's answer contradicts them and is
 * recorded for a later pass with the caller graph.
 *
 * Naming note: the file's own 79 symbols are named above.  What the escape still covers is
 * precisely the names this file *references* in other units - the 40-odd unsplit lobby/runtime
 * callees (`fn_80334A34`, `broadcastSessionCommand`, `fn_800CF384`, ...) and the two neighbouring handlers
 * `eft_net_recv_state`..`eft_net_recv_release` - which are not this lane's to rename; the dump answers `zz_` for
 * them too and no `__FILE__` string covers the region.  Removing the escape would put every one of
 * those occurrences into rule 7's `fn_` half, and this file is new to `main`, so every finding would
 * be an addition the gate's style-lint row refuses.
 *
 * WHAT IT IS.  UI band: 133 functions / 27240 B that draw and maintain a lobby screen through the
 * 2D library - `get_lsp_data`, `draw_sprite_ary`/`draw_sprite_idx`/`draw_sprite_anim_*`,
 * `draw_font`/`draw_font_idx`, `subTransSet(Prio)`, `sysSE_req` - and read the lobby state
 * (`lb_param_w`, `lb_deli_data`, `Screen_w`, `system_w`) plus the lobby string helper `LbStr`.  The
 * act dispatchers (`lb_act_dispatch`, `fn_80339F10`) are called from the game-root dispatcher at
 * 0x80432154 as `(u8 index, LbActReq* req)`, and every per-act handler below has that shape.
 *
 * MODULE AND NAME (brief section 2).  1. No `__FILE__` string: the range's whole `.data`/`.sdata`
 * reference set is the shared UI vocabulary.  2. `dumpmap.py` answers only `zz_` placeholders.
 * 3. Module `lobby`: the range references the lobby work block `lobby_w` (`.bss` 0x806AAB44),
 * `lb_param_w` (`.bss` 0x806590B4) and the global `lb_deli_data` (`.data` 0x8060DDB8), calls the
 * lobby string helper `LbStr` 11 times, and its callee profile is the one the registered lobby
 * bands document (`LbStr`, `get_lsp_data`, `draw_sprite*`/`draw_font*`, `GetMenuFontColor`,
 * `sysSE_req` - `lobby/fn_801E7530.cpp`, `lobby/fn_801F3294.cpp`, `lobby/fn_802FA9A0.cpp`).
 * 4. The file name is the band's own (`lb_companion_ui`); every symbol it defines carries a
 *    body-derived name (see NAMES above).
 *
 * SEAM: UNPROVEN (measured 2026-09-26, merger lane; the seam round's `undecidable yet` stands).
 * The range is an `attribute.py` `--max-bytes` cut of the unclaimed 0x8030121C..0x8035E034
 * stretch, and no evidence class settles either edge:
 *  - no `__FILE__` string exists for the band at all, so the decisive class-1 test cannot fire:
 *    the region 0x8030121C..0x80349DD8 holds exactly two (`menu_infomation.cpp`, `menu_note.cpp`,
 *    both outside), and a relocation sweep of every target object finds no `.c`/`.cpp` string
 *    cited by any function of 0x80334568..0x803432B4;
 *  - `.sdata2` pins: 4805 single-referrer labels, 4 inversions DOL-wide, 0 near either edge; the
 *    band's pool run is an ordered disjoint partition - 0x8079B2B0 (below) | 0x8079B2B8/BC (here)
 *    | 0x8079B2C0+ (above);
 *  - nothing private crosses an edge: no `.sdata2`/`.sdata`/`.data` label is cited from both
 *    sides (only the globals `lobby_world_block`, `lobby_w` and the `_savegpr_*` helpers are), so no
 *    must-link exists in either direction;
 *  - the `scope:local` anchors (654 `.data` labels, 0/653 owner-order inversions DOL-wide) put
 *    this band's three switch tables in order - 0x805E27D4 <- lb_act_dispatch, 0x805E27F8 <-
 *    fn_80339F10, 0x805E71FC <- fn_8033C9B0 - and the `.data`/`.sdata` runs continue across both
 *    edges with ascending owners (fn_8033F13C -> fn_8033F270).  That is the `candidate, never
 *    proof` class: a contiguous data run with ascending owners looks identical whether it is one
 *    object or two adjacent ones, which is why the DOL alone cannot settle this seam.
 * What does agree with the extent: the unwind pair (extab starts where `eft_net_send`'s record ends,
 * 0x8001698C + 8 = 0x80016994, and ends where the next function's begins, 0x80016CA4; extabindex
 * 0x80035CB8 + 0xC = 0x80035CC4 .. 0x8003615C) and the two bracketing registrations
 * (`hud/net_char_sync.cpp` ends at 0x80338808, `ef/eft050.cpp` starts at 0x8033F270).  The
 * "game-root dispatcher calls both sides" argument is NOT evidence - a dispatcher calls handlers
 * from several TUs.  Registered whole; a re-cut needs evidence that does not exist yet.
 *
 * RENAME (done 2026-09-26, the naming pass this branch was parked for; merger.md M5).  The map's 78
 * `fn_` rows in this range were renamed with the file's own definitions and every source/header that
 * spells them (`include/unsplit/menu.h`, `include/unsplit/unknown.h`, `include/stage/fn_802B2AA0.h`,
 * `src/Pl/pl_act.cpp`, `src/ef/eft050.cpp`, `src/enemy/fn_80137604.cpp`,
 * `src/enemy/fn_801A9540.cpp`, `src/menu/fn_802E4978.cpp`, `src/stage/fn_802B2AA0.cpp`), so objdiff
 * still pairs every row: 0 rows moved by the rename, and the one row the merge itself had broken
 * (0x8033A0BC, 100 -> 0 when main renamed its map row) is restored to 100.  The 79th name,
 * `hud_key_lookup`, was already main's map row, so only this file's definition moved.
 * The dump's `homebutton::MotorCallback` answer for 0x8033C4D8/0x8033C570 is recorded under NAMES
 * and not adopted; it also questions this file's `lobby` home for that sub-band.
 *
 * LANGUAGE AND SECTIONS.  C++ - the range's callees include genuine manglings
 * (`get_lsp_data__FUsP10_mh_ivec2_`, `draw_sprite_ary__FPCUsPC10_mh_ivec2_`, `LbStr__FUcUs`,
 * `sysSE_req__Fl`) that rule 9 forbids spelling as identifiers, so they are declared at C++ scope
 * with the real signatures in this unit's header; every plain `fn_` definition is `extern "C"` so
 * it keeps the map's name (playbook 42).  Lib `lobby` (`cflags_lobby`: `-O3`, `-inline noauto` -
 * the target packs its functions on 4 B, so it is not `-O4,p`): the target object carries extab
 * 0x80016994..0x80016CA4 (98 records) and extabindex 0x80035CC4..0x8003615C (98 x 12 B).
 *
 * FLAGS.  `cflags_lobby` (which now sets `-Cpp_exceptions on`, flags-audit 2026-09-28) plus one
 * per-file pragma, measured over this whole file:
 *   * `-Cpp_exceptions on` - the target object carries the 98 unwind records the old default
 *     (`-Cpp_exceptions off`) did not emit.
 *   * `#pragma peephole off` - retail keeps the unfused narrow forms (`clrlwi` + `slwi`, `lobby_world_block
 *     + (i >> 3)` kept in a register) that the pass folds into one `rlwinm`/`addi`.  A/B over the whole
 *     file: 24 -> 47 functions byte-identical and 60 -> 73 of the 133 at or above the 80 % bar.
 *
 * STATUS / RESIDUALS.  78 of the 133 functions are written below and 76 of them are at or above the
 * 80 % bar (47 byte-identical); the official report reads fuzzy 25.447577 %, matched_code 3292 of
 * 27240 B.  The 55 unwritten functions keep the map's `fn_XXXXXXXX` names and are absent from this
 * file, so objdiff reports them as 0 %; the largest are `fn_8033A160` (1204 B), `fn_8033C9B0`
 * (1036 B), `fn_8033D590` (992 B), `fn_8033AED0` (968 B), `fn_8033BCB0` (956 B), `fn_8033E960`
 * (816 B), `fn_8033B380` (764 B), `fn_8033D1F8` (740 B), `fn_8033C77C` (564 B) and `fn_8033EDAC`
 * (504 B) - the tutorial/quest state machine, the page/dialog update chain and the drawing helpers.
 * Two written functions are the honest residual (measured with `recompile.py --measure`):
 *   * `lb_page_entry_bit_set` 29.58 % - retail computes `lobby_world_block + (index >> 3)` into a register and keeps
 *     the bit-field offset as the load displacement (`add r5,r3,r0` + `lbz r4,14688(r5)`); ours folds
 *     the offset into the base (`addi r5,r3,14688`) and indexes by the raw shift, so the two
 *     addressing idioms differ on every instruction of this 48-byte helper.
 *   * `lb_entry_flags_clear` 79.95 % - the 16 byte stores: retail materialises both `.sbss` bases before the
 *     first store, ours folds the first one into its `stb` sda21 operand.  `u8* seen = lbl_80794B90;`
 *     locals (the variant below) do not change it; the remaining difference is one instruction.
 *   * `fn_8033B67C` and the two other functions that call `sprintf` with a pool format string are not
 *     written yet (they need the `.sdata` format operand, which the pool claims do not cover).
 *
 * DATA.  `.data` 0x805E27D4..0x805E27F8 is claimed: it is `lb_act_dispatch`'s own 9-entry switch table
 * (`jumptable_805E27D4`, `scope:local`, referenced by no other unit - playbook 58's private entry),
 * and claiming it makes the target object's `.data` pair with ours.  `datagap.py --unit
 * lobby/lb_act_dispatch` reports no `ours-extra` section at all and `--flip-blockers` does not list this
 * unit.  The extab/extabindex claims are complete for the whole range while only 78 functions are
 * written, so our object's unwind sections are short (368/552 B of the target's 784/1176 B).
 */

#include "types.h"
#include "lobby/lb_companion_ui.h"
#include "quest/quest_result_enter.h"  /* quest_reward_faint_penalty, quest_result_enter, quest_start_enter (rule 2) */
#include "quest/quest_item_slot.h"     /* quest_item_pair_copy_row (rule 2) */
#include "menu/quest_str_tbl_35_get.h"  /* quest_str_tbl_35_get (rule 2) */
#include "ef/eft052.h"               /* hud_msg_push (rule 2) */
#include "Pl/pl_item_add.h"          /* pl_item_add (rule 2) */
#include "lobby/lb_quest_screen.h"   /* quest_time_limit_set (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */

#pragma peephole off

extern "C" {

/* This unit's own forward declarations (the definitions follow in address order). */
void lb_seen_bit_set(u8 bit, u8 index);
void lb_handled_set(u8 unused, u8 index);
void lb_sub0d_send(u8 index, LbActReq* req, s8 flag);
void lb_sub0e_send(u8 index, u16 id, u8 flag, u16 value);
void lb_name_tail_copy(LbNameTail* dst, LbNameTail* src);
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd);

/* The per-act entry point: gate the request against `Pl_net_can_send`/`fn_800CF384`, look the entry up
 * from the request's two ids, and dispatch on its act byte to the matching handler.
 * Name: the band's per-act entry: gate (`Pl_net_can_send`), entry lookup (`fn_803438E4`/`fn_80343B74`) and
 *   a switch on `act_0x03` 1..8 into the neighbouring band's handlers */
void lb_act_dispatch(u8 index, LbActReq* req) {
    void* entry;

    if (Pl_net_can_send() != 0 && (s32)req->index_0x01 != fn_800CF384()) {
        entry = fn_803438E4(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
        if (entry == NULL) {
            entry = fn_80343B74(req->sel_0x04.bytes_0x00.b_0x01, req->sel_0x04.bytes_0x00.a_0x00,
                                req->sel_0x04.bytes_0x00.c_0x02);
        }
        if (fn_80343B44(entry) != 0) {
            switch (req->act_0x03) {
            case 1:
                eft_net_recv_state((EftSlot*)entry, (NetEftStateMsg*)req);
                return;
            case 2:
                eft_net_recv_step((EftSlot*)entry, (NetEftStepMsg*)req);
                return;
            case 3:
                eft_net_recv_live((EftSlot*)entry, (NetEftLiveMsg*)req);
                return;
            case 4:
                eft_net_recv_pos((EftSlot*)entry, (NetEftPosMsg*)req);
                return;
            case 5:
                eft_net_recv_work((EftSlot*)entry, (NetEftWorkMsg*)req, fn_800CF384());
                return;
            case 6:
                eft_net_recv_mark((EftSlot*)entry, (NetEftMarkMsg*)req);
                return;
            case 7:
                eft_net_recv_bind((EftSlot*)entry, (NetEftBindMsg*)req);
                return;
            case 8:
                eft_net_recv_release((EftSlot*)entry, (NetEftReleaseMsg*)req);
                break;
            }
        }
    }
}

/* Sends the "entry +0x08 selected" command to the lobby server when the link is up.
 * Name: the header-only sub-0x05 packet ("this entry was selected"), id = the request's mask byte; the
 *   only sender with no payload */
void lb_entry_selected_send(LbActReq* req) {
    LbCmdSub05 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(req->mask_0x08.byte_0x00, 0xD, 5);
        broadcastSessionCommand(&cmd, 4);
    }
}

/* The same command with the index taken from the caller and the entry id offset by six.
 * Name: the same header-only packet with sub-command 6+index and the entry id in its pad field;
 *   `lb_act_announce` calls it with index 1 */
void lb_entry_notify_send(s32 index, u8 entry) {
    LbCmdSub05 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(entry, 0xD, (u8)(index + 6));
        broadcastSessionCommand(&cmd, 4);
    }
}

/* Acts 6 and 7: 6 announces the companion work's entry when its step byte is not 4, 7 hands the
 * index to the entry handler.
 * Name: act 6 announces the companion entry when its step byte is not 4, act 7 forwards the index to
 *   `quest_reward_faint_penalty` */
void lb_act_announce(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    switch (req->act_0x03) {
    case 6:
        if (isReadyCountOne() != 0) {
            work = get_move_work_adrs(0);
            if (work != NULL) {
                companion = work->companion_0xDC;
                if (companion != NULL && (s8)companion->step_0x2C != 4) {
                    lb_entry_notify_send(1, req->index_0x01);
                    return;
                }
            }
        }
        return;
    case 7:
        quest_reward_faint_penalty(req->index_0x01);
        break;
    }
}

/* Clears the two per-entry byte arrays (`lbl_80794B90`/`lbl_80794B98`, 8 entries each).
 * Name: zeroes the two 8-byte per-entry arrays `lbl_80794B90`/`lbl_80794B98`; no caller in this file */
void lb_entry_flags_clear(void) {
    u8* seen = lbl_80794B90;
    u8* handled = lbl_80794B98;

    seen[0] = 0;
    handled[0] = 0;
    seen[1] = 0;
    handled[1] = 0;
    seen[2] = 0;
    handled[2] = 0;
    seen[3] = 0;
    handled[3] = 0;
    seen[4] = 0;
    handled[4] = 0;
    seen[5] = 0;
    handled[5] = 0;
    seen[6] = 0;
    handled[6] = 0;
    seen[7] = 0;
    handled[7] = 0;
}

/* Sends the index-selected command with a u8 payload.
 * Name: (guess) sub-0x0A with one word payload - the caller is unwritten, so the protocol slot is the
 *   evidence */
void lb_sub0a_send(u8 index, u8 value) {
    LbCmdSub0A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 10);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 8);
    }
}

/* Sets this entry's seen bit in `lbl_80794B90` (act 10).
 * Name: act 10: one `lb_seen_bit_set` call with the request's word */
void lb_act_seen_set(u8 bit, LbActReq* req) {
    lb_seen_bit_set(bit, (u8)req->sel_0x04.word_0x00);
}

/* The same command as `lb_sub0a_send` with sub-command 11.
 * Name: (guess) the sub-0x0B twin of `lb_sub0a_send`; caller unwritten */
void lb_sub0b_send(u8 index, u8 value) {
    LbCmdSub0A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 11);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 8);
    }
}

/* Marks this entry handled in `lbl_80794B98` (act 11).
 * Name: act 11: one `lb_handled_set` call with the request's word */
void lb_act_handled_set(u8 bit, LbActReq* req) {
    lb_handled_set(bit, (u8)req->sel_0x04.word_0x00);
}

/* Act 9: publishes the entry's id into the system block's ring cell 3 unless the pad owns it.
 * Name: act 9: writes the entry id into `system_w.ring_0x18[3]` unless the pad owns it */
void lb_act_entry_publish(u8 unused, LbActReq* req) {
    if (isReadyCountOne() != 1) {
        system_w.ring_0x18[3] = (u16)req->sel_0x04.word_0x00;
    }
}

/* Sets bit `bit` of the per-entry byte `lbl_80794B90[index]`.
 * Name: sets one bit of the per-entry byte `lbl_80794B90[index]` */
void lb_seen_bit_set(u8 bit, u8 index) {
    lbl_80794B90[(u8)index] = lbl_80794B90[(u8)index] | (u8)(1 << (u8)bit);
}

/* Marks the per-entry byte `lbl_80794B98[index]` handled.
 * Name: writes 1 into `lbl_80794B98[index]` (the first parameter is unused) */
void lb_handled_set(u8 unused, u8 index) {
    lbl_80794B98[(u8)index] = 1;
}

/* Whether every set bit of `lbl_80794B90[index]`'s low nibble belongs to a pad that is present.
 * Name: counts the set low-nibble bits whose pad is present and compares the count with `countOccupiedServerSlots()` */
s32 lb_seen_pad_ck(u8 index) {
    s32 count;
    u8 bits;
    u8 i;

    count = 0;
    bits = lbl_80794B90[index] & 0xF;
    i = 0;
    do {
        if ((bits & 1) != 0 && isServerSlotOccupied(i) != 0) {
            count += 1;
        }
        bits = (u8)((s32)bits >> 1);
        i += 1;
    } while ((s32)i < 4);
    return count == countOccupiedServerSlots();
}

/* Whether the per-entry byte `lbl_80794B98[index]` is set.
 * Name: the per-entry byte `lbl_80794B98[index] != 0` */
s32 lb_handled_ck(u8 index) {
    return lbl_80794B98[index] != 0;
}

/* Act 8: hands the award screen over to act 5's or act 3's entry state.
 * Name: act 8: sel 5/3 set `work->state_0xFA` and start the `quest_time_limit_set` delay scaled by `Screen_w` */
void lb_act_award_handover(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    if (isServerSelectState() != 0) {
        work = get_move_work_adrs(0);
        if (work != NULL) {
            companion = work->companion_0xDC;
            if (companion != NULL) {
                if (req->sel_0x04.word_0x00 == 5) {
                    work->state_0xFA = 5;
                    companion->value_0x20 = companion->value_0x24;
                    companion->step_0x2C = companion->step_0x2C + 1;
                    quest_time_limit_set((s32)(lbl_8079B2B8 * Screen_w.scale_0x14));
                }
                if (req->sel_0x04.word_0x00 == 3) {
                    fn_802A0188();
                    work->state_0xFA = 3;
                    companion->value_0x20 = companion->value_0x24;
                    companion->step_0x2C = companion->step_0x2C + 1;
                    quest_time_limit_set((s32)(lbl_8079B2BC * Screen_w.scale_0x14));
                    snd_quest_start_bgm_set();
                }
            }
        }
    }
}

/* Sends the "hand this entry over" command (act 12) with the companion work's two value words.
 * Name: the sub-0x0C/0x01 handover packet from the companion's two mask words; `Pl/pl_act.cpp` calls it
 *   too */
void lb_entry_handover_send(u8 kind, u8 index, u8 value) {
    LbCmdSub010C cmd;
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL && isServerSelectState() != 0) {
            if (kind == 1) {
                ((NetMsgHeader*)&cmd)->fill(index, 0xD, 1);
                cmd.pad_index_0x04 = index;
                cmd.flag_0x05 = 0;
                cmd.value_0x07 = value;
            } else {
                ((NetMsgHeader*)&cmd)->fill(index, 0xD, 12);
                cmd.pad_index_0x04 = fn_800CF384();
                cmd.entry_0x06 = index;
                cmd.flag_0x05 = 1;
                cmd.value_0x07 = value;
                cmd.mask_0x08 = companion->bits_0x684[0];
                cmd.mask_0x0C = companion->bits_0x684[1];
            }
            broadcastSessionCommand(&cmd, 0x10);
        }
    }
}

/* Acts 1 and 12: records this entry's bit in the companion work's mask and announces it.
 * Name: acts 1 and 12: records the entry bit in `companion->bits_0x684` and announces it */
void lb_act_handover(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbMoveEntry* moves;
    LbCompanionWork* companion;
    LbCompanionPair* pair;
    u8 index;
    u32 bit;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (req->sel_0x04.bytes_0x00.b_0x01 == 0) {
                if (isReadyCountOne() == 1) {
                    index = req->sel_0x04.bytes_0x00.d_0x03;
                    bit = 1 << (index & 0x1F);
                    if ((companion->bits_0x684[index >> 5] & bit) != 0) {
                        index = 0xFF;
                    } else {
                        companion->bits_0x684[index >> 5] =
                            companion->bits_0x684[index >> 5] | bit;
                        index = req->sel_0x04.bytes_0x00.d_0x03;
                    }
                    lb_entry_handover_send(12, req->sel_0x04.bytes_0x00.a_0x00, index);
                }
            } else {
                index = req->sel_0x04.bytes_0x00.c_0x02;
                if ((s32)index == fn_800CF384()) {
                    moves = (LbMoveEntry*)get_move_work_adrs(2);
                    if (moves == NULL) {
                        return;
                    }
                    moves[index].flag_0x659 = 0;
                    if (req->sel_0x04.bytes_0x00.d_0x03 != 0xFF) {
                        pair = &companion->pairs_0x5E2[req->sel_0x04.bytes_0x00.d_0x03];
                        pl_item_add((_PLW*)&moves[index], pair->id_0x00, pair->value_0x02);
                        fn_802E5D68(pair->id_0x00, (s8)req->sel_0x04.bytes_0x00.d_0x03);
                    }
                }
                companion->bits_0x684[0] = companion->bits_0x684[0] | req->mask_0x08.word_0x00;
                companion->bits_0x684[1] = companion->bits_0x684[1] | req->mask_0x0C.word_0x00;
            }
        }
    }
}

/* Sends the sub-0x0D command with the request's three bytes and its word.
 * Name: sub-0x0D with the request's three bytes and its halfword; called from the act-13 row update */
void lb_sub0d_send(u8 index, LbActReq* req, s8 flag) {
    LbCmdSub0D cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xD);
        cmd.value_0x04 = req->sel_0x04.bytes_0x00.b_0x01;
        cmd.index_0x05 = req->sel_0x04.bytes_0x00.c_0x02;
        cmd.flag_0x06 = flag;
        cmd.word_0x08 = req->mask_0x08.half_0x00;
        broadcastSessionCommand(&cmd, 0xA);
    }
}

/* Act 13: hands the request's row and word to the row updater, or marks the row started.
 * Name: act 13: `serial_find` row lookup, then either the state-4 start (with `lb_sub0d_send`) or
 *   `serial_state_set_word` */
void lb_act_row_update(u8 unused, LbActReq* req) {
    ShellSerialEntry* row;

    row = serial_find(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
    if (row != NULL) {
        if (isReadyCountOne() == 1 && req->sel_0x04.bytes_0x00.c_0x02 == 3 && row->state_0x07 <= 3) {
            row->word_0x08 = req->mask_0x08.half_0x00;
            row->state_0x07 = 4;
            lb_sub0d_send(row->player_0x05, req, 4);
            return;
        }
        serial_state_set_word(row, req->sel_0x04.bytes_0x00.c_0x02, req->mask_0x08.half_0x00);
    }
}

/* Sends the sub-0x0E command, or hands the two ids to the local row updater when the link is down.
 * Name: sub-0x0E, or the local row writer `fn_803B6998` when the link is down; `enemy/fn_80137604.cpp`
 *   calls it */
void lb_sub0e_send(u8 index, u16 id, u8 flag, u16 value) {
    LbCmdSub0E cmd;
    u8 set = flag;

    if (isServerSelectState() == 0) {
        fn_803B6998(id, value);
        return;
    }
    if ((s32)set == 0) {
        set = 1;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xE);
    cmd.value_0x04 = set;
    cmd.flag_0x05 = 0;
    cmd.id_0x06 = id;
    cmd.word_0x08 = value;
    broadcastSessionCommand(&cmd, 0xA);
}

/* Act 14: 1 sends the row over when this pad owns it, anything else updates it locally.
 * Name: act 14: kind 1 sends the row with `lb_sub0e_send` when this pad owns it, other kinds write it
 *   locally */
void lb_act_row_apply(u8 unused, LbActReq* req) {
    u8 kind;

    kind = req->sel_0x04.bytes_0x00.a_0x00;
    if ((s32)kind != 0) {
        if (kind == 1) {
            if (isReadyCountOne() == 1) {
                lb_sub0e_send(0, req->sel_0x04.bytes_0x00.c_0x02, 2, req->mask_0x08.half_0x00);
            }
        } else {
            fn_803B6998(req->sel_0x04.bytes_0x00.c_0x02, req->mask_0x08.half_0x00);
        }
    }
}

/* Act 15: sends the sub-0x0F text command with the caller's block copied into its payload.
 * Name: act 15: the 0x2C-byte sub-0x0F text packet built from the caller's block; link down ->
 *   `fn_80142C58` */
void lb_sub0f_send(u8 index, u8 value, void* text, u16 id, u8 flag, f32 scale) {
    LbCmdSub0F cmd;

    lb_sub0f_init(&cmd);
    if (isServerSelectState() == 0) {
        fn_80142C58(value, text, id, flag, scale);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0xF);
    cmd.value_0x04 = value;
    lb_name_tail_copy(&cmd.text_0x0C, (LbNameTail*)text);
    cmd.id_0x06 = id;
    cmd.flag_0x05 = flag;
    cmd.scale_0x08 = scale;
    broadcastSessionCommand(&cmd, 0x2C);
}

/* Copies the 0x20-byte text block: its first 8 bytes a byte at a time, the rest word-wise (MWCC's
 * idiom for the two members' alignments).
 * Name: the 0x20-byte `LbNameTail` copy (8 bytes + 6 words), MWCC's split-copy idiom for the two member
 *   alignments */
void lb_name_tail_copy(LbNameTail* dst, LbNameTail* src) {
    u8 i;

    for (i = 0; i < 8; i++) {
        dst->bytes_0x00[i] = src->bytes_0x00[i];
    }
    for (i = 0; i < 6; i++) {
        dst->words_0x08[i] = src->words_0x08[i];
    }
}

/* Clears the sub-0x0F packet's text payload and returns the packet.
 * Name: clears the sub-0x0F packet's text payload (`fn_80125F54`) and returns the packet */
LbCmdSub0F* lb_sub0f_init(LbCmdSub0F* cmd) {
    fn_80125F54(cmd->text_0x0C.bytes_0x00);
    return cmd;
}

/* Act 15 with the link down: hands the packet's fields straight to the text writer.
 * Name: the link-down half of act 15: the packet's own fields go to `fn_80142C58` */
void lb_text_apply(u8 unused, LbCmdSub0F* cmd) {
    void* text = cmd->text_0x0C.bytes_0x00;

    fn_80142C58(cmd->value_0x04, text, cmd->id_0x06, cmd->flag_0x05, cmd->scale_0x08);
}

/* Sends the sub-0x10 command, or hands the byte to the local handler when the link is down.
 * Name: sub-0x10 with one signed byte; link down -> `fn_80146C00` */
void lb_sub10_send(u8 index, s8 value) {
    LbCmdSub10 cmd;

    if (isServerSelectState() == 0) {
        fn_80146C00(value, index);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x10);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 16: hands the request's byte and index to the local handler.
 * Name: act 16: the request's byte and the pad index go to `fn_80146C00` */
void lb_act_byte_apply(u8 unused, LbActReq* req) {
    fn_80146C00((s8)req->sel_0x04.bytes_0x00.a_0x00, req->index_0x01);
}

/* Sends the sub-0x11 command with two signed bytes.
 * Name: (guess) sub-0x11 with two signed bytes; caller unwritten */
void lb_sub11_send(s8 a, s8 b) {
    LbCmdSub11 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x11);
        cmd.value_0x04 = a;
        cmd.value_0x05 = b;
        broadcastSessionCommand(&cmd, 6);
    }
}

/* Act 17: hands the request's two bytes to the local handler.
 * Name: act 17: the request's two bytes go to `fn_802B09B8` */
void lb_act_pair_apply(u8 unused, LbActReq* req) {
    fn_802B09B8(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
}

/* Sends the sub-0x12 command, or hands the byte to the local handler when the link is down.
 * Name: sub-0x12 with one byte; link down -> `fn_802B45F4`; `stage/fn_802B2AA0.cpp` calls it */
void lb_sub12_send(u8 value) {
    LbCmdSub12 cmd;

    if (isServerSelectState() == 0) {
        fn_802B45F4(value);
        return;
    }
    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x12);
    cmd.value_0x04 = value;
    broadcastSessionCommand(&cmd, 5);
}

/* Act 18: hands the request's byte to the local handler.
 * Name: act 18: the request's byte goes to `fn_802B45F4` */
void lb_act_index_apply(u8 unused, LbActReq* req) {
    fn_802B45F4(req->sel_0x04.bytes_0x00.a_0x00);
}

/* Sends the sub-0x13 command with the two values and this pad's index.
 * Name: (guess) sub-0x13: value byte, two halfwords and this pad's index; caller unwritten */
void lb_sub13_send(s16 first, s16 second, u8 value) {
    LbCmdSub13 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x13);
        cmd.value_0x04 = value;
        cmd.first_0x08 = first;
        cmd.second_0x0A = second;
        cmd.pad_index_0x0C = fn_800CF384();
        broadcastSessionCommand(&cmd, 0x10);
    }
}

/* Act 19: hands the request's three bytes to the value writer when its word is clear.
 * Name: act 19: with the request's word clear, three bytes go to `fn_803B3074` */
void lb_act_value_apply(u8 unused, LbActReq* req) {
    if (req->mask_0x08.word_0x00 == 0) {
        fn_803B3074(req->mask_0x0C.byte_0x00, req->mask_0x08.halves.low_0x00,
                    req->mask_0x08.halves.high_0x02);
    }
}

/* Sends the sub-0x14 command with one signed byte.
 * Name: (guess) sub-0x14 with one signed byte; caller unwritten */
void lb_sub14_send(s8 value) {
    LbCmdSub10 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x14);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 5);
    }
}

/* Act 20: sets the companion's limit flag once its value reaches the limit byte.
 * Name: act 20: sets `companion->flag_0x6A29` once the request's word reaches `limit_0x6A2A` */
void lb_act_limit_set(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL && (s32)req->mask_0x08.word_0x00 >= (s8)companion->limit_0x6A2A) {
            companion->flag_0x6A29 = 1;
        }
    }
}

/* Sends the header-only sub-0x15 command.
 * Name: (guess) header-only sub-0x15; caller unwritten */
void lb_sub15_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x15);
    broadcastSessionCommand(&cmd, 4);
}

/* Increments the companion work's tick counter.
 * Name: increments `companion->count_0x69A4` */
void lb_companion_tick(void) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            companion->count_0x69A4 = companion->count_0x69A4 + 1;
        }
    }
}

/* Sends the header-only sub-0x1C command.
 * Name: (guess) header-only sub-0x1C; caller unwritten */
void lb_sub1c_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x1C);
    broadcastSessionCommand(&cmd, 4);
}

/* Sends the sub-0x16 command with a word and two signed bytes.
 * Name: sub-0x16: a word and two signed bytes; act 21 builds the same packet inline */
void lb_sub16_send(s32 value, s8 flag) {
    LbCmdSub16 cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x16);
        cmd.flag_0x08 = flag;
        cmd.value_0x04 = value;
        cmd.value_0x09 = 0;
        broadcastSessionCommand(&cmd, 0xC);
    }
}

/* Act 21: sends the row over when this pad owns it, or writes the value into the companion slot.
 * Name: act 21: sends the row when this pad owns it, else `fn_803A9F28` writes the companion slot */
void lb_act_slot_write(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    LbCmdSub16 cmd;
    u8 index;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (req->sel_0x04.bytes_0x00.d_0x03 == 0) {
                if (isReadyCountOne() != 0 && quest_sub_state_end_ck(1) == 0 &&
                    quest_element_pick_ck((QuestWork*)companion, req->mask_0x08.byte_0x00, 1) != 1 &&
                    (s8)companion->step_0x2C != 4) {
                    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x16);
                    cmd.flag_0x08 = req->mask_0x08.byte_0x00;
                    cmd.value_0x04 = req->sel_0x04.word_0x00;
                    cmd.value_0x09 = 1;
                    broadcastSessionCommand(&cmd, 0xC);
                }
            } else if (work->state_0xFA <= 2 && quest_sub_state_end_ck(1) == 0 &&
                       quest_element_pick_ck((QuestWork*)companion, req->mask_0x08.byte_0x00, 1) != 1) {
                index = req->mask_0x08.byte_0x00;
                fn_803A9F28(companion, &companion->slots_0x94[index], (u16)index, 0);
            }
        }
    }
}

/* Sends the sub-0x17 command with a halfword and three signed bytes.
 * Name: (guess) sub-0x17: a halfword and three signed bytes; caller unwritten */
void lb_sub17_send(s16 value, s8 first, s8 second, s8 third) {
    LbCmdSub17 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x17);
    cmd.value_0x04 = value;
    cmd.first_0x06 = first;
    cmd.second_0x07 = second;
    cmd.third_0x08 = third;
    broadcastSessionCommand(&cmd, 0xA);
}

/* Act 22: keeps the companion work's high score and hands the row on.
 * Name: act 22: keeps `companion->best_0x8F` and hands the row on (`quest_item_pair_copy_row`,
 * `hud_msg_push`) */
void lb_act_best_keep(u8 unused, LbActReq* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    u8 value;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            value = req->sel_0x04.bytes_0x00.c_0x02;
            if (companion->best_0x8F < (s8)value) {
                companion->best_0x8F = value;
                companion->value_0x8B = req->sel_0x04.bytes_0x00.d_0x03;
            }
            quest_item_pair_copy_row((Q_ItemPair*)companion->pairs_0x5E2,
                                     req->sel_0x04.half_0x00,
                                     (s8)companion->best_0x8F, req->mask_0x08.byte_0x00);
            hud_msg_push(1, quest_str_tbl_35_get(0x14));
        }
    }
}

/* Sends the header-only sub-0x18 command.
 * Name: (guess) header-only sub-0x18; caller unwritten */
void lb_sub18_send(void) {
    LbCmdSub05 cmd;

    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x18);
    broadcastSessionCommand(&cmd, 4);
}

/* Puts the companion work into mode 3 (the area-change announcement).
 * Name: sets `companion->mode_0x6978 = 3`, the area-change announcement */
void lb_companion_mode_set(void) {
    LbMoveWork* work;
    LbCompanionWork* companion;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            companion->mode_0x6978 = 3;
        }
    }
}

/* Sends the sub-0x19 entry-start command and records the start in the companion work; while the act
 * is already announced the caller's flag is dropped.
 * Name: the sub-0x19 entry-start announcement from the companion's slots and params; records
 *   `started_0x6A40` */
void lb_entry_start_send(LbCompanionWork* companion, s8 value, s32 arg, u8 flag) {
    LbCmdSub19 cmd;
    u8 started;

    started = flag;
    if (flag == 1) {
        if (isReadyCountOne() == 0) {
            started = 0;
        } else if (companion->started_0x6A40 != 0) {
            return;
        }
    }
    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x19);
    cmd.value_0x04 = arg;
    cmd.value_0x08 = companion->slots_0x94[0].value_0x00;
    cmd.value_0x0C = companion->slots_0x94[1].value_0x00;
    cmd.value_0x10 = companion->slots_0x94[2].value_0x00;
    cmd.value_0x16 = companion->param_0x6A3C;
    cmd.index_0x15 = value;
    cmd.value_0x17 = companion->param_0x6A3A;
    cmd.value_0x18 = companion->param_0x6A3B;
    cmd.value_0x19 = companion->param_0x6A3D;
    cmd.value_0x1A = companion->param_0x6A3E;
    cmd.flag_0x14 = companion->param_0x6A3F;
    if (started == 1) {
        companion->started_0x6A40 = 1;
        cmd.started_0x1B = 1;
    } else {
        cmd.started_0x1B = 0;
    }
    broadcastSessionCommand(&cmd, 0x1C);
    companion->index_0x6A68 = value;
    companion->value_0x20 = arg;
    companion->step_0x2C = 4;
}

/* The act-19 announcement with the default flag.
 * Name: `lb_entry_start_send` with the default flag 0 */
void lb_entry_start_default(LbCompanionWork* companion, u8 value, s32 arg) {
    lb_entry_start_send(companion, value, arg, 0);
}

/* Act 23: copies the request's fields into the companion work and sends or applies the entry start.
 * Name: act 23: copies the request into the companion work, then sends or applies the entry start */
void lb_act_entry_start(u8 unused, LbCmdSub19* req) {
    LbMoveWork* work;
    LbCompanionWork* companion;
    LbCmdSub19 cmd;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        companion = work->companion_0xDC;
        if (companion != NULL) {
            if (companion->index_0x6A68 == 0) {
                if (isReadyCountOne() != 0 && companion->started_0x6A40 == 0) {
                    ((NetMsgHeader*)&cmd)->fill(fn_800CF384(), 0xD, 0x19);
                    cmd.value_0x04 = req->value_0x04;
                    cmd.value_0x08 = req->value_0x08;
                    cmd.value_0x0C = req->value_0x0C;
                    cmd.value_0x10 = req->value_0x10;
                    cmd.flag_0x14 = req->flag_0x14;
                    cmd.index_0x15 = req->index_0x15;
                    cmd.value_0x16 = req->value_0x16;
                    cmd.value_0x17 = req->value_0x17;
                    cmd.started_0x1B = 1;
                    broadcastSessionCommand(&cmd, 0x1C);
                    companion->started_0x6A40 = 1;
                }
            } else if (work->state_0xFA <= 2) {
                companion->value_0x24 = req->value_0x04;
                companion->slots_0x94[0].value_0x00 = req->value_0x08;
                companion->slots_0x94[1].value_0x00 = req->value_0x0C;
                companion->slots_0x94[2].value_0x00 = req->value_0x10;
                work->state_0xFA = req->flag_0x14;
                companion->index_0x6A68 = req->index_0x15;
                companion->param_0x6A3A = req->value_0x16;
                companion->param_0x6A3B = req->value_0x17;
                work->value_0xFD = req->value_0x18;
                work->value_0xFC = req->value_0x19;
                work->value_0xFB = req->value_0x1A;
                companion->param_0x6A3C = work->state_0xFA;
                companion->param_0x6A3E = work->value_0xFC;
                companion->param_0x6A3F = work->value_0xFB;
                if (work->state_0xFA == 5) {
                    quest_result_enter((Q_ItemWork*)companion, (Q_MoveWork*)work, companion->index_0x6A68);
                    return;
                }
                quest_start_enter((Q_ItemWork*)companion, (Q_MoveWork*)work);
            }
        }
    }
}

/* Sends the sub-0x1A command with one signed byte.
 * Name: (guess) sub-0x1A with one signed byte; caller unwritten */
void lb_sub1a_send(u8 index, s8 value) {
    LbCmdSub1A cmd;

    if (isServerSelectState() != 0) {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x1A);
        cmd.value_0x04 = value;
        broadcastSessionCommand(&cmd, 5);
    }
}

/* Act 24: hands the request's byte to the page handler.
 * Name: act 24: the pad index and the request's byte go to `fn_802BC000` */
void lb_act_page_apply(u8 index, LbActReq* req) {
    fn_802BC000((u8)index, req->sel_0x04.bytes_0x00.a_0x00);
}

/* Sends the header-only sub-0x1B command, or flags the area change locally when the link is down.
 * Name: header-only sub-0x1B, or `work->flag_0x22E3` locally; `enemy/fn_801A9540.cpp` calls it */
void lb_area_change_send(u8 index) {
    LbCmdSub1B cmd;
    LbMoveWork* work;

    if (isServerSelectState() == 0) {
        work = get_move_work_adrs(0);
        if (work != NULL) {
            work->flag_0x22E3 = 1;
        }
    } else {
        ((NetMsgHeader*)&cmd)->fill(index, 0xD, 0x1B);
        broadcastSessionCommand(&cmd, 4);
    }
}

/* Flags the area change in the move work without sending anything.
 * Name: sets `work->flag_0x22E3 = 1` without sending */
void lb_area_change_flag(void) {
    LbMoveWork* work;

    work = get_move_work_adrs(0);
    if (work != NULL) {
        work->flag_0x22E3 = 1;
    }
}

/* Sends the sub-0x1D command with two bytes.
 * Name: (guess) sub-0x1D with two bytes; caller unwritten */
void lb_sub1d_send(u8 value, s8 flag) {
    LbCmdSub1D cmd;

    ((NetMsgHeader*)&cmd)->fill(value, 0xD, 0x1D);
    cmd.value_0x04 = value;
    cmd.value_0x05 = flag;
    broadcastSessionCommand(&cmd, 8);
}

/* Act 25: hands the request's two bytes to the pad handler.
 * Name: act 25: the request's two bytes go to `arena_other_player_eq_set` */
void lb_act_pad_apply(u8 unused, LbActReq* req) {
    arena_other_player_eq_set(req->sel_0x04.bytes_0x00.a_0x00, req->sel_0x04.bytes_0x00.b_0x01);
}

/* Scans the ten 0x130-byte page records for the six-byte key the caller points at; 0xFF on a hole.
 * Name: the map name main's `hud/move_work_update` landing gave 0x8033A0BC (that unit calls it by this
 *   name); the body scans the ten 0x130-byte records for a 6-byte key, 0xFF on a hole */
u8 hud_key_lookup(u8* key) {
    u8 i;

    for (i = 0; i < 0xA; i++) {
        if (memcmp(key, &lbl_806BE340[i].key_0x03, 6) == 0) {
            return i;
        }
    }
    return 0xFF;
}

/* Resets the tutorial/quest announcement block and stores the kind byte.
 * Name: resets `lbl_806BEF20`'s four fields and stores the kind byte */
void lb_quest_work_init(u8 kind) {
    lbl_806BEF20.active_0x00 = 0;
    lbl_806BEF20.flag_0x08 = 0;
    lbl_806BEF20.kind_0x06 = kind;
    lbl_806BEF20.ptr_0x0C = NULL;
}

/* Whether the announcement block is idle.
 * Name: `lbl_806BEF20.active_0x00 != 0` (the old comment said "idle"; the body is the opposite) */
s32 lb_quest_work_active_ck(void) {
    return lbl_806BEF20.active_0x00 != 0;
}

/* The three-level table lookup `lb_tbl3_get`/`lb_triplet_value2_get`/`lb_triplet_value0_get`/`lb_triplet_value1_get` share.
 * Name: the three-level lookup `lbl_805E6A20[a][b][c]` */
s32 lb_tbl3_get(u8 a, u8 b, u8 c) {
    u32** level = (u32**)lbl_805E6A20[a];

    return (s32)level[b][c];
}

/* The third byte of the entry's 12-byte record.
 * Name: the third byte of the `LbTriplet` row at `lbl_805E69E8[a][b]` */
u8 lb_triplet_value2_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x02;
}

/* The word at the head of the entry's 12-byte record.
 * Name: the first byte of that row */
u8 lb_triplet_value0_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x00;
}

/* The second byte of the entry's 12-byte record.
 * Name: the second byte of that row */
u8 lb_triplet_value1_get(u8 a, u8 b) {
    return lbl_805E69E8[a][b].value_0x01;
}

/* The tutorial quest's name pointer.
 * Name: `*tutorial_quest_name` */
s32 lb_quest_name_get(void) {
    return *tutorial_quest_name;
}

/* The tutorial message pointer for one entry.
 * Name: `tutorial_quest_msg[index]` */
s32 lb_quest_msg_get(u8 index) {
    return *(tutorial_quest_msg + index);
}

/* Scans the 4-word-stride id table for `id`; the 0xFFFF terminator answers NULL.
 * Name: scans the 4-word-stride `lbl_806042B8` table for the id; the 0xFFFF terminator answers NULL */
u16* lb_page_id_find(u16 id) {
    u16* row;

    row = lbl_806042B8;
    do {
        if (*row == id) {
            return row;
        }
        row += 4;
    } while (*row != 0xFFFF);
    return NULL;
}

/* Forwards the row of the page table at `index` to the row writer.
 * Name: forwards `lbl_806043E8[index]`'s two halfwords to `fn_8033AC78` */
void lb_page_row_apply(u16 index, s32 value) {
    fn_8033AC78(lbl_806043E8[index].first_0x00, lbl_806043E8[index].second_0x02, value);
}

/* Act 26: whether the request carries no "already handled" bit.
 * Name: act 26: whether the request's fourth byte has bit 0 clear */
u32 lb_act_handled_bit_ck(u8 unused, LbActReq* req) {
    if (req != NULL && (req->sel_0x04.bytes_0x00.d_0x03 & 1) == 0) {
        return 1;
    }
    return 0;
}

/* Looks the row up and hands it to the writer; -1 when the row is not in the page table.
 * Name: `fn_8033AC78`'s row lookup; -1 when the row is absent, else `fn_8033AED0` */
s8 lb_page_row_ck(u16 id, u16 value, s16* out) {
    if (fn_8033AC78(id, value, 0) == NULL) {
        return -1;
    }
    return fn_8033AED0(out, 0, 0);
}

/* Sets the page block's per-entry bit for `index`.
 * Name: sets the bit for `index` in `lobby_world_block->bits_0x3960` */
void lb_page_entry_bit_set(u8 unused, LbActReq* req) {
    u8 index = req->sel_0x04.bytes_0x00.c_0x02;

    lobby_world_block->bits_0x3960[index >> 3] =
        lobby_world_block->bits_0x3960[index >> 3] | (u8)(1 << (index & 7));
}

/* Copies the 0xC-byte settings record from one owner to another.
 * Name: copies the 0xC-byte `LbSettings` record field by field */
void lb_settings_copy(LbSettings* dst, LbSettings* src) {
    dst->first_0x00 = src->first_0x00;
    dst->second_0x02 = src->second_0x02;
    dst->third_0x04 = src->third_0x04;
    dst->fourth_0x06 = src->fourth_0x06;
    dst->value_0x08 = src->value_0x08;
    dst->value_0x0C = src->value_0x0C;
}

/* The area-name/quest path helper the tutorial block drives.
 * Name: a 4-byte tail call of `fn_80217934`, the area-name/quest path helper */
void lb_area_name_apply(void) {
    fn_80217934();
}

/* The page block's id-table row `index` (the entry the companion page binds).
 * Name: `&lobby_world_block->ids_0x5180[index]`; `ef/eft050.cpp` calls it */
LbEntryId* lb_entry_id_get(u8 index) {
    return &lobby_world_block->ids_0x5180[index];
}

/* Clears the block's "changed" bit and republishes the entry byte to `lb_param_w`.
 * Name: clears the block's 0x80 "changed" bit and republishes the entry byte to `lb_param_w` */
void lb_entry_changed_clr(void) {
    lobby_world_block->entry_0x3E03 = (u8)(lobby_world_block->entry_0x3E03 & 0x7F);
    lb_param_w.entry_0x08 = (u8)lobby_world_block->entry_0x3E03;
}

/* Republishes the selected entry's id and its five sub-values into `lb_param_w`.
 * Name: republishes the selected entry id and its five sub-values into `lb_param_w` */
void lb_entry_publish(void) {
    LbEntryId* entry;
    u8 index;

    lb_param_w.entry_0x08 = lobby_world_block->entry_0x3E03;
    index = lobby_world_block->entry_0x3E03 & 0x7F;
    entry = lb_entry_id_get(index);
    lb_param_w.sub_0x30 = lobby_world_block->ids_0x51A6[index];
    lb_param_w.sub_0x26 = entry->byte_0x01;
    lb_param_w.sub_0x27 = entry->byte_0x02;
    lb_param_w.sub_0x28 = entry->byte_0x03;
    lb_param_w.sub_0x29 = lobby_world_block->byte_0x51A4;
    lb_param_w.sub_0x2A = lobby_world_block->byte_0x51A5;
    lb_param_w.sub_0x2C = lobby_world_block->word_0x51A0;
    lb_param_w.sub_0x2E = lobby_world_block->word_0x51A2;
}

/* Recomputes the block's page flags from the entry's model id.
 * Name: ORs `fn_802D8F84(fn_802D7B5C(word_0x51A0))` into `flags_0x519C` */
void lb_page_flags_update(void) {
    lobby_world_block->flags_0x519C =
        lobby_world_block->flags_0x519C | fn_802D8F84(fn_802D7B5C(lobby_world_block->word_0x51A0));
}

/* Publishes the selected entry's model id into the block's id table.
 * Name: stores `fn_802D7C6C(ids_0x51A6[entry])` into the id-table row */
void lb_entry_model_publish(void) {
    u8 entry = lobby_world_block->entry_0x3E03 & 0x7F;

    lobby_world_block->ids_0x5180[entry].word_0x00 = fn_802D7C6C(lobby_world_block->ids_0x51A6[entry]);
}

/* Both of the block's refresh steps, in order.
 * Name: the two refresh steps in order (`lb_page_flags_update`, `lb_entry_model_publish`) */
void lb_page_refresh(void) {
    lb_page_flags_update();
    lb_entry_model_publish();
}

/* Republishes the entry byte and then the page's name path.
 * Name: `lb_entry_publish` then `fn_80217934` */
void lb_entry_republish(void) {
    lb_entry_publish();
    fn_80217934();
}

/* Sets the bit of the entry the block has selected in the companion page's bit field.
 * Name: sets `page->bits_0x72` for the entry the block selected and mirrors the count */
void lb_page_bits_set(LbPageWork* page) {
    u8 i;
    u32 selected;

    selected = lobby_world_block->entry_0x3E03 & 0x7F;
    page->saved_0x76 = page->count_0x06;
    for (i = 0; i < page->count_0x06; i++) {
        if (selected == page->ids_0x08[i]) {
            page->bits_0x72 = (u16)(1 << i);
        }
    }
}

/* Whether the NUL-terminated byte list contains `value`.
 * Name: whether the NUL-terminated byte list contains `value` */
u32 lb_byte_list_has(u8* list, u8 value) {
    u8* p;

    for (p = list; ; p++) {
        if (*p == 0) {
            return 0;
        }
        if (value == *p) {
            return 1;
        }
    }
}

/* Draws the sub-page's arrow sprites at the layout position.
 * Name: `get_lsp_data(0x1EB6)` + `draw_sprite_ary(lbl_805E723C)` */
void lb_subpage_arrow_draw(void) {
    _mh_ivec2_ pos;

    get_lsp_data(0x1EB6, &pos);
    draw_sprite_ary(lbl_805E723C, &pos);
}

} /* extern "C" */
