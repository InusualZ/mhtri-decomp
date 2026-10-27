/* The types and declarations of `lobby/lb_companion_ui.cpp`, with the map's own type names its manglings need
 * (`_mh_ivec2_`); `unsplit/lobby.h` cannot be included beside it (a byte-array `lobby_world_block`, not the 4-byte
 * pointer, and an `lb_param_w` without the +0x26..+0x30 fields this unit writes). */
#ifndef MHTRI_LOBBY_LB_COMPANION_UI_H
#define MHTRI_LOBBY_LB_COMPANION_UI_H

#include "types.h"
#include "hud/Pl_net_can_send.h"
#include "hud/eft_net_recv_state.h"
#include "hud/NetMsgHeader.h" /* `NetMsgHeader::fill` writes each command packet's 4-byte header */
#include "quest/arenatask.h" /* arena_other_player_eq_set: this band's act 25 calls it (rule 2) */
#include "stage/shell.h" /* ShellSerialEntry, serial_find, serial_state_set_word (rule 2) */
#include "lobby/lobby_hunter_cards.h" /* LbQuestPage, lobby_hunter_cards - this unit's leaf header */
#include "id_value.h" /* IdValue, the item box slots */
#include "hud/layout.h" /* _mh_ivec2_, _SPR_DATA_ and the draw_* family (rule 1, rule 2) */
#include "lobby/lb_list.h" /* LbList, LbItemCount (rule 1) */

/* `quest_element_pick_ck` (owner `lobby/lb_quest_screen.h`) is declared below with the owner's signature. */
struct QuestWork;
struct _PLW;
struct _LB_NPC;


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
    /* +0x00 */ u8 a_0x00;   /* the first byte `enemy_data_find`/`eft_slot_spawn` look the entry up with */
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
    /* +0x01 */ u8 index_0x01;   /* the pad index every handler checks against `my_player_no()` */
    /* +0x02 */ u8 unused_0x02;
    /* +0x03 */ u8 act_0x03;     /* the act id the dispatchers switch on */
    /* +0x04 */ LbActSel sel_0x04;
    /* +0x08 */ LbActMask mask_0x08;   /* ORed into the work block's +0x684 mask */
    /* +0x0C */ LbActMask mask_0x0C;   /* ORed into the work block's +0x688 mask */
    /* +0x10 */ u8 unused_0x10[0x4];
} LbActReq; /* size: 0x14 */

/* One 4-byte companion entry of `LbCompanionWork::pairs_0x5E2` (`pl_item_add`/`cockpit_pointer_target_set` are handed
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
    /* +0x068C */ u8 unused_0x068C[0x62EC];
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

typedef struct LbCmdSub16 {          /* sub 0x16: a word and two bytes */
    /* +0x00 */ u32 head_0x00;
    /* +0x04 */ s32 value_0x04;
    /* +0x08 */ u8 flag_0x08;
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
/* One 4-byte companion setup of the block's table at +0x5180 (`lb_entry_id_get` hands one out, `lb_entry_model_publish`
 * fills it): the mask level and the three skills, read as bytes or copied as a word.  size: 0x4 */
typedef union LbEntryId {
    /* +0x00 */ u32 word_0x00;
    struct {
        /* +0x00 */ u8 level_0x00;       /* the row of the behaviour record's slot counts */
        /* +0x01 */ u8 skills_0x01[3];   /* the skills set, 0 for an empty slot */
    };
} LbEntryId; /* size: 0x4 */

typedef struct LbDataBlock {
    /* +0x0000 */ u8 unused_0x0000[0x180];
    /* +0x0180 */ IdValue box_0x180[0x3E8];  /* the item box (`userdata_box_capacity` slots; the count is a GUESS) */
    /* +0x1120 */ u8 unused_0x1120[0x2840];
    /* +0x3960 */ u8 bits_0x3960[0x8];   /* the per-entry bit field `lb_page_entry_bit_set` sets */
    /* +0x3968 */ u8 unused_0x3968[0x49B];
    /* +0x3E03 */ u8 entry_0x3E03;    /* the selected entry, low 7 bits = index, 0x80 = "changed" */
    /* +0x3E04 */ u8 unused_0x3E04[0x100];
    /* +0x3F04 */ s32 zenny_0x3F04;   /* the zenny `userdata_zenny_add` credits */
    /* +0x3F08 */ u8 unused_0x3F08[0x1278];
    /* +0x5180 */ LbEntryId ids_0x5180[7];  /* the companion setup of each entry */
    /* +0x519C */ s32 flags_0x519C;    /* the skills unlocked, one bit per skill */
    /* +0x51A0 */ u16 word_0x51A0;
    /* +0x51A2 */ u16 word_0x51A2;
    /* +0x51A4 */ u8 byte_0x51A4;     /* the two dances picked (screen 13's dance page) */
    /* +0x51A5 */ u8 byte_0x51A5;
    /* +0x51A6 */ u16 ids_0x51A6[7];  /* the level of each entry */
    /* +0x51B4 */ LbItemCount gets_0x51B4[12];  /* the items collected for the player, two runs */
    /* +0x51E4 */ LbItemCount gets_0x51E4[30];
    /* +0x525C */ u8 unused_0x525C[0x1748];
} LbDataBlock; /* size: 0x69A4 */

/* The companion setup `lb_status_preview_update` hands `ai_npc_status_preview`: the layout of `ai/ai_npc.h`'s
 * `AINPCSetup`, which cannot be included beside this unit's headers (its `_HIT_W` collides with `Pl/hit_w.h`'s).
 * size: 0xA */
typedef struct LbCompanionSetup {
    /* +0x0 */ u8 entry_0x0;        /* the entry (its behaviour record) */
    /* +0x1 */ u8 skills_0x1[3];
    /* +0x4 */ u16 attack_0x4;
    /* +0x6 */ u16 gauge_0x6;
    /* +0x8 */ u16 level_0x8;
} LbCompanionSetup; /* size: 0xA */

/* One 6-byte row of `lb_skill_unlock_items` (ended by `need_0x00` 0xFFFF): once `need_0x00` is set and `done_0x02`
 * still clear, the skill is unlocked and announced.  size: 0x6 */
typedef struct LbSkillUnlock {
    /* +0x00 */ u16 need_0x00;
    /* +0x02 */ u16 done_0x02;
    /* +0x04 */ u16 skill_0x04;
} LbSkillUnlock; /* size: 0x6 */

/* One 4-byte row of `lb_skill_unlock_flags` (ended by 0xFFFF): the progress flag set once the skill is unlocked.
 * size: 0x4 */
typedef struct LbSkillFlag {
    /* +0x00 */ u16 flag_0x00;
    /* +0x02 */ u16 skill_0x02;
} LbSkillFlag; /* size: 0x4 */

/* The screen-13 work block (`lobby_w.screen13_0x0AC`, cleared over 0x2000 bytes when the screen opens): the screen's
 * state, the main choice and the per-page list, the entry and dance rows, the skill page's cursors and rows, the status
 * preview and the collected items handed to the item box.  size: 0x2000 */
typedef struct LbScreen13Work {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 sub_0x001;           /* the page's own state */
    /* +0x002 */ u8 picking_0x002;       /* the skill list steps only while 0 */
    /* +0x003 */ u8 help_0x003;          /* the help view the 0x80 command toggles */
    /* +0x004 */ u8 changed_0x004;       /* a skill was set or removed */
    /* +0x005 */ u8 unused_0x005;
    /* +0x006 */ s16 count_0x006;        /* the entries listed in `ids_0x008` */
    /* +0x008 */ u8 ids_0x008[8];        /* the entries the entry and skill pages list */
    /* +0x010 */ s16 timer_0x010;        /* the input delay after opening */
    /* +0x012 */ u8 dances_0x012[4];     /* the dances the dance page lists */
    /* +0x016 */ u8 dance_0x016[2];     /* the two dances picked (0xFF: none) */
    /* +0x018 */ u8 skill_rows_0x018[8]; /* the skills of the shown skill page */
    /* +0x020 */ u8 others_0x020[3];     /* the skills set in the other slots */
    /* +0x023 */ u8 unused_0x023;
    /* +0x024 */ u16 skill_flags_0x024[8];  /* per skill row: 1 already set, 2..0x20 a clash */
    /* +0x034 */ s16 slot_0x034;         /* the skill slot cursor */
    /* +0x036 */ s16 action_0x036;       /* the slot action cursor: change or remove */
    /* +0x038 */ s16 cursor_0x038;       /* the skill row cursor */
    /* +0x03A */ s16 page_0x03A;         /* the skill page shown */
    /* +0x03C */ s16 pages_0x03C;
    /* +0x03E */ s16 skill_count_0x03E;  /* the skill rows of the page */
    /* +0x040 */ u16 moved_0x040;        /* the page step's "moved" word */
    /* +0x042 */ u8 unused_0x042[0x2];
    /* +0x044 */ s32 confirm_0x044;      /* the yes/no toggle word */
    /* +0x048 */ s32 talk_0x048;         /* the talk page `lb_menu_step` runs */
    /* +0x04C */ s32 talk_arg_0x04C;
    /* +0x050 */ LbList menu_0x050;      /* the main choice */
    /* +0x070 */ LbList list_0x070;      /* the page's list */
    /* +0x090 */ LbEntryId* entry_0x090; /* the setup being edited (`edit_0x098`) */
    /* +0x094 */ const u8* behaviour_0x094;  /* the entry's behaviour record (+0x04: the skill slots per level) */
    /* +0x098 */ LbEntryId edit_0x098;
    /* +0x09C */ u8 status_0x09C[0x1C];  /* the `ai_npc_status_preview` result (`ai/ai_npc.h`'s `AINPCStatus`) */
    /* +0x0B8 */ LbItemCount gets_0x0B8[20];  /* the collected items handed to the item box */
    /* +0x108 */ LbItemCount gets_0x108[30];
    /* +0x180 */ struct _LB_NPC* npc_0x180;
    /* +0x184 */ struct _PLW* player_0x184;
    /* +0x188 */ u8 unused_0x188[0x1E78];
} LbScreen13Work; /* size: 0x2000 */


/* One 10-byte row of a tutorial event's entry list (`lb_tutorial_entry_lists[event]`, ended by a negative
 * `flag_0x00`): the row is taken when its kind matches, its own progress flag is still clear, `need_0x06` is set
 * (or -1) and `block_0x08` is clear (or -1).  size: 0xA */
typedef struct LbTutorialEntry {
    /* +0x00 */ s16 flag_0x00;    /* the progress flag the row sets once shown */
    /* +0x02 */ u16 kind_0x02;    /* compared with `LbQuestWork::kind_0x06` */
    /* +0x04 */ u8 page_0x04;     /* the page set `lb_tutorial_entry_find` returns */
    /* +0x05 */ s8 note_0x05;     /* the note list the row opens (negative: none) */
    /* +0x06 */ s16 need_0x06;    /* a progress flag that must be set, or -1 */
    /* +0x08 */ s16 block_0x08;   /* a progress flag that must be clear, or -1 */
} LbTutorialEntry; /* size: 0xA */

/* The tutorial announcement block (`lb_tutorial_work`, 0x10 B): the state `lb_tutorial_step` walks (0 idle, 1 the
 * pages, 2 the note list, 3 closing), the page cursor and the entry row it shows.  size: 0x10 */
typedef struct LbQuestWork {
    /* +0x00 */ u8 active_0x00;      /* the state; 0 while the block is idle */
    /* +0x01 */ u8 page_0x01;        /* the page shown */
    /* +0x02 */ u8 page_count_0x02;  /* the last page (`lb_triplet_value2_get`) */
    /* +0x03 */ u8 note_row_0x03;    /* the note list's cursor row */
    /* +0x04 */ u16 cursor_0x04;     /* the frame cursor word, cleared every step */
    /* +0x06 */ u8 kind_0x06;        /* the tutorial kind `lb_quest_work_init` stores */
    /* +0x07 */ u8 index_0x07;       /* the page set the request picked */
    /* +0x08 */ u8 flag_0x08;        /* a request is pending */
    /* +0x09 */ u8 last_0x09;        /* the last page is up */
    /* +0x0A */ s8 mode_0x0A;        /* the requesting event id */
    /* +0x0B */ u8 timer_0x0B;       /* the input delay after opening */
    /* +0x0C */ LbTutorialEntry* ptr_0x0C;  /* the entry row shown */
} LbQuestWork; /* size: 0x10 */

/* One 0x10-byte row of the delivery list table `lb_deli_list_data` (ended by `cond_0x00` 0xFFFF) and of the list
 * `lb_deli_list_build` keeps: the row is offered while its unlock condition holds, its two progress flags are set
 * (or 0xFFFF) and `flag_0x08` is still clear (or -1).  size: 0x10 */
typedef struct LbDeliEntry {
    /* +0x00 */ u16 cond_0x00;     /* the `lb_unlock_cond_ck` id */
    /* +0x02 */ u16 need_0x02;     /* a progress flag that must be set, or 0xFFFF */
    /* +0x04 */ u16 need_0x04;     /* a second one */
    /* +0x06 */ s16 order_0x06;    /* the `lb_deli_data` row the delivery asks for */
    /* +0x08 */ s32 flag_0x08;     /* the progress flag the delivery unlocks (-1: none) */
    /* +0x0C */ s8 talk_0x0C;      /* the talk page opened once it is delivered */
    /* +0x0D */ u8 unused_0x0D[0x3];
} LbDeliEntry; /* size: 0x10 */

/* One 0x1C-byte row of `lb_deli_data` as the delivery menu reads it: four item/count pairs (item 0xFFFF is a zenny
 * cost, 0 an empty slot) and the three text pointers its window prints.  size: 0x1C */
typedef struct LbDeliOrder {
    /* +0x00 */ LbItemCount items_0x00[4];
    /* +0x10 */ s8* text_0x10;     /* printed at layout 0x1E8E */
    /* +0x14 */ s8* title_0x14;    /* printed at layout 0x1E8C */
    /* +0x18 */ s8* client_0x18;   /* printed at layout 0x1E8D */
} LbDeliOrder; /* size: 0x1C */

/* The delivery menu's work block (`lb_deli_work`): the area-select screen's state, the menu's own state and cursor,
 * the choice list, the offered rows and the player/NPC it was built for.  size: 0xD8 */
typedef struct LbDeliWork {
    /* +0x00 */ u8 state_0x00;       /* `lb_area_select_screen_step`'s state */
    /* +0x01 */ u8 menu_state_0x01;  /* `lb_deli_menu_step`'s state: 0 the list, 1 the confirm, 2 the talk */
    /* +0x02 */ u8 anim_0x02;        /* the talk mark's animation (7 is stepped to 8) */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ u16 moved_0x04;      /* the cursor step's "moved" word, cleared every step */
    /* +0x06 */ u8 unused_0x06[0x2];
    /* +0x08 */ s16 cursor_0x08;
    /* +0x0A */ s16 count_0x0A;      /* the rows offered */
    /* +0x0C */ s32 confirm_0x0C;    /* the yes/no toggle word */
    /* +0x10 */ LbList list_0x10;
    /* +0x30 */ LbDeliEntry entries_0x30[10];
    /* +0xD0 */ u8* npc_0xD0;        /* the NPC record; +0x02 is its id */
    /* +0xD4 */ struct _PLW* plw_0xD4;
} LbDeliWork; /* size: 0xD8 */

/* One 3-byte row of the per-kind tables `tutorial_page_sets` indexes.  size: 0x3 */
typedef struct LbTriplet {
    /* +0x00 */ u8 value_0x00;
    /* +0x01 */ u8 value_0x01;
    /* +0x02 */ u8 value_0x02;
} LbTriplet; /* size: 0x3 */

/* One 8-byte combine recipe of `lb_combo_recipe_tbl`: the second ingredient (the first is the index row's item), the
 * result, the rate class, the yield list and the progress bit that marks it known.  size: 0x8 */
typedef struct LbComboRecipe {
    /* +0x00 */ s16 partner_0x00;   /* the second ingredient */
    /* +0x02 */ s16 result_0x02;    /* the item the combination makes */
    /* +0x04 */ s8 rate_0x04;       /* the `lb_combo_base_rates` class */
    /* +0x05 */ u8 yield_0x05;      /* the `lb_combo_yield_odds`/`lb_combo_yield_tbl` index */
    /* +0x06 */ u8 known_bit_0x06;  /* the bit of `LbDataBlock::bits_0x3960` that marks it known */
    /* +0x07 */ u8 flags_0x07;      /* bit 0 hides it; 2/4/8/0x10 grant 100 % with a skill */
} LbComboRecipe; /* size: 0x8 */

/* One 4-byte row of `lb_combo_index_tbl`: a first ingredient and its run of recipes.  size: 0x4 */
typedef struct LbComboIndex {
    /* +0x00 */ u16 item_0x00;      /* 0xFFFF ends the table */
    /* +0x02 */ u8 count_0x02;
    /* +0x03 */ u8 first_0x03;      /* the run's first `lb_combo_recipe_tbl` row */
} LbComboIndex; /* size: 0x4 */

/* One 4-byte row of `lb_combo_pair_tbl`: the two ingredients of a listed recipe.  size: 0x4 */
typedef struct LbComboPair {
    /* +0x00 */ u16 first_0x00;
    /* +0x02 */ u16 second_0x02;
} LbComboPair; /* size: 0x4 */

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
extern u8 lbl_80794B90[8];         /* .sbss 0x80794B90 - the per-entry "seen" bit array */
extern u8 lbl_80794B98[8];         /* .sbss 0x80794B98 - the per-entry "handled" byte array */
extern const f32 lbl_8079B2B8;     /* .sdata2 - the delay `lb_act_award_handover` scales by Screen_w */
extern const f32 lbl_8079B2BC;     /* .sdata2 */
extern LbQuestWork lb_tutorial_work;   /* .bss 0x806BEF20 - the tutorial announcement block */
extern LbTutorialEntry* lb_tutorial_entry_lists[];  /* .data 0x805E68FC - the entry list of each tutorial event */
extern s8** tutorial_name;              /* .sbss 0x80794BD0 - the tutorial kinds' title strings */
extern s8** tutorial_end;               /* .sbss 0x80794BD4 - the two closing-page strings */
extern u16 tutorial_kind_sprite_ids[4];  /* .sdata 0x80792D38 - the title sprite of each tutorial kind */
extern u16 tutorial_end_sprite_ids[4];   /* .sdata 0x80792D40 - the closing page's button sprites */
extern u16 tutorial_end_sprite_ids_alt[4];  /* .sdata 0x80792D48 - the same with option 7 set */
extern s8*** tutorial_page_texts[]; /* .data 0x805E6A20 - each tutorial kind's page texts, by page set */
extern LbTriplet* tutorial_page_sets[];  /* .data 0x805E69E8 - each tutorial kind's page sets */
extern s32* tutorial_quest_name;   /* .sbss - the tutorial quest name pointer */
extern s32* tutorial_quest_msg;    /* .sbss - the tutorial message table */

/* The unsplit plain-C callees.  A `fn_XXXXXXXX` stem is the map's own placeholder, so these take C
 * linkage; each is declared with the argument count and widths the target's call site shows. */
void* fn_803438E4(u8 a, u8 b);
void* fn_80343B74(u8 a, u8 b, u8 c);
s32 fn_80343B44(void* entry);
void menu_item_close(void);
void snd_quest_start_bgm_set(void);
void quest_slot_arm_all_a(s8 index);
void cockpit_pointer_target_set(u16 id, s8 index);
void em_ground_rec_clear(void* text);
void em_small_spawn_stat(u8 value, void* text, u16 id, u8 flag, f32 scale);
void em_res_peer_count_inc(s8 value, u8 index);
void stage_area_flag_set(u8 a, u8 b);
void stage_area_timer_arm(u8 index);
s8 fn_800CF384(void);
/* The runtime byte-compare (its symbol is defined by `Runtime.PPCEABI.H/memcmp.c`, which no band
 * header declares yet; the sibling units declare it the same way). */
int memcmp(const void* a, const void* b, u32 n);
void camera_slot_bit_set(u8 index, u8 value);
s32 ai_npc_level_get(u16 id);
s32 fn_802D7C6C(u16 id);
void userdata_progress_flag_set(u32 a);
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
u8 get_option_cfg(u8 index);
void sysSE_req(s32 id);
void font_flush(void);
void set_blendmode(u8 a, u8 b, u8 c);
void set_zmode(bool test, u8 func, bool update);
u8 get_now_areano(void);
u16 ran_suu(s32 index);

#endif /* __cplusplus */

#endif /* MHTRI_LOBBY_LB_COMPANION_UI_H */
