/*
 * include/Network/NetworkCommunityPat.h - the `NetworkCommunityPat` view the pat-control band drives.
 *
 * `NetworkCommunityPat` is the +0x08 element of the `NetworkPat` holder (include/Network/NetworkPat.h):
 * the class the constructor at 0x803F02C4 builds (allocation 0x25EC).  The band reaches it only through the friend
 * requests below - direct (non-virtual) calls into the class's own code at 0x803F10E4..0x803F1424 - so
 * only those methods are declared and the layout is left to its owner (a class without declared data is
 * never instantiated or read here).  GUESS on every method name: they are derived from what the
 * pat-control band passes (a network id, and for the invite a second word) and from the friend flow the
 * band drives.
 */
#ifndef MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H
#define MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H

#include "types.h"

/* The network id the friend requests take (defined in `Network/NetworkLayerPat.h`; only its address
 * crosses the calls below, so the name is enough and this header stays free of the session types). */
typedef struct NetId NetId;

/* The roster block `NetCtrlWk::roster_sync_0x61CC` holds (defined by the work record header). */
struct NetRosterSync;

class NetworkCommunityPat {   /* size: 0x25EC (the allocation `initNetworkPatControl` makes for it) */
public:
    /* 0x803F02C4 - builds the community layer (defined by the owner's range; declared so `new` can name the
     * class). */
    NetworkCommunityPat();

    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void setReflectCallback(u32 callback, u32 user);   /* the reflect callback and its user word */
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void pad_14();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void openCommunity_1C();
    /* +0x20 */ virtual void shutdown_20();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 */ virtual void pad_30();
    /* +0x34 */ virtual void pad_34();
    /* +0x38 */ virtual void requestNews_38();
    /* +0x3C */ virtual void pad_3C();
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void pad_44();
    /* +0x48 */ virtual void pad_48();
    /* +0x4C */ virtual void writeProfile_4C(const u8* data, u32 size);   /* 0x803F60C4: the whole block (offset 0) */
    /* +0x50 */ virtual void writeProfileRange_50(const u8* data, u32 size, u32 offset);   /* 0x803F60D8: offset + size <= 0x100 */
    /* +0x54 */ virtual void pad_54();
    /* +0x58 */ virtual void pad_58();
    /* +0x5C */ virtual void pad_5C();
    /* +0x60 */ virtual void request_60(const u8* data, s32 id, u32 size, u32 flags);   /* 0x803F0068 (GUESS: a request starter with descriptor 0x805FC3F0) */

    /* 0x803F10E4 - sends the friend roster held at `roster` (`NetCtrlWk::roster_sync_0x61CC`). */
    void syncFriends(NetRosterSync* roster);
    /* 0x803F1204 - sends an invite to `id`; `kind` is the invite type. */
    void inviteFriend(const NetId* id, s32 kind);
    /* 0x803F129C - removes `id` from the friend list. */
    void removeFriend(const NetId* id);
    /* 0x803F139C - sends a friend request to `id`. */
    void sendFriendRequest(const NetId* id);
    /* 0x803F1424 - accepts the friend request from `id`. */
    void acceptFriendRequest(const NetId* id);
    /* 0x803F1324 - requests the block list (once). */
    void requestBlockList(void);

    /* +0x0004 */ u8 pad_0004[0x25E8];
};

#endif
