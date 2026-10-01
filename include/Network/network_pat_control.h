/*
 * include/Network/network_pat_control.h - shared views for the network pat-control band
 * (`Network/network_pat_control.cpp`, `.text` 0x80429B94..0x8043065C).
 *
 * The `net_ctrl_wk` singleton (map: `.sbss:0x80794CF8`, a 4-byte pointer) is the record every
 * function in the band dereferences; its owner TU is unclaimed, so the extern lives here beside the
 * one unit that reads it today (docs/plan.md 6.5 rule 2's unsplit-address gap).  Only the offsets
 * this unit reads are named; every other byte stays `pad_0xNNN`.
 *
 * `NetCtrlWk` size: 0xC4A0 approximate - the highest offset any function in the band touches is
 * +0xC49F; the record is larger than anything this unit proves.  It is a class: the band's functions
 * that work on the singleton are its (static) members, and it is packed because the record carries a
 * `u32` run at the odd offset +0x7996 that natural alignment would slide.
 *
 * The four-slot "*Pat" holder and the accessor family that walks it live in their owner's header,
 * `Network/NetworkPat.h`, which this file includes (rule 1/2) - the family is declared once, by the
 * unit that defines it.
 */
#ifndef MHTRI_NETWORK_NETWORK_PAT_CONTROL_H
#define MHTRI_NETWORK_NETWORK_PAT_CONTROL_H

#include "types.h"
#include "MSL_C/alloc.h"           /* strcpy (owner: MSL_C/alloc.cpp, rule 2) */
#include "Network/NetworkPat.h"
#include "Network/NetworkSessionManagerPat.h"   /* getPatsObject, isNetworkSessionManagerPatReady (owner's header, rule 2) */
#include "Network/NetworkLayerPat.h"   /* NetId and the layer records the work record embeds */

#ifdef __cplusplus
extern "C" {
#endif

/* One slot of the 16-entry server/message table at `NetCtrlWk::entries_0x7CD8` (stride 0x5C). */
typedef struct NetCtrlEntry {
    /* +0x00 */ u8 in_use_0x00;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 kind_0x02;
    /* +0x03 */ u8 flags_0x03;
    /* +0x04 */ char name_0x04[0x48];
    /* +0x4C */ u32 value_0x4C;
    /* +0x50 */ char text_0x50[0xC];
} NetCtrlEntry; /* size: 0x5C */

/* One slot of the 100-record `slots_0x3ED4` table (the `.sbss` 0x80794CF8 band).  The first byte is the
 * slot's state (0 = free), +0x04 is the owner pointer the state machine hands out, +0x3C/+0x40 the two
 * float members and +0x54 the slot's mode word. */
typedef struct NetSlot {
    /* +0x00 */ u8 state_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ void* owner_0x04;   /* untyped: opaque handle - the slot hands its owner back to the caller */
    /* +0x08 */ u8 pad_0x08[0x34];
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

/*@TYPES begin@*/
/* One 0x40-byte slot of a profile's id table: the id first, the rest untouched here. */
typedef struct NetIdSlot {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x36];
} NetIdSlot; /* size: 0x40 */

/* The per-profile record `NetCtrlWk::profiles_0x168` points at (stride 0x2B4). */
typedef struct NetProfileRec {
    /* +0x000 */ u8 pad_0x000[0xB0];
    /* +0x0B0 */ NetIdSlot ids_0x0B0[8];
    /* +0x2B0 */ u8 pad_0x2B0[0x4];
} NetProfileRec; /* size: 0x2B4 */

/* The name field of a peer record (0x14 bytes). */
typedef struct NetPeerName {
    /* +0x00 */ char text_0x00[0x14];
} NetPeerName; /* size: 0x14 */

/* The 0x100-byte character blob of a peer record (`decodePlayerCard` decodes it). */
typedef struct NetPeerBlob {
    /* +0x00 */ u32 words_0x00[0x40];
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

/* The decoded player card `decodePlayerCard` fills; only the two strings this unit writes are named.
 * size: 0x5E5 (approximate: the highest byte written here) */
typedef struct NetPlayerCard {
    /* +0x000 */ u8 pad_0x000[0x5CA];
    /* +0x5CA */ char name_0x5CA[0x11];
    /* +0x5DB */ char id_0x5DB[0xA];
} NetPlayerCard; /* size: 0x5E5 (approximate) */

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

/* One 0x40-byte entry of the server table (`NetCtrlWk::servers_0x668`, `server_count_0x660` valid). */
typedef struct NetSrvRec {
    /* +0x00 */ s32 id_0x00;
    /* +0x04 */ char name_0x04[0x24];
    /* +0x28 */ s32 population_0x28;
    /* +0x2C */ s32 capacity_0x2C;
    /* +0x30 */ u8 pad_0x30[0x8];
    /* +0x38 */ u32 flags_0x38;
    /* +0x3C */ u32 type_0x3C;          /* 1-based server type, matched against `NetServerType` */
} NetSrvRec; /* size: 0x40 */

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

/* The remote-file client the fetch state machines drive (constructor `NetworkFileFetcher::NetworkFileFetcher`, `open` at
 * 0x803F6A94, error copy `NetworkFileFetcher::copyError`; GUESS on the class name: it opens a numbered file on the
 * server, polls, reads and closes).  A class with declared, undefined virtuals: MWCC emits no table of
 * ours (rule 10), and the slots are +8+4*i. */
class NetworkFileFetcher {   /* size: 0x48 */
public:
    /* +0x08 */ virtual ~NetworkFileFetcher();
    /* +0x0C */ virtual s32 poll(u32* status);
    /* +0x10 */ virtual s32 open(s32 flags, const char* path);
    /* +0x14 */ virtual s32 read(void* buffer, s32 size);   /* untyped: byte range (the caller's file buffer) */
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual void pad_20();
    /* +0x24 */ virtual s32 close();

    NetworkFileFetcher();
    void copyError(NetFetchError* error);
private:
    /* +0x04 */ u8 pad_04[0x44];
};

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
    /* +0x00 */ char name_0x00[0xB];
    /* +0x0B */ u8 valid_0x0B;
} NetInviteRec; /* size: 0xC */

/* One 0x180-byte text record of the big-data file. */
typedef struct NetBigDataRec {
    /* +0x000 */ char text_0x000[0x180];
} NetBigDataRec; /* size: 0x180 */

/* The big-data file (`NetCtrlWk::bigdata_0x96F0`): six text records and the event bits.  size: 0x904 */
typedef struct NetBigData {
    /* +0x000 */ NetBigDataRec records_0x000[6];
    /* +0x900 */ u32 event_bits_0x900;
} NetBigData; /* size: 0x904 */

/* One 0x84-byte roster entry (`NetCtrlWk::roster_0xA1BC`): the id and a 0x14-byte name at +0x20. */
typedef struct NetRosterRec {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x16];
    /* +0x20 */ u8 name_0x20[0x14];
    /* +0x34 */ u8 pad_0x34[0x50];
} NetRosterRec; /* size: 0x84 */

/* One 0x38-byte recent-player entry (`NetCtrlWk::recent_0xBB88`): the id and a 0x14-byte name at +0x20. */
typedef struct NetRecentRec {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x16];
    /* +0x20 */ u8 name_0x20[0x14];
    /* +0x34 */ u8 pad_0x34[0x4];
} NetRecentRec; /* size: 0x38 */

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

/* The text-layout state `NetTextTagState::parseTag` parses tags out of (GUESS on the owner: a message layout record);
 * only the tag buffer, the read cursor and the status byte are named.  size: 0x30E (approximate) */
typedef struct NetTextTagState {
    /* +0x000 */ u8 pad_0x000[0x2D8];
    /* +0x2D8 */ char tag_0x2D8[0x20];
    /* +0x2F8 */ u8 pad_0x2F8[0x10];
    /* +0x308 */ char* cursor_0x308;
    /* +0x30C */ u8 pad_0x30C;
    /* +0x30D */ u8 status_0x30D;

    void parseTag(void);
} NetTextTagState; /* size: 0x30E (approximate) */

/* One 0x5C-byte row of the login server list (`NetCtrlWk::rows_0x17C`): the server id and its name. */
typedef struct NetRowRec {
    /* +0x00 */ s32 id_0x00;
    /* +0x04 */ char name_0x04[0x58];
} NetRowRec; /* size: 0x5C */

/* The roster-sync block `NetCtrlWk::roster_sync_0x61CC` (built by `buildRosterSync`, sent by
 * `NetworkCommunityPat::syncFriends`); its layout is the callee's own.  size: 0x44 */
struct NetRosterSync {
    /* +0x00 */ u8 bytes_0x00[0x44];
};

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

/* The user record `get_userdata` hands out, seen only as the name at +0x03 the band copies. */
typedef struct NetUserData {
    /* +0x00 */ u8 pad_0x00[0x3];
    /* +0x03 */ char name_0x03[0x20];
} NetUserData; /* size: 0x23 (approximate) */
/*@TYPES end@*/

/* The network-control work record reached through `net_ctrl_wk`.  `entries_0x7CD8` is 16 records
 * of 0x5C; the tail offsets (+0x82C4, +0xBF2C, +0xC3F2, ...) are separate flags/counters. */
/* Packed: the record carries a `u32` run at the odd offset +0x7996 (`slots_0x7996`), and natural
 * alignment would slide every later field by four bytes. */
#pragma pack(push, 1)
typedef struct NetCtrlWk {
/*@NetCtrlWk begin@*/
    /* +0x000 */ s32 reset_0x000;
    /* +0x004 */ u8 pad_0x004[0xC];
    /* +0x010 */ u8 mode_0x010;
    /* +0x011 */ u8 state_0x011;
    /* +0x012 */ u8 pad_0x012[0x2];
    /* +0x014 */ u8 entered_0x014;
    /* +0x015 */ u8 flag_0x015;
    /* +0x016 */ u8 flag_0x016;
    /* +0x017 */ u8 sub_state_0x017;
    /* +0x018 */ u8 step_0x018;
    /* +0x019 */ u8 substep_0x019;
    /* +0x01A */ u8 pad_0x01A[0x2];
    /* +0x01C */ s32 cursor_0x01C;
    /* +0x020 */ s32 cursor_0x020;
    /* +0x024 */ s32 scroll_0x024;
    /* +0x028 */ u8 pad_0x028[0x4];
    /* +0x02C */ s32 repeat_0x02C;
    /* +0x030 */ s32 repeat_0x030;
    /* +0x034 */ u8 pad_0x034[0xC];
    /* +0x040 */ u8 server_slot_state_0x040[4];
    /* +0x044 */ u32 ready_count_0x044;
    /* +0x048 */ u8 pad_0x048[0x4];
    /* +0x04C */ s32 flag_0x04C;
    /* +0x050 */ u8 pad_0x050[0x4];
    /* +0x054 */ s32 error_0x054;
    /* +0x058 */ s32 error_0x058;
    /* +0x05C */ s32 error_0x05C;
    /* +0x060 */ s32 error_0x060;
    /* +0x064 */ s32 layer_state_0x064;
    /* +0x068 */ u8 flag_0x068;
    /* +0x069 */ u8 flag_0x069;
    /* +0x06A */ u8 selected_server_0x06A;
    /* +0x06B */ u8 pad_0x06B[0x9];
    /* +0x074 */ s8 server_index_0x074[4];
    /* +0x078 */ u8 pad_0x078[0x20];
    /* +0x098 */ s32 flag_0x098;
    /* +0x09C */ u8 pad_0x09C[0x98];
    /* +0x134 */ u32 net_error_0x134;
    /* +0x138 */ s32 net_error_0x138;
    /* +0x13C */ u8 pad_0x13C[0x2C];
    /* +0x168 */ NetProfileRec* profiles_0x168;
    /* +0x16C */ u8 pad_0x16C[0x4];
    /* +0x170 */ s32 profile_index_0x170;
    /* +0x174 */ s32 status_0x174;
    /* +0x178 */ u32 row_count_0x178;
    /* +0x17C */ NetRowRec rows_0x17C[13];
    /* +0x628 */ u8 pad_0x628[0x38];
    /* +0x660 */ u32 server_count_0x660;
    /* +0x664 */ u8 pad_0x664[0x4];
    /* +0x668 */ NetSrvRec servers_0x668[100];
    /* +0x1F68 */ u8 pad_0x1F68[0x1F68];
    /* +0x3ED0 */ NetSlot* slot_list_0x3ED0;
    /* +0x3ED4 */ NetSlot slots_0x3ED4[100];
    /* +0x6134 */ u8 pad_0x6134[0x98];
    /* +0x61CC */ NetRosterSync roster_sync_0x61CC;
    /* +0x6210 */ u32 settings_0x6210[4];
    /* +0x6220 */ u8 pad_0x6220[0x28];
    /* +0x6248 */ s32 status_0x6248;
    /* +0x624C */ s32 status_0x624C;
    /* +0x6250 */ u8 pad_0x6250[0x8];
    /* +0x6258 */ s32 status_0x6258;
    /* +0x625C */ u8 pad_0x625C[0xC];
    /* +0x6268 */ s32 status_0x6268;
    /* +0x626C */ u8 pad_0x626C[0x14];
    /* +0x6280 */ s32 status_0x6280;
    /* +0x6284 */ u8 pad_0x6284[0x8];
    /* +0x628C */ s32 status_0x628C;
    /* +0x6290 */ s32 status_0x6290;
    /* +0x6294 */ u8 pad_0x6294[0x3C];
    /* +0x62D0 */ s32 status_0x62D0;
    /* +0x62D4 */ u8 pad_0x62D4[0x14];
    /* +0x62E8 */ s32 peer_count_0x62E8;
    /* +0x62EC */ u8 pad_0x62EC[0x34];
    /* +0x6320 */ NetPoolEntry pool_0x6320[128];
    /* +0x7320 */ u8 pad_0x7320[0x48];
    /* +0x7368 */ char name_0x7368[0xA];
    /* +0x7372 */ char name2_0x7372[0xA];
    /* +0x737C */ u8 pad_0x737C[0x10C];
    /* +0x7488 */ union {
        u8 msgTable_0x7488[0x480];   /* the byte view `Network/network_pat_control.cpp` indexes */
        NetPeerRec peers_0x7488[4];
    };
    /* +0x7908 */ u8 pad_0x7908[0x80];
    /* +0x7988 */ u8 used_0x7988[4];
    /* +0x798C */ u8 pad_0x798C[0xA];
    /* +0x7996 */ u32 slots_0x7996[0x40];
    /* +0x7A96 */ u8 pad_0x7A96[0x2];
    /* +0x7A98 */ u32* arrA_0x7A98[0x40];
    /* +0x7B98 */ u32 arrB_0x7B98[0x40];
    /* +0x7C98 */ u8 arrC_0x7C98[0x40];
    /* +0x7CD8 */ NetCtrlEntry entries_0x7CD8[16];
    /* +0x8298 */ u8 pad_0x8298[0x2C];
    /* +0x82C4 */ u8 flag_0x82C4;
    /* +0x82C5 */ u8 flag_0x82C5;
    /* +0x82C6 */ u8 flag_0x82C6;
    /* +0x82C7 */ u8 flag_0x82C7;
    /* +0x82C8 */ s32 field_0x82C8;
    /* +0x82CC */ NetworkFileFetcher* fetcher_0x82CC;
    /* +0x82D0 */ u32 fetch_step_0x82D0;
    /* +0x82D4 */ s32 fetch_index_0x82D4;
    /* +0x82D8 */ NetServerType server_types_0x82D8[4];
    /* +0x85D8 */ u8 pad_0x85D8[0x4];
    /* +0x85DC */ NetSchedule schedule_0x85DC;
    /* +0x85E4 */ NetEventRec events_0x85E4[32];
    /* +0x8C64 */ NetFilterRec filters_0x8C64[8];
    /* +0x8D84 */ NetFilterRec filters_0x8D84[64];
    /* +0x9684 */ NetRange ranges_0x9684[4];
    /* +0x9694 */ u16 timeouts_0x9694[40];
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
    /* +0xA144 */ u8 pad_0xA144[0x8];
    /* +0xA14C */ s32 status_0xA14C;
    /* +0xA150 */ u8 pad_0xA150[0xC];
    /* +0xA15C */ s32 status_0xA15C;
    /* +0xA160 */ u8 pad_0xA160[0x14];
    /* +0xA174 */ s32 status_0xA174;
    /* +0xA178 */ u8 pad_0xA178[0x14];
    /* +0xA18C */ s32 status_0xA18C;
    /* +0xA190 */ u8 pad_0xA190[0x14];
    /* +0xA1A4 */ s32 status_0xA1A4;
    /* +0xA1A8 */ u8 pad_0xA1A8[0x10];
    /* +0xA1B8 */ s32 roster_count_0xA1B8;
    /* +0xA1BC */ NetRosterRec roster_0xA1BC[50];
    /* +0xBB84 */ s32 recent_count_0xBB84;
    /* +0xBB88 */ NetRecentRec recent_0xBB88[16];
    /* +0xBF08 */ char message_0xBF08[0x24];
    /* +0xBF2C */ u8 flag_0xBF2C;
    /* +0xBF2D */ u8 flag_0xBF2D;
    /* +0xBF2E */ u8 flag_0xBF2E;
    /* +0xBF2F */ u8 pad_0xBF2F[0x1];
    /* +0xBF30 */ NetDialog* dialog_0xBF30;
    /* +0xBF34 */ NetListMenu* list_0xBF34;
    /* +0xBF38 */ u8 pad_0xBF38[0x4];
    /* +0xBF3C */ NetInviteRec invites_0xBF3C[8];
    /* +0xBF9C */ u8 pad_0xBF9C[0x120];
    /* +0xC0BC */ s8 link_state_0xC0BC;
    /* +0xC0BD */ u8 flag_0xC0BD;
    /* +0xC0BE */ u8 pad_0xC0BE[0x6];
    /* +0xC0C4 */ char account_name_0xC0C4[0x40];
    /* +0xC104 */ char nickname_0xC104[0x40];
    /* +0xC144 */ u8 pad_0xC144[0xC];
    /* +0xC150 */ u8 screen_0xC150;
    /* +0xC151 */ u8 pad_0xC151[0x1];
    /* +0xC152 */ u8 action_0xC152;
    /* +0xC153 */ u8 pad_0xC153[0x5];
    /* +0xC158 */ u8 screen_0xC158;
    /* +0xC159 */ u8 screen_0xC159;
    /* +0xC15A */ u8 pad_0xC15A[0x63];
    /* +0xC1BD */ s8 flag_0xC1BD;
    /* +0xC1BE */ u8 flag_0xC1BE;
    /* +0xC1BF */ u8 flag_0xC1BF;
    /* +0xC1C0 */ u8 pad_0xC1C0[0x8];
    /* +0xC1C8 */ char player_name_0xC1C8[0x16];
    /* +0xC1DE */ char support_code_0xC1DE[0x20];
    /* +0xC1FE */ u8 pad_0xC1FE[0x2];
    /* +0xC200 */ s32 terms_version_0xC200;
    /* +0xC204 */ u8 pad_0xC204[0x54];
    /* +0xC258 */ s8 error_code_0xC258;
    /* +0xC259 */ s8 sub_error_0xC259;
    /* +0xC25A */ u8 pad_0xC25A[0x2];
    /* +0xC25C */ NetDialog* (*create_dialog_0xC25C)(NetCtrlWk* work);
    /* +0xC260 */ u8 pad_0xC260[0x8];
    /* +0xC268 */ void (*callback_0xC268)(void);
    /* +0xC26C */ u8 pad_0xC26C[0x4];
    /* +0xC270 */ void (*callback_0xC270)(void);
    /* +0xC274 */ void (*callback_0xC274)(void);
    /* +0xC278 */ void (*callback_0xC278)(void);
    /* +0xC27C */ s32 (*callback_0xC27C)(void);
    /* +0xC280 */ void (*callback_0xC280)(void);
    /* +0xC284 */ s32 (*callback_0xC284)(void);
    /* +0xC288 */ u8 pad_0xC288[0x8];
    /* +0xC290 */ s8* result_0xC290;
    /* +0xC294 */ NetLayerRequest layer_request_0xC294;
    /* +0xC2C8 */ NetId target_id_0xC2C8;
    /* +0xC2D2 */ u8 pad_0xC2D2[0x16];
    /* +0xC2E8 */ char text_0xC2E8[0x18];
    /* +0xC300 */ s32 msg_state_0xC300;
    /* +0xC304 */ u8 pad_0xC304[0x3C];
    /* +0xC340 */ NetId request_id_0xC340;
    /* +0xC34A */ u8 pad_0xC34A[0x16];
    /* +0xC360 */ u16 counter_0xC360;
    /* +0xC362 */ u16 counter_0xC362;
    /* +0xC364 */ u32 counter_0xC364;
    /* +0xC368 */ u8 pad_0xC368[0x1];
    /* +0xC369 */ u8 flag_0xC369;
    /* +0xC36A */ u8 pad_0xC36A[0x2];
    /* +0xC36C */ s32 timeout_0xC36C;
    /* +0xC370 */ u8 pad_0xC370[0x8];
    /* +0xC378 */ s32 field_0xC378;
    /* +0xC37C */ s32 fetch_timeouts_0xC37C[8];
    /* +0xC39C */ u8* file_buffers_0xC39C[10];
    /* +0xC3C4 */ u32 file_sums_0xC3C4[10];
    /* +0xC3EC */ u8* staging_0xC3EC;
    /* +0xC3F0 */ u8 flag_0xC3F0;
    /* +0xC3F1 */ u8 pad_0xC3F1[0x1];
    /* +0xC3F2 */ u8 flag_0xC3F2;
    /* +0xC3F3 */ u8 pad_0xC3F3[0x1];
    /* +0xC3F4 */ u8 record_0xC3F4[0x88];
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
    /* +0xC498 */ s8 field_0xC498;
    /* +0xC499 */ u8 flag_0xC499;
    /* +0xC49A */ u8 flag_0xC49A;
    /* +0xC49B */ u8 pad_0xC49B[0x3];
    /* +0xC49E */ u8 flag_0xC49E;
    /* +0xC49F */ u8 flag_0xC49F;


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
    static char* findPeerName(NetId* id);
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
/*@NetCtrlWk end@*/
} NetCtrlWk; /* size: 0xC4A0 (approximate, see above) */
#pragma pack(pop)

/* The work-record singleton, defined by another (unclaimed) TU. */
extern NetCtrlWk* net_ctrl_wk;

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
/* `getPatsObject` (0x803DA020) and `isNetworkSessionManagerPatReady` (0x803DF1A8) are declared in
 * `Network/NetworkSessionManagerPat.h` (their owner since the phase 4 fold). */

#ifdef __cplusplus
}
#endif

/* Mangled callees are declared with their real signatures, never with the map's mangling
 * (docs/plan.md 6.5 rule 9): the C++ front-end emits the map's name itself.  `sysSE_req` (`__Fl`) has
 * no registered owner; `flfntStrLen` (`__FPc`, 0x8005B874) is owned by `g3d/g3d_anmchr.cpp`, so it is
 * declared in that unit's header (`g3d/g3d_anmchr.h`) and included where it is called (rule 2) -
 * `g3d/g3d_anmchr.h` and this header declare no other symbol in common, so a TU may include both. */

/* The band's C++-linkage helpers (their map names are the compiler's manglings). */
s32 getUtf8CharLength(u8* bytes);
s32 getPrintedWidth(char* text);
void setTextColor(u8 index);
void setTextSize(s16 size);
void printTextRuns(s16 x, s16 y, s32 unused, char* text);
char* getOnlineSupportCode(void);
char* get_network_sub_error_msg(void);

/* The 0x80423E74..0x80429B94 band's declarations (absorbed from `fn_80423E74.h` at phase 4): the PatCamellia wrapper over the
 * retail Camellia cipher and the work record's arena vectors, slot table and message pool. */
#ifdef __cplusplus
extern "C" {
#endif

/* Camellia's key schedule (owner: src/Camellia/camellia.c).  Declared here because the vendor header
 * sits beside its source and is not on the include path; a second consumer should promote it into
 * `include/` (rule 2).  `PatCamelliaKey` is the vendor's `KEY_TABLE_TYPE`. */
typedef unsigned int PatCamelliaKey[68];

void Camellia_Ekeygen(int keyBitLength, const unsigned char* rawKey, PatCamelliaKey keyTable);
void Camellia_EncryptBlock(int keyBitLength, const unsigned char* plaintext,
                           const PatCamelliaKey keyTable, unsigned char* cipherText);
void Camellia_DecryptBlock(int keyBitLength, const unsigned char* cipherText,
                           const PatCamelliaKey keyTable, unsigned char* plaintext);

/* The unit's Camellia key schedule (.bss 0x806D3670, 0x110 B) and the 64 x 0x400 arena base
 * (.sbss 0x80794CEC); neither has a registered owner. */
extern PatCamelliaKey lbl_806D3670;
extern u8* lbl_80794CEC;

/* Unsplit game callees. */
u8* fn_800404BC(u32 size);
/* The band's own entry points this neighbour calls (owner: src/Network/network_pat_control.cpp, rule 2). */
void resetNetSlots(NetCtrlWk* work);
/* 0x80427284 - queues a network command (1 = accepted): the command id, the caller's result byte, an
 * unused word, the argument count and the argument words (at most four). */
s32 queueNetCommand(u32 command, s8* result, s32 unused, s32 arg_count, const s32* args);
void syncScheduleClock(NetCtrlWk* work);
s32 fn_804C2380(u32 id);

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

#endif /* MHTRI_NETWORK_NETWORK_PAT_CONTROL_H */
