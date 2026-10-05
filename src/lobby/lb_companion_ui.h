/* This unit's own types and declarations (`src/lobby/lb_companion_ui.cpp`, the lobby companion/status UI
 * band, `.text` 0x80338808..0x8033F270).
 *
 * A header rather than the unit's own source because the lobby C++ ABI it declares is spelled with the
 * map's own type names (`_mh_ivec2_` must be a type with exactly that name for MWCC to re-emit
 * `get_lsp_data__FUsP10_mh_ivec2_`, `LbStr__FUcUs`, ...), and a name that more than one file needs
 * belongs in one header (docs/plan.md 6.5 rule 1).  `_mh_ivec2_` is also defined by
 * `include/unsplit/lobby.h`; this unit cannot include that header, because it spells `lobby_world_block`
 * as a byte array where this range loads the 4-byte pointer the map records (`size:0x4 data:4byte`)
 * and it gives `lb_param_w` a view without the +0x26..+0x30 fields this range writes.
 *
 * docs/plan.md 6.5 rules 3/4/5: every type states its size, every field its offset and a name.
 */
#ifndef MHTRI_LOBBY_LB_COMPANION_UI_H
#define MHTRI_LOBBY_LB_COMPANION_UI_H

#include "types.h"
#include "hud/Pl_net_can_send.h"
#include "hud/eft_net_recv_state.h"
#include "hud/NetMsgHeader.h" /* `NetMsgHeader::fill` writes each command packet's 4-byte header */
#include "quest/arenatask.h" /* arena_other_player_eq_set: this band's act 25 calls it (rule 2) */
#include "stage/shell.h" /* ShellSerialEntry, serial_find, serial_state_set_word (rule 2) */

/* `include/lobby/lb_quest_screen.h` (where `quest_element_pick_ck` belongs) cannot be included from
 * here: it declares `fmt_803AA41C(s32, f32)` (the owner's two-argument form) where this header's own
 * list carries the one-argument `quest_sub_state_end_ck(s32)`, so the pair trips `illegal function overloading`.
 * The declaration below is the owner's own signature, so a TU that sees both still agrees. */
struct QuestWork;

/* The 2D integer vector the lobby/HUD helpers exchange (`_mh_ivec2_` in the map's mangling).
 * size: 0x4 */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_;

/* The sprite-data record the `draw_sprite`/`draw_font` family takes by reference; only ever passed
 * through, so the incomplete type is enough here. */
struct _SPR_DATA_;

/* ---------------------------------------------------------------------------------------------- *
 * The records this band works on.
 * ---------------------------------------------------------------------------------------------- */

/* The per-act request packet the lobby's act dispatchers hand down.  Two shapes of the same bytes are
 * read, which is why the two middle runs are unions:
 *   * +0x04..+0x07 are read as bytes by the act handlers (`lb_act_dispatch` masks them into the entry
 *     lookup, `lb_act_handover` forwards +0x04..+0x07 to `lb_entry_handover_send`) and as one word by
 *     `lb_act_award_handover` (`lwz r0,4(r29)`, `cmplwi r0,3|5`) and `lb_act_entry_publish`;
 *   * +0x08..+0x0F are one word each in `lb_act_handover` (ORed into the work block's bit masks) and a
 *     byte in `lb_entry_selected_send` (`lbz r4,8(r31)`).
 * size: 0x14 */
typedef struct LbActSelBytes {
    /* +0x00 */ u8 a_0x00;   /* the first byte `fn_803438E4`/`fn_80343B74` look the entry up with */
    /* +0x01 */ u8 b_0x01;
    /* +0x02 */ u8 c_0x02;
    /* +0x03 */ u8 d_0x03;
} LbActSelBytes; /* size: 0x4 */

typedef union LbActSel {
    /* +0x00 */ u32 word_0x00;
    /* +0x00 */ u16 half_0x00;   /* the 16-bit view `lb_act_best_keep` hands on (the target loads `lhz`) */
    /* +0x00 */ LbActSelBytes bytes_0x00;
} LbActSel; /* size: 0x4 */

typedef struct LbActHalves {
    /* +0x00 */ u16 low_0x00;
    /* +0x02 */ s16 high_0x02;
} LbActHalves; /* size: 0x4 */

typedef union LbActMask {
    /* +0x00 */ u32 word_0x00;
    /* +0x00 */ u16 half_0x00;
    /* +0x00 */ u8 byte_0x00;
    /* +0x00 */ LbActHalves halves;
    /* +0x01 */ u8 unused_0x01[0x3];
} LbActMask; /* size: 0x4 */

typedef struct LbActReq {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 index_0x01;   /* the pad index every handler checks against `fn_800CF384()` */
    /* +0x02 */ u8 unused_0x02;
    /* +0x03 */ u8 act_0x03;     /* the act id the dispatchers switch on */
    /* +0x04 */ LbActSel sel_0x04;
    /* +0x08 */ LbActMask mask_0x08;   /* ORed into the work block's +0x684 mask */
    /* +0x0C */ LbActMask mask_0x0C;   /* ORed into the work block's +0x688 mask */
    /* +0x10 */ u8 unused_0x10[0x4];
} LbActReq; /* size: 0x14 */

/* One 4-byte companion entry of `LbCompanionWork::pairs_0x5E2` (`pl_item_add`/`fn_802E5D68` are handed
 * its two halves).  size: 0x4 */
typedef struct LbCompanionPair {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ s16 value_0x02;
} LbCompanionPair; /* size: 0x4 */

/* One 0x60-byte companion slot of `LbCompanionWork::slots_0x94` (0x803A9F28` is handed one by
 * address, and the act copies fill the word at its head).  size: 0x60 */
typedef struct LbCompanionSlot {
    /* +0x00 */ s32 value_0x00;
    /* +0x04 */ u8 unused_0x04[0x5C];
} LbCompanionSlot; /* size: 0x60 */

/* The lobby/companion work record `get_move_work_adrs(0) + 0xDC` points at: the state bytes and the
 * value slots this band's handlers write.  The rest of the record belongs to the units that own those
 * offsets, so it carries its offset as filler.  size: >= 0x6A6C (the extent this band reads) */
typedef struct LbCompanionWork {
    /* +0x0000 */ u8 unused_0x0000[0x20];
    /* +0x0020 */ s32 value_0x20;      /* saved from +0x24 by the act handlers */
    /* +0x0024 */ s32 value_0x24;
    /* +0x0028 */ u8 unused_0x0028[0x4];
    /* +0x002C */ u8 step_0x2C;        /* incremented when an act takes over */
    /* +0x002D */ u8 unused_0x002D[0x5E];
    /* +0x008B */ u8 value_0x8B;       /* stored with the high score below */
    /* +0x008C */ u8 unused_0x008C[0x3];
    /* +0x008F */ s8 best_0x8F;        /* the high score `lb_act_best_keep` keeps */
    /* +0x0090 */ u8 unused_0x0090[0x4];
    /* +0x0094 */ LbCompanionSlot slots_0x94[3];  /* the three per-entry value slots the act copies fill */
    /* +0x01B4 */ u8 unused_0x01B4[0x42E];
    /* +0x05E2 */ LbCompanionPair pairs_0x5E2[1];  /* the per-companion id/value pairs */
    /* +0x05E6 */ u8 unused_0x05E6[0x9E];
    /* +0x0684 */ u32 bits_0x684[2];   /* the mask words the act handlers set (`index >> 5`) */
    /* +0x068C */ u8 unused_0x068C[0x2EC];
    /* +0x06978 */ u8 mode_0x6978;     /* set to 3 by the area-change announcement */
    /* +0x06979 */ u8 unused_0x06979[0x2B];
    /* +0x069A4 */ u8 count_0x69A4;    /* incremented once per tick (`lb_companion_tick`) */
    /* +0x069A5 */ u8 unused_0x069A5[0x84];
    /* +0x06A29 */ u8 flag_0x6A29;     /* set when the entry reaches its limit (`lb_act_limit_set`) */
    /* +0x06A2A */ s8 limit_0x6A2A;    /* the limit that byte compares against */
    /* +0x06A2B */ u8 unused_0x06A2B[0xF];
    /* +0x06A3A */ u8 param_0x6A3A;
    /* +0x06A3B */ u8 param_0x6A3B;
    /* +0x06A3C */ u8 param_0x6A3C;
    /* +0x06A3D */ u8 param_0x6A3D;
    /* +0x06A3E */ u8 param_0x6A3E;
    /* +0x06A3F */ u8 param_0x6A3F;
    /* +0x06A40 */ u32 started_0x6A40;  /* 1 once the act has been announced */
    /* +0x06A44 */ u8 unused_0x06A44[0x24];
    /* +0x06A68 */ u8 index_0x6A68;
    /* +0x06A69 */ u8 unused_0x06A69[0x3];
} LbCompanionWork; /* size: 0x6A6C */

/* The record `get_move_work_adrs(0)` returns, as this band reads it: the companion work pointer at
 * +0xDC, the act state bytes at +0xFA (set by the act handlers) and the flag at +0x22E3.  The kind-0
 * record is not the 0xB20-stride array (that is `get_move_work_adrs(2)`'s, `LbMoveEntry`).
 * size: >= 0x22E4 (the extent this band reads) */
typedef struct LbMoveWork {
    /* +0x0000 */ u8 unused_0x0000[0xDC];
    /* +0x00DC */ LbCompanionWork* companion_0xDC;
    /* +0x00E0 */ u8 unused_0x00E0[0x1A];
    /* +0x00FA */ u8 state_0xFA;    /* the act state `lb_act_award_handover` sets to 3/5 */
    /* +0x00FB */ u8 value_0xFB;    /* the three value bytes `lb_act_entry_start` copies in */
    /* +0x00FC */ u8 value_0xFC;
    /* +0x00FD */ u8 value_0xFD;
    /* +0x00FE */ u8 unused_0x00FE[0x21E5];
    /* +0x22E3 */ u8 flag_0x22E3;   /* set by the area-change announcement */
} LbMoveWork; /* size: 0x22E4 (>= 0x22E4 read) */

/* One element of the 0xB20-stride array `get_move_work_adrs(2)` returns (the per-monster move work);
 * only the flag `lb_act_handover`'s act 1 clears is named here.  size: 0xB20 (the array stride) */
typedef struct LbMoveEntry {
    /* +0x000 */ u8 unused_0x000[0x659];
    /* +0x659 */ u8 flag_0x659;
    /* +0x65A */ u8 unused_0x65A[0x4C6];
} LbMoveEntry; /* size: 0xB20 */

/* The lobby command packets `fn_80334A34` builds (a 4-byte header it fills with the command class
 * and the sub-command, then the caller's payload) and `broadcastSessionCommand` sends.  Each sub-command has its
 * own payload shape, which is why there is one type per sub-command rather than one generic buffer -
 * the same bytes carry a byte at +0x06 for sub 0x0D and a halfword there for sub 0x0E.  The size is
 * the send size rounded up to 4 (it is what the frame shows). */

typedef struct LbCmdSub010C {        /* sub 0x01 (only +0x04/+0x05/+0x07 set) and 0x0C */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 pad_index_0x04;
    /* +0x05 */ s8 flag_0x05;
    /* +0x06 */ u8 entry_0x06;
    /* +0x07 */ u8 value_0x07;
    /* +0x08 */ s32 mask_0x08;
    /* +0x0C */ s32 mask_0x0C;
} LbCmdSub010C; /* size: 0x10 */

typedef struct LbCmdSub05 {          /* sub 0x05: "this entry was selected" (header only) */
    /* +0x00 */ u32 head_0x00;
} LbCmdSub05; /* size: 0x4 */

typedef struct LbCmdSub0A {          /* sub 0x0A / 0x0B: one word payload */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u32 value_0x04;
} LbCmdSub0A; /* size: 0x8 */

typedef struct LbCmdSub0D {          /* sub 0x0D: three bytes and a halfword */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 value_0x04;
    /* +0x05 */ u8 index_0x05;
    /* +0x06 */ s8 flag_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ u16 word_0x08;
    /* +0x0A */ u8 unused_0x0A[0x2];
} LbCmdSub0D; /* size: 0xC */

typedef struct LbCmdSub0E {          /* sub 0x0E: two halfwords */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 value_0x04;
    /* +0x05 */ s8 flag_0x05;
    /* +0x06 */ u16 id_0x06;
    /* +0x08 */ u16 word_0x08;
    /* +0x0A */ u8 unused_0x0A[0x2];
} LbCmdSub0E; /* size: 0xC */

/* The 0x20-byte text block `lb_name_tail_copy` copies whole: MWCC copies its first 8 bytes a byte at a
 * time and the rest word-wise, which is what a plain struct assignment emits for this layout.
 * size: 0x20 */
typedef struct LbNameTail {
    /* +0x00 */ u8 bytes_0x00[8];
    /* +0x08 */ u32 words_0x08[6];
} LbNameTail; /* size: 0x20 */

typedef struct LbCmdSub0F {          /* sub 0x0F: the text/name command (0x2C bytes) */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 value_0x04;
    /* +0x05 */ u8 flag_0x05;
    /* +0x06 */ u16 id_0x06;
    /* +0x08 */ f32 scale_0x08;
    /* +0x0C */ LbNameTail text_0x0C;
} LbCmdSub0F; /* size: 0x2C */

typedef struct LbCmdSub10 {          /* sub 0x10: one signed byte */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s8 value_0x04;
    /* +0x05 */ u8 unused_0x05[0x3];
} LbCmdSub10; /* size: 0x8 */

typedef struct LbCmdSub11 {          /* sub 0x11: two signed bytes */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s8 value_0x04;
    /* +0x05 */ s8 value_0x05;
    /* +0x06 */ u8 unused_0x06[0x2];
} LbCmdSub11; /* size: 0x8 */

typedef struct LbCmdSub12 {          /* sub 0x12: one unsigned byte */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 value_0x04;
    /* +0x05 */ u8 unused_0x05[0x3];
} LbCmdSub12; /* size: 0x8 */

typedef struct LbCmdSub13 {          /* sub 0x13: a word, two halfwords and a byte */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s32 value_0x04;
    /* +0x08 */ s16 first_0x08;
    /* +0x0A */ s16 second_0x0A;
    /* +0x0C */ u8 pad_index_0x0C;
    /* +0x0D */ u8 unused_0x0D[0x3];
} LbCmdSub13; /* size: 0x10 */

typedef struct LbCmdSub16 {          /* sub 0x16: a word and two signed bytes */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s32 value_0x04;
    /* +0x08 */ s8 flag_0x08;
    /* +0x09 */ u8 value_0x09;
    /* +0x0A */ u8 unused_0x0A[0x2];
} LbCmdSub16; /* size: 0xC */

typedef struct LbCmdSub19 {          /* sub 0x19: the entry-start announcement */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s32 value_0x04;
    /* +0x08 */ s32 value_0x08;
    /* +0x0C */ s32 value_0x0C;
    /* +0x10 */ s32 value_0x10;
    /* +0x14 */ s8 flag_0x14;
    /* +0x15 */ u8 index_0x15;
    /* +0x16 */ u8 value_0x16;
    /* +0x17 */ u8 value_0x17;
    /* +0x18 */ u8 value_0x18;
    /* +0x19 */ u8 value_0x19;
    /* +0x1A */ u8 value_0x1A;
    /* +0x1B */ u8 started_0x1B;
} LbCmdSub19; /* size: 0x1C */

typedef struct LbCmdSub1B {          /* sub 0x1B (header only) */
    /* +0x00 */ u32 head_0x00;
} LbCmdSub1B; /* size: 0x4 */

typedef struct LbCmdSub1A {          /* sub 0x1A: one signed byte */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s8 value_0x04;
    /* +0x05 */ u8 unused_0x05[0x3];
} LbCmdSub1A; /* size: 0x8 */

typedef struct LbCmdSub17 {          /* sub 0x17: a halfword and three signed bytes */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s16 value_0x04;
    /* +0x06 */ s8 first_0x06;
    /* +0x07 */ s8 second_0x07;
    /* +0x08 */ s8 third_0x08;
    /* +0x09 */ u8 unused_0x09[0x3];
} LbCmdSub17; /* size: 0xC */

typedef struct LbCmdSub1D {          /* sub 0x1D: a byte and a signed byte */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ u8 value_0x04;
    /* +0x05 */ s8 value_0x05;
    /* +0x06 */ u8 unused_0x06[0x2];
} LbCmdSub1D; /* size: 0x8 */

/* The game/system state block's ran_suu ring this band reads: `system_w` + 0x1E is cell 3 of the
 * u16[4] at +0x18 (`lb_act_entry_publish`).  Only that word is named; the rest belongs to the units that own
 * those offsets.  size: 0xA5C (the map's size for `system_w`) */
typedef struct LbSystemView {
    /* +0x000 */ u8 unused_0x000[0x18];
    /* +0x018 */ u16 ring_0x18[4];
    /* +0x020 */ u8 unused_0x020[0xA3C];
} LbSystemView; /* size: 0xA5C */

/* `Screen_w` (0x8065903C, .bss, 0x54 B): the frame scale at +0x14 is the field this band multiplies
 * its menu values with.  size: 0x54 */
typedef struct LbScreenView {
    /* +0x00 */ u8 unused_0x00[0x14];
    /* +0x14 */ f32 scale_0x14;
    /* +0x18 */ u8 unused_0x18[0x3C];
} LbScreenView; /* size: 0x54 */

/* `lb_param_w` (0x806590B4, .bss, 0x9C B) as this band writes it: the +0x08 byte and the
 * +0x26..+0x30 run the companion page keeps there.  The other fields belong to the units that own
 * those offsets.  size: 0x9C */
typedef struct LbParamWork {
    /* +0x00 */ u8 unused_0x00[0x8];
    /* +0x08 */ u8 entry_0x08;    /* the selected entry id */
    /* +0x09 */ u8 unused_0x09[0x1D];
    /* +0x26 */ u8 sub_0x26;
    /* +0x27 */ u8 sub_0x27;
    /* +0x28 */ u8 sub_0x28;
    /* +0x29 */ u8 sub_0x29;
    /* +0x2A */ u8 sub_0x2A;
    /* +0x2B */ u8 unused_0x2B;
    /* +0x2C */ u16 sub_0x2C;
    /* +0x2E */ u16 sub_0x2E;
    /* +0x30 */ u16 sub_0x30;
    /* +0x32 */ u8 unused_0x32[0x6A];
} LbParamWork; /* size: 0x9C */

/* The block the `.sbss` pointer `lobby_world_block` (0x80794880, a 4-byte pointer) points at: the page
 * block's selected-entry byte at +0x3E03, the two entry tables at +0x5180/+0x51A0 and the per-entry
 * u16 run at +0x51A6.  Everything between them carries its offset.  size: 0x6010 (the neighbour views'
 * extent; this band reads nothing above +0x69A4) */
/* One 4-byte entry of the block's id table at +0x5180 (`lb_entry_id_get` hands one out, `lb_entry_model_publish`
 * fills it): the page stores a word there and the callers read its bytes.  size: 0x4 */
typedef union LbEntryId {
    /* +0x00 */ u32 word_0x00;
    /* +0x00 */ u8 byte_0x00;
    /* +0x01 */ u8 byte_0x01;
    /* +0x02 */ u8 byte_0x02;
    /* +0x03 */ u8 byte_0x03;
} LbEntryId; /* size: 0x4 */

typedef struct LbDataBlock {
    /* +0x0000 */ u8 unused_0x0000[0x3960];
    /* +0x3960 */ u8 bits_0x3960[0x8];   /* the per-entry bit field `lb_page_entry_bit_set` sets */
    /* +0x3968 */ u8 unused_0x3968[0x49B];
    /* +0x3E03 */ u8 entry_0x3E03;    /* the selected entry, low 7 bits = index, 0x80 = "changed" */
    /* +0x3E04 */ u8 unused_0x3E04[0x13F8];
    /* +0x51FC */ u8 unused_0x51FC[0x4];
    /* +0x5180 */ LbEntryId ids_0x5180[1];  /* the per-entry id table `lb_entry_id_get`/`lb_entry_model_publish` use */
    /* +0x5184 */ u8 unused_0x5184[0x18];
    /* +0x519C */ s32 flags_0x519C;    /* the OR-ed page flags `lb_page_flags_update` sets */
    /* +0x51A0 */ u16 word_0x51A0;
    /* +0x51A2 */ u16 word_0x51A2;
    /* +0x51A4 */ u8 byte_0x51A4;
    /* +0x51A5 */ u8 byte_0x51A5;
    /* +0x51A6 */ u16 ids_0x51A6[0x20];
    /* +0x51E6 */ u8 unused_0x51E6[0x17BE];
} LbDataBlock; /* size: 0x69A4 */

/* The companion page record `fn_8033CE30`/`lb_page_bits_set`/`fn_8033CFE4`/`fn_8033D104` work on: the
 * entry count at +0x06, the per-entry id bytes at +0x08, the bit field at +0x72 and its mirror at
 * +0x76.  Only what this band reads is named, so the size is its lower bound.  size: >= 0x9C */
typedef struct LbPageWork {
    /* +0x00 */ u8 unused_0x00[0x6];
    /* +0x06 */ s16 count_0x06;   /* the entry count the loops bound themselves with */
    /* +0x08 */ u8 ids_0x08[0x3C];
    /* +0x44 */ u8 unused_0x44[0x2E];
    /* +0x72 */ u16 bits_0x72;    /* the entry the block's selected byte matches */
    /* +0x74 */ u8 unused_0x74[0x2];
    /* +0x76 */ s16 saved_0x76;   /* the count mirrored at the head of the update */
    /* +0x78 */ u8 unused_0x78[0x24];
} LbPageWork; /* size: 0x9C (>= 0x9C read) */

/* The 0x130-byte quest/page record `hud_key_lookup` scans for a six-byte key.  size: 0x130 */
typedef struct LbQuestPage {
    /* +0x000 */ u8 unused_0x000[0x3];
    /* +0x003 */ u8 key_0x03[6];
    /* +0x009 */ u8 unused_0x009[0x127];
} LbQuestPage; /* size: 0x130 */

/* The tutorial/quest announcement block (`lbl_806BEF20`, 0x10 B): the state bytes the state machine
 * walks and the data pointer it hands out.  size: 0x10 */
typedef struct LbQuestWork {
    /* +0x00 */ u8 active_0x00;   /* 0 while the block is idle */
    /* +0x01 */ u8 state_0x01;
    /* +0x02 */ u8 value_0x02;
    /* +0x03 */ u8 value_0x03;
    /* +0x04 */ u8 flag_0x04;
    /* +0x05 */ u8 unused_0x05;
    /* +0x06 */ u8 kind_0x06;
    /* +0x07 */ u8 index_0x07;
    /* +0x08 */ u8 flag_0x08;
    /* +0x09 */ u8 value_0x09;
    /* +0x0A */ s8 mode_0x0A;
    /* +0x0B */ u8 timer_0x0B;
    /* +0x0C */ void* ptr_0x0C;
} LbQuestWork; /* size: 0x10 */

/* The 0xC-byte settings record `lb_settings_copy` copies whole.  size: 0x10 */
typedef struct LbSettings {
    /* +0x00 */ u16 first_0x00;
    /* +0x02 */ u16 second_0x02;
    /* +0x04 */ u16 third_0x04;
    /* +0x06 */ s16 fourth_0x06;
    /* +0x08 */ s32 value_0x08;
    /* +0x0C */ u8 value_0x0C;
    /* +0x0D */ u8 unused_0x0D[0x3];
} LbSettings; /* size: 0x10 */

/* One 3-byte row of the per-kind tables `lbl_805E69E8` indexes.  size: 0x3 */
typedef struct LbTriplet {
    /* +0x00 */ u8 value_0x00;
    /* +0x01 */ u8 value_0x01;
    /* +0x02 */ u8 value_0x02;
} LbTriplet; /* size: 0x3 */

/* One 4-byte row of the page table `lbl_806043E8`.  size: 0x4 */
typedef struct LbRowRef {
    /* +0x00 */ u16 first_0x00;
    /* +0x02 */ u16 second_0x02;
} LbRowRef; /* size: 0x4 */

/* ---------------------------------------------------------------------------------------------- *
 * The `.bss`/`.sbss`/`.sdata` objects this band reads.  An `extern` for an unsplit symbol belongs in
 * the band header (rule 2); the names are the map's own.
 * ---------------------------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

extern LbDataBlock* lobby_world_block;  /* .sbss 0x80794880 */
extern LbParamWork lb_param_w;     /* .bss 0x806590B4 */
extern LbSystemView system_w;      /* .bss 0x806585E0 */
extern LbScreenView Screen_w;      /* .bss 0x8065903C */
extern u8 lb_deli_data[];          /* .data 0x8060DDB8 */
extern u8 lbl_80794B90[8];         /* .sbss 0x80794B90 - the per-entry "seen" bit array */
extern u8 lbl_80794B98[8];         /* .sbss 0x80794B98 - the per-entry "handled" byte array */
extern const f32 lbl_8079B2B8;     /* .sdata2 - the delay `lb_act_award_handover` scales by Screen_w */
extern const f32 lbl_8079B2BC;     /* .sdata2 */
extern LbQuestWork lbl_806BEF20;   /* .bss 0x806BEF20 - the tutorial/quest announcement block */
extern LbQuestPage lbl_806BE340[10]; /* .bss 0x806BE340 - the ten 0x130-byte quest/page records */
extern u16 lbl_805E6A20[];         /* .data 0x805E6A20 - the three-level table (see the cast below) */
extern LbTriplet* lbl_805E69E8[];  /* .data 0x805E69E8 - the per-kind 3-byte row tables */
extern u16 lbl_806042B8[];         /* .data 0x806042B8 - the id table `lb_page_id_find` scans */
extern LbRowRef lbl_806043E8[];    /* .data 0x806043E8 - the page table `lb_page_row_apply` indexes */
extern u16 lbl_805E723C[];         /* .data - the arrow sprite rows `lb_subpage_arrow_draw` draws */
extern s32* tutorial_quest_name;   /* .sbss - the tutorial quest name pointer */
extern s32* tutorial_quest_msg;    /* .sbss - the tutorial message table */
extern u16 lbl_805E7248[];         /* .data - the sprite rows `fn_8033E088` walks */
extern u16 lbl_805E725C[];         /* .data */
extern u16 lbl_805E728C[];         /* .data */

/* The unsplit plain-C callees.  A `fn_XXXXXXXX` stem is the map's own placeholder, so these take C
 * linkage; each is declared with the argument count and widths the target's call site shows. */
void* fn_803438E4(u8 a, u8 b);
void* fn_80343B74(u8 a, u8 b, u8 c);
s32 fn_80343B44(void* entry);
void fn_802A0188(void);
void snd_quest_start_bgm_set(void);
void quest_slot_arm_all_a(s8 index);
void fn_802E5D68(u16 id, s8 index);
void fn_80125F54(void* text);
void fn_80142C58(u8 value, void* text, u16 id, u8 flag, f32 scale);
void fn_80146C00(s8 value, u8 index);
void fn_802B09B8(u8 a, u8 b);
void fn_802B45F4(u8 index);
void fn_803B3074(u8 value, u16 a, s16 b);
void fn_803B6998(u16 a, u16 b);
s32 quest_sub_state_end_ck(s32 a);
u32 quest_element_pick_ck(struct QuestWork* work, u8 index, s32 use_alt);
void fn_803A9F28(LbCompanionWork* work, LbCompanionSlot* slot, u16 index, u32 a);
s8 fn_800CF384(void);
/* The runtime byte-compare (its symbol is defined by `Runtime.PPCEABI.H/memcmp.c`, which no band
 * header declares yet; the sibling units declare it the same way). */
int memcmp(const void* a, const void* b, u32 n);
void fn_802BC000(u8 index, u8 value);
s32 fn_802D8F84(s32 a);
s32 fn_802D7B5C(u16 id);
s32 fn_802D7C6C(u16 id);
void fn_80217934(void);
void fn_800DCFE4(void);
void fn_800DBD1C(s32 a, u8 b);
void fn_8004D210(u32 a);
void fn_80046EE4(void);
s32 fn_803C482C(void* psw);
s32 fn_8028D574(void);
u8* fn_8033AC78(u16 a, u16 b, s32 c);
s8 fn_8033AED0(s16* out, s32 a, s32 b);
s8 fn_800CF384(void);
/* The runtime byte-compare (its symbol is defined by `Runtime.PPCEABI.H/memcmp.c`, which no band
 * header declares yet; the sibling units declare it the same way). */
int memcmp(const void* a, const void* b, u32 n);


#ifdef __cplusplus
}
#endif

/* 0x80338A84 - clears the lobby entry flags (the leaf header carries the prototype). */
#include "lobby/lb_entry_flags_clear.h"

#ifdef __cplusplus
/* The C++-linkage AI/move-work accessor whose map name is `get_move_work_adrs__FUc` (rule 9). */
LbMoveWork* get_move_work_adrs(u8 kind);
#endif

#ifdef __cplusplus

/* The mangled map names are the compiler's spelling of these declarations (rule 9): the front-end
 * reproduces each map name exactly and the call site writes the plain function. */
void* LbStr(u8 kind, u16 index);
u32 chk_pointer(void);
void get_lsp_data(u16 id, _mh_ivec2_* out);
u8 get_option_cfg(u8 index);
void sysSE_req(s32 id);
void font_flush(void);
void set_blendmode(u8 a, u8 b, u8 c);
void set_zmode(u32 a, u8 b, u32 c);
void draw_sprite(const _SPR_DATA_& spr, const _mh_ivec2_* pos);
void draw_sprite_ary(const u16* table, const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void draw_sprite_anim_ary(const u16* table, u16 index, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 index, const _mh_ivec2_* pos);
void draw_font_idx(u16 id, s8* text, u32 len, const _mh_ivec2_* pos);
u8 get_now_areano(void);
u16 ran_suu(s32 index);
void Pl_cat_skill_ck(struct _PLW* plw, u16 skill);

#endif /* __cplusplus */

#endif /* MHTRI_LOBBY_LB_COMPANION_UI_H */
