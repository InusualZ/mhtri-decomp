/*
 * ARC/arc.c - the ARC archive library: archive handle init, file open by path or entry number, path lookup and the
 *    directory walk.
 *
 * RANGE. .text 0x8046D9F0-0x8046E3D0 (12 functions, 0x9E0 B); .data 0x8060F868-0x8060F8D8; .sdata
 *    0x80793D08-0x80793D10.  Cut from the old ARC/AX block 0x8046D9F0-0x80474CB0; the left edge is the end of
 *    `AI_SDK/ai.c`, the right edge 0x8046E3D0 is `AXInit`.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the ARC* names are the map's (the dump agrees); `entryToPath` (0x8046E010, static) is a GUESS from what it
 *    does; the file name `arc.c` is a GUESS from the library.
 * EVIDENCE. `.data` 0x8060F868 (0x22 B) and 0x8060F88C (0x46 B) are the message strings of `ARCInitHandle` and
 *    `ARCOpen`; `.sdata` 0x80793D08 (6 B) is read by `ARCInitHandle` only; the twelve ARC* rows are contiguous
 *    in the map and call only each other, and the next reader of a new data object is `AXInit` (the AX build
 *    string).
 * RESIDUALS. ARCConvertPathToEntrynum 0x8046DD90: the entry search is a bottom-tested loop in the target (a "." entry
 *    re-enters it below the bound check) and ours tests at the top; ARCOpen 0x8046DA90: the target inlines `entryToPath`
 *    to a third level (684 B) where `inline_depth(2)` stops at two (436 B); ARCReadDir: register colouring.
 * SHAPES. the file system table is an array of 12-byte records; `entryToPath` is a recursive static inline helper and
 *    `#pragma inline_depth(2)` reproduces the two inlined levels of its own body.
 */

#include "types.h"

#include "ARC/arc.h"
#include "MSL_C/locale.h"
#include "OS/OSError.h"

#pragma inline_depth(2)

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

/* 0x8046DD90 (0x27C): resolves a path, relative to the current directory unless it starts with '/', to an entry number. */
s32 ARCConvertPathToEntrynum(ARCHandle* handle, const char* pathPtr)
{
    u32 dirLookAt = handle->currDir;
    ARCEntry* fst = handle->fstStart;
    u32 i;
    u32 length;
    const char* end;
    const char* name;
    const char* path;
    BOOL isDir;
    BOOL same;
    int c1;
    int c2;

    while (TRUE) {
        if (*pathPtr == '\0') {
            return dirLookAt;
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
                    return fst[dirLookAt].offsetOrParent;
                }
            } else if (pathPtr[1] == '/') {
                pathPtr += 2;
                continue;
            } else if (pathPtr[1] == '\0') {
                return dirLookAt;
            }
        }
        for (end = pathPtr;; end++) {
            if (*end == '\0') {
                isDir = FALSE;
                break;
            }
            if (*end == '/') {
                isDir = TRUE;
                break;
            }
        }
        length = end - pathPtr;
        i = dirLookAt + 1;
        while (TRUE) {
            if (i >= fst[dirLookAt].lengthOrNext) {
                return -1;
            }
            if (ARC_ENTRY_IS_DIR(&fst[i]) || isDir != TRUE) {
                name = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(&fst[i]);
                if (name[0] == '.' && name[1] == '\0') {
                    i++;
                    continue;
                }
                path = pathPtr;
                for (;;) {
                    if (*name == '\0') {
                        same = (*path == '/' || *path == '\0');
                        break;
                    }
                    c1 = arcToLower(*name++);
                    c2 = arcToLower(*path++);
                    if (c1 != c2) {
                        same = FALSE;
                        break;
                    }
                }
                if (same == TRUE) {
                    if (isDir == FALSE) {
                        return i;
                    }
                    dirLookAt = i;
                    pathPtr += length + 1;
                    break;
                }
            }
            if (ARC_ENTRY_IS_DIR(&fst[i])) {
                i = fst[i].lengthOrNext;
            } else {
                i++;
            }
        }
    }
}

/* Writes the absolute path of an entry into `path`, at most `maxLength` characters, and returns its length. */
static inline u32 entryToPath(ARCHandle* handle, u32 entry, char* path, u32 maxLength)
{
    ARCEntry* fst = handle->fstStart;
    const char* name;
    u32 length;
    u32 remaining;

    if (entry == 0) {
        return 0;
    }
    name = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(&fst[entry]);
    length = entryToPath(handle, fst[entry].offsetOrParent, path, maxLength);
    if (length == maxLength) {
        return length;
    }
    path[length++] = '/';
    remaining = maxLength - length;
    {
        char* dst = path + length;

        while (remaining != 0 && *name != '\0') {
            *dst++ = *name++;
            remaining--;
        }
    }
    return length + (maxLength - length - remaining);
}

/* 0x8046DA90 (0x2AC): opens a file by path; when it is missing, reports the name and the current directory. */
BOOL ARCOpen(ARCHandle* handle, const char* fileName, ARCFileInfo* fileInfo)
{
    ARCEntry* fst = handle->fstStart;
    s32 entry = ARCConvertPathToEntrynum(handle, fileName);
    char buffer[ARC_PATH_MAX];
    u32 length;

    if (entry < 0) {
        length = entryToPath(handle, handle->currDir, buffer, ARC_PATH_MAX);
        if (length == ARC_PATH_MAX) {
            buffer[ARC_PATH_MAX - 1] = '\0';
        } else if (ARC_ENTRY_IS_DIR(&handle->fstStart[handle->currDir])) {
            if (length == ARC_PATH_MAX - 1) {
                buffer[length] = '\0';
            } else {
                buffer[length++] = '/';
                buffer[length] = '\0';
            }
        } else {
            buffer[length] = '\0';
        }
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
    u32 location = dir->location;
    ARCHandle* handle = dir->handle;
    ARCEntry* fst = handle->fstStart;
    ARCEntry* entry = &fst[location];
    char* name;

    while (TRUE) {
        if (location <= dir->entryNum || dir->next <= location) {
            return FALSE;
        }
        dirent->handle = handle;
        dirent->entryNum = location;
        dirent->isDir = ARC_ENTRY_IS_DIR(entry);
        name = handle->fstStringStart + ARC_ENTRY_NAME_OFFSET(entry);
        dirent->name = name;
        if (name[0] == '.' && name[1] == '\0') {
            entry++;
            location++;
            continue;
        }
        break;
    }
    if (ARC_ENTRY_IS_DIR(&fst[location])) {
        location = fst[location].lengthOrNext;
    } else {
        location = location + 1;
    }
    dir->location = location;
    return TRUE;
}

/* 0x8046E3C0 (0x8): closes a directory. */
BOOL ARCCloseDir(ARCDir* dir)
{
    return TRUE;
}
