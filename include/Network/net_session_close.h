/* Declarations owned by `src/Network/net_session_close.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_NET_SESSION_CLOSE_H
#define MHTRI_NETWORK_NET_SESSION_CLOSE_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
typedef struct NetId NetId;                 /* include/Network/NetworkLayerPat.h */
struct NetRosterSync;

#ifdef __cplusplus
extern "C" {
#endif

/* This player's community profile record (`.bss` 0x806E1770, 0x100 B), filled by the user-data band (0x8004A5AC)
 * and sent to the community layer: only the bytes this unit reads or writes are named.  size: 0x100 */
typedef struct NetUserProfile {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u8 pad_0x02[0x3];
    /* +0x05 */ u8 region_0x05;
    /* +0x06 */ u8 pad_0x06[0x96];
    /* +0x9C */ u8 record_0x9C[0x56];   /* the record handed to the mediator (0x80415FAC) */
    /* +0xF2 */ u8 rank_0xF2;
    /* +0xF3 */ u8 level_0xF3;
    /* +0xF4 */ u8 settings_0xF4[4];
    /* +0xF8 */ u8 server_0xF8;
    /* +0xF9 */ u8 in_party_0xF9;
    /* +0xFA */ u8 pad_0xFA[0x6];
} NetUserProfile; /* size: 0x100 */

/* One kind/value pair of the presence record the layer is sent. */
typedef struct NetStatusPair {
    /* +0x00 */ s32 kind_0x00;
    /* +0x04 */ s32 value_0x04;
} NetStatusPair; /* size: 0x8 */

/* The presence record (`.bss` 0x806E1870): the pair count and four pairs.  size: 0x24 */
typedef struct NetStatusRecord {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetStatusPair pairs_0x04[4];
} NetStatusRecord; /* size: 0x24 */

/* The community layer's state block (`.bss` 0x806E18A0, 0x26C B): community command 17 delivers it whole.  This
 * player's community profile sits at +0x28, a 0x44-byte block at +0x128 and four 0x40-byte member records at
 * +0x16C.  The static initialiser builds it with the NetworkCommunityPat band's constructor (0x803F0730).
 * size: 0x26C */
typedef struct NetCommunityState {
    /* +0x000 */ u8 pad_0x000[0x28];
    /* +0x028 */ NetUserProfile profile_0x028;
    /* +0x128 */ u8 block_0x128[0x44];
    /* +0x16C */ u8 members_0x16C[4][0x40];
} NetCommunityState; /* size: 0x26C */

extern NetUserProfile net_user_profile;
extern NetStatusRecord net_presence_record;
extern NetCommunityState net_community_state;

/* 0x80435F38 / 0x804360B0 / 0x804360C0 - the community state's profile, its +0x128 block and member record
 * `index`. */
NetUserProfile* getCommunityUserProfile(void);
u8* getCommunityStateBlock(void);
u8* getCommunityMemberRecord(u8 index);

/* 0x804360DC / 0x8043614C / 0x804361B0 - community requests (news, block list, friend-roster sync): clear the
 * command's result word, issue it and hand the caller's result byte to the community callback. */
void requestCommunityNews(s8* result);
void requestCommunityBlockList(s8* result);
void requestFriendSync(s8* result);

/* 0x804362E0..0x8043656C - the friend roster and the recent-player list: membership, append and remove. */
s32 isRosterMember(const struct NetworkSmallObject* address);
void appendRosterEntry(const struct NetRosterRec* src);
void removeRosterEntry(const struct NetworkSmallObject* address);
void appendRecentEntry(const struct NetworkSmallObject* address);
void removeRecentEntry(const struct NetworkSmallObject* address);

/* 0x80436658 - the community layer's reflect callback. */
/* untyped: caller-owned payload - the community layer's reflect payload */
s32 communityReflectCallback(u32 command, s32 result, s32 count, void* data);

/* 0x80435394 - this player's region byte from the community profile. */
u8 getUserRegion(void);

/* 0x804338E0 - the quest record id of profile `index` (+0x190), or NULL while the profile is not active
 * (the lobby quest board looks the record up with it). */
u16* getProfileQuestRecord(s32 index);

/* 0x804343C4 - starts the network session's close-down sequence (0 when there is no session manager or one
 * is already closing, 1 once started); 0x80434668 - its progress: -1 on error, 1 when done or when there is
 * no manager, else 0.  No registered unit owns the addresses, so the band is their home (rule 2).  Added
 * with `quest/arenatask.cpp`; the names are GUESSES from the bodies. */
s32 net_session_close_start(void);

s32 net_session_close_state_get(void);

/* 0x80437040 - renders a network id as text into `out` (a `%` in the id prints as `*`). */
void formatNetId(char* out, const NetId* id);

/* 0x804370BC - imports a network id from `src` into `dst`. */
void importNetId(NetId* dst, const NetId* src);

/* 0x804344DC.. - the handlers of the actions queued in the work record's action byte (1, 4, 5, 6, 7, 8); each
 * issues one session command and returns 1, or 0 when it could not (no session manager, a busy request). */
s32 runPendingAction1(void);

s32 runPendingAction5(void);

s32 runPendingAction6(void);

s32 runPendingAction7(void);

s32 runPendingAction8(void);

/* 0x8043390C / 0x80433B0C / 0x80433BC8 - the circle list: its completion handler, the request (command 39)
 * and the copy of circle `index` into the party table. */
void onCircleListReceived(s32 status, s32* values);
s32 requestCircleList(void);
void copyCircleToProfile(s32 index, s32 unused);

/* 0x80433DC4 / 0x80433EEC / 0x80433FC8 - the party enter (command 6): its result and the request; the
 * published session name. */
s32 getProfileEnterResult(void);
s32 requestProfileEnter(s32 index);
void setSessionDisplayName(const char* name);

/* 0x8043405C / 0x8043413C / 0x8043420C..0x80434214 - the ready on/off requests (command 0x1A) and the quest
 * board's entry points to them. */
s32 requestReadyOn(void);
s32 requestReadyOff(void);
void requestReadyOnAlias(void);
void requestReadyOnAlias2(void);
void requestReadyOffAlias(void);

/* 0x804344BC / 0x804346CC - the completions of pending action 1 (command 0x18) and of the leave (command 0x1E),
 * which the session callback also runs for commands 25 and 31. */
void onAction1Done(s32 status, s32* values);
void onLeaveDone(s32 status, s32* values);

/* 0x804346F8 / 0x804347CC - the leave request (command 0x1E) and the leave-or-abort choice. */
s32 requestLeave(void);
void requestLeaveOrAbort(void);

/* 0x80434908 / 0x80434924 / 0x804349A4 - the party table queries. */
BOOL hasSelectedProfile(void);
struct NetProfileRec* refreshAllProfiles(void);
BOOL getProfileMemberCounts(u8* members, u8* active, u8* capacity);

/* 0x80434AF4..0x80434CC8 - the quest board's party queries (GUESS names from the fields they read). */
BOOL isPartyCountRead(void);
u8 getOwnMemberFlag(void);
u8 getProfileFlag(void);
u32 isProfileUnselected(void);
u16 getSelectedQuestId(void);

/* 0x804353A4 / 0x804356D0 / 0x80435BAC / 0x80435D1C - the friend roster sync helpers (buildRosterSync fills the
 * presence pairs `NetworkCommunityPat::syncFriends` sends). */
void buildRosterSync(struct NetRosterSync* sync);

void flushRosterSync(void);

void refreshRosterCache(void);

void startRosterFetch(s32 mode);

/* 0x804334B4 / 0x80433518 / 0x8043361C - the session join (command 4): its result (-1 failed, 1 done, 0
 * pending), its completion handler and the request itself; 0x80433814 - whether this player holds a server
 * slot.  GUESS names from the command and the fields they write. */
s32 getSessionJoinResult(void);
void onSessionJoined(s32 status, s32* values);
BOOL hasJoinedServer(void);

/* 0x80433384 / 0x8043339C / 0x80434FB4 / 0x80433B0C - the shutdown phase slots and the friend slot lookup. */
void clearPhaseSlot(s32 slot);

u32 isPhaseSlotDone(s32 slot);

s8 lookupFriendSlot(const u8* id);

/* 0x80434D34 / 0x80434E48 / 0x80434EA0 - whether an address object (or six raw id bytes) belongs to a session
 * member, and the member's slot (-1 when none). */
s32 isSessionMember(const struct NetworkSmallObject* address);
s32 isSessionMemberId(const NetId* id);
s8 findSessionMemberSlot(const struct NetworkSmallObject* address);

/* 0x804338AC - hands the mediator a quest-board record (the quest board's only network call). */
void postQuestBoardRecord(u8* record);

/* The join parameters the quest board hands `requestSessionJoin`: the quest (published as name entry 1), the
 * party size (name entry 0, and the join request's argument), the session name and the circle comment.
 * size: 0x99 (approximate: the comment is the session manager's 0x91-byte field) */
typedef struct NetJoinParams {
    /* +0x00 */ u16 quest_0x00;
    /* +0x02 */ s8 party_size_0x02;
    /* +0x03 */ char session_name_0x03[0x5];
    /* +0x08 */ char comment_0x08[0x91];
} NetJoinParams; /* size: 0x99 (approximate) */

/* 0x8043361C - requests the session join (command 4); 0 when there is no session manager or a request is busy. */
s32 requestSessionJoin(const NetJoinParams* params);



/* 0x80433870 - reads the link state; 1 = the network link is up. */
u32 getLinkStatus(void);

/* 0x804344A0 - whether the session-start request has completed (`net_ctrl_wk` +0xC153, set by the start
 * request's completion callback and cleared by `net_session_close_start`).  GUESS name from those writers. */
u32 isSessionStartDone(void);

/* 0x80434800 - drops the session after a link loss: clears `net_ctrl_wk` +0x98 and raises pending action 8
 * (`runPendingAction8`); returns 0 when there is no work record.  GUESS name from its callers, which are the
 * link-loss exits of the arena and the lobby. */
s32 net_session_abort_start(void);

/* 0x804333B0 - resets the link state the reconnect path relies on. */
void resetLinkState(void);

/* 0x80432104 - stores `state` as member `member`'s link state in the kind-0 move work (GUESS name: the
 * session callback writes 4 when a member drops). */
void setMoveWorkMemberState(u8 member, u8 state);

/* 0x80433254 / 0x80435570 - install the session manager's and the community layer's reflect callbacks
 * (`sessionReflectCallback` 0x80432154, `communityReflectCallback` 0x80436658) after clearing their result
 * words. */
void installSessionCallback(void);
/* 0x80432154 - the session manager's reflect callback: records command `command`'s result and runs its
 * completion (always 0). */
/* untyped: caller-owned payload - each session command delivers its own record */
s32 sessionReflectCallback(s32 command, s8 member, s32 result, s32 count, void* data);
void installCommunityCallback(void);

/* 0x8043713C / 0x804371A8 - send the pat interface's check request carrying `code`: for item-box page `page`
 * (tag 1) and for the equipment set (tag 2) (GUESS names: the callee is `sendReqUnknownCheck`). */
void sendBoxPageCheckRequest(u8 code, s32 page);
void sendCheckRequest(u8 code);

/* 0x804370CC - whether two network ids are equal (1). */
u32 isSameNetId(const NetId* left, const NetId* right);

#ifdef __cplusplus
}
#endif

/* 0x8043500C - the message id of the session layer's last error (C++ linkage in the map,
 * `MH3GetSessionErrorCode__Fv`; the session twin of `getNetErrorMessageId`). */
s32 MH3GetSessionErrorCode(void);

#endif /* MHTRI_NETWORK_NET_SESSION_CLOSE_H */
