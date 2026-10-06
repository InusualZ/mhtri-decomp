/*
 * Network/network_pat_control.h - the views of `Network/network_pat_control.cpp`: the `net_ctrl_wk` singleton (`.sbss`
 *   0x80794CF8, pointing at `net_ctrl_work`, `.bss` 0x806D3790) and its class `NetCtrlWk` (0xC4A8 B, the map's size).
 * SHAPES. `NetCtrlWk` is packed for the `u32` run at the odd offset +0x7996; the band's functions on the singleton are
 *   its (static) members.  The "*Pat" holder is `Network/NetworkPat.h`'s.
 */
#ifndef MHTRI_NETWORK_NETWORK_PAT_CONTROL_H
#define MHTRI_NETWORK_NETWORK_PAT_CONTROL_H

#include "types.h"
#include "OS/mem.h"                /* MEMiHeapHead - the network heap's head */
#include "MSL_C/alloc.h"           /* strcpy (owner: MSL_C/alloc.cpp, rule 2) */
#include "Network/NetworkPat.h"
#include "Network/NetworkSessionManagerPat.h"   /* getPatsObject, isNetworkSessionManagerPatReady (owner's header, rule 2) */
#include "Network/NetworkLayerPat.h"   /* NetId and the layer records the work record embeds */
#include "Network/NetworkCommunityPat.h"   /* NetworkCommunityFriendList / NetworkCommunityBlockList - the two lists */
#include "Network/NetworkUniqueId.h"       /* NetworkUniqueId - the roster entries' address objects */
#include "Network/NetworkFileFetcher.h"     /* NetworkFileFetcher - the fetch state machines' client */
#include "menu/PatTerms.h"                  /* PatTerms - the terms object (owner menu/menu_plsearch.cpp) */

#ifdef __cplusplus
extern "C" {
#endif

/* One slot of the 16-entry server/message table at `NetCtrlWk::entries_0x7CD8` (stride 0x5C). */
typedef struct NetCtrlEntry {
    /* +0x00 */ u8 in_use_0x00;
    /* +0x01 */ u8 index_0x01;    /* the slot's own index (initWorkRecord numbers them) */
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 flags_0x03;
    /* +0x04 */ char name_0x04[0x48];
    /* +0x4C */ u32 value_0x4C;
    /* +0x50 */ char text_0x50[0xC];
} NetCtrlEntry; /* size: 0x5C */

/* One slot of the 100-record `slots_0x3ED4` table.  The first byte is the slot's state (0 = free), +0x04 the
 * member's friend record (the record's address is the handle `findSlotByOwner` matches), +0x3C/+0x40 the two
 * float members and +0x54 the slot's mode word.  The record makes the slot a class the compiler builds and
 * destroys (inline constructor 0x804320D0, destructor 0x80431FCC). */
typedef struct NetSlot {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ NetFriendRec rec_0x04;
    /* +0x3C */ f32 valueA_0x3C;
    /* +0x40 */ f32 valueB_0x40;
    /* +0x44 */ u8 pad_0x44[0x10];
    /* +0x54 */ u32 mode_0x54;
} NetSlot; /* size: 0x58 */

/* One record of the 128-entry `pool_0x6320` array (stride 0x20).  `in_use_0x08` marks a handed-out
 * record; the first record also carries a live count at +0x04. */
typedef struct NetPoolEntry {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 count_0x04;
    /* +0x08 */ u32 in_use_0x08;
    /* +0x0C */ u8 pad_0x0C[0x14];
} NetPoolEntry; /* size: 0x20 */

/* A pool record seen as a queued network command (`queueNetCommand`, `updateMessagePool`): its state (1 queued,
 * 2 sent, 3..6 a layer entry's later steps), the command, the caller's result byte, a data pointer, the argument
 * count and up to four argument words.  size: 0x20 */
typedef struct NetCommandEntry {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 command_0x01;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ s8* result_0x04;
    /* +0x08 */ s32 data_0x08;   /* commands 1 and 2: the caller's buffer / word */
    /* +0x0C */ s32 arg_count_0x0C;
    /* +0x10 */ s32 args_0x10[4];
} NetCommandEntry; /* size: 0x20 */

/* The command pool from its head: the live count, then the 128 records.  size: 0x1008 */
typedef struct NetCommandPool {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ u32 count_0x004;
    /* +0x008 */ NetCommandEntry entries_0x008[128];
} NetCommandPool; /* size: 0x1008 */

/*@TYPES begin@*/
/* One 0x40-byte member slot of a profile: the joined word and flag the session join writes, the value
 * session commands 20/21 report, the member's display name (`getPlayerRecordName`, 16 bytes) and the
 * member's id at +0x20 (`getPlayerRecord`). */
typedef struct NetMemberSlot {
    /* +0x00 */ u32 joined_0x00;
    /* +0x04 */ u8 flag_0x04;
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ s32 value_0x08;
    /* +0x0C */ char name_0x0C[0x10];
    /* +0x1C */ u8 pad_0x1C[0x4];
    /* +0x20 */ NetworkUniqueId id_0x20;   /* the slot's implicit constructor 0x80431EC8 builds it */
} NetMemberSlot; /* size: 0x40 */

/* The 136-byte block a circle carries in its record bytes (`NetworkSessionCircleInfo::records_188`, taken only when
 * the circle reports exactly 136 of them): four 16-byte labels, four more and four small counts the copy clamps to
 * 1..8.  GUESS names from that copy (`copyCircleToProfile`).  size: 0x88 */
typedef struct NetProfileLabels {
    /* +0x00 */ char label_0x00[4][0x10];
    /* +0x40 */ char sublabel_0x40[4][0x10];
    /* +0x80 */ u8 count_0x80[4];
    /* +0x84 */ u8 pad_0x84[0x4];
} NetProfileLabels; /* size: 0x88 */

/* The per-profile record `NetCtrlWk::profiles_0x168` points at (stride 0x2B4): the circle (lobby room) copied in by
 * `copyCircleToProfile`/`onCircleListReceived` - its id, name, address object, size and option block - and the
 * four session member slots. */
typedef struct NetProfileRec {
    /* +0x000 */ u8 index_0x000;            /* the profile's own index */
    /* +0x001 */ u8 pad_0x001[0x3];
    /* +0x004 */ s32 circle_id_0x004;
    /* +0x008 */ char name_0x008[0x14];
    /* +0x01C */ NetworkUniqueId address_0x01C;
    /* +0x03C */ s32 capacity_0x03C;
    /* +0x040 */ s32 active_0x040;          /* the circle's first limit word: 0 leaves the profile empty */
    /* +0x044 */ NetworkCircleOptions options_0x044;   /* slot 1's value is the quest id (copied to quest_id_0x190) */
    /* +0x08C */ u8 rank_0x08C;
    /* +0x08D */ u8 flag_0x08D;
    /* +0x08E */ u8 pad_0x08E[0x2];
    /* +0x090 */ NetMemberSlot members_0x090[4];
    /* +0x190 */ u16 quest_id_0x190;   /* the quest record id `getProfileQuestRecord` hands out */
    /* +0x192 */ s8 option_0x192;      /* option slot 0's value, narrowed */
    /* +0x193 */ u8 pad_0x193[0x5];
    /* +0x198 */ char comment_0x198[0x94];   /* the circle comment (at most 144 characters) */
    /* +0x22C */ NetProfileLabels labels_0x22C;
    /* +0x2B4 */
} NetProfileRec; /* size: 0x2B4 */

/* The name field of a peer record (0x14 bytes). */
typedef struct NetPeerName {
    /* +0x00 */ char text_0x00[0x14];
} NetPeerName; /* size: 0x14 */

/* The 0x100-byte character blob of a peer record (`decodePlayerCard` decodes it; the peer's community profile).
 * It is the peer's `NetUserProfile` (net_session_close.h) seen through the words the record copy moves: +0x05
 * the region (copied into the circle record's region row when a member joins), +0x9C the mediator record and
 * +0xF9 the in-party byte (a member in a party gets no ready/unready chat line). */
typedef struct NetPeerBlob {
    union {
        /* +0x00 */ u32 words_0x00[0x40];
        struct {
            /* +0x00 */ u8 pad_0x00[0x5];
            /* +0x05 */ u8 region_0x05;
            /* +0x06 */ u8 pad_0x06[0x96];
            /* +0x9C */ u8 record_0x9C[0x56];   /* the record the mediator is handed when the peer joins */
            /* +0xF2 */ u8 pad_0xF2[0x7];
            /* +0xF9 */ u8 in_party_0xF9;
            /* +0xFA */ u8 pad_0xFA[0x6];
        };
    };
} NetPeerBlob; /* size: 0x100 */

/* One 0x120-byte connected-peer record (`NetCtrlWk::peers_0x7488`): the id, a 0x14-byte name field and the
 * 0x100-byte character blob.  The members are separate class types so the compiler's own assignment copies
 * them one by one (it leaves the two padding bytes at +0x1E alone, as the retail copy does). */
typedef struct NetPeerRec {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ NetPeerName name_0x0A;
    /* +0x1E */ u8 pad_0x1E[0x2];
    /* +0x20 */ NetPeerBlob blob_0x20;

    void assign(const NetPeerRec* src);
} NetPeerRec; /* size: 0x120 */

/* The peer list `NetCtrlWk::copyPeerList` copies out for the caller: the count, then the four records. */
typedef struct NetPeerList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetPeerRec peers_0x004[4];
} NetPeerList; /* size: 0x484 */

/* A player's move-work record (`get_move_work_adrs(2)` hands out four), which `decodePlayerCard` fills from a peer's
 * card; only the active byte and the two strings this unit reads or writes are named.  size: 0xB20 (the array
 * stride) */
typedef struct NetPlayerCard {
    /* +0x000 */ u8 active_0x000;
    /* +0x001 */ u8 pad_0x001[0x5C9];
    /* +0x5CA */ char name_0x5CA[0x11];
    /* +0x5DB */ char id_0x5DB[0xA];
    /* +0x5E5 */ u8 pad_0x5E5[0x53B];
} NetPlayerCard; /* size: 0xB20 */

/* One 0x24-byte entry of the two filter tables at `NetCtrlWk::filters_0x8C64` (8 entries) and
 * `filters_0x8D84` (64 entries): an enabled flag, a kind (0 = always, 1 = id range, 2 = bit lookup)
 * and the range or index words the kind reads. */
typedef struct NetFilterRec {
    /* +0x00 */ u8 pad_0x00[0x1E];
    /* +0x1E */ u8 enabled_0x1E;
    /* +0x1F */ u8 kind_0x1F;
    /* +0x20 */ u16 low_0x20;
    /* +0x22 */ u16 high_0x22;
} NetFilterRec; /* size: 0x24 */

/* A u16 low/high pair (the per-server id ranges `can_enter_server` tests). */
typedef struct NetRange {
    /* +0x00 */ u16 low_0x00;
    /* +0x02 */ u16 high_0x02;
} NetRange; /* size: 0x4 */

/* One 0x40-byte entry of the server table (`NetSrvList`). */
typedef struct NetSrvRec {
    /* +0x00 */ s32 id_0x00;
    /* +0x04 */ char name_0x04[0x24];
    /* +0x28 */ s32 population_0x28;
    /* +0x2C */ s32 capacity_0x2C;
    /* +0x30 */ s32 value_0x30;
    /* +0x34 */ u8 pad_0x34[0x4];
    /* +0x38 */ u32 flags_0x38;
    /* +0x3C */ u32 type_0x3C;          /* 1-based server type, matched against `NetServerType` */
} NetSrvRec; /* size: 0x40 */

/* The server table layer command 18 delivers: the count and up to 80 servers.  size: 0x1408 */
typedef struct NetSrvList {
    /* +0x000 */ u32 count_0x000;
    /* +0x004 */ u8 pad_0x004[0x4];
    /* +0x008 */ NetSrvRec entries_0x008[80];
} NetSrvList; /* size: 0x1408 */

/* One 0xC0-byte server type (`NetCtrlWk::server_types_0x82D8`): a 0x18-byte name and a description. */
typedef struct NetServerType {
    /* +0x00 */ char name_0x00[0x18];
    /* +0x18 */ char description_0x18[0xA8];
} NetServerType; /* size: 0xC0 */

/* The type row `NetCtrlWk::getServerTypeInfo` copies out for a server type: its id, name and description. */
typedef struct NetServerTypeInfo {
    /* +0x00 */ s16 id_0x00;
    /* +0x02 */ char name_0x02[0x10];
    /* +0x12 */ char description_0x12[0xA8];
} NetServerTypeInfo; /* size: 0xBA (approximate: the caller's buffer is not sized here) */

/* The event window: the time base and the period (ms, 3000 when unset) `NetSchedule::getPhase` phases on. */
typedef struct NetSchedule {
    /* +0x00 */ u32 base_0x00;
    /* +0x04 */ u32 period_0x04;

    f32 getPhase(void);
} NetSchedule; /* size: 0x8 */

/* One 0x34-byte event entry (`NetCtrlWk::events_0x85E4`): an enabled flag and a bit mask of the
 * `NetCtrlWk::collectEvents` categories it applies to. */
typedef struct NetEventRec {
    /* +0x00 */ u8 pad_0x00[0x31];
    /* +0x31 */ u8 enabled_0x31;
    /* +0x32 */ u8 mask_0x32;
    /* +0x33 */ u8 pad_0x33;
} NetEventRec; /* size: 0x34 */

/* The three timestamps `NetRaidTimes::classifyNow` compares the game time against: the start, the warning point and
 * the end of the event window. */
typedef struct NetRaidTimes {
    /* +0x00 */ u32 start_0x00;
    /* +0x04 */ u32 warning_0x04;
    /* +0x08 */ u32 end_0x08;

    s32 classifyNow(void);
} NetRaidTimes; /* size: 0xC */

/* The error triple a failed fetch leaves (`NetworkFileFetcher::copyError` also writes it). */
typedef struct NetFetchError {
    /* +0x0 */ u32 code_0x0;
    /* +0x4 */ u32 detail_0x4;
    /* +0x8 */ u32 reason_0x8;
} NetFetchError; /* size: 0xC */

/* The 240-byte notice block the fourth fetch fills (`NetCtrlWk::getServerNotice` hands it out). */
typedef struct NetServerNotice {
    /* +0x00 */ u8 bytes_0x00[0xF0];
} NetServerNotice; /* size: 0xF0 */

/* The remote-file client the fetch state machines drive, `NetworkFileFetcher`, is declared by its owner's header
 * `Network/NetworkFileFetcher.h` (included above). */

/* The three list views the layer's list copies fill (all 0x2C or 0x30 bytes per row). */
typedef struct NetListView {
    /* +0x00 */ s32 population_0x00;
    /* +0x04 */ s32 capacity_0x04;
    /* +0x08 */ s16 id_0x08;
    /* +0x0A */ char name_0x0A[0x22];
} NetListView; /* size: 0x2C */

/* The server row `NetCtrlWk::listServers` copies out (kind is always 0x28). */
typedef struct NetServerView {
    /* +0x00 */ s32 population_0x00;
    /* +0x04 */ s32 capacity_0x04;
    /* +0x08 */ s16 kind_0x08;
    /* +0x0A */ s16 id_0x0A;
    /* +0x0C */ char name_0x0C[0x20];
} NetServerView; /* size: 0x2C */

/* The city row `NetCtrlWk::copyCityViews` copies out: population, capacity, id, order, a state (0 open, 1 closed,
 * 2 busy, 3 full), the name and the four settings. */
typedef struct NetCityView {
    /* +0x00 */ s32 population_0x00;
    /* +0x04 */ s32 capacity_0x04;
    /* +0x08 */ s16 id_0x08;
    /* +0x0A */ s16 order_0x0A;
    /* +0x0C */ s32 state_0x0C;
    /* +0x10 */ char name_0x10[0x10];
    /* +0x20 */ s32 values_0x20[4];
} NetCityView; /* size: 0x30 */

/* The four words the settings submit copies from the caller. */
typedef struct NetSettingsWords {
    /* +0x00 */ u32 word_0x00;
    /* +0x04 */ u32 word_0x04;
    /* +0x08 */ u32 word_0x08;
    /* +0x0C */ u32 word_0x0C;
} NetSettingsWords; /* size: 0x10 */

/* One 0xC-byte invite slot (`NetCtrlWk::invites_0xBF3C`): the invited name and a filled flag (GUESS on
 * the record's role: `NetCtrlWk::checkOwnInvite` compares its name with this player's own). */
typedef struct NetInviteRec {
    /* +0x00 */ char id_text_0x00[0xA];
    /* +0x0A */ u8 index_0x0A;   /* the entry's own 1-based index */
    /* +0x0B */ u8 valid_0x0B;
} NetInviteRec; /* size: 0xC */

/* One invite the layer reports (layer commands 31..35): the inviter's id, the 1-based invite index and its
 * valid byte.  size: 0x24 */
typedef struct NetInviteEntry {
    /* +0x00 */ NetworkUniqueId id_0x00;
    /* +0x20 */ u8 index_0x20;
    /* +0x21 */ u8 valid_0x21;
    /* +0x22 */ u8 pad_0x22[0x2];
} NetInviteEntry; /* size: 0x24 */

/* The invite list layer command 35 delivers: the count and up to 32 entries.  size: 0x484 */
typedef struct NetInviteList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetInviteEntry entries_0x004[32];
} NetInviteList; /* size: 0x484 */

/* The chat record the session (commands 18/19), community (10/20) and layer (15/16) callbacks deliver: the
 * sender's id object, its display name, the message text the mediator is handed and the text colour.
 * size: 0x23C (approximate: the highest byte read) */
typedef struct NetSessionChatRecord {
    /* +0x000 */ NetworkUniqueId sender_0x000;
    /* +0x020 */ char name_0x020[0x15];
    /* +0x035 */ u8 text_0x035[0x203];
    /* +0x238 */ s32 color_0x238;
} NetSessionChatRecord; /* size: 0x23C (approximate) */

/* One 0x180-byte text record of the big-data file. */
typedef struct NetBigDataRec {
    /* +0x000 */ char text_0x000[0x180];
} NetBigDataRec; /* size: 0x180 */

/* The big-data file (`NetCtrlWk::bigdata_0x96F0`): six text records and the event bits.  size: 0x904 */
typedef struct NetBigData {
    /* +0x000 */ NetBigDataRec records_0x000[6];
    /* +0x900 */ u32 event_bits_0x900;
} NetBigData; /* size: 0x904 */

/* One 0x84-byte roster entry (`NetCtrlWk::roster_0xA1B8`): the member's address object (read as an id by
 * `formatNetId`, copied through the object's +0x28 slot) and a 0x14-byte name at +0x20. */
typedef struct NetRosterRec {
    /* +0x00 */ NetworkUniqueId address_0x00;   /* 0x20 bytes */
    /* +0x20 */ char name_0x20[0x14];
    /* +0x34 */ u8 pad_0x34;
    /* +0x35 */ u8 state_0x35;   /* in a presence update (community command 22): 1 = added as a friend, 2.. = other states */
    /* +0x36 */ u8 pad_0x36[0x4E];
} NetRosterRec; /* size: 0x84 */

/* One 0x38-byte recent-player entry (`NetCtrlWk::recent_0xBB84`): the address object and a 0x14-byte name at
 * +0x20. */
typedef struct NetRecentRec {
    /* +0x00 */ NetworkUniqueId address_0x00;   /* 0x20 bytes */
    /* +0x20 */ char name_0x20[0x14];
    /* +0x34 */ u8 pad_0x34[0x4];
} NetRecentRec; /* size: 0x38 */

/* The friend roster (`NetCtrlWk::roster_0xA1B8`, and the payload community command 6 delivers): the count and
 * up to 50 entries.  size: 0x19CC */
typedef struct NetRosterList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetRosterRec entries_0x004[50];
} NetRosterList; /* size: 0x19CC */

/* The recent-player list (`NetCtrlWk::recent_0xBB84`, and the payload of community command 24): the count and
 * up to 16 entries.  size: 0x384 */
typedef struct NetRecentList {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetRecentRec entries_0x004[16];
} NetRecentList; /* size: 0x384 */

/* One row the roster copy hands out: the name and the id as text. */
typedef struct NetRosterView {
    /* +0x00 */ u8 name_0x00[0x14];
    /* +0x14 */ char id_text_0x14[0x10];
} NetRosterView; /* size: 0x24 */

/* The roster copy: the count and up to 50 rows.  size: 0x70C */
typedef struct NetRosterListView {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetRosterView rows_0x004[50];
} NetRosterListView; /* size: 0x70C */

/* One row the recent-player copy hands out: the name and the id as text. */
typedef struct NetRecentView {
    /* +0x00 */ u8 name_0x00[0x14];
    /* +0x14 */ char id_text_0x14[0xA];
} NetRecentView; /* size: 0x1E */

/* The recent-player copy: the count and up to 16 rows.  size: 0x1E4 */
typedef struct NetRecentListView {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetRecentView rows_0x004[16];
} NetRecentListView; /* size: 0x1E4 */

/* One row the friend-list copy hands out. */
typedef struct NetFriendView {
    /* +0x00 */ char name_0x00[0x14];
    /* +0x14 */ char id_text_0x14[0xA];
    /* +0x1E */ s16 area_0x1E;
    /* +0x20 */ u8 status_0x20;
    /* +0x21 */ u8 pad_0x21;
} NetFriendView; /* size: 0x22 */

/* The friend-list copy: the count and the rows (the caller sizes the run).  size: 0x4+ */
typedef struct NetFriendListView {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetFriendView rows_0x04[1];
} NetFriendListView;

/* One row the community-list copy hands out. */
typedef struct NetCommunityView {
    /* +0x00 */ u8 valid_0x00;
    /* +0x01 */ char name_0x01[0x20];
    /* +0x21 */ char leader_0x21[0x20];
    /* +0x41 */ char comment_0x41[0x13];
    /* +0x54 */ s32 value_0x54;
    /* +0x58 */ s32 value_0x58;
    /* +0x5C */ s32 value_0x5C;
    /* +0x60 */ s16 index_0x60;
    /* +0x62 */ u8 pad_0x62[0x2];
    /* +0x64 */ s32 slots_0x64[4];
} NetCommunityView; /* size: 0x74 */

/* Four slot ids the layer selection request takes (-1 = unset). */
typedef struct NetSlotIds {
    /* +0x00 */ u32 slot_0x00;
    /* +0x04 */ u32 slot_0x04;
    /* +0x08 */ u32 slot_0x08;
    /* +0x0C */ u32 slot_0x0C;
} NetSlotIds; /* size: 0x10 */

/* One 12-byte run of a laid-out message: where its text starts, the tag flags it carries (1 body, 2 size,
 * 4 colour, 8 line break, 0x10..0x80 alignment, 0x100 line feed), the colour and size indices, the line-feed
 * count and the run's length in bytes. */
typedef struct NetTextRun {
    /* +0x00 */ char* text_0x00;
    /* +0x04 */ u32 flags_0x04;
    /* +0x08 */ u8 color_0x08;
    /* +0x09 */ u8 size_0x09;
    /* +0x0A */ u8 lines_0x0A;
    /* +0x0B */ u8 length_0x0B;
} NetTextRun; /* size: 0xC */

/* The text-layout state `NetTextTagState::parseTag` parses tags out of (GUESS on the owner: a message layout record);
 * the work record's `text_layout_0xC144` is one (allocateDialogRecord allocates 0x314 bytes for it).
 * size: 0x314 */
typedef struct NetTextTagState {
    /* +0x000 */ NetTextRun runs_0x000[50];
    /* +0x258 */ char line_0x258[0x80];     /* the run being printed, NUL-terminated */
    /* +0x2D8 */ char tag_0x2D8[0x20];
    /* +0x2F8 */ NetTextRun* run_0x2F8;     /* the run being filled (layout) or printed (draw) */
    /* +0x2FC */ NetTextRun* last_0x2FC;    /* the run text is appended to */
    /* +0x300 */ s16 origin_x_0x300;
    /* +0x302 */ s16 origin_y_0x302;
    /* +0x304 */ s16 pen_x_0x304;
    /* +0x306 */ s16 pen_y_0x306;
    /* +0x308 */ char* cursor_0x308;
    /* +0x30C */ u8 mode_0x30C;          /* 2 = start a new run before the next character */
    /* +0x30D */ u8 status_0x30D;        /* 1 = laid out to the end, 2 = a broken tag */
    /* +0x30E */ u8 tag_id_0x30E;        /* the index of the last tag in text_tag_names */
    /* +0x30F */ u8 align_0x30F;         /* 5 centre, 6 left, 7 right */
    /* +0x310 */ u8 color_0x310;
    /* +0x311 */ u8 size_0x311;          /* index into text_font_size_table */
    /* +0x312 */ u16 width_0x312;        /* the characters on the current line */

    void parseTag(void);
} NetTextTagState; /* size: 0x314 */

/* The 16 bytes `NetworkLayerIdExportTo` writes for a layer id, as the layer-change checks read them: the server
 * word and the two level numbers below it (GUESS names: a zero room/city means the console stands above that
 * level).  size: 0x10 */
typedef struct NetLayerIdText {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u32 server_0x04;
    /* +0x08 */ u8 pad_0x08[0x2];
    /* +0x0A */ u16 city_0x0A;
    /* +0x0C */ u16 room_0x0C;
    /* +0x0E */ u8 pad_0x0E[0x2];
} NetLayerIdText; /* size: 0x10 */

/* One 0x5C-byte row of the login server list (`NetCtrlWk::rows_0x17C`): the server id and its name. */
typedef struct NetRowRec {
    /* +0x00 */ s32 id_0x00;
    /* +0x04 */ char name_0x04[0x58];
} NetRowRec; /* size: 0x5C */

/* One key/value pair of the roster-sync block: the presence key (1..7) and its state byte. */
typedef struct NetRosterSyncItem {
    /* +0x00 */ s32 key_0x00;
    /* +0x04 */ s8 value_0x04;
    /* +0x05 */ u8 pad_0x05[0x3];
} NetRosterSyncItem; /* size: 0x8 */

/* The roster-sync block `NetCtrlWk::roster_sync_0x61CC` (built by `buildRosterSync`, sent by
 * `NetworkCommunityPat::syncFriends`): the pair count and up to eight pairs.  size: 0x44 */
struct NetRosterSync {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetRosterSyncItem items_0x04[8];
};

/* An 8-byte member of the Pat settings block (an s64 wrapped in a record: the wrapper is what gives the
 * aggregate copy its high-word-first move order). */
#pragma pack(push, 4)
typedef struct PatSettingsPair {
    /* +0x00 */ s64 value_0x00;
} PatSettingsPair; /* size: 0x8 */

/* The 0x40-byte Pat settings block the work record keeps at +0xC208 and the network save carries.  The
 * member split is the one `copyPatSettings`'s aggregate copy reveals (single words and the 8-byte s64 moves;
 * packed to 4 so the s64 members keep their offsets); the meaning of each member is not known. */
typedef struct PatSettings {
    /* +0x00 */ u32 word_0x00;
    /* +0x04 */ PatSettingsPair pair_0x04;
    /* +0x0C */ u32 word_0x0C;
    /* +0x10 */ PatSettingsPair pair_0x10;
    /* +0x18 */ u32 word_0x18;
    /* +0x1C */ u32 word_0x1C;
    /* +0x20 */ u32 word_0x20;
    /* +0x24 */ u32 word_0x24;
    /* +0x28 */ PatSettingsPair pair_0x28;
    /* +0x30 */ PatSettingsPair pair_0x30;
    /* +0x38 */ u32 word_0x38;
    /* +0x3C */ u32 word_0x3C;
} PatSettings; /* size: 0x40 */
#pragma pack(pop)

/* The network save record `exportNetworkSave`/`importNetworkSave` move to and from the work record.
 * size: 0x88 (approximate: the highest byte written) */
typedef struct NetSaveRecord {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ s32 terms_version_0x04;
    /* +0x08 */ PatSettings settings_0x08;
    /* +0x48 */ char support_code_0x48[0x20];
    /* +0x68 */ char name_0x68[0xB];
    /* +0x73 */ s8 byte_0x73;
    /* +0x74 */ u8 pad_0x74[0x4];
    /* +0x78 */ s32 words_0x78[4];
} NetSaveRecord; /* size: 0x88 (approximate) */

/* An error triple the layer leaves: the error code, its detail and the server's reason word.  size: 0xC */
typedef struct NetErrorTriple {
    /* +0x0 */ u32 code_0x0;
    /* +0x4 */ s32 detail_0x4;
    /* +0x8 */ s32 reason_0x8;
} NetErrorTriple; /* size: 0xC */

/* The pending session-manager request (`NetCtrlWk::session_request_0x829C`): a busy word, the command, the
 * completion handler `sessionReflectCallback` calls with the status and the four result values, and two
 * state bytes (`ok_0x21` = the request succeeded). */
typedef struct NetSessionRequest {
    /* +0x00 */ s32 busy_0x00;
    /* +0x04 */ s32 command_0x04;
    /* +0x08 */ void (*done_0x08)(s32 status, s32* values);
    /* +0x0C */ s32 status_0x0C;
    /* +0x10 */ s32 values_0x10[4];
    /* +0x20 */ u8 pending_0x20;
    /* +0x21 */ u8 ok_0x21;
    /* +0x22 */ u8 pad_0x22[0x2];
} NetSessionRequest; /* size: 0x24 */

/* The confirm-dialog record `NetCtrlWk::dialog_0xBF30` points at; only the four bytes and the counter
 * the state machine drives are named.  size: 0x12A (approximate) */
typedef struct NetDialog {
    /* +0x000 */ u8 pad_0x000[0x124];
    /* +0x124 */ u8 flag_0x124;
    /* +0x125 */ u8 flag_0x125;
    /* +0x126 */ u8 mode_0x126;
    /* +0x127 */ u8 value_0x127;
    /* +0x128 */ s16 timer_0x128;
} NetDialog; /* size: 0x12A (approximate) */

/* One row of the type list the menu shows (`NetListMenu::rows_0x00C`): the id, the name and its description. */
typedef struct NetMenuRow {
    /* +0x00 */ s16 id_0x00;
    /* +0x02 */ char name_0x02[0x10];
    /* +0x12 */ char description_0x12[0x80];
} NetMenuRow; /* size: 0x92 */

/* The list-menu record `NetCtrlWk::list_0xBF34` points at: a header, the eight type rows, the
 * server-page view and the city-page view (each with its own cursor, count and page).  size: 0x76C */
typedef struct NetListMenu {
    /* +0x000 */ u8 mode_0x000;
    /* +0x001 */ u8 pad_0x001[0x5];
    /* +0x006 */ s16 field_0x006;
    /* +0x008 */ s16 count_0x008;
    /* +0x00A */ s16 cursor_0x00A;
    /* +0x00C */ NetMenuRow rows_0x00C[8];
    /* +0x49C */ s16 cursor_0x49C;
    /* +0x49E */ s16 count_0x49E;
    /* +0x4A0 */ s16 page_0x4A0;
    /* +0x4A2 */ s16 pages_0x4A2;
    /* +0x4A4 */ NetServerView views_0x4A4[8];
    /* +0x604 */ s16 cursor2_0x604;
    /* +0x606 */ s16 count2_0x606;
    /* +0x608 */ s16 page2_0x608;
    /* +0x60A */ s16 pages2_0x60A;
    /* +0x60C */ NetListView views2_0x60C[8];
} NetListMenu; /* size: 0x76C */

/* The per-member circle records the session manager publishes (`NetCtrlWk::circle_records_0xC3F4`, sent with
 * `setCircleRecords`): each member's id text and display name (written when the member joins, cleared when
 * it leaves) and its region byte (`getUserRegion` for this player, the peer card's +0x05 for the others).
 * size: 0x88 */
typedef struct NetCircleRecords {
    /* +0x00 */ char id_text_0x00[4][0x10];
    /* +0x40 */ char name_0x40[4][0x10];
    /* +0x80 */ u8 region_0x80[4];
    /* +0x84 */ u8 pad_0x84[0x4];
} NetCircleRecords; /* size: 0x88 */

/* The user record `get_userdata` hands out, seen only as the name at +0x03 the band copies. */
typedef struct NetUserData {
    /* +0x00 */ u8 pad_0x00[0x3];
    /* +0x03 */ char name_0x03[0x20];
    /* +0x23 */ u8 pad_0x23[0x3DC1];
    /* +0x3DE4 */ u16 hunter_rank_0x3DE4;   /* compared with the servers' and filters' rank ranges */
} NetUserData; /* size: 0x3DE6 (approximate: the last field read) */
/*@TYPES end@*/

/* The server configuration the fetch steps fill (`initWorkRecord` clears the whole 0x140C bytes at once): the four
 * server types, the schedule, the event table, the two filter tables, the per-type rank ranges and the timeouts.
 * size: 0x140C */
typedef struct NetServerConfig {
    /* +0x0000 */ NetServerType server_types_0x000[4];
    /* +0x0300 */ u8 pad_0x300[0x4];
    /* +0x0304 */ NetSchedule schedule_0x304;
    /* +0x030C */ NetEventRec events_0x30C[32];
    /* +0x098C */ NetFilterRec filters_0x98C[8];
    /* +0x0AAC */ NetFilterRec filters_0xAAC[64];
    /* +0x13AC */ NetRange ranges_0x13AC[4];
    /* +0x13BC */ u16 timeouts_0x13BC[40];
} NetServerConfig; /* size: 0x140C */

/* The network-control work record reached through `net_ctrl_wk`.  `entries_0x7CD8` is 16 records
 * of 0x5C; the tail offsets (+0x82C4, +0xBF2C, +0xC3F2, ...) are separate flags/counters. */
/* Packed: the record carries a `u32` run at the odd offset +0x7996 (`slots_0x7996`), and natural
 * alignment would slide every later field by four bytes. */
#pragma pack(push, 1)
typedef struct NetCtrlWk {
/*@NetCtrlWk begin@*/
    /* +0x000 */ NetworkPat* pats_0x000;   /* the Pat holder `net_pats_object` (initNetworkPatControl) */
    /* +0x004 */ NetworkSessionManagerPat* session_manager_0x004;  /* built by initNetworkPatControl */
    /* +0x008 */ NetworkLayerPat* layer_0x008;
    /* +0x00C */ NetworkCommunityPat* community_0x00C;
    /* +0x010 */ u8 mode_0x010;
    /* +0x011 */ u8 state_0x011;
    /* +0x012 */ u8 flag_0x012;
    /* +0x013 */ u8 flag_0x013;
    /* +0x014 */ u8 entered_0x014;
    /* +0x015 */ u8 flag_0x015;
    /* +0x016 */ u8 flag_0x016;
    /* +0x017 */ u8 sub_state_0x017;
    /* +0x018 */ u8 step_0x018;
    /* +0x019 */ u8 substep_0x019;
    /* +0x01A */ u8 substep_0x01A;
    /* +0x01B */ u8 pad_0x01B;
    /* +0x01C */ s32 cursor_0x01C;
    /* +0x020 */ s32 cursor_0x020;
    /* +0x024 */ s32 scroll_0x024;
    /* +0x028 */ s32 cursor_0x028;
    /* +0x02C */ s32 repeat_0x02C;
    /* +0x030 */ s32 repeat_0x030;
    /* +0x034 */ s32 repeat_0x034;
    /* +0x038 */ s32 repeat_0x038;
    /* +0x03C */ s32 field_0x03C;
    /* +0x040 */ u8 server_slot_state_0x040[4];
    /* +0x044 */ s32 ready_count_0x044;
    /* +0x048 */ s32 leave_0x048;      /* 1 while a leave is pending (the 0x1E request's completion tests it) */
    /* +0x04C */ s32 flag_0x04C;
    /* +0x050 */ s32 error_0x050;
    /* +0x054 */ s32 error_0x054;
    /* +0x058 */ s32 error_0x058;
    /* +0x05C */ s32 error_0x05C;
    /* +0x060 */ s32 error_0x060;
    /* +0x064 */ s32 layer_state_0x064;
    /* +0x068 */ u8 flag_0x068;
    /* +0x069 */ u8 flag_0x069;
    /* +0x06A */ s8 selected_server_0x06A;   /* this player's slot, -1 = none */
    /* +0x06B */ s8 flag_0x06B;
    /* +0x06C */ s8 flag_0x06C;
    /* +0x06D */ u8 pad_0x06D[0x3];
    /* +0x070 */ s32 field_0x070;
    /* +0x074 */ s8 server_index_0x074[4];
    /* +0x078 */ s32 results_0x078[44];   /* one result word per session command (sessionReflectCallback) */
    /* +0x128 */ NetFetchError community_error_0x128;   /* the community layer's last error triple (initWorkRecord clears the 0x3C run from here) */
    /* +0x134 */ NetErrorTriple net_error_0x134;   /* the layer's last error (getNetErrorMessageId maps it) */
    /* +0x140 */ NetErrorTriple session_error_0x140;   /* the session layer's last error (MH3GetSessionErrorCode maps it) */
    /* +0x14C */ NetFetchError reflect_error_0x14C;
    /* +0x158 */ u8 pad_0x158[0xC];
    /* +0x164 */ s32 profile_count_0x164;
    /* +0x168 */ NetProfileRec* profiles_0x168;
    /* +0x16C */ s32 field_0x16C;
    /* +0x170 */ s32 profile_index_0x170;
    /* +0x174 */ s32 status_0x174;
    /* +0x178 */ u32 row_count_0x178;
    /* +0x17C */ NetRowRec rows_0x17C[8];
    /* +0x45C */ u32 page_count_0x45C;
    /* +0x460 */ u8 page_records_0x460[8][0x40];
    /* +0x660 */ NetSrvList server_list_0x660;   /* layer command 18 copies it whole */
    /* +0x1A68 */ u8 layer_block_0x1A68[0x74];   /* the 0x74-byte block layer command 8 delivers */
    /* +0x1ADC */ NetFriendTable friends_0x1ADC;   /* the layer's members (layer command 11 sets the count; resetMessagePool rebuilds from it) */
    /* +0x30C0 */ NetFriendSession friend_sessions_0x30C0[100];   /* each member's session record (the peer import copies them) */
    /* +0x3ED0 */ NetSlot* slot_list_0x3ED0;
    /* +0x3ED4 */ NetSlot slots_0x3ED4[100];
    /* +0x6134 */ NetUserPosition position_0x6134;   /* the position `sendUserPosition` publishes */
    /* +0x614C */ NetworkLayerId room_id_0x614C;   /* the room `readRoomHeader_C8` copies out before a jump into it */
    /* +0x618C */ NetworkLayerId layer_id_0x618C;   /* the layer id `saveLayerId` keeps for the change checks */
    /* +0x61CC */ NetRosterSync roster_sync_0x61CC;
    /* +0x6210 */ u32 settings_0x6210[4];
    /* +0x6220 */ u16 layer_stack_0x6220[16];   /* the layer ids entered (command 5 pushes, command 4 pops) */
    /* +0x6240 */ s32 layer_depth_0x6240;
    /* +0x6244 */ s32 layer_results_0x6244[41];   /* one result word per layer command (installLayerCallback clears them) */
    /* +0x62E8 */ s32 peer_count_0x62E8;
    /* +0x62EC */ s32 member_total_0x62EC;   /* the layer's friend count after a user-list read */
    /* +0x62F0 */ u8 pad_0x62F0[0x20];
    /* +0x6310 */ u32 sizes_0x6310[4];   /* 0x2E0 / 0x200 / 0x2260 / 0 after init */
    /* +0x6320 */ union {
        NetPoolEntry pool_0x6320[128];
        NetCommandPool command_pool_0x6320;   /* the same records seen from the pool head: the queued commands */
    };
    /* +0x7328 */ u8 pad_0x7328[0x40];
    /* +0x7368 */ char name_0x7368[0xA];
    /* +0x7372 */ char name2_0x7372[0xA];
    /* +0x737C */ u8 pad_0x737C[0x10C];
    /* +0x7488 */ union {
        u8 msgTable_0x7488[0x480];   /* the byte view `Network/network_pat_control.cpp` indexes */
        NetPeerRec peers_0x7488[4];
    };
    /* +0x7908 */ NetworkUniqueId peer_addresses_0x7908[4];   /* the peers' address objects (the record's ctor builds them) */
    /* +0x7988 */ u8 used_0x7988[4];
    /* +0x798C */ u8 pad_0x798C[0x4];
    /* +0x7990 */ u8 transfer_flag_0x7990;   /* handed to the mediator (0x804172DC) */
    /* +0x7991 */ u8 transfer_flag_0x7991;   /* handed to the mediator (0x804172CC) */
    /* +0x7992 */ u8 transfer_flag_0x7992;
    /* +0x7993 */ u8 transfer_level_0x7993;
    /* +0x7994 */ u8 peer_total_0x7994;
    /* +0x7995 */ u8 peer_cards_changed_0x7995;   /* cleared once a peer's card block was refreshed (community command 11) */
    /* +0x7996 */ u32 slots_0x7996[0x40];
    /* +0x7A96 */ u8 pad_0x7A96[0x2];
    /* +0x7A98 */ u32* arrA_0x7A98[0x40];
    /* +0x7B98 */ u32 arrB_0x7B98[0x40];
    /* +0x7C98 */ u8 arrC_0x7C98[0x40];
    /* +0x7CD8 */ NetCtrlEntry entries_0x7CD8[16];
    /* +0x8298 */ u8 pad_0x8298[0x4];
    /* +0x829C */ NetSessionRequest session_request_0x829C;
    /* +0x82C0 */ s32 join_result_0x82C0;   /* the join command's (4) result word, copied by the session callback */
    /* +0x82C4 */ u8 flag_0x82C4;
    /* +0x82C5 */ u8 flag_0x82C5;
    /* +0x82C6 */ u8 flag_0x82C6;
    /* +0x82C7 */ u8 flag_0x82C7;
    /* +0x82C8 */ u8 flag_0x82C8;
    /* +0x82C9 */ u8 pad_0x82C9[0x3];
    /* +0x82CC */ NetworkFileFetcher* fetcher_0x82CC;
    /* +0x82D0 */ u32 fetch_step_0x82D0;
    /* +0x82D4 */ s32 fetch_index_0x82D4;
    /* +0x82D8 */ NetServerConfig config_0x82D8;
    /* +0x96E4 */ NetRaidTimes times_0x96E4;
    /* +0x96F0 */ NetBigData bigdata_0x96F0;
    /* +0x9FF4 */ NetServerNotice notice_0x9FF4;
    /* +0xA0E4 */ u32 fetch_sums_0xA0E4[4];
    /* +0xA0F4 */ NetFetchError fetch_error_0xA0F4;
    /* +0xA100 */ s32 type_ids_0xA100[4];
    /* +0xA110 */ s32 group_count_0xA110;
    /* +0xA114 */ s32 selected_server_index_0xA114;
    /* +0xA118 */ s32 field_0xA118;
    /* +0xA11C */ s32 chosen_type_0xA11C;
    /* +0xA120 */ s32 field_0xA120[4];
    /* +0xA130 */ u8 pad_0xA130[0x4];
    /* +0xA134 */ s32 chosen_city_0xA134;
    /* +0xA138 */ s32 pages_0xA138;
    /* +0xA13C */ s32 per_page_0xA13C;
    /* +0xA140 */ s32 total_0xA140;
    /* +0xA144 */ s32 community_results_0xA144[27];   /* one result word per community command (communityReflectCallback) */
    /* +0xA1B0 */ s32 cmd4_value_0xA1B0;   /* the word community command 4 reports */
    /* +0xA1B4 */ s32 cmd5_value_0xA1B4;   /* the word community command 5 reports */
    /* +0xA1B8 */ NetworkCommunityFriendList roster_0xA1B8;   /* the friend roster (built and destroyed out of line) */
    /* +0xBB84 */ NetworkCommunityBlockList recent_0xBB84;    /* the recent-player list (likewise) */
    /* +0xBF08 */ char message_0xBF08[0x24];
    /* +0xBF2C */ u8 flag_0xBF2C;
    /* +0xBF2D */ u8 flag_0xBF2D;
    /* +0xBF2E */ u8 flag_0xBF2E;
    /* +0xBF2F */ u8 pad_0xBF2F[0x1];
    /* +0xBF30 */ NetDialog* dialog_0xBF30;
    /* +0xBF34 */ NetListMenu* list_0xBF34;
    /* +0xBF38 */ s32 invite_count_0xBF38;
    /* +0xBF3C */ NetInviteRec invites_0xBF3C[32];
    /* +0xC0BC */ s8 link_state_0xC0BC;
    /* +0xC0BD */ u8 flag_0xC0BD;
    /* +0xC0BE */ u8 pad_0xC0BE[0x2];
    /* +0xC0C0 */ s32 link_id_0xC0C0;   /* compared with layer command 37's word (a match clears flag_0xC0BD) */
    /* +0xC0C4 */ char account_name_0xC0C4[0x40];
    /* +0xC104 */ char nickname_0xC104[0x40];
    /* +0xC144 */ NetTextTagState* text_layout_0xC144;   /* allocateDialogRecord: work_mem_alloc(0x314) */
    /* +0xC148 */ s32 profile_write_0xC148;   /* the profile-write request word the community writes take (always 2) */
    /* +0xC14C */ u8 pad_0xC14C[0x4];
    /* +0xC150 */ u8 screen_0xC150;
    /* +0xC151 */ u8 flag_0xC151;
    /* +0xC152 */ u8 action_0xC152;
    /* +0xC153 */ u8 start_done_0xC153;   /* set by the session close's completion, read by isSessionStartDone */
    /* +0xC154 */ s32 field_0xC154;
    /* +0xC158 */ u8 screen_0xC158;
    /* +0xC159 */ u8 screen_0xC159;
    /* +0xC15A */ u8 pad_0xC15A[0x63];
    /* +0xC1BD */ s8 flag_0xC1BD;
    /* +0xC1BE */ u8 flag_0xC1BE;
    /* +0xC1BF */ u8 flag_0xC1BF;
    /* +0xC1C0 */ u8 flags_0xC1C0[0x4];
    /* +0xC1C4 */ u8 pad_0xC1C4[0x4];
    /* +0xC1C8 */ char player_name_0xC1C8[0xB];
    /* +0xC1D3 */ char save_name_0xC1D3[0xB];   /* the name the network save keeps (exportNetworkSave) */
    /* +0xC1DE */ char support_code_0xC1DE[0x20];
    /* +0xC1FE */ u8 pad_0xC1FE[0x2];
    /* +0xC200 */ s32 terms_version_0xC200;
    /* +0xC204 */ s8 save_byte_0xC204;
    /* +0xC205 */ u8 pad_0xC205[0x3];
    /* +0xC208 */ PatSettings pat_settings_0xC208;   /* handed to the Pat holder by initNetworkPatControl */
    /* +0xC248 */ s32 save_words_0xC248[4];
    /* +0xC258 */ s8 error_code_0xC258;
    /* +0xC259 */ s8 sub_error_0xC259;
    /* +0xC25A */ u8 pad_0xC25A[0x2];
    /* +0xC25C */ NetDialog* (*create_dialog_0xC25C)(NetCtrlWk* work);
    /* +0xC260 */ u8 pad_0xC260[0x4];
    /* +0xC264 */ void (*result_callback_0xC264)(void);
    /* +0xC268 */ void (*callback_0xC268)(void);
    /* +0xC26C */ u8 pad_0xC26C[0x4];
    /* +0xC270 */ void (*callback_0xC270)(void);
    /* +0xC274 */ void (*callback_0xC274)(void);
    /* +0xC278 */ void (*callback_0xC278)(void);
    /* +0xC27C */ s32 (*callback_0xC27C)(void);
    /* +0xC280 */ void (*callback_0xC280)(void);
    /* +0xC284 */ s32 (*callback_0xC284)(void);
    /* +0xC288 */ s32 (*query_0xC288)(void);   /* initNetworkPatControl stores its answer at +0xC28C */
    /* +0xC28C */ s32 field_0xC28C;
    /* +0xC290 */ s8* result_0xC290;
    /* +0xC294 */ NetLayerRequest layer_request_0xC294;   /* the record's constructor 0x8043202C builds it (0x803E1230) */
    /* +0xC300 */ s32 msg_state_0xC300;
    /* +0xC304 */ s32 view_key_0xC304;     /* compared with seen_key_0xC310 (GUESS on the pair's role) */
    /* +0xC308 */ s32 view_key_0xC308;     /* compared with seen_key_0xC314 */
    /* +0xC30C */ s32 view_key_0xC30C;
    /* +0xC310 */ s32 seen_key_0xC310;
    /* +0xC314 */ s32 seen_key_0xC314;
    /* +0xC318 */ s32 seen_key_0xC318;
    /* +0xC31C */ u8 pad_0xC31C[0x24];
    /* +0xC340 */ NetworkUniqueId request_id_0xC340;   /* the record's constructor 0x8043202C builds it */
    /* +0xC360 */ u16 idle_hold_0xC360;     /* pad 0's hold word last frame (refreshServerScreen's idle check) */
    /* +0xC362 */ u16 idle_press_0xC362;    /* pad 0's press word last frame */
    /* +0xC364 */ s32 idle_frames_0xC364;   /* frames both stayed unchanged (36000 raises error 23) */
    /* +0xC368 */ s8 flag_0xC368;
    /* +0xC369 */ u8 flag_0xC369;
    /* +0xC36A */ u8 pad_0xC36A[0x2];
    /* +0xC36C */ s32 timeout_0xC36C;
    /* +0xC370 */ s32 field_0xC370;
    /* +0xC374 */ s32 field_0xC374;
    /* +0xC378 */ u8 flag_0xC378;
    /* +0xC379 */ u8 flag_0xC379;     /* cleared when the session join marks this player's slot */
    /* +0xC37A */ u8 pad_0xC37A[0x2];
    /* +0xC37C */ s32 fetch_timeouts_0xC37C[8];
    /* +0xC39C */ u8* file_buffers_0xC39C[10];
    /* +0xC3C4 */ u32 file_sums_0xC3C4[10];
    /* +0xC3EC */ u8* staging_0xC3EC;
    /* +0xC3F0 */ u8 flag_0xC3F0;
    /* +0xC3F1 */ s8 profile_result_0xC3F1;   /* `requestPeerProfileById`'s answer for a pending layer entry */
    /* +0xC3F2 */ u8 flag_0xC3F2;
    /* +0xC3F3 */ u8 pad_0xC3F3[0x1];
    /* +0xC3F4 */ NetCircleRecords circle_records_0xC3F4;   /* handed to the session manager (setCircleRecords) */
    /* +0xC47C */ NetFriendList* friend_list_0xC47C;
    /* +0xC480 */ u8 flag_0xC480;
    /* +0xC481 */ u8 pad_0xC481[0x3];
    /* +0xC484 */ s32 timeout_0xC484;
    /* +0xC488 */ s16 delay_0xC488;
    /* +0xC48A */ u8 flag_0xC48A;
    /* +0xC48B */ u8 pad_0xC48B[0x1];
    /* +0xC48C */ s32 hold_0xC48C;
    /* +0xC490 */ s32 group_max_0xC490;
    /* +0xC494 */ s32 group_0xC494;
    /* +0xC498 */ s8 field_0xC498;   /* set once layer command 40 has answered */
    /* +0xC499 */ u8 flag_0xC499;
    /* +0xC49A */ u8 flag_0xC49A;
    /* +0xC49B */ u8 flag_0xC49B;
    /* +0xC49C */ u8 flag_0xC49C;
    /* +0xC49D */ u8 flag_0xC49D;
    /* +0xC49E */ u8 flag_0xC49E;
    /* +0xC49F */ u8 flag_0xC49F;
    /* +0xC4A0 */ s32 field_0xC4A0;
    /* +0xC4A4 */ u8 flags_0xC4A4[4];

    /* Members of the work record (each is a function of the band, in address order). */
    static void updateIfActive(void);
    static BOOL isServerSelectSubState(void);
    static u8 getSelectedServer(void);
    static BOOL isSessionManagerReady(void);
    static u8 getLowestServerIndex(void);
    static void flushSession(void);
    static BOOL hasServerIndex(u32 server_index);
    static u8 countValidServerIndexes(void);
    static BOOL isCityModeAlias(void);
    static NetCtrlEntry* reserveEntry(void);
    static BOOL isSessionFlagSet(void);
    static void clearSessionFlag(void);
    static BOOL postEntry(u8 kind, char* name, u32 value, u8 flags);
    static BOOL postEntryWithText(u8 kind, char* name, u32 value, u8 flags, char* text);
    static BOOL restartSession(void);
    static BOOL isRequestStateTwo(void);
    static void setRequestStateOne(void);
    static BOOL loadPeerCard(NetPlayerCard* card, u8 slot);
    static char* findPeerName(const NetworkUniqueId* id);
    static BOOL isConnectionSettled(void);
    static BOOL pollBigDataFetch(void);
    static BOOL pollNoticeFetch(void);
    static void raiseRequestFlag(void);
    static BOOL passesFirstFilter(s32 index, u16 value);
    s32 stepFetch(s32 kind);
    static u32 getNetworkTime(void);
    static BOOL getServerTypeInfo(s32 type, NetServerTypeInfo* info);
    static s32 listServers(s32 type, NetServerView* views, s32 skip, s32 max);
    static s32 getRoomPageCount(s32 per_page);
    static s32 copyRoomViews(NetListView* views, u32 first, s32 max);
    static s32 copyCityListViews(NetListView* views, u32 first, s32 max);
    static s32 getCityPageCount(s32 per_page);
    static s32 getCityCount(void);
    static s32 copyCityViews(NetCityView* views, u32 first, s32 max, u8 keep_counts);
    static void raiseSecondRequestFlag(void);
    static void requestListRefresh(s8* result);
    static void requestListRefreshAlias(s8* result);
    static s32 requestEnterCity(s8* result, s32 index, u8 forced);
    static s32 requestCommand08(s8* result);
    static s32 requestCommand07(s8* result);
    static void requestCommand0E(s8* result, s32 value);
    static void requestCommand0F(s8* result, s32 value);
    static void requestCommand17(s8* result, s32 value);
    static void requestCommand18(const s32* args, s8* result);
    static void requestSettingsCommand(s8* result, s32 value, const NetSettingsWords* settings);
    static void requestCommand12(s8* result, s32 value);
    static s32 copyPeerList(NetPeerList* list);
    static s32 collectEvents(s8* out, u8 category, s32 max);
    static s32 collectSecondFilters(s8* out, u16 value, const u16* table_a, const u16* table_b, s32 table_count, s32 max);
    static s32 collectFirstFilters(s8* out, u16 value, s32 max);
    static NetEventRec* getEvent(s32 index);
    static NetFilterRec* getFirstFilter(s32 index);
    static NetFilterRec* getSecondFilter(s32 index);
    static s32 checkOwnInvite(u8 id, s8* result);
    static BOOL isAccountLinked(void);
    static char* getSelectedServerTypeName(void);
    static char* getSelectedServerName(void);
    static char* getAccountNameOrBlank(void);
    static char* getNicknameOrBlank(void);
    static char* getBigDataText(u8 index);
    static void updateLobbyEventFlags(void);
    static NetServerNotice* getServerNotice(void);
    static s32 getSelectedServerTypeIndex(void);
    static u8 getLobbyReadyByte(void);
    static BOOL hasSubError(void);
    static void clearSubError(void);
    static void setSubError(s8 code);
    static char* getErrorMessage(void);
    static u8 getLayerModeByte(void);
    static void setLayerModeByte(s32 value);
    static f32 getSchedulePhase(void);
    static u8 getLayerSetting(s32 index);
    static void copyRosterLists(NetRosterListView* roster, NetRecentListView* recent);
    static BOOL sendFriendRequest(const NetId* id, const char* message, s8* result);
    static BOOL acceptFriendRequest(const NetId* id, s8* result);
    static BOOL removeFriend(const NetId* id, s8* result);
    static BOOL inviteFriend(const NetId* id, s32 kind, s8* result);
    static BOOL submitTextSelect(const char* text, s8* result);
    static BOOL submitIdSelect(const NetId* id, s8* result);
    static BOOL startLayerRefresh(s8* result);
    static BOOL submitSlotRequest(const NetSlotIds* ids, s8* result);
    static void copyFriendList(NetFriendListView* out, u8 use_own);
    static s32 copyCommunityViews(NetCommunityView* views, s16 first, s32 max, s16* total);
    static BOOL hasFriendDetail(const NetId* id);
    static void setFetchMode(u32 mode);
    s32 stepFileDownloads(void);
    static void copyStaging(u8* dst);
    s32 stepStagingDownload(s32 file, u16 version);
    static s32 stepStagingDownloadIfUp(s32 file, u32 version);

    /* 0x8043202C / 0x80431EFC - build and destroy the member objects (inline: emitted after the static init). */
    NetCtrlWk();
    ~NetCtrlWk();
/*@NetCtrlWk end@*/
} NetCtrlWk; /* size: 0xC4A8 (the .bss instance net_ctrl_work) */
#pragma pack(pop)

/* The work-record singleton pointer (`.sbss` 0x80794CF8) and the record it points at (`.bss` 0x806D3790),
 * both defined by `Network/network_pat_control.cpp`. */
extern NetCtrlWk* net_ctrl_wk;
extern NetCtrlWk net_ctrl_work;
/* The network heap (`.sbss` 0x80794CF4, a MEM expanded-heap handle). */
extern MEMiHeapHead* net_exp_heap;

/* The network facade the band drives. */
/* The band's entry points that keep C linkage in the map (`can_enter_server` and its siblings are the
 * retail names; `updateNetworkPatControl` is the per-frame update). */
void updateNetworkPatControl(void);
BOOL can_enter_server(s32 index, u32 value);
char* get_server_type_name(s32 index);
char* get_server_type_desc(s32 index);
u16 get_server_big_data_timeout_element(s32 index);
s32 CalculateEvents(void);
/* The band's functions other units call by name (each still carries its own local declaration). */
u32 isServerSelectState(void);
u32 isReadyCountOne(void);
/* untyped: byte range - the command record the session manager sends to the players */
void broadcastSessionCommand(void* cmd, u32 size);
s32 isServerSlotOccupied(u8 index);
s8 countOccupiedServerSlots(void);
s32 isCityMode(void);
s32 checkOtherInvite(u8 id, s8* result);
void setErrorCode(s8 code);
/* `getPatsObject` (0x803DA020) and `isNetworkSessionManagerPatReady` (0x803DF1A8) are declared in their owner's
 * header, `Network/NetworkSessionManagerPat.h`. */

#ifdef __cplusplus
}
#endif

/* Mangled callees are declared with their real signatures, never with the map's mangling
 * (docs/plan.md 6.5 rule 9): the C++ front-end emits the map's name itself.  `sysSE_req` (`__Fl`) has
 * no registered owner; `flfntStrLen` (`__FPc`, 0x8005B874) is owned by `font/flfnt.cpp`, so it is
 * declared in that unit's header (`font/flfnt.h`) and included where it is called (rule 2) -
 * `font/flfnt.h` and this header declare no other symbol in common, so a TU may include both. */

/* The band's C++-linkage helpers (their map names are the compiler's manglings). */
s32 getUtf8CharLength(u8* bytes);
s32 getPrintedWidth(char* text);
void setTextColor(u8 index);
void setTextSize(s16 size);
void printTextRuns(s16 x, s16 y, s32 unused, char* text);
char* getOnlineSupportCode(void);
char* get_network_sub_error_msg(void);
/* 0x80431194 / 0x80431304 - prints an error message (returns the final pen y); the message of an error code. */
s16 MH3DispErrorString(s16 x, s16 y, s8* text);
char* MH3GetErrorString2(s32 code);

/* The 0x80423E74..0x80429B94 band's declarations: the PatCamellia wrapper over the retail Camellia cipher and the
 * work record's arena vectors, slot table and message pool. */
#ifdef __cplusplus
extern "C" {
#endif

/* Camellia's key schedule (owner: src/Camellia/camellia.c, whose vendor header `Camellia/camellia.h` declares it
 * too; including that header instead is an open rule-2 item).  `PatCamelliaKey` is the vendor's `KEY_TABLE_TYPE`. */
typedef unsigned int PatCamelliaKey[68];

void Camellia_Ekeygen(int keyBitLength, const unsigned char* rawKey, PatCamelliaKey keyTable);
void Camellia_EncryptBlock(int keyBitLength, const unsigned char* plaintext,
                           const PatCamelliaKey keyTable, unsigned char* cipherText);
void Camellia_DecryptBlock(int keyBitLength, const unsigned char* cipherText,
                           const PatCamelliaKey keyTable, unsigned char* plaintext);

/* The unit's Camellia key schedule (.bss 0x806D3670, 0x110 B), the 64 x 0x400 arena base (.sbss 0x80794CEC)
 * and the frame stamp of the last layer re-entry (.sbss 0x80794CE8, layer command 5): the peer-join chat line
 * is skipped in the frame the stamp was taken. */
extern PatCamelliaKey lbl_806D3670;
extern u8* net_arena_base;
extern u32 net_peer_join_stamp;

/* 0x80423E74 / 0x80423E88 / 0x8042402C - the PatCamellia wrapper `PatConnection` calls: build the 256-bit key
 * schedule from `rawKey` (a tail call into `Camellia_Ekeygen`; name a GUESS in the scheme of `PatCryptDecrypt`),
 * and encrypt / decrypt `*len` bytes of `buf` in place (`encryptBuffer` logs "PatCryptEncrypt fail" on the first's
 * failure, which names it; `PatCryptDecrypt` is the map's own). */
void PatCryptSetKey(const u8* rawKey);
s32 PatCryptEncrypt(u8* buf, u16* len);
s32 PatCryptDecrypt(u8* buf, u16* len);

/* The band's own entry points this neighbour calls (owner: src/Network/network_pat_control.cpp, rule 2). */
void resetNetSlots(NetCtrlWk* work);
/* 0x80427868 - copies `size` bytes of a peer's card block (at `offset`) from a community update into the peer
 * record whose id is `id` (GUESS name: community command 13 hands it the payload's id, bytes, size and offset). */
s32 updatePeerCardBlock(const NetworkUniqueId* id, const u8* src, u32 size, s32 offset);
/* 0x80424444 / 0x804244D8 / 0x804245C8 / 0x804246F8 - the arena blocks (hand one out, set its value word), the
 * slot table reset and the slot lookup by owner (GUESS names from the bodies). */
u32* allocArenaBlock(void);
void setArenaBlockValue(u32 block, u32 value);
void resetSlotTable(void);
NetSlot* findSlotByOwner(void* owner /* untyped: opaque handle - the owner pointer the slot was handed */);
/* 0x80427024 / 0x80427240 / 0x80428628 - reset the peer table, find a free peer-event word, copy the layer's
 * friend list into the work record (GUESS names from the bodies). */
void resetPeerTable(void);
/* 0x80427C00 (GUESS) - rebuilds the peer table and `list` from the layer's members; the member total. */
u8 importLayerPeers(NetPeerList* list);
u32* findFreePeerEvent(void);
void refreshFriendList(void);
/* 0x80427284 - queues a network command (1 = accepted): the command id, the caller's result byte, an
 * unused word, the argument count and the argument words (at most four). */
s32 queueNetCommand(u32 command, s8* result, s32 unused, s32 arg_count, const s32* args);
void syncScheduleClock(NetCtrlWk* work);

/* The layer facade both network units drive.  `getNetworkLayerPat` and the holder type it takes are
 * declared in their owner's header, `Network/NetworkPat.h`, which this header reaches through
 * `Network/network_pat_control.h` (rule 2) - this unit only calls them. */

/* The slot mode-word source value (.sdata2 0x8079C888). */
extern f32 lbl_8079C888;

/* MSL primitives. */
void* memcpy(void* dst, const void* src, u32 size);
void* memset(void* dst, int value, u32 size);

/* ---- the pat-control band's callees in this range (`Network/network_pat_control.cpp`; GUESS on every
 * name below: they are derived from the caller's use) ---- */
struct PatTerms;
struct SystemWork;
/* The terms object `getPatTerms` hands out is `menu/menu_plsearch.cpp`'s class `PatTerms` (`menu/PatTerms.h`). */
/* One friend card the place-info menu shows (`readFriendCards` fills them): the friend's slot, its id text and name,
 * its area and status bytes and its packed settings.  Field names GUESSED from the session words they come from.
 * size: 0x130 (the stride the readers clear and fill) */
typedef struct NetFriendCard {
    /* +0x000 */ u8 valid_0x000;
    /* +0x001 */ u8 index_0x001;
    /* +0x002 */ u8 pad_0x002;
    /* +0x003 */ char id_0x003[0xA];        /* `formatNetId` text */
    /* +0x00D */ char name_0x00D[0x17];
    /* +0x024 */ u16 area_0x024;            /* the session's area half */
    /* +0x026 */ u8 pad_0x026[0x3];
    /* +0x029 */ u8 status_0x029;           /* the session status's top byte */
    /* +0x02A */ u8 pad_0x02A[0xF3];
    /* +0x11D */ u8 status_low_0x11D;       /* the session status's low byte */
    /* +0x11E */ u8 pad_0x11E[0xA];
    /* +0x128 */ u8 settings_0x128[4];
    /* +0x12C */ u32 members_0x12C;
} NetFriendCard; /* size: 0x130 */

/* The selection `readCommunityMemberCards` reads the community index from (the place-info menu's record; only that
 * field is named).  size: 0x62 (approximation: the last field read) */
typedef struct NetCommunitySelection {
    /* +0x00 */ u8 pad_0x00[0x60];
    /* +0x60 */ s16 community_0x60;
} NetCommunitySelection; /* size: 0x62 */

/* The socket allocator pair `initNetworkPatControl` copies out of `.sdata` 0x807939A8 (`pat_so_allocator`:
 * `soAlloc`, `soFree`).  size: 0x8 */
typedef struct PatSoAllocator {
    /* untyped: byte range - a raw heap block */
    /* +0x00 */ void* (*alloc_0x00)(u32 name, s32 size);
    /* untyped: byte range - a raw heap block */
    /* +0x04 */ void (*free_0x04)(u32 name, void* block, s32 size);
} PatSoAllocator; /* size: 0x8 */
/* 0x80426D24 / 0x80426E08 / 0x804270D4 / 0x804271A8 / 0x80427814 - the peer and friend lookups (by id, by
 * address object) and whether the layer is ready (GUESS names from the bodies). */
s32 findPeerIndex(const NetworkUniqueId* id);
s32 findFriendIndex(const NetId* id);
s32 findFreePeerSlot(const NetworkUniqueId* address);
s32 findPeerSlot(const NetworkUniqueId* address);
BOOL isLayerReady(void);
/* 0x804273EC / 0x80427530 - publish part of this player's profile, or his position (GUESS names from the bodies; the
 * item menu and the lobby's per-frame update call them). */
s32 sendUserProfilePart(u8 kind, const u8* data, u32 size);
/* 0x80427EE4 / 0x80428034 - fill `count` friend cards from friend `first` on: of the layer's friend list, or of the
 * member table of the community `selection` names; each returns how many cards it filled (GUESS names). */
s32 readFriendCards(struct NetFriendCard* out, s32 first, s32 count);
s32 readCommunityMemberCards(const struct NetCommunitySelection* selection, struct NetFriendCard* out, s32 first,
                             s32 count);
s32 sendUserPosition(u8 action, const f32* position, const u32* values, u8 mode, u8 low, u8 high, u8 mid);
/* 0x8042822C / 0x8042826C / 0x80428538 - keep the layer's id, read how it changed since, and classify the change
 * (GUESS names from the bodies). */
void saveLayerId(void);
/* 0x804281C4 (GUESS) - the layer's community record `index` (a layer entry's target). */
NetCommunityRec* getCommunityRecord(s32 index);
void readLayerIdChange(s32* changed, u32* server, s32* city, s32* room);
s32 classifyLayerIdChange(void);
/* 0x8042835C - whether this player may enter the layer `target`: 2 not (no known server, or no city), 4 already
 * there, 3 the hunter rank is outside the server's range or the profile's city filter, else whether it is another
 * server (1) or this one (0) (GUESS name from the body). */
s32 checkLayerEntry(const NetworkLayerId* target, const struct NetUserProfile* profile);
/* 0x80427714 / 0x804247D0 - installs the layer's reflect callback (and clears the layer status words);
 * the callback itself. */
void installLayerCallback(void);
/* untyped: caller-owned payload - the layer's reflect payload */
s32 layerReflectCallback(u32 command, s32 result, s32 count, void* data);
/* 0x80428CAC - points `net_ctrl_wk` at the work record and resets all of it (GUESS name). */
void initWorkRecord(void);
extern NetProfileRec net_profile_table[10];
/* 0x804292F8 - builds the work record, the network heap and the three Pat layers (GUESS name). */
void initNetworkPatControl(void);
/* 0x804295B8 - copies a socket allocator pair. */
void copySoAllocator(PatSoAllocator* dst, const PatSoAllocator* src);
/* The parameter block `initNetworkPatControl` hands the Pat library (0x80419BB4): the allocators, the game
 * name and code, the product id, two strings and the settings block.  size: 0x2C */
typedef struct PatLibraryParams {
    /* +0x00 */ PatSoAllocator so_allocator_0x00;
    /* +0x08 */ u32 flags_0x08;
    /* untyped: byte range - a raw heap block */
    /* +0x0C */ void* (*alloc_0x0C)(u32 name, u32 size, s32 align);
    /* untyped: byte range - a raw heap block */
    /* +0x10 */ void (*free_0x10)(u32 name, void* block, u32 size);
    /* +0x14 */ const char* game_name_0x14;
    /* +0x18 */ u32 game_code_0x18;
    /* +0x1C */ s32 product_0x1C;
    /* +0x20 */ const char* secret_0x20;
    /* +0x24 */ const char* extra_0x24;
    /* +0x28 */ struct PatSettings* settings_0x28;
} PatLibraryParams; /* size: 0x2C */
/* The socket allocator pair (`.sdata` 0x807939A8) and the Pat holder (`.bss` 0x806DFC44). */
extern PatSoAllocator pat_so_allocator;
extern NetworkPat net_pats_object;
/* 0x804295CC - runs the work record's result callback. */
void invokeResultCallback(void);
/* 0x804295EC - clears the control's state bytes, cursors and error words. */
void resetControlFields(void);
/* 0x804297E0 - installs the reflect callbacks and resets the control's state bytes. */
void resetControlState(void);
/* 0x804298F0 - the per-frame network hook. */
void runNetworkFrame(void);
/* 0x80429A08 / 0x80429A54 - clear the refresh timeout on a view change; clear the big-data timeout. */
void clearRefreshTimeoutOnChange(void);
void clearBigDataTimeout(void);
/* 0x80429B10 / 0x80429B4C / 0x80429B6C - the terms-check queries (GUESS names from the flag bytes). */
u32 isTermsUpdateRunning(void);
u8 getTermsCheckDone(void);
void acknowledgeTermsCheck(void);
/* 0x804286F0 - the message id of the layer's last error (GUESS name: its values are message ids). */
s32 getNetErrorMessageId(void);
/* 0x80428A78..0x80428CA4 - the socket and DWC allocators over the network heap, the heap's free size, and
 * two empty hooks (GUESS names: the failure strings name the libraries). */
/* untyped: byte range - a raw heap block */
void* soAlloc(u32 name, s32 size);
u32 getNetHeapFreeSize(MEMiHeapHead* heap);
/* untyped: byte range - a raw heap block */
void soFree(u32 name, void* block, s32 size);
/* untyped: byte range - a raw heap block */
void* dwcAlloc(u32 name, u32 size, s32 align);
/* untyped: byte range - a raw heap block */
void dwcFree(u32 name, void* block, u32 size);
s32 netNullQuery(void);
void netNullHook(void);
extern u32 net_heap_free_size;
/* 0x80424198 - points the record's arena vectors at the shared arena (allocating it on first use). */
void setupArenaVectors(NetCtrlWk* work);
/* 0x80429A68 - raises the network error (state 0x5A) for the work record. */
void setErrorHappened(NetCtrlWk* work);
/* 0x804295A4 - whether the terms object has reached its finished state. */
u32 isTermsCheckFinished(struct PatTerms* terms);
/* 0x80429994 - the per-frame timer tick of the control. */
void tickPatControl(void);
/* 0x80429A94 - stores `code` and sends the control to its shutdown state. */
void abortNetworkControl(NetCtrlWk* work, u8 code);
/* 0x80429AB0 - turns the current state into its failure state. */
void failNetworkControl(NetCtrlWk* work);
/* 0x80429990 - a stub (`blr`). */
void resetFailureState(NetCtrlWk* work);
/* 0x80429A40 - clears the refresh timeout. */
void clearRefreshTimeout(void);
/* 0x80429850 - resets the pat interface singletons. */
void resetPatInterfaces(void);
/* 0x804292B4 - allocates and clears the dialog record. */
void allocateDialogRecord(void);
/* 0x80425648 - repaints the server-select screen and counts the held-button frames. */
void refreshServerScreen(NetCtrlWk* work);
/* 0x80425790 - the message-pool state machine. */
void updateMessagePool(void);
/* 0x80426EA0 - resets the message pool. */
void resetMessagePool(void);
/* 0x80428CA8 - a stub (`blr`) taking the system record. */
void resetSystemState(struct SystemWork* system);
/* 0x8042968C - the reflect (page/event) callback the mediator is handed. */
/* untyped: caller-owned payload - the two trailing words are the mediator's own event payload words */
void patReflectCallback(s32 a, s32 b, s32 c, s32 d, void* e, void* f);
/* 0x80603858 - the per-language (group, group max) word pairs and 0x806038D8 - the server host name,
 * both in this range's `.data`. */
extern s32 language_group_table[10];
extern char pat_server_host[];

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043065C / 0x80430958 / 0x804309E8 / 0x80430D8C - the message layout: apply the tag just parsed, consume
 * the tags at the cursor, split the text into runs, print the runs (GUESS names from the bodies). */
void applyTextTag(NetTextTagState* state);
void parseTextTags(NetTextTagState* state);
void layoutTextRuns(NetTextTagState* state);
void drawTextRuns(NetTextTagState* state, s16 size, s16 line_gap);

/* 0x8043137C / 0x80431420 / 0x804314A4 - the network save: export the work record's settings, copy a Pat
 * settings block, import them back (GUESS names from the fields they move). */
void exportNetworkSave(NetSaveRecord* save);
void copyPatSettings(PatSettings* dst, const PatSettings* src);
void importNetworkSave(const NetSaveRecord* save);

/* The error message table and its length (`.sbss` 0x80794CF0 / 0x80794D00). */
extern char** net_err_msg_tbl_adrs;
extern u16 net_err_msg_num;

/* 0x80431324 / 0x804312B8 - the message for the network error / sub-error `-1` (a generic failure). */
char* getDefaultErrorMessage(void);

char* getDefaultSubErrorMessage(void);

/* 0x804312FC / 0x80431368 - the singleton's negative-reply and shutdown-notice records (GUESS names, after the
 * fields). */
struct PatErrorRecord* getErrorRecord613c(class PatInterface* self);
struct PatErrorRecord* getShutdownRecord6344(class PatInterface* self);

/* 0x80431370 / 0x80431374 / 0x80431378 - tail calls into the ENC converters (GUESS names; the third is the one the
 * system-message menu converts its texts with). */
s32 netUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength);
s32 netUtf16ToUtf8(u8* dst, s32* dstLength, const u16* src, s32* srcLength);
s32 netMessageUtf8ToUtf16(u16* dst, s32* dstLength, const u8* src, s32* srcLength);

/* 0x80431548 / 0x80431578 / 0x80431638 - force the mediator's link error, the graded terms progress, and hand the
 * layer the system's transfer flag (GUESS names from the bodies). */
void forceNetLinkError(void);
s32 getNetTermsProgressLevel(void);
void syncNetTransferFlag(void);

/* 0x8043172C - switches the transfer mode (1 on / 0 off). */
void setTransferMode(u32 mode);

/* 0x8043159C / 0x80431690 - apply the system's transfer mode and transfer level (GUESS names). */
void applyTransferSettings(void);
void applyTransferLevel(void);

/* 0x80431A9C - hands the layer each connected friend's transfer mode; 0x804317E8 - the transfer mode update; the
 * control runs both each frame (GUESS names). */
void updateFriendTransferModes(void);

void updateTransferMode(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_PAT_CONTROL_H */
