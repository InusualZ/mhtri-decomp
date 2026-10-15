/*
 * NCD/ncdsystem.c - the NCD library (`NCDGetCurrentIfConfig`, `NCDGetCurrentIpConfig` and their `/dev/net/ncd/manage`
 *   ioctl helpers).
 * RANGE. .text 0x8051C554-0x8051CDD0 (7 functions); .data 0x80630FE0-0x806310D0, .bss 0x80766920-0x80766980, .sdata
 *   0x80794428-0x80794430, .sbss 0x80795890-0x80795898.  No extab, .rodata or .sdata2.
 * RANGE. Left edge 0x8051C554: cut from `SSL/ssl.cpp` (its header holds the evidence).  Right edge: `NET/netmisc.c`
 *   at 0x8051CDD0 (the NET `.data`/`.sdata` start 8-aligned after this object's; that header holds the evidence).
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24` (docs/network.md "SDK library compilers"); the file sets
 *   `#pragma auto_inline off` (retail calls the helpers it defines earlier).
 * NAMES. `ncdsystem.c` is the `__FILE__` string the range's own panic passes (0x8051CD40, "Could not reserve heap
 *   for NCD library from IPC arena").
 *   `NCDiGetEnabledConfigList` is the name its own request passes; `NCDGetLinkStatus`, `NCDiGetWirelessMacAddress`,
 *   `NCDiConfigRequest`, `NCDiInitAndLock` and the data names (`ncdMutex`, `ncdReply`, `ncdVectors`, `ncdInitFlags`,
 *   `ncdConfigBuffer`, `ncdVersion`) are GUESSes.
 *   GUESS (map name; the runtime dump has only a placeholder or another name at the address):
 *   GUESS: `NCDGetCurrentIfConfig`, `NCDGetLinkStatus`, `NCDiGetWirelessMacAddress`, `NCDiGetEnabledConfigList`
 *   GUESS: `NCDiConfigRequest`, `NCDiInitAndLock`
 *   GUESS: `NCDiUnlock` (a dead-stripped helper the `.bss` order implies; its name and body are guesses)
 * SHAPES. The mutex, the reply words and the vectors are three file statics MWCC addresses from one base register.
 * SHAPES. `NULL` is the SDK's `((void*)0)`: `NCDGetCurrentIfConfig`/`NCDGetCurrentIpConfig` compare their pointer
 *   against it with `li r31,0; cmplw` (the register `result` starts in), the integer 0 gives `cmpwi`; the
 *   `OSGetCurrentThread()` tests stay integer compares (`cmpwi r3,0`).
 * SHAPES. `.bss` is laid out in first-use order: the never-called static `NCDiUnlock` ahead of the API is the first
 *   user of `ncdMutex`, which puts it at +0x00 ahead of `ncdReply`/`ncdVectors` as in retail (without it every
 *   base-relative offset in `NCDiGetWirelessMacAddress`, `NCDiConfigRequest`, `NCDiInitAndLock` and
 *   `NCDGetLinkStatus` swaps 0x00/0x20); mwld strips the unreferenced helper again.
 * RESIDUALS. `NCDiGetEnabledConfigList` re-reads the profile's mode byte
 *   (`lbz r0,0x7ca(r4)` twice per profile) and keeps the masks in r25-r27 (ours CSEs the byte, masks in r29-r31;
 *   an inline predicate, a declaration reorder and a volatile view were measured, none moves it).
 *   `NCDGetLinkStatus` branches `blt`+`b` around its `result = -1` where ours emits one `bge` (five spellings of the
 *   negative-status test measured identical).
 */

#include "NCD/ncdsystem.h"
#include "NAND/nand.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcclt.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/OS.h"

#pragma auto_inline off

/* The SDK's C headers spell NULL as a void pointer: a pointer compared against it is a register compare
   (`li rX,0` + `cmplw`), where the integer 0 gives `cmplwi`. */
#undef NULL
#define NULL ((void*)0)

/* One network profile of the NCD configuration. */
typedef struct NCDProfile {
    /* +0x000 */ u8 flags;
    /* +0x001 */ u8 pad_0x001[0x03];
    /* +0x004 */ u8 ip[0x14];
    /* +0x018 */ u8 dns[0x0C];
    /* +0x024 */ u8 proxy[0x79C];
    /* +0x7C0 */ u8 wireless[0x15C];
} NCDProfile; /* size: 0x91C */

/* The NCD configuration `/dev/net/ncd/manage` reads and writes. */
typedef struct NCDConfig {
    /* +0x00 */ u8 pad_0x00[0x06];
    /* +0x06 */ u8 linkType;
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ NCDProfile profiles[3];
} NCDConfig; /* size: 0x1B5C */

/* The interface configuration `NCDGetCurrentIfConfig` fills. */
typedef struct NCDIfConfig {
    /* +0x000 */ u8 type;
    /* +0x001 */ u8 linkType;
    /* +0x002 */ u8 wireless[0x15C];
} NCDIfConfig; /* size: 0x15E */

/* The IP configuration `NCDGetCurrentIpConfig` fills. */
typedef struct NCDIpConfig {
    /* +0x000 */ s32 useDhcp;
    /* +0x004 */ s32 useProxy;
    /* +0x008 */ u8 ip[0x14];
    /* +0x01C */ u8 dns[0x0C];
    /* +0x028 */ u8 proxy[0x79C];
} NCDIpConfig; /* size: 0x7C4 */

char* ncdVersion = "<< RVL_SDK - NCD \trelease build: Jun  9 2009 11:59:48 (0x4199_60831) >>";

NCDConfig* ncdConfigBuffer;
u32 ncdInitFlags;

static OSMutex ncdMutex;
static s32 ncdReply[8] __attribute__((aligned(32)));
static IPCIOVector ncdVectors[4] __attribute__((aligned(32)));

/* Releases the library lock.  Nothing in the image calls it, so the linker strips it; it is compiled
   before every other user of the lock, which is what puts `ncdMutex` first in `.bss`. */
static void NCDiUnlock(void)
{
    OSUnlockMutex(&ncdMutex);
}

s32 NCDiConfigRequest(const char* function, NCDConfig* config, s32 command);
void NCDiInitAndLock(void);

/* 0x8051C554 (0xF8): fills `config` with the selected profile's interface settings. */
s32 NCDGetCurrentIfConfig(u8* config) {
    NCDIfConfig* out = (NCDIfConfig*)config;
    s32 result = 0;
    NCDConfig* buffer;
    s32 index;
    NCDProfile* profile;

    if (out == NULL) {
        return -3;
    }
    NCDiInitAndLock();
    result = NCDiConfigRequest("NCDGetCurrentIfConfig", NULL, 3);
    if (result == 0) {
        buffer = ncdConfigBuffer;
        index = ncdReply[1];
        if (index < 0 || index >= 3) {
            result = -7;
        } else {
            profile = &buffer->profiles[index];
            if (profile->flags & 1) {
                out->type = 2;
                memcpy(out->wireless, profile->wireless, 4);
            } else {
                out->type = 1;
                memcpy(out->wireless, profile->wireless, sizeof(profile->wireless));
            }
            out->linkType = buffer->linkType;
        }
    }
    OSUnlockMutex(&ncdMutex);
    return result;
}

/* 0x8051C64C (0x158): fills `config` with the selected profile's IP settings. */
/* untyped: caller-owned payload */
s32 NCDGetCurrentIpConfig(void* config) {
    NCDIpConfig* out = (NCDIpConfig*)config;
    s32 result = 0;
    NCDConfig* buffer;
    s32 index;

    if (out == NULL) {
        return -3;
    }
    NCDiInitAndLock();
    result = NCDiConfigRequest("NCDGetCurrentIpConfig", NULL, 3);
    if (result == 0) {
        buffer = ncdConfigBuffer;
        index = ncdReply[1];
        if (index < 0 || index >= 3) {
            result = -7;
        } else {
            memcpy(out->dns, buffer->profiles[index].dns, sizeof(out->dns));
            if (buffer->profiles[index].flags & 6) {
                out->useDhcp = 1;
                memcpy(out->ip, buffer->profiles[index].ip, sizeof(out->ip));
            } else {
                out->useDhcp = 0;
                memcpy(out->ip, buffer->profiles[index].ip, sizeof(out->ip));
            }
            if (buffer->profiles[index].flags & 0x10) {
                out->useProxy = 1;
                memcpy(out->proxy, buffer->profiles[index].proxy, sizeof(out->proxy));
            } else {
                out->useProxy = 0;
                memset(out->proxy, 0, sizeof(out->proxy));
            }
        }
    }
    OSUnlockMutex(&ncdMutex);
    return result;
}

/* 0x8051C7A4 (0x104): the link state `/dev/net/ncd/manage` reports. */
s32 NCDGetLinkStatus(void) {
    s32 result;
    s32 fd;

    if (OSGetCurrentThread() == 0) {
        return -5;
    }
    NCDiInitAndLock();
    fd = IOS_Open("/dev/net/ncd/manage", 0);
    if (fd < 0) {
        if (fd == -6) {
            result = -8;
        } else {
            result = -2;
        }
    } else {
        ncdVectors[0].base = ncdReply;
        ncdVectors[0].length = 32;
        if (IOS_Ioctlv(fd, 7, 0, 1, ncdVectors) < 0) {
            result = -2;
        } else {
            result = ncdReply[0];
            if (result == 0) {
                result = ncdReply[1];
                if (result < 0) {
                    result = -1;
                }
            }
        }
        if (IOS_Close(fd) < 0) {
            result = -1;
        }
    }
    OSUnlockMutex(&ncdMutex);
    return result;
}

/* 0x8051C8A8 (0x128): copies the wireless MAC address (6 bytes) into `address`. */
s32 NCDiGetWirelessMacAddress(u8* address) {
    s32 result = 0;
    s32 fd;

    if (address == NULL) {
        return -3;
    }
    if (OSGetCurrentThread() == 0) {
        return -5;
    }
    NCDiInitAndLock();
    fd = IOS_Open("/dev/net/ncd/manage", 0);
    if (fd < 0) {
        if (fd == -6) {
            result = -8;
        } else {
            result = -2;
        }
    } else {
        ncdVectors[0].base = ncdReply;
        ncdVectors[0].length = 32;
        ncdVectors[1].base = ncdConfigBuffer;
        ncdVectors[1].length = 6;
        if (IOS_Ioctlv(fd, 8, 0, 2, ncdVectors) < 0) {
            result = -2;
        } else {
            result = ncdReply[0];
            if (result == 0) {
                memcpy(address, ncdConfigBuffer, 6);
            }
        }
        if (IOS_Close(fd) < 0) {
            result = -1;
        }
    }
    OSUnlockMutex(&ncdMutex);
    return result;
}

/* 0x8051C9D0 (0x17C): the bit masks of the enabled wired, wireless and other-wireless profiles. */
s32 NCDiGetEnabledConfigList(u32* wired, u32* wireless, u32* wirelessOther) {
    u32 wiredMask = 0;
    u32 wirelessMask = 0;
    u32 otherMask = 0;
    s32 result;
    NCDConfig* buffer;
    s32 i;

    NCDiInitAndLock();
    result = NCDiConfigRequest("NCDiGetEnabledConfigList", NULL, 3);
    if (result == 0) {
        buffer = ncdConfigBuffer;
        for (i = 0; i < 3; i++) {
            if (buffer->profiles[i].flags & 0x80) {
                if (buffer->profiles[i].flags & 1) {
                    wiredMask |= 1 << i;
                } else {
                    if (buffer->profiles[i].wireless[2] != 1) {
                        wirelessMask |= 1 << i;
                    }
                    if (buffer->profiles[i].wireless[2] == 1) {
                        otherMask |= 1 << i;
                    }
                }
            }
        }
    }
    OSUnlockMutex(&ncdMutex);
    if (wired != NULL) {
        *wired = wiredMask;
    }
    if (wireless != NULL) {
        *wireless = wirelessMask;
    }
    if (wirelessOther != NULL) {
        *wirelessOther = otherMask;
    }
    return result;
}

/* 0x8051CB4C (0x194): reads (3/5) or writes (4/6) the NCD configuration through the shared IPC buffer. */
s32 NCDiConfigRequest(const char* function, NCDConfig* config, s32 command) {
    s32 result = 0;
    s32 fd;

    if (OSGetCurrentThread() == 0) {
        return -5;
    }
    NCDiInitAndLock();
    fd = IOS_Open("/dev/net/ncd/manage", 0);
    if (fd < 0) {
        if (fd == -6) {
            result = -8;
        } else {
            result = -2;
        }
    } else {
        ncdVectors[0].base = ncdConfigBuffer;
        ncdVectors[0].length = sizeof(NCDConfig);
        ncdVectors[1].base = ncdReply;
        ncdVectors[1].length = 32;
        switch (command) {
        case 3:
        case 5:
            if (IOS_Ioctlv(fd, command, 0, 2, ncdVectors) < 0) {
                result = -2;
            } else {
                result = ncdReply[0];
                if (result == 0 && config != NULL) {
                    memcpy(config, ncdConfigBuffer, sizeof(NCDConfig));
                }
            }
            break;
        case 4:
        case 6:
            if (config != NULL) {
                memcpy(ncdConfigBuffer, config, sizeof(NCDConfig));
            }
            if (IOS_Ioctlv(fd, command, 1, 1, ncdVectors) < 0) {
                result = -2;
            } else {
                result = ncdReply[0];
            }
            break;
        }
        if (IOS_Close(fd) < 0) {
            result = -1;
        }
    }
    OSUnlockMutex(&ncdMutex);
    return result;
}

/* 0x8051CCE0 (0xF0): sets the library up once (version, lock, the IPC-arena configuration buffer) and takes the
 * lock. */
void NCDiInitAndLock(void) {
    BOOL enabled = OSDisableInterrupts();
    u8* lo;

    if (!(ncdInitFlags & 1)) {
        OSRegisterVersion(ncdVersion);
        OSInitMutex(&ncdMutex);
        lo = (u8*)(((u32)IPCGetBufferLo() + 31) & ~31);
        if ((u32)((u8*)IPCGetBufferHi() - lo) < 0x1B60) {
            OSPanic("ncdsystem.c", 1500, "Could not reserve heap for NCD library from IPC arena");
        }
        IPCSetBufferLo(lo + 0x1B60);
        ncdConfigBuffer = (NCDConfig*)lo;
        memset(lo, 0, 0x1B60);
        memset(ncdReply, 0, sizeof(ncdReply));
        memset(ncdVectors, 0, sizeof(ncdVectors));
        ncdInitFlags |= 1;
    }
    OSRestoreInterrupts(enabled);
    OSLockMutex(&ncdMutex);
}
