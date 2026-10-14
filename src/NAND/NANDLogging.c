/*
 * NAND/NANDLogging.c - the Revolution SDK NAND library's error log: appends formatted messages to the log file
 *    through an asynchronous ISFS state machine.
 * RANGE. .text 0x804C9910-0x804C9F00 (3 functions); .data 0x8061BBD8-0x8061BC20 (the log path and the line format);
 *    .bss 0x8074CF80-0x8074D280 (the message, read and line buffers); .sdata 0x80793F48-0x80793F50 (`s_fd`); .sbss
 *    0x807952C0-0x807952C8 (`s_callback`, `s_stage`).  Cut from the NAND library 0x804C70E0-0x804C9F00.  Seam with
 *    NAND/NANDCore.c in .sbss: 0x807952BC is a 4-byte hole no code references, the padding before this unit's
 *    8-aligned start.  Seam with NAND/NANDCheck.c: data order only (no shared statics).
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no float code.  `#pragma dont_inline` covers
 *    `reserveFileDescriptor` (the target calls it out of line).  The three buffers are 32-byte aligned (ISFS DMA).
 * NAMES. `reserveFileDescriptor`, `NANDLoggingAddMessageAsync` and `asyncRoutine` are the map's (dump).  GUESS: the
 *    unit name (SDK spelling), `s_fd`, `s_readBuffer` and `s_writeBuffer` (`asyncRoutine`'s buffers after
 *    `s_message`), and the `NAND_LOG_STAGE_*` step names.
 * RESIDUALS. `asyncRoutine`: in the header-write step the zero stored to `s_readBuffer[255]` is kept in r29 across
 *    the calls and reused for the title-id terminator, which costs a third saved register; the target rematerializes
 *    the zero (`li r0,0` then `li r4,0`).
 * SHAPES. the step dispatch is an `if`/`else if` chain on `s_stage` after the increment (the target compares the
 *    incremented value in r0 against 2..9 in order); the line-seek step reads the line count into a local first (the
 *    target copies the `atoi` result to r0 before the shift).
 */

#include "types.h"
#include "FS/fs.h"
#include "MSL_C/alloc.h"
#include "MSL_C/printf.h"
#include "OS/OSInterrupt.h"
#include "OS/OSTime.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "NAND/NANDCore.h"
#include "NAND/NANDLogging.h"

#define NAND_FILE_SLOT_FREE (-255)
#define NAND_FILE_SLOT_TAKEN (-254)
#define NAND_LOG_LINE_SIZE 256
#define NAND_LOG_LINE_COUNT 63
#define NAND_LOG_STAGE_OPEN 1
#define NAND_LOG_STAGE_SEEK_HEADER 2
#define NAND_LOG_STAGE_READ_HEADER 3
#define NAND_LOG_STAGE_REWIND 4
#define NAND_LOG_STAGE_WRITE_HEADER 5
#define NAND_LOG_STAGE_SEEK_LINE 6
#define NAND_LOG_STAGE_WRITE_LINE 7
#define NAND_LOG_STAGE_CLOSE 8
#define NAND_LOG_STAGE_DONE 9
#define ISFS_MODE_READ_WRITE 3
#define ISFS_SEEK_START 0

static char s_message[NAND_LOG_LINE_SIZE] __attribute__((aligned(32)));
static char s_readBuffer[NAND_LOG_LINE_SIZE] __attribute__((aligned(32)));
static char s_writeBuffer[NAND_LOG_LINE_SIZE] __attribute__((aligned(32)));
static s32 s_fd = NAND_FILE_SLOT_FREE;
static s32 s_stage;
static NANDLoggingCallback s_callback;

static void asyncRoutine(s32 result, NANDCommandBlock* block);

#pragma dont_inline on

/* 0x804C9910 (0x54): takes the file descriptor slot; FALSE when it is already taken. */
static BOOL reserveFileDescriptor(void)
{
    BOOL level;
    BOOL taken;

    level = OSDisableInterrupts();
    if (s_fd == NAND_FILE_SLOT_FREE) {
        s_fd = NAND_FILE_SLOT_TAKEN;
        taken = FALSE;
    } else {
        taken = TRUE;
    }
    OSRestoreInterrupts(level);
    return taken == FALSE;
}

#pragma dont_inline reset

/* 0x804C9970 (0xF8): formats the message and starts appending it to the log file; FALSE when the log is busy. */
BOOL NANDLoggingAddMessageAsync(NANDLoggingCallback callback, const char* format, ...)
{
    va_list args;

    if (!reserveFileDescriptor()) {
        return FALSE;
    }
    va_start(args, format);
    vsnprintf(s_message, NAND_LOG_LINE_SIZE, format, args);
    va_end(args);
    s_callback = callback;
    s_stage = NAND_LOG_STAGE_OPEN;
    if (ISFS_OpenAsync("/shared2/test2/nanderr.log", ISFS_MODE_READ_WRITE, asyncRoutine, NULL) == 0) {
        return TRUE;
    }
    return FALSE;
}

/* 0x804C9A70 (0x484): steps the log append: read the line counter, stamp and write the new line, then close. */
static void asyncRoutine(s32 result, NANDCommandBlock* block)
{
    s_stage++;
    if (s_stage == NAND_LOG_STAGE_SEEK_HEADER) {
        if (result >= 0) {
            s_fd = result;
            if (ISFS_SeekAsync(result, 0, ISFS_SEEK_START, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_READ_HEADER) {
        if (result == 0) {
            if (ISFS_ReadAsync(s_fd, s_readBuffer, NAND_LOG_LINE_SIZE, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_REWIND) {
        if (result == NAND_LOG_LINE_SIZE) {
            if (ISFS_SeekAsync(s_fd, 0, ISFS_SEEK_START, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_WRITE_HEADER) {
        if (result == 0) {
            OSCalendarTime time;
            char titleId[64];
            s32 length;
            s32 lineCount;

            s_readBuffer[NAND_LOG_LINE_SIZE - 1] = '\0';
            lineCount = atoi(s_readBuffer);
            memset(s_writeBuffer, ' ', NAND_LOG_LINE_SIZE - 2);
            OSTicksToCalendarTime(OSGetTime(), &time);
            strncpy(titleId, nandGetHomeDir() + 7, 17);
            titleId[8] = '-';
            titleId[17] = '\0';
            length = snprintf(s_writeBuffer, NAND_LOG_LINE_SIZE, "%d %04d/%02d/%02d %02d:%02d:%02d %s %s",
                              lineCount % NAND_LOG_LINE_COUNT + 1, time.year, time.mon + 1, time.mday, time.hour,
                              time.min, time.sec, titleId, s_message);
            if (length < NAND_LOG_LINE_SIZE) {
                s_writeBuffer[length] = ' ';
            }
            s_writeBuffer[NAND_LOG_LINE_SIZE - 2] = '\r';
            s_writeBuffer[NAND_LOG_LINE_SIZE - 1] = '\n';
            if (ISFS_WriteAsync(s_fd, s_writeBuffer, NAND_LOG_LINE_SIZE, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_SEEK_LINE) {
        if (result == NAND_LOG_LINE_SIZE) {
            s32 lineCount = atoi(s_readBuffer);

            if (ISFS_SeekAsync(s_fd, lineCount * NAND_LOG_LINE_SIZE, ISFS_SEEK_START, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_WRITE_LINE) {
        if (result == atoi(s_readBuffer) * NAND_LOG_LINE_SIZE) {
            if (ISFS_WriteAsync(s_fd, s_writeBuffer, NAND_LOG_LINE_SIZE, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_CLOSE) {
        if (result == NAND_LOG_LINE_SIZE) {
            if (ISFS_CloseAsync(s_fd, asyncRoutine, NULL) != 0) {
                if (s_callback) {
                    s_callback(FALSE);
                }
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    } else if (s_stage == NAND_LOG_STAGE_DONE) {
        if (result == 0) {
            s_fd = NAND_FILE_SLOT_FREE;
            if (s_callback) {
                s_callback(TRUE);
            }
        } else if (s_callback) {
            s_callback(FALSE);
        }
    }
}
