/*
 * OS/OSNandbootInfo.c - the OS NAND boot info: `__OSCreateNandbootInfo` and `__OSWriteNandbootInfo`.
 * RANGE. .text 0x804D6800-0x804D6A10 (2 functions); .data 0x806297B8-0x806297D8.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the NANDBOOTINFO path (.data 0x806297B8) is read only by these two
 *    functions.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names throughout; `NANDPrivateGetStatus`, `NANDPrivateDelete` and `NANDWrite` replace the placeholders of the
 *    NAND band (each sits beside its `...Async` twin); `OSNandbootInfo` is a GUESS (the 0x1020-byte file the two functions
 *    sum and write).
 * RESIDUALS. none recorded yet.
 * SHAPES. plain C; the checksum is a word loop the compiler unrolls.
 */

#include "types.h"

#include "NAND/nand.h"
#include "OS/OSNandbootInfo.h"

#define NANDBOOTINFO_PATH "/shared2/sys/NANDBOOTINFO"
#define NANDBOOTINFO_PERMISSION 0x3F

/* Creates the NANDBOOTINFO file when it is missing or has the wrong permission. */
BOOL __OSCreateNandbootInfo(void)
{
    NANDStatus status;
    s32 result;

    result = NANDPrivateGetStatus(NANDBOOTINFO_PATH, &status);
    if (result == 0 && status.permission == NANDBOOTINFO_PERMISSION) {
        return TRUE;
    }
    if (result == 0 && status.permission != NANDBOOTINFO_PERMISSION) {
        if (NANDPrivateDelete(NANDBOOTINFO_PATH) != 0) {
            return FALSE;
        }
    } else if (result != -12) {
        return FALSE;
    }
    if (NANDPrivateCreate(NANDBOOTINFO_PATH, NANDBOOTINFO_PERMISSION, 0) != 0) {
        return FALSE;
    }
    return TRUE;
}

/* Sums the record, stores the sum in its first word and writes the whole record to the NANDBOOTINFO file. */
BOOL __OSWriteNandbootInfo(OSNandbootInfo* info)
{
    NANDFileInfo file;
    u32 sum;
    u32* word;
    u32 i;

    sum = 0;
    word = info->payload;
    for (i = 0; i < 0x407; i++) {
        sum += *word++;
    }
    info->checksum = sum;
    if (NANDPrivateOpen(NANDBOOTINFO_PATH, &file, 2) == 0) {
        if (NANDWrite(&file, info, sizeof(OSNandbootInfo)) != sizeof(OSNandbootInfo)) {
            NANDClose(&file);
            return FALSE;
        }
        return NANDClose(&file) == 0;
    }
    return FALSE;
}
