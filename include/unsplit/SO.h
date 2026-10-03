/*
 * SO declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The three SDK socket helpers at 0x80520604/0x8052065C/0x80520668 - `SOAddressToString` (a `u32*` address
 * word rendered as text), `SOAddressToHostPort` (a port through the SDK's own conversion) and `SOHtoNs`
 * (host -> network u16) - sit in the SO/SSL band that no registered unit covers.  stylelint's rule 2
 * cannot place them (`resolve` returns `unsplit` with no module: the nearest registered ranges are
 * `NWC24/nwc24_io.c` below and the game-UI band above, different modules); `SOHtoNs`'s own `SO`
 * prefix and its neighbours' socket vocabulary name the library, and no `include/SO/` owner exists,
 * so this file is their home.
 *
 * `SOHtoNs` was declared in `include/unsplit/Network.h` (the Network band's header, which is
 * C++-only because it carries a class); it moved here and that header now includes this one, so the
 * symbol has one home and the DWCi units (plain `.c`) can reach it.
 *
 * Added with the networking conformance pass.
 */
#ifndef MHTRI_UNSPLIT_SO_H
#define MHTRI_UNSPLIT_SO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80520668 - host-to-network 16-bit byte swap. */
u16 SOHtoNs(u16 port);

/* The IPv4 socket address `SOConnect` takes: the length byte (8), the family (2, AF_INET), the port in
 * network order and the address word. size: 0x8 */
typedef struct SOSockAddrIn {
    /* +0x0 */ u8 len;
    /* +0x1 */ u8 family;
    /* +0x2 */ u16 port;
    /* +0x4 */ u32 addr;
} SOSockAddrIn;

/* The resolver result `SOGetAddrInfo` fills and `SOFreeAddrInfo` releases (a getaddrinfo record).
 * size: 0x20 */
typedef struct SOAddrInfo {
    /* +0x00 */ s32 flags;
    /* +0x04 */ s32 family;
    /* +0x08 */ s32 socketType;
    /* +0x0C */ s32 protocol;
    /* +0x10 */ u32 addrLength;
    /* +0x14 */ char* canonName;
    /* +0x18 */ SOSockAddrIn* addr;
    /* +0x1C */ struct SOAddrInfo* next;
} SOAddrInfo;

/* 0x8051FE40 - connect the socket to the address. */
s32 SOConnect(s32 fd, SOSockAddrIn* addr);

/* 0x80520D0C / 0x80520FF0 - resolve a host name (getaddrinfo) and release the result. */
s32 SOGetAddrInfo(const char* node, const char* service, const SOAddrInfo* hints, SOAddrInfo** result);
void SOFreeAddrInfo(SOAddrInfo* info);

/* 0x805204F8 - parse the dotted-quad text `name` into the four address bytes at `out`; 1 on success
 * (the SDK's INETATON ioctl), negative on failure. */
s32 SOInetAtoN(const char* name, u8* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_SO_H */
