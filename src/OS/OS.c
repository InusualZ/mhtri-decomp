/*
 * OS/OS.c - the OS core: FPR init, console type and arena clearing, `OSInit`, the exception vector and table, version
 *    registration and the app identity.
 * RANGE. .text 0x804C9F00-0x804CB440 (20 functions); .data 0x8061BC20-0x8061C0C8; .bss 0x8074D280-0x8074D2F0; .sdata
 *    0x80793F50-0x80793F80; .sbss 0x807952C8-0x80795310; .sdata2 0x8079D2D0-0x8079D2D8.  Cut from the OS core
 *    band 0x804C1760-0x804D9B4C.  Evidence: the build string "<< RVL_SDK - OS ... (0x4302_145) >>" opens its
 *    .data (0x8061BC20, read by `ReportOSInfo`, `OSInit`, `OSExceptionInit`); it owns
 *    `__OSInIPL`/`OSExceptionTable`/`BootInfo`/`ZeroPS`/`ZeroF` (.sbss 0x807952C8..0x80795310) and `__OSVersion`;
 *    `__OSInitAlarm` (0x804CB440) is the first function that reads `AlarmQueue`.
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from; unchanged by the cut.
 * NAMES. `OSInit`, `OSExceptionInit`, `ReportOSInfo`, `__OSPSInit`, `OSRegisterVersion`, ... are the map's names; no
 *    `__FILE__` string for OS.c is in the image (the anchors the band names are OSReset.c, OSStateTM.c,
 *    OSPlayTime.c, OSLaunch.c; the `OSPanic` file operand is the 5-byte string "OS.c").  GUESS: `OSIOSRev` (the record
 *    `__OSGetIOSRev` fills), `InquiryDriveInfo`/`InquiryCommandBlock` (the two halves of the former 0x50-byte .bss row),
 *    `AppGamename`, `DefaultGamename`, `OSBootInfo`/`OSBootInfo2` (the blocks at 0x80000000 and *0x800000F4),
 *    `OSRebootParams.arenaLimit`/`arenaResume`, `BI2DebugFlagHolder`, `AreWeInitialized`, the `OS_*` low-memory macros.
 * RESIDUALS. `OSInit` (0x554 B): register numbering of the BI2 pointer (r5 in the target, r3 here) and of the IOS
 *    date/version pair (swapped), the two firmware-record colours land in swapped stack slots, and the arena-low default
 *    0x80004000 is a plain constant here where the target's disassembly names it `_f_init` (the same bits).  `OSExceptionInit`
 *    (99.1 %): one callee-saved register offset in the whole function (the target keeps one more value live).
 *    `ClearArena`/`ClearMEM2Arena` (98.9 %): the size/flush-start register pair is swapped.  Flip: not READY, the above.
 * SHAPES. the exception vector, FPR/PS init, default handler and debugger entries are `asm` functions with `nofralloc`;
 *    `ClearRange` is an inline helper whose arguments evaluate right to left; `ClearArena`/`ClearMEM2Arena` are
 *    `never_inline` (the target calls them); the target's `.bss` and `.sbss` order is the order the compiler first meets
 *    the definitions, so the definitions sit ahead of the functions in that order; the empty `(void)0` bodies keep the
 *    target's branch shape; `inquiry` is signed (the compare tree is the signed one) and the inquiry word is volatile
 *    (the target reads it twice).
 */

#include "types.h"
#include "DB/__DBIsExceptionMarked.h"
#include "DB/DBPrintf.h"
#include "DVD/dvd.h"
#include "DVD/dvddevice.h"
#include "DVD/dvdfs.h"
#include "EXI/EXIBios.h"
#include "IPC/ipcclt.h"
#include "NWC24/nwc24_io.h"
#include "OS/OSAlarm.h"
#include "OS/OSAudioSystem.h"
#include "OS/OSExec.h"
#include "OS/OSFatal.h"
#include "OS/OSInterrupt.h"
#include "OS/OSLink.h"
#include "OS/OSPlayRecord.h"
#include "OS/OSPlayTime.h"
#include "OS/OSRtc.h"
#include "OS/OSStateTM.h"
#include "OS/OSSync.h"
#include "OS/OSThread.h"
#include "OS/__OSGetSystemTime.h"
#include "PAD/pad.h"
#include "SC/SCSystemConfig.h"
#include "SI/SIBios.h"
#include "TRK/dolphin_trk.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OS.h"
#include "OS/OSCache.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSArena.h"
#include "OS/OSMemory.h"
#include "OS/OSReset.h"
#include "OS/PPCArch.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The low-memory words the OS core reads: the disc header's game code and disc number, the IOS revision and its build date,
 * the system-menu inquiry result and the DI configuration register. */
#define OS_DISC_GAME_CODE ((char*)0x80003194)
#define OS_DISC_FALLBACK_GAME_CODE ((char*)0x80003180)
#define OS_DISC_APP_TYPE (*(u8*)0x80003184)
#define OS_IOS_VERSION_WORD (*(u32*)0xC0003140)
#define OS_IOS_DATE_WORD (*(u32*)0xC0003144)
#define OS_INQUIRY_RESULT (*(volatile u16*)0x800030E6)
#define OS_DI_CONFIG_REGISTER (*(volatile u32*)0xCD006024)

/* size: 0x38 (cut at the last field) - the boot information block at 0x80000000. */
typedef struct OSBootInfo {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ u32 consoleType; /* zero on a console that reported none */
    /* +0x30 */ void* arenaLo; /* low bound of the MEM1 arena the loader chose, zero for none */
    /* +0x34 */ void* arenaHi; /* high bound of the MEM1 arena the loader chose, zero for none */
} OSBootInfo;

/* size: 0x28 - the second boot information block (BI2); only the debug flag and the pad specification are read. */
typedef struct OSBootInfo2 {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ u32 debugFlag;
    /* +0x10 */ u8 pad_0x10[0x14];
    /* +0x24 */ u32 padSpec;
} OSBootInfo2;

const char* __OSVersion = "<< RVL_SDK - OS 	release build: Feb 27 2009 10:04:29 (0x4302_145) >>";

OSRebootParams __OSRebootParams;
DVDDriveInfo InquiryDriveInfo __attribute__((aligned(32)));
DVDCommandBlock InquiryCommandBlock;

/* The small-data words are defined in reverse of their address order (the compiler lays them out last to first). */
s64 __OSStartTime;
static OSBootInfo* BootInfo;
u32* BI2DebugFlag;
static u32 BI2DebugFlagHolder;
static char AppGamename[5];
static f64 ZeroF;
static f32 ZeroPS[2];
OSExceptionHandler* OSExceptionTable;
static BOOL AreWeInitialized;
__declspec(weak) BOOL __OSIsGcam;
BOOL __OSInNandBoot;
BOOL __OSInIPL;

/* 0x804C9F00 (0x128): enables the FPU and zeroes every floating-point register, paired singles included when enabled. */
asm void __OSFPRInit(void)
{
    nofralloc
    mfmsr r3
    ori r3, r3, 0x2000
    mtmsr r3
    mfspr r3, 920
    rlwinm. r3, r3, 3, 31, 31
    beq skipPairedSingles
    lis r3, ZeroPS@ha
    addi r3, r3, ZeroPS@l
    psq_l f0, 0(r3), 0, 0
    ps_mr f1, f0
    ps_mr f2, f0
    ps_mr f3, f0
    ps_mr f4, f0
    ps_mr f5, f0
    ps_mr f6, f0
    ps_mr f7, f0
    ps_mr f8, f0
    ps_mr f9, f0
    ps_mr f10, f0
    ps_mr f11, f0
    ps_mr f12, f0
    ps_mr f13, f0
    ps_mr f14, f0
    ps_mr f15, f0
    ps_mr f16, f0
    ps_mr f17, f0
    ps_mr f18, f0
    ps_mr f19, f0
    ps_mr f20, f0
    ps_mr f21, f0
    ps_mr f22, f0
    ps_mr f23, f0
    ps_mr f24, f0
    ps_mr f25, f0
    ps_mr f26, f0
    ps_mr f27, f0
    ps_mr f28, f0
    ps_mr f29, f0
    ps_mr f30, f0
    ps_mr f31, f0
skipPairedSingles:
    lfd f0, ZeroF
    fmr f1, f0
    fmr f2, f0
    fmr f3, f0
    fmr f4, f0
    fmr f5, f0
    fmr f6, f0
    fmr f7, f0
    fmr f8, f0
    fmr f9, f0
    fmr f10, f0
    fmr f11, f0
    fmr f12, f0
    fmr f13, f0
    fmr f14, f0
    fmr f15, f0
    fmr f16, f0
    fmr f17, f0
    fmr f18, f0
    fmr f19, f0
    fmr f20, f0
    fmr f21, f0
    fmr f22, f0
    fmr f23, f0
    fmr f24, f0
    fmr f25, f0
    fmr f26, f0
    fmr f27, f0
    fmr f28, f0
    fmr f29, f0
    fmr f30, f0
    fmr f31, f0
    mtfsf 255, f0
    blr
}

/* 0x804CA030 (0x6C): reads the IOS version and its BCD build date. */
void __OSGetIOSRev(OSIOSRev* rev)
{
    u32 version;
    u32 date;

    date = OS_IOS_DATE_WORD;
    version = OS_IOS_VERSION_WORD;

    rev->platform = version >> 24;
    rev->major = version >> 16;
    rev->minor = version >> 8;
    rev->micro = version;
    rev->month = ((date >> 16) & 0xF) + ((date >> 20) & 0xF) * 10;
    rev->day = ((date >> 8) & 0xF) + ((date >> 12) & 0xF) * 10;
    rev->year = (date & 0xF) + ((date >> 4) & 0xF) * 10 + 2000;
}

/* The consoles `OSGetConsoleType` names: the retail values and the same with the development-hardware bit. */
#define OS_CONSOLE_BIT_DEV 0x10000000
#define OS_CONSOLE_MASK_NOT_DEV 0x7FFF
#define OS_CONSOLE_ARTHUR 0x10000002
#define OS_CONSOLE_ARCADE 0x100
#define OS_MEM2_RETAIL_SIZE 0x04000000
#define OS_BOOT_CONSOLE_WORD (*(u32*)0x80003138)

/* 0x804CA0A0 (0x254): returns the console type word from the boot information, the inquiry result and the MEM2 size. */
u32 OSGetConsoleType(void)
{
    u32 type;
    s32 inquiry;
    u32 mem2Size;

    if (BootInfo == NULL || BootInfo->consoleType == 0) {
        return OS_CONSOLE_ARTHUR;
    }
    type = OS_BOOT_CONSOLE_WORD;
    if (OS_INQUIRY_RESULT & 0x8000) {
        inquiry = OS_INQUIRY_RESULT & ~0x8000;
        switch (inquiry) {
        case 2:
        case 3:
        case 0x203:
            switch (type) {
            case 0:
                return 0x10;
            case 1:
                return 0x11;
            case 2:
                return 0x12;
            case 0x10:
                return 0x20;
            case 0x11:
                return 0x21;
            default:
                if (type > 0x11) {
                    return 0x21;
                }
            }
        case 0x201:
        case 0x202:
            switch (type) {
            case 0:
                return OS_CONSOLE_BIT_DEV + 0x10;
            case 1:
                return OS_CONSOLE_BIT_DEV + 0x11;
            case 2:
                return OS_CONSOLE_BIT_DEV + 0x12;
            case 0x10:
                return OS_CONSOLE_BIT_DEV + 0x20;
            case 0x11:
                return OS_CONSOLE_BIT_DEV + 0x21;
            default:
                if (type > 0x11) {
                    return OS_CONSOLE_BIT_DEV + 0x21;
                }
            }
        case 0x300:
            return OS_CONSOLE_ARCADE;
        }
    }
    mem2Size = OSGetPhysicalMem2Size();
    switch (type) {
    case 0:
        return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x10 : OS_CONSOLE_BIT_DEV + 0x10;
    case 1:
        return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x11 : OS_CONSOLE_BIT_DEV + 0x11;
    case 2:
        return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x12 : OS_CONSOLE_BIT_DEV + 0x12;
    case 0x10:
        return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x20 : OS_CONSOLE_BIT_DEV + 0x20;
    case 0x11:
        return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x21 : OS_CONSOLE_BIT_DEV + 0x21;
    default:
        if (type > 0x11) {
            return mem2Size == OS_MEM2_RETAIL_SIZE ? 0x21 : OS_CONSOLE_BIT_DEV + 0x21;
        }
        return BootInfo->consoleType;
    }
}

/* The most the arena clear flushes after zeroing a range. */
#define OS_CLEAR_FLUSH_SIZE 0x40000

/* Zeroes `size` bytes at `start` and flushes the last 256 KiB of them. */
static inline void ClearRange(u8* start, u32 size)
{
    u8* flushStart;

    if (size > OS_CLEAR_FLUSH_SIZE) {
        flushStart = start + size - OS_CLEAR_FLUSH_SIZE;
    } else {
        flushStart = start;
    }
    DCZeroRange(start, size);
    DCFlushRange(flushStart, OS_CLEAR_FLUSH_SIZE);
}

/* 0x804CA300 (0x1DC): zeroes the MEM1 arena, or the part of it the previous title did not leave to keep. */
static void __attribute__((never_inline)) ClearArena(void)
{
    if ((OSGetResetCode() >> 31) == 0) {
        ClearRange(OSGetArenaLo(), (u8*)OSGetArenaHi() - (u8*)OSGetArenaLo());
        return;
    }
    if (__OSRebootParams.arenaLimit == 0 || (__OSRebootParams.arenaLimit & 0x30000000) != 0) {
        ClearRange(OSGetArenaLo(), (u8*)OSGetArenaHi() - (u8*)OSGetArenaLo());
        return;
    }
    if ((u32)OSGetArenaLo() < __OSRebootParams.arenaLimit) {
        if ((u32)OSGetArenaHi() <= __OSRebootParams.arenaLimit) {
            ClearRange(OSGetArenaLo(), (u8*)OSGetArenaHi() - (u8*)OSGetArenaLo());
            return;
        }
        ClearRange(OSGetArenaLo(), __OSRebootParams.arenaLimit - (u32)OSGetArenaLo());
        if ((u32)OSGetArenaHi() > __OSRebootParams.arenaResume) {
            ClearRange((u8*)__OSRebootParams.arenaResume, (u32)OSGetArenaHi() - __OSRebootParams.arenaResume);
        }
    }
}

/* 0x804CA4E0 (0x1E4): zeroes the MEM2 arena, or the part of it the previous title did not leave to keep. */
static void __attribute__((never_inline)) ClearMEM2Arena(void)
{
    if ((OSGetResetCode() >> 31) == 0) {
        ClearRange(OSGetMEM2ArenaLo(), (u8*)OSGetMEM2ArenaHi() - (u8*)OSGetMEM2ArenaLo());
        return;
    }
    if (__OSRebootParams.arenaLimit == 0 || (__OSRebootParams.arenaLimit & 0x30000000) != 0x10000000) {
        ClearRange(OSGetMEM2ArenaLo(), (u8*)OSGetMEM2ArenaHi() - (u8*)OSGetMEM2ArenaLo());
        return;
    }
    if ((u32)OSGetMEM2ArenaLo() < __OSRebootParams.arenaLimit) {
        if ((u32)OSGetMEM2ArenaHi() <= __OSRebootParams.arenaLimit) {
            ClearRange(OSGetMEM2ArenaLo(), (u8*)OSGetMEM2ArenaHi() - (u8*)OSGetMEM2ArenaLo());
            return;
        }
        ClearRange(OSGetMEM2ArenaLo(), __OSRebootParams.arenaLimit - (u32)OSGetMEM2ArenaLo());
        if ((u32)OSGetMEM2ArenaHi() > __OSRebootParams.arenaResume) {
            ClearRange((u8*)__OSRebootParams.arenaResume, (u32)OSGetMEM2ArenaHi() - __OSRebootParams.arenaResume);
        }
    }
}

/* 0x804CA6D0 (0x3C): stores the drive inquiry result as the OS device code. */
static void InquiryCallback(s32 result, DVDCommandBlock* block)
{
    switch (block->state) {
    case 0:
        OS_INQUIRY_RESULT = InquiryDriveInfo.deviceCode | 0x8000;
        break;
    default:
        OS_INQUIRY_RESULT = 1;
        break;
    }
}

/* 0x804CA710 (0x2A4): prints the kernel build, console type, firmware version and the arenas to the debug log. */
static void ReportOSInfo(void)
{
    u32 consoleType;
    u32 version;
    u32 date;
    u32 memoryMB;

    OSReport("\nRevolution OS\n");
    OSReport("Kernel built : %s %s\n", "Feb 27 2009", "10:04:29");
    OSReport("Console Type : ");
    consoleType = OSGetConsoleType();
    switch (consoleType & 0xF0000000) {
    case 0:
        switch (consoleType) {
        case 0x11:
            OSReport("Pre-production board 1\n");
            break;
        case 0x12:
            OSReport("Pre-production board 2-1\n");
            break;
        case 0x20:
            OSReport("Pre-production board 2-2\n");
            break;
        case 0x100:
            OSReport("RVA 1\n");
            break;
        default:
            OSReport("Retail %d\n", consoleType);
            break;
        }
        break;
    case OS_CONSOLE_BIT_DEV:
        switch (consoleType - OS_CONSOLE_BIT_DEV) {
        case 0x21:
            OSReport("NDEV 2.1\n");
            break;
        case 0x20:
            OSReport("NDEV 2.0\n");
            break;
        case 0x12:
            OSReport("NDEV 1.2\n");
            break;
        case 0x11:
            OSReport("NDEV 1.1\n");
            break;
        case 0x10:
            OSReport("NDEV 1.0\n");
            break;
        case 8:
            OSReport("Revolution Emulator\n");
            break;
        default:
            OSReport("Emulation platform (%08x)\n", consoleType);
            break;
        }
        break;
    case 0x20000000:
        OSReport("TDEV-based emulation HW%d\n", (consoleType & 0x0FFFFFFF) - 3);
        break;
    default:
        OSReport("%08x\n", consoleType);
        break;
    }
    version = OS_IOS_VERSION_WORD;
    date = OS_IOS_DATE_WORD;
    OSReport("Firmware     : %d.%d.%d ", (u8)(version >> 16), (u8)(version >> 8), (u8)version);
    OSReport("(%d/%d/%d)\n", (u8)(((date >> 16) & 0xF) + ((date >> 20) & 0xF) * 10),
             (u8)(((date >> 8) & 0xF) + ((date >> 12) & 0xF) * 10), (u16)((date & 0xF) + ((date >> 4) & 0xF) * 10 + 2000));
    memoryMB = (OSGetConsoleSimulatedMem1Size() + OSGetConsoleSimulatedMem2Size()) >> 20;
    OSReport("Memory %d MB\n", memoryMB);
    OSReport("MEM1 Arena : 0x%x - 0x%x\n", OSGetMEM1ArenaLo(), OSGetMEM1ArenaHi());
    OSReport("MEM2 Arena : 0x%x - 0x%x\n", OSGetMEM2ArenaLo(), OSGetMEM2ArenaHi());
}

/* The boot-time words OSInit reads: the BI2 pointer, the arena bounds the loader left, the DVD and boot-program markers. */
#define OS_BI2_POINTER (*(OSBootInfo2**)0x800000F4)
#define OS_DEBUG_FLAG_BYTE (*(u8*)0x800030E8)
#define OS_PAD_SPEC_BYTE (*(u8*)0x800030E9)
#define OS_SAVED_ARENA_LO (*(void**)0x8000310C)
#define OS_SAVED_ARENA_HI (*(void**)0x80003110)
#define OS_BOOT_PROGRAM_BYTE_A (*(u8*)0x8000315C)
#define OS_BOOT_PROGRAM_BYTE_B (*(u8*)0x8000315D)
#define OS_REQUIRED_IOS_WORD (*(u32*)0x80003188)
#define OS_MEM2_PHYS_BASE 0x90000000
#define OS_MEM2_RESERVED_END 0x90000800
#define OS_ARENA_REGION_MASK 0x30000000
#define OS_INQUIRY_GCAM 0x9000
#define OS_APP_TYPE_DIAG 0x80
#define OS_LINE_BOOT_PROGRAM_A 1153
#define OS_LINE_BOOT_PROGRAM_B 1171
#define OS_LINE_IOS_VERSION 1236

extern u8 _stack_addr[];
extern u8 __ArenaLo[];
extern u8 __ArenaHi[];

/* 0x804CA9C0 (0x554): brings up the OS once: arenas, exception vectors, interrupts, caches, devices and the title checks. */
void OSInit(void)
{
    OSBootInfo2* bi2;
    u8 appType;
    void* arenaLo;
    void* arenaHi;
    void* mem2Lo;
    OSIOSRev rev;

    if (AreWeInitialized != FALSE) {
        return;
    }
    AreWeInitialized = TRUE;
    __OSStartTime = __OSGetSystemTime();
    OSDisableInterrupts();
    __OSGetExecParams((OSExecParams*)&__OSRebootParams);
    PPCMtmmcr0(0);
    PPCMtmmcr1(0);
    PPCMtpmc1(0);
    PPCMtpmc2(0);
    PPCMtpmc3(0);
    PPCMtpmc4(0);
    PPCMthid4(0x83900000);
    PPCDisableSpeculation();
    PPCSetFpNonIEEEMode();
    BootInfo = (OSBootInfo*)0x80000000;
    BI2DebugFlag = NULL;
    __DVDLongFileNameFlag = FALSE;
    bi2 = OS_BI2_POINTER;
    if (bi2 != NULL) {
        BI2DebugFlag = &bi2->debugFlag;
        __PADSpec = bi2->padSpec;
        OS_DEBUG_FLAG_BYTE = bi2->debugFlag;
        OS_PAD_SPEC_BYTE = __PADSpec;
    } else if (BootInfo->arenaHi != NULL) {
        BI2DebugFlagHolder = OS_DEBUG_FLAG_BYTE;
        BI2DebugFlag = &BI2DebugFlagHolder;
        __PADSpec = OS_PAD_SPEC_BYTE;
    }
    __DVDLongFileNameFlag = TRUE;

    arenaLo = OS_SAVED_ARENA_LO;
    if (arenaLo == NULL) {
        arenaLo = __ArenaLo;
        if (((u32)__ArenaLo & OS_ARENA_REGION_MASK) == 0) {
            if (BootInfo->arenaLo == NULL) {
                (void)0;
            } else {
                arenaLo = BootInfo->arenaLo;
            }
            if (BootInfo->arenaLo == NULL && BI2DebugFlag != NULL && *BI2DebugFlag < 2) {
                arenaLo = (void*)(((u32)_stack_addr + 31) & ~31);
            }
        } else {
            arenaLo = (void*)0x80004000;
        }
    }
    OSSetMEM1ArenaLo(arenaLo);

    arenaHi = OS_SAVED_ARENA_HI;
    if (arenaHi == NULL) {
        arenaHi = BootInfo->arenaHi;
        if (arenaHi == NULL) {
            arenaHi = __ArenaHi;
        }
    }
    OSSetMEM1ArenaHi(arenaHi);

    mem2Lo = OS_SAVED_MEM2_ARENA_LO;
    if (mem2Lo != NULL) {
        if (((u32)__ArenaLo & OS_ARENA_REGION_MASK) == 0x10000000) {
            mem2Lo = __ArenaLo;
            if (BI2DebugFlag != NULL && *BI2DebugFlag < 2) {
                mem2Lo = (void*)(((u32)_stack_addr + 31) & ~31);
            }
        } else if ((u32)mem2Lo >= OS_MEM2_PHYS_BASE && (u32)mem2Lo < OS_MEM2_RESERVED_END) {
            mem2Lo = (void*)OS_MEM2_RESERVED_END;
        }
        OSSetMEM2ArenaLo(mem2Lo);
    }
    if (OS_SAVED_MEM2_ARENA_HI != NULL) {
        OSSetMEM2ArenaHi(OS_SAVED_MEM2_ARENA_HI);
    }

    __OSInitIPCBuffer();
    OSExceptionInit();
    __OSInitSystemCall();
    __OSInitAlarm();
    __OSModuleInit();
    __OSInterruptInit();
    __OSContextInit();
    __OSCacheInit();
    EXIInit();
    SIInit();
    __OSInitSram();
    __OSThreadInit();
    __OSInitAudioSystem();
    PPCMthid2(PPCMfhid2() & 0xBFFFFFFF);
    if (__OSInIPL == FALSE) {
        __OSInitMemoryProtection();
    }
    ReportOSInfo();
    OSReport("%s\n", __OSVersion);
    if (BI2DebugFlag != NULL && *BI2DebugFlag >= 2) {
        EnableMetroTRKInterrupts();
    }
    if (__OSInNandBoot == FALSE && __OSInReboot == FALSE) {
        ClearArena();
        ClearMEM2Arena();
    }
    OSEnableInterrupts();
    IPCCltInit();
    if (__OSInNandBoot == FALSE && __OSInReboot == FALSE) {
        __OSInitSTM();
        SCInit();
        while (SCCheckStatus() == 1) {
        }
        __OSInitNet();
    }
    if (__OSInIPL == FALSE) {
        if ((s32)OS_BOOT_PROGRAM_BYTE_A != 0x81) {
            if ((s32)OS_BOOT_PROGRAM_BYTE_A < 0x81) {
                (void)0;
            }
        } else {
            OSReport("OS ERROR: boot program is not for RVL target. Please use correct boot program.\n");
            OSPanic("OS.c", OS_LINE_BOOT_PROGRAM_A, "Failed to run app");
        }
        if ((s32)OS_BOOT_PROGRAM_BYTE_B != 0x81) {
            if ((s32)OS_BOOT_PROGRAM_BYTE_B < 0x81) {
                (void)0;
            }
        } else {
            OSReport("OS ERROR: apploader[D].img is not for RVL target. Please use correct apploader[D].img.\n");
            OSPanic("OS.c", OS_LINE_BOOT_PROGRAM_B, "Failed to run app");
        }
        {
            GXColor bgColor = { 0, 0, 255, 0 };
            GXColor fgColor = { 255, 255, 255, 0 };

            __OSGetIOSRev(&rev);
            if (rev.major != (OS_REQUIRED_IOS_WORD >> 16)
                || (rev.major == (OS_REQUIRED_IOS_WORD >> 16) && (OS_IOS_VERSION_WORD & 0xFFFFFF) < OS_REQUIRED_IOS_WORD)) {
                OSReport("OS ERROR: This firmware is an improper version for this SDK. Please use a correct Firmware.\n");
                OSFatal(fgColor, bgColor,
                        "\n\nERROR #002\nAn error has occurred.\nPress the Eject Button, remove the\nGame Disc, and turn off the power to \nthe console. \nPlease read the Wii Operations Manual \nfor further instructions.\n");
                OSPanic("OS.c", OS_LINE_IOS_VERSION, "Failed to run app");
            }
        }
        DVDInit();
        if (__OSIsGcam != FALSE) {
            OS_INQUIRY_RESULT = OS_INQUIRY_GCAM;
        } else if (OS_INQUIRY_RESULT == 0) {
            DCInvalidateRange(&InquiryDriveInfo, sizeof(InquiryDriveInfo));
            DVDInquiryAsync(&InquiryCommandBlock, &InquiryDriveInfo, InquiryCallback);
        }
        appType = __OSInIPL != FALSE ? 0x40 : OS_DISC_APP_TYPE;
        if (appType == OS_APP_TYPE_DIAG && __OSInReboot == FALSE && __DVDCheckDevice() == 0) {
            OSReturnToMenu();
        }
    }
    if (__OSInIPL == FALSE && __OSInNandBoot == FALSE) {
        __OSInitPlayTime();
    }
    if (__OSInIPL == FALSE && __OSInNandBoot == FALSE && __OSInReboot == FALSE) {
        __OSStartPlayRecord();
    }
}

/* The labels inside the asm vectors (map labels at 0x804CB1A0, 0x804CB1D0 and 0x804CB210 and their ends). */
extern u8 __OSDBINTSTART[];
extern u8 __OSDBINTEND[];
extern u8 __OSDBJUMPSTART[];
extern u8 __OSDBJUMPEND[];
extern u8 __OSEVStart[];
extern u32 __OSEVSetNumber[];
extern u8 __OSEVEnd[];
extern u32 __DBVECTOR[];

#define OS_EXCEPTION_COUNT 15
#define OS_DB_INTEGRATOR_ADDR ((u8*)0x80000060)
#define OS_NOP 0x60000000

/* The low-memory offset of the vector of each exception. */
static u32 ExceptionVectorOffset[OS_EXCEPTION_COUNT] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0x900, 0xC00, 0xD00, 0xF00, 0x1300, 0x1400, 0x1700,
};

/* 0x804CAF20 (0x280): copies the exception vector to every exception's low-memory address and points the handler table at the default handler. */
void OSExceptionInit(void)
{
    u8 exception;
    u32* setNumber = __OSEVSetNumber;
    u32 setNumberInsn = *setNumber;
    u8* vectorStart = __OSEVStart;
    u32 vectorSize = __OSEVEnd - vectorStart;
    u32 jumpSize;
    u32 vectorAddr;
    u8* dest;
    u32* nop;
    u32 offset;

    if (*(u32*)OS_DB_INTEGRATOR_ADDR == 0) {
        u32 integratorSize;

        DBPrintf("Installing OSDBIntegrator\n");
        integratorSize = __OSDBINTEND - __OSDBINTSTART;
        memcpy(OS_DB_INTEGRATOR_ADDR, __OSDBINTSTART, integratorSize);
        DCFlushRangeNoSync(OS_DB_INTEGRATOR_ADDR, integratorSize);
        asm { sync }
        ICInvalidateRange(OS_DB_INTEGRATOR_ADDR, integratorSize);
    }
    jumpSize = __OSDBJUMPEND - __OSDBJUMPSTART;
    for (exception = 0; exception < OS_EXCEPTION_COUNT; exception++) {
        if (BI2DebugFlag != NULL && *BI2DebugFlag >= 2 && __DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d commandeered by TRK\n", exception);
            continue;
        }
        *setNumber = setNumberInsn | exception;
        if (__DBIsExceptionMarked(exception)) {
            DBPrintf(">>> OSINIT: exception %d vectored to debugger\n", exception);
            memcpy(__DBVECTOR, __OSDBJUMPSTART, jumpSize);
        } else {
            nop = __DBVECTOR;
            for (offset = 0; offset < jumpSize; offset += 4) {
                *nop++ = OS_NOP;
            }
        }
        vectorAddr = ExceptionVectorOffset[exception] + 0x80000000;
        dest = (u8*)vectorAddr;
        memcpy(dest, vectorStart, vectorSize);
        DCFlushRangeNoSync(dest, vectorSize);
        asm { sync }
        ICInvalidateRange(dest, vectorSize);
    }
    OSExceptionTable = (OSExceptionHandler*)0x80003000;
    OSExceptionTable[0] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[1] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[2] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[3] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[4] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[5] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[6] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[7] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[8] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[9] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[10] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[11] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[12] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[13] = (OSExceptionHandler)OSDefaultExceptionHandler;
    OSExceptionTable[14] = (OSExceptionHandler)OSDefaultExceptionHandler;
    *setNumber = setNumberInsn;
    DBPrintf("Exceptions initialized...\n");
}

/* 0x804CB1A0 (0x24): returns to the interrupted code through the debugger integrator entry. */
__declspec(export) asm void __OSDBIntegrator(void)
{
    nofralloc
    li r5, 64
    mflr r3
    stw r3, 12(r5)
    lwz r3, 8(r5)
    oris r3, r3, 0x8000
    mtlr r3
    li r3, 48
    mtmsr r3
    blr
}

/* 0x804CB1D0 (0x4): jumps to the debugger vector. */
__declspec(export) asm void __OSDBJump(void)
{
    nofralloc
    bla 0x60
}

/* 0x804CB1E0 (0x14): installs the handler of CPU exception `exception` and returns the previous one. */
OSExceptionHandler __OSSetExceptionHandler(u8 exception, OSExceptionHandler handler)
{
    OSExceptionHandler old = OSExceptionTable[exception];

    OSExceptionTable[exception] = handler;
    return old;
}

/* 0x804CB200 (0x10): returns the handler of CPU exception `exception`. */
OSExceptionHandler __OSGetExceptionHandler(u8 exception)
{
    return OSExceptionTable[exception];
}

/* 0x804CB210 (0x9C): the common exception vector: saves the interrupted state in the context and enters the handler of the exception. */
__declspec(export) asm void OSExceptionVector(void)
{
    nofralloc
    mtspr 272, r4
    lwz r4, 0xC0(r0)
    stw r3, 12(r4)
    mfspr r3, 272
    stw r3, 16(r4)
    stw r5, 20(r4)
    lhz r3, 418(r4)
    ori r3, r3, 2
    sth r3, 418(r4)
    mfcr r3
    stw r3, 128(r4)
    mflr r3
    stw r3, 132(r4)
    mfctr r3
    stw r3, 136(r4)
    mfxer r3
    stw r3, 140(r4)
    mfspr r3, 26
    stw r3, 408(r4)
    mfspr r3, 27
    stw r3, 412(r4)
    mr r5, r3
    nop
    mfmsr r3
    ori r3, r3, 0x30
    mtspr 27, r3
    li r3, 0
    lwz r4, 0xD4(r0)
    rlwinm. r5, r5, 0, 30, 30
    bne dispatchToTable
    lis r5, OSDefaultExceptionHandler@ha
    addi r5, r5, OSDefaultExceptionHandler@l
    mtspr 26, r5
    rfi
dispatchToTable:
    rlwinm r5, r3, 2, 22, 29
    lwz r5, 0x3000(r5)
    mtspr 26, r5
    rfi
    nop
}

/* 0x804CB2B0 (0x58): the exception handler nobody replaced; saves the registers the vector did not and reports the exception. */
asm void OSDefaultExceptionHandler(register u8 exception, register OSContext* context)
{
    nofralloc
    stw r0, 0(context)
    stw r1, 4(context)
    stw r2, 8(context)
    stmw r6, 24(context)
    mfspr r0, 913
    stw r0, 424(context)
    mfspr r0, 914
    stw r0, 428(context)
    mfspr r0, 915
    stw r0, 432(context)
    mfspr r0, 916
    stw r0, 436(context)
    mfspr r0, 917
    stw r0, 440(context)
    mfspr r0, 918
    stw r0, 444(context)
    mfspr r0, 919
    stw r0, 448(context)
    mfdsisr r5
    mfdar r6
    stwu r1, -8(r1)
    b __OSUnhandledException
}

/* 0x804CB310 (0x54): enables paired singles and the locked-cache DMA and zeroes the quantization registers. */
asm void __OSPSInit(void)
{
    nofralloc
    stwu r1, -16(r1)
    mflr r0
    stw r0, 20(r1)
    bl PPCMfhid2
    oris r3, r3, 0xA000
    bl PPCMthid2
    bl ICFlashInvalidate
    sync
    li r3, 0
    mtspr 912, r3
    mtspr 913, r3
    mtspr 914, r3
    mtspr 915, r3
    mtspr 916, r3
    mtspr 917, r3
    mtspr 918, r3
    mtspr 919, r3
    lwz r0, 20(r1)
    mtlr r0
    addi r1, r1, 16
    blr
}

/* 0x804CB370 (0x10): returns the DI configuration byte. */
u32 __OSGetDIConfig(void)
{
    return OS_DI_CONFIG_REGISTER & 0xFF;
}

/* 0x804CB380 (0x10): prints the version string a library registers. */
void OSRegisterVersion(const char* version)
{
    OSReport("%s\n", version);
}

static char* DefaultGamename = "HAEA";

/* 0x804CB390 (0x84): returns the four-character game code of the running title in a static buffer. */
char* OSGetAppGamename(void)
{
    char* code = OS_DISC_GAME_CODE;

    if (__OSInIPL) {
        code = DefaultGamename;
    } else if (code[0] < '0' || (code[0] > '9' && 'A' > code[0]) || code[0] > 'Z') {
        code = OS_DISC_FALLBACK_GAME_CODE;
    }
    AppGamename[0] = code[0];
    AppGamename[1] = code[1];
    AppGamename[2] = code[2];
    AppGamename[3] = code[3];
    AppGamename[4] = 0;
    return AppGamename;
}

/* 0x804CB420 (0x20): returns the application type from the disc header, or 0x40 when started from the IPL. */
u8 OSGetAppType(void)
{
    if (__OSInIPL) {
        return 0x40;
    }
    return OS_DISC_APP_TYPE;
}
