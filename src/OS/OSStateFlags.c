/*
 * OS/OSStateFlags.c - the OS state flags: `__OSWriteStateFlags` and `__OSReadStateFlags`.
 * RANGE. .text 0x804D6520-0x804D6740 (2 functions); .data 0x80629628-0x80629650; .bss 0x8074E360-0x8074E380.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the state.dat path (.data 0x80629628) and the
 *    0x20-byte buffer (.bss 0x8074E360) are read only by these two functions.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names; `StateFlagsBuffer` is a GUESS (the 0x20-byte staging record both functions fill); `NANDOpen`, `NANDRead`
 *    and `NANDDelete` replace the NAND band's placeholders (each sits beside its `...Async` twin).
 * RESIDUALS. both rows: the target sums the seven words with a plain load/add pair per word where the unrolled loop hoists two loads
 *    ahead (`__OSReadStateFlags`); `__OSWriteStateFlags` returns through `beq`/`li` where the compiler folds the close result into
 *    `cntlzw`/`srwi` (the same fold the NANDBOOTINFO writer's target does contain). .data: the path literal is emitted once.
 * SHAPES. plain C; the checksum is the word sum of the record after its first word.
 */

#include "types.h"

#include "NAND/nand.h"
#include "OS/OSStateFlags.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#define STATE_FLAGS_PATH "/title/00000001/00000002/data/state.dat"

static OSStateFlags StateFlagsBuffer;

/* Stores the record with its checksum into the state file. */
BOOL __OSWriteStateFlags(OSStateFlags* flags)
{
    NANDFileInfo file;
    u32 sum;
    u32 i;

    memcpy(&StateFlagsBuffer, flags, sizeof(OSStateFlags));
    sum = 0;
    for (i = 1; i < 8; i++) {
        sum += StateFlagsBuffer.words[i];
    }
    StateFlagsBuffer.checksum = sum;
    if (NANDOpen(STATE_FLAGS_PATH, &file, 2) == 0) {
        if (NANDWrite(&file, &StateFlagsBuffer, sizeof(OSStateFlags)) != sizeof(OSStateFlags)) {
            NANDClose(&file);
            return FALSE;
        }
        if (NANDClose(&file) != 0) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

/* Loads the record from the state file; clears `flags` and returns FALSE when the file is missing, short or corrupt. */
BOOL __OSReadStateFlags(OSStateFlags* flags)
{
    NANDFileInfo file;
    u32 sum;
    u32 i;
    s32 length;

    if (NANDOpen(STATE_FLAGS_PATH, &file, 1) == 0) {
        length = NANDRead(&file, &StateFlagsBuffer, sizeof(OSStateFlags));
        NANDClose(&file);
        if (length != sizeof(OSStateFlags)) {
            NANDDelete(STATE_FLAGS_PATH);
            memset(flags, 0, sizeof(OSStateFlags));
            return FALSE;
        }
    } else {
        memset(flags, 0, sizeof(OSStateFlags));
        return FALSE;
    }
    sum = 0;
    for (i = 1; i < 8; i++) {
        sum += StateFlagsBuffer.words[i];
    }
    if (StateFlagsBuffer.checksum != sum) {
        memset(flags, 0, sizeof(OSStateFlags));
        return FALSE;
    }
    memcpy(flags, &StateFlagsBuffer, sizeof(OSStateFlags));
    return TRUE;
}
