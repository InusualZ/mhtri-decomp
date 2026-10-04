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
 * Residuals (2026-10-03: 43 of 88 rows at 100 %, report metric 29.2 %):
 *  - Not written: `sessionReflectCallback` (4352 B, its 43-case table is this unit's `.data` 0x80603D70) and
 *    `communityReflectCallback` (2536 B); the circle-list copies (`onCircleListReceived`, `copyCircleToProfile`:
 *    the profile record's +0x1C small object and +0x44 record block need the small network object as a C++
 *    class); the roster requests 0x80435740..0x80435E34 and `flushRosterSync` (NetworkCommunityPat slots +0x4C/
 *    +0x50/+0x60 and NetworkLayerPat +0xA0 are still `pad_NN()` in their owners' headers); the small-object
 *    lookups 0x80434D34..0x80434FB4, the NetId helpers 0x80437040..0x804370CC, the `.bss` object block
 *    0x806E18A0 accessors and the static initialiser 0x80437204.
 *  - Partial: `installCommunityCallback` 88.4 % (retail strides the three result records from the work pointer;
 *    the 2-D index folds the offsets), `onSessionCloseDone` 96.9 % (the member loop's registers; the member's
 *    +0x20 id is handed to `getPlayerRecord` as the small network object it copies into),
 *    `getProfileMemberCounts` 99.6 %, `buildRosterSync` 99.3 % (one register swap), `getProfileQuestRecord` 98.6 %.
 */

#include "Network/net_session_close.h"
#include "Network/network_pat_control.h"
#include "Network/NetworkSessionManager.h"   /* NetworkSessionManagerPat - the class the session calls dispatch through */
#include "lobby/lobby_w.h"                    /* lobby_w - owner lobby/lb_menu_pos_tbl.cpp */
#include "fn_80047398/lobby_world_block.h"    /* lobby_world_block - owner fn_80047398.cpp */
#include "sound/fn_800E46E8.h"                /* getInstance - owner sound/fn_800E46E8.cpp */

#include "Network/network_opening.h"            /* postMediatorRecord - owner Network/network_opening.cpp */
#include "menu/get_pop_dat_ptr.h"               /* ck_option_cfg - owner menu/get_pop_dat_ptr.cpp */
#include "userdata_item.h"                      /* buildNetUserProfile - owner userdata_item.cpp */
#include "Network/NetworkCommunityPat.h"        /* NetworkCommunityPat - the community layer's reflect slot */

#pragma peephole off

/* This player's community profile and the presence record the layer is sent (`.bss` 0x806E1770 / 0x806E1870). */
NetUserProfile net_user_profile;
NetStatusRecord net_presence_record;

#ifdef __cplusplus
extern "C" {
#endif

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
            work->community_results_0xA144[i].words_0x00[0] = 0;
            work->community_results_0xA144[i].words_0x00[1] = 0;
            work->community_results_0xA144[i].words_0x00[2] = 0;
            work->community_results_0xA144[i].words_0x00[3] = 0;
            work->community_results_0xA144[i].words_0x00[4] = 0;
            work->community_results_0xA144[i].words_0x00[5] = 0;
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

#ifdef __cplusplus
}
#endif
