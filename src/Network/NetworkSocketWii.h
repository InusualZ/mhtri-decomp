/*
 * Network/NetworkSocketWii.h - the class `src/Network/NetworkSocketWii.cpp` defines (`.text` 0x803F7538..0x803F84B8):
 * the Wii socket over the SO library (and SSL for mode 4), and the two error accessors the socket streams poll.
 */
#ifndef MHTRI_NETWORK_NETWORKSOCKETWII_H
#define MHTRI_NETWORK_NETWORKSOCKETWII_H

#include "types.h"
#include "Network/NetworkFileFetcher.h"   /* NetworkSocketBase */

class NetworkSocketHandle;

/* The Wii socket `sNetworkLibraryWii::createSocket` allocates (table 0x805FC984; the "NetworkSocketWii::init()" /
   "::open()" log strings name the class): an SO descriptor opened in one of three modes - 1 TCP, 2 UDP, 4 TCP under
   SSL - the SSL context for mode 4, the connect machine's step and the time the SSL handshake started.  Field and
   slot names are GUESSes from the bodies. */
class NetworkSocketWii : public NetworkSocketBase {
public:
    NetworkSocketWii();
    /* +0x08 */ virtual ~NetworkSocketWii();
    /* +0x0C */ virtual s32 open(s32 mode);
    /* +0x10 */ virtual s32 openSecure(const char* host, const u8* rootCA, s32 rootCASize);
    /* +0x14 */ virtual s32 shutdown();
    /* +0x18 */ virtual s32 connect(const NetworkPeerAddress* address);
    /* +0x1C */ virtual s32 pollConnect();
    /* +0x20 */ virtual s32 listen(const NetworkPeerAddress* address);
    /* +0x24 */ virtual s32 accept();
    /* +0x28 */ virtual s32 isConnected();
    /* +0x2C */ virtual s32 getLocalAddress(NetworkPeerAddress* out);
    /* +0x30 */ virtual s32 getPeerAddress(NetworkPeerAddress* out);
    /* +0x34 */ virtual s32 close();
    /* +0x38 */ virtual s32 send(const u8* data, s32 size, const NetworkPeerAddress* address);
    /* +0x3C */ virtual s32 receive(u8* out, s32 size, NetworkPeerAddress* address);
    /* +0x40 */ virtual void getAnyAddress(NetworkPeerAddress* out);
    /* +0x44 */ virtual void getBroadcastAddress(NetworkPeerAddress* out);
    /* +0x48 */ virtual s32 getMode();

    s32 setOptions();
    u32 getErrorSource(s32 code);

    /* +0x0C */ s32 socket_0C;          /* the SO descriptor, -1 when closed */
    /* +0x10 */ s32 connected_10;
    /* +0x14 */ s32 mode_14;            /* 0 closed, 1 TCP, 2 UDP, 4 TCP under SSL */
    /* +0x18 */ s32 sslContext_18;      /* the SSLNew context, 0 when none */
    /* +0x1C */ u8 connectStep_1C;      /* 0 polling the connect, 1 the SSL handshake */
    /* +0x1D */ u8 pad_1D[3];
    /* +0x20 */ f32 handshakeStart_20;  /* the logger clock when the SSL connect was issued */
};   /* size: 0x24 (the `__nw` size `createSocket` allocates) */

#ifdef __cplusplus
extern "C" {
#endif

/* The socket readers `network_socket_streams.cpp` polls: the error source and the error code of the socket
 * (its +0x04 and +0x08 words; GUESS on the first name, which predates the class). */
/* untyped: opaque handle passed through - the transport users hold the socket through their own view */
s32 getBytesAvailableToRead(void* handle);

/* 0x803F7540 - the last error code the socket handle recorded (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKSOCKETWII_H */
