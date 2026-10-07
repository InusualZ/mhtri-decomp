/*
 * OS/OSLaunch.c - the OS title launcher: argument packing, title loading, `__OSGetValidTicketIndex`,
 *    `__OSRelaunchTitle` and the Shop Channel help launch.
 * RANGE. .text 0x804D71F0-0x804D7FF0 (8 functions); .data 0x80629818-0x80629B38; .sdata 0x80793FD8-0x80793FE0; .sbss
 *    0x80795408-0x80795410.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor
 *    "OSLaunch.c" at .data 0x80629874 (read by `__OSGetValidTicketIndex`) and the "/title/%08x/%08x/data" string
 *    (0x80629818) read by both `OSCheckTitleMakerCode` and `OSLaunchTitleWithArgs` (one string pool); the
 *    argument-packing helper has a twin at 0x804CDD70.
 * FLAGS. `cflags_base` (configure.py), the default of the OS band; `#pragma dont_inline on` from `OSCheckTitleMakerCode`
 *    on (the target calls the earlier helpers; with it off they inline into `OSLaunchTitleWithArgs`).
 * NAMES. GUESS: `OSLaunchPackArgs` (twin of `OSExecPackArgs`), `OSCheckTitleMakerCode`, `OSTitleHasContent`,
 *    `OSLaunchTitleDirect`, `OSLaunchTitleWithArgs` (its log text says "OSLaunchTitle()"), `OSLaunchShopChannelHelp`,
 *    `OSLaunchSystemMode`, `NANDGetUsage` (calls `ISFS_GetUsage`); `ESTmdView` is a local record.  The foreign DVD function
 *    `DVDResetDriveSync` is a GUESS (a blocking drive-command wrapper that waits on a flag a callback sets).
 * RESIDUALS. `OSTitleHasContent` (84.7 %): the target loads the title id after the three count stores and holds
 *    it in r30/r31 swapped; `__OSRelaunchTitle` and `OSLaunchTitleWithArgs`: the target compares the allocation result
 *    with `li r0,0` + `cmplw` where the source compare gives `cmpwi`; `OSLaunchTitleWithArgs` also swaps r28/r29 (tmd
 *    view / check flag) and the argv copy loop; `OSLaunchPackArgs` register numbering of two locals; `OSLaunchShopChannelHelp`
 *    r30/r31 swap.  Flip blockers: .data holds 0x118 bytes of strings (0x118..0x31C: the `__OSLaunchTitlevForSystem`,
 *    `OSLaunchManualViewer` and `OSLaunchPDChannel` texts) that no function of the unit or the DOL references (code the
 *    linker dropped), .sdata/.sbss are emitted but unclaimed bytes differ, and .text claims 0x180 more than the object.
 * SHAPES. the argument packer walks `argv` from the end with `while (arg--, --argc >= 0)` and keeps the count in a copy;
 *    the buffers the ES calls fill are `aligned(32)` locals; the path buffers of `OSCheckTitleMakerCode` are 76 and 68 bytes
 *    (the frame size says so); the ticket-view buffer is one view rounded up to 32 bytes.
 */

#include "types.h"
#include "DVD/dvd.h"
#include "ESP/esp.h"
#include "MSL/strlen.h"
#include "MSL_C/printf.h"
#include "MSL_C/string.h"
#include "NAND/nand.h"
#include "OS/OS.h"
#include "OS/OSArena.h"
#include "OS/OSError.h"
#include "OS/OSExec.h"
#include "OS/OSNandbootInfo.h"
#include "OS/OSPlayTime.h"
#include "OS/OSReset.h"
#include "OS/OSStateFlags.h"
#include "OS/OSThread.h"
#include "SC/SCProductInfo.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x80795408 - non-zero when the launcher runs for the system; only its first word is read. */
static u32 OSLaunchSystemMode[2];

/* The head of the title metadata view `ESP_GetTmdView` fills; only the fields the launcher reads are named. size: 0x34 (cut at the last field) */
#pragma pack(push, 1)
typedef struct ESTmdView {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ u32 titleIdHi;
    /* +0x08 */ u32 titleIdLo;
    /* +0x0C */ u8 pad_0x0C[0x0E];
    /* +0x1A */ u32 region; /* 3 for every region, else the product game region */
    /* +0x1E */ u8 pad_0x1E[0x14];
    /* +0x32 */ u8 flags;   /* bit 0: the title needs no disc drive reset */
} ESTmdView;
#pragma pack(pop)

/* size: 0x2000 - the argument page the launched title reads: a header word at +0x08 and the packed strings; its tail is the
 * NAND boot info the next title boots from. */
typedef struct OSLaunchArgPage {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 argsOffset; /* offset of the argc word, zero when there are no arguments */
    /* +0x0C */ u8 data[0xFD4];
    /* +0xFE0 */ OSNandbootInfo bootInfo;
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

#pragma dont_inline on

/* 0x804D7370 (0x114): compares the maker code of the home directory with the one of the title's data directory (or of the disc). */
BOOL OSCheckTitleMakerCode(u64 titleId, BOOL useDisc)
{
    char homePath[76];
    char titlePath[68];
    NANDStatus status;
    u16 homeGroup;
    u16 titleGroup;

    if (NANDGetHomeDir(homePath) != 0) {
        return FALSE;
    }
    if (NANDGetStatus(homePath, &status) != 0) {
        return FALSE;
    }
    homeGroup = status.groupId;
    if (homeGroup == 2 && OSGetAppType() == 0x80 && (OSGetConsoleType() & 0xF0000000) == 0x10000000) {
        homeGroup = DVDGetCurrentDiskID()->makerCode;
    }
    if (useDisc) {
        titleGroup = DVDGetCurrentDiskID()->makerCode;
    } else {
        sprintf(titlePath, "/title/%08x/%08x/data", (u32)((titleId >> 32) & 0xFFFFFFFF), (u32)(titleId & 0xFFFFFFFF));
        if (NANDGetStatus(titlePath, &status) != 0) {
            return FALSE;
        }
        titleGroup = status.groupId;
    }
    return homeGroup == titleGroup;
}

/* 0x804D7490 (0xD4): checks that the title has one ticket and more than two files below its content directory. */
BOOL OSTitleHasContent(const ESTmdView* tmdView)
{
    u32 ticketCount = 0;
    u32 blockCount = 0;
    u32 inodeCount = 0;
    char path[96] __attribute__((aligned(32)));
    u64 titleId;

    titleId = ((u64)tmdView->titleIdHi << 32) | tmdView->titleIdLo;
    if (ESP_GetTicketViews(titleId, NULL, &ticketCount) != 0 || ticketCount != 1) {
        return FALSE;
    }
    sprintf(path, "/title/%08x/%08x/content", (u32)((titleId >> 32) & 0xFFFFFFFF), (u32)(titleId & 0xFFFFFFFF));
    if (NANDGetUsage(path, &blockCount, &inodeCount) != 0 || inodeCount <= 2) {
        return FALSE;
    }
    return TRUE;
}

/* 0x804D7570 (0x1E0): picks the ticket view to launch with: the first unlimited one, else the one with the most access bits or the longest time limit. */
s32 __OSGetValidTicketIndex(const ESTicketView* views, u32 count)
{
    const ESTicketView* view;
    u32 limitValue;
    s32 limitKind;
    u32 i;
    u32 bestIndex = 0;
    BOOL foundUnlimited = FALSE;
    u32 bestBits = 0;
    u32 bestTime = 0;
    s32 result;

    if (views == NULL) {
        OSReport("NULL pointer detected: line %d in %s\n", 407, "OSLaunch.c");
        return -1017;
    }
    view = views;
    for (i = 0; i < count; view++, i++) {
        result = __OSGetPlayTime((ESTicketView*)view, &limitKind, &limitValue);
        if (result > 0) {
            continue;
        }
        if (result != 0) {
            return result;
        }
        switch (limitKind) {
        case 0: {
            u32 mask;
            s32 bit;
            u32 bits = 0;

            if (!foundUnlimited) {
                bestIndex = i;
                foundUnlimited = TRUE;
            }
            mask = view->accessMaskLow | (view->accessMaskHigh << 8);
            for (bit = 0; bit < 16; bit++) {
                if (mask & (1 << bit)) {
                    bits++;
                }
            }
            if (bits > bestBits) {
                bestIndex = i;
                bestBits = bits;
            }
            break;
        }
        case 1:
            if (!foundUnlimited && limitValue > bestTime) {
                bestIndex = i;
                bestTime = limitValue;
            }
            break;
        }
    }
    if (!foundUnlimited && bestTime == 0) {
        return -1;
    }
    return bestIndex;
}

/* The ticket view buffer the launcher allocates: one view rounded up to 32 bytes. */
#define TICKET_VIEW_BUFFER_SIZE ((sizeof(ESTicketView) + 31) & ~31)

/* 0x804D7750 (0x204): relaunches the running title with `launchCode` as its entry code and never returns. */
void __OSRelaunchTitle(u32 launchCode)
{
    u32 viewCount;
    s32 limitKind;
    u32 limitValue;
    u64 titleId __attribute__((aligned(32)));
    ESTicketView* ticketView;
    ESTicketView* views;
    OSLaunchArgPage* page;
    OSNandbootInfo* info;
    OSStateFlags flags __attribute__((aligned(32)));
    s32 result;

    viewCount = 1;
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    if (ESP_InitLib() != 0) {
        __OSReturnToMenuForError();
    }
    if (ESP_GetTitleId(&titleId) != 0) {
        __OSReturnToMenuForError();
    }
    ticketView = (ESTicketView*)OSAllocFromMEM1ArenaLo(TICKET_VIEW_BUFFER_SIZE, 32);
    if (!ticketView) {
        __OSReturnToMenuForError();
    }
    memset(ticketView, 0, TICKET_VIEW_BUFFER_SIZE);
    result = ESP_DiGetTicketView(NULL, ticketView);
    if (result == -1017) {
        if (ESP_GetTicketViews(titleId, NULL, &viewCount) != 0) {
            __OSReturnToMenuForError();
        }
        views = (ESTicketView*)OSAllocFromMEM1ArenaLo((viewCount * sizeof(ESTicketView) + 31) & ~31, 32);
        if (!views) {
            __OSReturnToMenuForError();
        }
        if (ESP_GetTicketViews(titleId, views, &viewCount) != 0) {
            __OSReturnToMenuForError();
        }
        memcpy(ticketView, views, sizeof(ESTicketView));
    } else if (result != 0) {
        __OSReturnToMenuForError();
    } else if (OSPlayTimeIsLimited()) {
        limitKind = 0;
        limitValue = -1;
        __OSGetPlayTime(ticketView, &limitKind, &limitValue);
        if (limitValue == 0) {
            __OSWriteExpiredFlag();
            __OSReturnToMenuForError();
        }
    }
    page = (OSLaunchArgPage*)OSAllocFromMEM1ArenaLo(sizeof(OSLaunchArgPage), 64);
    info = &page->bootInfo;
    memset(page, 0, sizeof(OSLaunchArgPage));
    info->body.fields.titleIdHi = (u32)(titleId >> 32);
    info->body.fields.titleIdLo = (u32)titleId;
    info->body.fields.appType = OSGetAppType();
    info->body.fields.launchKind = 1;
    info->body.fields.launchCode = launchCode | 0x80000000;
    __OSCreateNandbootInfo();
    __OSWriteNandbootInfo(info);
    __OSReadStateFlags(&flags);
    flags.type = 3;
    __OSWriteStateFlags(&flags);
    if (ESP_LaunchTitle(titleId, ticketView) != 0) {
        __OSReturnToMenuForError();
    }
    for (;;) {
    }
}

/* 0x804D7960 (0x11C): launches `titleId` with its first valid ticket and returns only when that fails. */
void OSLaunchTitleDirect(u64 titleId)
{
    u32 viewCount = 1;
    ESTicketView* ticketView;
    ESTicketView* views;
    s32 index;

    if (ESP_InitLib() != 0) {
        return;
    }
    ticketView = (ESTicketView*)OSAllocFromMEM1ArenaLo(TICKET_VIEW_BUFFER_SIZE, 32);
    if (ticketView == NULL) {
        return;
    }
    memset(ticketView, 0, TICKET_VIEW_BUFFER_SIZE);
    if (ESP_GetTicketViews(titleId, NULL, &viewCount) != 0) {
        return;
    }
    views = (ESTicketView*)OSAllocFromMEM1ArenaLo((viewCount * sizeof(ESTicketView) + 31) & ~31, 32);
    if (views == NULL) {
        return;
    }
    if (ESP_GetTicketViews(titleId, views, &viewCount) != 0) {
        return;
    }
    index = __OSGetValidTicketIndex(views, viewCount);
    if (index < 0) {
        return;
    }
    memcpy(ticketView, &views[index], sizeof(ESTicketView));
    if (ESP_LaunchTitle(titleId, ticketView) == 0) {
        for (;;) {
        }
    }
}

/* 0x804D7A80 (0x44C): checks the title (and optionally its maker and region), stores the boot info and state flags for it and launches it; returns only on failure. */
void OSLaunchTitleWithArgs(u64 titleId, u32 launchCode, char** argv, BOOL checkTitle)
{
    ESTmdView* tmdView = NULL;
    u32 tmdSize = 0;
    OSLaunchArgPage* page;
    OSNandbootInfo* info;
    char** table;
    OSStateFlags flags;
    char name[24] __attribute__((aligned(32)));
    u64 bootTitleId __attribute__((aligned(32)));
    BOOL ok;
    u32 argc;
    u32 i;
    s32 result;

    if (ESP_InitLib() != 0) {
        __OSReturnToMenuForError();
    }
    if (ESP_GetTmdView(titleId, NULL, &tmdSize) != 0) {
        __OSReturnToMenuForError();
    }
    tmdView = (ESTmdView*)OSAllocFromMEM1ArenaLo((tmdSize + 31) & ~31, 64);
    if (tmdView == NULL) {
        __OSReturnToMenuForError();
    }
    if (ESP_GetTmdView(titleId, tmdView, &tmdSize) != 0) {
        __OSReturnToMenuForError();
    }
    if (!OSTitleHasContent(tmdView)) {
        OSReport("OSLaunchTitle(): Firmware is not installed\n");
        __OSReturnToMenuForError();
    }
    if (checkTitle) {
        if (!OSCheckTitleMakerCode(titleId, FALSE)) {
            OSReport("OSLaunchTitle(): Company code is not correct\n");
            __OSReturnToMenuForError();
        }
        if (tmdView->region == 3) {
            ok = TRUE;
        } else if (tmdView->region == SCGetProductGameRegion()) {
            ok = TRUE;
        } else {
            ok = FALSE;
        }
        if (!ok) {
            OSReport("OSLaunchTitle(): Country code is not correct\n");
            __OSReturnToMenuForError();
        }
    }
    ok = (tmdView->flags & 1) ? TRUE : FALSE;
    if (!ok && __DVDGetCoverStatus() == 2) {
        DVDResetDriveSync();
    }
    if (argv != NULL || launchCode != 0) {
        page = (OSLaunchArgPage*)OSAllocFromMEM1ArenaLo(sizeof(OSLaunchArgPage), 64);
        info = &page->bootInfo;
        sprintf(name, "%016llx", titleId);
        argc = 0;
        if (argv != NULL) {
            while (argv[argc] != NULL) {
                argc++;
            }
        }
        table = (char**)OSAllocFromMEM1ArenaLo((argc + 2) * sizeof(char*), 1);
        argc++;
        table[0] = name;
        for (i = 1; i < argc; i++) {
            table[i] = argv[i - 1];
        }
        OSLaunchPackArgs(page, argc, table);
        if (__OSInIPL || OSLaunchSystemMode[0]) {
            bootTitleId = 0x0000000100000002ULL;
        } else {
            result = ESP_InitLib();
            if (result != 0) {
                OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 803);
                __OSDoHotReset();
            }
            result = ESP_GetTitleId(&bootTitleId);
            if (result != 0) {
                OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 811);
                __OSDoHotReset();
            }
        }
        info->body.fields.titleIdHi = (u32)(bootTitleId >> 32);
        info->body.fields.titleIdLo = (u32)bootTitleId;
        info->body.fields.argsOffset = page->argsOffset;
        info->body.fields.appType = OSGetAppType();
        if (titleId == 0x0000000100000002ULL) {
            info->body.fields.launchKind = 4;
        } else {
            info->body.fields.launchKind = 2;
        }
        info->body.fields.launchCode = launchCode;
        __OSCreateNandbootInfo();
        __OSWriteNandbootInfo(info);
        __OSReadStateFlags(&flags);
        flags.type = 3;
        if (titleId == 0x0000000100000002ULL) {
            flags.discState = __OSGetDiscState(flags.discState);
            flags.returnToMenu = 2;
        }
        __OSWriteStateFlags(&flags);
    }
    OSDisableScheduler();
    __OSShutdownDevices(6);
    OSEnableScheduler();
    OSLaunchTitleDirect(titleId);
    __OSLaunchMenu();
}

/* 0x804D7ED0 (0x118): launches the Shop Channel help page of the console's region and returns only on failure. */
void OSLaunchShopChannelHelp(void)
{
    char* url;
    char** argv;
    u64 helpTitleId;

    OSGetAppType();
    __OSUnRegisterStateEvent();
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    url = (char*)OSAllocFromMEM1ArenaLo(0x40, 32);
    memset(url, 0, 0x40);
    strncpy(url, "/startup?initpage=showHelp", strlen("/startup?initpage=showHelp"));
    argv = (char**)OSAllocFromMEM1ArenaLo(4, 0x1000);
    argv[0] = url;
    argv[1] = NULL;
    switch (SCGetProductGameRegion()) {
    case 0:
    case 1:
    case 2:
        helpTitleId = 0x0001000248414241ULL;
        break;
    case 4:
        helpTitleId = 0x000100024841424BULL;
        break;
    case 5:
        helpTitleId = 0x0001000248414243ULL;
        break;
    default:
        helpTitleId = 0x0001000248414241ULL;
        break;
    }
    OSLaunchTitleWithArgs(helpTitleId, 3, argv, FALSE);
}
