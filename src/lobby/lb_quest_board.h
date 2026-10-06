/* `lobby/lb_quest_board.cpp`'s views of the lobby records it reads and the other units' callees it needs; `lobby_w` is
 * viewed as `LbQuestBoardLobby`, one view per unit like `lobby/fn_8021E1EC.h` and `lobby/fn_802FA9A0.h` (the views
 * disagree on which byte is the "state"). */
#ifndef MHTRI_LOBBY_LB_QUEST_BOARD_H
#define MHTRI_LOBBY_LB_QUEST_BOARD_H

#include "types.h"
#include "nw4r/math.h"
#include "Network/network_pat_control.h"   /* NetProfileLabels: the board entry's label block */

namespace nw4r {
namespace ef {
class Effect;
}  // namespace ef
}  // namespace nw4r

struct MHchar;
struct _EFT;

/* The work block the board's effect records (`_EFT::work_0x38`) carry.  Kinds 0..7 hold one effect, its scale, the
 * joint it rides on and the offset from that joint; kinds 8 and 9 hold one model and the frame window it flashes in.
 * size: 0x20 */
typedef struct LbQuestBoardEft {
    /* +0x00 */ s32 count;
    /* +0x04 */ union {
        nw4r::ef::Effect* effect;          /* kinds 0..7 */
        MHchar* model;                     /* kinds 8 and 9 */
    };
    /* +0x08 */ union {
        f32 scale;                         /* kinds 0..7: the effect's parameter scale */
        s32 flash_from;                    /* kind 8: the first frame of the flash */
    };
    /* +0x0C */ union {
        u32 joint;                         /* kinds 0..7: the source's joint the effect rides on */
        s32 flash_to;                      /* kind 8: the frame the flash ends at */
    };
    /* +0x10 */ VEC3 offset;               /* kinds 0..7: the offset from that joint */
    /* +0x1C */ u8 pad_0x1C[0x4];
} LbQuestBoardEft; /* size: 0x20 */

#include "lobby/lb_quest_board_data.h" /* LbQuestBoardData */

/* One board entry as `getProfileQuestRecord` hands it out: the tail of a lobby profile record (`NetProfileRec` from
 * +0x190) - the quest id, the players the quest wants, the comment and the label block.  size: 0x124 */
typedef struct LbQuestBoardEntry {
    /* +0x000 */ u16 quest_id;
    /* +0x002 */ s8 players;               /* the players the quest wants */
    /* +0x003 */ u8 pad_0x003[0x5];
    /* +0x008 */ char comment[0x94];
    /* +0x09C */ NetProfileLabels labels;
} LbQuestBoardEntry; /* size: 0x124 */

/* The quest-board screen work block reachable through `lobby_w.menu_0xAC` - the object the band memsets whole
 * (0x2000 bytes) and the update/draw steps take as their `this`.
 * size: 0x2000 */
typedef struct LbQuestBoardWork {
    /* +0x000 */ u8 unused_0x000[0x240];
    /* +0x240 */ s16 blink_0x240;          /* the waiting dialog's blink counter */
    /* +0x242 */ s8 step_0x242;            /* the screen's step */
    /* +0x243 */ s8 enter_step_0x243;      /* the profile-enter sub-step */
    /* +0x244 */ s8 enter_error_0x244;     /* why the enter failed (1 quest mismatch, 2 download) */
    /* +0x245 */ s8 kbd_step_0x245;        /* the name keyboard's sub-step */
    /* +0x246 */ u8 return_step_0x246;     /* the step a notice returns to */
    /* +0x247 */ s8 row_0x247;             /* the cursor's row on the page */
    /* +0x248 */ u8 unused_0x248;
    /* +0x249 */ s8 index_0x249;           /* the selected entry: row + page * 4 */
    /* +0x24A */ s16 page_0x24A;           /* the page of four entries the list shows */
    /* +0x24C */ s16 field_0x24C;          /* set to 1 on entry */
    /* +0x24E */ s16 cursor_0x24E;         /* the detail page's tab */
    /* +0x250 */ s16 count_0x250;          /* the detail page's tab count */
    /* +0x252 */ s16 rows_0x252;           /* the list's row count */
    /* +0x254 */ s32 quest_id_0x254;       /* the selected entry's quest id */
    /* +0x258 */ s32 field_0x258;          /* the yes/no answer */
    /* +0x25C */ u16 page_moved_0x25C;     /* the page-step flags the arrows light up with */
    /* +0x25E */ u16 message_0x25E;        /* the help line of step 9 (0xFF on entry) */
    /* +0x260 */ u8 anim_0x260;            /* the selected row's frame, counted up to 8 */
    /* +0x261 */ u8 unused_0x261[0x3];
    /* +0x264 */ LbQuestBoardData* data_0x264;  /* the payload the screen was opened with */
    /* +0x268 */ u8 unused_0x268[0x3];
    /* +0x26B */ char name_0x26B[0x5];     /* the session name the keyboard set */
    /* +0x270 */ u8 unused_0x270[0x38C - 0x270];
    /* +0x38C */ u16 value_0x38C;
    /* +0x38E */ u8 unused_0x38E[0x2];
    /* +0x390 */ s32 field_0x390;
    /* +0x394 */ u8 unused_0x394;
    /* +0x395 */ u8 capacity_0x395;        /* the party's capacity */
    /* +0x396 */ u8 active_0x396;          /* the party's active members */
    /* +0x397 */ u8 members_0x397;         /* the party's members */
    /* +0x398 */ char kbd_buf_0x398[0xA];  /* the name keyboard's buffer */
    /* +0x3A2 */ u8 renamed_0x3A2;         /* the session takes the keyboard's name */
    /* +0x3A3 */ u8 not_last_0x3A3;        /* the player is not the party's last member */
    /* +0x3A4 */ s8 wait_0x3A4;            /* frames before the list accepts the next press */
    /* +0x3A5 */ u8 unused_0x3A5[0x2000 - 0x3A5];
} LbQuestBoardWork; /* size: 0x2000 */

/* The `.bss` lobby work block (`lobby_w`, 0x806AAB44, 0x17C B in the map) as this unit reads it:
 * the screen selector, the screen work block and the two bytes the open/close helpers toggle. */
typedef struct LbQuestBoardLobby {
    /* +0x000 */ u8 state_0x000;
    /* +0x001 */ u8 unused_0x001[0x002];
    /* +0x003 */ u8 camera_mode_0x003;    /* 2 once the lobby camera has settled on the board */
    /* +0x004 */ u8 unused_0x004[0x004];
    /* +0x008 */ u8 active_0x008;
    /* +0x009 */ u8 unused_0x009[0x049];
    /* +0x052 */ u8 flags_0x052;
    /* +0x053 */ u8 unused_0x053[0x2F];
    /* +0x082 */ s16 value_0x082;
    /* +0x084 */ u8 unused_0x084[0x028];
    /* +0x0AC */ LbQuestBoardWork* menu_0xAC;
} LbQuestBoardLobby; /* size: 0x17C */

extern "C" {
extern LbQuestBoardLobby lobby_w;   /* .bss 0x806AAB44 */

/* Other units' callees. */
void userdata_hunter_rank_info_get(void* work, u32* out_a, u32* out_b);
void sysSE_stop(u32 id);
void eft_res_slot_release(void* work);
void fn_800F8A44(void* list, s32 count);
void quest_board_flags_send(u8 a, u8 b);
}

/* Mangled callees, declared through their owner (rule 9); each spelling was confirmed with
 * `tools/units/mangle.py` against the map's own symbol. */
void set_zmode(bool depth_test, u8 arg1, bool arg2);      /* set_zmode__FbUcb */
void set_blendmode(u8 src, u8 dst, u8 op);                /* set_blendmode__FUcUcUc */
void sysSE_req(long id);                                  /* sysSE_req__Fl */

/* 0x80395C84 - resets the quest board: the prototype is the leaf header's (`lb_quest_board_reset` has C linkage). */
#include "lobby/lb_quest_board_reset.h"

#endif /* MHTRI_LOBBY_LB_QUEST_BOARD_H */
