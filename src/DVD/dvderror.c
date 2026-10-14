/*
 * DVD/dvderror.c - the SDK DVD error log: `__DVDStoreErrorCode` records an error with the drive registers and
 * appends it to the NAND file `/shared2/test2/dvderror.dat` through a chain of NAND callbacks.
 *
 * RANGE. `.text` 0x804AB2C0-0x804ABCE0 (14 functions / 0x9DC B); `.data` 0x80618C10-0x80618C40 (the two path
 *   strings); `.bss` 0x80746720-0x80746980 (`NandInfo`, `NandCb`, `__ErrorInfo`, `ErrorInfoBuffer`); `.sbss`
 *   0x80795138-0x80795148 (`ExistFlag`, `NextOffset`, `Callback`).
 *   - the file is a ring: record 0 is an index whose `nextOffset` names the slot the next record goes to, the
 *     following records hold errors; `__ErrorInfo` is filled by `dvd.c` and written here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the unit name is a GUESS (named for the error file the strings name). GUESS: `cbForNandSeekRecord`,
 *   `cbForNandWriteIndex`, `cbForNandSeekIndexWrite`, `cbForNandReadIndex`, `cbForNandSeekHead`, `cbForNandSeekIndex`
 *   (named for the NAND step each one follows), `ExistFlag` (set when the create finds the file), `NextOffset` (the
 *   ring slot read from the index); the remaining `cbFor*` names follow the SDK scheme.
 * RESIDUALS. none in `.text`; relocations differ by name only: the two `.data` path strings keep the map's
 *   generated labels (ours are string literals), and `NandInfo` at `.bss` offset 0 is referenced through the
 *   section symbol.
 * SHAPES. `.bss` is defined in map order (`NandInfo`, `NandCb`, `__ErrorInfo`, `ErrorInfoBuffer`, the last two
 *   aligned 32); `.sbss` is defined `Callback`, `NextOffset`, `ExistFlag` because MWCC emits it reversed. A NAND
 *   request that fails to start calls its own completion with -1 (inlined), except in `cbForNandCreateDir` and the
 *   two register callbacks, which call `cbForNandClose(-1)`; `cbForNandOpen` on a new file calls
 *   `cbForNandSeekHead(0)` directly; the time divides by `OS_BUS_CLOCK / 4` inline.
 */

#include "types.h"
#include "DVD/dvderror.h"
#include "DVD/dvd_broadway.h"
#include "NAND/nand.h"
#include "OS/OS.h"
#include "OS/OSCache.h"
#include "OS/OSTime.h"

static NANDFileInfo NandInfo;
static NANDCommandBlock NandCb;
DVDErrorInfo __ErrorInfo __attribute__((aligned(32)));
static DVDErrorInfo ErrorInfoBuffer __attribute__((aligned(32)));
static DVDCBCallback Callback;
static u32 NextOffset;
static BOOL ExistFlag;

/* 0x804AB2C0 (0x2C): Reports the end of the store to the caller: 1 on success, 2 on failure. */
static void cbForNandClose(s32 result, NANDCommandBlock* block) {
    if (Callback != NULL) {
        Callback(result == 0 ? 1 : 2, NULL);
    }
}

/* 0x804AB2F0 (0x5C): Closes the error file after the record is written. */
static void cbForNandWrite(s32 result, NANDCommandBlock* block) {
    if (NANDCloseAsync(&NandInfo, cbForNandClose, &NandCb) != 0) {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804AB350 (0xFC): Writes the error record at the ring slot the seek reached. */
static void cbForNandSeekRecord(s32 result, NANDCommandBlock* block) {
    if (result == (NextOffset + 1) * sizeof(DVDErrorInfo)) {
        if (NextOffset == 0) {
            __ErrorInfo.nextOffset = (NextOffset + 1) % 7;
        }
        DCFlushRange(&__ErrorInfo, sizeof(DVDErrorInfo));
        if (NANDWriteAsync(&NandInfo, &__ErrorInfo, sizeof(DVDErrorInfo), cbForNandWrite, &NandCb) != 0) {
            cbForNandWrite(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804AB450 (0x154): Seeks to the record's ring slot after the index is written. */
static void cbForNandWriteIndex(s32 result, NANDCommandBlock* block) {
    if (result == sizeof(DVDErrorInfo)) {
        /* whence 0: from the start of the file */
        if (NANDSeekAsync(&NandInfo, (NextOffset + 1) * sizeof(DVDErrorInfo), 0, cbForNandSeekRecord, &NandCb) != 0) {
            cbForNandSeekRecord(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804AB5B0 (0xBC): Advances the index's ring slot and writes the index back. */
static void cbForNandSeekIndexWrite(s32 result, NANDCommandBlock* block) {
    if (result == sizeof(DVDErrorInfo)) {
        ErrorInfoBuffer.nextOffset = (ErrorInfoBuffer.nextOffset + 1) % 7;
        if (NANDWriteAsync(&NandInfo, &ErrorInfoBuffer, sizeof(DVDErrorInfo), cbForNandWriteIndex, &NandCb) != 0) {
            cbForNandWriteIndex(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804AB670 (0xE8): Takes the ring slot from the index just read, or writes the record when the read failed. */
static void cbForNandReadIndex(s32 result, NANDCommandBlock* block) {
    if (result == sizeof(DVDErrorInfo)) {
        NextOffset = ErrorInfoBuffer.nextOffset;
        if (NANDSeekAsync(&NandInfo, sizeof(DVDErrorInfo), 0, cbForNandSeekIndexWrite, &NandCb) != 0) {
            cbForNandSeekIndexWrite(-1, NULL);
        }
    } else {
        __ErrorInfo.nextOffset = 1;
        if (NANDWriteAsync(&NandInfo, &__ErrorInfo, sizeof(DVDErrorInfo), cbForNandWrite, &NandCb) != 0) {
            cbForNandWrite(-1, NULL);
        }
    }
}

/* 0x804AB760 (0xA0): Starts a new ring at slot 0 and writes the index at the head of the file. */
static void cbForNandSeekHead(s32 result, NANDCommandBlock* block) {
    if (result == 0) {
        NextOffset = 0;
        __ErrorInfo.nextOffset = 1;
        if (NANDWriteAsync(&NandInfo, &ErrorInfoBuffer, sizeof(DVDErrorInfo), cbForNandWriteIndex, &NandCb) != 0) {
            cbForNandWriteIndex(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804AB800 (0x100): Reads the index, or rewinds to the head of the file when the seek failed. */
static void cbForNandSeekIndex(s32 result, NANDCommandBlock* block) {
    if (result == sizeof(DVDErrorInfo)) {
        if (NANDReadAsync(&NandInfo, &ErrorInfoBuffer, sizeof(DVDErrorInfo), cbForNandReadIndex, &NandCb) != 0) {
            cbForNandReadIndex(-1, NULL);
        }
    } else {
        if (NANDSeekAsync(&NandInfo, 0, 0, cbForNandSeekHead, &NandCb) != 0) {
            cbForNandSeekHead(-1, NULL);
        }
    }
}

/* 0x804AB900 (0x11C): Seeks to the index of an existing file, or starts a new ring in a new one. */
static void cbForNandOpen(s32 result, NANDCommandBlock* block) {
    if (result == 0) {
        if (ExistFlag) {
            if (NANDSeekAsync(&NandInfo, sizeof(DVDErrorInfo), 0, cbForNandSeekIndex, &NandCb) != 0) {
                cbForNandSeekIndex(-1, NULL);
            }
        } else {
            cbForNandSeekHead(0, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804ABA20 (0xA0): Opens the error file once it exists, noting whether it was already there. */
static void cbForNandCreate(s32 result, NANDCommandBlock* block) {
    /* -6: the file already exists */
    if (result == 0 || result == -6) {
        if (result == -6) {
            ExistFlag = TRUE;
        }
        /* mode 3: read and write */
        if (NANDPrivateOpenAsync("/shared2/test2/dvderror.dat", &NandInfo, 3, cbForNandOpen, &NandCb) != 0) {
            cbForNandOpen(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804ABAC0 (0x94): Creates the error file once its directory exists. */
static void cbForNandCreateDir(s32 result, NANDCommandBlock* block) {
    /* -6: the directory already exists */
    if (result == 0 || result == -6) {
        /* permission 0x3F: read and write for owner, group and other */
        if (NANDPrivateCreateAsync("/shared2/test2/dvderror.dat", 0x3F, 0, cbForNandCreate, &NandCb) != 0) {
            cbForNandClose(-1, NULL);
        }
    } else {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804ABB60 (0x90): Stores the control register in the record and creates the error directory. */
static void cbForPrepareControlRegister(u32 intType) {
    if (intType == 1) {
        __ErrorInfo.control = DVDLowGetControlRegister();
    } else {
        __ErrorInfo.control = 0xFFFFFFFF;
    }
    if (NANDPrivateCreateDirAsync("/shared2/test2", 0x3F, 0, cbForNandCreateDir, &NandCb) != 0) {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804ABBF0 (0x78): Stores the status register in the record and requests the control register. */
static void cbForPrepareStatusRegister(u32 intType) {
    if (intType == 1) {
        __ErrorInfo.status = DVDLowGetStatusRegister();
    } else {
        __ErrorInfo.status = 0xFFFFFFFF;
    }
    if (!DVDLowPrepareControlRegister(cbForPrepareControlRegister)) {
        cbForNandClose(-1, NULL);
    }
}

/* 0x804ABC70 (0x68): Stores `error` and the time in the record and starts writing it to NAND. */
void __DVDStoreErrorCode(u32 error, DVDCBCallback callback) {
    __ErrorInfo.error = error;
    __ErrorInfo.dateTime = OSGetTime() / (OS_BUS_CLOCK / 4);
    Callback = callback;
    DVDLowPrepareStatusRegister(cbForPrepareStatusRegister);
}
