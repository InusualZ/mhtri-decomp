/*
 * Network/NetworkPeerUdp.h - the Udp peer class `Network/NetworkPeerUdp.cpp` defines and the binding it copies in.
 */

#ifndef MHTRI_NETWORK_NETWORKPEERUDP_H
#define MHTRI_NETWORK_NETWORKPEERUDP_H

#include "types.h"
#include "Network/NetworkPeerBase.h"
#include "Network/network_socket_streams.h"

/* ---------------- the peers ------------------------------------------------------------------- */

/* The Udp peer (table 0x805F9540): the abstract peer, the peer index it owns on the socket at +0x10 and
   the socket at +0x14.  Its constructor lives outside this unit (the table is stored by an unowned
   function); the `destroy` defined here is the key function that makes MWCC emit the table. */
class NetworkPeerUdp : public NetworkPeerBase {
public:
    /* 0x803CA3BC (`Network/NetworkConnection.cpp`, where the connection base creates the Udp peer) */
    NetworkPeerUdp();
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C (docs/memory-dump.md names this one `setPeerAndSocket`) */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 (`sendPackets`) */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 (`receivePackets`) */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 (GUESS: the reset helper's tail twin) */ virtual void reset();

    s32 peerIndex_10;                   /* +0x10 - the slot the peer owns on the Udp socket */
    NetworkMultipleUdp* udp_14;         /* +0x14 - the socket */
};   /* size: 0x18 (approximation - only the two fields the range touches are evidenced) */

/* The pair `setPeerAndSocket` copies in. */
struct NetworkPeerUdpBinding {
    s32 peerIndex;                      /* +0x00 */
    NetworkMultipleUdp* udp;            /* +0x04 */
};   /* size: 0x08 */

#endif /* MHTRI_NETWORK_NETWORKPEERUDP_H */
