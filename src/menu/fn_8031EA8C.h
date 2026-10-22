/*
 * menu/fn_8031EA8C.h - `menu/fn_8031EA8C.cpp`'s views, traced from the target: `MenuFxWork`/`MenuFxWorkB`, the effect
 *   instance's +0x38 work records (cast from `_EFT::work_0x38`), `MenuPlJoints`, and `MenuQuestWork`, the 0x2000-byte
 *   quest screen work block at `lobby_w.menu_0xAC`.
 */
#ifndef MHTRI_MENU_FN_8031EA8C_H
#define MHTRI_MENU_FN_8031EA8C_H

#include "types.h"
#include "ef.h"
#include "pl.h"
#include "lobby/lb_list.h"

/* The effect instance's `+0x38` work record, in the family view these updaters use: a model count, the
 * `MHchar` model pointers the count covers, the per-model s16 the state machine seeds, and a counter.
 * size: 0x18 lower bound (the highest offset the band touches is +0x16). */
struct MenuFxWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[3];
    /* +0x10 */ s16 value_0x10[3];
    /* +0x16 */ s16 counter_0x16;
};
/* size: 0x18 (approximate) */

/* The second effect family's `+0x38` view: a single model and the s16 byte `menu_target_marker_set` seats beside
 * it (the same bytes `MenuFxWork` reads as `models[0]`/`models[1]`; the family spawns one model). */
struct MenuFxWorkB {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ u16 bob_phase_0x08;   /* the bob angle, +0x200 a frame (`menu_target_marker_follow`) */
    /* +0x0A */ u8  pad_0x0A[0x06];
};
/* size: 0x10 (approximate) */

/* The joint holder `_PLW::physics_0x13C` points at, in the view the menu effect reads: the player's `MHchar` sits
 * at +0x04.  size: 0x144 (approximate: the model record's end) */
struct MenuPlJoints {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar joint_0x004;
};

/* The quest screens' work block at `lobby_w.menu_0xAC` (0x2000 bytes; the open functions memset it whole): the
 * screen state, the join request (`NetJoinParams`'s layout at +0x40: quest, party size, the 4-character session
 * name and the 0x90-character comment the keyboard edits), the player's board data and the NPC talked to, the
 * yes/no list, and the edit copies the keyboard writes back at +0x19C and +0x1A6.  Only the offsets the band
 * reads are named - the rest is `pad_0xNN`.
 * size: 0x237 lower bound (approximate: the highest offset the band touches is +0x1A6 + 0x91). */
struct MenuQuestWork {
    /* +0x000 */ u8  state_0x000;      /* the screen state the step functions switch on */
    /* +0x001 */ u8  state_0x001;      /* the list/keyboard sub-state machine (`quest_list_select_step`, `fn_80321A14`) */
    /* +0x002 */ u8  submode_0x002;    /* the sub-state that machine's states switch on */
    /* +0x003 */ u8  pad_0x003;
    /* +0x004 */ u16 open_frame_0x004; /* the group menu's open count-up, 0..6 */
    /* +0x006 */ u8  accept_anim_0x006;  /* the talk page the join refusal shows; 7 -> 8 once the accept wait starts */
    /* +0x007 */ s8  wait_0x007;       /* the accept wait, 3 frames */
    /* +0x008 */ u8  unlocked_0x008;   /* 1 when the board's unlock condition 0x1A holds offline */
    /* +0x009 */ u8  talking_0x009;    /* 1 while the closing talk page runs */
    /* +0x00A */ u16 field_0x00A;      /* the cursor steps' `moved` word */
    /* +0x00C */ s16 group_cursor_0x00C;
    /* +0x00E */ s16 group_count_0x00E;
    /* +0x010 */ u8  groups_0x010[8];  /* the group menu's rows (board groups, 6 the special, 7 the event row) */
    /* +0x018 */ u8  tabs_0x018[8];    /* the same rows as tab ids */
    /* +0x020 */ s16 field_0x020;      /* the selected board group */
    /* +0x022 */ s16 value_0x022;      /* the quest list's row cursor */
    /* +0x024 */ s16 count_0x024;      /* the quests on the page */
    /* +0x026 */ u16 ids_0x026[8];     /* the page's quest ids */
    /* +0x036 */ u16 field_0x036;      /* the quest id the screen shows and accepts */
    /* +0x038 */ s16 page_0x038;
    /* +0x03A */ s16 pages_0x03A;
    /* +0x03C */ s16 field_0x03C;      /* the detail page's tab cursor */
    /* +0x03E */ s16 tab_count_0x03E;
    /* +0x040 */ u16 field_0x040;      /* the join request's quest, mirrored from +0x36 */
    /* +0x042 */ s8  row_0x042;        /* the party size, 1..4 `fn_80321A14` clamps */
    /* +0x043 */ char name_0x043[5];   /* the 4-character session name + NUL the keyboard edits */
    /* +0x048 */ char text_0x048[0x91];/* the 0x90-character comment + NUL the keyboard edits */
    /* +0x0D9 */ u8  pad_0x0D9[0x08B];
    /* +0x164 */ s16 field_0x164;
    /* +0x166 */ s16 field_0x166;      /* 4 or 2, mirrored into +0x42 */
    /* +0x168 */ u32 field_0x168;
    /* +0x16C */ struct LbQuestBoardData* board_0x16C;  /* the player's board data the screen was opened with */
    /* +0x170 */ struct _LB_NPC* npc_0x170;             /* the NPC the player talks to */
    /* +0x174 */ struct QuestRecord* rec_0x174;  /* the record of the quest at +0x36 */
    /* +0x178 */ LbList list_0x178;    /* the yes/no list */
    /* +0x198 */ u8  no_draw_0x198;    /* 1 skips the frame's draw request */
    /* +0x199 */ u8  pad_0x199;
    /* +0x19A */ u16 rank_0x19A;       /* the hunter rank `userdata_hunter_rank_info_get` reports */
    /* +0x19C */ char edit_name_0x19C[5];
    /* +0x1A1 */ u8  pad_0x1A1[0x005];
    /* +0x1A6 */ char edit_text_0x1A6[0x91];
};

/* `lobby_world_block` (a `u8*` in its owner's header) in the view the quest screens read: the money word the
 * entry fee is checked against and charged from.  size: 0x1C lower bound (approximate) */
struct MenuWorldBlockView {
    /* +0x000 */ u8  pad_0x000[0x18];
    /* +0x018 */ s32 money_0x018;
};

struct QuestRecord;
struct LbQuestBoardData;

#ifdef __cplusplus
extern "C" {
#endif

/* The quest board's entry points into this range (rule 2: their owner's header).  GUESS names, from the lobby quest
 * board's calls: 0x80321830 takes quest `quest_id` for the player work `plw` (lobby parameter block, `lobby_w`'s
 * quest bytes, the payload's flag byte); 0x8032194C confirms and resets the party state; 0x80322CEC / 0x80322650 /
 * 0x80322B98 / 0x80322C68 / 0x80322DE4 draw the quest detail page's header, its target, reward and label tabs and
 * its footer; 0x80323428 wraps a comment into `out`, 24 characters a line. */
void quest_board_accept(struct LbQuestBoardData* plw, u16 quest_id, u8 mode);
void quest_party_state_reset(_PLW* plw);
void quest_detail_header_draw(struct QuestRecord* rec);
void quest_detail_target_draw(struct QuestRecord* rec);
void quest_detail_reward_draw(struct QuestRecord* rec);
void quest_detail_label_draw(struct MenuQuestWork* self);
void quest_detail_footer_draw(struct QuestRecord* rec);
void quest_comment_wrap(char* text, char* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_FN_8031EA8C_H */
