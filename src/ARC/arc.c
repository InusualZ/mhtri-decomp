/*
 * ARC/arc.c - the ARC archive library: archive handle init, file open by path or entry number, path lookup and the
 *    directory walk.
 *
 * RANGE. .text 0x8046D9F0-0x8046E3D0 (12 functions, 0x9E0 B); .data 0x8060F868-0x8060F8D8; .sdata
 *    0x80793D08-0x80793D10.  Cut from the old ARC/AX block 0x8046D9F0-0x80474CB0; the left edge is the end of
 *    `AI_SDK/ai.c`, the right edge 0x8046E3D0 is `AXInit`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the ARC* names are the map's (the dump agrees); `entryToPath` (0x8046E010, static) and the inlined statics
 *    `isSame`, `myStrncpy`, `ARCConvertEntrynumToPath` and `ARCGetCurrentDir` are GUESSes after the SDK's `dvdfs.c`
 *    spellings of the same helpers; `findEntry` is a GUESS from what it does; the file name `arc.c` is a GUESS from
 *    the library.
 * EVIDENCE. `.data` 0x8060F868 (0x22 B) and 0x8060F88C (0x46 B) are the message strings of `ARCInitHandle` and
 *    `ARCOpen`; `.sdata` 0x80793D08 (6 B) is read by `ARCInitHandle` only; the twelve ARC* rows are contiguous
 *    in the map and call only each other, and the next reader of a new data object is `AXInit` (the AX build
 *    string).
 * RESIDUALS. ARCConvertPathToEntrynum 0x8046DD90: the target is the SDK's `goto` shape (a "." entry re-enters the
 *    search below its bound check, a match jumps out of the search to the path loop); rule 8 forbids it, and the
 *    `findEntry` helper that replaces it leaves `li r9,-1; cmpwi r9,0; bge` for the inlined `return -1`, a `bne`+`b`
 *    pair where the target has `beq` to the match, and r28-r31 permuted (652 B vs 636 B; the goto shape measures 636 B
 *    and byte-identical code).
 * SHAPES. `#pragma ipa file` (it must precede every declaration) compiles the file as a whole: `ARCOpen` keeps its
 *    source position before `ARCFastOpen` and still inlines `ARCGetCurrentDir` -> `ARCConvertEntrynumToPath` ->
 *    `entryToPath`, which is defined after it, three levels deep before its own `bl entryToPath`; the out-of-line
 *    `entryToPath` is itself plus two inlined levels. `ARCConvertEntrynumToPath` reads `fstStart` into a local before
 *    the path walk, so the inlined walk's load is reused. The file system table is an array of 12-byte records.
 */

#pragma ipa file

#include "types.h"

#include "ARC/arc.h"
#include "MSL_C/locale.h"
#include "OS/OSError.h"

static BOOL ARCGetCurrentDir(ARCHandle* handle, char* path, u32 maxLength);

#define ARC_MAGIC 0x55AA382D
#define ARC_PATH_MAX 128

#define ARC_ENTRY_IS_DIR(entry) (((entry)->typeAndName & 0xFF000000) != 0)
#define ARC_ENTRY_NAME_OFFSET(entry) ((entry)->typeAndName & 0x00FFFFFF)

static inline int out_of_range(int c)
{
    return c < 0 || c > 255;
}

/* Folds a character to lower case through the active locale tables. */
static inline int arcToLower(int c)
{
    return out_of_range(c) ? c : (int)_current_locale.ctype->lower_map[c];
}

/* 0x8046D9F0 (0xA0): sets up a handle over an archive image and checks its magic. */
/* untyped: caller-owned archive image */
BOOL ARCInitHandle(void* bin, ARCHandle* handle)
{
    ARCHeader* header = (ARCHeader*)bin;
    ARCEntry* fst;
    u32 entryNum;

    if (header->magic != ARC_MAGIC) {
        OSPanic("arc.c", 74, "ARCInitHandle: bad archive format");
    }
    handle->archiveStartAddr = header;
    fst = (ARCEntry*)((u8*)header + header->fstOffset);
    handle->fstStart = fst;
    handle->fileStart = (u8*)header + header->fileOffset;
    entryNum = fst->lengthOrNext;
    handle->entryNum = entryNum;
    handle->fstStringStart = (char*)(fst + entryNum);
    handle->fstLength = header->fstLength;
    handle->currDir = 0;
    return TRUE;
}

/* 0x8046DA90 (0x2AC): opens a file by path; when it is missing, reports the name and the current directory. */
BOOL ARCOpen(ARCHandle* handle, const char* fileName, ARCFileInfo* fileInfo)
{
    ARCEntry* fst = handle->fstStart;
    s32 entry = ARCConvertPathToEntrynum(handle, fileName);
    char buffer[ARC_PATH_MAX];

    if (entry < 0) {
        ARCGetCurrentDir(handle, buffer, ARC_PATH_MAX);
        OSReport("Warning: ARCOpen(): file '%s' was not found under %s in the archive.\n", fileName, buffer);
        return FALSE;
    }
    if (ARC_ENTRY_IS_DIR(&fst[entry])) {
        return FALSE;
    }
    fileInfo->handle = handle;
    fileInfo->startOffset = fst[entry].offsetOrParent;
    fileInfo->length = fst[entry].lengthOrNext;
    return TRUE;
}

/* 0x8046DD40 (0x50): opens a file by entry number. */
BOOL ARCFastOpen(ARCHandle* handle, s32 entrynum, ARCFileInfo* fileInfo)
{
    ARCEntry* fst = handle->fstStart;

    if (entrynum < 0 || (u32)entrynum >= handle->entryNum || ARC_ENTRY_IS_DIR(&fst[entrynum])) {
        return FALSE;
    }
    fileInfo->handle = handle;
    fileInfo->startOffset = fst[entrynum].offsetOrParent;
    fileInfo->length = fst[entrynum].lengthOrNext;
    return TRUE;
}

/* Compares one path component with an entry name, ignoring case; the component ends at '/' or the end. */
static BOOL isSame(const char* path, const char* string)
{
    while (*string != '\0') {
        if (arcToLower(*path++) != arcToLower(*string++)) {
            return FALSE;
        }
    }
    if (*path == '/' || *path == '\0') {
        return TRUE;
    }
    return FALSE;
}

/* Returns the entry named by the first component of `ptr` inside directory `dirLookAt`, skipping "." entries, or -1. */
static s32 findEntry(ARCHandle* handle, ARCEntry* fst, u32 dirLookAt, const char* ptr, BOOL isDir)
{
    u32 i;
    char* stringPtr;

    for (i = dirLookAt + 1; i < fst[dirLookAt].lengthOrNext; i = ARC_ENTRY_IS_DIR(&fst[i]) ? fst[i].lengthOrNext : i + 1) {
        for (;;) {
            if (ARC_ENTRY_IS_DIR(&fst[i]) == FALSE && isDir == TRUE) {
                break;
            }
            stringPtr = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(&fst[i]);
            if (stringPtr[0] == '.' && stringPtr[1] == '\0') {
                i++;
                continue;
            }
            if (isSame(ptr, stringPtr) == TRUE) {
                return (s32)i;
            }
            break;
        }
    }
    return -1;
}

/* 0x8046DD90 (0x27C): resolves a path, relative to the current directory unless it starts with '/', to an entry number. */
s32 ARCConvertPathToEntrynum(ARCHandle* handle, const char* pathPtr)
{
    const char* ptr;
    BOOL isDir;
    u32 length;
    u32 dirLookAt;
    u32 i;
    ARCEntry* fst;

    dirLookAt = handle->currDir;
    fst = handle->fstStart;

    while (TRUE) {
        if (*pathPtr == '\0') {
            return (s32)dirLookAt;
        } else if (*pathPtr == '/') {
            dirLookAt = 0;
            pathPtr++;
            continue;
        } else if (*pathPtr == '.') {
            if (pathPtr[1] == '.') {
                if (pathPtr[2] == '/') {
                    dirLookAt = fst[dirLookAt].offsetOrParent;
                    pathPtr += 3;
                    continue;
                } else if (pathPtr[2] == '\0') {
                    return (s32)fst[dirLookAt].offsetOrParent;
                }
            } else if (pathPtr[1] == '/') {
                pathPtr += 2;
                continue;
            } else if (pathPtr[1] == '\0') {
                return (s32)dirLookAt;
            }
        }
        for (ptr = pathPtr; *ptr != '\0' && *ptr != '/'; ptr++) {
        }
        isDir = (*ptr == '\0') ? FALSE : TRUE;
        length = (u32)(ptr - pathPtr);
        i = (u32)findEntry(handle, fst, dirLookAt, pathPtr, isDir);
        if ((s32)i < 0) {
            return -1;
        }
        if (!isDir) {
            return (s32)i;
        }
        dirLookAt = i;
        pathPtr += length + 1;
    }
}

/* Copies at most `maxLength` characters of `src` and returns how many were copied. */
static u32 myStrncpy(char* dst, const char* src, u32 maxLength)
{
    u32 i = maxLength;

    while (i > 0 && *src != '\0') {
        *dst++ = *src++;
        i--;
    }
    return maxLength - i;
}

/* 0x8046E010 (0x1D0): writes the absolute path of an entry into `path`, at most `maxLength` characters, and returns its length. */
static u32 entryToPath(ARCHandle* handle, u32 entry, char* path, u32 maxLength)
{
    ARCEntry* fst = handle->fstStart;
    const char* name;
    u32 length;

    if (entry == 0) {
        return 0;
    }
    name = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(&fst[entry]);
    length = entryToPath(handle, fst[entry].offsetOrParent, path, maxLength);
    if (length == maxLength) {
        return length;
    }
    path[length++] = '/';
    length += myStrncpy(path + length, name, maxLength - length);
    return length;
}

/* Writes the terminated path of an entry, with a trailing '/' for a directory; FALSE when it does not fit. */
static BOOL ARCConvertEntrynumToPath(ARCHandle* handle, s32 entrynum, char* path, u32 maxLength)
{
    ARCEntry* fst = handle->fstStart;
    u32 length = entryToPath(handle, (u32)entrynum, path, maxLength);

    if (length == maxLength) {
        path[maxLength - 1] = '\0';
        return FALSE;
    }
    if (ARC_ENTRY_IS_DIR(&fst[entrynum])) {
        if (length == maxLength - 1) {
            path[length] = '\0';
            return FALSE;
        }
        path[length++] = '/';
    }
    path[length] = '\0';
    return TRUE;
}

/* Writes the terminated path of the current directory. */
static BOOL ARCGetCurrentDir(ARCHandle* handle, char* path, u32 maxLength)
{
    return ARCConvertEntrynumToPath(handle, (s32)handle->currDir, path, maxLength);
}

/* 0x8046E1E0 (0x14): returns the address of a file's data in memory. */
/* untyped: caller-owned file data */
void* ARCGetStartAddrInMem(ARCFileInfo* fileInfo)
{
    return (u8*)fileInfo->handle->archiveStartAddr + fileInfo->startOffset;
}

/* 0x8046E200 (0x8): returns the length of a file. */
u32 ARCGetLength(ARCFileInfo* fileInfo)
{
    return fileInfo->length;
}

/* 0x8046E210 (0x8): closes a file. */
BOOL ARCClose(ARCFileInfo* fileInfo)
{
    return TRUE;
}

/* 0x8046E220 (0x58): makes a directory the current directory. */
BOOL ARCChangeDir(ARCHandle* handle, const char* dirName)
{
    s32 entry = ARCConvertPathToEntrynum(handle, dirName);
    ARCEntry* fst = handle->fstStart;

    if (entry < 0 || !ARC_ENTRY_IS_DIR(&fst[entry])) {
        return FALSE;
    }
    handle->currDir = entry;
    return TRUE;
}

/* 0x8046E280 (0x7C): opens a directory for listing. */
BOOL ARCOpenDir(ARCHandle* handle, const char* dirName, ARCDir* dir)
{
    s32 entry = ARCConvertPathToEntrynum(handle, dirName);
    ARCEntry* fst = handle->fstStart;

    if (entry < 0 || !ARC_ENTRY_IS_DIR(&fst[entry])) {
        return FALSE;
    }
    dir->entryNum = entry;
    dir->handle = handle;
    dir->location = entry + 1;
    dir->next = fst[entry].lengthOrNext;
    return TRUE;
}

/* 0x8046E300 (0xBC): reads the next directory entry, skipping "." entries. */
BOOL ARCReadDir(ARCDir* dir, ARCDirEntry* dirent)
{
    u32 location;
    ARCEntry* fst;
    ARCHandle* handle;

    handle = dir->handle;
    fst = handle->fstStart;
    location = dir->location;
    while (TRUE) {
        if (location <= dir->entryNum || dir->next <= location) {
            return FALSE;
        }
        dirent->handle = handle;
        dirent->entryNum = location;
        dirent->isDir = ARC_ENTRY_IS_DIR(&fst[location]);
        dirent->name = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(&fst[location]);
        if (dirent->name[0] == '.' && dirent->name[1] == '\0') {
            location++;
            continue;
        }
        break;
    }
    dir->location = ARC_ENTRY_IS_DIR(&fst[location]) ? fst[location].lengthOrNext : location + 1;
    return TRUE;
}

/* 0x8046E3C0 (0x8): closes a directory. */
BOOL ARCCloseDir(ARCDir* dir)
{
    return TRUE;
}
