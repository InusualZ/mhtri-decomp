/*
 * OS/OSPlayTime.c - the OS play-time limit: the expired flag, the alarm, `__OSGetPlayTime` and `__OSInitPlayTime`.
 * RANGE. .text 0x804D6A10-0x804D71F0 (8 functions); .data 0x806297D8-0x80629818; .bss 0x8074E380-0x8074E3B0; .sdata
 *    0x80793FD0-0x80793FD8; .sbss 0x807953F0-0x80795408; .sdata2 0x8079D320-0x8079D330.  Cut from the OS core
 *    band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor "OSPlayTime.c" at .data 0x806297EC and the
 *    expired-flag path (0x806297D8) open its strings; `__OSExpireAlarm` (.bss 0x8074E380) and the `__OSExpire*`
 *    words (.sbss 0x807953F0..0x80795408) are read only here; `OSLaunchPackArgs` heads the next unit.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut.
 * NAMES. `OSPlayTimeIsLimited`, `__OSWriteExpiredFlag`, `__OSGetPlayTime`, `__OSInitPlayTime` are the map's names.  GUESS:
 *    `__OSPlayTimeFadeCallback`, `__OSPlayTimeExpiryThread` (the fade-out DMA callback and the thread the expiry alarm starts),
 *    `__OSPlayTimeFadeState` (the pointer to the thread's stack state), `PlayTimeFadeState`.
 * RESIDUALS. `__OSInitPlayTime`: the target tests the two switch results as a compare chain (-1017 first) where the source
 *    switch gives a tree, and adds the 20 s margin with `li`+`addc` where the source gives `addic`; `__OSPlayTimeAlarmExpired`:
 *    one `mr` scheduled earlier; `__OSGetPlayTime`: the empty-branch shape of the consumption test and a limit
 *    reload; `__OSPlayTimeFadeCallback`: a callee-saved swap and the order of the two float constants; `__OSWriteExpiredFlag`:
 *    `li` pair order.  The .sdata2 claim (float 0.995 and the int-to-float bias) is not emitted by name; .sbss differs by
 *    the 64-bit alignment pad.
 * SHAPES. the expiry thread's state and the ticket-view copy are `aligned(32)` locals (the ES and DMA calls need aligned buffers).
 */

#define OS_THREAD_TYPED_API

#include "types.h"
#include "AI_SDK/ai.h"
#include "ESP/esp.h"
#include "MSL_C/string.h"
#include "NAND/nand.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSPlayTime.h"
#include "OS/OSReset.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSThread.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "VI/vi.h"

/* The low-memory words the play-time code reads: the active thread list and the top of the arena the stack is cut from. */
#define OS_ACTIVE_THREAD_QUEUE (*(OSThreadQueue*)0x800000DC)
#define OS_ARENA_HI_WORD (*(u32*)0x80003128)

#define PLAY_TIME_THREAD_STACK_SIZE 0x1000
#define PLAY_TIME_THREAD_AREA_SIZE 0x1320
#define PLAY_TIME_FADE_STEPS 20

/* size: 0x494 - the fade-out DMA state the expiry thread keeps on its stack: two sample buffers, the one being filled, the read
 * pointer into the playing buffer, a block counter, the last stereo sample and the callback it replaced. */
typedef struct PlayTimeFadeState {
    /* +0x000 */ s16 buffer[2][0x120];
    /* +0x480 */ s32 bufferIndex;
    /* +0x484 */ s16* readPtr;
    /* +0x488 */ u32 blockCount;
    /* +0x48C */ s16 left;
    /* +0x48E */ s16 right;
    /* +0x490 */ AIDMACallback previousCallback;
} PlayTimeFadeState;

/* The expiry fade state, the alarm that fires the expiry, the time it fires at and the hook that replaces it. */
PlayTimeFadeState* __OSPlayTimeFadeState;
BOOL __OSExpireSetExpiredFlag;
void (*__OSExpireCallback)(void);
s64 __OSExpireTime;
OSAlarm __OSExpireAlarm;

/* 0x804D6A10 (0x18): returns whether an expiry time is armed. */
BOOL OSPlayTimeIsLimited(void)
{
    return __OSExpireTime != 0;
}

/* 0x804D6A30 (0x1BC): the DMA callback of the expiry fade; refills the next buffer with the last sample scaled down. */
void __OSPlayTimeFadeCallback(void)
{
    PlayTimeFadeState* state;
    s16* start;
    s16* out;
    u32 length;
    u32 remaining;

    if (__OSPlayTimeFadeState->previousCallback != NULL) {
        __OSPlayTimeFadeState->previousCallback();
    }
    if (__OSPlayTimeFadeState->blockCount == 0) {
        __OSPlayTimeFadeState->readPtr = (s16*)(AIGetDMAStartAddr() + 0x80000000);
    }
    if (__OSPlayTimeFadeState->blockCount == 1) {
        DCInvalidateRange(__OSPlayTimeFadeState->readPtr, 4);
        __OSPlayTimeFadeState->left = *__OSPlayTimeFadeState->readPtr++;
        __OSPlayTimeFadeState->right = *__OSPlayTimeFadeState->readPtr;
    }
    if (__OSPlayTimeFadeState->blockCount >= 1) {
        start = &__OSPlayTimeFadeState->buffer[__OSPlayTimeFadeState->bufferIndex][0];
        out = start;
        length = __ARGetInterruptStatus();
        for (remaining = length; remaining != 0; remaining -= 4) {
            out[0] = __OSPlayTimeFadeState->left;
            out[1] = __OSPlayTimeFadeState->right;
            out += 2;
            __OSPlayTimeFadeState->left = (s16)((f32)__OSPlayTimeFadeState->left * 0.995f);
            __OSPlayTimeFadeState->right = (s16)((f32)__OSPlayTimeFadeState->right * 0.995f);
        }
        DCFlushRange(start, length);
        AIInitDMA((u32)start, length);
        __OSPlayTimeFadeState->bufferIndex++;
        __OSPlayTimeFadeState->bufferIndex &= 1;
    }
    __OSPlayTimeFadeState->blockCount++;
}

/* 0x804D6BF0 (0x114): writes the current title id to the expired-flag file, creating it when needed; returns whether it succeeded. */
BOOL __OSWriteExpiredFlag(void)
{
    u8 titleId[32] __attribute__((aligned(32)));
    NANDFileInfo file __attribute__((aligned(32)));
    s32 result;
    BOOL opened = FALSE;

    result = NANDPrivateCreate("/shared2/expired", 0x3F, 0);
    if (result == 0 || result == -6) {
        result = NANDPrivateOpen("/shared2/expired", &file, 2);
        if (result == 0) {
            opened = TRUE;
            result = ESP_InitLib();
            if (result == 0) {
                memset(titleId, 0, sizeof(titleId));
                result = ESP_GetTitleId((u64*)titleId);
                if (result == 0) {
                    result = NANDWrite(&file, titleId, sizeof(titleId));
                    if (result >= 0) {
                        if (result != 32) {
                            result = -8;
                        } else {
                            result = 0;
                        }
                    }
                }
            }
        }
    }
    if (opened) {
        NANDClose(&file);
    }
    if (result == 0) {
        return TRUE;
    }
    return FALSE;
}

/* 0x804D6D10 (0x18): writes the expired flag when the expiry asked for it. */
BOOL __OSWriteExpiredFlagIfSet(void)
{
    if (__OSExpireSetExpiredFlag) {
        return __OSWriteExpiredFlag();
    }
    return FALSE;
}

/* 0x804D6D30 (0xEC): the expiry thread: dims the screen while the audio fades, records the flag and returns to the menu. */
/* untyped: opaque handle - the thread entry's argument and exit value */
void* __OSPlayTimeExpiryThread(void* arg)
{
    PlayTimeFadeState state __attribute__((aligned(32)));
    BOOL enabled;
    u32 i;
    u32 level;

    __OSPlayTimeFadeState = &state;
    memset(&state, 0, sizeof(PlayTimeFadeState));
    __OSPlayTimeFadeState->previousCallback = AIRegisterDMACallback(__OSPlayTimeFadeCallback);
    for (i = 0; i < PLAY_TIME_FADE_STEPS; i++) {
        level = i / 5 + 1;
        if (level > 7) {
            level = 7;
        }
        VIWaitForRetrace();
        __OSSetVIForceDimming(1, level, level);
    }
    AIRegisterDMACallback(NULL);
    VISetBlack(TRUE);
    VIFlush();
    enabled = OSDisableInterrupts();
    if (__OSExpireSetExpiredFlag) {
        __OSWriteExpiredFlag();
    }
    OSRestoreInterrupts(enabled);
    OSReturnToMenu();
    return NULL;
}

/* 0x804D6E20 (0xA0): the alarm handler of the expiry: runs the installed hook, else suspends every thread and starts the expiry thread. */
void __OSPlayTimeAlarmExpired(OSAlarm* alarm, OSContext* context)
{
    OSThread* thread;
    OSThread* expiryThread;

    if (__OSExpireCallback != NULL) {
        __OSExpireCallback();
        return;
    }
    for (thread = OS_ACTIVE_THREAD_QUEUE.head; thread != NULL; thread = thread->linkActive.next) {
        OSSuspendThread(thread);
    }
    expiryThread = (OSThread*)(OS_ARENA_HI_WORD - PLAY_TIME_THREAD_AREA_SIZE);
    if (!OSCreateThread(expiryThread, __OSPlayTimeExpiryThread, NULL, (u8*)expiryThread + PLAY_TIME_THREAD_AREA_SIZE,
                        PLAY_TIME_THREAD_STACK_SIZE, 0, 0)) {
        __OSDoHotReset();
    }
    OSResumeThread(expiryThread);
}

/* 0x804D6EC0 (0x1CC): reads the kind and amount of the ticket's play-time limit, less what it has consumed; returns 0 on success. */
s32 __OSGetPlayTime(ESTicketView* ticketView, s32* limitKind, u32* limitValue)
{
    ESTicketView localView __attribute__((aligned(32)));
    ESConsumption consumed[8] __attribute__((aligned(32)));
    u32 consumedCount = 0;
    s32 result;
    s32 lastLimit = 0;
    s32 i;

    if ((u32)ticketView & 0x1F) {
        memcpy(&localView, ticketView, sizeof(ESTicketView));
        ticketView = &localView;
    }
    result = ESP_GetConsumption(((u64)ticketView->ticketIdHi << 32) | ticketView->ticketIdLo, NULL, &consumedCount);
    if (result <= 0) {
        if (result != 0) {
        } else if (consumedCount != 0) {
            result = ESP_GetConsumption(((u64)ticketView->ticketIdHi << 32) | ticketView->ticketIdLo, consumed, &consumedCount);
        }
    }
    if (result == 0) {
        for (i = 0; i < 8; i++) {
            if (ticketView->limits[i].kind == 1) {
                *limitKind = 1;
                if (consumedCount == 0) {
                    *limitValue = ticketView->limits[i].value;
                } else if (consumed[i].used >= ticketView->limits[i].value) {
                    *limitValue = 0;
                } else {
                    *limitValue = ticketView->limits[i].value - consumed[i].used;
                }
                return result;
            }
            if (ticketView->limits[i].kind != 0) {
                lastLimit = i + 1;
            }
        }
        if (lastLimit == 0) {
            *limitKind = 0;
            *limitValue = -1;
        } else {
            lastLimit--;
            if (ticketView->limits[lastLimit].kind == 4) {
                *limitKind = 4;
                *limitValue = ticketView->limits[lastLimit].value;
                if (consumedCount != 0) {
                    *limitValue = ticketView->limits[lastLimit].value - consumed[lastLimit].used;
                }
            } else {
                *limitKind = 9;
            }
        }
    }
    return result;
}

/* 0x804D7090 (0x158): reads the running title's ticket and arms the expiry alarm when it carries a time limit. */
void __OSInitPlayTime(void)
{
    ESTicketView ticketView __attribute__((aligned(32)));
    u32 limitValue;
    s32 limitKind;
    s32 result;
    u32 busClock;
    u64 ticks;

    __OSExpireTime = 0;
    __OSExpireCallback = NULL;
    __OSExpireSetExpiredFlag = TRUE;
    if (ESP_InitLib() != 0) {
    } else {
        result = ESP_DiGetTicketView(NULL, &ticketView);
        switch (result) {
        case 0:
            result = __OSGetPlayTime(&ticketView, &limitKind, &limitValue);
            break;
        case -1017:
            break;
        }
        switch (result) {
        case 0:
            if (limitKind != 0 && limitKind == 1) {
                if (limitValue == 0) {
                    OSPanic("OSPlayTime.c", 737, "Expired");
                }
                OSCreateAlarm(&__OSExpireAlarm);
                busClock = OS_BUS_CLOCK / 4;
                ticks = (limitValue + 20ULL) * busClock;
                OSSetAlarm(&__OSExpireAlarm, ticks, __OSPlayTimeAlarmExpired);
                __OSExpireTime = __OSExpireAlarm.fire;
                OSReport("PlayTime: %d seconds left
", limitValue);
            }
            break;
        case -1017:
            break;
        }
    }
    ESP_CloseLib();
}
