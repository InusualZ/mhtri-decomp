/*
 * SO/socket.h - declarations of the symbols owned by `SO/socket.cpp` (the SOCKET library) that other units
 * call or read.
 */
#ifndef SO_SOCKET_H
#define SO_SOCKET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80520658 / 0x80520664 - the peer-id word at `msg+8` and the negotiator socket token a record's
 * +0x08 holds.  Retail's body for both is a bare `blr`: the argument comes back unchanged, so this
 * build has them as two 4-byte SOCKET entries that do no conversion, and their names are taken from
 * how their call sites use the result (`DWCi_socketTokenFromPeer(token)`, the peer word read out of
 * a received `msg+8`, compared against a record's token; `DWCi_peerTokenFromSocket(rec->token)`,
 * written into `msg+8` of an outgoing one). */
s32 DWCi_socketTokenFromPeer(u32 peerId);

s32 DWCi_peerTokenFromSocket(s32 sock);

/* 0x80520604 - format the network-order address word at *addr as "a.b.c.d". */
char* SOAddressToString(u32* addr);

/* 0x8052065C - the port conversion `DWCi_sendControlFrame`/`DWCi_sendTo` store into a connection. */
u16 SOAddressToHostPort(u16 port);

/* 0x8051FAC0 - close the socket whose descriptor is inside the SO band's connection record; the
 * NHTTP library's async cleanup closes the one `NHTTPi_Startup` opened (`-1` when there is none). */
s32 SOClose(s32 fd);

/* 0x80521054 - set one socket option; `value` is the option's own bytes. */
s32 SOSetSockOpt(s32 fd, s32 level, s32 option, u32* value, s32 length);

/* 0x805202E8 - shut the socket down (how 2 = both directions). */
s32 SOShutdown(s32 fd, s32 how);

/* 0x80520148 / 0x80520194 - receive into / send from a byte range; the byte count, or a negative error. */
s32 SORecv(s32 fd, void* buf, s32 length, s32 flags); /* untyped: byte range */

s32 SOSend(s32 fd, void* buf, s32 length, s32 flags); /* untyped: byte range */

/* 0x80520B54 - the console's own host address, stored by `sNetworkLibraryWii::start` as its host id. */
u32 SOGetHostID(void);

/* The IPv4 socket address `SOConnect` takes: the length byte (8), the family (2, AF_INET), the port in
 * network order and the address word. size: 0x8 */
typedef struct SOSockAddrIn {
    /* +0x0 */ u8 len;
    /* +0x1 */ u8 family;
    /* +0x2 */ u16 port;
    /* +0x4 */ u32 addr;
} SOSockAddrIn;

/* One IPv4 address word of a resolver answer. size: 0x4 */
typedef struct SOInAddr {
    /* +0x0 */ u32 addr;
} SOInAddr;

/* The resolver answer `SOGetHostByName` returns (a `hostent`): name, aliases, address type, address length
 * and the NULL-terminated address list. size: 0x10 */
typedef struct SOHostEnt {
    /* +0x00 */ char* name;
    /* +0x04 */ char** aliases;
    /* +0x08 */ s16 addrType;
    /* +0x0A */ s16 length;
    /* +0x0C */ SOInAddr** hosts;
} SOHostEnt;

/* 0x80520BCC - resolves a host name (NULL when it cannot be resolved). */
SOHostEnt* SOGetHostByName(const char* name);

/* The poll record `SOPoll` takes (`NetworkSocketWii::pollConnect` stores fd/events/revents with `stw` at
 * +0/+4/+8 and reads revents back). size: 0xC */
typedef struct SOPollFD {
    /* +0x0 */ s32 fd;
    /* +0x4 */ s32 events;
    /* +0x8 */ s32 revents;
} SOPollFD;

/* 0x8051FE40 - connect the socket to the address. */
s32 SOConnect(s32 fd, SOSockAddrIn* addr);

/* 0x80521158 - read one option of a network interface (NULL: the default one) into `value` (`*length`
 * bytes); negative on error.  The mediator's link check passes NULL, level 0xFFFE, option 0x1005 and a word. */
struct SOInterface;
s32 SOGetInterfaceOpt(struct SOInterface* iface, s32 level, s32 option, void* value, s32* length); /* untyped: byte range */

/* 0x8051F9D0 */
s32 SOSocket(s32 domain, s32 type, s32 protocol);
/* 0x8051FB64 */
s32 SOListen(s32 fd, s32 backlog);
/* 0x8051FC18 */
s32 SOAccept(s32 fd, SOSockAddrIn* address);
/* 0x8051FD58 */
s32 SOBind(s32 fd, const SOSockAddrIn* address);
/* 0x8051FF28 */
s32 SOGetSockName(s32 fd, SOSockAddrIn* address);
/* 0x80520024 */
s32 SOGetPeerName(s32 fd, SOSockAddrIn* address);
/* 0x80520120 */
s32 SORecvFrom(s32 fd, u8* buffer, s32 length, s32 flags, SOSockAddrIn* from);
/* 0x8052016C */
s32 SOSendTo(s32 fd, const u8* buffer, s32 length, s32 flags, const SOSockAddrIn* to);
/* 0x805201B8 */
s32 SOFcntl(s32 fd, s32 command, ...);
/* 0x8052039C */
s32 SOPoll(SOPollFD* fds, u32 count, s64 timeout);

/* 0x80520668 - host-to-network 16-bit byte swap. */
u16 SOHtoNs(u16 port);

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

/* 0x80520D0C / 0x80520FF0 - resolve a host name (getaddrinfo) and release the result. */
s32 SOGetAddrInfo(const char* node, const char* service, const SOAddrInfo* hints, SOAddrInfo** result);
void SOFreeAddrInfo(SOAddrInfo* info);

/* 0x805204F8 - parse the dotted-quad text `name` into the four address bytes at `out`; 1 on success
 * (the SDK's INETATON ioctl), negative on failure. */
s32 SOInetAtoN(const char* name, u8* out);

/* 0x80520670 / 0x80520928 */
s32 RecvFrom(const char* function, s32 fd, u8* buffer, s32 length, s32 flags, SOSockAddrIn* from);
s32 SendTo(const char* function, s32 fd, const u8* buffer, s32 length, s32 flags, const SOSockAddrIn* to);

#ifdef __cplusplus
}
#endif

#endif
