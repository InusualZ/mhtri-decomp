/*
 * ARC/arc.h - the ARC archive types and the declarations of the symbols owned by `ARC/arc.c`.
 */
#ifndef ARC_ARC_H
#define ARC_ARC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x10; the archive header. */
typedef struct ARCHeader {
    /* +0x00 */ u32 magic;
    /* +0x04 */ u32 fstOffset;
    /* +0x08 */ u32 fstLength;
    /* +0x0C */ u32 fileOffset;
} ARCHeader;

/* size: 0xC; one file system table record. */
typedef struct ARCEntry {
    /* +0x0 */ u32 typeAndName; /* high byte: nonzero for a directory; low 24 bits: name offset in the string table */
    /* +0x4 */ u32 offsetOrParent; /* file: data offset; directory: parent entry */
    /* +0x8 */ u32 lengthOrNext; /* file: length; directory: first entry after the directory */
} ARCEntry;

/* size: 0x1C; an opened archive. */
typedef struct ARCHandle {
    /* +0x00 */ ARCHeader* archiveStartAddr;
    /* +0x04 */ ARCEntry* fstStart;
    /* +0x08 */ void* fileStart; /* untyped: the start of the file data */
    /* +0x0C */ u32 entryNum;
    /* +0x10 */ char* fstStringStart;
    /* +0x14 */ u32 fstLength;
    /* +0x18 */ u32 currDir;
} ARCHandle;

/* size: 0xC; an opened file. */
typedef struct ARCFileInfo {
    /* +0x0 */ ARCHandle* handle;
    /* +0x4 */ u32 startOffset;
    /* +0x8 */ u32 length;
} ARCFileInfo;

/* size: 0x10; a directory being walked. */
typedef struct ARCDir {
    /* +0x00 */ ARCHandle* handle;
    /* +0x04 */ u32 entryNum;
    /* +0x08 */ u32 location;
    /* +0x0C */ u32 next;
} ARCDir;

/* size: 0x10; one directory listing entry. */
typedef struct ARCDirEntry {
    /* +0x00 */ ARCHandle* handle;
    /* +0x04 */ u32 entryNum;
    /* +0x08 */ BOOL isDir;
    /* +0x0C */ char* name;
} ARCDirEntry;

/* untyped: caller-owned archive image */
BOOL ARCInitHandle(void* bin, ARCHandle* handle);
BOOL ARCOpen(ARCHandle* handle, const char* fileName, ARCFileInfo* fileInfo);
BOOL ARCFastOpen(ARCHandle* handle, s32 entrynum, ARCFileInfo* fileInfo);
s32 ARCConvertPathToEntrynum(ARCHandle* handle, const char* pathPtr);
/* untyped: caller-owned file data */
void* ARCGetStartAddrInMem(ARCFileInfo* fileInfo);
u32 ARCGetLength(ARCFileInfo* fileInfo);
BOOL ARCClose(ARCFileInfo* fileInfo);
BOOL ARCChangeDir(ARCHandle* handle, const char* dirName);
BOOL ARCOpenDir(ARCHandle* handle, const char* dirName, ARCDir* dir);
BOOL ARCReadDir(ARCDir* dir, ARCDirEntry* dirent);
BOOL ARCCloseDir(ARCDir* dir);

#ifdef __cplusplus
}
#endif

#endif
