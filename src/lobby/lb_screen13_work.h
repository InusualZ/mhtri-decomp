/* `LbScreen13Work`, the screen-13 work block of `lobby/lb_companion_ui.cpp`, and the `LbEntryId` setup record it
 * embeds (rule 1: one definition, included by `lobby/lb_companion_ui.h` and the screen-13 draw in `ef/eft050.cpp`). */
#ifndef MHTRI_LOBBY_LB_SCREEN13_WORK_H
#define MHTRI_LOBBY_LB_SCREEN13_WORK_H

#include "types.h"
#include "lobby/lb_list.h" /* LbList, LbItemCount (rule 1) */

struct _LB_NPC;
struct _PLW;

/* One 4-byte companion setup of the block's table at +0x5180 (`lb_entry_id_get` hands one out, `lb_entry_model_publish`
 * fills it): the mask level and the three skills, read as bytes or copied as a word.  size: 0x4 */
typedef union LbEntryId {
    /* +0x00 */ u32 word_0x00;
    struct {
        /* +0x00 */ u8 level_0x00;       /* the row of the behaviour record's slot counts */
        /* +0x01 */ u8 skills_0x01[3];   /* the skills set, 0 for an empty slot */
    };
} LbEntryId; /* size: 0x4 */

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

#endif /* MHTRI_LOBBY_LB_SCREEN13_WORK_H */
