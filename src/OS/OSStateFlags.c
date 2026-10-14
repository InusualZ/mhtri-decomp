/*
 * OS/OSStateFlags.c - the OS state flags: `__OSWriteStateFlags` and `__OSReadStateFlags`.
 * RANGE. .text 0x804D6520-0x804D6740 (2 functions); .data 0x80629628-0x80629650; .bss 0x8074E360-0x8074E380.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the state.dat path (.data 0x80629628) and the
 *    0x20-byte buffer (.bss 0x8074E360) are read only by these two functions.
 * FLAGS. `cflags_base` per object (configure.py); both functions sit under a scoped `#pragma global_optimizer off`
 *    (playbook 120): with it on, the checksum loop hoists two loads ahead and the close result folds into
 *    `cntlzw`/`srwi`, where the target keeps a load/add pair per word and a `beq`/`li` return.
 * NAMES. map names; `StateFlagsBuffer` is a GUESS (the 0x20-byte staging record both functions fill); `NANDOpen`, `NANDRead`
 *    and `NANDDelete` replace the NAND band's placeholders (each sits beside its `...Async` twin).
 * RESIDUALS. none in code; the path literal is the pooled `@NNN` label where the map names it `lbl_80629628`
 *    (link-neutral).  The object's .text ends 8 bytes before the claimed end (alignment fill).
 * SHAPES. plain C; the checksum is the word sum of the record after its first word; `__OSWriteStateFlags` returns FALSE
 *    from the failed-open `else` and TRUE from the common tail; `StateFlagsBuffer` is global (map binding).
 */

#include "types.h"

#include "NAND/nand.h"
#include "OS/OSStateFlags.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#define STATE_FLAGS_PATH "/title/00000001/00000002/data/state.dat"

OSStateFlags StateFlagsBuffer;

#pragma push
#pragma global_optimizer off

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
    } else {
        return FALSE;
    }
    return TRUE;
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

#pragma pop
