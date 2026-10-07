/*
 * OS/OSExec.c - the OS program launcher: argument packing, the exec parameters, `__OSLaunchNextFirmware`,
 *    `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`.
 * RANGE. .text 0x804CDD70-0x804CF350 (11 functions); .data 0x8061C850-0x8061C8C0; .bss 0x8074D340-0x8074D360; .sdata
 *    0x80793F98-0x80793FA8; .sbss 0x80795330-0x80795348.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: .data 0x8061C850 ("OSExec(): Failed to exec") and 0x8061C8B0 (the apploader date) are read by
 *    `__OSLaunchNextFirmware` and `__OSBootDolSimple`; the first four functions are argument-packing helpers
 *    (`strlen`/`strcpy`/`memset`) whose twin heads the OSLaunch unit at 0x804D71F0.
 * FLAGS. `cflags_base` (configure.py), the default of the OS band.
 * NAMES. `OSExecPackArgs`, `OSExecJump`, `OSExecSetReady`, `OSExecSetState`, `OSExecReady`, `OSExecState`, `OSExecParams`,
 *    `OSExecWideToHex`, `OSExecPackArgsWide` are GUESSes (the argument packer twins `OSLaunchPackArgs`; the jump flushes the icache and branches; the two
 *    setters store one `.sbss` word each).  `__OSLaunchNextFirmware`, `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`, `__OSGetExecParams` are the
 *    map's names; the left edge (0x804CDD70) is medium evidence (no data read by the four helpers).
 * RESIDUALS. not attempted: `__OSLaunchNextFirmware` (0x804CE2B0, 0x684 B) and `__OSBootDolSimple` (0x804CEA10, 0x754 B); .bss,
 *    .data, .sdata and the rest of .sbss are not emitted yet.  `OSExecWideToHex` (82.7 %): the target keeps the constant 4 in a
 *    register for its `4 & ~mask` shift and reloads the byte after the nibble test; `OSExecPackArgs`/`OSExecPackArgsWide`
 *    register numbering of locals; `__OSLaunchMenu` (95.8 %): the target branches `beq`+`b` over the result test where the source
 *    gives one `bne`; `OSExecJump` swaps two epilogue loads after the `bctr`.
 * SHAPES. `OSExecJump` flushes the instruction cache and branches with inline asm.
 */

#include "types.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"
#include "MSL_C/strstr.h"
#include "ESP/esp.h"
#include "OS/OSArena.h"
#include "OS/OSCache.h"
#include "OS/OSExec.h"
#include "OS/OSReboot.h"
#include "MSL_C/printf.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The exec parameter block's address in low memory, valid once it points into MEM1. */
#define OS_EXEC_PARAMS_ADDR (*(u32*)0x800030F0)

/* size: 0x2000 - the argument page the launched title reads. */
typedef struct OSExecArgPage {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 argsOffset; /* offset of the argc word, zero when there are no arguments */
    /* +0x0C */ u8 data[0x1FF4];
} OSExecArgPage; /* size: 0x2000 */

/* Defined by this unit (0x804CEA10); boots the DOL image at `dolOffset`. */
void __OSBootDolSimple(u32 dolOffset, u32 resetCode, u8* saveStart, u8* saveEnd, u32 flags, s32 argc, const char** argv);

u32 OSExecReady;
u32 OSExecState;

/* 0x804CDD70 (0x180): packs `argc` strings of `argv` at the end of `page`, followed by the argv table and argc. */
BOOL OSExecPackArgs(OSExecArgPage* page, s32 argc, char** argv)
{
    u8* base = (u8*)page;
    u32* table;
    char** arg;
    char* str;
    u8* cursor;
    u32 count = argc;
    u32 j;

    memset(page, 0, sizeof(OSExecArgPage));
    if (argc == 0) {
        page->argsOffset = 0;
    } else {
        cursor = base + sizeof(OSExecArgPage);
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

/* 0x804CE220 (0x3C): flushes the instruction cache and jumps to `entry`. */
void OSExecJump(register u32 entry)
{
    ICFlashInvalidate();
    asm {
        sync
        isync
        mtctr entry
        bctr
    }
}

/* 0x804CE260 (0xC): marks the exec state ready. */
void OSExecSetReady(void)
{
    OSExecReady = 1;
}

/* 0x804CE270 (0x24): copies the exec parameter block out of low memory, or clears its valid word when none was left. */
void __OSGetExecParams(OSExecParams* params)
{
    u32 addr = OS_EXEC_PARAMS_ADDR;

    if (addr >= 0x80000000) {
        memcpy(params, (void*)addr, sizeof(OSExecParams));
    } else {
        params->valid = 0;
    }
}

/* 0x804CE2A0 (0x8): stores the exec state word. */
void OSExecSetState(u32 state)
{
    OSExecState = state;
}

/* 0x804CDEF0 (0x158): writes the wide string `src` to `dst` as lower-case hex digits, four per character; returns FALSE on a bad digit. */
BOOL OSExecWideToHex(char* dst, const char* src)
{
    s32 nibble;
    s32 odd;
    s32 mask;
    s32 digit;
    u8 shift;

    if (src != NULL) {
        while (src[0] != 0 || src[1] != 0) {
            for (nibble = 0; nibble < 4; nibble++) {
                odd = nibble & 1;
                mask = odd ? 0x0F : 0xF0;
                shift = 4 & ~((-odd | odd) >> 31);
                digit = (*src & mask) >> shift;
                if (digit >= 0 && 10 > digit) {
                    *dst = (char)(digit + '0');
                } else if (digit >= 10 && 16 > digit) {
                    *dst = (char)(digit + 'a' - 10);
                } else {
                    return FALSE;
                }
                dst++;
                if (odd) {
                    src++;
                }
            }
        }
        *dst = 0;
        return TRUE;
    }
    return FALSE;
}

/* 0x804CE050 (0x1CC): packs `argc` arguments of `argv` at the end of `page`; every other argument past the second is a wide string stored as hex. */
BOOL OSExecPackArgsWide(OSExecArgPage* page, s32 argc, char** argv)
{
    u8* base = (u8*)page;
    u32* table;
    char** arg;
    char* str;
    u8* cursor;
    u32 count = argc;
    u32 j;

    memset(page, 0, sizeof(OSExecArgPage));
    if (argc == 0) {
        page->argsOffset = 0;
    } else {
        cursor = base + sizeof(OSExecArgPage);
        arg = argv + argc;
        while (arg--, --argc >= 0) {
            if (argc < 2 || argc % 2 != 0) {
                str = *arg;
                cursor -= strlen(str) + 1;
                strcpy((char*)cursor, str);
                *arg = (char*)(cursor - base);
            } else {
                str = *arg;
                cursor -= wcslen((const u16*)str) * 4 + 1;
                OSExecWideToHex((char*)cursor, str);
                *arg = (char*)(cursor - base);
            }
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

/* 0x804CE940 (0xC8): launches the system menu (title 1-2) with its single ticket view and returns only on failure. */
void __OSLaunchMenu(void)
{
    u32 viewCount = 1;
    ESTicketView* views;
    s32 result;

    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    if (ESP_InitLib() == 0) {
        result = ESP_GetTicketViews(0x0000000100000002ULL, NULL, &viewCount);
        if (viewCount == 1) {
            if (result != 0) {
                return;
            }
            views = (ESTicketView*)OSAllocFromMEM1ArenaLo((viewCount * sizeof(ESTicketView) + 31) & ~31, 32);
            if (ESP_GetTicketViews(0x0000000100000002ULL, views, &viewCount) == 0) {
                if (ESP_LaunchTitle(0x0000000100000002ULL, views) == 0) {
                    for (;;) {
                    }
                }
            }
        }
    }
}

/* 0x804CF170 (0x1E0): boots the DOL at `bootDol` with its offset as the first argument followed by `argv`. */
void __OSBootDol(u32 bootDol, u32 resetCode, const char** argv)
{
    void* saveStart;
    void* saveEnd;
    char name[32];
    const char** walk;
    const char** table;
    s32 argc;
    s32 i;

    OSGetSaveRegion(&saveStart, &saveEnd);
    sprintf(name, "%08x", bootDol);
    argc = 0;
    if (argv != NULL) {
        walk = argv;
        while (*walk != NULL) {
            walk++;
            argc++;
        }
    }
    table = (const char**)OSAllocFromMEM1ArenaLo((argc + 2) * sizeof(char*), 1);
    table[0] = name;
    for (i = 1; i < argc + 1; i++) {
        table[i] = argv[i - 1];
    }
    __OSBootDolSimple(-1, resetCode, saveStart, saveEnd, 0, argc + 1, table);
}
