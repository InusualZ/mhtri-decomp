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
#include "Network/NetworkLayerPat.h"   /* NetId */

/* The roster block `NetCtrlWk::roster_sync_0x61CC` holds (defined by the work record header). */
struct NetRosterSync;

class NetworkCommunityPat {
public:
    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void pad_0C();
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
};

#endif
