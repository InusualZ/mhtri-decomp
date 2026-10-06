/*
 * SSL/ssl.cpp - the SSL library (`SSLNew`, `SSLConnect`, `SSLDoHandshake`, `SSLRead`, `SSLWrite`, the certificate
 *   setters): the RVL_SDK "SSL" build, driving `/dev/net/ssl`.
 * RANGE. .text 0x8051B7FC-0x8051C554 (13 functions); .data 0x80630F88-0x80630FE0, .bss 0x80763900-0x80766920, .sdata
 *   0x80794420-0x80794428, .sbss 0x80795888-0x80795890.  No extab, .rodata or .sdata2.
 * RANGE. Left edge: `NHTTP/d_nhttp.c` ends at 0x8051B7FC.  Right edge 0x8051C554: `NCD/ncdsystem.c` starts with
 *   `NCDGetCurrentIfConfig`.  No `.text` reference crosses the edge (the three 4-byte mutex wrappers 0x8051C548-0x8051C550
 *   are called only by `SSLSetClientCert`/`SSLSetRootCA`); the `.data` run is the "SSL" version string and
 *   `/dev/net/ssl`, followed by the "NCD" version string at 0x80630FE0; every `.bss`/`.sdata`/`.sbss` label below its
 *   edge is read only from this range and every one above only from the next.
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24` (docs/network.md "SDK library compilers").
 * NAMES. The map's SDK names; `SSLiInitMutex` / `SSLiLockMutex` / `SSLiUnlockMutex` (the three wrappers) and the data
 *   names `sslVersion`, `sslVersionRegistered`, `sslBufferInitialized`, `sslMutex`, `sslClientCert`,
 *   `sslClientKey`, `sslRootCA` are GUESSes.
 *   GUESS (map name; the runtime dump has only a placeholder or another name at the address): `SSLiInitMutex`
 *   GUESS: `SSLiLockMutex`, `SSLiUnlockMutex`
 * SHAPES. Every request opens `/dev/net/ssl`, passes 32-byte aligned locals (`result`, the context id, the payload)
 *   through `IOS_Ioctlv` and closes the device again; the order of the two stores before the ioctl is load-bearing.
 *   `SSLRead`/`SSLWrite` split the transfer into an unaligned head (through a 32-byte bounce buffer), the 32-byte
 *   multiple and the tail, each through the `SSLiRecv`/`SSLiSend` inline. The certificate buffers are four
 *   file statics in one `.bss` run (retail addresses them from one base register, `addi rX,base,0` for the
 *   mutex), and the one-time buffer set-up is written out in both setters.
 * RESIDUALS. `SSLRead`/`SSLWrite`: the inlined chunk locals sit below their vectors (retail: above) and the
 *   parameters take other saved registers. `.data`/`.sdata` end 3 / 4 bytes short of the claims (the NCD
 *   object's 8-byte alignment pad).
 *   `SSLSetClientCert`/`SSLSetRootCA`: the base `lis`/`addi` of the statics relocates against our local section
 *   symbol where retail names `sslMutex`. Flip blocker (row 36): retail `.comment` marks `sslClientCert`, `sslClientKey`
 *   and `sslRootCA` force-active; ours does not, so a flipped object would let the linker deadstrip them.
 */
#include "SSL/ssl.h"
#include "NAND/nand.h"
#include "RVLGX/GXTexture_tail.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/OS.h"

extern "C" {

void SSLiInitMutex(OSMutex* mutex);
void SSLiLockMutex(OSMutex* mutex);
void SSLiUnlockMutex(OSMutex* mutex);

char* sslVersion = "<< RVL_SDK - SSL \trelease build: May 12 2009 09:12:41 (0x4199_60831) >>";

BOOL sslVersionRegistered;
BOOL sslBufferInitialized;

/* The certificate staging buffers and their lock; MWCC addresses the four from one base register. */
static OSMutex sslMutex;
static u8 sslClientCert[0x1000] __attribute__((aligned(32)));
static u8 sslClientKey[0x1000] __attribute__((aligned(32)));
static u8 sslRootCA[0x1000] __attribute__((aligned(32)));

/* 0x8051B7FC (0x158): creates an SSL context for `host` (at most 256 name bytes); the context id, or negative. */
s32 SSLNew(u32 verifyOption, char* host) {
    char name[0x100] __attribute__((aligned(32)));
    IPCIOVector vectors[3] __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    u32 verify __attribute__((aligned(32)));
    s32 fd;
    const char* p;
    u32 count;
    u32 length;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (!sslVersionRegistered) {
        OSRegisterVersion(sslVersion);
        sslVersionRegistered = TRUE;
    }
    if (fd < 0) {
        return -1;
    }
    for (p = host, count = 0; count < 0x100 && *p != '\0'; count++, p++) {
    }
    length = p - host;
    if (length == 0) {
        return -1;
    }
    memset(name, 0, sizeof(name));
    if (length > 0x100) {
        length = 0x100;
    }
    memcpy(name, host, length);
    result = -1;
    verify = verifyOption;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &verify;
    vectors[1].length = 32;
    vectors[2].base = name;
    vectors[2].length = 0x100;
    IOS_Ioctlv(fd, 1, 1, 2, vectors);
    IOS_Close(fd);
    return *(s32*)vectors[0].base;
}

/* 0x8051B954 (0xC8): attaches the context to the connected socket `socket`. */
s32 SSLConnect(s32 ssl, s32 socket) {
    IPCIOVector vectors[3] __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    s32 sock __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    id = ssl;
    sock = socket;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    vectors[2].base = &sock;
    vectors[2].length = 32;
    IOS_Ioctlv(fd, 2, 1, 2, vectors);
    IOS_Close(fd);
    return result;
}

/* 0x8051BA1C (0xAC): runs the handshake. */
s32 SSLDoHandshake(s32 ssl) {
    IPCIOVector vectors[2] __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    id = ssl;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    IOS_Ioctlv(fd, 3, 1, 1, vectors);
    IOS_Close(fd);
    return result;
}

/* One `/dev/net/ssl` receive of `size` bytes into `data`; the byte count, or negative. */
static inline s32 SSLiRecv(s32 fd, s32 ssl, u8* data, u32 size) {
    s32 result __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    IPCIOVector vectors[3] __attribute__((aligned(32)));

    id = ssl;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = data;
    vectors[1].length = size;
    vectors[2].base = &id;
    vectors[2].length = 32;
    IOS_Ioctlv(fd, 4, 2, 1, vectors);
    return result;
}

/* One `/dev/net/ssl` send of `size` bytes from `data`; the byte count, or negative. */
static inline s32 SSLiSend(s32 fd, s32 ssl, u8* data, u32 size) {
    s32 result __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    IPCIOVector vectors[3] __attribute__((aligned(32)));

    id = ssl;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    vectors[2].base = data;
    vectors[2].length = size;
    IOS_Ioctlv(fd, 5, 1, 2, vectors);
    return result;
}

/* 0x8051BAC8 (0x2D0): reads up to 32 KB into `buf`; the byte count, or negative. */
/* untyped: byte range */
s32 SSLRead(s32 ssl, void* buf, s32 length) {
    u8 bounce[32] __attribute__((aligned(32)));
    u8* dst = (u8*)buf;
    u32 remaining = length;
    s32 fd;
    s32 total;
    s32 ret;
    u32 size;

    fd = IOS_Open("/dev/net/ssl", 0);
    ret = -1;
    if (fd < 0) {
        return -1;
    }
    if (remaining > 0x8000) {
        remaining = 0x8000;
    }
    size = ((u32)dst & 0x1F) ? 32 - ((u32)dst & 0x1F) : 0;
    total = 0;
    memset(bounce, 0, sizeof(bounce));
    if (size != 0) {
        if (size > remaining) {
            size = remaining;
        }
        ret = SSLiRecv(fd, ssl, bounce, size);
        if (ret > 0) {
            total = ret;
            memcpy(dst, bounce, ret);
            if ((u32)ret < size) {
                IOS_Close(fd);
                return total;
            }
            dst += ret;
            remaining -= ret;
        } else {
            IOS_Close(fd);
            return ret;
        }
    }
    if (remaining != 0 && (size = remaining & ~0x1F) != 0) {
        ret = SSLiRecv(fd, ssl, dst, size);
        if (ret > 0) {
            total += ret;
            if ((u32)ret < size) {
                IOS_Close(fd);
                return total;
            }
            dst += ret;
            remaining -= ret;
        } else {
            IOS_Close(fd);
            if (total > 0) {
                return total;
            }
            return ret;
        }
    }
    if (remaining != 0 && (size = remaining & 0x1F) != 0) {
        memset(bounce, 0, sizeof(bounce));
        ret = SSLiRecv(fd, ssl, bounce, size);
        if (ret > 0) {
            memcpy(dst, bounce, ret);
            total += ret;
        } else {
            IOS_Close(fd);
            if (total > 0) {
                return total;
            }
            return ret;
        }
    }
    if (total > 0) {
        ret = total;
    }
    IOS_Close(fd);
    return ret;
}

/* 0x8051BD98 (0x2C0): writes `buf`; the byte count, or negative. */
/* untyped: byte range */
s32 SSLWrite(s32 ssl, void* buf, s32 length) {
    u8 bounce[32] __attribute__((aligned(32)));
    u8* src = (u8*)buf;
    u32 remaining = length;
    s32 fd;
    s32 total;
    s32 ret;
    u32 size;

    fd = IOS_Open("/dev/net/ssl", 0);
    ret = -1;
    if (fd < 0) {
        return -1;
    }
    size = ((u32)src & 0x1F) ? 32 - ((u32)src & 0x1F) : 0;
    total = 0;
    memset(bounce, 0, sizeof(bounce));
    if (size != 0) {
        if (size > remaining) {
            size = remaining;
        }
        memcpy(bounce, src, size);
        ret = SSLiSend(fd, ssl, bounce, size);
        if (ret > 0) {
            total = ret;
            if ((u32)ret < size) {
                IOS_Close(fd);
                return ret;
            }
            src += ret;
            remaining -= ret;
        } else {
            IOS_Close(fd);
            return ret;
        }
    }
    if (remaining != 0 && (size = remaining & ~0x1F) != 0) {
        ret = SSLiSend(fd, ssl, src, size);
        if (ret > 0) {
            total += ret;
            if ((u32)ret < size) {
                IOS_Close(fd);
                return total;
            }
            src += ret;
            remaining -= ret;
        } else {
            IOS_Close(fd);
            if (total > 0) {
                return total;
            }
            return ret;
        }
    }
    if (remaining != 0 && (size = remaining & 0x1F) != 0) {
        memset(bounce, 0, sizeof(bounce));
        memcpy(bounce, src, size);
        ret = SSLiSend(fd, ssl, bounce, size);
        if (ret > 0) {
            total += ret;
        } else {
            IOS_Close(fd);
            if (total > 0) {
                return total;
            }
            return ret;
        }
    }
    if (total > 0) {
        ret = total;
    }
    IOS_Close(fd);
    return ret;
}

/* 0x8051C058 (0xAC): closes the context. */
s32 SSLShutdown(s32 ssl) {
    IPCIOVector vectors[2] __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    id = ssl;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    IOS_Ioctlv(fd, 6, 1, 1, vectors);
    IOS_Close(fd);
    return result;
}

/* 0x8051C104 (0x16C): hands the client certificate and its key to the context. */
s32 SSLSetClientCert(s32 ssl, u32 cert, u32 certLength, u32 key, u32 keyLength) {
    IPCIOVector vectors[4] __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    BOOL enabled;
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    enabled = OSDisableInterrupts();
    if (!sslBufferInitialized) {
        SSLiInitMutex(&sslMutex);
        memset(sslClientCert, 0, sizeof(sslClientCert));
        memset(sslClientKey, 0, sizeof(sslClientKey));
        memset(sslRootCA, 0, sizeof(sslRootCA));
        sslBufferInitialized = TRUE;
    }
    OSRestoreInterrupts(enabled);
    SSLiLockMutex(&sslMutex);
    memcpy(sslClientCert, (const u8*)cert, certLength);
    memcpy(sslClientKey, (const u8*)key, keyLength);
    result = -1;
    id = ssl;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    vectors[2].base = sslClientCert;
    vectors[2].length = certLength;
    vectors[3].base = sslClientKey;
    vectors[3].length = keyLength;
    IOS_Ioctlv(fd, 7, 1, 3, vectors);
    SSLiUnlockMutex(&sslMutex);
    IOS_Close(fd);
    return result;
}

/* 0x8051C270 (0x148): hands the root CA to the context. */
s32 SSLSetRootCA(s32 ssl, u32 ca, u32 caLength) {
    IPCIOVector vectors[3] __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    BOOL enabled;
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    enabled = OSDisableInterrupts();
    if (!sslBufferInitialized) {
        SSLiInitMutex(&sslMutex);
        memset(sslClientCert, 0, sizeof(sslClientCert));
        memset(sslClientKey, 0, sizeof(sslClientKey));
        memset(sslRootCA, 0, sizeof(sslRootCA));
        sslBufferInitialized = TRUE;
    }
    OSRestoreInterrupts(enabled);
    SSLiLockMutex(&sslMutex);
    memcpy(sslRootCA, (const u8*)ca, caLength);
    id = ssl;
    result = -1;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    vectors[2].base = sslRootCA;
    vectors[2].length = caLength;
    IOS_Ioctlv(fd, 10, 1, 2, vectors);
    SSLiUnlockMutex(&sslMutex);
    IOS_Close(fd);
    return result;
}

/* Hands the built-in certificate `builtin` to the context through `command`. */
static inline s32 SSLiSetBuiltin(s32 ssl, u32 builtin, s32 command) {
    IPCIOVector vectors[3] __attribute__((aligned(32)));
    u32 which __attribute__((aligned(32)));
    s32 id __attribute__((aligned(32)));
    s32 result __attribute__((aligned(32)));
    s32 fd;

    fd = IOS_Open("/dev/net/ssl", 0);
    if (fd < 0) {
        return -1;
    }
    id = ssl;
    result = -1;
    which = builtin;
    vectors[0].base = &result;
    vectors[0].length = 32;
    vectors[1].base = &id;
    vectors[1].length = 32;
    vectors[2].base = &which;
    vectors[2].length = 32;
    IOS_Ioctlv(fd, command, 1, 2, vectors);
    IOS_Close(fd);
    return result;
}

/* 0x8051C3B8 (0xC8): selects a built-in root CA. */
s32 SSLSetBuiltinRootCA(s32 ssl, u32 id) {
    return SSLiSetBuiltin(ssl, id, 13);
}

/* 0x8051C480 (0xC8): selects a built-in client certificate. */
s32 SSLSetBuiltinClientCert(s32 ssl, u32 id) {
    return SSLiSetBuiltin(ssl, id, 14);
}

/* 0x8051C548 (0x4): makes the staging buffers' lock. */
void SSLiInitMutex(OSMutex* mutex) {
    OSInitMutex(mutex);
}

/* 0x8051C54C (0x4): takes the staging buffers' lock. */
void SSLiLockMutex(OSMutex* mutex) {
    OSLockMutex(mutex);
}

/* 0x8051C550 (0x4): releases the staging buffers' lock. */
void SSLiUnlockMutex(OSMutex* mutex) {
    OSUnlockMutex(mutex);
}

}
