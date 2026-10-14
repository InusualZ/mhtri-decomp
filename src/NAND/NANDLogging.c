/*
 * NAND/NANDLogging.c - the Revolution SDK NAND library's error log: appends formatted messages to the log file
 *    through an asynchronous ISFS state machine.
 * RANGE. .text 0x804C9910-0x804C9F00 (3 functions); .data 0x8061BBD8-0x8061BC20; .bss 0x8074CF80-0x8074D280 (the
 *    message, read and line buffers); .sdata 0x80793F48-0x80793F50 (`s_fd`); .sbss 0x807952C0-0x807952C8
 *    (`s_callback`, `s_stage`).  Cut from the NAND library 0x804C70E0-0x804C9F00.  Seam with NAND/NANDCore.c in .sbss:
 *    0x807952BC is a 4-byte hole no code references, the padding before this unit's 8-aligned start.  Seam with
 *    NAND/NANDCheck.c: data order only (no shared statics).
 * FLAGS. cflags_base in the `OS` lib block of configure.py (Wii/1.3); no pragma; no float code.
 * NAMES. `reserveFileDescriptor`, `NANDLoggingAddMessageAsync` and `asyncRoutine` are the map's (dump).  GUESS: the
 *    unit name (SDK spelling), `s_fd`, `s_readBuffer` and `s_writeBuffer` (`asyncRoutine`'s buffers after
 *    `s_message`).
 * RESIDUALS. written (1 function at 100 %): `reserveFileDescriptor`.  Not attempted (2 functions):
 *    `NANDLoggingAddMessageAsync` and `asyncRoutine`; the .data, .bss and .sbss objects are not emitted.
 * SHAPES. none yet.
 */

#include "types.h"
#include "OS/OSInterrupt.h"
#include "NAND/NANDLogging.h"

#define NAND_FILE_SLOT_FREE (-255)
#define NAND_FILE_SLOT_TAKEN (-254)

static s32 s_fd = NAND_FILE_SLOT_FREE;

/* 0x804C9910 (0x54): takes the file descriptor slot; FALSE when it is already taken. */
BOOL reserveFileDescriptor(void)
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
