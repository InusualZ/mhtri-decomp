/*
 * OS/OSPlayRecord.c - the OS play record: the NAND state machine that reads, updates and writes the title's play-time
 *    record (`play_rec.dat`), its alarm handler, and `__OSStartPlayRecord`/`__OSStopPlayRecord`.
 * RANGE. .text 0x804D5E00-0x804D6520 (4 functions); .data 0x806295E0-0x80629628; .bss 0x8074E160-0x8074E360; .sdata
 *    0x80793FC8-0x80793FD0; .sbss 0x807953D0-0x807953F0.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: the play-record path string and the callback jump table (.data 0x806295E0..0x80629628), the
 *    0x200-byte work buffer (.bss 0x8074E160) and the `PlayRecord*` words are read only here; the 12-byte handler
 *    `PlayRecordAlarmHandler` (0x804D5E00) tail-branches to the `scope:local` `PlayRecordCallback`, which pins the
 *    left edge two words before the callback (a local referenced from another unit is renamed by dtk, measured).
 * FLAGS. `cflags_base` with `mw_version` GC/3.0a5.2 (the per-object override in configure.py): the callback's 7-case switch
 *    is a jump table in the target and a compare chain under Wii/1.3 (that compiler tables from 8 cases, the GC
 *    compilers from 7; GC/3.0a3 and 3.0a5 measure the same, GC/2.7 is worse).
 * NAMES. `PlayRecordCallback`, `PlayRecord*` words, `__OSStartPlayRecord`, `__OSStopPlayRecord` are the map's names;
 *    `PlayRecordAlarmHandler`, `PlayRecordPath`, `PlayRecordWork`, `PlayRecordStartTime` are GUESSes (the handler
 *    re-enters the callback with a zero result; the path string, the 0x200-byte buffer and the 64-bit time the
 *    callback copies out of the record).
 * RESIDUALS. `__OSStartPlayRecord` schedules `li r3,0` one slot early under the GC compiler (Wii/1.3 gives it 100 %
 *    but no jump table); the callback and stop path take the record address as a fresh `addi rX,rBase,0` where ours
 *    uses the base register; stop path's 9/8 compare pair stays two compares in the target; `.sdata` pad word
 *    0x80793FCC (4 B of the claim, unreferenced) is not emitted; flipcheck: text bytes differ.
 * SHAPES. the callback is `switch (state)` plus a shared tail `switch (state)` reached by `break`; the record write
 *    (time, word sum, checksum) is one `static inline` helper used by the callback and by the stop path.
 */
#include "types.h"
#include "NAND/nand.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSPlayRecord.h"
#include "OS/OSTime.h"

/* States of the record machine. */
#define PLAYREC_IDLE 0
#define PLAYREC_OPEN 1
#define PLAYREC_READ 2
#define PLAYREC_SEEK 3
#define PLAYREC_WAIT 4
#define PLAYREC_WRITE 5
#define PLAYREC_CLOSE 6
#define PLAYREC_FAILED 7
#define PLAYREC_TIMEOUT 8
#define PLAYREC_STOPPED 9

#define PLAYREC_TIMER_CLOCK (OS_BUS_CLOCK >> 2)

/* size: 0x80 - the stored record: a word sum over the body and the time the title last ran. */
typedef struct PlayRecord {
    /* +0x00 */ u32 checksum;
    /* +0x04 */ u32 words[23];
    /* +0x60 */ s64 time;
    /* +0x68 */ u32 tail[6];
} PlayRecord; /* size: 0x80 */

/* size: 0x200 */
typedef struct PlayRecordBuffer {
    /* +0x000 */ PlayRecord record;
    /* +0x080 */ OSAlarm alarm;
    /* +0x0B0 */ NANDFileInfo file;
    /* +0x13C */ u8 commandBlock[0xBC]; /* a NANDCommandBlock, whose layout stays with the NAND band */
    /* +0x1F8 */ u8 pad_0x1F8[0x8];
} PlayRecordBuffer; /* size: 0x200 */

#define PLAYREC_BLOCK ((NANDCommandBlock*)PlayRecordWork.commandBlock)
#define PLAYREC_BLOCK_OF(w) ((NANDCommandBlock*)(w)->commandBlock)

static void PlayRecordCallback(s32 result, NANDCommandBlock* block);

PlayRecordBuffer PlayRecordWork;
char PlayRecordPath[] = "/title/00000001/00000002/data/play_rec.dat";

s64 PlayRecordStartTime;
static s32 PlayRecordLastError;
static s32 PlayRecordRetry;
static s32 PlayRecordTerminated;
static s32 PlayRecordTerminate;
static s32 PlayRecordError;
static s32 PlayRecordGet;

static s32 PlayRecordState = PLAYREC_STOPPED;

/* 0x804D5E00 (0xC): re-enters the record machine with a zero result when the retry alarm fires. */
void PlayRecordAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    PlayRecordCallback(0, NULL);
}

static inline void PlayRecordSeal(PlayRecord* record)
{
    u32* p;
    u32 sum;
    s32 count;

    record->time = OSGetTime();
    sum = 0;
    p = record->words;
    count = 31;
    while (count-- > 0) {
        sum += *p++;
    }
    record->checksum = sum;
}

/* 0x804D5E10 (0x4B4): one step of the open, read, wait, write and close sequence; every NAND command completes
 * back into it. */
static void PlayRecordCallback(s32 result, NANDCommandBlock* block)
{
    PlayRecordBuffer* work = &PlayRecordWork;
    PlayRecord* record = &work->record;
    s32 error = 0;

    PlayRecordLastError = result;
    if (PlayRecordTerminate != 0) {
        PlayRecordTerminated = 1;
        return;
    }
    if (PlayRecordRetry == 0) {
        switch ((u32)PlayRecordState) {
        case PLAYREC_IDLE:
            PlayRecordState = PLAYREC_OPEN;
            break;
        case PLAYREC_OPEN:
            if (result == -10) {
                PlayRecordRetry = 1;
                OSCreateAlarm(&work->alarm);
                OSSetAlarm(&work->alarm, PLAYREC_TIMER_CLOCK, PlayRecordAlarmHandler);
                return;
            }
            if (result == 0) {
                if (PlayRecordGet == 0) {
                    PlayRecordState = PLAYREC_READ;
                } else {
                    PlayRecordState = PLAYREC_WAIT;
                }
                break;
            }
            PlayRecordError = 1;
            PlayRecordState = PLAYREC_FAILED;
            return;
        case PLAYREC_READ:
            if ((u32)result == sizeof(PlayRecord)) {
                PlayRecordGet = 1;
                PlayRecordStartTime = record->time;
                PlayRecordState = PLAYREC_SEEK;
            } else {
                PlayRecordError = 1;
                PlayRecordState = PLAYREC_CLOSE;
            }
            break;
        case PLAYREC_SEEK:
            if (result == 0) {
                PlayRecordState = PLAYREC_WAIT;
            } else {
                PlayRecordError = 1;
                PlayRecordState = PLAYREC_CLOSE;
            }
            break;
        case PLAYREC_WAIT:
            PlayRecordState = PLAYREC_WRITE;
            break;
        case PLAYREC_WRITE:
            if ((u32)result == sizeof(PlayRecord)) {
                if (OSGetTime() - PlayRecordStartTime > (s64)PLAYREC_TIMER_CLOCK * 300) {
                    PlayRecordState = PLAYREC_CLOSE;
                } else {
                    PlayRecordState = PLAYREC_SEEK;
                }
            } else {
                PlayRecordError = 1;
                PlayRecordState = PLAYREC_CLOSE;
            }
            break;
        case PLAYREC_CLOSE:
            if (PlayRecordError != 0) {
                PlayRecordState = PLAYREC_FAILED;
                return;
            }
            if (result == 0) {
                PlayRecordStartTime = record->time;
                PlayRecordState = PLAYREC_OPEN;
                break;
            }
            PlayRecordState = PLAYREC_FAILED;
            PlayRecordError = 1;
            return;
        default:
            PlayRecordState = PLAYREC_FAILED;
            PlayRecordError = 1;
            return;
        }
    }

    PlayRecordRetry = 0;
    switch (PlayRecordState) {
    case PLAYREC_OPEN:
        error = NANDOpenAsync(PlayRecordPath, &work->file, 3, PlayRecordCallback, PLAYREC_BLOCK_OF(work));
        break;
    case PLAYREC_READ:
        error = NANDReadAsync(&work->file, record, sizeof(PlayRecord), PlayRecordCallback,
                              PLAYREC_BLOCK_OF(work));
        break;
    case PLAYREC_SEEK:
        error = NANDSeekAsync(&work->file, 0, 0, PlayRecordCallback, PLAYREC_BLOCK_OF(work));
        break;
    case PLAYREC_WAIT:
        OSCreateAlarm(&work->alarm);
        OSSetAlarm(&work->alarm, (s64)PLAYREC_TIMER_CLOCK * 60, PlayRecordAlarmHandler);
        break;
    case PLAYREC_WRITE:
        PlayRecordSeal(record);
        error = NANDWriteAsync(&work->file, record, sizeof(PlayRecord), PlayRecordCallback,
                               PLAYREC_BLOCK_OF(work));
        break;
    case PLAYREC_CLOSE:
        error = NANDCloseAsync(&work->file, PlayRecordCallback, PLAYREC_BLOCK_OF(work));
        break;
    }

    if (error != 0) {
        if (error == -3) {
            OSCreateAlarm(&work->alarm);
            OSSetAlarm(&work->alarm, PLAYREC_TIMER_CLOCK, PlayRecordAlarmHandler);
            PlayRecordRetry = 1;
        } else {
            PlayRecordError = 1;
            switch (PlayRecordState) {
            case PLAYREC_READ:
            case PLAYREC_SEEK:
            case PLAYREC_WRITE:
                PlayRecordState = PLAYREC_CLOSE;
                error = NANDCloseAsync(&work->file, PlayRecordCallback, PLAYREC_BLOCK_OF(work));
                if (error == -3) {
                    PlayRecordRetry = 1;
                    OSCreateAlarm(&work->alarm);
                    OSSetAlarm(&work->alarm, PLAYREC_TIMER_CLOCK, PlayRecordAlarmHandler);
                }
                break;
            default:
            case PLAYREC_WAIT:
                PlayRecordState = PLAYREC_FAILED;
                break;
            }
        }
    }
    PlayRecordLastError = error;
}

/* 0x804D62D0 (0x54): starts the record machine when the NAND filesystem comes up. */
void __OSStartPlayRecord(void)
{
    if (NANDInit() == 0) {
        PlayRecordTerminate = 0;
        PlayRecordGet = 0;
        PlayRecordState = PLAYREC_IDLE;
        PlayRecordError = 0;
        PlayRecordRetry = 0;
        PlayRecordTerminated = 0;
        PlayRecordLastError = 0;
        PlayRecordCallback(0, NULL);
    }
}

/* 0x804D6330 (0x1EC): stops the record machine, writing the record out itself when the machine is waiting. */
void __OSStopPlayRecord(void)
{
    PlayRecordBuffer* work = &PlayRecordWork;
    BOOL enabled = OSDisableInterrupts();
    s32 state;

    PlayRecordTerminate = 1;
    state = PlayRecordState;
    if (state == PLAYREC_FAILED || state == PLAYREC_IDLE || state == PLAYREC_STOPPED || state == PLAYREC_TIMEOUT) {
        OSRestoreInterrupts(enabled);
    } else if (state == PLAYREC_WAIT) {
        OSCancelAlarm(&work->alarm);
        OSRestoreInterrupts(enabled);
        PlayRecordSeal(&work->record);
        NANDWrite(&work->file, &work->record, sizeof(PlayRecord));
        NANDClose(&work->file);
    } else {
        if (PlayRecordRetry != 0) {
            OSCancelAlarm(&work->alarm);
            OSRestoreInterrupts(enabled);
        } else {
            s64 start;

            OSRestoreInterrupts(enabled);
            start = OSGetTime();
            for (;;) {
                if (PlayRecordTerminated != 0) {
                    break;
                }
                if (OSGetTime() - start > (PLAYREC_TIMER_CLOCK / 1000) * 500) {
                    PlayRecordState = PLAYREC_TIMEOUT;
                    return;
                }
            }
        }
        switch (PlayRecordState) {
        case PLAYREC_WAIT:
            break;
        case PLAYREC_READ:
        case PLAYREC_SEEK:
        case PLAYREC_WRITE:
            NANDClose(&work->file);
            break;
        case PLAYREC_OPEN:
            if (PlayRecordLastError == 0 && PlayRecordRetry == 0) {
                NANDClose(&work->file);
            }
            break;
        case PLAYREC_CLOSE:
            if (PlayRecordRetry != 0) {
                NANDClose(&work->file);
            }
            break;
        }
    }
    PlayRecordState = PLAYREC_STOPPED;
}
