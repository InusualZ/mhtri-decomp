/*
 * NCD/ncdsystem.c - the NCD library (`NCDGetCurrentIfConfig`, `NCDGetCurrentIpConfig` and their `/dev/net/ncd/manage`
 *   ioctl helpers) and the NET helpers behind it (`NETGetStartupErrorCode`, `NETMemCpy`, `NETMemSet`).
 * RANGE. .text 0x8051C554-0x8051D710 (14 functions); .data 0x80630FE0-0x80631128, .bss 0x80766920-0x80766980, .sdata
 *   0x80794428-0x80794438, .sbss 0x80795890-0x80795898.  No extab, .rodata or .sdata2.
 * RANGE. Left edge 0x8051C554: cut from `SSL/ssl.cpp` (its header holds the evidence).  Right edge:
 *   `NWC24/nwc24_msg.c` at 0x8051D710.
 * RANGE. Two libraries, not split: `.data` is the "NCD" version string, the NCD names and `ncdsystem.c` (read by
 *   0x8051C554-0x8051CCE0), then "Unknown SOStartup Error: %d\n" (read by 0x8051D0C8) and the "REX-PPC" version
 *   string (through `.sdata` 0x80794430, read by 0x8051D240).  The `.text` edge lies in 0x8051CDD0-0x8051D048
 *   (0x8051CDD0 reads no data and is called only by `NETGetStartupErrorCode`), so the range stays one unit.
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24` (docs/network.md "SDK library compilers"); the file sets
 *   `#pragma auto_inline off` (retail calls the helpers it defines earlier).
 * NAMES. `ncdsystem.c` is the `__FILE__` string the range's own panic passes (0x8051CD40, "Could not reserve heap
 *   for NCD library from IPC arena"); the NET tail shares the file only because its edge is unproven.
 *   `NCDiGetEnabledConfigList` is the name its own request passes; `NCDGetLinkStatus`, `NCDiGetWirelessMacAddress`,
 *   `NCDiConfigRequest`, `NCDiInitAndLock`, `NETiConfigListToErrorBase`, `NETiStartupErrorToCode`, `NETGetRexVersion`
 *   and the data names (`ncdMutex`, `ncdReply`, `ncdVectors`, `ncdInitFlags`, `ncdConfigBuffer`, `ncdVersion`,
 *   `netRexVersion`) are GUESSes.
 *   GUESS (map name; the runtime dump has only a placeholder or another name at the address):
 *   GUESS: `NCDGetCurrentIfConfig`, `NCDGetLinkStatus`, `NCDiGetWirelessMacAddress`, `NCDiGetEnabledConfigList`
 *   GUESS: `NCDiConfigRequest`, `NCDiInitAndLock`, `NETiConfigListToErrorBase`, `NETGetStartupErrorCode`
 *   GUESS: `NETiStartupErrorToCode`, `NETGetRexVersion`, `NCDGetWirelessMacAddress`
 * SHAPES. The mutex, the reply words and the vectors are three file statics MWCC addresses from one base register.
 *   `NETMemCpy`/`NETMemSet` are `asm` functions: retail replicates the fill byte with in-place `rlwimi r4,r4`, runs
 *   counted `mtctr` loops whose shape no C spelling of ours reproduces (unrolled at -O4,p, `mtctr` hoisted above the
 *   zero test under `optimize_for_size`) and saves r31 in a leaf without a link-register frame.
 * RESIDUALS. The `.data` "Unknown SOStartup Error" string and the `.sdata` "REX-PPC" pointer start 8-aligned in the
 *   target (a second TU, the NET helpers, begins between 0x8051CDD0 and 0x8051D0C8), so `.data` ends 6 bytes and
 *   `.sdata` 8 bytes short until that seam is drawn. `.bss`: ours lays the statics out in first-use order (`ncdReply`,
 *   `ncdMutex`, `ncdVectors`), retail puts `ncdMutex` first, so every base-relative offset in `NCDiGetWirelessMacAddress`,
 *   `NCDiConfigRequest` and `NCDGetLinkStatus` swaps 0x00/0x20. Retail tests the NULL pointer parameters with
 *   `li rX,0; cmplw rY,rX` (ours `cmpwi rY,0`) in `NCDGetCurrentIfConfig`, `NCDGetCurrentIpConfig`,
 *   `NCDiGetWirelessMacAddress` and `NCDiConfigRequest`. `NCDiGetEnabledConfigList` re-reads the profile's mode byte
 *   and keeps the masks in r25-r27.
 */

#include "NCD/ncdsystem.h"
#include "NAND/nand.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcclt.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/OS.h"

#pragma auto_inline off

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

s32 NCDiConfigRequest(const char* function, NCDConfig* config, s32 command);
void NCDiInitAndLock(void);
s32 NCDiGetWirelessMacAddress(u8* address);
s32 NCDiGetEnabledConfigList(u32* wired, u32* wireless, u32* wirelessOther);
s32 NETiConfigListToErrorBase(u32 wired, u32 wireless, u32 wirelessOther);
s32 NETiStartupErrorToCode(s32 result, s32 base);

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

    if (OSGetCurrentThread() == NULL) {
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
    if (OSGetCurrentThread() == NULL) {
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

    if (OSGetCurrentThread() == NULL) {
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

/* The index of the lowest set bit of `mask`, or -1. */
static inline s32 NETiLowestBit(u32 mask) {
    s32 i;
    u32 bit;
    for (i = 0, bit = 1; i < 32; i++, bit <<= 1) {
        if (mask & bit) {
            return i;
        }
    }
    return -1;
}

/* 0x8051CDD0 (0x278): the error-code base for the enabled profile masks (99 when none or several kinds). */
s32 NETiConfigListToErrorBase(u32 wired, u32 wireless, u32 wirelessOther) {
    s32 base = 99;
    if (wired != 0) {
        if (wireless == 0 && wirelessOther == 0) {
            base = NETiLowestBit(wired) + 20;
        }
    } else if (wireless != 0) {
        if (wirelessOther == 0) {
            base = NETiLowestBit(wireless) + 30;
        }
    } else if (wirelessOther != 0) {
        base = NETiLowestBit(wirelessOther) + 40;
    }
    return base;
}

/* 0x8051D048 (0x80): the user-facing network error code for a failed `SOStartup` result. */
s32 NETGetStartupErrorCode(s32 result) {
    s32 base = 99;
    u32 wired;
    u32 wireless;
    u32 wirelessOther;

    if (NCDiGetEnabledConfigList(&wired, &wireless, &wirelessOther) >= 0) {
        base = NETiConfigListToErrorBase(wired, wireless, wirelessOther);
    }
    if (base < 0) {
        result = (s32)0x80000000;
        base = 99;
    }
    return NETiStartupErrorToCode(result, base) - base;
}

/* 0x8051D0C8 (0x178): maps an `SOStartup` result onto its error code (offset by the profile `base`). */
s32 NETiStartupErrorToCode(s32 result, s32 base) {
    if (result >= 0) {
        return 0;
    }
    switch (result) {
    case -45:
        return -50200;
    case -28:
        return -50300;
    case -62:
        return -50400;
    case -111:
        return -52700;
    case -121:
        if (base >= 20 && base < 30) {
            return -51400;
        }
        return -51000;
    case -112:
    case -76:
    case -48:
    case -39:
        if (base >= 20 && base < 30) {
            return -51400;
        }
        return -51300;
    case -102:
    case -101:
    case -100:
        return -52000;
    case (s32)0x80000000:
        return -50100;
    default:
        OSReport("Unknown SOStartup Error: %d\n", result);
        return -50100;
    }
}

char* netRexVersion = "<< REX-PPC 2.4.255.0 (RevoEX-2.4) REL 090609111526 >>";

/* 0x8051D240 (0x8): the RevoEX version string. */
char* NETGetRexVersion(void) {
    return netRexVersion;
}

/* 0x8051D248 (0x4): copies the wireless MAC address (6 bytes) into `address`. */
void NCDGetWirelessMacAddress(u8* address) {
    NCDiGetWirelessMacAddress(address);
}

/* 0x8051D24C (0x3C0): copies `size` bytes (overlap-safe), 32-byte blocks through `dcbz` when the run is long. */
/* untyped: byte range */
asm void* NETMemCpy(void* dst, const void* src, u32 size) {
    nofralloc
    stwu r1,-16(r1)
    cmplw r3,r4
    mr r0,r3
    stw r31,12(r1)
    bne cpy_1
    b cpy_27
cpy_1:
    ble cpy_5
    add r6,r4,r5
    cmplw r3,r6
    bge cpy_5
    clrlwi. r4,r5,30
    add r7,r3,r5
    srwi r5,r5,2
    beq cpy_3
    mtctr r4
cpy_2:
    lbzu r3,-1(r6)
    stbu r3,-1(r7)
    bdnz cpy_2
cpy_3:
    cmpwi r5,0
    beq cpy_26
    mtctr r5
cpy_4:
    lwzu r3,-4(r6)
    stwu r3,-4(r7)
    bdnz cpy_4
    b cpy_26
cpy_5:
    addi r6,r4,-32
    cmplw r3,r6
    ble cpy_6
    cmplw r3,r4
    blt cpy_22
cpy_6:
    cmplwi r5,64
    blt cpy_22
    clrlwi. r10,r3,27
    beq cpy_11
    subfic r10,r10,32
    mr r8,r4
    srwi. r6,r10,2
    mr r9,r3
    clrlwi r7,r10,30
    beq cpy_8
    mtctr r6
cpy_7:
    lwz r6,0(r8)
    addi r8,r8,4
    stw r6,0(r9)
    addi r9,r9,4
    bdnz cpy_7
cpy_8:
    cmpwi r7,0
    beq cpy_10
    mtctr r7
cpy_9:
    lbz r6,0(r8)
    addi r8,r8,1
    stb r6,0(r9)
    addi r9,r9,1
    bdnz cpy_9
cpy_10:
    add r3,r3,r10
    add r4,r4,r10
    subf r5,r10,r5
cpy_11:
    clrlwi r8,r4,30
    mr r7,r4
    cmpwi r8,2
    mr r6,r3
    clrrwi r31,r5,5
    srwi r9,r5,5
    beq cpy_17
    bge cpy_12
    cmpwi r8,0
    beq cpy_13
    bge cpy_15
    b cpy_21
cpy_12:
    cmpwi r8,4
    bge cpy_21
    b cpy_19
cpy_13:
    mtctr r9
cpy_14:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    stw r12,0(r6)
    stw r11,4(r6)
    lwz r12,8(r7)
    lwz r11,12(r7)
    stw r12,8(r6)
    stw r11,12(r6)
    lwz r12,16(r7)
    lwz r11,20(r7)
    stw r12,16(r6)
    stw r11,20(r6)
    lwz r12,24(r7)
    lwz r11,28(r7)
    addi r7,r7,32
    stw r12,24(r6)
    stw r11,28(r6)
    addi r6,r6,32
    bdnz cpy_14
    b cpy_21
cpy_15:
    lwz r10,-1(r4)
    mtctr r9
    addi r7,r4,3
    slwi r10,r10,8
cpy_16:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,8,24,31
    slwi r8,r12,8
    stw r10,0(r6)
    rlwimi r8,r11,8,24,31
    slwi r9,r11,8
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,8,24,31
    slwi r8,r12,8
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,8,24,31
    slwi r10,r11,8
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,8,24,31
    slwi r8,r12,8
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,8,24,31
    slwi r9,r11,8
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,8,24,31
    slwi r8,r12,8
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,8,24,31
    slwi r10,r11,8
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_16
    b cpy_21
cpy_17:
    lwz r10,-2(r4)
    mtctr r9
    addi r7,r4,2
    slwi r10,r10,16
cpy_18:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,16,16,31
    slwi r8,r12,16
    stw r10,0(r6)
    rlwimi r8,r11,16,16,31
    slwi r9,r11,16
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,16,16,31
    slwi r8,r12,16
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,16,16,31
    slwi r10,r11,16
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,16,16,31
    slwi r8,r12,16
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,16,16,31
    slwi r9,r11,16
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,16,16,31
    slwi r8,r12,16
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,16,16,31
    slwi r10,r11,16
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_18
    b cpy_21
cpy_19:
    lwz r10,-3(r4)
    mtctr r9
    addi r7,r4,1
    slwi r10,r10,24
cpy_20:
    dcbz 0,r6
    lwz r12,0(r7)
    lwz r11,4(r7)
    rlwimi r10,r12,24,8,31
    slwi r8,r12,24
    stw r10,0(r6)
    rlwimi r8,r11,24,8,31
    slwi r9,r11,24
    lwz r12,8(r7)
    stw r8,4(r6)
    rlwimi r9,r12,24,8,31
    slwi r8,r12,24
    lwz r11,12(r7)
    stw r9,8(r6)
    rlwimi r8,r11,24,8,31
    slwi r10,r11,24
    lwz r12,16(r7)
    stw r8,12(r6)
    rlwimi r10,r12,24,8,31
    slwi r8,r12,24
    lwz r11,20(r7)
    stw r10,16(r6)
    rlwimi r8,r11,24,8,31
    slwi r9,r11,24
    lwz r12,24(r7)
    stw r8,20(r6)
    rlwimi r9,r12,24,8,31
    slwi r8,r12,24
    lwz r11,28(r7)
    addi r7,r7,32
    stw r9,24(r6)
    rlwimi r8,r11,24,8,31
    slwi r10,r11,24
    stw r8,28(r6)
    addi r6,r6,32
    bdnz cpy_20
cpy_21:
    add r3,r3,r31
    add r4,r4,r31
    subf r5,r31,r5
cpy_22:
    srwi. r7,r5,2
    clrlwi r6,r5,30
    beq cpy_24
    mtctr r7
cpy_23:
    lwz r5,0(r4)
    addi r4,r4,4
    stw r5,0(r3)
    addi r3,r3,4
    bdnz cpy_23
cpy_24:
    cmpwi r6,0
    beq cpy_26
    mtctr r6
cpy_25:
    lbz r5,0(r4)
    addi r4,r4,1
    stb r5,0(r3)
    addi r3,r3,1
    bdnz cpy_25
cpy_26:
    mr r3,r0
cpy_27:
    lwz r31,12(r1)
    addi r1,r1,16
    blr
}

/* 0x8051D60C (0x104): fills `size` bytes with the byte `value`, 32-byte blocks through `dcbz` when the run is long. */
/* untyped: byte range */
asm void* NETMemSet(void* dst, s32 value, u32 size) {
    nofralloc
    cmpwi r5,0
    mr r9,r3
    beqlr
    cmplwi r5,64
    rlwimi r4,r4,8,16,23
    rlwimi r4,r4,16,0,15
    blt set_10
    clrlwi. r8,r3,27
    beq set_5
    subfic r8,r8,32
    mr r7,r3
    srwi. r6,r8,2
    clrlwi r0,r8,30
    beq set_2
    mtctr r6
set_1:
    stw r4,0(r7)
    addi r7,r7,4
    bdnz set_1
set_2:
    cmpwi r0,0
    beq set_4
    mtctr r0
set_3:
    stb r4,0(r7)
    addi r7,r7,1
    bdnz set_3
set_4:
    add r3,r3,r8
    subf r5,r8,r5
set_5:
    cmpwi r4,0
    mr r6,r3
    clrrwi r7,r5,5
    srwi r0,r5,5
    bne set_7
    mtctr r0
set_6:
    dcbz 0,r6
    addi r6,r6,32
    bdnz set_6
    b set_9
set_7:
    mtctr r0
set_8:
    dcbz 0,r6
    stw r4,0(r6)
    stw r4,4(r6)
    stw r4,8(r6)
    stw r4,12(r6)
    stw r4,16(r6)
    stw r4,20(r6)
    stw r4,24(r6)
    stw r4,28(r6)
    addi r6,r6,32
    bdnz set_8
set_9:
    add r3,r3,r7
    subf r5,r7,r5
set_10:
    srwi. r6,r5,2
    clrlwi r0,r5,30
    beq set_12
    mtctr r6
set_11:
    stw r4,0(r3)
    addi r3,r3,4
    bdnz set_11
set_12:
    cmpwi r0,0
    beq set_14
    mtctr r0
set_13:
    stb r4,0(r3)
    addi r3,r3,1
    bdnz set_13
set_14:
    mr r3,r9
    blr
}
