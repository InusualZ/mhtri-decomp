/*
 * DVD/dvdfs.c - the SDK DVD file-system layer (path to entry number, open, read-async).
 *
 * RANGE. `.text` 0x804A5AB0-0x804A6410; `.data` 0x806186C8-0x80618800; `.sdata` 0x80793DD0-0x80793DE0; `.sbss`
 *   0x80795070-0x807950A0 (8 functions / 0x928 B).
 *   - the DOL string `dvdfs.c` (.sdata 0x80793DD8) is the `__FILE__` of the asserts in `DVDConvertPathToEntrynum` and
 *     `DVDReadAsyncPrio`; the right edge is the first function that reads the `dvd.c` string (`StampCommand`)
 *   - `.data` holds this file's three message strings; `.sbss` the FST pointers, `currentDirectory` and
 *     `__DVDThreadQueue` (read by `DVDInit` and two `dvd.c` functions)
 * FLAGS. `cflags_base` (`Wii/1.3`, -O4,p) in the BTE-tail SDK block of `configure.py`; `#pragma ipa file` (before every
 *   declaration) lets `DVDOpen` inline the later-defined `DVDGetCurrentDir` chain and keeps the target's function order.
 * NAMES. SDK names: `DVDOpen` (0x804A5E60), `DVDClose` (0x804A6120), `currentDirectory` (.sbss 0x80795070); the SDK's
 *   static `entryToPath` (0x804A6150) is spelled `DVDEntryToPath` (GUESS) because `ARC/arc.c`'s static owns the map
 *   name `entryToPath`; `isSame`, `myStrncpy`, `DVDConvertEntrynumToPath`, `DVDGetCurrentDir` are the inlined SDK
 *   helpers. The map rows of the three `.data` literals and `dvdfs.c` keep their generated `lbl_` names (the source
 *   spells them as literals).
 * RESIDUALS. none in code.
 *   `.sbss` 0x80795090..0x807950A0 is the padding before `DVD/dvd.c`'s 32-aligned `.sbss` (its `__DVDNumTmdBytes`
 *   is `aligned(32)`); a flip of this unit needs `align:32` on that unit's `.sbss` split line while `DVD/dvd.c` is
 *   not linked from source. 0x80795074 is the hole before the 8-aligned `__DVDThreadQueue`.
 *   `fsLowerChar` is MSL's inline `tolower` (`MSL_C/ctype.h` has no `tolower` yet).
 * SHAPES. `DVDEntryToPath` inlines into itself and into `DVDOpen` two levels deep (the third level is the `bl`).
 *   goto: owner exemption 2026-10-14, docs/pipeline.md 11 - `DVDConvertPathToEntrynum` keeps the SDK's one
 *   `goto next_hier` past its `return -1`; the search bound is re-read through `nextDir` on every iteration.
 */
#pragma ipa file
#include "DVD/dvdfs.h"
#include "DVD/dvd.h"
#include "MSL_C/ctype.h"
#include "OS/OS.h"
#include "OS/OSError.h"
#include "OS/OSThread.h"

/* One file-system table entry: a directory or a file. */
typedef struct FSTEntry {
    /* +0x00 */ u32 isDirAndStringOff;  /* top byte: directory flag; low 24 bits: name offset in the string table */
    /* +0x04 */ u32 parentOrPosition;   /* directory: parent entry; file: disc position */
    /* +0x08 */ u32 nextEntryOrLength;  /* directory: entry after its last child; file: length in bytes */
} FSTEntry; /* size: 0xC */

/* The cached (K0) view of a physical address. */
#define OSPhysicalToCached(paddr) ((void*)((u32)(paddr) + 0x80000000))

static OSBootInfo* BootInfo;
static FSTEntry* FstStart;
static char* FstStringStart;
static u32 MaxEntryNum;
OSThreadQueue __DVDThreadQueue;
static u32 currentDirectory;
BOOL __DVDLongFileNameFlag = TRUE;

static void cbForReadAsync(s32 result, DVDCommandBlock* block);
static inline BOOL DVDGetCurrentDir(char* path, u32 maxlen);

#define entryIsDir(i) ((FstStart[i].isDirAndStringOff & 0xFF000000) ? TRUE : FALSE)
#define stringOff(i) (FstStart[i].isDirAndStringOff & 0x00FFFFFF)
#define parentDir(i) (FstStart[i].parentOrPosition)
#define nextDir(i) (FstStart[i].nextEntryOrLength)
#define filePosition(i) (FstStart[i].parentOrPosition)
#define fileLength(i) (FstStart[i].nextEntryOrLength)

/* MSL's inline tolower over the active locale's table. */
static inline int fsLowerChar(int c) {
    return ctype_out_of_range(c) ? c : _current_locale.ctype->lower_map[c];
}

/* 0x804A5AB0 (0x30): reads the file-system table pointers the boot code left in low memory. */
void __DVDFSInit(void) {
    BootInfo = (OSBootInfo*)OSPhysicalToCached(0);
    FstStart = (FSTEntry*)BootInfo->FSTLocation;

    if (FstStart) {
        MaxEntryNum = FstStart[0].nextEntryOrLength;
        FstStringStart = (char*)&FstStart[MaxEntryNum];
    }
}

/* Compares one path element with an entry name, ignoring case. */
static inline BOOL isSame(const char* path, const char* string) {
    while (*string != '\0') {
        if (fsLowerChar(*path++) != fsLowerChar(*string++)) {
            return FALSE;
        }
    }

    if (*path == '/' || *path == '\0') {
        return TRUE;
    }

    return FALSE;
}

/* 0x804A5AE0 (0x308): returns the entry number of a path relative to the current directory, -1 when not found. */
s32 DVDConvertPathToEntrynum(const char* pathPtr) {
    const char* ptr;
    char* stringPtr;
    BOOL isDir;
    u32 length;
    u32 dirLookAt;
    u32 i;
    const char* origPathPtr = pathPtr;
    const char* extentionStart;
    BOOL illegal;
    BOOL extention;

    dirLookAt = currentDirectory;

    for (;;) {
        if (*pathPtr == '\0') {
            return (s32)dirLookAt;
        } else if (*pathPtr == '/') {
            dirLookAt = 0;
            pathPtr++;
            continue;
        } else if (*pathPtr == '.') {
            if (*(pathPtr + 1) == '.') {
                if (*(pathPtr + 2) == '/') {
                    dirLookAt = parentDir(dirLookAt);
                    pathPtr += 3;
                    continue;
                } else if (*(pathPtr + 2) == '\0') {
                    return (s32)parentDir(dirLookAt);
                }
            } else if (*(pathPtr + 1) == '/') {
                pathPtr += 2;
                continue;
            } else if (*(pathPtr + 1) == '\0') {
                return (s32)dirLookAt;
            }
        }

        if (__DVDLongFileNameFlag == 0) {
            extention = FALSE;
            illegal = FALSE;

            for (ptr = pathPtr; *ptr != '\0' && *ptr != '/'; ptr++) {
                if (*ptr == '.') {
                    if (ptr - pathPtr > 8 || extention == TRUE) {
                        illegal = TRUE;
                        break;
                    }
                    extention = TRUE;
                    extentionStart = ptr + 1;
                } else if (*ptr == ' ') {
                    illegal = TRUE;
                }
            }

            if (extention == TRUE && ptr - extentionStart > 3) {
                illegal = TRUE;
            }

            if (illegal) {
                OSPanic(__FILE__, 443,
                        "DVDConvertEntrynumToPath(possibly DVDOpen or DVDChangeDir or DVDOpenDir): specified directory "
                        "or file (%s) doesn't match standard 8.3 format. This is a temporary restriction and will be "
                        "removed soon\n",
                        origPathPtr);
            }
        } else {
            for (ptr = pathPtr; *ptr != '\0' && *ptr != '/'; ptr++) {
            }
        }

        isDir = (*ptr == '\0') ? FALSE : TRUE;
        length = (u32)(ptr - pathPtr);
        ptr = pathPtr;

        for (i = dirLookAt + 1; i < nextDir(dirLookAt); i = entryIsDir(i) ? nextDir(i) : (i + 1)) {
            if (entryIsDir(i) == FALSE && isDir == TRUE) {
                continue;
            }

            stringPtr = FstStringStart + stringOff(i);
            if (isSame(ptr, stringPtr) == TRUE) {
                goto next_hier;
            }
        }

        return -1;

    next_hier:
        if (!isDir) {
            return (s32)i;
        }

        dirLookAt = i;
        pathPtr += length + 1;
    }
}

/* 0x804A5DF0 (0x68): opens the file with entry number `entrynum`; FALSE for a directory or a bad number. */
BOOL DVDFastOpen(s32 entrynum, DVDFileInfo* fileInfo) {
    if (entrynum < 0 || entrynum >= MaxEntryNum || entryIsDir(entrynum)) {
        return FALSE;
    }

    fileInfo->startAddr = filePosition(entrynum) >> __DVDLayoutFormat;
    fileInfo->length = fileLength(entrynum);
    fileInfo->callback = NULL;
    fileInfo->cb.state = 0;

    return TRUE;
}

/* 0x804A5E60 (0x2B4): opens the file at `fileName`; FALSE (with a warning) when it does not exist. */
BOOL DVDOpen(const char* fileName, DVDFileInfo* fileInfo) {
    s32 entry;
    char currentDir[128];

    entry = DVDConvertPathToEntrynum(fileName);

    if (entry < 0) {
        DVDGetCurrentDir(currentDir, sizeof(currentDir));
        OSReport("Warning: DVDOpen(): file '%s' was not found under %s.\n", fileName, currentDir);
        return FALSE;
    }

    if (entryIsDir(entry)) {
        return FALSE;
    }

    fileInfo->startAddr = filePosition(entry) >> __DVDLayoutFormat;
    fileInfo->length = fileLength(entry);
    fileInfo->callback = NULL;
    fileInfo->cb.state = 0;

    return TRUE;
}

/* 0x804A6120 (0x24): closes `fileInfo`, cancelling its pending read. */
BOOL DVDClose(DVDFileInfo* fileInfo) {
    DVDCancel(&fileInfo->cb);
    return TRUE;
}

/* Copies at most `maxlen` characters of `src` (no terminator) and returns how many it copied. */
static inline u32 myStrncpy(char* dest, char* src, u32 maxlen) {
    u32 i = maxlen;

    while (i > 0 && *src != '\0') {
        *dest++ = *src++;
        i--;
    }

    return maxlen - i;
}

/* 0x804A6150 (0x1C8): writes the path of `entry` (without a trailing slash) into `path`; returns its length. */
static u32 DVDEntryToPath(u32 entry, char* path, u32 maxlen) {
    char* name;
    u32 loc;

    if (entry == 0) {
        return 0;
    }

    name = FstStringStart + stringOff(entry);
    loc = DVDEntryToPath(parentDir(entry), path, maxlen);

    if (loc == maxlen) {
        return loc;
    }

    path[loc++] = '/';
    loc += myStrncpy(path + loc, name, maxlen - loc);

    return loc;
}

/* Writes the path of `entrynum` into `path`, a directory with a trailing slash; FALSE when it does not fit. */
static inline BOOL DVDConvertEntrynumToPath(s32 entrynum, char* path, u32 maxlen) {
    u32 loc;

    loc = DVDEntryToPath((u32)entrynum, path, maxlen);

    if (loc == maxlen) {
        path[maxlen - 1] = '\0';
        return FALSE;
    }

    if (entryIsDir(entrynum)) {
        if (loc == maxlen - 1) {
            path[loc] = '\0';
            return FALSE;
        }
        path[loc++] = '/';
    }

    path[loc] = '\0';
    return TRUE;
}

/* Writes the current directory's path into `path`. */
static inline BOOL DVDGetCurrentDir(char* path, u32 maxlen) {
    return DVDConvertEntrynumToPath((s32)currentDirectory, path, maxlen);
}

/* 0x804A6320 (0xD0): queues a read of `length` bytes at `offset` in the file into `addr`. */
/* untyped: byte range the read fills */
BOOL DVDReadAsyncPrio(DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, DVDCallback callback, s32 prio) {
    if (!(0 <= offset && offset <= fileInfo->length)) {
        OSPanic(__FILE__, 823, "DVDReadAsync(): specified area is out of the file  ");
    }

    if (!(0 <= offset + length && offset + length < fileInfo->length + 32)) {
        OSPanic(__FILE__, 829, "DVDReadAsync(): specified area is out of the file  ");
    }

    fileInfo->callback = callback;
    DVDReadAbsAsyncPrio(&fileInfo->cb, addr, length, (s32)(fileInfo->startAddr + (offset >> 2)), cbForReadAsync, prio);

    return TRUE;
}

/* 0x804A63F0 (0x18): forwards a finished read to the file's callback. */
static void cbForReadAsync(s32 result, DVDCommandBlock* block) {
    DVDFileInfo* fileInfo = (DVDFileInfo*)block;

    if (fileInfo->callback) {
        fileInfo->callback(result, fileInfo);
    }
}
