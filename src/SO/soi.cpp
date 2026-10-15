/*
 * SO/soi.cpp - the SO library (`SOInit`, `SOFinish`, `SOStartup`, `SOCleanup` and the `/dev/net/ip/top` session
 *   helpers `SOiPrepare` / `SOiAlloc` / `SOiFree` / `SOiConclude` the SOCKET library builds on): the RVL_SDK "SO"
 *   build. Its sibling `SO/socket.cpp` is the SOCKET build.
 *
 * RANGE. `.text` 0x8051E864..0x8051F9D0 (17 functions); `.data` 0x80631230..0x80631410; `.bss`
 *   0x80766C40..0x80766C58; `.sdata` 0x80794448..0x80794450; `.sbss` 0x807958D8..0x807958E8.
 * RANGE. Two retail TUs by the two version strings (SO built 12:00:00, SOCKET 12:00:01): every referrer of this
 *   unit's data (`soWork`, `soState`, `soLastError`, `soRegistered`, `soBufferAddrCheck`, the three jump tables)
 *   lies below 0x8051F9D0, and the SOCKET TU's `.data`, `.sdata` and `.sbss` start 8-aligned after these ranges.
 * FLAGS. the NWC24 lib block's GC/3.0a5.2 + `cflags_nwc24` (docs/network.md "SDK library compilers"); the file sets
 *   `#pragma auto_inline off` (retail calls `SOiPrepare`/`SOiConclude`/`SOiAlloc` from bodies after them).
 * NAMES. The map's SDK names; `SOStartupEx`, `SOiWaitForDHCPEx` and every `.bss`/`.sbss`/`.sdata` object name
 *   (`soState`, `soLastError`, `soRegistered`, `soVersion`, `soBufferAddrCheck`, `soWork`) are GUESSes in the SDK's
 *   scheme; `NCDGetLinkStatus` (0x8051C7A4) is a GUESS too.
 *   GUESS (map name; the runtime dump has only a placeholder or another name at the address): `SOInit`, `SOFinish`
 *   GUESS: `SOStartup`, `SOStartupEx`, `SOCleanup`, `SOiGetLastError`, `SOiWaitForDHCPEx`
 * SHAPES. Every request is `SOiPrepare` -> `SOiAlloc`'d IPC block -> `IOS_Ioctl(v)` -> `SOiFree` -> `SOiConclude`,
 *   written `if (result == 0) { ...; result = SOiConclude(...); } return result;`; the local declaration order is
 *   load-bearing for the register allocation. The state switches spell `case 0: default:` (one compare tree), the
 *   NWC24 lock switch carries an extra `case -22` so MWCC builds retail's jump table, the alloc / free / MEM2 /
 *   alignment checks are static inlines, the per-thread error lives at the OS thread record's +0x30C.
 * SHAPES. `SOiAlloc` tests the size and the allocator itself and returns the inline `SOiAllocChecked` (its own
 *   `li r3,0` return beside the helper's `mr r3,r4`); returning the nested `SOiAllocBlock` that `SOInit` inlines
 *   shares one `li r4,0` tail instead.
 * RESIDUALS. The `.data` (4 B) and `.sbss` (4 B) ranges end in the alignment padding before the SOCKET TU's data,
 *   which the source cannot emit as a definition.
 */
#include "SO/soi.h"
#include "NAND/nand.h"
#include "OS/__OSGetSystemTime.h"
#include "NCD/ncdsystem.h"
#include "NWC24/nwc24_msg.h"
#include "IPC/ipcclt.h"
#include "unsplit/OS.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/__va_arg.h"

#pragma auto_inline off

extern "C" {

/* The OS thread record as SO sees it: the per-thread socket error at +0x30C. */
typedef struct SOThreadView {
    /* +0x000 */ u8 pad_0x000[0x30C];
    /* +0x30C */ s32 soError;
} SOThreadView; /* size: 0x310 (the head of the 0x318-byte OSThread) */

char* soVersion = "<< RVL_SDK - SO \trelease build: Jun  9 2009 12:00:00 (0x4199_60831) >>";
BOOL soBufferAddrCheck = 1;

u8 soState;
s32 soLastError;
u32 soRegistered;

SOSysWork soWork;

/* Records `error` as the calling thread's socket error (the library's own word when there is no thread). */
static inline void SOiSetError(s32 error) {
    OSThread* thread = OSGetCurrentThread();
    if (thread != NULL) {
        ((SOThreadView*)thread)->soError = error;
    } else {
        soLastError = error;
    }
}

/* Sleeps `msec` milliseconds. */
static inline void SOiSleep(s64 msec) {
    OSSleepTicks(msec * (OS_BUS_CLOCK / 4 / 1000));
}

/* Frees a block through the registered free function, counted. */
/* untyped: caller-owned payload - an IPC buffer */
static inline void SOiFreeBlock(u32 name, void* p, s32 size) {
    if (p != NULL) {
        if (soWork.free != NULL) {
            soWork.allocCount--;
            soWork.free(name, p, size);
        }
    }
}

/* Allocates through the registered alloc function, counted; NULL outside MEM2 when the check is on. */
/* untyped: caller-owned payload - an IPC buffer */
static inline void* SOiAllocChecked(u32 name, s32 size) {
    void* p = soWork.alloc(name, size);
    u32 physical;

    if (p != NULL) {
        soWork.allocCount++;
        if (soBufferAddrCheck) {
            physical = (u32)p & 0x1FFFFFFF;
            if (physical < 0x10000000 || physical >= 0x18000000) {
                SOiFreeBlock(name, p, size);
                p = NULL;
            }
        }
    }
    return p;
}

/* Allocates through the registered allocator, counted; NULL for an empty request or no allocator. */
/* untyped: caller-owned payload - an IPC buffer */
static inline void* SOiAllocBlock(u32 name, s32 size) {
    if (size > 0 && soWork.alloc != NULL) {
        return SOiAllocChecked(name, size);
    }
    return NULL;
}

/* 0x8051E864 (0x1CC): registers the allocator pair and allocates the resolver buffer. */
s32 SOInit(const SOLibraryConfig* config) {
    s32 result = 0;
    BOOL enabled = OSDisableInterrupts();

    if (!(soRegistered & 1)) {
        OSRegisterVersion(soVersion);
        soRegistered |= 1;
    }
    switch (soState) {
    case 1:
    case 2:
        result = -7;
        break;
    case 0:
    default:
        if (config == NULL || config->alloc == NULL || config->free == NULL) {
            result = -28;
            break;
        }
        NETMemSet(&soWork, 0, sizeof(SOSysWork));
        soWork.alloc = config->alloc;
        soWork.free = config->free;
        soWork.allocCount = 0;
        soWork.state = -2;
        soWork.fd = -1;
        soWork.hostBuffer = (u8*)SOiAllocBlock(11, 0x460);
        if (soWork.hostBuffer == NULL) {
            result = -49;
        }
        if (soWork.hostBuffer != NULL) {
            soState = 1;
        }
        break;
    }
    SOiSetError(result);
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051EA30 (0xFC): releases the resolver buffer once the interface is down. */
s32 SOFinish(void) {
    s32 result = 0;
    BOOL enabled = OSDisableInterrupts();

    switch (soState) {
    case 0:
        result = -7;
        break;
    case 1:
        if (soWork.state > -2) {
            result = -10;
        } else if (soWork.allocCount > 1) {
            result = -6;
        } else {
            soState = 0;
            SOiFreeBlock(11, soWork.hostBuffer, 0x460);
        }
        break;
    case 2:
        result = -26;
        break;
    }
    SOiSetError(result);
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051EB2C (0xC): brings the interface up with a one-minute timeout. */
s32 SOStartup(void) {
    return SOStartupEx(60000);
}

/* Maps an NWC24 command answer onto an SO result. */
static inline s32 SOiConvertNWC24Result(s32 answer, s32 value) {
    s32 result = -28;
    switch (answer) {
    case 0:
        result = 0;
        break;
    case -22:
    case -13:
        result = -48;
        break;
    case -33:
    case -2:
        result = value;
        break;
    case -1:
        result = (s32)0x80000000;
        break;
    case -29:
        result = -26;
        break;
    }
    return result;
}

/* 0x8051EB38 (0x428): brings the interface up: waits for the link, opens the IP device, asks the NWC24 daemon to
 * release the network and waits for an address; retried while the address wait reports -112. */
s32 SOStartupEx(s32 timeout) {
    s32 result;
    BOOL enabled;
    s64 deadline = 0;
    s32 retry;
    s32 link;
    s32 answer;
    u32 value;

    if (timeout != 0) {
        deadline = __OSGetSystemTime() + (s64)(u32)(timeout * (OS_BUS_CLOCK / 4 / 1000));
    }
    retry = 4;
    do {
        enabled = OSDisableInterrupts();
        switch (soState) {
        case 0:
        default:
            result = -39;
            break;
        case 2:
            result = -7;
            break;
        case 1:
            if (soWork.state > -2) {
                result = -10;
            } else if (OSGetCurrentThread() == NULL) {
                result = (s32)0x80000000;
            } else {
                soWork.state = -1;
                OSRestoreInterrupts(enabled);
                for (;;) {
                    link = NCDGetLinkStatus();
                    if (link == -8 || link == 1) {
                        SOiSleep(100);
                        if (deadline != 0 && __OSGetSystemTime() > deadline) {
                            if (link == 1) {
                                result = -121;
                            } else {
                                result = (s32)0x80000000;
                            }
                            break;
                        }
                    } else if (link < 0) {
                        result = (s32)0x80000000;
                        break;
                    } else if (link == 2) {
                        result = -45;
                        break;
                    } else {
                        for (;;) {
                            soWork.fd = IOS_Open("/dev/net/ip/top", 0);
                            if (soWork.fd == -6) {
                                SOiSleep(100);
                                if (deadline != 0 && __OSGetSystemTime() > deadline) {
                                    result = (s32)0x80000000;
                                    break;
                                }
                            } else if (soWork.fd < 0) {
                                result = (s32)0x80000000;
                                break;
                            } else {
                                result = 0;
                                value = 0;
                                for (;;) {
                                    answer = NWC24iRequestCommand6(&value);
                                    if (answer == -29) {
                                        SOiSleep(100);
                                        if (deadline != 0 && __OSGetSystemTime() > deadline) {
                                            result = (s32)0x80000000;
                                            break;
                                        }
                                    } else {
                                        break;
                                    }
                                }
                                if (result == 0) {
                                    result = SOiConvertNWC24Result(answer, value);
                                }
                                if (result != 0) {
                                    if (IOS_Close(soWork.fd) < 0) {
                                        result = (s32)0x80000000;
                                    } else {
                                        soWork.fd = -1;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
                enabled = OSDisableInterrupts();
                if (result == 0) {
                    soState = 2;
                    soWork.state = 0;
                } else {
                    soState = 1;
                    if (result != (s32)0x80000000) {
                        soWork.state = -2;
                    }
                }
            }
            break;
        }
        SOiSetError(result);
        OSRestoreInterrupts(enabled);
        if (result == 0) {
            s64 remaining = deadline != 0 ? deadline - __OSGetSystemTime() : 0;
            if (deadline != 0 && remaining <= 0) {
                result = -76;
            } else {
                result = SOiWaitForDHCPEx((u32)(remaining / (OS_BUS_CLOCK / 4 / 1000)));
            }
            if (result != 0) {
                SOCleanup();
            }
        }
        SOiSetError(result);
    } while (result == -112 && --retry >= 0);
    return result;
}

/* 0x8051EF60 (0x1B0): takes the interface down and hands the network back to the NWC24 daemon. */
s32 SOCleanup(void) {
    s32 result;
    BOOL enabled = OSDisableInterrupts();
    u32 value = 0;
    s32 answer;

    switch (soState) {
    case 0:
    default:
        result = -39;
        break;
    case 1:
        result = -7;
        break;
    case 2:
        if (soWork.state < 0) {
            result = -10;
        } else if (OSGetCurrentThread() == NULL) {
            result = (s32)0x80000000;
        } else {
            soWork.state = -1;
            OSRestoreInterrupts(enabled);
            answer = NWC24iRequestCommand7(&value);
            result = SOiConvertNWC24Result(answer, value);
            if (result == 0) {
                if (IOS_Close(soWork.fd) < 0) {
                    result = (s32)0x80000000;
                } else {
                    soWork.fd = -1;
                }
            }
            enabled = OSDisableInterrupts();
            if (result == 0) {
                soState = 1;
                soWork.state = -2;
            } else if (result != (s32)0x80000000) {
                soState = 2;
                soWork.state = 0;
            }
        }
        break;
    }
    SOiSetError(result);
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051F110 (0x34): the calling thread's socket error. */
s32 SOiGetLastError(void) {
    OSThread* thread = OSGetCurrentThread();
    if (thread == NULL) {
        return soLastError;
    }
    return ((SOThreadView*)thread)->soError;
}

/* 0x8051F144 (0xC): the library's work record. */
SOSysWork* SOiGetSysWork(void) {
    return &soWork;
}

/* 0x8051F150 (0x8): non-zero when IPC buffers must be in MEM2. */
BOOL SOiIsBufferAddrCheck(void) {
    return soBufferAddrCheck;
}

/* 0x8051F158 (0x50): 1 once `SOInit` ran (and until `SOFinish`). */
BOOL SOiIsInitialized(void) {
    BOOL initialized = FALSE;
    BOOL enabled = OSDisableInterrupts();
    switch (soState) {
    case 1:
    case 2:
        initialized = TRUE;
        break;
    }
    OSRestoreInterrupts(enabled);
    return initialized;
}

/* 0x8051F1A8 (0xE8): allocates `size` bytes through the registered allocator (NULL outside MEM2 when the check is
 * on). */
/* untyped: caller-owned payload - an IPC buffer */
void* SOiAlloc(u32 name, s32 size) {
    if (size > 0 && soWork.alloc != NULL) {
        return SOiAllocChecked(name, size);
    }
    return NULL;
}

/* 0x8051F290 (0x34): frees a block `SOiAlloc` returned. */
/* untyped: caller-owned payload - an IPC buffer */
void SOiFree(u32 name, void* p, s32 size) {
    SOiFreeBlock(name, p, size);
}

/* 0x8051F2C4 (0xE8): checks the interface is up and hands out the IP device fd; the error otherwise. */
s32 SOiPrepare(const char* function, s32* fd) {
    s32 result = 0;
    BOOL enabled = OSDisableInterrupts();

    switch (soState) {
    case 0:
        result = -39;
        break;
    case 1:
    default:
        result = -28;
        break;
    case 2:
        if (soWork.state < 0) {
            result = -10;
        } else if (OSGetCurrentThread() == NULL) {
            result = (s32)0x80000000;
        } else {
            *fd = soWork.fd;
        }
        break;
    }
    if (result != 0) {
        SOiSetError(result);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051F3AC (0x5C): records the request's result as the thread's error and answers it. */
s32 SOiConclude(const char* function, s32 result) {
    BOOL enabled = OSDisableInterrupts();
    SOiSetError(result);
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051F408 (0x2D8): like `SOiPrepare`, but opens the IP device for one request when the interface is down
 * (`*opened` says so, for `SOiConcludeTempRm`). */
s32 SOiPrepareTempRm(const char* function, s32* fd, s32* opened) {
    s32 result = 0;
    BOOL enabled = OSDisableInterrupts();
    s32 lock;

    switch (soState) {
    case 0:
        result = -39;
        break;
    case 1:
    default:
        if (soWork.state > -2) {
            result = -10;
        } else if (OSGetCurrentThread() == NULL) {
            result = (s32)0x80000000;
        } else {
            soWork.state = -1;
            OSRestoreInterrupts(enabled);
            soWork.fd = IOS_Open("/dev/net/ip/top", 0);
            if (soWork.fd < 0) {
                enabled = OSDisableInterrupts();
                if (soWork.fd == -6) {
                    result = -26;
                    soWork.state = -2;
                } else {
                    result = (s32)0x80000000;
                }
            } else {
                *fd = soWork.fd;
                lock = NWC24iLockSocket();
                if (lock == 0) {
                    switch (NCDGetLinkStatus()) {
                    case 3:
                    case 4:
                    case 5:
                        result = IOS_Ioctl(soWork.fd, 0x1F, NULL, 0, NULL, 0);
                        if (result == 0) {
                            *opened = 1;
                        } else {
                            result = SOiConcludeTempRm(function, result, 1);
                        }
                        break;
                    case -8:
                        result = SOiConcludeTempRm(function, -26, 1);
                        break;
                    case -2:
                    case -1:
                        result = SOiConcludeTempRm(function, (s32)0x80000000, 1);
                        break;
                    default:
                        result = SOiConcludeTempRm(function, -48, 1);
                        break;
                    }
                    enabled = OSDisableInterrupts();
                } else {
                    switch (lock) {
                    case -29:
                        result = -26;
                        break;
                    case -2:
                        result = -39;
                        break;
                    case -15:
                        result = -48;
                        break;
                    case -22:
                    case -1:
                    default:
                        result = (s32)0x80000000;
                        break;
                    }
                    if (IOS_Close(soWork.fd) < 0) {
                        result = (s32)0x80000000;
                    }
                    enabled = OSDisableInterrupts();
                    if (result != (s32)0x80000000) {
                        soWork.fd = -1;
                        soWork.state = -2;
                    }
                }
            }
        }
        break;
    case 2:
        if (soWork.state < 0) {
            result = -10;
        } else if (OSGetCurrentThread() == NULL) {
            result = (s32)0x80000000;
        } else {
            *opened = 0;
            *fd = soWork.fd;
        }
        break;
    }
    if (result != 0) {
        SOiSetError(result);
    }
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051F6E0 (0xE4): ends a `SOiPrepareTempRm` request: closes the device it opened and records the result. */
s32 SOiConcludeTempRm(const char* function, s32 result, s32 opened) {
    BOOL enabled;

    if (opened == 1) {
        switch (NWC24iUnlockSocket()) {
        case -29:
            result = -26;
            break;
        case -1:
        default:
            result = (s32)0x80000000;
            break;
        case 0:
            break;
        }
        if (IOS_Close(soWork.fd) < 0) {
            result = (s32)0x80000000;
        }
        enabled = OSDisableInterrupts();
        if (result != (s32)0x80000000) {
            soWork.fd = -1;
            soWork.state = -2;
        }
    } else {
        enabled = OSDisableInterrupts();
    }
    SOiSetError(result);
    OSRestoreInterrupts(enabled);
    return result;
}

/* 0x8051F7C4 (0x138): waits until the interface has an address (or `timeout` milliseconds pass). */
s32 SOiWaitForDHCPEx(u32 timeout) {
    s32 result = 0;
    s64 deadline = 0;
    s32 status;
    s32 length;
    s32 error;

    if (timeout != 0) {
        deadline = __OSGetSystemTime() + (s64)(u32)(timeout * (OS_BUS_CLOCK / 4 / 1000));
    }
    for (;;) {
        SOiSleep(10);
        length = 4;
        error = SOGetInterfaceOpt(NULL, 0xFFFE, 0x1003, &status, &length);
        if (error != 0) {
            result = error;
            break;
        }
        if (error == 0 && status != 0) {
            result = status;
            break;
        }
        if (SOGetHostID() != 0) {
            break;
        }
        if (deadline != 0 && __OSGetSystemTime() > deadline) {
            result = -76;
            break;
        }
    }
    return result;
}

/* Opens a socket of `domain` / `type` / `protocol`. */
static inline s32 SOiCreateSocket(s32 domain, s32 type, s32 protocol) {
    s32 fd;
    s32* block;
    s32 result;

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

/* 0x8051F8FC (0xD4): opens a socket. */
s32 __SOCreateSocket(s32 domain, s32 type, s32 protocol) {
    return SOiCreateSocket(domain, type, protocol);
}

}
