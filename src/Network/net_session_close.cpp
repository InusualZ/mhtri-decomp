/*
 * Network/net_session_close.cpp - the session-manager half of the network control: the session reflect
 * callback and its command results, the session join / close / abort requests, the pending-action
 * handlers, the friend-roster sync and the community callback (`.text` 0x80432104..0x80437270; 88
 * functions).  Every function works on the `net_ctrl_wk` record `Network/network_pat_control.cpp` owns.
 *
 * Sections: extab 0x8001D824..0x8001DA94; extabindex 0x8003E214..0x8003E514; .text 0x80432104..0x80437270;
 * .ctors 0x8056F3C4..0x8056F3C8; .data 0x80603D70..0x80603E90; .bss 0x806E1770..0x806E1B38.
 *
 * Name: no `__FILE__` string covers the range; the left edge is the end of the `Network/network_pat_control`
 * fold, the right edge the end of the `.ctors` initialiser's closure (0x80431CD8).  Function names are GUESSES
 * from the bodies (the session command each one issues or reads back).
 *
 * Lib/flags: `cflags_main` (the link neighbour's group); `#pragma peephole off` for the whole file, as in
 * the neighbour (measured on the first bodies: the kept `extsb`/`cmpwi` pairs need it).
 *
 * Residuals (2026-10-04: 58 of 88 rows at 100 %, report metric 80.6 %):
 *  - Not written: the circle-list copies (`onCircleListReceived`, `copyCircleToProfile`: the profile record's +0x1C small object
 *    and +0x44 record block need the small network object as a C++ class); the roster requests
 *    0x80435740..0x80435E34 and `flushRosterSync` (the NetworkCommunityPat slots +0x4C/+0x50/+0x60 and NetworkLayerPat
 *    +0xA0 they dispatch are now declared: `writeProfile_4C`/`writeProfileRange_50`/`request_60`/`setPresence_A0`); `fn_80435E34`/`fn_80435F48` also need
 *    the 0x20-byte address object at 0x806E1B18 and NetworkCommunityPat's 0x803F0F20); `fn_80436220` (192 B: it calls NetworkCommunityPat's slot +0x48 and its 0x803F116C, neither declared yet);
 *    the static initialiser 0x80437204 (`net_community_state` is built with the NetworkCommunityPat band's
 *    constructor 0x803F0730 / destructor 0x803F0578, and the 0x20-byte address object at 0x806E1B18 with the
 *    small object's; both classes belong to that band, so `net_community_state` is a plain object here).
 *  - `communityReflectCallback` 97.7 %: the roster/recent copies keep three induction pointers where retail keeps
 *    four; the mail text address is CSE'd as in the session callback (request #22).  The address object's +0x28
 *    copy is the sink's real virtual `copyFrom` (request #21: `appendRosterEntry`/`appendRecentEntry` 99.57 -> 100,
 *    `removeRosterEntry`/`removeRecentEntry` 97.54 -> 97.80).
 *  - `sessionReflectCallback` 97.7 %: the case-29 message switch lowers to a binary compare tree where retail
 *    tests `(u32)(kind - 1) <= 3` linearly; the chat-record text address (+0x35) is CSE'd into r27 where retail
 *    recomputes it; the peer offset (index * 0x120) is recomputed for the chat-log call in cases 7 and 27; the
 *    server-slot copy keeps an `extsb` before its `stb` (cases 24/25).  Each session-request completion is
 *    spelled with a block-local `req` pointer: an inline helper (or `work->session_request_0x829C.` directly)
 *    rematerialises the `addis` base after the call (-4 points).
 *  - `isSameNetId`, `isSessionMember`/`isSessionMemberId`/`findSessionMemberSlot`/`lookupFriendSlot` reach 100 % with
 *    `NetworkSmallObject` at its retail 0x20 (the stack object's frame size).
 *  - Partial: `installCommunityCallback` 88.4 % (retail strides the three result records from the work pointer;
 *    the 2-D index folds the offsets), `onSessionCloseDone` 96.9 % (the member loop's registers; the member's
 *    +0x20 id is handed to `getPlayerRecord` as the small network object it copies into),
 *    `getProfileMemberCounts` 99.6 %, `buildRosterSync` 99.3 % (one register swap), `getProfileQuestRecord` 98.6 %.
 *  - GUESS names added 2026-10-04: `setMoveWorkMemberState` (writes 4 when a member drops), `sendBoxPageCheckRequest`
 *    (its record is packed by the item-box page exporter), the community helpers `getCommunityUserProfile`/
 *    `getCommunityStateBlock`/`getCommunityMemberRecord` (offsets into the state block), `requestCommunityNews`/
 *    `requestCommunityBlockList`/`requestFriendSync` (the community call each issues), the roster/recent helpers
 *    (what each does to the list), `net_community_state` (the block community command 17 delivers), the member
 *    lookups `isSessionMember`/`isSessionMemberId`/`findSessionMemberSlot` (they walk the four session players)
 *    and `postQuestBoardRecord` (its one caller is the quest board).
 */

#include "Network/net_session_close.h"
#include "Network/network_pat_control.h"
#include "Network/NetworkSessionManager.h"   /* NetworkSessionManagerPat - the class the session calls dispatch through */
#include "lobby/lobby_w.h"                    /* lobby_w - owner lobby/lb_menu_pos_tbl.cpp */
#include "fn_80047398/lobby_world_block.h"    /* lobby_world_block - owner fn_80047398.cpp */
#include "sound/fn_800E46E8.h"                /* getInstance - owner sound/fn_800E46E8.cpp */

#include "Network/NetworkWiiMediator.h"         /* postMediatorRecord - owner Network/NetworkWiiMediator.cpp */
#include "menu/get_pop_dat_ptr.h"               /* ck_option_cfg - owner menu/get_pop_dat_ptr.cpp */
#include "userdata_item.h"                      /* buildNetUserProfile - owner userdata_item.cpp */
#include "Network/NetworkCommunityPat.h"        /* NetworkCommunityPat - the community layer's reflect slot */
#include "Network/NetworkUniqueId.h"            /* the address-object helpers - owner Network/NetworkUniqueId.cpp */
#include "MSL/strlen.h"                        /* strlen - owner MSL/strlen.cpp */
#include "enemy/em020_ai.h"                     /* getInstance_ - owner enemy/em020_ai.cpp */
#include "Network/NetworkPeerBase.h"            /* NetworkSmallObjectSink::destroy - owner Network/NetworkPeerBase.cpp */
#include "ef/get_move_work_adrs.h"              /* get_move_work_adrs - owner ef/fn_800CDB2C.cpp */
#include "lobby/lb_menu_pos_tbl.h"              /* getItemListSelection - owner lobby/lb_menu_pos_tbl.cpp */
#include "hud/net_char_sync.h"                  /* Pl_net_recv, em_net_recv, emc_net_recv - owner hud/pl_frame_sync.cpp */
#include "sound/fn_800D7F54.h"                  /* sysSE_stop - owner sound/fn_800D7F54.cpp (needs nw4r/math.h first) */
#include "enemy/em020_prog.h"   /* sendMemberJoinNotice, dropLobbyMail, addFriendNotice */
#include "enemy/postChatLogLine.h"   /* postChatLogLine - owner enemy/em_prog_support.cpp (its leaf header) */
#include "lobby/lb_npc.h"   /* lb_party_state_reset */
#include "lobby/lb_act_dispatch.h"   /* lb_act_dispatch, lb_act_dispatch_ex - owner lobby/lb_companion_ui.cpp (its leaf header) */
#include "Network/network_state.h"   /* sendReqUnknownCheck */
#include "fn_8004CAD8.h"   /* exportItemBoxPage, exportEquipRecord */
#include "hud/cockpit.h"   /* cockpitShowNewMail */
#include "menu/menu_plsearch.h"   /* getLobbyMailBox */

#pragma peephole off

/* The kind-0 move work, seen only as the four per-member link-state bytes `setMoveWorkMemberState` writes.
 * size: 0x22E1 (approximate: the highest byte written here) */
typedef struct NetMoveWorkView {
    /* +0x0000 */ u8 pad_0x0000[0x22DD];
    /* +0x22DD */ u8 member_state_0x22DD[4];
} NetMoveWorkView; /* size: 0x22E1 (approximate) */

/* The peer-card update community commands 11 and 13 deliver: the peer's id, the offset and size of the changed
 * run (command 13) and the card bytes.  size: 0x128 (approximate: command 11 copies 0x100 bytes from +0x28) */
typedef struct NetPeerCardUpdate {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x16];
    /* +0x20 */ s32 offset_0x20;
    /* +0x24 */ u32 size_0x24;
    /* +0x28 */ u8 blob_0x28[0x100];
} NetPeerCardUpdate; /* size: 0x128 (approximate) */

/* One 0xE0-byte mail of the lobby's mailbox: the sender's name and id text, whether it came from a friend, and
 * the text (0xC1 bytes, terminated). */
typedef struct NetMailEntry {
    /* +0x00 */ char sender_name_0x00[0x14];
    /* +0x14 */ char sender_id_0x14[0xA];
    /* +0x1E */ u8 from_friend_0x1E;
    /* +0x1F */ char text_0x1F[0xC1];
} NetMailEntry; /* size: 0xE0 */

/* The lobby's mailbox (the lobby state block +0x1FC4): the count, an unread byte per mail and sixteen mails.
 * size: 0xE14 */
typedef struct NetMailBox {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ u8 unread_0x004[0x10];
    /* +0x014 */ NetMailEntry entries_0x014[16];
} NetMailBox; /* size: 0xE14 */

/* This player's community profile and the presence record the layer is sent (`.bss` 0x806E1770 / 0x806E1870). */
NetUserProfile net_user_profile;
NetStatusRecord net_presence_record;
/* The community layer's state block (`.bss` 0x806E18A0). */
NetCommunityState net_community_state;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stores `state` as member `member`'s link state in the kind-0 move work, when there is one.
 */
void setMoveWorkMemberState(u8 member, u8 state)
{
    NetMoveWorkView* move = (NetMoveWorkView*)get_move_work_adrs(0);

    if (move != NULL) {
        move->member_state_0x22DD[member] = state;
    }
}

/*
 * Copies the server-slot list a ready request returned and notes whether this player's slot is in it; the
 * slots past `count` are cleared to -1.
 */
static inline void copyServerIndexes(NetCtrlWk* work, const u8* slots, s32 count)
{
    s32 i;

    work->field_0x070 = -1;
    for (i = 0; i < count; i++) {
        if ((s8)*slots == work->selected_server_0x06A) {
            work->field_0x070 = 1;
        }
        work->server_index_0x074[i] = *slots++;
    }
    for (; i < 4; i++) {
        work->server_index_0x074[i] = -1;
    }
}

/*
 * The session manager's reflect callback: records command `command`'s result word (and, for a failed join,
 * leave or kick, the error triple), then runs the command's own bookkeeping and the pending request's
 * completion.
 */
/* untyped: caller-owned payload - each session command delivers its own record */
s32 sessionReflectCallback(s32 command, s8 member, s32 result, s32 count, void* data)
{
    s32* slot;
    NetProfileRec* profile;
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* joined;
    NetMemberSlot* slot_member;
    NetCtrlEntry* entry;
    NetSessionRequest* req;
    s32 i;

    if (work->profile_index_0x170 >= 0) {
        profile = &work->profiles_0x168[work->profile_index_0x170];
    } else {
        profile = NULL;
    }
    if (result < 0 && data != NULL && (command == 28 || command == 35 || command == 12)) {
        work->session_error_0x140 = *(NetErrorTriple*)data;
    }
    if (result < 0) {
        slot = &work->results_0x078[command];
        *slot = result;
    } else {
        slot = &work->results_0x078[command];
        *slot = 1;
    }
    switch (command) {
    case 1:
        if (result < 0) {
            *slot = result;
            work->error_0x050 = result;
            setErrorCode(4);
        } else {
            *slot = 1;
            work->entered_0x014 = 1;
        }
        break;
    case 3:
        work->error_0x060 = result;
        break;
    case 4:
        work->join_result_0x82C0 = *slot;
        if (*slot == 1) {
            if (data == NULL) {
                setErrorCode(5);
                break;
            }
            copyCircleToProfile(*(s32*)data, 0);
            lobby_w.item_list_selection_0x162 = getItemListSelection();
        }
        req = &work->session_request_0x829C;
        if (data != NULL && req->done_0x08 != NULL) {
            req->status_0x0C = 2;
            if (*slot == 1) {
                req->values_0x10[0] = member;
                req->values_0x10[1] = *(s32*)data;
            } else {
                req->values_0x10[0] = -1;
                req->values_0x10[1] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        break;
    case 5:
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = work->selected_server_0x06A;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        break;
    case 6:
        if (*slot == 1) {
            copyCircleToProfile(work->profile_index_0x170, 0);
        } else {
            work->sub_error_0xC259 = -1;
        }
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = member;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        break;
    case 7:
        if (work->profile_index_0x170 >= 0) {
            joined = &work->profiles_0x168[work->profile_index_0x170];
            if (joined != NULL) {
                char join_text[0x18];
                char line[0x80];
                s32 peer;

                slot_member = &joined->members_0x090[member];
                slot_member->joined_0x00 = 1;
                slot_member->flag_0x04 = 0;
                getNetworkSessionManagerPat(getPatsObject(), 0)->getPlayerRecord(
                    member, (NetworkSmallObject*)&slot_member->id_0x20);
                getNetworkSessionManagerPat(getPatsObject(), 0)->getPlayerRecordName(member, slot_member->name_0x0C,
                                                                                     16);
                work->server_slot_state_0x040[member] = 1;
                if (work->ready_count_0x044 != 0) {
                    formatNetId(join_text, &slot_member->id_0x20);
                    sendMemberJoinNotice(join_text);
                    work->circle_records_0xC3F4.region_0x80[member] =
                        work->peers_0x7488[findPeerIndex(&slot_member->id_0x20)].blob_0x20.region_0x05;
                    strcpy(work->circle_records_0xC3F4.id_text_0x00[member], join_text);
                    strcpy(work->circle_records_0xC3F4.name_0x40[member], slot_member->name_0x0C);
                    work->action_0xC152 = 2;
                }
                peer = findPeerIndex(&slot_member->id_0x20);
                if (get_option_cfg(12) == 0) {
                    sprintf(line, MH3GetErrorString2(41), work->peers_0x7488[peer].name_0x0A.text_0x00);
                } else {
                    sprintf(line, MH3GetErrorString2(41), work->peers_0x7488[peer].id_0x00.text_0x00);
                }
                postChatLogLine(1, 0, 0xFFFFF0, line, work->peers_0x7488[peer].id_0x00.text_0x00,
                            work->peers_0x7488[peer].name_0x0A.text_0x00);
            }
        }
        break;
    case 8:
        if (work != NULL) {
            work->field_0x070 = 0;
            work->flag_0xC379 = 0;
        }
        break;
    case 12:
        if (work->field_0x070 == 1) {
            setMoveWorkMemberState(member, 4);
            work->error_0x050 = result;
            work->server_slot_state_0x040[member] = 0;
        } else {
            if (result < 0 && data != NULL) {
                if (*(u32*)data == 0x80050036) {
                    work->sub_error_0xC259 = 0x12;
                } else {
                    work->sub_error_0xC259 = 0x13;
                    sysSE_stop(1);
                }
            }
            work->action_0xC152 = 6;
            work->flag_0xC151 = 0;
            work->server_slot_state_0x040[member] = 0;
        }
        break;
    case 13:
        setMoveWorkMemberState(member, 4);
        work->server_slot_state_0x040[member] = 0;
        if (work->field_0x070 != 1) {
            work->flag_0xC151 = 0;
        }
        break;
    case 36:
        setMoveWorkMemberState(member, 4);
        work->server_slot_state_0x040[member] = 0;
        if (work->field_0x070 != 1) {
            work->flag_0xC151 = 0;
        }
        break;
    case 14:
        work->flag_0x06B = member;
        if (work->selected_server_0x06A == member) {
            work->ready_count_0x044 = 1;
        }
        break;
    case 15:
        if (*slot == 1 && profile != NULL) {
            profile->flag_0x08D = *(u8*)data;
        }
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        if (*slot == 1 && work->ready_count_0x044 != 0) {
            work->action_0xC152 = 1;
        }
        break;
    case 16:
        joined = &work->profiles_0x168[work->profile_index_0x170];
        if (joined != NULL) {
            joined->members_0x090[member].joined_0x00 = 0;
            joined->members_0x090[member].flag_0x04 = 0;
            work->flag_0xC379 = 0;
        }
        work->profile_index_0x170 = -1;
        work->server_slot_state_0x040[member] = 0;
        work->selected_server_0x06A = -1;
        if (work->field_0x070 != 1) {
            lb_party_state_reset();
            work->flag_0xC151 = 0;
            work->sub_state_0x017 = 0x19;
        }
        work->action_0xC152 = 7;
        break;
    case 17:
        work->server_slot_state_0x040[member] = 0;
        joined = &work->profiles_0x168[work->profile_index_0x170];
        if (joined != NULL) {
            joined->members_0x090[member].joined_0x00 = 0;
            joined->members_0x090[member].flag_0x04 = 0;
        }
        if (work->field_0x070 != 1) {
            work->flag_0xC151 = 0;
            if (work->ready_count_0x044 != 0) {
                work->circle_records_0xC3F4.id_text_0x00[member][0] = 0;
                work->circle_records_0xC3F4.name_0x40[member][0] = 0;
                work->action_0xC152 = 2;
            }
        } else {
            setMoveWorkMemberState(member, 4);
        }
        break;
    case 18:
        if (result >= 0) {
            char id_text[0x14];

            formatNetId(id_text, &((NetSessionChatRecord*)data)->sender_0x000);
            postMediatorRecord(getInstance(), ((NetSessionChatRecord*)data)->text_0x035);
            postChatLogLine(0, member, ((NetSessionChatRecord*)data)->color_0x238,
                        (const char*)((NetSessionChatRecord*)data)->text_0x035, id_text,
                        ((NetSessionChatRecord*)data)->name_0x020);
        }
        entry = work->entries_0x7CD8;
        for (i = 0; i < 16; i++, entry++) {
            if (entry->in_use_0x00 == 2) {
                entry->in_use_0x00 = 0;
                break;
            }
        }
        break;
    case 19:
        if (work->flag_0xC48A != 1) {
            char id_text[0x14];

            formatNetId(id_text, &((NetSessionChatRecord*)data)->sender_0x000);
            postMediatorRecord(getInstance(), ((NetSessionChatRecord*)data)->text_0x035);
            postChatLogLine(0, member, ((NetSessionChatRecord*)data)->color_0x238,
                        (const char*)((NetSessionChatRecord*)data)->text_0x035, id_text,
                        ((NetSessionChatRecord*)data)->name_0x020);
        }
        break;
    case 20:
        if (*slot == 1 && profile != NULL) {
            profile->members_0x090[member].value_0x08 = *(s32*)data;
        }
        break;
    case 21:
        if (profile != NULL) {
            profile->members_0x090[member].value_0x08 = *(s32*)data;
        }
        break;
    case 24:
        if (result < 0) {
            work->field_0x070 = result;
        } else {
            copyServerIndexes(work, (const u8*)data, count);
        }
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        if (work->field_0x070 == 1) {
            if (work->ready_count_0x044 != 0) {
                work->action_0xC152 = 3;
            }
            work->sub_state_0x017 = 0x1F;
            work->step_0x018 = 0;
            resumeSoundEngine();
        }
        break;
    case 25:
        if (result < 0) {
            work->field_0x070 = result;
        } else {
            copyServerIndexes(work, (const u8*)data, count);
            if (work->field_0x070 != 1) {
                work->action_0xC152 = 6;
                work->flag_0xC151 = 0;
                work->sub_error_0xC259 = 0x12;
                break;
            }
        }
        onAction1Done(0, NULL);
        if (work->field_0x070 == 1) {
            if (work->ready_count_0x044 == 0) {
                work->action_0xC152 = 3;
            }
            work->sub_state_0x017 = 0x1F;
            work->step_0x018 = 0;
            resumeSoundEngine();
        }
        break;
    case 26:
        if (*slot == 1 && profile != NULL) {
            profile->members_0x090[work->selected_server_0x06A].flag_0x04 = *(u8*)data;
        }
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 2;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->values_0x10[1] = *(u8*)data;
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        if (*slot == 1 && profile != NULL) {
            slot_member = &profile->members_0x090[work->selected_server_0x06A];
            if (slot_member->flag_0x04 == 1) {
                work->flag_0xC379 = 1;
                work->server_slot_state_0x040[work->selected_server_0x06A] = 2;
            } else {
                work->flag_0xC379 = 0;
                work->server_slot_state_0x040[work->selected_server_0x06A] = 1;
            }
            if (slot_member->flag_0x04 == 1 && work->ready_count_0x044 != 0) {
                work->action_0xC152 = 5;
            }
        }
        break;
    case 27:
        if (profile != NULL) {
            slot_member = &profile->members_0x090[member];
            slot_member->flag_0x04 = *(u8*)data;
            work->flag_0xC151 = 0;
            if (slot_member->flag_0x04 == 1) {
                if (work->flag_0x06B != member) {
                    sysSE_stop(30);
                }
                work->server_slot_state_0x040[member] = 2;
            } else {
                work->server_slot_state_0x040[member] = 1;
            }
            if (work->flag_0x06B != member) {
                s32 peer = findPeerIndex(&slot_member->id_0x20);

                if (strcmp(work->peers_0x7488[peer].id_0x00.text_0x00, work->name_0x7368) != 0 &&
                    work->peers_0x7488[peer].blob_0x20.in_party_0xF9 == 0) {
                    char line[0x80];

                    if (slot_member->flag_0x04 == 1) {
                        if (get_option_cfg(12) == 0) {
                            sprintf(line, MH3GetErrorString2(35), work->peers_0x7488[peer].name_0x0A.text_0x00);
                        } else {
                            sprintf(line, MH3GetErrorString2(35), work->peers_0x7488[peer].id_0x00.text_0x00);
                        }
                    } else if (get_option_cfg(12) == 0) {
                        sprintf(line, MH3GetErrorString2(36), work->peers_0x7488[peer].name_0x0A.text_0x00);
                    } else {
                        sprintf(line, MH3GetErrorString2(36), work->peers_0x7488[peer].id_0x00.text_0x00);
                    }
                    postChatLogLine(1, 0, 0xFFFFF0, line, work->peers_0x7488[peer].id_0x00.text_0x00,
                                work->peers_0x7488[peer].name_0x0A.text_0x00);
                }
            }
        }
        break;
    case 28:
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        break;
    case 29:
        switch (((NetMsgHeader*)data)->to_slot) {
        case 1:
        case 2:
        case 3:
        case 4:
            Pl_net_recv(member, (NetMsgHeader*)data);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            em_net_recv(member, (NetEmStateMsg*)data);
            break;
        case 9:
            emc_net_recv(member, (NetMsgHeader*)data);
            break;
        case 10:
            lb_act_dispatch(member, (struct LbActReq*)data);
            break;
        case 13:
            lb_act_dispatch_ex(member, (NetMsgHeader*)data);
            break;
        }
        break;
    case 30:
        if (result < 0) {
            work->leave_0x048 = result;
        } else {
            work->leave_0x048 = 1;
        }
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        work->action_0xC152 = 8;
        break;
    case 31:
        if (result < 0) {
            work->leave_0x048 = result;
        } else {
            work->leave_0x048 = 1;
        }
        onLeaveDone(0, NULL);
        break;
    case 32:
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        work->action_0xC152 = 6;
        break;
    case 35:
        work->error_0x050 = result;
        work->server_slot_state_0x040[member] = 0;
        work->sub_error_0xC259 = 2;
        break;
    case 39:
        req = &work->session_request_0x829C;
        if (req->done_0x08 != NULL) {
            req->status_0x0C = 1;
            if (*slot == 1) {
                req->values_0x10[0] = 0;
            } else {
                req->values_0x10[0] = -1;
            }
            req->done_0x08(req->status_0x0C, req->values_0x10);
            req->busy_0x00 = 0;
            req->done_0x08 = NULL;
        }
        break;
    case 40:
        if (work->sub_state_0x017 == 0x19) {
            sysSE_stop(0);
        }
        if (work->flag_0x82C4 == 2) {
            copyCircleToProfile(*(s32*)data, 0);
        }
        break;
    case 41:
        if (work->flag_0x82C4 == 2) {
            copyCircleToProfile(*(s32*)data, 0);
        }
        break;
    case 42:
        if (work->flag_0x82C4 == 2) {
            copyCircleToProfile(*(s32*)data, 1);
        }
        break;
    }
    return 0;
}

/*
 * Clears every session command result and hands the session manager its reflect callback; marks the session
 * callback installed.
 */
void installSessionCallback(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 i;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
        for (i = 0; i < 44; i++) {
            work->results_0x078[i] = 0;
        }
        getNetworkSessionManagerPat(getPatsObject(), 0)->init((u32)sessionReflectCallback, 0);
        work->state_0x011 = 1;
        work->flag_0xC151 = 0;
    }
}

/*
 * Clears the result word of session command `slot`.
 */
void clearPhaseSlot(s32 slot)
{
    net_ctrl_wk->results_0x078[slot] = 0;
}

/*
 * The result word of session command `slot` (0 while it is still pending).
 */
u32 isPhaseSlotDone(s32 slot)
{
    return net_ctrl_wk->results_0x078[slot];
}

/*
 * Clears every session command result, marks the session callback installed and clears the link error.
 */
void resetLinkState(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 i;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
        for (i = 0; i < 44; i++) {
            work->results_0x078[i] = 0;
        }
        work->state_0x011 = 1;
        work->error_0x050 = 0;
        work->flag_0xC151 = 0;
    }
}

/*
 * The join request's result: -1 without a session manager or on failure, 1 once it succeeded, 0 while pending.
 */
s32 getSessionJoinResult(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return -1;
    }
    if (work->results_0x078[4] < 0) {
        return -1;
    }
    return work->results_0x078[4] != 0;
}

/*
 * The join request's completion: records the joined server and profile, resets the profile's member slots
 * and marks this player's own slot joined, then moves the control on to state 0x1D.
 */
void onSessionJoined(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* profile;
    NetMemberSlot* member;
    s32 i;

    work->session_request_0x829C.pending_0x20 = 0;
    work->selected_server_0x06A = values[0];
    if (work->selected_server_0x06A < 0) {
        work->session_request_0x829C.ok_0x21 = 0;
    } else {
        work->session_request_0x829C.ok_0x21 = 1;
    }
    for (i = 0; i < 4; i++) {
        work->server_slot_state_0x040[i] = 0;
    }
    if (work->session_request_0x829C.ok_0x21 != 1) {
        return;
    }
    work->profile_index_0x170 = values[1];
    if (work->profile_index_0x170 >= 0) {
        profile = &work->profiles_0x168[work->profile_index_0x170];
        if (profile != NULL) {
            for (i = 0; i < 4; i++) {
                profile->members_0x090[i].joined_0x00 = 0;
                profile->members_0x090[i].flag_0x04 = 0;
                work->server_slot_state_0x040[i] = 0;
            }
            member = &profile->members_0x090[work->selected_server_0x06A];
            member->joined_0x00 = 1;
            member->flag_0x04 = 0;
            work->flag_0xC379 = 0;
            work->server_slot_state_0x040[work->selected_server_0x06A] = 1;
        }
    }
    work->ready_count_0x044 = 1;
    work->sub_state_0x017 = 0x1D;
    work->step_0x018 = 0;
}

/*
 * Requests the session join (session command 4): publishes the party size and quest as the name list, the
 * session name and comment, arms circle mode 1 and publishes this player's circle record; 0 when there is no
 * session manager or a request is busy.
 */
s32 requestSessionJoin(const NetJoinParams* params)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetworkNameList names;

    work->session_request_0x829C.ok_0x21 = 0;
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.pending_0x20 = 1;
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 4;
    work->session_request_0x829C.done_0x08 = onSessionJoined;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(4);
    clearPhaseSlot(0x1A);
    names.count_04 = 2;
    names.entries_08[0].enabled_00 = 1;
    names.entries_08[0].value_04 = params->party_size_0x02;
    names.entries_08[1].enabled_00 = 1;
    names.entries_08[1].value_04 = params->quest_0x00;
    getNetworkSessionManagerPat(getPatsObject(), 0)->copyNameList(&names);
    if (strlen(params->session_name_0x03) != 0) {
        getNetworkSessionManagerPat(getPatsObject(), 0)->setSessionName(params->session_name_0x03);
    }
    getNetworkSessionManagerPat(getPatsObject(), 0)->setCircleComment(params->comment_0x08);
    getNetworkSessionManagerPat(getPatsObject(), 0)->setCircleMode(1);
    memset(&work->circle_records_0xC3F4, 0, sizeof(NetCircleRecords));
    strcpy(work->circle_records_0xC3F4.id_text_0x00[0], work->name_0x7368);
    strcpy(work->circle_records_0xC3F4.name_0x40[0], work->name2_0x7372);
    work->circle_records_0xC3F4.region_0x80[0] = getUserRegion();
    getNetworkSessionManagerPat(getPatsObject(), 0)->setCircleRecords((const u8*)&work->circle_records_0xC3F4,
                                                                      sizeof(NetCircleRecords));
    getNetworkSessionManagerPat(getPatsObject(), 0)->request372(params->party_size_0x02, 0);
    return 1;
}

/*
 * Whether this player holds a server slot (the joined slot index is not -1); 0 without a session manager.
 */
BOOL hasJoinedServer(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return FALSE;
    }
    return work->selected_server_0x06A != -1;
}

/*
 * The link state: 0 without a work record, 1 after a link error, else whether the link word is 1.
 */
u32 getLinkStatus(void)
{
    if (net_ctrl_wk == NULL) {
        return 0;
    }
    if (net_ctrl_wk->error_0x050 < 0) {
        return 1;
    }
    return net_ctrl_wk->flag_0x04C == 1;
}

/*
 * Hands the mediator a quest-board record.
 */
void postQuestBoardRecord(u8* record)
{
    postMediatorRecord(getInstance(), record);
}

/*
 * The quest record of profile `index` (at +0x190), or NULL while the profile is not active.
 */
u16* getProfileQuestRecord(s32 index)
{
    NetProfileRec* profiles = net_ctrl_wk->profiles_0x168;

    if (profiles[index].active_0x040 == 0) {
        return NULL;
    }
    return &profiles[index].quest_id_0x190;
}

/*
 * The session manager's circle list.
 */
NetworkSessionCircleList* getCircleList(NetworkSessionManagerPat* manager)
{
    return &manager->circleList_AF0;
}

/*
 * Requests the circle list (session command 39); 0 when there is no session manager or a request is busy.
 */
s32 requestCircleList(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 39;
    work->session_request_0x829C.done_0x08 = onCircleListReceived;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(39);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request380(work->profile_count_0x164);
    return 1;
}

/*
 * The party-enter request's result (command 6): -1 without a session manager or on failure, 1 once done.
 */
s32 getProfileEnterResult(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return -1;
    }
    if (work->results_0x078[6] < 0) {
        return -1;
    }
    return work->results_0x078[6] != 0;
}

/*
 * The party-enter completion: records the slot this player was given, resets the party's member slots and
 * marks the own slot joined, then moves the control on to state 0x1D.
 */
void onProfileEntered(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* profile;
    s32 i;

    work->selected_server_0x06A = values[0];
    if (work->selected_server_0x06A < 0) {
        return;
    }
    if (work->profile_index_0x170 >= 0) {
        profile = &work->profiles_0x168[work->profile_index_0x170];
        if (profile != NULL) {
            for (i = 0; i < 4; i++) {
                profile->members_0x090[i].joined_0x00 = 0;
                profile->members_0x090[i].flag_0x04 = 0;
                work->server_slot_state_0x040[i] = 0;
            }
            profile->members_0x090[work->selected_server_0x06A].joined_0x00 = 1;
            work->flag_0xC379 = 0;
            work->server_slot_state_0x040[work->selected_server_0x06A] = 1;
        }
    }
    work->ready_count_0x044 = 0;
    work->state_0x011 = 4;
    work->sub_state_0x017 = 0x1D;
    work->step_0x018 = 0;
}

/*
 * Requests entry into party `index` (session command 6); 0 when there is no session manager or a request
 * is busy.
 */
s32 requestProfileEnter(s32 index)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 6;
    work->session_request_0x829C.done_0x08 = onProfileEntered;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    work->server_slot_state_0x040[0] = 0;
    work->server_slot_state_0x040[1] = 0;
    work->server_slot_state_0x040[2] = 0;
    work->server_slot_state_0x040[3] = 0;
    work->profile_index_0x170 = index;
    clearPhaseSlot(6);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request384(index);
    return 1;
}

/*
 * Hands the session manager the name to publish.
 */
void setSessionDisplayName(const char* name)
{
    getNetworkSessionManagerPat(getPatsObject(), 0)->setSessionName((char*)name);
}

/*
 * The ready request's completion (command 0x1A): state 4 / 0x1D when the ready flag came back clear, else
 * state 5 / 0x1E.
 */
void onReadyChanged(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (values[0] >= 0) {
        if (values[1] == 0) {
            work->state_0x011 = 4;
            work->sub_state_0x017 = 0x1D;
        } else {
            work->state_0x011 = 5;
            work->sub_state_0x017 = 0x1E;
        }
        work->step_0x018 = 0;
    }
}

/*
 * Requests the ready state on (session command 0x1A); clears the depending results when the player is
 * already counted ready.
 */
s32 requestReadyOn(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x1A;
    work->session_request_0x829C.done_0x08 = onReadyChanged;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x1A);
    if (work->ready_count_0x044 != 0) {
        clearPhaseSlot(0x18);
        clearPhaseSlot(0x0F);
        clearPhaseSlot(0x1C);
    }
    getNetworkSessionManagerPat(getPatsObject(), 0)->request428(1);
    return 1;
}

/*
 * Requests the ready state off (session command 0x1A); refused while the player is counted ready.
 */
s32 requestReadyOff(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->ready_count_0x044 != 0) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x1A;
    work->session_request_0x829C.done_0x08 = onReadyChanged;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x1A);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request428(0);
    return 1;
}

/*
 * The quest board's ready-on entry points (two call sites) and its ready-off entry point.
 */
void requestReadyOnAlias(void)
{
    requestReadyOn();
}

void requestReadyOnAlias2(void)
{
    requestReadyOn();
}

void requestReadyOffAlias(void)
{
    requestReadyOff();
}

/*
 * The leave request's completion (command 0x0F): a failure raises network error 5.
 */
void onLeaveRequestDone(s32 status, s32* values)
{
    if (values[0] < 0) {
        setErrorCode(5);
    }
}

/*
 * Pending action 5: requests the leave (session command 0x0F).
 */
s32 runPendingAction5(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x0F;
    work->session_request_0x829C.done_0x08 = onLeaveRequestDone;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x0F);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request404(1);
    return 1;
}

/*
 * The session close's completion (command 0x1C): on failure it queues action 6 with sub-error 2; else it
 * fetches the player record of every joined member and flags the close done.
 */
void onSessionCloseDone(s32 status, s32* values)
{
    NetMemberSlot* member;
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* profiles = work->profiles_0x168;
    NetProfileRec* profile;
    s8 i;

    if (values[0] < 0) {
        work->sub_error_0xC259 = 2;
        work->action_0xC152 = 6;
        work->flag_0xC151 = 0;
        work->flag_0x04C = 1;
        return;
    }
    profile = &profiles[work->profile_index_0x170];
    if (profile != NULL) {
        member = profile->members_0x090;
        for (i = 0; i < 4; member++, i++) {
            if (member->joined_0x00 == 1) {
                getNetworkSessionManagerPat(getPatsObject(), 0)->getPlayerRecord(i, (NetworkSmallObject*)&member->id_0x20);
            }
        }
        work->start_done_0xC153 = 1;
    }
}

/*
 * Starts the network session's close-down (session command 0x1C): 0 when there is no session manager or
 * a request is busy, 1 once started.
 */
s32 net_session_close_start(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x1C;
    work->session_request_0x829C.done_0x08 = onSessionCloseDone;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x1C);
    getNetworkSessionManagerPat(getPatsObject(), 0)->setCircleMode(0);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request436();
    work->start_done_0xC153 = 0;
    return 1;
}

/*
 * Whether the session close's completion has run.
 */
u32 isSessionStartDone(void)
{
    return net_ctrl_wk->start_done_0xC153 != 0;
}

/*
 * Pending action 1's completion (command 0x18): clears the leave and link words when the abort flag is set.
 */
void onAction1Done(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work->field_0x070 == 1) {
        work->leave_0x048 = 0;
        work->flag_0x04C = 0;
    }
}

/*
 * Pending action 1: session command 0x18, only while the player is counted ready.
 */
s32 runPendingAction1(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->ready_count_0x044 != 1) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x18;
    work->session_request_0x829C.done_0x08 = onAction1Done;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x18);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request432();
    return 1;
}

/*
 * Pending action 6: session command 0x10 (no completion handler).
 */
s32 runPendingAction6(void)
{
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    clearPhaseSlot(0x10);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request408();
    return 1;
}

/*
 * Pending action 7: session command 8 (no completion handler).
 */
s32 runPendingAction7(void)
{
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    clearPhaseSlot(8);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request392();
    return 1;
}

/*
 * The close-down's progress: 1 when there is no session manager, -1 on error, 1 when done, else 0.
 */
s32 net_session_close_state_get(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 1;
    }
    if (work->results_0x078[8] < 0) {
        return -1;
    }
    return work->results_0x078[8] != 0;
}

/*
 * The leave request's completion (command 0x1E): with a leave pending, the control goes to state 9 / 0x22.
 */
void onLeaveDone(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work->leave_0x048 == 1) {
        work->state_0x011 = 9;
        work->sub_state_0x017 = 0x22;
        work->step_0x018 = 0;
    }
}

/*
 * Requests the leave (session command 0x1E), only while the player is counted ready.
 */
s32 requestLeave(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->ready_count_0x044 != 1) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x1E;
    work->session_request_0x829C.done_0x08 = onLeaveDone;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x1E);
    clearPhaseSlot(0x20);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request440();
    return 1;
}

/*
 * Leaves when the player is the one counted ready, else drops the session.
 */
void requestLeaveOrAbort(void)
{
    if (isReadyCountOne() == 0) {
        runPendingAction8();
    } else {
        requestLeave();
    }
}

/*
 * Drops the session after a link loss: clears the close result and raises pending action 8; 0 when there
 * is no work record.
 */
s32 net_session_abort_start(void)
{
    if (net_ctrl_wk == NULL) {
        return 0;
    }
    net_ctrl_wk->results_0x078[8] = 0;
    return runPendingAction8();
}

/*
 * The abort's completion (command 0x20): with the abort flag set, the control goes to state 0x0A / 0x22.
 */
void onAbortDone(s32 status, s32* values)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (work->field_0x070 == 1) {
        work->state_0x011 = 0x0A;
        work->sub_state_0x017 = 0x22;
        work->step_0x018 = 0;
    }
}

/*
 * Pending action 8: the abort (session command 0x20).
 */
s32 runPendingAction8(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->session_request_0x829C.busy_0x00 != 0) {
        return 0;
    }
    work->session_request_0x829C.busy_0x00 = 1;
    work->session_request_0x829C.command_0x04 = 0x20;
    work->session_request_0x829C.done_0x08 = onAbortDone;
    work->session_request_0x829C.status_0x0C = 0;
    work->session_request_0x829C.values_0x10[0] = 0;
    work->session_request_0x829C.values_0x10[1] = 0;
    work->session_request_0x829C.values_0x10[2] = 0;
    work->session_request_0x829C.values_0x10[3] = 0;
    clearPhaseSlot(0x20);
    getNetworkSessionManagerPat(getPatsObject(), 0)->request444();
    return 1;
}

/*
 * Whether a party is selected.
 */
BOOL hasSelectedProfile(void)
{
    return net_ctrl_wk->profile_index_0x170 != -1;
}

/*
 * Re-copies the ten circle records into the party table once the layer and the fetch are both ready (2), and
 * returns the table; NULL otherwise.
 */
NetProfileRec* refreshAllProfiles(void)
{
    s32 i;

    if (net_ctrl_wk == NULL) {
        return NULL;
    }
    if (net_ctrl_wk->layer_state_0x064 == 2 && net_ctrl_wk->flag_0x82C4 == 2) {
        for (i = 0; i < 10; i++) {
            copyCircleToProfile(i, 0);
        }
        return net_ctrl_wk->profiles_0x168;
    }
    return NULL;
}

/*
 * Reports the selected party's member count (joined and flagged), its active flag and its capacity.
 */
BOOL getProfileMemberCounts(u8* members, u8* active, u8* capacity)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetProfileRec* profile;
    s32 count;

    *members = 0;
    *active = 0;
    *capacity = 0;
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return FALSE;
    }
    if (work->ready_count_0x044 != 1) {
        return FALSE;
    }
    if (work->profile_index_0x170 < 0) {
        return FALSE;
    }
    profile = &work->profiles_0x168[work->profile_index_0x170];
    *capacity = profile->capacity_0x03C;
    *active = profile->active_0x040;
    count = 0;
    if (profile->members_0x090[0].joined_0x00 != 0 && profile->members_0x090[0].flag_0x04 != 0) {
        count = 1;
    }
    if (profile->members_0x090[1].joined_0x00 != 0 && profile->members_0x090[1].flag_0x04 != 0) {
        count++;
    }
    if (profile->members_0x090[2].joined_0x00 != 0 && profile->members_0x090[2].flag_0x04 != 0) {
        count++;
    }
    if (profile->members_0x090[3].joined_0x00 != 0 && profile->members_0x090[3].flag_0x04 != 0) {
        count++;
    }
    *members = count;
    work->flag_0xC151 = 1;
    return TRUE;
}

/*
 * Whether the party counts have been read since the last reset (only while the player is counted ready).
 */
BOOL isPartyCountRead(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return FALSE;
    }
    if (work->ready_count_0x044 != 1) {
        return FALSE;
    }
    return work->flag_0xC151 == 1;
}

/*
 * This player's own member flag in the selected party (0 while counted ready, unjoined or unselected).
 */
u8 getOwnMemberFlag(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetMemberSlot* member;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->ready_count_0x044 == 1) {
        return 0;
    }
    if (work->profile_index_0x170 < 0) {
        return 0;
    }
    member = &work->profiles_0x168[work->profile_index_0x170].members_0x090[work->selected_server_0x06A];
    if (member->joined_0x00 == 0) {
        return 0;
    }
    return member->flag_0x04;
}

/*
 * The selected party's flag byte at +0x8D (0 while counted ready or unselected).
 */
u8 getProfileFlag(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->ready_count_0x044 == 1) {
        return 0;
    }
    if (work->profile_index_0x170 < 0) {
        return 0;
    }
    return work->profiles_0x168[work->profile_index_0x170].flag_0x08D;
}

/*
 * Whether no party is selected (also when there is no session manager).
 */
u32 isProfileUnselected(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 1;
    }
    return (u32)work->profile_index_0x170 >> 31;
}

/*
 * The selected party's quest id (0 without a session manager or a selection).
 */
u16 getSelectedQuestId(void)
{
    NetCtrlWk* work = net_ctrl_wk;

    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        return 0;
    }
    if (work->profile_index_0x170 < 0) {
        return 0;
    }
    return work->profiles_0x168[work->profile_index_0x170].quest_0x058;
}

/*
 * Whether the address object `address` belongs to one of the four session members.
 */
s32 isSessionMember(const NetworkSmallObject* address)
{
    s32 found = 0;
    NetworkSmallObject member;
    s32 i;

    networkSmallObject_construct(&member);
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        NetworkSmallObjectSink::destroy(&member);
        return 0;
    }
    if (address == NULL) {
        NetworkSmallObjectSink::destroy(&member);
        return 0;
    }
    if (networkSmallObject_isValid(address) == 0) {
        NetworkSmallObjectSink::destroy(&member);
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (getNetworkSessionManagerPat(getPatsObject(), 0)->getPlayerRecord(i, &member) != 0 &&
            networkSmallObject_isEqual(address, &member) != 0) {
            found = 1;
            break;
        }
    }
    NetworkSmallObjectSink::destroy(&member);
    return found;
}

/*
 * Whether the six raw id bytes `id` belong to one of the session members.
 */
s32 isSessionMemberId(const NetId* id)
{
    NetworkSmallObject address;
    s32 found;

    networkSmallObject_construct(&address);
    importNetId((NetId*)&address, id);
    found = isSessionMember(&address);
    NetworkSmallObjectSink::destroy(&address);
    return found;
}

/*
 * The session slot (0..3) of the member whose address object is `address`, or -1.
 */
s8 findSessionMemberSlot(const NetworkSmallObject* address)
{
    NetworkSmallObject member;
    s8 i;

    networkSmallObject_construct(&member);
    if (getNetworkSessionManagerPat(getPatsObject(), 0) == NULL) {
        NetworkSmallObjectSink::destroy(&member);
        return -1;
    }
    if (address == NULL) {
        NetworkSmallObjectSink::destroy(&member);
        return -1;
    }
    if (networkSmallObject_isValid(address) == 0) {
        NetworkSmallObjectSink::destroy(&member);
        return -1;
    }
    for (i = 0; i < 4; i++) {
        if (getNetworkSessionManagerPat(getPatsObject(), 0)->getPlayerRecord(i, &member) != 0 &&
            networkSmallObject_isEqual(address, &member) != 0) {
            NetworkSmallObjectSink::destroy(&member);
            return i;
        }
    }
    NetworkSmallObjectSink::destroy(&member);
    return -1;
}

/*
 * The session slot of the member whose six raw id bytes are `id`, or -1.
 */
s8 lookupFriendSlot(const u8* id)
{
    NetworkSmallObject address;
    s8 slot;

    networkSmallObject_construct(&address);
    importNetId((NetId*)&address, (const NetId*)id);
    slot = findSessionMemberSlot(&address);
    NetworkSmallObjectSink::destroy(&address);
    return slot;
}

#ifdef __cplusplus
}
#endif

/*
 * The message id of the session layer's last error: the server's own message for the detail codes 50..75 and 95,
 * else the message mapped from the error code (11625, the generic failure, when none is).
 */
s32 MH3GetSessionErrorCode(void)
{
    NetErrorTriple* error = &net_ctrl_wk->session_error_0x140;

    if (error != NULL) {
        if ((u32)(error->detail_0x4 - 50) <= 25 && error->reason_0x8 != 0x80000000) {
            return error->reason_0x8;
        }
        if (error->detail_0x4 == 95) {
            return error->reason_0x8;
        }
        switch (error->code_0x0) {
        case 0x80000002:
            return 11620;
        case 0x80050012:
            if (error->detail_0x4 == 0x70) {
                return 11621;
            }
            break;
        case 0x80050011:
            if (error->detail_0x4 == 0x70) {
                return 11622;
            }
            break;
        case 0x80050037:
            return 11623;
        case 0x80020001:
            return 11624;
        case 0x80000008:
        case 0x80050000:
            return 11625;
        case 0x80030000:
            return 11630;
        case 0x80030001:
            return 11631;
        case 0x80030002:
            return 11632;
        case 0x80030003:
            return 11633;
        case 0x80030004:
            return 11634;
        case 0x80030005:
            return 11635;
        case 0x80030011:
            return 11640;
        case 0x80030012:
            return 11641;
        case 0x80030013:
            return 11642;
        case 0x80030014:
            return 11643;
        case 0x80030015:
            return 11644;
        case 0x80030021:
            return 11650;
        case 0x80030022:
            return 11651;
        case 0x80030031:
            return 11660;
        case 0x80030032:
            return 11661;
        case 0x80030033:
            return 11662;
        case 0x80030034:
            return 11663;
        case 0x80030035:
            return 11664;
        case 0x80030036:
            return 11665;
        case 0x80030037:
            return 11666;
        case 0x80030038:
            return 11667;
        case 0x80030039:
            return 11668;
        case 0x8003003A:
            return 11669;
        case 0x8003003B:
            return 11670;
        case 0x8003003C:
            return 11671;
        case 0x8003003D:
            return 11672;
        case 0x8003003E:
            return 11673;
        case 0x8003003F:
            return 11674;
        case 0x80030040:
            return 11675;
        case 0x80030041:
            return 11676;
        case 0x80030042:
            return 11677;
        case 0x80030043:
            return 11678;
        case 0x80030044:
            return 11679;
        }
    }
    return 11625;
}

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This player's region byte from the community profile.
 */
u8 getUserRegion(void)
{
    return net_user_profile.region_0x05;
}

/*
 * Fills the roster-sync block with this player's presence: in town (1), away or offline (2), in a party (3)
 * or hiding by option (4), per presence key.
 */
void buildRosterSync(struct NetRosterSync* sync)
{
    BOOL in_town = lobby_world_block[0x3F0C] == 7;
    BOOL hidden = ck_option_cfg(27) == 0;
    BOOL in_party = net_ctrl_wk->flag_0xC379 == 1;
    BOOL away = lobby_w.field_0x0B0 == 3;

    if (net_ctrl_wk->layer_state_0x064 == 0) {
        away = TRUE;
    }
    sync->count_0x00 = 6;
    sync->items_0x04[0].key_0x00 = 1;
    sync->items_0x04[0].value_0x04 = in_town ? 1 : (away ? 2 : 0);
    sync->items_0x04[1].key_0x00 = 2;
    sync->items_0x04[1].value_0x04 = in_town ? 1 : (away ? 2 : 0);
    sync->items_0x04[2].key_0x00 = 3;
    sync->items_0x04[2].value_0x04 = in_town ? 1 : (away ? 2 : 0);
    sync->items_0x04[3].key_0x00 = 4;
    sync->items_0x04[3].value_0x04 = in_town ? 1 : (away ? 2 : (in_party ? 3 : 0));
    sync->items_0x04[4].key_0x00 = 6;
    sync->items_0x04[4].value_0x04 = in_town;
    sync->items_0x04[5].key_0x00 = 7;
    sync->items_0x04[5].value_0x04 = in_town ? 1 : (away ? 2 : (in_party ? 3 : (hidden ? 4 : 0)));
}

/*
 * Clears the community command results, hands the community layer its reflect callback, fills this player's
 * profile and presence record and hands the mediator the profile record.
 */
void installCommunityCallback(void)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 i;

    if (getNetworkCommunityPat(getPatsObject(), 0) != NULL) {
        for (i = 0; i < 3; i++) {
            work->community_results_0xA144[i * 6 + 0] = 0;
            work->community_results_0xA144[i * 6 + 1] = 0;
            work->community_results_0xA144[i * 6 + 2] = 0;
            work->community_results_0xA144[i * 6 + 3] = 0;
            work->community_results_0xA144[i * 6 + 4] = 0;
            work->community_results_0xA144[i * 6 + 5] = 0;
        }
        getNetworkCommunityPat(getPatsObject(), 0)->setReflectCallback((u32)communityReflectCallback, 0);
        buildNetUserProfile(&net_user_profile);
        net_user_profile.settings_0xF4[0] = 0xFF;
        net_user_profile.settings_0xF4[1] = 0xFF;
        net_user_profile.settings_0xF4[2] = 0xFF;
        net_user_profile.settings_0xF4[3] = 0xFF;
        net_presence_record.count_0x00 = 4;
        net_presence_record.pairs_0x04[0].kind_0x00 = 1;
        net_presence_record.pairs_0x04[0].value_0x04 = net_user_profile.region_0x05 << 24;
        net_presence_record.pairs_0x04[1].kind_0x00 = 1;
        net_presence_record.pairs_0x04[1].value_0x04 =
            net_user_profile.rank_0xF2 | ((net_user_profile.id_0x00 << 16) | (net_user_profile.level_0xF3 << 8));
        net_presence_record.pairs_0x04[2].kind_0x00 = 1;
        net_presence_record.pairs_0x04[2].value_0x04 = -1;
        net_presence_record.pairs_0x04[3].kind_0x00 = 1;
        net_presence_record.pairs_0x04[3].value_0x04 = 0;
        postMediatorRecord(getInstance(), net_user_profile.record_0x9C);
    }
}

/*
 * This player's community profile inside the community state block.
 */
NetUserProfile* getCommunityUserProfile(void)
{
    return &net_community_state.profile_0x028;
}

/*
 * The community state block's 0x44-byte block at +0x128.
 */
u8* getCommunityStateBlock(void)
{
    return net_community_state.block_0x128;
}

/*
 * Member record `index` of the community state block.
 */
u8* getCommunityMemberRecord(u8 index)
{
    return net_community_state.members_0x16C[index];
}

/*
 * Requests the community news (community command 6).
 */
void requestCommunityNews(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    work->community_results_0xA144[6] = 0;
    getNetworkCommunityPat(getPatsObject(), 0)->requestNews_38();
    work->result_0xC290 = result;
    *result = 0;
}

/*
 * Requests the block list (community command 24).
 */
void requestCommunityBlockList(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    work->community_results_0xA144[24] = 0;
    getNetworkCommunityPat(getPatsObject(), 0)->requestBlockList();
    work->result_0xC290 = result;
    *result = 0;
}

/*
 * Rebuilds this player's presence pairs and sends them with the friend roster (community command 18).
 */
void requestFriendSync(s8* result)
{
    NetCtrlWk* work = net_ctrl_wk;

    work->community_results_0xA144[18] = 0;
    buildRosterSync(&work->roster_sync_0x61CC);
    getNetworkCommunityPat(getPatsObject(), 0)->syncFriends(&work->roster_sync_0x61CC);
    work->result_0xC290 = result;
    *result = 0;
}

/*
 * Whether the address object `address` is on the friend roster.
 */
s32 isRosterMember(const NetworkSmallObject* address)
{
    NetRosterList* roster = &net_ctrl_wk->roster_0xA1B8;
    s32 i;

    for (i = 0; i < roster->count_0x000; i++) {
        if (networkSmallObject_isEqual(address, &roster->entries_0x004[i].address_0x00) == 1U) {
            return 1;
        }
    }
    return 0;
}

/*
 * Appends `src` (its address object and name) to the friend roster while there is room.
 */
void appendRosterEntry(const NetRosterRec* src)
{
    NetRosterRec* entry;
    NetRosterList* roster = &net_ctrl_wk->roster_0xA1B8;

    if (roster->count_0x000 < 50) {
        entry = &roster->entries_0x004[roster->count_0x000];
        ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)&src->address_0x00);
        strcpy(entry->name_0x20, src->name_0x20);
        roster->count_0x000++;
    }
}

/*
 * Removes the roster entry whose address object is `address`, moving the later entries up.
 */
void removeRosterEntry(const NetworkSmallObject* address)
{
    NetRosterList* roster = &net_ctrl_wk->roster_0xA1B8;
    s32 i;

    for (i = 0; i < roster->count_0x000; i++) {
        if (networkSmallObject_isEqual(address, &roster->entries_0x004[i].address_0x00) == 1U) {
            break;
        }
    }
    if (i < roster->count_0x000) {
        for (; i < roster->count_0x000 - 1; i++) {
            NetRosterRec* entry = &roster->entries_0x004[i];
            NetRosterRec* next = &roster->entries_0x004[i + 1];

            ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)&next->address_0x00);
            strcpy(entry->name_0x20, next->name_0x20);
        }
        roster->count_0x000 = roster->count_0x000 - 1;
        if (roster->count_0x000 < 0) {
            roster->count_0x000 = 0;
        }
    }
}

/*
 * Appends the address object `address` to the recent-player list, named with the work record's message text.
 */
void appendRecentEntry(const NetworkSmallObject* address)
{
    NetCtrlWk* work = net_ctrl_wk;
    NetRecentRec* entry;
    NetRecentList* recent = &work->recent_0xBB84;

    if (recent->count_0x000 < 16) {
        entry = &recent->entries_0x004[recent->count_0x000];
        ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)address);
        strcpy(entry->name_0x20, work->message_0xBF08);
        recent->count_0x000++;
    }
}

/*
 * Removes the recent-player entry whose address object is `address`, moving the later entries up.
 */
void removeRecentEntry(const NetworkSmallObject* address)
{
    NetRecentList* recent = &net_ctrl_wk->recent_0xBB84;
    s32 i;

    for (i = 0; i < recent->count_0x000; i++) {
        if (networkSmallObject_isEqual(address, &recent->entries_0x004[i].address_0x00) == 1U) {
            break;
        }
    }
    if (i < recent->count_0x000) {
        for (; i < recent->count_0x000 - 1; i++) {
            NetRecentRec* entry = &recent->entries_0x004[i];
            NetRecentRec* next = &recent->entries_0x004[i + 1];

            ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)&next->address_0x00);
            strcpy(entry->name_0x20, next->name_0x20);
        }
        recent->count_0x000 = recent->count_0x000 - 1;
        if (recent->count_0x000 < 0) {
            recent->count_0x000 = 0;
        }
    }
}

/*
 * The community layer's reflect callback: records command `command`'s result word (and a failure's error
 * triple), copies what the command delivered (the roster, the recent players, the community state, mail,
 * presence changes) and reports the outcome through the caller's result byte.
 */
/* untyped: caller-owned payload - each community command delivers its own record */
s32 communityReflectCallback(u32 command, s32 result, s32 count, void* data)
{
    NetCtrlWk* work = net_ctrl_wk;
    s32 i;

    if (result < 0 && data != NULL) {
        work->community_error_0x128 = *(NetFetchError*)data;
    }
    switch (command) {
    case 1:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            work->error_0x05C = result;
            setErrorCode(4);
        } else {
            work->community_results_0xA144[command] = 1;
            work->flag_0x016 = 1;
        }
        break;
    case 2:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
        }
        break;
    case 3:
        work->error_0x05C = result;
        break;
    case 4:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
            work->cmd4_value_0xA1B0 = *(s32*)data;
        }
        break;
    case 5:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
            work->cmd5_value_0xA1B4 = *(s32*)data;
        }
        break;
    case 6:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
            work->community_results_0xA144[command] = 1;
            work->roster_0xA1B8.count_0x000 = ((NetRosterList*)data)->count_0x000;
            for (i = 0; i < work->roster_0xA1B8.count_0x000; i++) {
                NetRosterRec* entry = &work->roster_0xA1B8.entries_0x004[i];

                ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)&((NetRosterList*)data)->entries_0x004[i].address_0x00);
                strcpy(entry->name_0x20, ((NetRosterList*)data)->entries_0x004[i].name_0x20);
            }
        }
        break;
    case 7:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
        }
        break;
    case 9:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            work->sub_error_0xC259 = -1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            work->community_results_0xA144[command] = 1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 10:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            NetMailBox* box = getLobbyMailBox();
            u8 room = 1;

            if (box->count_0x000 >= 16) {
                room = dropLobbyMail(0);
            }
            if (room == 1) {
                strcpy(box->entries_0x014[box->count_0x000].sender_name_0x00, ((NetSessionChatRecord*)data)->name_0x020);
                formatNetId(box->entries_0x014[box->count_0x000].sender_id_0x14,
                            &((NetSessionChatRecord*)data)->sender_0x000);
                box->entries_0x014[box->count_0x000].from_friend_0x1E = 0;
                box->unread_0x004[box->count_0x000] = 0;
                postMediatorRecord(getInstance(), ((NetSessionChatRecord*)data)->text_0x035);
                strncpy(box->entries_0x014[box->count_0x000].text_0x1F,
                        (const char*)((NetSessionChatRecord*)data)->text_0x035, 0xC1);
                box->entries_0x014[box->count_0x000].text_0x1F[0xC0] = 0;
                box->count_0x000++;
                cockpitShowNewMail();
            }
            work->community_results_0xA144[command] = 1;
        }
        break;
    case 11:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            s32 peer;

            work->community_results_0xA144[command] = 1;
            peer = findPeerIndex((const NetId*)data);
            if (peer > 0) {
                memcpy(&work->peers_0x7488[peer].blob_0x20, ((NetPeerCardUpdate*)data)->blob_0x28, 0x100);
            }
            work->peer_cards_changed_0x7995 = 0;
        }
        break;
    case 17:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
            if (work->flag_0xC368 == 0) {
                work->sub_error_0xC259 = -1;
            }
            work->flag_0xC368 = 0;
        } else {
            NetUserProfile* profile;

            work->community_results_0xA144[command] = 1;
            memcpy(&net_community_state, data, sizeof(NetCommunityState));
            profile = getCommunityUserProfile();
            applyNetUserProfile((const u8*)profile);
            postMediatorRecord(getInstance(), profile->record_0x9C);
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 12:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            work->error_0x05C = result;
            setErrorCode(5);
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            work->community_results_0xA144[command] = 1;
            lobby_w.community_flag_0x009 = 0;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 13:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
            updatePeerCardBlock((const NetId*)data, ((NetPeerCardUpdate*)data)->blob_0x28,
                                ((NetPeerCardUpdate*)data)->size_0x24, ((NetPeerCardUpdate*)data)->offset_0x20);
        }
        break;
    case 18:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
            work->error_0x05C = result;
            setErrorCode(5);
        } else {
            work->community_results_0xA144[command] = 1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 19:
    case 21:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            work->sub_error_0xC259 = -1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            work->community_results_0xA144[command] = 1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 25:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            appendRecentEntry((const NetworkSmallObject*)data);
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 26:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            removeRecentEntry((const NetworkSmallObject*)data);
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 24:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            work->community_results_0xA144[command] = 1;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
            work->recent_0xBB84.count_0x000 = ((NetRecentList*)data)->count_0x000;
            for (i = 0; i < work->recent_0xBB84.count_0x000; i++) {
                NetRecentRec* entry = &work->recent_0xBB84.entries_0x004[i];

                ((NetworkSmallObjectSink*)&entry->address_0x00)->copyFrom((const u8*)&((NetRecentList*)data)->entries_0x004[i].address_0x00);
                strcpy(entry->name_0x20, ((NetRecentList*)data)->entries_0x004[i].name_0x20);
            }
        }
        break;
    case 23:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = -1;
            }
        } else {
            work->community_results_0xA144[command] = 1;
            removeRosterEntry((const NetworkSmallObject*)data);
            if (work->result_0xC290 != NULL) {
                *work->result_0xC290 = 1;
            }
        }
        break;
    case 22:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            work->community_results_0xA144[command] = 1;
            if (isRosterMember((const NetworkSmallObject*)data) == 0) {
                char id_text[0x14];
                NetRosterRec* presence = (NetRosterRec*)data;

                formatNetId(id_text, &presence->id_0x00);
                if (presence->state_0x35 == 1) {
                    appendRosterEntry(presence);
                    addFriendNotice(id_text, presence->name_0x20);
                    postChatLogLine(1, 0, 0xFFFFF0, MH3GetErrorString2(30), id_text, presence->name_0x20);
                } else if (presence->state_0x35 == 2) {
                    postChatLogLine(1, 0, 0xFFFFF0, MH3GetErrorString2(31), id_text, presence->name_0x20);
                } else {
                    postChatLogLine(1, 0, 0xFFFFF0, MH3GetErrorString2(32), id_text, presence->name_0x20);
                    if (presence->state_0x35 == 3) {
                        postChatLogLine(1, 0, 0xFFFFF0, MH3GetErrorString2(33), id_text, presence->name_0x20);
                    } else {
                        postChatLogLine(1, 0, 0xFFFFF0, MH3GetErrorString2(34), id_text, presence->name_0x20);
                    }
                }
            }
        }
        break;
    case 20:
        if (result < 0) {
            work->community_results_0xA144[command] = result;
        } else {
            NetMailBox* box;
            u8 room;

            work->community_results_0xA144[command] = 1;
            box = getLobbyMailBox();
            room = 1;
            if (box->count_0x000 >= 16) {
                room = dropLobbyMail(0);
            }
            if (room == 1) {
                formatNetId(box->entries_0x014[box->count_0x000].sender_id_0x14,
                            &((NetSessionChatRecord*)data)->sender_0x000);
                strcpy(box->entries_0x014[box->count_0x000].sender_name_0x00, ((NetSessionChatRecord*)data)->name_0x020);
                box->entries_0x014[box->count_0x000].from_friend_0x1E = 1;
                box->unread_0x004[box->count_0x000] = 0;
                postMediatorRecord(getInstance(), ((NetSessionChatRecord*)data)->text_0x035);
                strncpy(box->entries_0x014[box->count_0x000].text_0x1F,
                        (const char*)((NetSessionChatRecord*)data)->text_0x035, 0xC1);
                box->entries_0x014[box->count_0x000].text_0x1F[0xC0] = 0;
                box->count_0x000++;
                cockpitShowNewMail();
            }
        }
        break;
    }
    return 0;
}

/*
 * Renders a network id as text into `out` (ten bytes), printing every `%` as `*`.
 */
void formatNetId(char* out, const NetId* id)
{
    memset(out, 0, 10);
    exportTo((const NetworkSmallObject*)id, (u8*)out, 10);
    for (; *out != 0; out++) {
        if (*out == '%') {
            *out = '*';
        }
    }
}

/*
 * Imports the six raw bytes of a network id into the address object `dst`.
 */
void importNetId(NetId* dst, const NetId* src)
{
    networkSmallObject_setAddress((NetworkSmallObject*)dst, 3, (const u8*)src, 6);
}

/*
 * Whether the address object `left` holds the six raw id bytes `right` (1).
 */
u32 isSameNetId(const NetId* left, const NetId* right)
{
    NetworkSmallObject object;
    u32 same;

    networkSmallObject_construct(&object);
    networkSmallObject_setAddress(&object, 3, (const u8*)right, 6);
    same = networkSmallObject_isEqual((const NetworkSmallObject*)left, &object);
    NetworkSmallObjectSink::destroy(&object);
    return same;
}

/*
 * Sends the check request for item-box page `page`: tag `code`/1 and the packed page record.
 */
void sendBoxPageCheckRequest(u8 code, s32 page)
{
    u8 tags[4];
    u8 record[0x2DC];

    if (getInstance_() != NULL) {
        tags[0] = code;
        tags[1] = 1;
        exportItemBoxPage(record, page);
        sendReqUnknownCheck(getInstance_(), tags, record, sizeof(record));
    }
}

/*
 * Sends the check request for the equipment set: tag `code`/2 and the packed equipment record.
 */
void sendCheckRequest(u8 code)
{
    u8 tags[8];
    u8 record[0xF0];

    if (getInstance_() != NULL) {
        tags[0] = code;
        tags[1] = 2;
        exportEquipRecord(record);
        sendReqUnknownCheck(getInstance_(), tags, record, sizeof(record));
    }
}

#ifdef __cplusplus
}
#endif
