/*
 * OS/OSExec.c - the OS program launcher: argument packing, the exec parameters, `__OSLaunchNextFirmware`,
 *    `__OSLaunchMenu`, `__OSBootDolSimple`, `__OSBootDol`.
 * RANGE. .text 0x804CDD70-0x804CF350 (11 functions); .data 0x8061C850-0x8061C8C0; .bss 0x8074D340-0x8074D360; .sdata
 *    0x80793F98-0x80793FA8; .sbss 0x80795330-0x80795348.  Cut from the OS core band 0x804C1760-0x804D9B4C.
 *    Evidence: .data 0x8061C850 ("OSExec(): Failed to exec") and 0x8061C8B0 (the apploader date) are read by
 *    `__OSLaunchNextFirmware` and `__OSBootDolSimple`; the first four functions are argument-packing helpers
 *    (`strlen`/`strcpy`/`memset`) whose twin heads the OSLaunch unit at 0x804D71F0.
 * FLAGS. `cflags_base` (configure.py), the default of the OS band, plus `opt_lifetimes off` and `opt_loop_invariants off`
 *    for the whole file: `__OSLaunchNextFirmware` goes from 95.3 % to 98.2 % and its size from 1644 to the target's 1668 B
 *    with them (each alone gives 97.4 % and 96.1 %); every other row is unchanged.
 * NAMES. `OSExecPackArgs`, `OSExecJump`, `OSExecSetReady`, `OSExecSetState`, `OSExecReady`, `OSExecState`, `OSExecParams`,
 *    `OSExecWideToHex`, `OSExecPackArgsWide` are GUESSes (the argument packer twins `OSLaunchPackArgs`; the jump flushes the
 *    icache and branches; the two setters store one `.sbss` word each).  `__OSLaunchNextFirmware`, `__OSLaunchMenu`,
 *    `__OSBootDolSimple`, `__OSBootDol`, `__OSGetExecParams` are the map's names; the left edge (0x804CDD70) is medium
 *    evidence (no data read by the four helpers).  GUESS: `OSExecDiskID` (the .bss disc id), `OSExecDataOffset` (the .sbss
 *    word 0x80795334 the partition-offset helper caches), `OSExecPartition*`/`OSExecTmd`/`OSAppLoaderHeader` and their
 *    fields, the `OS_*` low-memory macros, the DVDLow* names in `DVD/dvd_broadway.h` (from the DI command shapes: open
 *    partition takes offset, ticket, certs, TMD and callback; the ticket-view variant takes seven arguments).
 * RESIDUALS. register numbering of the six pointers of `__OSLaunchNextFirmware` (99.2 %, bytes and sizes equal); in
 *    `__OSBootDolSimple` (96.9 %) the offset helper keeps its value in r4 where the target uses r6, and params/header take
 *    swapped r30/r31; `OSExecWideToHex` (82.7 %): the target keeps the constant 4 in a register for its `4 & ~mask` shift and
 *    reloads the byte after the nibble test; `OSExecPackArgs`/`OSExecPackArgsWide` register numbering of locals;
 *    `__OSLaunchMenu` (95.8 %): the target branches `beq`+`b` over the result test where the source gives one `bne`;
 *    `OSExecJump` swaps two epilogue loads after the `bctr`.  Flip: not READY (the rows above; .data/.sdata/.sbss contents
 *    equal, only tail padding differs).
 * SHAPES. `OSExecJump` flushes the instruction cache and branches with inline asm; the empty `(void)0` bodies keep the
 *    target's branch shape; the eight disc-read command blocks are locals of `__OSBootDolSimple` declared in the reverse of
 *    their address order (the first declared sits highest); `titleId` is 32-aligned (the frame aligns dynamically).
 */

#include "types.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"
#include "MSL_C/strstr.h"
#include "ESP/esp.h"
#include "DVD/dvd.h"
#include "DVD/dvd_broadway.h"
#include "IPC/ipcMain.h"
#include "IPC/ipcclt.h"
#include "OS/OS.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OSArena.h"
#include "OS/OSCache.h"
#include "OS/OSError.h"
#include "OS/OSExec.h"
#include "OS/OSInterrupt.h"
#include "OS/OSIpc.h"
#include "OS/OSMemory.h"
#include "OS/OSPlayTime.h"
#include "OS/OSReset.h"
#include "OS/OSReboot.h"
#include "MSL_C/printf.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma opt_lifetimes off
#pragma opt_loop_invariants off

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

DVDDiskID OSExecDiskID;
volatile u32 OSExecReady;
BOOL __OSInReboot;
volatile u32 OSExecState;
static s32 OSExecDataOffset;
u32 __OSBootFlag;

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

/* size: 0x20 - the partition table header read from the disc at 0x40000. */
typedef struct OSExecPartitionHeader {
    /* +0x00 */ u32 groupCount;       /* entries in the first table */
    /* +0x04 */ u32 groupOffset;      /* where that table sits (in 4-byte units) */
    /* +0x08 */ u32 channelCount;     /* entries in the second table */
    /* +0x0C */ u32 channelOffset;
    /* +0x10 */ u8 pad_0x10[0x10];
} OSExecPartitionHeader;

/* size: 0x8 - one partition table entry. */
typedef struct OSExecPartitionEntry {
    /* +0x0 */ u32 offset; /* in 4-byte units */
    /* +0x4 */ u32 type;
} OSExecPartitionEntry;

/* size: 0x4A00 (cut at the last field) - the TMD of the launched partition; only the IOS it runs on is read. */
typedef struct OSExecTmd {
    /* +0x000 */ u8 pad_0x000[0x184];
    /* +0x184 */ u32 sysVersionHi; /* the title id of the IOS the title runs on */
    /* +0x188 */ u32 sysVersionLo;
    /* +0x18C */ u8 pad_0x18C[0x4A00 - 0x18C];
} OSExecTmd;

#define OS_BOOT_PARTITION_TYPE (*(u32*)0x80003194)
#define OS_BOOT_PARTITION_OFFSET (*(u32*)0x80003198)
#define OS_DISC_FLAGS_BYTE (*(u8*)0x80003187)
#define OS_DISC_DEBUG_FLAG 0x80
#define OS_LOW_MEMORY_BLOCK ((void*)0x80003100)
#define OS_LOW_MEMORY_BLOCK_SIZE 0x100
#define OS_PARTITION_TABLE_ADDRESS 0x10000

/* Reports a failed DI step and returns to the system menu; `line` is the number the original build embedded. */
#define OS_EXEC_CHECK(line)                                   \
    while (OSExecState == 0) {                                \
    }                                                         \
    if (OSExecState != 1) {                                   \
        OSReport("\nOSExec(): Failed to exec %d in %d\n", OSExecState, (line));     \
        __OSReturnToMenuForError();                           \
    }

/* 0x804CE2B0 (0x684): closes the running partition and opens the next one's IOS, then relaunches the title through ES. */
void __OSLaunchNextFirmware(void)
{
    u32 viewCount = 1;
    u32 tmdSize = 0;
    s32 result = -1;
    ESTicketView* ticketView;
    OSExecPartitionEntry* table;
    OSExecPartitionHeader* header;
    OSExecPartitionEntry* partition;
    OSExecTmd* tmd;
    OSExecPartitionEntry* entry;
    ESTicketView* views;
    u64 iosTitleId;
    u32 mem2Base;
    u32 mem2End;
    u32 limitKind;
    u32 limitValue;
    u8 i;

    header = OSAllocFromMEM1ArenaLo(0x20, 0x20);
    table = OSAllocFromMEM1ArenaLo(0x800, 0x20);
    tmd = OSAllocFromMEM1ArenaLo(0x4A00, 0x40);
    ticketView = OSAllocFromMEM1ArenaLo(0xE0, 0x20);
    if (__OSBootFlag == OS_BOOT_PARTITION_TYPE && OS_BOOT_PARTITION_OFFSET != 0) {
        result = ESP_InitLib();
        if (result == 0) {
            result = ESP_DiGetTicketView(NULL, ticketView);
        }
        if (result == 0) {
            result = ESP_DiGetTmd(NULL, &tmdSize);
        }
        if (result == 0) {
            result = ESP_DiGetTmd(tmd, &tmdSize);
        }
        ESP_CloseLib();
        if (OSPlayTimeIsLimited()) {
            limitKind = 0;
            limitValue = -1;
            __OSGetPlayTime(ticketView, (s32*)&limitKind, &limitValue);
            if (limitValue == 0) {
                __OSWriteExpiredFlag();
                __OSReturnToMenuForError();
            }
        }
    }
    if (result == 0) {
        partition = table;
        table->type = OS_BOOT_PARTITION_TYPE;
        table->offset = OS_BOOT_PARTITION_OFFSET;
    } else {
        OSExecState = 0;
        DVDLowClosePartition(OSExecSetState);
        OS_EXEC_CHECK(920)
        OSExecState = 0;
        DVDLowUnencryptedRead(header, 0x20, OS_PARTITION_TABLE_ADDRESS, OSExecSetState);
        OS_EXEC_CHECK(930)
        OSExecState = 0;
        DVDLowUnencryptedRead(table, 0x800, header->groupOffset, OSExecSetState);
        OS_EXEC_CHECK(942)
        partition = NULL;
        entry = table;
        for (i = 0; i < header->groupCount; i++, entry++) {
            if (entry->type == __OSBootFlag) {
                partition = entry;
            }
        }
        if (partition == NULL) {
            entry = table + 4;
            for (i = 0; i < header->channelCount; i++, entry++) {
                if (entry->type == __OSBootFlag) {
                    partition = entry;
                }
            }
        }
        if (partition == NULL) {
            OSReport("\nOSExec(): The specified game doesn't exist in the disc\n");
            __OSReturnToMenuForError();
        }
        OS_BOOT_PARTITION_TYPE = partition->type;
        OS_BOOT_PARTITION_OFFSET = partition->offset;
        OSExecState = 0;
        if (OS_DISC_FLAGS_BYTE == OS_DISC_DEBUG_FLAG) {
            DVDLowOpenPartitionWithTmdAndTicketView(partition->offset, ticketView, tmdSize, tmd, 0, NULL, OSExecSetState);
        } else {
            DVDLowOpenPartition(partition->offset, NULL, 0, NULL, tmd, OSExecSetState);
        }
        OS_EXEC_CHECK(1003)
        OSExecState = 0;
        DVDLowClosePartition(OSExecSetState);
        OS_EXEC_CHECK(1013)
    }
    iosTitleId = ((u64)tmd->sysVersionHi << 32) | tmd->sysVersionLo;
    result = ESP_InitLib();
    if (result != 0) {
        OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 1024);
        __OSDoHotReset();
    }
    result = ESP_GetTicketViews(iosTitleId, NULL, &viewCount);
    if (viewCount != 1 || result != 0) {
        OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 1033);
        __OSDoHotReset();
    }
    views = OSAllocFromMEM1ArenaLo((viewCount * sizeof(ESTicketView) + 31) & ~31, 32);
    result = ESP_GetTicketViews(iosTitleId, views, &viewCount);
    if (result != 0) {
        OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 1042);
        __OSDoHotReset();
    }
    DVDLowClose();
    mem2Base = OS_MEM2_AREA_BASE;
    mem2End = OS_MEM2_AREA_END;
    DCStoreRange(OS_LOW_MEMORY_BLOCK, OS_LOW_MEMORY_BLOCK_SIZE);
    result = ESP_LaunchTitle(iosTitleId, views);
    if (result != 0) {
        OSReport("\nOSExec(): Failed to exec %d in %d\n", result, 1058);
        __OSDoHotReset();
    }
    ESP_CloseLib();
    DCInvalidateRange(OS_LOW_MEMORY_BLOCK, OS_LOW_MEMORY_BLOCK_SIZE);
    if (mem2Base < OS_MEM2_AREA_BASE) {
        OS_MEM2_AREA_END = mem2Base - (OS_MEM2_AREA_BASE - OS_MEM2_AREA_END);
        OS_SAVED_MEM2_ARENA_HI = (void*)(mem2Base - (OS_MEM2_AREA_BASE - (u32)OS_SAVED_MEM2_ARENA_HI));
        OS_IPC_BUFFER_LO = (void*)(mem2Base - (OS_MEM2_AREA_BASE - (u32)OS_IPC_BUFFER_LO));
        OS_IPC_BUFFER_HI = (void*)(mem2Base - (OS_MEM2_AREA_BASE - (u32)OS_IPC_BUFFER_HI));
        OS_MEM2_AREA_BASE = mem2Base;
    }
    if (mem2End < OS_MEM2_AREA_END) {
        __OSInitMemoryProtection();
    }
    __OSInitIPCBuffer();
    IPCReInit();
    IPCCltInitSmallHeap();
    DVDLowInit();
    OSExecState = 0;
    DVDLowReadDiskID(&OSExecDiskID, OSExecSetState);
    OS_EXEC_CHECK(1102)
    OSExecState = 0;
    if (OS_DISC_FLAGS_BYTE == OS_DISC_DEBUG_FLAG) {
        DVDLowOpenPartitionWithTmdAndTicketView(partition->offset, ticketView, tmdSize, tmd, 0, NULL, OSExecSetState);
    } else {
        DVDLowOpenPartition(partition->offset, NULL, 0, NULL, tmd, OSExecSetState);
    }
    OS_EXEC_CHECK(1125)
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

typedef void (*OSAppLoaderInit)(void (*report)(const char*, ...));
typedef int (*OSAppLoaderMain)(u8** dst, u32* size, u32* offset); /* nonzero while there is a chunk to read */
typedef u32 (*OSAppLoaderClose)(void);                           /* returns the entry point of the loaded DOL */
typedef void (*OSAppLoaderEntry)(OSAppLoaderInit* init, OSAppLoaderMain* main, OSAppLoaderClose* close);

/* size: 0x20 - the apploader header at the start of a partition's data area. */
typedef struct OSAppLoaderHeader {
    /* +0x00 */ char date[0x10]; /* build date, "YYYY/MM/DD" */
    /* +0x10 */ OSAppLoaderEntry entry;
    /* +0x14 */ u32 size;        /* bytes of the apploader image that follow the header */
    /* +0x18 */ u32 trailerSize; /* bytes of the trailer that follow the image */
    /* +0x1C */ u8 pad_0x1C[4];
} OSAppLoaderHeader;

#define OS_DISC_PARTITION_BASE_WORD (*(s32*)0x800030F4)
#define OS_DISC_PARTITION_DEFAULT_OFFSET 0x910
#define OS_RESET_CODE_FROM_NAND 0xA0000000
#define OS_APPLOADER_LEGACY_DATE "2004/02/01"
#define OS_APPLOADER_IMAGE_ADDRESS ((void*)0x81200000)
#define OS_APPLOADER_LEGACY_ADDRESS ((void*)0x81330000)
#define OS_LEGACY_SAVE_START (*(u8**)0x812FDFF0)
#define OS_LEGACY_SAVE_END (*(u8**)0x812FDFEC)
#define OS_LEGACY_BOOT_FLAG (*(u8*)0x800030E2)
#define OS_BOOT_GAME_ID (*(u32*)0x80003180)
#define OS_BOOT_DISC_ID_WORD (*(u32*)0x80000000)
#define OS_DISC_APP_TYPE_WORD (*(u8*)0x80003184)
#define OS_DI_CONTROL_REGISTER (*(u32*)0xCC003024)

/* Reads `length` bytes at `offset` (in 4-byte units) of the disc into `dst` and waits; a failing drive returns to the menu. */
/* untyped: byte range the read fills */
static inline void ReadDiscBlocking(DVDCommandBlock* block, void* dst, s32 length, s32 offset)
{
    DVDReadAbsAsyncPrio(block, dst, length, offset, NULL, 0);
    while (DVDGetCommandBlockStatus(block) != 0) {
        if (DVDGetCommandBlockStatus(block) > 2 || DVDGetCommandBlockStatus(block) < 0) {
            __OSReturnToMenuForError();
        }
    }
}

/* size: 0x40 - the block read from the start of the disc's data area; only the offset word is used. */
typedef struct OSExecDiscInfo {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ s32 dataOffset;
    /* +0x3C */ u8 pad_0x3C[4];
} OSExecDiscInfo;

/* Returns where the partition data starts (in 4-byte units), reading it from the disc the first time. */
static inline s32 GetPartitionDataOffset(DVDCommandBlock* block)
{
    OSExecDiscInfo* info;
    s32 dataOffset = OSExecDataOffset;

    if (dataOffset != 0) {
        (void)0;
    } else if (OS_DISC_PARTITION_BASE_WORD != 0) {
        info = OSAllocFromMEM1ArenaLo(0x40, 0x20);
        ReadDiscBlocking(block, info, 0x40, OS_DISC_PARTITION_BASE_WORD >> 2);
        dataOffset = (OS_DISC_PARTITION_BASE_WORD + info->dataOffset) >> 2;
        OSExecDataOffset = dataOffset;
    } else {
        dataOffset = OS_DISC_PARTITION_DEFAULT_OFFSET;
        OSExecDataOffset = dataOffset;
    }
    return dataOffset;
}

/* 0x804CEA10 (0x754): boots the DOL image at `dolOffset`: runs the apploader for it (or loads the legacy one) and jumps. */
void __OSBootDolSimple(u32 dolOffset, u32 resetCode, u8* saveStart, u8* saveEnd, u32 flags, s32 argc, const char** argv)
{
    OSAppLoaderHeader* header;
    OSExecParams* params;
    OSExecParams* paramsCopy;
    OSAppLoaderInit appInit;
    OSAppLoaderMain appMain;
    OSAppLoaderClose appClose;
    u8* dst;
    u32 size;
    u32 offset;
    u64 titleId __attribute__((aligned(32)));
    BOOL newerAppLoader;
    u32 entry;
    s32 dataOffset;
    DVDCommandBlock blockImage;
    DVDCommandBlock blockOffsetImage;
    DVDCommandBlock blockHeader;
    DVDCommandBlock blockOffsetHeader;
    DVDCommandBlock blockOffsetDol;
    DVDCommandBlock blockMain;
    DVDCommandBlock blockLegacyOffset;
    DVDCommandBlock blockLegacyImage;

    OSDisableInterrupts();
    if (__OSInReboot != 0) {
        __OSBootFlag = OS_BOOT_PARTITION_TYPE;
    }
    __OSRestoreCodeExecOnMEM1(0xBA2CF);
    params = OSAllocFromMEM1ArenaLo(0x1C, 1);
    params->valid = 1;
    params->resetCode = resetCode;
    params->saveStart = saveStart;
    params->saveEnd = saveEnd;
    params->flags = flags;
    if (flags == 0) {
        params->argPage = OSAllocFromMEM1ArenaLo(0x2000, 1);
        if (__OSBootFlag == 2 && __OSInReboot == 0) {
            OSExecPackArgsWide(params->argPage, argc, (char**)argv);
        } else {
            OSExecPackArgs(params->argPage, argc, (char**)argv);
        }
    }
    DVDInit();
    DVDSetAutoInvalidation(TRUE);
    DVDResume();
    OSExecReady = 0;
    __DVDPrepareResetAsync(OSExecSetReady);
    __OSMaskInterrupts(0xFFFFFFF0);
    __OSUnmaskInterrupts(0x10);
    OSEnableInterrupts();
    while ((s32)OSExecReady != 1) {
    }
    __OSLaunchNextFirmware();
    if (resetCode == OS_RESET_CODE_FROM_NAND && __OSInReboot == 0) {
        if (ESP_InitLib() != 0) {
            return;
        }
        if (ESP_GetTitleId(&titleId) != 0) {
            return;
        }
        if (ESP_CloseLib() != 0) {
            return;
        }
        snprintf((char*)params->argPage + (u32)argv[1], 17, "%016llx", titleId);
    }
    header = OSAllocFromMEM1ArenaLo(0x20, 0x20);
    dataOffset = GetPartitionDataOffset(&blockOffsetHeader);
    ReadDiscBlocking(&blockHeader, header, 0x20, dataOffset);
    dataOffset = GetPartitionDataOffset(&blockOffsetImage);
    ReadDiscBlocking(&blockImage, OS_APPLOADER_IMAGE_ADDRESS, (header->size + 31) & ~31, dataOffset + 8);
    ICInvalidateRange(OS_APPLOADER_IMAGE_ADDRESS, (header->size + 31) & ~31);
    if (strncmp(header->date, OS_APPLOADER_LEGACY_DATE, 10) > 0) {
        newerAppLoader = TRUE;
    } else {
        newerAppLoader = FALSE;
    }
    if (newerAppLoader) {
        if (dolOffset == (u32)-1) {
            dataOffset = GetPartitionDataOffset(&blockOffsetDol);
            dolOffset = ((header->size + 0x20) >> 2) + dataOffset;
        }
        params->dolOffset = dolOffset;
        header->entry(&appInit, &appMain, &appClose);
        paramsCopy = OSAllocFromMEM1ArenaLo(0x1C, 1);
        memcpy(paramsCopy, params, 0x1C);
        OS_EXEC_PARAMS_ADDR = (u32)paramsCopy;
        appInit(OSReport);
        OSSetArenaLo(paramsCopy);
        while (appMain(&dst, &size, &offset)) {
            ReadDiscBlocking(&blockMain, dst, size, offset >> __DVDLayoutFormat);
        }
        entry = appClose();
        OS_BOOT_GAME_ID = OS_BOOT_DISC_ID_WORD;
        OS_DISC_APP_TYPE_WORD = 0x80;
        paramsCopy = OSAllocFromMEM1ArenaLo(0x1C, 1);
        memcpy(paramsCopy, params, 0x1C);
        OS_EXEC_PARAMS_ADDR = (u32)paramsCopy;
        OS_DI_CONTROL_REGISTER = 7;
        OSDisableInterrupts();
        OSExecJump(entry);
    } else {
        OS_LEGACY_SAVE_START = saveStart;
        OS_LEGACY_SAVE_END = saveEnd;
        OS_LEGACY_BOOT_FLAG = 1;
        dataOffset = GetPartitionDataOffset(&blockLegacyOffset);
        ReadDiscBlocking(&blockLegacyImage, OS_APPLOADER_LEGACY_ADDRESS, (header->trailerSize + 31) & ~31,
                         ((header->size + 0x20) >> 2) + dataOffset);
        ICInvalidateRange(OS_APPLOADER_LEGACY_ADDRESS, (header->trailerSize + 31) & ~31);
        OSDisableInterrupts();
        ICFlashInvalidate();
        OSExecJump((u32)OS_APPLOADER_LEGACY_ADDRESS);
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
    sprintf(name, "%d", bootDol);
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
