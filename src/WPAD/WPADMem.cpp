/* WPAD/WPADMem.cpp - the memory transfers of the Wii remote driver (the SDK's WPADMem.c): the memory-block reset, the
 *   title-checked game-data write and read, and the remote memory and extension register reads and writes.
 * RANGE. .text 0x804F8EC0-0x804F9AE0 (13 functions); .bss 0x8075EA68-0x8075EAA0 (`wpadTitleBuf`).  Seam with
 *   WPAD/WPADClamp.cpp at 0x804F8EC0: the SDK file list, codegen-neutral.  Not WPAD.c's TU: this unit calls the 8-byte
 *   `wpadGetGameName`/`wpadGetAppType` out of line, and `wpadReadMemory`, `wpadWriteMemory` and `WPADWriteExtReg` call
 *   `WPADiSendWriteData`/`wpadSendReadMemory` out of line, which one TU inlines (measured: inlined they are 556 B against the
 *   original's 204 B).  Seam with WPAD/debug_msg.cpp at 0x804F9AE0: the SDK file list, codegen-neutral.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment).
 * NAMES. the map's `WPADWriteExtReg`.  GUESS: the unit name (SDK spelling), `WPADiClearMemBlock` (the memory-block reset
 *   `wpadResetChannel` calls), `WPADWriteGameData`, `WPADReadGameData` (the title-checked game-data write and read; game code
 *   calls the write) and their completion `wpadGameDataCallback`, `wpadBulkWriteNext`, `wpadBulkWriteFirst`,
 *   `wpadSetDpdStatusA/B`, `wpadGetDpdCalibration`, `wpadReadMemory`, `wpadWriteMemory`, `wpadCopyTitleString`.
 * RESIDUALS. None; no flip blocker.
 * SHAPES. The definitions follow the address order (the object's layout is the source order).  The status/ready pair is
 *   read after `OSDisableInterrupts` with `ready` declared first (register order r29/r30); a bulk-write chunk goes through
 *   `wpadBulkWriteChunk` (its own `level`); the game-data record is written in
 *   `WPADMemChunk` steps from its first byte (an indexed `calBytes` spelling stops CSE-ing the chunk addresses).
 */

#include "types.h"
#include "WPAD/wpad.h"
#include "WPAD/WPADMem.h"
#include "OS/OSTime.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "MSL_C/alloc.h"

/* One 16-byte chunk of a remote memory transfer. */
typedef u8 WPADMemChunk[0x10];

/* .bss */
static u16 wpadTitleBuf[0x1C];

/* Clears the speaker state words of a channel. */
void WPADiClearMemBlock(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    cb->speakerStateA = 0;
    cb->speakerStateB = 0;
    cb->speakerStateC = 0;
    cb->speakerStateD = 0;
    cb->bulkDoneCallback = NULL;
}

/* Runs the callback the bulk transfer stored. */
void wpadGameDataCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCallback callback;

    cb->speakerStateA = 0;
    callback = cb->bulkDoneCallback;
    cb->bulkDoneCallback = NULL;
    if (callback != NULL) {
        callback(chan, result);
    }
}

/* Continues a bulk write: sends the next 16-byte chunk, or reports completion when the data is spent or a chunk fails. */
/* Sends the next chunk of a bulk write with interrupts off. */
static inline void wpadBulkWriteChunk(s32 chan, WPADCB* cb, u16 chunk)
{
    u32 level = OSDisableInterrupts();
    wpadWriteMemory(chan, (const void*)cb->speakerStateB, chunk, cb->speakerStateD, wpadBulkWriteNext);
    OSRestoreInterrupts(level);
}

void wpadBulkWriteNext(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 remaining = cb->speakerStateC;
    u16 chunk;
    WPADCallback done;

    if (remaining == 0) {
        cb->speakerStateA = 0;
        done = cb->bulkDoneCallback;
        cb->bulkDoneCallback = NULL;
        if (done != NULL) {
            done(chan, result);
        }
    } else {
        chunk = (remaining <= 0x10) ? remaining : 0x10;
        if (result == 0) {
            cb->speakerStateC -= chunk;
            cb->speakerStateD = cb->speakerStateD + 0x10;
            cb->speakerStateB = cb->speakerStateB + 0x10;
            wpadBulkWriteChunk(chan, wpadCBTable[chan], chunk);
        } else if (result == -2) {
            wpadBulkWriteChunk(chan, cb, chunk);
        } else {
            cb->speakerStateA = 0;
            done = cb->bulkDoneCallback;
            cb->bulkDoneCallback = NULL;
            if (done != NULL) {
                done(chan, result);
            }
        }
    }
}

/* Finishes the calibration write and starts sending the bulk data, or reports the failure. */
void wpadBulkWriteFirst(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 chunk;

    if (cb->dpdStatusB == 0) {
        cb->dpdStatusB = (result == 0) ? 0 : -3;
    }
    if (result == 0) {
        if (cb->dpdStatusA == 0 || cb->dpdStatusB == 0) {
            chunk = (cb->speakerStateC <= 0x10) ? cb->speakerStateC : 0x10;
            cb->speakerStateC -= chunk;
            wpadBulkWriteChunk(chan, wpadCBTable[chan], chunk);
            return;
        }
        result = -3;
    }
    if (cb->bulkDoneCallback != NULL) {
        cb->speakerStateA = 0;
        cb->bulkDoneCallback(chan, result);
        cb->bulkDoneCallback = NULL;
    }
}

/* Records the outcome of the first pointer-calibration read (non-zero `error` = failed). */
void wpadSetDpdStatusA(s32 chan, s32 error)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dpdStatusA == 0) {
        cb->dpdStatusA = (error == 0) ? 0 : -3;
    }
}

void wpadSetDpdStatusB(s32 chan, s32 error)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dpdStatusB == 0) {
        cb->dpdStatusB = (error == 0) ? 0 : -3;
    }
}

/* Stores the calibration block with the title code and checksum, then writes it to the remote followed by the bulk data. */
s32 WPADWriteGameData(s32 chan, const void* src, u16 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    u32 level;
    WPADCB* cb = wpadCBTable[chan];
    s32 status;
    s32 ready;

    level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else if (cb->speakerStateA == 0) {
            if (wpadQueueHasRoom(&cb->cmdQueue, 9) == 0) {
                status = -2;
            } else {
                u8 i;
                cb->speakerStateA = 1;
                cb->bulkDoneCallback = callback;
                cb->speakerStateC = size;
                cb->speakerStateD = addr + 0x9A;
                cb->speakerStateB = (u32)src;
                memcpy(cb->cal.titleCode, wpadGetGameName(), 4);
                memcpy(cb->cal.dpdCalibration, wpadTitleBuf, 0x22);
                cb->cal.timestamp = OSGetTime();
                cb->cal.appType = wpadGetAppType();
                cb->cal.checksum = 0;
                for (i = 0; i < 0x2F; i++) {
                    cb->cal.checksum = cb->cal.checksum + cb->calBytes[i];
                }
                cb->cal.checksum = cb->cal.checksum + 0x55;
                cb->dpdStatusA = 0;
                cb->dpdStatusB = 0;
                wpadWriteMemory(chan, (u8*)&cb->cal, 0x10, 0x2A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk), 0x10, 0x3A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk) * 2, 0x10, 0x4A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk) * 3, 8, 0x5A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, (u8*)&cb->cal, 0x10, 0x62, wpadSetDpdStatusB);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk), 0x10, 0x72, wpadSetDpdStatusB);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk) * 2, 0x10, 0x82, wpadSetDpdStatusB);
                wpadWriteMemory(chan, (u8*)&cb->cal + sizeof(WPADMemChunk) * 3, 8, 0x92, wpadBulkWriteFirst);
                OSRestoreInterrupts(level);
                return 0;
            }
        } else {
            status = -2;
        }
    }
    OSRestoreInterrupts(level);
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Starts a guarded read of the remote's memory once the extension signature matches the title's game code. */
s32 WPADReadGameData(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 ready;
    s32 status;
    u32 level = OSDisableInterrupts();

    status = cb->status;
    ready = cb->ready;
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else if (cb->speakerStateA == 0) {
            s32 missing = 0;
            if (cb->dpdStatusA == 0 || cb->dpdStatusB == 0) {
                missing = 1;
            }
            status = missing != 0 ? 0 : -6;
            if (status == 0) {
                if (memcmp(cb->cal.titleCode, wpadGetGameName(), 4) == 0) {
                    cb->speakerStateA = 1;
                    cb->bulkDoneCallback = callback;
                    OSRestoreInterrupts(level);
                    return wpadReadMemory(chan, dest, size, addr + 0x9A, wpadGameDataCallback);
                }
                status = -5;
            }
        } else {
            status = -2;
        }
    }
    OSRestoreInterrupts(level);
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Queues a memory read of the remote; returns 0 or a negative error. */
s32 wpadReadMemory(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 ready;
    s32 status;
    u32 level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = wpadSendReadMemory(&cb->cmdQueue, dest, size, addr, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Queues a memory write to the remote; returns 0 or a negative error. */
s32 wpadWriteMemory(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 ready;
    s32 status;
    u32 level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = WPADiSendWriteData(&cb->cmdQueue, src, size, addr, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Hands out the pointer-calibration block once both calibration reads have finished; returns 0 or a negative error. */
s32 wpadGetDpdCalibration(s32 chan, u8** out)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 missing = 0;
    s32 status;

    if (cb->dpdStatusA == 0 || cb->dpdStatusB == 0) {
        missing = 1;
    }
    status = missing != 0 ? 0 : -4;
    if (status == 0) {
        *out = cb->cal.dpdCalibration;
    } else {
        *out = NULL;
    }
    OSRestoreInterrupts(level);
    return status;
}

/* Copies the title string (at most 16 wide characters, zero terminated) into the title buffer. */
void wpadCopyTitleString(const u16* title)
{
    s32 i;

    for (i = 0; i < 0x10; i++) {
        wpadTitleBuf[i] = title[i];
        if (title[i] == 0) {
            break;
        }
    }
    wpadTitleBuf[0x10] = 0;
}

/* Writes to the extension register space (the remote adds the 0x04A4 prefix). */
s32 WPADWriteExtReg(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 ready;
    s32 status;
    u32 level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = WPADiSendWriteData(&cb->cmdQueue, src, size, addr | 0x04A40000, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}
