/*
 * Network/NetworkCommunityPat.h - the declarations of `Network/NetworkCommunityPat.cpp`: `NetworkCommunityPat` (the
 *   `NetworkPat` holder's +0x08 element, derived from `NetworkCommunity`), its request record and the records it embeds.
 */
#ifndef MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H
#define MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H

#include "types.h"
#include "Network/NetworkCommunity.h"   /* NetworkCommunity - the base class */
#include "Network/NetworkUniqueId.h"    /* NetworkUniqueId - the ids the records embed */
#include "Network/NetworkLayer.h"       /* NetworkLayerId - the peer record embeds one */
#include "menu/movie.h"                 /* NetworkFriendInfo - the record a friend entry ends with */

/* The roster block `NetCtrlWk::roster_sync_0x61CC` holds (defined by the work record header). */
struct NetRosterSync;

class NetworkCommunityPat;
struct NetworkCommunityPatRequest;
struct NetworkErrorInfo;                                   /* the 12-byte error record - Network/gamespy_interface_types.h */

/* The handler a friend request runs: a non-virtual member function of the layer (the descriptors are `{0, -1, fn}`). */
typedef s32 (NetworkCommunityPat::*NetworkCommunityPatHandler)(NetworkCommunityPatRequest* request);

/* The friend requests' record: the `NetworkCommunityRequest` layout with the owner and the handler typed for this class.
 * Its members are inline - retail emits each one after the first function that calls it. */
struct NetworkCommunityPatRequest {
    /* +0x00 */ s32 state_00;
    /* +0x04 */ u32 unused_04;
    /* +0x08 */ u32 unused_08;
    /* +0x0C */ u32 unused_0C;
    /* +0x10 */ u32 unused_10;
    /* +0x14 */ u32 unused_14;
    /* +0x18 */ u32 unused_18;
    /* +0x1C */ u32 unused_1C;
    /* +0x20 */ u32 unused_20;
    /* +0x24 */ u32 unused_24;
    /* +0x28 */ u32 count_28;              /* how many of `args_2C` the starter filled */
    /* +0x2C */ u32 args_2C[8];
    /* +0x4C */ f32 interval_4C;
    /* +0x50 */ f32 timeout_50;            /* the start time `begin` stamps */
    /* +0x54 */ u32 record_54;
    /* +0x58 */ u32 record_58;
    /* +0x5C */ u32 record_5C;
    /* +0x60 */ u32 unused_60;
    /* +0x64 */ u32 unused_64;
    /* +0x68 */ u32 unused_68;
    /* +0x6C */ u32 unused_6C;
    /* +0x70 */ u32 requestId_70;
    /* +0x74 */ u8 cancelled_74;
    /* +0x75 */ u8 pad_75[0x03];
    /* +0x78 */ u8 mutex_78[0x1C];
    /* +0x94 */ NetworkCommunityPat* owner_94;     /* set while the request runs */
    /* +0x98 */ NetworkCommunityPatHandler handler_98;

    NetworkCommunityPatRequest();
    ~NetworkCommunityPatRequest();
    void clear();
    void reset();
    s32 isOwned();
    void run();
    void begin(NetworkCommunityPat* owner, NetworkCommunityPatHandler handler, u32 count, ...);
    s32 getRecord(NetworkErrorInfo* out);
    s32 getArgument(u32 index);
    void setRecord(u32 code, u32 arg0, u32 arg1);
};   /* size: 0xA4 (the pool's element size) */

/* A profile block: an owner id and up to 0x100 profile bytes with a window - the bytes `writeProfileRange` changed for
 * this player's block, the bytes a fetch returned for a peer's. */
struct NetworkCommunityProfile {
    /* +0x000 */ NetworkUniqueId owner_000;
    /* +0x020 */ u32 windowOffset_020;    /* the window's first byte */
    /* +0x024 */ u32 windowSize_024;      /* how many bytes from it */
    /* +0x028 */ u8 data_028[0x100];
};   /* size: 0x128 (the length `clear` zeroes) */

/* Who a profile write is announced to: `kind_00` 0..4 (1 = one player, named by `id_04`). */
struct NetworkCommunityTarget {
    /* +0x00 */ u32 kind_00;
    /* +0x04 */ NetworkUniqueId* id_04;
};   /* size: 0x08 (approximation: the two fields the profile handler reads) */

/* The mail record the mail handler delivers (command 9): the sender, its name and tag, the text, the caller's word
 * and the server time. */
struct NetworkCommunityMail {
    /* +0x000 */ NetworkUniqueId sender_000;
    /* +0x020 */ char name_020[20];
    /* +0x034 */ char tag_034[1];
    /* +0x035 */ char text_035[513];
    /* +0x236 */ u8 pad_236[2];
    /* +0x238 */ u32 value_238;
    /* +0x23C */ s32 time_23C;
};   /* size: 0x240 (the handler's frame) */

/* The invite record the invite handler delivers (command 21): the friend, an empty name and tag, and the invite type
 * (1, or 2 for every other kind). */
struct NetworkCommunityInvite {
    /* +0x00 */ NetworkUniqueId id_000;
    /* +0x20 */ char name_020[20];
    /* +0x34 */ char tag_034[1];
    /* +0x35 */ u8 type_035;
    /* +0x36 */ u8 pad_036[0x02];
};   /* size: 0x38 (approximation: the fields the handler writes, rounded to a word) */

/* The message record the friend-message handler delivers (command 19). */
struct NetworkCommunityMessage {
    /* +0x000 */ NetworkUniqueId sender_000;
    /* +0x020 */ char name_020[20];
    /* +0x034 */ char tag_034[1];
    /* +0x035 */ char text_035[513];
    /* +0x236 */ u8 pad_236[0x02];
};   /* size: 0x238 (the frame slots of `onPatEvent` and the message handler) */

/* A peer's profile as the peer-profile reply (Pat message 0x8073) fills it: the profile block, the peer's layer id, up
 * to three names (each at most 63 characters), its title, name and tag and two words. */
struct NetworkCommunityPeer {
    /* +0x000 */ NetworkCommunityProfile profile_000;
    /* +0x128 */ NetworkLayerId layerId_128;
    /* +0x168 */ s32 nameCount_168;
    /* +0x16C */ char names_16C[3][64];
    /* +0x22C */ bool isRemote_22C;              /* the reply's kind byte is 2 */
    /* +0x22D */ char title_22D[32];
    /* +0x24D */ char name_24D[20];
    /* +0x261 */ char tag_261[1];
    /* +0x262 */ u8 pad_262[2];
    /* +0x264 */ u32 value_264;
    /* +0x268 */ u32 value_268;
};   /* size: 0x26C (the gap to the friend list) */

/* The data record the data message (Pat message 0x806D) delivers (command 15): the sender, its name and tag, a word
 * and up to 0x100 bytes. */
struct NetworkCommunityData {
    /* +0x000 */ NetworkUniqueId sender_000;
    /* +0x020 */ char name_020[20];
    /* +0x034 */ char tag_034[1];
    /* +0x035 */ u8 pad_035[3];
    /* +0x038 */ u32 value_038;
    /* +0x03C */ u32 size_03C;
    /* +0x040 */ u8 data_040[0x100];
};   /* size: 0x140 (the frame slot of `onPatEvent`) */

/* One entry of the friend list. */
struct NetworkCommunityFriend {
    /* +0x00 */ NetworkUniqueId id_00;
    /* +0x20 */ char name_20[20];
    /* +0x34 */ char tag_34[1];
    /* +0x35 */ u8 pad_35[0x03];
    /* +0x38 */ NetworkFriendInfo info_38;
};   /* size: 0x84 (the list's element size) */

/* The friend list (`isFriend` walks `count_00` entries). */
struct NetworkCommunityFriendList {
    /* +0x000 */ s32 count_00;
    /* +0x004 */ NetworkCommunityFriend entries_04[50];
};   /* size: 0x19CC */

/* One entry of the block list. */
struct NetworkCommunityBlocked {
    /* +0x00 */ NetworkUniqueId id_00;
    /* +0x20 */ char name_20[20];
    /* +0x34 */ char tag_34[1];
    /* +0x35 */ u8 pad_35[0x03];
};   /* size: 0x38 (the list's element size) */

/* The block list. */
struct NetworkCommunityBlockList {
    /* +0x000 */ s32 count_00;
    /* +0x004 */ NetworkCommunityBlocked entries_04[16];
};   /* size: 0x384 */

class NetworkCommunityPat : public NetworkCommunity {   /* size: 0x25EC (the allocation `initNetworkPatControl` makes for it) */
public:
    NetworkCommunityPat();
    virtual ~NetworkCommunityPat();                                          /* +0x08 */
    virtual void setReflectCallback(u32 callback, u32 user);                 /* +0x0C */
    virtual void clear();                                                    /* +0x10 */
    virtual void release();                                                  /* +0x14 */
    virtual void move();                                                     /* +0x18 */
    virtual void getSelfId(NetworkUniqueId* out);                            /* +0x24 */
    virtual void getName(char* out, s32 size);                               /* +0x28 */
    virtual void getTag(char* out, s32 size);                                /* +0x2C */
    virtual void rejectRequest_40();                                         /* +0x40 */
    virtual s32 isFriend(const NetworkUniqueId* id);                         /* +0x44 */
    virtual void writeProfile_4C(const u8* data, u32 size);                  /* +0x4C */
    virtual void writeProfileRange_50(const u8* data, u32 size, u32 offset); /* +0x50 */
    virtual s32 handle_68(NetworkCommunityRequest* request);                 /* +0x68 */
    virtual s32 handle_6C(NetworkCommunityRequest* request);                 /* +0x6C */
    virtual s32 handle_70(NetworkCommunityRequest* request);                 /* +0x70 */
    virtual s32 handle_74(NetworkCommunityRequest* request);                 /* +0x74 */
    virtual s32 handle_78(NetworkCommunityRequest* request);                 /* +0x78 */
    virtual s32 handle_7C(NetworkCommunityRequest* request);                 /* +0x7C */
    virtual s32 handle_80(NetworkCommunityRequest* request);                 /* +0x80 */
    virtual s32 handle_84(NetworkCommunityRequest* request);                 /* +0x84 */
    virtual s32 handle_88(NetworkCommunityRequest* request);                 /* +0x88 */
    virtual s32 handle_8C(NetworkCommunityRequest* request);                 /* +0x8C */

    /* 0x803F10E4 - sends the friend roster held at `roster` (`NetCtrlWk::roster_sync_0x61CC`). */
    void syncFriends(NetRosterSync* roster);
    /* 0x803F1204 - sends an invite to `id`; `kind` is the invite type. */
    void inviteFriend(const NetworkUniqueId* id, s32 kind);
    /* 0x803F129C - removes `id` from the friend list. */
    void removeFriend(const NetworkUniqueId* id);
    /* 0x803F139C - adds `id` to the server's block list (Pat request 283, BlackAdd). */
    void blockPlayer(const NetworkUniqueId* id);
    /* 0x803F1424 - removes `id` from the server's block list (Pat request 285, BlackDelete). */
    void unblockPlayer(const NetworkUniqueId* id);
    /* 0x803F1324 - requests the block list (once). */
    void requestBlockList(void);

    /* 0x803F0F20 - requests the profile of the peer `id`. */
    void requestPeerProfile(const NetworkUniqueId* id);
    /* 0x803F116C - sends the message `text` to the friend `id`. */
    void sendFriendMessage(const NetworkUniqueId* id, const char* text);

    s32 handleRequestPeerProfile(NetworkCommunityPatRequest* request);
    s32 handleSyncFriends(NetworkCommunityPatRequest* request);
    s32 handleSendFriendMessage(NetworkCommunityPatRequest* request);
    s32 handleInviteFriend(NetworkCommunityPatRequest* request);
    s32 handleRemoveFriend(NetworkCommunityPatRequest* request);
    s32 handleRequestBlockList(NetworkCommunityPatRequest* request);
    s32 handleBlockPlayer(NetworkCommunityPatRequest* request);
    s32 handleUnblockPlayer(NetworkCommunityPatRequest* request);

    /* the request error helpers: the session-lost and aborted records, or an explicit one sent to the server too, for
     * a base request and for a friend request (by analogy with `NetworkLayerPat`'s `setCollectionLog*` siblings) */
    void setCollectionLogSessionLost(NetworkCommunityRequest* request);
    void setCollectionLogSessionLost(NetworkCommunityPatRequest* request);
    void setCollectionLogAborted(NetworkCommunityRequest* request);
    void setCollectionLogAborted(NetworkCommunityPatRequest* request);
    void setCollectionLog(NetworkCommunityRequest* request, u32 code, u32 arg0, u32 arg1);
    void setCollectionLog(NetworkCommunityPatRequest* request, u32 code, u32 arg0, u32 arg1);

    /* 0x803F4E18 - whether `id` is a friend and not on the block list. */
    s32 isAcceptedPeer(const NetworkUniqueId* id);
    /* 0x803F4EC0 - hands `command` and its result to the reflect callback (a failed one with the kept Pat error). */
    /* untyped: caller-owned payload - the record each command delivers */
    void notifyReflect(u32 command, s32 result, s32 count, void* data, u32 user);

    NetworkCommunityPatRequest* allocPatRequest();
    void deletePatRequest(NetworkCommunityPatRequest** slot);
    void movePatRequests();
    void onPatEvent(s32 code, s32 requestId, s32 flag, s32 count, const u8* data);
    /* 0x803E8B9C (GUESS) - widens the profile's window back to the last sent one (none at 0x100) and returns the
     * profile; inline, emitted by Network/NetworkLayerPat.cpp after its first caller. */
    NetworkCommunityProfile* restoreSentProfile();


    /* +0x184 */ NetworkCommunityPatRequest* patRequests_184[9];   /* the running friend request of each starter */
    /* +0x1A8 */ NetworkCommunityPatRequest patPool_1A8[2];       /* the two records the friend starters hand out */
    /* +0x2F0 */ u32 replyFlags_2F0[21];                          /* the reply bits each request waits on */
    /* +0x344 */ s32 pendingIds_344[21];                          /* the server request id each request waits for, -1 when none */
    /* +0x398 */ f32 lastProfileSend_398;                         /* when the profile was last sent (`move` waits a second) */
    /* +0x39C */ u8 open_39C;                                     /* set while the community session is open */
    /* +0x39D */ u8 profileDirty_39D;                             /* the profile has bytes `move` has not sent */
    /* +0x39E */ u8 flag_39E;
    /* +0x39F */ u8 pad_39F;
    /* +0x3A0 */ NetworkUniqueId selfId_3A0;                      /* this player's id */
    /* +0x3C0 */ char name_3C0[20];                               /* this player's name */
    /* +0x3D4 */ char tag_3D4[1];
    /* +0x3D5 */ u8 pad_3D5[0x03];
    /* +0x3D8 */ NetworkCommunityProfile profile_3D8;             /* this player's profile */
    /* +0x500 */ u32 sentOffset_500;                              /* the window already sent, 0x100 when none */
    /* +0x504 */ u32 sentSize_504;
    /* +0x508 */ NetworkCommunityProfile peerProfile_508;
    /* +0x630 */ NetworkCommunityPeer peer_630;
    /* +0x89C */ NetworkCommunityFriendList friends_89C;
    /* +0x2268 */ NetworkCommunityBlockList blocked_2268;
};

#endif
