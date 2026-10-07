/*
 * OS/OSLaunch.c - the OS title launcher: argument packing, title loading, `__OSGetValidTicketIndex`,
 *    `__OSRelaunchTitle` and the Shop Channel help launch.
 * RANGE. .text 0x804D71F0-0x804D7FF0 (8 functions); .data 0x80629818-0x80629B38; .sdata 0x80793FD8-0x80793FE0; .sbss
 *    0x80795408-0x80795410.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor
 *    "OSLaunch.c" at .data 0x80629874 (read by `__OSGetValidTicketIndex`) and the "/title/%08x/%08x/data" string
 *    (0x80629818) read by both `fn_804D7370` and `fn_804D7A80` (one string pool); the argument-packing helper
 *    `fn_804D71F0` has a twin at 0x804CDD70.
 * FLAGS. `cflags_base` (configure.py), the default of the OS band; the compiler version is not settled (the other bodies
 *    decide it).
 * NAMES. `OSLaunchPackArgs` is a GUESS (it packs argv into the argument page; twin of `OSExecPackArgs`).
 *    `__OSGetValidTicketIndex`, `__OSRelaunchTitle`, `OSLaunchShopChannelHelp` (a GUESS in the scheme of the
 *    `OSLaunchPDChannel` log text) keep their names.
 * RESIDUALS. unwritten (7 functions): 0x804D7370-0x804D7FE8 (`__OSGetValidTicketIndex`,
 *    `__OSRelaunchTitle`, `OSLaunchShopChannelHelp` and four unnamed helpers); `OSLaunchPackArgs` differs only in the
 *    register numbering of its locals (page/argv swapped); .data, .sdata and .sbss are not emitted yet.
 * SHAPES. the argument packer walks `argv` from the end with `while (arg--, --argc >= 0)` and keeps the count in a copy.
 */

#include "types.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"

/* size: 0x2000 - the argument page the launched title reads: a header word at +0x08 and the packed strings. */
typedef struct OSLaunchArgPage {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 argsOffset; /* offset of the argc word, zero when there are no arguments */
    /* +0x0C */ u8 data[0x1FF4];
} OSLaunchArgPage; /* size: 0x2000 */

/* 0x804D71F0 (0x180): packs `argc` strings of `argv` at the end of `page`, followed by the argv table and argc. */
BOOL OSLaunchPackArgs(OSLaunchArgPage* page, s32 argc, char** argv)
{
    u8* base = (u8*)page;
    u32* table;
    char** arg;
    char* str;
    u8* cursor;
    u32 count = argc;
    u32 j;

    memset(page, 0, sizeof(OSLaunchArgPage));
    if (argc == 0) {
        page->argsOffset = 0;
    } else {
        cursor = base + sizeof(OSLaunchArgPage);
        arg = argv + argc;
        while (arg--, --argc >= 0) {
            str = *arg;
            cursor -= strlen(str) + 1;
            strcpy((char*)cursor, str);
            *arg = (char*)(cursor - base);
        }
        table = (u32*)(base + (((u32)(cursor - base)) & ~3)) - (count + 1);
        for (j = 0; j < count + 1; j++) {
            table[j] = (u32)argv[j];
        }
        table[-1] = count;
        page->argsOffset = (u32)(table - 1) - (u32)base;
    }
    return TRUE;
}
