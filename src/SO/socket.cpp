/*
 * SO/socket.cpp - the SOCKET library (`SOSocket`, `SOClose`, `SOConnect`, `SORecvFrom`, `SOSendTo`, `SOPoll`,
 *   `SOGetHostByName`, `SOGetAddrInfo`, ...): the RVL_SDK "SOCKET" build over the SO library's
 *   `/dev/net/ip/top` session (`SOiPrepare` / `SOiAlloc` / `SOiFree` / `SOiConclude` in `SO/soi.cpp`).
 *
 * RANGE. `.text` 0x8051F9D0..0x80521340 (29 functions, the last 0xC bytes alignment padding); `.data`
 *   0x80631410..0x80631468; `.bss` 0x80766C58..0x80766C68; `.sdata` 0x80794450..0x80794458; `.sbss`
 *   0x807958E8..0x807958F0.
 * RANGE. The SOCKET version string ("SOCKET release build ... 12:00:01", `.data` 0x80631410) is registered by
 *   `SOSocket`, the first function past `SO/soi.cpp`'s `__SOCreateSocket`, through `socketRegistered` (`.sbss`
 *   0x807958E8, read only by `SOSocket`); the unit's `.data`, `.sdata` and `.sbss` each start 8-aligned after the
 *   SO TU's, and no SO data object (`soWork`, `soState`, `soLastError`, `soBufferAddrCheck`) has a referrer in
 *   this range (`tools/units/callers.py`).
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24` (docs/network.md "SDK library compilers"), compiled as
 *   C++ with C linkage like its sibling; the file sets `#pragma auto_inline off`.
 * NAMES. `socket.cpp` is the SDK's library name ("SOCKET"), not a recovered file name (GUESS). The function names
 *   are the map's, GUESSes as listed in `SO/soi.cpp`'s header: `SOSocket`, `SOListen`, `SOAccept`,
 *   `SOGetSockName`, `SOGetPeerName`, `SORecvFrom`, `SOSendTo`, `SOFcntl`, `SOPoll`, `SOInetAtoN`,
 *   `SOAddressToString`, `DWCi_socketTokenFromPeer`, `SOAddressToHostPort`, `DWCi_peerTokenFromSocket`,
 *   `SOGetHostByName`, `SOGetAddrInfo`, `SOFreeAddrInfo`; `socketVersion`, `socketRegistered` and `soAddressText`
 *   are GUESSes in the SDK's scheme.
 * SHAPES. Every request is `SOiPrepare` -> `SOiAlloc`'d IPC block -> `IOS_Ioctl(v)` -> `SOiFree` -> `SOiConclude`,
 *   written `if (result == 0) { ...; result = SOiConclude(...); } return result;`; the local declaration order is
 *   load-bearing for the register allocation; the MEM2 / alignment checks are static inlines.
 * RESIDUALS. `SOAddressToString` takes its address BY VALUE in retail (`char* SOAddressToString(SOInAddr in)`):
 *   every caller stores the word to a stack slot and passes its address, and the by-value definition measures
 *   100 % here; the prototype stays `u32*` until the callers in `DWCi/natneg.c` and `DWCi/fn_805113B0.c` pass
 *   the struct (request net-so#2).
 * RESIDUALS. Register allocation only: `SOGetSockName` /
 *   `SOGetPeerName` (the `NETMemCpy` argument moves ordered `r4`, `r5`, `r3` in retail; three spellings of the
 *   copy measured identical), `RecvFrom`, `SendTo`, `SOGetHostByName` (`result` and the text block swap r29/r31;
 *   a pairwise declaration-order search and reusing `result` for the length measured identical),
 *   `SOGetInterfaceOpt` (parameter registers swapped), `RecvFrom` / `SendTo` (the parameters sit two / one
 *   registers lower than retail's r25-r30; a pairwise declaration-order search on `RecvFrom` measured identical).
 * SHAPES. `RecvFrom` names the argument record (`args = &block->args`), which is what makes retail address the
 *   peer-address bytes from it (`addi r21,r4,0x20`) and save from r21; its copy-back length is
 *   `from->len > address[0] ? (u32)address[0] : (u32)from->len` (u8 arms add a `clrlwi`).
 */
#include "SO/socket.h"
#include "SO/soi.h"
#include "NCD/ncdsystem.h"
#include "IPC/ipcclt.h"
#include "unsplit/OS.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/__va_arg.h"

#pragma auto_inline off

extern "C" {

/* An ioctl block: the request words, then the request's variable-length payload at +0x20. */
typedef struct SOIoctlBlock {
    /* +0x00 */ s32 words[8];
    /* +0x20 */ u8 payload[0x20];
} SOIoctlBlock; /* size: 0x40 (approximation: the payload grows with the request) */

/* The `SOPoll` block: the timeout in milliseconds, then the poll records. */
typedef struct SOPollBlock {
    /* +0x00 */ s64 timeout;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u8 fds[0x20];
} SOPollBlock; /* size: 0x40 (approximation: the records grow with the count) */

/* The `SOGetAddrInfo` request block: the four vectors, then the node and service texts and the hints. */
typedef struct SOAddrInfoRequest {
    /* +0x00 */ IPCIOVector vectors[4];
    /* +0x20 */ char text[0x20];
} SOAddrInfoRequest; /* size: 0x40 (approximation: the texts grow with the names) */

/* One socket-address slot of a `SOGetAddrInfo` answer. */
typedef struct SOSockAddrStorage {
    /* +0x00 */ u8 bytes[0x1C];
} SOSockAddrStorage; /* size: 0x1C */

/* The `SOGetAddrInfo` answer: 35 records, then their 35 address slots. */
typedef struct SOAddrInfoAnswer {
    /* +0x000 */ SOAddrInfo records[35];
    /* +0x460 */ SOSockAddrStorage addresses[35];
} SOAddrInfoAnswer; /* size: 0x834 */

/* The resolver buffer `SOGetHostByName` answers into: the entry, its name text, then the address list. */
typedef struct SOHostBuffer {
    /* +0x000 */ SOHostEnt entry;
    /* +0x010 */ char name[0x330];
    /* +0x340 */ u32 hosts[0x48];
} SOHostBuffer; /* size: 0x460 */

/* The `SOGetInterfaceOpt` block: the vectors, the level/option request, the answer length and the answer. */
typedef struct SOInterfaceOptBlock {
    /* +0x00 */ IPCIOVector vectors[3];
    /* +0x18 */ u8 pad_0x18[0x08];
    /* +0x20 */ s32 request[8];
    /* +0x40 */ s32 answerLength[8];
    /* +0x60 */ u8 answer[0x20];
} SOInterfaceOptBlock; /* size: 0x80 (approximation: the answer grows with the option) */

/* The IPC block of a receive / send: the vectors, then the request words. */
typedef struct SORecvArgs {
    /* +0x00 */ s32 fd;
    /* +0x04 */ s32 flags;
    /* +0x08 */ u8 pad_0x08[0x18];
    /* +0x20 */ u8 address[0x20];
} SORecvArgs; /* size: 0x40 */

typedef struct SORecvBlock {
    /* +0x00 */ IPCIOVector vectors[3];
    /* +0x18 */ u8 pad_0x18[0x08];
    /* +0x20 */ SORecvArgs args;
} SORecvBlock; /* size: 0x60 */

typedef struct SOSendArgs {
    /* +0x00 */ s32 fd;
    /* +0x04 */ s32 flags;
    /* +0x08 */ s32 hasAddress;
    /* +0x0C */ u8 address[0x1C];
} SOSendArgs; /* size: 0x28 */

typedef struct SOSendBlock {
    /* +0x00 */ IPCIOVector vectors[2];
    /* +0x10 */ u8 pad_0x10[0x10];
    /* +0x20 */ SOSendArgs args;
    /* +0x48 */ u8 pad_0x48[0x18];
} SOSendBlock; /* size: 0x60 */

/* Non-zero when `p` is a MEM2 address (the IPC buffers must be). */
static inline BOOL SOiIsMem2(u32 address) {
    u32 physical = address & 0x1FFFFFFF;
    return physical >= 0x10000000 && physical < 0x18000000;
}

char* socketVersion = "<< RVL_SDK - SOCKET \trelease build: Jun  9 2009 12:00:01 (0x4199_60831) >>";
u32 socketRegistered;
char soAddressText[0x10];

/* 0x8051F9D0 (0xF0): opens a socket (registering the SOCKET build first). */
s32 SOSocket(s32 domain, s32 type, s32 protocol) {
    s32 fd;
    s32* block;
    s32 result;

    if (!socketRegistered) {
        OSRegisterVersion(socketVersion);
        socketRegistered = 1;
    }
    result = SOiPrepare(NULL, &fd);
    if (result == 0) {
        if (domain == 23) {
            result = -5;
        } else {
            block = (s32*)SOiAlloc(12, 0x20);
            if (block == NULL) {
                result = -49;
            } else {
                block[0] = domain;
                block[1] = type;
                block[2] = protocol;
                result = IOS_Ioctl(fd, 0xF, block, 0xC, NULL, 0);
                SOiFree(12, block, 0x20);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8051FAC0 (0xA4): closes a socket. */
s32 SOClose(s32 fd) {
    s32 device;
    s32 result;
    s32* block;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        block = (s32*)SOiAlloc(12, 0x20);
        if (block == NULL) {
            result = -49;
        } else {
            block[0] = fd;
            result = IOS_Ioctl(device, 3, block, 4, NULL, 0);
            SOiFree(12, block, 0x20);
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8051FB64 (0xB4): starts listening. */
s32 SOListen(s32 fd, s32 backlog) {
    s32 device;
    s32 result;
    s32* block;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        block = (s32*)SOiAlloc(12, 0x20);
        if (block == NULL) {
            result = -49;
        } else {
            block[0] = fd;
            block[1] = backlog;
            result = IOS_Ioctl(device, 0xA, block, 8, NULL, 0);
            SOiFree(12, block, 0x20);
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8051FC18 (0x140): accepts a connection (its address into `address` when given). */
s32 SOAccept(s32 fd, SOSockAddrIn* address) {
    s32 device;
    s32 size;
    s32 result;
    SOIoctlBlock* block;
    u8* out;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (address != NULL && (address->len > 8 || address->len < 8)) {
            result = -28;
        } else {
            size = ((address == NULL ? 4 : address->len + 0x20) + 0x1F) & ~0x1F;
            block = (SOIoctlBlock*)SOiAlloc(12, size);
            if (block == NULL) {
                result = -49;
            } else {
                block->words[0] = fd;
                out = block->payload;
                if (address == NULL) {
                    result = IOS_Ioctl(device, 1, block, 4, NULL, 0);
                } else {
                    NETMemCpy(out, address, address->len);
                    result = IOS_Ioctl(device, 1, block, 4, out, address->len);
                    if (result >= 0) {
                        NETMemCpy(address, out, out[0]);
                    }
                }
                SOiFree(12, block, size);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* Binds or connects a socket (the two requests share the block layout). */
static inline s32 SOiAddressRequest(s32 fd, const SOSockAddrIn* address, s32 command) {
    s32 device;
    s32* block;
    s32 result;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (address == NULL || address->len > 8 || address->len < 8) {
            result = -28;
        } else {
            block = (s32*)SOiAlloc(12, 0x40);
            if (block == NULL) {
                result = -49;
            } else {
                block[0] = fd;
                block[1] = 1;
                NETMemCpy(&block[2], address, address->len);
                result = IOS_Ioctl(device, command, block, 0x24, NULL, 0);
                SOiFree(12, block, 0x40);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8051FD58 (0xE8): binds a socket. */
s32 SOBind(s32 fd, const SOSockAddrIn* address) {
    return SOiAddressRequest(fd, address, 2);
}

/* 0x8051FE40 (0xE8): connects a socket. */
s32 SOConnect(s32 fd, SOSockAddrIn* address) {
    return SOiAddressRequest(fd, address, 4);
}

/* Reads a socket's own or peer address. */
static inline s32 SOiNameRequest(s32 fd, SOSockAddrIn* address, s32 command) {
    s32 device;
    u8* out;
    SOIoctlBlock* block;
    s32 result;
    s32 size;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (address == NULL || address->len > 8 || address->len < 8) {
            result = -28;
        } else {
            size = (address->len + 0x3F) & ~0x1F;
            block = (SOIoctlBlock*)SOiAlloc(12, size);
            if (block == NULL) {
                result = -49;
            } else {
                block->words[0] = fd;
                out = block->payload;
                NETMemCpy(out, address, address->len);
                result = IOS_Ioctl(device, command, block, 4, out, address->len);
                if (result >= 0) {
                    NETMemCpy(address, out, out[0]);
                }
                SOiFree(12, block, size);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8051FF28 (0xFC): the socket's own address. */
s32 SOGetSockName(s32 fd, SOSockAddrIn* address) {
    return SOiNameRequest(fd, address, 7);
}

/* 0x80520024 (0xFC): the socket's peer address. */
s32 SOGetPeerName(s32 fd, SOSockAddrIn* address) {
    return SOiNameRequest(fd, address, 6);
}

/* 0x80520120 (0x28): receives into `buffer`, the sender's address into `from`. */
s32 SORecvFrom(s32 fd, u8* buffer, s32 length, s32 flags, SOSockAddrIn* from) {
    return RecvFrom(NULL, fd, buffer, length, flags, from);
}

/* 0x80520148 (0x24): receives into `buffer`. */
/* untyped: byte range */
s32 SORecv(s32 fd, void* buffer, s32 length, s32 flags) {
    return RecvFrom(NULL, fd, (u8*)buffer, length, flags, NULL);
}

/* 0x8052016C (0x28): sends `buffer` to `to`. */
s32 SOSendTo(s32 fd, const u8* buffer, s32 length, s32 flags, const SOSockAddrIn* to) {
    return SendTo(NULL, fd, buffer, length, flags, to);
}

/* 0x80520194 (0x24): sends `buffer`. */
/* untyped: byte range */
s32 SOSend(s32 fd, void* buffer, s32 length, s32 flags) {
    return SendTo(NULL, fd, (const u8*)buffer, length, flags, NULL);
}

/* 0x805201B8 (0x130): a socket control command with one integer argument. */
s32 SOFcntl(s32 fd, s32 command, ...) {
    va_list args;
    s32 value;
    s32 device;
    s32 result;
    s32* block;

    va_start(args, command);
    value = *(s32*)__va_arg(args, 1);
    va_end(args);
    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        block = (s32*)SOiAlloc(12, 0x20);
        if (block == NULL) {
            result = -49;
        } else {
            block[0] = fd;
            block[1] = command;
            block[2] = value;
            result = IOS_Ioctl(device, 5, block, 0xC, NULL, 0);
            SOiFree(12, block, 0x20);
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x805202E8 (0xB4): shuts a socket down. */
s32 SOShutdown(s32 fd, s32 how) {
    s32 device;
    s32 result;
    s32* block;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        block = (s32*)SOiAlloc(12, 0x20);
        if (block == NULL) {
            result = -49;
        } else {
            block[0] = fd;
            block[1] = how;
            result = IOS_Ioctl(device, 0xE, block, 8, NULL, 0);
            SOiFree(12, block, 0x20);
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x8052039C (0x15C): polls `count` sockets for `timeout` ticks (-1: forever). */
s32 SOPoll(SOPollFD* fds, u32 count, s64 timeout) {
    s32 device;
    s32 size;
    s32 length;
    s32 result;
    SOPollBlock* block;
    u8* out;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (fds == NULL) {
            result = -28;
        } else {
            length = count * sizeof(SOPollFD);
            size = (length + 0x3F) & ~0x1F;
            block = (SOPollBlock*)SOiAlloc(12, size);
            if (block == NULL) {
                result = -49;
            } else {
                out = block->fds;
                if (timeout <= -1) {
                    NETMemCpy(&block->timeout, &timeout, 8);
                } else {
                    block->timeout = timeout / (OS_BUS_CLOCK / 4 / 1000);
                }
                NETMemCpy(out, fds, length);
                result = IOS_Ioctl(device, 0xB, block, 8, out, length);
                if (result >= 0) {
                    NETMemCpy(fds, out, length);
                }
                SOiFree(12, block, size);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x805204F8 (0x10C): parses a dotted-quad address. */
s32 SOInetAtoN(const char* name, u8* out) {
    s32 device;
    s32 opened;
    s32 size;
    s32 result;
    SOIoctlBlock* block;
    char* text;

    result = SOiPrepareTempRm(NULL, &device, &opened);
    if (result == 0) {
        if (name == NULL) {
            result = -28;
        } else {
            size = (strlen(name) + 0x40) & ~0x1F;
            block = (SOIoctlBlock*)SOiAlloc(12, size);
            if (block == NULL) {
                result = -49;
            } else {
                text = (char*)block->payload;
                if (name != NULL) {
                    strcpy(text, name);
                }
                result = IOS_Ioctl(device, 0x15, text, strlen(name), block, 4);
                if (result >= 0 && out != NULL) {
                    NETMemCpy(out, block->words, 4);
                }
                SOiFree(12, block, size);
            }
        }
        result = SOiConcludeTempRm(NULL, result, opened);
    }
    return result;
}

/* 0x80520604 (0x54): formats a network-order address word as "a.b.c.d". */
char* SOAddressToString(u32* addr) {
    u8* bytes = (u8*)addr;
    sprintf(soAddressText, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return soAddressText;
}

/* 0x80520658 (0x4): network-to-host order of a word (identity on this big-endian machine). */
s32 DWCi_socketTokenFromPeer(u32 peerId) {
    return peerId;
}

/* 0x8052065C (0x8): network-to-host order of a port. */
u16 SOAddressToHostPort(u16 port) {
    return port;
}

/* 0x80520664 (0x4): host-to-network order of a word. */
s32 DWCi_peerTokenFromSocket(s32 sock) {
    return sock;
}

/* 0x80520668 (0x8): host-to-network order of a port. */
u16 SOHtoNs(u16 port) {
    return port;
}

/* Non-zero when a caller buffer can be handed to IOS as is: 32-byte aligned, a multiple of 32 bytes, in MEM2. */
static inline BOOL SOiIsAlignedBuffer(const u8* buffer, s32 length) {
    return !((u32)buffer & 0x1F) && length % 32 == 0;
}

static inline BOOL SOiIsDirectBuffer(const u8* buffer, s32 length) {
    BOOL direct = FALSE;
    BOOL usable;
    if (SOiIsAlignedBuffer(buffer, length)) {
        usable = TRUE;
        if (SOiIsBufferAddrCheck() && !SOiIsMem2((u32)buffer)) {
            usable = FALSE;
        }
        if (usable) {
            direct = TRUE;
        }
    }
    return direct;
}

/* 0x80520670 (0x2B8): receives up to 32 KB, through a bounce buffer when `buffer` is not IPC-ready. */
s32 RecvFrom(const char* function, s32 fd, u8* buffer, s32 length, s32 flags, SOSockAddrIn* from) {
    s32 device;
    s32 result;
    BOOL direct;
    s32 size;
    SORecvBlock* block;
    u8* data;
    u8* address;
    SORecvArgs* args;

    if (length > 0x8000) {
        length = 0x8000;
    }
    result = SOiPrepare(function, &device);
    if (result == 0) {
        if (from != NULL && (from->len > 8 || from->len < 8)) {
            result = -28;
        } else if (length < 0 || (length > 0 && buffer == NULL)) {
            result = -28;
        } else {
            direct = TRUE;
            if (length != 0 && !SOiIsDirectBuffer(buffer, length)) {
                direct = FALSE;
            }
            size = ((from == NULL ? 0 : from->len) + 0x5F) & ~0x1F;
            block = (SORecvBlock*)SOiAlloc(12, size);
            data = !direct ? (u8*)SOiAlloc(13, (length + 0x1F) & ~0x1F) : buffer;
            if (block == NULL || data == NULL) {
                result = -49;
            } else {
                args = &block->args;
                args->fd = fd;
                args->flags = flags;
                address = args->address;
                block->vectors[0].base = args;
                block->vectors[0].length = 8;
                block->vectors[1].base = data;
                block->vectors[1].length = length;
                if (from == NULL) {
                    block->vectors[2].base = NULL;
                    block->vectors[2].length = 0;
                    result = IOS_Ioctlv(device, 0xC, 1, 2, block->vectors);
                } else {
                    NETMemCpy(address, from, from->len);
                    block->vectors[2].base = address;
                    block->vectors[2].length = from->len;
                    result = IOS_Ioctlv(device, 0xC, 1, 2, block->vectors);
                    if (result >= 0) {
                        NETMemCpy(from, address, from->len > address[0] ? (u32)address[0] : (u32)from->len);
                    }
                }
                if (result >= 0 && !direct) {
                    NETMemCpy(buffer, data, length);
                }
            }
            if (!direct) {
                SOiFree(13, data, (length + 0x1F) & ~0x1F);
            }
            SOiFree(12, block, size);
        }
        result = SOiConclude(function, result);
    }
    return result;
}

/* 0x80520928 (0x22C): sends, through a bounce buffer when `buffer` is not IPC-ready. */
s32 SendTo(const char* function, s32 fd, const u8* buffer, s32 length, s32 flags, const SOSockAddrIn* to) {
    s32 device;
    s32 result;
    BOOL direct;
    SOSendArgs* args;
    SOSendBlock* block;
    u8* data;

    result = SOiPrepare(function, &device);
    if (result == 0) {
        if (to != NULL && (to->len > 8 || to->len < 8)) {
            result = -28;
        } else if (length < 0 || (length > 0 && buffer == NULL)) {
            result = -28;
        } else {
            direct = TRUE;
            if (length != 0 && !SOiIsDirectBuffer(buffer, length)) {
                direct = FALSE;
            }
            block = (SOSendBlock*)SOiAlloc(12, 0x60);
            data = !direct ? (u8*)SOiAlloc(14, (length + 0x1F) & ~0x1F) : (u8*)buffer;
            if (block == NULL || data == NULL) {
                result = -49;
            } else {
                args = &block->args;
                args->fd = fd;
                args->flags = flags;
                if (to == NULL) {
                    args->hasAddress = 0;
                } else {
                    args->hasAddress = 1;
                    NETMemCpy(args->address, to, to->len);
                }
                if (!direct) {
                    NETMemCpy(data, buffer, length);
                }
                block->vectors[0].base = data;
                block->vectors[0].length = length;
                block->vectors[1].base = args;
                block->vectors[1].length = sizeof(SOSendArgs);
                result = IOS_Ioctlv(device, 0xD, 2, 0, block->vectors);
            }
            if (!direct) {
                SOiFree(14, data, (length + 0x1F) & ~0x1F);
            }
            SOiFree(12, block, 0x60);
        }
        result = SOiConclude(function, result);
    }
    return result;
}

/* 0x80520B54 (0x78): the console's own address (0 when the interface is down). */
u32 SOGetHostID(void) {
    s32 device;
    s32 error;
    u32 result = 0;

    error = SOiPrepare(NULL, &device);
    if (error == 0) {
        result = IOS_Ioctl(device, 0x10, NULL, 0, NULL, 0);
        SOiConclude(NULL, error);
    }
    return result;
}

/* 0x80520BCC (0x140): resolves a host name into the library's resolver buffer (the IOS answer's offsets are
 * rebased onto the buffer). */
SOHostEnt* SOGetHostByName(const char* name) {
    s32 device;
    s32 length;
    s32 result;
    SOHostBuffer* buffer;
    char* text;
    s32 size;
    SOHostEnt* entry = NULL;
    SOHostEnt* answer;
    u32* host;
    s32 shift;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (name == NULL) {
            result = -28;
        } else {
            length = strlen(name);
            size = (length + 0x20) & ~0x1F;
            text = (char*)SOiAlloc(12, size);
            buffer = (SOHostBuffer*)SOiGetSysWork()->hostBuffer;
            if (text == NULL) {
                result = -49;
            } else {
                strcpy(text, name);
                result = IOS_Ioctl(device, 0x11, text, length + 1, buffer, 0x460);
                if (result >= 0) {
                    answer = &buffer->entry;
                    shift = (s32)buffer->name - (s32)answer->name;
                    host = buffer->hosts;
                    while (*host != 0) {
                        *host++ += shift;
                    }
                    entry = answer;
                    answer->aliases = (char**)((u8*)answer->aliases + shift);
                    answer->name += shift;
                    answer->hosts = (SOInAddr**)((u8*)answer->hosts + shift);
                }
                SOiFree(12, text, size);
            }
        }
        SOiConclude(NULL, result);
    }
    return entry;
}

/* 0x80520D0C (0x2E4): resolves `node` / `service` (getaddrinfo); the records live in one 0x840-byte block. */
s32 SOGetAddrInfo(const char* node, const char* service, const SOAddrInfo* hints, SOAddrInfo** result) {
    s32 device;
    s32 error;
    s32 size;
    SOAddrInfo* info;
    SOAddrInfoRequest* block;
    char* nodeText;
    char* serviceText;
    SOAddrInfo* hintCopy;
    SOSockAddrStorage* address;

    error = SOiPrepare(NULL, &device);
    if (error == 0) {
        size = ((((node == NULL ? 0 : strlen(node) + 1) + 0x1F) & ~0x1F) +
                (((service == NULL ? 0 : strlen(node) + 1) + 0x1F) & ~0x1F) + 0x5F) & ~0x1F;
        block = (SOAddrInfoRequest*)SOiAlloc(12, size);
        if (block == NULL) {
            error = -49;
        } else {
            info = (SOAddrInfo*)SOiAlloc(10, 0x840);
            if (info == NULL) {
                SOiFree(12, block, size);
                error = -49;
            } else {
                nodeText = block->text;
                serviceText = nodeText + (((node == NULL ? 0 : strlen(node) + 1) + 0x1F) & ~0x1F);
                hintCopy = (SOAddrInfo*)(serviceText + (((service == NULL ? 0 : strlen(node) + 1) + 0x1F) & ~0x1F));
                if (node != NULL) {
                    strcpy(nodeText, node);
                }
                nodeText = node != NULL ? nodeText : NULL;
                block->vectors[0].base = nodeText;
                block->vectors[0].length = node == NULL ? 0 : strlen(node);
                if (service != NULL) {
                    strcpy(serviceText, service);
                }
                serviceText = service != NULL ? serviceText : NULL;
                block->vectors[1].base = serviceText;
                block->vectors[1].length = service == NULL ? 0 : strlen(service);
                if (hints != NULL) {
                    NETMemCpy(hintCopy, hints, sizeof(SOAddrInfo));
                } else {
                    NETMemSet(hintCopy, 0, sizeof(SOAddrInfo));
                }
                if (hintCopy->family == 0) {
                    hintCopy->family = 2;
                }
                if (hintCopy->family == 23) {
                    *result = NULL;
                    error = -68;
                    SOiFree(10, info, 0x840);
                } else {
                    block->vectors[2].base = hintCopy;
                    block->vectors[2].length = sizeof(SOAddrInfo);
                    block->vectors[3].base = info;
                    block->vectors[3].length = 0x834;
                    error = IOS_Ioctlv(device, 0x18, 3, 1, block->vectors);
                    if (error >= 0) {
                        *result = info;
                        for (address = ((SOAddrInfoAnswer*)info)->addresses; info != NULL; address++) {
                            info->addr = (SOSockAddrIn*)address;
                            if (info->next != NULL) {
                                info->next = info + 1;
                            }
                            info = info->next;
                        }
                    } else {
                        *result = NULL;
                        SOiFree(10, info, 0x840);
                    }
                }
                SOiFree(12, block, size);
            }
        }
        error = SOiConclude(NULL, error);
    }
    return error;
}

/* 0x80520FF0 (0x64): releases a `SOGetAddrInfo` result. */
void SOFreeAddrInfo(SOAddrInfo* info) {
    BOOL enabled = OSDisableInterrupts();
    if (SOiIsInitialized() == 1 && info != NULL) {
        SOiFree(10, info, 0x840);
    }
    OSRestoreInterrupts(enabled);
}

/* 0x80521054 (0x104): sets one socket option (at most 20 bytes of value). */
s32 SOSetSockOpt(s32 fd, s32 level, s32 option, u32* value, s32 length) {
    s32 device;
    s32 result;
    s32* block;

    result = SOiPrepare(NULL, &device);
    if (result == 0) {
        if (length < 0 || length > 20) {
            result = -28;
        } else {
            block = (s32*)SOiAlloc(12, 0x40);
            if (block == NULL) {
                result = -49;
            } else {
                block[0] = fd;
                block[1] = level;
                block[2] = option;
                block[3] = length;
                if (value != NULL) {
                    NETMemCpy(&block[4], value, length);
                } else {
                    NETMemSet(&block[4], 0, length);
                }
                result = IOS_Ioctl(device, 9, block, 0x24, NULL, 0);
                SOiFree(12, block, 0x40);
            }
        }
        result = SOiConclude(NULL, result);
    }
    return result;
}

/* 0x80521158 (0x1DC): reads one interface option into `value` (`*length` in: its capacity, out: its size). */
/* untyped: byte range */
s32 SOGetInterfaceOpt(struct SOInterface* iface, s32 level, s32 option, void* value, s32* length) {
    s32 device;
    s32 opened;
    s32 size;
    s32 result;
    SOInterfaceOptBlock* block;
    s32* request;
    s32* answerLength;
    u8* answer;

    result = SOiPrepareTempRm(NULL, &device, &opened);
    if (result == 0) {
        if ((u32)(option - 0x1001) <= 1) {
            result = -28;
        } else {
            size = ((length == NULL || *length < 0 ? 0 : *length) + 0x7F) & ~0x1F;
            block = (SOInterfaceOptBlock*)SOiAlloc(12, size);
            if (block == NULL) {
                result = -49;
            } else {
                request = block->request;
                request[0] = level;
                answerLength = &request[8];
                request[1] = option;
                answer = (u8*)&answerLength[8];
                *answerLength = length == NULL || *length < 0 ? 0 : *length;
                block->vectors[0].base = request;
                block->vectors[0].length = 8;
                block->vectors[1].base = answer;
                block->vectors[1].length = length == NULL || *length < 0 ? 0 : *length;
                block->vectors[2].base = answerLength;
                block->vectors[2].length = 4;
                result = IOS_Ioctlv(device, 0x1C, 1, 2, block->vectors);
                if (result >= 0 && length != NULL) {
                    if (*length >= *answerLength) {
                        if (value != NULL) {
                            NETMemCpy(value, answer, *answerLength);
                        }
                        *length = *answerLength;
                    } else {
                        *length = *answerLength;
                        result = -28;
                    }
                }
                SOiFree(12, block, size);
            }
        }
        result = SOiConcludeTempRm(NULL, result, opened);
    }
    return result;
}


}
