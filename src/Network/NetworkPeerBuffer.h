/*
 * Network/NetworkPeerBuffer.h - the peer payload buffer class `Network/NetworkPeerBuffer.cpp` defines.
 */

#ifndef MHTRI_NETWORK_NETWORKPEERBUFFER_H
#define MHTRI_NETWORK_NETWORKPEERBUFFER_H

#include "types.h"
#include "Network/NetworkPeerBase.h"

/* ---------------- the peer payload buffer --------------------------------------------------- */

/* The 0x2014-byte peer buffer (table 0x805F9510): the 0x2000-byte payload at +0x10 and the used-byte
   count at +0x2010.  Its `init` slot dispatches the clear through +0x28, which is MWCC's own virtual-call
   shape. */
class NetworkPeerBuffer : public NetworkPeerBase {
public:
    NetworkPeerBuffer();
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C (docs/memory-dump.md names this one `clearBuffer`) */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 */ virtual void reset();

    u8 payload_10[0x2000];           /* +0x0010 - the bytes queued for the socket */
    u32 used_2010;                   /* +0x2010 - bytes in use */
};   /* size: 0x2014 */

#endif /* MHTRI_NETWORK_NETWORKPEERBUFFER_H */
