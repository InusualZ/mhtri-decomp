/*
 * Network/NetworkPeerMcs.h - the Mcs peer class `Network/NetworkPeerMcs.cpp` defines and the record a peer
 *   publishes itself from.
 */

#ifndef MHTRI_NETWORK_NETWORKPEERMCS_H
#define MHTRI_NETWORK_NETWORKPEERMCS_H

#include "types.h"
#include "Network/NetworkPeerBase.h"
#include "Network/network_socket_streams.h"

/* The Mcs peer (table 0x805F95E0, the `NetworkPeerMcs` the log strings name): the abstract peer, the
   one-byte flags, the state the connect machine walks, its own 0x2400-byte work area and the connection
   it registers with. */
class NetworkPeerMcs : public NetworkPeerBase {
public:
    NetworkPeerMcs(u8 armed);
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C (GUESS: the record it publishes itself from) */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C (GUESS: the connect machine) */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 (GUESS: closes the peer's connection) */ virtual void reset();

    u8  armed_10;                    /* +0x10 - set while the peer has something to drop */
    u8  pad_11[0x03];                /* +0x11..+0x13 */
    s32 state_14;                    /* +0x14 - the connect machine's state */
    u8  dropped_18;                  /* +0x18 - raised when the drop lands */
    u8  work_19[0x2400];             /* +0x19..+0x2418 - the peer's own work area */
    u32 workUsed_241C;               /* +0x241C - bytes in use */
    NetworkPeerAddress peerAddress_2420; /* +0x2420..+0x2425 - the address the peer publishes */
    u8  pad_2426[0x02];              /* +0x2426..+0x2427 */
    NetworkSingleTcp* connection_2428; /* +0x2428 - the connection the peer registers with */
    char label_242C[0x40];           /* +0x242C..+0x246B - the label the peer publishes */
    s32  labelSize_246C;             /* +0x246C - bytes of it in use */
};   /* size: 0x2470 */

/* The record a peer publishes itself from: the six-byte address at +0x00, its connection at +0x08 and
   the label it answers to with its used length (GUESS on the name - the setInfo slot and the stream
   writer are the only readers). */
typedef struct NetworkPeerInfo {
    NetworkPeerAddress address_00;      /* +0x00..+0x05 */
    u8  pad_06[0x02];                   /* +0x06..+0x07 */
    NetworkSingleTcp* connection_08;    /* +0x08 - the connection the peer registers with */
    char label_0C[0x40];                /* +0x0C..+0x4B - the label it publishes */
    s32  labelSize_4C;                  /* +0x4C - bytes of it in use */
} NetworkPeerInfo;   /* size: 0x50 */

#endif /* MHTRI_NETWORK_NETWORKPEERMCS_H */
